#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "tempo2.h" // If you need to remove this then delete TKleastSquares_svd_psr
#include "T2toolkit.h"
#include "TKfit.h"
#include "T2accel.h"
#include <string.h>

double TKpythag(double a,double b);
void TKbidiagonal(double **a,double *anorm,int ndata,int nfit,double **v,double *w,double **u,double *rv1);

void TKremovePoly_f(float *px,float *py,int n,int m)
{
   int i,j;
   double x[n],y[n];
   double p[m];
   double chisq;
   double v[m];

   for (i=0;i<n;i++)
   {
	  x[i] = (float)px[i];
	  y[i] = (float)py[i];
   }
   TKleastSquares_svd_noErr(x,y,n, p, m, TKfitPoly);  

   for (i=0;i<n;i++)
   {
	  TKfitPoly(x[i],v,m);
	  for (j=0;j<m;j++)
		 py[i] -= v[j]*p[j];
   }
}

void TKremovePoly_d(double *x,double *y,int n,int m)
{
   int i,j;
   double p[m];
   double chisq;
   double v[m];

   TKleastSquares_svd_noErr(x,y,n, p, m, TKfitPoly);  

   for (i=0;i<n;i++)
   {
	  TKfitPoly(x[i],v,m);
	  for (j=0;j<m;j++)
		 y[i] -= v[j]*p[j];
   }
}

void TKfitPoly(double x,double *v,int m)
{
   int i;
   double t=1;
   for (i=0;i<m;i++)
   {
	  v[i] = t;
	  t*=x;
   }
}

/* Least squares fitting routines */

void TKleastSquares_svd_psr(double *x,double *y,double *sig,int n,double *p,double *e,int nf,double **cvm, double *chisq, void (*fitFuncs)(double, double [], int,pulsar *,int),int weight,pulsar *psr,double tol, int *ip)
{
   double **designMatrix; //[n][nf];
   double basisFunc[nf],b[n];
   double **v,**u;
   double w[nf],wt[nf],sum,wmax;
   int    i,j,k;
   if (!(designMatrix = (double **)malloc(n*sizeof(double *))))
   {
	  printf("Unable to allocate enough memory for the design matrix\n");
	  exit(1);
   }
   if (!(v = (double **)malloc(nf*sizeof(double *))))
   {
	  printf("Unable to allocate enough memory for the fitting\n");
	  exit(1);      
   }
   if (!(u = (double **)malloc(n*sizeof(double *))))
   {
	  printf("Unable to allocate enough memory for the fitting\n");
	  exit(1);      
   }

   for (i=0;i<n;i++) 
   {
	  if ((designMatrix[i] = (double *)malloc(nf*sizeof(double))) == NULL) {printf("OUT OF MEMORY\n"); exit(1);}
	  if ((u[i] = (double *)malloc(nf*sizeof(double))) == NULL)  {printf("OUT OF MEMORY\n"); exit(1);}
	  if (weight==0) sig[i]=1.0;
   }
   for (i=0;i<nf;i++) v[i] = (double *)malloc(nf*sizeof(double));

   /* This routine has been developed from Section 15 in Numerical Recipes */

   /* Determine the design matrix - eq 15.4.4 
	* and the vector 'b' - eq 15.4.5 
	*/
   //  printf("Set b\n");
   //  printf("Got this far 3\n");

   for (i=0;i<n;i++)
   {
	  //      printf("Doing the fit\n");
	  fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  //      printf("Done the fit\n");
	  for (j=0;j<nf;j++) 
	  {
		 //	  printf("Setting %d %d max = %d %d\n",i,j,n,nf);
		 designMatrix[i][j] = basisFunc[j]/sig[i];
	  }
	  b[i] = y[i]/sig[i];
   }


   if(writeResiduals==1){
	  logdbg("Writing out prefit residuals");
	  FILE* wFile=fopen("prefit.res","w");
	  for (i=0;i<n;i++)
		 fprintf(wFile,"%lg %lg %lg\n",x[i],y[i],b[i]);
	  fclose(wFile);

	  wFile=fopen("design.matrix","w");
	  for (i=0;i<n;i++) {
		 for (j=0;j<nf;j++){
			fprintf(wFile,"%d %d %lg\n",i,j,designMatrix[i][j]);
		 }
		 fprintf(wFile,"\n");
	  }
	  fclose(wFile);

   }



   /* Now carry out the singular value decomposition */
   //  printf("Decomposition\n");
   //  printf("Got this far 4\n");

   TKsingularValueDecomposition_lsq(designMatrix,n,nf,v,w,u);
   //  printf("Weights\n");
   wmax = TKfindMax_d(w,nf);
   if (wmax > pow(2,55)){
	  logerr("Warning: wmax very large. Precision issues likely to break fit\nwmax=%lf\ngood=%lf",wmax,pow(2,55));
   }

   for (i=0;i<nf;i++)
   {
	  if (w[i] < tol*wmax) w[i]=0.0;
   }
   //  printf("Back substitution\n");
   /* Back substitution */
   TKbacksubstitution_svd(v, w, designMatrix, b, p, n, nf);
   //  printf("Wts\n");
   /* Now form the covariance matrix */
   for (i=0;i<nf;i++)
   {
	  //      printf("w[i] = %g\n",w[i]);
	  if (w[i]!=0) wt[i] = 1.0/w[i]/w[i];
	  else wt[i] = 0.0;     
   }
   for (i=0;i<nf;i++)
   {
	  for (j=0;j<=i;j++)
	  {
		 sum=0.0;
		 for (k=0;k<nf;k++)
			sum+=v[i][k]*v[j][k]*wt[k];
		 cvm[i][j] = cvm[j][i] = sum;
	  }
	  e[i] = sqrt(cvm[i][i]);
   }
   *chisq = 0.0;
   for (j=0;j<n;j++)
   {
	  fitFuncs(x[j],basisFunc,nf,psr,ip[j]);
	  sum=0.0;
	  for (k=0;k<nf;k++)
	  {
		 sum+=p[k]*basisFunc[k];
		 if (j==0)free(v[k]);
	  }
	  (*chisq) += pow((y[j]-sum)/sig[j],2);

	  free(designMatrix[j]); // Remove memory allocation
	  free(u[j]);

   }
   if (weight==0 || (weight==1 && psr->rescaleErrChisq==1))
   {
	  for (j=0;j<nf;j++)
		 e[j] *= sqrt(*chisq/(n-nf));
   }
   //  printf("Done\n");
   //
   if (writeResiduals){
	  for (i=0;i<n;i++)
	  {
		 b[i]=y[i];
		 for (j=0;j<nf;j++){
			// get the correction for each parameter
			fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
			b[i] -= basisFunc[j]*p[j];
		 }
	  }
	  FILE* wFile=fopen("postfit.res","w");
	  for (i=0;i<n;i++)
		 fprintf(wFile,"%lg %lg %lg\n",x[i],b[i],b[i]/sig[i]);
	  fclose(wFile);

   }



   free(v); 
   free(u); free(designMatrix);
}


