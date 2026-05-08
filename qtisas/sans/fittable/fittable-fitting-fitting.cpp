/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s)'s fitting tools
 ******************************************************************************/

#include <QElapsedTimer>

#include "fit-function-explorer.h"
#include "fittable18.h"

inline void printInfoTiming(QElapsedTimer &t, qint64 &pre_t, const char *msg, bool justSimulateYN)
{
    if (justSimulateYN)
        return;

    qint64 now = t.elapsed();
    printf("%s\t%5.4lgsec\n", msg, static_cast<double>(now - pre_t) / 1000.0);
    pre_t = now;
}

inline void checkLimitsLocal(int p, gsl_vector *x, const gsl_vector *xleft, const gsl_vector *xright,
                             const gsl_vector_int *bayesian)
{
    for (int i = 0; i < p; ++i)
    {
        if (gsl_vector_int_get(bayesian, i) == 1)
            continue;

        const double xx = gsl_vector_get(x, i);
        const double xxleft = gsl_vector_get(xleft, i);
        const double xxright = gsl_vector_get(xright, i);
        
        if (xx < xxleft)
            gsl_vector_set(x, i, xxleft);
        else if (xx > xxright)
            gsl_vector_set(x, i, xxright);
    }
}

void fittable18::buildFitParametersLimits(gsl_vector *limitLeft, gsl_vector *limitRight, gsl_vector_int *bayesian,
                                          int mLocal)
{
    if ((limitLeft == nullptr || limitRight == nullptr || bayesian == nullptr) && mLocal < 0)
        return;

    int M = spinBoxNumberCurvesToFit->value();
    int p = spinBoxPara->value();

    size_t pFit = 0;

    double left, right;
    double leftGlobal, rightGlobal;
    double leftLocal, rightLocal;

    int mmStart = (mLocal < 0 || mLocal >= M) ? 0 : mLocal;
    int mmEnd = (mLocal < 0 || mLocal >= M) ? M : mLocal + 1;

    for (int pp = 0; pp < p; pp++)
    {
        // ---- GLOBAL LIMITS ----
        bool bayesianYNglobal = ((QComboBox *)tableControl->cellWidget(pp, 1))->currentIndex() == 1;

        leftGlobal = tableControl->item(pp, 0)->text().trimmed().toDouble();
        rightGlobal = tableControl->item(pp, 4)->text().trimmed().toDouble();

        if (bayesianYNglobal && (rightGlobal < 1e-100 || rightGlobal == pinf || leftGlobal == minf))
        {
            bayesianYNglobal = false;
            leftGlobal = minf;
            rightGlobal = pinf;
        }

        for (int mm = mmStart; mm < mmEnd; mm++)
        {
            auto *itA0 = tablePara->item(pp, 3 * mm + 1);

            QString txtVary = itA0->text().remove(" ");
            QStringList lstTmpLimits;

            // ---- LOCAL LIMITS ----
            leftLocal = minf;
            rightLocal = pinf;

            bool bayesianYNlocal = txtVary.contains("+-");

            if (txtVary.isEmpty() || txtVary == ".." || txtVary == "+-" ||
                (!txtVary.contains("+-") && !txtVary.contains("..")))
            {
                bayesianYNlocal = false;
            }
            else
            {
                lstTmpLimits = (bayesianYNlocal) ? txtVary.split("+-") : txtVary.split("..");

                txtVary = lstTmpLimits[0];
                if (txtVary.isEmpty())
                    bayesianYNlocal = false;

                leftLocal = (txtVary.isEmpty() || txtVary == "-inf") ? minf : txtVary.toDouble();

                txtVary = lstTmpLimits[1];
                if (txtVary.isEmpty())
                    bayesianYNlocal = false;

                rightLocal = (txtVary.isEmpty() || txtVary == "inf") ? pinf : txtVary.toDouble();
            }

            if (bayesianYNlocal && ((rightLocal < 1e-100 || rightLocal == pinf) || leftLocal == minf))
            {
                bayesianYNlocal = false;
                leftLocal = minf;
                rightLocal = pinf;
            }

            if (!bayesianYNlocal && bayesianYNglobal && (rightLocal != pinf || leftLocal != minf))
            {
                bayesianYNglobal = false;
                leftGlobal = minf;
                rightGlobal = pinf;
            }

            bool bayesianYN = (bayesianYNglobal || bayesianYNlocal);

            // ---- MERGE LOGIC ----
            if (bayesianYNlocal)
            {
                left = leftLocal;
                right = rightLocal;
            }
            else if (bayesianYNglobal)
            {
                left = leftGlobal;
                right = rightGlobal;
            }
            else
            {
                if (leftLocal > rightLocal)
                    std::swap(leftLocal, rightLocal);

                if (leftGlobal > rightGlobal)
                    std::swap(leftGlobal, rightGlobal);

                left = std::max(leftLocal, leftGlobal);
                right = std::min(rightLocal, rightGlobal);

                if (left > right)
                {
                    left = minf;
                    right = pinf;
                }
            }

            // ---- STORE FIT LIMITS ----
            if (itA0->checkState() && mLocal < 0)
            {
                gsl_vector_set(limitLeft, pFit, left);
                gsl_vector_set(limitRight, pFit, right);
                gsl_vector_int_set(bayesian, pFit, int(bayesianYN));
                pFit++;
            }

            // ---- STORE FUNCTION LIMITS (only once per parameter) ----
            if (mm == 0 || mLocal >= 0)
            {
                if (bayesianYN)
                {
                    gsl_vector_set(F_para_limit_left, pp, minf);
                    gsl_vector_set(F_para_limit_right, pp, pinf);
                }
                else
                {
                    gsl_vector_set(F_para_limit_left, pp, left);
                    gsl_vector_set(F_para_limit_right, pp, right);
                }
            }
        }
    }
}

