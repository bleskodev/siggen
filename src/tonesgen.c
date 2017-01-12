/* tonesgen.c
 * Parsing and playing of tones commands for "tones" program
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
 * 18May99  Fixed mixing bug, and added a non-auto scale mode, where
 *          mixing is absolute and it is upto the user to make sure
 *          mixing doesn't cause the final output to clip by going above
 *          0dB level.
 * 13May99  Added Mark E. Shoulson's code to allow attenuation factors
 *          in dB - i.e. 1000@-10   means 1KHz at -10dB
 * 23Feb98  made all samples be 16 bit, with 8 bit conversion when writing
 *          to final audio buffer.
 * 20Feb98  Added recognition of musical notes format e.g. G3 for G
 *          in 3rd octave, A#2 for A# in second octave. Octave '0'
 *          runs from C at 33Hz to C at 65Hz
 *  8Sep97  split tones into 2 parts - this is the tones generation stuff
 *          tones.c is all the preamble etc.
 *
 * Problems:
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "config.h"
#include "wavsubs.h"

#define CHAN_SEPERATOR(X) (X==',')
#define ATT_START(X) (X=='@')
 
/* defines the frequencies for basic musical notes - these are 
 * integers storing the bottom octave freqs x 1000
 */
/*              A      A#     B      B#     C      C#     D      D#    */
int Notes[]={ 55000, 58270, 61735, 32703, 32703, 34649, 36709, 38892,
              41204, 43654, 43654, 46250, 49000, 51913, };
/*              E      E#     F      F#     G      G#  */
/* YEAH I know there is no E# and B# but by equiv'ing them to
 * frequencies for F and C it makes decoding from the table simpler.
 * Index into table is  twice the letter-'A' and add one if a '#'
 * This means that E# gets decoded but produces F
 */

/* We hold data for a number of channels (voices or generators) in arrays
 * indexed by the channel number. This is done so that we can preserve 
 * waveform integrity when we have to generate long tones over several
 * fragment buffers! (see the doTomeCmd() function)
 * Each generator has an assigned waveform, names and 1Hz samples
 * and a pointer into the sample space of the next sample to play.
 * 
 * The arrays get initialised by init_samples(), and updated when waveforms
 * get changed. New waveforms can be specified for all channels,
 * or on channel by channel basis.
 */

char **WaveFormName=NULL;   /* array of Current waveform name          */
char **WaveForm=NULL;       /* array of Current waveform samples       */
int  *CurSample=NULL;       /* array of indices of next samples in WaveForm */

char **WavForms=NULL;       /* Array of valid names for waveforms   */
char **WavBufs=NULL;        /* 1 Hz samples of all valid waveforms  */
char *Silence;              /* Silence waveform samples       */

/* These values are setup by a call to init_samples() - see below. */

int samplerate=0,                /* Samples/sec        */
    afmt=-1,                     /* Format of samples, 8bit or 16bit */
    channels=0,                  /* number of generators */
    stereo=-1;                   /* Mono or stereo     */
double def_db=0;
int BuffSize=0;                  /* size of buffer to use          */

int FD;                          /* File Descriptor for writing samples to */
FILE *FO;                        /* and FILE * */

/* --------------------------------------------------------- */

short int *Buff;                 /* Actual sample buffer but in ints
				    even for 8 bit */
int Buffsamples=0;               /* number of samples per buffer   */

unsigned int samples=0;          /* count of number of samples written
				  * since init_samples() called 
				  * returned by getsamples()   */

unsigned int Tms;                /* period in millisecs for each tone */
unsigned int Nt;                 /* number of samples for Tms         */

/* --------------------------------------------------------- */

int GenErr=0;             /* like errno, records status of last tonegen 
			   *  operation */

#define ERR_NOMEM  1       /* No memory */
#define ERR_NOWAV  2       /* Unknown Waveform */
#define ERR_BADPAR 3       /* Bad parameter */

char GenErrStr[130];      /*  Explanatory message for GenErr    */

#define SetErrStr(S) strncpy(GenErrStr,S,128)

/* --------------------------------------------------------- */

extern int vflg,Abs_Level;
FILE *fWAVopen();
char *loadWavFile();

#define VERBOSE (vflg>1)

#define chkword(a,b) ((n>=a)&&(strncmp(p,b,n)==0))

#define IS16BIT (afmt==AFMT_S16_LE)

/* getGenErrStr() return the error string for the last recorded error
 */

char *getGenErrStr()
{
  return(GenErrStr);
}

