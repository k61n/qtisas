
#ifndef GENMIN_H
#define GENMIN_H

#include <vector>

#include <problem.h>

using namespace std;

/*	
 *	This class represents the main class of the 
 *	software, that implements the proposed algorithm.
 * 
*/
class GenMin
{
	private:
		/*	
		 *	problem: The objective function to be minimized.
		 *	bestx, besty: The point of the located global minimum.
		 * */
		Problem *problem;
		DataG bestx;
	public:
		/*	
		 *	GenMin(p): The constructor of the class.
		 *	localSearch(x): Start from the point x a local
		 *	                search procedure (tolmin)
		 *	Solve(): This method implements the proposed algorithm.
		 *	getMinimum(x,y): Return the located global minimum.
		 *	~GenMin(): The destructor  of the class.
		 * */
		GenMin(Problem *p);
		double	localSearch(DataG &x);
		double	localSearchGSL(DataG &x,int prec);
		void	Solve();
		void	getMinimum(DataG &x,double &y);
        double besty;
        int    iterations;
		~GenMin();
};

# endif
