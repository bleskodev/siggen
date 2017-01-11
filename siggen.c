/* siggen.c
 * Ncurses based Signal generator, 16/8 bit, 2 channels if stereo card,
 * Emulates usual functions of a lab sig. gen. generating sine, square,
 *  triangle, sawtooth, pulse, noise waveforms, along with loadable
 *  waveforms.
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
 * siggen :
 * 
 * A one second's worth buffer of samples is maintained (samplerate samples)
 * for each channel. These are mixed into a 1 second play buffer
 * that in play mode is played in a circular fashion. By keeping freq.
 * as an integer number of Hertz there is always an exact number of full
 * cycles in 1 sec, so the 1 sec play buffer start and finish match
 * seamlessly. If only mono card then the two channels are mixed into
 * one playing channel.
 * 
 * 
 * History:
 *  2002    V1.6   Added fractional frequency settings, can be setup to
 *                 generate freq. in 1Hz, 0.1Hz or 0.01Hz resolution
 *  1999    V1.5   Added ability for settings for one channel to track the 
 *                 settings on the other channel, e.g. setting different 
 *                 waveforms, and setting tracking in one of the freq fields
 *                 allows simultaneous changing of the freqs
 *                 of both channels. 't' or 'T' toggles tracking on/off. 
 *                 Also added a feature to "fix" or "lock" certain fields
 *                 The values in these locked fields cannot be changed and
 *                 are skipped over when tabbing from field to field. 
 *                 Pressing 'l', 'L' or '!' while in a field locks it. 
 *                 'u' or 'U' unlocks all fields, so that the values can
 *                 be changed. If VI keys are enabled only '!' locks.
 *                 Check config.h
 *  20May97 V1.4   Tidied up generation, and added phase offset for
 *                 second channel with respect to first channel.
 *                 Added 'R' key to resync the two channels, in case
 *                 phase was off because of playing different frequencies
 *  13May97 V1.3   Changed IO library in order to action single key
 *                 presses on fields, so that we can generate sound
 *                 all the time and key actions take 'immediate' effect.
 *                 i.e. continuous playing. Generation changed to sampling
 *                 every F'th sample from a 1Hz set of samples to create
 *                 samples for a frequency of F Hz.
 *  15Mar97 V1.2   Pulse not working - fixed by changing default ratio and
 *                 ratio2 from 0 to -1, forces generator() to pick up def. value
 *  --Feb97 V1.1   Various screen changes - help line, some tweeking
 *                 of ncfio key handling etc.
 *  --Jan97 V1.0   2 channels - if stereo support, then each chan.
 *                 is fed to a seperate stereo output. If only mono
 *                 is possible then the two channels are mixed into the
 *                 one output.
 *                 While entering details of function required
 *                 the program ceases playing any previously generated
 *                 samples.
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
#include "siggen.h"

#define MAXPRM 32
#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

int vflg,dflg,vikeys;

int resolution;                  /* Freq. resolution for signal generation 
				  * 1 for 1Hz res ; 10 for 0.1Hz res; 
				  * 100 for 0.01Hz res                   */
int DAC;                         /* File Handle for DSP */
unsigned int samplerate;         /* Samples/sec        */
unsigned int stereo;             /* stereo mono */
unsigned int afmt;               /* format for DSP  */
int Bufspersec;                  /* number of sounds fragments per second */
int Nfragbufs;                   /* number of driver frag buffers */
int fragsize;                    /* size of driver buffer fragments */
int fragsamplesize;              /* size of fragments in samples */
int LWn;                         /* number of specified loadable waveforms */
char **LWaa;                     /* array of specifed loadable waveforms */

    /* channel 1  ... */
char wf[32]="sine";              /* waveform type */
unsigned int freq=440;           /* signal frequency */
int ratio=-1;                    /* used in pulse, sweep etc */
int Gain=1000;                   /* Amplification factor 
				    Samples are scaled by Gain/1000 */

    /* channel 2  ... */
char wf2[32]="off";              /* waveform type */
unsigned int freq2=440;          /* signal frequency */
int ratio2=-1;                   /* used in pulse, sweep etc */
int Gain2=1000;                  /* Amplification factor see above */
int phase=0;                     /* phase difference of chan2 from chan1 */

char *sys;
char *configfile;
char dac[130];

