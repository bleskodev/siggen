/* wavfile.c
 * WAV File handling routines for reading/writing samples from buffers etc.
 * Jim Jackson    Aug 97
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson                 jj@franjam.org.uk
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

/* These functions are simplistic single threaded functions for handling
 * WAV files. A single WavValues structure is used to record WAV file
 * parameters such as samplerate, stereo/mono, and the afmt (8/16)
 * 
 * Typical uses......
 * 
 * Writing WAV files
 * 
 *     Open a FILE * for writing
 *     call fWAVwrhdr() with appropriate audio parameters. Don't worry
 *      if you don't know how many samples are to be written yet.
 *     call fWAVwrsamples() to write samples away to file. While doing this
 *      keep account of the number of samples you have written away.
 *     When finished writing samples call fWAVwrhdr() again but this time
 *      give it the correct number of samples parameter - the file will be 
 *      rewound and the correct headers (RIFF/WAV/DATA) will be written.
 * 
 * e.g.    f=fopen("sample.wav","W");
 *         if (f==NULL) { do error processing etc.....  }
 *         stereo=0;  / * mono data * /
 *         afmt=16;   / * 16 bit data * /
 *         samplerate=32000;  / * samplerate * /
 *         Ns=0;
 *         fWAVwrhdr(f,Ns,samplerate,afmt,stereo);
 *         while (samples_to_write) {
 *           fWAVwrsamples(f,buf,bufsize);
 *           Ns+=(bufsize>>(afmt==16));   / * if 16 bit divide by 2 to get
 *                                          * number of samples * /
 *         }
 *         fWAVwrhdr(f,Ns,samplerate,afmt,stereo);
 *         fclose(f);
 *
 * When writing RAW files then there must be a call to fWAVinit()
 *   with the sound parameters, then use the fWAVwrsamples() function.
 *   Of course there are no calls to fWAVwrhdr() 'cos RAW files don't have
 *   headers!
 * 
 * Reading WAV files
 * 
 *    Open wav file with the fWAVopen() function
 *    Use the fWAVsamplerate(), fWAVafmt(), fWAVstereo() and fWAVsamples()
 *      to access those parameters.
 *    Read the number of samples in the file and use them.
 *    close the file
 * 
 * e.g.   f=fWAVopen("samples.wav");
 *        if (f==NULL) { do error stuff......  }
 *        N=fWAVsamples();  / * get number of samples * /
 *        S=fWAVsamplerate();  / * get samplerate * /
 *        stereo=fWAVstereo();  / *  ditto stereo/mono status * /
 *        afmt=fWAVafmt();    / * ditto 8/16 bit * /
 *        for ( i=0; i<N; ) {
 *          n=fWAVrdsamples(f,buf,bufsize);
 *          i+=n;
 *          _do things with samples in buf_
 *        }
 *        fclose(f);
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "wavsubs.h"

struct WavValues *W=NULL;       /* fWAV* functions are single threaded
				 * and the call to fWAVwrhdr or fWAVopen
				 * can set this up with the audio params.
				 */

/* fWAVdebug(f)  write out state of struct WavValues *W to FILE *f
 */

fWAVdebug(f)
FILE *f;
{
  if (W==NULL) {
    fprintf(f,"struct WavValues *W is NULL\n"); return(0);
  }
  fprintf(f,"Setup struct WavValues.....\n");
  fprintf(f,"  %d %s, %s, samples/sec\n",W->samplerate,
	  (W->stereo)?"stereo":"mono",
	  (W->afmt==AFMT_U8)?"unsigned 8 bit":
	  ((W->afmt==AFMT_S16_LE)?"signed 16 bit, little endian":
	   "Unknown audio format"));
  fprintf(f,"  %d samples, %d Bytes per sec, %d Bytes per sample\n",
	  W->samples, W->Bps, W->Bypsam);
  return(0);
}

/* fWAVinit(Ns,samplerate,afmt,stereo)
 *   Initialises the struct WavValues *W with values derived from
 *   the calling parameters, so that other fWAVxxxx functions work
 * returns 0 if all ok, else returns -1 and errno is set.
 */

fWAVinit(Ns,samplerate,afmt,stereo)
int Ns,samplerate,afmt,stereo;
{
  int st;

  if ((W=Winit(W,Ns,samplerate,afmt,stereo))==NULL) { return(-1); }
  return(0);
}

/* fWAVopen(fn)    attempt to open file with name fn for reading 
 *    read WAV header and setup *W
 *    return FILE * of opened file
 *    return NULL if file doesn't exist or is not a WAV file.
 */

