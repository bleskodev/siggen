/* dB.c
 * converts dB valuesgiven as parameters into linear ratio values
 * i.e. dB 20   will give the value 10
 *      dB 26   will give the value 20
 *      dB 0                        1
 */

#include <math.h>

#define db(x) pow(10,x/20.0)

main(argc,argv)
int argc;
char **argv;
{
  int i,j,k,n,inv;
  double x;

  argc--; argv++;
  for ( inv=0 ; argc; argc--, argv++) {
    if (strcmp(*argv,"-i")==0) { inv=1; continue; }
    x=(double)(atoi(*argv));
    if (inv==0)
      printf("%fdB = %f times\n",x,db(x));
    else
      printf("%fdB = %f times\n",20.0*log10(x),x);
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