/* init_samples(fo,fdo,N,S,F,St,Ch,&Wn)  initialise the generation variables etc.
 *   So that we don't need to specify these again - store them in local
 *   static variables, and initialise the 1Hz sample buffers.
 *   We always generate 16 bit samples and convert to 8 bit on output if nec.
 * 
 *   fo  is output FILE * or NULL if writing to a device
 *   fdo is output file/device number,
 *   N  is buffer size to use in bytes,
 *   S  is sample rate
 *   F  is sample format (8 or 16 bit)
 *   St is 0 for mono, 1 for stereo
 *   Ch is number of generating channels to allow
 *   *Wn is extra no. of entries to leave in Waveform arrays for loadable waveforms
 *       it is updated by number of waveforms generated internally.
 */

init_samples(fo,fdo,N,S,F,St,Ch,Wn)
FILE *fo;
int fdo,N,S,F,St,Ch,*Wn;
{
  int i;
  char *p,**aa;
   
  GenErr=samples=0; SetErrStr("OK");

  /* calculate size of waveform name array and waveform sample array
   *  This is Wn + number of waveforms that the internal generator code
   *  creates + 1 for the NULL terminator.
   */
  aa=(char **)getWavNames();
  for ( i=0; aa[i]!=NULL; i++) { (*Wn)++; }   /* Inc Wn for generator waves */
  /* now allocate waveform arrays for Wn waveforms.... */
  if (WavForms!=NULL) free(WavForms);
  if (WavBufs!=NULL) free(WavBufs);
  WavForms=(char **)malloc(((*Wn)+1)*sizeof(char *));
  WavBufs=(char **)malloc(((*Wn)+1)*sizeof(char *));
  if (WavForms==NULL || WavBufs==NULL) return(ENOMEM);
  for (i=0; i<=*Wn; i++) { WavBufs[i]=WavForms[i]=NULL;  }
  
  if (i=genAllWaveforms(&WavForms,&WavBufs,1,S,AFMT_S16_LE)) return(i);
  
  if ((i=setwaveform("off",&Silence,NULL))) return(i);
   
  if (Buff!=NULL) free(Buff);
  /* if 8bit then double buffer size 'cos we do all stuff 16bit internally */
  if ((Buff=malloc(N<<(F==AFMT_U8)))==NULL) { 
    SetErrStr("Cannot allocate Play Buffer");
    return(GenErr=ERR_NOMEM);
  }
  if (WaveFormName!=NULL) free(WaveFormName);
  WaveFormName=(char **)malloc(Ch*sizeof(char *));
  if (WaveForm!=NULL) free(WaveForm);
  WaveForm=(char **)malloc(Ch*sizeof(char *));
  if (CurSample!=NULL) free(CurSample);
  CurSample=(int *)malloc(Ch*sizeof(int));
  if (WaveFormName==NULL || WaveForm==NULL || CurSample==NULL) {
    SetErrStr("Cannot allocate Generator Arrays");
    return(GenErr=ERR_NOMEM);
  }
  
  FO=fo; FD=fdo; BuffSize=N; samplerate=S; afmt=F; stereo=St; channels=Ch;
  def_db=0;
  Tms=0;
  Buffsamples=(N>>stereo)>>(afmt==AFMT_S16_LE);  /* calc number of samples
						  * in buffer */
  for (i=0; i<channels; i++) {
    CurSample[i]=0;
    WaveForm[i]=WavBufs[0]; WaveFormName[i]=WavForms[0];
  }
  return(0);
}

/* do_loadable_waveforms(aa,n,T)  try and load loadable waveforms
 *    for all filenames aa[0] .... aa[n] 
 * loadable waveforms must be WAV files with same samplerate as we have 
 * already and be 16 bit. There must be exactly one seconds worth of samples
 * on error report, but continue.
 */

do_loadable_waveforms(aa,n,T)
char **aa;
int n,T;
{
   int i,w;
   FILE *f;
   char *p,bf[256];
   
   for (w=0; WavForms[w]!=NULL; w++) { }  /* find free location */
   for (i=0; i<n; i++) {
      strcpy(bf,aa[i]);
      if ((p=loadWavFile(bf,samplerate,AFMT_S16_LE,stereo,samplerate))==NULL)
	err_rpt(errno,bf);
      else {
	WavBufs[w]=p;
	if ((p=strrchr(aa[i],'.'))!=NULL) *p=0;
	if ((p=strrchr(aa[i],'/'))!=NULL) ++p; else p=aa[i];
	WavForms[w++]=p;
	if (vflg)	printf("Waveform '%s' loaded from WAV file %s\n",p,bf);
      }
   }
   return(0);
}

