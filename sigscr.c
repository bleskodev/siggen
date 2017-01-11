/* sigscr.c
 * Screen handling for ncurses SigGEN program
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
#include "siggen.h"
#include <sys/soundcard.h>

/* set number of millisecs to wait while checking for ESC sequences */

#define KEY_DELAY 25

/* Main screen character strings............
 */

char *hl[]={
   "     /====\\            /====\\  ======  |\\    |  ",
   "     |                 |       |       | \\   |    ",
   "     \\====\\  o  /--\\   |   __  |===    |  \\  | ",
   "          |  |  |  |   |    |  |       |   \\ |    ",
   "     \\====/  |  \\--|   \\====/  ======  |    \\| ",
   "                   |  ",
   "       Digital  \\--/  Signal Generator ", 
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

char Mchan1[]="  Channel 1 ",
     Mchan2[]="  Channel 2 ",
     Mstarting[]=" Starting up .......... ",
     Mwavef[]="  Waveform: ",
     Mfreq[]= " Frequency: ",
     Mgain[]= "Hz Gain: ",
     Mphase[]="Phase:        deg.",
     Mratio[]=" M/S Ratio: ",
     Moffs[]= "Phaseshift: ",
     Msampl[]="       samples/sec                 x       byte         ms buffers",
     Minfo[]=" : ",
     Mtime[16]="",
     Mstereo[]="STEREO",
     Mmono[]=  " MONO ",
     M16b[]=  "16 bit",
     M8b[]=   " 8 bit",
     Mmain[]=
     "Tab/Return move fields,   'R'esync channels, 'Q'uit, 'S'etup samplerate etc",
     Mcard[]=
     "'S'et new values                         Tab/Return move to next field.";

char *Mbits[]={ M8b, M16b, NULL };
char *Mchans[]={ Mmono, Mstereo, NULL };
char *Mtrack[]={ "  ", "tracking ||", NULL };

struct SCField
 FsamP= { SCF_string, 8, 0, 0, Msampl, 80, 0, 0, 0 },
 FsamV= { SCF_integer, 8, 0, 0, &Lsamplerate, 6, 0, 0, 0 },
 Fbits= { SCF_option, 8, 19, 0, Mbits, 6, 0, 0, 0 },
 Fchans= { SCF_option, 8, 26, 0, Mchans, 6, 0, 0, 0 },
 FNfrag= { SCF_integer, 8, 33, 0, &LNfb, 2, 0, 0, 0 },
 Ffrag= { SCF_integer, 8, 36, 0, &fragsize, 6, 0, 0, 0 },
 Ffragtim= { SCF_integer, 8, 48, 0, &fragtim, 8, 0, 0, 3 },
 FinfoP= { SCF_string, 21, 0, 0, Minfo, 80, 0, 0, 0 },
 FinfoP2= { SCF_string, 22, 0, 0, Minfo, 80, 0, 0, 0 },
 Finfo= { SCF_string, 21, 3, 0, Mstarting, 75, 0, 0, 0 },
 Finfo2= { SCF_string, 22, 3, 0, "", 75, 0, 0, 0 },
 Ftime= { SCF_string, 8, 69, 0, Mtime, 10, 0, 0, 0 },

 Fchan1= { SCF_string,  11, 0, 0, Mchan1, 12, 0, 0, 0 },
 Fwav1P= { SCF_string,  12, 2, 0, Mwavef, 12, 0, 0, 0 },
 Fwav1V= { SCF_option,  12, 14, 0, NULL, 8, 0, 0, 0 },
 Ffrq1P= { SCF_string,  12, 22, 0, Mfreq, 12, 0, 0, 0 },
 Ffrq1V= { SCF_integer, 12, 34, 0, &freq, 8, 0, 0, 1 },
 Fgn1P=  { SCF_string,  12, 42, 0, Mgain, 10, 0, 0, 0 },
 Fgn1V=  { SCF_integer, 12, 51, 0, &Gain, 6, 0, 0, 3 },
    
 FtrWav= { SCF_option,  14, 10, 0, Mtrack, 11, 0, 0, 0 },
 FtrFrq= { SCF_option,  14, 29, 0, Mtrack, 11, 0, 0, 0 },
 FtrGn=  { SCF_option,  14, 44, 0, Mtrack, 11, 0, 0, 0 },

 Fchan2= { SCF_string,  15, 0, 0, Mchan2, 12, 0, 0, 0 },
 Fwav2P= { SCF_string,  16, 2, 0, Mwavef, 12, 0, 0, 0 },
 Fwav2V= { SCF_option,  16, 14, 0, NULL, 8, 0, 0, 0 },
 Ffrq2P= { SCF_string,  16, 22, 0, Mfreq, 12, 0, 0, 0 },
 Ffrq2V= { SCF_integer, 16, 34, 0, &freq2, 8, 0, 0, 1 },
 Fgn2P=  { SCF_string,  16, 42, 0, Mgain, 10, 0, 0, 0 },
 Fgn2V=  { SCF_integer, 16, 51, 0, &Gain2, 6, 0, 0, 3 },
 FphP=   { SCF_string,  16, 60, 0, Mphase, 18, 0, 0, 0 },
 FphV=   { SCF_integer, 16, 67, 0, &phase, 6, 0, 0, 0 };

/* These arrays define groups of SCFields that are used together.
 * CHhdrs[]  is the array of titles for the generation parameters
 *           for the 2 channels, this displayed at init.
 * CHvals[]  is the array of generation input parameter fields that
 *           are cycled round and altered.
 * scrP[]    array of screen titles for soundcard config and status lines
 * scrV[]    array of soundcard config values
 */

struct SCField 
 *CHhdrs[]= { &Fchan1, &Fwav1P, &Ffrq1P, &Fgn1P,
              &Fchan2, &Fwav2P, &Ffrq2P, &Fgn2P, &FphP, NULL },
 *CHvals[]= { &Fwav1V, &Ffrq1V, &Fgn1V,
              &Fwav2V, &Ffrq2V, &Fgn2V, &FphV, NULL },
 *scrP[]= { &FsamP, &FinfoP, &FinfoP2, &Finfo, &Finfo2, 
            &Ffrag, &Ffragtim, &Ftime, NULL, },
 *scrV[]= { &FsamV, &Fbits, &Fchans, &FNfrag, NULL };

/* 
 * TRKvals[] is the array of fields holding and displaying the tracking
 *           values for the waveform, frequency and gain fields 
 *           it has same numbers of elements as CHvals[] and has same
 *           tracking field def for the fields that can track.
 * TRKpairs[]   is an array of integers of same size as CHvals.
 *              if CHvals[i] entry has a possible tracking partner
 *              it is CHvals[TRKpairs[i]]. When tracking, changing one
 *              value of a pair, alters its partner's setting too.
 *              entry of -1 indicates no tracking partner.
 */

struct SCField
   *TRKvals[]=  { &FtrWav, &FtrFrq, &FtrGn, 
                  &FtrWav, &FtrFrq, &FtrGn, NULL, NULL };
int TRKpairs[]= { 3, 4, 5, 0, 1, 2, -1 };

/* Help strings for second info line.
 */

char Hwav[]="DOWN ARROW (or SPACE) & UP ARROW to select WaveForm",
     Hnum[]="Enter Digits, DEL or <- to delete, LEFT/RIGHT move digits, UP/DOWN Inc/Dec",
     Hbits[]="Press <SPACE> to toggle 8 / 16 bit setting",
     Hchan[]="Press <SPACE> to toggle Mono / Stereo setting";
    

char *Hvals[]={ Hwav, Hnum, Hnum, 
                Hwav, Hnum, Hnum, Hnum, NULL };
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
   int X,Y;
   int f,wavef,card;
   char **hh;
   struct SCField **ff;
   
   WinGenInit();
   
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
	    if ((Lsamplerate!=samplerate) || (Lafmt!=afmt)) {
	       if (st=genAllWaveforms(&WAVEFORMS,&wavbufs,1,Lsamplerate*resolution,Lafmt)) {
	          move(COLS-1,0); endwin();
		  return(err_rpt(st,"Problem generating Internal waveforms."));
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

      if (c=='q' || c=='Q') { move(COLS-1,0); endwin();	return(c); }

      if (c=='r' || c=='R') {
	playsamples(-1);   /* synch left and right channels */
	continue;
      } 

      if ((c=='t' || c=='T') && (i=(TRKpairs[f]))>=0) {
	/* if there is an id value then it can track */
	n=!(GetOptionIndex(TRKvals[i]));
	SetOptionIndex(TRKvals[i],n);
	getyx(stdscr,Y,X); putfield(TRKvals[i]); move(Y,X);
      } else {
	f=doinput(c,ff,hh,&Finfo,f);
      }
      if ((i=TRKpairs[f])>=0 && GetOptionIndex(TRKvals[i])) { 
	scfeql(ff[i],ff[f]);
	getyx(stdscr,Y,X); putfield(ff[i]); move(Y,X);
      }
      refresh();
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
      return(err_rpt(n,"Problem generating Internal waveforms."));
   }
   
   Fwav1V.val.sp=WAVEFORMS;   Fwav2V.val.sp=WAVEFORMS;
   setwaveform(&Fwav1V,wf,WAVEFORMS);
   setwaveform(&Fwav2V,wf2,WAVEFORMS);
   
   n=0;   /* setup the correct number of Decimal points depending on resolution */
   if (resolution==10) n=1;
   else if (resolution==100) n=2;
   Ffrq1V.dp=Ffrq2V.dp=n; freq*=resolution; freq2*=resolution;
  
   tzset();              /* set timezone stuff up for dotime() function */
   
   if (vflg) {  /* ========= if vflg pause before going into ncurses =======*/
     printf("\n<Press Return to Continue>\n");
     getchar();
   }
  
   initscr();  /* ============== for here on in ncurses active ==============*/
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
   dofatt(TRKvals,att_h5);
 
   sprintf(VERS,VERSTR,sys,VERSION);
   dohdr(0,att_h1,att_h2,att_h3,hl,oh,sig,VERS);
   for (i=FsamP.row; i<Finfo2.row; i++) {
      mvputfstr(i,0,att_h5,"",80);
   }

   dofields(scrP); dofields(scrV);
   dofields(CHhdrs);
   dofields(CHvals);
   dofields(TRKvals);
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
   static int of1,of2;

   short int v,*vp,*wv1,*wv2;
   unsigned char *p,c,*w1,*w2;
   int i,n,ph,tof2,t;
  
   ph=(resolution*samplerate*(phase%360)+180)/360;
                                     /* phase diff of chan2 reduced */
                                     /* to 0-359 degrees then converted */
                                     /* to equiv offset into wave buffs */ 

   if (fd<0) { of2=0; return(of1=0); }     /* reset pointers */ 
   if (fragsize!=fr_size) {
      if (fr_size) free(frag);
      if ((frag=malloc(fragsize))==NULL) return(-1);
      fr_size=fragsize; of1=0; of2=0;
   }

   for (;;) {
      if ((i=getfreeobufs(fd))<=0) return(i);
      tof2=(of2+ph)%(samplerate*resolution);  /* add phase to current generator offset */
      if (afmt==AFMT_S16_LE) {
	 wv1=(short int *)(wavbufs[Fwav1V.adj]); 
	 wv2=(short int *)(wavbufs[Fwav2V.adj]);
	 if (stereo) {
	    for (i=0, vp=(short int *)frag; i<fragsamplesize; i++) {
/*	       t=(5000+(((long long)(wv1[of1]))*(long long)Gain))/(long long)10000; */
	       t=(500+(((int)(wv1[of1]))*Gain))/1000;
	       *vp++=(short int)((t>32767)?32767:((t<-32767)?-32767:t));
/*	       t=(5000+(((long long)(wv2[tof2]))*(long long)Gain2))/(long long)10000; */
	       t=(500+(((int)(wv2[tof2]))*Gain2))/1000;
	       *vp++=(short int)((t>32767)?32767:((t<-32767)?-32767:t));
	       of1=(of1+freq)%(samplerate*resolution);
	       tof2=(tof2+freq2)%(samplerate*resolution);
	    }
	 } else {
	    for (i=0, vp=(short int *)frag; i<fragsamplesize; i++) {
/*	       t=(((long long)(wv1[of1])*(long long)Gain)+
		  ((long long)(wv2[tof2])*(long long)Gain2)+10000)/(long long)20000; */
	       t=(((int)(wv1[of1])*Gain)+
		  ((int)(wv2[tof2])*Gain2)+1000)/2000;
	       *vp++=(short int)((t>32767)?32767:((t<-32767)?-32767:t));
	       of1=(of1+freq)%(samplerate*resolution);
	       tof2=(tof2+freq2)%(samplerate*resolution);
	    }
	 }
      } else {  /* afmt is 8 bit */
	 w1=(unsigned char *)(wavbufs[Fwav1V.adj]); 
	 w2=(unsigned char *)(wavbufs[Fwav2V.adj]);
	 if (stereo) {
	    for (i=0, p=frag; i<fragsamplesize; i++) {
/*	       t=128+(5000+((int)(w1[of1])-128)*Gain)/10000; */
	       t=128+(500+(((int)(w1[of1])-128)*Gain))/1000;
	       *p++=(unsigned char)((t>255)?255:((t<0)?0:t));
/*	       t=128+(5000+((int)(w2[tof2])-128)*Gain2)/10000; */
	       t=128+(500+(((int)(w2[tof2])-128)*Gain2))/1000;
	       *p++=(unsigned char)((t>255)?255:((t<0)?0:t));
	       of1=(of1+freq)%(samplerate*resolution);
	       tof2=(tof2+freq2)%(samplerate*resolution);
	    }
	 } else {
	    for (i=0, p=frag; i<fragsamplesize; i++) {
/*	       t=128+(((int)(w1[of1])*Gain)+
		      ((int)(w2[tof2])*Gain2)-256+10000)/20000; */
	       t=128+((((int)(w1[of1])-128)*Gain)+
		      (((int)(w2[tof2])-128)*Gain2)+1000)/2000;
	       /*  -256 + 10000 = -128-128+5000+5000 */
	       *p++=(unsigned char)((t>255)?255:((t<0)?0:t));
	       of1=(of1+freq)%(samplerate*resolution);
	       tof2=(tof2+freq2)%(samplerate*resolution);
	    }
	 }
      }   /* afmt==AFMT_S16_LE */
      of2=(tof2-ph+(samplerate*resolution))%(samplerate*resolution);  /* remove phase from generator offset */
      if (write(fd,frag,fragsize)<0) return(-1);
   }
   return(0);
}
