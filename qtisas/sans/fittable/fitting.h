/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Data structures and fitting functions
 ******************************************************************************/

#ifndef FITTING_H
#define FITTING_H

#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_sf_bessel.h>
#include <gsl/gsl_deriv.h>

#include <math.h> 

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_cblas.h>

#include <qstring.h>	
#include <qfile.h>
#include <qtextstream.h>
#include <iostream>

#ifndef  TFUNCTION_H
#define TFUNCTION_H

//****************************************************
//+++  new data structure for compiled function             2014-04-01
//****************************************************

struct functionT
{    
    gsl_vector *para;
    gsl_vector *para_limit_left;
    gsl_vector *para_limit_right;
    gsl_vector_int *para_fit_yn;
    //+++
    double *Q;
    double *I;
    double *dI;     //+++ new: 2019 to have access to y-errors inside of the fitting function
    double *SIGMA;  //+++ changed: alwazs accesible inside of the  fitting function
    //+++
    int   *listLastPoints;   //+++ new: 2019: vector of "LastPoints"
    int currentM;           //+++ new: 2019: current M value
    //+++
    int currentFirstPoint;
    int currentLastPoint;
    int currentPoint;
    //+++ 
    bool polyYN;
    int polyFunction;
    //+++ 
    bool beforeFit;
    bool afterFit;
    bool beforeIter;
    bool afterIter;
    //+++
    double Int1;
    double Int2;
    double Int3;
    //+++
    int currentInt; 
    //+++
    int prec;
    //+++  matrix-to-table results : new > 2016
    char *tableName;
    char **tableColNames;
    int *tableColDestinations;
    gsl_matrix * mTable;
    //+++ Superposisional Function
    int currentFunction;// =0 for default function and 1..N for components
};

#endif


//*******************************************
//+++ SansData
//*******************************************
struct sansData
{
	double * Q;			// Vector of Q
	double * I;			// Vector of I
	double * Weight;			// Vector of weights
	double * sigmaReso;		// Vector of Sigma(resolution)
};

//*******************************************
//+++ Integral Control
//*******************************************
struct integralControl
{
	double 	absErr;			// Absolute stop-Criterium
	double 	relErr;			// Relative stp-Criterium
	int 	intWorkspase;		// Number of Integrations
	int 	numberSigma;		// +/- n*sigma range of integral
	int 	n_function;   		//Reso: 0-Gauss; Poly:
};

//*******************************************
//+++ Size Numbers
//*******************************************
struct sizetNumbers{
	size_t 	N;	 	    // selected from Ntotal for fitting
	size_t 	M; 		    // Number of Data-sets
	size_t 	p; 		    // Number of Parameters of Function
	size_t 	np;		    // Number of AdjustibleParameters
    double  STEP;       // Derivative: a step-size
};

//*******************************************
//+++ Fit-Data-Structure(SANS Support)
//*******************************************
struct fitDataSANSpoly
{
	sizetNumbers		*SizetNumbers; 		// N, M, p, np
	sansData 		    *SansData;		// Q, I, dO, Sigma
	int			        *controlM; 		// file control  ???
	gsl_vector 		    *para;			//Initial parameters (pxM)
	gsl_vector_int		*paraF;			//Adjustible-Fittible parameters (np)
	gsl_function		*function;		//Fittable Function F
	integralControl		*resoIntegralControl;	// integralControl: reso
	int  			    *polyNumber; 		//polydisperse parameter [0..p-1]
	integralControl 	*polyIntegralControl;	// integralControl: Poly
	gsl_vector 		    *limitLeft;			//Initial parameters
	gsl_vector 		    *limitRight;			//Initial parameters
};

//*******************************************
//+++ Poly0-structure
//*******************************************
struct poly0_SANS
{
	gsl_function 	*function;	//Fittable Function
	gsl_vector  	*para;		//Initial parameters
	int  		    polyNumber; 	//polydisperse parameter [0..p-1]
};

//*******************************************
//+++ Poly2 - structure
//*******************************************
struct poly2_SANS
{
	gsl_function 	*function;			//Fittable Function
	int  		    polyNumber; 			//polydisperse parameter [0..p-1]
	integralControl *polyIntegralControl;		// integralControl :Reso
};

