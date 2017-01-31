/* generator.h
 * The main DSP signal generation functions plus some misc. functions
 * Jim Jackson     Linux Version
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson                    jj@franjam.org.uk
 * Copyright (C) 2017      Blesko Dev                     bleskodev@gmail.com
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mk8bit(bf,ip,N)  convert N 16 bit samples from *ip to 8 bit samples
 *                  in bf
 */
int mk8bit(unsigned char *bf, short int *ip, int N);

/*
 * mixplaybuf(a,b,c,N,afmt)   mix N samples from buffers b and c into 
 *                            sound play buffer a
 *                            afmt defines format of samples
 */
int mixplaybuf(unsigned char *a, unsigned char *b, unsigned char *c, int N, int afmt);

/*
 * chanmix(a,b,Nb,c,Nc,N,afmt)  digitally mix N samples from buffers b and c 
 *                              into buffer a.  Nb and Nc are the relative amplitudes of
 *                              the two input channels as percentages of fullscale.
 *                              afmt defines format of samples
 */
int chanmix(unsigned char *a, unsigned char *b, int Nb, unsigned char *c, int Nc,int N, int afmt);

/*
 * do_antiphase(a,b,N,afmt)   create an antiphase version of sample b in
 *                            a. afmt defines 8/16bit, N is number of samples
 */
int do_antiphase(unsigned char *a,unsigned char *b, int N, int afmt);

/*
 * maketone(wf,fr,N,G,afmt)   generate a tone of frequency fr, samplerate N/sec,
 *                            waveform wf, and digital Gain G,
 *                            does a malloc to get a new tone buffer.
 */
unsigned char *maketone(char *wf, int fr, int N, int G, int afmt);

/*
 * mksweep(bf,bfn,FMbuf,wavbuf,Fmin,Fmax,N,afmt)
 *                            create a swept waveform in bf, size bfn, of format afmt. 
 *                            Sweeping waveform is in FMbuf - THIS is 16 bit samples!!!!
 *                            Swept waveform is in wavbuf
 *                            the Freq range is Fmin to Fmax Hz.   Current Samplerate is N/sec
 */
int mksweep(char *bf, int bfn, char *FMbuf, char *wavbuf,
            int Fmin, int Fmax, unsigned int N, unsigned int afmt);

/*
 * generate()  generate samples of waveform wf, in buf, at frequency fr,
 *             for a playing rate of S samples/sec. R is a further param
 *             if needed for pulse or sweep. buf is size N bytes.
 *             format required is afmt and samples are scaled by A/100
 *             N should be a factor of S, or, more usually, an exact multiple
 *             of S. Usually N=S for 8 bit samples or =2S for 16 bits
 *  return number of samples in buffer, or 0 if there is an error.
 */
int generate(char *wf, char *buf, unsigned int N, unsigned int fr,
             unsigned int A, unsigned int S, int R, int afmt);

/*
 * getWavNames()  return pointer to an array of strings containing
 *                the names for the waveforms that we can generate
 */

char **getWavNames();

/*
 * genAllWaveforms(Wfs,bufs,F,samplerate,afmt)   generate all possible
 *                                               waveforms at freq F and setup
 *                                               (*Wfs)[] with waveform names and
 *                                               (*bufs)[] with the actual samples.
 */
int genAllWaveforms(char **Wfs[], char **bufs[], int F, int samplerate, int afmt);

#ifdef __cplusplus
} /* extern "C" */
#endif
