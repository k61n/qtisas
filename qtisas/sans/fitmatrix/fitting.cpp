/***************************************************************************
 File                 : fitting.cpp
 Project              : QtiSAS :: Table(s) Fitting Interface
 --------------------------------------------------------------------
 Copyright /QtiSAS/   : (C) 2006-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 
 Description          : Data Structures and Fitting Functions
 
 ***************************************************************************/

/***************************************************************************
 *                                                                         
 *  This program is free software; you can redistribute it and/or modify   
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or      
 *  (at your option) any later version.                                    
 *                                                                         
 *  This program is distributed in the hope that it will be useful,        
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 *  GNU General Public License for more details.                           
 *                                                                         
 *   You should have received a copy of the GNU General Public License     
 *   along with this program; if not, write to the Free Software           
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    
 *   Boston, MA  02110-1301  USA                                           
 *                                                                         
 ***************************************************************************/

#include "fitting.h"

// new >15.09.2017
/*
double round2prec(double f, int prec)
{
    return f;
    if(f==0.0) return 0.0;
    int sign=1; if (f<0.0) sign=-1;
    f=fabs(f);
    if (f>1.0) prec--;
    int precData=(int)log10(f);
    f= rint(f*pow(10.0,prec-precData))*pow(10.0,precData-prec);

    return sign*f;
}
*/
//*******************************************
//+++ inversion
//*******************************************
int inversion (int n, gsl_matrix * m, gsl_matrix * inverse)
{
	int s;

	// Define all the used matrices
	gsl_permutation * perm = gsl_permutation_alloc (n);

	// Make LU decomposition of matrix m
	gsl_linalg_LU_decomp (m, perm, &s);

	// Invert the matrix m
	return gsl_linalg_LU_invert (m, perm, inverse);
}


//*******************************************
//+++ poly-Function SZ   
//*******************************************
double polyFunctionSZ(double P, void *poly1) 
{
	//+++ poly1_SANS:
	int	polyNumber	=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;
	
	//+++
	gsl_vector *para 		= ((functionT *)F->params)->para;
	
	//+++
	int p=para->size;

	//+++	
	double P0		=gsl_vector_get(para, polyNumber);
	double sigma		=gsl_vector_get(para, p-2);

	if (P0<=0.0 || P==0.0 || sigma==0.0) return 0.0;
	//---
	double k=1/sigma/sigma;
	//---
	double t=k*P/P0;
	//---
	gsl_vector_set(para, polyNumber,P);
	//---
	double result;
	result=k/P0*exp((k-1)*log(t)-t-gsl_sf_lngamma(k))*GSL_FN_EVAL(F,Q);
	//
	gsl_vector_set(para, polyNumber,P0); 
	//
	return result;
}


//*******************************************
//+++ poly-Function Gamma 
//*******************************************
double polyFunctionGamma(double P, void *poly1)
{
	// poly1_SANS:
	int	polyNumber	=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;
	
	//+++ 
	gsl_vector *para 		= ((functionT *)F->params)->para;
	
	int p=para->size;
	//+++
	double P0		=gsl_vector_get(para, polyNumber);
	double sigma		=gsl_vector_get(para, (p-2));
	
	// +++   
	gsl_vector_set(para, polyNumber,P);
	
	//+++
	double result;
	
	//+++
	if (P==0 || sigma==0 || P0==0) return 0;
	
	//+++
	double theta=sigma*sigma*P0;
	double k=P0/theta;
	double t=P/theta;
	
	//+++
	result=1/theta*exp( (k-1)*log(t) -P/theta - gsl_sf_lngamma(k) )*GSL_FN_EVAL(F,Q);
	
	//+++
	gsl_vector_set(para, polyNumber,P0); 
	
	//+++
	return result;
}

//*******************************************
//+++ poly-Function Uniform
//*******************************************
double polyFunctionLogNormal(double P, void *poly1)
{
	//+++ poly1_SANS:
	int	polyNumber	=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;

	//+++
	gsl_vector *para 		= ((functionT *)F->params)->para;
	
	//+++
	int p=para->size;
	
	//+++
	double P0		=gsl_vector_get(para, polyNumber);
	double sigma		=gsl_vector_get(para, (p-2) );
	
	// +++   
	gsl_vector_set(para, polyNumber,P);
	
	// +++
	double result=GSL_FN_EVAL(F,Q);
	
	// +++
	gsl_vector_set(para, polyNumber,P0); 
	
	// +++
	double C=sqrt(2*M_PI)*sigma*P;
	
	// +++
	if (P>0 && sigma>0 && P0>0) result*=exp( - log(P/P0)*log(P/P0) /2 /sigma/sigma) / C; else result=0;
	
	return result;
}

//*******************************************
// +++ poly-Function Uniform
//*******************************************
double polyFunctionUniform(double P, void *poly1)
{
	// poly1_SANS:
	int	polyNumber	=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;
 
	//+++
	gsl_vector *para 		= ((functionT *)F->params)->para;
	
	// +++
	double P0 =gsl_vector_get(para, polyNumber);
	
	// +++
	gsl_vector_set(para, polyNumber,P);
	
	// +++
	double result=GSL_FN_EVAL(F,Q);
	
	// +++
	gsl_vector_set(para, polyNumber,P0);
	
	// +++
	return result;
}

//*******************************************
// +++ poly-Function Triangular
//*******************************************
double polyFunctionTriangular(double P, void *poly1)
{
	// poly1_SANS:
	int	polyNumber	=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;
	
	//+++
	gsl_vector *para 		= ((functionT *)F->params)->para;	
	
	// +++
	int p=para->size;
	
	// +++
	double P0	=gsl_vector_get(para, polyNumber);
	double A		=gsl_vector_get(para, p-2);
	double B		=gsl_vector_get(para, p-1);
	
	// +++
	gsl_vector_set(para, polyNumber,P);
	
	// +++
	double result;
	
	// +++
	if (P<P0) result=2*(P-A)/(B-A)/(P0-A)*GSL_FN_EVAL(F,Q);
	else result=2*(B-P)/(B-A)/(B-P0)*GSL_FN_EVAL(F,Q);
	
	// +++
	gsl_vector_set(para, polyNumber,P0); 
	
	// +++
	return result;
}

//*******************************************
//+++ poly-Function-Gauss
//*******************************************
double polyFunctionGauss(double P, void *poly1)
{
	//+++ poly1_SANS:
	int polyNumber		=((struct poly1_SANS *)poly1)->polyNumber;
	double Q 		=((struct poly1_SANS *)poly1)->Q;
	gsl_function *F		=((struct poly1_SANS *)poly1)->function;
	
	//+++
	gsl_vector *para 		= ((functionT *)F->params)->para;
	
	//+++
	int p=para->size;
	
	//+++
	double sigma		=gsl_vector_get(para, p-2);
	
	//+++
	double P0		=gsl_vector_get(para, polyNumber);
	
	//+++
	sigma*=P0;

	//+++
	gsl_vector_set(para, polyNumber,P);

	//+++
	double result=exp(-0.5*(P-P0)*(P-P0)/sigma/sigma)*GSL_FN_EVAL(F,Q);

	//+++
	gsl_vector_set(para, polyNumber,P0);
	
	//+++ 
	return result;
}



