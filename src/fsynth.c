/* fsynth.c
 * Ncurses based fourier waveform synthesiser
 *
 * Linux Version
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson                    jj@franjam.org.uk
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program - see the file COPYING; if not, write to 
 *  the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
 *  MA 02139, USA.
 */

/*
 * fsynth :
 * 
 *  1Hz sample buffers are created.
 *  A waveform is built up dymanically, by adding into the
 *  fundamental Freq F the specified amounts of the various harmonics N*F
 *  The generation method is to sample the 1Hz samples at intervals
 *  of the required frequencies.
 * 
 * History:
 *  26Oct98 V1.0   Built by modifying the siggen.c and sigscr.c
 *                 to give the basic framework
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <math.h>
#include "fsynth.h"

#define MAXPRM 32
#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

int vflg,dflg,vikeys;

int DAC;                         /* File Handle for DSP */
unsigned int samplerate;                  /* Samples/sec        */
unsigned int afmt;                        /* format for DSP  */
unsigned int stereo;
int Bufspersec;                  /* number of sounds fragments per second */
int Nfragbufs;                   /* number of driver frag buffers */
int fragsize;                    /* size of driver buffer fragments */
int fragsamplesize;              /* size of fragments in samples */

    /* Fundamental Freq details */
char wf[32]="sine";              /* waveform type */
unsigned int freq=440;           /* signal frequency */

int channels;                    /* number of generating channels */

char *sys;
char *configfile;
char dac[130];

help(e)
int e;
{  
   char **aa;

   fprintf(stderr,VERSTR,sys,VERSION);
   fputs("\nUsage: \n 1: fsynth [flags] [waveform [freq]]]\n",stderr);

#ifdef HAVE_DAC_DEVICE
   fprintf(stderr,"Defaults: SINE wave, %d harmonics, output to %s, %d samples/sec,\n",
	   DEF_FSYNTH_CHANNELS,DAC_FILE,SAMPLERATE);
   fputs("         16 bit mono samples if possible, else 8 bit.\n",stderr);
#else
   fprintf(stderr,"Defaults: SINE wave, %d harmonics, %d samples/sec,\n",
	   DEF_FSYNTH_CHANNELS,SAMPLERATE);
   fputs("         16 bit mono samples. Must be used with -o or -w option.\n",stderr);
#endif

   fputs("Valid waveforms are:",stderr);
   for ( aa=(char **)getWavNames(); *aa; aa++ ) fprintf(stderr," %s",*aa);

   fputs("\nflags: -s samples    generate with samplerate of samples/sec\n",stderr);
   fputs(" -8/-16 or -b 8|16   force 8 bit or 16 bit mode.\n",stderr);
   fputs("       -c channels   number of harmonics or channels\n",stderr);
   fputs("       -C file       use file as local configuration file\n",stderr);
   fputs("      -NB n          Numer of Buffers to create is n, def. is 3\n",stderr);
   fputs("     -BPS n          Number of Buffers to play per sec, def. is 15\n",stderr);
   return(e);
}

/* main
 *
 */
 
