/* fsynth.h
 * header file for the Ncurses based fourier synth
 * Jim Jackson  <jj@franjam.org.uk>  May 2008
 */

#include "config.h"

#define VERSTR "%s  Ver. %s   Ncurses based Fourier Waveform Synthesiser"

extern int vflg,dflg,vikeys;
extern char *sys;

extern char dac[];                      /* name of output device */
extern int DAC;
extern unsigned int samplerate;         /* Samples/sec        */
extern unsigned int stereo;             /* stereo mono */
extern unsigned int afmt;               /* format for DSP  */
extern int Bufspersec;                  /* Number of Buffers per sec */
extern int Nfragbufs;                   /* number of driver buffers */
extern int fragsize;                    /* size of driver buffer fragments */
extern int fragsamplesize;              /* size of fragments in samples */

extern char wf[32];                     /* waveform type */
extern unsigned int freq;               /* signal frequency */
extern int channels;                    /* number of harmonics */


