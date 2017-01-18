/* misc.h
 * Miscellaneous Functions 
 * Jim Jackson     Dec 96
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
 * delay(us)  wait us microsecs using select. Effectively
 *            causes current process to stop and a reschedule take place
 *            as well as doing the delay.
 */
void delay(int us);

/* 
 * err_rpt(err,msg)
 */
int err_rpt(short err, const char *msg);

/*
 * hcf(x,y)   find highest common factor of x and y
 */
unsigned int hcf(unsigned int x, unsigned int y);

/*
 * parse(s,aa,sep) splits s up into parameters and stores ptrs
 *                 to each prm in ptr array aa (upto MAX_ARGS)
 *                 params are space or tab or 'sep' seperated, and may be
 *                 enclosed in single or double quotes e.g. 'this is 1 prm'
 *
 * returns number of parameters parsed
 */
enum { MAX_ARGS = 50 };
int parse(char *s, const char **aa, char sep);

/*
 * mstosamples(ms,sr)  convert ms millisecs into number of samples
 *                     given that the sample rate is sr samples/sec
 */
int mstosamples(int ms, int sr);

#ifdef __cplusplus
} /* extern "C" */
#endif