//*******************************************
//+++ Poly-Integral
//*******************************************
double polyIntegral(double Q, void *poly2)
{
    
	//+++ poly2_SANS
	gsl_function *F	 	=((struct poly2_SANS *)poly2)->function;
	int polyNumber		=((struct poly2_SANS *)poly2)->polyNumber;
	integralControl *polyIntegralControl	 	=((struct poly2_SANS *)poly2)->polyIntegralControl;

	//+++
	gsl_vector *F_para			= ((functionT *)F->params)->para;
	
	//+++
	double absErrPoly			=((struct integralControl *)polyIntegralControl)->absErr;
	double relErrPoly				=((struct integralControl *)polyIntegralControl)->relErr;    
	int intWorkspasePoly			=((struct integralControl *)polyIntegralControl)->intWorkspase;   
	int numberSigmaPoly			=((struct integralControl *)polyIntegralControl)->numberSigma;
	int n_function				=((struct integralControl *)polyIntegralControl)->n_function;
	
	//+++
	double 	P				=gsl_vector_get(F_para, polyNumber);
	
	//+++
	int p=F_para->size;
	
	//+++
	poly1_SANS poly1={polyNumber,Q,F};
	
	//+++
	gsl_function 	  FIpoly;
	FIpoly.params	= &poly1;
	
	//+++
	double polySigma=gsl_vector_get(F_para, p-2);
	
	//+++
	gsl_vector *x;
	gsl_vector *w;

	//+++	
	double i0=0.0; 
	double i=0.0;
	int nodes=15;
	if (absErrPoly==0.0 && relErrPoly==0.0) nodes=intWorkspasePoly;

	//+++
	int ii;
	double Pmin, Pmax, delta;
	
	//+++ Integration Range
	if (polySigma*numberSigmaPoly < 1.0) Pmin=P*(1.0-polySigma*numberSigmaPoly); else Pmin=0;
	Pmax=P*(1+polySigma*numberSigmaPoly);
	
	// +++
	switch (n_function)
	{
	case 0:		
	    //+++"Gauss"
	    FIpoly.function = &polyFunctionGauss;
		
	    // Integral: Gauss-Legendre-Quadrature 
	    do
	    {
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//
		GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
		//
		for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
		// step=5
		//nodes=nodes+1;
        nodes=nodes*2;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	    }
	    while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);  

	    i=i/M_SQRT2/M_SQRTPI/(P*polySigma)/ (1-0.5*gsl_sf_erfc(1/polySigma/M_SQRT2));
	    //
	    break;   

	case 1:
	//+++"Schultz-Zimm"
	FIpoly.function = &polyFunctionSZ;
		
	// Integral: Gauss-Legendre-Quadrature 
	do
	{
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//
		GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
		//
		for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
		//+++
        //nodes=nodes+1;
        nodes=nodes*2;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	}
	while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);  
	//
	break;   


	case 2:
	//+++"Gamma"
	FIpoly.function = &polyFunctionGamma;
		
	// Integral: Gauss-Legendre-Quadrature 
	do
	{
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//
		GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
		//
		for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
		//+++
        //nodes=nodes+1;
        nodes=nodes*2;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	}
	while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);  
	//    
	break;
	
	case 3:  
	//+++"Log-Normal"	    
	FIpoly.function = &polyFunctionLogNormal;

	// Integral: Gauss-Legendre-Quadrature 
	do
	{
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//
		GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
		//
		for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
		//+++
        //nodes=nodes+1;
        nodes=nodes*2;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	}
	while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);  
	
	//+++
	break;   

	case 4: 
	//+++"Uniform"
	//+++
	FIpoly.function = &polyFunctionUniform;
	//
	delta=gsl_vector_get(F_para, p-2);


	//+++ Integration Range
       	if (polySigma	 < 1.0) Pmin=P*(1.0-polySigma); else Pmin=0;
	Pmax=P*(1+polySigma);


	// Integral: Gauss-Legendre-Quadrature 
	do
	{
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//
		GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
		//
		for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
		//+++
        //nodes=nodes+1;
        nodes=nodes*2;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	}
	while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);
			
	//
	i/=(Pmax-Pmin);
		
		break;   		 
	case 5:
		//+++Triangular"
		//
		FIpoly.function = &polyFunctionTriangular;
		//
		Pmin=gsl_vector_get(F_para, p-2);
		Pmax=gsl_vector_get(F_para, p-1);
		//
		if (Pmin>=Pmax || (P>Pmin && P>Pmax ) || (P<Pmin && P<Pmax)) i=GSL_FN_EVAL(F,Q);
		else
		{
			// Integral: Gauss-Legendre-Quadrature 
			do
			{
				i0=i;
				i=0;
				//
				x=gsl_vector_alloc(nodes);
				w=gsl_vector_alloc(nodes);
				//
				GaussLegendreQuadrature(nodes, Pmin, Pmax, x, w);
				//
				for (ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&FIpoly,gsl_vector_get(x,ii));
				// step=5
                //nodes=nodes+1;
                nodes=nodes*2;
				//
				gsl_vector_free(x);
				gsl_vector_free(w);
			}
			while(fabs(i-i0)>absErrPoly && fabs(i-i0)>relErrPoly*fabs(i0) &&   nodes<= intWorkspasePoly);  
		}
		
		break;   		 
	default :  break;
}
	return i;
}

//*******************************************
//*reso-poly-Function
//*******************************************
double resoPolyFunction(double Q, void *polyReso0)
{  
	//
	poly2_SANS *poly2 	=((struct polyReso0_SANS *)polyReso0)->poly2;
	//   
	double 	resoSigma	=((struct polyReso0_SANS *)polyReso0)->resoSigma;
	double 	Q0 		=((struct polyReso0_SANS *)polyReso0)->Q0;
	//
	double result=exp(-0.5*(Q-Q0)*(Q-Q0)/resoSigma/resoSigma)*polyIntegral(Q,poly2);
	//
	return result;
}

//*******************************************
//*Reso-Poly-Integral
//*******************************************
double resoPolyFunctionNew(double Q0, void *polyReso1)
{

poly2_SANS 	*poly2 		=((struct polyReso1_SANS *)polyReso1)->poly2;
double		 resoSigma 	=((struct polyReso1_SANS *)polyReso1)->resoSigma;
integralControl *resoIntegralControl 	=((struct polyReso1_SANS *)polyReso1)->resoIntegralControl;

gsl_function 	polyFunctionLocal;
polyFunctionLocal.function=&polyIntegral;
polyFunctionLocal.params=poly2;


resoSANS resoSANS_poly={resoSigma, Q0, &polyFunctionLocal,resoIntegralControl};

return resoIntegral(Q0,&resoSANS_poly) ;
};

//*******************************************
//*Reso-Poly-Integral
//*******************************************
double resoPolyIntegral(double Q0, void *polyReso1)
{	
	// +++
	poly2_SANS	*poly2 			=((struct polyReso1_SANS *)polyReso1)->poly2;
	double 		resoSigma		=((struct polyReso1_SANS *)polyReso1)->resoSigma;
	integralControl 	*resoIntegralControl 	=((struct polyReso1_SANS *)polyReso1)->resoIntegralControl;
	// +++
	double 		absErrReso			=((struct integralControl *)resoIntegralControl)->absErr;
	double 		relErrReso			=((struct integralControl *)resoIntegralControl)->relErr;    
	int 		intWorkspaseReso		=((struct integralControl *)resoIntegralControl)->intWorkspase;   
	int 		numberSigmaReso		=((struct integralControl *)resoIntegralControl)->numberSigma;
	// +++
	polyReso0_SANS polyReso0={poly2,resoSigma,Q0}; 
	// +++
	gsl_function 	FresoPoly;
	FresoPoly.function = &resoPolyFunction;
	FresoPoly.params =  &polyReso0;
	// +++
	double result, error;
	// +++
	gsl_integration_workspace *w = gsl_integration_workspace_alloc(intWorkspaseReso);
	double Q0min=0;
	if ((Q0-(numberSigmaReso*resoSigma))>0) Q0min=Q0-(numberSigmaReso*resoSigma);
	// +++
	gsl_integration_qags (&FresoPoly,Q0min,Q0+(numberSigmaReso*resoSigma), absErrReso, relErrReso, intWorkspaseReso, w, &result, &error);
	// +++
	gsl_integration_workspace_free(w);
	// +++
	return result/M_SQRT2/M_SQRTPI/resoSigma/(1-0.5*gsl_sf_erfc(Q0/resoSigma/M_SQRT2));
}

