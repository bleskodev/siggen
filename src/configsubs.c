/* configsubs.c
 * functions for dealing with configuration files and returning config values
 * Jim Jackson  Sep 98
 */

/*
 * Copyright (C) 1998-2008 Jim Jackson                    jj@franjam.org.uk
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

/*  OK what these functions do........
 * 
 *  init_conf_files(local,home,global,verb)    opens upto 3 configuration
 *     files - local, home and global - that may or may not exist.
 *        local   is typically just a file in the local directory
 *        home    has the contents of the HOME env variable prepended
 *        global  is typically a system wide readable global conf. file
 *     This function must be called before any other. If called a second 
 *     time all previous conf. files are closed automatically.
 *     The int verb sets the verbose/debug/level. 
 *      0 = no output, 1 = say what config files are opened/closed
 *      2 = also say where config values are found/matched.
 *     Returns 0 is all ok else returns an error code.
 *  
 *  close_conf_files()     simply closes down any open conf. files
 *     Not really necessary, but could be useful.
 * 
 *  get_conf_value(sys,name,def)   try and get the config. value
 *     for parameter 'name', system 'sys', from the config. files.
 *     config files are searched in order home, local, global.
 *     If no specific value under system 'sys' found then any global
 *     setting for 'name' is returned. Finally if nothing is found the 
 *     'def' is returned.
 *     All values are strings, as is the returned value.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysfio.h"

#define SEP ":"
#define STR_SIZE 130

/* #define DEBUG(F,FP) fprintf(stderr,"%s is %s opened\n",F,(FP==NULL)?"NOT":"")*/

/* static variables for this module.....
 */

#define LOCAL  0
#define HOME   1
#define GLOBAL 2
#define NUM_CONF_FILES 3

#define V_OPEN(F,T,N) (verbose) && (F!=NULL) && fprintf(stderr,"%s config file %s opened.\n",T,N)
#define V_CLOSE(T) (verbose) && fprintf(stderr,"Config file %s closed.\n",T)
#define V_PARAM(P,V,T) (verbose>1) && fprintf(stderr,"%s : %s = %s\n",T,P,V)

/* verbose enables output to stderr saying what is happenning
 * 0 = no output, 1 = output details of opening conf files
 * 2 = output where param matches are got from as well
 */

int verbose=0;

char *Xlat();

char nm1[STR_SIZE+2];
char nm2[STR_SIZE+2];
char nm3[STR_SIZE+2];

FILE *F[NUM_CONF_FILES] = { NULL, NULL, NULL };
char *Fnm[NUM_CONF_FILES] = { nm1, nm2, nm3 };

init_conf_files(local,home,global,v)
char *local,*home,*global;
int v;
{
  int st;
  char f[STR_SIZE+4],*p,*s;
  
  verbose=v;    /* set verbose/debug level */
  close_conf_files();

  if (local != NULL) {
    F[LOCAL]=sysfio_fopen(local,"r");
    strncpy(Fnm[LOCAL],local,STR_SIZE);
    V_OPEN(F[LOCAL],"Local",local);
  }
  if (home != NULL) {
    s=getenv("HOME");
    strncpy(f,(s!=NULL)?s:"",STR_SIZE);
    strcat(f,"/");
    st=STR_SIZE+2-strlen(f);
    strncat(f,home,st);
    F[HOME]=sysfio_fopen(f,"r");
    strncpy(Fnm[HOME],f,STR_SIZE);
    V_OPEN(F[HOME],"Home",f);
  }
  if (global != NULL) {
    F[GLOBAL]=sysfio_fopen(global,"r");
    strncpy(Fnm[GLOBAL],global,STR_SIZE);
    V_OPEN(F[GLOBAL],"Global",global);
  }
  return(0);
}

close_conf_files()
{
  if (F[LOCAL] != NULL)  { sysfio_fclose(F[LOCAL]); F[LOCAL]=NULL; V_CLOSE(Fnm[LOCAL]); }
  if (F[HOME] != NULL)   { sysfio_fclose(F[HOME]); F[HOME]=NULL;  V_CLOSE(Fnm[HOME]); }
  if (F[GLOBAL] != NULL) { sysfio_fclose(F[GLOBAL]); F[GLOBAL]=NULL;  V_CLOSE(Fnm[GLOBAL]); }
}

char *get_conf_value(sys,name,def)
char *sys,*name,*def;
{
  int st,i,n;
  char f[STR_SIZE+4],*p,*s;

  *f=0; n=0;
  if (name == NULL || *name == 0) return(def);
  if (sys != NULL) {
    strncpy(f,sys,STR_SIZE);
    strcat(f,SEP);
    n=strlen(f);
  }
  s=f+n;
  strncat(s,name,STR_SIZE+2-n);

  /* ok here we have f[]=sysSEPname     (if sys exists)
   * and s points to the name part of the buffer
   * First we search the config files in local, home, global order
   * for any specific sysSEPname match, returning if anything found.
   * If no match and sys specified then just search for name.
   * If still no match return default def value
   * This allows simple global defaults to be set with specific 
   * exceptions by using the sys name etc
   */
  
  for (i=0; i<NUM_CONF_FILES; i++) {
    if (F[i]==NULL) continue;
    sysfio_rewind(F[i]);
    if ((p=Xlat(f,F[i]))!=NULL) {
      V_PARAM(f,p,Fnm[i]);
      return(p);
    }
  }
  if (f != s) {
    for (i=0; i<NUM_CONF_FILES; i++) {
      if (F[i]==NULL) continue;
      sysfio_rewind(F[i]);
      if ((p=Xlat(s,F[i]))!=NULL) {
        V_PARAM(s,p,Fnm[i]);
        return(p);
      }
    }
  }
  V_PARAM(f,def,"DEFAULT");
  return(def);
}

/***XLAT***/
/*  char *Xlat(s,fptr)
 *
 *    File fptr contains pairs of space delimited strings - one pair
 *    per line. Xlat searches for a line starting with string s.
 *    Xlat returns a ptr to the second string on the line.
 *
 *    The strings can be seperated by any number of spaces
 *    File fptr is an ASCII text file
 *    max line length is 256 chars
 *    returned ptr is to a string in this function -
 *     - subsequant calls to Xlat destroy string
 *     - caller should copy string after calling Xlat
 *    Xlat starts search at current fptr position - hence multiple
 *       calls get multiple matches - caller must rewind if needed
 *
 *    Xlat returns NULL on EOF or error - use ferror to check error
 */

static char Xlat_bf[258];

char *Xlat(s,fptr)
char *s;
FILE *fptr;

 { int n;
   char *p;

   n=strlen(s);
   p=Xlat_bf+n;

   while ( sysfio_fgets(Xlat_bf,256,fptr) != NULL )
    { if ((strncmp(s,Xlat_bf,n) == 0) && (*p==' '))
       { while ( *++p==' ');
         if ((s=strchr(p,'\n'))!=NULL) *s='\0';
         return(p);
       }
    }

   return(NULL);
 }


