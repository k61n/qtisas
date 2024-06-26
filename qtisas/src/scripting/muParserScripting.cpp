/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Evaluate mathematical expression using muParser
 ******************************************************************************/

#include <QLocale>

#include "muParserScripting.h"


using namespace mu;

const char* muParserScripting::langName = "muParser";

const muParserScripting::mathFunction muParserScripting::math_functions[] = {
    {"abs", 1, nullptr, nullptr, nullptr, QObject::tr("abs(x):\n Absolute value of x.")},
    {"acos", 1, nullptr, nullptr, nullptr, QObject::tr("acos(x):\n Inverse cos function.")},
    {"acosh", 1, nullptr, nullptr, nullptr, QObject::tr("acosh(x):\n Hyperbolic inverse cos function.")},
    {"asin", 1, nullptr, nullptr, nullptr, QObject::tr("asin(x):\n Inverse sin function.")},
    {"asinh", 1, nullptr, nullptr, nullptr, QObject::tr("asinh(x):\n Hyperbolic inverse sin function.")},
    {"atan", 1, nullptr, nullptr, nullptr, QObject::tr("atan(x):\n Inverse tan function.")},
    {"atanh", 1, nullptr, nullptr, nullptr, QObject::tr("atanh(x):\n  Hyperbolic inverse tan function.")},
    {"avg", -1, nullptr, nullptr, nullptr, QObject::tr("avg(x,y,...):\n Mean value of all arguments.")},
    {"bessel_i0", 1, bessel_I0, nullptr, nullptr,
     QObject::tr("bessel_i0(x):\n Regular modified cylindrical Bessel function of zeroth order, I_0(x).")},
    {"bessel_i1", 1, bessel_I1, nullptr, nullptr,
     QObject::tr("bessel_i1(x):\n Regular modified cylindrical Bessel function of first order, I_1(x).")},
    {"bessel_in", 2, nullptr, bessel_In, nullptr,
     QObject::tr("bessel_in(double x, int n):\n Regular modified cylindrical Bessel function of order n, I_n(x).")},
    {"bessel_j0", 1, bessel_J0, nullptr, nullptr,
     QObject::tr("bessel_j0(x):\n Regular cylindrical Bessel function of zeroth order, J_0(x).")},
    {"bessel_j1", 1, bessel_J1, nullptr, nullptr,
     QObject::tr("bessel_j1(x):\n Regular cylindrical Bessel function of first order, J_1(x).")},
    {"bessel_jn", 2, nullptr, bessel_Jn, nullptr,
     QObject::tr("bessel_jn(double x, int n):\n Regular cylindrical Bessel function of order n, J_n(x).")},
    {"bessel_jn_zero", 2, nullptr, bessel_Jn_zero, nullptr,
     QObject::tr("bessel_jn_zero(double n, unsigned int s):\n s-th positive zero x_s of regular cylindrical Bessel "
                 "function of order n, J_n(x_s)=0")},
    {"bessel_y0", 1, bessel_Y0, nullptr, nullptr,
     QObject::tr("bessel_y0(x):\n Irregular cylindrical Bessel function of zeroth order, Y_0(x), for x>0.")},
    {"bessel_y1", 1, bessel_Y1, nullptr, nullptr,
     QObject::tr("bessel_y1(x):\n Irregular cylindrical Bessel function of first order, Y_1(x), for x>0.")},
    {"bessel_yn", 2, nullptr, bessel_Yn, nullptr,
     QObject::tr("bessel_yn(double x, int n):\n Irregular cylindrical Bessel function of order n, Y_n(x), for x>0.")},
    {"beta", 2, nullptr, beta, nullptr,
     QObject::tr("beta(a,b):\n Computes the Beta Function, B(a,b) = Gamma(a)*Gamma(b)/Gamma(a+b) for a > 0, b > 0.")},
    {"ceil", 1, ceil, nullptr, nullptr,
     QObject::tr("ceil(x):\n Round to the next larger integer,\n smallest integer larger or equal to x.")},
    {"chi2cdf", 2, nullptr, chi2cdf, nullptr,
     QObject::tr("chi2cdf(x, df):\n Computes the cumulative distribution function for a chi-squared distribution with "
                 "df degrees of freedom.")},
    {"chi2inv", 2, nullptr, chi2inv, nullptr,
     QObject::tr("chi2inv(p, df):\n Computes the inverse of the cumulative distribution function for a chi-squared "
                 "distribution with df degrees of freedom.")},
    {"cos", 1, nullptr, nullptr, nullptr, QObject::tr("cos(x):\n Calculate cosine.")},
    {"cosh", 1, nullptr, nullptr, nullptr, QObject::tr("cosh(x):\n Hyperbolic cos function.")},
    {"dilog", 1, dilog, nullptr, nullptr,
     QObject::tr("dilog(x):\n Compute the dilogarithm for a real argument. In Lewin's notation this is Li_2(x), the "
                 "real part of the dilogarithm of a real x.")},
    {"erf", 1, erf, nullptr, nullptr, QObject::tr("erf(x):\n  The error function.")},
    {"erfc", 1, erfc, nullptr, nullptr, QObject::tr("erfc(x):\n Complementary error function erfc(x) = 1 - erf(x).")},
    {"erfz", 1, erf_Z, nullptr, nullptr, QObject::tr("erfz(x):\n The Gaussian probability density function Z(x).")},
    {"erfq", 1, erf_Q, nullptr, nullptr,
     QObject::tr("erfq(x):\n The upper tail of the Gaussian probability function Q(x).")},
    {"exp", 1, nullptr, nullptr, nullptr, QObject::tr("exp(x):\n Exponential function: e raised to the power of x.")},
    {"floor", 1, floor, nullptr, nullptr,
     QObject::tr("floor(x):\n Round to the next smaller integer,\n largest integer smaller or equal to x.")},
    {"ftable", 3, nullptr, nullptr, ftable,
     QObject::tr("ftable(x, m, n):\n F-Distribution with m and n degrees of freedom.")},
    {"gamma", 1, gamma, nullptr, nullptr,
     QObject::tr("gamma(x):\n Computes the Gamma function, subject to x not being a negative integer.")},
    {"gammaln", 1, lngamma, nullptr, nullptr,
     QObject::tr("gammaln(x):\n Computes the logarithm of the Gamma function, subject to x not a being negative "
                 "integer. For x<0, log(|Gamma(x)|) is returned.")},
    {"gauss_cdf", 2, nullptr, gauss_cdf, nullptr,
     QObject::tr("gauss_cdf(x, sigma):\n Computes the cumulative distribution function for a Gaussian distribution "
                 "with standard deviation sigma.")},
    {"gauss_pdf", 2, nullptr, gauss_pdf, nullptr,
     QObject::tr("gauss_pdf(x, sigma):\n Computes the probability density at x for a Gaussian distribution with "
                 "standard deviation sigma.")},
    {"hazard", 1, hazard, nullptr, nullptr,
     QObject::tr("hazard(x):\n Computes the hazard function for the normal distribution h(x) = erfz(x)/erfq(x).")},
    {"if", 3, nullptr, nullptr, nullptr, QObject::tr("if(e1, e2, e3):\n if e1 then e2 else e3.")},
    {"inv_gauss_cdf", 2, nullptr, inv_gauss_cdf, nullptr,
     QObject::tr("inv_gauss_cdf(x, sigma):\n Computes the inverse of the cumulative distribution function for a "
                 "Gaussian distribution with standard deviation sigma.")},
    {"invf", 3, nullptr, nullptr, invf,
     QObject::tr("invf(x, m, n):\n Inverse F-Distribution with m and n degrees of freedom.")},
    {"invt", 2, nullptr, invt, nullptr, QObject::tr("invt(x, n):\n Inverse t-distribution with n degrees of freedom.")},
    {"ln", 1, nullptr, nullptr, nullptr, QObject::tr("ln(x):\n Calculate natural logarithm log_e.")},
    {"log", 1, nullptr, nullptr, nullptr, QObject::tr("log(x):\n Calculate decimal logarithm log_10.")},
    {"log10", 1, nullptr, nullptr, nullptr, QObject::tr("log10(x):\n Calculate decimal logarithm log_10.")},
    {"log2", 1, nullptr, nullptr, nullptr, QObject::tr("log2(x):\n Calculate binary logarithm log_2.")},
    {"min", -1, nullptr, nullptr, nullptr, QObject::tr("min(x,y,...):\n Calculate minimum of all arguments.")},
    {"max", -1, nullptr, nullptr, nullptr, QObject::tr("max(x,y,...):\n Calculate maximum of all arguments.")},
    {"mod", 2, nullptr, mod, nullptr, QObject::tr("mod(x,y):\n Calculate rest of integer division x/y,\n x modulo y.")},
    {"normal", 1, normal, nullptr, nullptr,
     QObject::tr("normal(seed):\n Returns a normal random number using the Ziggurat algorithm.")},
    {"normcdf", 1, normcdf, nullptr, nullptr,
     QObject::tr("normcdf(x):\n Computes the cumulative distribution function for a Gaussian distribution with "
                 "standard deviation sigma = 1.0.")},
    {"norminv", 1, norminv, nullptr, nullptr,
     QObject::tr("norminv(p):\n Computes the inverse of the cumulative distribution function for a Gaussian "
                 "distribution with standard deviation sigma = 1.0.")},
    {"pow", 2, nullptr, pow, nullptr, QObject::tr("pow(x,y):\n Raise x to the power of y, x^y.")},
    {"rint", 1, nullptr, nullptr, nullptr, QObject::tr("rint(x):\n Round to nearest integer.")},
    {"rnd", 1, rnd, nullptr, nullptr,
     QObject::tr("rnd(seed):\n Returns a pseudo-random number in the range 0 to 1. Calling this function several times "
                 "with the same seed will return the same value.")},
    {"sign", 1, nullptr, nullptr, nullptr, QObject::tr("sign(x):\n Sign function: -1 if x<0; 1 if x>0.")},
    {"sin", 1, nullptr, nullptr, nullptr, QObject::tr("sin(x):\n Calculate sine.")},
    {"sinh", 1, nullptr, nullptr, nullptr, QObject::tr("sinh(x):\n Hyperbolic sin function.")},
    {"sqrt", 1, nullptr, nullptr, nullptr, QObject::tr("sqrt(x):\n Square root function.")},
    {"sum", -1, nullptr, nullptr, nullptr, QObject::tr("sum(x,y,...):\n Calculate sum of all arguments.")},
    {"tan", 1, nullptr, nullptr, nullptr, QObject::tr("tan(x):\n Calculate tangent function.")},
    {"tanh", 1, nullptr, nullptr, nullptr, QObject::tr("tanh(x):\n Hyperbolic tan function.")},
    {"ttable", 2, nullptr, ttable, nullptr,
     QObject::tr("ttable(x, n):\n Student's t-distribution with n degrees of freedom.")},
    {"w0", 1, lambert_W0, nullptr, nullptr,
     QObject::tr("w0(x):\n Compute the principal branch of Lambert's W function, W_0(x).\n W is defined as a solution "
                 "to the equation W(x)*exp(W(x))=x.\n For x<0, there are two real-valued branches; this function "
                 "computes the one where W>-1 for x<0 (also see wm1(x)).")},
    {"wm1", 1, lambert_Wm1, nullptr, nullptr,
     QObject::tr("wm1(x):\n Compute the secondary branch of Lambert's W function, W_{-1}(x).\n W is defined as a "
                 "solution to the equation W(x)*exp(W(x))=x.\n For x<0, there are two real-valued branches; this "
                 "function computes the one where W<-1 for x<0. (also see w0(x)).")},
    {nullptr, 0, nullptr, nullptr, nullptr, nullptr}};

