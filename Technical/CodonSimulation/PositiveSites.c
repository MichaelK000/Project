/* PositiveSites.c
   Ziheng Yang, June 2004 

   cc -DOLD -o PositiveSitesOLD -O2 PositiveSites.c -lm
   cc -DNEB -o PositiveSitesNEB -O2 PositiveSites.c -lm
   cc -DBEB -o PositiveSitesBEB -O2 PositiveSites.c -lm

   cl -O2 -DOLD -FePositiveSitesOLD.exe PositiveSites.c
   cl -O2 -DNEB -FePositiveSitesNEB.exe PositiveSites.c
   cl -O2 -DBEB -FePositiveSitesBEB.exe PositiveSites.c

   PositiveSites <#sites> <#repl>
   PositiveSites <#sites> <#repl> <Evolverf> <Codemlf>

   This compares siterates from evolverNSsites and mlc from codeml to calculate 
   the accuracy, power, and false positive rate of codeml inference of sites 
   under positive selection.  The measures are defined as follows (Anisimova et 
   al. 2002; Wong et al. 2004).

                             codeml inference
                              +        -         Total
        evolver    +          N++      N+-       N+.
                   -          N-+      N--       N-.
                   Total      N.+      N.-       N

   Accuracy      = N++/N.+
   Power         = N++/N+.
   FalsePositive = N-+/N-.

  The program collects N++ (NmatchB & NmatchC), N+. (NEvolver), and 
  N.+ (NCodemlB & NcodemlC), and then calculates the three measures as above.
  Note that codeml inference depends on cutoff P, hence the B (for binned) and 
  C (for cumulative) difference.  All proportions are calculated as the ratio 
  of averages, taking the ratio after counting sites over replicate data sets.  
  Output is on the screen.
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#define BEB
*/

#if(defined OLD)
char *Codemlf="mlc", startCodeml[]="Positively", startEvolver[]="replicate ";
#elif(defined BEB)
char *Codemlf="mlc", startCodeml[]="Bayes Empirical", startEvolver[]="replicate ";
#elif(defined NEB)
char *Codemlf="mlc", startCodeml[]="Naive Empirical", startEvolver[]="replicate ";
#endif


