/* randtone.c   
 * generate random a random walk for freq and duration
 * to feed to tones program to play random tones.
 * 
 * Usage: randtone freq [millisecs [iterations]]
 * 
 * default millisecs is 250, default iterations is 1000
 */

#include <stdio.h>

main(argc,argv)
int argc;
char **argv;

{
   int i,f,d,N;
   
   argv++; argc--;
   if (argc<1 || argc>3) {
      printf("Usage: randtone freq [duration [num_tones]]\n");
      printf("\n   duration is in millisecs (def. 250), default num_tones is 1000.\n");
      exit(1);
   }
   d=250; N=1000;
   f=atoi(*argv); argv++; argc--;
   if (argc) {
      d=atoi(*argv++); argc--;
      if (argc) {
         N=atoi(*argv);
      }
   }
   srandom(time(NULL)%(d+f));
   for (i=0; i<N; i++) {
      printf("%d:%d\n",f,d);
      f=f+(random()%(f/4))-f/8;
      f=(f<100)?(200-f):((f>5000)?(10000-f):f);
      d=d+(random()%200)-100;
      d=(d<25)?(50-d):((d>1000)?(2000-d):d);
   }
}