size_t fittable18::initializeParameters(gsl_vector *params, gsl_vector_int *paramsControl, QStringList &activeParaNames)
{
    size_t np = 0;

    size_t M = spinBoxNumberCurvesToFit->value();
    size_t p = spinBoxPara->value();

    tablePara->blockSignals(true);

    for (int pp = 0; pp < p; pp++)
    {
        QTableWidgetItem *itS = tablePara->item(pp, 0);  // Share?
        QTableWidgetItem *itA0 = tablePara->item(pp, 1); // Vary?

        // First parameter in the row
        if (itA0->checkState())
        {
            gsl_vector_int_set(paramsControl, M * pp, 0); // active
            activeParaNames << tablePara->verticalHeaderItem(pp)->text() + (M > 1 ? "1" : "");
            np++;
            gsl_vector_int_set(F_para_fit_yn, pp, 1);
        }
        else
        {
            gsl_vector_int_set(paramsControl, M * pp, 1); // fixed
            gsl_vector_int_set(F_para_fit_yn, pp, 0);
        }

        gsl_vector_set(params, M * pp, tablePara->item(pp, 2)->text().toDouble());

        // Remaining parameters in the row
        for (int mm = 1; mm < M; mm++)
        {
            QTableWidgetItem *itA = tablePara->item(pp, 3 * mm + 1); // Vary?

            if (itS->checkState()) // shared
            {
                itA->setCheckState(Qt::Unchecked);
                gsl_vector_int_set(paramsControl, M * pp + mm, 2); // shared
                tablePara->item(pp, 3 * mm + 2)
                    ->setText(
                        QString::number(tablePara->item(pp, 2)->text().toDouble(), 'G', spinBoxSignDigits->value()));
            }
            else
            {
                if (itA->checkState()) // active
                {
                    gsl_vector_int_set(paramsControl, M * pp + mm, 0);
                    activeParaNames << tablePara->verticalHeaderItem(pp)->text() +
                                           QString::number(mm + 1); // only in simpleFit
                    np++;
                }
                else // fixed
                {
                    gsl_vector_int_set(paramsControl, M * pp + mm, 1);
                }
            }

            gsl_vector_set(params, M * pp + mm, tablePara->item(pp, 3 * mm + 2)->text().toDouble());
        }

        // Store first parameter for the row
        gsl_vector_set(F_para, pp, tablePara->item(pp, 2)->text().toDouble());
    }

    tablePara->blockSignals(false);
    return np;
}

