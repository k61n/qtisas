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
        sample = monitors->readMonitor3Normalized(NumberSample);
        if (fabs(monitors->readMonitor3(NumberSample)) > 0.0)
            sigma2 += 1 / fabs(monitors->readMonitor3(NumberSample));
        ec = monitors->readMonitor3Normalized(NumberEC);
        if (fabs(monitors->readMonitor3(NumberEC)) > 0.0)
            sigma2 += 1 / fabs(monitors->readMonitor3(NumberEC));
    } // Direct Beam  [dead-time+]
    else if (comboBoxTransmMethod->currentIndex() == 1)
    {
        sample = integralVSmaskUniDeadTimeCorrected(NumberSample, mask, VShift, HShift);
        if (fabs(sample) > 0.0)
            sigma2 += 1 / fabs(sample);

        sample =
            sample / monitors->deadTimeFactorDetector(NumberSample) * monitors->deadTimeFactorDetectorDB(NumberSample);
        sample *= monitors->normalizationFactor(NumberSample);

        ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
        if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error
        
        ec = ec / monitors->deadTimeFactorDetector(NumberEC) * monitors->deadTimeFactorDetectorDB(NumberEC);
        ec *= monitors->normalizationFactor(NumberEC);

    } //Tr in Header  [dead-time -]
    else if (comboBoxTransmMethod->currentIndex()==2)
    {
        sample = monitors->readMonitor3(NumberSample);
        ec = monitors->readMonitor3(NumberEC);
        if (ec == 0)
            ec = 1;
    } // ROI in Header  [dead-time +]
    else if (comboBoxTransmMethod->currentIndex() == 3)
    {
        sample = monitors->readMonitor3Normalized(NumberSample);
        if (fabs(monitors->readMonitor3(NumberSample)) > 0.0)
            sigma2 += 1 / fabs(monitors->readMonitor3(NumberSample));
        sample *= monitors->deadTimeFactorDetectorDB(NumberSample);

        ec = monitors->readMonitor3Normalized(NumberEC);
        if (fabs(monitors->readMonitor3(NumberEC)) > 0.0)
            sigma2 += 1 / fabs(monitors->readMonitor3(NumberEC));

        ec *= monitors->deadTimeFactorDetectorDB(NumberEC);
    }
    else if (comboBoxTransmMethod->currentIndex() == 4)
    {
        if (selector->readLambda(NumberSample, monitors->readDuration(NumberSample)) < 9.5)
        { // ROI in Header  [dead-time +]
            sample = monitors->readMonitor3Normalized(NumberSample);
            if (fabs(monitors->readMonitor3(NumberSample)) > 0.0)
                sigma2 += 1 / fabs(monitors->readMonitor3(NumberSample));

            sample *= monitors->deadTimeFactorDetectorDB(NumberSample);

            ec = monitors->readMonitor3Normalized(NumberEC);
            if (fabs(monitors->readMonitor3(NumberEC)) > 0.0)
                sigma2 += 1 / fabs(monitors->readMonitor3(NumberEC));

            ec *= monitors->deadTimeFactorDetectorDB(NumberEC);
        }
        else
        { // Direct Beam  [dead-time+]
            sample = integralVSmaskUniDeadTimeCorrected(NumberSample, mask, VShift, HShift);
            if (fabs(sample) > 0.0)
                sigma2 += 1 / fabs(sample);

            sample = sample / monitors->deadTimeFactorDetector(NumberSample) *
                     monitors->deadTimeFactorDetectorDB(NumberSample);
            sample *= monitors->normalizationFactor(NumberSample);

            ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
            if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error

            ec = ec / monitors->deadTimeFactorDetector(NumberEC) * monitors->deadTimeFactorDetectorDB(NumberEC);
            ec *= monitors->normalizationFactor(NumberEC);
        }
    }

    if (comboBoxNorm->currentText() != "Time" && comboBoxTransmMethod->currentIndex() != 2)
    {
        sigma2 += fabs(monitors->normalizationFactor(NumberEC));
        sigma2 += fabs(monitors->normalizationFactor(NumberSample));
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
            
    sample = sample / monitors->deadTimeFactorDetector(NumberSample) * monitors->deadTimeFactorDetectorDB(NumberSample);
    sample *= monitors->normalizationFactor(NumberSample);

    ec=integralVSmaskUniDeadTimeCorrected( NumberEC, mask, 0, 0 );
    std::cout<<ec<<"\n"<<std::flush;
    if (fabs(ec)>0.0) sigma2 += 1/fabs(ec); //+++2019 error
                
    ec = ec / monitors->deadTimeFactorDetector(NumberEC) * monitors->deadTimeFactorDetectorDB(NumberEC);
    ec *= monitors->normalizationFactor(NumberEC);

    if (comboBoxNorm->currentText() != "Time" && comboBoxTransmMethod->currentIndex() != 2)
    {
        sigma2 += fabs(monitors->normalizationFactor(NumberEC));
        sigma2 += fabs(monitors->normalizationFactor(NumberSample));
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