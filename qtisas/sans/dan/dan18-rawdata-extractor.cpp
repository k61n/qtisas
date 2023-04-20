/***************************************************************************
 File : dan18-rawdata-extractor.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
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

//*******************************************
//+++  2017: Extractor ...
//*******************************************
void dan18::extractorConnectSlots()
{
    connect( pushButtonNewInfoExtractor, SIGNAL( clicked() ), this, SLOT( newInfoExtractor() ) );
    connect( pushButtonMakeListExtractor, SIGNAL( clicked() ), this, SLOT( addToInfoExtractor() ) );
    connect( pushButtonExtractorAddCol, SIGNAL( clicked() ), this, SLOT( addColToInfoExtractor() ) );
}

QString dan18::getHeaderInfoString(QString number, QString name)
{
    
    if (name =="Sample") return readInfo( number );
    if (name =="C")      return QString::number( readDataIntCinM( number ));
    if (name =="D")      return QString::number(readDataDinM( number ));
    if (name =="lambda") return QString::number(readLambda( number ),'f',4);
    if (name =="Sum")    return QString::number(readSum( number ));
    if (name =="Duration") return QString::number(readDuration( number ));
    if (name =="cps") return QString::number(readSum( number )/readDuration( number ),'f',5);
    if (name =="Thickness") return QString::number(readThickness( number ),'f',5);
    if (name =="Sum-vs-Mask") return QString::number(integralVSmaskSimmetrical( number ));
    if (name =="Sum-vs-Mask-Dead-Time-Corrected") return QString::number(integralVSmaskUniDeadTimeCorrected(number));
    if (name =="Q2-vs-Mask") return QString::number(Q2_VS_maskSimmetrical( number ));
    if (name =="Runs") return number;

    if (name=="Name") name="Experiment-Title";
    if (name=="Sample-Nr") name="Sample-Position-Number";
    if (name=="Who") name="User-Name";
    if (name=="Offset") name="C,D-Offset";
    
    int indexInHeader=listOfHeaders.indexOf("["+name+"]");
                                                        
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
                                                        
    QString res=readNumberString( number, pos, num);
                                                        
    if (name.contains("Monitor-")) res=QString::number(res.toDouble(),'f',0);
    if (name.contains("Motor-")) res=QString::number(res.toDouble(),'f',3);
    if (name.contains("Field-")) res=QString::number(res.toDouble(),'f',5);
    if (name.contains("Selector")) res=QString::number(res.toDouble(),'f',4);
    
    return res;
}


QString dan18::generateUniqueStringInList (QStringList lst, const QString str)
{
    int index = 0;

    
    for ( int i = 0; i < int ( lst.count() ); i++ )
    {
        if ( lst[i].startsWith ( str ) && (lst[i].toInt()>=1 || lst[i]==str) ) index++;
    }
    


    if (index==0) return str;
    
    QString newName = str;

    newName += QString::number ( index );
    
    while ( lst.contains ( newName ) )
    {
        newName = str + QString::number ( ++index );
    }
    return newName;
}

void dan18::addColToInfoExtractor()
{
    //+++
    QString activeTable=comboBoxInfoExtractor->currentText()
;
    
    //+++ very IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);
    
    
    QString TableName;
    
    if ( infoTablesList.count()==0 || !infoTablesList.contains(activeTable))
    {

        infoTablesList<<TableName;
        infoTablesList.sort();
        comboBoxInfoExtractor->clear();
        
        comboBoxInfoExtractor->insertItems(0, infoTablesList);

        return;
    }
    else TableName=activeTable;
    
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    QString wildCardLocal=wildCard;
    wildCardLocal=wildCard;


    int startRaw=0;
    

    
    //+++
    if (!checkTableExistence(TableName)) return;
    
    //+++ Find table
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if (t->name()==TableName)  tableDat=(Table *)t;
    //+++
    
    QString what2add=comboBoxInfoExtractorCol->currentText();

    int initNumberCols=tableDat->numCols();
    QList<int> initColType=tableDat->columnTypes();
    
    if (initNumberCols<1) return;
    
    
    tableDat->addCol();
    tableDat->setColComment(initNumberCols, what2add);
    tableDat->setColName(initNumberCols,generateUniqueStringInList (tableDat->colNames(), comboBoxInfoExtractorCol->currentText()));

    
    if (what2add=="Sample"||what2add=="Beam"||what2add=="Date"||what2add=="Time"||what2add.contains("Comment")||what2add=="Name"||what2add=="Who")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="Files"||what2add=="RT-number"||what2add=="RT-Time-Factor"||what2add=="RT-Repetitions")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="RT-Frame-Duration"||what2add=="Attenuator"||what2add=="Polarization"||what2add=="Lenses")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<1;}
    else if (what2add=="C"||what2add =="D"||what2add=="lambda"||what2add=="Offset"||what2add=="Sample-Nr"||what2add=="Thickness"||what2add=="Time-Factor")
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<0;}
    else if (what2add.contains("Field-")||what2add.contains("Beamwindow-")||what2add.contains("-Position")||what2add.contains("BeamWin-"))
    { tableDat->setColPlotDesignation(initNumberCols,Table::None); initColType<<0;}
    else if (what2add.contains("Motor-"))
    { tableDat->setColPlotDesignation(initNumberCols,Table::X); initColType<<0;}
    else    { tableDat->setColPlotDesignation(initNumberCols,Table::Y); initColType<<0;};
    
    tableDat->setColumnTypes(initColType);
    
    for(int iter=0; iter<tableDat->numRows();iter++) tableDat->setText(iter,initNumberCols,getHeaderInfoString(tableDat->text(iter,0), what2add));
    


    
    tableDat->table()->resizeColumnToContents(initNumberCols);
    tableDat->table()->setColumnWidth(initNumberCols, tableDat->table()->columnWidth(initNumberCols)+10);
    maximizeWindow(TableName);
    
    /*
    


    //+++ Beamwindow_X 	[itBeamwindowX]
    indexInHeader=listOfHeaders.findIndex("[CA-X]");
    pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    num=tableHeaderPosNew->item(indexInHeader,1)->text();
    tableDat->setText( iter, itBeamwindowX, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
    //+++ Beamwindow_Y 	[itBeamwindowY]
    indexInHeader=listOfHeaders.findIndex("[CA-Y]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itBeamwindowY, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
    //+++ Detector Offet  	[itOffset ]
    indexInHeader=listOfHeaders.findIndex("[C,D-Offset]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itOffset , readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ DetectorX 		[itXposition]
    tableDat->setText(iter,itXposition, readDetectorX(lst, index, name2ndHeader));
    //+++ DetectorY 		[itYposition]
    tableDat->setText(iter,itYposition, readDetectorY(lst, index, name2ndHeader));
    //+++ Sample discription 	[itSampleNr]
    ss=QString::number(readSampleNumber(lst, index, name2ndHeader).toDouble(),'f',0);
    tableDat->setText(iter,itSampleNr,ss);
    //+++ SMotor-1 	[itMotor1]
    tableDat->setText(iter,itMotor1, readSMotor1(lst, index, name2ndHeader) );
    //+++ SMotor-2 	[itMotor2]
    tableDat->setText(iter,itMotor2, readSMotor2(lst, index, name2ndHeader) );
    //+++ Thickness 	[itThickness]
    tableDat->setText( iter, itThickness, QString::number( readThickness(lst, index, name2ndHeader), 'f', 2) );
    //+++ BeamWin-Xs 	[itBeamWinXs]
    indexInHeader=listOfHeaders.findIndex("[SA-X]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itBeamWinXs,QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2));
    //+++ BeamWin-Ys 	[itBeamWinYs]
    indexInHeader=listOfHeaders.findIndex("[SA-Y]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itBeamWinYs, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
    //+++ SA-Pos-X 	[itBeamWinXpos]
    indexInHeader=listOfHeaders.findIndex("[SA-Pos-X]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itBeamWinXpos, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ SA-Pos-Y 	[itBeamWinYpos]
    indexInHeader=listOfHeaders.findIndex("[SA-Pos-Y]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itBeamWinYpos, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Time-Factor 	[itTimeFactor]
    indexInHeader=listOfHeaders.findIndex("[Duration-Factor]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText( iter, itTimeFactor, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Comment1 		[itComment1]
    tableDat->setText(iter,itComment1, readComment1(lst, index, name2ndHeader));
    //+++ Comment2 		[itComment2]
    tableDat->setText(iter,itComment2, readComment2(lst, index, name2ndHeader));
    //+++ Experimental Name 	[itName]
    tableDat->setText(iter,itName, readExpName(lst, index, name2ndHeader) );
    //+++ Who 		[itWho]
    tableDat->setText(iter,itWho, readWho(lst, index, name2ndHeader) );
    //+++ Selector 		[itSelector]
    indexInHeader=listOfHeaders.findIndex("[Selector]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSelector, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Monitor1 		[itMonitor1]
    indexInHeader=listOfHeaders.findIndex("[Monitor-1]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itMonitor1, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Monitor2 		[itMonitor2]
    indexInHeader=listOfHeaders.findIndex("[Monitor-2]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itMonitor2, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Monitor3 		[itMonitor3]
    indexInHeader=listOfHeaders.findIndex("[Monitor-3|Tr|ROI]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itMonitor3, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ RT-Current-Number [itRTnumber]
    tableDat->setText(iter,itRTnumber,QString::number(readRtCurrentNumber( lst, index, name2ndHeader ))  );
    //+++ RT-Time-Factor [itRTtimefactor]
    tableDat->setText(iter,itRTtimefactor, QString::number(readTimefactor( lst, index, name2ndHeader )) );
    //+++ RT-Number-Frames [itRTrepetitions]
    tableDat->setText(iter,itRTrepetitions, QString::number(readNumberRepetitions( lst, index, name2ndHeader )) );
    //+++ Attenuator
    indexInHeader=listOfHeaders.findIndex("[Attenuator]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itAttenuator, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Polarization
    indexInHeader=listOfHeaders.findIndex("[Polarization]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itPolarization, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Lenses
    indexInHeader=listOfHeaders.findIndex("[Lenses]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itLenses, readNumber( lst, pos, num, index, name2ndHeader) );
    //+++ Slices-Count
    indexInHeader=listOfHeaders.findIndex("[Slices-Count]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCount, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Duration
    tableDat->setText(iter,itSlicesDuration, readSlicesDuration( lst, index, name2ndHeader) );
    //+++ Slices-Current-Number
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Number]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentNumber, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Current-Duration
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Duration]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentDuration, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Current-Monitor1
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor1]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentMonitor1, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Current-Monitor2
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor2]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentMonitor2, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Current-Monitor3
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor3]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentMonitor3, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    //+++ Slices-Current-Sum
    indexInHeader=listOfHeaders.findIndex("[Slices-Current-Sum]");
    pos=tableHeaderPosNew->item(indexInHeader,0);
    num=tableHeaderPosNew->item(indexInHeader,1);
    tableDat->setText(iter,itSlicesCurrentSum, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
    
    

    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    int skip;
    QString s,ss,nameMatrix,name2ndHeader;
    int iter;
    QStringList lst;
    int index;
    //
    QString pos,num;
    
    for(iter=startRaw; iter<(startRaw+filesNumber);iter++)
    {
        tableDat->setText( iter, itFiles, selectedDat[iter-startRaw]);
        lst.clear();
        
        //+++ header
        nameMatrix=findFileNumberInFileName(wildCard, selectedDat[iter-startRaw].remove(Dir));
        
        int index=-1;
        if (nameMatrix.contains("["))
        {
            index=nameMatrix.right(nameMatrix.length()-nameMatrix.find("[")).remove("[").remove("]").toInt();
        }
        
        readHeaderNumberFull ( nameMatrix, lst );
        
        name2ndHeader=nameMatrix;
        //+++ Info 		[itSample]
        int indexInHeader=listOfHeaders.findIndex("[Sample-Title]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        
        tableDat->setText( iter, itSample, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Run Nuber [itRuns]
        tableDat->setText(iter,itRuns, nameMatrix);
        //+++ Collimation 		[itC]
        tableDat->setText( iter, itC, QString::number(readDataIntC(lst, index, name2ndHeader)/100.0, 'f', 0) );
        //+++ Detector Z  		[itD]
        tableDat->setText( iter, itD, QString::number(readDataD(lst, index, name2ndHeader)/100.0, 'f', 3) );
        //+++ Lambda            [itLambda]
        tableDat->setText( iter, itLambda, QString::number(readLambda(lst, index, name2ndHeader), 'f',3) );
        //+++ Sum 		[itSum]
        tableDat->setText(iter,itSum, QString::number(readSum(lst, index, name2ndHeader)) );
        //+++ Duration 		[itDuration]
        tableDat->setText(iter,itDuration, QString::number(readDuration(lst, index, name2ndHeader)) );
        //+++ Date 		[itDate]
        tableDat->setText(iter,itDate, readDate(lst, index, name2ndHeader) );
        //+++ Time 		[itTime]
        tableDat->setText(iter,itTime, readTime(lst, index, name2ndHeader) );
        //+++ Field-1 [itField1]
        indexInHeader=listOfHeaders.findIndex("[Field-1]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itField1, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Field-2 [itField2]
        indexInHeader=listOfHeaders.findIndex("[Field-2]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itField2, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Field-3 [itField3]
        indexInHeader=listOfHeaders.findIndex("[Field-3]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itField3, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Field-4 [itField4]
        indexInHeader=listOfHeaders.findIndex("[Field-4]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itField4, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Beamwindow_X 	[itBeamwindowX]
        indexInHeader=listOfHeaders.findIndex("[CA-X]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamwindowX, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
        //+++ Beamwindow_Y 	[itBeamwindowY]
        indexInHeader=listOfHeaders.findIndex("[CA-Y]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamwindowY, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
        //+++ Detector Offet  	[itOffset ]
        indexInHeader=listOfHeaders.findIndex("[C,D-Offset]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itOffset , readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ DetectorX 		[itXposition]
        tableDat->setText(iter,itXposition, readDetectorX(lst, index, name2ndHeader));
        //+++ DetectorY 		[itYposition]
        tableDat->setText(iter,itYposition, readDetectorY(lst, index, name2ndHeader));
        //+++ Sample discription 	[itSampleNr]
        ss=QString::number(readSampleNumber(lst, index, name2ndHeader).toDouble(),'f',0);
        tableDat->setText(iter,itSampleNr,ss);
        //+++ SMotor-1 	[itMotor1]
        tableDat->setText(iter,itMotor1, readSMotor1(lst, index, name2ndHeader) );
        //+++ SMotor-2 	[itMotor2]
        tableDat->setText(iter,itMotor2, readSMotor2(lst, index, name2ndHeader) );
        //+++ Thickness 	[itThickness]
        tableDat->setText( iter, itThickness, QString::number( readThickness(lst, index, name2ndHeader), 'f', 2) );
        //+++ BeamWin-Xs 	[itBeamWinXs]
        indexInHeader=listOfHeaders.findIndex("[SA-X]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamWinXs,QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2));
        //+++ BeamWin-Ys 	[itBeamWinYs]
        indexInHeader=listOfHeaders.findIndex("[SA-Y]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamWinYs, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',2) );
        //+++ SA-Pos-X 	[itBeamWinXpos]
        indexInHeader=listOfHeaders.findIndex("[SA-Pos-X]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamWinXpos, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ SA-Pos-Y 	[itBeamWinYpos]
        indexInHeader=listOfHeaders.findIndex("[SA-Pos-Y]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itBeamWinYpos, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Time-Factor 	[itTimeFactor]
        indexInHeader=listOfHeaders.findIndex("[Duration-Factor]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText( iter, itTimeFactor, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Comment1 		[itComment1]
        tableDat->setText(iter,itComment1, readComment1(lst, index, name2ndHeader));
        //+++ Comment2 		[itComment2]
        tableDat->setText(iter,itComment2, readComment2(lst, index, name2ndHeader));
        //+++ Experimental Name 	[itName]
        tableDat->setText(iter,itName, readExpName(lst, index, name2ndHeader) );
        //+++ Who 		[itWho]
        tableDat->setText(iter,itWho, readWho(lst, index, name2ndHeader) );
        //+++ Selector 		[itSelector]
        indexInHeader=listOfHeaders.findIndex("[Selector]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSelector, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Monitor1 		[itMonitor1]
        indexInHeader=listOfHeaders.findIndex("[Monitor-1]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itMonitor1, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Monitor2 		[itMonitor2]
        indexInHeader=listOfHeaders.findIndex("[Monitor-2]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itMonitor2, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Monitor3 		[itMonitor3]
        indexInHeader=listOfHeaders.findIndex("[Monitor-3|Tr|ROI]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itMonitor3, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ RT-Current-Number [itRTnumber]
        tableDat->setText(iter,itRTnumber,QString::number(readRtCurrentNumber( lst, index, name2ndHeader ))  );
        //+++ RT-Time-Factor [itRTtimefactor]
        tableDat->setText(iter,itRTtimefactor, QString::number(readTimefactor( lst, index, name2ndHeader )) );
        //+++ RT-Number-Frames [itRTrepetitions]
        tableDat->setText(iter,itRTrepetitions, QString::number(readNumberRepetitions( lst, index, name2ndHeader )) );
        //+++ Attenuator
        indexInHeader=listOfHeaders.findIndex("[Attenuator]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itAttenuator, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Polarization
        indexInHeader=listOfHeaders.findIndex("[Polarization]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itPolarization, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Lenses
        indexInHeader=listOfHeaders.findIndex("[Lenses]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itLenses, readNumber( lst, pos, num, index, name2ndHeader) );
        //+++ Slices-Count
        indexInHeader=listOfHeaders.findIndex("[Slices-Count]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCount, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Duration
        tableDat->setText(iter,itSlicesDuration, readSlicesDuration( lst, index, name2ndHeader) );
        //+++ Slices-Current-Number
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Number]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentNumber, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Current-Duration
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Duration]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentDuration, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Current-Monitor1
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor1]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentMonitor1, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Current-Monitor2
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor2]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentMonitor2, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Current-Monitor3
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Monitor3]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentMonitor3, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        //+++ Slices-Current-Sum
        indexInHeader=listOfHeaders.findIndex("[Slices-Current-Sum]");
        pos=tableHeaderPosNew->item(indexInHeader,0);
        num=tableHeaderPosNew->item(indexInHeader,1);
        tableDat->setText(iter,itSlicesCurrentSum, QString::number(readNumber( lst, pos, num, index, name2ndHeader).toDouble(),'f',0) );
        
        
        //cps
        double cps=tableDat->text(iter,itSum).toDouble() /
        tableDat->text(iter,itDuration).toDouble()*
        tableDat->text(iter,itRTtimefactor).toDouble(); //2015 RT-TOF
        tableDat->setText(iter,itCps, QString::number(cps));//+1
        
        //integral
        if (checkBoxSumVsMask->isChecked() )
        {
            tableDat->setText( iter, itSumvsmask,QString::number(integralVSmaskSimmetrical( nameMatrix )) );// integralVSmaskUniByName( selectedDat[iter-startRaw]) );
            tableDat->setText( iter, itSumvsmaskdeadtimecorrected, QString::number(integralVSmaskUniDeadTimeCorrected( nameMatrix )));
            tableDat->setText( iter, itQ2vsmask, QString::number(Q2_VS_maskSimmetrical( nameMatrix )));
        }
        else
        {
            tableDat->setText( iter, itSumvsmask, "---" );
            tableDat->setText( iter, itSumvsmaskdeadtimecorrected, "---" );
            tableDat->setText( iter, itQ2vsmask, "---" );
        }
        // Frame Duration
        double frameDuration=tableDat->text(iter,itDuration).toDouble();
        frameDuration*=tableDat->text(iter,itRTrepetitions).toDouble();
        if (tableDat->text(iter,itRTtimefactor).toDouble()!=0) frameDuration/=tableDat->text(iter,itRTtimefactor).toDouble();
        tableDat->setText(iter,itRTframeduration,QString::number(frameDuration));//new
        
        // Beam Size
        s=QString::number(tableDat->text(iter,itBeamwindowX).toDouble(),'f',1)+"x" +QString::number(tableDat->text(iter,itBeamwindowY).toDouble(),'f',1) +"|";
        s+=QString::number(tableDat->text(iter,itBeamWinXs).toDouble(),'f',1)+"x" +QString::number(tableDat->text(iter,itBeamWinYs).toDouble(),'f',1);
        tableDat->setText(iter,itBeam,s);
        
        // real D
        //tableDat->setText(iter,3,QString::number(tableDat->text(iter,3).toDouble()+tableDat->text(iter,17).toDouble(),'f',2));
        
        
        if(checkBoxDatYesNo->isChecked())
        {
            if (checkBoxSortOutputToFolders->isChecked())
            {
                app()->changeFolder("DAN :: rawdata");
            }
            
            gsl_matrix *data=gsl_matrix_alloc(MD,MD);
            gsl_matrix_set_all(data, 0.0);
            
            readMatrix( nameMatrix, data );
            
            nameMatrix=nameMatrix.replace("_","-").replace("/","-").replace("\\","-").replace("%","-").replace(",","-");
            
            makeMatrixSymmetric(data,"Matrix-"+nameMatrix, "Rawdata::Matrix", MD );
            
            gsl_matrix_free(data);
        }
        
    }
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
    
    maximizeWindow(TableName);
    
    int tt;
    // adjust columns
    if (checkBoxShortList->isChecked())
    {
        
        for (tt=14; tt<21; tt++)
        {
            tableDat->table()->hideColumn(tt);
        }
        for (tt=24; tt<tableDat->numCols(); tt++)
        {
            tableDat->table()->hideColumn(tt);
        }
    }
    for (tt=0; tt<tableDat->numCols(); tt++)
    {
        if (!checkBoxShortList->isChecked()) tableDat->table()->showColumn(tt);
        tableDat->table()->resizeColumnToContents(tt);
        tableDat->table()->setColumnWidth(tt, tableDat->table()->columnWidth(tt)+10);
    }
    
    int sampleLength=0;
    for(i=0;i<tableDat->numRows();i++)
    {
        s=tableDat->text(i,0);
        if (s.length()>sampleLength ) sampleLength=s.length();
    }
    
    
    for(i=0;i<tableDat->numRows();i++)
    {
        s=tableDat->text(i,0);
        while(s.length()<sampleLength)s+=" ";
        tableDat->setText(i,0,s);
    }
   */
}

