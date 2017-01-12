/* scfio.c
 * Single char. action structured field functions using ncurses. 
 * Jim Jackson  Apr 97 onwards
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson               jj@franjam.org.uk
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

/* Some Bug fixes and reasons for non-obvious code....
 *
 * Under solaris isprint() and isdigit() seem to ignore the parity bit
 * hence I have had to precede some of these tests with isascii() && ...
 * to make sure it worked right.
 *
 * To work right the keyboard routines need to be.....
 *
 *  initscr(); noecho(); nonl(); intrflush(stdscr,FALSE); 
 *  keypad(stdscr,TRUE); halfdelay(3);
 *
 * To get it to work under solaris and xterm.
 */  
 
/*
 * These functions are given a key character to action on a given field.
 * Either the key does something to the field or it doesn't. Either way
 * the value of the field always shows the current value.
 * 
 * These functions are useful in screen based programs that may need to 
 * schedule several things, one of which is single key input handling.
 * 
 * Typical use is :-
 * 
 *  struct SCField *FS;
 * 
 *  while {
 *    if (keypress())  {
 *       c=getchar();
 *       if (c==XXX) { .... do any special key handling ....  }
 *       else { 
 *         c=actfield(c,FS);
 *         .....
 *       }
 *    }
 *    do_other_things();
 *  }
 *   
 * See tfch.c for demo of use.
 * 
 * In absence of any manual/documentation here is a brief summary
 * of the functions provided.
 * 
 * These are main 'external' functions that would be used normally.
 * They revolve round a field structure that defines the field type, its
 * position on the screen, the attribute used for displaying the field,
 * keeps some internal information about where in the field the 
 * cursor is etc.
 * 
 * newfield(t,att,x,y,w,dp,id,dp)  create a new SCField structure and init. it
 * actfield(k,SCFp)    action key on field defined by SCField *SCFp
 * putfield(SCFp)      display field defined by SCField *SCFp
 * 
 * 
 *  For definitions of SCField etc see  scfio.h
 * 
 * These functions can be considered 'internal' but can be used by the 
 *  brave :-) .....
 * 
 * actfstr(k,att,s,ip,w)  action key on a string on screen - w max characters
 * putfstr(att,s,w)     display a string on screen - w max characters
 * actfrac(k,att,ip,w,o,dp) action key on a fraction on screen - w max chars, dp dec places
 * putfrac(att,i,w,dp) display a fraction on screen - w max chars, dp dec places
 * actfopt(k,att,sp,ip,w) action key on one of options char *sp[] set *ip to selected
 * putfopt(att,sp,i,w)  display char *sp[i] in field of width w
 * 
 * Remember an integer is a decimal fraction with 0 decimal points,
 * therefore for integers use....
 * 
 *  actfrac(k,att,ip,w,o,0)   and   putfrac(att,i,w,0)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include "scfio.h"                     /* local header file */

/* reverse_attr(a)  calculate the reverse attribute of attribute 'a'
 *    if its not a color pair then simply toggle A_REVERSE on 'a'
 *    if it is a color pair then make it COLOR_PAIRS-pair_number
 */

reverse_attr(a)
int a;
{
   int r;

   if (PAIR_NUMBER(a)) r=(a&(~A_COLOR))+(COLOR_PAIR(COLOR_PAIRS-PAIR_NUMBER(a)));
   else r=((a+A_REVERSE)&A_REVERSE)+(a&~A_REVERSE);
   return(r);
}

/* Fixed Point Fractions.
 * 
 * The fixed point fraction support defines the fraction as
 * an integer value plus the number of decimal places required.
 * Hence 2.34 would be stored as the integer 234 with DP of 2
 *
 * An Integer I, and decimal places of d defines the number I * 10^-d
 * 
 * OK its tacky and awkward, but I just don't like real numbers, and
 * this way you can deal with money values easily (e.g. all values can be
 * integer pence but displayed as pounds with a DP of 2) and alot of other
 * things.
 */
	 
/* actfrac(k,att,ip,w,o,DP)   action key k on fixed fractional field of
 *         width w, attribute att, number of decimal points DP
 *           cursor is *o chars back from end of display string
 *           digits build up integer value, pushing digits at cursor leftwards
 *           DEL or backspace erases digit above cursor,
 *             moving digits in from left,
 *           KEY_UP (or +) / KEY_DOWN (or -)   inc's/dec's the integer
 *             by 1 at the digit above the cursor, with carry/borrow to left.
 *           KEY_LEFT / KEY_RIGHT  move cursor thru' the number.
 *           ip points to where to store the integer.
 */