//*******************************************
//+++ PolyReso0-Structure
//*******************************************
struct polyReso0_SANS
{
	poly2_SANS	 *poly2;
	double 		resoSigma;
	double 		Q0;	
};
//*******************************************
//+++ Fit-Data-Structure: resoSANS
//*******************************************
struct resoSANS
{ 
	double 		resoSigma;				// sigma of resolution function
	double 		Q0;					// Q0
	gsl_function 	*function;					// Function
	integralControl 	*resoIntegralControl;			// integralControl: Reso
};


//******************************************* 
//+++ Poly1-structure
//*******************************************
struct poly1_SANS
{
	int  		polyNumber; 	//polydisperse parameter [0..p-1]
	double 		Q; 		//Q
	gsl_function 	*function;		//Function
};

//*******************************************
//+++ PolyReso1-structure
//*******************************************
struct polyReso1_SANS
{
	poly2_SANS 	*poly2;
	double 		resoSigma;
	integralControl 	*resoIntegralControl;
};

//*******************************************
//+++ fit-data-structure simplyFitP 
//*******************************************
struct simplyFitP
{
	size_t 			N;	 		// selected from Ntotal for fitting
	size_t 			M; 			// Number of Data-sets
	size_t 			p; 			// Number of Parameters of Function
	size_t 			np;			// Number of AdjustibleParameters	
	double 			* Q;		// Vector of Q
	double 			* I;		// Vector of I
	double 			* Weight;	// Vector of weights
	int 			*controlM; 		//file control
	gsl_vector 		*para;			//Initial parameters
	gsl_vector_int 		*paraF;			//Adjustible-Fittible parameters
	gsl_function 		*function;		//Fittable Function
	gsl_vector 		*limitLeft;		//Initial parameters	
	gsl_vector 		*limitRight;		//Initial parameters
    double  STEP;       // Derivative: a step-size 
};

//*******************************************
//+++ fit-data-structure simplyFitP 
//*******************************************
struct simplyFitDerivative
{
    gsl_function *function;
    size_t indexX;
    double Q;
};

//*******************************************
//+++ fit-data-structure simplyFitP
//*******************************************
struct sasFitDerivative
{
    fitDataSANSpoly *sansPoly;
    size_t indexX;
    double Q;
    int m;
};

//*******************************************
//Polydispersity
//*******************************************
double polyIntegral(double Q, void *poly2);
double resoPolyFunction(double Q, void *polyReso0);
double resoPolyIntegral(double Q0, void *polyReso1);

double resoPolyFunctionNew(double Q0, void *polyReso1);

//*******************************************
// Resolution
//*******************************************
double ResoFunction(double Q, void *paraM);
double resoIntegral(double Q, void *paraM);


//*******************************************
//*Gauss Quadrature
//*******************************************
void GaussLaguerreQuadrature(int n, double alpha, gsl_vector *x, gsl_vector *w);
void GaussLegendreQuadrature(int n, double x1, double x2, gsl_vector *x, gsl_vector *w);

//
//double function_d (const gsl_vector * x, void *params);
//
//*******************************************
// Simple fit functions
//*******************************************
int function_f (const gsl_vector * x, void *params, gsl_vector * f);
int function_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int function_df (const gsl_vector * x, void *params, gsl_matrix * J);
int function_fm (const gsl_vector * x, void *params, gsl_vector * f);
int function_fdfm (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int function_dfm (const gsl_vector * x, void *params, gsl_matrix * J);

double function_dm (const gsl_vector * x, void *params);

double function_simplyFit_derivative (double x, void *params);
double function_sasFit_derivative (double x, void *params);

//*******************************************
// Poly-Reso fit functions
//*******************************************
double function_dmPoly (const gsl_vector * x, void *params);
int function_fmPoly (const gsl_vector * x, void *params, gsl_vector * f);
int function_fdfmPoly (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int function_fdfmPolyFast (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int function_dfmPoly (const gsl_vector * x, void *params, gsl_matrix * J);
int function_dfmPolyFast (const gsl_vector * x, void *params, gsl_matrix * J);
//double round2prec(double f, int prec);
int inversion (int n, gsl_matrix * m, gsl_matrix * inverse);

#endif