/* loadWavFile(fn,SR,afmt,st,SN)  load the samples from WAV format file fn
 *        WAV file should have a samplerate of SR, sample format of afmt,
 *        be stereo if st!=0 and mono if st==0, and have SN samples
 *        otherwise return NULL with errno set to err code.
 */

char *loadWavFile(fn,SR,afmt,st,SN)
char *fn;
int SR,afmt,st,SN;
{
   FILE *f;
   char *bf;
   struct WavValues *W;
   int e,n;

   if ((f=fopen(fn,"r"))==NULL) return(NULL);
   if ((W=NewWavValues())==NULL) { 
      errno=ENOMEM; fclose(f); return(NULL);
   }
   e=0;
   if ((isriff(f)==-1) || (errno=wavreadfmt(f,W))) {
      e=EINVAL;
      if (vflg) 
	printf("File %s is not a RIFF/WAV file.\n",fn);
   } else {
      if (vflg) {
	 printf("WAV file %s :\n",fn);
	 fprintfmt(stdout,W);
      }
      if ((SR > 0) && (W->samplerate != SR)) {
	 e=EINVAL;
	 if (vflg) 
	   printf("Samplerate is incorrect %d != %d\n",SR,W->samplerate);
      }
      if ((afmt>0) && (W->afmt != afmt)) {
	 e=EINVAL;
	 if (vflg) 
	   printf("Sample format is incorrect %d != %d\n",afmt,W->afmt);
      } 
      if ((st > -1) && (W->stereo != st)) {
	 e=EINVAL;
	 if (vflg) 
	   printf("Number of channels is incorrect %d != %d\n",st+1,(W->stereo)+1);
      } 
      if ((SN > 0) && (W->samples != SN)) {
	 e=EINVAL;
	 if (vflg) 
	   printf("Number of samples is incorrect %d != %d\n",SR,W->samples);
      }
   }
   if (e==0) {
      if ((bf=malloc(SN<<(afmt==16)))!=NULL) {
	 if ((n=wavread(f,W,bf,SN<<(afmt==16)))!=SN) {
	    e=EINVAL;
	    if (vflg) 
	      printf("Number of samples read is incorrect %d != %d\n",SR,n);
	    free(bf);
	 }
      } else e=ENOMEM;
   }
   free(W); fclose(f);
   return((e)?NULL:bf);
}

/* setwaveform(s,WFs,WFN)  search for string s in list of waveform names
 *   and if found set up *WFs with sample space and *WFN as name. 
 *   Return 0 if all ok
 *   else return error number and set up GenErr and GenErrStr
 */

setwaveform(s,WFs,WFN)
char *s;
char **WFs,**WFN;
{
   int i,n;
   char *p;
   
   p=(s==NULL)?"":s;    /* Zero length string selects first waveform */
   n=strlen(p);
   for ( i=0; WavForms[i]!=NULL; i++) {
      if (chkword(2,WavForms[i])) {
	 if (WFs!=NULL) *WFs=WavBufs[i];
	 if (WFN!=NULL) *WFN=WavForms[i];
	 return(0);
      }
   }
   sprintf(GenErrStr,"No such waveform: \"%s\".",p);
   return(GenErr=ERR_NOWAV);
}

/* getsamples()   return number of samples written since init_samples 
 *                  called.
 */

unsigned int getsamples()
{
   return(samples);
}

/* flush_samples()  to write enough silence to fill the current
 *        kernel buffer fragment, so we don't get spurious sounds.
 */

flush_samples()
{
   int n;
   int cp=0;
   
   if (samplerate==0) return(0);
   if ((n=samples%Buffsamples)) {
      n=Buffsamples-(samples%Buffsamples);
      newsamples(Buff,Silence,&cp,n,10000);
      writesamples(Buff,n);
   }
   samplerate=0;
   return(0);
}

/* newsamples(bf,wav,N,F,G)   create N new samples from waveform wav
 *                at freq F and store in buffer bf. *cp is the 'phase' pointer
 *                for selecting next sample from wav. Scale samples 
 *                by G/10000.   BEWARE of G>32000 because of overflow probs.
 */

