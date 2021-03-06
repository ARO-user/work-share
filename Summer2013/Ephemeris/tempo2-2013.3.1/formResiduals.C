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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tempo2.h"

/* Form the timing residuals from the timing model and the barycentric arrival times */
void residualTracking(pulsar *psr);

void formResiduals(pulsar *psr,int npsr,int removeMean)
{
   longdouble residual;  /* Residual in phase */
   longdouble nphase,phase5[MAX_OBSN],phase2,phase3,phase4,lastResidual=0,priorResidual=0,ppRes=0;
   longdouble lastBat=0.0,priorBat=0.0,ppBat=0.0;
   longdouble phaseJ,phaseW;
   longdouble ftpd,fct,ff0,phaseint;
   longdouble torb,deltaT,dt00=0.0,dtm1=0.0,phas1=0.0;
   longdouble mean,ct00=0.0;
   int zeroID=0; // Observation number for point where the residual is set to zero
   int dtm1s=0;
   int nmean;
   int ntpd,nf0;
   int i,p,k,l;
   int time=0;
   int ntrk=0;
   int gotit=0;
   int pn0=-1;
   const char *CVS_verNum = "$Revision: 1.34 $";
   
   if (displayCVSversion == 1) CVSdisplayVersion("formResiduals.C","formResiduals()",CVS_verNum);

   for (p=0;p<npsr;p++)
     {
       mean = 0.0L;
       nmean = 0;

       for (i=0;i<psr[p].nobs;i++)
	 {
	   
	   torb = 0.0; 

	   /* Binary parameters */
	   if (psr[p].param[param_pb].paramSet[0]==1 || psr[p].param[param_fb].paramSet[0]==1) 
	     {
	       if (strcmp(psr[p].binaryModel,"BT")==0)         torb = BTmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"BTJ")==0)   torb = BTJmodel(psr,p,i,-1,0);
	       else if (strcmp(psr[p].binaryModel,"BTX")==0)   torb = BTXmodel(psr,p,i,-1,0);
	       else if (strcmp(psr[p].binaryModel,"ELL1")==0)  torb = ELL1model(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"DD")==0)    torb = DDmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"DDK")==0)   torb = DDKmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"DDS")==0)   torb = DDSmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"MSS")==0)   torb = MSSmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"DDGR")==0)  torb = DDGRmodel(psr,p,i,-1);
	       else if (strcmp(psr[p].binaryModel,"T2")==0)    torb = T2model(psr,p,i,-1,0);
	       else if( strcmp( psr[p].binaryModel, "DDH" ) == 0) torb = DDHmodel( psr, p, i, -1 );
	       else if( strcmp( psr[p].binaryModel, "ELL1H" ) == 0) torb = ELL1Hmodel( psr, p, i, -1 );
	       else {printf("Warning: Unknown binary model '%s'\n",psr[p].binaryModel); exit(1);}
	     }
	   psr[p].obsn[i].torb = torb; // save for plotting etc
	   deltaT = (psr[p].obsn[i].bbat-psr[p].param[param_pepoch].val[0])*86400.0+torb;
	   fct = psr[p].obsn[i].bbat-(int)psr[p].obsn[i].bbat - (psr[p].param[param_pepoch].val[0] - 
							       (int)psr[p].param[param_pepoch].val[0]);	   
	   ftpd = fct+torb/86400.0;	   
	   psr[p].obsn[i].pet = psr[p].obsn[i].bbat - torb/SECDAY;

	   /* What is the extra -ntpd*(int)(psr.f0) term on the end??? */
	   /* The extra (not included integral parts (int)f0.(int)bat) are not necessary! */
	   /* ntpd = (int)psr[p].obsn[i].bat-(int)psr[p].param[param_pepoch].val[0]; */
	   /*	   phase2 = deltaT*(psr[p].param[param_f0].val)-(ntpd*(int)(psr[p].param[param_f0].val)*86400.0); */

	   nf0  = (int)psr[p].param[param_f].val[0];
	   ntpd = ((int)psr[p].obsn[i].bbat-(int)psr[p].param[param_pepoch].val[0]);
	   ff0  = psr[p].param[param_f].val[0]-(int)psr[p].param[param_f].val[0];
	   phase2 = (nf0*ftpd+ntpd*ff0+ftpd*ff0); 
	   phase2 *= 86400.0;
	   /* redwards, all these calls to pow are slow & imprecise. changed */
	   longdouble arg = deltaT*deltaT;	   
	   phase3 = 0.5*psr[p].param[param_f].val[1]*arg; 
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[2]==1) phase3 += (psr[p].param[param_f].val[2]/6.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[3]==1) phase3 += (psr[p].param[param_f].val[3]/24.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[4]==1) phase3 += (psr[p].param[param_f].val[4]/120.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[5]==1) phase3 += (psr[p].param[param_f].val[5]/720.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[6]==1) phase3 += (psr[p].param[param_f].val[6]/5040.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[7]==1) phase3 += (psr[p].param[param_f].val[7]/40320.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[8]==1) phase3 += (psr[p].param[param_f].val[8]/362880.0L)*arg;
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[9]==1) phase3 += (psr[p].param[param_f].val[9]/3628800.0L)*arg; 
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[10]==1) phase3 += (psr[p].param[param_f].val[10]/39916800.0L)*arg; 
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[11]==1) phase3 += (psr[p].param[param_f].val[11]/479001600.0L)*arg; 
	   arg *= deltaT; if (psr[p].param[param_f].paramSet[12]==1) phase3 += (psr[p].param[param_f].val[12]/6227020800.0L)*arg; 

	   /* Must check glitch parameters */
	   phase4 = 0.0; 
	   for (k=0;k<psr[p].param[param_glep].aSize;k++)
	     {
	       if (psr[p].param[param_glep].paramSet[k]==1)
		 {
		   longdouble tp,dt1,expf,tgl;
		   tp = (ntpd+ftpd)*86400.0;	
		   tgl = (psr[p].param[param_glep].val[k] - psr[p].param[param_pepoch].val[0])*86400.0; // - psr[p].param[param_pepoch].val[0])*86400.0;
		   /* Only has effect after the glitch epoch */
		   if (tp >= tgl) 
		     {
		       dt1 = tp-tgl;
		       if (psr[p].param[param_gltd].val[k]!=0.0)
			 expf = exp(-dt1/86400.0/psr[p].param[param_gltd].val[k]);
		       else
			 expf = 1.0;
		       
		       // What happens if GLF2 (or GLF1) is not set?
		       phase4+=psr[p].param[param_glph].val[k]+
			 psr[p].param[param_glf0].val[k]*dt1 + 
			 0.5*psr[p].param[param_glf1].val[k]*dt1*dt1 +
			 1.0/6.0*psr[p].param[param_glf2].val[k]*dt1*dt1*dt1 +
			 psr[p].param[param_glf0d].val[k]*
			 psr[p].param[param_gltd].val[k]*86400.0*(1.0-expf);
		       //		       printf("Glitch phase = %10f\n",(double)(psr[p].param[param_glph].val[k]+
		       //			 psr[p].param[param_glf0].val[k]*dt1 + 
		       //			 0.5*psr[p].param[param_glf1].val[k]*dt1*dt1 +
		       //			 1.0/6.0*psr[p].param[param_glf2].val[k]*dt1*dt1*dt1 +
		       //			 psr[p].param[param_glf0d].val[k]*
		       //			      psr[p].param[param_gltd].val[k]*86400.0*(1.0-expf)));
		     }
		 }
	     }
	   
	   /* Add in extra phase due to jumps */
	   phaseJ = 0.0;
	   for (k=1;k<=psr[p].nJumps;k++)	    
	     {
	       for (l=0;l<psr[p].obsn[i].obsNjump;l++)
		 {		
		   if (psr[p].obsn[i].jump[l]==k)
		     phaseJ+=psr[p].jumpVal[k]*psr[p].param[param_f].val[0];
		 }
	     }

	   /* Add in extra phase due to whitening procedures */
	   phaseW = 0.0;
	   if (psr[p].param[param_wave_om].paramSet[0] == 1)
	     {
	       double om;  /* Fundamental frequency */
	       double dt;  /* Change in time from pepoch */

	       dt = psr[p].obsn[i].bbat - psr[p].param[param_waveepoch].val[0] ;
	       om = psr[p].param[param_wave_om].val[0];
	       for (k=0;k<psr[p].nWhite;k++)
		 {
		   if (psr[p].waveScale==0)
		     {		       
		       phaseW += psr[p].wave_sine[k]*sin(om*(k+1)*dt)*psr[p].param[param_f].val[0] +
			 psr[p].wave_cos[k]*cos(om*(k+1)*dt)*psr[p].param[param_f].val[0];
		     }
		   else 
		     {
		       double freq= psr[p].obsn[i].freq;
		       phaseW += (psr[p].wave_sine[k]*sin(om*(k+1)*dt)*psr[p].param[param_f].val[0] +
			 psr[p].wave_cos[k]*cos(om*(k+1)*dt)*psr[p].param[param_f].val[0])/(DM_CONST*freq*freq);
		     }
		 }
	     }
	 
	   // Add in extra phase due to quadrupolar signal 
	   if (psr[p].param[param_quad_om].paramSet[0] == 1)
	     {
	       double kp_theta,kp_phi,kp_kg,p_plus,p_cross,gamma,omega_g;
	       //	       double res_e,res_i;
	       long double resp,resc,res_r,res_i;
	       double theta_p,theta_g,phi_p,phi_g;
	       double lambda_p,beta_p,lambda,beta;
	       long double time;
	       double n1,n2,n3;
	       double e11p,e21p,e31p,e12p,e22p,e32p,e13p,e23p,e33p;
	       double e11c,e21c,e31c,e12c,e22c,e32c,e13c,e23c,e33c;
	       double cosTheta;

	       double om;  /* Fundamental frequency */
	       double dt;  /* Change in time from pepoch */

	       dt = (psr[p].obsn[i].bbat - psr[p].quadEpoch)*86400.0;
	       om = psr[p].param[param_quad_om].val[0];

	       if (psr[p].param[param_raj].paramSet[1] == 1)
		 lambda_p = (double)psr[p].param[param_raj].val[1];
	       else
		 lambda_p = (double)psr[p].param[param_raj].val[0];

	       if (psr[p].param[param_decj].paramSet[1] == 1)
		 beta_p   = (double)psr[p].param[param_decj].val[1];
	       else
		 beta_p   = (double)psr[p].param[param_decj].val[0];
	       lambda   = psr[p].quadRA;
	       beta     = psr[p].quadDEC;

	       // Pulsar vector
	       n1 = cosl(lambda_p)*cosl(beta_p);
	       n2 = sinl(lambda_p)*cosl(beta_p);
	       n3 = sinl(beta_p);
	       cosTheta = cosl(beta)*cosl(beta_p)*cosl(lambda-lambda_p)+
		 sinl(beta)*sinl(beta_p);

	       // From KJ's paper
	       // Gravitational wave matrix
	       e11p = pow(sinl(lambda),2)-pow(cosl(lambda),2)*pow(sinl(beta),2);
	       e21p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
	       e31p = cosl(lambda)*sinl(beta)*cosl(beta);

	       e12p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
	       e22p = pow(cosl(lambda),2)-pow(sinl(lambda),2)*pow(sinl(beta),2);
	       e32p = sinl(lambda)*sinl(beta)*cosl(beta);
	       
	       e13p = cosl(lambda)*sinl(beta)*cosl(beta);
	       e23p = sinl(lambda)*sinl(beta)*cosl(beta);
	       e33p = -powl(cosl(beta),2);

	       resp = (n1*(n1*e11p+n2*e12p+n3*e13p)+
		       n2*(n1*e21p+n2*e22p+n3*e23p)+
		       n3*(n1*e31p+n2*e32p+n3*e33p));
	       //	       printf("resp = %Lg\n",resp);

	       // Determine cross term
	       e11c = sin(2*lambda)*sin(beta);
	       e21c = -cos(2*lambda)*sin(beta);
	       e31c = -sin(lambda)*cos(beta);

	       e12c = -cos(2*lambda)*sin(beta);
	       e22c = -sin(2*lambda)*sin(beta);
	       e32c = cos(lambda)*cos(beta);
	       
	       e13c = -sin(lambda)*cos(beta);
	       e23c = cos(lambda)*cos(beta);
	       e33c  = 0;

	       resc = (n1*(n1*e11c+n2*e12c+n3*e13c)+
		       n2*(n1*e21c+n2*e22c+n3*e23c)+
		       n3*(n1*e31c+n2*e32c+n3*e33c));

	       for (k=0;k<psr[p].nQuad;k++)
		 {
		   omega_g = (double)psr[p].param[param_quad_om].val[0]*(k+1);		   
 		   res_r = (psr[p].quad_aplus_r[k]*resp+psr[p].quad_across_r[k]*resc)*sinl(omega_g*dt);
		   //		   res_i = (psr[p].quad_aplus_i[k]*resp+psr[p].quad_across_i[k]*resc)*(cos(omega_g*dt)-1);
		   res_i = (psr[p].quad_aplus_i[k]*resp+psr[p].quad_across_i[k]*resc)*(cosl(omega_g*dt));


		   if (psr[p].gwsrc_psrdist>0) // Add in the pulsar term
		     {
		       printf("Pulsar distance = %g\n",(double)psr[p].gwsrc_psrdist);
		       res_r -= (psr[p].quad_aplus_r[k]*resp+psr[p].quad_across_r[k]*resc)*sinl(omega_g*dt-(1-cosTheta)*psr[p].gwsrc_psrdist/SPEED_LIGHT*omega_g); 
		       //
		       res_i -= (psr[p].quad_aplus_i[k]*resp+psr[p].quad_across_i[k]*resc)*(cosl(omega_g*dt-(1-cosTheta)*psr[p].gwsrc_psrdist/SPEED_LIGHT*omega_g));
		     }
		   //		   printf("cosTheta = %g\n",(double)cosTheta);
		   if ((1-cosTheta)==0.0)
		     {
		       res_r = 0.0;
		       res_i = 0.0;
		     }
		   else
		     {
		       res_r = 1.0L/(2.0L*omega_g*(1.0L-cosTheta))*(res_r); 
		       res_i = 1.0L/(2.0L*omega_g*(1.0L-cosTheta))*(res_i); 
		       //		       res_r = 1.0L/(omega_g)*(res_r);
		       //res_i = 1.0L/(omega_g)*(res_i);
		     }
		   phaseW += (res_r+res_i)*psr[p].param[param_f].val[0];
		   
		 }
	     } 


	   /* Add in extra phase due to gravitational wave signal */
	   if (psr[p].param[param_gwsingle].paramSet[0]==1)
	     {
	       double kp_theta,kp_phi,kp_kg,p_plus,p_cross,gamma,omega_g;
	       //	       double res_e,res_i;
	       long double resp,resc,res_r,res_i;
	       double theta_p,theta_g,phi_p,phi_g;
	       double lambda_p,beta_p,lambda,beta;
	       long double time;	      
	       double n1,n2,n3;
	       double e11p,e21p,e31p,e12p,e22p,e32p,e13p,e23p,e33p;
	       double e11c,e21c,e31c,e12c,e22c,e32c,e13c,e23c,e33c;
	       double cosTheta;

	       time    = (psr[p].obsn[i].bbat - psr[p].gwsrc_epoch)*86400.0L;

	       lambda_p = (double)psr[p].param[param_raj].val[0];
	       beta_p   = (double)psr[p].param[param_decj].val[0];
	       lambda   = psr[p].gwsrc_ra;
	       //	       beta     = M_PI/2.0-psr[p].gwsrc_dec;
	       beta     = psr[p].gwsrc_dec;
	       //			       phi_g   = psr[p].gwsrc_ra;

	       // Pulsar vector
	       n1 = cosl(lambda_p)*cosl(beta_p);
	       n2 = sinl(lambda_p)*cosl(beta_p);
	       n3 = sinl(beta_p);
	       //	       printf("n = %g %g %g\n",n1,n2,n3);
	       cosTheta = cosl(beta)*cosl(beta_p)*cosl(lambda-lambda_p)+
		 sinl(beta)*sinl(beta_p);
	       //	       printf("cosTheta = %g\n",cosTheta);

	       // From KJ's paper
	       // Gravitational wave matrix
	       e11p = pow(sinl(lambda),2)-pow(cosl(lambda),2)*pow(sinl(beta),2);
	       e21p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
	       e31p = cosl(lambda)*sinl(beta)*cosl(beta);
	       //	       printf("ex1p = %g %g %g (%g %g)\n",e11p,e21p,e31p,lambda,beta);

	       e12p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
	       e22p = pow(cosl(lambda),2)-pow(sinl(lambda),2)*pow(sinl(beta),2);
	       e32p = sinl(lambda)*sinl(beta)*cosl(beta);
	       //	       printf("ex2p = %g %g %g\n",e12p,e22p,e32p);
	       
	       e13p = cosl(lambda)*sinl(beta)*cosl(beta);
	       e23p = sinl(lambda)*sinl(beta)*cosl(beta);
	       e33p = -powl(cosl(beta),2);
	       //	       printf("ex3p = %g %g %g\n",e13p,e23p,e33p);
	       //	       exit(1);

	       omega_g = (double)psr[p].param[param_gwsingle].val[0];
	       
	       resp = (n1*(n1*e11p+n2*e12p+n3*e13p)+
		       n2*(n1*e21p+n2*e22p+n3*e23p)+
		       n3*(n1*e31p+n2*e32p+n3*e33p));
	       //	       printf("resp = %Lg\n",resp);

	       // Determine cross term
	       e11c = sin(2*lambda)*sin(beta);
	       e21c = -cos(2*lambda)*sin(beta);
	       e31c = -sin(lambda)*cos(beta);

	       e12c = -cos(2*lambda)*sin(beta);
	       e22c = -sin(2*lambda)*sin(beta);
	       e32c = cos(lambda)*cos(beta);
	       
	       e13c = -sin(lambda)*cos(beta);
	       e23c = cos(lambda)*cos(beta);
	       e33c  = 0;


	       resc = (n1*(n1*e11c+n2*e12c+n3*e13c)+
		       n2*(n1*e21c+n2*e22c+n3*e23c)+
		       n3*(n1*e31c+n2*e32c+n3*e33c));

	       res_r = (psr[p].gwsrc_aplus_r*resp+psr[p].gwsrc_across_r*resc)*sinl(omega_g*time);
	       res_i = (psr[p].gwsrc_aplus_i*resp+psr[p].gwsrc_across_i*resc)*(cosl(omega_g*time));
	       //	       res_i = (psr[p].gwsrc_aplus_i*resp+psr[p].gwsrc_across_i*resc)*(cos(omega_g*time)-1);
	       if (psr[p].gwsrc_psrdist>0) // Add in the pulsar term
		 {
		   res_r += (psr[p].gwsrc_aplus_r*resp+psr[p].gwsrc_across_r*resc)*sinl(omega_g*time-(1-cosTheta)*psr[p].gwsrc_psrdist/SPEED_LIGHT*omega_g);
		   //		   res_i += (psr[p].gwsrc_aplus_i*resp+psr[p].gwsrc_across_i*resc)*(cos(omega_g*time-(1-cosTheta)*psr[p].gwsrc_psrdist/SPEED_LIGHT*omega_g)-1);
		   res_i += (psr[p].gwsrc_aplus_i*resp+psr[p].gwsrc_across_i*resc)*(cosl(omega_g*time-(1-cosTheta)*psr[p].gwsrc_psrdist/SPEED_LIGHT*omega_g));
		 }

	       if ((1-cosTheta)==0.0)
		 {
		   res_r = 0.0;
		   res_i = 0.0;
		 }
	       else
		 {
		   res_r = 1.0L/(2.0L*omega_g*(1.0L-cosTheta))*(res_r); 
		   res_i = 1.0L/(2.0L*omega_g*(1.0L-cosTheta))*(res_i); 
		 }
	       phaseW += (res_r+res_i)*psr[p].param[param_f].val[0];
	       //	       printf("Res = %g\n",(double)res);
	     }

	   if (psr[p].param[param_gwm_amp].paramSet[0]==1)
	     {
	       double kp_theta,kp_phi,kp_kg,p_plus,p_cross,gamma,omega_g;
	       //	       double res_e,res_i;
	       long double resp,resc,res_r,res_i;
	       double theta_p,theta_g,phi_p,phi_g;
	       double lambda_p,beta_p,lambda,beta;
	       long double time;	      
	       double n1,n2,n3;
	       double e11p,e21p,e31p,e12p,e22p,e32p,e13p,e23p,e33p;
	       double e11c,e21c,e31c,e12c,e22c,e32c,e13c,e23c,e33c;
	       double cosTheta;
	       double g1,g2,g3;
	       time    = (psr[p].obsn[i].bbat - psr[p].gwm_epoch)*86400.0L;

	       if (psr[p].param[param_raj].paramSet[1] == 1)
		 lambda_p = (double)psr[p].param[param_raj].val[1];
	       else
		 lambda_p = (double)psr[p].param[param_raj].val[0];

	       if (psr[p].param[param_decj].paramSet[1] == 1)
		 beta_p   = (double)psr[p].param[param_decj].val[1];
	       else
		 beta_p   = (double)psr[p].param[param_decj].val[0];
	       lambda   = psr[p].gwm_raj;
	       //	       beta     = M_PI/2.0-psr[p].gwsrc_dec;
	       beta     = psr[p].gwm_decj;
	       //			       phi_g   = psr[p].gwsrc_ra;

	       // GW vector
	       g1 = -cosl(lambda)*cosl(beta);
	       g2 = -sinl(lambda)*cosl(beta);
	       g3 = -sinl(beta);


	       // Pulsar vector
	       n1 = cosl(lambda_p)*cosl(beta_p);
	       n2 = sinl(lambda_p)*cosl(beta_p);
	       n3 = sinl(beta_p);
	       //	       printf("n = %g %g %g\n",n1,n2,n3);
	       cosTheta = -(cosl(beta)*cosl(beta_p)*cosl(lambda-lambda_p) + sinl(beta)*sinl(beta_p));

	       //	       printf("cosTheta = %g\n",cosTheta);

	       /* Only has effect after the glitch epoch */
	       if (psr[p].obsn[i].sat >= psr[p].gwm_epoch)
		 {
		   long double dt,scale;
		   double cos2Phi;
		   double cosPhi;
		   double l1,l2,l3,m1,m2,m3;
		   //double beta_m;
	           double d1,d2,d3,md;
	           double a1,a2,a3,ma;
		   
		 /* define the GW coordinate system see Hobbs,G. (2009)*/
		 /* the d vector point to north pole or south pole*/ 

              if (beta == 0.0 )
              {
               d1 = 0.0;
               d2 = 0.0;
               d3 = 1.0;
              }

		   if ( beta > 0)
		    {
                     d1 = g1*cosl(0.5*M_PI - beta);
                     d2 = g2*cosl(0.5*M_PI - beta);
                     d3 = 1.0 + g3*cos(0.5*M_PI - beta);
		     md = sqrt(d1*d1 + d2*d2 + d3*d3);
		     d1 = d1/md;
		     d2 = d2/md;
		     d3 = d3/md;
		     /*covert d to unit vector */
 		    } 
		   else if (beta < 0)
		    {
                     d1 = g1*cosl(-0.5*M_PI - beta);
                     d2 = g2*cosl(-0.5*M_PI - beta);
                     d3 = -1.0 + g3*cos(-0.5*M_PI - beta);
		     md = sqrt(d1*d1 + d2*d2 + d3*d3);
		     d1 = d1/md;
		     d2 = d2/md;
		     d3 = d3/md;
 		    } 

		 //if (g2*d3-d2*g3 != 0)
		 // {
                 //  a1 = 1.0; 
                 //  a2 = (d1*g3-g1*d3)/(g2*d3-d2*g3);
                 //  a3 = (g2*d1-g1*d2)/(g3*d2-g2*d3); 
	         // }
		 //else if (g1*d3-d1*g3 != 0)
		 // {
                 //  a1 = (g3*d2-d3*g2)/(g1*d3-g3*d1); 
                 //  a2 = 1.0;
                 //  a3 = (g1*d2-d1*g2)/(g3*d1-d1*d3);
		 // }
                 //else if (d2*g1-g2*d1 != 0)			
		 // {
                 //  a1 = (g2*d3-d2*g3)/(d2*g1-g2*d1); 
                 //  a2 = (g1*d3-d1*g3)/(d1*g2-g1*d2);
                 //  a3 =1.0; 
		 // }
                   a1 = -(d2*g3-d3*g2);
                   a2 = -(d3*g1-d1*g3);
                   a3 = -(d1*g2-d2*g1);
		   /* conver it to unit vector */
		   ma = sqrt(a1*a1 +a2*a2 + a3*a3);
		   a1 = a1/ma;
		   a2 = a2/ma;
		   a3 = a3/ma;

		  /* polarisation vector of GW source */
	           m1 = d1*cosl(psr[p].gwm_phi)	+ a1*sinl(psr[p].gwm_phi);   
                   m2 = d2*cosl(psr[p].gwm_phi)	+ a2*sinl(psr[p].gwm_phi);
                   m3 = d3*cosl(psr[p].gwm_phi)	+ a3*sinl(psr[p].gwm_phi);

//////		   if  (g3 != 0) 
//		     {beta_m = atan2(-cos(beta)*cos(lambda-psr[p].gwm_phi),sin(beta));}
//		   else  
	//	     {
	//	       beta_m = atan2(sinl(psr[p].gwm_phi),cosl(psr[p].gwm_phi));
	//	       psr[p].gwm_phi = lambda + 1.5708;
		//     }	
		//   m1 = cosl(psr[p].gwm_phi)*cosl(beta_m);
		//   m2 = sinl(psr[p].gwm_phi)*cosl(beta_m);
		//   m3 = sinl(beta_m);
		   if  (cosTheta != 1.0 && cosTheta != -1.0)
		       {g1 = g1*cosTheta; 
		    	g2 = g2*cosTheta;
		    	g3 = g3*cosTheta;

			/*l is  the projection of pulsar vector on the plane which pependicular to the GW source direction */
		    	l1 = n1 - g1;
		    	l2 = n2 - g2;
		    	l3 = n3 - g3;
		    	cosPhi = (l1*m1 + l2*m2 + l3*m3)/sqrt(l1*l1 + l2*l2 + l3*l3);
			//		        if  (cosPhi >= 1.0/sqrt(2.0))
			cos2Phi = 2*cosPhi*cosPhi - 1.0;
			//		       else
			//		      	   cos2Phi = 2*sqrt(1.0 - cosPhi*cosPhi)*sqrt(1.0 - cosPhi*cosPhi) - 1.0;
		       }
		   else 
       		       {cos2Phi = 0;}

		   dt = (psr[p].obsn[i].sat - psr[p].gwm_epoch)*86400.0;
		   scale = -0.5*cos2Phi*(1-cosTheta);
		   phaseW += scale*(psr[p].param[param_gwm_amp].val[0]*psr[p].param[param_f].val[0])*dt;

		 }
	       //	       printf("Res = %g\n",(double)res);
	     }
	 
	   /* Add in extra phase due to clock offset */
	   if (psr[p].param[param_clk_offs].paramSet[0] == 1)
	     {
	       int j;
	       printf("IN HERE SETTING THE CLOCKS %d\n",psr[p].clkOffsN);
	       for (j=0;j<psr[p].clkOffsN-1;j++)
		 {
		   if (psr[p].obsn[i].bbat >= psr[p].clk_offsT[j] &&
		       psr[p].obsn[i].bbat < psr[p].clk_offsT[j+1])
		     {
		       phaseW += (psr[p].clk_offsV[j]*psr[p].param[param_f].val[0]);
		       break;
		     }
		 }
	     }

	   /* Add in extra phase due to interpolation */
	   if (psr[p].param[param_ifunc].paramSet[0] == 1)
	     {
	       long double wi,t1,t2;
	       long double dt,speriod,tt;

	       if (psr[p].param[param_ifunc].val[0] == 1) // Sinc interpolation
		 {
		   t1=0.0L,t2=0.0L;
		   speriod = (long double)(psr[p].ifuncT[1]-psr[p].ifuncT[0]); 
		   //	       printf("ifuncN = %d\n",psr[p].ifuncN);
		   for (k=0;k<psr[p].ifuncN;k++)
		     //	       for (k=3;k<4;k++)
		     {
		       //		   printf("Have %g %g\n",psr[p].ifuncT[k],psr[p].ifuncV[k]);
		       dt = psr[p].obsn[i].bbat - (long double)psr[p].ifuncT[k];
		       wi=1;
		       if (dt==0)
			 {
			   t1 += wi*(long double)psr[p].ifuncV[k];
			 }
		       else
			 {
			   tt = M_PI/speriod*(dt);
			   t1 += wi*(long double)psr[p].ifuncV[k]*sinl(tt)/(tt);
			   //		       t2 += wi*sinl(tt)/(tt);
			 }
		     }
		   phaseW += (psr[p].param[param_f].val[0]*t1);
		 }
	       else if (psr[p].param[param_ifunc].val[0] == 2) // Linear interpolation
		 {
		   int k;
		   double m,c,ival;
		   for (k=0;k<psr[p].ifuncN-1;k++)
		     {
		       if ((double)psr[p].obsn[i].sat >= psr[p].ifuncT[k] &&
			   (double)psr[p].obsn[i].sat < psr[p].ifuncT[k+1])
			 {
			   m = (psr[p].ifuncV[k]-psr[p].ifuncV[k+1])/(psr[p].ifuncT[k]-psr[p].ifuncT[k+1]);
			   c = psr[p].ifuncV[k]-m*psr[p].ifuncT[k];
			   
			   ival = m*(double)psr[p].obsn[i].sat+c;
			   break;
			 }
		     }
		   phaseW += (psr[p].param[param_f].val[0]*ival); 				
		 }
	     }

	   /* Add in extra phase due to interpolation */
	   if (psr[p].param[param_quad_ifunc_p].paramSet[0] == 1)
	     {
	       long double wi,t1,t2;
	       long double dt,speriod,tt;
	       int k;
	       double m,c,ival;
	       double kp_theta,kp_phi,kp_kg,p_plus,p_cross,gamma,omega_g;
	       //	       double res_e,res_i;
	       long double resp,resc,res_r,res_i;
	       double theta_p,theta_g,phi_p,phi_g;
	       double lambda_p,beta_p,lambda,beta;
	       double n1,n2,n3;
	       double e11p,e21p,e31p,e12p,e22p,e32p,e13p,e23p,e33p;
	       double e11c,e21c,e31c,e12c,e22c,e32c,e13c,e23c,e33c;
	       double cosTheta;

	       if (psr[p].quad_ifunc_geom_p == 0)
		 {
		   lambda_p = (double)psr[p].param[param_raj].val[0];
		   beta_p   = (double)psr[p].param[param_decj].val[0];
		   lambda   = psr[p].quad_ifunc_p_RA;
		   beta     = psr[p].quad_ifunc_p_DEC;
		   
		   // Pulsar vector
		   n1 = cosl(lambda_p)*cosl(beta_p);
		   n2 = sinl(lambda_p)*cosl(beta_p);
		   n3 = sinl(beta_p);
		   
		   cosTheta = cosl(beta)*cosl(beta_p)*cosl(lambda-lambda_p)+
		     sinl(beta)*sinl(beta_p);
		   
		   // From KJ's paper
		   // Gravitational wave matrix
		   
		   // NOTE: This is for the plus terms.  For cross should use different terms
		   e11p = pow(sinl(lambda),2)-pow(cosl(lambda),2)*pow(sinl(beta),2);
		   e21p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
		   e31p = cosl(lambda)*sinl(beta)*cosl(beta);
		   
		   e12p = -sinl(lambda)*cosl(lambda)*(pow(sinl(beta),2)+1);
		   e22p = pow(cosl(lambda),2)-pow(sinl(lambda),2)*pow(sinl(beta),2);
		   e32p = sinl(lambda)*sinl(beta)*cosl(beta);
		   
		   e13p = cosl(lambda)*sinl(beta)*cosl(beta);
		   e23p = sinl(lambda)*sinl(beta)*cosl(beta);
		   e33p = -powl(cosl(beta),2);
		   
		   resp = (n1*(n1*e11p+n2*e12p+n3*e13p)+
			   n2*(n1*e21p+n2*e22p+n3*e23p)+
			   n3*(n1*e31p+n2*e32p+n3*e33p));

		   if ((1-cosTheta)==0.0)
		     resp = 0.0;  // Check if this is sensible
		   else
		     resp = 1.0L/(2.0L*(1.0L-cosTheta))*(resp); 

		   psr[p].quad_ifunc_geom_p = resp;

		 }
	       for (k=0;k<psr[p].quad_ifuncN_p-1;k++)
		 {
		   if ((double)psr[p].obsn[i].sat >= psr[p].quad_ifuncT_p[k] &&
		       (double)psr[p].obsn[i].sat < psr[p].quad_ifuncT_p[k+1])
		     {
		       m = (psr[p].quad_ifuncV_p[k]-psr[p].quad_ifuncV_p[k+1])/(psr[p].quad_ifuncT_p[k]-psr[p].quad_ifuncT_p[k+1]);
		       c = psr[p].quad_ifuncV_p[k]-m*psr[p].quad_ifuncT_p[k];
		       
		       ival = m*(double)psr[p].obsn[i].sat+c;
		       break;
		     }
		 }
	       phaseW += (psr[p].param[param_f].val[0]*ival*psr[p].quad_ifunc_geom_p); 				
	     }
	   // Cross term
	   if (psr[p].param[param_quad_ifunc_c].paramSet[0] == 1)
	     {
	       long double wi,t1,t2;
	       long double dt,speriod,tt;
	       int k;
	       double m,c,ival;
	       double kp_theta,kp_phi,kp_kg,p_plus,p_cross,gamma,omega_g;
	       //	       double res_e,res_i;
	       long double resp,resc,res_r,res_i;
	       double theta_p,theta_g,phi_p,phi_g;
	       double lambda_p,beta_p,lambda,beta;
	       double n1,n2,n3;
	       double e11p,e21p,e31p,e12p,e22p,e32p,e13p,e23p,e33p;
	       double e11c,e21c,e31c,e12c,e22c,e32c,e13c,e23c,e33c;
	       double cosTheta;

	       if (psr[p].quad_ifunc_geom_c == 0)
		 {
		   lambda_p = (double)psr[p].param[param_raj].val[0];
		   beta_p   = (double)psr[p].param[param_decj].val[0];
		   lambda   = psr[p].quad_ifunc_p_RA;
		   beta     = psr[p].quad_ifunc_p_DEC;
		   
		   // Pulsar vector
		   n1 = cosl(lambda_p)*cosl(beta_p);
		   n2 = sinl(lambda_p)*cosl(beta_p);
		   n3 = sinl(beta_p);
		   
		   cosTheta = cosl(beta)*cosl(beta_p)*cosl(lambda-lambda_p)+
		     sinl(beta)*sinl(beta_p);
		   
		   // From KJ's paper
		   // Gravitational wave matrix
		   
		   // NOTE: These are for the cross terms. 
		   e11c = sin(2*lambda)*sin(beta);
		   e21c = -cos(2*lambda)*sin(beta);
		   e31c = -sin(lambda)*cos(beta);
		   
		   e12c = -cos(2*lambda)*sin(beta);
		   e22c = -sin(2*lambda)*sin(beta);
		   e32c = cos(lambda)*cos(beta);
		   
		   e13c = -sin(lambda)*cos(beta);
		   e23c = cos(lambda)*cos(beta);
		   e33c  = 0;
		   
		   resc = (n1*(n1*e11c+n2*e12c+n3*e13c)+
			   n2*(n1*e21c+n2*e22c+n3*e23c)+
			   n3*(n1*e31c+n2*e32c+n3*e33c));
		   
		   if ((1-cosTheta)==0.0)
		     resc = 0.0;  // Check if this is sensible
		   else
		     resc = 1.0L/(2.0L*(1.0L-cosTheta))*(resc); 
		   psr[p].quad_ifunc_geom_c = resc;
		 }
		   for (k=0;k<psr[p].quad_ifuncN_c-1;k++)
		     {
		       if ((double)psr[p].obsn[i].sat >= psr[p].quad_ifuncT_c[k] &&
			   (double)psr[p].obsn[i].sat < psr[p].quad_ifuncT_c[k+1])
			 {
		       m = (psr[p].quad_ifuncV_c[k]-psr[p].quad_ifuncV_c[k+1])/(psr[p].quad_ifuncT_c[k]-psr[p].quad_ifuncT_c[k+1]);
		       c = psr[p].quad_ifuncV_c[k]-m*psr[p].quad_ifuncT_c[k];
		       
		       ival = m*(double)psr[p].obsn[i].sat+c;
		       break;
		     }
		 }
	       phaseW += (psr[p].param[param_f].val[0]*ival*psr[p].quad_ifunc_geom_c); 				
	     }
	   


	   phase5[i] = phase2+phase3+phase4+phaseJ+phaseW;
	   //	   printf("Point 1: %.5f %.5f %.5f %.5f %.5f %.5f\n",(double)phase5[i],(double)phase2,(double)phase3,(double)phase4,(double)phaseJ,(double)phaseW);
	   if (psr[p].obsn[i].nFlags>0) /* Look for extra factor to add to residuals */
	     {
	       int k;
	       longdouble extra;

	       for (k=0;k<psr[p].obsn[i].nFlags;k++)
		 {
		   if (strcmp(psr[p].obsn[i].flagID[k],"-radd")==0) // Add in extra time
		     {
		       sscanf(psr[p].obsn[i].flagVal[k],"%Lf",&extra);
		       /* psr[p].obsn[i].residual+=extra; */
		       phase5[i]+=(extra*psr[p].param[param_f].val[0]);
		     }
		   if (strcmp(psr[p].obsn[i].flagID[k],"-padd")==0) // Add in extra phase
		     {
		       sscanf(psr[p].obsn[i].flagVal[k],"%Lf",&extra);
		       /* psr[p].obsn[i].residual+=extra; */
		       phase5[i]+=(extra);
		     }
		 }
	     }
	   
	   /* Set the first residual to be zero */
	   /* in this case fortran_mod) returns [phase5 - (int)(phase5)] 
            * ie. returns the fractional part of phase5
            */
	   if (psr[p].param[param_iperharm].paramSet[0]==1)
	     {  /* This code has been added for observations taken at the wrong rotational period */
	       phase5[i] = fortran_mod(phase5[i],1.0/psr[p].param[param_iperharm].val[0]);
	       if (phase5[i] >= 1.0/2.0/psr[p].param[param_iperharm].val[0])
		 {
		   while (phase5[i] >= 1.0/2.0/psr[p].param[param_iperharm].val[0])
		     phase5[i] -= 1.0/psr[p].param[param_iperharm].val[0];
		 }
	       else if (phase5[i] < -1.0/2.0/psr[p].param[param_iperharm].val[0])
		 {
		   while (phase5[i] < -1.0/2.0/psr[p].param[param_iperharm].val[0])
		     phase5[i] += 1.0/psr[p].param[param_iperharm].val[0];
		 }
	       //	       if (i==0) phas1  = phase5[i];
	       phase5[i] = phase5[i]; // -phas1; 	       
	     }
	   else
	     {
	       // This is where the first residual is set to equal zero
	       //	       if (i==0) phas1  = fortran_mod((phase5[i]),1.0L); 
	       phase5[i] = phase5[i]; //-phas1; 
	     }
	 }
       // Now calculate phas1 to set the first residual equal to zero
       for (i=0;i<psr[p].nobs;i++)
	 {
	   if (psr[p].obsn[i].deleted==0 && 
	       (psr[0].param[param_start].fitFlag[0]==0 
		|| psr[0].obsn[i].sat > psr[0].param[param_start].val[0]) &&
	       (psr[0].param[param_finish].fitFlag[0]==0 
		|| psr[0].obsn[i].sat < psr[0].param[param_finish].val[0]))
	     {
	       if (psr[p].param[param_iperharm].paramSet[0]==1)
		 phas1 = phase5[i];
	       else
		 phas1 = fortran_mod((phase5[i]),1.0L); 
		 //		 phas1 = phase5[i];
		 //
		 
		 //		 phas1 = fortran_mod((phase5[i]),1.0L); 

		 //
	       zeroID=i;
	       //	       printf("phas1 set to observation number %d\n",i);
	       break;
	     }
	       
	 }
       for (i=0;i<psr[p].nobs;i++)
	 {
	   /* phase5 - nphase is also the fractional part of phase5
            * -- this is different to using fortran_mod() as above as the nearest integer
            *    to a negative or a positive number is calculated differently. 
	    */
	   phase5[i] -= phas1;
	   nphase = (longdouble)fortran_nint(phase5[i]);
	   psr[p].obsn[i].nphase = nphase;

	   residual = phase5[i] - nphase;
	   /* residual = residual in phase */
	   if (psr[p].obsn[i].deleted!=1)
	     gotit = 1;
	   else
	     gotit = 0;
	   
	   //	   printf("%s deleted = %d\n",psr[p].obsn[i].fname,psr[p].obsn[i].deleted);
	   if (psr[p].param[param_track].paramSet[0]==1 && psr[p].param[param_track].val[0] > 0 && time==1)
	     {
	       residual+=ntrk;
	       // Note that this requires that the points be in time order
	       if (gotit==1)
		 {
		   if (psr[p].obsn[i].bbat - ct00 < 0.0L && fabs(psr[p].obsn[i].bbat-ct00) > 1 && i > 0)
		     {
		       printf("ERROR: Points must be in time order for tracking to work\n");
		       printf("Pulsar = %s\n",psr[p].name);
		       printf("Observation %d (%s) has BBAT = %.5g\n",i,psr[p].obsn[i].fname,(double)psr[p].obsn[i].bbat);
		       printf("Observation %d (%s) has BBAT = %.5g\n",i-1,psr[p].obsn[i-1].fname,(double)psr[p].obsn[i-1].bbat);
		       printf("Difference = %Lg %Lg %Lg\n",psr[p].obsn[i].bbat, ct00,psr[p].obsn[i].bbat - ct00);
		       exit(1);
		     }	       
		   if (psr[p].obsn[i].bbat - ct00 < fabs(psr[p].param[param_track].val[0]))
		     {
		       if (fabs(residual+1.0-dt00) < fabs(residual-dt00))
			 {
			   residual+=1.0;
			   ntrk+=1;
			   printf("ADDING PHASE %s\n",psr[p].obsn[i].fname);
			 } 
		       else if (fabs(residual-1.0-dt00) < fabs(residual-dt00))
			 {
			   residual-=1.0;
			   ntrk-=1;
			   printf("SUBTRACT PHASE %s\n",psr[p].obsn[i].fname);
			 } 
		     }
		 }
	     }
	   
	   if ((double)psr[p].param[param_track].val[0] == -1) // Do extra tracking
	     {
	       if (dtm1s==0) printf("Attempting tracking via the gradient method\n");
	       
	       if (dtm1s>1 && fabs(psr[p].obsn[i].bat-lastBat) > 1 
		   && fabs(lastBat-priorBat) > 1) // Have 3 points each separated by more than 1 day
		 {
		   double m1,m2,m3,m4,m5;
		   m1 = (double)(lastResidual-priorResidual)/(lastBat-priorBat);
		   m2 = (double)(residual-lastResidual)/(psr[p].obsn[i].bat-lastBat);
		   m3 = (double)(residual-1-lastResidual)/(psr[p].obsn[i].bat-lastBat);
		   m4 = (double)(residual+1-lastResidual)/(psr[p].obsn[i].bat-lastBat);
		   //		   printf("pos %d %g %g %g %g (%g) (%g) (%g)\n",i,m1,m2,m3,m4,fabs(m2-m1),fabs(m3-m1),fabs(m4-m1));
		   
		   if (fabs(m1-m2) < fabs(m1-m3) && fabs(m1-m2) < fabs(m1-m4))
		     {
		     }
		   else if (fabs(m1-m3) < fabs(m1-m2) && fabs(m1-m3) < fabs(m1-m4))
		     {
		       //		       printf("Updating neg\n");
		       residual-=1.0;
		       ntrk-=1;
		     }
		   else if (fabs(m1-m4) < fabs(m1-m2) && fabs(m1-m4) < fabs(m1-m3))
		     {
		       //		       printf("Updating pos\n");
		       residual+=1.0;
		       ntrk+=1;
		     } 
		 }
	     }
	   if ((double)psr[p].param[param_track].val[0] == -2) // Track on pulse number
	     {
	       long long pnNew;
	       long long pnAct;
	       long long addPhase;

	       nf0  = (int)psr[p].param[param_f].val[0];
	       ntpd = ((int)psr[p].obsn[i].bbat-(int)psr[p].param[param_pepoch].val[0]);
	       phaseint = nf0*ntpd*86400.0;
	       pnNew = (int)(phaseint + fortran_nint(phase5[i]));
	       if (pn0 == -1)
		 {
		   pn0 = pnNew;
		   pnNew = 0;
		 }
	       else
		 pnNew -= pn0;
	       printf("Have %g %lld\n",(double)psr[p].obsn[i].sat,pnNew);
	       // Compare with flag
	       for (int kk=0;kk<psr[p].obsn[i].nFlags;kk++)
		 {
		   if (strcmp(psr[p].obsn[i].flagID[kk],"-pn")==0)
		     {
		       sscanf(psr[p].obsn[i].flagVal[kk],"%lld",&pnAct);
		       addPhase = pnNew-pnAct;
		       residual += addPhase;
		       ntrk += addPhase;
		       printf("*** Adding phase %lld ***\n",addPhase);
		     }
		 }
	     }
	   if (gotit==1)
	     {
	       dtm1s ++;
	       dtm1 = dt00;
	       dt00  = residual;
	       
	       ct00 = psr[p].obsn[i].bbat;
	     }
	   if (gotit==1 && time==0)
	     time=1;
	   psr[p].obsn[i].residual = residual/psr[p].param[param_f].val[0];

	   ppRes = priorResidual;
	   priorResidual=lastResidual;
	   ppBat = priorBat;
	   priorBat = lastBat;
	   lastResidual=residual;
	   lastBat = psr[p].obsn[i].bat;

 	   /* Check for phase offsets */
	   for (k=0;k<psr[p].nPhaseJump;k++)
	     {
	       //	       if (psr[p].obsn[i].sat > psr[p].phaseJump[k])
	       //	       printf("Comparing with %g\n",(double)psr[p].obsn[psr[p].phaseJumpID[k]].sat);
	       if (psr[p].obsn[i].sat > psr[p].obsn[psr[p].phaseJumpID[k]].sat)
		 psr[p].obsn[i].residual += (double)psr[p].phaseJumpDir[k]/psr[p].param[param_f].val[0];
	     }
	   nf0  = (int)psr[p].param[param_f].val[0];
	   ntpd = ((int)psr[p].obsn[i].bbat-(int)psr[p].param[param_pepoch].val[0]);

	   phaseint = nf0*ntpd*86400.0;
	   /*	   psr[p].obsn[i].phase = nint(phase5)+phaseint; */
	   /* dt in resid.f */
	   /*	   psr[p].obsn[i].phase = phaseint+nphase+phase5-nphase+dphase+ddnprd; */	   
	   psr[p].obsn[i].phase = phaseint+phase5[i];  
	   psr[p].obsn[i].pulseN = (long double)(phaseint + fortran_nint(phase5[i]));
	   //	   printf("At this point: %.5f %.5f %.5f %.5f %d %.5f %g\n",(double)psr[p].obsn[i].sat,(double)phase5[i],(double)nphase,(double)fortran_nint(phase5[i]),zeroID,(double)phas1,(double)psr[p].obsn[i].pulseN);
	   if (psr[p].obsn[i].deleted!=1)
	     {
	       mean+=psr[p].obsn[i].residual;
	       nmean++;
	     }
	 }
       if (removeMean==1)
	 {
	   mean/=(long double)nmean;
	   for (i=0;i<psr[p].nobs;i++)
	     psr[p].obsn[i].residual-=mean;
	 }

     }
}

