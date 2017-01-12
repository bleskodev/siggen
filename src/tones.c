/* tones.c
 * generate and play tones for given period....
 * Jim Jackson     Linux Version
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson               jj@franjam.org.uk
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
 * 17Mar06  Fixed Wav writing bug reported by Lukas Loehrer
 *  9Mar06  Fixed raw writing bug reported by Lukas Loehrer
 *   Apr00  added loadable waveforms
 * 18May99  added absolute level mode.
 *  1Sep98  added configuration file(s) support
 * 18Jun98  Modified to compile with no sound card support etc and under SUNOS
 * 20Feb98  Added recognition of musical notes format e.g. G3 for G
 *          in 3rd octave, A2# for A# in second octave. Octave '0'
 *          runs from C at 33Hz to C at 65Hz
 *  9Sep97  Split out the tone spec. parsing and tone playing into 
 *          tonesgen.c
 *  8Sep97  Alter tones specification to be able to specify different
 *          durations. i.e. t1,t2:ms. 
 *          Also start the change of generating technique to do sampling
 *          1Hz sample space for requisite number of samples.
 * --Aug97  Re-organise some of DAC routines out into DAC.c
 * 18Feb97  -f flag missing, added it
 *          also added facility to be able to change wavform. A
 *          non-numeric parameter will cause a change in waveform
 *          for rest of tones.
 *          Also still occasional noise after tones played - altered DSPend again!
 * 16Feb97  altered it so you can specify several freqs to be played together
 *          by using punctuation of some sort to seperate freqs 
 *          e.g. 440,880   would play two freqs of 440Hz and 880Hz together
 * 13Feb97  Based on sgen, using generator.c - only mono version.
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
#include "config.h"

#define VERSTR "%s  Ver. %s   Tone Generator.\n"

#define EFAIL -1

#define MAXPRM 32
#define MAXTONES 64
#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

int aflg,fflg,vflg,dflg,wfmt;
int loopflg;

int Abs_Level ;                  /* if set cancel auto-scale mode */
unsigned int stereo;             /* stereo or mono mode      */
unsigned int samplerate;         /* Samples/sec              */
unsigned int ratio;              /* used in pulse, sweep etc */
unsigned int afmt;               /* format for DSP  */
unsigned int DSPbytecount;       /* number of bytes sent to DSP  */
int frag_size;                   /* size of a DSP buffer  */
int channels;                    /* number of generating channels */
int Wn;                          /* number of allowed loadable waveforms */
int TWn;                         /* Total number of waveforms  */
int LWn;                         /* number of specified loadable waveforms */
char **LWaa;                     /* array of specifed loadable waveforms */

char *sys,*outfile,*infile;
char dac[130];
char *configfile;

extern char *getGenErrStr();