help(e)
int e;
{ 
  char **aa;
  
  fprintf(stderr,VERSTR,sys,VERSION);
  fputs("\nUsage: siggen [flags] [waveform [freq [param]]]\n",stderr);

#ifdef HAVE_DAC_DEVICE
  fprintf(stderr,"Defaults: SINE wave, output to %s, %d samples/sec,\n",
	  DAC_FILE,SAMPLERATE);
  fputs("         16 bit stereo samples if possible, else 8 bit and/or mono.\n",stderr);
#else
  fprintf(stderr,"Defaults: SINE wave, %d samples/sec,\n",SAMPLERATE);
  fputs("         16 bit mono samples. Must be used with -o or -w option.\n",stderr);
#endif

  fputs("Valid waveforms are:",stderr);
  for ( aa=(char **)getWavNames(); *aa; aa++ ) fprintf(stderr," %s",*aa);
  fputs("\n",stderr);
  fprintf(stderr,"Default Config files are \"%s\", \"$HOME/%s\" and\n\"%s\", searched in that order.\n",
	  DEF_CONF_FILENAME, DEF_CONF_FILENAME, DEF_GLOBAL_CONF_FILE);
	  
  fputs("flags: -s samples    generate with samplerate of samples/sec\n",stderr);
  fputs(" -8/-16 or -b 8|16   force 8 bit or 16 bit mode.\n",stderr);
  fputs("       -1,-2         force mono or stereo (1 or 2 channels)\n",stderr);
  fputs("      -NB n          Number of Buffers to create is n, def. is 3\n",stderr);
  fputs("     -BPS n          Number of Buffers to play per sec, def. is 15\n",stderr);
/*  fputs("    -load wave_file  load waveform from wav_file\n",stderr);*/
  fputs("     -res n          n=1, 0.1 or 0.01  - frequency resolution\n",stderr);
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
   double d;
   FILE *f; 
   char *p,bf[130];
   int c;
   unsigned int t;
   
   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;
   
   configfile=DEF_CONF_FILENAME;
   samplerate=0; afmt=AFMT_QUERY;
   LWn=0; LWaa=argv;
   Nfragbufs=0;
   Bufspersec=0;
   vflg=dflg=0;
   stereo=-1;
   resolution=0;
  
   while (argc && **argv=='-') {          /* all flags and options must come */
      n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (chkword(1,"samplerate")) {
	 if (argc && isdigit(**argv)) { samplerate=atoi(*argv++); argc--; }
      }
      else if (chkword(2,"NB")) {   /* Number of buffer fragments */
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
      else if (chkword(1,"A")) {
	 if (argc && isdigit(**argv)) { 
	    Gain=atoi(*argv++)*10; argc--;
	 }
      }
      else if (chkword(3,"load")) {     /* load a waveform */
	if (argc) { LWaa[LWn++]=*argv++; argc--; } /* waveform file name */
      }
      else if (chkword(3,"resolution")) {  /* set freq generation resolution */
	if (argc) { 
	  if ((resolution=set_resolution(*argv++))==0) { 
	    exit(err_rpt(EINVAL,"bad resolution value, must be 1, 0.1 or 0.01"));
	  } 
	  argc--;
	}
      }
      else {                              /* check for single char. flags    */
	 for (; *p; p++) {
	    if (*p=='h') exit(help(EFAIL));
	    else if (*p=='1') stereo=0;
	    else if (*p=='2') stereo=1;
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
   if (stereo==-1) {
      stereo=atoi(get_conf_value(sys,"channels","1"));
      if (stereo) stereo--;
   }
   if (Nfragbufs==0) {
     Nfragbufs=atoi(get_conf_value(sys,"fragments",QDEFAULT_FRAGMENTS));
   }
   if (resolution==0) {
     resolution=set_resolution(get_conf_value(sys,"resolution",DEFAULT_RESOLUTION));
     if (resolution==0) {
       exit(err_rpt(EINVAL,"bad resolution value, must be 1, 0.1 or 0.01"));
     }
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
	 freq2=freq=atoi(*argv++); argc--;
	 if (argc) { ratio=atoi(*argv++); argc--; }
	 if (argc) exit(help(err_rpt(EINVAL,"Too many parameters")));
      }
   }
   
   /* if no format specified then try 16 bit */
   i=afmt;
   n=stereo;
   if ((DAC=DACopen(dac,"w",&samplerate,&i,&n))<0) {
      exit(err_rpt(errno,dac));
   }
   if (((afmt!=AFMT_QUERY) && (i!=afmt)) ||
       ((stereo!=-1) && (n!=stereo))) {
      exit(err_rpt(EINVAL,"Sound card doesn't support format requested."));
   }
   afmt=i; stereo=n;

   if ((fragsize=setfragsize(DAC,Nfragbufs,Bufspersec,samplerate,afmt,stereo))<0) {
      exit(err_rpt(errno,"Problem setting appropriate fragment size."));
   }
   fragsamplesize=(fragsize>>(afmt==AFMT_S16_LE))>>(stereo);
       
   if (freq > samplerate/2) {
      fprintf(stderr,"%d Hz is more than half the sampling rate!!!\n",freq);
   }

   if (vflg) {
      printf("%s %s bit samples being generated.\n",
	     (stereo)?"Stereo, ":"Mono, ",(afmt==AFMT_S16_LE)?"16":"8");
      printf("Samples amplified by a factor of %d/1000\n",Gain);
      printf("Playing at %d samples/sec\n",samplerate);
      printf("Frequency resolution is %.2fHz\n",1.0/resolution);
      printf("Initial Frequencies: ch1 %dHz  ch2 %dHz\n",freq,freq2);
      printf("ch2 at a phase difference of %d degrees to ch1\n",phase);
      printf("Buffer fragment size is %d bytes (%d samples). Aprox. %d millisecs.\n",
	     fragsize, fragsamplesize,
	     1000*((fragsize>>(stereo))>>(afmt==AFMT_S16_LE))/samplerate);
      printf("Requested %d buffers/sec and have %d buffs/sec\n",Bufspersec,
	     (samplerate+(fragsamplesize/2))/fragsamplesize);
      printf("%d loadable waveforms specified.\n",LWn);
   }

   WinGen(DAC);
   
   close(DAC);
   exit(0);
}  

set_resolution(p)
char *p;
{
  char *s;
  int r;
  double d;
  
  d=strtod(p,&s); r=0;
  if (d==1.0) r=1;
  else if (d==0.1) r=10;
  else if (d==0.01) r=100;
  return(r);
}
