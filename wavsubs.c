/* wavsubs.c
 * Functions for opening, closing, reading, writing WAV files.
 * Jim Jackson 
 */
/*
 * Copyright (C) 1998-2008 Jim Jackson           jj@franjam.org.uk
 */
/*
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

/* HISTORY
 * 
 *   9 Mar 06   RAW write bug fixed
 *   2 Dec 98   Started
 */

/* Functions in this file........
 * 
 * W is a struct WavValues *
 * f is a FILE *
 *
 * Winit()              Set values in WavValues* or if NULL create a new
 *                      struct and set values
 * setendian()          tests whether running on a big or little endian machine
 * riffhdr(f,N)         write riff header for file of size N bytes
 * isriff(f)            return TRUE if f is a RIFF format file - skips 1st hdrs
 * wavreadfmt(f,W)      read wav filehdr set Wav file values in *W
 *                       returning 0 for ok else errno
 * wavwritefmt(f,W)     write wav filehdr from values in *W
 * wavread(f,W,buf,N)   read samples into buffer buf of size N bytes
 *                        return 0 on error (errno refers) or number of samples
 * wavwrite(f,W,buf,N)  write wav samples from buffer buf of size N bytes
 * fprintfmt(f,W)       writes out formatted details from W in ASCII to f
 * 
 * Macros provided in wavsubs.h for struct WavValues .......
 * 
 * NewWavValues()         call malloc to get a struct WavValues, return ptr
 * GetWavSamplerate(W)    get the samplerate in *W
 * SetWavSamplerate(W,S)  set samplerate S in *W
 * GetWavStereo(W)        get where stereo samples, 1=stereo, 0=mono
 * SetWavStereo(W,S)      set stereo/mono S in *W
 * GetWavAFmt(W)          get the Audio Format from *W
 * SetWavAFmt(W,F)        set the Audio Format F in *W
 * GetWavSamples(W)       get the number of samples in this chunk
 * SetWavSamples(W,N)     set the number of samples S in *W
 * 
 * IsWavStereo(W)         TRUE if *W has stereo data
 * IsWavMono(W)           TRUE if *W has mono data
 * IsWav16bit(W)          TRUE if *W has 8 bit unsigned data
 * IsWav8bit(W)           TRUE if *W has 16 bit signed, little endian data
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "wavsubs.h"

int ISBIGENDIAN=0;      

struct WavValues *Winit(W,Ns,samplerate,afmt,stereo)
struct WavValues *W;
int Ns,samplerate,afmt,stereo;
{
  if (W==NULL && (W=NewWavValues())==NULL) { errno=ENOMEM; return(NULL); }
  W->samplerate=samplerate;
  W->afmt=afmt;
  W->stereo=stereo;
  W->Bps=samplerate<<(stereo+(afmt==16));
  W->Bypsam=(afmt==8)?1:2;
  W->samples=Ns;
  return(W);
}

setendian()
{
  short int *v,i;
  
  i=10; v=&i;
  ISBIGENDIAN=(*((unsigned char *)v)!=10);
}

/* isriff(f)     read FILE *f and check that it is a RIFF format file.
 *               And is a WAVE chunk.
 *               if so then it returns the length of the file.
 *               FILE *f is left pointing to the WAVE section
 *               if not it returns -1
 */

isriff(f)
FILE *f;
{
  unsigned char bf[4];
  int n,l;
  
  n=fread(bf,1,4,f);     /*  Read the 4 chars "RIFF"  */
  if ( n!=4 || strncmp(bf,"RIFF",4) ) return(-1);
  n=fread(bf,1,4,f);     /*  Read a little endian 32 bit integer */
  if (n!=4) return(-1);
  l=(*bf) + (bf[1]<<8) + (bf[2]<<16) + (bf[3]<<24) ;
  n=fread(bf,1,4,f);     /*  Read the 4 chars "RIFF"  */
  if ( n!=4 || strncmp(bf,"WAVE",4) ) return(-1);
  return(l);
}

