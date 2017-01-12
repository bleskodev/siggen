/* Taken from the sndkit distribution
 * 
 *      Autor:    Michael Beck - beck@informatik.hu-berlin.de
 *      Amended:  Jim Jackson  - jj@franjam.org.uk
 *
 * changes:
 *
 *  Alterations to cope with little endian and big endian machines.
 *  Original assumed only x86 type.
 *
 *  audio structure additions at end
 */

#ifndef _FMTHEADERS_H
#define _FMTHEADERS_H	1

#include <sys/types.h>

/* Definitions for .VOC files */

#define MAGIC_STRING	"Creative Voice File\0x1A"
#define ACTUAL_VERSION	0x010A
#define VOC_SAMPLESIZE	8

#define MODE_MONO	0
#define MODE_STEREO	1

#define DATALEN(bp)	((u_long)(bp->datalen) | \
                         ((u_long)(bp->datalen_m) << 8) | \
                         ((u_long)(bp->datalen_h) << 16) )

typedef struct _vocheader {
  u_char  magic[20];            /* must be MAGIC_STRING */
  u_short headerlen;	        /* Headerlength, should be 0x1A */
  u_short version;	        /* VOC-file version */
  u_short coded_ver;	        /* 0x1233-version */
} VocHeader;

typedef struct _blocktype {
  u_char  type;
  u_char  datalen;              /* low-byte    */
  u_char  datalen_m;	        /* medium-byte */
  u_char  datalen_h;	        /* high-byte   */
} BlockType;

typedef struct _voice_data {
  u_char  tc;
  u_char  pack;
} Voice_data;

typedef struct _ext_block {
  u_short tc;
  u_char  pack;
  u_char  mode;
} Ext_Block;


/* Definitions for Microsoft WAVE format */

#define RIFF		"RIFF"
#define WAVE		"WAVE"
#define FMT		"fmt "
#define DATA		"data"
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* it's in chunks like .voc and AMIGA iff, but my source say there
 * are in only in this combination, so I combined them in one header;
 * it works on all WAVE-file I have
 */


typedef struct _waveheader {
  char  	main_chunk[4];	/* 'RIFF' */
  u_long	length;		/* filelen */
  char  	chunk_type[4];	/* 'WAVE' */

  char          sub_chunk[4];	/* 'fmt ' */
  u_long	sc_len;		/* length of sub_chunk, =16 */
  u_short	format;		/* should be 1 for PCM-code */
  u_short	modus;		/* 1 Mono, 2 Stereo */
  u_long	sample_fq;	/* frequence of sample */
  u_long	byte_p_sec;
  u_short	byte_p_spl;	/* samplesize; 1 or 2 bytes */
  u_short	bit_p_spl;	/* 8, 12 or 16 bit */ 

  char  	data_chunk[4];	/* 'data' */
  u_long	data_length;	/* samplecount */
} WaveHeader;

/* struct audio_params     used to keep together audio parameters
 *                         values are as used by the /dev/dsp device
 */

struct audio_params {
   unsigned int samplerate;
   unsigned int afmt;
   unsigned int stereo;
   unsigned char *abuf;
   unsigned int size;
} ;

#endif
