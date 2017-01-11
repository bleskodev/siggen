/* generator.c
 * The main DSP signal generation functions plus some misc. functions
 * Jim Jackson     Linux Version
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

/* History...................
 * 
 * 11May08  Bug in mksweep with x10 or x100 options and wide ranges
 *          caused int overflow problems and corrupt waveforms
 * 18Jun98  altered to make it compile on systems without soundcard
 *          or dsp support.
 * 18Mar97  Moved maketone() into here
 * 17Mar97  added a mksweep() function - but as a separate function
 *          is not available from the generate() function
 * 09Feb97  made buffer mixing routine take account of gains, in order
 *          to maintain maximum absolute sample sizes, while maintaining
 *          relative sizes of mixed samples.
 * 07Feb97  discovered and fixed bug in generating sin/cos waves
 *          which are generated over very many cycles. This caused w
 *          (the radians counter) to get  >> 2*PI and meant that
 *          the sin calculation got slightly inaccurate causing a mismatch
 *          when the buffer is looped. Fix is to check if w > 2*PI and
 *          if so sub 2*PI. Now the beginning and end of the buffer match
 * 19Jan97  Added cosine - sin with 90 degrees offset
 * 02Jan97  Triangle generation sorted.
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

/* A one second's worth buffer of samples is generated (samplerate samples)
 * for 8 bit mono this is bytes, for 16 mono this is doubled
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "config.h"

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.141592653589793
# endif
#endif

#define chkword(a,b) ((n>=a)&&(strncasecmp(p,b,n)==0))

extern int vflg;   /* within this package vflg must be >=2 for diagnostics */

#define VERBOSE (vflg>1)

/* mk8bit(bf,ip,N)  convert N 16 bit samples from *ip to 8 bit samples
 *                  in bf
 */

mk8bit(bf,ip,N)
unsigned char *bf;
short int *ip;
int N;
{
   unsigned char *p;
   
   for (p=bf; N--; ) {
      *p++=(unsigned char)(((*ip++)>>8)+128);
   }
   return(0);
}

/* mixplaybuf(a,b,c,N,afmt)   mix N samples from buffers b and c into 
 *     sound play buffer a
 *     afmt defines format of samples
 */

mixplaybuf(a,b,c,N,afmt)
unsigned char *a,*b,*c;
int N,afmt;

{
   int i,word;

   word=(afmt==AFMT_S16_LE);
   for (i=0; i<N; i++) {
      *a++=*b++; if (word) *a++=*b++;
      *a++=*c++; if (word) *a++=*c++;
   }
}

/* chanmix(a,b,Nb,c,Nc,N,afmt)  digitally mix N samples from buffers b and c 
 *            into buffer a.  Nb and Nc are the relative amplitudes of
 *            the two input channels as percentages of fullscale.
 *            afmt defines format of samples
 */

chanmix(a,b,Nb,c,Nc,N,afmt)
unsigned char *a,*b;
int Nb;
unsigned char *c;
int Nc,N,afmt;

{
   int i,u,v,w;
   short int *p,*q,*r;

   Nc+=Nb; Nb=Nc>>1;
   if (afmt==AFMT_S16_LE) {   /* 16 bit mixing...... */
      p=(short int *)b; q=(short int *)c; r=(short int *)a;
      for (i=0; i<N; i++) {
	 u=((*p++)+(*q++))*100;
	 *r++=(short int)((u+((u<0)?-Nb:Nb))/Nc);
      }
   } else {                   /* 8 bit mixing....... */
      for (i=0; i<N; i++) {
	 u=((int)(*b++) + (int)(*c++) - 256)*100;
	 *a++=(unsigned char)((u+((u<0)?-Nb:Nb))/Nc + 128);
      }
   }
}

/* do_antiphase(a,b,N,afmt)   create an antiphase version of sample b in
 *                            a. afmt defines 8/16bit, N is number of samples
 */

do_antiphase(a,b,N,afmt)
unsigned char *a,*b;
int N,afmt;
{
   int i,word;

   if (afmt==AFMT_S16_LE) {
      short int *p=(short int *)a;
      short int *q=(short int *)b;
      for (i=0; i<N; i++) {
	 *p++=-*q++;
      }
   }
   else {
      for (i=0; i<N; i++) {
	 *a++=(unsigned char)(256-(int)(*b++));
      }
   }
}