//*******************************************
//*Reso-Function:: Gauss
//*******************************************
double ResoFunction(double Q, void *paraM)
{
	double 	resoSigma	=((struct resoSANS *)paraM)->resoSigma;
	double 	Q0 		=((struct resoSANS *)paraM)->Q0;
	gsl_function *F 		=((struct resoSANS *)paraM)->function;
	//
	return exp(-0.5*(Q-Q0)*(Q-Q0)/resoSigma/resoSigma)*GSL_FN_EVAL(F,Q);
}

//*******************************************
//*Reso-Function:: Triangular
//*******************************************
double ResoFunctionTriangle(double Q, void *paraM)
{
	double 	resoSigma	=((struct resoSANS *)paraM)->resoSigma;
	double 	Q0 		=((struct resoSANS *)paraM)->Q0;
	gsl_function *F 		=((struct resoSANS *)paraM)->function;
	//
	double result=0;
	
	if (Q>=(Q0-2*resoSigma) && Q<=Q0 ) result =(Q-Q0+2*resoSigma)/resoSigma/resoSigma/4*GSL_FN_EVAL(F,Q);
	if (Q<=(Q0+2*resoSigma) && Q>Q0) result =-(Q-Q0-2*resoSigma)/resoSigma/resoSigma/4*GSL_FN_EVAL(F,Q);
	
	return result;
}

//*******************************************
//*Reso-Function:: Bessel I0
//*******************************************
double ResoFunctionBessel(double Q, void *paraM)
{
	double 	resoSigma	=((struct resoSANS *)paraM)->resoSigma;
	double 	Q0 		=((struct resoSANS *)paraM)->Q0;
	gsl_function *F 		=((struct resoSANS *)paraM)->function;
	//
	double result=0;
	
	if (resoSigma<0 || Q<0 || Q0<=0) return 0;
    
	//--- See GSL reference    
	result= Q/resoSigma/resoSigma*exp(-0.5*(Q*Q+Q0*Q0)/resoSigma/resoSigma);
	result*=gsl_sf_bessel_I0(Q*Q0/resoSigma/resoSigma);
		
	return result*GSL_FN_EVAL(F,Q);
}

//*******************************************
//*Reso-Integral
//*******************************************
double resoIntegral(double Q, void *paraM)
{
	gsl_function 	Freso;
	
	//+++
	integralControl *resoIntegralControl =((struct resoSANS *)paraM)->resoIntegralControl;
	
	//+++    
	double 	absErr		=((struct integralControl *)resoIntegralControl)->absErr;
	double 	relErr		=((struct integralControl *)resoIntegralControl)->relErr;    
	int 	intWorkspase	=((struct integralControl *)resoIntegralControl)->intWorkspase;   
	int 	numberSigma	=((struct integralControl *)resoIntegralControl)->numberSigma; 
	int 	n_function 	=((struct integralControl *)resoIntegralControl)->n_function;
	
	//+++
	double 	resoSigma	=((struct resoSANS *)paraM)->resoSigma;
	

	if (resoSigma<=0 || n_function>3)
	{
	     gsl_function *function =((struct resoSANS *)paraM)->function;
	    return GSL_FN_EVAL(function,Q);
	}
	//+++
	int nodes=10;
	//+++
	if (absErr==0 && relErr==0) nodes=intWorkspase;

	//+++
	double Qmin, Qmax, i0, i;
	gsl_vector *x,*w;
	//

	switch (n_function)
    {
        case 0:
            //+++ "Gauss"
            Freso.function = &ResoFunction;
            break;
            
        case 1:
            //+++ Triangular
            Freso.function = &ResoFunctionTriangle;
            //+++
            numberSigma=2;
            
            break;
            
        case 2:
            //+++ Bessel
            Freso.function = &ResoFunctionBessel;
            break;
            
        case 3:
            //+++ "Gauss"
            Freso.function = &ResoFunction;
            break;
    }
	 
	Freso.params = (void *)paraM;

	//+++
	//2014-02-test if ((Q-numberSigma*resoSigma)>0) Qmin=Q-numberSigma*resoSigma;  else Qmin= 0;
    Qmin=Q-numberSigma*resoSigma; //2014-02-test
    if(Qmin<0 && n_function<=3 && n_function!=1) Qmin=0;
    
	Qmax=Q+numberSigma*resoSigma;

	i=0;
	    
	// Integral: Gauss-Legendre-Quadrature 
	do
	{
		i0=i;
		i=0;
		//
		x=gsl_vector_alloc(nodes);
		w=gsl_vector_alloc(nodes);
		//	
		GaussLegendreQuadrature(nodes, Qmin, Qmax, x, w);
		//
		for (int ii=0;ii<nodes;ii++) i+=gsl_vector_get(w,ii)* GSL_FN_EVAL(&Freso,gsl_vector_get(x,ii));
        
		nodes=nodes+1;
		//
		gsl_vector_free(x);
		gsl_vector_free(w);
	}
	while(fabs(i-i0)>absErr && fabs(i-i0)>relErr*fabs(i0) &&   nodes < intWorkspase);  
	//

	switch (n_function)
    {
        case 0:
            //+++ "Gauss"
            i=i/M_SQRT2/M_SQRTPI/resoSigma/ (1-0.5*gsl_sf_erfc(Q/resoSigma/M_SQRT2));
            break;
        case 3:
            //+++ "Gauss"
            i=i/M_SQRT2/M_SQRTPI/resoSigma;// 2014-02
            break;
    }
	
	return i;
}



