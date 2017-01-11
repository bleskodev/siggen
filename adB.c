/* adB.c
 * converts gain values given as parameters into dB values
 * i.e. 1    will give 0dB
 *      0.5  will give -6dB amplitude / -3dB power
 *      10   will give 20dB amplitude / 10dB power
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define db(x) pow(10,x/20.0)
#define adb(g) (10*log10(g))

main(argc,argv)
int argc;
char **argv;
{
   int i,j,k,n;
   double x,y;
   char *p;

   argc--; argv++;
   for ( ; argc; argc--, argv++) {
      x=atof(*argv);
      if ((p=strchr(*argv,'/'))!=NULL) {
	 y=atof(++p);
	 printf("%f/%f = %f = %f dB Ampl. or %f dB power\n",
		x,y,x/y,2*adb(x/y),adb(x/y));
      } else {
	 printf("Ratio of %f = %f dB Ampl. or %f dB power\n",
		x,2*adb(x),adb(x));
      }
   }
}

/*
double db(x)
double x;
{
  return(pow(10,x/20.0);
}
*/
/*
db(x)
int x;
{
  int i;
  static int dB_Conv[20]={
      100000000,  112201845,  125892541,  141253754,  158489319,
      177827941,  199526231,  223872114,  251188643,  281838293,
      316227766,  354813389,  398107171,  446683592,  501187234,
      562341325,  630957344,  707945784,  794328235,  891250938
  };

  for (i=100000000; i && x>=20; x-=20) { i/=10; }
  return((i)?(dB_Conv[x]/i):-1);
}
*/
