/***************************************************************************
File                   : fittable18.h
Project                : QtiSAS
--------------------------------------------------------------------
Copyright              : (C) 2012-2021 by Vitaliy Pipich
Email (use @ for *)    : v.pipich*gmail.com
Description            : table(s) fitting interface: header file

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
#ifndef FITTABLE18_H
#define FITTABLE18_H

#include "ApplicationWindow.h"

#include "ui_fittable.h"

#include "../common/combobox-in-table.h"
#include "../compile/compile18.h"

#include <Note.h>
#include "PlotCurve.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_spline.h>

#include <qdatetime.h>
#include <qprocess.h>
#include <qaction.h>
#include <QMdiArea>
#include <qregexp.h>
#include <qinputdialog.h>

#include <qprogressdialog.h>
#include <qprogressbar.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qlibrary.h>
#include <qfiledialog.h>
#include <qtextstream.h>
#include <qapplication.h>
#include <qprocess.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qtabwidget.h>
#include <qtoolbox.h>
#include <ColorButton.h>
#include <problem.h>
#include <genmin.h>
#include <get_options.h>
#include <qfont.h>
#include <QDebug>
#include <QStringListModel>

#include "fitting.h"

#ifdef JNSE
#include "../jnse/jnse18.h"
#endif

struct simplyFitP;

#include "../common/msleep.h"
/*
 #include <QThread>

class SleepThread : public QThread {
public:
    static inline void msleep(unsigned long msecs) {
        QThread::msleep(msecs);
    }
};
*/
class fittable18 : public QWidget, public Ui::fittable
{
    Q_OBJECT
public:
    fittable18(QWidget *parent = 0);
    ~fittable18();

    QString libPath;
    QStringList undoRedo;
    size_t undoRedoActive;
    //+++ 2020:QListView integration
    
// interface to parent widget
    ApplicationWindow* app();
    void toResLog( QString text );
    void setPathExtern(QString path);
    void removeTables( QString pattern );
    bool findActiveGraph( Graph * & g );
    bool findActivePlot( MultiLayer* & plot);
    bool AddCurve( Graph * g, QString curveName );
    bool checkTableExistence( QString tableName, Table * & w );
    bool checkTableExistence( QString tableName );
    void findTableListByLabel( QString winLabel, QStringList & list );
    bool callFromTerminal( QString commandLine );
    
// init
    void changeFixedSizeH(QWidget *obj, int H);
    void changeFixedSizeHW(QWidget *obj, int H, int W);
    
    void setFontForce(QFont font);
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);

    void chekLimitsAndFittedParameters();
    QColor getColor(int pos);
    bool selectActiveCurve( int m );
    bool selectData( int m, QString name );
    bool selectRange( int m, QString QN, double min, double max );
    bool findActiveCurve( QString & name, bool & selectedRange, double & min, double & max );
    
// data
    bool findFitDataTable( QString curveName, Table * & table, int & xColIndex, int & yColIndex );
    
// fitting-fitting
    bool simplyFit();
    bool sansFit();
    bool chi2m( int m, int & Nres, double & sumChi2, double & sumChi2Norm, double & R2 );
    bool chi2();
    QString covarMatrix( int N, int P, double chi, double chiNormalization, QStringList paraActive, gsl_matrix * covar, gsl_vector * paraAdjust );
    void makeNote( QString info, QString name, QString label );
    bool readDataForFitAllM( int & Ntotal, int * & numberM, int * & numberSigma, double * & QQ, double * & II, double * & dII, double * & sigmaReso, double * & WEIGHT, double * & sigmaF);
    bool makeTableFromMatrix(char *name, char **tableColNames, int * tableColDestinations, gsl_matrix * m );   
// simulate-simulate
    bool generateSimulatedTable( bool createTable, int source, int m, bool progressShow, QString & simulatedTable, Table * & ttt, int & np, double & chi2, double & TSS );
    bool addGeneralCurve( Graph * g, QString tableName, int m, Table * & table, bool rightYN=false );
    bool SetQandIuniform( int & N, double * & QQ, double * & sigmaQ, int m );
    bool SetQandIgivenM( int &Ntotal, double * &Qtotal, double * &Itotal, double * &dItotal, double * &Sigmatotal, double * &Weighttotal, double * &Sigmaftotal, int m);
    bool simulateData( int & N, double * Q, double * & I, double * & dI, double * sigmaReso, double * sigmaf, bool progressShow );
    bool simulateDataTable(int source, int number, QString &simulatedTable, int N, double *Q, double *Idata, double *dI, double * sigma, double * Isim, Table * & t );
    bool checkCell( QString & line );
    int simulateNoSANS( int N, double * Q, double * & I, gsl_function FF, bool progressShow );
    int simulateSANSreso( int N, double * Q, double * sigma, double * & I, resoSANS & paraReso, bool progressShow );
    int simulateSANSpoly( int N, double * Q, double * & I, poly2_SANS poly2, bool progressShow );
    int simulateSANSpolyReso( int N, double *Q, double *sigma, double * &I, polyReso1_SANS &polyReso1, bool progressShow);

    QStringList groupFunctions( const QString &groupName,  bool onlyEFIT);
    QStringList scanGroup(QStringList lstFIF, bool checkEfit );
    QStringList scanGroupEfit();
    
    QString formatString(double num, QString prefix, int maxField, int prec, QString suffix);
    