//*function_fmPoly*******************************************
int function_fmPoly (const gsl_vector * x, void *params, gsl_vector * f)
{ 
	//+++
	sizetNumbers *sizetNumbers	=((struct fitDataSANSpoly *)params)->SizetNumbers;
	
	size_t M 		= ((struct sizetNumbers *)sizetNumbers)->M;


	//+++
	sansData *sansData	= ((struct fitDataSANSpoly *)params)->SansData;

	double 	*Q 		= ((struct sansData *)sansData)->Q;
	double	*I 		= ((struct sansData *)sansData)->I;
	double	*Weight = ((struct sansData *)sansData)->Weight;
	double	*sigmaReso 	= ((struct sansData *)sansData)->sigmaReso;

	//+++
	int *controlM	 	= ((struct fitDataSANSpoly *)params)->controlM;
	gsl_vector *para	= ((struct fitDataSANSpoly *)params)->para;
	gsl_vector_int *paraF	= ((struct fitDataSANSpoly *)params)->paraF;
	
	//+++
	gsl_function *F		= ((struct fitDataSANSpoly *)params)->function;
	
	//+++ 2011-09-15
	gsl_vector *paraP= ((functionT *)F->params)->para;
	int  prec= ((functionT *)F->params)->prec;
    
    size_t p         = ((struct sizetNumbers *)sizetNumbers)->p;
    size_t pPoly         = paraP->size;
    
	//+++
	integralControl *resoIntegralControl	= ((struct fitDataSANSpoly *)params)->resoIntegralControl;
	int 		*polyNumber		= ((struct fitDataSANSpoly *)params)->polyNumber;   
	integralControl *polyIntegralControl	= ((struct fitDataSANSpoly *)params)->polyIntegralControl;
	
	gsl_vector *limitLeft	= ((struct fitDataSANSpoly *)params)->limitLeft;
	gsl_vector *limitRight	= ((struct fitDataSANSpoly *)params)->limitRight;	
	//+++
	size_t pM=p*M;  // pM=p x M (total number of parameters)
	//+++
	size_t pFit=0;   	// number adjustable parameters
	//+++
	size_t i;
	
	double temp;
	
	
	
	//+++ Check pFit number +++
    for (i=0; i<pM; i++)
    {
        if (gsl_vector_int_get(paraF,i)==0)
        {
            if ( gsl_vector_get(x,pFit)<gsl_vector_get(limitLeft,pFit) )
                gsl_vector_set(para,i,gsl_vector_get(limitLeft,pFit));
            else if ( gsl_vector_get(x,pFit)>gsl_vector_get(limitRight,pFit) )
                gsl_vector_set(para,i,gsl_vector_get(limitRight,pFit));
            else
                gsl_vector_set(para,i, gsl_vector_get(x,pFit));
            pFit++;
        }
        if (gsl_vector_int_get(paraF,i)==2)
        {
            int ii=i;
            while (gsl_vector_int_get(paraF,ii)==2) {ii--;};
            gsl_vector_set(para,i, gsl_vector_get(para,ii));
        }
    }

	//+++
	size_t iMstart=0, iMfinish=0;
	double Ii;
	
	//***  m: from data set to data set... ***
	size_t mm;

	
	
	for (mm = 0; mm < M; mm++)
	{
		if (mm==0) 
		{
			iMstart=0;
			iMfinish=controlM[0];
		}
		else 
		{
			iMfinish+=controlM[mm];
			iMstart=iMfinish-controlM[mm];
		}

		
		((functionT *)F->params)->currentFirstPoint=iMstart;
		((functionT *)F->params)->currentLastPoint=iMfinish-1;
		((functionT *)F->params)->currentPoint=iMstart;
		
		//+++
		size_t pp;
		for (pp=0;pp<p;pp++) gsl_vector_set(paraP,pp,gsl_vector_get(para,M*pp+mm));

        
		//+++
		size_t im;
        
        double polySigma;
		if (((struct integralControl *)resoIntegralControl)->n_function>5)
		{
		    polySigma=-1.0;
		}
        else if (((struct integralControl *)resoIntegralControl)->n_function==5)
        {
            polySigma=gsl_vector_get(paraP, pPoly-1)+gsl_vector_get(paraP, pPoly-2);
        }
		else
		{
		    polySigma=gsl_vector_get(paraP, pPoly-2);
		}
		
		if (polyNumber[mm]>=0 && polySigma>0.0) ((functionT *)F->params)->polyYN=true;
		
		double Int1, Int2, Int3;
		
		if (polyNumber[mm]>=0 && polySigma > 0.0)
		{
		    poly2_SANS poly2={F, polyNumber[mm], polyIntegralControl};
		    
		    //____before integrals
		    ((functionT *)F->params)->beforeIter=true;
		    ((functionT *)F->params)->currentPoint=iMstart;
		    ((functionT *)F->params)->currentInt=-1;
		    //polyIntegral(Q[iMstart],&poly2);
            GSL_FN_EVAL(F,Q[iMstart]);//+++2020.04
		    
		    //+++
		    // integral1
		    ((functionT *)F->params)->currentInt=1; 
		    Int1=polyIntegral(Q[iMstart],&poly2); 
		    ((functionT *)F->params)->Int1=Int1;
		    // integral2
		    ((functionT *)F->params)->currentInt=2; 
		    Int2=polyIntegral(Q[iMstart],&poly2);
		    ((functionT *)F->params)->Int2=Int2;    
		    // integral3
		    ((functionT *)F->params)->currentInt=3; 
		    Int3=polyIntegral(Q[iMstart],&poly2);
		    ((functionT *)F->params)->Int3=Int3;
		    
            //___ after integrals
		    ((functionT *)F->params)->currentInt=0;
            //polyIntegral(Q[iMstart],&poly2);
            GSL_FN_EVAL(F,Q[iMstart]);//+++2020.04
            
            ((functionT *)F->params)->beforeIter=false;
            
        }
		else
		{
		    //_______________________________________________    
		    ((functionT *)F->params)->beforeIter=true;
            ((functionT *)F->params)->currentPoint=iMstart;
		    ((functionT *)F->params)->currentInt=-1;
		    GSL_FN_EVAL(F,Q[iMstart]);
		    
		    //+++
		    // integral1
		    ((functionT *)F->params)->currentInt=1; 
		    Int1=GSL_FN_EVAL(F,Q[iMstart]); 
		    ((functionT *)F->params)->Int1=Int1;
		    // integral2
		    ((functionT *)F->params)->currentInt=2; 
		    Int2=GSL_FN_EVAL(F,Q[iMstart]);
		    ((functionT *)F->params)->Int2=Int2;    
		    // integral3
		    ((functionT *)F->params)->currentInt=3; 
		    Int3=GSL_FN_EVAL(F,Q[iMstart]);
		    ((functionT *)F->params)->Int3=Int3;
		    
		    ((functionT *)F->params)->currentInt=0;
            GSL_FN_EVAL(F,Q[iMstart]);
            
            ((functionT *)F->params)->beforeIter=false;
		}
		
		//+++
		((functionT *)F->params)->currentInt=0; 
		//_______________________________________________
		
		
		
		//+++
		for (im=iMstart;im<iMfinish;im++)
		{
		    ((functionT *)F->params)->currentPoint=im;
		    if (sigmaReso[im]>0)   //reso Yes
            {
                if (polyNumber[mm]<0 || polySigma<=0.0)
                {
                    resoSANS paraReso = {sigmaReso[im], Q[im], F, resoIntegralControl};
                    Ii = resoIntegral(Q[im],&paraReso);
                    Ii = (Ii - I[im])/Weight[im];
                    gsl_vector_set (f, im, Ii);
                }
                else
                {
                    poly2_SANS poly2={F, polyNumber[mm], polyIntegralControl};
                    polyReso1_SANS polyReso1={&poly2,sigmaReso[im], resoIntegralControl};
                    Ii=resoPolyFunctionNew(Q[im], &polyReso1);
                    Ii=(Ii - I[im])/Weight[im];
                    gsl_vector_set (f, im, Ii);
                }
            }
		    else
            {
                if (polyNumber[mm]<0 || polySigma<=0.0)
                {
                    Ii =GSL_FN_EVAL(F,Q[im]);
                    Ii=(Ii - I[im])/Weight[im];
                    gsl_vector_set (f, im, Ii);
                }
                else
                {
                    poly2_SANS poly2={F, polyNumber[mm], polyIntegralControl};
                    Ii=polyIntegral(Q[im],&poly2);
                    Ii=(Ii - I[im])/Weight[im];
                    gsl_vector_set (f, im, Ii);
                }
            }
		}	
		//+++ new::  initAfterIteration 2011-08-19
		((functionT *)F->params)->afterIter=true;		
		((functionT *)F->params)->currentPoint=iMfinish-1;
		GSL_FN_EVAL(F,Q[iMfinish-1]);
		((functionT *)F->params)->afterIter=false;
		((functionT *)F->params)->polyYN=false; 
		//---	
	}
	return GSL_SUCCESS;
}
//-function_fmPoly*******************************************