//+++ doFit: Fit and Simulation (all modes)
bool fittable18::doFit(bool sansYN, bool justSimulateYN)
{
    //+++00 timer
    QElapsedTimer t;
    t.start();
    qint64 pre_t = 0;

    int prec = spinBoxSignDigits->value();

    //+++01 Limits
    chekLimits();
    chekLimitsAndFittedParameters();

    //+++02 Algorithm
    QString algorithm = justSimulateYN ? "Simulation" : comboBoxFitMethod->currentText();

    //+++03 Progress dialog
    int progressIter = 0;
    int maxProgress = algorithm.contains("[GenMin]") ? spinBoxMaxNumberGenerations->value() : spinBoxMaxIter->value();

    QProgressDialog *progress = nullptr;
    if (!setToSetProgressControl && !justSimulateYN)
    {
        progress = new QProgressDialog(app());
        progress->setWindowFlags(Qt::Dialog);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(0);
        progress->setRange(0, maxProgress + 3);
        progress->setCancelButtonText("Abort FIT");
        progress->setMinimumWidth(400);
        progress->setMaximumWidth(600);
        progress->setLabelText("Maximal Number of Iterations: " + QString::number(maxProgress) + ". Progress:");
        progress->show();
        progressIter++;
        progress->setValue(progressIter);
    }

    printInfoTiming(t, pre_t, "\nFit|Init:\t\t\t", justSimulateYN);

    //+++05
    size_t M = spinBoxNumberCurvesToFit->value();
    size_t p = spinBoxPara->value();
    size_t pM = p * M;

    if (p == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Select Function");
        return false;
    }

    //+++06 Parameters && Sharing && Varying
    gsl_vector_int *paramsControl = gsl_vector_int_alloc(pM);
    gsl_vector *params = gsl_vector_alloc(pM);
    QStringList activeParaNames;

    size_t np = initializeParameters(params, paramsControl, activeParaNames);
    if (np == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "No adjustible parameters");
        if (progress)
            progress->cancel();
        return false;
    }

    if (progress)
    {
        progressIter++;
        progress->setLabelText("Started | Data Loading |\n\n\n\n\n\n");
        progress->setValue(progressIter);
    }

    //+++07 DATA reading
    int *numberSigma;
    size_t N;
    int *numberM;
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf;

    if (!readDataForFitAllM(N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf))
    {
        if (progress)
            progress->cancel();
        QMessageBox::warning(this, "QtiSAS", "A problem with Reading Data");
        return false;
    }

    //+++08 Adjustable vector
    gsl_vector *paraAdjust = gsl_vector_alloc(np);
    size_t pFit = 0;
    for (int i = 0; i < pM; i++)
    {
        if (gsl_vector_int_get(paramsControl, i) != 0)
            continue;
        gsl_vector_set(paraAdjust, pFit, gsl_vector_get(params, i));
        pFit++;
    }

    //+++09 Limits
    gsl_vector *limitLeft = gsl_vector_alloc(np);
    gsl_vector *limitRight = gsl_vector_alloc(np);
    gsl_vector_int *bayesian = gsl_vector_int_calloc(np);
    buildFitParametersLimits(limitLeft, limitRight, bayesian);

    printInfoTiming(t, pre_t, "Fit|Started|Data Loading:\t", justSimulateYN);

    //+++10
    bool polyYN = false;
    int polyFunction = sansYN ? comboBoxPolyFunction->currentIndex() : -1;
    bool beforeFit = false, afterFit = false, beforeIter = false, afterIter = false;

    double STEP = lineEditSTEP->text().toDouble();
    if (STEP <= 10.0 / pow(10.0, prec))
        STEP = 10.0 / pow(10.0, prec);

    int currentFirstPoint = 0, currentLastPoint = 0, currentPoint = 0;

    //+++11 Fit control parameters
    int d_max_iterations = spinBoxMaxIter->value();
    double d_tolerance = lineEditTolerance->text().toDouble();
    double absError = lineEditToleranceAbs->text().toDouble();
    bool stopConstChi2 = checkBoxConstChi2->isChecked();
    bool stopConstChi2Simplex = checkBoxConstChi2Simplex->isChecked();

    if (d_tolerance < 0)
        d_tolerance = 0;
    if (absError < 0)
        absError = 0;

    //+++12
    char *tableName = nullptr;
    char **tableColNames = nullptr;
    int *tableColDestinations = nullptr;
    gsl_matrix *mTable = nullptr;
    int currentFunction = spinBoxCurrentFunction->value();

    double Int1 = 1.0, Int2 = 1.0, Int3 = 1.0;
    int currentInt = 0, currentM = 0;

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

    //+++14 RESO (SANS only)
    double absErrReso = 0, relErrReso = 0;
    int intWorkspaseReso = 0, numberSigmaReso = 0, func_reso = 0;

    //+++15 POLY (SANS only)
    double absErrPoly = 0, relErrPoly = 0;
    int intWorkspasePoly = 0, numberSigmaPoly = 0, func_poly = 0;

    if (sansYN)
    {
        absErrReso = lineEditAbsErr->text().toDouble();
        relErrReso = lineEditRelErr->text().toDouble();
        intWorkspaseReso = spinBoxIntWorkspase->value();
        numberSigmaReso = spinBoxIntLimits->value();
        func_reso = comboBoxResoFunction->currentIndex();
        if (comboBoxInstrument->currentText().contains("Back"))
            func_reso += 10;

        absErrPoly = lineEditAbsErrPoly->text().toDouble();
        relErrPoly = lineEditRelErrPoly->text().toDouble();
        intWorkspasePoly = spinBoxIntWorkspasePoly->value();
        numberSigmaPoly = spinBoxIntLimitsPoly->value();
        func_poly = comboBoxPolyFunction->currentIndex();
    }

    //+++16 Build fit parameter structs
    // Simple fit struct (used when !sansYN)
    simplyFitP paraSimple = {N,  M,         p,          np,       Q,   I, weight, numberM, params, paramsControl,
                             &F, limitLeft, limitRight, bayesian, STEP};

    // SANS fit struct (used when sansYN)
    sizetNumbers sizetNums = {N, M, p, np, STEP};
    sansData sansdata = {Q, I, weight, sigmaReso};
    integralControl resoIntCtrl = {absErrReso, relErrReso, intWorkspaseReso, numberSigmaReso, func_reso};
    integralControl polyIntCtrl = {absErrPoly, relErrPoly, intWorkspasePoly, numberSigmaPoly, func_poly};
    fitDataSANSpoly para2 = {&sizetNums,   &sansdata,   numberM,      params,    paramsControl, &F,
                             &resoIntCtrl, numberSigma, &polyIntCtrl, limitLeft, limitRight,    bayesian};

    // Unified function pointers
    auto *fn_f = sansYN ? &function_fmPoly : &function_fm;
    auto *fn_df = sansYN ? &function_dfmPoly : &function_dfm;
    auto *fn_fdf = sansYN ? &function_fdfmPoly : &function_fdfm;
    auto *fn_dm = sansYN ? &function_dmPoly : &function_dm;
    auto *fn_dm_bayesian = sansYN ? &function_dmPoly_bayesian : &function_dm_bayesian;
    auto *fn_r2 = sansYN ? &function_r2Poly : &function_r2;
    void *fitParams = sansYN ? (void *)&para2 : (void *)&paraSimple;

    if (progress)
    {
        progressIter++;
        progress->setLabelText("Started | Loaded | Fitting |\n\n\n\n\n\n");
        progress->setValue(progressIter);
        progress->update();
    }

    //+++17 off gsl errors
    gsl_set_error_handler_off();

    //+++18
    double dof = N - np;

    //+++19 Time of Fit Run
    t.restart();
    pre_t = 0;

    // stagnation break threshold differs between modes
    const int constChi2Limit = sansYN ? 10 : 8;

    size_t iter = 0;
    int status;

    //+++20 Algorithm branches
    if (algorithm.contains("[GenMin]"))
    {
        genome_count = spinBoxGenomeCount->value();
        genome_size = spinBoxGenomeSize->value();
        generations = spinBoxMaxNumberGenerations->value();
        selection_rate = lineEditSelectionRate->text().toDouble();
        mutation_rate = lineEditMutationRate->text().toDouble();
        random_seed = spinBoxRandomSeed->value();

        levenberg_abs = absError;
        levenberg_rel = d_tolerance;
        levenberg_step = STEP;
        levenberg_iters = d_max_iterations;
        levenberg_constchi = stopConstChi2;
        levenberg_unscaled = comboBoxLevenberg->currentText().contains("unscaled");
        levenberg_delta = comboBoxLevenberg->currentText().contains("delta");
        levenberg_yn = comboBoxLocalSearch->currentText().contains("levenberg");
        genmin_yn = comboBoxLocalSearch->currentText().contains("genmin");

        srand(random_seed);

        Problem myproblem(static_cast<int>(np));
        myproblem.YN2D(false);
        myproblem.sansSupportYN(sansYN);
        myproblem.dof = int(dof);
        myproblem.prec = prec;

        if (sansYN)
            myproblem.setSANSfitP(para2);
        else
            myproblem.setSimplyFitP(paraSimple);

        DataG L, R;
        L.resize(np);
        R.resize(np);

        for (int i = 0; i < np; i++)
        {
            double lL = gsl_vector_get(limitLeft, i);
            double lR = gsl_vector_get(limitRight, i);
            L[i] = (lL != minf) ? lL : minf;
            R[i] = (lR != pinf) ? lR : pinf;
        }
        myproblem.setMargin(L, R);

        myproblem.fln.f = fn_f;
        myproblem.fln.df = fn_df;
        myproblem.fln.fdf = fn_fdf;
        myproblem.fln.n = N;
        myproblem.fln.p = np;
        myproblem.fln.params = fitParams;

        GenMin opt(&myproblem);

        printInfoTiming(t, pre_t, "Fit|Started|Loaded|Fit Preparation:", justSimulateYN);
        printf("Fit|Started|Loaded|Prepared|Iterations:\n\n");

        opt.Solve(progress);

        DataG x;
        double y;
        x.resize(np);
        opt.getMinimum(x, y);

        if (opt.besty != 0.0 && genmin_yn)
            opt.localSearch(x, progress);
        if (opt.besty != 0.0 && levenberg_yn)
            opt.localSearchGSL(x, prec, progress);

        printf("X = [");
        for (int i = 0; i < np; i++)
            printf(" %lg ", x[i]);
        printf("] \nchi^2 =%lg\n", y / dof);
        printf("FUNCTION CALLS =%6d\nGRADIENT CALLS=%6d\n\n", myproblem.fevals, myproblem.gevals);

        for (int pp = 0; pp < np; pp++)
            gsl_vector_set(paraAdjust, pp, x[pp]);

        iter = opt.iterations;
    }
    else if (algorithm.contains("Nelder-Mead Simplex"))
    {
        algorithm += " " + comboBoxSimplex->currentText();

        gsl_multimin_function f;
        f.f = fn_dm;
        f.n = np;
        f.params = fitParams;

        const gsl_multimin_fminimizer_type *TT;
        if (algorithm.contains("nmsimplex2rand"))
            TT = gsl_multimin_fminimizer_nmsimplex2rand;
        else if (algorithm.contains("nmsimplex2"))
            TT = gsl_multimin_fminimizer_nmsimplex2;
        else
            TT = gsl_multimin_fminimizer_nmsimplex;

        gsl_vector *ss = gsl_vector_alloc(np);
        int convRate = comboBoxConvRate->currentIndex();
        if (convRate == 1)
            gsl_vector_set_all(ss, 1.0);
        else if (convRate == 0)
            gsl_vector_set_all(ss, 10.0);
        else
            gsl_vector_set_all(ss, 0.1);

        gsl_multimin_fminimizer *s_min = gsl_multimin_fminimizer_alloc(TT, np);
        status = gsl_multimin_fminimizer_set(s_min, &f, paraAdjust, ss);

        if (status != 0)
        {
            QString report = tr("<b> %1 </b>: GSL error -1- :: ").arg(QString::number(status));
            report += gsl_strerror(status);
            QMessageBox::warning(this, "QtiSAS", report);
        }

        printInfoTiming(t, pre_t, "Fit|Started|Loaded|Fit Preparation:", justSimulateYN);
        printf("Fit|Started|Loaded|Prepared|Iterations:\n\n");

        double size = gsl_multimin_fminimizer_size(s_min);
        double chi2local = pinf;
        double chi2localOld = pinf;
        iter = 0;
        int stagnationCount = 0;
        int maxStagnation = 5;

        do
        {
            progressIter++;
            QString st = "Started | Loaded | Fitting > Iterations\n\n" + algorithm + "\n\n";
            st += " # \t\t\t\t\t\t\t Stopping Criterion \t\t\t\t\t\t\t chi^2 \n\n";
            st += QString::number(iter) + "[<" + QString::number(d_max_iterations) + "] \t\t\t " +
                  QString::number(size, 'E', prec) + " [>" + QString::number(d_tolerance, 'E', prec) + "] \t\t\t " +
                  QString::number(chi2local, 'E', prec + 2) + "\n\n";

            if (progress)
            {
                progress->setLabelText(st);
                progress->setValue(progressIter);
                if (progress->wasCanceled())
                {
                    progress->cancel();
                    break;
                }
            }

            status = gsl_multimin_fminimizer_iterate(s_min);
            checkLimitsLocal(static_cast<int>(np), s_min->x, limitLeft, limitRight, bayesian);

            size = gsl_multimin_fminimizer_size(s_min);
            status = gsl_multimin_test_size(size, d_tolerance);

            chi2localOld = chi2local;
            chi2local = s_min->fval / dof;

            st = "# (simplex) %4d[<%4d] %10.5lg [>%10.5lg] chi^2 %15.10lg";
            printf(st.toLocal8Bit().constData(), (int)iter, d_max_iterations, size, d_tolerance, chi2local);
            printf(" [ ");
            for (int i = 0; i < GSL_MIN(np, 10); i++)
                printf("%8.6lg ", gsl_vector_get(s_min->x, i));
            if (np > 10)
                printf("...");
            printInfoTiming(t, pre_t, " ]", justSimulateYN);

            iter++;

            if (stopConstChi2Simplex)
            {
                if (fabs(chi2local - chi2localOld) < chi2local / pow(10, prec))
                    stagnationCount++;
                else
                    stagnationCount = 0;

                if (stagnationCount >= maxStagnation)
                {
                    gsl_vector_memcpy(paraAdjust, s_min->x);
                    gsl_multimin_fminimizer_free(s_min);

                    for (int i = 0; i < np; ++i)
                    {
                        double x = gsl_vector_get(paraAdjust, i);
                        gsl_vector_set(ss, i, fabs(x) * 0.1 + 1e-6);
                    }

                    s_min = gsl_multimin_fminimizer_alloc(TT, np);
                    gsl_multimin_fminimizer_set(s_min, &f, paraAdjust, ss);

                    stagnationCount = 0;
                    chi2local = chi2localOld = pinf;
                    qDebug() << "\nSimplex stagnation detected. Restarting simplex optimization.\n";
                    maxStagnation += 10;
                    continue;
                }
            }

            if (iter > 1 && chi2local == 0.0)
            {
                qDebug() << "Chi^2 is zero. Optimization has converged to a perfect fit: " << s_min->fval / dof;
                status = GSL_SUCCESS;
            }
        }
        while (status == GSL_CONTINUE && (int)iter < d_max_iterations);

        gsl_vector_memcpy(paraAdjust, s_min->x);
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free(s_min);
    }
    else if (algorithm.contains("Levenberg-Marquardt")) // Levenberg-Marquardt
    {
        algorithm += " " + comboBoxLevenberg->currentText();

        const bool deltaStop = algorithm.contains("delta");
        const bool unscaled = algorithm.contains("unscaled");

        gsl_multifit_function_fdf fln;
        fln.f = fn_f;
        fln.df = fn_df;
        fln.fdf = fn_fdf;
        fln.n = N;
        fln.p = np;
        fln.params = fitParams;

        const gsl_multifit_fdfsolver_type *Tln =
            unscaled ? gsl_multifit_fdfsolver_lmder : gsl_multifit_fdfsolver_lmsder;

        gsl_multifit_fdfsolver *sln = gsl_multifit_fdfsolver_alloc(Tln, N, np);
        status = gsl_multifit_fdfsolver_set(sln, &fln, paraAdjust);

        if (status != GSL_SUCCESS)
            QMessageBox::warning(this, "QtiSAS", tr("<b>%1</b>: GSL error -3-").arg(status));

        printInfoTiming(t, pre_t, "Fit|Started|Loaded|Fit Preparation:", justSimulateYN);
        printf("Fit|Started|Loaded|Prepared|Iterations:\n\n");

        gsl_vector *grad = gsl_vector_alloc(np);
        gsl_matrix *J = gsl_matrix_alloc(N, np);

        auto chi2loc = [&] {
            const double n = gsl_blas_dnrm2(sln->f);
            return (n * n) / dof;
        };

        double chi2localOld = 0;
        double chi2local = chi2loc();
        double ssizeAbs = 0;
        double ssizeDelta = 0;
        int countConstChi2 = 0;
        iter = 0;

        do
        {
            ++iter;
            status = gsl_multifit_fdfsolver_iterate(sln);
            if (status > 1 && !stopConstChi2)
            {
                qDebug() << "stopped by GSL error:" << gsl_strerror(status);
                break;
            }

            checkLimitsLocal(static_cast<int>(np), sln->x, limitLeft, limitRight, bayesian);
            ssizeDelta = ssizeAbs = 0;

            if (deltaStop)
            {
                for (int i = 0; i < np; ++i)
                {
                    const double dx = fabs(gsl_vector_get(sln->dx, i));
                    const double x = fabs(gsl_vector_get(sln->x, i));
                    const double v = fabs(dx - d_tolerance * x);
                    if (v > ssizeDelta)
                        ssizeDelta = v;
                }
                status = (iter > 1 && ssizeDelta < absError) ? GSL_SUCCESS : GSL_CONTINUE;
            }
            else
            {
                gsl_multifit_fdfsolver_jac(sln, J);
                gsl_multifit_gradient(J, sln->f, grad);
                for (int i = 0; i < np; ++i)
                    ssizeAbs += fabs(gsl_vector_get(grad, i));
                status = gsl_multifit_test_gradient(grad, absError);
            }

            chi2localOld = chi2local;
            chi2local = chi2loc();
            ++progressIter;

            const QString stopValue = QString::number(deltaStop ? ssizeDelta : ssizeAbs, 'E', prec);
            const QString stopLimit = QString::number(absError, deltaStop ? 'E' : 'g', deltaStop ? prec : 6);

            if (progress)
            {
                QString st = "Started | Loaded | Fitting > Iterations\n\n" + algorithm + "\n\n" +
                             " # \t\t\t Stopping Criterion \t\t\t chi^2 \n\n" + QString::number(iter) + "[<" +
                             QString::number(d_max_iterations) + "] \t " + stopValue + " [> " + stopLimit + "] \t " +
                             QString::number(chi2local, 'E', prec + 2) + "\n\n";
                progress->setLabelText(st);
                progress->setValue(progressIter);
                if (progress->wasCanceled())
                {
                    progress->cancel();
                    break;
                }
            }

            if (stopConstChi2)
                countConstChi2 = (chi2localOld <= chi2local) ? countConstChi2 + 1 : 0;
            else if (iter > 1 && chi2localOld < chi2local)
                break;

            printf("# (levenberg) %4zu[<%4d] %10.5lg [>%10.5lg] chi^2 %15.10lg", iter, d_max_iterations,
                   deltaStop ? ssizeDelta : ssizeAbs, absError, chi2local);
            printf(" [ ");

            for (int i = 0; i < GSL_MIN(np, 10); ++i)
                printf("%8.6lg ", gsl_vector_get(sln->x, i));
            if (np > 10)
                printf("...");

            printf(countConstChi2 > 0 ? " ]%2d [x const chi]" : " ]", countConstChi2);
            printInfoTiming(t, pre_t, "", justSimulateYN);

            if (iter > 1 && chi2local == 0.0)
            {
                qDebug() << "Chi^2 is zero. Optimization has converged to a perfect fit: " << chi2local / dof;
                status = GSL_SUCCESS;
            }

            if (stopConstChi2 && countConstChi2 > 0 && status == GSL_CONTINUE)
            {
                if (countConstChi2 == constChi2Limit)
                    break;

                if (sansYN)
                {
                    auto *sn = ((struct sizetNumbers *)((struct fitDataSANSpoly *)fln.params)->SizetNumbers);
                    if (countConstChi2 == 2 || countConstChi2 == 3)
                        sn->STEP = STEP * 10;
                    else if (countConstChi2 == 6 || countConstChi2 == 7)
                        sn->STEP = STEP * 1000;
                    else if (countConstChi2 >= 4 && countConstChi2 <= 9)
                        sn->STEP = STEP;
                }
                else
                {
                    auto *sp = static_cast<simplyFitP *>(fln.params);
                    if (countConstChi2 == 2 || countConstChi2 == 3)
                        sp->STEP = STEP * 10;
                    if (countConstChi2 == 4 || countConstChi2 == 7)
                        sp->STEP = STEP;
                    if (countConstChi2 == 5 || countConstChi2 == 6)
                        sp->STEP = STEP * 1000;
                }

                gsl_multifit_fdfsolver_set(sln, &fln, sln->x);
            }
        } while (status == GSL_CONTINUE && (int)iter < d_max_iterations);

        gsl_vector_memcpy(paraAdjust, sln->x);
        gsl_vector_free(grad);
        if (J)
            gsl_matrix_free(J);
        gsl_multifit_fdfsolver_free(sln);
    }

    checkLimitsLocal(static_cast<int>(np), paraAdjust, limitLeft, limitRight, bayesian);

    //+++ Chi
    double bayesianTermChi2;
    double chi2 = fn_dm_bayesian(paraAdjust, fitParams, bayesianTermChi2);
    double chi = sqrt(chi2);
    double c = chi / sqrt(dof);

    textLabelChi->setText(QString::number(chi2 / dof, 'E', prec + 2));

    QString chiTooltip = "χ2 = χ2 [total] = χ2 [data] + χ2 [Bayesian]\n\n";
    chiTooltip += "χ2/dof [total] = " + QString::number(chi2 / dof, 'E', prec + 2) + "\n";
    chiTooltip += "χ2/dof [data] = " + QString::number((chi2 - bayesianTermChi2) / dof, 'E', prec + 2) + "\n";
    chiTooltip += "χ2/dof [Bayesian] = " + QString::number((bayesianTermChi2) / dof, 'E', prec + 2);

    textLabelChi->setToolTip(chiTooltip);

    double r2 = fn_r2(paraAdjust, fitParams);
    textLabelR2->setText(QString::number(r2, 'E', prec + 2));

    //+++ Jacobian
    gsl_matrix *J = gsl_matrix_alloc(N, np);
    fn_df(paraAdjust, fitParams, J);

    if (!justSimulateYN)
        printf("\nFit|Finished\n");
    printInfoTiming(t, pre_t, "Fit|Finished|Jacobian Matrix:\t", justSimulateYN);

    //+++ Covariance
    gsl_matrix *covar = gsl_matrix_alloc(np, np);
    gsl_multifit_covar(J, 0.0, covar);

    printInfoTiming(t, pre_t, "Fit|Finished|Covariant Matrix:\t", justSimulateYN);