const QStringList muParserScripting::mathFunctions() const
{
    return functionsList();
}

QString muParserScripting::mathFunctionDoc(const QString &name) const
{
    return explainFunction(name);
}

const QStringList muParserScripting::functionsList(bool tableContext)
{
    QStringList l;
    if (tableContext)
    {
        l << "AVG";
        l << "MAX";
        l << "MIN";
        l << "SUM";
    }

    for (const mathFunction *i = math_functions; i->name; i++)
        l << i->name;

    return l;
}

const QString muParserScripting::explainFunction(const QString &name)
{
    if (name == "AVG")
        return QObject::tr("AVG(\"colName\", i, j):\n The average of all cells from row i to j in column colName.");
    if (name == "MIN")
        return QObject::tr("MIN(\"colName\", i, j):\n The minimum of all cells from row i to j in column colName.");
    if (name == "MAX")
        return QObject::tr("MAX(\"colName\", i, j):\n The maximum of all cells from row i to j in column colName.");
    if (name == "SUM")
        return QObject::tr("SUM(\"colName\", i, j):\n The sum of all cells from row i to j in column colName.");

    for (const mathFunction *i = math_functions; i->name; i++)
        if (name == i->name)
        {
            QString s = i->description;
            if (QLocale().decimalPoint() == ',')
                s.replace(",", ";");
            return s;
        }
    return {};
}