//*******************************************
//+++  function_simplyFit_derivative +++
//*******************************************
double function_sasFit_derivative (double x, void *params)
{
    double res=0;
    fitDataSANSpoly *sansPoly   =((struct sasFitDerivative *)params)->sansPoly;
    size_t indexX               = ((struct sasFitDerivative *)params)->indexX;
    double Q                    = ((struct sasFitDerivative *)params)->Q;
    int mm                      =((struct sasFitDerivative *)params)->m;
    
    
    gsl_vector *para = sansPoly->para;

    size_t M         = sansPoly->SizetNumbers->M;

    
    // ((struct fitDataSANSpoly *)params)->
    int       *polyNumber                   = sansPoly->polyNumber;

    integralControl *resoIntegralControl    = sansPoly->resoIntegralControl;
    integralControl *polyIntegralControl    = sansPoly->polyIntegralControl;
    
    gsl_function *F                         = sansPoly->function;
    
    int  prec                               = ((functionT *)F->params)->prec;
    gsl_vector *paraP                       = ((functionT *)F->params)->para;
    size_t p         = paraP->size;
    
    int   currentPoint                      = ((functionT *)F->params)->currentPoint;
    
    sansData *sansData                      = sansPoly->SansData;
    
    //((struct sansData *)sansData)
    double    *sigmaReso     = sansData->sigmaReso;
    
    double polySigma;
    if (resoIntegralControl->n_function>5)
    {
        polySigma=-1.0;
    }
    else if (resoIntegralControl->n_function==5)
    {
        polySigma=gsl_vector_get(paraP, p-1)+gsl_vector_get(paraP, p-2);
    }
    else
    {
        polySigma=gsl_vector_get(paraP, p-2);
    }
    
    //+++ data structure:: simplyFitDerivative :: extracting of parameters

    
    
    //+++
    double oldX=gsl_vector_get(paraP,indexX);
    gsl_vector_set(paraP,indexX,x);
    //+++
    double oldXglobal=gsl_vector_get(para,indexX*M+mm);
    gsl_vector_set(para,indexX*M+mm,x);
    
    if (sigmaReso[currentPoint]>0)   //reso Yes
    {
        if (polyNumber[mm]<0 || polySigma<=0.0)
        {
            resoSANS paraReso = {sigmaReso[currentPoint], Q, F, resoIntegralControl};
            res = resoIntegral(Q,&paraReso);
        }
        else
        {
            poly2_SANS poly2={F, polyNumber[mm], polyIntegralControl};
            polyReso1_SANS polyReso1={&poly2,sigmaReso[currentPoint], resoIntegralControl};
            res=resoPolyFunctionNew(Q, &polyReso1);
        }
    }
    else
    {
        if (polyNumber[mm]<0 || polySigma<=0.0)
        {
            res = GSL_FN_EVAL(F,Q);
        }
        else
        {
            poly2_SANS poly2={F, polyNumber[mm], polyIntegralControl};
            res=polyIntegral(Q,&poly2);
        }
    }

    //+++
    gsl_vector_set(paraP,indexX,oldX);
    //+++
    gsl_vector_set(para,indexX*M+mm,oldXglobal);
    
    return res;
}

//*******************************************
//+++ function_dfm +++
//*******************************************
int function_dfmPoly (const gsl_vector * x, void *params, gsl_matrix * J)
{
    //+++ preparation of derivative function
    gsl_function *F         = ((struct fitDataSANSpoly *)params)->function;
    gsl_vector *paraP    = ((functionT *)F->params)->para;
    int  prec= ((functionT *)F->params)->prec;
    
    sasFitDerivative  sasFitDerivativeL = {(struct fitDataSANSpoly *)params, 0, 0, 0};
    
    gsl_function FD;
    FD.function = function_sasFit_derivative;
    FD.params = &sasFitDerivativeL ;
    
    //+++ getting of different parameters/datasets
    sizetNumbers *sizetNumbers    =((struct fitDataSANSpoly *)params)->SizetNumbers;
    //
    size_t N         = ((struct sizetNumbers *)sizetNumbers)->N;
    size_t M         = ((struct sizetNumbers *)sizetNumbers)->M;
    size_t p         = ((struct sizetNumbers *)sizetNumbers)->p;
    size_t np         = ((struct sizetNumbers *)sizetNumbers)->np;
    double STEP     = ((struct sizetNumbers *)sizetNumbers)->STEP;

    double STEPlocal=STEP;

    gsl_vector_int *paraF    = ((struct fitDataSANSpoly *)params)->paraF;
    gsl_vector     *para     = ((struct fitDataSANSpoly *)params)->para;
    int *controlM            = ((struct fitDataSANSpoly *)params)->controlM;
    //
    
    double   *Weight     = ((struct fitDataSANSpoly *)params)->SansData->Weight;
    double   *Q          = ((struct fitDataSANSpoly *)params)->SansData->Q;


    size_t pM=p*M;
    
    gsl_vector *limitLeft     = ((struct fitDataSANSpoly *)params)->limitLeft;
    gsl_vector *limitRight    = ((struct fitDataSANSpoly *)params)->limitRight;
    
    
    //+++  npCurrent -> parameter number ; mCurrent -> related dataset numbet ; importand when M>1
    int *npCurrent=new int[np];
    int *mCurrent=new int[np];
    
    int ii=0;
    int pFit=0;
    double xx, xxleft, xxright;
    //+++ filling of parameter vectos with current fiffable parametres
    for (int pp=0; pp<p; pp++)  for (int mm=0; mm<M; mm++)
    {
        if (gsl_vector_int_get(paraF,ii)==0)  //+++ =0 fittable parameter
        {
            xx=gsl_vector_get(x,pFit);
            xxleft=gsl_vector_get(limitLeft,pFit);
            xxright=gsl_vector_get(limitRight,pFit);
            
            if ( xx<xxleft ) xx=xxleft;
            if (xx>xxright) xx=xxright;
            
            gsl_vector_set(para,ii,xx); //+++ update of fittable parameter
            
            npCurrent[pFit]=pp; //+++  pp  in [0..p-1]
            mCurrent[pFit]=mm; //+++  mm in [0..M-1]
            pFit++;
        }
        if (gsl_vector_int_get(paraF,ii)==2)
        {
            gsl_vector_set(para,ii,gsl_vector_get(para,M*pp)); // update global parameters
        }
        ii++; //+++ ii < pM
    }
    
    //+++
    double result, abserr;
    
    //+++
    size_t iMstart;
    size_t iMfinish;
    
    int current=0;
    double xxx, pOld;
    
    // +++ new 2016 : filling of the Jacobian matrix of derivatieves
    for ( ii=0;ii<pFit; ii++)
    {
        //xxx=gsl_vector_get(paraP,npCurrent[ii]);
        xxx=gsl_vector_get(x,ii);
        xxleft=gsl_vector_get(limitLeft,ii);
        xxright=gsl_vector_get(limitRight,ii);
        
        if (xxx<xxleft ) xxx=xxleft;
        if (xxx>xxright) xxx=xxright;

        
        current=0;
        for (int mm = 0; mm < M; mm++)
        {
            //+++
            if (mm==0)
            {
                iMstart=0;
                iMfinish=controlM[0];
            }
            else
            {
                iMfinish+=controlM[mm];
                iMstart=iMfinish-controlM[mm];
            }
            
            // +++ move parameters to function
            for (int pp=0;pp<p;pp++) gsl_vector_set(paraP,pp,gsl_vector_get(para,M*pp+mm));
            pOld=gsl_vector_get(paraP,0);
            
            sasFitDerivativeL.Q=Q[current];
            sasFitDerivativeL.m=mm;
            sasFitDerivativeL.indexX=0;
            
            // +++ call function before
            ((functionT *)F->params)->beforeIter=true;
            ((functionT *)F->params)->currentPoint=current;
            ((functionT *)F->params)->currentInt=-1;
            //GSL_FN_EVAL(&FD,pOld);
            GSL_FN_EVAL(F,Q[current]);//+++2020.04
            
            //+++ calculate integrals
            // integral1
            ((functionT *)F->params)->currentInt=1;
            double Int1=GSL_FN_EVAL(&FD,pOld);
            ((functionT *)F->params)->Int1=Int1;
            // integral2
            ((functionT *)F->params)->currentInt=2;
            double Int2=GSL_FN_EVAL(&FD,pOld);
            ((functionT *)F->params)->Int2=Int2;
            // integral3
            ((functionT *)F->params)->currentInt=3;
            double Int3=GSL_FN_EVAL(&FD, pOld);
            ((functionT *)F->params)->Int3=Int3;
            
            //+++
            ((functionT *)F->params)->currentInt=0;
            //GSL_FN_EVAL(&FD,pOld);
            GSL_FN_EVAL(F,Q[current]);//+++2020.04
            
            ((functionT *)F->params)->beforeIter=false;
            
            
            for (int im=iMstart;im<iMfinish;im++)
            {
                if (mCurrent[ii]!=mm && gsl_vector_int_get(paraF,M*npCurrent[ii]+mm)!=2)
                {
                    //+++ important if parameters do not related to current data set (mCurrent[ii]!=mm) and parameter is not GLOBAL
                    //+++ derivatieve = 0.0
                    gsl_matrix_set (J, current, ii, 0.0);
                    current++;
                    continue;
                }
                
                //+++ derivative function filling
                ((functionT *)F->params)->currentPoint=current;
                ((sasFitDerivative *)FD.params)->Q=Q[current];
                ((sasFitDerivative *)FD.params)->indexX=npCurrent[ii];
                
                //+++ calculation
                if (xxx==0.0) STEPlocal=STEP; else STEPlocal=STEP*fabs(xxx);
                
                if ( (xxx-xxleft) < STEPlocal) gsl_deriv_forward(&FD, xxx, STEPlocal, &result, &abserr);
                else if((xxright-xxx)<STEPlocal) gsl_deriv_backward(&FD, xxx, STEPlocal, &result, &abserr);
                else gsl_deriv_central (&FD, xxx, STEPlocal, &result, &abserr);
                
                //gsl_deriv_central (&FD, xxx, STEPlocal, &result, &abserr);
                //std::cout<<current<<" "<<ii<<" "<<result<<" "<<Weight[current]<<"\n"<<std::flush;
                //if (Weight[im]!=0) gsl_matrix_set (J, current, ii, result/Weight[current]);
                if (Weight[current]!=0) gsl_matrix_set (J, current, ii, result/Weight[current]);
                else gsl_matrix_set (J, current, ii, result);
                //std::cout<<current<<" "<<ii<<" "<<Weight[current]<<"\n"<<std::flush;

                current++;
            }
            
            
            // +++ call function after
            sasFitDerivativeL.Q=Q[current-1];
            sasFitDerivativeL.indexX=0;
            
            ((functionT *)F->params)->afterIter=true;
            ((functionT *)F->params)->currentPoint=current-1;
            //GSL_FN_EVAL(&FD,pOld);
            GSL_FN_EVAL(F,Q[current-1]); //+++2020.04
            ((functionT *)F->params)->afterIter=false;
            
        }
    }
    
    delete[] npCurrent;
    delete[] mCurrent;
    
    return GSL_SUCCESS;
}

