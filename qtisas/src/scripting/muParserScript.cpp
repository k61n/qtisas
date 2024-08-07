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

#include <QApplication>
#include <QStringList>

#include <gsl/gsl_math.h>

#include "ApplicationWindow.h"
#include "Folder.h"
#include "Matrix.h"
#include "muParserScript.h"
#include "muParserScripting.h"
#include "Table.h"

using namespace mu;

muParserScript::muParserScript(ScriptingEnv *env, const QString &code, QObject *context, const QString &name)
  : Script(env, code, context, name)
{
  if (QString(Context->metaObject()->className()) == "Table") {
	  parser.DefineFun("col", mu_col, false);
	  parser.DefineFun("cell", mu_tableCell);
	  parser.DefineFun("tablecol", mu_tablecol, false);
	  parser.DefineFun("AVG", mu_avg, false);
	  parser.DefineFun("SUM", mu_sum, false);
	  parser.DefineFun("MIN", mu_min, false);
	  parser.DefineFun("MAX", mu_max, false);
  } else if (QString(Context->metaObject()->className()) == "Matrix")
	  parser.DefineFun("cell", mu_cell);

	parser.addGSLConstants();
	rparser = parser;
	if (Context->inherits("Table") || QString(Context->metaObject()->className()) == "Matrix"){
		connect(this, SIGNAL(error(const QString&,const QString&,int)), env, SIGNAL(error(const QString&,const QString&,int)));
		connect(this, SIGNAL(print(const QString&)), env, SIGNAL(print(const QString&)));
		if (code.count("\n") > 0){//autodetect new variables only for scripts having minimum 2 lines
			parser.SetVarFactory(mu_addVariable);
			rparser.SetVarFactory(mu_addVariableR);
		}
	} else {
		parser.SetVarFactory(mu_addVariable);
		rparser.SetVarFactory(mu_addVariableR);
	}
}

double muParserScript::col(const QString &arg)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("col() works only on tables!").toLocal8Bit().constData());
	QStringList items;
	QString item = "";

	for (int i=0; i < arg.size(); i++) {
		if (arg[i] == '"') {
			item += "\"";
			for (i++; i < arg.size() && arg[i] != '"'; i++)
				if (arg[i] == '\\') {
					item += "\\";
					item += arg[++i];
				} else
					item += arg[i];
				item += "\"";
		} else if (arg[i] == ',') {
			items << item;
			item = "";
		} else
			item += arg[i];
	}
	items << item;

	Table *table = (Table*) Context;
	int col, row;
	Parser local_parser(rparser);
	if (items[0].startsWith("\"") && items[0].endsWith("\"")) {
		col = table->colNames().indexOf(items[0].mid(1,items[0].length()-2));
		if (col<0)
            throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                             .arg(items[0], Context->objectName())
                                             .toLocal8Bit()
                                             .constData());
	} else
		col = items[0].toInt() - 1; //use column index

	if (items.count() == 2)
	{

        local_parser.SetExpr(items[1].toLocal8Bit().constData());
		if (local_parser.Eval()==0) return 0; 
		row = qRound(local_parser.Eval()) - 1;
	} else if (((int) *variables["i"])>0)
		row = (int) *(variables["i"]) - 1;
	else
		return 0;
	rvariables.clear();


	if (row < 0 || row >= table->numRows())
        throw Parser::exception_type(
            tr("There's no row %1 in table %2!").arg(row + 1).arg(Context->objectName()).toLocal8Bit().constData());
	if (col < 0 || col >= table->numCols())
        throw Parser::exception_type(
            tr("There's no column %1 in table %2!").arg(col + 1).arg(Context->objectName()).toLocal8Bit().constData());
	if (table->text(row, col).isEmpty())
		throw new EmptySourceError();
	else
		return table->cell(row,col);
}

