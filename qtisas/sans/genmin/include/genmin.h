/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Genetic Minimization Class
 ******************************************************************************/

#ifndef GENMIN_H
#define GENMIN_H

#include <problem.h>
#include <vector>

#include "QProgressDialog"

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
    double localSearch(DataG &x, QProgressDialog *progress);
    double localSearchGSL(DataG &x, int prec, QProgressDialog *progress);
    void Solve(QProgressDialog *progress);
		void	getMinimum(DataG &x,double &y);
        double besty;
        int    iterations;
		~GenMin();
};

# endif
