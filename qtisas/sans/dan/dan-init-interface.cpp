/***************************************************************************
 File : dan-init-interface.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2019-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface
 
 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/
#include "dan18.h"
/*
void dan18::initOptionUnits()
{
    //++ Units
    QTreeWidgetItem *units = new QTreeWidgetItem(treeWidgetProperties);
    units->setText(0, tr("[Units] Options"));
    treeWidgetProperties->setFirstItemColumnSpanned(units, true);
    
    //++++ Q-Lambda
    comboBoxUnitsLambda = new QComboBox();
    
    QString s="[";s+=QChar(955);s+="]=";s+=QChar(8491);s+=";  [Q]=1/"; s+=QChar(8491);
    comboBoxUnitsLambda->addItem(s);
    s="[";s+=QChar(955);s+="]=nm;  [Q]=1/"; s+=QChar(8491);
    comboBoxUnitsLambda->addItem(s);
    comboBoxUnitsLambda->setCurrentItem(0);
    QTreeWidgetItem *item001 = new QTreeWidgetItem(units);
    s=QChar(955);s+=" and Q";
    item001->setText(0,s);
    treeWidgetProperties->setItemWidget(item001, 1,comboBoxUnitsLambda);
    //++++ Intensity
    comboBoxUnitsOutput = new QComboBox();
    comboBoxUnitsOutput->addItem("1/cm");
    QTreeWidgetItem *item002 = new QTreeWidgetItem(units);
    item002->setText(0,"Intensity");
    treeWidgetProperties->setItemWidget(item002, 1,comboBoxUnitsOutput);
    //++++ Appertures
    comboBoxUnitsBlends = new QComboBox();
    comboBoxUnitsBlends->addItem("cm x cm");
    comboBoxUnitsBlends->addItem("mm x mm");
    s=QChar(181);s+="m x ";s+=QChar(181);s+="m";
    comboBoxUnitsBlends->addItem(s);
    comboBoxUnitsBlends->setCurrentItem(1);
    QTreeWidgetItem *item003 = new QTreeWidgetItem(units);
    item003->setText(0,"Appertures");
    treeWidgetProperties->setItemWidget(item003, 1,comboBoxUnitsBlends);
    //++++ Thickness
    comboBoxThicknessUnits = new QComboBox();
    comboBoxThicknessUnits->addItem("cm");
    comboBoxThicknessUnits->addItem("mm");
    comboBoxThicknessUnits->setCurrentItem(1);
    QTreeWidgetItem *item004 = new QTreeWidgetItem(units);
    item004->setText(0,"Thickness");
    treeWidgetProperties->setItemWidget(item004, 1,comboBoxThicknessUnits);
    //++++ C, D, Offset
    comboBoxUnitsCandD = new QComboBox();
    comboBoxUnitsCandD->addItem("m");
    comboBoxUnitsCandD->addItem("cm");
    comboBoxUnitsCandD->addItem("mm");
    comboBoxUnitsCandD->setCurrentItem(0);
    QTreeWidgetItem *item005 = new QTreeWidgetItem(units);
    item005->setText(0,"C, D, Offset");
    treeWidgetProperties->setItemWidget(item005, 1,comboBoxUnitsCandD);
    //++++ Time: Duration
    comboBoxUnitsTime = new QComboBox();
    comboBoxUnitsTime->addItem("sec");
    comboBoxUnitsTime->addItem("sec/10");
    comboBoxUnitsTime->addItem("msec");
    s=QChar(181);s+="sec";
    comboBoxUnitsTime->addItem(s);
    comboBoxUnitsTime->setCurrentItem(0);
    QTreeWidgetItem *item006 = new QTreeWidgetItem(units);
    item006->setText(0,"Time: Duration");
    treeWidgetProperties->setItemWidget(item006, 1,comboBoxUnitsTime);
    //++++ Time: TOF,RT
    comboBoxUnitsTimeRT = new QComboBox();
    comboBoxUnitsTimeRT->addItem("sec");
    comboBoxUnitsTimeRT->addItem("sec/10");
    comboBoxUnitsTimeRT->addItem("msec");
    s=QChar(181);s+="sec";
    comboBoxUnitsTimeRT->addItem(s);
    comboBoxUnitsTimeRT->setCurrentItem(0);
    QTreeWidgetItem *item007 = new QTreeWidgetItem(units);
    item007->setText(0,"Time: TOF,RT");
    treeWidgetProperties->setItemWidget(item007, 1,comboBoxUnitsTimeRT);
    //++++ Selector
    comboBoxUnitsSelector = new QComboBox();
    comboBoxUnitsSelector->addItem("Number");
    comboBoxUnitsSelector->addItem("Hz");
    comboBoxUnitsSelector->addItem("RPM");
    comboBoxUnitsSelector->setCurrentItem(0);
    QTreeWidgetItem *item008 = new QTreeWidgetItem(units);
    item008->setText(0,"Selector");
    treeWidgetProperties->setItemWidget(item008, 1,comboBoxUnitsSelector);
    
    treeWidgetProperties->addTopLevelItem(units);
    
}


void dan18::initOptionHeader()
{
    //+++++++++ Header
    QTreeWidgetItem *header = new QTreeWidgetItem(treeWidgetProperties);
    header->setText(0, "Header Map");
    treeWidgetProperties->setFirstItemColumnSpanned(header, true);
    treeWidgetProperties->addTopLevelItem(header);
    
    //++++ "Header Format"
    QTreeWidgetItem *item001 = new QTreeWidgetItem(header);
    item001->setText(0, "Header Format");
    
    comboBoxHeaderFormat = new QComboBox();
    
    comboBoxHeaderFormat->addItem("Free ASCII format [standart]");
    comboBoxHeaderFormat->addItem("XML format   [only header file]");
    comboBoxHeaderFormat->addItem("YAML format [only header file]");
    comboBoxHeaderFormat->setCurrentItem(0);
    connect(comboBoxHeaderFormat, SIGNAL( activated(int) ), this, SLOT( dataFormatSelected(int) ) );
    
    treeWidgetProperties->setItemWidget(item001, 1,comboBoxHeaderFormat);
    
    //++++ "Xml-Base"
    itemXmlBase = new QTreeWidgetItem(item001);
    itemXmlBase->setText(0, "Xml-Base");
    
    lineEditXMLbase = new QLineEdit();
    lineEditXMLbase->setText("");
    treeWidgetProperties->setItemWidget(itemXmlBase, 1,lineEditXMLbase);
    
    //++++ "Flexible Header"
    itemFlexiStop = new QTreeWidgetItem(item001);
    itemFlexiStop->setText(0, "\"Flexible\" Header");
    
    checkBoxHeaderFlexibility = new QCheckBox();
    checkBoxHeaderFlexibility->setChecked(false);
    checkBoxHeaderFlexibility->setText("Use: Yes/Not?");
    
    treeWidgetProperties->setItemWidget(itemFlexiStop, 1,checkBoxHeaderFlexibility);
    
    //++++ "Last Line"
    QTreeWidgetItem *item004 = new QTreeWidgetItem(itemFlexiStop);
    item004->setText(0, "Last Line");
    
    lineEditFlexiStop = new QLineEdit();
    lineEditFlexiStop->setText("$");
    treeWidgetProperties->setItemWidget(item004, 1,lineEditFlexiStop);
    
    //++++ "Header Table"
    QTreeWidgetItem *item005 = new QTreeWidgetItem(header);
    
    tableHeaderPos = new QTableWidget(54, 2, this);
    treeWidgetProperties->setItemWidget(item005, 0, tableHeaderPos);
    
    treeWidgetProperties->setFirstItemColumnSpanned(item005, true);
    
    QStringList lst;
    
    lst<<"[Experiment-Title]";
    lst<<"[User-Name]";
    lst<<"[Sample-Run-Number]";
    lst<<"[Sample-Title]";
    lst<<"[Sample-Thickness]";
    lst<<"[Sample-Position-Number]";
    lst<<"[Date]";
    lst<<"[Time]";
    lst<<"[C]";
    lst<<"[D]";
    lst<<"[D-TOF]";
    lst<<"[C,D-Offset]";
    lst<<"[CA-X]";
    lst<<"[CA-Y]";
    lst<<"[SA-X]";
    lst<<"[SA-Y]";
    lst<<"[Sum]";
    lst<<"[Selector]";
    lst<<"[Lambda]";
    lst<<"[Delta-Lambda]";
    lst<<"[Duration]";
    lst<<"[Duration-Factor]";
    lst<<"[Monitor-1]";
    lst<<"[Monitor-2]";
    lst<<"[Monitor-3|Tr|ROI]";
    lst<<"[Comment1]";
    lst<<"[Comment2]";
    lst<<"[Detector-X || Beamcenter-X]";
    lst<<"[Detector-Y || Beamcenter-Y]";
    lst<<"[Sample-Motor-1]";
    lst<<"[Sample-Motor-2]";
    lst<<"[Sample-Motor-3]";
    lst<<"[Sample-Motor-4]";
    lst<<"[Sample-Motor-5]";
    lst<<"[SA-Pos-X]";
    lst<<"[SA-Pos-Y]";
    lst<<"[Field-1]";
    lst<<"[Field-2]";
    lst<<"[Field-3]";
    lst<<"[Field-4]";
    lst<<"[RT-Number-Repetitions]";
    lst<<"[RT-Time-Factor]";
    lst<<"[RT-Current-Number]";
    lst<<"[Attenuator]";
    lst<<"[Polarization]";
    lst<<"[Lenses]";
    lst<<"[Slices-Count]";
    lst<<"[Slices-Duration]";
    lst<<"[Slices-Current-Number]";
    lst<<"[Slices-Current-Duration]";
    lst<<"[Slices-Current-Monitor1]";
    lst<<"[Slices-Current-Monitor2]";
    lst<<"[Slices-Current-Monitor3]";
    lst<<"[Slices-Current-Sum]";
    
    tableHeaderPos->setVerticalHeaderLabels(lst);
    
    dataFormatSelected(0);
}




void dan18::initOptionSelector()
{
    QString s=QChar(955);
    
    //+++++++++ Selector, Wavelength
    QTreeWidgetItem *selector = new QTreeWidgetItem(treeWidgetProperties);
    selector->setText(0, "["+s + "] extracting method");
    treeWidgetProperties->setFirstItemColumnSpanned(selector, true);
    treeWidgetProperties->addTopLevelItem(selector);
    
    //++++ "Method"
    QTreeWidgetItem *item001 = new QTreeWidgetItem(selector);
//    item001->setText(0, "Select Method to get WL");
    
    comboBoxSelector = new QComboBox();
    
    comboBoxSelector->addItem("["+s+"] Calculate from Selector Frequency");
    comboBoxSelector->addItem("["+s+"] Read from Header");
    comboBoxSelector->setCurrentItem(1);
    connect(comboBoxSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectLambdaMethod(int) ) );
    
    treeWidgetProperties->setItemWidget(item001, 0,comboBoxSelector);
    treeWidgetProperties->setFirstItemColumnSpanned(item001, true);
    
    //++++ "Equation"
    itemSelector1 = new QTreeWidgetItem(selector);
    itemSelector1->setText(0, s+"(f) = A/f + B, A=");
    lineEditSel1 = new QLineEdit();
    lineEditSel1->setText("2096");
    treeWidgetProperties->setItemWidget(itemSelector1, 1,lineEditSel1);
    
    
    itemSelector2 = new QTreeWidgetItem(selector);
    itemSelector2->setText(0, s+"(f) = A/f + B, B=");
    lineEditSel2 = new QLineEdit();
    lineEditSel2->setText("-4.66");
    treeWidgetProperties->setItemWidget(itemSelector2, 1,lineEditSel2);
    
    selectLambdaMethod(1);
}





void dan18::selectLambdaMethod(int method)
{
    if (method==1)
    {
        itemSelector1->setHidden(true);
        itemSelector2->setHidden(true);
    }
    else
    {
        itemSelector1->setHidden(false);
        itemSelector2->setHidden(false);
    }
}

*/