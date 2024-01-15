/*************************************************************************
Computation of nodes and weights for a Gauss-Legendre quadrature formula

The  algorithm  calculates  the  nodes  and  weights of the Gauss-Legendre
quadrature formula on domain [-1, 1].

Input parameters:
    n   -   a required number of nodes.
            n>=1.

Output parameters:
    x   -   array of nodes.
            Array whose index ranges from 0 to N-1.
    w   -   array of weighting coefficients.
            Array whose index ranges from 0 to N-1.

The algorithm was designed by using information from the QUADRUPLE library.
*************************************************************************/

#include <gsl/gsl_vector.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_math.h>
#include <math.h>

void GaussLegendreQuadrature(int n, double x1, double x2, gsl_vector *x, gsl_vector *w)
{
    int i;
    int j;
    double r;
    double r1;
    double p1;
    double p2;
    double p3;
    double dp3;
	//
	double xm=0.5*(x2+x1);
	double xl=0.5*(x2-x1); 
	//
    for(i = 0; i <= (n+1)/2-1; i++)
    {
        r = cos(M_PI*(4*i+3)/(4*n+2));
        do
        {
            p2 = 0;
            p3 = 1;
            for(j = 0; j <= n-1; j++)
            {
                p1 = p2;
                p2 = p3;
                p3 = ((2*j+1)*r*p2-j*p1)/(j+1);
            }
            dp3 = n*(r*p3-p2)/(r*r-1);
            r1 = r;
            r = r-p3/dp3;
        }
        while(fabs(r-r1)>=1E-10*(1+fabs(r))*100);
        gsl_vector_set(x,i, xm-xl*r);
        gsl_vector_set(x,n-1-i, xm+xl*r);
		
		gsl_vector_set(w,i,		2*xl/((1-r*r)*dp3*dp3));
		gsl_vector_set(w,n-1-i, 2*xl/((1-r*r)*dp3*dp3));

    }
}
