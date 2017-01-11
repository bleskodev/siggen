/* swgen.c
 * sweep generator and more.
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
 * 09Mar03  Bug fixed in calculating playsamples for long periods
 * 16Feb03  Add -A option for digital gain - as per sgen
 *  3Jun99  Some tidying up of WAV file writing
 *  1Sep98  added configuration file(s) support
 * 18Jun98  added the x10 feature where frequencies are specified
 *          as integers 10 times too big, so that freqs with a resolution
 *          of 0.1 Hz can be generated.
 * 18Jun98   Made compilable on machines without soundcard support and
 *           under sunos (bigendian machine)
 * 04Aug97   Add -t time playing option.
 * 24Mar97   Added a specialist stereo option. It outputs the FM waveform
 *           on one channel and the sweep on the second channel. 
 *           The FM signal can be used to trigger the scope - or even
 *           drive the scope timebase used to monitor the swept freq.
 *           in freq response measurements.
 * 17Mar97   initial version based on tones.c as a skeleton.
 *           generates a 1Hz wave for the basic waveform and a standard
 *           wave of correct frequency for the 
 *           FM waveform. For unmodulated freq F, sample the basic buffer
 *           every F samples, wrapping approp. from end to start. 
 *           To FM modulate it use the sample values from the FM waveform
 *           to determine the (now variable) increment F.
 *           The default is to generate a sawtooth ramp wave for the FM
 *           and a sine wave as the basic, swept, waveform.
 * 
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
#include "fmtheaders.h"
#include "config.h"

#define VERSTR "%s  Ver. %s   Digital Sweep Generator\n"

#define EFAIL -1

#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

int aflg,fflg,vflg,dflg,wfmt;

unsigned int Gain;               /* Amplification factor */
unsigned int stereo;             /* stereo or mono mode      */
unsigned int freqX;              /* multiplication factor for freq. spec. */
                                 /* actual freq. is given freq / freqX   */
unsigned int samplerate;         /* Samples/sec              */
unsigned int afmt;               /* format for DAC  */
unsigned int abuf_size;          /* audio buffer size */
int frag_size;                   /* size of a DAC buffer  */

char *waveform;                  /* Waveform to generate for tones    */
unsigned char *swbuf;            /* sample buff for swept waveform */
unsigned int Frq,Frqmin,Frqmax;  /* Mid point freq of sweep and min and max */
char *FMwave;                    /* modulation waveform               */
unsigned char *FMbuf;            /* sample buff for sweep waveform */
unsigned char *FM8buf;
unsigned int FMfrq;              /* frequency of modulation waveform   */

unsigned int tbuf_size;          /* tone buffer size */
unsigned int plbuf_size;         /* play buffer size */
unsigned char *silbuf;           /* silence buffer   */
unsigned char *tbuf,*plbuf;      /* tbuf holds the sweep samples for 1 sec
				  * plbuf is tbuf if in mono mode
				  * else is the interleaved 2 channels 
				  */
long playtime,playsamples;       /* number of millisecs/samples to play
                                    0 means play for ever                  */

char *sys,*outfile;
char dac[130];
char *configfile;

unsigned char *maketone();

