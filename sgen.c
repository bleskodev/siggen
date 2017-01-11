/* sgen.c
 * makes a raw 8 or 16 bit digital sound signal of a waveform of a particular
 * frequency and sampling rate. Output is to a file, or /dev/dsp (linux)
 * Jim Jackson     Linux Version
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson           jj@franjam.org.uk
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

/* History...................
 * 
 * Based on my noddy DOS version which created sample files to be played
 *   by other utils. This had lotsa faults, now fixed.
 * 
 * 09Mar03  Bug fixed in calculating playsamples for long periods
 *  3Jun99  Some tidying up of WAV file writing
 *  1Sep98  added configuration file(s) support
 * 18Jun98  added the x10 feature where frequencies are specified
 *          as integers 10 times too big, so that freqs with a resolution
 *          of 0.1 Hz can be generated.
 * 18Jun98  added the define NO_DAC_DEVICE
 *          if true it compiles to a raw/wav file creator only.
 * 04Aug97  Added -t option to specify time to play 
 * 18Mar97  -f flag was missing - added it back.
 *          Added default sine waveform if none specified on line.
 * 15Mar97  Bug in initialising second channel to silence - it re-init first
 *          channel again. Reported by Michael Meifert <mime@dk3hg.ampr.org>
 * 10Feb97  Added -w option to write out samples as a wave file
 *   ---    for signal generation notes etc see generator.c
 * 29Dec96  Added amplitude factor -A N where N is a percentage.
 *          The sample is created to optimally fill the sample space when
 *          N is 100 (the default value). The samples generated are scaled
 *          by N/100, overly large values simply being 'clipped'.
 *          To create a trapezoid wave form, generate a triangle wave
 *          with N>100, depending on slope needed on waveform.
 * 18Dec96  Started splitting up stuff into different files so that I can 
 *          write some different front ends. All the code to create the 
 *          samples is in generator.c, and some misc. stuff in misc.c
 * --Dec96  Added noise generator using the random function - sounds ok
 *          Need to figure what to do to get pink noise - I think!
 * --Oct96  Original Linux version. Fixed faulty sample generation
 *          in DOS program - you have to generate samples over several
 *          cycles to ensure that a you can put samples end-to-end
 *          to fill a second, see comments in mksin(). Added stereo
 *          and antiphase stuff. Eventually worked out how to generate
 *          square/pulse signals accurately, and sawtooth. 
 *          Triangle still to do.
 */

/* In mono only one buffer is used to hold one secs worth of samples
 *   because we only allow integers to specify the frequency, we always
 *   have an exact number of full cycles in one sec, therefore this buffer
 *   can be treated cyclically to generate the signal for any period.
 * 
 * In stereo, two buffers hold the signal for each channel - 2nd channel is
 *   either an in phase or antiphase copy of channel 1 at moment - and they 
 *   are mixed into a large double size playing buffer.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "config.h"

#define VERSTR "%s  Ver. %s   Digital Signal Generator\n"

#define EFAIL -1

#define MAXPRM 32
#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

int aflg,fflg,vflg,dflg,wfmt;

unsigned int samplerate,freq;    /* Samples/sec and signal frequency */
unsigned int freqX;              /* generate samples at samplerate*freqX */
unsigned int ratio;              /* used in pulse, sweep etc */
unsigned int antiphase;          /* 2nd channel in antiphase to 1st channel */
unsigned int Gain;               /* Amplification factor */
int stereo;                      /* stereo mono */
unsigned int afmt;               /* format for DAC  */
unsigned int frag_size;          /* size of DAC buffer fragments */

int lbuf_flag,rbuf_flag;       
unsigned char *lbuf,*rbuf;       /* waveform bufs, left and right channels */
unsigned int lbuf_size;          /* size of 1 sec, 1 chan, waveform buffer */
unsigned char *plbuf;            /* play buffer - mixed left and right bufs */
unsigned char *silbuf;           /* silence buffer - used for timed output */
int plbuf_size;                  /* size of playback buffer */
long playtime,playsamples;       /* number of millisecs/samples to play
                                    0 means play for ever                  */
char *sys,*outfile;
char dac[130];
char *configfile;