newsamples(bf,wav,cp,N,F,G)
short int *bf;
short int *wav;
int *cp,N,F,G;
{
  int i,n,t;
  short int *vbp;
  
  /* all sorts of things can go wrong in this code
   * it is assumed that the calling code calls with sensible
   * parameters
   */
  
  vbp=bf;
  if (G>32000) G=32000;
  for (i=0; i<N; i++) {
    t=((int)(wav[*cp])*G/10000);
    *vbp++=(short int)((t>32767)?32767:((t<-32768)?-32768:t)); /* clipping */
    *cp=(*cp+F)%samplerate;
  }
}

/* addsamples(bf,wav,cp,N,F,G,L)   add N new samples from waveform wav
 *                at freq F into abuffer bf, of 'level' L, *cp is the 'phase'
 *                pointer for selecting next sample from wav. scale 
 *                new samples by G/10000. Like G, L is scaled by 10000
 * returns the mixing "level" index.
 */

addsamples(bf,wav,cp,N,F,G,L)
short int *bf;
short int *wav;
int *cp,N,F,G,L;
{
  int i,n,t,NL;
  short int *vbp;

  /* all sorts of things can go wrong in this code
   * it is assumed that the calling code calls with sensible
   * parameters
   */
  
  vbp=bf;
  if (L) {    /* with an auto-'level' we are autoscaling */
    for ( NL=L+G ; (L+G)>32767; L>>=1, G>>=1)
      { }       /* this keeps the arith to 32 bits */
    for (i=0; i<N; i++) {
      t=(int)(((int)(*vbp)*L)+((int)(wav[*cp])*G))/(L+G);
      *vbp++=(short int)((t>32767)?32767:((t<-32768)?-32768:t)); /* clipping */
      *cp=(*cp+F)%samplerate;
    }
  } else {    /* without an auto-'level' we are doing absolute mixing */
    for (NL=i=0; i<N; i++) {
      t=(int)(*vbp)+(int)(wav[*cp])*G/10000;
      *vbp++=(short int)((t>32767)?32767:((t<-32768)?-32768:t)); /* clipping */
      *cp=(*cp+F)%samplerate;
    }
  }
  return(NL);
}

/* doToneCmd(s)   interpret tone command in string s
 * 
 *  if *s is alphanumeric then it changes waveform or is a directive,
 *  it can be a comma
 *    separated list of waveforms to setup the different channels.
 *  if *s is numeric and default tone period not set then sets tone period
 *  else it is a tone specification
 *    N    a single freq. to play for default time period
 *    N:t  a single freq. played for t millisecs (overrides def. time period)
 *    N1,N2,... mix freqs N1,N2 etc and play the mix for default time period
 *    N1,N2,...:t  as above but played for t millisecs
 *    any of the above N can be replaced with N@db  where db is number
 *      of deciBells by which to amplify or attenuate that tone.
 * 
 *  e.g.  300,600@-20:500  play a mixed tone of 300Hz mixed with 600Hz
 *        attenuated by 20dB for 500 millisecs.
 * 
 *  :t   on its own sets the default time period to t millisecs
 *  @db  on its own sets the default dB level 
 * 
 * Directives currently are......
 * 
 *  echo restofline     outputs restofline to standard output
 *  reset|resync        resets sample pointers for all channels to 0
 *                      forces known phase relationships
 *  absolute|relative   set absolute or relative amplitude mixing
 * 
 * If a command starts with an alphabetic char. abd isn't one of above
 * and isn't a named waveform, then tones checks if there is a file
 * with the name and if so it opens it as a file of tones specifications.
 * When that file has been processed tones continues with where it is.
 */

doToneCmd(s)
char *s;

{
  char *p,*ss;
  int i,f,n,N,t,st,sCS;
  int Tt,T;
  
  GenErr=0; SetErrStr("OK");
  for ( ; isspace(*s); s++) {}
  
  if (isalpha(*s) && isalpha(*(s+1))) {
    if (strncmp("echo",s,4)==0) {
      printf("%s\n",s+5);
    } else if (strncmp("reset",s,5)==0 || (strncmp("resync",s,6)==0)) {
      for (i=0; i<channels; i++) CurSample[i]=0;
    } else if ((i=(strncmp("absolute",s,8)==0)) || strncmp("relative",s,8)==0) {
      Abs_Level=i;
      if (vflg)
	  printf("Tone levels set to %s.\n",(Abs_Level)?"ABSOLUTE":"RELATIVE");
    } else if (GenErr=docmdwaveform(s)) {
      GenErr=docmdfile(s);
    }
    return(GenErr);
  }
  if (*s=='.' || *s=='/') return(GenErr=docmdfile(s));
  /* check if initial or subsequent tone duration spec..... */
  if ((*s==':') || ((Tms==0) && isdigit(*s))) {
    if (*s==':') s++;
    return(doToneDur(s,&t));
  }
  if (*s=='@') return(doDefDB(s+1));
  /* now set up duration for this tone(s) spec..... */
  t=Tms;
  for ( p=s; *p; p++) {
    if (*p==':') {
      *p=0;
      if (st=doToneDur(p+1,&t)) return(st);
      GenErr=docmdtones(s,t);
      *p=':';
      return(GenErr);
    }
  }
  return(GenErr=docmdtones(s,t));
}