//*function_dfmPoly*******************************************
int function_dfmPolyFast (const gsl_vector * x, void *params, gsl_matrix * J)
{
	sizetNumbers *sizetNumbers	=((struct fitDataSANSpoly *)params)->SizetNumbers;
	//
	size_t N 		= ((struct sizetNumbers *)sizetNumbers)->N;
	size_t M 		= ((struct sizetNumbers *)sizetNumbers)->M;
	size_t p 		= ((struct sizetNumbers *)sizetNumbers)->p;
	size_t np 		= ((struct sizetNumbers *)sizetNumbers)->np;
    double STEP     = ((struct sizetNumbers *)sizetNumbers)->STEP;
    
	//
	sansData *sansData	=((struct fitDataSANSpoly *)params)->SansData;
	
	double   *Weight 	= ((struct sansData *)sansData)->Weight;
    double   *I     = ((struct sansData *)sansData)->I;
	
	gsl_vector_int *paraF	= ((struct fitDataSANSpoly *)params)->paraF; 
	// 
	
	gsl_function *F 		= ((struct fitDataSANSpoly *)params)->function;
    int  prec= ((functionT *)F->params)->prec;
	//+++ 2011-09-15
	gsl_vector *para= ((functionT *)F->params)->para;	

    gsl_vector *limitLeft    = ((struct fitDataSANSpoly *)params)->limitLeft;
    gsl_vector *limitRight    = ((struct fitDataSANSpoly *)params)->limitRight;
    
	size_t pFit=0;
	size_t i;
	size_t pM=p*M;
	
    //+++
    for (i=0; i<pM; i++)
    {
        if (gsl_vector_int_get(paraF,i)==0)
        {
            if ( gsl_vector_get(x,pFit)<gsl_vector_get(limitLeft,pFit) ) gsl_vector_set(para,i,gsl_vector_get(limitLeft,pFit));
            else if ( gsl_vector_get(x,pFit)>gsl_vector_get(limitRight,pFit) ) gsl_vector_set(para,i,gsl_vector_get(limitRight,pFit));
            else gsl_vector_set(para,i, gsl_vector_get(x,pFit));
            pFit++;
        }
        if (gsl_vector_int_get(paraF,i)==2)
        {
            int ii=i;
            while (gsl_vector_int_get(paraF,ii)==2) {ii--;};
            gsl_vector_set(para,i, gsl_vector_get(para,ii));
        }
    }
    
	double h, xxx, temp;
	
	gsl_vector *xMinus2h=gsl_vector_alloc(np);
    gsl_vector *xMinus1h=gsl_vector_alloc(np);
    gsl_vector *xPlus1h=gsl_vector_alloc(np);
    gsl_vector *xPlus2h=gsl_vector_alloc(np);
    
	gsl_vector *xStep=gsl_vector_alloc(np);
	gsl_vector *xCurr=gsl_vector_alloc(np);
	
	for (i=0;i<np;i++ )
	{
		xxx=gsl_vector_get(x,i);
		if (xxx<pow(10, -prec)) h=STEP; else h=fabs(STEP*xxx);
        h=h/2;
        
		gsl_vector_set(xMinus2h,i,xxx-2*h);
        gsl_vector_set(xMinus1h,i,xxx-h);
        gsl_vector_set(xPlus1h,i,xxx+h);
        gsl_vector_set(xPlus2h,i,xxx+2*h);
		
        gsl_vector_set(xStep,i,h);
	}
	
	gsl_vector 	*fMinus2h 	=gsl_vector_alloc(N);
    gsl_vector  *fMinus1h     =gsl_vector_alloc(N);
    gsl_vector 	*fPlus1h 	=gsl_vector_alloc(N);
    gsl_vector  *fPlus2h     =gsl_vector_alloc(N);
    
    double tmpIi, tmpIim, tmpW;
	for (i=0;i<np; i++)
	{
		gsl_vector_memcpy(xCurr,x);
		gsl_vector_set(xCurr,i,gsl_vector_get(xMinus2h,i));
		function_fmPoly(xCurr,params,fMinus2h);
        gsl_vector_set(xCurr,i,gsl_vector_get(xMinus1h,i));
        function_fmPoly(xCurr,params,fMinus1h);
		gsl_vector_set(xCurr,i,gsl_vector_get(xPlus1h,i));
		function_fmPoly(xCurr,params,fPlus1h);
        gsl_vector_set(xCurr,i,gsl_vector_get(xPlus2h,i));
        function_fmPoly(xCurr,params,fPlus2h);
        
        //+++ 2019-06
        for (int nn = 0; nn < N; nn++)
        {
            tmpIim=I[nn];
            tmpW=Weight[nn];
            tmpIi=gsl_vector_get(fMinus2h,nn);
            gsl_vector_set(fMinus2h,nn,tmpIi*tmpW+tmpIim);
            tmpIi=gsl_vector_get(fMinus1h,nn);
            gsl_vector_set(fMinus1h,nn,tmpIi*tmpW+tmpIim);
            tmpIi=gsl_vector_get(fPlus1h,nn);
            gsl_vector_set(fPlus1h,nn,tmpIi*tmpW+tmpIim);
            tmpIi=gsl_vector_get(fPlus2h,nn);
            gsl_vector_set(fPlus2h,nn,tmpIi*tmpW+tmpIim);
        }
        
		//+++

		for (int nn = 0; nn < N; nn++)
		{
			/* Jacobian matrix J(i,j) = dfi / dxj, */
			/* where fi = (Yi - yi)/sigma[i],      */

            if (gsl_vector_get(xStep,i)!=0.0 )
            {
                temp=-gsl_vector_get(fPlus2h,nn) + 8*gsl_vector_get(fPlus1h,nn);
                temp=temp-8*gsl_vector_get(fMinus1h,nn)+gsl_vector_get(fMinus2h,nn);
                temp=temp/12.0/gsl_vector_get(xStep,i);
            }
            else temp=0.0;
            
			if (Weight[nn]!=0.0) gsl_matrix_set (J, nn, i, temp/Weight[nn]);
			else gsl_matrix_set (J, nn, i, 0.0);		
		}
	}
    gsl_vector_free(xMinus2h);
	gsl_vector_free(xMinus1h);
	gsl_vector_free(xPlus1h);
    gsl_vector_free(xPlus2h);
	gsl_vector_free(xStep);
	gsl_vector_free(xCurr);

    gsl_vector_free(fMinus2h);
    gsl_vector_free(fMinus1h);
	gsl_vector_free(fPlus1h);
    gsl_vector_free(fPlus2h);
	return GSL_SUCCESS;
}
//-function_dfmPoly*******************************************


