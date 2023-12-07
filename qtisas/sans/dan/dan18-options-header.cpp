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

//+++
QString dan18::readNumberString(QString Number, QString &pos, QString &num)
{
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "" +++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos=="" ) { pos="0"; num="0";    return "---";} // not defined
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "const" +++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos.contains("const") )
    {
        pos="0";
        return num;
    }
    if (comboBoxHeaderFormat->currentIndex()==2) return readYAMLentry(Number, pos, num);

    if (comboBoxHeaderFormat->currentIndex()==1) return readXMLentry(Number, pos);
    
    QString line="";
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "{Flexi Position }" ++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos.contains("{") && pos.contains("}"))
    {
        
        pos=pos.trimmed();
        int shiftLine=0;
        
        if (pos.contains("-{"))
        {
            shiftLine--;
            pos=pos.remove("-{");
        }
        if (pos.contains("}+"))
        {
            shiftLine++;
            pos=pos.remove("}+");
        }
        
        
        pos=pos.remove("{").remove("}");
        int posLength=pos.length();
        
        pos=QString::number(readHeaderLineFlexi( Number, pos, line, shiftLine ));
        
        //--- string range ---
        if (num.contains("-"))
        {
            QString sss= line.mid(num.left(num.indexOf("-")).toInt()-1, num.right(num.length()-num.indexOf("-")-1).toInt());
            num=QString::number(num.left (num.indexOf("-")).toInt()+posLength);
            return sss;
        }
        
        //--- separator & position
        if (num.contains("s"))
        {
            QString sep=num.mid(1,1);
            num=num.right(num.length()-2);
            if (num.toInt()>0)
            {
                QString sss=findStringInHeader( line, num.toInt(), sep, num );
                num=QString::number(num.toInt()+posLength);
                return sss;
                
            }
            return "---";
        }
        
        //--- just number position
        QString sss=findNumberInHeader( line, num.toInt(), num);
        num=QString::number(num.toInt()+posLength);
        return sss;
    }
    
    
    int index=0;
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "{Standard reader}" +++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if (pos.left(1) == "["  && pos.right(1) == "]")
    {
        if (Number.contains("["))
        {
            index=Number.right(Number.length()-Number.indexOf("[")).remove("[").remove("]").toInt();
        }
        pos=pos.remove("[").remove("]");
        index+=pos.toInt();
    }
    else index=pos.toInt();
    
    if (index>0 )
    {
        readHeaderLine( Number, index, line );
        
        if (num.contains("-"))
        {
            QString sss= line.mid(num.left(num.indexOf("-")).toInt()-1, num.right(num.length()-num.indexOf("-")-1).toInt());
            num=num.left(num.indexOf("-")).toInt();
            return sss;
        }
        
        if (num.contains("s"))
        {
            QString sep=num.mid(1,1);
            num=num.right(num.length()-2);
            if (num.toInt()>0)
                return findStringInHeader( line, num.toInt(), sep, num );
            return "---";
        }
        
        return findNumberInHeader( line, num.toInt(), num );
    }
    
    pos="0";
    num="0";
    return "---";
}


//+++
double dan18::readNumberDouble(QString Number, QString pos, QString num)
{
    return readNumberString( Number, pos, num).simplified().toDouble();
}

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