void TKleastSquares_svd_psr_dcm(double *x,double *y,double *sig,int n,double *p,double *e,int nf,double **cvm, double *chisq, void (*fitFuncs)(double, double [], int,pulsar *,int),int weight,pulsar *psr,double tol, int *ip,double **uinv)
{
   double **designMatrix; //[n][nf];
   double basisFunc[nf],b[n];
   double **v,**u,**uout;
   double w[nf],wt[nf],sum,wmax,*bout;
   int    i,j,k;

   if(uinv[n-1] != uinv[0]+(n-1)*n){
	  logerr("uinv matrix not declared as consecutive memory.");
	  logmsg("Please use malloc_uinv() and free_uinv() from cholesky.C");
	  logmsg("and - ensure that uinv is %d by %d",n,n);
#ifdef ACCEL_MULTMATRIX
	  // if we are using the accelerated code then it will crash... otherwise it's just a warning
	  exit(1);
#endif
   }

   // store memory in BLAS compatible format for accelerator code.
   double* _designMatrix=(double*)malloc(sizeof(double)*n*nf);
   double* _v=(double*)malloc(sizeof(double)*nf*nf);
   double* _u=(double*)malloc(sizeof(double)*n*nf);
   double* _uout=(double*)malloc(sizeof(double)*n*nf);

   if((_designMatrix == NULL)||(_v==NULL)||(_u==NULL)||(_uout==NULL)){
	  logerr("OUT OF MEMORY");
	  exit(1);
   }

   designMatrix = (double **)malloc(n*sizeof(double *)); // malloc-TKleastSquares_svd_psr_dcm-designMatrix**
   v = (double **)malloc(nf*sizeof(double *));   // malloc-TKleastSquares_svd_psr_dcm-v**
   u = (double **)malloc(n*sizeof(double *));    // malloc-TKleastSquares_svd_psr_dcm-u**
   uout = (double **)malloc(n*sizeof(double *)); // malloc-TKleastSquares_svd_psr_dcm-uout**
   bout = (double *)malloc(n*sizeof(double));    // malloc-TKleastSquares_svd_psr_dcm-bout*


   // create "C" style pointers to arrays for ease of use
   for (i=0;i<n;i++) 
   {
	  designMatrix[i]=_designMatrix + nf*i;
	  u[i]=_u+nf*i;
	  uout[i]=_uout+nf*i;
	  sig[i]=1.0;
   }
   for (i=0;i<nf;i++) v[i] = _v+nf*i;
   /* This routine has been developed from Section 15 in Numerical Recipes */

   /* Determine the design matrix - eq 15.4.4 
	* and the vector 'b' - eq 15.4.5 
	*/
   for (i=0;i<n;i++)
   {
	  // fitFuncs is not threadsafe!
	  fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  for (j=0;j<nf;j++) designMatrix[i][j] = basisFunc[j]/sig[i];
	  b[i] = y[i]/sig[i];
	  //      for (j=0;j<nf;j++)
	  //      	printf("Designmatrix = %g %g %g\n",designMatrix[i][j],basisFunc[j],sig[i]);
   }
   // Take into account the data covariance matrix
   TKmultMatrix(uinv,designMatrix,n,nf,uout);  
   TKmultMatrixVec(uinv,b,n,bout);
   if(writeResiduals==1){
	  logdbg("Writing out whitened residuals");
	  FILE* wFile=fopen("prefit.res","w");
	  for (i=0;i<n;i++)
		 fprintf(wFile,"%lg %lg %lg\n",x[i],y[i],bout[i]);
	  fclose(wFile);

	  wFile=fopen("design.matrix","w");
	  for (i=0;i<n;i++) {
		 for (j=0;j<nf;j++){
			fprintf(wFile,"%d %d %lg %lg\n",i,j,designMatrix[i][j],uout[i][j]);
		 }
		 fprintf(wFile,"\n");
	  }
	  fclose(wFile);

   }
   //replace things by their whitened versions
   for (i=0;i<n;i++)
   {
	  for (j=0;j<nf;j++)
	  {
		 designMatrix[i][j] = uout[i][j];
	  }
   }


   for (i=0;i<n;i++)
	  b[i] = bout[i];
   /* Now carry out the singular value decomposition */
   TKsingularValueDecomposition_lsq(designMatrix,n,nf,v,w,u);
   wmax = TKfindMax_d(w,nf);
   if (wmax > pow(2,55)){
	  logerr("Warning: wmax very large. Precision issues likely to break fit\nwmax=%lf\ngood=%lf",wmax,pow(2,55));
   }

   for (i=0;i<nf;i++)
   {
	  if (w[i] < tol*wmax) w[i]=0.0;
   }
   /* Back substitution */
   TKbacksubstitution_svd(v, w, designMatrix, b, p, n, nf);
   /* Now form the covariance matrix */
   for (i=0;i<nf;i++)
   {
	  if (w[i]!=0) wt[i] = 1.0/w[i]/w[i];
	  else wt[i] = 0.0;     
   }
   for (i=0;i<nf;i++)
   {
	  for (j=0;j<=i;j++)
	  {
		 sum=0.0;
		 for (k=0;k<nf;k++)
			sum+=v[i][k]*v[j][k]*wt[k];
		 cvm[i][j] = cvm[j][i] = sum;
	  }
	  e[i] = sqrt(cvm[i][i]);
   } 

   if(debugFlag==1) {
	  FILE *fout;
	  fout = fopen("cvm.matrix","w");
	  for (i=0;i<nf;i++)
	  {
		 for (j=0;j<=i;j++)
		 {
			fprintf(fout,"%+.8f ",cvm[i][j]/sqrt(cvm[i][i]*cvm[j][j]));
			// fprintf(fout,"%g ",cvm[i][j]); ///sqrt(cvm[i][i]*cvm[j][j]));
		 }
		 fprintf(fout,"\n");
	  }
	  fclose(fout);
   }
   *chisq = 0.0;
   for (i=0;i<n;i++)
   {
	  // fitFuncs is not threadsafe!
	  fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  for (j=0;j<nf;j++) designMatrix[i][j] = basisFunc[j];
	  b[i] = y[i];
   }
   TKmultMatrix(uinv,designMatrix,n,nf,uout);  
   for (i=0;i<n;i++)
   {
	  for (j=0;j<nf;j++)
		 designMatrix[i][j] = uout[i][j];
   }
   TKmultMatrixVec(uinv,b,n,bout);
   for (i=0;i<n;i++) b[i] = bout[i];
   for (j=0;j<n;j++)
   {
	  sum = 0.0;
	  for (k=0;k<nf;k++)
	  {
		 sum+=p[k]*designMatrix[j][k];
	  }
	  (*chisq) += pow((b[j]-sum)/sig[j],2);
   }
   //  printf("chisq = %g %g\n",*chisq,*chisq/(n-nf));


   if (weight==0 || (weight==1 && psr->rescaleErrChisq==1))
   {
	  for (j=0;j<nf;j++)
		 e[j] *= sqrt(*chisq/(n-nf));
   }

   if (writeResiduals){
	  for (i=0;i<n;i++)
	  {
		 b[i]=y[i];
		 for (j=0;j<nf;j++){
			// get the correction for each parameter
			fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
			b[i] -= basisFunc[j]*p[j];
		 }
	  }
	  TKmultMatrixVec(uinv,b,n,bout);
	  FILE* wFile=fopen("postfit.res","w");
	  for (i=0;i<n;i++)
		 fprintf(wFile,"%lg %lg %lg\n",x[i],b[i],bout[i]);
	  fclose(wFile);

   }




   // this funny method of freeing is because of the BLAS style matricies. M.Keith 2012
   free(v[0]);       // free-TKleastSquares_svd_psr_dcm-v*
   free(uout[0]);         // free-TKleastSquares_svd_psr_dcm-uout*
   free(designMatrix[0]); // free-TKleastSquares_svd_psr_dcm-designMatrix*
   free(u[0]);            // free-TKleastSquares_svd_psr_dcm-u*

   free(uout);  // free-TKleastSquares_svd_psr_dcm-uout**
   free(bout);  // free-TKleastSquares_svd_psr_dcm-bout*
   free(v);     // free-TKleastSquares_svd_psr_dcm-v**
   free(u);     // free-TKleastSquares_svd_psr_dcm-u**
   free(designMatrix); // free-TKleastSquares_svd_psr_dcm-designMatrix**
}