/* doDefDB(s)  setup def. dB level
 */

doDefDB(s)
char *s;
{
  if (*s=='-' || *s=='+' || isdigit(*s)) {
    def_db=atof(s); 
    if (vflg)
	printf("Default Amplitude level set to %.2fdB.\n",def_db);
    return(0);
  }
  SetErrStr("Bad Amplitude dB: Digit expected");
  return(GenErr=ERR_BADPAR);
}

/* doToneDur(s,t)  convert string s and set *t to resultant integer time
 *   if Tms is zero set it up and also Nt
 */

doToneDur(s,t)
char *s;
unsigned int *t;
{
  char *ss;
   
  *t=strtol(s,&ss,10);
  for ( s=ss; isspace(*s); s++) {}
  if (*s || (*t==0)) {
    SetErrStr("Bad Tone duration: 0 or unexpected character found");
    return(GenErr=ERR_BADPAR);
  }
  if (Tms==0) {
    Nt=((Tms=*t)*samplerate+500)/1000;    /* calc. number of samples for
					   * Tms (millisecs)         */
    if (vflg)
	printf("Default tone duration set to %d millisecs.\n",Tms);
  }
  return(0);
}

/* docmdtones(s,t)  run thru. freq(s) specified and build up waveform..... 
 *     and play it for t millisecs.
 */

docmdtones(s,t)
char *s;
int t;
{
  int i,f,g,more,n,N,L;
  double db;
  char *p,*ss;
  
  ss=s;                      /* save pointer for freq. specifier string */
  for ( N=((t*samplerate)+500)/1000 ; N; N-=n) {
    n=(N<=Buffsamples)?N:Buffsamples;
    for ( i=more=0,s=ss; *s && i<channels; i++) {
      for (; isspace(*s); s++) {}
      if (CHAN_SEPERATOR(*s)) { s++; continue; }
      if (isdigit(*s)) { f=strtol(s,&p,10); s=p; }
      else if ((f=ConvNote(&s))==0) {
	SetErrStr("Bad Tone Specifier: Digit or Note expected");
	return(GenErr=ERR_BADPAR);
      }
      for ( ; isspace(*s); s++) {}
      /* Modifications by Mark E. Shoulson May 1999 */
      if (*s && ATT_START(*s)) { /* Attenuation specified */
	s++;
	if (isdigit(*s) || *s=='-' || *s=='+' ) {
	  db=def_db+strtod(s,&p);   /* Gain level is relative to def_db */
	  s=p;
	} else {
	  SetErrStr("Bad Attenuation Specifier: Digit expected");
	  return(GenErr=ERR_BADPAR);
	}
      }
      else { db=def_db; }      /* db level is default dB level */
      g=rint(10000.0*dB(db));
      if (VERBOSE) {
	printf("%s Wave, Freq %dHz for %d millisecs (%d samples) at %.2fdB\n",
	       WaveFormName[i],f,(1000*n+samplerate/2)/samplerate,n,db);
	fflush(stdout);
      }
      if (more) {
	L=addsamples(Buff,WaveForm[i],&CurSample[i],n,f,g,L);
      } else {
	if (Abs_Level) { L=0; } else { L=g; g=10000; }
	newsamples(Buff,WaveForm[i],&CurSample[i],n,f,g);
	more=1;
      }
      if (*s && !CHAN_SEPERATOR(*s) && !(isspace(*s))) {
	SetErrStr("Bad Tone Specifier: ',' expected");
	return(GenErr=ERR_BADPAR);
      }
      if (*s) s++;
    }
    writesamples(Buff,n);
  }
  return(0);
}

/* ConvNote(**s)  check if *s defines a note [A-G][|#][0-9]
 * if so convert to equivalent frequency. C3 is middle C
 */

