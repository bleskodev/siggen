/* scrsubs.c
 * Screen handling routines using scfio and ncurses
 * Jim Jackson    Oct 98
 */

/*
 * Copyright (C) 1997-2008 Jim Jackson                jj@franjam.org.uk
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

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "scfio.h"
#include "config.h"

extern int vikeys;

/*
 *  doinput(c,aa,H,f)  action char c on field aa[f]
 *      if field changes display old field in normal and  new
 *      field in reverse, and display relevant new help line from H[]
 *  return new field value.
 */

doinput(c,aa,H,HF,f)
int c;
struct SCField *aa[];
char *H[];
struct SCField *HF;
int f;
{
   int fn,i;
  
   if ((c&0x7F)==12) {                      /* redraw screen '^L'  */
     touchwin(stdscr); wrefresh(curscr); return(f);
   }

   if (vikeys) {
     if (c=='h' || c=='H') c=KEY_LEFT;
     else if (c=='j' || c=='J') c=KEY_UP;
     else if (c=='k' || c=='K') c=KEY_DOWN;
     else if (c=='l' || c=='L') c=KEY_RIGHT;
   }
   c=actfield(c,aa[f]);
   fn=-1;
   if (strchr(UNLOCK_FIELD_CHARS,c)!=NULL) {
     for (i=0; aa[i]!=NULL; i++) { 
       if (!(CanInput(aa[i]))) {
	 aa[i]->attr=reverse_attr(aa[i]->attr); 
	 putfield(aa[i]);
	 InputField(aa[i]);
       }
     }
     putfield(aa[f]);
   } else if (strchr(LOCK_FIELD_CHARS,c)!=NULL) {
     fn=next_field(aa,f);
     if (fn!=f) {
       FixedField(aa[f]);
       aa[f]->attr=reverse_attr(aa[f]->attr);
     }
   } else if (c==KEY_LEFT) {
     fn=prev_field(aa,f);
   } else if (c==KEY_RIGHT || c=='\r' || c=='\n' || c=='\t') {
     fn=next_field(aa,f);
   } else if (c==KEY_HOME) { fn=0; }
   if (fn!=-1) {
      dispfield(aa,H,HF,f);
      f=fn;
      dispfield(aa,H,HF,f);
   }
   refresh();
   return(f);
}

/* dispfield(ff,hh,HF,f)   display SCField ff[f] along with associated
 *                      help line hh[f] on info line.
 */

dispfield(ff,hh,HF,f)
struct SCField *ff[];
char *hh[];
struct SCField *HF;
int f;
{
   doinfo(hh[f],0,HF);
   ff[f]->attr=reverse_attr(ff[f]->attr);
   putfield(ff[f]);
}

/* prev_field(aa,n)   find previous field to field n
 */

int prev_field(aa,n)
struct SCField *aa[];
int n;

{ 
  int i,fn;
  
  for ( i=n; ; i=fn ) {
    if (i) fn=i-1;
    else {
      for (fn=i ; aa[fn+1]!=NULL; fn++) { }
    }
    if (CanInput(aa[fn]) || fn==n) break;
  }
  return(fn);
}

/* next_field(aa,n)   find next field to field n
 *   if n is -1 find first field
 */

int next_field(aa,n)
struct SCField *aa[];
int n;

{ 
  int i,fn;
  
  for ( i=n ; ; i=fn ) {
    fn=i+1; if (aa[fn]==NULL) fn=0;
    if (CanInput(aa[fn]) || fn==n) break;
  }
  return(fn);
}

/* setwaveform(sp,wv,WFS) 
 */

setwaveform(sp,wv,WFS)
struct SCField *sp;
char *wv;
char *WFS[];

{
   int i,n;
   
   n=strlen(wv);
   for (sp->adj=i=0; WFS[i]!=NULL; i++) {
      if (strncasecmp(wv,WFS[i],n)==0) {
	 sp->adj=i; return(0);
      }
   }
}

/* dohdr(y,a1,a2,a3,h1,h2,S,V)  write header out starting at line y using 
 *    attributes a1, a2, and a3. Header lines are in arrays h1[] and h2[]
 *    S is sig line and V is version line at bottom of screen.
 */

dohdr(y,a1,a2,a3,h1,h2,S,V)
int y,a1,a2,a3;
char *h1[],*h2[],*S,*V;
{
   int i;

   for ( i=0; h1[i]!=NULL; i++) {
      mvputfstr(y+i,0,a1,h1[i],80);
   }
   for ( i=0; h2[i]!=NULL; i++) {
      mvputfstr(y+i+1,55,a2,h2[i],17);
   }
   mvputfstr(y+6,44,a3,S,strlen(S));
   mvputfstr(LINES-2,0,a3," ",80);
   mvputfstr(LINES-1,0,a1,V,80);
}

/* Display string s in the Info field of the screen, and pause for w
 * seconds, if w is non-zero
 */

doinfo(s,w,SF)
char *s;
int w;
struct SCField *SF;

{
   SF->val.s=s;
   putfield(SF);
   refresh();
   if (w) sleep(w);
}

/* dotime(F)   gettimeofday and display
 */

dotime(F)
struct SCField *F;
{
  int y,x;
  time_t t;
  struct tm *BT;
  
  t=time(NULL);
  BT=localtime(&t);
  sprintf(F->val.s,"%2d:%02d:%02d",BT->tm_hour,BT->tm_min,BT->tm_sec);
  getyx(stdscr,y,x);
  putfield(F);
  move(y,x);
  refresh();
}

dofields(aa)
struct SCField *aa[];
{
   int i;
   
   for (i=0; aa[i]!=NULL; i++) { putfield(aa[i]); }
}

dofatt(aa,at)
struct SCField *aa[];
int at;
{
   int i;
   
   for (i=0; aa[i]!=NULL; i++) { aa[i]->attr=at; }
}

mergefields(aa1,aa2)
struct SCField *aa1[],*aa2[];
{
   int i,j;
   
   for (i=0; aa1[i]!=NULL; i++) { }
   for (j=0; aa2[j]!=NULL; i++,j++) { aa1[i]=aa2[j]; }
   aa1[i]=aa2[j];      /* copy the terminating null */
}

/* scfeql(f1,f2)    make f1 contain the same value as field f2
 *                  if int/frac make dp's same, integer same, and offset
 *                  if string   make string value same and offset
 *                  if option   make option value same
 */

scfeql(f1,f2)
struct SCField *f1,*f2;
{
  if (GetType(f1)!=GetType(f2)) return(1);
  if (GetType(f2)==SCF_integer) {
    SetInt(f1,GetInt(f2)); SetOffset(f1,GetOffset(f2)); SetDP(f1,GetDP(f2));
  } else if (IsFieldStr(f2)) {
    SetOffset(f1,GetOffset(f2));
    strncpy(GetStr(f1),GetStr(f2),GetWidth(f1));
  } else if (IsFieldOption(f2)) {
    SetOptionIndex(f1,GetOptionIndex(f2));
  }
}
