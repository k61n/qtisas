
#include <cmath>
#include <cstdio>

#include <QProgressDialog>

#include "genmin.h"
#include "get_options.h"

double GenMin::localSearchGSL(DataG &x,int prec)
{
    bool const_chi = levenberg_constchi;
    double STEP = levenberg_step;
    int d_max_iterations=levenberg_iters;
    double d_tolerance=levenberg_rel;
    double absError =levenberg_abs;
    
    bool test=levenberg_unscaled;
    // Levenberg: gradient, delta
    int dimension = problem->getDimension();
    int N=problem->fln.n;
    
    //+++
    gsl_vector *xx=gsl_vector_alloc(dimension);
    //for(int i=0;i<dimension;i++) gsl_vector_set(xx,i,round2prec(x[i],prec));
    for(int i=0;i<dimension;i++) gsl_vector_set(xx,i,x[i]);
    
    //+++ ok
    const gsl_multifit_fdfsolver_type *Tln = gsl_multifit_fdfsolver_lmsder;
    
    //+++ ok
    gsl_multifit_fdfsolver *sln = gsl_multifit_fdfsolver_alloc(Tln, N,dimension);
    
    //+++ ok
    int status=gsl_multifit_fdfsolver_set(sln, &problem->fln, xx);
    
    
    int iter=0;
    
    double chi2=1e199, chi2prev=1e200;
    
    
    bool iterContinue=true;
    double tmp,ssizeDelta;
    int countConstChi2=0;
    
    status = GSL_CONTINUE;
    
    do
    {
        //+++ iteration
        //status = gsl_multifit_fdfsolver_iterate (sln);
        gsl_multifit_fdfsolver_iterate (sln);
        
        if (problem->sansSupportYN())
            chi2 = function_dmPoly(sln->x, problem->fln.params);
        else chi2=function_dm(sln->x,problem->fln.params);
        
        // (deltaStop)
        //+++ Delta Stop
        tmp=0;
        ssizeDelta=0;
        for (int vvv=0;vvv<dimension;vvv++)
        {
            ssizeDelta+=gsl_vector_get(sln->dx, vvv)*gsl_vector_get(sln->dx, vvv);
            tmp+=gsl_vector_get(sln->x, vvv)*gsl_vector_get(sln->x, vvv);
        }
        ssizeDelta=sqrt(ssizeDelta);
        tmp=sqrt(tmp);
        ssizeDelta-=d_tolerance*tmp;
        ssizeDelta=fabs(ssizeDelta);
        // (deltaStop)
        
        iter++;
        
        //+++ terminal output
        //if (iter>1)
        {
            printf("# local: levenberg %4d[<%4d]\tchi^2\t%15.10e\t%10.5lg [<%10.5lg]\t[",iter-1,d_max_iterations,chi2/(N-dimension), ssizeDelta,absError);
            for(int i=0;i<GSL_MIN(dimension,8);i++) printf("%10.5lg  ",gsl_vector_get(sln->x,i));
            if (dimension>8) printf("\t...\t]\n");else printf("\t]\n");
        }
        
        //+++ fit control
        if (iter>2 && chi2==0.0) break;
        //if (iter>2 && fabs((chi2prev-chi2)/chi2)<d_tolerance) break;
        //if (iter>2 && fabs(chi2prev-chi2) < absError) break;
        
        if (const_chi)
        {
            if (chi2prev<=chi2) countConstChi2++;
            else countConstChi2=0;
        }
        else
        {
            if (iter>2 && chi2prev<chi2) break;
        }
        
        chi2prev=chi2;
        
        // (deltaStop)
        if (ssizeDelta<absError) break;
        //status = gsl_multifit_test_delta (sln->dx, sln->x, absError, d_tolerance);

        if (iter > 1 && chi2prev == 0.0)
            break;

        if (const_chi && countConstChi2 > 0)
        {
            if (countConstChi2 == 10)
                break;

            if (problem->sansSupportYN())
            {
                if (countConstChi2 == 2)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP * 10;
                if (countConstChi2 == 3)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP * 10;
                if (countConstChi2 == 4)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP;
                if (countConstChi2 == 5)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP;

                if (countConstChi2 == 6)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP * 1000;
                if (countConstChi2 == 7)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP * 1000;

                if (countConstChi2 == 8)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP;
                if (countConstChi2 == 9)
                    ((struct sizetNumbers *)((struct fitDataSANSpoly *)problem->fln.params)->SizetNumbers)->STEP = STEP;
            }
            else
            {
                if (countConstChi2 == 2)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP * 10;
                if (countConstChi2 == 3)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP * 10;

                if (countConstChi2 == 4)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP;
                if (countConstChi2 == 5)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP;

                if (countConstChi2 == 6)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP * 1000;
                if (countConstChi2 == 7)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP * 1000;

                if (countConstChi2 == 8)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP;
                if (countConstChi2 == 9)
                    ((struct simplyFitP *)problem->fln.params)->STEP = STEP;
            }

            gsl_multifit_fdfsolver_set(sln, &problem->fln, sln->x);
            status = GSL_CONTINUE;
        }
    }
    while (status == GSL_CONTINUE  && iter < d_max_iterations);
    
    //+++
    //for(int i=0;i<dimension;i++) x[i]= round2prec(gsl_vector_get(sln->x,i),prec);
    for(int i=0;i<dimension;i++) x[i]= gsl_vector_get(sln->x,i);
    //+++
    gsl_multifit_fdfsolver_free (sln);
    gsl_vector_free(xx);
    
    return chi2;
}