void dan18::addToInfoExtractor()
{
    //+++
    QString activeTable=comboBoxInfoExtractor->currentText();
    
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);
    
    
    QString TableName;
    
    if (infoTablesList.count()==0 || !infoTablesList.contains(activeTable))
    {
        bool ok;
        TableName = QInputDialog::getText(this,
                                          "Table's Extractor", "Enter name of Table:",
                                          QLineEdit::Normal,
                                          "info-extrator", &ok);
        if ( !ok ||  TableName.isEmpty() )
        {
            return;
        }
        
        if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
        infoTablesList<<TableName;
        infoTablesList.sort();
        comboBoxInfoExtractor->clear();
        
        comboBoxInfoExtractor->insertItems(0, infoTablesList);
        comboBoxInfoExtractor->setCurrentIndex(infoTablesList.indexOf(TableName));
    }
    else TableName=activeTable;
    
    
    int i;
    
    //+++ create table
    Table* tableDat;
    
    QString wildCardLocal=wildCard;
    
    if (wildCardLocal.count("#")<3)
        wildCardLocal=wildCardLocal.replace("#","*");
    else if (wildCard.count("#")>2)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (#>1)");
        return;
    }
    else if (wildCardLocal.count("*")!=1)
    {
        QMessageBox::critical( 0, "qtiSAS", "Check >> File : Pattern (* != 1)");
        return;
    }
    
    QString filter=textEditPattern->text();
    
    //+++ select files
    QFileDialog *fd = new QFileDialog(this,"DAN - Getting File Information",Dir,"*");

    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setFilter(filter+";;"+textEditPattern->text());
    foreach( QComboBox *obj, fd->findChildren< QComboBox * >( ) ) if (QString(obj->objectName()).contains("fileTypeCombo")) obj->setEditable(true);
    
    if (!fd->exec() == QDialog::Accepted ) return;
    
    QStringList selectedDat=fd->selectedFiles();
    int filesNumber= selectedDat.count();
    
    if (filesNumber==0)
    {
        QMessageBox::critical( 0, "qtiSAS", "Nothing was selected");
        return;
    }
    
    QString test=selectedDat[0];
    if ( test.contains(Dir) )
    {
        test=test.remove(Dir);
        if (!dirsInDir && test.contains("/") )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
            return;
        }
        else if (dirsInDir && test.count("/")>1 )
        {
            QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder:: + sub-folders");
            return;
        }
    }
    else
    {
        QMessageBox::critical( 0, "qtiSAS", "Selected data not in ::Input Folder::");
        return;
    }
    
    
    int startRaw=0;
    
    wildCardLocal=wildCard;
    
    //app()->ws->hide();
    //app()->ws->blockSignals ( true );
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++
    if (checkTableExistence(TableName))
    {
        //+++ Find table
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if (t->name()==TableName)  tableDat=(Table *)t;
        //+++

        if (tableDat->numCols()<1)
        {
            QMessageBox::critical( 0, "qtiSAS", "Create new table (# cols)");
            //app()->ws->show();
            //app()->ws->blockSignals ( false );
            return;
        }
        
        startRaw=tableDat->numRows();
        
    }
    else
    {
        
        tableDat=app()->newTable(TableName, filesNumber,1);
        
        //+++
        tableDat->setWindowLabel("Info::Extractor");
        app()->setListViewLabel(tableDat->name(), "Info::Extractor");
        app()->updateWindowLists(tableDat);
        
        QStringList colType;
        //+++ Cols Names
        tableDat->setColName(0,"Runs");	tableDat->setColPlotDesignation(0,Table::X);colType<<"1";
        tableDat->setColumnTypes(colType);
    }
    
    
    tableDat->setNumRows(startRaw+filesNumber);
    
    
    //+++
    tableDat->show();
    //+++
    QString fnameOnly;
    int skip;
    QString s,ss,nameMatrix;
    int iter;
    //
    QString pos,num;
    
    for(iter=startRaw; iter<(startRaw+filesNumber);iter++)
    {
        //+++ header
        nameMatrix=findFileNumberInFileName(wildCard, selectedDat[iter-startRaw].remove(Dir));

        //+++ Run Nuber [itRuns]
        tableDat->setText(iter,0, nameMatrix);
        for(int i=1; i<tableDat->numCols();i++) tableDat->setText(iter,i, getHeaderInfoString(nameMatrix, tableDat->comment(i)));
    }
    
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //app()->ws->show();
    //app()->ws->blockSignals ( false );
    
    maximizeWindow(TableName);
    

    tableDat->table()->resizeColumnToContents(0);
    tableDat->table()->setColumnWidth(0, tableDat->table()->columnWidth(0)+10);
}