/* wavnextchunk(f,p)  read next WAVE chunk from FILE *f, put name of chunk
 *                    in char *p, and retunr length of chunk.
 *  on error return -1
 */

wavnextchunk(f,p)
FILE *f;
char *p;
{
  int n;
  unsigned char bf[4];
  
  n=fread(p,1,4,f);     /*  Read the 4 char name of this chunk  */
  if (n!=4) return(-1);
  n=fread(bf,1,4,f);     /*  Read a little endian 32 bit integer */
  if (n!=4) return(-1);
  return((*bf) + (bf[1]<<8) + (bf[2]<<16) + (bf[3]<<24));
}

/* findwavchunk(f,s)   find next WAVE file chunk with name "s"
 *                     when found return length of chunk.
 *                     f is positioned at start of chunk.
 *   return -1 if not found
 */

findwavchunk(f,s)
FILE *f;
char *s;
{
  int n,i;
  unsigned char bf[130];
  
  for ( ; ; ) {
    n=wavnextchunk(f,bf);
    if (n<0) return(-1);
    if (strncmp(bf,s,4)==0) return(n); /* if correct chunk return size */
                                       /* otherwise skip this chunk    */
    for ( ; n>0; n-=i) {       /* skip the data for this chunk - to next */
      i=(n>128)?128:n;
      fread(bf,1,i,f);
    }
  }
}

/* wavreadfmt(f,W)   read a WAVE header fmt chunk from file stream f
 *                   set sample parameters in *W
 * return -1 if EOF found immediately. Otherwise return an error.
 */

wavreadfmt(f,W)
FILE *f;
struct WavValues *W;
{
  unsigned char bf[130];
  int n,HL;
  
  HL=findwavchunk(f,"fmt ");
  if (HL<0) return((feof(f))?-1:ferror(f));
  if (HL<16 || HL>128) return(EINVAL); /*  "fmt " header length must be 16 or more */
  n=fread(bf,1,HL,f);                  /* read fmt block */
  if (n!=HL) return((feof(f))?-1:ferror(f));
  if (n=Wconvfmt(bf,W)) return(n);
  HL=findwavchunk(f,"data");
  if (HL>0) {
    W->samples=(HL>>((W->afmt)!=8)); return(0);
  }
  return((feof(f))?-1:ferror(f));
}

Wconvfmt(p,W)
unsigned char *p;
struct WavValues *W;
{
  int n;

  n=(*p) + (p[1]<<8); p+=2;
  if (n!=1) return(EINVAL);     /*  PCM data  is  1  */
  /*  no. of chans - convert to stereo/mono ... */
  W->stereo=(*p) + (p[1]<<8) - 1; p+=2;
  /*  samplerate ...... */
  W->samplerate=(*p)+(p[1]<<8)+(p[2]<<16)+(p[3]<<24); p+=4;
  /*  bytes per sec .... */
  W->Bps=(*p)+(p[1]<<8)+(p[2]<<16)+(p[3]<<24); p+=4;
  /*  sample size in bytes ...... */
  W->Bypsam=(*p) + (p[1]<<8); p+=2;
  /*  bits per sample....... */
  W->afmt=(*p) + (p[1]<<8); p+=2;   /* same as afmt :-) */
  return(0);
}

/* riffhdr(f,N)   write a RIFF file header to f for size N bytes
 *                return 0 if all ok else retrun errcode
 */

riffhdr(f,N)
FILE *f;
unsigned int N;
{
  unsigned char bf[8],*p;
  
  p=bf;
  *p++='R'; *p++='I'; *p++='F'; *p++='F';
  *p++=(N)&0xFF;  *p++=(N>>8)&0xFF;  *p++=(N>>16)&0xFF;  *p++=(N>>24)&0xFF;
  if (fwrite(bf,1,8,f)!=8) return(errno);
  return(0);
}

