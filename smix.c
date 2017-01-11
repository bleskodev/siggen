/* smix.c
 * Simple Linux Mixer controller
 * Jim Jackson  Fri 16 Jun 1995
 */
/*
smix: (version 1) Nov 1996
based on the dos sb16 mixer......
smix: (version 1) Fri 16 Jun 1995

NAME:
        smix  -  Simple Linux Mixer controller

SYNOPSIS:
        smix [-v] [-h] [-o file] [-i file] [command(s)] 

DESCRIPTION:

Controls the Mixer settings from the command line parameter(s).
The commands are detailed below, capitals showing the minimum abbreviation
allowed. Upper or lower case can be used on the command line. All Volume
settings are in range 0-100 (0 min, 100 max), but these are scaled to the
mixers actual range, hence set volume may be slightly different.

SHow              outputs the settings of the mixer. This is the default 
                   if no command line parameters are given
dev N or L,R      sets mixer device 'dev' to volume N, or to seperate
                    left and right stereo volume L,R
                  If device doesn't support stereo settings then max of L,R
                    is used.
INput dev         set the DSP input to be 'dev' or 'NOne' to turn inputs off
Verbose           makes the program output the settings after doing the
                   commands
                   
Use '-' as a filename to indicate standard input.

FLAGS:

-state       outputs details is a form that can used as the command
              line parameters to an smix command. This can be used
              to save the settings in a shell variable and then reset them.
-h           lists usage summary, which also lists the mixer devices and
              the possible input devices.
-v           verbose - outputs the results of commands. Same as Verbose above

OPTIONS:

-i file      read commands from file
-o file      output to file file
-m mixerdev  mixer device to use
-C configfile  read configfile

NOTES:

BUGS:

Any questions, suggestions or problems, etc. should be sent to

Jim Jackson

Email:  jj@franjam.org.uk

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "config.h"
#include "mixer.h"

#define PROGNAME "smix"

#define EFAIL -1

#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE !FALSE
#endif

#define nl puts("")
#define option(b,a) ((n>=a)&&(strncasecmp(p,b,n)==0))

#define doshow(F) printvols(devmask,F)

char mixname[130];
int mixer;           /* Mixer File Descriptor */

char *outfile;
char *infile;
char *configfile;
char *sys;
int state;
int vflg;
int hflg;

char *strupper(),*reads(),*sindex(),*tab();
FILE *fopen();
unsigned char readmixer();

main(argc,argv)
int argc;
char **argv;

 { int i,j,n,st;
   char fname[132],*p;
   FILE *fpr,*fpw;


   if ((p=strrchr(sys=*argv++,'/'))!=NULL) sys=++p;
   argc--;
   
   configfile=DEF_CONF_FILENAME;
   infile=outfile=NULL;
   mixname[0]=0;
   vflg=hflg=FALSE;
   state=FALSE;

   while (argc && **argv=='-') {
     n=strlen(p=(*argv++)+1); argc--;
     if (option("output",1)) {
       if (argc) {
	 outfile=*argv++; argc--; 
       }
     }
     else if (option("input",1)) {
       if (argc) {
	 infile=*argv++; argc--; 
       }
     }
     else if (option("Config",1)) {
       if (argc) {
	 configfile=*argv++; argc--; 
       }
     }
     else if (option("mixer",1)) {
       if (argc && **argv!='-') {
	 strncpy(mixname,*argv++,sizeof(mixname)-1); mixname[sizeof(mixname)-1]=0; argc--; 
       }
     }
     else if (option("state",1)) {
       state=TRUE;
     }
     else {
       for (; *p; p++) {
	 if (*p=='h') hflg++;
	 else if (*p=='v') vflg++;
	 else {
	   *fname='-'; *(fname+1)=*p; *(fname+2)=0;
	   exit(err_rpt(EINVAL,fname));
	 }
       }
     }
   }

   /* interrogate config file....... */

   init_conf_files(configfile,DEF_CONF_FILENAME,DEF_GLOBAL_CONF_FILE,vflg);

   if (vflg==0) {
      vflg=atoi(get_conf_value(sys,"verbose","0"));
   }
   if (mixname[0]==0) {
     strncpy(mixname,get_conf_value(sys,"mixerfile",MIXER_FILE),sizeof(mixname)-1);
     mixname[sizeof(mixname)-1]=0;
   }

   if ((mixer=mixinit(mixname)) < 0) {
      exit(err_rpt(errno,mixname));
   }

   if (hflg) exit(help());
   
   if (vflg)  printf("Mixer     %s\n",mixname);

   if (outfile!=NULL)
    { if (strcmp(outfile,"-")==0) fpw=stdout;
      else if ((fpw=fopen(outfile,"w"))==NULL)
       { exit(err_rpt(errno,outfile)); }
    }
   else
    { fpw=stdout; }

   if (infile!=NULL)
    { if (strcmp(infile,"-")==0) fpr=stdin;
      else if ((fpr=fopen(infile,"r"))==NULL)
       { exit(err_rpt(errno,infile)); }
    }
   else
    { fpr=stdin; }

   if (infile!=NULL)
    { st=docmdfile(infile,fpr,fpw);  /* do any command file first */
      st+=docmds(argc,argv,fpw);
    }
   else if (argc)
      st+=docmds(argc,argv,fpw);
   else
      st+=docmdline("show",fpw);
   
   exit(st);
 }