//+++
QString dan18::readNumber(QStringList lst, QString &pos, QString &num, int index, QString Number) // [sec]
{
    if (pos.contains("["))
    {
        pos=QString::number(pos.remove("[").remove("]").toInt()+index);
    }
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "" +++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos=="" ) pos="0"; // not defined
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "const" +++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos.contains("const") ) return num;

    if (comboBoxHeaderFormat->currentIndex()==2) return readYAMLentry(Number, pos, num);
    
    if (comboBoxHeaderFormat->currentIndex()==1) return readXMLentry(Number, pos);
 
    if (lst.count()==0) return "";
    
    QString line="";
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "{Flexi Position }" ++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    if ( pos.contains("{") && pos.contains("}"))
    {
        int shiftLine=0;
        
        if (pos.contains("-{"))
        {
            shiftLine--;
            pos=pos.remove("-{");
        }
        if (pos.contains("}+"))
        {
            shiftLine++;
            pos=pos.remove("}+");
        }
        
        pos=pos.remove("{").remove("}");
        readHeaderLineFlexi( Number, pos, line,shiftLine );
        line=line.remove(pos);
        if (line=="") return "";
        
        //--- string range ---
        if (num.contains("-"))
        {
            return line.mid(num.left(num.indexOf("-")).toInt()-1, num.right(num.length()-num.indexOf("-")-1).toInt());
        }
        
        if (line=="") return "";
        
        //--- separator & position
        if (num.contains("s"))
        {
            QString sep=num.mid(1,1);
            num=num.right(num.length()-2);

            if (num.toInt()>0)
                return findStringInHeader( line, num.toInt(), sep, num );
            return "";
        }
        
        //--- just number position
        return findNumberInHeader( line, num.toInt(), num );
    }
    
    
    
    
    //++++++++++++++++++++++++++++++++++++
    //+++ "{Standard reader}" +++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++
    
    
    if (pos.toInt()>0)
    {
        QString line=lst[pos.toInt() - 1];
        
        if (num.contains("-"))
        {
            return line.mid(num.left(num.indexOf("-")).toInt()-1, num.right(num.length()-num.indexOf("-")-1).toInt());
        }
        
        if (num.contains("s"))
        {
            QString sep=num.mid(1,1);
            num=num.right(num.length()-2);
            if (num.toInt()>0)
                return findStringInHeader( line, num.toInt(), sep, num);
            return "";
        }
        
        if (num.toInt()>0)
            return findNumberInHeader( line, num.toInt(), num );
    }
    
    return "---";
}
//+++ read duration
double dan18::readDuration( QString Number ) // [sec]
{
    int indexInHeader=listOfHeaders.indexOf("[Duration]");
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double duration=readNumberDouble( Number, pos, num );
    
    if (comboBoxUnitsTime->currentIndex()==1) duration/=10.0;
    if (comboBoxUnitsTime->currentIndex()==2) duration/=1000.0;
    if (comboBoxUnitsTime->currentIndex()==3) duration/=1000000.0;
    
    return duration;
}


