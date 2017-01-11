/* sweepgen.h
 * header file for the Ncurses based sweep generator
 * Jim Jackson  <jj@franjam.org.uk>
 */

#include "config.h"

#define VERSTR "%s  Ver. %s   Ncurses based Digital Sweep Generator"

extern int vflg,dflg,vikeys;
extern char *sys;

extern char dac[];			/* name of output device */
extern int DAC;
extern unsigned int samplerate;         /* Samples/sec        */
extern unsigned int stereo;             /* stereo mono */
extern unsigned int afmt;               /* format for DSP  */
extern int Bufspersec;                  /* Number of Buffers per sec */
extern int Nfragbufs;                   /* number of driver buffers */
extern int fragsize;                    /* size of driver buffer fragments */
extern int fragsamplesize;              /* size of fragments in samples */
extern int resolution;                  /* 1 = 1Hz resolution, 10 = 0.1HZ,
extern int LWn;                         /* number of specified loadable waveforms */
extern char **LWaa;                     /* array of specifed loadable waveform

    /* Sweeping channel.... */
extern char wf[32];                     /* waveform type */
extern unsigned int freq;               /* signal frequency */
extern int ratio;                       /* used in pulse, sweep etc */
extern int Gain;                        /* Amplification factor */

    /* Swept channel..... */
extern char wf2[32];                   /* waveform type */
extern unsigned int freqF, freqT;      /* low and upper freq of sweep */
extern unsigned int freqC;             /* centre freq and deviation of sweep */
extern int frdev;
extern int Gain2;                      /* Amplification factor */