/* maketone(wf,fr,N,G,afmt)   generate a tone of frequency fr, samplerate N/sec,
 *   waveform wf, and digital Gain G, does a malloc to get a new tone buffer.
 */

unsigned char *maketone(wf,fr,N,G,afmt)
char *wf;
int fr,N,G,afmt;

{
   unsigned char *bf;
   int n,bn;
   
   bn=N<<(afmt==AFMT_S16_LE);
   if ((bf=(unsigned char *)malloc(bn))==NULL) return(NULL);
   n=generate((fr)?wf:"OFF",bf,bn,fr,G,N,-1,afmt);
   return((n)?bf:NULL);
}

/* mksweep(bf,bfn,FMbuf,wavbuf,Fmin,Fmax,N,afmt)
 *    create a swept waveform in bf, size bfn, of format afmt. 
 *    Sweeping waveform is in FMbuf - THIS is 16 bit samples!!!!
 *    Swept waveform is in wavbuf
 *    the Freq range is Fmin to Fmax Hz.   Current Samplerate is N/sec
 */

mksweep(bf,bfn,FMbuf,wavbuf,Fmin,Fmax,N,afmt)
char *bf;
int bfn;
char *FMbuf;
char *wavbuf;
int Fmin,Fmax;
unsigned int N,afmt;
{
   int i,D,F,dF,x;
   short int *vp,*wb,*fmb;
   unsigned char *p;

   F=(Fmin+Fmax+1)/2;    /* calc centre freq */
   dF=Fmax-F;            /* scale factor     */
   D=32767;              /* adj to prevent int overflow */
   while( dF>32767 ) { dF=(dF+1)/2; D=(D+1)/2; }

   if (VERBOSE) {
      printf("Generating sweep waveform from %d Hz to %d Hz\n",Fmin,Fmax);
      printf("Centre Freq. %d Hz, sweep scaling %d/%d\n",F,dF,D);
   }
   fmb=(short int *)FMbuf;
   if (afmt==AFMT_S16_LE) {
      wb=(short int *)wavbuf;
      for (x=i=0,vp=(short int *)bf; i<N; i++) {
	 *vp++=wb[x];
	 x=(N+F+x+(((int)(*fmb++)*dF)/D))%N;
      }
   } else if (afmt==AFMT_U8) {
      for (x=i=0,p=bf; i<N; i++ ) {
	 *p++=wavbuf[x];
	 x=(N+F+x+(((int)(*fmb++)*dF)/D))%N;
      }
   } else {
      i=0;
   }
   return(i);  /* i=N for ok generation else 0 */
}

/* mkcos(bf,bfn,freq,A,N,R,afmt)
 * cosine wave is 90 degrees ahead of sine wave
 */

mkcos(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   return(mksin(bf,bfn,freq,A,N,90+R,afmt));
}

/* mksin(bf,bfn,freq,A,N,R,afmt)
 * sine wave of freq Hertz played at N samples/sec, scaled by A/100,
 * starting at R degreees. i.e. for sin R=0, for cos R=90
 */