//++++++SLOT::newInfoExtractor++
void dan18::newInfoExtractor()
{
    bool ok;
    QString TableName = QInputDialog::getText(this,
                                              "Table's Extractor", "Constract yourself...",
                                              QLineEdit::Normal,
                                              "info-extractor", &ok);
    if ( !ok ||  TableName.isEmpty() )
    {
        return;
    }
    newInfoExtractor(TableName);
}

//++++++SLOT::newInfoExtractor++
void dan18::newInfoExtractor(QString TableName)
{
    
    //+++ vry IMPORTANT
    ImportantConstants(); //+++ wildcard, numberLines ...
    
    QStringList infoTablesList;
    findTableListByLabel("Info::Extractor", infoTablesList);

    if (comboBoxInfoExtractor->count()>0 && TableName=="") return; //
    
    comboBoxInfoExtractor->clear();
    if (TableName=="" ) {comboBoxInfoExtractor->insertItems(0, infoTablesList);return;}

    if (infoTablesList.contains(TableName)) TableName=app()->generateUniqueName(TableName+"-");
    infoTablesList<<TableName;
    infoTablesList.sort();

    
    comboBoxInfoExtractor->insertItems(0, infoTablesList);
    comboBoxInfoExtractor->setCurrentIndex(infoTablesList.indexOf(TableName));
    
    
    if (checkBoxSortOutputToFolders->isChecked())
    {
        app()->changeFolder("DAN :: script, info, ...");
    }
    
    //+++ create table
    Table* tableDat;
    
    tableDat=app()->newTable(TableName, 0,1);
    
    //+++
    tableDat->setWindowLabel("Info::Extractor");
    app()->setListViewLabel(tableDat->name(), "Info::Extractor");
    app()->updateWindowLists(tableDat);
    
    QStringList colType;
    //+++ Cols Names
    tableDat->setColName(0,"Runs");	tableDat->setColPlotDesignation(0,Table::X);colType<<"1";
    tableDat->setColumnTypes(colType);
    
    //+++
    tableDat->show();
    
    maximizeWindow(TableName);
    

}

