/* ramp.c
 * output sequential number from start to end
 * reepeatedly!
 *
 * Usage: ramp [start [end]]
 *
 * Default values are 200 and 2000
 */

#include <stdio.h>

#define START 200
#define END   2000
#define STEP  1
#define LOOPS 10

main(argc,argv)
int argc;
char **argv;

{
  int d,step,i,st,end,l;

  st=START; end=END; step=STEP; l=LOOPS;
  argc--; argv++;
  if (argc) {
    argc--; st=atoi(*argv++);
    if (argc) {
      argc--; end=atoi(*argv++);
      if (argc) {
        argc--; step=atoi(*argv++);
        if (argc) {
          argc--; l=atoi(*argv++);
        }
      }
    }
  }

  d=(st>end);
  if (d) step=-step;

  for (; l-- ; ) {
    for (i=st; (d)?(i>=end):(i<=end); i+=step) printf("%d\n",i);
  }
}