public slots:

    void multiNSEfit(QString tableName, QString fitFunction, int from,int to);
// fittable-init
    void initFITTABLE();
    void connectSlot();
    void initColorBox();
    void weightChanged();
    void algorithmSelected();
    void levenbergSelected();
    void SANSsupportYN();
    void SDchanged( int poly );
    void speedControlReso();
    void speedControlPoly();
    void updateDatasets();
    void initMultiTable();
    void initLimits();
    void chekLimits();
    void initFitPage();
    void initMultiParaTable();
    void lineValidator();
    void headerPressedTablePara( int col );
    void vertHeaderPressedTablePara( int raw );
    void headerTableMultiFit( int col );
    void selectRowsTableMultiFit();
    void rangeFirstCheck();
    void rangeLastCheck();

// fittable-exploer
    void scanGroup();
    void groupFunctions(const QModelIndex &c, const QModelIndex &p);//+++2020-06
    void openDLL();
    void openDLL( const QString & file );
    void openDLL(const QModelIndex &c, const QModelIndex &p);//+++2020-06
    void openDLLgeneral( QString file );
    
    void readFIFheader( QString fifName );
    void slotStackFitPrev();
    bool slotStackFitNext();
    void updateEFunctions();
    void saveFittingSession();
    void saveFittingSession( QString table );
    bool saveFittingSessionSimulation( int m, QString table );
    void readSettingsTable();
    
    bool iFitAdv();
    bool iFit();
    bool iFit( bool modeAdv );
    
// fittable-data
    void tableCurvechanged( int raw, int col );
    void colList( QString tableName, int col );
    void QvsN();
    bool SetQandI( int & Ntotal, double * & Qtotal, double * & Itotal, double * & dItotal, double * & Sigmatotal );
    bool datasetChangedSim( int num );
    bool findFitDataTableDirect( QString curveName, Table * & table, int & xColIndex, int & yColIndex );
    void setSigmaAndWeightCols( QStringList lst, QStringList lstDI, QStringList lstSigma, int col, int Raws );
    void vertHeaderTableCurves( int raw );
    void horizHeaderCurves( int col );
    void changedSetToSet( int raw, int col );
    void checkGlobalParameters( int raw, int col );
    void optionSelected();
    
// fittable-fitting
    void saveUndo();
    void undo();
    void redo();
    
    void changeFunctionLocal( const QString & newFunction );
    
    void plotSwitcher();
    void fitSwitcher();
    void initParametersBeforeFit();
    void initParametersAfterFit();
    void fitOrCalculate( bool calculateYN );
    void fitOrCalculate( bool calculateYN, int mmm );
    
// fittable-simulate
    void uniformSimulChanged( bool status );
    void theSameSimulChanged( bool status );
    void logStepChanged( bool status );
    void selectPattern();
    void selectMultyFromTable();
    
    void resToLogWindow();
    void resToLogWindowOne();
    void newTabRes();
    void newTabResCol();
    void addFitResultToActiveGraph();
    void addFitTableScreenshotToActiveGraph();
    void addDataScreenshotToActiveGraph();
 
    void removeSimulatedDatasets();
    void removeFitCurve();
    void removeSimulatedCurve();
    void removeGlobal();
    
    void dataLimitsSimulation( int value );

    void simulateSwitcher();
    void checkConstrains( int m );
    
    void simulateSuperpositional();
    
    void setBySetFit();
    void setBySetFitOrSim( bool fitYN );
    
    void simulateMultifitTables();
    void autoSimulateCurveInSimulations(int,int);

protected:
    int setToSetNumber;
    QString libName;
    QStringList F_initValuesF;
    QStringList SANS_initValues;
    QStringList SANS_param_names;
    QString XQ;
    bool setToSetProgressControl;
    
private:
    QStringList SANS_adjustPara;
    QStringList F_paraListF;
    QLibrary *lib;
    size_t pSANS;
    QStringList F_paraListComments;
    QStringList F_adjustPara;
    QStringList F_paraList;
    gsl_function F;
    QStringList d_param_names;
    QStringList F_initValues;
    gsl_vector *F_para;
    gsl_vector *F_para_limit_left;
    gsl_vector *F_para_limit_right;
    gsl_vector_int *F_para_fit_yn;
    size_t pF;
    QStringList F_adjustParaF;
    QStringList F_paraListCommentsF;
    QStringList SANS_paraListComments;
    bool setToSetSimulYN;

    bool eFitWeight;

};

#endif

