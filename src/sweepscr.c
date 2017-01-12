/* sweepscr.c
 * Screen handling for ncurses SweepGEN program
 * Jim Jackson    Oct 99
 */

/*
 * Copyright (C) 1997-2008
 *                    Jim Jackson                    jj@franjam.org.uk
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

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "scfio.h"
#include "sweepgen.h"
#include <sys/soundcard.h>

/* set number of millisecs to wait while checking for ESC sequences */

#define KEY_DELAY 25

/* Main screen character strings............
 */

char *hl[]={
   " /====\\                         /====\\ ====== |\\    |  ",
   " |                              |      |      | \\   |  ",
   " \\====\\ |    |  /-\\  /-\\  /--\\  |   __ |===   |  \\  | ",
   "      | |    | |  / |  /  |  |  |    | |      |   \\ |    ",
   " \\====/  \\/\\/   \\--  \\--  |--/  \\====/ ====== |    \\| ",
   "                          |     ",
   "           Digital Sweep  |  Generator ", 
   " -----------------------------------------------------------------------------",
   NULL };

char *oh[]={
   "      /\\         ",
   "   --/--\\--/--   ", 
   "         \\/      ",
   NULL };

char *sig=" Jim Jackson <jj@franjam.org.uk> ";

char VERS[128];

int Lsamplerate,             /* Local copy of card params to prevent */
    LNfb,                   
    Lafmt,Lstereo;           /* playing being screwed while updating */
                             /* card characteristics.                */
int fragtim;                 /* Playing Time of one fragment in Microsecs */

char Mchan1[]="  Sweep WaveForm ",
     Mchan2[]="  Swept WaveForm ",
     Mstarting[]=" Starting up .......... ",
     Mwavef[]="  Waveform: ",
     Mfreq[]= " Frequency:         Hz",
     Mfreq1[]= " Frequency From:         Hz  To:          Hz",
     Mfreq2[]= "         Centre:         Hz  +/-          Hz",
     Mratio[]=" M/S Ratio: ",
     Mgain[]= "Hz  Gain: ",
     Msampl[]="       samples/sec                 x       byte         ms buffers",
     Minfo[]=" : ",
     Mtime[16]="",
     Mstereo[]="STEREO",
     Mmono[]=  " MONO ",
     M16b[]=  "16 bit",
     M8b[]=   " 8 bit",
     Mmain[]=
     "Tab/Return move fields,                      'Q'uit, 'S'etup samplerate etc",
     Mcard[]=
     "'S'et new values                         Tab/Return move to next field.";


char *Mbits[]={ M8b, M16b, NULL };
char *Mchans[]={ Mmono, Mstereo, NULL };
   
struct SCField
 FsamP= { SCF_string, 8, 0, 0, Msampl, 80, 0, 0 },
 FsamV= { SCF_integer, 8, 0, 0, &Lsamplerate, 6, 0, 0 },
 Fbits= { SCF_option, 8, 19, 0, Mbits, 6, 0, 0 },
 Fchans= { SCF_option, 8, 26, 0, Mchans, 6, 0, 0 },
 FNfrag= { SCF_integer, 8, 33, 0, &LNfb, 2, 0, 0 },
 Ffrag= { SCF_integer, 8, 36, 0, &fragsize, 6, 0, 0 },
 Ffragtim= { SCF_integer, 8, 48, 0, &fragtim, 8, 0, 0, 3 },
 FinfoP= { SCF_string, 21, 0, 0, Minfo, 80, 0, 0 },
 FinfoP2= { SCF_string, 22, 0, 0, Minfo, 80, 0, 0 },
 Finfo= { SCF_string, 21, 3, 0, Mstarting, 75, 0, 0 },
 Finfo2= { SCF_string, 22, 3, 0, "", 75, 0, 0 },
 Ftime= { SCF_string, 8, 69, 0, Mtime, 10, 0, 0 },

 Fchan1= { SCF_string,  11, 0, 0, Mchan1, 12, 0, 0 },
 Fwav1P= { SCF_string,  12, 2, 0, Mwavef, 12, 0, 0 },
 Fwav1V= { SCF_option,  12, 14, 0, NULL, 8, 0, 0 },
 FfreqP= { SCF_string,  12, 22, 0, Mfreq, 22, 0, 0 },
 FfreqV= { SCF_integer, 12, 34, 0, &freq, 8, 0, 0 },

 Fchan2= { SCF_string,  15, 0, 0, Mchan2, 12, 0, 0 },
 Fwav2P= { SCF_string,  16, 2, 0, Mwavef, 12, 0, 0 },
 Fwav2V= { SCF_option,  16, 14, 0, NULL, 8, 0, 0 },
 Ffrq1P= { SCF_string,  16, 22, 0, Mfreq1, 48, 0, 0 },
 Ffrq2P= { SCF_string,  17, 22, 0, Mfreq2, 48, 0, 0 },
 FfrqFV= { SCF_integer, 16, 39, 0, &freqF, 8, 0, 1 },
 FfrqTV= { SCF_integer, 16, 56, 0, &freqT, 8, 0, 2 },
 FfrqCV= { SCF_integer, 17, 39, 0, &freqC, 8, 0, 3 },
 FfdevV= { SCF_integer, 17, 56, 0, &frdev, 8, 0, 4 },
 Fgn2P=  { SCF_string,  16, 40, 0, Mgain, 10, 0, 0 },
 Fgn2V=  { SCF_integer, 16, 50, 0, &Gain2, 6, 0, 3 };