//+++ read duration
double dan18::readDuration( QStringList lst, int index, QString Number ) // [sec]
{
    int indexInHeader=listOfHeaders.indexOf("[Duration]");
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double duration=readNumber( lst, pos, num, index, Number ).simplified().toDouble();
    
    if (comboBoxUnitsTime->currentIndex()==1) duration/=10.0;
    if (comboBoxUnitsTime->currentIndex()==2) duration/=1000.0;
    if (comboBoxUnitsTime->currentIndex()==3) duration/=1000000.0;
    
    return duration;
}
//+++ read  Monitor1
double dan18::readMonitor1(const QString &Number, double deadTime)
{
    int indexInHeader = listOfHeaders.indexOf("[Monitor-1]");

    QString pos = tableHeaderPosNew->item(indexInHeader, 0)->text();
    QString num = tableHeaderPosNew->item(indexInHeader, 1)->text();
    double M1 = readNumberString(Number, pos, num).simplified().toDouble();
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
    int indexInHeader = listOfHeaders.indexOf("[Monitor-2]");
    QString pos = tableHeaderPosNew->item(indexInHeader, 0)->text();
    QString num = tableHeaderPosNew->item(indexInHeader, 1)->text();
    double M2 = readNumberString(Number, pos, num).simplified().toDouble();
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
    int indexInHeader = listOfHeaders.indexOf("[Monitor-3|Tr|ROI]");
    QString pos = tableHeaderPosNew->item(indexInHeader, 0)->text();
    QString num = tableHeaderPosNew->item(indexInHeader, 1)->text();
    double M3 = readNumberString(Number, pos, num).simplified().toDouble();
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
int dan18::readRtCurrentNumber( QString Number ) // [1]
{
    QString line="";
    int indexInHeader=listOfHeaders.indexOf("[RT-Current-Number]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int currentNumber=int(readNumberDouble( Number, pos, num ));
    
    if (currentNumber<1) currentNumber=1;
    
    return currentNumber;
}
//+++ read  readTimefactor
int dan18::readRtCurrentNumber( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[RT-Current-Number]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int currentNumber=int(readNumber( lst, pos, num, index, Number ).toDouble());
    
    if (currentNumber<1) currentNumber=1;
    
    return currentNumber;
}


//+++ Timefactor
double dan18::readTimefactor( QString Number ) // [1]
{
    QString line="";
    int indexInHeader=listOfHeaders.indexOf("[RT-Time-Factor]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double readTimefactor=readNumberDouble( Number, pos, num );
    
    if (readTimefactor<=0) readTimefactor=1.0;
    
    return readTimefactor;
}


//+++ read  readTimefactor
double dan18::readTimefactor( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[RT-Time-Factor]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double readTimefactor=readNumber( lst, pos, num, index, Number ).toDouble();
    
    if (readTimefactor<=0) readTimefactor=1.0;
    
    return readTimefactor;
}


//+++ Timefactor numberRepetitions
int dan18::readNumberRepetitions( QString Number ) // [1]
{
    int indexInHeader=listOfHeaders.indexOf("[RT-Number-Repetitions]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int numberRepetitions= readNumberString( Number, pos, num ).toInt();
    if (numberRepetitions<1) numberRepetitions=1;
    
    return numberRepetitions;
}
//+++ read  readNumberRepetitions
int dan18::readNumberRepetitions( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[RT-Number-Repetitions]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int numberRepetitions= readNumber( lst, pos, num, index, Number ).toInt();
    if (numberRepetitions<1) numberRepetitions=1;
    
    return numberRepetitions;
}
//+++ read  Slices-Count
QString dan18::readSlicesCount( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Count]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    return QString::number(readNumber( lst, pos, num, index, Number ).toDouble(),'f',0);
}


//
//+++ read  Slices-Count
int dan18::readSlicesCount( QString Number)
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Count]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int slicesCount=int(readNumberDouble( Number, pos, num ));
    
    if (slicesCount<1) slicesCount=1;
    
    return slicesCount;
}



//+++ read  Slices-Duration
QString dan18::readSlicesDuration( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Duration]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double slicesDuration=readNumber( lst, pos, num, index, Number ).toDouble();
    
    if (comboBoxUnitsTimeRT->currentIndex()==1) slicesDuration/=10.0;
    if (comboBoxUnitsTimeRT->currentIndex()==2) slicesDuration/=1000.0;
    if (comboBoxUnitsTimeRT->currentIndex()==3) slicesDuration/=1000000.0;
    
    return QString::number(slicesDuration);
}


//+++ read  Slices-Duration
double dan18::readSlicesDuration( QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Duration]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double slicesDuration=readNumberDouble( Number, pos, num );
    
    if (comboBoxUnitsTimeRT->currentIndex()==1) slicesDuration/=10.0;
    if (comboBoxUnitsTimeRT->currentIndex()==2) slicesDuration/=1000.0;
    if (comboBoxUnitsTimeRT->currentIndex()==3) slicesDuration/=1000000.0;
    
    
    if (slicesDuration<0) slicesDuration=0;
    
    return slicesDuration;
}



//+++ read  Slices-Current-Number
int dan18::readSlicesCurrentNumber( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Number]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    int SlicesCurrentNumber=readNumber( lst, pos, num, index, Number ).toInt();
    
    if (SlicesCurrentNumber<0) SlicesCurrentNumber=0;
    
    return SlicesCurrentNumber;
}



//+++ read  Slices-Current-Duration
double dan18::readSlicesCurrentDuration( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Duration]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    
    if(num.contains("list"))
    {
        QString pos1=tableHeaderPosNew->item(indexInHeader,0)->text();
        QString num1=tableHeaderPosNew->item(indexInHeader,1)->text();
        
    }
    
    double SlicesCurrentDuration=readNumber( lst, pos, num, index, Number ).toDouble();
    
    
    
    
    if (comboBoxUnitsTimeRT->currentIndex()==1) SlicesCurrentDuration/=10.0;
    if (comboBoxUnitsTimeRT->currentIndex()==2) SlicesCurrentDuration/=1000.0;
    if (comboBoxUnitsTimeRT->currentIndex()==3) SlicesCurrentDuration/=1000000.0;
    
    
    if (SlicesCurrentDuration<0) SlicesCurrentDuration=0;
    
    return SlicesCurrentDuration;
}



