
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


/*	The constructor of the class.
 * */
GenMin::GenMin(Problem *p)
{
	problem = p;
	bestx.resize(p->getDimension());
	besty=1e+100;
}

/*	Evaluate the local search procedure at point x.
 *	The function returns the local minimum obtained by 
 *	the local search procedure. The local search procedure
 *	used is a BFGS variant due to Powell. The maximum number
 *	of iterations is 2001.
 * */
double	GenMin::localSearch(DataG &x)
{
	MinInfo Info;
	Info.p=problem;
    Info.iters=500;//2001;
	return tolmin(x,Info);
}


/*	
 *	The proposed algorithm.
 * */
void	GenMin::Solve()
{
    QString genMinMessage;
    genMinMessage = "\tStarted | Loaded | Fitting > Iterations\n\n";
    genMinMessage+= "\t[GenMin] Genetic Algorithm\n\n";
    
    /*
     genMinMessage+="Genetic Algorithm info:\n\n";
     genMinMessage+="Genetic Algorithm code was taken from paper:\n\n";
     genMinMessage+="I.G. Tsoulos, I.E. Lagaris,\n";
     genMinMessage+="GenMin: An enhanced genetic algorithm for global optimization,\n";
     genMinMessage+="Computer Physics Communications, 178(11), 2008, pp. 843-851,\n";
     genMinMessage+="DOI: 10.1016/j.cpc.2008.01.040.\n\n";
     genMinMessage+="Authors would be appreciated if you give a citation or an acknowledgement in case of using Genetic Algorithm here.\n\n";
     */
    QProgressDialog *progress;
    
    progress= new QProgressDialog(genMinMessage+"\n\n\n",  "Abort FIT", 0, generations);
    progress->setMinimumDuration(0);
    progress->setMinimumWidth(400);
    progress->setMaximumWidth(400);
    progress->setWindowModality(Qt::WindowModal);
    progress->setLabelText("Maximal Number of Generations "+QString::number(generations)+". Progress:");
    
    progress->setValue( 1 );
    progress->setValue( 2 );


	/*	
	 *	The initialization step of the algorithm.
	 * */
    
    
    //+++ new: 2016 DoF
    int dof=problem->dof;
    int prec=problem->prec;
    
	//int genome_length=5;
    int dimension = problem->getDimension();
	RlsProgram *program = new RlsProgram(problem);
	MultiPopulation pop(genome_count,genome_size,problem->getDimension(),program);
	pop.setSelectionRate(selection_rate);
	pop.setMutationRate(mutation_rate);

	vector<int> genome;
	genome.resize(pop.getSize());
	DataG trialx;
	trialx.resize(bestx.size());
    problem->getSample(trialx);
	
    double trialy=1e+100;
	double oldbest=1e+300;
	double xx1=0.0;
	double xx2=0.0;
	double stopat=0.0;
	/*
	 *	The main loop of the algorithm.
	 * */



    
    //+++ Start +++  1
	QString plusStr;
    int iters;
	for(iters=3;iters<=generations;iters++)
	{

		/*	
		 *	This is the Genetic operations step of the algorithm.
		 * */

		pop.nextGeneration();
		double f=pop.getBestFitness();
		int newmin=0;
		double v;
        
		/*	*
		 *      This is the Local Search step of the algorithm.
		 *	    A decision is to be made about perfoming a local search.
		 * */
        
		if( fabs(f-oldbest)>1e-5 && besty!=0.0)
		{
            for(int i=0;i<dimension;i++)
            {
                //trialx[i]=round2prec(trialx[i], prec);
                
                if (trialx[i]<problem->lmargin[i]) trialx[i]=problem->lmargin[i];
                else if (trialx[i]>problem->rmargin[i]) trialx[i]=problem->rmargin[i];
            }
            
			oldbest = f;
			genome=pop.getBestGenome();
			program->getX(genome,trialx);
            
            /* *    local search procedure
			 * */
        
            if (genmin_yn) trialy=localSearch(trialx);
            if (levenberg_yn) trialy=localSearchGSL(trialx,prec);

            //trialy=round2prec(trialy,prec);
            
            
			if (trialy==0)
            {
                
                besty=trialy;
                bestx=trialx;
                break;
            }
            if ( trialy<besty && fabs(trialy-besty)>1e-7 )
			{
                for(int i=0;i<dimension;i++)
                {
                    //trialx[i]=round2prec(trialx[i], prec);
                    
                    if (trialx[i]<problem->lmargin[i]) trialx[i]=problem->lmargin[i];
                    else if (trialx[i]>problem->rmargin[i]) trialx[i]=problem->rmargin[i];
                }
                
				newmin=1;
				besty=trialy;
				bestx=trialx;
			}
		}
		/*
		 *	This is the Termination Check step.
		 * */

		/*	Calculate the variance of the best values.
		 * */
		v=fabs(besty);
		v=besty;
		xx1+=(v);
		xx2+=(v)*(v);
		double variance=xx2/iters-(xx1/iters)*(xx1/iters);

		variance = fabs(variance);
		if(newmin || stopat<1e-7)
        {
			stopat=variance/2.0;
		}
		/*	Print some output to the screen.
		 * */
        
        printf("# global:genmin    %4d[<%4d]\tchi^2\t%15.10e\t%10.5lg [<%10.5lg]",iters-2,generations,besty/dof,variance/dof,stopat/dof);
        printf("\t[");
        for(int i=0;i<GSL_MIN(dimension,8);i++) printf("%10.5lg  ",bestx[i]);
        if (dimension>8) printf("...");
        printf(" ]\n");
        
        plusStr = "#\t\t\t\t\t\t\t\t Stopping Criterion \t \t \t Chi^2/dof\n\n";
        plusStr = plusStr + QString::number(iters-2)+"[<"+QString::number(generations)+"]\t\t "+QString::number(variance/dof)+" [<"+QString::number(stopat/dof)+"]\t "+QString::number(besty/dof)+"\n\n";
        
		progress->setValue( iters );
        progress->setLabelText(genMinMessage+plusStr);
		

		/*	If the stopping rule holds, then terminate
		 * */
#ifdef Q_OS_WIN
//        if (besty/dof<1e-30) besty=0;
#endif
        
        if(besty/dof==0.0) break;
        
        if(variance>1e-8 && variance <=stopat ) break;
		if(variance<1e-10 && iters>5) break;


		if ( progress->wasCanceled() ) 
		{
		    progress->close();
		    break;
		}
        
        if (iters==10) random_seed+=1;
        if (iters==15) random_seed-=1;
        if (iters==20) random_seed+=2;
        if (iters==25) random_seed-=2;
	}
        progress->close();

    iterations=iters-2;
	delete program;
}

/*	Return the global minimum obtained by the proposed algorithm.
 *	The array x is the located global minimum and the variable y
 *	is the value of the objective function at x.
 * */
void	GenMin::getMinimum(DataG &x,double &y)
{
	x = bestx;
	y = besty;
}

/*	The destructor of the class.
 * */
GenMin::~GenMin()
{
}