help(e)
int e;
{  
   char **aa;
   
   fprintf(stderr,VERSTR,sys,VERSION);
   fputs("\nUsage: tones [-s samplerate] [-8/-16|-b 8/16] [waveform] :T freq(s)\n\n",stderr);

#ifdef HAVE_DAC_DEVICE
   fprintf(stderr,"Defaults: SINE wave, %d channels, output to %s, %d samples/sec,\n",
	   DEF_TONES_CHANNELS,DAC_FILE,SAMPLERATE);
   fputs("         16 bit mono samples if possible, else 8 bit.\n",stderr);
#else
   fprintf(stderr,"Defaults: SINE wave, %d channels, %d samples/sec,\n",
	   DEF_TONES_CHANNELS,SAMPLERATE);
   fputs("         16 bit mono samples. Must be used with -o or -w option.\n",stderr);
#endif
   fputs("\
Generates T millisecs of each frequency specified. Several freqs can be\n\
played together, e.g. 150,200,300. Relative amplitudes can be specified\n\
in dB e.g. 150@-6,200,300@-12. The channels option gives the maximum number\n\
of freqs. Each channel adds processing overhead to the generation process.\n\
Non-default specific durations maybe given e.g. 440,880:1000\n\
",stderr);
#if FALSE
   fputs("Generates tones of given frequencies for T millisecs each.\n",stderr);
   fputs("Several freqs can be played together by specifying a list of freqs\n",stderr);
   fputs("separated by ',', e.g. 150,200,300. relative amplitudes can be specified\n",stderr);
   fputs("by using the suffix @-dB after the freq.\n",stderr);
   fputs("The maximum number of frequencies which can be played together\n",stderr);
   fputs("is specified by the '-c channels' option. Each channel adds\n",stderr);
   fputs("processing overhead to the generation process.\n",stderr);
   fputs("A specific duration of N millisecs maybe given, overriding the default,\n",stderr);
   fputs("by appending ':N'\n",stderr);
   fputs("e.g. 150,300:250 plays 150Hz and 300Hz together for 250 millisecs\n",stderr);
   fputs("     0:1000 plays one second (1000ms) of silence.\n",stderr);
#endif
   fputs("Valid waveforms are:",stderr);
   for ( aa=(char **)getWavNames(); *aa; aa++ ) fprintf(stderr," %s",*aa);
   fputs(".\nand can be specified anywhere and affect all later tones.\n",stderr);
   fputs("flags: -f,-a         force overwrite/append of/to file\n",stderr);
   fputs("       -o/-w file    write samples to raw/WAVE file ('-' is stderr)\n",stderr);
   fputs("       -i file       contents of file specify tones program ('-' is stdin)\n",stderr);
   fputs("       -load wave_file  load waveform from wav_file\n",stderr);
   fputs("       -s samples    generate with samplerate of samples/sec\n",stderr);
   fputs("       -c channels   number of channels or voices\n",stderr);
   fprintf(stderr,"       -lw N         number of loadable waveforms allowed - def %d\n",DEF_WN);
   fputs("       -loop N       repeat play the tones N times.\n",stderr);
   fputs("       -l            repeat play the tones ad nauseam.\n",stderr);
   fputs("      -v/-vv         be verbose / be very verbose\n",stderr);
   fputs(" -8/-16 or -b 8|16   force 8 bit or 16 bit mode.\n",stderr);
   fputs("    -abs|-rel        set absolute or relative amplitude control.\n",stderr);
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
   FILE *f,*fi;
   char *p,*fnm,bf[260],*omode;
   int fd,st;
   unsigned int t;
   
   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;
   
   configfile=DEF_CONF_FILENAME;
   outfile=infile=NULL;
   samplerate=0; afmt=AFMT_QUERY;
   channels=0; Wn=-1; LWn=0; LWaa=argv;
   stereo=0;
   loopflg=wfmt=frag_size=0;                 /* force mono mode */
   Abs_Level=dflg=vflg=aflg=fflg=0;
   
   while (argc && **argv=='-') {          /* all flags and options must come */
      n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (chkword(1,"samplerate")) {
	 if (argc && isdigit(**argv)) { samplerate=atoi(*argv++); argc--; }
      }
      else if (chkword(1,"output")) {     /* specify output file       */
	 if (argc) { outfile=*argv++; argc--; } /* output file name          */
      }
      else if (chkword(1,"input")) {     /* specify input file       */
	 if (argc) { infile=*argv++; argc--; } /* input file name          */
      }
      else if (chkword(3,"load")) {     /* load a waveform           */
	 if (argc && **argv!='-') { LWaa[LWn++]=*argv++; argc--; } /* waveform file name   */
      }
      else if (chkword(2,"lw")) {
	 i=0;
	 if (argc && **argv!='-') { 
	    i=atoi(*argv++); argc--;
	    Wn=(i>0)?i:0;
	 }
      } 
      else if (chkword(3,"loop")) {       /*  specify loop option   */
	 if (argc && **argv!='-') {
	    loopflg=atoi(*argv++); argc--;
	 }
      }
      else if (chkword(3,"absolute")) {   /*  absolute amplitude levels */
         Abs_Level=1;
      }
      else if (chkword(3,"relative")) {   /*  relative amplitude levels */
         Abs_Level=0;
      }
      else if (chkword(1,"wave")) {     /* e.g. write WAVE format file  */
	 if (argc) { outfile=*argv++; argc--; wfmt=1; } /* output file name  */
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
      else if (chkword(1,"channels")) {
	 i=0;
	 if (argc) { 
	    i=atoi(*argv++); argc--;
	 }
	 if (i<1) exit(err_rpt(EINVAL,"Must have 1 or more channels."));
	 channels=i;
      }
      else {                             /* check for single char. flags    */
	 for (; *p; p++) {
	    if (*p=='h') exit(help(EFAIL));
	    else if (*p=='8') afmt=AFMT_U8;
	    else if (*p=='a') aflg=1;
	    else if (*p=='f') fflg=1;
	    else if (*p=='d') dflg=1;
	    else if (*p=='l') loopflg=-1;
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
      channels=atoi(get_conf_value(sys,"channels",QDEF_TONES_CHANNELS));
   }
   if (Wn==-1) {
      Wn=atoi(get_conf_value(sys,"loadable_waveforms",QDEF_WN));
   }
   if (afmt==AFMT_QUERY) {
      afmt=atoi(get_conf_value(sys,"samplesize",QAFMT_QUERY));
   }
   strncpy(dac,get_conf_value(sys,"dacfile",DAC_FILE),sizeof(dac)-1);

  /* Now check if any input file to be read........ */
  
   if (vflg) printf(VERSTR,sys,VERSION);

   fi=NULL;            /* fi!=NULL if a command file is to be read */
   if (infile!=NULL) { /* check for valid infile if specified */
      if (strcmp(infile,"-")==0) {
	 fi=stdin; infile="stdin";
      } else if ((fi=fopen(infile,"r"))==NULL) {
	 exit(err_rpt(errno,infile));
      }
   }

   /* open the right device or output file............ */
   f=NULL;
   if (outfile==NULL) {
#ifdef HAVE_DAC_DEVICE
                                  /* if no outfile then write direct to DAC */
                                  /* if no format specified then try 16 bit */
      i=(afmt==AFMT_QUERY)?AFMT_S16_LE:afmt ; 
      if ((fd=DACopen(fnm=dac,"w",&samplerate,&i,&stereo))<0) {
	 if (afmt==AFMT_QUERY) {  /* if no format specified try for 8 bit.. */
	    i=AFMT_U8;
	    fd=DACopen(fnm,"w",&samplerate,&i,&stereo);
	 }
	 if (fd<0) exit(err_rpt(errno,fnm));
      }
      if ((frag_size=getfragsize(fd))<0)
	exit(err_rpt(errno,"Problem getting DAC Buffer size."));

      if (vflg) {
	 printf("%s : DAC Opened for output\n",fnm);
         printf("%d bytes per DAC buffer.\n",frag_size);
      }
      if ((afmt!=AFMT_QUERY && i!=afmt) || stereo!=0) {
	 if (i!=afmt) 
	   err_rpt(EINVAL,"Sound card doesn't support requested sample format.");
	 if (stereo!=0)
	   err_rpt(EINVAL,"Card doesn't do MONO - nah, don't believe it.");
	 exit(EINVAL);
      }
      afmt=i;
#else
                                 /* error if no outfile specified */
      exit(help(err_rpt(EINVAL,"No output file specified, use -w or -o option.")));
#endif
   }
   else {
     if (strcmp(outfile,"-")==0) {
       if (aflg)
         exit(err_rpt(EINVAL,"Cannot write append to stdout."));
       f=stdout; fnm="stdout";
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
       frag_size=32768;
       if (vflg) { 
	 printf("%s samples to %s file %s\n",
		(*omode=='a')?"Appending":"Writing",
		(wfmt)?"WAVE":"RAW", fnm);	       
         printf("Buffer size is %d bytes.\n",frag_size);
       }
       fd=fileno(f);
       if (loopflg<0) loopflg=0; /* cannot loop forever if writing to a file */
       if (wfmt && (fWAVwrhdr(f,0,samplerate,afmt,stereo)<0)) {
	 exit(err_rpt(errno,"Writing WAVE file header."));
       }
     }
     if (fWAVinit(0,samplerate,afmt,stereo)) {
	 exit(err_rpt(errno,"Initialising sound file parameters."));
     }
   }
   if (!(afmt==AFMT_S16_LE || afmt==AFMT_U8)) {
      exit(err_rpt(EINVAL,
		   "Only unsigned 8 and signed 16 bit samples supported."));
   }
 	
   if (vflg) {
     if (vflg>1) fWAVdebug(stdout);
     printf("%d %s, %s, samples/sec\n",samplerate,
	    (stereo)?"stereo":"mono",
	    (afmt==AFMT_U8)?"unsigned 8 bit":
	    ((afmt==AFMT_S16_LE)?"signed 16 bit, little endian":
	     "Unknown audio format"));
     printf("Upto %d Frequencies can be played together.\n",channels);
     printf("%d loadable waveforms specified.\n",LWn);
     printf("Tone levels are %s.\n",(Abs_Level)?"ABSOLUTE":"RELATIVE");
   }

   /* initialise base samples etc */
   TWn=LWn;
   init_samples(f,fd,frag_size,samplerate,afmt,stereo,channels,&TWn);
   if (LWn && (st=do_loadable_waveforms(LWaa,LWn,TWn,vflg))) exit(st);

   do {
      for ( i=0; i<argc; i++) {
	 if (vflg) printf("%s\n",argv[i]);
	 if (st=doToneCmd(argv[i])) {
	    fprintf(stderr,"%s -> %s\n",argv[i],getGenErrStr());
	 }
      }
      if (fi!=NULL) ftones(fi,infile);
      if (loopflg>0) loopflg--;
   } while (loopflg);
   
   if (outfile==NULL) {
     flush_samples();
     close(fd);
   } else {
     if (wfmt && 
	 (fWAVwrhdr(f,getsamples(),samplerate,afmt,stereo)<0)) {
       err_rpt(errno,"Rewriting WAVE file header.");
     }
     fclose(f);
   }
   
   exit(0);
}  
