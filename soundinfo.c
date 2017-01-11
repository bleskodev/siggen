/* soundinfo.c
 * Describes the Linux Sound system support
 * Jim Jackson
 *
 *  Date     Vers   Comment
 *  -----    -----  ---------------
 *  27Oct96  1.0    Initial Program Built from generic filter.c
 *                  Reports mixer capabilities etc.
 *  18Nov96  1.1    Added DSP info
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

/***MANPAGE***/
/*
soundinfo: (version 1) 27Oct96

NAME:
        soundinfo  -  Describes the Linux Sound system support

SYNOPSIS:
        soundinfo [-v] [options]

DESCRIPTION:

Describes the Linux Sound system support on the system. Mixer devices,
DSP capabilities etc.

FLAGS:

-h       give usage
-v       verbose output

OPTIONS:


NOTES:

BUGS:

This program is covered by the GNU General Public License, see file
COPYING for further details.

Any questions, suggestions or problems, etc. should be sent to

Jim Jackson

Email Internet:  jj@franjam.org.uk

 */

/***INCLUDES***/
/* includes files 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef __FreeBSD__
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif

/***DEFINES***/
/* defines
 */

#define VERSION "2.3.10 (May 2008)"
#define MIXDEV "/dev/mixer"
#define DSPDEV "/dev/dsp"

#define EFAIL -1

#define TRUE 1
#define FALSE 0
#define nl puts("")

#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

/***GLOBALS***/
/* global variables
 */

char *sys;
short int debug;                       /* set if the -d flag is used */
short int vflg;                        /* set if -v flag used        */

/***DECLARATIONS***/
/* declare non-int functions 
 */

char *delnl(),*reads(),*sindex(),
     *sindex_nc(),*tab(),*ctime();
FILE *fopen(),*mfopen();

/***MAIN***/
/* main - for program  soundinfo
 *        Describes the Linux Sound system support
 */

main(argc,argv)
int argc;
char **argv;

 { int i,j,n,st;
   char fname[132],*fnm,*p;
   int fd;
    
   argv[argc]=NULL;
   sys=*argv++; argc--;
   if ((p=strrchr(sys,'/'))!=NULL) { sys=p+1; }

   debug=vflg=FALSE;

   printf("%s Ver. %s  (c) 1996-2008  Jim Jackson\n",sys,VERSION);

   while (argc && **argv=='-')            /* all flags and options must come */
    { n=strlen(p=(*argv++)+1); argc--;    /* before paramters                */
      if (*p==0) n=atoi(p+1);             /* e.g. num. option  -n23          */
      else                                /* check for single char. flags    */
	{ for (; *p; p++)
	    { if (*p=='h') exit(help(EFAIL));
	      else if (*p=='d') debug=TRUE;
	      else if (*p=='v') vflg=TRUE;
	      else 
		{ *fname='-'; *(fname+1)=*p; *(fname+2)=0;
		  exit(help(err_rpt(EINVAL,fname)));
		}
	    }
	}
    }

   if ((fd = open(fnm=MIXDEV, O_RDONLY)) < 0) {
     err_rpt(errno,fnm);
   } else {
     do_mix(fd,fnm);
   }

   if ((fd = open(fnm=DSPDEV, O_RDWR)) < 0) {
     err_rpt(errno,fnm);
   } else {
     do_dsp(fd,fnm);
   }

   exit(st);
 }

/***HELP***/
/* help(e)  print brief help message - return parameter given
 */

help(e)
int e;

 {
   puts("\n\
Describes some features of the Linux Sound system support in the kernel.\n\
");
   printf("\
Usage:   %s\n\
",sys);
   return(e);
 }