#define FIT(i) gsl_vector_get(paraAdjust, i)
#define ERR(i) sqrt(gsl_matrix_get(covar, i, i))

    int npnp = 0;
    if (!checkBoxScaleErrors->isChecked())
        c = 1.0;

    tablePara->blockSignals(true);
    for (int pp = 0; pp < p; pp++)
        for (int mm = 0; mm < M; mm++)
        {
            if (gsl_vector_int_get(paramsControl, M * pp + mm) == 0)
            {
                tablePara->item(pp, 3 * mm + 2)->setText(QString::number(FIT(npnp), 'G', prec));
                tablePara->item(pp, 3 * mm + 3)->setText(QChar(177) + QString::number(c * ERR(npnp), 'G', prec));
                gsl_vector_set(params, M * pp + mm, FIT(npnp));
                npnp++;
            }
            else if (gsl_vector_int_get(paramsControl, M * pp + mm) == 1)
            {
                if (mm == 0 && mm + 1 < M && gsl_vector_int_get(paramsControl, M * pp + mm + 1) == 2)
                    tablePara->item(pp, 3 * mm + 2)
                        ->setText(QString::number(gsl_vector_get(params, M * pp), 'G', prec));
                tablePara->item(pp, 3 * mm + 3)->setText("---");
            }
            else if (gsl_vector_int_get(paramsControl, M * pp + mm) == 2)
            {
                if (mm > 0)
                    tablePara->item(pp, 3 * mm + 2)
                        ->setText(QString::number(gsl_vector_get(params, M * pp), 'G', prec));
                tablePara->item(pp, 3 * mm + 2)->setText(tablePara->item(pp, 2)->text());
                tablePara->item(pp, 3 * mm + 3)->setText("---");
            }
        }
    tablePara->blockSignals(false);

    printInfoTiming(t, pre_t, "Fit|Finished|Parameter Transfer:", justSimulateYN);

    //+++ Covariance matrix note
    if (checkBoxCovar->isChecked())
    {
        double chiWeight2 = 0;
        for (int nn = 0; nn < N; ++nn)
            chiWeight2 += (weight[nn] != 0.0) ? 1.0 / (weight[nn] * weight[nn]) : 1.0;
        chiWeight2 /= double(N);

        QString info = covarMatrix(static_cast<int>(N), static_cast<int>(np), chi, sqrt(bayesianTermChi2), chiWeight2,
                                   activeParaNames, covar, paraAdjust);
        makeNote(info, "fitCurve-" + comboBoxFunction->currentText() + "-statistics", "TableFit :: statistics info ");
        printInfoTiming(t, pre_t, "Fit|Finished|Statistics Note:", justSimulateYN);
    }

    if (!justSimulateYN)
        printf("\n");

    //+++ Cleanup
    if (mTable)
        gsl_matrix_free(mTable);

    delete[] tableColNames;
    delete[] tableColDestinations;
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
    gsl_vector_int_free(bayesian);
    gsl_vector_free(params);
    gsl_vector_int_free(paramsControl);
    gsl_vector_free(paraAdjust);
    gsl_matrix_free(covar);
    gsl_matrix_free(J);