double muParserScript::tablecol(const QString &arg)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("tablecol() works only on tables!").toLocal8Bit().constData());
	QStringList items;
	QString item = "";
	for (int i=0; i < arg.size(); i++) {
		if (arg[i] == '"') {
			item += "\"";
			for (i++; i < arg.size() && arg[i] != '"'; i++)
				if (arg[i] == '\\') {
					item += "\\";
					item += arg[++i];
				} else
					item += arg[i];
				item += "\"";
		} else if (arg[i] == ',') {
			items << item;
			item = "";
		} else
			item += arg[i];
	}
	items << item;
	Table *this_table = (Table*) Context;
	int col, row;
	Parser local_parser(rparser);
	if (items.count() != 2)
        throw Parser::exception_type(
            tr("tablecol: wrong number of arguments (need 2, got %1)").arg(items.count()).toLocal8Bit().constData());
	if (!items[0].startsWith("\"") || !items[0].endsWith("\""))
        throw Parser::exception_type(
            tr("tablecol: first argument must be a string (table name)").toLocal8Bit().constData());
	Table *target_table = scriptingEnv()->application()->table(items[0].mid(1, items[0].length()-2));
	if (!target_table)
        throw Parser::exception_type(tr("Couldn't find a table named %1.").arg(items[0]).toLocal8Bit().constData());

	QString arg2 = items[1].trimmed();
	if (arg2.startsWith("\"") && arg2.endsWith("\"")){
		col = target_table->colIndex(arg2.mid(1, arg2.length() - 2));
		if (col < 0)
            throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                             .arg(arg2)
                                             .arg(target_table->name())
                                             .toLocal8Bit()
                                             .constData());
	} else {
        local_parser.SetExpr(arg2.toLocal8Bit().constData());
		col = qRound(local_parser.Eval()) - 1;
	}
	if (variables["i"])
		row = (int) *(variables["i"]) - 1;
	else
		row = -1;
	rvariables.clear();
	if (row < 0 || row >= target_table->numRows())
        throw Parser::exception_type(
            tr("There's no row %1 in table %2!").arg(row + 1).arg(target_table->name()).toLocal8Bit().constData());
	if (col < 0 || col >= target_table->numCols())
        throw Parser::exception_type(
            tr("There's no column %1 in table %2!").arg(col + 1).arg(target_table->name()).toLocal8Bit().constData());
	if (target_table->text(row,col).isEmpty())
		throw new EmptySourceError();
	else
		return target_table->cell(row,col);
}

double muParserScript::cell(int row, int col)
{
	if (QString(Context->metaObject()->className()) != "Matrix")
        throw Parser::exception_type(tr("cell() works only on tables and matrices!").toLocal8Bit().constData());
	Matrix *matrix = (Matrix*) Context;
	if (row < 1 || row > matrix->numRows())
        throw Parser::exception_type(
            tr("There's no row %1 in matrix %2!").arg(row).arg(Context->objectName()).toLocal8Bit().constData());
	if (col < 1 || col > matrix->numCols())
        throw Parser::exception_type(
            tr("There's no column %1 in matrix %2!").arg(col).arg(Context->objectName()).toLocal8Bit().constData());
	if (matrix->text(row - 1,col - 1).isEmpty())
		throw new EmptySourceError();
	else
		return matrix->cell(row - 1, col - 1);
}

double muParserScript::tableCell(int col, int row)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("cell() works only on tables and matrices!").toLocal8Bit().constData());
	Table *table = (Table*) Context;
	if (row < 1 || row > table->numRows())
        throw Parser::exception_type(
            tr("There's no row %1 in table %2!").arg(row).arg(Context->objectName()).toLocal8Bit().constData());
	if (col < 1 || col > table->numCols())
        throw Parser::exception_type(
            tr("There's no column %1 in table %2!").arg(col).arg(Context->objectName()).toLocal8Bit().constData());
	if (table->text(row-1,col-1).isEmpty())
		throw new EmptySourceError();
	else
		return table->cell(row-1,col-1);
}

double *muParserScript::addVariable(const char *name)
{
	double *valptr = new double;
	if (!valptr)
        throw Parser::exception_type(tr("Out of memory").toLocal8Bit().constData());
	*valptr = 0;
	variables.insert(name, QSharedPointer<double>(valptr));
	rparser.DefineVar(name, valptr);
	return valptr;
}

double *muParserScript::addVariableR(const char *name)
{
	double *valptr = new double;
	if (!valptr)
        throw Parser::exception_type(tr("Out of memory").toLocal8Bit().constData());
	*valptr = 0;
	rvariables.insert(name, QSharedPointer<double>(valptr));
	return valptr;
}

