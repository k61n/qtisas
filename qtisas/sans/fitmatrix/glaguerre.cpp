/*************************************************************************
Computation of nodes and weights for a Gauss-Laguerre quadrature formula

The algorithm calculates the  nodes  and  weights  of  the  Gauss-Laguerre
quadrature  formula  on  domain  [0, +infinity)  with  weighting  function
W(x)=Power(x,Alpha)*Exp(-x).

Input parameters:
    n       –   a required number of nodes.
                n >= 1
    alpha   –   power of the first factor of the weighting function.
                alpha > -1

Output parameters:
    x       -   array of nodes.
                Array whose index ranges from 0 to N-1.
    w       -   array of weighting coefficients.
                Array whose index ranges from 0 to N-1.

The algorithm was designed by using information from the QUADRULE library.
*************************************************************************/
#include <gsl/gsl_vector.h>
#include <gsl/gsl_sf_gamma.h>
#include <math.h>

void GaussLaguerreQuadrature(int n, double alpha, gsl_vector *x, gsl_vector *w)
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
    for(i = 0; i < n; i++)
    {
        if( i==0 )
        {
            r = (1+alpha)*(3+0.92*alpha)/(1+2.4*n+1.8*alpha);
        }
        else
        {
            if( i==1 )
            {
                r = r+(15+6.25*alpha)/(1+0.9*alpha+2.5*n);
            }
            else
            {
                r +=( (1+2.55*(i-1)) / (1.9*(i-1)) + 1.26*(i-1)*alpha/(1+3.5*(i-1)) ) /(1+0.3*alpha)*(r-gsl_vector_get(x,(i-2)));
            }
        }
        do
        {
            p2 = 0;
            p3 = 1;
            for(j = 0; j <= n-1; j++)
            {
                p1 = p2;
                p2 = p3;
                p3 = ((-r+2*j+alpha+1)*p2-(j+alpha)*p1)/(j+1);
            }
            dp3 = (n*p3-(n+alpha)*p2)/r;
            r1 = r;
            r = r-p3/dp3;
        }
        while(fabs(r-r1)>=1E-10*(1+fabs(r))*100);
        gsl_vector_set(x,i,r);
        gsl_vector_set(w,i, -exp(gsl_sf_lngamma(alpha+n)-gsl_sf_lngamma(double(n)))/(dp3*n*p2) );
    }
}