#undef FIT
#undef ERR

    QString durationText = QString::number(static_cast<double>(t.elapsed()), 'G', 3) + " ms - ";
    durationText += justSimulateYN ? "simulation" : QString::number(iter) + " iteration(s)";
    textLabelTime->setText(durationText);

    if (progress)
        progress->cancel();

    return true;
}

QString fittable18::formatString(double num, QString prefix, int maxField, int prec, QString suffix)
{

    QLocale locale(QLocale::C);

    QString format = "%";
    if (num >= 0)
        format += " ";
    format += QString::number(maxField) + "." + QString::number(prec) + suffix;

    char buf[128];
    snprintf(buf, sizeof(buf), format.toLatin1().constData(), num);
    return prefix + QString::fromLatin1(buf);
}
//+++ covarMatrix string
QString fittable18::covarMatrix(int N, int P, double chi, double chiBayesian, double chiNormalization,
                                QStringList paraActive, gsl_matrix *covar, gsl_vector *paraAdjust)
{
    if (P < 1 || P != paraActive.count())
        return {};

    int prec = spinBoxSignDigits->value();

    gsl_matrix *covarCopy = gsl_matrix_alloc(P, P);
    gsl_matrix_memcpy(covarCopy, covar);

    gsl_matrix *inverse = gsl_matrix_alloc(P, P);
    inversion(P, covarCopy, inverse);
    gsl_matrix_free(covarCopy);

    int sMinLength = prec + 6;
    QString s;

    // Header
    QString F_name = textLabelFfunc->text();
    QDateTime dt = QDateTime::currentDateTime();
    QLocale l = QLocale::system();

    s += "[ " + l.toString(dt, QLocale::ShortFormat) + " ]\n\n";
    s += tr("Using Function") + ":\t " + F_name + "\n";
    s += tr("Weighting") + ":\t\t " +
         (chiNormalization == 1.0 ? "No" : "Yes \t\t - \t\t" + comboBoxWeightingMethod->currentText()) + "\n";
    s += tr("Bayesian") + ":\t\t\t " + (chiBayesian == 0.0 ? "No" : "Yes") + "\n\n";

    // Covariance matrix
    s += "The Variance-Covariance Matrix cov(i,i):\n";
    int fieldWidth = prec + 12;

    // Headers centered above each column
    QString covMatrixHeader;
    for (int p = 0; p < P; ++p)
    {
        const QString &header = paraActive[p];
        int padding = fieldWidth - int(header.length());
        int leftPad = padding / 2;
        int rightPad = padding - leftPad;
        if (padding < 0)
        {
            leftPad = 0;
            rightPad = 0;
        }
        covMatrixHeader += QString(leftPad, ' ') + header + QString(rightPad, ' ');
    }
    covMatrixHeader += "\n";

    // Header of Covariance matrix
    QString line = QString((P + 1) * fieldWidth, '-') + "\n";
    s += line;
    s += covMatrixHeader;
    s += line;

    // Covariance matrix rows with row labels
    for (int p = 0; p < P; ++p)
    {
        for (int pp = 0; pp < P; ++pp)
        {
            double val = gsl_matrix_get(covar, pp, p);
            s += QString::asprintf("%+*.*E", fieldWidth, prec, val);
        }
        s += " \t\t| " + paraActive[p] + "\n";
    }
    s += line;

    line = QString(110, '-') + "\n";
    
    s += "\nValues, Errors and Dependences:\n";
    s += line;
    s += "Value \t\t ";
    s += "Error[√ χ²/(N-p)·cov(i,i)]  \t Error[√ cov(i,i)] \t Dependency [1 - 1/cov(i,i)/cov'(i,i)]\t\t Name\n";
    s += line;

    for (int p = 0; p < P; ++p)
    {
        double val = gsl_vector_get(paraAdjust, p);
        double var = gsl_matrix_get(covar, p, p);
        double errNorm = sqrt(fabs(chi * chi / (N - P) * var));
        double errRaw = sqrt(var);

        s += QString::number(val, 'E', prec - 1) + "\t\t± " + QString::number(errNorm, 'E', prec - 1) + "\t\t± " +
             QString::number(errRaw, 'E', prec - 1) + "\t\t";

        double tmp = gsl_matrix_get(covar, p, p) * gsl_matrix_get(inverse, p, p);
        tmp = fabs(tmp);

        if (tmp > pow(10.0, prec + 2))
            tmp = 1.0;
        else if (tmp < 1.0)
            tmp = 0.0;
        else
            tmp = 1.0 - 1.0 / tmp;

        if (tmp < 0.0 || tmp > 1.0)
            tmp = 1.0;

        s += QString::number(tmp, 'E', prec - 1) + "\t\t\t| " + paraActive[p] + "\n";
    }
    s += line;

    //--- Chi² summary
    s += "χ² = " + QString::number(chi * chi, 'E', prec + 2) + "\t\t";
    s += "χ²/(N-p) = " + QString::number(chi * chi / (N - P), 'E', prec + 2) + "\t\t";
    if (chiNormalization != 1.0)
        s += "χ²/(N-p)/Weight = " + QString::number(chi * chi / (N - P) / chiNormalization, 'E', prec + 2);
    s += "\n\n";

    if (chiNormalization == 1.0)
        s += "χ² = ∑ |y[i] - f(x[i])|²:\t" + QString::number(chi * chi, 'E', prec + 2) + "\n";
    else
    {
        s += "χ² = ∑ w[i] (y[i] - f(x[i]))² :\t" + QString::number(chi * chi, 'E', prec + 2) + "\n";
        s += "Weight = ∑ w[i] / N:\t\t" + QString::number(chiNormalization, 'E', prec + 2) + "\n";

        if (comboBoxWeightingMethod->currentIndex() == 0)
            s +=
                "\nQ-factor = " + QString::number(gsl_sf_gamma_inc_Q(double(N - P) / 2.0, chi * chi / 2.0), 'g') + "\n";
    }

    //--- Summary
    s += "\nNumber of Points, N:\t\t\t" + QString::number(N) + "\n";
    s += "Degrees of Freedom, N-p:\t\t" + QString::number(N - P) + "\n";
    s += "Residual Sum of Squares:\t\t" + QString::number(chi * chi / chiNormalization, 'E', prec + 2) +
         (chiNormalization == 1.0 ? "\t-> χ²\n" : "\t-> χ²/Weight\n");
    s += "Residual Standard Deviation:\t" +
         QString::number(sqrt(chi * chi / (N - P) / chiNormalization), 'E', prec + 2) +
         (chiNormalization == 1.0 ? "\t-> √[χ²/(N-p)]\n" : "\t-> √[χ²/(N-p)/Weight]\n");

    if (chiBayesian > 0.0)
    {
        s += "\n\nBayesian Fit:\n\n";
        s += "χ² = χ²[total] = χ²[data] + χ²[Bayesian]\n\n";
        s += "χ²/(N-p) [total]:\t\t\t\t" + QString::number(chi * chi / (N - P), 'E', prec + 2) + "\n";
        s += "χ²/(N-p) [data]:\t\t\t\t" +
             QString::number((chi * chi - chiBayesian * chiBayesian) / (N - P), 'E', prec + 2) + "\n";
        s += "χ²/(N-p) [Bayesian]:\t\t\t" + QString::number(chiBayesian * chiBayesian / (N - P), 'E', prec + 2) + "\n";
    }

    gsl_matrix_free(inverse);

    s.replace('-', QChar(0x2212));
    return s;
}