actfrac(k,att,ip,w,o,DP)
int k,att,*ip,w,*o,DP;
{
   int c,N,P,y,x;
		  
   getyx(stdscr,y,x);
   w=RANGE(0,w,COLS-x);
   *o=RANGE(0,*o,MAXINTWIDTH-1);
   P=POWERS10[(DP)?((*o>DP)?*o-1:*o):*o];
   c=-1; N=*ip;
   if (k=='\b' || k==KEY_BACKSPACE || k==127 || k==KEY_DC) {
      N=(N/(P*10))*P+N%P;
   } else if (isascii(k) && isdigit(k)) {
      N=(((N/P)*10+k-'0')*P) + N%P ;
   } else if (k==KEY_UP || k=='+') {
      N+=P;
   } else if ((k==KEY_DOWN || k=='-') && *ip/P) {
      N-=P;
#ifdef CLOSED_FIELDS
   } else if (k==KEY_LEFT) {
      if (*o<(w-1) && (*o<(MAXINTWIDTH-1))) { 
         (*o)++;
         if (DP && *o==DP) *o+=(*o==(w-1))?-1:1;
      }
   } else if (k==KEY_RIGHT) {
      if (*o) {
         (*o)--;
         if (DP && *o==DP) (*o)--;
      }
#else
   } else if ((k==KEY_LEFT) && (*o<(w-1)) && (*o<(MAXINTWIDTH-1))) { 
     (*o)++;
     if (DP && *o==DP) *o+=(*o==(w-1))?-1:1;
   } else if ((k==KEY_RIGHT) && *o) {
     (*o)--;
     if (DP && *o==DP) (*o)--;
#endif
   } else {
     c=k;
   }
   if (c==-1) {
     *ip=( N>(LIMITS(RANGE(0,((DP>0)?w-1:w),MAXINTWIDTH))) ) ? *ip : N;
     putfrac(att,*ip,w,DP);
   }
   move(y,x+w-1-*o);
   refresh();
   return(c);
}


/* putfrac(att,n,w,DP)   display fixed decimal n in a field
 *           of width w, with DP decimal places, using attribute att.
 *  assumes correctly positioned at start of field.
 */

putfrac(att,n,w,DP)
int att,n,w,DP;
{
   int c,i,j,wd,x,y;
   chtype f[MAXCOL];
   
   w=RANGE(0,w,COLS-1);
   if (DP>0 && w>DP) wd=w-DP; else wd=w+1;
	
   for (c='0', i=w, j=n; i; i--) {
      if (i==wd) {
	 f[i]=att+'.'; continue;
      }
	   
      f[i]=j%10+att+c; j/=10;
      if (!j && i<wd) c=' ';
   }
   addchnstr(f+1,w);
}

/* actfstr(k,att,s,op,w)  Action key k on an input field of width w using 
 *           attribute att.
 *           Input chars to string s at position s[*op]
 *           DELETE deletes char over cursor,
 *           <- or ^H deletes char to left of cursor,
 *           Left and right arrows move cursor,
 *           HOME and END move cursor to start, end of string,
 */

actfstr(k,att,s,op,w)
int k,att;
char *s;
int *op,w;
{
   int i,l,y,x;
   char *q,*p;
   
   getyx(stdscr,y,x);
   
   w=RANGE(0,w,COLS-x);

   for (i=0; i<w && s[i]; i++) { }   /* make sure string is w chars wide */
   for ( ; i<w; i++) { s[i]=' '; }
   s[i]=0;
   if (*op>=w) *op=w-1;
   if (*op<0) *op=0;

   if (k=='\b' || k==KEY_BACKSPACE || k==127) {
      if (*op) {
	 for ( q=s+(--(*op)) ; *q=*(q+1); q++) {  }
	 *q=' ';
      }
   } else if (isascii(k) && isprint(k)) {
      for ( q=s+*op ; *q; q++) {
	 l=*q; *q=k; k=l;
      }
      if (*op<(w-1)) (*op)++;
   } else if (k==KEY_DC) {
      for ( q=s+*op ; *q=*(q+1); q++) {  }
      *q=' ';
#ifdef CLOSED_FIELDS
   } else if (k==KEY_LEFT) {
      if (*op) (*op)--; 
   } else if (k==KEY_RIGHT) {
      if (*op<(w-1)) (*op)++;
#else               /* open fields */     
   } else if (k==KEY_LEFT && *op) {
      (*op)--; 
   } else if (k==KEY_RIGHT && (*op<(w-1))) {
     (*op)++;
#endif     
   } else if (k==KEY_HOME) {
      *op=0;
   } else if (k==KEY_END) {
      *op=w-1;
   } else {
      move(y,x+*op); return(k);
   }
   s[w]=0;
   mvputfstr(y,x,att,s,w);
   move(y,x+*op);
   refresh();

   return(-1);
}

/* putfstr(att,s,o,w)   display string s in a field
 *           of width w using attribute att.
 */

putfstr(att,s,w)
int att;
char *s;
int w;
{
   int c,i,x,y;
   chtype f[MAXCOL],*p; 
   
   if (w>=COLS) w=COLS-1;
   for ( p=f, i=0 ; i<w ; i++) {
      if (*s) c=*s++; else c=' ';
      *p++=att+c;
   }
   *p=0;
   addchnstr(f,w);
}

