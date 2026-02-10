#include <cmath>
#include <cstdio>

#include <QProgressDialog>
#include <QString>

#include "genmin.h"
#include "get_options.h"
#include "multi_population.h"
#include "rlsprogram.h"
#include "tolmin.h"

/*	This class implements the main algorithm.
 *	CLASS VARIABLES
 *	======================================================================
 *	problem: A pointer to the objective function to be minimized.
 *	bestx:   A vector that will hold the global minimum located.
 *	besty:   The value of objective function at the point bestx.
 * */

//+++ The constructor of the class.
GenMin::GenMin(Problem *p)
{
    problem = p;
    bestx.resize(p->getDimension());
    besty = 1e+100;
}

/*	Evaluate the local search procedure at point x.
 *	The function returns the local minimum obtained by 
 *	the local search procedure. The local search procedure
 *	used is a BFGS variant due to Powell. The maximum number
 *	of iterations is 2001.
 * */
double GenMin::localSearch(DataG &x, QProgressDialog *progress)
{
    MinInfo Info;
    Info.p = problem;
    Info.iters = 500;
    return tolmin(x, Info);
}

//+++ The proposed algorithm.
void GenMin::Solve(QProgressDialog *progress)
{
    QString genMinMessage;
    /*
     genMinMessage+="Genetic Algorithm info:\n\n";
     genMinMessage+="Genetic Algorithm code was taken from paper:\n\n";
     genMinMessage+="I.G. Tsoulos, I.E. Lagaris,\n";
     genMinMessage+="GenMin: An enhanced genetic algorithm for global optimization,\n";
     genMinMessage+="Computer Physics Communications, 178(11), 2008, pp. 843-851,\n";
     genMinMessage+="DOI: 10.1016/j.cpc.2008.01.040.\n\n";
     genMinMessage+="Authors would be appreciated if you give a citation or an acknowledgement in case of using Genetic Algorithm here.\n\n";
     */

    if (progress)
    {
        genMinMessage = "\tStarted | Loaded | Fitting > Iterations\n\n";
        genMinMessage += "\t[GenMin] Genetic Algorithm\n\n";

        progress->setLabelText(genMinMessage + "\n\n\n");
        progress->setValue(1);
        progress->setValue(2);
        progress->update();        
    }

    // The initialization step of the algorithm.
    int dof = problem->dof;
    int prec = problem->prec;

    int dimension = problem->getDimension();
    auto *program = new RlsProgram(problem);
    MultiPopulation pop(genome_count,genome_size,problem->getDimension(), program);
    pop.setSelectionRate(selection_rate);
    pop.setMutationRate(mutation_rate);

    vector<int> genome;
    genome.resize(pop.getSize());
    DataG trialx;
    trialx.resize(bestx.size());
    problem->getSample(trialx);
    
    double trialy = 1e+100;
    double oldbest = 1e+300;
    double xx1 = 0.0;
    double xx2 = 0.0;
    double stopat = 0.0;

    // The main loop of the algorithm.

    QString plusStr;
    double variance = INFINITY;
    double v, f;
    int newmin, iters;

    for (iters = 3; iters <= generations; iters++)
    {  
        printf("# global: genmin   %4d[<%4d]\tchi^2\t%15.10e\t%10.5lg [<%10.5lg]", iters - 3, generations, besty / dof,
               variance / dof, stopat / dof);

        printf("\t[");
        for (int i = 0; i < GSL_MIN(dimension, 8); i++)
            printf("%10.5lg  ", bestx[i]);
        printf(dimension > 8 ? "...]\n" : "]\n");

        // This is the Genetic operations step of the algorithm.
        pop.nextGeneration();
        f = pop.getBestFitness();
        newmin = 0;

        if (progress)
        {
            if (progress->wasCanceled())
                break;
            plusStr = "#\t\t\t\t\t\t\t\t Stopping Criterion \t \t \t Chi^2/dof\n\n";
            plusStr += QString::number(iters - 3) + "[<" + QString::number(generations) + "]\t\t ";
            plusStr += QString::number(variance / dof) + " [<" + QString::number(stopat / dof) + "]\t ";
            plusStr += QString::number(besty / dof) + "\n\n";

            progress->setValue(iters);
            progress->setLabelText(genMinMessage + plusStr);
            progress->update();
        }

        // This is the Local Search step of the algorithm
        if (fabs(f - oldbest) > 1e-5 && besty != 0.0)
        {
            for (int i = 0; i < dimension; i++)
                trialx[i] = fmin(problem->rmargin[i], fmax(problem->lmargin[i], trialx[i]));

            oldbest = f;
            genome = pop.getBestGenome();
            program->getX(genome, trialx);

            // local search procedure
            if (genmin_yn)
                trialy = localSearch(trialx, progress);

            if (levenberg_yn)
                trialy = localSearchGSL(trialx, prec, progress);

            if (trialy == 0)
            {
                besty = trialy;
                bestx = trialx;
                break;
            }

            if (trialy < besty && fabs(trialy - besty) > 1e-7)
            {
                for (int i = 0; i < dimension; i++)
                    trialx[i] = fmin(problem->rmargin[i], fmax(problem->lmargin[i], trialx[i]));

                newmin = 1;
                besty = trialy;
                bestx = trialx;
            }
        }

        // This is the Termination Check step.
        // Calculate the variance of the best values.
        v = fabs(besty);
        v = besty;
        xx1 += v;
        xx2 += v * v;
        variance = xx2 / iters - (xx1 / iters) * (xx1 / iters);

        variance = fabs(variance);
        if (newmin || stopat < 1e-7)
            stopat = variance / 2.0;

        // If the stopping rule holds, then terminate
        if (besty / dof == 0.0)
            break;
        
        if (variance > 1e-8 && variance <= stopat)
            break;

        if (variance < 1e-10 && iters > 5)
            break;

        if (progress && progress->wasCanceled())
            break;
    }

    iterations = iters - 2;
	delete program;
}

/*	Return the global minimum obtained by the proposed algorithm.
 *	The array x is the located global minimum and the variable y
 *	is the value of the objective function at x.
 * */
void GenMin::getMinimum(DataG &x, double &y)
{
    x = bestx;
    y = besty;
}

// The destructor of the class.
GenMin::~GenMin()
{
}