help(e)
int e;
{  
   char **aa;

   fprintf(stderr,VERSTR,sys,VERSION);
   fputs("\nGenerates a swept waveform. Both the swept waveform and the sweeping\n",stderr);
   fputs("waveform can be:",stderr);
   for ( aa=(char **)getWavNames(); *aa; aa++ ) fprintf(stderr," %s",*aa);
   fputs("\n",stderr);
   fputs("The sweep range can be specified as a min. freq and max. freq, or as\n",stderr);
   fputs("centre freq and percentage variation. All frequencies are Hertz\n",stderr);
   fputs("and are integers (but see the -x10 and -x100 options).\n\n",stderr);
   fputs("Usage: swgen [options/flags] [sweepwaveform] sweepfreq\n",stderr);
   fputs("         [sweptwaveform] minfreq maxfreq | centrefreq percent%\n",stderr);
   fputs("Defaults: sweeping waveform is sawtooth, swept waveform is sine,\n",stderr);

#ifdef HAVE_DAC_DEVICE
   fprintf(stderr,"       output to %s, %d 16 bit mono samples/sec, if possible,\n",
	          DAC_FILE,SAMPLERATE);
   fputs("       else 8 bit samples.\n",stderr);
#else
   fprintf(stderr,"       %d 16 bit mono samples/sec. Must be used with -o or -w option.\n",
	          SAMPLERATE);
#endif

   fputs("flags: -f,-a         force overwrite/append of/to file\n",stderr);
   fputs("       -o file       write digital samples to file ('-' is stdout)\n",stderr);
   fputs("       -w file       as '-o' but written as a wave file\n",stderr);
   fputs("       -s samples    generate with samplerate of samples/sec\n",stderr);
   fputs("       -v            be verbose.\n",stderr);
   fputs(" -8/-16 or -b 8|16   force 8 bit or 16 bit mode.\n",stderr);
   fputs("       -2            special stereo mode, swept waveform to one channel\n",stderr);
   fputs("                     sweeping waveform to other channel.\n",stderr);
   fputs("       -A n          scale samples by n/100, def. n is 100\n",stderr);
   fputs("       -t N|Nm       N secs or Nm millisecs of samples\n",stderr);
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
   unsigned int i,j,k,l,m,n,N;
   FILE *f;
   char *p,*wf,*fnm,fname[130],*omode;
   int fd,st;
   unsigned int t;
   
   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;
   
   configfile=DEF_CONF_FILENAME;
   waveform="sine"; FMwave="sawtooth";
   outfile=NULL;
   samplerate=0; afmt=AFMT_QUERY;
   stereo=-1;
   Gain=100;
   playtime=playsamples=dflg=vflg=aflg=fflg=0;
   freqX=1;
   
   while (argc && **argv=='-') {          /* all flags and options must come */
      n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (chkword(1,"samplerate")) {
	 if (argc && isdigit(**argv)) { samplerate=atoi(*argv++); argc--; }
      }
      else if (chkword(4,"x100")) { freqX=100; }
      else if (chkword(3,"x10")) { freqX=10; }
      else if (chkword(1,"output")) {     /* specify output file       */
	 if (argc) { outfile=*argv++; argc--; } /* output file name          */
      }
      else if (chkword(1,"wave")) {     /* e.g. write WAVE format file  */
	 if (argc) { outfile=*argv++; argc--; wfmt=1; } /* output file name  */
      }
      else if (chkword(1,"A")) {
	 if (argc && isdigit(**argv)) { 
	    Gain=atoi(*argv++); argc--;
	 }
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
      else if (chkword(1,"config")) {
	 if (argc && **argv != '-') {
	    configfile=*argv++; argc--;
	 }
      }
      else {                              /* check for single char. flags    */
	 for (; *p; p++) {
	    if (*p=='h') exit(help(EFAIL));
	    else if (*p=='8') afmt=AFMT_U8;
	    else if (*p=='1') stereo=0;
	    else if (*p=='2') stereo=1;
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
 
  /* check sweep params ........ */
  
   if (argc && !isdigit(**argv)) {   /* if first param is not numeric */
      FMwave=*argv++; argc--;        /* must be the modulating waveform type */
   }
   if (argc==0 || !isdigit(**argv)) { /* next param must be sweep frequency */
     exit(help(err_rpt(EINVAL,"Invalid sweep frequency.")));
   }
   FMfrq=atoi(*argv++); argc--;
   if (argc && !isdigit(**argv)) {   /* if next param is not numeric  */
      waveform=*argv++; argc--;      /* it must be swept waveform type */
   }
   if (argc!=2) {                    /* should be exactly 2 params left */
     exit(help(err_rpt(EINVAL,"Invalid number of parameters.")));
   }
   Frq=atoi(*argv++); argc--;
   n=strtol(*argv,&p,10);
   if (*p=='%') {
      if (n>100)
	exit(help(err_rpt(EINVAL,"Percent variation must be <= 100%")));
      Frqmin=Frq-((Frq*n+50)/100);
      Frqmax=Frq+((Frq*n+50)/100);
   } else {
      Frqmin=Frq; Frqmax=n; Frq=(Frqmin+Frqmax+1)/2;
      if (Frqmin>Frqmax) {
	 n=Frqmin; Frqmin=Frqmax; Frqmax=n;
      }
   }
   if (Frqmax==0 || Frqmax>=((freqX*samplerate)/2)) {
      exit(help(err_rpt(EINVAL,"Specified Max freq. is either 0 or greater than half the samplerate")));
   }
      
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
	 printf("%d %s, %s, samples/sec\n",samplerate,
                (stereo)?"stereo":"mono",
		(i==AFMT_U8)?"unsigned 8 bit":
		((i==AFMT_S16_LE)?"signed 16 bit, little endian":
		 "Unknown audio format"));
         printf("%d bytes per DAC buffer.\n",frag_size);
      }
      if (i!=afmt || stereo!=n) {
	 exit(err_rpt(EINVAL,"Sound card doesn't support requested format."));
      }
#else
                                 /* error if no outfile specified */
      exit(help(err_rpt(EINVAL,"No output file specified, use -w or -o option.")));
#endif
   } else { 
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
	 printf("%s : Opened %s.\n",fnm,
		(*omode=='a')?"in append mode":"clean for writing");
      }
      fd=fileno(f);
      if (playtime==0) playtime=DEF_PLAYTIME;
   }
                                          /* calc number of samples to play */
   if (playtime) {
     if ((playsamples=mstosamples(playtime,samplerate))==0) 
       exit(err_rpt(EINVAL,"Arithmetic overflow when calculating number of samples."));
   }
   
   tbuf_size=samplerate*freqX;            /* buf sizes if 8 bit samples */
   if (afmt==AFMT_S16_LE) tbuf_size<<=1;  /* double size if 16 bit      */
   else if (afmt!=AFMT_U8) {
      exit(err_rpt(EINVAL,"Only unsigned 8 and signed 16 bit, supported."));
   }
   
   if (vflg) {
      printf("%s bit samples being generated.\n",
	     (afmt==AFMT_S16_LE)?"16":"8");
      printf("%d byte buffers being allocated.\n",tbuf_size);
      if (freqX>1) printf("All frequency values to be divided down by %d\n",freqX);
      printf("%s wave being swept from %d Hz to %d Hz by\n",
	     waveform,Frqmin,Frqmax);
      printf("a sweeping %s wave at %d Hz.\n",FMwave,FMfrq);
      if (playtime) {
	 printf("Playing for %d.%03d millisecs, %d samples.\n",
		playtime/1000,playtime%1000,playsamples);
      }
      printf("Samples scaled by a factor of %d/100.\n",Gain);
   }

   if ((swbuf=maketone(waveform,1,samplerate*freqX,Gain,afmt))==NULL || 
       (FMbuf=maketone(FMwave,FMfrq,samplerate*freqX,100,AFMT_S16_LE))==NULL) {
      exit(err_rpt(ENOMEM,"Out of Memory or Cannot Generate Waveforms."));
   }
   if ((tbuf=malloc(tbuf_size))==NULL ||
       mksweep(tbuf,tbuf_size,(short int *)FMbuf,swbuf,Frqmin,Frqmax,samplerate*freqX,afmt)==0) {
     exit(err_rpt(EINVAL,"Out of Memory or Problem generating Swept waveform."));
   }

   if (stereo) {
      plbuf_size=tbuf_size<<stereo;
      if ((plbuf=malloc(plbuf_size))==NULL)
	exit(err_rpt(ENOMEM,"Cannot allocate memory!"));
      if (afmt==AFMT_U8) {
	 if ((FM8buf=malloc(tbuf_size))==NULL)
	   exit(err_rpt(ENOMEM,"Cannot allocate memory!"));
	 mk8bit(FM8buf,FMbuf,samplerate*freqX);
         mixplaybuf(plbuf,tbuf,FM8buf,samplerate*freqX,afmt);
      } else {
         mixplaybuf(plbuf,tbuf,FMbuf,samplerate*freqX,afmt);
      }
      if (vflg) {
	 printf("Stereo mode - swept waveform one channel 1\n");
	 printf("              sweeping waveform on other channel\n");
      }
   } else {
      plbuf=tbuf; plbuf_size=tbuf_size;
      if (vflg) printf("Mono mode - swept waveform only....\n");
   }

   i=vflg; vflg=0;
   if (playtime) {
      silbuf=(unsigned char *)malloc(plbuf_size);
      if (silbuf==NULL) exit(err_rpt(ENOMEM,"Attempting to get buffers"));
      mknull(silbuf,plbuf_size,0,0,samplerate*freqX,0,afmt);
   }
   vflg=i;

   if (outfile==NULL) { 
      if (playtime) {
	 n=(playsamples<<stereo)<<(afmt==AFMT_S16_LE);
	 st=writecyclic(fd,plbuf,plbuf_size,n);
	 writecyclic(fd,silbuf,plbuf_size,frag_size-(n%frag_size));
	 close(fd);
      } else {
         playloop(fd,plbuf,plbuf_size,frag_size);
      }
   } else {
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

   exit(0);
}  

