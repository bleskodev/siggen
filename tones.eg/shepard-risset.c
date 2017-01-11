/* shepard-risset.c   
 * generate indefinitely rising/descending tones sequence/amplitude
 * to feed to tones program.
 * 
 * Usage: shepard-risset mid-freq partials density shifts [db]
 * 
 * mid-freq    specifies the middle freq.
 * partials    is the number of seperate frequencies in the mixture
 * density     ratio between partials
 * shifts      is the number of freq shifts to create
 * [db]        optional attenuation in dB at the edges
 * 
 * Problems...........
 *  with the dB values going positive and oscillating.
 *  Initial values are correct - it's the amendment for successive shifts
 * 
 *  also need to loop a total of partial blocks of shifts specs!
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAXSHIFTS 100
#define MAXPARTIALS 50
#define DEFDB -40.0

double midfreq,density,minf,maxf;
int partials,shifts;

help(e)
int e;
{
  puts("\
Usage: shepard-risset mid-freq partials density shifts [-db]\n\
\n\
generate indefinitely rising/descending tones sequence/amplitude\n\
to feed to tones program.\n\
\n\
mid-freq    specifies the middle freq.\n\
partials    is the number of seperate frequencies in the mixture\n\
density     ratio between partials\n\
shifts      is the number of freq shifts to create\
");
  printf("-db         optional - number of db down at limits, default %.2fdb\n",DEFDB);
  printf("\nmid-freq must be >= 100Hz, partials >=3 and < %d,\n shifts >=3 and < %d, and density>0\n",
	 MAXPARTIALS, MAXSHIFTS);
  return(0);
}

main(argc,argv)
int argc;
char **argv;

{
  int i,j,k,l,m,n,N;
  double x,y,z,f,sr;
  double db,dbi,Dbs[MAXPARTIALS*MAXSHIFTS];
  int freqs[MAXPARTIALS*MAXSHIFTS];
  
  argv++; argc--;
  if (argc!=4 && argc!=5) exit(help(1));

  midfreq=strtod(argv[0],NULL);
  partials=atoi(argv[1]);
  density=strtod(argv[2],NULL);
  shifts=atoi(argv[3]);
  db=DEFDB;
  if (argc==5) db=strtod(argv[4],NULL);
      
  printf("echo midfreq=%.3fHz  number of partials is %d\n",midfreq,partials);
  printf("echo density=%.3f   number of frequency shifts=%d\n",density,shifts);
  if (midfreq<100.0 || partials<3 || partials>MAXPARTIALS 
      || shifts<3 || shifts>MAXSHIFTS || density<=0) {
    printf("mid freq must be >= 100Hz, partials >=3 and < %d,\n shifts >=3 and < %d, and density > 0",
	   MAXPARTIALS, MAXSHIFTS);
    exit(1);
  }
  maxf=midfreq*pow(density,((double)partials)/2.0);
  minf=midfreq/pow(density,((double)partials)/2.0);
  printf("echo  Freq range is %.3fHz - %.3fHz a bandwidth of %.3fHz\n",
	 minf,maxf,maxf-minf);

  N=shifts*partials;
  sr=pow(density,1/(double)shifts);  /* ratio for freq increase for each shift */
  if (db>0) db=-db;
  dbi=(2.0*-db)/(double)(N);
  for (i=0,x=db,f=minf; i<N; i++, x+=dbi, f*=sr) {
    Dbs[i]=(x<0)?x:-x;
    freqs[i]=(int)rint(f);
  }

  for (i=0; i<N; i++) {
    for ( l=i, j=0; j<partials; j++, l=(l+shifts)%N) {
      printf("%s%d@%.2f",(j)?",":"",freqs[l],Dbs[l]);
    }
    puts("");
  }
  exit(0);

  for (i=0, f=minf; i<shifts; i++, f*=sr) {
    z=(dbi*i)/shifts;
    for (j=0, x=f; j<partials; j++, x*=density) {
      y=Dbs[j]+z;
      printf("%s%d@%.2f",(j)?",":"",(int)rint(x),(y<0)?y:-y);
    }
    puts("");
  }
}