void TKleastSquares_svd_noErr(double *x,double *y,int n,double *p,int nf, void (*fitFuncs)(double, double [], int))
{
   double **designMatrix; //[n][nf];
   double basisFunc[nf],b[n];
   double **v,**u;
   double w[nf],wt[nf],sum,wmax;
   double tol = 1.0e-5;
   int    i,j,k;

   designMatrix = (double **)malloc(n*sizeof(double *));
   v = (double **)malloc(nf*sizeof(double *));
   u = (double **)malloc(n*sizeof(double *));
   for (i=0;i<n;i++) 
   {
	  designMatrix[i] = (double *)malloc(nf*sizeof(double));
	  u[i] = (double *)malloc(nf*sizeof(double));
   }
   for (i=0;i<nf;i++) v[i] = (double *)malloc(nf*sizeof(double));

   /* This routine has been developed from Section 15 in Numerical Recipes */

   /* Determine the design matrix - eq 15.4.4 
	* and the vector 'b' - eq 15.4.5 
	*/
   for (i=0;i<n;i++)
   {
	  //      fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  fitFuncs(x[i],basisFunc,nf);
	  for (j=0;j<nf;j++) designMatrix[i][j] = basisFunc[j];
	  b[i] = y[i];
   }
   /* Now carry out the singular value decomposition */
   TKsingularValueDecomposition_lsq(designMatrix,n,nf,v,w,u);
   wmax = TKfindMax_d(w,nf);
   for (i=0;i<nf;i++)
   {
	  if (w[j] < tol*wmax) w[j]=0.0;
   }

   /* Back substitution */
   TKbacksubstitution_svd(v, w, designMatrix, b, p, n, nf);

   // Remove memory allocation
   for (j=0;j<n;j++)
   {
	  if (j==0)
	  {
		 for (k=0;k<nf;k++) free(v[k]);
	  }
	  free(designMatrix[j]); 
	  free(u[j]);      
   }



   free(v); 
   free(u); free(designMatrix);
}