struct SCField 
 *CHhdrs[]= { &Fchan1, &Fwav1P, &FfreqP, 
              &Fchan2, &Fwav2P, &Ffrq1P, &Ffrq2P,
/*              &Fgn2P, */
              NULL },
 *CHvals[]= { &Fwav1V, &FfreqV,
              &Fwav2V, &FfrqFV, &FfrqTV, &FfrqCV, &FfdevV,
/*              &Fgn2V, */
              NULL },
 *scrP[]= { &FsamP, &FinfoP, &FinfoP2, &Finfo, &Finfo2, 
            &Ffrag, &Ffragtim, &Ftime, NULL, },
 *scrV[]= { &FsamV, &Fbits, &Fchans, &FNfrag, NULL };
    
/* Help strings for second info line.
 */

char Hwav[]="DOWN ARROW (or SPACE) & UP ARROW to select WaveForm",
     Hnum[]="Enter Digits, DEL or <- to delete, LEFT/RIGHT move digits, UP/DOWN Inc/Dec",
     Hbits[]="Press <SPACE> to toggle 8 / 16 bit setting",
     Hchan[]="Press <SPACE> to toggle Mono / Stereo setting";
    

char *Hvals[]={ Hwav, Hnum, 
                Hwav, Hnum, Hnum, Hnum, Hnum, NULL };
char *Hset[]= { Hnum, Hbits, Hchan, Hnum, NULL };

/* waveform name and buffer array.......
 */

char **WAVEFORMS=NULL;
int **wavbufs=NULL;

/* WinGen()   The main interactive screen
 */