//+++ read  Slices-Current-Monitor1
double dan18::readSlicesCurrentMonitor1( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Monitor1]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double SlicesCurrentMonitor1=readNumber( lst, pos, num, index, Number ).toDouble();
    
    
    
    if (SlicesCurrentMonitor1<0) SlicesCurrentMonitor1=0;
    
    return SlicesCurrentMonitor1;
}


//+++ read  Slices-Current-Monitor2
double dan18::readSlicesCurrentMonitor2( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Monitor2]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double SlicesCurrentMonitor2=readNumber( lst, pos, num, index, Number ).toDouble();
    
    if (SlicesCurrentMonitor2<0) SlicesCurrentMonitor2=0;
    
    return SlicesCurrentMonitor2;
}


//+++ read  Slices-Current-Monitor3
double dan18::readSlicesCurrentMonitor3( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Monitor3]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double SlicesCurrentMonitor3=readNumber( lst, pos, num, index, Number ).toDouble();
    
    if (SlicesCurrentMonitor3<0) SlicesCurrentMonitor3=0;
    
    return SlicesCurrentMonitor3;
}


//+++ read  Slices-Current-Sum
double dan18::readSlicesCurrentSum( QStringList lst, int index, QString Number )
{
    int indexInHeader=listOfHeaders.indexOf("[Slices-Current-Sum]");
    
    QString pos=tableHeaderPosNew->item(indexInHeader,0)->text();
    QString num=tableHeaderPosNew->item(indexInHeader,1)->text();
    
    double SlicesCurrentSum=readNumber( lst, pos, num, index, Number ).toDouble();
    
    if (SlicesCurrentSum<0) SlicesCurrentSum=0;
    
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


//*******************************************
//+++  Read Header :: spetial Line
//*******************************************
int dan18::readHeaderLineFlexi( QString runNumber, QString pos, QString &str, int shift )
{
    QString wildCardInUse;

    if (separateHeaderYes)
    {
        wildCardInUse = wildCard2nd;
        str = pos;
        return readHeaderLineFullIntuitive(filesManager->fileNameFull(runNumber, wildCardInUse), linesInSeparateHeader,
                                           str, shift);
    }
    wildCardInUse = wildCard;
    str = pos;
    return readHeaderLineFullIntuitive(filesManager->fileNameFull(runNumber, wildCardInUse),
                                       linesInHeader + linesInDataHeader, str, shift);
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

//+++
QString dan18::findNumberInHeader(QString line, int digitNumber, QString &num)
{
    QString result;
    int posInline=0; num="0";
    int pos=0;
    int currentNumber=0;
    QRegExp rx("((\\-|\\+)?\\d\\d*(\\.\\d*)?((E\\-|E\\+)\\d\\d?\\d?\\d?)?)");
    
    line=line.trimmed();
    line.replace(",",".");
    line.replace("e","E");
    line.replace("E","E0");
    line.replace("E0+","E+0");
    line.replace("E0-","E-0");
    line.replace("E0","E+0");
    line.replace("E+00","E+0");
    line.replace("E-00","E-0");
    
    
    while ( pos >= 0 && currentNumber<digitNumber)
    {
        pos = rx.indexIn( line, pos );
        if ( pos <0 ) return "";
        posInline=pos;
        
        result=rx.cap( 1 );
        
        if ( ( pos==0 || line[pos-1]==' ' || line[pos-1]=='\t') && ( line[pos+rx.matchedLength()]==' ' || line[pos+rx.matchedLength()]=='\t' ||  pos+rx.matchedLength() == line.length())) currentNumber++;
        
        pos  += rx.matchedLength();
    }
    
    //    result.replace("E+0","E+");
    posInline++;
    num=QString::number(posInline);
    return result;
}


//+++
QString dan18::findStringInHeader(QString line, int digitNumber, QString sep, QString &num)
{
    QStringList lst;
    
    lst = line.trimmed().split(sep, QString::SkipEmptyParts);
    
    if ( digitNumber>lst.count() ) return "";
    
    if (digitNumber==0) num="0";
    else
    {
        
        int sum=1;
        for (int i=0; i<digitNumber-1;i++) sum+=lst[i].length()+1;
        num=QString::number(sum);
    }
    
    if ( digitNumber<=lst.count() ) return lst[digitNumber-1];
    
    return "";
}
QString dan18::readYAMLentry(QString runNumber, QString yamlCode, QString num)
{
    QString res = readYAMLentry(runNumber, yamlCode);
    
    //--- string range ---
    if (num.contains("-"))
    {
        return res.mid(num.left(num.indexOf("-")).toInt()-1, num.right(num.length()-num.indexOf("-")-1).toInt());
    }
    
    //--- separator & position
    if (num.contains("[s"))
    {
        num=num.remove("[").remove("]");
        res=res.mid(res.indexOf("[")+1, res.indexOf("]")-res.indexOf("[")-1);
        
        QString sep=num.mid(1,1);
        num=num.right(num.length()-2);
        if (num.toInt()>0)
            return findStringInHeader( res, num.toInt(), sep, num );
        return "";
    }
    
    //--- separator & position
    if (num.contains("(s"))
    {
        num=num.remove("(").remove(")");
        res=res.mid(res.indexOf("(")+1, res.indexOf(")")-res.indexOf("(")-1);
        QString sep=num.mid(1,1);
        
        num=num.right(num.length()-2);
        if (num.toInt()>0)
            return findStringInHeader( res, num.toInt(), sep, num );
        return "";
    }
    
    //--- separator & position
    if (num.contains("{s"))
    {
        num=num.remove("{").remove("}");
        res=res.mid(res.indexOf("{")+1, res.indexOf("}")-res.indexOf("{")-1);
        QString sep=num.mid(1,1);
        num=num.right(num.length()-2);
        if (num.toInt()>0)
            return findStringInHeader( res, num.toInt(), sep, num );
        return "";
    }
    
    //--- separator & position
    if (num.contains("s"))
    {
        QString sep=num.mid(1,1);
        num=num.right(num.length()-2);
        if (num.toInt()>0)
            return findStringInHeader( res, num.toInt(), sep, num );
        return "";
    }
    
    
    return res;
    
}

QString dan18::readYAMLentry(QString runNumber, QString yamlCode)
{
    // +++
    if (!separateHeaderYes) return "-1";
    // +++
    yamlCode=yamlCode.remove(" ");
    yamlCode=yamlCode.replace("::",":");
    yamlCode=yamlCode.replace("::",":");
    
    yamlCode=yamlCode.replace("||","|");
    yamlCode=yamlCode.replace("||","|");
    
    
    // +++
    QStringList lst = yamlCode.split(":", QString::SkipEmptyParts);

    // +++
    int countLevels=lst.count();
    
    // +++
    if (countLevels>4) return "-4";
    
    // +++
    std::ifstream fin(filesManager->fileNameFull(std::move(runNumber), wildCard2nd).toLatin1().constData());
    
    // +++
    try {
        
        YAML::Parser parser(fin);
        YAML::Node doc;

        // +++
        while(parser.GetNextDocument(doc) )
        {
            // first level
            std::string key, value;
            
            for(YAML::Iterator it=doc.begin();it!=doc.end();++it)
            {
                key=""; value="";
                it.first() >> key;
                
                if ( key!=lst[0].toLatin1().constData() ) continue;
                
                if (countLevels==1)
                {
                    it.second()>>value;
                    return value.c_str();
                }
              
                // second level
                std::string key2, value2;
                
                for(YAML::Iterator it2=it.second().begin();it2!=it.second().end();++it2)
                {
                    key2=""; value2="";
                    it2.first() >> key2;
                    
                    if ( key2!=lst[1].toLatin1().constData()) continue;
                    
                    if (countLevels==2)
                    {
                        it2.second()>>value2;
                        return value2.c_str();
                    }
                    
                    
                    // third level
                    std::string key3, value3;

                    
                    if (it2.second().Type() == YAML::NodeType::Sequence)
                    {
                        bool foundYN=false;
                        std::string key3a, value3a;
                        std::string tvalue;
                        QStringList sLst = lst[2].split("|", QString::SkipEmptyParts);
                        
                        if (countLevels>3) return "limit-3-levels";

                        if (sLst.size()==1)
                        {
                            for(unsigned int i = 0 ; i < it2.second().size() ; i++)
                            {
                                it2.second()[i].begin().first() >> key3a;
                                it2.second()[i].begin().second() >> key3;
                               
                                if ( key3a == sLst[0].toLatin1().constData()) return key3.c_str();
                            }
                               return "not-found";
                        }
                        else
                        {
                            for(unsigned int i = 0 ; i < it2.second().size() ; i++)
                            {
                                tvalue="";
                                for(YAML::Iterator it3=it2.second()[i].begin();it3!=it2.second()[i].end();++it3)
                                {
                                    key3a="";
                                    key3="";
                                    
                                    if ( it3.second().Type() == YAML::NodeType::Sequence)
                                    {
                                        it3.first() >> key3a;
                                        
                                        if (sLst.count()<3 || key3a != sLst[2].toLatin1().constData()) continue;
                                        if (it3.second()[0].Type()!= YAML::NodeType::Scalar) continue;
                                        
                                        for(unsigned int i = 0 ; i < it3.second().size() ; i++)
                                        {
                                            it3.second()[i]>>key3;
                                            tvalue+=key3+"; ";
                                        }
                                        continue;
                                    }
                                    
                                    if (it3.second().Type() != YAML::NodeType::Scalar && sLst.count() < 4) continue;
                                    it3.first() >> key3a;

                                     if (it3.second().Type() == YAML::NodeType::Scalar) it3.second() >> key3;
                                    
                                    if ( key3a == sLst[0].toLatin1().constData() && key3 == sLst[1].toLatin1().constData() &&sLst.count() == 3) foundYN=true;
                                    
                                    if ( sLst.count()>2 && key3a == sLst[2].toLatin1().constData()) tvalue=key3;
                                    if ( sLst.count()==2 ) tvalue+=key3+"; ";
                                    
                                    if ( key3a == sLst[2].toLatin1().constData()  && sLst.count() == 4)
                                    {
                                        // third level
                                        std::string key4, value4;
                                        tvalue="";
                                        for(YAML::Iterator it4=it3.second().begin();it4!=it3.second().end();++it4)
                                        {
                                            it4.first() >> key4;
                                            
                                            //if (it4.second().Type() != YAML::NodeType::Scalar) continue;

                                            
                                            if (key4 != sLst[3].toLatin1().constData()) continue;
                                            it4.second() >> value4;
                                            return value4.c_str();
                                        }
                                        
                                    }
                                }
                                if (foundYN ) return tvalue.c_str();
                            }
                        }
                            
                       }
                       else
                       {
                           
                           
                       for(YAML::Iterator it3=it2.second().begin();it3!=it2.second().end();++it3)
                       {
                           key3=""; value3="";
                           it3.first() >> key3;
                           
                           if ( key3!=lst[2].toLatin1().constData()) continue;

                           if (countLevels==3)
                           {
                               it3.second()>>value3;
                               return value3.c_str();
                           }
                           
                           // 4th level
                           std::string key4, value4;
                           
                           if (it3.second().Type() == YAML::NodeType::Sequence)
                           {
                               bool foundYN=false;
                               std::string key4a, value4a;
                               std::string tvalue;
                               QStringList sLst = lst[3].split("|", QString::SkipEmptyParts);
                               
                               if (countLevels>4) return "limit-4-levels";
                               
                               if (sLst.size()==1)
                               {
                                   for(unsigned int i = 0 ; i < it3.second().size() ; i++)
                                   {
                                       it3.second()[i].begin().first() >> key4a;
                                       it3.second()[i].begin().second() >> key4;
                                       
                                       if ( key4a == sLst[0].toLatin1().constData()) return key4.c_str();
                                   }
                                   return "not-found";
                               }
                               else
                               {
                                   for(unsigned int i = 0 ; i < it3.second().size() ; i++)
                                   {
                                       tvalue="";
                                       for(YAML::Iterator it4=it3.second()[i].begin();it4!=it3.second()[i].end();++it4)
                                       {
                                           key4a="";
                                           key4="";
                                           
                                           if ( it4.second().Type() == YAML::NodeType::Sequence)
                                           {
                                               it4.first() >> key4a;
                                               
                                               if (sLst.count()<3 || key4a != sLst[2].toLatin1().constData()) continue;
                                               if (it4.second()[0].Type()!= YAML::NodeType::Scalar) continue;
                                               
                                               for(unsigned int i = 0 ; i < it4.second().size() ; i++)
                                               {
                                                   it4.second()[i]>>key4;
                                                   tvalue+=key4+"; ";
                                               }
                                               continue;
                                           }
                                           
                                           if (it4.second().Type() != YAML::NodeType::Scalar && sLst.count() < 4) continue;
                                           it4.first() >> key4a;
                                           
                                           if (it4.second().Type() == YAML::NodeType::Scalar) it4.second() >> key4;
                                           
                                           if ( key4a == sLst[0].toLatin1().constData() && key4 == sLst[1].toLatin1().constData() &&sLst.count() == 3) foundYN=true;
                                           
                                           if ( sLst.count()>2 && key4a == sLst[2].toLatin1().constData()) tvalue=key4;
                                           if ( sLst.count()==2 ) tvalue+=key4+"; ";

                                           if ( key4a == sLst[2].toLatin1().constData()  && sLst.count() == 4)
                                           {
                                               // third level
                                               std::string key5, value5;
                                               tvalue="";
                                               for(YAML::Iterator it5=it4.second().begin();it5!=it4.second().end();++it5)
                                               {
                                                   it5.first() >> key5;
                                                   
                                                   //if (it4.second().Type() != YAML::NodeType::Scalar) continue;
                                                   
                                                   
                                                   if (key5 != sLst[3].toLatin1().constData()) continue;
                                                   it5.second() >> value5;
                                                   return value5.c_str();
                                               }
                                               
                                           }
                                       }
                                       if (foundYN ) return tvalue.c_str();
                                   }
                               }
                               
                           }
                           else
                           {
                               for(YAML::Iterator it4=it3.second().begin();it4!=it3.second().end();++it4)
                               {
                                   if (countLevels>4) return "limit-4-levels";
                                   key4=""; value4="";
                                   it4.first() >> key4;
                               
                                   if ( key4!=lst[3].toLatin1().constData()) continue;
                               
                                   if (countLevels==4)
                                   {
                                       it4.second()>>value4;
                                       return value4.c_str();
                                   }
                               }
                           }
    
                       }
                       }
                    
                    }
                }

            }

        
        } catch(const YAML::Exception& e) {
        std::cerr << e.what() << "\n";
    }

    
    return "";
}

QString dan18::readXMLentry(QString runNumber,  QString xmlCode)
{
    QString fileName = filesManager->fileNameFull(std::move(runNumber), wildCard2nd);
    QString xmlBase=lineEditXMLbase->text();
    
    xmlBase=xmlBase.remove(" ");
    if (xmlBase!="")
    {
        xmlBase+=":";
        xmlBase=xmlBase.replace("::",":");
        xmlBase=xmlBase.replace("::",":");
    }
    
    xmlCode=xmlBase+xmlCode;
    
    xmlCode=xmlCode.remove(" ");
    xmlCode=xmlCode.replace("::",":");
    xmlCode=xmlCode.replace("::",":");
    
    QStringList lst = xmlCode.split(":", QString::SkipEmptyParts);
    
    
    QDomDocument 	doc;
    QDomElement 		root;
    QDomElement 		element;
    
    
    QString 	errorStr;
    int 		errorLine;
    int 		errorColumn;
    
    
    //+++
    QFile *xmlFile= new QFile(fileName);
    if (!xmlFile->open(QIODevice::ReadOnly)) return "";
    if (!doc.setContent(xmlFile, true, &errorStr, &errorLine,&errorColumn)) return "";
    
    root = doc.documentElement();
    readXMLentry(root, lst, element,0);
    xmlFile->close();
    
    return element.text().simplified();
}


bool dan18::readXMLentry(QDomElement root, QStringList lst, QDomElement &element, int order)
{
    int number=lst.count();
    if (number<1) return false;
    if (number==1)
    {
        if (root.tagName() != lst[0] ) return false;
        element=root;
        return true;
    }
    int numberCycles=0;
    QList<int> positions;
    
    //+++ number of cycles ...
    for (int i=1; i<number;i++)
    {
        if (lst[i].contains("{") && lst[i].contains("}"))
        {
            lst[i]=lst[i].remove("}");
            lst[i]=lst[i].remove("{");
            
            positions<<i;
            numberCycles++;
        }
    }
    
    if (numberCycles>2) return  false;
    
    
    if (numberCycles<=1)
    {
        if (root.tagName() != lst[0])
        {
            qWarning("XML(0-1) file error: level # 0");
            return false;
        }
        
        QDomNode node = root.firstChild();
        bool nodeFound=false;
        
        bool repeatedNote;
        int repeatCounter;
        
        for(int i=1; i<number;i++)
        {
            repeatedNote=false;
            repeatCounter=0;
            
            if (lst[i].contains("{") && lst[i].contains("}"))
            {
                repeatedNote=true;
                lst[i]=lst[i].remove("{");
                lst[i]=lst[i].remove("}");
            }
            
            while (!node.isNull() && !nodeFound )
            {
                
                if (node.toElement().tagName() == lst[i] )
                {
                    if (repeatCounter<order && repeatedNote)
                    {
                        repeatCounter++;
                        node=node.nextSibling();
                    }
                    else
                    {
                        //qWarning("current node %s", node.toElement().tagName());
                        if (i<(number-1)) node = node.firstChild();
                        nodeFound=true;
                    }
                }
                else node = node.nextSibling();
            }
            if (!nodeFound)
            {
                qWarning("XML file error: level # %d, %s", i, lst[i].toLatin1().constData());
                return false;
            }
            nodeFound=false;
        }
        if (node.toElement().tagName() == lst[number-1]) element=node.toElement();
        else return false;
    }
    
    if (numberCycles==2)
    {
        int i=positions[0];
        
        if (root.tagName() != lst[0])
        {
            return -5;
        }
        
        QDomNode node = root.firstChild();
        
        bool nodeFound=false;	
        bool repeatedNote;
        int repeatCounter2=0;
        
        for(int ii=1; ii<=positions[0];ii++)
        {
            nodeFound=false;
            repeatedNote=false;
            
            if (ii==positions[0]) repeatedNote=true;
            
            while (!node.isNull() && !nodeFound ) 
            {	
                if (node.toElement().tagName() == lst[ii] )
                {	
                    if (repeatedNote) 
                    {			
                        QDomNode node2 = node;
                        bool nodeFound2=false;	
                        bool repeatedNote2;
                        
                        for(int iii=ii; iii<=positions[1];iii++)
                        {
                            nodeFound2=false;
                            repeatedNote2=false;
                            
                            if (iii==positions[1]) repeatedNote2=true;
                            
                            while (!node2.isNull() && !nodeFound2 ) 
                            {	
                                if (node2.toElement().tagName() == lst[iii] )
                                {	
                                    if (repeatedNote2) 
                                    {    	
                                        if (repeatCounter2==order) 
                                        {
                                            if (iii<number-1)
                                            {
                                                for(int iiii=iii+1; iiii<number;iiii++) 
                                                {
                                                    node2 = node2.firstChild();
                                                    while (!node2.isNull() && node2.toElement().tagName() != lst[iiii])
                                                        node2 = node2.nextSibling();
                                                }
                                            }
                                            element=node2.toElement();
                                            return true;
                                        }
                                        
                                        node2 = node2.nextSibling(); 
                                        repeatCounter2++;
                                        if (node2.toElement().tagName() != lst[positions[1]] ) break;
                                        
                                    }
                                    else
                                    {
                                        nodeFound2=true;
                                        if (iii<positions[1]) node2 = node2.firstChild();    
                                    }
                                }
                                else
                                {
                                    node2 = node2.nextSibling();
                                }
                            }
                            nodeFound2=false;
                        }
                        
                        node = node.nextSibling(); 
                        if (node.toElement().tagName() != lst[positions[0]] ) return repeatCounter2;
                    }
                    else
                    {
                        nodeFound=true;
                        if (ii<positions[0]) node = node.firstChild();    
                    }
                }
                else
                {
                    node = node.nextSibling();
                }
            }
            if (!nodeFound) 
            {
                qWarning("XML(2) file error: level # %d, %s", ii, lst[ii].toLatin1().constData());
                return 0;
            }
            if (repeatedNote) return repeatCounter2;
            nodeFound=false;
        }
        return -5;
    }    
    
    
    return true;
}