void TKleastSquares_svd(double *x,double *y,double *sig2,int n,double *p,double *e,int nf,double **cvm, double *chisq, void (*fitFuncs)(double, double [], int),int weight)
{
   double **designMatrix; //[n][nf];
   double basisFunc[nf],b[n];
   double sig[n];
   double **v,**u;
   double w[nf],wt[nf],sum,wmax;
   double tol = 1.0e-5;
   int    i,j,k;
   designMatrix = (double **)malloc(n*sizeof(double *));
   v = (double **)malloc(nf*sizeof(double *));
   u = (double **)malloc(n*sizeof(double *));
   for (i=0;i<n;i++) 
   {
	  designMatrix[i] = (double *)malloc(nf*sizeof(double));
	  u[i] = (double *)malloc(nf*sizeof(double));
	  if (weight==0) sig[i]=1.0;
	  else sig[i] = sig2[i];  //to avoid side effect of setting all errors on the INPUT time series to 1.
   }
   for (i=0;i<nf;i++) v[i] = (double *)malloc(nf*sizeof(double));

   /* This routine has been developed from Section 15 in Numerical Recipes */

   /* Determine the design matrix - eq 15.4.4 
	* and the vector 'b' - eq 15.4.5 
	*/
   for (i=0;i<n;i++)
   {
	  //      fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  fitFuncs(x[i],basisFunc,nf);
	  for (j=0;j<nf;j++) designMatrix[i][j] = basisFunc[j]/sig[i];
	  b[i] = y[i]/sig[i];
   }
   /* Now carry out the singular value decomposition */

   TKsingularValueDecomposition_lsq(designMatrix,n,nf,v,w,u);
   wmax = TKfindMax_d(w,nf);
   for (i=0;i<nf;i++)
   {
	  if (w[i] < tol*wmax) w[i]=0.0;
   }

   /* Back substitution */
   TKbacksubstitution_svd(v, w, designMatrix, b, p, n, nf);

   /* Now form the covariance matrix, giving the covariance between each of the fitted parameters */
   for (i=0;i<nf;i++)
   {
	  if (w[i]!=0) wt[i] = 1.0/w[i]/w[i];
	  else wt[i] = 0.0;     
   }
   for (i=0;i<nf;i++)
   {
	  for (j=0;j<=i;j++)
	  {
		 sum=0.0;
		 for (k=0;k<nf;k++)
			sum+=v[i][k]*v[j][k]*wt[k];
		 cvm[i][j] = sum;
		 cvm[j][i] = sum;
	  }
	  e[i] = sqrt(cvm[i][i]); // this gives the standard deviation (i.e. error) in the estimate of the i-th parameter
   }
   *chisq = 0.0;
   for (j=0;j<n;j++)
   {
	  fitFuncs(x[j],basisFunc,nf);
	  sum=0.0;
	  for (k=0;k<nf;k++)
	  {
		 sum+=p[k]*basisFunc[k];
		 if (j==0) free(v[k]);
	  }
	  (*chisq) += pow((y[j]-sum)/sig[j],2);

	  free(designMatrix[j]); // Remove memory allocation
	  free(u[j]);
   }

   if (weight==0)
   {
	  for (j=0;j<nf;j++)
		 e[j] *= sqrt(*chisq/(n-nf));
   }

   //printf("at end of memory allocation in TKfit.C0.5\n");
   //free(v);     //TRY TO UNCOMMENT THIS LINE AT SOME POINT!! Hmm actually maybe it's not necessary...
   //printf("at end of memory allocation in TKfit.C1\n");
   free(u); 
   //printf("at end of memory allocation in TKfit.C1.5\n");
   free(designMatrix);
   //printf("at end of memory allocation in TKfit.C2\n");
}

