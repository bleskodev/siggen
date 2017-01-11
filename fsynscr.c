/* fsynscr.c
 * Screen handling for ncurses fsynth program
 * Jim Jackson    Oct 99
 */

/*
 * Copyright (C) 1997-2008
 *                    Jim Jackson                jj@franjam.org.uk
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
#include "fsynth.h"
#include <sys/soundcard.h>

#define CARDLINE 8
#define F0LINE  CARDLINE+1

#define Fsampl setV[0]
#define Fbits  setV[1]
#define Fchans setV[2]
#define FNFrag setV[3]

#define Fwave  CHvals[0]
#define Ffreq  CHvals[1]

/* set number of millisecs to wait while checking for ESC sequences */

#define KEY_DELAY 25

/* set number of microsecs to wait if no keys pressed. Is kind to
 * other processes - we don't hog time */

#define NO_KEY_WAIT 25000 

/* Main screen character strings............
 */

char *hl[]={
   "  ======  /====\\         |\\    ||  ======  ||   || ",
   "  ||      ||             ||\\   ||    ||    ||   || ",
   "  |===    \\====\\         || \\  ||    ||    |=====| ",
   "  ||          ||  |  |   ||  \\ ||    ||    ||   || ",
   "  ||      \\====/  \\--|   ||   \\||    ||    ||   || ",
   "                     |  ",
   "  Digital Fourier \\--/ Waveform Synthesiser  ",
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

char Mstarting[]=" Starting up .......... ",
     Mwavef[]="  Waveform: ",
     Mfreq[]= " Frequency: ",
     McardL[]="       samples/sec                 x       byte         ms buffers",
     Mtime[12],
     Minfo[]=" : ",
     Mstereo[]="STEREO",
     Mmono[]=  " MONO ",
     M16b[]=  "16 bit",
     M8b[]=   " 8 bit",
     Mmain[]=
     "Tab/Return next field,'Z'ero amplitudes,'R'esync chans,'Q'uit,'S'etup Card params",
     Mcard[]=
     "'S'et new values                         Tab/Return move to next field.";

char *Mbits[]={ M8b, M16b, NULL };
char *Mchans[]={ Mmono, Mstereo, NULL };
   
struct SCField  **CHvals;

struct SCField  *setV[6];               /* The variable card parameters */
struct SCField  *Ftime,*Finfo,*Finfo2,*Ffrag,*Ffragtim;  

/* Help strings for second info line.
 */

char Hwav[]="DOWN ARROW (or SPACE) & UP ARROW to select WaveForm",
     Hnum[]="Enter Digits, DEL or <- to delete, LEFT/RIGHT move digits, UP/DOWN Inc/Dec",
     Hbits[]="Press <SPACE> to toggle 8 / 16 bit setting",
     Hchan[]="Press <SPACE> to toggle Mono / Stereo setting";
    

char **Hvals;

char *Hset[]= { Hnum, Hbits, Hchan, Hnum, NULL };

/* waveform name and buffer array.......
 */

int *levels=NULL;                /* array to hold harmonic levels */
char **WAVEFORMS=NULL;
int **wavbufs=NULL;

/* WinGen()   The main interactive screen
 */

WinGen(fd)
int fd;
{
   int c,i,n,st,t;
   int f,savef;
   int card;                  /* if true we are altering the card params
			       * if false we are inputing in the main screen */
   char **helpMs;             /* ptr to array of help msgs for info line */
   struct SCField **inFs;     /* ptr to arrays of current input fields
			       * alternates between the card parameters
			       * and the main screen input
			       */
   
   WinGenInit();
   
   helpMs=Hvals; inFs=CHvals;
   doinfo(Mmain,0,Finfo2);
   savef=card=0; f=1;
   dispfield(inFs,helpMs,Finfo,f);
   refresh();
   t=0;
 
   for (;;) {
      if ((i=time(NULL))-t) { t=i; dotime(Ftime); } /* update time every sec */
      if (playsamples(fd)<0) {     /* do any generation */
	 move(COLS-1,0); endwin();
	 return(err_rpt(errno,"Problem generating output samples."));
      }
      if ((c=getch())==-1) {      /* if no keypressed..... */
	playsamples(fd);          /* try to generate another buffer */
	delay(NO_KEY_WAIT);       /* then wait preset time - be kind */
	continue;
      }
      if (c=='s' || c=='S') {     /* enter or exit card parameter change */
	 if (card) {              /* exit card param changes..... */
	    /* check if card params have changed - if so close and reopen */
            Lafmt=(GetOptionIndex(Fbits))?AFMT_S16_LE:AFMT_U8;
	    Lstereo=0;            /* ignore setting force mono */
            if ((Lsamplerate!=samplerate) || (Lafmt!=afmt) 
		|| (Lstereo!=stereo) || (LNfb!=Nfragbufs)) {
	       close(fd);
	       if ((fd=DACopen(dac,"w",&Lsamplerate,&Lafmt,&Lstereo))<0 ||
		   (fragsize=setfragsize(fd,LNfb,Bufspersec,Lsamplerate,Lafmt,Lstereo))<0) {
		  move(COLS-1,0); endwin();
		  return(err_rpt(errno,"Opening DSP for output."));
	       }
	       fragsamplesize=(fragsize>>(Lafmt==AFMT_S16_LE))>>(Lstereo);
	       fragtim=fragsamplesize*1000000/Lsamplerate;
	       putfield(Ffrag); putfield(Ffragtim);
	    }
	    dofields(setV);
	    /* if samplerate or format has changed, re-generate waveforms */
	    if ((Lsamplerate!=samplerate) || (Lafmt!=afmt)) {
	       if (st=genAllWaveforms(&WAVEFORMS,&wavbufs,1,Lsamplerate,Lafmt)) {
	          move(COLS-1,0); endwin();
		  return(err_rpt(st,"Problem generating Base waveforms."));
	       }
	    }
	    samplerate=Lsamplerate; afmt=Lafmt; stereo=Lstereo; Nfragbufs=LNfb;
	    SetOptionIndex(Fbits,(afmt==16));
	    SetOptionIndex(Fchans,stereo);
            playsamples(-1);    /* reset/resync the 'generators' */
	    dispfield(inFs,helpMs,Finfo,f); /* update card param display */
	    card=0; f=savef;                /* restore main screen field */
	                                    /* and turn off 'in card' flag */
	    inFs=CHvals; helpMs=Hvals;     /* set main screen input and help */
	    doinfo(Mmain,0,Finfo2);         /* and 2nd info line  */
	    dispfield(inFs,helpMs,Finfo,f); /* and redisplay main screen */
	 } else {               /* enter card setting alteration stuff */
	    dispfield(inFs,helpMs,Finfo,f); /* display main screen */
	    card=1; savef=f;                /* save current position and */
					    /* set 'in card' flag */
	    inFs=setV; helpMs=Hset;         /* set card input and help */
	    f=next_field(inFs,-1);          /* find first valid input field */
	    doinfo(Mcard,0,Finfo2);
	    dispfield(inFs,helpMs,Finfo,f);
	 }
	 continue;
      }
      if (c=='q' || c=='Q') {
	 move(COLS-1,0); endwin();
	 return(c);
      }
      if (c=='r' || c=='R') {
         playsamples(-1);   /* set generators for harmonics to be in phase */
         continue;
      }
      if (c=='z' || c=='Z') {
	dispfield(inFs,helpMs,Finfo,f);
	levels[0]=10000;
	for ( i=1; i<channels; i++) { levels[i]=0; }
	dofields(CHvals);
	if (card) savef=1; else f=1;
	dispfield(inFs,helpMs,Finfo,f);
	refresh();
	continue;
      }
      f=doinput(c,inFs,helpMs,Finfo,f);
   }
}

WinGenInit()
{
   int att_h1,att_h2,att_h3,att_h4,att_h5,att_h6;
   int n,i;
   char bf[100];
  
   Lsamplerate=samplerate;         /* set local working copy of samplerate */
   Lafmt=afmt; Lstereo=stereo=0;   /* and force mono working */
   LNfb=Nfragbufs;
   fragtim=fragsamplesize*1000000/samplerate;
                                   /* generate the waveforms at 1Hz  */
   if (n=genAllWaveforms(&WAVEFORMS,&wavbufs,1,samplerate,afmt)) {
      return(err_rpt(n,"Problem generating Base waveforms."));
   }

   tzset();              /* set timezone stuff up for dotime() function */
   
   initscr();            /* do ncurses setup................ */
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

   CHvals=(struct SCField **)malloc(sizeof(struct SCField *)*(channels+4));
   levels=(int *)malloc(sizeof(int)*channels);
   Hvals=(char **)malloc(sizeof(char *)*(channels+4));
  
   levels[0]=10000;
   for ( i=1; i<channels; i++) { levels[i]=0; }
  
   Fwave=newfield(SCF_option,att_h6,7,F0LINE,10,WAVEFORMS,0,0);
   Hvals[0]=Hwav;
   Ffreq=newfield(SCF_integer,att_h6,20,F0LINE,6,&freq,0,0);
   Hvals[1]=Hnum;
   setwaveform(Fwave,wf,WAVEFORMS);
   for (i=0; i<channels; i++) {
     CHvals[2+i]=newfield(SCF_integer,att_h6,42,F0LINE+i,6,&(levels[i]),0,4);
     Hvals[2+i]=Hnum;
   }
   CHvals[i+2]=NULL;
 
   sprintf(VERS,VERSTR,sys,VERSION); 
   dohdr(0,att_h1,att_h2,att_h3,hl,oh,sig,VERS);

   mvputfstr(CARDLINE,0,att_h3,McardL,80);
   mvputfstr(CARDLINE+1,0,att_h5,"",80);
   mvputfstr(LINES-2,0,att_h3,Minfo,80);
   mvputfstr(LINES-3,0,att_h3,Minfo,80);
   mvputfstr(LINES-4,0,att_h5,"",80);
  
   setV[0]=newfield(SCF_integer,att_h4,0,CARDLINE,6,&Lsamplerate,0,0);
   setV[1]=newfield(SCF_option,att_h4,19,CARDLINE,6,Mbits,0,0);
   SetOptionIndex(setV[1],(afmt==16));
   setV[2]=newfield(SCF_option,att_h4,26,CARDLINE,6,Mchans,0,0);
   SetOptionIndex(setV[2],stereo);
   setV[3]=newfield(SCF_integer,att_h4,33,CARDLINE,2,&LNfb,0,0);
   setV[4]=NULL;
   Ffrag=newfield(SCF_integer,att_h4,36,CARDLINE,6,&fragsize,0,0);
   Ffragtim=newfield(SCF_integer,att_h4,48,CARDLINE,8,&fragtim,0,3);
   Ftime=newfield(SCF_string,att_h4,69,CARDLINE,10,Mtime,0,0);
   Finfo =newfield(SCF_string,att_h3,3,LINES-3,75,Mstarting,0,0);
   Finfo2=newfield(SCF_string,att_h3,3,LINES-2,75,"",0,0);
  
   mvputfstr(F0LINE,0,att_h5,"Wave                       Hz  amplitude",80);
   for (i=2; i+F0LINE-1 < LINES-3; i++) {
     if ((i-1)<channels) {
       sprintf(bf,"    %2d%s Harmonic              amplitude ",i,
	       (i==1)?"st":((i==2)?"nd":((i==3)?"rd":"th")) );
       mvputfstr(i+F0LINE-1,0,att_h5,bf,80);
     } else {
       mvputfstr(i+F0LINE-1,0,att_h5,"",80);
     }
   }

   putfield(Finfo); putfield(Finfo2); putfield(Ffrag); putfield(Ffragtim);
   dofields(setV);
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
  static int *off=NULL;
  
  short int v,*wv1,*wpend;
  short int *vp,*wp;
  unsigned char *p,c,*w1;
  int i,l,fl,ff,n,t,tr,of,*lp,freqn;
  
  if (off==NULL) { 
    if ((off=(int *)malloc(channels*sizeof(int)))==NULL) return(-1);
  }
  if (fd<0) {        /* reset pointers */
    for ( l=0 ; l<channels; l++) { off[l]=0; }
    return(0);
  }
  if (fragsize!=fr_size) {
    if (frag) free(frag);
    if ((frag=malloc(fragsize))==NULL) return(-1);
    fr_size=fragsize;
    for ( l=0 ; l<channels; l++) { off[l]=0; }
  }

  for (n=Nfragbufs; n; n--) {
    if ((i=getfreeobufs(fd))<=0) return(i);

    if (afmt==AFMT_S16_LE) {
      wv1=(short int *)(wavbufs[Fwave->adj]);
      wpend=wv1+samplerate;
      fl=0; ff=1;
      for ( l=1, lp=levels; l<=channels ; l++, lp++ ) { 
	freqn=l*freq;
	if (!(*lp) || freqn>=(samplerate/2)) {
	  off[l-1]=(((off[l-1]*freqn)%samplerate)*fragsamplesize)%samplerate;
	  continue;
	}
	wp=wv1+(of=off[l-1]); 
	t=fl+((*lp)/ff);
	for (i=fragsamplesize, vp=(short int *)frag; i ; i--) {
	  /*	   t=(int)(wv1[of1]); */
	  /*	   *vp++=(short int)((t>32767)?32767:((t<-32767)?-32767:t)); */
	  *vp++=(short int)((((int)(*vp)*fl)+(((int)(*wp))*(*lp))/ff)/t);
	  wp=wv1+(of=(of+freqn)%samplerate);
	}
	off[l-1]=of; 
	for ( fl=t ; fl>32000 ; fl>>=1, ff<<=1 ) { }
      }
    } else {  /* afmt is 8 bit this is still to implement */
      w1=(unsigned char *)(wavbufs[Fwave->adj]); 
      for (i=0, p=frag; i<fragsamplesize; i++) {
	/*	t=(int)(w1[of1]);
	 *p++=(unsigned char)((t>255)?255:((t<0)?0:t));
	 of1=(of1+freq)%samplerate;
	 */      }
    }
    if (write(fd,frag,fragsize)<0) return(-1);
  }
  return(0);
}

