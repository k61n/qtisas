/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Parameters functions of compile interface
 ******************************************************************************/

#include "compile18.h"

//*******************************************
//+++ change XY info
//*******************************************
void compile18::changedFXYinfo()
{
    QString yText = lineEditY->text().remove(' ');
    QString xText = lineEditXXX->text().remove(' ');

    QString firstParaName = "P1";
    if (tableParaNames->rowCount() > 0 && !tableParaNames->item(0, 0)->text().isEmpty())
        firstParaName = tableParaNames->item(0, 0)->text();

    QString newStr = "// --->  " + yText + " = f ( " + xText + ", {" + firstParaName + ", ...} );";

    QStringList lines = textEditCode->toPlainText().split('\n');

    if (!lines.isEmpty() && lines[0].startsWith("// --->  "))
        lines[0] = newStr;
    else
        lines.prepend(newStr);

    textEditCode->setPlainText(lines.join('\n'));
}
//*******************************************
//+++ set number parameters in table
//*******************************************
void compile18::setNumberparameters(int paraNumber)
{
    int oldNumberParameters = tableParaNames->rowCount();
    tableParaNames->setRowCount(paraNumber);

    for (int i = oldNumberParameters; i < paraNumber; ++i)
    {
        auto checkItem = new QTableWidgetItem();
        checkItem->setCheckState(Qt::Unchecked);
        tableParaNames->setItem(i, 2, checkItem);

        for (int col : {0, 1, 3})
            tableParaNames->setItem(i, col, new QTableWidgetItem(""));

        QString label = QString("%1 ^  ").arg(i + 1, 3);
        tableParaNames->setVerticalHeaderItem(i, new QTableWidgetItem(label));
    }

    int hun = paraNumber / 100;
    int dec = (paraNumber % 100) / 10;
    int unit = paraNumber % 10;

    spinBoxParaHun->setValue(hun);
    spinBoxParaDec->setValue(dec);
    spinBoxPara->setValue(unit);

    spinBoxParaHun->setVisible(hun > 0);
    spinBoxParaDec->setVisible(hun > 0 || dec > 0);
}
//********************************************************************
//+++ change number of independent parameters: relevant for matrix fit
//********************************************************************
void compile18::changedNumberIndepvar(int newNumber)
{
    QString input = lineEditXXX->text();
    input.remove(' ');

    QStringList names = input.split(",", Qt::SkipEmptyParts);

    for (int i = static_cast<int>(names.count()); i < newNumber; ++i)
        names << "x" + QString::number(i);

    names = names.mid(0, newNumber);

    QString newText = names.join(",");

    QStringList maskParts;
    for (int i = 0; i < newNumber; ++i)
        maskParts << "nnnn";
    QString inputMask = maskParts.join(",") + ";";

    lineEditXXX->setInputMask(inputMask);
    lineEditXXX->setText(newText);
}
//*******************************************
//+++ move Para Line
//*******************************************
void compile18::moveParaLine(int line)
{
    int rowsNumber = spinBoxP->value();
    if (rowsNumber == 1)
        return;

    int oldPos = line;
    int newPos = line - 1;
    if (newPos < 0)
        newPos = rowsNumber - 1;

    QString s;
    s = tableParaNames->item(newPos, 0)->text();
    tableParaNames->item(newPos, 0)->setText(tableParaNames->item(oldPos, 0)->text());
    tableParaNames->item(oldPos, 0)->setText(s);

    s = tableParaNames->item(newPos, 1)->text();
    tableParaNames->item(newPos, 1)->setText(tableParaNames->item(oldPos, 1)->text());
    tableParaNames->item(oldPos, 1)->setText(s);

    s = tableParaNames->item(newPos, 3)->text();
    tableParaNames->item(newPos, 3)->setText(tableParaNames->item(oldPos, 3)->text());
    tableParaNames->item(oldPos, 3)->setText(s);

    Qt::CheckState oldChecked = tableParaNames->item(oldPos, 2)->checkState();
    s = tableParaNames->item(oldPos, 2)->text();
    tableParaNames->item(oldPos, 2)->setCheckState(tableParaNames->item(newPos, 2)->checkState());
    tableParaNames->item(oldPos, 2)->setText(tableParaNames->item(newPos, 2)->text());
    tableParaNames->item(newPos, 2)->setCheckState(oldChecked);
    tableParaNames->item(newPos, 2)->setText(s);
}
//*******************************************
//+++ selectRowsTableMultiFit
//*******************************************
void compile18::selectRowsTableMultiFit()
{
    for (int i = 0; i < tableParaNames->rowCount(); i++)
    {
        if (!tableParaNames->item(i, 2)->isSelected())
            continue;
        if (!tableParaNames->item(i, 3)->isSelected() && tableParaNames->item(i, 1)->isSelected())
            tableParaNames->item(i, 2)->setCheckState(Qt::Checked);

        if (tableParaNames->item(i, 3)->isSelected() && !tableParaNames->item(i, 1)->isSelected())
            tableParaNames->item(i, 2)->setCheckState(Qt::Unchecked);

        if (!tableParaNames->item(i, 3)->isSelected() && !tableParaNames->item(i, 1)->isSelected())
            tableParaNames->item(i, 2)->setCheckState(
                tableParaNames->item(i, 2)->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
}