/* actfopt(k,att,sp,ip,w)  action key k in an option field char *sp[]
 *           current option is sp[*ip]
 *           display with attribute att, in field of width w
 *           UP and DOWN arrows move back and forward an option.
 *           SPACE moves forward thru' options
 *           HOME and END move to first and last option
 */

actfopt(k,att,sp,ip,w)
int k,att;
char **sp;
int *ip,w;
{
   int i,l,y,x,ratt;
   char f[MAXCOL+1],*q,*p;
   
   getyx(stdscr,y,x);
   if (w>=COLS-x) w=COLS-x;

   if (k=='\b' || k==KEY_BACKSPACE || k==127 || k==KEY_UP) {
      k=KEY_UP;
      if (*ip) (*ip)--;
      else k=KEY_END;
   }  /* no else here on purpose */
   if (k==KEY_DOWN || k==' ') {
      (*ip)++;
      if (sp[*ip]==NULL) *ip=0;
   } else if (k==KEY_HOME) {
      *ip=0;
   } else if (k==KEY_END) {
      for ( ; sp[*ip+1]!=NULL; (*ip)++) { }
   } else if (k!=KEY_UP) {
      return(k);
   }
   putfstr(att,sp[*ip],w);
   refresh();
   return(-1);
}

/* newfield(t,att,x,y,w,dp,id,dp)    create a SCField structure and fill it in
 *          if any problems return NULL else return ptr to structure.
 */

struct SCField *newfield(t,att,x,y,w,d,id,dp)
int t,att,x,y,w;
void *d;
int id,dp;
{
  struct SCField *sp;
  int ft,n;
  
  ft=t&SCF_type;
  if (ft==SCF_integer) {
/*    if (RANGE(0,w,MAXINTWIDTH)!=w) return(NULL); */
    n=sizeof(struct SCField) + ((d==NULL)?sizeof(int):0);
    if ((sp=(struct SCField *)malloc(n))==NULL) return(NULL);
    if (d==NULL) d=(int *)(sp+sizeof(struct SCField));
    sp->val.i=(int *)d;
  } else if (ft==SCF_string) {
    n=sizeof(struct SCField) + ((d==NULL)?w+2:0);
    if ((sp=(struct SCField *)malloc(n))==NULL) return(NULL);
    if (d==NULL) d=(int *)(sp+sizeof(struct SCField));
    sp->val.s=(char *)d;
  } else if (ft==SCF_option) {
    if (d==NULL) return(NULL);
    if ((sp=(struct SCField *)malloc(sizeof(struct SCField)))==NULL) 
	return(NULL);
    sp->val.sp=(char **)d;    
  } else return(NULL);
  sp->type=t;
  sp->attr=att;
  sp->row=y;
  sp->col=x;
  sp->W=w;
  sp->id=id;
  sp->adj=0;
  sp->dp=dp;
  return(sp);
}

/* putfield(fp)   display field *fp 
 */

putfield(fp)
struct SCField *fp;
{
   int t;
   
   t=(fp->type)&SCF_type;
   if (t==SCF_integer) {
      mvputfrac(fp->row,fp->col,fp->attr,*(fp->val.i),fp->W,fp->dp);
      t=(fp->W)-(fp->adj)-1;
   } else if (t==SCF_string) {
      mvputfstr(fp->row,fp->col,fp->attr,fp->val.s,fp->W);
      t=(fp->adj);
   } else if (t==SCF_option) {
      mvputfstr(fp->row,fp->col,fp->attr,(fp->val.sp)[fp->adj],fp->W);
      t=(fp->W)-1;
   }
   else return(1);

   setsyx(fp->row,(fp->col)+t);
   return(0);
}

/* dupfield(fp)   duplicate field *fp 
 */

struct SCField *dupfield(fp)
struct SCField *fp;
{
  struct SCField *p;
   
  p=(struct SCField *)malloc(sizeof(struct SCField));
  if (p==NULL) return(NULL);
  return( (struct SCField *)memcpy(p,fp,sizeof(struct SCField)) );
}


/* actfield(k,fp)   action key k on field *fp 
 */

actfield(k,fp)
int k;
struct SCField *fp;
{
   int t,c;
   
   if ((fp->type)&SCF_fixed) return(k);
   t=(fp->type)&SCF_type;
   if (t==SCF_integer) 
     c=mvactfrac(fp->row,fp->col,k,fp->attr,fp->val.i,fp->W,&(fp->adj),fp->dp);
   else if (t==SCF_string) 
     c=mvactfstr(fp->row,fp->col,k,fp->attr,fp->val.s,&(fp->adj),fp->W);
   else if (t==SCF_option) 
     c=mvactfopt(fp->row,fp->col,k,fp->attr,fp->val.sp,&(fp->adj),fp->W);
   else return(-1);
   return(c);
}