ConvNote(s)
char **s;
{
   int n;

   n=**s;
   if (islower(n)) n=toupper(n);
   if ( n>='A' && n<='G') {
      n=(n - 'A')*2; (*s)++;
      if (**s == '#') {
	 (*s)++; n++;
      }
      n=Notes[n];
      if (isdigit(**s)) {
	 n=((n<<((**s) - '0'))+500)/1000;
	 (*s)++;
	 if (vflg) printf("  (%dHz)\n",n);
	 return(n);
      }
   }
   return(0);
}

/* docmdwaveform(s)   parse string s as a waveform selection string
 * 
 *  either  "waveform"   selects all generators                 done
 *   or     "waveform,waveform,etc" sets waveforms sequentially done
 */

docmdwaveform(s)
char *s;
{
   char *p,c,bf[130];
   unsigned char *wf,*wfn;
   int st;
   
   strcpy(bf,s);
   for ( s=bf, st=0; *s && st<channels; st++) {
      for ( ; isspace(*s); s++) { }
      if (CHAN_SEPERATOR(*s)) { s++; continue; }
      for (p=s; isalnum(*s); s++) { }
      for ( ; isspace(*s); s++) { *s=0; }
      if (c=*s) *s++=0;
      if (GenErr=setwaveform(p,&wf,&wfn)) {
	 SetErrStr("Unrecognised Waveform.");
	 return(GenErr);
      }
      WaveForm[st]=wf;
      WaveFormName[st]=wfn;
      if (c==0 && st==0) {
	 if (vflg) {
	    printf("Setting all %d Channels to %s waveform.\n",channels,wfn);
	 }
	 for (; st<channels; st++) {
	    WaveForm[st]=wf; WaveFormName[st]=wfn;
	 }
	 return(0);
      }
      if (vflg) {
	 printf("Channel %d waveform set to %s\n",st+1,wfn);
      }
      if (c && !CHAN_SEPERATOR(c)) {
	 SetErrStr("Bad Char in Waveform Specification: ',' expected.");
	 return(GenErr=ERR_BADPAR);
      }
   }
   return(0);
}

/* Write samples away to opened file/DAC 
 */

writesamples(bf,N)
short int *bf;
int N;
{
   int i,n;
   char *p;
   short int *s;

   n=N*2;
   if (afmt!=AFMT_S16_LE) {
      n=N;
      for (p=(char *)(s=bf), i=0; i<n ; i++) { 
	 *p++=(char)((((int)(*s++))/256)+128); 
      }
   }
   if (FO==NULL) {   /* if not writing to a file...... */
     write(FD,(char *)bf,n);
   } else {         /* we are writing to a file...... */
     fWAVwrsamples(FO,(char *)bf,n);
   }
   samples+=N;
   if (VERBOSE) {
      printf("played %6d buffers %6d samples\n",samples/Buffsamples,samples%Buffsamples);
      fflush(stdout);
   }
}

/* docmdfile(fn)   open and read command file with name fn
 * 
 * return 0 is all ok else set GenErr and Str to report errors
 * and return error code
 */

docmdfile(fn)
char *fn;
{
  FILE *f;
  
  if (strcmp(fn,"-")==0) f=stdin;
  else if ((f=fopen(fn,"r"))==NULL) {
    GenErr=errno;
    sprintf(GenErrStr,"Couldn't open file '%s'.",fn);
    return(GenErr);
  }
  if (vflg) 
      printf("Processing command file '%s'.\n",fn);
  ftones(f,fn);
  fclose(f);
  return(0);
}

/* ftones(f,fn)    read FILE *f interpreting the tones commands therein
 *                 fn is name of file.
 */

ftones(f,fn)
FILE *f;
char *fn;
{
  int i,st;
  char *p,bf[262];
  
  for (i=1,rewind(f); (p=fgets(bf,260,f))!=NULL; i++) {
    for ( ; isspace(*p); p++) {}
    if (*p && *p!='#') {
      if (vflg) printf("%s\n",p);
      if (st=doToneCmd(p)) {
	fprintf(stderr,"[%s:%3d] %s -> %s\n",fn,i,p,getGenErrStr());
      }
    }
  }
}

doline(s)
char *s;

{
   int i,n;
   char **aa[50];
   
   for ( ; isspace(*s); s++) {}
   if (*s && *s!='#') {
      n=parse(s,aa,'\0');
      for (i=0; i<n; i++) doToneCmd(aa[i]);
   }
}