double* muParserScript::defineVariable(const char *name, double val)
{
  double *valptr = variables[name].data();
  if (!valptr)
  {
    valptr = new double;
    if (!valptr)
    {
      emit_error(tr("Out of memory"), 0);
      return 0;
    }
    try {
      parser.DefineVar(name, valptr);
      rparser.DefineVar(name, valptr);
      variables.insert(name, QSharedPointer<double>(valptr));
    } catch (mu::ParserError &e) {
      delete valptr;
      emit_error(QString(e.GetMsg().c_str()), 0);
      return valptr;
    }
  }
  *valptr = val;
  return valptr;
}

bool muParserScript::setDouble(double val, const char *name)
{
  double *valptr = variables[name].data();
  if (!valptr)
  {
    valptr = new double;
    if (!valptr)
    {
      emit_error(tr("Out of memory"),0);
      return false;
    }
    try {
      parser.DefineVar(name, valptr);
      rparser.DefineVar(name, valptr);
      variables.insert(name, QSharedPointer<double>(valptr));
    } catch (mu::ParserError &e) {
      delete valptr;
      emit_error(QString(e.GetMsg().c_str()), 0);
      return false;
    }
  }
  *valptr = val;
  return  true;
}

bool muParserScript::setInt(int val, const char *name)
{
  return setDouble((double) val, name);
}

bool muParserScript::setQObject(QObject*, const char*)
{
  return false;
}

QString muParserScript::compileColArg(const QString &in)
{
	QString out = "\"";
	for (int i=0; i < in.size(); i++)
		if (in[i] == 'c' && in.mid(i,4)=="col(") {
			out += "col(";
			QString arg = "";
			int paren = 1;
			for (i+=4; i < in.size() && paren > 0; i++)
				if (in[i] == '"') {
					arg += "\"";
					for (i++; i < in.size() && in[i] != '"'; i++)
						if (in[i] == '\\') {
							arg += "\\";
							arg += in[++i];
						} else
							arg += in[i];
					arg += "\"";
				} else if (in[i] == '(') {
					paren++;
					arg += "(";
				} else if (in[i] == ')') {
					paren--;
					if(paren>0) arg += ")";
				}
				else
					arg += in[i];
			out += compileColArg(arg).replace('"',"\\\"");
			out += ")";
			i--;
		}
		else if (in[i] == '\\')
			out += "\\\\";
		else if (in[i] == '"')
			out += "\\\"";
		else
			out += in[i];
	return out + "\"";
}

bool muParserScript::compile(bool)
{
	muCode.clear();
	QString muCodeLine = "";
	for (int i=0; i < Code.size(); i++){
		if (Code[i] == 'c' && Code.mid(i,4)=="col(") {
			muCodeLine += "col(";
			QString arg = "";
			int paren = 1;
			for (i+=4; i < Code.size() && paren > 0; i++)
				if (Code[i] == '"') {
					arg += "\"";
					for (i++; Code[i] != '"' && i < Code.size(); i++)
						if (Code[i] == '\\') {
							arg += "\\";
							arg += Code[++i];
						} else
							arg += Code[i];
					arg += "\"";
				} else if (Code[i] == '(') {
					paren++;
					arg += "(";
				} else if (Code[i] == ')') {
					paren--;
					if(paren>0) arg += ")";
				}
				else
					arg += Code[i];
			muCodeLine += compileColArg(arg);
			muCodeLine += ")";
			i--;
		} else if (Code[i] == '#')
                        for (i++; Code[i] != '\n' && i < Code.size(); i++) ;
		else if (Code[i] == '\n') {
			muCodeLine = muCodeLine.trimmed();
			if (!muCodeLine.isEmpty())
				muCode += muCodeLine;
			muCodeLine = "";
		} else
			muCodeLine += Code[i];
	}

	muCodeLine = muCodeLine.trimmed();
	if (!muCodeLine.isEmpty())
		muCode += muCodeLine;
	compiled = Script::isCompiled;

	if (muCode.size() == 1){
	    current = this;
        parser.SetExpr(muCode[0].toLocal8Bit().constData());

        try {
			parser.Eval();
		} catch (EmptySourceError *e) {
		    QApplication::restoreOverrideCursor();
            return false;
        } catch (mu::ParserError &e) {
            if (e.GetCode() != ecVAL_EXPECTED){
                QApplication::restoreOverrideCursor();
                emit_error(e.GetMsg().c_str(), 0);
                return false;
            }
        }
	}
	return true;
}