FILE *fWAVopen(fn)
char *fn;
{
   FILE *f;
   
   if ((f=fopen(fn,"r"))==NULL) return(NULL);
   if (W==NULL && (W=NewWavValues())==NULL) { 
      errno=ENOMEM; fclose(f); return(NULL);
   }
   errno=EINVAL;
   if ((isriff(f)==-1) || (errno=wavreadfmt(f,W))) {
      fclose(f); return(NULL);
   }
   return(f);
}

/* fWAVwrfile(f,bf,bfn,Ns,samplerate,afmt,stereo)
 *  write riff and wav hdr for Ns samples of appropriate format then
 *  Write Ns samples from buffer bf, of size bfn bytes, to file f
 */

fWAVwrfile(f,bf,bfn,Ns,samplerate,afmt,stereo)
FILE *f;
unsigned char *bf;
int bfn,Ns,samplerate,afmt,stereo;
{
  int st,n;
  
  if (fWAVwrhdr(f,Ns,samplerate,afmt,stereo)==-1) return(-1);
  
  n=Ns<<(afmt==16);      /* set n to number of samples in buffer */
  for (;;) {
    if (n<=bfn) { 
      if (n) st=wavwrite(f,W,bf,n); 
      break;
    }
    n-=bfn;
    if ((st=wavwrite(f,W,bf,bfn))==0) break;
  }
  return((st)?0:-1);
}

/* fWAVwrhdr(f,Ns,samplerate,afmt,stereo)
 *  write riff and wav hdrs for Ns samples of appropriate format to file f
 */

fWAVwrhdr(f,Ns,samplerate,afmt,stereo)
FILE *f;
int Ns,samplerate,afmt,stereo;
{
  int st;

  if ((W=Winit(W,Ns,samplerate,afmt,stereo))==NULL) { return(-1); }
  setendian();
  fflush(f);
  rewind(f);
  riffhdr(f,(Ns<<(afmt==16))+36);
  if (st=wavwritefmt(f,W)) { errno=st; return(-1); }
  return(0);
}

/* fWAVwrsamples(f,bf,N)  write bf of size N to FILE *f
 *    return number of samples written or 0 if error
 */

fWAVwrsamples(f,bf,N)
FILE *f;
unsigned char *bf;
int N;
{
  return(wavwrite(f,W,bf,N));
}

/* fWAVrdsamples(f,bf,N)  read bf of size N from FILE *f
 *    return number of samples read or 0 if error
 */

fWAVrdsamples(f,bf,N)
FILE *f;
unsigned char *bf;
int N;
{
  return(wavread(f,W,bf,N));
}

/*  writecyclic(fd,bf,bfn,Ns)   bf is a sample buffer of size bfn bytes.
 *      writecyclic writes Ns bytes from this buffer
 *      to fd, treating the buffer as circular.
 */

writecyclic(fd,bf,bfn,Ns)
int fd;
unsigned char *bf;
int bfn,Ns;
{
  int i;
  
  for (;;) {
    if (Ns<=bfn) { return(write(fd,bf,Ns)); }
    Ns-=bfn;
    if (write(fd,bf,bfn) < 0) return(-1);
  }
}

/*
 *  playloop(fd,bf,bfn,N)   bf is a sample buffer of size bfn bytes.
 *   play the buffer cyclically (in a loop) in chunks of N
 *    
 */

playloop(fd,bf,bfn,N)
int fd;
unsigned char *bf;
int bfn;
int N;
{
  int i;
  unsigned char *p;
  
  for (;;) {
    for ( p=bf,i=0; i<= (bfn-N); i+=N) { 
      if (write(fd,p+i,N) < 0) return(0);
      /* check here if need to supend output or handle keypresses etc*/
    }
    if ((i!=bfn) && (write(fd,p+i,bfn-i)<0)) return(0);
  }
}

/* fWAVsamplerate()   return samplerate  of opened file.
 * fWAVafmt()         return afmt        of opened file.
 * fWAVstereo()       return stereo mode of opened file.
 * fWAVsamples()      return Number of samples in opened file.
 *     or -1 if non opened
 */

fWAVsamplerate()
{
   if (W==NULL) return(-1);
   return(W->samplerate);
}

fWAVafmt()
{
   if (W==NULL) return(-1);
   return(W->afmt);
}

fWAVstereo()
{
   if (W==NULL) return(-1);
   return(W->stereo);
}

fWAVsamples()
{
   if (W==NULL) return(-1);
   return(W->samples);
}
