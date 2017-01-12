/* mixer.c
 * Simple Linux Mixer Functions
 * Jim Jackson  Wed Mar 15 2000
 */
/*
mixer.c: (version 1) Mar 2000
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mixer.h"

#define EFAIL -1

#define TRUE 1
#define FALSE 0

int mixFD=-1;           /* Mixer File Descriptor */
int devmask=0;          /* Defines which mixer channels are present */
int recmask=0;          /* Defines which devices can be an input device */
int sources=0;          /* Defines which devices are the current input dev(s) */
int stereodevs=0;       /* Defines which devices are stereo */
char *devname[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
                        /* Gives a list of names for the various mixer channels */

/* mixinit(DEV)  open mixer device DEV and interrogate it setting up
 *               various parameters.
 */

mixinit(DEV)
char *DEV;
{
   if (((mixFD = open(DEV, O_RDONLY)) >= 0) &&
       (ioctl(mixFD, SOUND_MIXER_READ_DEVMASK, &devmask) >= 0) &&
       (ioctl(mixFD, SOUND_MIXER_READ_RECMASK, &recmask) >= 0) &&
       (ioctl(mixFD, SOUND_MIXER_READ_RECSRC, &sources) >= 0)  &&
       (ioctl(mixFD, SOUND_MIXER_READ_STEREODEVS, &stereodevs) >= 0)) {
      return(mixFD);
   }
   return(-1);
}

/* isdev(dev,n)   check string dev against the list of mixer device names
 *                and if found return device/channel device number
 *                otherwise return -1
 */
 
isdev(dev)
char *dev;
{
   int i,n;
  
   if (dev==NULL) return(-1);
   n=strlen(dev);
   for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
      if (strncasecmp(dev,MIX_DEV_NAME(i),n)==0) {
	 return((IS_MIX_DEV(i))?i:-1);
      }
   }
   return(-1);
}

/* set mixer input device to dcevice number d
 * this currently assumes only one input device at a time.
 */

SET_MIX_INPUT_DEV(d)
int d;
{
   int i,st;
   
   i=(d<0)?0:(1<<d);
   if ((st=ioctl(mixFD, SOUND_MIXER_WRITE_RECSRC, &i))>=0) sources=i;
   return(st);
}

GET_MIX_INPUT_DEV()
{
   int i;
   
   for ( i=0; i<SOUND_MIXER_NRDEVICES ; i++) {
      if (sources&(1<<i)) return(i);
   }
   return(-1);
}