void TKleastSquares_svd_passN(double *x,double *y,double *sig2,int n,double *p,double *e,int nf,double **cvm, double *chisq, void (*fitFuncs)(double, double [], int,int),int weight)
{
   double **designMatrix; //[n][nf];
   double basisFunc[nf],b[n];
   double sig[n];
   double **v,**u;
   double w[nf],wt[nf],sum,wmax;
   double tol = 1.0e-5;
   int    i,j,k;
   designMatrix = (double **)malloc(n*sizeof(double *));
   v = (double **)malloc(nf*sizeof(double *));
   u = (double **)malloc(n*sizeof(double *));
   for (i=0;i<n;i++) 
   {
	  designMatrix[i] = (double *)malloc(nf*sizeof(double));
	  u[i] = (double *)malloc(nf*sizeof(double));
	  if (weight==0) sig[i]=1.0;
	  else sig[i] = sig2[i];  //to avoid side effect of setting all errors on the INPUT time series to 1.
   }
   for (i=0;i<nf;i++) v[i] = (double *)malloc(nf*sizeof(double));

   /* This routine has been developed from Section 15 in Numerical Recipes */

   /* Determine the design matrix - eq 15.4.4 
	* and the vector 'b' - eq 15.4.5 
	*/
   for (i=0;i<n;i++)
   {
	  //      fitFuncs(x[i],basisFunc,nf,psr,ip[i]);
	  fitFuncs(x[i],basisFunc,nf,i);
	  for (j=0;j<nf;j++) designMatrix[i][j] = basisFunc[j]/sig[i];
	  b[i] = y[i]/sig[i];
   }
   /* Now carry out the singular value decomposition */

   TKsingularValueDecomposition_lsq(designMatrix,n,nf,v,w,u);
   wmax = TKfindMax_d(w,nf);
   for (i=0;i<nf;i++)
   {
	  if (w[j] < tol*wmax) w[j]=0.0;
   }

   /* Back substitution */
   TKbacksubstitution_svd(v, w, designMatrix, b, p, n, nf);

   /* Now form the covariance matrix, giving the covariance between each of the fitted parameters */
   for (i=0;i<nf;i++)
   {
	  if (w[i]!=0) wt[i] = 1.0/w[i]/w[i];
	  else wt[i] = 0.0;     
   }
   for (i=0;i<nf;i++)
   {
	  for (j=0;j<=i;j++)
	  {
		 sum=0.0;
		 for (k=0;k<nf;k++)
			sum+=v[i][k]*v[j][k]*wt[k];
		 cvm[i][j] = sum;
		 cvm[j][i] = sum;
	  }
	  e[i] = sqrt(cvm[i][i]); // this gives the standard deviation (i.e. error) in the estimate of the i-th parameter
   }
   *chisq = 0.0;
   for (j=0;j<n;j++)
   {
	  fitFuncs(x[j],basisFunc,nf,j);
	  sum=0.0;
	  for (k=0;k<nf;k++)
	  {
		 sum+=p[k]*basisFunc[k];
		 if (j==0) free(v[k]);
	  }
	  (*chisq) += pow((y[j]-sum)/sig[j],2);

	  free(designMatrix[j]); // Remove memory allocation
	  free(u[j]);
   }

   if (weight==0)
   {
	  for (j=0;j<nf;j++)
		 e[j] *= sqrt(*chisq/(n-nf));
   }

   //printf("at end of memory allocation in TKfit.C0.5\n");
   //free(v);     //TRY TO UNCOMMENT THIS LINE AT SOME POINT!! Hmm actually maybe it's not necessary...
   //printf("at end of memory allocation in TKfit.C1\n");
   free(u); 
   //printf("at end of memory allocation in TKfit.C1.5\n");
   free(designMatrix);
   //printf("at end of memory allocation in TKfit.C2\n");
}



