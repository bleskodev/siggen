/* config.h
 * global config header file for all programs
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

#define VERSION "2.3.10 (May 2008)"

#ifndef _config_siggen_h
#define _config_siggen_h

/* USER CONFIGUARABLE STUFF ------------------------------- */
/*
 * NO_DAC_DEVICE     uncomment/set this to build sgen, swgen or tones
 *                   in wav/raw data file output mode only.
 *                   This turns off all reference to soundcards and
 *                   DSP/Audio devices.
 */

/*#define NO_DAC_DEVICE*/

/* SUNOS             uncomment/set this for compiling under SUNOS 4
 */

/*#define SUNOS*/

/* ---------------------------------------------
 * These options are dynamically configurable via the runtime config
 * files. The values below define default values.
 * 
 * Non-string configurable options should have a Qxxxxx definition
 * to define the string equivalent of the value.
 */

/* DEF_CONF_FILENAME     name of local runtime config file. 
 */

#define DEF_CONF_FILENAME ".siggen.conf"

/* DEF_GLOBAL_CONF_FILE  name of default global configuration file. 
 *            The startup process looks for a config file in local directory,
 *            then home directory, and finally looks for this file.
 *            For structure of config files see sample .siggen.conf
 */

#define DEF_GLOBAL_CONF_FILE "/etc/siggen.conf"

/* DAC_FILE is name of sound output device
 */

#define DAC_FILE "/dev/dsp"

/* MIXER_FILE is name of mixer device
 */

#define MIXER_FILE "/dev/mixer"

/* DEF_PLAYTIME is default playing time for file output in millisecs
 */

#define DEF_PLAYTIME  1000
#define QDEF_PLAYTIME "1000"

/* SAMPLERATE is default samplerate to use, could be 8000 for simple
 *            stuff or 44100 for HiFi quality.
 */

#define SAMPLERATE  22050
#define QSAMPLERATE "22050"

/* RESOLUTION sets the resolution with which some programs generate
 *            signal frequencies. Valid values are 1Hz, 0.1Hz or 0.01Hz
 */

#define DEFAULT_RESOLUTION "0.1Hz"

/* VI_KEYS    set to 1 if the VI cursor moving keys "HJKL" are to be active.
 */

#define VI_KEYS   0
#define QVI_KEYS  "0"

/* DEF_TONES_CHANNELS default number of channels for tones program
 */

#define DEF_TONES_CHANNELS  4
#define QDEF_TONES_CHANNELS "4"

/* DEF_FSYNTH_CHANNELS default number of channels for fsynth program
 */

#define DEF_FSYNTH_CHANNELS  13
#define QDEF_FSYNTH_CHANNELS "13"

/* DEFAULT_FRAGMENTS  number of buffer fragments to configure in sound driver
 *             for playing. Used by interactive programs to limit
 *             the 'play ahead' of sound samples so that parameter changes
 *             appear to happen 'straight' away. This is a compromise
 *             too few buffers and there may not be enough play ahead
 *             to cover the time that other programs are running,
 *             too many buffers and the programs feel very unresponsive.
 *             The programs attempt to set the buffer size dependant on
 *             the samplerate, so that one buffer takes very approx. 
 *             100millisecs of samples (buffer size is the nearest 
 *             power of 2).
 */

#define DEFAULT_FRAGMENTS  3
#define QDEFAULT_FRAGMENTS "3"

/* DEFAULT_BUFFSPERSEC  number of buffers to play per second - aproximately
 *       This is used to calculate the buffer fragment size, depending on this
 *       samplerate, stereo/mono, 16 or 8 bit samples. Buffer size is always
 *       a power of 2 anyway.
 */

#define DEFAULT_BUFFSPERSEC 15
#define QDEFAULT_BUFFSPERSEC "15"

/* DEF_WN     default number of loadable waveforms to allow
 */

#define DEF_WN     10
#define QDEF_WN    "10"

/* these config params are hard coded here.....
 * 
 * LOCK_FIELD_CHARS   is the string of characters that are recognised
 *                    for locking/freezing values in a field in curses progs
 *                    by default "!lL" but if vikeys is used "lL" will not be 
 *                    available 'cos it equates KEY_LEFT
 * UNLOCK_FIELD_CHARS ditto for chars to cause unlocking of all fields
 */

#define LOCK_FIELD_CHARS  "!lL"
#define UNLOCK_FIELD_CHARS "uU"

/* Color configuration. 6 color combinations are used.
 * Each consists of Foreground and Background colors.
 * If the color pair are used for input fields then the fore and back
 * ground colors are reversed when that field is being input/altered.
 * 
 *  ColorPair1   for main header and footer
 *  ColorPair2   for the "ASCII ART" header banner
 *  ColorPair3   for the author sig field, the 2 "help" lines at bottom of
 *                screen, and the clock and labels on the digital audio
 *                details line under main header.
 *  ColorPair4   Digital Audio sample value fields - samplerate, 
 *                bits, mono/stereo, buffers etc
 *  ColorPair5   Main central screen fixed layout
 *  ColorPair6   Input value fields in main central screen
 */

#define ColorPair1  Define_Color(1,COLOR_BLUE,COLOR_GREEN)
#define ColorPair2  Define_Color(2,COLOR_YELLOW,COLOR_BLUE)
#define ColorPair3  Define_Color(3,COLOR_GREEN,COLOR_RED)
#define ColorPair4  Define_Color(4,COLOR_CYAN,COLOR_RED)
#define ColorPair5  Define_Color(5,COLOR_WHITE,COLOR_BLUE)
#define ColorPair6  Define_Color(6,COLOR_RED,COLOR_CYAN)

/* End of USER CONFIGURABLE STUFF ------------------------- */

/* Some color setting macros......
 */

#define Define_Color(C,F,B)  { init_pair(C,F,B) ; \
                               init_pair(COLOR_PAIRS-C,B,F); }


#ifndef ColorPair1      
# define ColorPair1  Define_Color(1,COLOR_BLUE,COLOR_GREEN)
#endif
#ifndef ColorPair2
# define ColorPair2  Define_Color(2,COLOR_YELLOW,COLOR_BLUE)
#endif
#ifndef ColorPair3
# define ColorPair3  Define_Color(3,COLOR_GREEN,COLOR_RED)
#endif
#ifndef ColorPair4
# define ColorPair4  Define_Color(4,COLOR_CYAN,COLOR_RED)
#endif
#ifndef ColorPair5
# define ColorPair5  Define_Color(5,COLOR_WHITE,COLOR_BLUE)
#endif
#ifndef ColorPair6
# define ColorPair6  Define_Color(6,COLOR_RED,COLOR_CYAN)
#endif

/* Set various things depending on what the user configures.....
 *
 * If no soundcard stuff, we need to define the AFMT values.
 * If SUNOS we need to do some extern stuff, and include the correct
 *  string(s).h
 */

#ifndef NO_DAC_DEVICE
#define HAVE_DAC_DEVICE
#include <sys/soundcard.h>
#else
#define AFMT_QUERY  0
#define AFMT_S16_LE 16
#define AFMT_U8     8
#endif

#define QAFMT_QUERY "0"

#ifdef SUNOS
extern char *sys_errlist[];
extern int sys_nerr;
extern char *strrchr(),*strlchr();
#define BIGENDIAN
#endif

#define EFAIL -1

/* Some useful defines that are common to all programs......
 */

/* define the dB function for converting a dB 
 * value to a linear amplitude factor. Assumes and returns double values
 */

#define dB(X) pow(10,X/20.0)

/* and some predefinitions......
 */

char *get_conf_value();

#endif   /*  _config_siggen_h */
