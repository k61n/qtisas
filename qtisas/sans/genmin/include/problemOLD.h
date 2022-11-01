# ifndef __PROBLEM__H
# include <vector>
using namespace std;
typedef vector<double> MatrixG; // +++ QtiKWS :: MatrixG instead of Matrix
typedef vector<double> DataG;   // +++ QtiKWS :: DataG instead of Data
class Problem;
typedef double (Problem::*FUNCTION)(MatrixG);
typedef void   (Problem::*GRADIENT)(MatrixG,MatrixG &);



// +++ QtiKWS
#include "../../fitTable10/fitting.h"
#include "../../fitMatrix10/fitting-2d.h"
// ---     QtiKWS


# define MARGINS_NONE	0
# define MARGINS_LEFT	1
# define MARGINS_RIGHT	2

/*	
 *	This class represents the objective function
 *	to be minimized.
 * */
class Problem
{
	protected:
		/*	
		 *	dimension: The dimension of the objective function.
		 *	has_margins: It determines if the objective function
		 *		     has margins (bounds).
		 *      lmargin,rmargin: The margins (bounds) of the objective function.
		 * */
		int      dimension;
		int	 has_margins;
		MatrixG 	 lmargin;
		MatrixG 	 rmargin;
		
		// +++ QtiKWS
		simplyFitP paraSimple; 		// +++ fittting: no SANS support
		fitDataSANSpoly paraSANS;		// +++ fittting:  SANS support
		bool sansSupport;			// +++ fittting:  SANS support is needed?
		
		simplyFit2D paraSimple2D; 		// +++ fittting:: 2D
		bool yn2D;			// +++ fittting:2D Yes or Not
		// --- QtiKWS
		
	public:
		/*	
		 *	Problem(d),Problem(): The constructor of the class.
		 *	setDimension(d): Set the dimension of the objective function to d.
		 *	setMargin(x1,x2): Set the left and the right margin of 
		 *			the objective function.
		 *	setLeftMargin(x1): Set the left margin of the objective function.
		 *	setRightMargin(x2): Set the right margin of the objective function.
		 *	funmin(x): A virtual function for the true objective function. The
		 *		   user must override this method.
		 *       granal(x,g): A virtual function for the gradient of the objective 
		 *       	   function at the point x. The user must override this method.
		 *       getDimension(): Return the dimension of the objective function.
		 *       getLeftMargin(x1): Return the left margin of the objective function.
		 *       getRightMargin(x2): Return the right margin of the objective function.
		 *       getFunmin(): Return a pointer to the objective function.
		 *       getGranal(): Return a pointer to the gradient function.
		 *       getSample(x): Return a feasible point x.
		 *       isPointIn(x): Return 1 if the point x is feasible and 0 otherwise.
		 *       hasMargins(): Return 1 if the function has margins.
		 *	~Problem(): The destructor of the class.
		 * */
	    	int fevals,gevals;
		Problem(int d);
		Problem();
		void	setDimension(int d);
		void	setMargin(MatrixG x1,MatrixG x2);
		void	setLeftMargin(MatrixG x1);
		void	setRightMargin(MatrixG x2);
		
		// +++ QtiKWS
		
		void        sansSupportYN(bool YN) {sansSupport=YN;}
		void        setSimplyFitP(simplyFitP paraSimpleNew) {paraSimple=paraSimpleNew;};				
		void        setSANSfitP(fitDataSANSpoly paraSANSnew) {paraSANS=paraSANSnew;};	
		
		void        YN2D(bool YN) {yn2D = YN;}
		void 	setSimplyFit2D (simplyFit2D paraSimple2Dnew) { paraSimple2D= paraSimple2Dnew;};

		//--- QtiKWS
		
		
		virtual double  funmin(MatrixG x);
		virtual void   granal(MatrixG x,MatrixG &g);
		
		int		getDimension() const;
		void		getLeftMargin(MatrixG &x1);
		void		getRightMargin(MatrixG &x2);
		FUNCTION	getFunmin();
		GRADIENT	getGranal();
		void		getSample(MatrixG &x);
		void		getSample(double *x);
		int		isPointIn(MatrixG x);
		int		hasMargins();	
		

		~Problem();
		
};

/*	
 *	This struct is used by the tolmin local search procedure.
 * */
typedef struct
{
	Problem *p;
	int	fevals;
	int	gevals;
	int	iters;
}MinInfo;

extern "C"{
extern double tolmin(MatrixG &,MinInfo&);
}
# define __PROBLEM__H
# endif
