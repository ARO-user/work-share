
//  Copyright (C) 2006,2007,2008,2009, George Hobbs, Russell Edwards

/*
 *    This file is part of TEMPO2. 
 * 
 *    TEMPO2 is free software: you can redistribute it and/or modify 
 *    it under the terms of the GNU General Public License as published by 
 *    the Free Software Foundation, either version 3 of the License, or 
 *    (at your option) any later version. 
 *    TEMPO2 is distributed in the hope that it will be useful, 
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *    GNU General Public License for more details. 
 *    You should have received a copy of the GNU General Public License 
 *    along with TEMPO2.  If not, see <http://www.gnu.org/licenses/>. 
 */

/*
 *    If you use TEMPO2 then please acknowledge it by citing 
 *    Hobbs, Edwards & Manchester (2006) MNRAS, Vol 369, Issue 2, 
 *    pp. 655-672 (bibtex: 2006MNRAS.369..655H)
 *    or Edwards, Hobbs & Manchester (2006) MNRAS, VOl 372, Issue 4,
 *    pp. 1549-1574 (bibtex: 2006MNRAS.372.1549E) when discussing the
 *    timing model.
 */

/**
 * 2012-11-01 (M.Keith) Updated to use standard libtempo routines
 *                      rather than custom 'plugin' versions.
 *                      No longer requires fftw or pgplot.
 */

/* Not sure who's comments these are...*/
// 1. Try and get rid of the need for a corner frequency
// 2. Look at how white the residuals actually are
// 3. It seems as if most of the specX are okay, but the average is high.  Look at this!
// 4. Try not as steep red noise


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tempo2.h"
#include "TKspectrum.h"
#include "TKfit.h"
#include "T2toolkit.h"
#include "constraints.h"

using namespace std;

#define MAX_SPEC_BINS 1024
double OMEGA0=0; 

long double toffset = 52601.0L;
void calculateSpectrum(pulsar *psr,double *px,double *py_r,double *py_i,int *nSpec,bool compute_spectral_window);

void help() /* Display help */
{
}


extern "C" int graphicalInterface(int argc,char *argv[],pulsar *psr,int *npsr) 
{
   char parFile[MAX_PSR][MAX_FILELEN];
   char timFile[MAX_PSR][MAX_FILELEN];
   double px[MAX_SPEC_BINS];
   double py_r[MAX_SPEC_BINS];
   double py_i[MAX_SPEC_BINS];
   double tspan,minx,maxx;
   int nSpec=0;
   int newpar=0;
   char newparname[MAX_FILELEN];

   int i;
   double globalParameter;
   const char *CVS_verNum = "$Revision: 1.4 $";
   bool compute_spectral_window=false;

   if (displayCVSversion == 1) CVSdisplayVersion("grTemplate.C","plugin",CVS_verNum);

   *npsr = 1;  /* For a graphical interface that only shows results for one pulsar */

   printf("Graphical Interface: cholSpectra\n");
   printf("Author:              G. Hobbs\n");
   printf("CVS Version:         $Revision: 1.4 $\n");
   printf(" --- type 'h' for help information\n");

   /* Obtain all parameters from the command line */
   for (i=2;i<argc;i++)
   {
	  if (strcmp(argv[i],"-f")==0) {
		 strcpy(parFile[0],argv[++i]); 
		 strcpy(timFile[0],argv[++i]);
	  }
	  if ((strcmp(argv[i],"-dcf")==0) || (strcmp(argv[i],"-chol")==0)){
		 strcpy(covarFuncFile,argv[++i]);
	  }
	  if (strcmp(argv[i],"-nspec")==0) {
		 nSpec=atoi(argv[++i]);
	  }
	  if (strcmp(argv[i],"-window")==0) {
		 compute_spectral_window=true;
	  }
	  else if (strcmp(argv[i],"-outpar")==0){
	    newpar=1;
		strcpy(newparname,argv[++i]);
	  }

   }

   readParfile(psr,parFile,timFile,*npsr); /* Load the parameters       */
   readTimfile(psr,timFile,*npsr); /* Load the arrival times    */
   preProcess(psr,*npsr,argc,argv);

   for (i=0;i<2;i++)                   /* Do two iterations for pre- and post-fit residuals*/
   {
	  formBatsAll(psr,*npsr);         /* Form the barycentric arrival times */
	  formResiduals(psr,*npsr,1);    /* Form the residuals                 */
	  if (i==0) doFitDCM(psr,"NULL",covarFuncFile,*npsr,0);   /* Do the fitting     */
	  else textOutput(psr,*npsr,globalParameter,0,0,newpar,newparname);  /* Display the output */
   }
   logmsg("Producing spectra");
   for (i=0;i<psr[0].nobs;i++)
   {
	  if (i==0)
	  {
		 minx = maxx = (double)psr[0].obsn[i].sat;
	  }
	  else
	  {
		 if (minx > (double)psr[0].obsn[i].sat) minx = (double)psr[0].obsn[i].sat;
		 if (maxx < (double)psr[0].obsn[i].sat) maxx = (double)psr[0].obsn[i].sat;
	  }
   }
   tspan = maxx-minx;
   OMEGA0 = (double)(2*M_PI/tspan);
   logmsg("Doing the calculation\n");
   verbose_calc_spectra=true;
   calculateSpectrum(psr,px,py_r,py_i,&nSpec,compute_spectral_window);
   logmsg("Finished\n");
   return 0;
}