mksin(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   unsigned i,n,h,nc;
   float w,wi,sc,pi2;
   unsigned char c,*p;
   short int v,*vp;
   int t;
   
   /* This looks a little complicated, but ........
    * we determine the minimum number of samples to generate
    * so that we have an exact whole number of
    * wavelengths. Then we can simply concatenate multiple copies of
    * these samples to generate the output - the join will be accurate.
    * It's worth noting that we could simply generate N samples.
    * This is guaranteed to generate an exact numbers of wavelengths
    * because we only allow integer definition of the required freq (Hz)
    * However in most cases this is a little more optimum and quicker.
    * 
    * w (radians) is the basic loop count. However sin(w) when w>>2*PI
    * becomes inaccurate, so in the increment loop we check if w>2*PI
    * and if so subtract 2*PI so that the value is kept low for each cycle
    */
   
   n=N/(h=hcf(N,freq));  /* n = N/hcf(N,freq) is number of samples to create */
   nc=freq/h;            /* nc is number of wavelengths generated    */
    
   if (VERBOSE) {
      printf("%dHz at a sampling rate/sec of %d, scaled by %d/100\n",
	     freq,N,A);
      printf("%d samples for %d cycle(s).\n",n,nc);
   }  
   pi2=PI+PI;
   w=((float)(R)*PI)/180.0;
   wi=((float)nc*2*PI)/(float)n;
   if (afmt==AFMT_U8) {
      sc=(float)A*127.0/100.0;
      for (i=0,p=bf; i<n; i++, w+=(wi-((w>pi2)?pi2:0)) ) {
	 t=128+rint(sc*sin(w));
	 *p++=(t>255)?255:((t<0)?0:t);
      }
   } else if (afmt==AFMT_S16_LE) {
      sc=(float)A*32767.0/100.0;
      for (i=0,vp=(short int *)bf; i<n; i++, w+=(wi-((w>pi2)?pi2:0)) ) {
	 t=rint(sc*sin(w));
	 *vp++=(t>32767)?32767:((t<-32767)?-32767:t);
      }
   } else return(0);

   return(n);
}

/* mksquare(bf,bfn,freq,A,N,R,afmt)
 *  square wave - calls mkpulse with 50% mark/space ratio, scaled by A/100
 */

mksquare(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   return(mkpulse(bf,bfn,freq,A,N,50,afmt));
}

/* mkpulse(bf,bfn,freq,A,N,R,afmt)
 * rectangular wave of freq Hertz played at N samples/sec,
 * mark/space ratio R (%)
 * scaled by A/100, 50% is equal mark/space, <50% is mark<space and vice-versa
 */

mkpulse(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   unsigned int i,n,nc,h;
   unsigned char c,*p;
   short int v,*vp;
   int t,b;
   
   n=N/(h=hcf(N,freq));  /* n = N/hcf(N,freq) is number of samples to create */
   nc=freq/h;            /* nc is number of wavelengths generated    */

   if (VERBOSE) { 
      printf("%dHz at a sampling rate/sec of %d, scaled by %d/100\n",
	     freq,N,A);
      printf("%d samples for %d cycle(s).\n",n,nc);
   }  
   R*=n;    /* scale the Ratio */
            /* we run i from 0 to n, take x=(i*nc)%n which gives result */
            /* between 0 and n and then check if x/n < R/100   or */
            /* if x*100 < R*n     hence the scale for R */
   if (afmt==AFMT_U8) {
      t=A*127/100; t=128+((t>127)?127:t);
      b=A*128/100; b=128-((b>128)?128:b);
      for (i=0,p=bf; i<n; i++) {
	 *p++=((((i*nc)%n)*100)<R)?t:b;
      }
   } else if (afmt==AFMT_S16_LE) {
      t=A*32767/100; t=(t>32767)?32767:t;
      for (i=0,vp=(short int *)bf; i<n; i++) {
	 *vp++=(((((i*nc)%n)*100)<R)?t:-t);
      }
   } else return(0);

   return(n);
}

/* mksawtooth(bf,bfn,freq,A,N,R,afmt)
 * sawtooth wave of freq Hertz, played at N samples, scaled by A/100
 */