WinGen(fd)
int fd;
{
   int c,i,n,st,t;
   unsigned int j,k;
   int f,wavef,card;
   char **hh;
   struct SCField **ff;
   
   if (n=WinGenInit()) return(n);
   
   hh=Hvals; ff=CHvals;
   doinfo(Mmain,0,&Finfo2);
   wavef=card=f=0;
   dispfield(ff,hh,&Finfo,f);
   refresh();
   t=time(NULL);
     
   for (;;) {
      if ((i=time(NULL))-t) { t=i; dotime(&Ftime); }
      if (playsamples(fd)<0) {
	 move(COLS-1,0); endwin();
	 return(err_rpt(errno,"Problem generating output samples."));
      }
      if ((c=getch())==-1) {
         delay(25000);
	 continue;
      }
      if (c=='s' || c=='S') {  
	 if (card) {
	    /* check if card params have changed - if so close and reopen */
            Lafmt=(Fbits.adj)?AFMT_S16_LE:AFMT_U8;
	    Lstereo=Fchans.adj; 
            if ((Lsamplerate!=samplerate) || (Lafmt!=afmt) || (Lstereo!=stereo) || (LNfb!=Nfragbufs)) {
	       close(fd);
	       if ((fd=DACopen(dac,"w",&Lsamplerate,&Lafmt,&Lstereo))<0 ||
		   (fragsize=setfragsize(fd,LNfb,Bufspersec,Lsamplerate,Lafmt,Lstereo))<0) {
		  move(COLS-1,0); endwin();
		  return(err_rpt(errno,dac));
	       }
	       fragsamplesize=(fragsize>>(Lafmt==AFMT_S16_LE))>>(Lstereo);
	       fragtim=fragsamplesize*1000000/Lsamplerate;
	       putfield(&Ffrag); putfield(&Ffragtim);
	    }
	    dofields(scrV);
	    /* if samplerate or format has changed, re-generate waveforms */
	    /* we generate 16 bit samples cos we need it for sweeping freq */
	    /*  then we adjust if we are writing 8 bit samples             */
	    if ((Lsamplerate!=samplerate) || (Lafmt!=afmt)) {
               if (st=genAllWaveforms(&WAVEFORMS,&wavbufs,1,Lsamplerate*resolution,AFMT_S16_LE)) {
	          move(COLS-1,0); endwin();
		  return(err_rpt(st,"Problem generating Base waveforms."));
	       }
	    }
	    samplerate=Lsamplerate; afmt=Lafmt; stereo=Lstereo; Nfragbufs=LNfb;
            playsamples(-1);    /* resync left and right channels */
	    dispfield(ff,hh,&Finfo,f);
	    card=0; f=wavef;
	    ff=CHvals; hh=Hvals;
	    doinfo(Mmain,0,&Finfo2);
	 } else {   /* enter card setting alteration stuff */
	    dispfield(ff,hh,&Finfo,f);
	    card=1; wavef=f; 
	    ff=scrV; hh=Hset;
	    f=next_field(ff,-1);
	    doinfo(Mcard,0,&Finfo2);
	 }
	 dispfield(ff,hh,&Finfo,f);
	 continue;
      }      

      if (c=='q' || c=='Q') {
	 move(COLS-1,0); endwin();
	 return(c);
      }
      f=doinput(c,ff,hh,&Finfo,f);
      if (i=(ff[f]->id)) {
	 if (i<=2) {
	    freqC=(freqF+freqT+1)/2;
	    frdev=((freqT>freqF)?freqT:freqF)-freqC;
	 } else {
	    if ((i==3) && (freqC<frdev)) freqC=frdev;
	    else if ((i==4) && (freqC<frdev)) frdev=freqC;
	    if (freqF<freqT) { 
	       freqF=freqC-frdev; freqT=freqC+frdev; 
	    } else {
	       freqT=freqC-frdev; freqF=freqC+frdev; 
	    }
	 }
	 putfield(&FfrqCV); putfield(&FfdevV);
	 putfield(&FfrqTV); putfield(&FfrqFV);
	 putfield(ff[f]);
      }
   }
}