docmdfile(fn,f,fo)
char *fn;
FILE *f,*fo;
{
  char bf[514],*p;
  
  while ((p=fgets(bf,512,f))!=NULL) { docmdline(bf,fo); }

}

docmdline(s,fo)
char *s;
FILE *fo;
{
  int n,v;
  char *aa[64],c,*p,*r,*q;

  for ( ; *s && isspace(*s); s++) {} /* skip initial whitespace */
  if (*s=='#') return(0);            /* comment line - ignore   */
  for (n=0; *s; n++)
   { 
     aa[n]=s++;
     for ( ; *s && !isspace(*s); s++ ) {}
     if (isspace(*s)) *s++=0;
     for ( ; *s && isspace(*s); s++) {}
   } 
  docmds(n,aa,fo);
}

docmds(c,v,fo)
int c;
char **v;
FILE *fo;
{
   int i,st,n,d,dopr;
   char *p;

   for ( ; c; )
    {
      n=strlen(p=*v++); c--;
      if (option("show",2)) doshow(fo);
      else if (option("verbose",1)) vflg++;
      else if (option("input",2)) {
         dopr=0;
	 if ( c && ((d=isdev(p=*v))>=0 || option("none",2))) {
	    v++; c--; 
	    if ( (d>=0) && IS_MIX_INPUT_DEV(d)==0 ) {
               fprintf(stderr,"%s is not a source device.\n",MIX_DEV_NAME(d));
	    } else {
	       if ( SET_MIX_INPUT_DEV(d) < 0) {
		  fprintf(stderr,"Couldn't set %s as recording source.\n",
                          (d>=0)?(MIX_DEV_NAME(d)):"no device");
	       }
               if (vflg) dopr=1;
            }
	 } else { dopr=1; }
         if (dopr) {
            if (sources) printvols(sources,fo);
            else fprintf(fo,"No source inputs active.");
	 }
      } else if (option("all",3)) {
	 if (c && isvol(*v)) {
	    p=*v++; c--;
	    doallvols(devmask,p,fo);
	 } else doshow(fo);
      } else if ((d=isdev(p,n))>=0) {
	 if (c && isvol(*v)) {
	    p=*v++; c--;
	 } else p="";
	 dovol(d,p,fo);
      } else { 
	 fprintf(stderr,"Command '%s' not recognised.\n",p);
	 st=1;
      }
    }
}

isvol(s)
char *s;
{
   return(isdigit(*s) ||
	  (strncasecmp(s,"off",3)==0) ||
	  (strncasecmp(s,"full",4)==0));
}

/* dovol(d,s)   set mixer device d to volume specified by string s
 *             s is an "NN" or "LL,RR", i.e. a numeric ascii string
 *             or two num ascii strings for stereo left and right settings
 */

dovol(d,s,fo)
int d;
char *s;
FILE *fo;
{
  int lv,rv,n,dopr;
  char buf[130];

   dopr=vflg;
   if (s!=NULL && *s) {
      if (strncasecmp(s,"off",3)==0) { lv=0; s+=3; }
      else if (strncasecmp(s,"full",4)==0) { lv=100; s+=4; }
      else { 
	 for ( lv=0; isdigit(*s); s++) { lv=lv*10+(*s-'0'); } 
      }
      if (*s == ',' || *s == '/') {
	 s++;
	 if (strcasecmp(s,"off")==0) { rv=0; }
	 else if (strcasecmp(s,"full")==0) { rv=100; }
	 else {
	    for ( rv=0; isdigit(*s); s++) { rv=rv*10+(*s-'0'); }
	 }
      } else rv=lv;
      if (IS_MIX_DEV_STEREO(d)) n=(rv<<8)+lv;
      else n=(lv>rv)?lv:rv;
      if (SET_MIX_VALUE(d,n) < 0) {
	 sprintf(buf,"Problem setting level of mixer channel %s.",
		 MIX_DEV_NAME(d));
	 err_rpt(ENODEV,buf);
      }
   } else { dopr=1; }
   if (dopr) printvol(d,fo);
}