mksawtooth(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned int freq,A,N;
int R,afmt;
{
   int i,n,nc,h,t,b,d;
   unsigned char *p;
   short int *vp;
   
   n=N/(h=hcf(N,freq));  /* n = N/hcf(N,freq) is number of samples to create */
   nc=freq/h;            /* nc is number of wavelengths generated    */

   if (VERBOSE) { 
      printf("%dHz at a sampling rate/sec of %d, scaled by %d/100\n",
	     freq,N,A);
      printf("%d samples for %d cycle(s).\n",n,nc);
   }  
   /* for sawtooth we use (i*nc)%n to give a value upto n-1       */
   /* then adjust by subtracting n/2 to give a range of -n/2 to +n/2 */
   /* we then scale to be in the correct range and clip if necessary */
   d=n/2;
   if (afmt==AFMT_U8) {
      t=2*A*127/100;
      for (i=0,p=bf; i<n; i++) {
	 b=((i*nc)%n-d)*t/n;
	 *p++=(unsigned char)(128+((b>127)?127:((b<-128)?-128:b)));
      }
   } else if (afmt==AFMT_S16_LE) {
      t=2*A*32767/100; 
      for (i=0,vp=(short int *)bf; i<n; i++) {
	 b=((long long int)((i*nc)%n-d)*(long long int)t)/n;
	 *vp++=(short int)((b>32767)?32767:((b<-32767)?-32767:b));
      }
   } else return(0);

  return(n);
}

/* mktriangle(bf,bfn,freq,A,N,R,afmt)
 * triangle wave of freq Hertz, played at N samples/sec, scaled by A/100
 */

mktriangle(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned int freq,A,N;
int R,afmt;
{
   int i,n,nc,t,d,b,h;
   short int *vp;
   unsigned char *p;
   
   n=N/(h=hcf(N,freq));   /* n = N/hcf(N,freq) is number of samples to create */
   nc=freq/h;             /* nc is number of wavelengths generated    */
   
   if (VERBOSE) { 
      printf("%dHz at a sampling rate/sec of %d, scaled by %d/100\n",
	     freq,N,A);
      printf("%d samples for %d cycle(s).\n",n,nc);
   }  
   /* for sawtooth we use  b=(i*nc)%n to give a value upto n-1         */
   /* if the value is < n/2 ok for upramp else set to n-b to down ramp */
   /* then adjust by subtracting n/2 to give a range of -n/2 to +n/2   */
   /* we then scale to be in the correct range and clip if necessary   */
   d=n/2;
   if (afmt==AFMT_U8) {
      t=2*A*127/100;
      for (i=0,p=bf; i<n; i++) {
	 b=(i*nc*2)%(2*n);
	 b=(((b<=n)?b:(2*n-b))-d)*t/n;
	 *p++=(unsigned char)(128+((b>127)?127:((b<-128)?-128:b)));
      }
   } else if (afmt==AFMT_S16_LE) {
      t=2*A*32767/100; 
      for (i=0,vp=(short int *)bf; i<n; i++) {
	 b=(i*nc*2)%(2*n);
	 b=((long long int)(((b<=n)?b:(2*n-b))-d)*(long long int)t)/n;
	 *vp++=(short int)((b>32767)?32767:((b<-32767)?-32767:b));
      }
   } else return(0);
   return(n);
}

/* mknoise(bf,bfn,freq,A,N,R,afmt)
 * make noise - freq is ignored. samples scaled by A/100
 * All bf is filled with pseudo random values
 */

mknoise(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   unsigned int i,n,r;
   unsigned char c,*p;
   short int *vp;
   short int v;
   int t,b;
   
   n=bfn;                 /* number of samples to generate - fill bf */
   if (afmt==AFMT_S16_LE) n/=2; /* if 16 bit samples = buffsize/2 */
   
   if (VERBOSE) { 
      printf("Noise, %d samples/sec\n%d samples.\n",N,n);
   }  

   srand(time(NULL));
   if (afmt==AFMT_U8) {
      for (i=0,p=bf; i<n; i++) {
	 *p++=(c=(rand()>>8)&255);
      }
   } else if (afmt==AFMT_S16_LE) {
      t=A*32767/100; t=(t>32767)?32767:t;
      for (i=0,vp=(short int *)bf; i<n; i++) {
	 *vp++=(short int)(((rand()>>8)&0xFF)+(rand()&0xFF00));
      }
   } else return(0);
   return(n);
}
  
/* mknull(bf,bfn,freq,A,N,R,afmt)
 * make quiet - fill the buffer with  zero values
 */

mknull(bf,bfn,freq,A,N,R,afmt)
char *bf;
int bfn;
unsigned freq,A,N;
int R,afmt;
{
   unsigned int i,n,r;
   unsigned char c,*p;
   short int *vp;
   short int v;
   int t,b;
   
   n=bfn;                 /* number of samples to generate - fill bf */
   if (afmt==AFMT_S16_LE) n/=2; /* if 16 bit samples = buffsize/2 */
   
   if (VERBOSE) { 
      printf("Quiet, %d samples/sec\n%d samples.\n",N,n);
   }  

   if (afmt==AFMT_U8) {
      for (i=0,p=bf; i<n; i++) {
	 *p++=(unsigned char)128;
      }
   } else if (afmt==AFMT_S16_LE) {
      for (i=0,vp=(short int *)bf; i<n; i++) {
	 *vp++=(short int)0;
      }
   } else return(0);
   return(n);
}
  
/* generate()  generate samples of waveform wf, in buf, at frequency fr,
 *             for a playing rate of S samples/sec. R is a further param
 *             if needed for pulse or sweep. buf is size N bytes.
 *             format required is afmt and samples are scaled by A/100
 *             N should be a factor of S, or, more usually, an exact multiple
 *             of S. Usually N=S for 8 bit samples or =2S for 16 bits
 *  return number of samples in buffer, or 0 if there is an error.
 */

static char *WavNAMES[]= {
      "sine","cosine","square","triangle","sawtooth","pulse",
      "noise", "off", NULL
   };
static int (*WavFuncs[])()= {
      mksin, mkcos, mksquare, mktriangle, mksawtooth, 
      mkpulse, mknoise, mknull, NULL
   };
static int WavDefr[]={ 0,0,0,0,0,10,0,0,0 };
      

generate(wf,buf,N,fr,A,S,R,afmt)
char *wf;
char *buf;
unsigned int N;
unsigned int fr,A;
unsigned int S;
int R,afmt;
{
   int i,h,nc,n;
   char *p;

   if (wf==NULL || fr>(N/2)) return(0);

   n=strlen(p=wf);
   for (i=0; WavNAMES[i]!=NULL; i++) {
      if (chkword(2,WavNAMES[i])) {
	 if (VERBOSE) {
	    fputs(wf,stdout); fputs(" matches generator function : ",stdout);
	    fputs(WavNAMES[i],stdout); fputs("\n",stdout);
	 }
	 if (R==-1) R=WavDefr[i];
	 if ((n=(*(WavFuncs[i]))(buf,N,fr,A,S,R,afmt))==0) return(0);
   /* put any common post-processing here */
   /*   here we fill the full N byte buf up with h (N/n) basic samples */
	 nc=(afmt==AFMT_S16_LE)?(n<<1):n ;
	 for (p=buf+nc,i=1,h=N/nc; i<h; i++) {
	    memcpy(p,buf,nc); p+=nc;
	 }
	 return(n*h);
      }
   }
   return(0);
}

/* getWavNames()  return pointer to an array of strings containing
 *         the names for the waveforms that we can generate
 */

char **getWavNames()
{
  return(WavNAMES);
}

/* genAllWaveforms(Wfs,bufs,F,samplerate,afmt)   generate all possible
 *    waveforms at freq F and setup (*Wfs)[] with waveform names and (*bufs)[]
 *    with the actual samples.
 */

genAllWaveforms(Wfs,bufs,F,samplerate,afmt)
char **Wfs[];
char **bufs[];
int F;
int samplerate;
int afmt;
{
  int i,st,n,N;
   
  if ((*Wfs)==NULL) *Wfs=WavNAMES;
  else {
    for (n=0; WavNAMES[n]!=NULL; n++) { (*Wfs)[n]=WavNAMES[n]; }
    (*Wfs)[n]=NULL;
  }
  for (n=0; WavNAMES[n]!=NULL; n++) { }  /* count the no. of waveforms */
  
  if ((*bufs) == NULL) {
    if ((*bufs=(char **)malloc((n+1)*sizeof(char *)))==NULL) return(ENOMEM);
    for (i=0; i<=n; i++) { (*bufs)[i]=NULL; }
  }
  N=samplerate<<(afmt==16);
  for (i=0; i<n ; i++) {
    if ((*bufs)[i]!=NULL) free((*bufs)[i]);
    if (((*bufs)[i]=(char *)malloc(N))==NULL) return(ENOMEM);
    if ((st=generate((WavNAMES)[i],(*bufs)[i],N,F,100,samplerate,-1,afmt))==0) 
	return(st);
  }
  return(0);
}

