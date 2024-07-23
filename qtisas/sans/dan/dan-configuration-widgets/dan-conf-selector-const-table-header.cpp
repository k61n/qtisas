/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN configuration selector widget: const/table/header
 ******************************************************************************/

#include "dan-conf-selector-const-table-header.h"
#include "MyParser.h"

ConfigurationSelector::ConfigurationSelector(ParserHeader *parserHeaderDan, QRadioButton *selectConstDan,
                                             QRadioButton *selectTableDan, QRadioButton *selectHeaderDan,
                                             QDoubleSpinBox *spinBoxValueDan, QLineEdit *lineEditTableDan,
                                             const QString &qstringHeaderDan)
{
    parserHeader = parserHeaderDan;
    selectConst = selectConstDan;
    selectTable = selectTableDan;
    selectHeader = selectHeaderDan;
    spinBoxValue = spinBoxValueDan;
    lineEditTable = lineEditTableDan;
    qstringHeader = qstringHeaderDan;

    selectHeader->setToolTip("[" + qstringHeader + "]");

    if (qstringHeader == "")
        selectHeaderDan->hide();
    else
        selectHeaderDan->show();

    // connect
    connect(selectConst, SIGNAL(toggled(bool)), spinBoxValue, SLOT(setEnabled(bool)));
    connect(selectTable, SIGNAL(toggled(bool)), lineEditTable, SLOT(setEnabled(bool)));
}
double calculatetFunction(const QString &formula, double x)
{
    try
    {
        MyParser parser;
        parser.DefineVar("x", &x);
        parser.SetExpr(formula.toLocal8Bit().constData());
        return parser.Eval();
    }
    catch (mu::ParserError &e)
    {
        return -1001;
    }
}
//+++ Settings: readSettingsString
bool ConfigurationSelector::readSettingsString(QString string)
{
    string = string.simplified();
    QStringList lst = string.split(";;;");
    if (lst.count() != 3)
        return false;
    //+++ selected Mode
    int selectedMode = lst[0].toInt();
    if (selectedMode == 1)
        selectConst->setChecked(true);
    else if (selectedMode == 2)
        selectTable->setChecked(true);
    else if (selectedMode == 3)
        selectHeader->setChecked(true);
    //+++ constant value
    spinBoxValue->setValue(lst[1].toDouble());
    //+++ table value
    lineEditTable->setText(lst[2]);
    return true;
}
//+++ Settings: wrighteSettingsString
QString ConfigurationSelector::writeSettingsString()
{
    QString result;
    //+++ selected Mode
    if (selectConst && selectConst->isChecked())
        result = "1;;;";
    else if (selectTable->isChecked())
        result = "2;;;";
    else if (selectHeader->isChecked())
        result = "3;;;";
    else
        result = "0;;;";
    //+++ constant value
    result += QString::number(spinBoxValue->value()) + ";;;";
    //+++ table value
    result += lineEditTable->text();
    return result;
}
//+++ getValue
double ConfigurationSelector::getValue(double x, const QString &number)
{
    if (selectConst->isChecked())
        return spinBoxValue->value();

    if (selectHeader->isChecked())
    {
        if (number == "-1" || number == "" || qstringHeader == "")
            return spinBoxValue->value();

        double result = parserHeader->readNumberString(number, "[" + qstringHeader + "]").toDouble();
        if (result >= spinBoxValue->minimum() && result <= spinBoxValue->maximum())
            return result;
        return spinBoxValue->value();
    }

    QString txt = lineEditTable->text();

    if (txt.left(1) == "=")
    {
        double result = calculatetFunction(txt.remove("="), x);
        if (x == -1001 || (result < spinBoxValue->minimum() || result > spinBoxValue->maximum()))
            return spinBoxValue->value();
        return result;
    }

    QStringList lst = txt.split(";");

    if (x == -1001 || lst.count() == 0)
        return spinBoxValue->value();

    QList<double> xList, yList;

    for (int i = 0; i < lst.count(); i++)
    {
        QStringList valuesList = lst[i].split(",");
        if (valuesList.count() != 2)
            continue;

        double yValue = valuesList[1].toDouble();
        if (yValue < spinBoxValue->minimum() || yValue > spinBoxValue->maximum())
            continue;
        xList << valuesList[0].toDouble();
        yList << yValue;
    }
    if (xList.count() == 0)
        return spinBoxValue->value();

    double min = abs(x - xList[0]);
    double result = yList[0];
    for (int i = 1; i < xList.count(); i++)
        if (abs(x - xList[i]) < min)
        {
            min = abs(x - xList[i]);
            result = yList[i];
        }
    return result;
}