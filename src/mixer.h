/* mixer.h
 * mixer control header file
 * Jim Jackson
 */

/*
 * Copyright (C) 2000-2008 Jim Jackson           jj@franjam.org.uk
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

#ifndef _mixer__h
#define _mixer__h

#ifdef __FreeBSD__
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif

/* Define some functions.....
 */

#define IS_MIX_INPUT_DEV(N) (recmask&(1<<(N)))
#define IS_MIX_DEV(N)       (devmask&(1<<(N)))
#define IS_MIX_DEV_STEREO(N) (stereodevs&(1<<(N)))
#define IS_MIX_SOURCE(N)    (sources&(1<<(N)))

#define MIX_DEV_NAME(N)     (devname[N])

#define GET_MIX_VALUE(D,V)  (ioctl(mixFD, MIXER_READ(D), &V))
#define SET_MIX_VALUE(D,V)  (ioctl(mixFD, MIXER_WRITE(D), &V))

/* extern declarations of some variables.
 */

extern int mixFD,devmask,recmask,sources,stereodevs;
extern char *devname[];



#endif   /*  _mixer__h */