char * plugVersionCheck = TEMPO2_h_VER;

void calculateSpectrum(pulsar *psr,double *px,double *py_r,double *py_i,int *nSpec,bool compute_spectral_window) {
   int i;
   double pe[MAX_SPEC_BINS];
   double **uinv;
   FILE *fin;
   char fname[128];
   int ndays=0;
   double resx[psr->nobs],resy[psr->nobs],rese[psr->nobs];
   int ip[psr->nobs];
   FILE *fout;

   //  printf("Calculating the spectrum\n");
   uinv = malloc_uinv(psr->nobs);


   sprintf(fname,"%s.res",psr->name);
   if (!(fout = fopen(fname,"w")))
   {
	  logerr("Unable to open output file");
	  exit(1);
   }
   for (i=0;i<psr->nobs;i++)
   {
	  if (psr->obsn[i].deleted != 0)
	  {
		 logerr("cholSpectrum cannot work with deleted points.\nPlease remove from .tim file and try again.");
		 exit(1);
	  }
	  resx[i] = (double)(psr->obsn[i].sat-toffset);
	  resy[i] = (double)(psr->obsn[i].residual);
	  rese[i] = (double)(psr->obsn[i].toaErr*1.0e-6);
	  ip[i]=i;
	  fprintf(fout,"%g %g %g\n",resx[i],resy[i],rese[i]);
   }
   fclose(fout);


   //  printf("Allocated memory\n");

   // Read in the covariance function
   if (strcmp(covarFuncFile,"NULL")==0){
	  sprintf(covarFuncFile,"covarFunc.dat_%s",psr->name);
	  logmsg("Warning: Assuming -dcf %s",covarFuncFile);
   }

   logmsg("Get Cholesky 'uinv' matrix from '%s'",covarFuncFile);
   getCholeskyMatrix(uinv,covarFuncFile,psr,resx,resy,rese,psr->nobs,0,ip);
   logdbg("Got uinv, now compute spectrum.");

   if(*nSpec < 1){
	  *nSpec = 124;
	  logdbg("nSpec hardcoded to %d",*nSpec);
   }

   // Must calculate uinv for the pulsar
   calcSpectra_ri(uinv,resx,resy,psr->nobs,px,py_r,py_i,*nSpec);

   sprintf(fname,"%s.spec",psr->name);
   fout = fopen(fname,"w");
   for (i=0;i<*nSpec;i++)
	  fprintf(fout,"%g %g %g %g\n",px[i],py_r[i]*py_r[i]+py_i[i]*py_i[i],py_r[i],py_i[i]);
   fclose(fout);
   // Free uinv
   free_uinv(uinv);
}