main(argc,argv)
int argc;
char **argv;
{
   unsigned int v[MAXPRM],maxv,i,j,k,l,m,n,N;
   FILE *f;
   char *p,bf[130];
   int c;
   unsigned int t;
   
   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;

   configfile=DEF_CONF_FILENAME;
   samplerate=0; afmt=AFMT_QUERY;
   Nfragbufs=0;
   Bufspersec=0;
   vflg=dflg=0;
   channels=0;
   stereo=0;
  
   while (argc && **argv=='-') {          /* all flags and options must come */
      n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (chkword(1,"samplerate")) {
	 if (argc && isdigit(**argv)) { samplerate=atoi(*argv++); argc--; }
      }
      else if (chkword(2,"NB")) {
	 if (argc && isdigit(**argv)) { Nfragbufs=atoi(*argv++); argc--; }
      }
      else if (chkword(3,"BPS")) { /* Buffers played per second - defines size */
	 if (argc && isdigit(**argv)) { Bufspersec=atoi(*argv++); argc--; }
      }
      else if (chkword(2,"16")) { afmt=AFMT_S16_LE; }
      else if (chkword(1,"bits")) {
	 i=0;
	 if (argc) { 
	    i=atoi(*argv++); argc--;
	 }
	 if (i==8) afmt=AFMT_U8;
	 else if (i==16) afmt=AFMT_S16_LE;
	 else exit(err_rpt(EINVAL,"must be '-b 8' or '-b 16'."));
      }
      else if (chkword(1,"Config")) {
	 if (argc && **argv != '-') {
	    configfile=*argv++; argc--;
	 }
      }
      else if (chkword(1,"channels")) {
	 i=0;
	 if (argc) { 
	    i=atoi(*argv++); argc--;
	 }
	 if (i<2) exit(err_rpt(EINVAL,"Must have 2 or more harmonics (channels)."));
	 channels=i;
      }
      else {                              /* check for single char. flags    */
	 for (; *p; p++) {
	    if (*p=='h') exit(help(EFAIL));
	    else if (*p=='8') afmt=AFMT_U8;
	    else if (*p=='d') dflg=1;
	    else if (*p=='v') vflg++;
	    else {
	       *bf='-'; *(bf+1)=*p; *(bf+2)=0;
	       exit(help(err_rpt(EINVAL,bf)));
	    }
	 }
      }
   }
   
  /* interrogate config file....... */

   init_conf_files(configfile,DEF_CONF_FILENAME,DEF_GLOBAL_CONF_FILE,vflg);
   if (vflg==0) {
      vflg=atoi(get_conf_value(sys,"verbose","0"));
   }
   if (samplerate==0) {
      samplerate=atoi(get_conf_value(sys,"samplerate",QSAMPLERATE));
   }
   if (channels==0) {
      channels=atoi(get_conf_value(sys,"channels",QDEF_FSYNTH_CHANNELS));
   }
   if (Nfragbufs==0) {
     Nfragbufs=atoi(get_conf_value(sys,"fragments",QDEFAULT_FRAGMENTS));
   }
   if (Bufspersec==0) {
     Bufspersec=atoi(get_conf_value(sys,"buffspersec",QDEFAULT_BUFFSPERSEC));
   }
   if (afmt==AFMT_QUERY) {
     afmt=atoi(get_conf_value(sys,"samplesize",QAFMT_QUERY));
   }
   strncpy(dac,get_conf_value(sys,"dacfile",DAC_FILE),sizeof(dac)-1);
   vikeys=atoi(get_conf_value(sys,"vi_keys",QVI_KEYS));
  
/* OK now check is waveform is specified on command line... */
  
   if (argc) {
      strncpy(wf,*argv++,32); wf[31]=0; argc--;    /* waveform type */
      if (argc) {
	 freq=atoi(*argv++); argc--;
	 if (argc) exit(help(err_rpt(EINVAL,"Too many parameters")));
      }
   }
   
   /* if no format specified then try 16 bit */
   i=afmt;
   if ((DAC=DACopen(dac,"w",&samplerate,&i,&stereo))<0) {
      exit(err_rpt(errno,"Opening DSP for output."));
   }
   if ((afmt!=AFMT_QUERY) && (i!=afmt)) {
      exit(err_rpt(EINVAL,"Sound card doesn't support format requested."));
   }
   afmt=i;

   if ((fragsize=setfragsize(DAC,Nfragbufs,Bufspersec,samplerate,afmt,stereo))<0) {
      exit(err_rpt(errno,"Problem setting appropriate fragment size."));
   }
   fragsamplesize=(fragsize>>(afmt==AFMT_S16_LE))>>(stereo);
       
   if (freq > samplerate/2) {
      fprintf(stderr,"%d Hz is more than half the sampling rate\n",freq);
      exit(err_rpt(EINVAL,"Frequency setting too great"));
   }

   if (vflg) {
      printf("Mono %s bit samples being generated.\n",(afmt==AFMT_S16_LE)?"16":"8");
      printf("Playing at %d samples/sec\n",samplerate);
      printf("%d Buffer fragments of %d bytes (%d samples). Aprox. %d millisecs.\n",
	     Nfragbufs,fragsize, fragsamplesize,
	     1000*((fragsize>>(stereo))>>(afmt==AFMT_S16_LE))/samplerate);
      printf("Requested %d buffers/sec and have %d buffs/sec\n",Bufspersec,
	     (samplerate+(fragsamplesize/2))/fragsamplesize);
      printf("\n<Press Return to Continue>\n");
      getchar();
   }

   WinGen(DAC);
   
   close(DAC);
   exit(0);
}  