//*function_fdfmPoly*******************************************
int function_fdfmPoly (const gsl_vector * x, void *params,
					   gsl_vector * f, gsl_matrix * J)
{
	function_fmPoly (x, params, f);
	function_dfmPoly (x, params, J);
	return GSL_SUCCESS;   
}
//-function_fdfmPoly*******************************************

//*function_fdfmPoly*******************************************
int function_fdfmPolyFast (const gsl_vector * x, void *params,
                       gsl_vector * f, gsl_matrix * J)
{
    function_fmPoly (x, params, f);
    function_dfmPolyFast (x, params, J);
    return GSL_SUCCESS;
}
//-function_fdfmPoly*******************************************

//*function_dmPoly*******************************************
double function_dmPoly (const gsl_vector * x, void *params)
{
	sizetNumbers *sizetNumbers	=((struct fitDataSANSpoly *)params)->SizetNumbers;
	size_t 	N 		= ((struct sizetNumbers *)sizetNumbers)->N;
	
	gsl_vector 	*fu 	=gsl_vector_alloc(N);
	
	function_fmPoly(x, params, fu);
	
	double Ii=0;
	
	//+++
	size_t i;
	for(i=0; i<N; i++) 
		Ii+=gsl_vector_get(fu,i)*gsl_vector_get(fu,i);
	
	gsl_vector_free(fu); //destroy fu
	
	return Ii;
}
//-function_dm*******************************************



/***
  ***
  *** Global Fit :: no instrumental options 
  ***
  ***  last update : ... 2016-02-22 ...
  ***
  ***/

//*******************************************
//+++function_fdfm
//*******************************************
int function_fdfm(const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J)
{

    function_fm (x, params, f);
    function_dfm(x, params, J);
    return GSL_SUCCESS;   
}

//*******************************************
//+++ function_dm :: chi2 calculation +++
//*******************************************
double function_dm (const gsl_vector * x, void *params)
{
    //+++ 
    size_t 		N = ((struct simplyFitP *)params)->N;
    gsl_function 	*F = ((struct simplyFitP *)params)->function;
    
    //+++
    gsl_vector 	*fu 	=gsl_vector_alloc(N);
    //+++
    function_fm(x, params, fu);
    //+++
    double Ii=0;
    //+++
    for(size_t i=0; i<N; i++) Ii+=gsl_vector_get(fu,i)*gsl_vector_get(fu,i);
    //+++
    gsl_vector_free(fu);
    //+++
    return Ii;
}


//*******************************************
//+++ function_fm
//*******************************************
int function_fm (const gsl_vector * x, void *params,  gsl_vector * f)
{	
    //+++
    size_t M = ((struct simplyFitP *)params)->M; 
    size_t p = ((struct simplyFitP *)params)->p;
    size_t np= ((struct simplyFitP *)params)->np; 
    double *Q = ((struct simplyFitP *)params)->Q;
    double *I = ((struct simplyFitP *)params)->I;
    double *Weight = ((struct simplyFitP *)params)->Weight;
    int *controlM = ((struct simplyFitP *)params)->controlM;
    gsl_vector *para = ((struct simplyFitP *)params)->para;
    gsl_vector_int *paraF = ((struct simplyFitP *)params)->paraF;
    gsl_function *F 	= ((struct simplyFitP *)params)->function;
    int  prec= ((functionT *)F->params)->prec;
    
    //+++
    size_t i;
    //+++
    size_t pM=p*M;
    //+++ 2011
    gsl_vector *paraP= ((functionT *)F->params)->para;
    
    
    gsl_vector *limitLeft    = ((struct simplyFitP *)params)->limitLeft;
    gsl_vector *limitRight    = ((struct simplyFitP *)params)->limitRight;
    
    
    //+++  npCurrent -> parameter number ; mCurrent -> related dataset numbet ; importand when M>1
    int *npCurrent=new int[np];
    int *mCurrent=new int[np];
    
    int ii=0;
    int pFit=0;
    double xx, xxleft, xxright;
    //+++ filling of parameter vectos with current fiffable parametres
    for (int pp=0; pp<p; pp++)  for (int mm=0; mm<M; mm++)
    {
        if (gsl_vector_int_get(paraF,ii)==0)  //+++ =0 fittable parameter
        {
            xx=gsl_vector_get(x,pFit);
            xxleft=gsl_vector_get(limitLeft,pFit);
            xxright=gsl_vector_get(limitRight,pFit);
            
            if ( xx<xxleft ) xx=xxleft;
            else if (xx>xxright) xx=xxright;
            
            gsl_vector_set(para,ii,xx); //+++ update of fittable parameter
            
            npCurrent[pFit]=pp; //+++  pp  in [0..p-1]
            mCurrent[pFit]=mm; //+++  mm in [0..M-1]
            pFit++;
        }
        if (gsl_vector_int_get(paraF,ii)==2)
        {
            gsl_vector_set(para,ii,gsl_vector_get(para,M*pp)); // update global parameters
        }
        ii++; //+++ ii < pM
    }
	
    //+++
    size_t iMstart=0, iMfinish=0;
    //+++
    size_t mm, pp, im;
    //+++
    double Ii;

    
    for (mm = 0; mm < M; mm++)
    {
        if (mm==0)
        {
            iMstart=0;
            iMfinish=controlM[0];
        }
        else
        {
            iMfinish+=controlM[mm];
            iMstart=iMfinish-controlM[mm];
        }
	
        ((functionT *)F->params)->currentFirstPoint=iMstart;
        ((functionT *)F->params)->currentLastPoint=iMfinish-1;
        ((functionT *)F->params)->currentPoint=iMstart;

        //+++
        for (pp=0;pp<p;pp++) gsl_vector_set(paraP,pp,gsl_vector_get(para,M*pp+mm));


        //_______________________________________________
        ((functionT *)F->params)->beforeIter=true;
        ((functionT *)F->params)->currentPoint=iMstart;
        ((functionT *)F->params)->currentInt=-1;
        GSL_FN_EVAL(F,Q[iMstart]);

        //+++
        // integral1
        ((functionT *)F->params)->currentInt=1;
        double Int1=GSL_FN_EVAL(F, Q[iMstart]);
        ((functionT *)F->params)->Int1=Int1;
        // integral2
        ((functionT *)F->params)->currentInt=2;
        double Int2=GSL_FN_EVAL(F, Q[iMstart]);
        ((functionT *)F->params)->Int2=Int2;
        // integral3
        ((functionT *)F->params)->currentInt=3;
        double Int3=GSL_FN_EVAL(F, Q[iMstart]);
        ((functionT *)F->params)->Int3=Int3;
        //+++
        ((functionT *)F->params)->currentInt=0;
        GSL_FN_EVAL(F,Q[iMstart]);
        ((functionT *)F->params)->beforeIter=false;
        //_______________________________________________
        

        for (im=iMstart;im<iMfinish;im++)
        {
            ((functionT *)F->params)->currentPoint=im;

            Ii = GSL_FN_EVAL(F,Q[im]);

            Ii=(Ii- I[im])/Weight[im];
            gsl_vector_set (f, im, Ii);
        }
		
	//+++ new::  initAfterIteration 2011, 2016
	((functionT *)F->params)->afterIter=true;
	((functionT *)F->params)->currentPoint=iMfinish-1;
	GSL_FN_EVAL(F,Q[iMfinish-1]);
	((functionT *)F->params)->afterIter=false;		
	//---
    }
    
    return GSL_SUCCESS;
}