/* Calculates SVD by following technique given in wikipedia */
void TKsingularValueDecomposition_lsq(double **designMatrix,int n,int nf,double **v,double *w,double **u)
{
   double an;
   int i,j,k,its,l,nm,jj;
   int max_its = 40,pos1;
   double c,s,f,g,h,y,z,x;
   double rv1[nf];
   /* For A = U.W.V^T - obtain U, W and V */  

   /* Step 1: Reduce the matrix to a bidiagonal matrix */
   TKbidiagonal(designMatrix,&an,n,nf,v,w,u,rv1);
   /* Step 2: Compute the SVD of the bidiagonal matrix */

   // Diagonalisation of the bidiagonal form: Loop over singular values
   // Code based on the numerical recipes routine
   for (k=nf-1;k>=0;k--)
   {
	  for (its=1;its<=max_its;its++)
	  {
		 pos1=0;
		 for (l=k;l>=0;l--)
		 {
			nm=l-1;
			if ((fabs(rv1[l])+an)==an) {pos1=2; break;}
			if ((fabs(w[nm])+an)==an)  {pos1=1; break;}
		 }
		 if (pos1!=2)
		 {
			c=0.0;
			s=1.0;
			for (i=l;i<=k;i++) // Check <= sign
			{
			   f=s*rv1[i];		  
			   rv1[i]=c*rv1[i];
			   if ((fabs(f)+an)==an) break;
			   g=w[i];
			   h=TKpythag(f,g);
			   w[i]=h;
			   h=1.0/h;
			   c= (g*h);
			   s= -(f*h);
			   for (j=0;j<n;j++)
			   {
				  y = designMatrix[j][nm];
				  z = designMatrix[j][i];
				  designMatrix[j][nm] = (y*c)+(z*s);
				  designMatrix[j][i] = -(y*s)+(z*c);
			   }
			}
		 }	      
		 z = w[k];
		 if (l==k)
		 {
			if (z < 0.0)
			{
			   w[k] = -z;
			   for (j=0;j<nf;j++)
				  v[j][k] = -v[j][k];
			}
			break;
		 }
		 if (its==30)
		 {
			printf("1. No convergence in singular value decomposition after 30 iterations\n");
			exit(1);
		 }
		 x = w[l];
		 nm = k-1;
		 y = w[nm];
		 g =rv1[nm];
		 h= rv1[k];
		 f = ((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
		 g = TKpythag(f,1.0);
		 f = ((x-z)*(x+z)+h*((y/(f+TKsign_d(g,f)))-h))/x;
		 c=1.0;
		 s=1.0;
		 for (j=l;j<=nm;j++)
		 {
			i = j+1;
			g = rv1[i];
			y = w[i];
			h = s*g;
			g = c*g;
			z = TKpythag(f,h);
			rv1[j] = z;
			c = f/z;
			s = h/z;
			f = (x*c)+(g*s);
			g = -(x*s)+(g*c);
			h = y*s;
			y = y*c;
			for (jj=0;jj<nf;jj++)
			{
			   x = v[jj][j];
			   z = v[jj][i];
			   v[jj][j] = (x*c)+(z*s);
			   v[jj][i] = -(x*s)+(z*c);		  
			}
			z = TKpythag(f,h);
			w[j] = z;
			if (z != 0.0)
			{
			   z = 1.0/z;
			   c=f*z;
			   s=h*z;
			}
			f = (c*g)+(s*y);
			x = -(s*g)+(c*y);
			for (jj=0;jj<n;jj++)
			{
			   y = designMatrix[jj][j];
			   z = designMatrix[jj][i];
			   designMatrix[jj][j] = (y*c)+(z*s);
			   designMatrix[jj][i] = -(y*s)+(z*c);		  
			}
		 }
		 rv1[l] = 0.0;
		 rv1[k] = f;
		 w[k]=x;
	  }
   }

}


/* Use Householder reflections to do reduce the matrix to bidiagonal form       */
/* This is converted from the Fortran EISPACK code which is very similar        */
/* to the more recent numerical recipes code - originally this came from        */
/* the algol procedure svd, num. math. 14, 403-420 (1970) by Golub and Reinsch  */
/* The Fortran code was developed by Burton S. Garbow from the Argonne National */
/* laboratory (1983)                                                            */

void TKbidiagonal(double **a,double *an,int ndata,int nfit,double **v,double *w,double **u,double *rv1)
{
   int i,j,k,l;
   double g=0.0;
   double scale=0.0,s,f,h;

   *an=0.0;
   for (i=0;i<nfit;i++)
   {
	  l=i+1;
	  rv1[i] = scale*g;
	  g=0.0; s=0.0; scale = 0.0;
	  if (i <= ndata)
	  {
		 for (k=i;k<ndata;k++)
			scale+=fabs(a[k][i]);
		 if (scale != 0.0)
		 {
			for (k=i;k<ndata;k++)
			{
			   a[k][i]/=scale;
			   s+=a[k][i]*a[k][i];
			}
			f=a[i][i];
			g=-TKsign_d(sqrt(s),f);
			h = f*g-s;
			a[i][i]=f-g;
			for (j=l;j<nfit;j++)
			{
			   s=0.0;
			   for (k=i;k<ndata;k++)
				  s+=a[k][i]*a[k][j];
			   f = s/h;
			   for (k=i;k<ndata;k++)
				  a[k][j]+=f*a[k][i];
			}
			for (k=i;k<ndata;k++)
			   a[k][i]*=scale;	      

		 }
	  }

	  w[i]=scale*g;
	  g=0.0;
	  s=0.0;
	  scale=0.0;
	  if (i<=ndata && i != nfit)
	  {
		 for (k=l;k<nfit;k++)
			scale+=fabs(a[i][k]);
		 if (scale != 0.0)
		 {
			for (k=l;k<nfit;k++)
			{
			   a[i][k] = a[i][k]/scale;
			   s+=a[i][k]*a[i][k];
			}
			f=a[i][l];
			g=-TKsign_d(sqrt(s),f);
			h=f*g-s;
			a[i][l] = f-g;
			for (k=l;k<nfit;k++)
			   rv1[k]=a[i][k]/h;
			for (j=l;j<ndata;j++)
			{
			   s=0.0;
			   for (k=l;k<nfit;k++)
				  s+=a[j][k]*a[i][k];
			   for (k=l;k<nfit;k++)
				  a[j][k]+=s*rv1[k];
			}
			for (k=l;k<nfit;k++)
			   a[i][k]=scale*a[i][k];
		 }
	  }
	  *an=TKretMax_d(*an,(fabs(w[i])+fabs(rv1[i])));
   }

   // Accumulation of right-hand transformations
   for (i=nfit-1;i>=0;i--)
   {
	  if (i < nfit-1)
	  {
		 if (g != 0.0)
		 {
			for (j=l;j<nfit;j++) v[j][i]=(a[i][j]/a[i][l])/g;
			for (j=l;j<nfit;j++)
			{
			   s=0.0;
			   for (k=l;k<nfit;k++) s+=a[i][k]*v[k][j];
			   for (k=l;k<nfit;k++) v[k][j]+=s*v[k][i];
			}			 
		 }
		 for (j=l;j<nfit;j++)
			v[i][j] = v[j][i] = 0.0;
	  }
	  v[i][i]=1.0;
	  g = rv1[i];
	  l=i;
   }

   // Accumulation of left-hand transformations
   for (i=TKretMin_i(ndata,nfit)-1;i>=0;i--)
   {
	  l=i+1;
	  g=w[i];
	  for (j=l;j<nfit;j++) a[i][j]=0.0;
	  if (g != 0.0)
	  {
		 g=1.0/g;
		 for (j=l;j<nfit;j++)
		 {
			s=0.0;
			for (k=l;k<ndata;k++) s+=a[k][i]*a[k][j];
			f = (s/a[i][i])*g;
			for (k=i;k<ndata;k++) a[k][j]=a[k][j]+f*a[k][i];
		 }
		 for (j=i;j<ndata;j++) a[j][i]*=g;
	  }
	  else
	  {
		 for (j=i;j<ndata;j++) a[j][i]=0.0;
	  }
	  a[i][i]++;
   }

}

/* Solves A.X = B for vector X using equation 2.6.7 in numerical recipes */
/* equation: x = V . [diag(1/w_j)] . (U^T.b)                             */ 
/*                                                                       */
/* Returns 'x'                                                           */
void TKbacksubstitution_svd(double **V, double *w,double **U,double *b,double *x,int n,int nf)
{
   int i,j;
   double uTb[nf];

   /* Calculate [diag(1/w_j)] . U^T.b */
   for (i=0;i<nf;i++)
   {
	  uTb[i]=0.0;
	  if (w[i]!=0.0)
	  {
		 for (j=0;j<n;j++)
			uTb[i]+=U[j][i]*b[j];
		 uTb[i]/=w[i];
	  }
   }
   /* Now multiply by V as in equation 2.6.7 */
   for (i=0;i<nf;i++)
   {
	  x[i]=0.0;
	  for (j=0;j<nf;j++)
		 x[i]+=V[i][j]*uTb[j];
   }
}

/* Computes (a^2 + b^2)^1/2 */
double TKpythag(double a,double b)
{
   double ret=0.0;
   double absa,absb;

   absa = fabs(a);
   absb = fabs(b);
   if (absa > absb)
	  ret = absa*sqrt(1.0+pow(absb/absa,2));
   else
   {
	  if (absb==0) ret = 0.0;
	  else ret = absb*sqrt(1.0+pow(absa/absb,2));
   }
   return ret;
}




void TKmultMatrix2(double **idcm,double **u,int ndata,int ndata2,int npol,double **uout)
{
#ifdef ACCEL_MULTMATRIX
   if(useT2accel){
	  logdbg("Using ACCELERATED multmatrix (M.Keith 2012)");
	  accel_multMatrix(idcm[0],u[0],ndata,ndata2,npol,uout[0]);
   } else {
#endif
	  logdbg("Using SLOW multmatrix");
	  int i,j,k;
	  for (j=0;j<npol;j++)
	  {
		 for (i=0;i<ndata;i++)
		 {
			uout[i][j]=0.0;
		 }
	  }
	  for (k=0;k<ndata2;k++) {
		 for (i=0;i<ndata;i++){
			for (j=0;j<npol;j++){
			   uout[i][j]+=idcm[i][k]*u[k][j];
			}
		 }
	  }

#ifdef ACCEL_MULTMATRIX
   }
#endif
}


void TKmultMatrix(double **idcm,double **u,int ndata,int npol,double **uout)
{
   TKmultMatrix2(idcm,u,ndata,ndata,npol,uout);
}
void TKmultMatrixVec2(double **idcm,double *b,int ndata,int ndata2,double *bout)
{
#ifdef ACCEL_MULTMATRIX
   if (useT2accel){
	  logdbg("using accelerated multmatrix (m.keith 2012)");
	  accel_multMatrix(idcm[0],b,ndata,ndata2,1,bout);
   }else{
#endif
	  int i,j;
	  for (i=0;i<ndata;i++)
	  {
		 bout[i] = 0.0;
	  }

	  for (j=0;j<ndata2;j++)
	  {
		 for (i=0;i<ndata;i++)
			bout[i]+=idcm[i][j]*b[j];
	  }

#ifdef ACCEL_MULTMATRIX
   }
#endif
}


void TKmultMatrixVec(double **idcm,double *b,int ndata,double *bout)
{
   TKmultMatrixVec2(idcm,b,ndata,ndata,bout);
}

void TKcholDecomposition(double **a, int n, double *p)
{
   int i,j,k;
   long double sum;
   // float sum;
   /*  for (i=0;i<n;i++)
	   {
	   for (j=0;j<n;j++)
	   printf("i=%d, j=%d, a=%g\n",i,j,a[i][j]);
	   }*/
   for (i=0;i<n;i++)
   {
	  for (j=i;j<n;j++)
	  {
		 for (sum=a[i][j],k=i-1;k>=0;k--) sum-=a[i][k]*a[j][k]; 
		 if (i==j)
		 {
			//	      printf("Currently have %d %d %Lg\n",i,j,sum);
			if (sum <= 0.0)
			{
			   printf("Here with %d %d %g %Lg\n",i,j,a[i][j],sum);
			   for (sum=a[i][j],k=i-1;k>=0;k--)
			   {
				  sum-=a[i][k]*a[j][k];
				  printf("Failed: %d %d %d %g %g %Lg\n",i,j,k,a[i][k],a[j][k],sum);
			   }
			   printf("Failed - the matrix is not positive definite\n");
			   exit(1);
			}
			p[i] = sqrt(sum);
			//	      printf("Currently have %d %d %Lg %g\n",i,j,sum,p[i]);
		 }
		 else
		 {
			a[j][i] = (double)(sum/p[i]);
			/*	      if (j==120)
					  printf("j=120, setting %g %Lg %g %d\n",a[j][i],sum,p[i],i);
					  if (j==130)
					  printf("j=130, setting %g %Lg %g %d\n",a[j][i],sum,p[i],i);*/
		 }
	  }
   }
}
