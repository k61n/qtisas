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
//+++
//*******************************************
void compile18::changedFXYinfo()
{
    
    QString newStr="// --->  "+lineEditY->text().remove(" ")+" = f ( "+lineEditXXX->text().remove(" ")+", {P1, ...} );";

    
    QString text=textEditCode->toPlainText();
    QStringList lst=text.split("\n");
    
    if (lst.count()>0 && lst[0].contains("// --->  ")) lst[0]=newStr;
    else lst.prepend(newStr);
    text="";
    for(int i=0; i<lst.count()-1;i++)text+=lst[i]+"\n";

    QTextCursor cursor = textEditCode->textCursor();
    cursor.movePosition( QTextCursor::End );
    textEditCode->setTextCursor( cursor );
    
    textEditCode->clear();
    textEditCode->append(text);
}

//*******************************************
//+++ set number parameters in table
//*******************************************
void compile18::setNumberparameters( int paraNumber )
{
    int i;
    int oldNumberParameters=tableParaNames->rowCount();
    tableParaNames->setRowCount(paraNumber);

    QString sPace="";
    for(i=oldNumberParameters;i<paraNumber;i++)
    {
        sPace=" ";
        if (i<9) sPace+=" ";
        if (i<99) sPace+=" ";
        //+++
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setCheckState(Qt::Unchecked);
        tableParaNames->setItem(i,2,item);
        
        
        tableParaNames->setItem(i,0,new QTableWidgetItem(""));
        tableParaNames->setItem(i,1,new QTableWidgetItem(""));
        tableParaNames->setItem(i,3,new QTableWidgetItem(""));
        
        tableParaNames->setVerticalHeaderItem(i, new QTableWidgetItem(sPace+QString::number(i+1)+" ^  "));
        
    }
    

    int hun=int(paraNumber/100);
    spinBoxParaHun->setValue(hun);
    if(hun==0) spinBoxParaHun->hide(); else spinBoxParaHun->show();
    int dec=int( (paraNumber-100*hun)/10);
    spinBoxParaDec->setValue(dec);
    if(dec==0 && hun==0) spinBoxParaDec->hide(); else spinBoxParaDec->show();
    spinBoxPara->setValue(paraNumber-hun*100-dec*10);
    
}

//*******************************************
//+++
//*******************************************
void compile18::changedNumberIndepvar(int newNumber)
{
    QString oldNames=lineEditXXX->text();
    oldNames.remove(" ");
    
    QStringList oldList = oldNames.split(",", Qt::SkipEmptyParts);
    
    if (oldList.count() < newNumber)
    {
        for (int i=oldList.count(); i<newNumber;i++) oldList<<"x"+QString::number(i+1-2);
    }
    
    QString newList;
    for (int i=0;i<newNumber-1;i++) newList+=oldList[i]+",";
    newList+=oldList[newNumber-1];
    
    QString newMask="nnnn";
    for (int i=1; i<newNumber;i++) newMask+=",nnnn";
    newMask+=";";
    
    lineEditXXX->setInputMask(newMask);
    lineEditXXX->setText(newList);

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
//+++ electRowsTableMultiFit
//*******************************************
void compile18::selectRowsTableMultiFit()
{
    int numRows=tableParaNames->rowCount();
    int i;
    for (i=0;i<numRows;i++)
    {
        if ( tableParaNames->item(i,2)->isSelected() && !tableParaNames->item(i,3)->isSelected()&& tableParaNames->item(i,1)->isSelected() ) {tableParaNames->item(i,2)->setCheckState(Qt::Checked); continue;}
        
        if ( tableParaNames->item(i,2)->isSelected() && tableParaNames->item(i,3)->isSelected()&& !tableParaNames->item(i,1)->isSelected() ) {tableParaNames->item(i,2)->setCheckState(Qt::Unchecked); continue;}
/*
        if ( tableParaNames->item(i,2)->isSelected() && !tableParaNames->item(i,3)->isSelected()&& !tableParaNames->item(i,1)->isSelected() )
        {

            if (tableParaNames->item(i,2)->checkState() == Qt::Checked)
                tableParaNames->item(i,2)->setCheckState(Qt::Unchecked);
            else
                tableParaNames->item(i,2)->setCheckState(Qt::Checked);
    }
 */
    }

    
}