void dan18::extractorInit()
{
QStringList lst;
    
    lst<<"Sample";
    lst<<"Comment1";
    lst<<"Comment2";
    lst<<"Name";
    lst<<"Who";
    lst<<"Sample-Nr";
    lst<<"Date";
    lst<<"Time";
    lst<<"Duration";
    lst<<"Sum";
    lst<<"cps";
    lst<<"C";
    lst<<"D";
    lst<<"Offset";
    lst<<"lambda";
    lst<<"Selector";
    lst<<"Thickness";
    lst<<"Monitor-1";
    lst<<"Monitor-2";
    lst<<"Monitor-3";
    lst<<"Sample-Motor-1";
    lst<<"Sample-Motor-2";
    lst<<"Sample-Motor-3";
    lst<<"Sample-Motor-4";
    lst<<"Sample-Motor-5";
    lst<<"Field-1";
    lst<<"Field-2";
    lst<<"Field-3";
    lst<<"Field-4";
    lst<<"Attenuator";
    lst<<"Polarization";
    lst<<"Lenses";
    lst<<"Sum-vs-Mask";
    lst<<"Sum-vs-Mask-Dead-Time-Corrected";
    lst<<"Q2-vs-Mask";
        lst<<"Runs";
/*
    lst<<"Beam";
    lst<<"Beamwindow-X";
    lst<<"Beamwindow-Y";

    lst<<"X-Position";
    lst<<"Y-Position";

    lst<<"BeamWin-Xs";
    lst<<"BeamWin-Ys";
    lst<<"Beamwin-X-Pos";
    lst<<"Beamwin-Y-Pos";
    lst<<"Time-Factor";

    lst<<"RT-number";
    lst<<"RT-Time-Factor";
    lst<<"RT-Repetitions";
    lst<<"RT-Frame-Duration";

    lst<<"Slices-Count";
    lst<<"Slices-Duration";
    lst<<"Slices-Current-Number";
    lst<<"Slices-Current-Duration";
    lst<<"Slices-Current-Monitor1";
    lst<<"Slices-Current-Monitor2";
    lst<<"Slices-Current-Monitor3";
    lst<<"Slices-Current-Sum";

    */
    comboBoxInfoExtractorCol->addItems(lst);
}


