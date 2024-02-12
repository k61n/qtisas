/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s fitting tools
 ******************************************************************************/

#include "fittable18.h"

inline void checkLimitsLocal(int p,int prec, gsl_vector *x, gsl_vector *xleft, gsl_vector *xright)
{
    double xx, xxleft, xxright;
    for(int i=0; i<p; i++)
    {
        xx=gsl_vector_get(x,i);
        xxleft=gsl_vector_get(xleft,i);
        xxright=gsl_vector_get(xright,i);

        if (xx<xxleft) xx=xxleft;
        else if (xx>xxright) xx=xxright;
        
        gsl_vector_set(x,i,xx);
    }
    
}

//*******************************************
//+++ fit: without SANS support
//*******************************************
bool fittable18::simplyFit()
{
    // timer
    QTime dt = QTime::currentTime();
    int prevTimeMsec = 0;

    // precision
    int prec = spinBoxSignDigits->value();

    //+++01 Limits check
    chekLimits();
    chekLimitsAndFittedParameters();

    //+++02 Algorithm
    QString algorithm = comboBoxFitMethod->currentText();

    //+++03 Progress dialog
    bool showProgress = true;
    if (algorithm.contains("[GenMin]") || setToSetProgressControl)
        showProgress = false;

    //+++03A
    QProgressDialog progress;
    int progressIter = 0;

    //+++03b
    if (showProgress)
    {
        progress.setMinimumDuration(0);
        progress.setWindowModality(Qt::WindowModal);
        progress.setRange(0, spinBoxMaxIter->text().toInt()+3);
        progress.setCancelButtonText("Abort FIT");
        progress.setMinimumWidth(400);
        progress.setMaximumWidth(600);
        progress.setLabelText("Maximal Number of Iterations: " + spinBoxMaxIter->text() + ". Progress:");
        progress.show();

        //+++ Start +++
        progressIter++;
        progress.setValue(progressIter);
    }

    printf("\nFit|Init:\t\t\t\t%6.5lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
    prevTimeMsec = dt.msecsTo(QTime::currentTime());

    //+++04
    int mm, i, ii, pp;
    int nn = 0, nnn = 0, nnnn = 0;
    size_t iter = 0;
    int status;

    //+++05
    int M = spinBoxNumberCurvesToFit->value(); //  Number of Curves
    int p = spinBoxPara->value();              //  Number of Parameters per Curve
    int pM = p * M;                            //  Total Number of Parameters

    //+++06 Number of Adjustible parameters && initial paramerers
    gsl_vector_int *paramsControl = gsl_vector_int_alloc(pM);
    gsl_vector *params = gsl_vector_alloc(pM);

    //+++06a chech Funcktion Selection: dll
    if (p == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("Select Function"));
        return false;
    }

    //+++07 DATA reading interface
    int *numberSigma;                                 // Polydispersity vector
    int N;                                            // Number of Points
    int *numberM;                                     // Number of Data sets
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf; // Data

    //+++07a Reading of data
    if (!readDataForFitAllM(N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf))
    {
        if (showProgress)
            progress.cancel();
        QMessageBox::warning(this, tr("QtiSAS"), tr("A problem with Reading Data"));
        return false;
    }

    //+++03c Progress continuation
    if (showProgress)
    {
        //+++ Data 2
        progressIter++;
        progress.setLabelText("Started | Data Loading |\n\n\n\n\n\n");
        progress.setValue(progressIter);
    }

    //+++08 Parameters && Sharing && Varying
    int np = 0;
    QStringList activeParaNames;

    tablePara->blockSignals(true);
    for (int pp = 0; pp < p; pp++)
    {
        auto *itS = (QTableWidgetItem *)tablePara->item(pp, 0);  //  Share?
        auto *itA0 = (QTableWidgetItem *)tablePara->item(pp, 1); //  Vary?

        if (itA0->checkState())
        {
            gsl_vector_int_set(paramsControl, M * pp, 0);
            activeParaNames << tablePara->verticalHeaderItem(pp)->text() + (M > 1 ? "1" : "");
            np++;
            gsl_vector_int_set(F_para_fit_yn, pp, 1);
        }
        else
        {
            gsl_vector_int_set(paramsControl, M * pp, 1);
            gsl_vector_int_set(F_para_fit_yn, pp, 0);
        }
        
        gsl_vector_set(params, M * pp, tablePara->item(pp, 2)->text().toDouble());

        for (int mm = 1; mm < M; mm++)
        {
            auto *itA = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1); //  Vary?

            if (itS->checkState())
            {
                itA->setCheckState(Qt::Unchecked);
                gsl_vector_int_set(paramsControl, M * pp + mm, 2);
                tablePara->item(pp, 3 * mm + 2)
                    ->setText(
                        QString::number(tablePara->item(pp, 2)->text().toDouble(), 'G', spinBoxSignDigits->value()));
            }
            else
            {
                if (itA->checkState())
                {
                    gsl_vector_int_set(paramsControl, M * pp + mm, 0);
                    activeParaNames << tablePara->verticalHeaderItem(pp)->text() +
                                           QString::number((mm > 0 ? mm + 1 : 1)); //   only in simpleFit
                    np++;
                }
                else
                {
                    gsl_vector_int_set(paramsControl, M * pp + mm, 1);
                }
            }
            gsl_vector_set(params, M * pp + mm, tablePara->item(pp, 3 * mm + 2)->text().toDouble());
        }
        //+++
        gsl_vector_set(F_para, pp, tablePara->item(pp, 2)->text().toDouble());
    }
    tablePara->blockSignals(false);

    //+++08a
    if (np == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("No adjustible parameters"));
        if (showProgress)
            progress.cancel();
        return false;
    }

    //+++09 Adjustible vector...
    gsl_vector *paraAdjust = gsl_vector_alloc(np);

    //+++09a Fill Adjustible vector...
    size_t pFit = 0;
    for (int i = 0; i < pM; i++)
    {
        if (gsl_vector_int_get(paramsControl, i) != 0)
            continue;

        gsl_vector_set(paraAdjust, pFit, gsl_vector_get(params, i));
        pFit++;
    }

    //+++10 Adjustible vector Limits...
    gsl_vector *limitLeft = gsl_vector_alloc(np);
    gsl_vector *limitRight = gsl_vector_alloc(np);

    pFit = 0;

    double left;
    double right;
    double value;
    QString txtVary;
    QStringList lstTmpLimits;
    
    for (int pp = 0; pp < p; pp++)
    {
        for (int mm = 0; mm < M; mm++)
        {
            auto *itA0 = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1); //  Vary?
            if (tableControl->item(pp, 0)->text().contains("-inf"))
                left = -1.0e308;
            else
                left = tableControl->item(pp, 0)->text().toDouble();
            if (tableControl->item(pp, 4)->text().contains("inf"))
                right = 1.0e308;
            else
                right = tableControl->item(pp, 4)->text().toDouble();

            txtVary = itA0->text().remove(" ");
            if (txtVary.contains("..") && txtVary != "..")
            {
                lstTmpLimits = txtVary.split("..", QString::KeepEmptyParts, Qt::CaseSensitive);
                if (lstTmpLimits.count() == 2)
                {
                    double leftNew;
                    if (lstTmpLimits[0] != "")
                        leftNew = lstTmpLimits[0].toDouble();
                    else
                        leftNew = left;

                    double rightNew;
                    if (lstTmpLimits[1] != "")
                        rightNew = lstTmpLimits[1].toDouble();
                    else
                        rightNew = right;

                    if (rightNew > leftNew)
                    {
                        if (leftNew > left && leftNew < right)
                            left = leftNew;
                        if (rightNew < right && rightNew > left)
                            right = rightNew;
                    }
                }
            }
            if (itA0->checkState())
            {
                gsl_vector_set(limitLeft, pFit, left);
                gsl_vector_set(limitRight, pFit, right);
                pFit++;
            }
            if (mm == 0)
            {
                gsl_vector_set(F_para_limit_left, pp, left);
                gsl_vector_set(F_para_limit_right, pp, right);
            }
        }
    }

    printf("Fit|Started|Data Loading:\t\t%6.5lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
    prevTimeMsec = dt.msecsTo(QTime::currentTime());

    //+++11
    bool polyYN = false;
    int polyFunction = -1;
    bool beforeFit = false;
    bool afterFit = false;
    bool beforeIter = false;
    bool afterIter = false;

    double STEP = lineEditSTEP->text().toDouble();
    if (STEP <= 10.0 / pow(10.0, prec))
        STEP = 10.0 / pow(10.0, prec);

    int currentFirstPoint = 0;
    int currentLastPoint = 0;
    int currentPoint = 0;
    //+++ Fit control parameters
    int d_max_iterations = spinBoxMaxIter->value();
    double d_tolerance = lineEditTolerance->text().toDouble();
    double absError = lineEditToleranceAbs->text().toDouble();
    bool stopConstChi2 = checkBoxConstChi2->isChecked();
    if (d_tolerance < 0)
        d_tolerance = 0;
    if (absError < 0)
        absError = 0;

    //+++12 tableName,tableColNames,tableColDestinations,mTable
    char *tableName = "";
    char **tableColNames = nullptr;
    int *tableColDestinations = nullptr;
    gsl_matrix *mTable = nullptr;
    //+++ 2019-09: new superpositional function number
    int currentFunction = spinBoxCurrentFunction->value();
    //+++
    double Int1 = 1.0;
    double Int2 = 1.0;
    double Int3 = 1.0;
    //+++
    int currentInt = 0;
    //+++
    int currentM = 0;

    functionT paraT = {F_para,
                       F_para_limit_left,
                       F_para_limit_right,
                       F_para_fit_yn,
                       Q,
                       I,
                       dI,
                       sigmaf,
                       numberM,
                       currentM,
                       currentFirstPoint,
                       currentLastPoint,
                       currentPoint,
                       polyYN,
                       polyFunction,
                       beforeFit,
                       afterFit,
                       beforeIter,
                       afterIter,
                       Int1,
                       Int2,
                       Int3,
                       currentInt,
                       prec,
                       tableName,
                       tableColNames,
                       tableColDestinations,
                       mTable,
                       currentFunction};

    //+++13 init parameters of F
    F.params = &paraT;

    //+++14 RESO (not in simple Fit)
    
    //+++15 POLY (...)
    
    //+++16 SIMPLY data
    simplyFitP paraSimple = {static_cast<size_t>(N),
                             static_cast<size_t>(M),
                             static_cast<size_t>(p),
                             static_cast<size_t>(np),
                             Q, I, weight, numberM, params, paramsControl,
                             &F, limitLeft, limitRight, STEP};
    
    //+++03d
    if (showProgress)
    {
        //+++ Ready 3
        progressIter++;
        progress.setLabelText("Started | Loaded | Fitting |\n\n\n\n\n\n");
        progress.setValue(progressIter);
    }

    //+++17 off gsl errors
    gsl_set_error_handler_off();

    //+++18
    // double chi; // only in SANS mode used
    double dof = N - np;

    //+++19 Time of Fit Run
    dt.restart();
    prevTimeMsec = dt.msecsTo(QTime::currentTime());

    //+++20 only SANS gsl_matrix *covar = gsl_matrix_alloc (np,np);

    if (algorithm.contains("[GenMin]"))
    {
        genome_count = spinBoxGenomeCount->value();
        genome_size = spinBoxGenomeSize->value();
        generations = spinBoxMaxNumberGenerations->value();
        selection_rate = lineEditSelectionRate->text().toDouble();
        mutation_rate = lineEditMutationRate->text().toDouble();
        random_seed = spinBoxRandomSeed->value();

        //+++ global levenberg para
        levenberg_abs = absError;
        levenberg_rel = d_tolerance;
        levenberg_step = STEP;
        levenberg_iters = d_max_iterations;
        levenberg_constchi = stopConstChi2;
        levenberg_unscaled = comboBoxLevenberg->currentText().contains("unscaled");
        levenberg_delta = comboBoxLevenberg->currentText().contains("delta");
        levenberg_yn = comboBoxLocalSearch->currentText().contains("levenberg");
        genmin_yn = comboBoxLocalSearch->currentText().contains("genmin");
        
        int problem_dimension;
        double *tempx, *tempg;

        srand(random_seed);
        srand(random_seed);

        problem_dimension = np;

        tempx = new double[problem_dimension];
        tempg = new double[problem_dimension];

        Problem myproblem(problem_dimension);

        myproblem.YN2D(false);
        myproblem.sansSupportYN(false);
        myproblem.setSimplyFitP(paraSimple);
        myproblem.dof = int(dof);
        myproblem.prec = prec;

        DataG L, R;
        L.resize(problem_dimension);
        R.resize(problem_dimension);

        double currentParameter, currentLeftLimit, currentRightLimit;

        for (int i = 0; i < problem_dimension; i++)
        {
            currentParameter = gsl_vector_get(paraAdjust, i);
            currentLeftLimit = gsl_vector_get(limitLeft, i);
            currentRightLimit = gsl_vector_get(limitRight, i);

            if (currentLeftLimit > -1.0E308)
                L[i] = currentLeftLimit;
            else
                L[i] = -1.0E308;

            if (currentRightLimit < 1.0E308)
                R[i] = currentRightLimit;
                
            else
                R[i] = 1.0E308;
        }
        myproblem.setLeftMargin(L);
        myproblem.setRightMargin(R);

        myproblem.fln.f = &function_fm;
        myproblem.fln.df = &function_dfm;
        myproblem.fln.fdf = &function_fdfm;
        myproblem.fln.n = N;
        myproblem.fln.p = np;
        myproblem.fln.params = &paraSimple;

        GenMin opt(&myproblem);

        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",
               (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
        prevTimeMsec = dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");

        opt.Solve();

        DataG x;
        double y;
        x.resize(problem_dimension);

        opt.getMinimum(x, y);
        if (opt.besty != 0.0 && genmin_yn)
            opt.localSearch(x);
        if (opt.besty != 0.0 && levenberg_yn)
            opt.localSearchGSL(x, prec);

        printf("X = [");
        for (int i = 0; i < problem_dimension; i++)
            printf(" %lg ", x[i]);

        printf("] \nchi^2 =%lg\n", y / dof);
        printf("FUNCTION CALLS =%6d\nGRADIENT CALLS=%6d\n\n", myproblem.fevals, myproblem.gevals);

        for (int pp = 0; pp < np; pp++)
            gsl_vector_set(paraAdjust, pp, x[pp]);

        delete[] tempx;
        delete[] tempg;
        iter = opt.iterations;
    }
    else if (algorithm.contains("Nelder-Mead Simplex"))
    {
        //+++ 00
        algorithm+=" "+comboBoxSimplex->currentText();
        //+++ 01
        gsl_multimin_function f;
        //+++ 02
        f.f = &function_dm;
        f.n = np;
        f.params = &paraSimple;
        //+++ 03
        const gsl_multimin_fminimizer_type *TT;
        //+++ 04
        if (algorithm.contains("nmsimplex2rand"))   TT=gsl_multimin_fminimizer_nmsimplex2rand;
        else if (algorithm.contains("nmsimplex2"))  TT=gsl_multimin_fminimizer_nmsimplex2;
        else                                        TT=gsl_multimin_fminimizer_nmsimplex;
        //+++ 05
        gsl_vector *ss = gsl_vector_alloc (np);
        //+++ 06 set all step sizes to 1 can be increased to converge faster
        int convRate=comboBoxConvRate->currentIndex();
        if(convRate==1) gsl_vector_set_all (ss,1.0);
        else if(convRate==0) gsl_vector_set_all (ss,10.0);
        else gsl_vector_set_all (ss,0.1);
        //+++ 07
        gsl_multimin_fminimizer *s_min = gsl_multimin_fminimizer_alloc (TT, np);
        //+++ 08
        status = gsl_multimin_fminimizer_set(s_min, &f, paraAdjust, ss);
        //+++ 09
        if (status != 0)
        {
            QString report=tr("<b> %1 </b>: GSL error -1- :: ").arg(QString::number(status));
            report+= gsl_strerror (status);
            QMessageBox::warning(this, tr("QtiSAS"),report);
        }
        //+++ 10
        iter=0;
        double size;
        QString st;
        double chi2local, chi2localOld;
        int countConstChi2=0;
        //+++ 11
        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
        prevTimeMsec=dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");
        //+++ 12
        do
        {
            //+++ 13
            if (iter>0)
            {
                //+++ 14
                chi2local=s_min->fval/dof;
                //+++ 15
                if (showProgress)
                {
                    //+++ Fit Started 1
                    progressIter++;
                    st="";
                    st="Started | Loaded | Fitting > Iterations\n\n";
                    st=st+algorithm+"\n\n";
                    st=st+" # \t\t\t\t\t\t\t Stopping Criterion \t\t\t\t\t\t\t chi^2 \n\n";
                    st=st+QString::number(iter)+"[<"+QString::number(d_max_iterations)+"] \t\t\t "+QString::number(size, 'E',prec)+" [<"+QString::number(d_tolerance,'E',prec)+"] \t\t\t "+QString::number(chi2local,'E',prec+2)+"\n\n";
                    progress.setLabelText(st);
                    progress.setValue( progressIter );
                    
                    if ( progress.wasCanceled())
                    {
                        progress.cancel();
                        break;
                    }
                }
                //+++ 16 terminal output
                st="# (simplex) %4d[<%4d] %10.5lg [<%10.5lg] chi^2 %15.10lg";
                printf(st.toLocal8Bit().constData(),iter,d_max_iterations,size,d_tolerance, chi2local);
                printf(" [ ");
                for(int i=0;i<GSL_MIN(np,10);i++) printf("%8.6lg ",gsl_vector_get(s_min->x, i));
                if (np>10) printf("...");
                printf(" ] %5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
                prevTimeMsec=dt.msecsTo(QTime::currentTime());
                //--- terminal output
            }
            //+++ 17
            iter++;
            //+++ 18
            status = gsl_multimin_fminimizer_iterate (s_min);
            checkLimitsLocal(np,prec,s_min->x, limitLeft, limitRight);
            //+++ 19
            size=gsl_multimin_fminimizer_size (s_min);
            status = gsl_multimin_test_size (size, d_tolerance);
            //+++ 20
            if (iter>1 && chi2local==0.0) break;
        }
        while (status == GSL_CONTINUE && (int)iter < d_max_iterations);
        
        for (int pp=0; pp<np;pp++) gsl_vector_set(paraAdjust, pp, gsl_vector_get(s_min->x, pp));
        
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free(s_min);
    }
    else
    {
        //+++
        algorithm+=" "+comboBoxLevenberg->currentText();
        //+++
        const gsl_multifit_fdfsolver_type *Tln;
        
        gsl_multifit_fdfsolver *sln;
        gsl_multifit_function_fdf fln;
        
        fln.f = &function_fm;
        fln.df =&function_dfm;
        fln.fdf = &function_fdfm;
        
        fln.n = N;
        fln.p = np;
        fln.params = &paraSimple;
        
        //+++
        if (algorithm.contains("unscaled"))
            Tln = gsl_multifit_fdfsolver_lmder;
        else
            Tln = gsl_multifit_fdfsolver_lmsder;
        
        bool deltaStop=false;
        if (algorithm.contains("delta")) deltaStop=true;
        
        //+++
        sln = gsl_multifit_fdfsolver_alloc(Tln, N,np);
        
        //+++
        status=gsl_multifit_fdfsolver_set(sln, &fln, paraAdjust);
        
        //+++
        if (status != 0) QMessageBox::warning(this, tr("QtiSAS"),
                                              tr("<b> %1 </b>: GSL error -3-").arg(QString::number(status)));

        //+++
        double ssizeAbs=0;
        double ssizeDelta=0;
        //+++
        QString st;
        //+++
        double tmp;
        //+++
        gsl_vector *vec=gsl_vector_alloc(np);
        //+++
        double chi2local, chi2localOld;
        //+++
        iter=0;
        int countConstChi2=0;
        
        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
        prevTimeMsec=dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");
        
        do
        {
            iter++;
            //+++
            status = gsl_multifit_fdfsolver_iterate (sln);
            checkLimitsLocal(np,prec,sln->x, limitLeft, limitRight);
            //+++
            ssizeDelta=0;
            ssizeAbs=0;
            if (deltaStop)
            {
                //+++ Delta Stop
                tmp=0;
                for (int vvv=0;vvv<np;vvv++)
                {
                    tmp=fabs(fabs(gsl_vector_get(sln->dx, vvv)) - d_tolerance*fabs(gsl_vector_get(sln->x, vvv)));
                    if (tmp>ssizeDelta) ssizeDelta=tmp;
                }
                //status = gsl_multifit_test_delta (sln->dx, sln->x, absError, d_tolerance);
                if (iter>1 && ssizeDelta < absError) status = GSL_SUCCESS;
                else status = GSL_CONTINUE;
            }
            else
            {
                gsl_multifit_gradient(sln->J, sln->f,vec);
                //+++ Abs Stop
                for (int vvv=0;vvv<np;vvv++) ssizeAbs+=fabs(gsl_vector_get(vec, vvv));
                status = gsl_multifit_test_gradient (vec, absError);
            }

            chi2localOld=chi2local;
            chi2local=pow(gsl_blas_dnrm2(sln->f),2)/dof;

            if (stopConstChi2)
            {
                if (chi2localOld<=chi2local ) countConstChi2++;
                else countConstChi2=0;
            }
            else
            {
                if (iter>1 && chi2localOld<chi2local) break;
            }
            
            
            
            if (showProgress)
            {
                //+++ Fit Started 1
                progressIter++;
                st="";
                st="Started | Loaded | Fiiting > Iterations\n\n";
                st=st+algorithm+"\n\n";
                st=st+" # \t\t\t\t\t\t\t Stopping Criterion \t\t\t\t\t\t\t chi^2 \n\n";
                st=st+QString::number(iter)+"[<"+QString::number(d_max_iterations)+"]";
                
                
                if (deltaStop)
                    st=st+" \t\t\t "+QString::number(ssizeDelta, 'E',prec)+" [<"+QString::number(absError)+"]";
                else
                    st=st+ " \t\t\t "+QString::number(ssizeAbs, 'E',prec)+" [<"+QString::number(absError)+"]";
                st=st+"\t\t\t "+QString::number(chi2local,'E',prec+2)+"\n\n";
                
                progress.setLabelText(st);
                progress.setValue( progressIter );
                if ( progress.wasCanceled() )
                {
                    progress.cancel();
                    break;
                }
            }
            
            //+++ terminal output
            st="# (levenberg) %4d[<%4d] %10.5lg [<%10.5lg] chi^2 %15.10lg";
            
            if (deltaStop) printf(st.toLocal8Bit().constData(),iter,d_max_iterations,ssizeDelta,absError, chi2local);
            else           printf(st.toLocal8Bit().constData(),iter,d_max_iterations,ssizeAbs,  absError, chi2local);
            
            printf(" [ ");
            for(int i=0;i<GSL_MIN(np,10);i++) printf("%8.6lg ",gsl_vector_get(sln->x, i));
            if (np>10) printf("...");
            if (countConstChi2>0) printf(" ]%2d [x const chi]",countConstChi2);
            else printf(" ]");
            printf(" %5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
            prevTimeMsec=dt.msecsTo(QTime::currentTime());
            //--- terminal output
            

            if (iter>1 && chi2local==0.0) break;
            
            if (stopConstChi2 && countConstChi2>0 && status == GSL_CONTINUE)
            {
                if (countConstChi2==8) break;
                
                if (countConstChi2==2) ((struct simplyFitP *)fln.params)->STEP=STEP*10;
                if (countConstChi2==3) ((struct simplyFitP *)fln.params)->STEP=STEP*10;
                
                if (countConstChi2==4) ((struct simplyFitP *)fln.params)->STEP=STEP;
                
                if (countConstChi2==5) ((struct simplyFitP *)fln.params)->STEP=STEP*1000;
                if (countConstChi2==6) ((struct simplyFitP *)fln.params)->STEP=STEP*1000;
                
                if (countConstChi2==7) ((struct simplyFitP *)fln.params)->STEP=STEP;

                gsl_multifit_fdfsolver_set(sln, &fln, sln->x);
            }
            
        }
        while ( status == GSL_CONTINUE  && (int)iter < d_max_iterations);
        
        
        
        for (int pp=0; pp<np;pp++) gsl_vector_set(paraAdjust, pp, gsl_vector_get(sln->x, pp));
        
        gsl_multifit_fdfsolver_free (sln);
        gsl_vector_free(vec);
    }
    
    checkLimitsLocal(np,prec,paraAdjust,limitLeft, limitRight);
    
    
    //+++ Chi
    double chi =sqrt(function_dm (paraAdjust, &paraSimple));
    double c = chi / sqrt(dof);
    //+++ Jacobian J
    gsl_matrix *J = gsl_matrix_alloc(N, np);
    while (J==0) printf("Fit|Finished\n");;
    printf("Fit|Finished\n");
    
    function_dfm(paraAdjust,&paraSimple,J);
    
    printf("Fit|Finished|Jacobian Matrix:\t\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());
    
    //+++ Covariant
    gsl_matrix *covar = gsl_matrix_alloc (np,np);
    gsl_multifit_covar (J, 0.0, covar);
    
    printf("Fit|Finished|Covariant Matrix:\t\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());
    //+++
#define FIT(i) gsl_vector_get(paraAdjust, i)
#define ERR(i) sqrt(gsl_matrix_get(covar,i,i))
    
    int npnp=0;
    
    //+++ Results to Interface
    if (!checkBoxScaleErrors->isChecked()) c=1.0;
    tablePara->blockSignals(true);
    for (pp=0; pp<p; pp++) for(mm=0; mm<M; mm++)
    {
        if (gsl_vector_int_get(paramsControl,M*pp+mm) == 0)
        {
            tablePara->item(pp,3*mm+2)->setText(QString::number(FIT(npnp),'G',prec));
            tablePara->item(pp,3*mm+3)->setText(QChar(177)+QString::number(c*ERR(npnp),'G',prec));
            gsl_vector_set(params,M*pp+mm,FIT(npnp));
            npnp++;
        }
        else if (gsl_vector_int_get(paramsControl,M*pp+mm) == 1)
        {
            if (mm==0 && mm+1<M && gsl_vector_int_get(paramsControl,M*pp+mm+1) == 2)
                tablePara->item(pp,3*mm+2)->setText(QString::number(gsl_vector_get(params,M*pp),'G',prec));
            tablePara->item(pp,3*mm+3)->setText("---");
        }
        else if (gsl_vector_int_get(paramsControl,M*pp+mm) == 2)
        {
            if (mm>0) tablePara->item(pp,3*mm+2)->setText(QString::number(gsl_vector_get(params,M*pp),'G',prec));//++ 2021-08-27
            tablePara->item(pp,3*mm+2)->setText(tablePara->item(pp,2)->text());
            tablePara->item(pp,3*mm+3)->setText("---");
        }
    }
    tablePara->blockSignals(false);
    
    printf("Fit|Finished|Parameter Transfer:\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());
    
    //+++ Covariant Matrix and errors
    if (checkBoxCovar->isChecked())
    {
        double chiWeight2=0;
        for(nn=0; nn<N; nn++) if (weight[nn]!=0.0) chiWeight2+=1/weight[nn]/weight[nn]; else chiWeight2++;
        chiWeight2=chiWeight2/N;
        
        
        QString info=covarMatrix(N, np, chi, chiWeight2, activeParaNames, covar, paraAdjust);
        
        makeNote(info, "fitCurve-"+comboBoxFunction->currentText()+"-statistics", "TableFit :: statistics info ");
        
        printf("Fit|Finished|Statistics Note:\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
        prevTimeMsec=dt.msecsTo(QTime::currentTime());
    }

    printf("\n");
    //+++ Delete  Variables

    if (tableColNames!=0) delete[] tableColNames;
    if (tableColDestinations!=0) delete[] tableColDestinations;
    if (mTable!=0) gsl_matrix_free(mTable);

    
    delete[] numberSigma;
    delete[] numberM;
    delete[] Q;
    delete[] I;
    delete[] dI;
    delete[] sigmaReso;
    delete[] weight;
    delete[] sigmaf;
    
    gsl_vector_free(limitLeft);
    gsl_vector_free(limitRight);
    
    gsl_vector_free(params);
    gsl_vector_int_free(paramsControl);
    gsl_vector_free(paraAdjust);
    gsl_matrix_free(covar);
    gsl_matrix_free(J);
    
    //+++Time After Fit Run
    textLabelTime->setText(QString::number(dt.msecsTo(QTime::currentTime()), 'G',3)+" ms - "+QString::number(iter)+" iteration(s)");
    
    if (showProgress) progress.cancel();
    
    return true;
}
//*******************************************
// fit: sansFit()
//*******************************************
bool  fittable18::sansFit()
{
    // timer
    QTime dt = QTime::currentTime();
    int prevTimeMsec = 0;
    // precision
    int prec = spinBoxSignDigits->value();
    //+++01 Limits
    chekLimits();
    chekLimitsAndFittedParameters();
    //+++02 Algorithm
    QString algorithm = comboBoxFitMethod->currentText();
    
    //+++03 Progress dialog
    bool showProgress = true;
    if (algorithm.contains("[GenMin]") || setToSetProgressControl)
        showProgress = false;

    //+++03A
    QProgressDialog progress;
    int progressIter = 0;

    //+++03b
    if (showProgress)
    {
        progress.setMinimumDuration(0);
        progress.setWindowModality(Qt::WindowModal);
        progress.setRange(0, spinBoxMaxIter->text().toInt() + 3);
        progress.setCancelButtonText("Abort FIT");
        progress.setMinimumWidth(400);
        progress.setMaximumWidth(600);
        progress.setLabelText("Maximal Number of Iterations: " + spinBoxMaxIter->text() + ". Progress:");
        progress.show();

        //+++ Start +++
        progressIter++;
        progress.setValue(progressIter);
    }

    printf("\nFit|Init:\t\t\t\t%6.5lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
    prevTimeMsec = dt.msecsTo(QTime::currentTime());

    //+++04
    int mm, i, pp;
    int nn = 0, nnn = 0, nnnn = 0;
    size_t iter = 0;
    int status;

    //+++05
    int M = spinBoxNumberCurvesToFit->value(); // Number of Curves
    int p = spinBoxPara->value();              // Number of Parameters per Curve
    int pM = p * M;                            // Total Number of Parameters

    //+++06 Number of Adjustible parameters && initial paramerers
    gsl_vector_int *paramsControl = gsl_vector_int_alloc(pM);
    gsl_vector *params = gsl_vector_alloc(pM);

    //+++06a check Funcktion Selection: DLL
    if (p == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("Select Function"));
        return false;
    }

    //+++07 DATA reading interface
    int *numberSigma;                                 // Polydispersity vector
    int N;                                            // Number of Points
    int *numberM;                                     // Number of Data sets
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf; // Data

    //+++07a Reading of data
    if (!readDataForFitAllM(N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf))
    {
        if (showProgress)
            progress.cancel();
        QMessageBox::warning(this, tr("QtiSAS"), tr("A problem with Reading Data"));
        return false;
    }

    //+++03c Progress continuation
    if (showProgress)
    {
        //+++ Data
        progressIter++;
        progress.setLabelText("Started | Data Loading |\n\n\n\n\n\n");
        progress.setValue( progressIter );
    }
    
    //+++08 Parameters && Sharing && Varying
    int np = 0;
    QStringList activeParaNames;
    tablePara->blockSignals(true);
    for (int pp = 0; pp < p; pp++)
    {
        auto *itS = (QTableWidgetItem *)tablePara->item(pp, 0);  // Share?
        auto *itA0 = (QTableWidgetItem *)tablePara->item(pp, 1); // Vary?

        if (itA0->checkState())
        {
            gsl_vector_int_set(paramsControl, M * pp, 0);
            activeParaNames << tablePara->verticalHeaderItem(pp)->text() + (M > 1 ? "1" : "");
            np++;
            gsl_vector_int_set(F_para_fit_yn, pp, 1);
        }
        else
        {
            gsl_vector_int_set(paramsControl, M * pp, 1);
            gsl_vector_int_set(F_para_fit_yn, pp, 0);
        }

        gsl_vector_set(params, M * pp, tablePara->item(pp, 2)->text().toDouble());

        for (int mm = 1; mm < M; mm++)
        {
            auto *itA = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1); // Vary?

            if (itS->checkState())
            {
                itA->setCheckState(Qt::Unchecked);
                gsl_vector_int_set(paramsControl, M * pp + mm, 2);
                tablePara->item(pp, 3 * mm + 2)->setText(tablePara->item(pp, 2)->text());
            }
            else
            {
                if (itA->checkState())
                {
                    gsl_vector_int_set(paramsControl, M * pp + mm, 0);
                    activeParaNames << tablePara->verticalHeaderItem(pp)->text() +
                                           QString::number((mm > 0 ? mm + 1 : 1));
                    np++;
                }
                else
                    gsl_vector_int_set(paramsControl, M * pp + mm, 1);
            }
            gsl_vector_set(params, M * pp + mm, tablePara->item(pp, 3 * mm + 2)->text().toDouble());
        }

        gsl_vector_set(F_para, pp, tablePara->item(pp, 2)->text().toDouble());
    }
    tablePara->blockSignals(false);

    //+++8a
    if (np == 0)
    {
        QMessageBox::warning(this, tr("QtiSAS"), tr("No adjustible parameters"));
        if (showProgress)
            progress.cancel();
        return false;
    }

    //+++09 Adjustible vector...
    gsl_vector *paraAdjust = gsl_vector_alloc(np);

    //+++09a Fill Adjustible vector...
    size_t pFit = 0;
    for (int i = 0; i < pM; i++)
    {
        if (gsl_vector_int_get(paramsControl, i) != 0)
            continue;
        gsl_vector_set(paraAdjust, pFit, gsl_vector_get(params, i));
        pFit++;
    }

    //+++10 Adjustible vector Limits...
    gsl_vector *limitLeft = gsl_vector_alloc(np);
    gsl_vector *limitRight = gsl_vector_alloc(np);

    double left;
    double right;
    double value;
    QString txtVary;
    QStringList lstTmpLimits;
    pFit = 0;

    for (int pp = 0; pp < p; pp++)
    {
        for (int mm = 0; mm < M; mm++)
        {
            auto *itA0 = (QTableWidgetItem *)tablePara->item(pp, 3 * mm + 1); // Vary?
            if (tableControl->item(pp, 0)->text().contains("-inf"))
                left = -1.0e308;
            else
                left = tableControl->item(pp, 0)->text().toDouble();
            if (tableControl->item(pp, 4)->text().contains("inf"))
                right = 1.0e308;
            else
                right = tableControl->item(pp, 4)->text().toDouble();

            txtVary = itA0->text().remove(" ");

            if (txtVary.contains("..") && txtVary != "..")
            {
                lstTmpLimits = txtVary.split("..", QString::KeepEmptyParts, Qt::CaseSensitive);
                if (lstTmpLimits.count() == 2)
                {
                    double leftNew;
                    if (lstTmpLimits[0] != "")
                        leftNew = lstTmpLimits[0].toDouble();
                    else
                        leftNew = left;
                    double rightNew;
                    if (lstTmpLimits[1] != "")
                        rightNew = lstTmpLimits[1].toDouble();
                    else
                        rightNew = right;
                    if (rightNew > leftNew)
                    {
                        if (leftNew > left && leftNew < right)
                            left = leftNew;
                        if (rightNew < right && rightNew > left)
                            right = rightNew;
                    }
                }
            }
            
            if (itA0->checkState())
            {
                gsl_vector_set(limitLeft, pFit, left);
                gsl_vector_set(limitRight, pFit, right);
                pFit++;
            }
            if (mm == 0)
            {
                gsl_vector_set(F_para_limit_left, pp, left);
                gsl_vector_set(F_para_limit_right, pp, right);
            }
        }
    }

    printf("Fit|Started|Data Loading:\t\t%6.5lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
    prevTimeMsec = dt.msecsTo(QTime::currentTime());

    bool polyYN = false;
    int polyFunction = comboBoxPolyFunction->currentIndex();

    bool beforeFit = false;
    bool afterFit = false;
    bool beforeIter = false;
    bool afterIter = false;

    double STEP = lineEditSTEP->text().toDouble();
    if (STEP <= 10.0 / pow(10.0, prec))
        STEP = 10.0 / pow(10.0, prec);

    int currentFirstPoint = 0;
    int currentLastPoint = 0;
    int currentPoint = 0;

    int d_max_iterations = spinBoxMaxIter->value();
    double d_tolerance = lineEditTolerance->text().toDouble();
    double absError = lineEditToleranceAbs->text().toDouble();
    bool stopConstChi2 = checkBoxConstChi2->isChecked();

    if (d_tolerance < 0)
        d_tolerance = 0;
    if (absError < 0)
        absError = 0;

    //+++12 tableName,tableColNames,tableColDestinations,mTable
    char *tableName = "";
    char **tableColNames = nullptr;
    int *tableColDestinations = nullptr;
    gsl_matrix *mTable = nullptr;
    //+++ 2019-09: new superpositional function number
    int currentFunction = spinBoxCurrentFunction->value();

    //+++14 RESO (not in simple Fit)
    double absErrReso = lineEditAbsErr->text().toDouble();
    double relErrReso = lineEditRelErr->text().toDouble();
    int intWorkspaseReso = spinBoxIntWorkspase->value();
    int numberSigmaReso = spinBoxIntLimits->value();
    int func_reso = comboBoxResoFunction->currentIndex();
    QString currentInstrument = comboBoxInstrument->currentText();
    if (currentInstrument.contains("Back"))
        func_reso += 10;
    //+++15 POLY (...)
    double absErrPoly = lineEditAbsErrPoly->text().toDouble();
    double relErrPoly = lineEditRelErrPoly->text().toDouble();
    int intWorkspasePoly = spinBoxIntWorkspasePoly->value();
    int numberSigmaPoly = spinBoxIntLimitsPoly->value();
    int func_poly = comboBoxPolyFunction->currentIndex();

    double Int1 = 1.0;
    double Int2 = 1.0;
    double Int3 = 1.0;
    int currentInt = 0;
    int currentM = 0;

    functionT paraT = {F_para,
                       F_para_limit_left,
                       F_para_limit_right,
                       F_para_fit_yn,
                       Q,
                       I,
                       dI,
                       sigmaf,
                       numberM,
                       currentM,
                       currentFirstPoint,
                       currentLastPoint,
                       currentPoint,
                       polyYN,
                       polyFunction,
                       beforeFit,
                       afterFit,
                       beforeIter,
                       afterIter,
                       Int1,
                       Int2,
                       Int3,
                       currentInt,
                       prec,
                       tableName,
                       tableColNames,
                       tableColDestinations,
                       mTable,
                       currentFunction};

    //+++13
    F.params = &paraT;
    
    //+++16 SANS data
    sizetNumbers sizetNumbers={static_cast<size_t>(N),
                               static_cast<size_t>(M),
                               static_cast<size_t>(p),
                               static_cast<size_t>(np),
                               STEP};
    sansData sansData = {Q, I, weight, sigmaReso};
    integralControl resoIntegralControl = {absErrReso, relErrReso, intWorkspaseReso, numberSigmaReso, func_reso};
    integralControl polyIntegralControl = {absErrPoly, relErrPoly, intWorkspasePoly, numberSigmaPoly, func_poly};

    fitDataSANSpoly para2 = {
        &sizetNumbers,        &sansData, numberM,   params, paramsControl, &F, &resoIntegralControl, numberSigma,
        &polyIntegralControl, limitLeft, limitRight};

    //+++03d
    if (showProgress)
    {
        //+++ Ready 3
        progressIter++;
        progress.setLabelText("Started | Loaded | Fitting |\n\n\n\n\n\n");
        progress.setValue(progressIter);
    }

    //+++17 off gsl errors
    gsl_set_error_handler_off();

    //+++18
    double dof = N - np;

    //+++19 Time of Fit Run
    dt.restart();
    prevTimeMsec = dt.msecsTo(QTime::currentTime());
    //+++20 only SANS
    //gsl_matrix *covar = gsl_matrix_alloc (np,np);
    
    if (algorithm.contains("[GenMin]"))
    {
        genome_count = spinBoxGenomeCount->value();
        genome_size = spinBoxGenomeSize->value();
        generations = spinBoxMaxNumberGenerations->value();
        selection_rate = lineEditSelectionRate->text().toDouble();
        mutation_rate = lineEditMutationRate->text().toDouble();
        random_seed = spinBoxRandomSeed->value();

        //+++ global levenberg para
        levenberg_abs = absError;
        levenberg_rel = d_tolerance;
        levenberg_step = STEP;
        levenberg_iters = d_max_iterations;
        levenberg_constchi = stopConstChi2;
        levenberg_unscaled = comboBoxLevenberg->currentText().contains("unscaled");
        levenberg_delta = comboBoxLevenberg->currentText().contains("delta");
        levenberg_yn = comboBoxLocalSearch->currentText().contains("levenberg");
        genmin_yn = comboBoxLocalSearch->currentText().contains("genmin");

        int problem_dimension;
        double *tempx, *tempg;

        srand(random_seed);
        srand(random_seed);

        problem_dimension = np;

        tempx = new double[problem_dimension];
        tempg = new double[problem_dimension];

        Problem myproblem(problem_dimension);

        myproblem.YN2D(false);
        myproblem.sansSupportYN(true);
        myproblem.setSANSfitP(para2);
        myproblem.dof = int(dof);
        myproblem.prec = prec;

        DataG L, R;
        L.resize(problem_dimension);
        R.resize(problem_dimension);

        double currentParameter, currentLeftLimit, currentRightLimit;

        for (int i = 0; i < problem_dimension; i++)
        {
            currentParameter = gsl_vector_get(paraAdjust, i);
            currentLeftLimit = gsl_vector_get(limitLeft, i);
            currentRightLimit = gsl_vector_get(limitRight, i);

            if (currentLeftLimit > -1.0E308)
                L[i] = currentLeftLimit;
            else
                L[i] = -1.0E308;

            if (currentRightLimit < 1.0E308)
                R[i] = currentRightLimit;
            else
                R[i] = 1.0E308;
        }

        myproblem.setLeftMargin(L);
        myproblem.setRightMargin(R);

        myproblem.fln.f = &function_fmPoly;
        myproblem.fln.df = &function_dfmPoly;
        myproblem.fln.fdf = &function_fdfmPoly;
        myproblem.fln.n = N;
        myproblem.fln.p = np;
        myproblem.fln.params = &para2;

        GenMin opt(&myproblem);

        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",
               (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
        prevTimeMsec = dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");

        opt.Solve();
        
        DataG x;
        double y;
        x.resize(problem_dimension);

        opt.getMinimum(x, y);

        if (opt.besty != 0.0 && genmin_yn)
            opt.localSearch(x);
        if (opt.besty != 0.0 && levenberg_yn)
            opt.localSearchGSL(x, prec);

        printf("X = [");
        for (int i = 0; i < problem_dimension; i++)
            printf(" %lg ", x[i]);

        printf("] \nchi^2 =%lg\n", y / dof);
        printf("FUNCTION CALLS =%6d\nGRADIENT CALLS=%6d\n\n", myproblem.fevals, myproblem.gevals);

        for (int pp = 0; pp < np; pp++)
            gsl_vector_set(paraAdjust, pp, x[pp]);

        delete[] tempx;
        delete[] tempg;
        iter = opt.iterations;
    }
    else if (algorithm.contains("Nelder-Mead Simplex"))
    {
        //+++ 00
        algorithm += " " + comboBoxSimplex->currentText();
        //+++ 01
        gsl_multimin_function f;
        //+++ 02
        f.f = &function_dmPoly;
        f.n = np;
        f.params = &para2;
        //+++ 03
        const gsl_multimin_fminimizer_type *TT;
        //+++ 04
        if (algorithm.contains("nmsimplex2rand"))
            TT = gsl_multimin_fminimizer_nmsimplex2rand;
        else if (algorithm.contains("nmsimplex2"))
            TT = gsl_multimin_fminimizer_nmsimplex2;
        else
            TT = gsl_multimin_fminimizer_nmsimplex;
        //+++ 05
        gsl_vector *ss = gsl_vector_alloc (np);
        //+++ 06 set all step sizes to 1 can be increased to converge faster
        int convRate = comboBoxConvRate->currentIndex();
        if (convRate == 1)
            gsl_vector_set_all(ss, 1.0);
        else if (convRate == 0)
            gsl_vector_set_all(ss, 10.0);
        else
            gsl_vector_set_all(ss, 0.1);
        //+++ 07
        gsl_multimin_fminimizer *s_min = gsl_multimin_fminimizer_alloc (TT, np);
        //+++ 08
        status = gsl_multimin_fminimizer_set(s_min, &f, paraAdjust, ss);
        //+++ 09
        if (status != 0)
        {
            QString report = tr("<b> %1 </b>: GSL error -1- :: ").arg(QString::number(status));
            report += gsl_strerror(status);
            QMessageBox::warning(this, tr("QtiSAS"), report);
        }
        //+++ 10
        iter = 0;
        double size;
        QString st;
        double chi2local;
        //+++ 11
        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",
               (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
        prevTimeMsec=dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");
        //+++ 12
        do
        {
            //+++ 13
            if (iter > 0)
            {
                //+++ 14
                chi2local = s_min->fval / dof;
                //+++ 15
                if (showProgress)
                {
                    //+++ Fit Started 1
                    progressIter++;

                    st = "";
                    st = "Started | Loaded | Fitting > Iterations\n\n";
                    st = st + algorithm + "\n\n";
                    st = st + " # \t\t\t\t\t\t\t Stopping Criterion \t\t\t\t\t\t\t chi^2 \n\n";
                    st = st + QString::number(iter) + "[<" + QString::number(d_max_iterations) + "] \t\t\t " +
                         QString::number(size, 'E', prec) + " [<" + QString::number(d_tolerance, 'E', prec) +
                         "] \t\t\t " + QString::number(chi2local, 'E', prec + 2) + "\n\n";
                    progress.setLabelText(st);
                    progress.setValue( progressIter );

                    if ( progress.wasCanceled())
                    {
                        progress.cancel();
                        break;
                    }
                }
                //+++ 16 terminal output
                st = "# (simplex) %4d[<%4d] %10.5lg [<%10.5lg] chi^2 %15.10lg";
                printf(st.toLocal8Bit().constData(), iter, d_max_iterations, size, d_tolerance, chi2local);
                printf(" [ ");
                for (int i = 0; i < GSL_MIN(np, 10); i++)
                    printf("%8.6lg ", gsl_vector_get(s_min->x, i));
                if (np > 10)
                    printf("...");
                printf(" ] %5.4lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
                prevTimeMsec = dt.msecsTo(QTime::currentTime());
                //--- terminal output
            }
            //+++ 17
            iter++;
            //+++ 18
            status = gsl_multimin_fminimizer_iterate(s_min);
            checkLimitsLocal(np, prec, s_min->x, limitLeft, limitRight);
            //+++ 19
            size = gsl_multimin_fminimizer_size(s_min);
            status = gsl_multimin_test_size(size, d_tolerance);
            //+++ 20
            if (iter > 1 && chi2local == 0.0)
                break;
        }
        while (status == GSL_CONTINUE && (int)iter < d_max_iterations);

        for (int pp = 0; pp < np; pp++)
            gsl_vector_set(paraAdjust, pp, gsl_vector_get(s_min->x, pp));

        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free(s_min);
    }
    else
    {
        algorithm += " " + comboBoxLevenberg->currentText();

        //+++
        const gsl_multifit_fdfsolver_type *Tln;
        gsl_multifit_fdfsolver *sln;
        gsl_multifit_function_fdf fln;

        fln.f = &function_fmPoly;

        fln.df = &function_dfmPoly;
        fln.fdf = &function_fdfmPoly;

        fln.n = N;
        fln.p = np;
        fln.params = &para2;

        //+++
        if (algorithm.contains("unscaled"))
            Tln = gsl_multifit_fdfsolver_lmder;
        else
           Tln = gsl_multifit_fdfsolver_lmsder;

        bool deltaStop = false;
        if (algorithm.contains("delta"))
            deltaStop = true;

        //+++
        sln = gsl_multifit_fdfsolver_alloc(Tln, N, np);

        status = gsl_multifit_fdfsolver_set(sln, &fln, paraAdjust);

        //+++
        if (status != 0)
            QMessageBox::warning(this, tr("QtiSAS"), tr("<b> %1 </b>: GSL error -3-").arg(QString::number(status)));

        //+++
        double ssizeAbs = 0;
        double ssizeDelta = 0;
        //+++
        QString st;
        //+++
        double tmp;
        //+++
        gsl_vector *vec = gsl_vector_alloc(np);
        //+++
        double chi2local, chi2localOld;
        //+++
        iter = 0;

        printf("Fit|Started|Loaded|Fit Preparation:\t%6.5lgsec\n",
               (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
        prevTimeMsec = dt.msecsTo(QTime::currentTime());
        printf("Fit|Started|Loaded|Prepared|Iterations:\n");

        int countConstChi2 = 0;

        if (((struct simplyFitP *)fln.params)->STEP != STEP)
        {
            ((struct simplyFitP *)fln.params)->STEP = STEP;
            gsl_multifit_fdfsolver_set(sln, &fln, sln->x);
        }

        do
        {
            iter++;
            //+++
            status = gsl_multifit_fdfsolver_iterate (sln);

            if (status > 1)
            {
                qDebug(gsl_strerror(status));
                qDebug() << QString::number(countConstChi2);
                //break;
            }

            checkLimitsLocal(np, prec, sln->x, limitLeft, limitRight);
            //+++
            ssizeDelta = 0;
            ssizeAbs = 0;
            if (deltaStop)
            {
                //+++ Delta Stop
                tmp = 0;
                for (int vvv = 0; vvv < np; vvv++)
                {
                    tmp = fabs(fabs(gsl_vector_get(sln->dx, vvv)) - d_tolerance * fabs(gsl_vector_get(sln->x, vvv)));
                    if (tmp > ssizeDelta)
                        ssizeDelta = tmp;
                }
                //status = gsl_multifit_test_delta (sln->dx, sln->x, absError, d_tolerance);
                if (iter > 1 && ssizeDelta < absError)
                    status = GSL_SUCCESS;
                else
                    status = GSL_CONTINUE;
            }
            else
            {
                gsl_multifit_gradient(sln->J, sln->f, vec);
                //+++ Abs Stop
                for (int vvv = 0; vvv < np; vvv++)
                    ssizeAbs += fabs(gsl_vector_get(vec, vvv));
                status = gsl_multifit_test_gradient (vec, absError);
            }
            chi2localOld = chi2local;
            chi2local = pow(gsl_blas_dnrm2(sln->f), 2) / dof;

            if (stopConstChi2)
            {
                if (chi2localOld <= chi2local)
                    countConstChi2++;
                else
                    countConstChi2 = 0;
                qDebug() << "chi2localOld: " + QString::number(chi2localOld, 'f', 20) +
                                " chi2local: " + QString::number(chi2local, 'f', 20);
            }
            else if (iter > 1 && chi2localOld < chi2local)
                break;

            if (showProgress)
            {
                //+++ Fit Started 1
                progressIter++;
                st = "";
                st = "Started | Loaded | Fiiting > Iterations\n\n";
                st = st + algorithm + "\n\n";
                st = st + " # \t\t\t\t\t\t\t Stopping Criterion \t\t\t\t\t\t\t chi^2 \n\n";
                st = st + QString::number(iter) + "[<" + QString::number(d_max_iterations) + "]";

                if (deltaStop)
                    st = st + " \t\t\t " + QString::number(ssizeDelta, 'E', prec) + " [<" + QString::number(absError) +
                         "]";
                else
                    st = st + " \t\t\t " + QString::number(ssizeAbs, 'E', prec) + " [<" + QString::number(absError) +
                         "]";

                st = st + "\t\t\t " + QString::number(chi2local, 'E', prec + 2) + "\n\n";

                progress.setLabelText(st);
                progress.setValue(progressIter);

                if (progress.wasCanceled())
                {
                    progress.cancel();
                    break;
                }
            }
            //+++ terminal output
            st = "# (levenberg) %4d[<%4d] %10.5lg [<%10.5lg] chi^2 %15.10lg";

            if (deltaStop)
                printf(st.toLocal8Bit().constData(), iter, d_max_iterations, ssizeDelta, absError, chi2local);
            else
                printf(st.toLocal8Bit().constData(), iter, d_max_iterations, ssizeAbs, absError, chi2local);

            printf(" [ ");
            for (int i = 0; i < GSL_MIN(np, 10); i++)
                printf("%8.6lg ", gsl_vector_get(sln->x, i));
            if (np > 10)
                printf("...");
            if (countConstChi2 > 0)
                printf(" ]%2d [x const chi]", countConstChi2);
            else
                printf(" ]");
            printf(" %5.4lgsec\n", (dt.msecsTo(QTime::currentTime()) - prevTimeMsec) / 1000.0);
            prevTimeMsec = dt.msecsTo(QTime::currentTime());
            //--- terminal output

            if (iter > 1 && chi2local == 0.0)
                break;

            if (stopConstChi2 && countConstChi2 > 0)
            {

                if (countConstChi2 == 10)
                    break;

                if (countConstChi2 == 2)
                    ((struct simplyFitP *)fln.params)->STEP = STEP * 10;
                if (countConstChi2 == 3)
                    ((struct simplyFitP *)fln.params)->STEP = STEP * 10;

                if (countConstChi2==4) ((struct simplyFitP *)fln.params)->STEP=STEP;
                if (countConstChi2==5) ((struct simplyFitP *)fln.params)->STEP=STEP;
                
                if (countConstChi2==6) ((struct simplyFitP *)fln.params)->STEP=STEP*1000;
                if (countConstChi2==7) ((struct simplyFitP *)fln.params)->STEP=STEP*1000;
                
                if (countConstChi2==8) ((struct simplyFitP *)fln.params)->STEP=STEP;
                if (countConstChi2==9) ((struct simplyFitP *)fln.params)->STEP=STEP;


                status = GSL_CONTINUE;
                gsl_multifit_fdfsolver_set(sln, &fln, sln->x);
            }
        }
        while (status == GSL_CONTINUE && (int)iter < d_max_iterations);
        
        for (int pp=0; pp<np;pp++) gsl_vector_set(paraAdjust, pp, gsl_vector_get(sln->x, pp));

        gsl_multifit_fdfsolver_free (sln);
        gsl_vector_free(vec);
    }

    checkLimitsLocal(np,prec,paraAdjust, limitLeft, limitRight);
    //+++ Chi
    double chi =sqrt(function_dmPoly (paraAdjust, &para2));
    double c = chi / sqrt(dof);

    //+++ Jacobian J
    gsl_matrix *J = gsl_matrix_alloc(N, np);
    while (J==0) J = gsl_matrix_alloc(N, np);
    printf("Fit|Finished\n");
    
    function_dfmPoly(paraAdjust,&para2,J);

    printf("Fit|Finished|Jacobian Matrix:\t\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());
    
    //+++ Covariant
    gsl_matrix *covar = gsl_matrix_alloc (np,np);
    gsl_multifit_covar (J, 0.0, covar);
    //+++

    printf("Fit|Finished|Covariant Matrix:\t\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());
    
#define FIT(i) gsl_vector_get(paraAdjust, i)
#define ERR(i) sqrt(gsl_matrix_get(covar,i,i))
    
    int npnp=0;
    
    //+++ Results to Interface
    if (!checkBoxScaleErrors->isChecked()) c=1.0;

    tablePara->blockSignals(true);
    for (pp=0; pp<p; pp++) for(mm=0; mm<M; mm++)
    {
        if (gsl_vector_int_get(paramsControl,M*pp+mm) == 0)
        {
            tablePara->item(pp,3*mm+2)->setText(QString::number(FIT(npnp),'G',prec));
            tablePara->item(pp,3*mm+3)->setText(QChar(177)+QString::number(c*ERR(npnp),'G',prec));
            gsl_vector_set(params,M*pp+mm,FIT(npnp));
            npnp++;
        }
        else if (gsl_vector_int_get(paramsControl,M*pp+mm) == 1)
        {
            if (mm==0 && mm+1<M && gsl_vector_int_get(paramsControl,M*pp+mm+1) == 2)
                tablePara->item(pp,3*mm+2)->setText(QString::number(gsl_vector_get(params,M*pp),'G',prec));
            tablePara->item(pp,3*mm+3)->setText("---");
        }
        else if (gsl_vector_int_get(paramsControl,M*pp+mm) == 2)
        {
            if (mm>0) tablePara->item(pp,3*mm+2)->setText(QString::number(gsl_vector_get(params,M*pp),'G',prec));//++ 2021-08-27
            tablePara->item(pp,3*mm+2)->setText(tablePara->item(pp,2)->text());
            tablePara->item(pp,3*mm+3)->setText("---");
        }
    }
    tablePara->blockSignals(false);
    printf("Fit|Finished|Parameter Transfer:\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
    prevTimeMsec=dt.msecsTo(QTime::currentTime());

    //+++ Covariant Matrix and errors
    if (checkBoxCovar->isChecked())
    {
        double chiWeight2=0;
        for(nn=0; nn<N; nn++) if (weight[nn]!=0.0) chiWeight2+=1/weight[nn]/weight[nn]; else chiWeight2++;
        chiWeight2=chiWeight2/N;
        
        QString info=covarMatrix(N, np, chi, chiWeight2, activeParaNames, covar, paraAdjust);
        
        makeNote(info, "fitCurve-"+comboBoxFunction->currentText()+"-statistics", "TableFit :: statistics info ");
        
        printf("Fit|Finished|Statistics Note:\t%5.4lgsec\n",(dt.msecsTo(QTime::currentTime())-prevTimeMsec)/1000.0);
        prevTimeMsec=dt.msecsTo(QTime::currentTime());
    }
    printf("\n");
    //+++ Delete  Variables
     if (tableColNames!=0) delete[] tableColNames;
     if (tableColDestinations!=0) delete[] tableColDestinations;
     if (mTable!=0) gsl_matrix_free(mTable);


    delete[] numberSigma;
    delete[] numberM;
    delete[] Q;
    delete[] I;
    delete[] dI;
    delete[] sigmaReso;
    delete[] weight;
    delete[] sigmaf;

    gsl_vector_free(limitLeft);
    gsl_vector_free(limitRight);

    gsl_vector_free(params);
    gsl_vector_int_free(paramsControl);
    gsl_vector_free(paraAdjust);
    gsl_matrix_free(covar);
    gsl_matrix_free(J);
    
    //+++Time After Fit Run
    textLabelTime->setText(QString::number(dt.msecsTo(QTime::currentTime()), 'G',3)+" ms - "+QString::number(iter)+" iteration(s)");
    
    if (showProgress) progress.cancel();

    return true;
}

//*******************************************
//+++ chi2()
//*******************************************
bool fittable18::chi2()
{
    int prec=spinBoxSignDigits->value();
    
    //+++
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();
    int pM=p*M;
    
    //+++
    if (p==0)
    {
        QMessageBox::warning(this,tr("QtiKWS"),
                             tr("Select Function"));
        return false;
    }
    
    int mm;
    
    
    //+++ number of adjustable parameters
    int np=0;
    for (int pp=0; pp<p;pp++)
    {
        QTableWidgetItem *itS = (QTableWidgetItem *)tablePara->item(pp,0); // Share?
        QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,1); // Vary?
        
        if (itA0->checkState()) np++;
        
        for (mm=1; mm<M;mm++)
        {
            QTableWidgetItem *itA = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            if (itS->checkState()==Qt::Unchecked && itA->checkState()) np++;
        }
    }
    
    //+++
    if (np==0)
    {
        QMessageBox::warning(this,tr("QtiKWS"),
                             tr("No adjustible parameters (to calculate chi2)"));
        return false;
    }
    
    
    int N=0;
    double chi2=0;
    double chi2weight=0;
    double sum=0;
    double sumWeight=0;
    double R2=1;
    
    int Nres=0;
    double  sumChi2, sumChi2Norm, sumR2;
    
    
    for (mm=0; mm<M; mm++)
    {
        datasetChangedSim(mm);
        if ( !chi2m(mm, Nres, sumChi2, sumChi2Norm,sumR2) ) return false;
        N+=Nres;
        chi2+=sumChi2;
        chi2weight+=sumChi2Norm;
        R2*=sumR2;
    }
    
    //    chi2=chi2/ (N-np)/chi2weight*N;
    chi2=chi2/ (N-np);
    
    textLabelChi->setText(QString::number(chi2,'E',prec+2));
    
    //+++R2
    //    R2=pow(fabs(R2),1/M);
    
    
    textLabelR2->setText(QString::number(R2,'E',prec+2));
    //+++ Delete
    
    return true;
}

//*******************************************
//+++ plot fit result: with SANS support
//*******************************************
bool fittable18::chi2m(int m, int &Nres, double &sumChi2, double &sumChi2Norm, double &R2 )
{
    int prec=spinBoxSignDigits->value();
    
    R2=1;
    sumChi2=0.0;
    sumChi2Norm=0.0;
    Nres=0;
    //++++++++++++++++++++++++++++++++++++++++
    //+ generate vectors::
    //++++++++++++++++++++++++++++++++++++++++
    double *Q, *Idata, *Isim, *dI, *sigma, * weight, *sigmaf;
    
    //+++ number points
    int N;
    
    //++++++++++++++++++++++++++++++++++++++++
    //+++ read data :: in case table defined case
    //++++++++++++++++++++++++++++++++++++++++
    if ( !SetQandIgivenM ( N, Q, Idata, dI, sigma, weight, sigmaf, m ) ) return false;
    
    //+++ Simulated vector
    Isim=new double[N]; // simulated dataset
    
    //++++++++++++++++++++++++++++++++++++++++
    //+++ Sign. Digits :: Q, ...
    //++++++++++++++++++++++++++++++++++++++++
    int i;

    for (i=0; i<N;i++)
    {
     Isim[i]    = Idata[i];
     /*
        Q[i] 	= round2prec(Q[i], prec); // QString::number(Q[i],'E',prec-1).toDouble();
        //+++
        Idata[i]	= round2prec(Idata[i], prec); // QString::number(Idata[i],'E',prec-1).toDouble();
        //+++
        Isim[i]	= Idata[i];
        //+++
        dI[i]	= round2prec(dI[i], prec); //QString::number(dI[i],'E',prec-1).toDouble();
        //+++
        sigma[i]	= round2prec(sigma[i], prec); //QString::number(sigma[i],'E',prec-1).toDouble();
        //+++
        weight[i]    = round2prec(weight[i], prec); //QString::number(sigma[i],'E',prec-1).toDouble();
        //+++
        sigmaf[i]    = round2prec(sigmaf[i], prec); //QString::number(sigma[i],'E',prec-1).toDouble();
        //+++
     */
    }
    
    if ( !simulateData( N, Q, Isim, dI, sigma, sigmaf, false ) ) return false;
    
    
    double Imean=0;
    for (i=0; i<N;i++) Imean+=Idata[i];
    
    if (N>0) Imean/=N;
    
    double ssl=0;
    
    //++++++++++++++++++++++++++++++++++++++++
    //+++ Sign. Digits :: Isim
    //++++++++++++++++++++++++++++++++++++++++
    double diff;
    for (i=0; i<N;i++)
    {
        sumChi2+=(Isim[i]-Idata[i])*(Isim[i]-Idata[i])/weight[i]/weight[i];
        ssl+=(Imean-Idata[i])*(Imean-Idata[i])/weight[i]/weight[i];
        sumChi2Norm+=1/weight[i]/weight[i];
    }
    
    if (ssl>0) R2=1-sumChi2/ssl;
    Nres=N;
    
    //++++++++++++++++++++++++++++++++++++++++
    //+++ clear memory
    //++++++++++++++++++++++++++++++++++++++++
    delete[] Q;
    delete[] Idata;
    delete[] Isim;
    delete[] dI;
    delete[] sigma;
    delete[] weight;
    delete[] sigmaf;
    return true;
    
}


QString fittable18::formatString(double num, QString prefix, int maxField, int prec, QString suffix)
{
    QString format="";
    if (num>=0)format=" ";
    format+=prefix;
    format+=QString::number(maxField);
    format+=".";
    format+=QString::number(prec);
    format+=suffix;
    
    size_t BufSize=100;
    char buf[BufSize];
    snprintf(buf, BufSize,format.toStdString().c_str(), num);
    
    return QString::fromLatin1(buf);
}

//*******************************************
//+++ covarMatrix
//*******************************************
QString fittable18::covarMatrix(int N, int P, double chi, double chiNormalization, QStringList paraActive, gsl_matrix *covar, gsl_vector *paraAdjust)
{
    if (P<1) return "";
    if (P!=paraActive.count()) return "";
    
    int prec=spinBoxSignDigits->value();
    
    
    gsl_matrix * covarCopy = gsl_matrix_alloc (P, P);
    gsl_matrix * inverse = gsl_matrix_alloc (P, P);
    gsl_matrix_memcpy(covarCopy, covar);
    
    inversion (P, covarCopy, inverse);
    
    
    QString s,ss, sChi, sChi2, sRoot, sDot, sSum;
    sChi=QChar(967);
    sChi2=sChi; sChi2+=QChar(178);
    sRoot=QChar(8730);
    sDot=QChar(8901);
    sSum=QChar(8721);
    
    int sMinLength=spinBoxSignDigits->value()+6;
    
    s="\n";
    
    QString F_name=textLabelFfunc->text();
    QDateTime dt = QDateTime::currentDateTime ();
    s += "[ " + dt.toString(Qt::LocalDate)+ " ]\n";
    for(int i=0; i<2*(5+sMinLength)*(P+1);i++) s+="-";
    s+="\n";
    
    s += tr("Using Function") + ":\t " + F_name + "\n";
    
    if (chiNormalization==1.0) s += tr("Weighting") + ":\t\t\t No\n";
    else s += tr("Weighting") + ":\t\t\t Yes\n";
    s+="\n\n";
    
    
    
    s+="The Variance-Covariance Matrix cov(i,i):\n";
    //------------------------------------------------------------------------
    for(int i=0; i<2*(5+sMinLength)*(P+1);i++) s+="-";
    s+="\n";
    
    for(int p=0;p<P;p++)
    {
        s+=paraActive[p];
        
        s+="\t\t";
    }
    s+="  \t \n";
    
    //------------------------------------------------------------------------
    for(int i=0; i<2*(5+sMinLength)*(P+1);i++) s+="-";
    s+="\n";
    
    
    
    for(int p=0;p<P;p++)
    {
        
        for(int pp=0;pp<P;pp++)
        {
            
            ss="+";
            if (gsl_matrix_get(covar, pp,p)<0) ss="";
            #if defined( Q_OS_WIN)
            s+=ss+QString::number(gsl_matrix_get(covar, pp,p),'E',spinBoxSignDigits->value()) +"\t";
            #else
            s+=formatString(gsl_matrix_get(covar, pp,p), "%", prec+5, prec,"lE\t");
            #endif
            //s+=ss+QString::number(gsl_matrix_get(covar, pp,p),'E',spinBoxSignDigits->value()-1) +"\t";
        }
        s+="| "+paraActive[p];
        s+="\n";
    }
    //------------------------------------------------------------------------
    for(int i=0; i<2*(5+sMinLength)*(P+1);i++) s+="-";
    s+="\n\n";
    
    s+="Values, Errors and Dependences:\n";
    //------------------------------------------------------------------------
    for(int i=0; i<(5+sMinLength)*12;i++) s+="-";
    s+="\n";
    
    s+="Value \t\t Error[ "+ sRoot+" "+sChi2+"/(N-p)"+sDot+"cov(i,i)  ]  \t Error[ "+ sRoot+" cov(i,i)  ] \t Dependency [1-1/cov(i,i)/cov'(i,i)]\t\t Name  \n";
    //------------------------------------------------------------------------
    for(int i=0; i<(5+sMinLength)*12;i++) s+="-";
    s+="\n";
    
    
    for(int p=0;p<P;p++)
    {
        s+=QString::number(gsl_vector_get(paraAdjust, p),'E',spinBoxSignDigits->value()-1)+"\t\t";
        s+=QChar(177);
        s+=" "+ QString::number( sqrt(fabs(chi*chi/(N-P)*gsl_matrix_get(covar,p,p))),'E',spinBoxSignDigits->value()-1)+"\t\t";
        s+=QChar(177);
        s+=" "+ QString::number( sqrt(gsl_matrix_get(covar,p,p)),'E',spinBoxSignDigits->value()-1)+"\t\t";
        
        
        double tmp=fabs(gsl_matrix_get(covar, p,p)*gsl_matrix_get(inverse, p,p));
        
        if (tmp>pow(10.0,prec+2)) tmp=1.0;
        else
        {
            if(tmp<1 ) tmp=0.0;
            else tmp=1-1/tmp;
        };
        if (tmp<0 || tmp>1) tmp=1.0;
        s+=QString::number(tmp,'E',spinBoxSignDigits->value()-1)+"\t\t\t\t";
        s+="| "+paraActive[p];
        s+="\n";
    }
    //------------------------------------------------------------------------
    for(int i=0; i<(5+sMinLength)*12;i++) s+="-";
    s+="\n";
    //s+="chi   = "+ QString::number(chi,'E',20)+"\tchi/sqrt(N-p) \t= "+QString::number(chi/sqrt(N-P),'E',20)+"\n";
    s+=sChi2+" = "+ QString::number(chi*chi,'E', prec+2)+"\t";
    s+=sChi2+"/(N-p) = "+QString::number(chi*chi/(N-P),'E', prec+2)+"\t";
    if (chiNormalization==1.0) s+="\n";
    else s+=sChi2+"/(N-p)/Weight = "+QString::number(chi*chi/(N-P)/chiNormalization,'E', prec+2)+"\n";
    
    s+="\n";
    if (chiNormalization==1.0)
        s+=sChi2+"\t\t\t=\t"+sSum+" |y[i] - f(x[i])|"+QChar(178)+":\t\t"+QString::number(chi*chi,'E', prec+2)+"\n";
    else
        s+=sChi2+"\t\t\t=\t"+sSum+" |y[i] - f(x[i])|"+QChar(178)+" / w"+QChar(178)+"[i]:\t\t"+QString::number(chi*chi,'E', prec+2)+"\n";
    if (chiNormalization!=1.0)
    {
        s += "Weight\t=\t" + sSum + " 1 / w" + QChar(178) + "[i] / N:\t\t\t\t\t\t\t" +
             QString::number(chiNormalization, 'E', prec + 2) + "\n";
        s += "Weight\t:\t" + comboBoxWeightingMethod->currentText() + "\n";
    }
    s += "Q-factor\t=\t" + QString::number(gsl_sf_gamma_inc_Q(double(N - P) / 2.0, chi * chi / 2.0), 'E', prec + 2);
    s += "\n\n";

    s+="Number of Points, N:\t\t\t\t\t"+QString::number(N)+"\n";
    s+="Degrees of Freedom, N-p:\t\t\t"+QString::number(N-P)+"\n";
    s+="Residual Sum of Squares:\t\t\t"+QString::number(chi*chi/chiNormalization,'E', prec+2);
    
    if (chiNormalization==1.0) s+="\t\t-\t"+sChi2+"\n";
    else s+="\t\t-\t"+sChi2+"/Weight\n";

    s+="Residual Standard Deviation:\t\t"+QString::number(sqrt(chi*chi/(N-P)/chiNormalization),'E', prec+2);
    
    if (chiNormalization==1.0) s+="\t\t-\t"+sRoot+"["+sChi2+"/(N-p)]\n";
    else s+="\t\t-\t"+sRoot+"["+sChi2+"/(N-p)/Weight]\n";


    gsl_matrix_free(covarCopy );
    gsl_matrix_free(inverse);
    
    return s;
}

//*******************************************
// fit: readDataForFitAllM() :: 
//*******************************************
// all info from data-tab
bool  fittable18::readDataForFitAllM
(int &Ntotal, int *&numberM, int *&numberSigma,double *&QQ, double *&II, double *&dII, double *&sigmaReso, double *&weightW, double *&sigmaFunction  )
{
    //+++
    int M=spinBoxNumberCurvesToFit->value(); 	//+++ Number of Curves
    int prec=spinBoxSignDigits->value();
    int mm;
    
    Ntotal=0;
    
    //+++ Number of real points per curve
    for (mm=0;mm<M;mm++)
    {
        //++++++++++++++++++++++++++++++++++++++++
        //+ generate vectors::
        //++++++++++++++++++++++++++++++++++++++++
        double *Q, *Idata, *dI, *sigma, *weight, *sigmaf;
        
        //+++ number points
        int N;
        
        //++++++++++++++++++++++++++++++++++++++++
        //+++ read data :: in case table defined case
        //++++++++++++++++++++++++++++++++++++++++
        if ( !SetQandIgivenM( N, Q, Idata, dI, sigma, weight, sigmaf, mm ) ) return false;
        if (N<2) return false;
        Ntotal+=N;
        
        delete[] Q;
        delete[] Idata;
        delete[] dI;
        delete[] sigma;
        delete[] weight;
        delete[] sigmaf;
    }
    
    
    numberM=new int[M];
    // +++
    QQ=new double[Ntotal];
    II=new double[Ntotal];
    dII=new double[Ntotal];
    sigmaReso=new double[Ntotal];
    weightW=new double[Ntotal];
    sigmaFunction=new double[Ntotal];
    
    Ntotal=0;
    //+++ Number of real points per curve
    for (mm=0;mm<M;mm++)
    {
        //++++++++++++++++++++++++++++++++++++++++
        //+ generate vectors::
        //++++++++++++++++++++++++++++++++++++++++
        double *Q, *Idata, *dI, *sigma, *weight, *sigmaf;
        
        //+++ number points
        int N;
        
        //++++++++++++++++++++++++++++++++++++++++
        //+++ read data :: in case table defined case
        //++++++++++++++++++++++++++++++++++++++++
        if ( !SetQandIgivenM ( N, Q, Idata, dI, sigma, weight, sigmaf, mm ) ) return false;
        if (N<2) return false;
        
        //++++++++++++++++++++++++++++++++++++++++
        //+++ Sign. Digits :: Q, ...
        //++++++++++++++++++++++++++++++++++++++++
        
        for (int i=0; i<N;i++)
        {
            QQ[i+Ntotal] 		= Q[i];
            //+++
            II[i+Ntotal]		= Idata[i];
            //+++
            dII[i+Ntotal]		= dI[i];
            //+++
            sigmaReso[i+Ntotal]	= sigma[i];
            //+++
            weightW[i+Ntotal]    = weight[i];
            //+++
            sigmaFunction[i+Ntotal]    = sigmaf[i];
        }
        Ntotal+=N;
        numberM[mm]=N;
        
        delete[] Q;
        delete[] Idata;
        delete[] dI;
        delete[] sigma;
        delete[] weight;
        delete[] sigmaf;
    }
    
    numberSigma=new int[M];
    
    QString currentInstrument=comboBoxInstrument->currentText();
    
    
    //+++ Polydispersity Checks
    for (mm=0;mm<M;mm++)
    {
        if (checkBoxSANSsupport->isChecked() && currentInstrument.contains("SANS") )
        {
            QTableWidgetItem *itS = (QTableWidgetItem *)tableCurves->item(6,2*mm);
            
            if (itS->checkState())
            {
                QComboBoxInTable *polyPara =(QComboBoxInTable*)tableCurves->cellWidget(6, 2*mm+1);
                numberSigma[mm]= polyPara->currentIndex() ;
            }
            else
            {
                numberSigma[mm]=-1;
            }
        }
        else
        {
            numberSigma[mm]=-1;
            
        }
        
    }
    return true;
}

//*******************************************
// initParametersBeforeFi
//*******************************************
void fittable18::initParametersBeforeFit()
{
    //+++
    chekLimits();
    chekLimitsAndFittedParameters();
    
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();
    int digits=spinBoxSignDigits->value()-1;
    
    //+++
    if (p==0)
    {
        QMessageBox::warning(this,tr("QtiKWS"),
                             tr("Select Function"));
        return;
    }
    
    
    int N;
    int *numberM;
    int *numberSigma;
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf;
    

    
    if ( !readDataForFitAllM (N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf) )
    {
        Q=new double[1];Q[0]=0.0;
        I=new double[1];I[0]=0.0;
        dI=new double[1];dI[0]=0.0;
        sigmaReso=new double[1];Q[0]=0.0;
        weight=new double[1];weight[0]=0.0;
        numberM=new int[1];numberM[0]=1;
        sigmaf=new double[1];Q[0]=0.0;
    }

    
    //+++
    bool polyYN=false;
    int polyFunction=comboBoxPolyFunction->currentIndex();
    //+++
    bool beforeFit=true;
    bool afterFit=false;
    bool beforeIter=false;
    bool afterIter=false;
    
    int currentFirstPoint=0;
    int currentLastPoint=0;
    int currentPoint=0;
    
    gsl_vector *limitLeft=gsl_vector_alloc(p);
    gsl_vector *limitRight=gsl_vector_alloc(p);
    
    tablePara->blockSignals(true);//+++2019
    for (int mm=0;mm<M;mm++)
    {
        //+++01 para_fit_yn
        for (int pp=0; pp<p;pp++)
        {
            QTableWidgetItem *itS = (QTableWidgetItem *)tablePara->item(pp,0); // Share?
            QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,1); // Vary?
            
            if (mm==0)
            {
                if (itA0->checkState()) gsl_vector_int_set(F_para_fit_yn, pp, 1);
                else gsl_vector_int_set(F_para_fit_yn, pp, 0);
                continue;
            }
            

            QTableWidgetItem *itA = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            
            if (itS->checkState())
            {
                itA->setCheckState(Qt::Unchecked);
                tablePara->item(pp,3*mm+2)->setText(QString::number(tablePara->item(pp,2)->text().toDouble(),'G',spinBoxSignDigits->value()));
                gsl_vector_int_set(F_para_fit_yn, pp, 0);
            }
            else
            {
                if (itA->checkState())gsl_vector_int_set(F_para_fit_yn, pp, 1);
                else gsl_vector_int_set(F_para_fit_yn, pp, 0);
            }
        }

        //+++02 F_para_limit_left, F_para_limit_right
        double left;
        double right;
        double value;
        QString txtVary;
        QStringList lstTmpLimits;
        for (int pp=0; pp<p;pp++)
        {
            if(tableControl->item(pp,0)->text().contains("-inf")) left=-1.0e308;
            else left=tableControl->item(pp,0)->text().toDouble();
            if(tableControl->item(pp,4)->text().contains("inf")) right=1.0e308;
            else right=tableControl->item(pp,4)->text().toDouble();
            
            QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            txtVary=itA0->text().remove(" ");
            
            if (txtVary.contains("..") && txtVary!="..")
            {
                lstTmpLimits=txtVary.split("..",QString::KeepEmptyParts,Qt::CaseSensitive);
                if(lstTmpLimits.count()==2)
                {
                    double leftNew;
                    if (lstTmpLimits[0]!="") leftNew=lstTmpLimits[0].toDouble();
                    else leftNew=left;
                    
                    double rightNew;
                    if (lstTmpLimits[1]!="") rightNew=lstTmpLimits[1].toDouble();
                    else rightNew=right;
                    
                    if (rightNew>leftNew)
                    {
                        if (leftNew>left && leftNew<right) left=leftNew;
                        if (rightNew<right && rightNew>left) right=rightNew;
                    }
                }
            }
            
            gsl_vector_set(F_para_limit_left,pp,left);
            gsl_vector_set(F_para_limit_right,pp,right);
            
            gsl_vector_set(limitLeft,pp,left);
            gsl_vector_set(limitRight,pp,right);
        }
        
        //+++ ,tableName,tableColNames,tableColDestinations,mTable
        char *tableName="";
        char **tableColNames=0;
        int *tableColDestinations=0;
        gsl_matrix * mTable=0;
        //+++ 2019-09: new superpositional function number
        int currentFunction=spinBoxCurrentFunction->value();
        
        for (int pp=0;pp<p;pp++) gsl_vector_set(F_para,pp,tablePara->item(pp,3*mm+2)->text().toDouble());
        if (mm>0) currentFirstPoint=numberM[mm-1];

        currentLastPoint=numberM[mm]-1;
        currentPoint=currentFirstPoint;
        
        int prec=spinBoxSignDigits->value();

        //+++
        double Int1=1.0;
        double Int2=1.0;
        double Int3=1.0;
        //+++
        int currentInt=0;
        //+++

        functionT paraT={F_para, F_para_limit_left, F_para_limit_right, F_para_fit_yn, Q, I, dI, sigmaf, numberM, mm, currentFirstPoint, currentLastPoint, currentPoint, polyYN, polyFunction, beforeFit, afterFit, beforeIter, afterIter, Int1, Int2, Int3, currentInt, prec ,tableName,tableColNames,tableColDestinations,mTable,currentFunction};
        
        //+++
        F.params=&paraT;

        GSL_FN_EVAL(&F,1.0);
        
        //+++ check
        for (int pp=0;pp<p;pp++)
        {
            //+++ parameters check
            tablePara->item(pp, 3*mm+2)->setText(QString::number(gsl_vector_get(F_para,pp),'G',prec));
            //+++ is Fittable
            QTableWidgetItem *itA = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
            if (gsl_vector_int_get(F_para_fit_yn, pp)==1) itA->setCheckState(Qt::Checked);
            else itA->setCheckState(Qt::Unchecked);
            //+++ limits are changed?
            if (gsl_vector_get(F_para_limit_left,pp)!=gsl_vector_get(limitLeft,pp) || gsl_vector_get(F_para_limit_right,pp)!=gsl_vector_get(limitRight,pp))
            {
                QString txt;
                txt=QString::number(gsl_vector_get(F_para_limit_left,pp),'G',prec);
                txt+="..";
                txt+=QString::number(gsl_vector_get(F_para_limit_right,pp),'G',prec);
                itA->setText(txt);
            }
        }
        
        //+++ Delete  Variables
         if (tableColNames!=0) delete[] tableColNames;
         if (tableColDestinations!=0) delete[] tableColDestinations;
         if (mTable!=0) gsl_matrix_free(mTable);
    }
    tablePara->blockSignals(false);//+++2019
    

    gsl_vector_free(limitLeft);
    gsl_vector_free(limitRight);
    
    delete[] Q;
    delete[] I;
    delete[] dI;
    delete[] sigmaReso;
    delete[] weight;
    delete[] sigmaf;
}

//*******************************************
// initParametersAfterFit
//*******************************************
void fittable18::initParametersAfterFit()
{
    chekLimits();
    chekLimitsAndFittedParameters();
    
    int prec=spinBoxSignDigits->value();
    
    int M=spinBoxNumberCurvesToFit->value();
    int p=spinBoxPara->value();

    //+++
    if (p==0)
    {
        QMessageBox::warning(this,tr("QtiKWS"),
                             tr("Select Function"));
        return;
    }
    
    int N;
    int *numberM;
    int *numberSigma;
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf;
    
    if ( !readDataForFitAllM (N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf) )
    {
        Q=new double[1];Q[0]=0.0;
        I=new double[1];I[0]=0.0;
        dI=new double[1];dI[0]=0.0;
        sigmaReso=new double[1];Q[0]=0.0;
        weight=new double[1];weight[0]=0.0;
        numberM=new int[1];numberM[0]=1;
        sigmaf=new double[1];Q[0]=0.0;
    }
    
    //+++
    bool polyYN=false;
    int polyFunction=comboBoxPolyFunction->currentIndex();
    //+++
    bool beforeFit=false;
    bool afterFit=true;
    bool beforeIter=false;
    bool afterIter=false;
    
    int currentFirstPoint=0;
    int currentLastPoint=0;
    int currentPoint=0;
    
    //+++
    size_t iMstart=0, iMfinish=0;
    //+++
    int Int1=1.0, Int2=1.0, Int3=1.0, currentInt=0;

    gsl_vector *limitLeft=gsl_vector_alloc(p);
    gsl_vector *limitRight=gsl_vector_alloc(p);

 
    tablePara->blockSignals(true);//+++2019
    for (int mm=0;mm<M;mm++)
    {
        //+++01 para_fit_yn
        for (int pp=0; pp<p;pp++)
        {
            QTableWidgetItem *itS = (QTableWidgetItem *)tablePara->item(pp,0); // Share?
            QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,1); // Vary?
            
            if (mm==0)
            {
                if (itA0->checkState()) gsl_vector_int_set(F_para_fit_yn, pp, 1);
                else gsl_vector_int_set(F_para_fit_yn, pp, 0);
                continue;
            }
            
            
            QTableWidgetItem *itA = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            
            if (itS->checkState())
            {
                itA->setCheckState(Qt::Unchecked);
                tablePara->item(pp,3*mm+2)->setText(QString::number(tablePara->item(pp,2)->text().toDouble(),'G',spinBoxSignDigits->value()));
                gsl_vector_int_set(F_para_fit_yn, pp, 0);
            }
            else
            {
                if (itA->checkState())gsl_vector_int_set(F_para_fit_yn, pp, 1);
                else gsl_vector_int_set(F_para_fit_yn, pp, 0);
            }
        }
        
        //+++02 F_para_limit_left, F_para_limit_right
        double left;
        double right;
        double value;
        QString txtVary;
        QStringList lstTmpLimits;
        for (int pp=0; pp<p;pp++)
        {
            if(tableControl->item(pp,0)->text().contains("-inf")) left=-1.0e308;
            else left=tableControl->item(pp,0)->text().toDouble();
            if(tableControl->item(pp,4)->text().contains("inf")) right=1.0e308;
            else right=tableControl->item(pp,4)->text().toDouble();
            QTableWidgetItem *itA0 = (QTableWidgetItem *)tablePara->item(pp,3*mm+1); // Vary?
            txtVary=itA0->text().remove(" ");
            
            if (txtVary.contains("..") && txtVary!="..")
            {
                lstTmpLimits=txtVary.split("..",QString::KeepEmptyParts,Qt::CaseSensitive);
                if(lstTmpLimits.count()==2)
                {
                    double leftNew;
                    if (lstTmpLimits[0]!="") leftNew=lstTmpLimits[0].toDouble();
                    else leftNew=left;
                    
                    double rightNew;
                    if (lstTmpLimits[1]!="") rightNew=lstTmpLimits[1].toDouble();
                    else rightNew=right;
                    
                    if (rightNew>leftNew)
                    {
                        if (leftNew>left && leftNew<right) left=leftNew;
                        if (rightNew<right && rightNew>left) right=rightNew;
                    }
                }
            }
            
            gsl_vector_set(F_para_limit_left,pp,left);
            gsl_vector_set(F_para_limit_right,pp,right);
            
            gsl_vector_set(limitLeft,pp,left);
            gsl_vector_set(limitRight,pp,right);
        }
        
        
        //+++ ,tableName,tableColNames,tableColDestinations,mTable
        char *tableName="";
        char **tableColNames=0;
        int *tableColDestinations=0;
        gsl_matrix * mTable=0;
        //+++ 2019-09: new superpositional function number
        int currentFunction=spinBoxCurrentFunction->value();
        
        for (int pp=0;pp<p;pp++) gsl_vector_set(F_para,pp,tablePara->item(pp, 3*mm+2)->text().toDouble());
        
        if (mm==0)
        {
            iMstart=0;
            iMfinish=numberM[0];
        }
        else
        {
            iMfinish+=numberM[mm];
            iMstart=iMfinish-numberM[mm];
        }
        
        currentLastPoint=numberM[mm]-1;
        currentPoint=currentFirstPoint;
        
        functionT paraT={F_para, F_para_limit_left, F_para_limit_right,
                         F_para_fit_yn, Q, I, dI, sigmaf, numberM, mm,
                         currentFirstPoint, currentLastPoint, currentPoint,
                         polyYN, polyFunction, beforeFit, afterFit, beforeIter,
                         afterIter, static_cast<double>(Int1),
                         static_cast<double>(Int2), static_cast<double>(Int3),
                         currentInt, prec ,tableName, tableColNames,
                         tableColDestinations, mTable, currentFunction};
        
        F.params=&paraT;
        GSL_FN_EVAL(&F,1.0);

        //+++ check
        for (int pp=0;pp<p;pp++)
        {
            //+++ parameters check
            tablePara->item(pp, 3*mm+2)->setText(QString::number(gsl_vector_get(F_para,pp),'G',prec));
            //+++ is Fittable
            QTableWidgetItem *itA = (QTableWidgetItem *)tablePara->item(pp,3*mm+1);
            if (gsl_vector_int_get(F_para_fit_yn, pp)==1) itA->setCheckState(Qt::Checked);
            else itA->setCheckState(Qt::Unchecked);
            //+++ limits are changed?
            if (gsl_vector_get(F_para_limit_left,pp)!=gsl_vector_get(limitLeft,pp) || gsl_vector_get(F_para_limit_right,pp)!=gsl_vector_get(limitRight,pp))
            {
                QString txt;
                txt=QString::number(gsl_vector_get(F_para_limit_left,pp),'G',prec);
                txt+="..";
                txt+=QString::number(gsl_vector_get(F_para_limit_right,pp),'G',prec);
                itA->setText(txt);
            }
        }
        
        //+++ 2016:: table after fit

        if (paraT.tableName!="") makeTableFromMatrix (paraT.tableName, paraT.tableColNames, paraT.tableColDestinations, paraT.mTable);
        
        //+++ Delete  Variables
         if (tableColNames!=0) delete[] tableColNames;
         if (tableColDestinations!=0) delete[] tableColDestinations;
         if (mTable!=0) gsl_matrix_free(mTable);

    }
    tablePara->blockSignals(false);//+++2019
    
    gsl_vector_free(limitLeft);
    gsl_vector_free(limitRight);
    
    delete[] Q;
    delete[] I;
    delete[] dI;
    delete[] sigmaReso;
    delete[] weight;
    delete[] sigmaf;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++FUNCTIONS::additional output table
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool fittable18::makeTableFromMatrix (char *name, char **tableColNames, int *tableColDestinations, gsl_matrix * m)
{
    QString tableName=name;
    if (tableName=="") return false;

    //+++ checking datat +++++
    tableName=tableName.remove(" ").remove("_").remove(",").remove("."); if (tableName=="") return false;
    //    if (tableColNames->size()!=tableColDestinations->size() || tableColNames->size()<1) return false;
    
    int mRows=m->size1; if (mRows<1) return false;
    int mCols=m->size2; if (mCols<1) return false;
    //    if (mCols!=tableColNames.count()) return false;
    
    //+++
    QList<MdiSubWindow *> windows = app()->windowsList();

    //+++
    bool existYN=false;
    //+++
    QString ss;
    //+++
    Table* t;
    
    int oldNumberRows=0;
    //+++ find existing table
    existYN=checkTableExistence(tableName,t);
    if (existYN)
    {
        oldNumberRows=t->numRows();
        t->setNumRows(0);
        t->setNumRows(mRows);
        t->setNumCols(mCols);
    }
    
    //+++create new table
    if (!existYN) t=app()->newHiddenTable(tableName,"Matrix Fit :: Matrix-to-Table", mRows ,mCols);
    
    QString s;
    //+++ col names & destination
    for(int cc=0; cc<mCols; cc++)
    {
        s=tableColNames[cc]; s=s.remove(" ").remove("_");
        t->setColName(cc,s);
        t->setColPlotDesignation(cc,Table::PlotDesignation(tableColDestinations[cc]));
    }
    t->setHeaderColType();
    
    int digits=spinBoxSignDigits->value()-1;
    
    //+++ transfet data to table
    for (int rr=0; rr<mRows;rr++) for(int cc=0;cc<mCols;cc++) t->setText(rr,cc,QString::number(gsl_matrix_get(m,rr,cc),'E',digits));
    
    //+++ table actions
    t->notifyChanges();
    app()->modifiedProject(t);
    if (existYN && oldNumberRows<mRows) app()->showFullRangeAllPlots(tableName);
    
    //+++
    t->setWindowLabel("Matrix Fit :: Matrix-to-Table");
    app()->setListViewLabel(t->name(), "Matrix Fit :: Matrix-to-Table");
    app()->updateWindowLists(t);
    
    for (int tt=0; tt<t->numCols(); tt++)
    {
        t->table()->resizeColumnToContents(tt);
        t->table()->setColumnWidth(tt, t->table()->columnWidth(tt)+10);
    }
    
}

