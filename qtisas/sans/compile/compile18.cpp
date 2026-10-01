/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compile interface
 ******************************************************************************/

#include <QDockWidget>
#include <QUrl>

#include "compile18.h"
#include "number-validator.h"
#include "numericLocale.h"

//*******************************************
//+++ compile18()
//*******************************************
compile18::compile18(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    initCompile();
    connectSlot();
    // the custom x-range: numbers only, in the numerical format, converted when it changes (may stay empty)
    for (QLineEdit *edit : {lineEditMin, lineEditMax, lineEditYmin})
        new NumberValidator(edit);
    d_widgetLocale = locale();
}
//*******************************************
//+++ ~compile18()
//*******************************************
compile18::~compile18()
{
}
//*******************************************
//+++ setPathExtern
//*******************************************
void compile18::setPathExtern(QString path)
{
    pathFIF = path;
    fitPath->setText(path);
}
//*******************************************
//+++ app
//*******************************************
ApplicationWindow *compile18::app()
{
    if (this->parentWidget()->parent())
        return (ApplicationWindow *)((QDockWidget *)this->parentWidget())->parent();
    return (ApplicationWindow *)this->parent();
}
//*******************************************
//+++ numbers in the Compile interface
//*******************************************
QLocale compile18::uiLocale() const
{
    return const_cast<compile18 *>(this)->app()->locale();
}

double compile18::uiToDouble(const QString &text, bool *ok) const
{
    double value = 0.0;
    const bool parsed = NumericLocale::toDouble(text, uiLocale(), value);
    if (ok)
        *ok = parsed;
    return value;
}

QString compile18::uiNumber(double value, char format, int precision) const
{
    return uiLocale().toString(value, format, precision);
}

QString compile18::uiToC(const QString &text) const
{
    return NumericLocale::convert(text, uiLocale(), QLocale::c());
}

QString compile18::cToUi(const QString &text) const
{
    return NumericLocale::convert(text, QLocale::c(), uiLocale());
}

QString compile18::rangeUiToC(const QString &text) const
{
    return NumericLocale::convertRange(text, uiLocale(), QLocale::c());
}

QString compile18::rangeCToUi(const QString &text) const
{
    return NumericLocale::convertRange(text, QLocale::c(), uiLocale());
}

QString compile18::algorithmUiToC(const QString &text) const
{
    return NumericLocale::replaceDecimalPoint(text, uiLocale(), QLocale::c());
}

QString compile18::algorithmCToUi(const QString &text) const
{
    return NumericLocale::replaceDecimalPoint(text, QLocale::c(), uiLocale());
}

void compile18::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::LocaleChange || locale() == d_widgetLocale)
        return;

    const QLocale from = d_widgetLocale, to = locale();
    const bool blocked = tableParaNames->blockSignals(true);
    for (int row = 0; row < tableParaNames->rowCount(); row++)
    {
        if (QTableWidgetItem *value = tableParaNames->item(row, 1))
            value->setText(NumericLocale::convert(value->text(), from, to));
        if (QTableWidgetItem *range = tableParaNames->item(row, 2))
            range->setText(NumericLocale::convertRange(range->text(), from, to));
    }
    tableParaNames->blockSignals(blocked);
    lineEditFitMethodPara->setText(NumericLocale::replaceDecimalPoint(lineEditFitMethodPara->text(), from, to));
    d_widgetLocale = to;
}

//*******************************************
//*** Log-output
//*******************************************
void compile18::toResLog(QString text)
{
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea &&
        !text.contains("<< compile status >>  ERROR:"))
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else
    {
        app()->showResults(text, true);
        app()->showResults(false);
        app()->showResults(true);
    }
}