int main (int argc, char* argv[])
{
   int nbin=21, noisy=0, nr=1, ls=100;
   double  PCut[]={.525, .55, .575, .6, .625, .65, .675, .7, .725, .75, .775, .8, .825, .85, .875, .9, .925, .95, .975, .99, 1}, PCut0=0.5;
/*
   int nbin=11, noisy=0, nr=1, ls=100;
   double  PCut[]={0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 0.99, 1}, PCut0=0.5;
   int nbin=7, noisy=0, nr=1, ls=100000;
   double  PCut[]={0.6, 0.7, 0.8, 0.9, 0.95, 0.99, 1}, PCut0=0.5;
*/
   int ir, ib, i,j, ch, nmatch0;
   double NmatchB[50], NCodemlB[50], NEvolver=0;
   double NmatchC[50], NCodemlC[50];
   int *siteEvolver, *siteCodeml, *ibin, nsiteEvolver, nsiteCodeml, lline=1000;
   char *Evolverf="siterates", line[1001];
   double p, AccuracyB, AccuracyC, Power, FalsePositive;
   FILE *fEvolver, *fCodeml;

   puts("CodemlEvolver <#sites>  <#repl>");
   if(argc>1) sscanf(argv[1],"%d", &ls);
   if(argc>2) sscanf(argv[2],"%d", &nr);
   if(argc>3) Evolverf=argv[3];
   if(argc>4) Codemlf=argv[4];
   printf("%d sites, %d replicate.\n", ls,nr);
   printf("codeml file %s starts with '%s' for each replicate.\n", Codemlf, startCodeml);

   if(nr>20) { printf("Hit Enter to continue.");  getchar(); }

   fEvolver=fopen(Evolverf,"r"); fCodeml=fopen(Codemlf,"r");
   if(!fEvolver || !fCodeml) { puts("file error"); exit(-1); }

   siteEvolver=(int*)malloc(ls*3*sizeof(int));
   if(siteEvolver==NULL) { puts("oom"); exit(-1); }
   siteCodeml=siteEvolver+ls;  ibin=siteCodeml+ls;

   for(i=0; i<nbin; i++) {
      NmatchB[i]=NCodemlB[i]=0;
      NmatchC[i]=NCodemlC[i]=0;
   }
   for(ir=0; ir<nr; ir++) {
      /* Read true sites from evovler siterates */
      for( ; ; ) {
         if(fgets(line, lline, fEvolver)==NULL) break;
         if(strstr(line, startEvolver)) break;
      }
      if(!strchr(line,':')) { puts("did not find ':' in line."); exit(-1); }
      sscanf(strchr(line,':')+1, "%d", &nsiteEvolver);
      if(nsiteEvolver>ls) { puts("Too many sites. ls wrong?"); exit(-1); }
      NEvolver+=nsiteEvolver;
      for(i=0; i<nsiteEvolver; i++)
         fscanf(fEvolver, "%d", &siteEvolver[i]);
      if(noisy) {
         printf("\n\n%d sites from Evolver:\n", nsiteEvolver);
         for(i=0;i<nsiteEvolver; i++) printf(" %3d", siteEvolver[i]);
      }

      /* Read inferred sites and probs from codeml mlc or rst, bin probs */
      for( ; ; ) {
         if(fgets(line, lline, fCodeml)==NULL) break;
         if(strstr(line, startCodeml)) break;
      }
      for(i=0; i<3; i++) fgets(line, lline, fCodeml);
      for(i=nsiteCodeml=0; i<ls; i++,nsiteCodeml++) {
         if(fscanf(fCodeml, "%d %c%lf", &siteCodeml[i], &ch, &p)!=3) break;
         fgets(line, lline, fCodeml);
         for(j=0; j<nbin-1; j++)  if(p<=PCut[j]) break;
         ibin[i]=j;
      }
      if(noisy) {
         printf("\n%d sites from codeml at 50%%:\n", nsiteCodeml);
         for(i=0; i<nsiteCodeml; i++)  printf("%4d", siteCodeml[i]);
      }

      /* count matches by looping over codeml sites */
      for(i=0,nmatch0=0; i<nsiteCodeml; i++) {
         ib=ibin[i];
         NCodemlB[ib]++;
         for(j=0; j<=ib; j++) NCodemlC[j]++;
         for(j=0; j<nsiteEvolver; j++)
            if(siteCodeml[i]==siteEvolver[j]) break;
         if(j<nsiteEvolver) {  /* a match */
            nmatch0++;
            NmatchB[ib]++;
            for(j=0; j<=ib; j++)
               NmatchC[j]++;
         }
      }

      printf("\nReplicate %3d: %3d evolver sites, %3d codeml sites at 50%%, %3d matches", 
                ir+1,nsiteEvolver,nsiteCodeml,nmatch0);
   }

   printf("\n\n%6s%22s%10s%17s%10s%10s\n\n", 
      "P", "AccuracyBin", "Pcut", "AccuracyCum", "Power", "FalsePos");
   for(j=0; j<nbin; j++) {
      AccuracyB = (NmatchB[j] ? NmatchB[j]/NCodemlB[j] : 0);
      AccuracyC = (NmatchC[j] ? NmatchC[j]/NCodemlC[j] : 0);
      Power = (NmatchC[j] ? NmatchC[j]/NEvolver : 0);
      FalsePositive = NCodemlC[j]-NmatchC[j];
      if(FalsePositive) FalsePositive/=(ls*nr-NEvolver);

      p = (j==0 ? PCut0 : PCut[j-1]);
      printf("%5.3f - %5.3f: %7.3f (%5.0f) ",   p, PCut[j], AccuracyB, NCodemlB[j]);
      printf( "   >%4.3f: %7.3f (%5.0f) %7.3f %7.3f\n", p,  AccuracyC, NCodemlC[j], Power, FalsePositive);
   }
   printf( "\nTrue positive sites from evolver: %5.0f out of %5d\n", NEvolver,ls*nr);
   fclose(fEvolver); fclose(fCodeml);
}
