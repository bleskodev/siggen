/* siggen.h
 * header file for the Ncurses based signal generator
 * Jim Jackson  <jj@franjam.org.uk> Copyright 1997-2008
 */

#include "config.h"

#define VERSTR "%s  Ver. %s   Ncurses based Digital Signal Generator"

extern int vflg,dflg,vikeys;
extern char *sys;

extern char dac[];                    /* name of output device */

extern int resolution;                /* 1 = 1Hz resolution, 10 = 0.1HZ,
			               * 100=0.01 */
extern int DAC;
extern unsigned int samplerate;       /* Samples/sec        */
extern unsigned int stereo;           /* stereo mono */
extern unsigned int afmt;             /* format for DSP  */
extern int Bufspersec;                /* Number of Buffers per sec */
extern int Nfragbufs;                 /* number of driver buffers */
extern int fragsize;                  /* size of driver buffer fragments */
extern int fragsamplesize;            /* size of fragments in samples */
extern int LWn;                       /* no. of specified loadable waveforms */
extern char **LWaa;                   /* array of specifed loadable waveforms */

    /* channel 1  -  or mono..... */
extern char wf[32];                     /* waveform type */
extern unsigned int freq;               /* signal frequency */
extern int ratio;                       /* used in pulse, sweep etc */
extern int Gain;                        /* Amplification factor */

    /* channel 2 when in stereo mode ..... */
extern char wf2[32];                    /* waveform type */
extern unsigned int freq2;              /* signal frequency */
extern int ratio2;                      /* used in pulse, sweep etc */
extern int Gain2;                       /* Amplification factor */
extern int phase;                       /* phase diff with chan1 */