QString muParserScript::evalSingleLineToString(const QLocale& locale, char f, int prec)
{
    double val = 0.0;
    try {
        val = parser.Eval();
    } catch (EmptySourceError *e) {
        return "";
    } catch (ParserError &e) {
		return "";
	}
    return locale.toString(val, f, prec);
}

double muParserScript::evalSingleLine()
{
    double val = 0.0;
    try {
        val = parser.Eval();
    } catch (EmptySourceError *e) {
        return GSL_NAN;
    } catch (ParserError &e) {
		return GSL_NAN;
	}
    return val;
}

muParserScript *muParserScript::current = nullptr;

QVariant muParserScript::eval()
{
	if (compiled != Script::isCompiled && !compile())
		return QVariant();
	double val = 0.0;
	try {
		current = this;
		for (QStringList::iterator i=muCode.begin(); i != muCode.end(); i++) {
            parser.SetExpr(i->toLocal8Bit().constData());
			val = parser.Eval();
		}
	} catch (EmptySourceError *e) {
		return QVariant("");
	} catch (ParserError &e) {
		emit_error(e.GetMsg().c_str(), 0);
		return QVariant();
	}
	return QVariant(val);
}

bool muParserScript::exec()
{
	if (compiled != Script::isCompiled && !compile())
		return false;
	try {
		current = this;
		for (QStringList::iterator i=muCode.begin(); i != muCode.end(); i++) {
            parser.SetExpr(i->toLocal8Bit().constData());
			parser.Eval();
		}
	} catch (EmptySourceError *e) {
		return true;
	} catch (mu::ParserError &e) {
		emit_error(e.GetMsg().c_str(), 0);
		return false;
	}
	return true;
}

double muParserScript::sum(const QString &arg, int start, int end)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("SUM() works only on tables!").toLocal8Bit().constData());

	Table *table = (Table*) Context;
	int col = table->colNames().indexOf(arg);
	if (col < 0)
        throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                         .arg(arg)
                                         .arg(Context->objectName())
                                         .toLocal8Bit()
                                         .constData());

	rvariables.clear();
	return table->sum(col, start, end);
}

double muParserScript::avg(const QString &arg, int start, int end)
{
	if (QString(Context->metaObject()->className()) == "Table")
        throw Parser::exception_type(tr("AVG() works only on tables!").toLocal8Bit().constData());

	Table *table = (Table*) Context;
	int col = table->colNames().indexOf(arg);
	if (col < 0)
        throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                         .arg(arg)
                                         .arg(Context->objectName())
                                         .toLocal8Bit()
                                         .constData());

	rvariables.clear();
	return table->avg(col, start, end);
}

double muParserScript::min(const QString &arg, int start, int end)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("MIN() works only on tables!").toLocal8Bit().constData());

	Table *table = (Table*) Context;
	int col = table->colNames().indexOf(arg);
	if (col < 0)
        throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                         .arg(arg)
                                         .arg(Context->objectName())
                                         .toLocal8Bit()
                                         .constData());

	rvariables.clear();
	return table->minColumnValue(col, start, end);
}

double muParserScript::max(const QString &arg, int start, int end)
{
	if (QString(Context->metaObject()->className()) != "Table")
        throw Parser::exception_type(tr("MAX() works only on tables!").toLocal8Bit().constData());

	Table *table = (Table*) Context;
	int col = table->colNames().indexOf(arg);
	if (col < 0)
        throw Parser::exception_type(tr("There's no column named %1 in table %2!")
                                         .arg(arg)
                                         .arg(Context->objectName())
                                         .toLocal8Bit()
                                         .constData());

	rvariables.clear();
	return table->maxColumnValue(col, start, end);
}