do_dsp(fd,dev)
int fd;
char *dev;
{
   int i,n;
   int caps,fmts,blksize;
   char buf[130],*p;
   audio_buf_info auobf,auibf;

   if (ioctl(fd, SNDCTL_DSP_GETCAPS , &caps) < 0) {
      return(err_rpt(errno,"Reading DSP capabilities."));
   }
   if (ioctl(fd, SNDCTL_DSP_GETFMTS , &fmts) < 0) {
      return(err_rpt(errno,"Reading DSP formats."));
   }
   if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE , &blksize) < 0) {
      return(err_rpt(errno,"Reading blocksize."));
   }
   if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &auobf) < 0) {
      return(err_rpt(errno,"Reading output audio buffer info."));
   }
   if (ioctl(fd, SNDCTL_DSP_GETISPACE, &auibf) < 0) {
      return(err_rpt(errno,"Reading input audio buffer info."));
   }

   printf("\nDSP details ................\n");
   printf("     capabilites = 0x%09X\n         formats = 0x%09X\n",caps,fmts);
   printf("       blocksize = %d\n", blksize);

   printf("\nDSP Capability revision level %d\n",caps&255);
   printf("    %s duplex operation (simultaneous read/write)\n",
          (caps&DSP_CAP_DUPLEX)?"Supports":"Does not support");
   if (caps&DSP_CAP_REALTIME) 
     printf("    Supports RealTime capability.\n" );
   if (caps&DSP_CAP_BATCH)
     printf("    Has batch facilities (buffers) which may cause timing problems etc.\n");
   if (caps&DSP_CAP_COPROC)
     printf("    Has a Co-Processor\n");
   printf("    %s SETTRIGGER operation.\n",
          (caps&DSP_CAP_TRIGGER)?"Supports":"Does not support");

   printf("\nDSP Formats supported are :-\n");
   if (fmts&AFMT_MU_LAW) printf("  Mu_Law ");
   if (fmts&AFMT_A_LAW) printf("   A_Law ");
   if (fmts&AFMT_IMA_ADPCM) printf("   IMA_ADPCM ");
   if (fmts&AFMT_U8) printf("   U8 ");
   if (fmts&AFMT_S16_LE) printf("   S16_LE ");
   if (fmts&AFMT_S16_BE) printf("   S16_BE ");
   if (fmts&AFMT_S8) printf("   S8 ");
   if (fmts&AFMT_U16_LE) printf("   U16_LE ");
   if (fmts&AFMT_U16_BE) printf("   U16_BE ");
   if (fmts&AFMT_MPEG) printf("   MPEG ");
   printf("\n\n"); 

   printf("Fragment details    Output.. Input..\n");
   printf(" total fragments      %4d     %4d\n",
                  auobf.fragstotal,auibf.fragstotal);
   printf("       available      %4d     %4d\n",
                  auobf.fragments,auibf.fragments);
   printf("   fragment size     %5d    %5d\n",
                  auobf.fragsize,auibf.fragsize);
   printf(" bytes available    %6d   %6d\n\n",
                  auobf.bytes,auibf.bytes);
}
 
do_mix(fd,dev)
int fd;
char *dev;
{
   int devmask,recmask,sources,stereodevs,capabilities;
   int i,n;
   char buf[130];
   char *devname[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
   int level[SOUND_MIXER_NRDEVICES];
   struct mixer_info Mix_Info;

   if (ioctl(fd, SOUND_MIXER_INFO, &Mix_Info) < 0) {
      err_rpt(errno,"Getting mixer_info details.");
      Mix_Info.id[0]=Mix_Info.name[0]=0;
   }
   if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0) {
      return(err_rpt(errno,"Reading Mixer Channels."));
   }
   if (devmask==0) {
      return(err_rpt(ENODEV,"Mixer has no Channels!"));
   }
   if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask) < 0) {
      return(err_rpt(errno,"Reading Recording Mask."));
   }
   if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &sources) < 0) {
      return(err_rpt(errno,"Reading Recording Devices."));
   }
   if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereodevs) < 0) {
      return(err_rpt(errno,"Reading Stereo Devices."));
   }
   if (ioctl(fd, SOUND_MIXER_READ_CAPS, &capabilities) < 0) {
      return(err_rpt(errno,"Reading Mixer Capabilities."));
   }

   printf("MIXER DETAILS ......................\n");
   printf("           Mixer Id = %s\n               Name = %s\n",Mix_Info.id,Mix_Info.name);
   printf("           devmask  = 0x%09X\n           recmask  = 0x%09X\n",devmask,recmask);
   printf("           sources  = 0x%09X\n        stereodevs  = 0x%09X\n",sources,stereodevs);
   printf("MIXER capabilities  = 0x%09X\n",capabilities);
   if (capabilities & SOUND_CAP_EXCL_INPUT)
     printf("      Only one sound source can be selected at a time.\n"); 
   
   printf("\nMIXER (%s) has following channels, set as :-\n",dev);
   for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
      if (devmask & (1<<i)) {
	 if (ioctl(fd, MIXER_READ(i), &n) < 0) {
	    sprintf(buf,"Problem reading current level of mixer channel %s.",
		    devname[i]);
	    err_rpt(ENODEV,buf);
	 } else {
	    printf("  [0x%08X] %2d  %-6s  %3d",1<<i,i,devname[i],n&255);
	    if (stereodevs & (1<<i)) { printf("/%3d ",(n>>8)&255);
	    } else { printf("     "); }
            printf("%9s ", (recmask & (1<<i))? "source":"");
            printf("%10s ", (sources & (1<<i))? "selected":"");
	    puts(""); /* nl */
	 }
      }
   }
}
    
/***ERR_RPT***/
/* err_rpt(err,msg)
 */

err_rpt(err,msg)
short int err;
char *msg;

 { 
   extern char *sys;

   if (err) fprintf(stderr,"[%s] %s : %s\n",sys,strerror(err),msg);
   return(err);
 }