help(e)
int e;
{
   char **aa;

   fprintf(stderr,VERSTR,sys,VERSION);
   fputs("\nUsage: \n 1: sgen [flags] waveform freq\n",stderr);
   fputs("      waveform is",stderr);
   for ( aa=(char **)getWavNames(); *aa; aa++ ) fprintf(stderr," %s",*aa);
   fputs("\n",stderr);
   fputs(" 2: sgen [flags] sin|cos freq [phase]\n",stderr);
   fputs("      sin/cos has extra phase param (def. is 0 degrees)\n",stderr);
   fputs(" 3: sgen [flags] pulse freq [Mark/Space]\n",stderr);
   fputs("      pulse has extra param Mark/Space % - def. is 10 (%)\n",stderr);

#ifdef HAVE_DAC_DEVICE
   fprintf(stderr,"Defaults: output continuously to %s, %d samples/sec, mono,\n",
                  DAC_FILE,SAMPLERATE);
   fputs("          16 bit samples if possible, else 8 bit.\n",stderr);
#else
   fputs("An output file MUST be specified with either the -o or -w option.\n",stderr);
   fprintf(stderr,"Defaults: 1 second's worth of %d samples/sec, mono, 16 bit samples.\n",SAMPLERATE);
#endif
   fprintf(stderr,"Default Config files are \"%s\", \"$(HOME)/%s\" and\n\"%s\", searched in that order.\n",
	  DEF_CONF_FILENAME, DEF_CONF_FILENAME, DEF_GLOBAL_CONF_FILE);

   fputs("flags: -f,-a         force overwrite/append of/to file\n",stderr);
   fputs("       -o file       write digital sample to file ('-' is stdout)\n",stderr);
   fputs("       -w file       as '-o' but written as a wave file\n",stderr);
   fputs("       -C file       use file as local configuration file\n",stderr);
   fputs("       -s samples    generate with samplerate of samples/sec\n",stderr);
   fputs("       -v            be verbose.\n",stderr);
   fputs(" -8/-16 or -b 8|16   force 8 bit or 16 bit mode.\n",stderr);
   fputs("       -1,-2a        mono (def) or stereo in antiphase\n",stderr);
   fputs("       -A n          scale samples by n/100, def. n is 100\n",stderr);
   fputs("       -t N|Nm       play for N secs or Nm millisecs\n",stderr);
   fputs("       -x10|-x100    scale freqs down by factor of 10/100\n",stderr);
   fputs("                     allows freqs to 0.1/0.01 of a hertz.\n",stderr);
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
   char *p,*wf,*fnm,fname[130],*omode;
   int fd,st;
   unsigned int t;
   
   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;
   
   configfile=DEF_CONF_FILENAME;
   outfile=NULL;
   samplerate=0; afmt=AFMT_QUERY;
   stereo=-1;
   antiphase=wfmt=0; Gain=100;
   playtime=playsamples=dflg=vflg=aflg=fflg=0;
   freqX=1;
   
   while (argc && **argv=='-') {          /* all flags and options must come */
      n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (chkword(1,"samplerate")) {
	 if (argc && isdigit(**argv)) { samplerate=atoi(*argv++); argc--; }
      }
      else if (chkword(4,"x100")) { freqX=100; }
      else if (chkword(3,"x10")) { freqX=10; }
      else if (chkword(1,"output")) {     /* e.g. string option              */
	 if (argc) { outfile=*argv++; argc--; } /* output file name          */
      }
      else if (chkword(1,"w")) {     /* e.g. string option              */
	 if (argc) { outfile=*argv++; argc--; wfmt=1; } /* output file name          */
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
      else if (chkword(1,"time")) {
	 i=0;
	 if (argc) { 
	    i=strtol(*argv++,&p,10); argc--;
	    if (*p=='s' || *p==0)
	      i*=1000;        /* time specified in secs - *1000 for ms */
	    else if (*p!='m')
	      exit(err_rpt(EINVAL,"Invalid time specification."));
	 }
	 playtime=i;
      }
      else if (chkword(1,"A")) {
	 if (argc && isdigit(**argv)) { 
	    Gain=atoi(*argv++); argc--;
	 }
      }
      else if (chkword(2,"2a")) {
	 stereo=antiphase=1;
      }
      else if (chkword(1,"Config")) {
	 if (argc && **argv != '-') {
	    configfile=*argv++; argc--;
	 }
      }
      else {                              /* check for single char. flags    */
	 for (; *p; p++) {
	    if (*p=='h') exit(help(EFAIL));
	    else if (*p=='1') stereo=0;
	    else if (*p=='2') stereo=1;
	    else if (*p=='8') afmt=AFMT_U8;
	    else if (*p=='a') aflg=1;
	    else if (*p=='f') fflg=1;
	    else if (*p=='d') dflg=1;
	    else if (*p=='v') vflg++;
	    else {
	       *fname='-'; *(fname+1)=*p; *(fname+2)=0;
	       exit(help(err_rpt(EINVAL,fname)));
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
   if (afmt==AFMT_QUERY) {
     afmt=atoi(get_conf_value(sys,"samplesize",QAFMT_QUERY));
   }
   strncpy(dac,get_conf_value(sys,"dacfile",DAC_FILE),sizeof(dac)-1);

  /* check signal params ......... */
  
   if (argc==0) exit(help(err_rpt(EINVAL,"Wrong number of parameters.")));
   wf="sine";
   if (!isdigit(**argv)) {
      wf=*argv++; argc--;    /* waveform type */
   }
   if (argc) {
      freq=atoi(*argv++); argc--;
      ratio=-1;
      if (argc) { ratio=atoi(*argv++); argc--; }
   }
   if (argc) exit(help(err_rpt(EINVAL,"Wrong number of parameters.")));

  /* open the right device or output file............ */
  
   if (vflg) printf(VERSTR,sys,VERSION);

   if (outfile==NULL) {
#ifdef HAVE_DAC_DEVICE
                                  /* if no outfile then write direct to DAC */
                                  /* if no format specified then try 16 bit */
      i=(afmt==AFMT_QUERY)?AFMT_S16_LE:afmt ; 
      n=stereo;
      if ((fd=DACopen(fnm=dac,"w",&samplerate,&i,&n))<0) {
	 if (afmt==AFMT_QUERY) {  /* if no format specified try for 8 bit.. */
	    i=AFMT_U8;
	    fd=DACopen(fnm,"w",&samplerate,&i,&n);
	 }
	 if (fd<0) exit(err_rpt(errno,fnm));
      }
      if ((frag_size=getfragsize(fd))<0)
	exit(err_rpt(errno,"Problem getting DAC Buffer size."));

      afmt=i;
      if (vflg) {
	 printf("%s : DAC Opened for output\n",fnm);
	 printf("%d %s, %s, samples/sec.\n",samplerate,
                (stereo)?"stereo":"mono",
		(i==AFMT_U8)?"unsigned 8 bit":
		((i==AFMT_S16_LE)?"signed 16 bit, little endian":
		 "Unknown audio format"));
         printf("%d bytes per DAC buffer.\n",frag_size);
      }
      if (i!=afmt || n!=stereo) {
	 exit(err_rpt(EINVAL,"Sound card doesn't support format requested."));
      }
#else
                                 /* error if no outfile specified */
      exit(help(err_rpt(EINVAL,"No output file specified, use -w or -o option.")));
#endif
   } else {   /*  outfile!=NULL  so writing to a file  */
      if (aflg && wfmt) {
         exit(err_rpt(EINVAL,"Cannot write append to a WAVE file."));
      }
      afmt=(afmt==AFMT_QUERY)?AFMT_S16_LE:afmt ; /* set 16 bit mode unless
						  * some format is forced */
      omode=(aflg)?"a":"w";
      if ((f=fopen(fnm=outfile,"r"))!=NULL) {
	 fclose(f);
	 if ( (!aflg) && (!fflg) ) exit(err_rpt(EEXIST,fnm));
      }
      if ((f=fopen(fnm,omode))==NULL) { exit(err_rpt(errno,fnm)); }
      if (vflg) { 
	 fputs(fnm,stdout);
	 if (*omode=='a') fputs(": Opened in append mode.\n",stdout);
	 else fputs(": Opened clean for writing.\n",stdout);
      }
      fd=fileno(f);
      if (playtime==0) playtime=DEF_PLAYTIME;
   }
                                          /* calc number of samples to play */
   if (playtime) {
     if ((playsamples=mstosamples(playtime,samplerate))==0) 
       exit(err_rpt(EINVAL,"Arithmetic overflow when calculating number of samples."));
   }
   
   lbuf_size=samplerate*freqX;            /* buf sizes if 8 bit samples */
   if (afmt==AFMT_S16_LE) lbuf_size<<=1;  /* double size if 16 bit      */
   else if (afmt!=AFMT_U8) {
      exit(err_rpt(EINVAL,"Only unsigned 8 and signed 16 bit, supported."));
   }
	
   lbuf_flag=rbuf_flag=0;
   lbuf=(unsigned char *)malloc(lbuf_size);
   rbuf=(unsigned char *)malloc(lbuf_size);
   plbuf=(unsigned char *)malloc(plbuf_size=lbuf_size<<stereo);
   if ((rbuf==NULL) || (lbuf==NULL) || (plbuf==NULL)) {
      exit(err_rpt(ENOMEM,"Attempting to get buffers"));
   }

   i=vflg; vflg=0;
   if (playtime) {
      silbuf=(unsigned char *)malloc(plbuf_size);
      if (silbuf==NULL) exit(err_rpt(ENOMEM,"Attempting to get buffers"));
      mknull(silbuf,plbuf_size,0,0,samplerate*freqX,0,afmt);
   }
   mknull(lbuf,lbuf_size,0,0,samplerate*freqX,0,afmt);
   mknull(rbuf,lbuf_size,0,0,samplerate*freqX,0,afmt);
   vflg=i;
   
   if (vflg) {
      if (freqX>1) printf("All frequency values to be divided down by %d\n",freqX);
      printf("%d Hz, %s bit %s samples being generated.\n",
	     freq,(afmt==AFMT_S16_LE)?"16":"8",(stereo)?"Stereo":"Mono");
      if (playtime) {
	 printf("Playing for %d.%03d millisecs, %d samples.\n",
		playtime/1000,playtime%1000,playsamples);
      }
      printf("Samples scaled by a factor of %d/100.\n",Gain);
      printf("%d byte buffer allocated per channel.\n",lbuf_size);
   }

   if ((freq==0) || (freq >= samplerate*freqX/2)) {
      fprintf(stderr,"Frequency (%dHz) should less than half the sampling rate and not zero.\n",freq);
      exit(err_rpt(EINVAL,"Frequency setting"));
   }

   if (generate(wf,lbuf,lbuf_size,freq,Gain,samplerate*freqX,ratio,afmt)==0) {
      exit(err_rpt(ENOSYS,wf));
   }

   if (stereo) {
      if (antiphase) {
	 do_antiphase(rbuf,lbuf,samplerate*freqX,afmt);
      } else memcpy(rbuf,lbuf,lbuf_size);
      mixplaybuf(plbuf,lbuf,rbuf,samplerate*freqX,afmt);
   } else {      
      memcpy(plbuf,lbuf,plbuf_size);
   }

   if (outfile==NULL) { /* writing to audio device.... */
      if (playtime) {
	n=(playsamples<<stereo)<<(afmt==AFMT_S16_LE);
	st=writecyclic(fd,plbuf,plbuf_size,n);
	writecyclic(fd,silbuf,plbuf_size,frag_size-(n%frag_size));
	close(fd);
      } else {
         playloop(fd,plbuf,plbuf_size,frag_size);
      }
   } else {   /* writing to a file */
      if (playtime==0) { playtime=freqX*1000; playsamples=samplerate*freqX; }
                                            /* freqX secs worth of samples */
      if (wfmt) {
	n=(playsamples<<stereo);
	st=fWAVwrfile(f,plbuf,plbuf_size,n,samplerate,afmt,stereo);
	fclose(f);
      } else {
	n=(playsamples<<stereo)<<(afmt==AFMT_S16_LE);
	st=writecyclic(fd,plbuf,plbuf_size,n);
	close(fd);
      }
      if (st<0) {
         err_rpt(errno,outfile);
      } else if (vflg) {
         printf("%d %s samples (%d.%03d secs) written to %s format file %s\n",
		playsamples,(stereo)?"stereo":"mono",playtime/1000,playtime%1000,
		(wfmt)?"WAVE":"RAW",outfile);
      }
   }
   
   free(lbuf); free(rbuf); free(plbuf); free(silbuf);
   exit(0);
}  