//+++ fit: readDataForFitAllM()
bool fittable18::readDataForFitAllM(size_t &Ntotal, int *&numberM, int *&numberSigma, double *&QQ, double *&II,
                                    double *&dII, double *&sigmaReso, double *&weightW, double *&sigmaFunction)
{
    size_t M = spinBoxNumberCurvesToFit->value(); //+++ Number of Curves
    int prec = spinBoxSignDigits->value();
    
    Ntotal=0;
    
    //+++ Number of real points per curve
    for (int mm = 0; mm < M; mm++)
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
    for (int mm = 0; mm < M; mm++)
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
    for (int mm = 0; mm < M; mm++)
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

    size_t N;
    int *numberM;
    int *numberSigma;
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf;

    if (!readDataForFitAllM(N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf))
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
                lstTmpLimits = txtVary.split("..");
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
        char *tableName = nullptr;
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

QString fittable18::readAfterFitPythonScript(const QString &functionName) const
{
    QString fifFileName = libPath + functionName + ".fif";

    QString content = FunctionsExplorer::readFifFileContent(fifFileName);
    QStringList listOfBlocks = FunctionsExplorer::getFifFileBlocks(content);

    if (listOfBlocks.isEmpty())
        return {};

    // +++ [after.fit: python]
    QStringList lst = FunctionsExplorer::readBlock(listOfBlocks, "[after.fit: python]");
    if (lst.count() < 2 || !lst[1].contains("1"))
        return {};

    // +++ [after.fit: python code]
    lst = FunctionsExplorer::readBlock(listOfBlocks, "[after.fit: python code]");
    if (lst.count() < 2)
        return {};

    QString script;
    foreach (const QString &block, lst.mid(1))
        script += block + "\n";

    return script;
}

//+++ initParametersAfterFit
void fittable18::initParametersAfterFit()
{
    chekLimits();
    chekLimitsAndFittedParameters();

    int prec = spinBoxSignDigits->value();

    size_t M = spinBoxNumberCurvesToFit->value();
    size_t p = spinBoxPara->value();

    if (p == 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Select Function");
        return;
    }

    size_t N;
    int *numberM;
    int *numberSigma;
    double *Q, *I, *dI, *sigmaReso, *weight, *sigmaf;

    if (!readDataForFitAllM(N, numberM, numberSigma, Q, I, dI, sigmaReso, weight, sigmaf))
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
                lstTmpLimits = txtVary.split("..");
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
        char *tableName = nullptr;
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

        if (paraT.tableName != nullptr)
            makeTableFromMatrix(paraT.tableName, paraT.tableColNames, paraT.tableColDestinations, paraT.mTable);
        
        //+++ Delete  Variables
         if (tableColNames!=0) delete[] tableColNames;
         if (tableColDestinations!=0) delete[] tableColDestinations;
         if (mTable!=0) gsl_matrix_free(mTable);

    }
    tablePara->blockSignals(false);//+++2019

    if (app()->activeScriptingLanguage() == QString("Python"))
        app()->scriptCaller(readAfterFitPythonScript(comboBoxFunction->currentText()));

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
void fittable18::makeTableFromMatrix(const char *name, char **tableColNames, int *tableColDestinations, gsl_matrix *m)
{
    QString tableName = QString(name);
    tableName = tableName.remove(" ").remove("_").remove(",").remove(".");
    if (tableName == QString())
        return;
    //    if (tableColNames->size()!=tableColDestinations->size() || tableColNames->size()<1) return false;
    
    int mRows = static_cast<int>(m->size1);
    if (mRows < 1)
        return;
    int mCols = static_cast<int>(m->size2);
    if (mCols < 1)
        return;
    //    if (mCols!=tableColNames.count()) return false;
    
    //+++
    QList<MdiSubWindow *> windows = app()->windowsList();

    //+++
    bool existYN=false;
    //+++
    QString ss;
    //+++
    Table* t;

    int oldNumberRows = 0;
    //+++ find existing table
    existYN = app()->checkTableExistence(tableName, t);
    if (existYN)
    {
        oldNumberRows=t->numRows();
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
    t->blockSignals(true);
    for (int rr=0; rr<mRows;rr++) for(int cc=0;cc<mCols;cc++) t->setText(rr,cc,QString::number(gsl_matrix_get(m,rr,cc),'E',digits));
    t->blockSignals(false);

    //+++ table actions
    t->notifyChanges();
    app()->modifiedProject(t);
    if (existYN && oldNumberRows<mRows) app()->showFullRangeAllPlots(tableName);
    
    //+++
    t->setWindowLabel("Matrix Fit :: Matrix-to-Table");
    app()->setListViewLabel(t->name(), "Matrix Fit :: Matrix-to-Table");
    app()->updateWindowLists(t);

    t->adjustColumnsWidth(false);
}

