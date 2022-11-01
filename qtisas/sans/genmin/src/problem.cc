# include <problem.h>	
# include <stdio.h>
# include <stdlib.h>
# include <math.h>

/*	This class implements the objective function.
 *	CLASS VARIABLES
 *	======================================================================
 *	dimension:	The dimension of the objective function.
 *	has_margins:    A flag indicated if the objective function has left 
 *			and / or right bounds.
 *	lmargin:	An array holding the left bounds of the 
 *			objective function.
 *	rmargin:	An array holding the right bounds of the
 *			objective function.
 * */

/*	A constructor for the class, which sets the problem dimension to d.
 * */
Problem::Problem(int d)
{
	fevals = 0;
	gevals = 0;
	dimension =d ;
	has_margins=MARGINS_NONE;
}

/*	A simple constructor for the class.
 * */
Problem::Problem()
{
	dimension =0;
	has_margins = MARGINS_NONE;
}

/*	Set the dimension of the objective function to d.
 * */
void	Problem::setDimension(int d)
{
	dimension = d;
}

/*	Set the left bounds to x1 and the right bounds to x2.
 * */
void	Problem::setMargin(MatrixG x1,MatrixG x2)
{
	lmargin = x1;
	rmargin = x2;
	has_margins=MARGINS_LEFT | MARGINS_RIGHT;
}

/*	Set the left bounds to x1.
 * */
void	Problem::setLeftMargin(MatrixG x1)
{
	lmargin = x1;
	has_margins|= MARGINS_LEFT;
}

/*	Set the right bounds to x2.
 * */
void	Problem::setRightMargin(MatrixG x2)
{
	rmargin = x2;
	has_margins|= MARGINS_RIGHT;
}

/*	Return the dimension of the objective function.
 * */
int	Problem::getDimension() const
{
	return dimension;
}

/*	Return the left bounds of the objective function.
 * */
void	Problem::getLeftMargin(MatrixG &x1)
{
	x1 = lmargin;
}

/*	Return the right bounds of the objective function.
 * */
void	Problem::getRightMargin(MatrixG &x2)
{
	x2 = rmargin;
}

//double	Problem::funmin(MatrixG x)
//{
//	return 0.0;
//}

/*	Return a pointer to the objective function.
 * */
FUNCTION	Problem::getFunmin()
{
	return &Problem::funmin;
}

/*	Return a pointer to the gradient function.
 * */
GRADIENT	Problem::getGranal()
{
	return &Problem::granal;
}

/*	Return 1 if the point x is located inside the bounds
 *	of the objective function.
 * */
int	Problem::isPointIn(MatrixG x)
{
	if(!has_margins) return 1;
	for(int i=0;i<dimension;i++)
		if(x[i]<lmargin[i] || x[i]>rmargin[i]) return 0;
	return 1;
}

/*	Return a random sample inside the bounds of the objective function.
 * */
void	Problem::getSample(MatrixG &x)
{
	if(has_margins==(MARGINS_LEFT|MARGINS_RIGHT))
	{
		for(int i=0;i<dimension;i++) 
		{
			x[i]=lmargin[i]+(rmargin[i]-lmargin[i])* (double(rand()) / RAND_MAX);//+++   (drand48()); 
		}
	}
	else
	{
		for(int i=0;i<dimension;i++)
			x[i]=2.0*1e-4*(double(rand()) / RAND_MAX)-1.0*1e-4; //+++ drand48()
    }
}

/*	Return the margins flag of the objective function.
 * */
int	Problem::hasMargins()
{
	return has_margins;
}


/*	This method evaluates using finite differences the gradient
 *	of the objective function at point x. The gradient will
 *	be stored in array g.
 * */
void	Problem::granal(MatrixG x,MatrixG &g)
{
	++gevals;
	const double eps=1e-6;
	for(int i=0;i<g.size();i++) g[i]=0.0;
	double a,b;
	b=funmin(x); 
	for(int i=0;i<x.size();i++)
	{
		x[i]+=eps;
		a=funmin(x); 
		x[i]-=eps;
		g[i]=(a-b)/eps;
	}
}

double	Problem::funmin(MatrixG x)
{
	++fevals;
	double result;
 
	gsl_vector *xx=gsl_vector_alloc(dimension);

	for(int i=0;i<dimension;i++)
	{
		gsl_vector_set(xx,i,x[i]);
	}


    if (yn2D)
    {
#ifdef FITMATRIX
       result=function_dm2D(xx, &paraSimple2D);
#endif
    }
    else if (sansSupport) result=function_dmPoly(xx, &paraSANS);
    else result=function_dm(xx, &paraSimple);

	gsl_vector_free(xx);

	return result;
}

/*	Return a random sample inside the bounds of the objective function.
 * */
/*
void	Problem::getSample(double *x)
{
	if(has_margins==(MARGINS_LEFT|MARGINS_RIGHT))
	{
		for(int i=0;i<dimension;i++) 
		{
			x[i]=lmargin[i]+(rmargin[i]-lmargin[i])*(double(rand()) / RAND_MAX); //+++  (drand48())
		}
	}
	else
	{
		for(int i=0;i<dimension;i++)
			x[i]=2.0*1e-4*(double(rand()) / RAND_MAX)-1.0*1e-4; //+++ (drand48())
	}
}
*/
/*	The destructor of the class.
 * */
Problem::~Problem()
{
}
