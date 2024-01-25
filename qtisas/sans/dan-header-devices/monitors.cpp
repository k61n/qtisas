/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Individual Header Parser for Monitors and Duration normalization
 ******************************************************************************/

#include "monitors.h"
#include "deadTimeRoot.h"
#include <gsl/gsl_roots.h>

Monitors::Monitors(ParserHeader *parserHeaderDAN, QComboBox *unitsDurationDAN, QLineEdit *deadTimeM1DAN,
                   QLineEdit *deadTimeM2DAN, QLineEdit *deadTimeM3DAN, QLineEdit *deadTimeDetectorDAN,
                   QLineEdit *deadTimeDetectorDBDAN, QRadioButton *deadTimeDetectorParalysableDAN,
                   QSpinBox *normalizationConstDAN, QComboBox *normalizationTypeDAN)
{
    parserHeader = parserHeaderDAN;
    unitsDuration = unitsDurationDAN;
    deadTimeM1 = deadTimeM1DAN;
    deadTimeM2 = deadTimeM2DAN;
    deadTimeM3 = deadTimeM3DAN;
    deadTimeDetector = deadTimeDetectorDAN;
    deadTimeDetectorDB = deadTimeDetectorDBDAN;
    deadTimeDetectorParalysable = deadTimeDetectorParalysableDAN;
    normalizationConst = normalizationConstDAN;
    normalizationType = normalizationTypeDAN;
}
//+++ Duration units converter to [sec]
double Monitors::unitsConverterDurationToSec(double duration)
{
    if (unitsDuration->currentIndex() == 1)
        duration /= 10.0;
    if (unitsDuration->currentIndex() == 2)
        duration /= 1000.0;
    if (unitsDuration->currentIndex() == 3)
        duration /= 1000000.0;
    return duration;
}
//+++ read duration [sec]
double Monitors::readDuration(const QString &Number, const QStringList &lst)
{
    return unitsConverterDurationToSec(parserHeader->readNumberString(Number, "[Duration]", lst).toDouble());
}
//+++ read  Monitor1
double Monitors::readMonitor1(const QString &Number, double deadTime, const QStringList &lst)
{
    double M1 = parserHeader->readNumberString(Number, "[Monitor-1]", lst).toDouble();
    double duration = readDuration(Number);

    if (deadTime < 0)
        deadTime = deadTimeM1->text().toDouble();
    if (duration == 0.0)
    {
        deadTime = 0.0;
        duration = 1.0;
    }
    return M1 / (1 - deadTime / duration * M1);
}
//+++ read  Monitor2
double Monitors::readMonitor2(const QString &Number, double deadTime, const QStringList &lst)
{
    double M2 = parserHeader->readNumberString(Number, "[Monitor-2]", lst).toDouble();
    double duration = readDuration(Number);

    if (deadTime < 0)
        deadTime = deadTimeM2->text().toDouble();
    if (duration == 0.0)
    {
        deadTime = 0.0;
        duration = 1.0;
    }
    return M2 / (1 - deadTime / duration * M2);
}
//+++ read  Monitor3
double Monitors::readMonitor3(const QString &Number, double deadTime, const QStringList &lst)
{
    double M3 = parserHeader->readNumberString(Number, "[Monitor-3|Tr|ROI]", lst).toDouble();
    double duration = readDuration(Number);

    if (deadTime < 0)
        deadTime = deadTimeM3->text().toDouble();
    if (duration == 0.0)
    {
        deadTime = 0.0;
        duration = 1.0;
    }
    return M3 / (1 - deadTime / duration * M3);
}
//+++ read Normalized Monitor3
double Monitors::readMonitor3Normalized(const QString &Number, const QStringList &lst)
{
    double norm = normalizationFactor(Number, lst);
    double M3 = readMonitor3(Number, -1.0, lst);
    return M3 * norm;
}
//+++ read :: [Sum]
double Monitors::readSum(const QString &Number, double deadTime, const QStringList &lst)
{
    double sum = parserHeader->readNumberString(Number, "[Sum]", lst).toDouble();
    if (deadTime == 0.0)
        return sum;
    if (deadTime < 0)
        deadTime = deadTimeDetector->text().toDouble();
    double duration = readDuration(Number, lst);
    if (duration == 0)
        return sum;
    return sum * deadTimeFactor(sum / duration, deadTime);
}
//+++ exp- dead-time
double Monitors::deadTimeFactor(double I, double tau)
{
    if (tau <= 0.0)
        return 1.0;
    if (I == 0.0)
        return 0.0;

    if (deadTimeDetectorParalysable->isChecked())
    {
        if (I > (0.99 / tau))
            return 100.0;

        return 1.0 / (1.0 - tau * I);
    }

    if (I > (1 / tau / exp(1.0)))
        return exp(1.0);

    int status, status1;
    int iter = 0, max_iter = 100;

    const gsl_root_fdfsolver_type *T;
    gsl_root_fdfsolver *s;

    double x0, x = I;

    gsl_function_fdf FDF;

    struct deadTimeRoot_params params = {I, tau};
    FDF.f = &deadTimeRoot_f;
    FDF.df = &deadTimeRoot_deriv;
    FDF.fdf = &deadTimeRoot_fdf;
    FDF.params = &params;

    T = gsl_root_fdfsolver_newton;
    s = gsl_root_fdfsolver_alloc(T);
    gsl_root_fdfsolver_set(s, &FDF, x);

    do
    {
        iter++;
        status1 = gsl_root_fdfsolver_iterate(s);
        x0 = x;
        x = gsl_root_fdfsolver_root(s);
        status = gsl_root_test_delta(x, x0, 0.0, 1e-5);
    } while (status1 == GSL_CONTINUE && status == GSL_CONTINUE && iter < max_iter);

    double res = x / I;

    gsl_root_fdfsolver_free(s);
    return res;
}
//+++ Dead Time Correction factor (detector)
double Monitors::deadTimeFactorDetector(const QString &Number)
{
    double deadTime = deadTimeDetector->text().toDouble();
    return deadTimeFactor(readSum(Number) / readDuration(Number), deadTime);
}
//+++ Dead Time Correction factor (DB)
double Monitors::deadTimeFactorDetectorDB(const QString &Number)
{
    double deadTime = deadTimeDetectorDB->text().toDouble();
    return deadTimeFactor(readSum(Number) / readDuration(Number), deadTime);
}
//+++ Normalization Factor
double Monitors::normalizationFactor(const QString &Number, const QStringList &lst)
{
    double normConstant = normalizationConst->value();
    if (normalizationType->currentText() == "Monitor2")
    {
        double M2 = readMonitor2(Number, -1, lst);
        if (M2 > 0.0)
            return normConstant / M2;
    }
    else if (normalizationType->currentText() == "Monitor1")
    {
        double M1 = readMonitor1(Number, -1, lst);
        if (M1 > 0.0)
            return normConstant / M1;
    }
    else
    {
        double time = readDuration(Number, lst);
        if (time > 0.0)
            return normConstant / time;
    }

    //+++
    std::cout << "DAN:: " << Number.toLatin1().constData() << " ---> check normalization!\n";

    return 0.0;
}
//+++ Timefactor  [1]
double Monitors::readTimefactor(const QString &Number, const QStringList &lst)
{
    double readTimefactor = parserHeader->readNumberString(Number, "[RT-Time-Factor]", lst).toDouble();
    if (readTimefactor <= 0)
        return 1.0;
    return readTimefactor;
}
//+++ Timefactor numberRepetitions [1]
int Monitors::readNumberRepetitions(const QString &Number, const QStringList &lst)
{
    int numberRepetitions = parserHeader->readNumberString(Number, "[RT-Number-Repetitions]", lst).toInt();
    if (numberRepetitions < 1)
        return 1;
    return numberRepetitions;
}
//++++ Normalization Factor RT
double Monitors::normalizationFactorRT(const QString &Number, const QStringList &lst)
{
    double timeFactor = readTimefactor(Number, lst);
    if (timeFactor == 1.0)
        return 1.0;
    int numberRepetitions = readNumberRepetitions(Number, lst);

    return timeFactor / numberRepetitions;
}