/* wavwritefmt(f,W)   write a WAVE header fmt chunk to file stream f
 *                   from sample parameters in *W
 * return 0 if all ok else return error code.
 */

wavwritefmt(f,W)
FILE *f;
struct WavValues *W;
{
  unsigned char bf[36],*p;
  int n,Bps;
    
  strncpy(p=bf,"WAVEfmt ",8); p+=8;
  *p++=16; *p++=0; *p++=0; *p++=0;    /* write header length 16 */
  *p++=1; *p++=0;                     /* PCM data  has value 1  */
  *p++=W->stereo+1; *p++=0;           /* Number of channels     */
  n=W->samplerate;                    /* write samplerate       */
  *p++=(n)&0xFF;  *p++=(n>>8)&0xFF;  *p++=(n>>16)&0xFF;  *p++=(n>>24)&0xFF;
  n=n<<(W->stereo+(W->afmt!=8));      /* Bytes per sec          */
  *p++=(n)&0xFF;  *p++=(n>>8)&0xFF;  *p++=(n>>16)&0xFF;  *p++=(n>>24)&0xFF;
  *p++=W->Bypsam; *p++=0;             /* Number of bytes per sample */
  *p++=W->afmt; *p++=0;               /* number of bits per sample */
  /* now data section */
  *p++='d'; *p++='a'; *p++='t'; *p++='a';  /* "data" section   */
  n=W->samples<<(W->afmt!=8);         /* amount of data in bytes  */
  *p++=(n)&0xFF;  *p++=(n>>8)&0xFF;  *p++=(n>>16)&0xFF;  *p++=(n>>24)&0xFF;
  
  if (fwrite(bf,1,36,f)!=36) return(errno);
  return(0);
}

/* wavread(f,W,buf,N)   read samples into buffer buf of size N bytes
 *                        return 0 on error (errno refers) or number of samples
 */

wavread(f,W,buf,N)
FILE *f;
struct WavValues *W;
unsigned char *buf;
int N;
{
  int s,i,n;
  unsigned char *p;
  short int *v;
  
  n=fread(buf,1,N,f);
  if (IsWav8bit(W) || (n==0)) return(n);
  if (ISBIGENDIAN) {
    for (v=(short int *)buf, p=buf, i=0; i<n; i+=2, v++, p+=2) { 
      *v=*p+(p[1]<<8);        /* this should work for either little or big 
			       *  endian machines
			       */
    }
  }
  return(n/2);
}

/* wavwrite(f,W,buf,N)   write samples from buffer buf of size N bytes
 *               return 0 on error (errno refers) or no. of samples written
 */

wavwrite(f,W,buf,N)
FILE *f;
struct WavValues *W;
unsigned char *buf;
int N;
{
  int s,i,n;
  unsigned char *p,c;
  short int *v;
  
  if (N<=0) return(0);
  if (ISBIGENDIAN && IsWav16bit(W)) {
    for (v=(short int *)buf, p=buf, i=0; i<N; i+=2, v++, p+=2) { 
      c=*p; *p=*(p+1); *(p+1)=c;  /* swap byte order */
    }
  }
  n=fwrite(buf,1,N,f);
  return((IsWav16bit(W))?n/2:n);
}

fprintfmt(f,W)
FILE *f;
struct WavValues *W;
{
  double x;
  
  fprintf(f," fmt   samplerate %5d : bytes/sec %6d : chan(s) %d : bits %2d : byte(s) %d\n",
	  W->samplerate,W->Bps,(W->stereo)+1,W->afmt,W->Bypsam);
  if (W->samplerate)
      x=(double)((W->samples)>>W->stereo)/(double)(W->samplerate);
  else
      x=0;
  fprintf(f," data  samples    %5d : duration %7.2f secs.\n",
          W->samples,x);
  fflush(f);
}