//*******************************************
//+++  function_simplyFit_derivative +++
//*******************************************
double function_simplyFit_derivative (double x, void *params)
{    
    //+++ data structure:: simplyFitDerivative :: extracting of parameters
    size_t  		indexX	= ((struct simplyFitDerivative *)params) ->indexX;
    double	Q 	= ((struct simplyFitDerivative *)params) ->Q;    
    gsl_function 	*F 	= ((struct simplyFitDerivative *)params)->function;
    gsl_vector 	*para	= ((functionT *)F->params)->para;
    int  prec= ((functionT *)F->params)->prec;
    //+++ 
    double oldX=gsl_vector_get(para,indexX);
    //+++ 
    gsl_vector_set(para,indexX,x);
    //+++
    double res=GSL_FN_EVAL(F,Q);
    //+++
    gsl_vector_set(para,indexX,oldX);
    //+++
    return res;
}

//*******************************************
//+++ function_dfm +++
//*******************************************
int function_dfm (const gsl_vector * x, void *params, gsl_matrix * J)
{
    //+++ preparation of derivative function
    gsl_function *F 		= ((struct simplyFitP *)params)->function;  
    gsl_vector *paraP	= ((functionT *)F->params)->para;    
    int  prec= ((functionT *)F->params)->prec;
    
    simplyFitDerivative  simplyFitDerivativeL = { F, 0, 0};    
    
    gsl_function FD;
    FD.function = function_simplyFit_derivative;
    FD.params = &simplyFitDerivativeL ;
     
    //+++ getting of different parameters/datasets
    simplyFitP     *simplyFit 	= (struct simplyFitP *)params;
    
    size_t 	N 		= ((struct simplyFitP *)params)->N;
    size_t 	M 		= ((struct simplyFitP *)params)->M;
    size_t 	p 		= ((struct simplyFitP *)params)->p;
    size_t 	np 		= ((struct simplyFitP *)params)->np; 
    int *controlM 		= ((struct simplyFitP *)params)->controlM;
    gsl_vector 	*para	= ((struct simplyFitP *)params)->para;
    gsl_vector_int 	*paraF	= ((struct simplyFitP *)params)->paraF;
    double *Q 		= ((struct simplyFitP *)params)->Q;
    double *Weight	= ((struct simplyFitP *)params)->Weight;
    double STEP         = ((struct simplyFitP *)params)->STEP;
    double STEPlocal=STEP;
    size_t pM=p*M;
    
    gsl_vector *limitLeft	= ((struct simplyFitP *)params)->limitLeft;
    gsl_vector *limitRight	= ((struct simplyFitP *)params)->limitRight;	

    
    //+++  npCurrent -> parameter number ; mCurrent -> related dataset numbet ; importand when M>1
    int *npCurrent=new int[np];    
    int *mCurrent=new int[np];
    
    int ii=0;
    int pFit=0;
    double xx, xxleft, xxright;
    //+++ filling of parameter vectos with current fiffable parametres
    for (int pp=0; pp<p; pp++)  for (int mm=0; mm<M; mm++) 
    {
	if (gsl_vector_int_get(paraF,ii)==0)  //+++ =0 fittable parameter
	{
	    xx=gsl_vector_get(x,pFit);
	    xxleft=gsl_vector_get(limitLeft,pFit);
	    xxright=gsl_vector_get(limitRight,pFit);
	    
	    if ( xx<xxleft ) xx=xxleft;
	    else if (xx>xxright) xx=xxright;
        
	    gsl_vector_set(para,ii,xx); //+++ update of fittable parameter
        
	    npCurrent[pFit]=pp; //+++  pp  in [0..p-1]
	    mCurrent[pFit]=mm; //+++  mm in [0..M-1]
	    pFit++;
	}
	if (gsl_vector_int_get(paraF,ii)==2)
	{
	    gsl_vector_set(para,ii,gsl_vector_get(para,M*pp)); // update global parameters
	}
	ii++; //+++ ii < pM
    }
        
    //+++
    double result, abserr;

    //+++
    size_t iMstart;
    size_t iMfinish;
    
    int current=0;
    double xxx;
    
    // +++ new 2016 : filling of the Jacobian matrix of derivatieves
    for ( ii=0;ii<pFit; ii++)
    {
    //xxx=gsl_vector_get(paraP,npCurrent[ii]);
    xxx=gsl_vector_get(x,ii);
    xxleft=gsl_vector_get(limitLeft,ii);
    xxright=gsl_vector_get(limitRight,ii);
   
    if ( xxx<xxleft ) xxx=xxleft;
    else if (xxx>xxright) xxx=xxright;
        
	current=0;
	for (int mm = 0; mm < M; mm++)
	{
        //+++
        if (mm==0)
        {
            iMstart=0;
            iMfinish=controlM[0];
        }
        else
        {
            iMfinish+=controlM[mm];
            iMstart=iMfinish-controlM[mm];
        }
        
	    // +++ move parameters to function
	    for (int pp=0;pp<p;pp++) gsl_vector_set(paraP,pp,gsl_vector_get(para,M*pp+mm));
        
	    // +++ call function before
	    ((functionT *)F->params)->beforeIter=true;
	    ((functionT *)F->params)->currentPoint=current;    
	    ((functionT *)F->params)->currentInt=-1;
        GSL_FN_EVAL(F,Q[current]);
	    
	    //+++ calculate integrals
	    // integral1
	    ((functionT *)F->params)->currentInt=1; 
	    double Int1=GSL_FN_EVAL(F, Q[current]);
	    ((functionT *)F->params)->Int1=Int1;
	    // integral2
	    ((functionT *)F->params)->currentInt=2; 
	    double Int2=GSL_FN_EVAL(F, Q[current]);
	    ((functionT *)F->params)->Int2=Int2;    
	    // integral3
	    ((functionT *)F->params)->currentInt=3; 
	    double Int3=GSL_FN_EVAL(F, Q[current]);
	    ((functionT *)F->params)->Int3=Int3;
	    
	    //+++
	    ((functionT *)F->params)->currentInt=0;
        GSL_FN_EVAL(F,Q[current]);
        ((functionT *)F->params)->beforeIter=false;
	    
	    

	    
	    for (int im=iMstart;im<iMfinish;im++)
	    {
		if (mCurrent[ii]!=mm && gsl_vector_int_get(paraF,M*npCurrent[ii]+mm)!=2)
		{
		    //+++ important if parameters do not related to current data set (mCurrent[ii]!=mm) and parameter is not GLOBAL 
		    //+++ derivatieve = 0.0
		    gsl_matrix_set (J, current, ii, 0.0);
		    current++;
		    continue;
		}	
		
		//+++ derivative function filling 
		((functionT *)F->params)->currentPoint=current;		
		((simplyFitDerivative *)FD.params)->Q=Q[current]; 	
		((simplyFitDerivative *)FD.params)->indexX=npCurrent[ii];
		
		//+++ calculation
        if (xxx==0.0) STEPlocal=STEP; else STEPlocal=STEP*fabs(xxx);

        
        if ((xxx-xxleft) < STEPlocal) gsl_deriv_forward(&FD, xxx, STEPlocal, &result, &abserr);
        else if((xxright-xxx)<STEPlocal) gsl_deriv_backward(&FD, xxx, STEPlocal, &result, &abserr);
        else gsl_deriv_central (&FD, xxx, STEPlocal, &result, &abserr);
		
        //gsl_deriv_central (&FD, xxx, STEPlocal, &result, &abserr);
            
		if (Weight[im]!=0) gsl_matrix_set (J, current, ii, result/Weight[current]);
		else gsl_matrix_set (J, current, ii, result);
		current++;
	    }

	    
	    // +++ call function before
	    ((functionT *)F->params)->afterIter=true;
	    ((functionT *)F->params)->currentPoint=current-1;    
	    GSL_FN_EVAL(F, Q[current-1]);
	    ((functionT *)F->params)->afterIter=false;
	    
	}
    }
    
    delete[] npCurrent;
    delete[] mCurrent;
    
    return GSL_SUCCESS;
}
