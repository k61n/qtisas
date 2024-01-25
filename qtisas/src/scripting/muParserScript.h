/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Evaluate mathematical expressions using muParser
 ******************************************************************************/

#ifndef MUPARSER_SCRIPT_H
#define MUPARSER_SCRIPT_H

#include "ScriptingEnv.h"
#include "Script.h"

#include <MyParser.h>
#include "math.h"
#include <gsl/gsl_sf.h>
#include <QSharedPointer>

//! TODO
class muParserScript: public Script
{
  Q_OBJECT

  public:
    muParserScript(ScriptingEnv *env, const QString &code, QObject *context=0, const QString &name="<input>");

  public slots:
    bool compile(bool asFunction=true);
    QVariant eval();
    double evalSingleLine();
    QString evalSingleLineToString(const QLocale& locale, char f, int prec);
    bool exec();
    bool setQObject(QObject *val, const char *name);
    bool setInt(int val, const char* name);
    bool setDouble(double val, const char* name);
    double* defineVariable(const char *name, double val = 0.0);
    int codeLines(){return muCode.size();};

  private:
	double avg(const QString &arg, int start = 0, int end = -1);
	double sum(const QString &arg, int start = 0, int end = -1);
	double min(const QString &arg, int start = 0, int end = -1);
	double max(const QString &arg, int start = 0, int end = -1);
	double col(const QString &arg);
    double tablecol(const QString &arg);
    double cell(int row, int col);
    double tableCell(int col, int row);
    double *addVariable(const char *name);
    double *addVariableR(const char *name);
    static double *mu_addVariableR(const char *name) { return current->addVariableR(name); }
	static double mu_avg(const char *arg, double start = 1, double end = -1) {return current->avg(arg, qRound(start - 1), qRound(end - 1));}
	static double mu_sum(const char *arg, double start = 1, double end = -1) {return current->sum(arg, qRound(start - 1), qRound(end - 1));}
	static double mu_min(const char *arg, double start = 1, double end = -1) {return current->min(arg, qRound(start - 1), qRound(end - 1));}
	static double mu_max(const char *arg, double start = 1, double end = -1) {return current->max(arg, qRound(start - 1), qRound(end - 1));}
	static double mu_col(const char *arg) { return current->col(arg); }
    static double mu_cell(double row, double col) { return current->cell(qRound(row), qRound(col)); }
    static double mu_tableCell(double col, double row) { return current->tableCell(qRound(col), qRound(row)); }
    static double mu_tablecol(const char *arg) { return current->tablecol(arg); }
    static double *mu_addVariable(const char *name, void *){ return current->addVariable(name); }
    static double *mu_addVariableR(const char *name, void *) { return current->addVariableR(name); }
    static QString compileColArg(const QString& in);

    MyParser parser, rparser;
    QMap<QString, QSharedPointer<double>> variables, rvariables;
    QStringList muCode;

  public:
    static muParserScript *current;
};

#endif