WinGenInit()
{
   int att_h1,att_h2,att_h3,att_h4,att_h5,att_h6;
   int n,i;
   
   Lsamplerate=samplerate;         /* set local working copy of samplerate */
   Lafmt=afmt; Lstereo=stereo;
   LNfb=Nfragbufs;

   Fbits.adj=(afmt==16);
   Fchans.adj=stereo;
   fragtim=fragsamplesize*1000000/samplerate;
   
   if (n=genAllWaveforms(&WAVEFORMS,&wavbufs,1,samplerate*resolution,afmt)) {
      return(err_rpt(n,"Problem generating Base waveforms."));
   }

   Fwav1V.val.sp=WAVEFORMS;   Fwav2V.val.sp=WAVEFORMS;
   setwaveform(&Fwav1V,wf,WAVEFORMS);
   setwaveform(&Fwav2V,wf2,WAVEFORMS);

   n=0;   /* setup the correct number of Decimal points depending on resolution */
   if (resolution==10) n=1;
   else if (resolution==100) n=2;
   FfreqV.dp=FfrqFV.dp=FfrqTV.dp=FfrqCV.dp=FfdevV.dp=n; 
   freq*=resolution; freqT*=resolution; 
   freqC*=resolution; freqF*=resolution; frdev*=resolution;

   tzset();              /* set timezone stuff up for dotime() function */
  
   if (vflg) {  /* ========= if vflg pause before going into ncurses =======*/
     printf("\n<Press Return to Continue>\n");
     getchar();
   }

   initscr();
   cbreak(); noecho();
   nonl(); intrflush(stdscr,FALSE); keypad(stdscr,TRUE);
   nodelay(stdscr,TRUE);
   timeout(KEY_DELAY);
   
   if (has_colors()) {
      start_color();
      ColorPair1;
      ColorPair2;
      ColorPair3;
      ColorPair4;
      ColorPair5;
      ColorPair6;
      att_h1=COLOR_PAIR(1);
      att_h2=COLOR_PAIR(2)+A_BOLD;
      att_h3=COLOR_PAIR(3);
      att_h4=COLOR_PAIR(4);
      att_h5=COLOR_PAIR(5);
      att_h6=COLOR_PAIR(6);
   } else {
      att_h1=A_REVERSE; att_h5=att_h6=att_h2=att_h4=A_BOLD;
      att_h3=A_NORMAL;
   }
   
   dofatt(scrV,att_h4); dofatt(scrP,att_h3);
   Ffrag.attr=Ffragtim.attr=att_h4;
   dofatt(CHhdrs,att_h5);
   dofatt(CHvals,att_h6);
   
   sprintf(VERS,VERSTR,sys,VERSION);
   dohdr(0,att_h1,att_h2,att_h3,hl,oh,sig,VERS);
   for (i=FsamP.row; i<Finfo2.row; i++) {
      mvputfstr(i,0,att_h5,"",80);
   }

   dofields(scrP); dofields(scrV);
   dofields(CHhdrs);
   dofields(CHvals);
   refresh();
}

/* playsamples(fd)   check state of output sound buffers, if necessary
 *                generate next fragment of samples and write it away.
 * if fd < 0 then the offset pointers are resynched by setting to zero.
 */

playsamples(fd)
int fd;
{
   static int fr_size=0;
   static unsigned char *frag=NULL;
   static int of1,of2;   /* of1 for the sweeping, of2 for swept frequencies */

   short int v,*vp,*wv1,*wv2;
   unsigned char *p;
   int i,n,t1,t2,dev;
  
   if (fd<0) { of2=0; return(of1=0); }     /* reset pointers */ 
   if (fragsize!=fr_size) {
      if (fr_size) free(frag);
      if ((frag=malloc(fragsize))==NULL) return(-1);
      fr_size=fragsize; of1=0; of2=0;
   }

#define TO8BIT(V) (unsigned char)((V>>8)+128)
     
   /*      (afmt==AFMT_S16_LE) */
   /* It's all done in signed 16 bit format - because we need the 
    * precision for the sweeping waveform. We adjust down to 8 bit
    * if writing 8 bit samples on output.
    */
      
   dev=(freqF<freqT)?frdev:-frdev;
   for (;;) {
      if ((i=getfreeobufs(fd))<=0) return(i);
      
      wv1=(short int *)(wavbufs[Fwav1V.adj]);  /* sweeping waveform */
      wv2=(short int *)(wavbufs[Fwav2V.adj]);  /* swept waveform */

      for (i=0, vp=(short int *)(p=frag); i<fragsamplesize; i++) {
	 t1=(int)(wv1[of1]);
	 if (stereo) {
	    if (afmt==AFMT_S16_LE) *vp++=wv1[of1]; else *p++=TO8BIT(wv1[of1]);
	 }
	 of1=(of1+freq)%(samplerate*resolution);
	 if (afmt==AFMT_S16_LE) *vp++=wv2[of2]; else *p++=TO8BIT(wv2[of2]);
	 of2=((int)(samplerate*resolution)+of2+(int)freqC+((t1*dev)/32768))%(samplerate*resolution);
      }
      if (write(fd,frag,fragsize)<0) return(-1);
   }
   return(0);
}
