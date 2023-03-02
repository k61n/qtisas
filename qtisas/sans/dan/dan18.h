/***************************************************************************
 File : dan18.h
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


#ifndef DAN18_H
#define DAN18_H

#include "ApplicationWindow.h"
#include <Matrix.h>
#include "ui_dan.h"
#include "../common/combobox-in-table.h"

#include <QTableWidget>
#include <QSettings>
#include <QLabel>
#include <QDir>
#include <QInputDialog>
#include <QDomElement>
#include <QColorDialog>
#include <QTextStream>
#include <QFileDialog>
#include <QProgressDialog>
#include <QImageWriter>
#include <QMdiArea>

#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>

#include "PlotCurve.h"
#include <gsl/gsl_matrix.h>

#ifdef YAMLYAML
#include "yaml-cpp/yaml.h"
#endif

#include <Spectrogram.h>

#include "../common/msleep.h"

class dan18 : public QWidget, private Ui::dan
{
    Q_OBJECT
public:
    dan18(QWidget *parent = 0);
    ~dan18();

    ApplicationWindow* app();
    void toResLog( QString text );
    
    // init
    void changeFixedSizeH(QWidget *obj, int H);
    void changeFixedSizeHW(QWidget *obj, int H, int W);
    void setFontForce(QFont font);
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);
    
    void connectSlot();
    void optionsConnectSlot();
    void rawdataConnectSlot();
    void maskConnectSlot();
    void sensConnectSlot();
    void addfilesConnectSlots();
    void processdataConnectSlots();
    void dandanConnectSlots();
    void rtConnectSlots();
    void tofConnectSlots();
    void extractorConnectSlots();
    void mergeSlots();
    
    // settings
    void readSettings();
    void writeSettings();
    
    // init
    void ImportantConstants();
    bool checkTableExistence(QString &tableName);
    bool checkTableExistence(QString &tableName, Table* &t);
    bool checkNoteExistence(QString &noteName);
    bool checkFileNumber( QString Number );
    QString findFileNumberInFileName(QString wildCardLocal, QString file);
    void expandModeSelection( bool YN);
    bool existWindow(QString name);
    void maximizeWindow(QString name);
    void initDAN();
    void advUser();
    void findSANSinstruments();
    void saveInstrumentAsCpp(QString instrPath, QString instrName);
    bool checkDataPath();
    bool selectFile(QString &fileNumber);
    bool findActiveGraph( Graph * & g); // move to standard : from fittingTable & Dan
    bool findFitDataTable(QString curveName, Table* &table, int &xColIndex, int &yColIndex ); // move to standard : from fittingTable & Dan
    bool AddCurve(Graph* g,QString curveName); // move to standard : from fittingTable & Dan
    
    //options
    QString fileNameUni( QString wildCardLocal, QString Number );
    QString newFileNameUni( QString wildCardLocal, QString Number );
    void deleteObjectByLabel(QString winLabel);
    void removeWindows(QString pattern);
    void findCalibrators();
    void calibratorChanged();
    
    // options header
    QString readNumberString( QString Number, QString & pos, QString & num );
    double readNumberDouble( QString Number, QString pos, QString num );
    double readDataDeadTime( QString Number );
    double readDataDeadTimeDB( QString Number );
    double readDataNormalization( QString Number );
    double readDataNormalizationRT( QString Number );
    double readDataM3norm( QString Number );
    QString readNumber( QStringList lst, QString & pos, QString & num, int index, QString Number );
    QString readInfo( QString Number );
    QString readInfo( QStringList lst, int index, QString Number );
    QString readRun( QStringList lst, int index, QString Number );
    int readDataIntC( QString Number );
    int readDataIntC( QStringList lst, int index, QString Number );
    int readDataIntCinM( QString Number );
    int readDataIntCinM( QStringList lst, int index, QString Number );
    double readDataD( QString Number );
    double readDataD( QStringList lst, int index, QString Number );
    double readDataDinM( QString Number );
    double readDataDinM( QStringList lst, int index, QString Number );
    double readDataR1( QString Number );
    double readDataR1( QStringList lst, int index, QString Number );
    double readDataR2( QString Number );
    double readDataR2( QStringList lst, int index, QString Number );
    QString readCA( QString Number );
    QString readCA( QStringList lst, int index, QString Number );
    QString readSA( QString Number );
    QString readSA( QStringList lst, int index, QString Number );
    double readLambda( QString Number );
    double readLambda( QStringList lst, int index, QString Number );
    double readSum( QString Number );
    double readSum( QStringList lst, int index, QString Number );
    double readDuration( QString Number );
    double readDuration( QStringList lst, int index, QString Number );
    QString readSampleNumber( QString Number );
    QString readSampleNumber( QStringList lst, int index, QString Number );
    double readThickness( QString Number );
    double readThickness( QStringList lst, int index, QString Number );
    double readDataSelector( QString Number );
    double readDataF( QString Number );
    double readDataF( QStringList lst, int index, QString Number );
    double readMonitor1( QString Number );
    double readMonitor1( QStringList lst, int index, QString Number );
    double readMonitor2( QString Number );
    double readMonitor2( QStringList lst, int index, QString Number );
    double readMonitor3( QString Number );
    double readMonitor3( QStringList lst, int index, QString Number );
    int readRtCurrentNumber( QString Number );
    int readRtCurrentNumber( QStringList lst, int index, QString Number );
    double readTimefactor( QString Number );
    double readTimefactor( QStringList lst, int index, QString Number );
    int readNumberRepetitions( QString Number );
    int readNumberRepetitions( QStringList lst, int index, QString Number );
    double readDeltaLambda( QString Number );
    double readDeltaLambda( QStringList lst, int index, QString Number );
    QString readExpName( QStringList lst, int index, QString Number );
    QString readWho( QStringList lst, int index, QString Number );
    QString readTime( QStringList lst, int index, QString Number );
    QString readDate( QStringList lst, int index, QString Number );
    QString readComment1( QStringList lst, int index, QString Number );
    QString readComment2( QStringList lst, int index, QString Number );
    QString readDetectorX( QStringList lst, int index, QString Number );
    double readDetectorX( QString Number );
    QString readDetectorY( QStringList lst, int index, QString Number );
    double readDetectorY( QString Number );

    double readDetRotationX( QStringList lst, int index, QString Number );
    double readDetRotationX( QString Number );
    double readDetRotationY( QStringList lst, int index, QString Number );
    double readDetRotationY( QString Number );
    
    QString readSMotor1( QStringList lst, int index, QString Number );
    QString readSMotor2( QStringList lst, int index, QString Number );
    QString readSMotor3( QStringList lst, int index, QString Number );
    QString readSMotor4( QStringList lst, int index, QString Number );
    QString readSMotor5( QStringList lst, int index, QString Number );
    QString readAttenuator( QString Number );
    QString readPolarization( QString Number );
    QString readLenses( QString Number );
    QString readSlicesCount( QStringList lst, int index, QString Number );
    int readSlicesCount( QString Number );
    QString readSlicesDuration( QStringList lst, int index, QString Number );
    double readSlicesDuration( QString Number );
    int readSlicesCurrentNumber( QStringList lst, int index, QString Number );
    double readSlicesCurrentDuration( QStringList lst, int index, QString Number );
    double readSlicesCurrentMonitor1( QStringList lst, int index, QString Number );
    double readSlicesCurrentMonitor2( QStringList lst, int index, QString Number );
    double readSlicesCurrentMonitor3( QStringList lst, int index, QString Number );
    double readSlicesCurrentSum( QStringList lst, int index, QString Number );
    double readTransmission(QString NumberSample,QString NumberEC,QString mask,double VShift,double HShift,double &sigmaTr);
    double readTransmissionMaskDB(QString NumberSample,QString NumberEC,double VShift,double HShift, double XCenter, double YCenter, double Radius, double &sigmaTr); //+++ 2021-04
    bool readHeaderNumberFull( QString Number, QStringList &header );
    bool readHeaderFile( QString fileName, int linesNumber, QStringList &header );
    bool readHeaderNumber( QString wildCardLocal, QString Number, int linesNumber, QStringList &header );
    bool readHeaderLine( QString runNumber, int lineNumber, QString &str );
    int readHeaderLineFlexi( QString runNumber, QString pos, QString &str, int shift );
    bool readHeaderLineFull( QString fileName, int linesNumber, QString &str );
    int readHeaderLineFullIntuitive( QString fileName, int maxLinesNumber, QString &str, int shift );
    bool compareTwoHeadersBeforeMerging(QStringList header1, QStringList header2, int &line );
    double extractTime(QString timeStr, QString str);
    int lengthMainHeader(QString fileName);
    QString findNumberInHeader(QString line, int digitNumber, QString &num);
    QString findStringInHeader(QString line, int digitNumber, QString sep, QString &num);
#ifdef YAMLYAML
    QString readYAMLentry(QString runNumber, QString yamlCode, QString num);
    QString readYAMLentry(QString runNumber, QString yamlCode);
#endif
    QString readXMLentry(QString runNumber,  QString xmlCode);
    bool readXMLentry(QDomElement root, QStringList lst, QDomElement &element, int order);
    
    
    // rawdata-tools
    void check(QString NumberIn, bool fromComboBox);
    void check(QString NumberIn, bool fromComboBox, QString whatToCheck);
    void openHeaderInNote( QString Number, bool activeYN );
    void newInfoTable(QString TableName);
    void viewIQ(QString whatToCheck, QString Number, QStringList lstNumberIn);
    void viewMatrixReduction(QString Number, QStringList lstNumberIn, bool activeYN);
    bool callFromTerminal(QString commandLine);
    
    // rawdata-realtime
    int rtMerge( int initialNumberFrames, int linearMerging, int geometricalMerging, QStringList inputFiles, QStringList outputFiles );
    void rtSumRead( int numberFrames, QStringList inputFiles, QString tableName );
    void rtSumReadBinary( int numberFrames, QStringList inputFiles, QString tableName );
    void rtSplit( int numberFrames, QStringList inputFiles, QStringList outputFiles );
    int geomerticalProgressionCalculation(int numberFrames, int geometricalRatio, QList<int> &geometricalSplitting);
    bool addNheadersUni(QStringList files, QStringList fileNumers, QStringList &header);
    bool addNmatrixesUni(QStringList files, QStringList fileNumers, QStringList &header);
    bool addGZippedMatrixes(QStringList fileNumers, QString file);
    void addNfilesUniASCII(QStringList files, QStringList fileNumers, QString file);
#ifdef YAMLYAML
    bool addNheadersYaml(QStringList fileNumers, QString fileName);
    void addNfilesYaml(QStringList files, QStringList fileNumers, QString file);
#endif
    // rawdata-tof
    void tofrtAddFiles(QStringList selectedDat, QString file );
    bool addNmatrixesTof(QStringList files, QStringList fileNumers, QStringList &matrixesText, int mainHeaderLength, int framesCount);
    bool addNmatrixes2016(QStringList files, QStringList fileNumers, QTextStream &matrixesText, int mainHeaderLength, int framesCount);
    bool addNmatrixes2016matrix(QStringList files, QStringList fileNumers, QTextStream &matrixesText, int mainHeaderLength, int framesCount);
    int  tofSumCulculate(QStringList lst, int matrixInFileLength, int numberTofPerLine);
    void tofSumRead(int numberFrames, QStringList inputFiles, QString tableName);
    void tofShift(int shift, int numberFrames, QStringList inputFiles, QStringList outputFiles);
    void tofCollapse(int collapse, int numberFrames, QStringList inputFiles, QStringList outputFiles);
    void tofRemove(int remove, int numberFrames, QStringList inputFiles, QStringList outputFiles);
    void tofMergeFrames(int merge, QStringList &lst, int tofHeaderBeforeLength, int matrixInFileLength, int frameLength, int numberTofPerLine);
    void tofMerge(int merge, int numberFrames, QStringList inputFiles, QStringList outputFiles);
    int  tofSplitConvert128to8(QStringList &lst, int DIM, int matrixInFileLength, int numberTofPerLine, int numberTofPerLineNormal);
    void tofSplit(int numberFrames, QStringList inputFiles, QStringList outputFiles);
    
    // rawdata-addfiles
    void addSeveralFilesUniSingleFrame(QStringList selectedFileList, QStringList selectedNumberList, QString fileName);
    void addSeveralFilesUniSingleFrame(QStringList selectedNumberList, QString fileNumber);
    bool generateTemplateToAddeFiles(QStringList selectedNumbers );
    QString configurationPlusSampleName(QString Number);
    
    //rawdata-extractor
    void extractorInit();
    void newInfoExtractor(QString TableName);
    QString generateUniqueStringInList (QStringList lst, const QString str);
    QString getHeaderInfoString(QString number, QString name);
    
    
    // mask
    void saveMaskAs(QString maskName);
    void createMaskFul      ( QString maskName );
    void createMaskFullTr   ( QString maskName );
    void loadMaskFul( QString maskName, QString maskFileName);
    void addBS2CurrentMask( QString maskName );
    bool maskTriangle(Matrix *m, int md, int x1, int y1, int x2, int y2, int x3, int y3);
    bool maskTriangle(Matrix *m, int md);
    bool readDisplay(double &x, double &y);
    bool checkExistenceOfMask(QString MaDe, QString maskToCheck);
    bool checkExistenceOfMaskNoMessage(QString MaDe, QString maskToCheck);
    void updateMaskList();
    
    // sens
    void createSensFul(QString sensName);
    void loadSensFul( QString sensName, QString sensFileName);
    bool checkExistenceOfSens(QString MaDe, QString sensToCheck);
    bool checkExistenceOfSensNoMessage(QString MaDe, QString sensToCheck);
    void updateSensList();
    QString getSensitivityNumber( QString sensName );
    void saveSensAs(QString sensName);
    
    // process data
    void findSettingTables();
    void saveSettings(QString tableName);
    bool readSettingNew(QString tableName );
    void vertHeaderTableECPressed(int raw,  bool headerReallyPressed );
    void horHeaderTableECPressed(int col,  bool headerReallyPressed) ;
    void calculateTransmission(int startRow);
    bool calcAbsCalNew( int col );
    bool calcAbsCalDB( int col );
    bool calcAbsCalTrFs( int col );
    bool calcAbsCalNew( );
    void updateScriptTables();
    bool comparePolarization( QString n1, QString n2 );
    bool compareBeamPosition( QString n1, QString n2 );
    bool compareAttenuators( QString n1, QString n2 );
    bool compareSamplePositions( QString n1, QString n2 );
    bool compareDetAnglePosition( QString n1, QString n2 );
    double tCalc(double lambda);
    double muCalc(double lambda);
    void readCenterfromMaskName( QString maskName, double &Xc, double &Yc, int MD );
    void updateCenterAll();
    void updateCenter(int startRow);
    void sensAndMaskSynchro( gsl_matrix* &mask, gsl_matrix* &sens, int MaDe );
    void copycorrespondentTransmissions();
    void copyCorrespondentTransmissions(int startRow);

    void updateMaskNamesInScript(int startRow, QString headerName);
    void updateColInScriptAll(QString colName, int rowIndex);
    void updateColInScript(int startRow, QString colName, int rowIndex);
    bool generateMergingTable(Table *scriptTable, QStringList generatedTables );
    
    // dandan
    void danDanMultiButton(QString button);
    bool danDanMultiButtonSingleLine(   QString button,
                                     QString label, QString Nsample,QString NEC, QString NBC, QString Nbuffer, QString maskName, QString sensName,
                                     double Detector, double C, double Lambda,
                                     double trans, double transBuffer, double fractionBuffer, double thickness, double abs0,
                                     double Xcenter, double Ycenter,
                                     double scale, double BackgroundConst,double VShift,double HShift
                                     );
    void radUniStandartMSmode
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask, double Xcenter, double Ycenter,
     QString &sampleMatrix, double C, double lambda, double deltaLambda, double detdist, double detelem,
     double r1, double r2, QString label, double numberF, double pixelAsymetry, double angle
     );
    
    void radUni
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask, double Xcenter, double Ycenter,
     QString &sampleMatrix, double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2,
     QString label, double numberF, double pixelAsymetry, double DetectorHShiftAngle, double DetectorVShiftAngle, double angleAnisotropy
     );

    void radAvStd
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
     double Xcenter, double Ycenter,
     double lambda, double detdist, double detelem,
     double pixelAsymetry, double DetRotationX, double DetRotationY, double angleAnisotropy,
     int &nTotal, double *&QQ, double *&II, double *&dII, double *&dIIIIcos2phi, double *&nn
     );
    void radAvDataCorrectionOptions(int nTotal, double *&QQ, double *&II, double *&dII, double *&IIcos2phi, int *&mergedPoints,  double *&nn, int &nCurrent);
    void radAvTableGeneration(QString &sampleMatrix, QString label, int N, double *Q,double *I, double *dI, double *dQ, double *sigma, double *anisotropy);
    void radAvASCIIGeneration(QString &sampleMatrix, QString label, int N, double *Q,double *I, double *dI, double *dQ, double *sigma, double *anisotropy);
    void radAvDataChangeSASpresentation(int nCurrent, double *&QQ, double *&II, double *&dII,  double *&dQQ);
    

    void radAvHF
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask,
     double Xcenter, double Ycenter,
     double lambda, double detdist, double detelem,
     double pixelAsymetry, double DetRotationX, double DetRotationY, double angleAnisotropy,
     int &nTotal, double *&QQ, double *&II, double *&dII, double *&dIIIIcos2phi, double *&nn
     );
    
    void horizontalSlice
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix,
     double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2, QString label
     );
    void verticalSlice
    (
     int md, gsl_matrix *Sample, gsl_matrix *SampleErr, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix,
     double C, double lambda, double deltaLambda, double detdist, double detelem, double r1, double r2, QString label
     );
    void radUniPolar
    (
     int md, gsl_matrix *Sample, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix,
     double lambda, double detdist, double detelem, double pixelAsymetry
     ); //+++ TODOAsymetry
    void sigmaMatrix
    (
     int md, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix,
     double lambda, double deltaLambda, double C, double detdist, double detelem, double r1, double r2
     );
    void sigmaGslMatrix
    (
     gsl_matrix *sigmaMa, int md, gsl_matrix *mask, double Xcenter, double Ycenter,
     double lambda, double deltaLambda, double C, double detdist, double detelem, double r1, double r2
     );
    void MatrixQ  ( int md, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix, double lambda, double detdist, double detelem, double pixelAsymetry, double DetectorHShiftAngle, double DetectorVShiftAngle );
    void dQmatrix ( int md, gsl_matrix *mask, double Xcenter, double Ycenter, QString sampleMatrix, double lambda, double detdist, double detelem, double pixelAsymetry );
    
    double sigma( double Q, double D, double C, double Lambda, double deltaLambda, double r1, double r2);
    double sigmaQmerged( double Q, double D, double C, double Lambda, double deltaLambda, double r1, double r2, int numberMergedQ);
    double dQ( double Q, int numberF , double d, double D, double pixel );
    // dandan-center
    void calcCenterNew();
    void calcCenterNew(int col);
    void calcCenterNew(QString sampleFile, int col, double &Xc, double &Yc, double &XcErr,double &YcErr, QString maskName, QString sensName);
    void calcCenterUniSymmetryX( int md, gsl_matrix *corund, gsl_matrix *mask, double &Xc, double &XcErr);
    void calcCenterUniSymmetryY (int md, gsl_matrix *corund, gsl_matrix *mask, double &Yc, double &YcErr);
    void calcCenterUniHF(int md, gsl_matrix *corund, gsl_matrix *mask, double &Xc, double &Yc, double &XcErr, double &YcErr);
    
    // matrix
    void findMatrixListByLabel(QString winLabelMask,QStringList  &listMask);
    void findTableListByLabel(QString winLabel,QStringList  &list);
    void makeMatrixSymmetric( gsl_matrix * gmatrix, QString name, QString label, int MD, bool hide = false );
    void makeMatrixSymmetric( gsl_matrix * gmatrix, QString name, QString label, int MD, double xs, double xe, double ys, double ye, bool hide = false );
    void makeMatrixUni( gsl_matrix * gmatrix, QString name, int xDim, int yDim, double xs, double xe, double ys, double ye,bool hide, bool maximizeNewYN = false);
    void makeMatrixUni(gsl_matrix * gmatrix, QString name, QString label, int xDim, int yDim,bool hide, bool maximizeNewYN = false);
    void makeMatrixUni(gsl_matrix * gmatrix, QString name, QString label, int xDim, int yDim, double xs, double xe, double ys, double ye,bool hide, bool maximizeNewYN = false);
    bool make_GSL_Matrix_Symmetric( QString mName, gsl_matrix * &gmatrix, int MD );
    bool make_GSL_Matrix_Uni( QString mName, gsl_matrix * &gmatrix, int &xDim, int &yDim, QString &label);
    void readMatrixCor( QString Number, gsl_matrix * &data );
    void readMatrixCorTimeNormalizationOnly( QString Number, gsl_matrix * &data );
    void readMatrixCor( QString Number, int DD, int RegionOfInteres, int binning, int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix * &data);
    void readMatrixCorTimeNormalizationOnly( QString Number, int DD, int RegionOfInteres, int binning, int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix * &data );
    void parallaxCorrection( gsl_matrix * &data, double Xc, double Yc, double D, double Tr );
    void transmissionThetaDependenceTrEC( gsl_matrix * &EC, double Xc, double Yc, double D, double Tr );
    void readErrorMatrix( QString Number, gsl_matrix * &error );
    void readErrorMatrixRel( QString Number, gsl_matrix * &error );
    void saveMatrixToFile( QString fname, gsl_matrix * m, int MaDe );
    void saveMatrixToFileInteger(QString fname, gsl_matrix *m, int MaDe);
    void saveMatrixToFile( QString fname, gsl_matrix * m, int MaDeY, int MaDeX );
    void deadtimeMatrix( QString Number, gsl_matrix * & data );
    void gslMatrixX2mX(gsl_matrix *&m);
    void gslMatrixY2mY( gsl_matrix *&m);
    void gslMatrixVShift( gsl_matrix * gmatrix, int MD, int VShift );
    void gslMatrixHShift( gsl_matrix * gmatrix, int MD, int HShift );
    void gslMatrixShift( gsl_matrix * gmatrix, int MD, double HShift, double VShift );
    void saveMatrixAsTableToFile( QString fname, gsl_matrix * i, gsl_matrix * di, gsl_matrix *sigma, gsl_matrix * mask, int MaDe, double xCenter, double yCenter, double wl, double dwl, double d, double xPixel, double yPixel );
    
    void gslMatrixWaTrDet( int md, double Tr, gsl_matrix *sensTrDet, double Xcenter, double Ycenter, double detdist, double detelem, double pixelAsymetry);
    
    double integralVSmaskUni( gsl_matrix * sample, gsl_matrix * mask, int MaDe );
    double integralVSmaskSimmetrical( QString Number );
    double Q2_VS_maskSimmetrical( QString Number, bool showLogYN=false );
    double integralVSmaskUniDeadTimeCorrected( QString Number );
    double integralVSmaskUniDeadTimeCorrected( QString Number, QString maskName, double VShift, double HShift );
    double integralVSmaskUniDeadTimeCorrected(QString Number, gsl_matrix *mask, double VShift, double HShift);//+++ 2021-04
    QString integralVSmaskUniByName( QString fileNumber );
    
    bool readMatrix( QString Number, int DD, int RegionOfInteres, int binning, int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data);
    bool readMatrix ( QString Number, gsl_matrix* &data );
    bool genetateMatrixInMatrix(QStringList selectedFiles, gsl_matrix *bigMatrix, int xFirst, int yFirst, int xLast, int yLast, int cols, int rrInit, int ccInit, int numberMatrixesInit);
    
    bool readMatrixByNameGSL (QString fileName, gsl_matrix* &data );
    bool readMatrixByName ( QString fileName, gsl_matrix* &data );
    bool readMatrixByName (QString fileName, int DD, int RegionOfInteres, int binning, int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data);
    bool readMatrixByName (QString fileName, int DD, int pixelPerLine, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data, bool readFrame);
#ifdef TIFFTIFF
    bool readMatrixByNameTiff( QString fileName, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix);
#endif
    bool readMatrixByNameImage ( QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix);

    bool readMatrixByNameBinaryGZipped( QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix); //+++2021-04
        bool readMatrixFromBiniryFile ( QString fn, int DD, bool XY, bool X2mX, bool Y2mY, gsl_matrix* &matrix);
    bool readMatrixByNameOne (QString fileName, int DD, bool XY, int pixelsInHeader, bool X2mX, bool Y2mY, gsl_matrix* &data);
    bool extractROI(gsl_matrix *bigMatrix, gsl_matrix *smallMatrix, int xFirst, int yFirst, int xLast, int yLast);
    bool readFromEnd (int M, gsl_matrix* &data );
    bool insertMatrixInMatrix(gsl_matrix *bigMatrix, gsl_matrix *smallMatrix, int xFirst, int yFirst);
    double deadTimeFaktor(double I, double tau);
    int matrixConvolusion( gsl_matrix *sample, gsl_matrix *mask, int MD);
    
    // merge functions
    bool checkTableExistance(QString tName, int &Rows, int &Cols, double &Qmin, double &Qmax );
    bool addTable(const QString table, gsl_matrix* &data, int &N, int Rows, int overlap, int &firstPosition);
    void saveMergedMatrix(QString name, QString labelList,gsl_matrix* data, int N, bool loadReso, bool loadQerr, bool tofYN, bool asciiYN);
    void saveMergedMatrixAscii(QString name, gsl_matrix* data, int N, bool loadReso, bool loadQerr, bool tofYN);
    
    //center
    void calculateCentersInScript(int startRow);
    //AF
    void calculateAbsFactorInScript(int startRow);
    //Tr vs DB
    void calculateTrMaskDB(int startRow);
    
    bool tiffData;
    int linesBetweenFrames;
    QStringList listOfHeaders;
    QStringList flexiStop;
    bool flexiHeader;
    bool separateHeaderYes;
    int linesInHeader;
    QString Dir;
    QString strPathRAD;
    QString strPathDAT;
    QString wildCard;
    QString wildCard2nd;
    int linesInSeparateHeader;
    int MD;
    bool dirsInDir;
    int linesInDataHeader;
    bool removeNonePrintable;
    
    
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ 2013 new: Numbers of lines in data-processing-table (dpt) table
    //+++ to simplify in case reordering
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    static const int dptEC=0;       // EC-file
    static const int dptBC=1;       // B4C-file
    static const int dptC=2;        // Collimation
    static const int dptD=3;        // Sample-to-Detector distance
    static const int dptWL=4;       // Wave-Length
    static const int dptBSIZE=5;    // Beam-size
    static const int dptACFS=6;     // Absolute callibration: Plexi...
    static const int dptACEB=7;     // Absolute callibration: EB...
    static const int dptACBC=8;    // Absolute callibration: BC...
    static const int dptDAC=9;     // Absolute callibration: D Plexy...
    static const int dptACMU=10;    // Absolute callibration: Mu...
    static const int dptACTR=11;    // Absolute callibration: Tr...
    static const int dptACFAC=12;    // Absolute callibration: Tr...
    static const int dptCENTER=13;  // Center: File...
    static const int dptCENTERX=14; // Center: X...
    static const int dptCENTERY=15; // Center: Y...
    static const int dptMASK=16;    // Mask
    static const int dptSENS=17;    // Sens
    static const int dptEB=18;       // EB-file
    static const int dptECTR=19;     // EC-TR
    static const int dptMASKTR=20;    //Mask Tr
    
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++ 2017 new: Numbers of lines in info-table (it)
    //+++ to simplify in case reordering
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    static const int itSample   = 0;       // Sample name
    static const int itPolarization  = 1;    // Polarization
    
    static const int itRuns     = 2;       // Runs
    static const int itC        = 3;       // C .. collimation
    static const int itD        = 4;       // D .. SDD
    static const int itLambda   = 5;       // Lambda
    static const int itBeam     = 6;       // Beam
    static const int itSum      = 7;       // Sum
    static const int itDuration = 8;       // Duration
    static const int itCps      = 9;       // cps
    static const int itDate     = 10;       // Date
    static const int itTime     = 11;      // Time
    static const int itField1   = 12;      // Field1
    static const int itField2   = 13;      // Field2
    static const int itField3   = 14;      // Field3
    static const int itField4   = 15;      // Field4
    
    static const int itBeamwindowX = 16;   // Beamwindow-X
    static const int itBeamwindowY = 17;   // Beamwindow-Y
    static const int itOffset      = 18;   // Offset
    static const int itXposition   = 19;   // X-Position
    static const int itYposition   = 20;   // Y-Position
    static const int itSampleNr    = 21;   // Sample-Nr
    static const int itMotor1      = 22;   // Motor-1
    static const int itMotor2      = 23;   // Motor-2
    static const int itThickness   = 24;   // Thickness
    static const int itBeamWinXs   = 25;   // BeamWin-Xs
    static const int itBeamWinYs   = 26;   // BeamWin-Ys
    static const int itBeamWinXpos = 27;   // BeamWin-X-Pos
    static const int itBeamWinYpos = 28;   // BeamWin-Y-Pos
    
    static const int itTimeFactor   = 29;    // Time-Factor
    static const int itComment1     = 30;    // Comment1
    static const int itComment2     = 31;    // Comment2
    static const int itName         = 32;    // Name
    static const int itWho          = 33;    // Who
    static const int itSelector     = 34;    // Selector
    static const int itMonitor1     = 35;    // Monitor-1
    static const int itMonitor2     = 36;    // Monitor-2
    static const int itMonitor3     = 37;    // Monitor-3
    
    static const int itFiles        = 38;    // Files
    static const int itRTnumber     = 39;    // RT-number
    static const int itRTtimefactor = 40;    // RT-Time-Factor
    static const int itRTrepetitions= 41;    // RT-Repetitions
    static const int itRTframeduration= 42;    // RT-Frame-Duration
    
    static const int itSumvsmask    = 43;    // Sum-vs-Mask
    static const int itSumvsmaskdeadtimecorrected    = 44;    // Sum-vs-Mask-Dead-Time-Corrected
    static const int itQ2vsmask     = 45;    // Q2-vs-Mask
    
    static const int itAttenuator    = 46;    // Attenuator
    static const int itLenses        = 47;    // Lenses
    
    static const int itSlicesCount          = 48;    // Slices-Count
    static const int itSlicesDuration       = 49;    // Slices-Duration
    static const int itSlicesCurrentNumber  = 50;    // Slices-Current-Number
    static const int itSlicesCurrentDuration= 51;    // Slices-Current-Duration
    static const int itSlicesCurrentMonitor1= 52;    // Slices-Current-Monitor1
    static const int itSlicesCurrentMonitor2= 53;    // Slices-Current-Monitor2
    static const int itSlicesCurrentMonitor3= 54;    // Slices-Current-Monitor3
    static const int itSlicesCurrentSum     = 55;    // Slices-Current-Sum
    
    void mergeMethod(bool asciiYN=false);
    void mergeMethodTOF(bool asciiYN=false);
    void mergeTOF(QString tableName, QStringList list, int nTable, int numberPoints, bool asciiYN);
    
    
    public slots:
    
    //init
    void selectMode();
    void selectModeTable();
    void kws1ORkws2();
    void buttomDATpath();
    void updateComboBoxActiveFolders();
    void buttomRADpath();
    void setPattern();
    void dataFormatSelected(int format);
    void instrumentSelected();
    void deleteCurrentInstrument();
    void saveInstrumentAs();
    void selectInstrumentColor();
    
    
    // options
    void sasPresentation();
    void deleteCurrentCalibrant();
    void saveCalibrantAs();
    void calibrantselected();
    void MDchanged();
    void binningChanged(const QString& newValue);
    void dataRangeOfInteresChanged(int newValue);
    void dataDimensionChanged(const QString& newDimension);
    void secondHeaderExist( bool exist );
    void readLambdaFromHeader( bool YN );
    
    
    // rawdata
    void newInfoTable();
    void addToInfoTable();
    void slotMakeBigMatrix();
    void slotMakeBigMatrix(QStringList selectedDat);
    void slotMakeBigMatrixFromTable();
    void newInfoMatrix();
    void check();
    void checkInList();
    void updateComboBoxActiveFile();
    void selectFileToHeader();
    void extractRawData();
    void asciiOrRawdata();
    
    // addfiles
    void addSeveralFilesUniSingleFrame();
    void readTableToAddCols();
    void generateTableToAdd();
    void tabSelected();
    
    // extractor
    void newInfoExtractor();
    void addToInfoExtractor();
    void addColToInfoExtractor();
    
    // mask
    void saveMaskAs();
    void createMask();
    void createMaskTr();
    void saveMaskFul();
    void loadMaskFul();
    void readCoord1();
    void readCoord2();
    void readCoord3();
    void readCoord4();
    void showDANP();
    void maskPlusMaskBS();
    void readCoordDRows();
    void readCoordDCols();
    void readCoordTriangle();
    void readMaskParaFromMaskMatrix( const QString &name );
    
    //  mask Tools
    void readCoorMaskTools();
    void matrixList( QString selectedName = "");
    void maskSetValue();
    void updateMatixActive();
    void maskSquared();
    void maskSector();
    void maskElips();
    void maskElipsShell();
    // sens
    void createSens();
    void saveSensAs();
    void saveSensFul();
    void loadSensFul();
    void selectAnyPlexy();
    void selectAnyEB();
    void selectAnyBC();
    void calculateAnyTr();
    void calculateTrHidden();
    void checkErrorLimits();
    void readFileNumbersFromSensitivityMatrix( const QString &name );
    void SensitivityLineEditCheck();
    
    // process data
    void newScriptTable();
    void newScriptTable(QString tableName);
    void makeScriptTable();
    void makeScriptTable(QStringList selectedDat);
    void saveSettingsSlot();
    void SetColNumberNew( int cols );
    void activeScriptTableSelected(int newLine);
    void addCopyOfLastConfiguration();
    void deleteFirstConfiguration();
    void addMaskAndSens(int condNumber);
    void addMaskAndSens(int condNumber, int oldNumber);
    void vertHeaderTableECPressed(int raw);
    void horHeaderTableECPressed(int col) ;
    void tableECclick(  int row, int col);
    void tableEChelp(int raw);
    void calculateTransmissionAll();
    void tableECcorner();//2020
    void dataProcessingOptionSelected();//2020
    
    // dandan
    void slotDANbuttonPolar();
    void slotDANbuttonIQ();
    void slotDANbuttonIxy();
    void slotDANbuttonDIxy();
    void slotDANbuttonIQx();
    void slotDANbuttonIQy();
    void slotDANbuttonSigma();
    void slotDANbuttonQxy();
    void slotDANbuttondQxy();
    // center
    void calculateCentersInScript();
    //AF
    void calculateAbsFactorInScript();
    //Tr vs DB
    void calculateTrMaskDB();
    
    // rt slots
    void rtSumRead();
    void rtAllselection();
    void rtMergeProgressive();
    void rtMergeLinear();
    void rtSplit();
    void rtAll();
    void geometricalRatioPrefix(int value);
    void geometricalRatioPrefixMeasured(int value);
    
    // tof slots
    void tofAddFiles();
    void tofSumRead();
    void tofShift();
    void tofCollapse();
    void tofRemove();
    void tofMerge();
    void tofSplit();
    void tofCheckShift();
    void tofCheck();
    void tofAll();
    
    // merge slots
    void mergingTableChange();
    void mergeProject() { return mergeMethod(false);};
    void mergeAscii() { return mergeMethod(true);};
    void saveMergeInfo();
    void readMergeInfo();
    
    void mergeProjectTOF() { return mergeMethodTOF(false);};
    void mergeAsciiTOF() { return mergeMethodTOF(true);};
    
    void mergeFromIntegratedTable (bool);
private:

};

#endif