/***************************************************************************
 File : dan18-options-header.cpp
 Project  : QtiSAS
 --------------------------------------------------------------------
 Copyright: (C) 2012-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description  : SANS Data Analisys Interface: headers reading tools
 
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

//+++ read f from DAT-files:: Dead Time Correction
double dan18::readDataDeadTime(const QString &Number)
{
    double deadTime = lineEditDeadTime->text().toDouble();
    return deadTimeFaktor(detector->readSum(Number) / readDuration(Number), deadTime);
}
//+++ read f from DAT-files:: Dead Time Correction :: DB
double dan18::readDataDeadTimeDB(const QString &Number)
{
    double deadTime = lineEditDBdeadtime->text().toDouble();
    return deadTimeFaktor(detector->readSum(Number) / readDuration(Number), deadTime);
}
//+++++FUNCTIONS::Read-DAT-files:: Normalization
double dan18::readDataNormalization( QString Number )
{
    double normConstant=spinBoxNorm->value();
    if ( comboBoxNorm->currentText()=="Monitor2")
    {
        double M2=readMonitor2( Number );
        if (M2>0.0) return normConstant/M2;
    }
    
    if ( comboBoxNorm->currentText()=="Monitor1")
    {
        double M1=readMonitor1( Number );
        if (M1>0.0) return normConstant/M1;
    }
    
    if ( comboBoxNorm->currentText()=="Time")
    {
        double time=readDuration( Number );
        if (time>0.0) return normConstant/time;
    }
    
    //+++
    toResLog("DAN::" + Number+"---> check normalization!\n");
    
    return 0.0;
}
//++++FUNCTIONS::Read-DAT-files:: Normalization
double dan18::readDataNormalizationRT( QString Number )
{
    double timeFactor=readTimefactor( Number );
    if(timeFactor==1.0) return 1.0;
    
    int numberRepetitions= readNumberRepetitions( Number );
    
    return timeFactor/numberRepetitions;
}
//+++++FUNCTIONS::Read-DAT-files:: S3norm
double dan18::readDataM3norm( QString Number )
{
    double norm 	=readDataNormalization( Number );
    double M3 	=readMonitor3( Number );
    
    return M3*norm;
}
//+++ read duration
double dan18::readDuration(const QString &Number) // [sec]
{
    double duration = parserHeader->readNumberString(Number, "[Duration]").simplified().toDouble();
 
    if (comboBoxUnitsTime->currentIndex() == 1)
        duration /= 10.0;
    if (comboBoxUnitsTime->currentIndex() == 2)
        duration /= 1000.0;
    if (comboBoxUnitsTime->currentIndex() == 3)
        duration /= 1000000.0;

    return duration;
}
//+++ read duration
double dan18::readDuration(const QStringList &lst, int index, const QString &Number) // [sec]
{
    double duration = parserHeader->readNumberString(Number, "[Duration]", lst).simplified().toDouble();
    
    if (comboBoxUnitsTime->currentIndex() == 1)
        duration /= 10.0;
    if (comboBoxUnitsTime->currentIndex() == 2)
        duration /= 1000.0;
    if (comboBoxUnitsTime->currentIndex() == 3)
        duration /= 1000000.0;

    return duration;
}
//+++ read  Monitor1
double dan18::readMonitor1(const QString &Number, double deadTime)
{
    double M1 = parserHeader->readNumberString(Number, "[Monitor-1]").simplified().toDouble();
    double duration = readDuration(Number);
    double deadTimeM1;
    if (deadTime < 0)
        deadTimeM1 = lineEditDeadTimeM1->text().toDouble();
    else
        deadTimeM1 = deadTime;
    if (duration == 0.0)
    {
        deadTimeM1 = 0.0;
        duration = 0;
    }
    return M1 / (1 - deadTimeM1 / duration * M1);
}
//+++ read  Monitor2
double dan18::readMonitor2(const QString &Number, double deadTime)
{
    double M2 = parserHeader->readNumberString(Number, "[Monitor-2]").simplified().toDouble();
    double duration = readDuration(Number);
    double deadTimeM2;
    if (deadTime < 0)
        deadTimeM2 = lineEditDeadTimeM2->text().toDouble();
    else
        deadTimeM2 = deadTime;
    if (duration == 0.0)
    {
        deadTimeM2 = 0.0;
        duration = 0;
    }
    return M2 / (1 - deadTimeM2 / duration * M2);
}
//+++ read  Monitor3
double dan18::readMonitor3(const QString &Number, double deadTime)
{
    double M3 = parserHeader->readNumberString(Number, "[Monitor-3|Tr|ROI]").simplified().toDouble();
    double duration = readDuration(Number);
    double deadTimeM3;
    if (deadTime < 0)
        deadTimeM3 = lineEditDeadTimeM3->text().toDouble();
    else
        deadTimeM3 = deadTime;
    if (duration == 0.0)
    {
        deadTimeM3 = 0.0;
        duration = 0;
    }
    return M3 / (1 - deadTimeM3 / duration * M3);
}
//+++ Timefactor
int dan18::readRtCurrentNumber(const QString &Number) // [1]
{
    int currentNumber = parserHeader->readNumberString(Number, "[RT-Current-Number]").simplified().toInt();

    if (currentNumber < 1)
        currentNumber = 1;

    return currentNumber;
}
//+++ read  readTimefactor
int dan18::readRtCurrentNumber(const QStringList &lst, int index, const QString &Number)
{
    int currentNumber = parserHeader->readNumberString(Number, "[RT-Current-Number]", lst).simplified().toInt();

    if (currentNumber < 1)
        currentNumber = 1;

    return currentNumber;
}
//+++ Timefactor
double dan18::readTimefactor(const QString &Number) // [1]
{
    double readTimefactor = parserHeader->readNumberString(Number, "[RT-Time-Factor]").toDouble();
    if (readTimefactor <= 0)
        readTimefactor = 1.0;

    return readTimefactor;
}
//+++ read  readTimefactor
double dan18::readTimefactor(const QStringList &lst, int index, const QString &Number)
{
    double readTimefactor = parserHeader->readNumberString(Number, "[RT-Time-Factor]", lst).toDouble();
    if (readTimefactor <= 0)
        readTimefactor = 1.0;
    return readTimefactor;
}
//+++ Timefactor numberRepetitions
int dan18::readNumberRepetitions(const QString &Number) // [1]
{
    int numberRepetitions = parserHeader->readNumberString(Number, "[RT-Number-Repetitions]").toInt();
    if (numberRepetitions < 1)
        numberRepetitions = 1;
    return numberRepetitions;
}
//+++ read  readNumberRepetitions
int dan18::readNumberRepetitions(const QStringList &lst, int index, const QString &Number)
{
    int numberRepetitions = parserHeader->readNumberString(Number, "[RT-Number-Repetitions]", lst).toInt();
    if (numberRepetitions < 1)
        numberRepetitions = 1;
    return numberRepetitions;
}
//+++ read  Slices-Count
QString dan18::readSlicesCount(const QStringList &lst, int index, const QString &Number)
{
    return QString::number(parserHeader->readNumberString(Number, "[Slices-Count]", lst).toDouble(), 'f', 0);
}
//
//+++ read  Slices-Count
int dan18::readSlicesCount(const QString &Number)
{
    int slicesCount = parserHeader->readNumberString(Number, "[Slices-Count]").toInt();
    if (slicesCount < 1)
        slicesCount = 1;
    return slicesCount;
}
//+++ read  Slices-Duration
QString dan18::readSlicesDuration(const QStringList &lst, int index, const QString &Number)
{
    double slicesDuration = parserHeader->readNumberString(Number, "[Slices-Duration]", lst).toDouble();

    if (comboBoxUnitsTimeRT->currentIndex() == 1)
        slicesDuration /= 10.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 2)
        slicesDuration /= 1000.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 3)
        slicesDuration /= 1000000.0;

    if (slicesDuration < 0)
        slicesDuration = 0;

    return QString::number(slicesDuration);
}
//+++ read  Slices-Duration
double dan18::readSlicesDuration(const QString &Number)
{
    double slicesDuration = parserHeader->readNumberString(Number, "[Slices-Duration]").toDouble();

    if (comboBoxUnitsTimeRT->currentIndex() == 1)
        slicesDuration /= 10.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 2)
        slicesDuration /= 1000.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 3)
        slicesDuration /= 1000000.0;

    if (slicesDuration < 0)
        slicesDuration = 0;

    return slicesDuration;
}
//+++ read  Slices-Current-Number
int dan18::readSlicesCurrentNumber(const QStringList &lst, int index, const QString &Number)
{
    int SlicesCurrentNumber = parserHeader->readNumberString(Number, "[Slices-Current-Number]", lst).toInt();
    if (SlicesCurrentNumber < 0)
        SlicesCurrentNumber = 0;
    return SlicesCurrentNumber;
}
//+++ read  Slices-Current-Duration
double dan18::readSlicesCurrentDuration(const QStringList &lst, int index, const QString &Number)
{
    double SlicesCurrentDuration = parserHeader->readNumberString(Number, "[Slices-Current-Duration]", lst).toDouble();

    if (comboBoxUnitsTimeRT->currentIndex() == 1)
        SlicesCurrentDuration /= 10.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 2)
        SlicesCurrentDuration /= 1000.0;
    if (comboBoxUnitsTimeRT->currentIndex() == 3)
        SlicesCurrentDuration /= 1000000.0;

    if (SlicesCurrentDuration < 0)
        SlicesCurrentDuration = 0;

    return SlicesCurrentDuration;
}
//+++ read  Slices-Current-Monitor1
double dan18::readSlicesCurrentMonitor1(const QStringList &lst, int index, const QString &Number)
{
    double SlicesCurrentMonitor1 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor1]", lst).toDouble();

    if (SlicesCurrentMonitor1 < 0)
        SlicesCurrentMonitor1 = 0;

    return SlicesCurrentMonitor1;
}
//+++ read  Slices-Current-Monitor2
double dan18::readSlicesCurrentMonitor2(const QStringList &lst, int index, const QString &Number)
{
    double SlicesCurrentMonitor2 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor2]", lst).toDouble();

    if (SlicesCurrentMonitor2 < 0)
        SlicesCurrentMonitor2 = 0;

    return SlicesCurrentMonitor2;
}
//+++ read  Slices-Current-Monitor3
double dan18::readSlicesCurrentMonitor3(const QStringList &lst, int index, const QString &Number)
{
    double SlicesCurrentMonitor3 = parserHeader->readNumberString(Number, "[Slices-Current-Monitor3]", lst).toDouble();

    if (SlicesCurrentMonitor3 < 0)
        SlicesCurrentMonitor3 = 0;

    return SlicesCurrentMonitor3;
}
//+++ read  Slices-Current-Sum
double dan18::readSlicesCurrentSum(const QStringList &lst, int index, const QString &Number)
{
    double SlicesCurrentSum = parserHeader->readNumberString(Number, "[Slices-Current-Sum]", lst).toDouble();

    if (SlicesCurrentSum < 0)
        SlicesCurrentSum = 0;

    return SlicesCurrentSum;
}
//+++ calculate Trans
double dan18::readTransmission( QString NumberSample, QString NumberEC, QString mask, double VShift, double HShift, double &sigmaTr )
{
    // sigmaTr relative error of the Tr
    
    if (NumberSample==NumberEC) return 1.00;
    if (!filesManager->checkFileNumber(NumberSample))
        return 1.00;
    if (!filesManager->checkFileNumber(NumberEC) && comboBoxTransmMethod->currentIndex() != 2)
        return 1.00;
    double sample;
    double ec;
    
    double sigma2=0;
    
    
    // Monitor-3 [dead-time-]
    if (comboBoxTransmMethod->currentIndex()==0)
    {
        sample=readDataM3norm( NumberSample );
        if (fabs(readMonitor3(NumberSample))>0.0) sigma2 += 1/fabs(readMonitor3(NumberSample)); //+++2019 error
        ec=readDataM3norm( NumberEC );
        if (fabs(readMonitor3(NumberEC))>0.0) sigma2 += 1/fabs(readMonitor3(NumberEC)); //+++2019 error
    }//Direct Beam  [dead-time+]
    else if (comboBoxTransmMethod->currentIndex()==1)
    { 	
        sample=integralVSmaskUniDeadTimeCorrected( NumberSample, mask, VShift,HShift);
        if (fabs(sample)>0.0) sigma2 += 1/fabs(sample); //+++2019 error
        
        sample=sample/readDataDeadTime(NumberSample)*readDataDeadTimeDB(NumberSample);
        sample*=readDataNormalization( NumberSample );
        
        ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
        if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error
        
        ec=ec/readDataDeadTime(NumberEC)*readDataDeadTimeDB(NumberEC);
        ec*=readDataNormalization( NumberEC );
        
    } //Tr in Header  [dead-time -]
    else if (comboBoxTransmMethod->currentIndex()==2)
    {
        sample=readMonitor3( NumberSample );
        ec=readMonitor3( NumberEC );
        if (ec==0) ec=1;
    }//	ROI in Header  [dead-time +]
    else if (comboBoxTransmMethod->currentIndex()==3)
    {
        sample= readDataM3norm( NumberSample );
        if (fabs(readMonitor3(NumberSample))>0.0) sigma2 += 1/fabs(readMonitor3(NumberSample)); //+++2019 error
        
        sample*=readDataDeadTimeDB( NumberSample );
        
        ec=readDataM3norm( NumberEC );
        if (fabs(readMonitor3(NumberEC))>0.0) sigma2 += 1/fabs(readMonitor3(NumberEC)); //+++2019 error
        
        ec*=readDataDeadTimeDB( NumberEC );
    }
    else if (comboBoxTransmMethod->currentIndex()==4)
    {
        if (selector->readLambda(NumberSample, readDuration(NumberSample)) < 9.5) // ROI in Header  [dead-time +]
        {
            
            sample= readDataM3norm( NumberSample );
            if (fabs(readMonitor3(NumberSample))>0.0) sigma2 += 1/fabs(readMonitor3(NumberSample)); //+++2019 error
            
            sample*=readDataDeadTimeDB( NumberSample );
            
            ec=readDataM3norm( NumberEC );
            if (fabs(readMonitor3(NumberEC))>0.0) sigma2 += 1/fabs(readMonitor3(NumberEC)); //+++2019 error
            
            ec*=readDataDeadTimeDB( NumberEC );
        }
        else //Direct Beam  [dead-time+]
        {
            sample=integralVSmaskUniDeadTimeCorrected( NumberSample, mask, VShift,HShift);
            if (fabs(sample)>0.0) sigma2 += 1/fabs(sample); //+++2019 error
            
            sample=sample/readDataDeadTime(NumberSample)*readDataDeadTimeDB(NumberSample);
            sample*=readDataNormalization( NumberSample );
            
            ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
            if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error
            
            ec=ec/readDataDeadTime(NumberEC)*readDataDeadTimeDB(NumberEC);
            ec*=readDataNormalization( NumberEC );
        }
    }
    
    if ( comboBoxNorm->currentText()!="Time" && comboBoxTransmMethod->currentIndex()!=2)
    {
        sigma2 += fabs(readDataNormalization( NumberEC ));
        sigma2 += fabs(readDataNormalization( NumberSample));
    }
    
    sigmaTr=sqrt(sigma2);
    
    if (ec<=0.0) return 0.0;
    return sample/ec;
}

double dan18::readTransmissionMaskDB(QString NumberSample,QString NumberEC,double VShift,double HShift, double xCenter, double yCenter, double Radius, double &sigmaTr)
{
    //+++ making mask
    MD=lineEditMD->text().toInt();
    gsl_matrix *mask=gsl_matrix_calloc(MD,MD);  // allocate sens matrix
    
    xCenter=xCenter-1;
    yCenter=yCenter-1;
    double xSize=2*Radius;
    double ySize=2*Radius;

    if ( xSize==0 || ySize==0)
    {
        gsl_matrix_free(mask);
        return 1.0;
    }
    
    for (int xx=xCenter-Radius-1;xx<xCenter+Radius+1;xx++) for (int yy=yCenter-Radius-1; yy<yCenter+Radius+1;yy++)
    {
        if ( (xCenter-xx)*(xCenter-xx)*4/xSize/xSize+(yCenter-yy)*(yCenter-yy)*4/ySize/ySize<=1 ) gsl_matrix_set(mask,yy,xx,1.0);
    }
    //---
    
    
    // sigmaTr relative error of the Tr
    
    if (NumberSample==NumberEC) { gsl_matrix_free(mask); return 1.0;};
    if (!filesManager->checkFileNumber(NumberSample))
    {
        gsl_matrix_free(mask);
        return 1.0;
    }
    if (!filesManager->checkFileNumber(NumberEC) && comboBoxTransmMethod->currentIndex() != 2)
    {
        gsl_matrix_free(mask);
        return 1.0;
    }
    double sample;
    double ec;
    
    double sigma2=0;
    
    
    sample=integralVSmaskUniDeadTimeCorrected( NumberSample, mask, VShift,HShift);
    std::cout<<sample<<"\t";
    if (fabs(sample)>0.0) sigma2 += 1/fabs(sample); //+++2019 error
            
    sample=sample/readDataDeadTime(NumberSample)*readDataDeadTimeDB(NumberSample);
    sample*=readDataNormalization( NumberSample );
            
    ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
    std::cout<<ec<<"\n"<<std::flush;
    if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error
                
    ec=ec/readDataDeadTime(NumberEC)*readDataDeadTimeDB(NumberEC);
    ec*=readDataNormalization( NumberEC );
                
    
    if ( comboBoxNorm->currentText()!="Time" && comboBoxTransmMethod->currentIndex()!=2)
    {
        sigma2 += fabs(readDataNormalization( NumberEC ));
        sigma2 += fabs(readDataNormalization( NumberSample));
    }
    
    sigmaTr=sqrt(sigma2);
    
    
    gsl_matrix_free(mask);
    
    if (ec<=0.0) return 0.0;
    return sample/ec;
}

//*******************************************
//+++  Read Header Full Number
//*******************************************
bool dan18::readHeaderNumberFull( QString Number, QStringList &header )
{
    
    header.clear();
    if (checkBoxYes2ndHeader->isChecked()) readHeaderNumber( wildCard2nd, Number, linesInSeparateHeader, header);
    
    QStringList header1;
    readHeaderNumber( wildCard, Number, linesInHeader+linesInDataHeader, header1);
    
    header+=header1;
    
    
    return true;
}

//*******************************************
//+++  Read Header
//*******************************************
bool dan18::readHeaderNumber( QString wildCardLocal, QString Number, int linesNumber, QStringList &header )
{
    int index=0;
    if (Number.contains("[") && !wildCardLocal.contains("["))
    {
        index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]").toInt();
    }
    return readHeaderFile(filesManager->fileNameFull(Number, wildCardLocal), linesNumber + index, header);
}
bool dan18::readHeaderFile( QString fileName, int linesNumber, QStringList &header )
{
    header.clear();
    
    if (linesNumber<=0) return true;
    
    QFile file(fileName);
    QTextStream t( &file );
    
    
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    
    if (flexiHeader && flexiStop[0]!="")
    {
        QString s;
        int symbolsNumber;
        bool endReached=false;
        
        for (int i=0;i<linesNumber;i++)
        {
            s=t.readLine();
            
            if (removeNonePrintable)
            {
                QString ss=s; s="";
                int smax=ss.length(); if (smax>10000) smax=10000;
                for(int ii=0; ii<smax;ii++) if (ss[ii].isPrint()) s+=ss[ii];
            }
            
            header << s;
            
            for (int iFlex=0; iFlex<flexiStop.count(); iFlex++)
            {
                symbolsNumber=flexiStop[iFlex].length();
                if (s.left(symbolsNumber)==flexiStop[iFlex] || t.atEnd() )
                {
                    endReached=true;
                    break; // skip header
                }
            }
            
            if (endReached) break;
            if (t.atEnd()) return false;
        }
    }
    else
    {
        QString s="";
        for (int i=0;i<linesNumber;i++)
        {
            s=t.readLine();
            
            if (removeNonePrintable)
            {
                QString ss=s; s="";
                int smax=ss.length(); if (smax>10000) smax=10000;
                for(int ii=0; ii<smax;ii++) if (ss[ii].isPrint()) s+=ss[ii];
            }
            
            header << s;
            
            if (t.atEnd()) return true;
        }
    }
    file.close();
    
    return true;
    
}


//*******************************************
//+++  Read Header :: spetial Line
//*******************************************
bool dan18::readHeaderLine( QString runNumber, int lineNumber, QString &str )
{
    QString wildCardInUse;
    
    int index=0;
    
    if (runNumber.contains("["))
    {
        index=runNumber.right(runNumber.length()-runNumber.indexOf("[")).remove("[").remove("]").toInt();
    }
    
    
    
    if (lineNumber<=(linesInSeparateHeader+index))
    {
        wildCardInUse=wildCard2nd;
    }
    else
    {
        lineNumber-=linesInSeparateHeader;
        wildCardInUse=wildCard;
    }
    return readHeaderLineFull(filesManager->fileNameFull(runNumber, wildCardInUse), lineNumber, str);
}

bool dan18::readHeaderLineFull( QString fileName, int linesNumber, QString &str )
{
    QFile file(fileName);
    QTextStream t( &file );
    
    
    if (!file.open(QIODevice::ReadOnly)) return false;
    //+++
    for(int skip=0; skip<linesNumber; skip++) str = t.readLine();
    
    file.close();
    
    return true;
    
}


int dan18::readHeaderLineFullIntuitive( QString fileName, int maxLinesNumber, QString &str, int shift )
{
    QString whatToFind=str;
    if (str=="") return 0;
    
    QFile file(fileName);
    QTextStream t( &file );
    
    
    if (!file.open(QIODevice::ReadOnly)) return false;
    //+++
    int skip=0;
    QString s="";
    
    int position;
    QString lineMinus;
    
    while ( skip < maxLinesNumber )
    {
        lineMinus=s;
        s = t.readLine();
        skip++;
        
        if(s.contains(str))
        {
            
            position = s.indexOf(str);
            QString sss=s.mid(position-1,1);
            
            if (position > 0 && sss!=" " && sss!="," && sss!=";") continue;
            
            if (shift==1)  {str=t.readLine(); file.close(); return skip+1;};
            if (shift==-1) {str=lineMinus; file.close(); return skip-1;};
            
            s=s.right(s.length() - position - str.length());
            str=s;
            
            
            return skip;
            
        }
    }
    
    file.close();
    
    
    
    if (s.contains(str))
    {
        str=s.right(s.length()-s.indexOf(str)-str.length());
    }
    else return 0;
    
    
    return skip;
}

//*******************************************
//+++  Compare Two Headers for Merging
//*******************************************
bool dan18::compareTwoHeadersBeforeMerging(QStringList header1, QStringList header2, int &line )
{
    
    line=-1;
    if (header1.size()!=header2.size() ) return false;
    line=20;
    if (header1[20] != header2[20] ) return false;
    line=27;
    if (header1[27] != header2[27] ) return false;
    line=20;
    if (header1[20] != header2[20] ) return false;
    
    line =-1;
    return true;
}

//*******************************************
//+++  extract and convert real time
//*******************************************
double dan18::extractTime(QString timeStr, QString str)
{
    timeStr=timeStr.trimmed();
    double time=timeStr.left(timeStr.indexOf(" ")).toInt();
    QString strAct=timeStr.right(timeStr.length()-timeStr.indexOf(" ")-1).trimmed();
    if (str!=strAct)
    {
        if ( str== "usec")
        {
            if ( strAct== "msec")    time*=1000.0;
            else if (strAct== "sec") time*=1000000.0;
            else if (strAct== "min") time*=60000000.0;
            else if (strAct== "h")   time*=3600000000.0;
        }
        else if (str=="msec")
        {
            if (strAct== "sec")      time*=1000.0;
            else if (strAct== "min") time*=60000.0;
            else if (strAct== "h")   time*=3600000.0;
        }
        else if (str=="sec")
        {
            if (strAct== strAct )
            {
                if (strAct== "min")    time*=60.0;
                else if (strAct== "h") time*=3600.0;
            }
        }
        else if (str== "min" && strAct== "h") time*=60.0;
    }
    return time;
}

int dan18::lengthMainHeader(QString fileName)
{
    
    if (!flexiHeader || flexiStop[0]=="") return spinBoxHeaderNumberLines->value();
    
    
    QFile file( fileName );
    QTextStream t( &file );
    
    if (!file.open(QIODevice::ReadOnly) ) return spinBoxHeaderNumberLines->value();
    
    int res=0;
    
    QString sTmp;
    int symbolsNumber;
    bool endReached=false;
    
    for (int i=0;i<spinBoxHeaderNumberLines->value();i++)
    {
        sTmp=t.readLine(); res++;
        
        for (int iFlex=0; iFlex<flexiStop.count(); iFlex++)
        {
            symbolsNumber=flexiStop[iFlex].length();
            if (sTmp.left(symbolsNumber)==flexiStop[iFlex] || t.atEnd() )
            {
                endReached=true;
                break; // skip header
            }
        }
        if (endReached) break;
    }
    
    file.close();
    return res;
}