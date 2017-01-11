/* randnote.c   
 * generate random a random walk for musical notes and duration
 * to feed to tones program to play random notes.
 * 
 * Usage: randnote startnote [millisecs [iterations]]
 * 
 * default millisecs is 250, default iterations is 1000
 */

#include <stdio.h>
char NT[6];
char Nl[]="CDEFGABC";

main(argc,argv)
int argc;
char **argv;

{
   int i,f,Id,d,N,x;
   
   argv++; argc--;
   if (argc<1 || argc>3) {
      printf("Usage: randnote start_note [duration [num_notes]]\n");
      printf("\n   start_note is letter[#]digit , where letter specifies the\n");
      printf("   note, '#' is used to sharpen the note, and digit specifies the\n");
      printf("   octave. Middle C is 'C3'.\n");
      printf("   duration is in millisecs (def. 250), def num_notes is 1000\n");
      exit(1);
   }
   Id=250; N=1000;
   strncpy(NT,*argv,5); argv++; argc--;
   NT[5]=0;
   if (islower(NT[0])) NT[0]=toupper(NT[0]);
   if (strlen(NT)!=2 || NT[0]<'A' || NT[0]>'G' || !isdigit(NT[1])) {
     printf("Illegal start note. Use A-G followed by a single digit.\n");
     exit(1);
   }
   f=((NT[0]-'C'+7)%7)+(NT[1]-'0')*7;
   if (argc) {
      Id=atoi(*argv++); argc--;
      if (argc) {
         N=atoi(*argv);
      }
   }
   srandom(time(NULL)%(Id+f));
   d=Id;
   for (i=0; i<N; i++) {
      printf("%c%c:%d\n",Nl[f%7],'0'+(f/7),d);
      f+=((random()%11)-5);
      if (f<7) f=14-f; 
      else if (f>69) f=70-f;
      x=(random()%5)-2;
      if (x>0)  d=(Id<<x);
      if (x<0)  d=(Id>>(-x));
      if (x==0) d=Id;
   }
}