printvols(devs,fo)
int devs;
FILE *fo;
{
   int i;

   for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
      if (devs&(1<<i)) {
	printvol(i,fo);
      }
   }
}

doallvols(devs,s,fo)
int devs,s;
FILE *fo;
{
   int i;

   for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
      if (devs&(1<<i)) {
         dovol(i,s,fo);
      }
   }
}

printvol(d,fo)
int d;
FILE *fo;
{
   int n;
   char buf[130];
   
   if (IS_MIX_DEV(d)) {
      if (GET_MIX_VALUE(d,n) < 0) {
	 sprintf(buf,"Problem reading current level of mixer channel %s.",
		 MIX_DEV_NAME(d));
	 err_rpt(ENODEV,buf);
      } else {
	 fprintf(fo,"%-6s  %3d",MIX_DEV_NAME(d),n&255);
	 if (IS_MIX_DEV_STEREO(d)) { fprintf(fo,"/%-3d ",(n>>8)&255);
	 } else { fprintf(fo,"     "); }
	 if (state) {
	   fprintf(fo,"\n");
	   if (IS_MIX_SOURCE(d)) fprintf(fo,"INPUT  %s\n",MIX_DEV_NAME(d));
	 } else {
	   fprintf(fo,"%2s ", (IS_MIX_INPUT_DEV(d))? "*":""); 
	   fprintf(fo,"%3s ", (IS_MIX_SOURCE(d))? "<-":"");
	   fprintf(fo,"\n");
	 }
      }
   }
}

help()
{
  int i;

  printf("\nSet Linux Mixer from the command line - mixer device is %s\n\n",mixname);
  printf("Commands are :-\n");
  printf(" Verbose or -v  verbose mode, outputs results of actions\n");
  printf(" DEV N or L,R   sets DEV to N or left to L, Right to R\n");
  printf("  where DEV can be :\n   ");
  for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
    if (IS_MIX_DEV(i)) printf("%s ",MIX_DEV_NAME(i));
  }
  printf("\n INput DEV      sets the DSP input to be DEV, where DEV can be :\n   ");
  for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
    if (IS_MIX_INPUT_DEV(i)) printf("%s ",MIX_DEV_NAME(i));
  }
  printf("\n\n");
  printf("Default Config files are \"%s\", \"$HOME/%s\" and\n\"%s\", searched in that order.\n\n",
	  configfile, DEF_CONF_FILENAME, DEF_GLOBAL_CONF_FILE);

  printf("Usage:   %s [-h] [-v] [-i cmdfile] [-o file] [-m mixdev] [command(s)]\n\n",sys);
  printf("       -C file       use file as local configuration file\n");
  printf("       -m mixdev     use mixdev as mixer instead of default /dev/mixer\n");
  printf("       -o file       put output in file instead of to standard output\n");
  printf("       -i file       take commands from file, after the command line\n");
  printf("       -s            output settings in form to be used as an input file\n");
  printf("\n");
}

/***ERR_RPT***/
/* err_rpt(err,msg)
 */

err_rpt(err,msg)
short int err;
char *msg;

 { 
   extern char *sys;

   if (err) fprintf(stderr,"[%s] %s : %s\n",sys,strerror(err),msg);
   return(err);
 }

/***STRUPPER***
 *  char *strupper(p)   convert all lower case chars to upper in str p
 */

char *strupper(p)
char *p;
 { char *i,d;
   d='a'-'A';
   for (i=p ; *i ; i++)
     if ((*i>='a') && (*i<='z')) *i-=d ;
   return(p);
 }

/***DELNL***/
/* char *delnl(s)   remove trailing nl from end of string s
 *                  return same pointer as given
 */

char *delnl(s)
char *s;

 { char *p;

   if (s!=NULL && *s && *(p=s+strlen(s)-1)=='\n') *p=0;
   return(s);
 }
