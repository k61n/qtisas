/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compile interface
 ******************************************************************************/

#ifndef COMPILE18_H
#define COMPILE18_H

#include <QStringListModel>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QWaitCondition>

#include <ui_compile.h>

#include "ApplicationWindow.h"
#include "editable-link-browser.h"
#include "LineNumberDisplay.h"

class compile18 : public QWidget, private Ui::compile
{
    Q_OBJECT
public:
    compile18(QWidget *parent = 0);
    ~compile18();

    LineNumberDisplay *lnTextEditCode{};
    LineNumberDisplay *lnTextEditFunctions{};
    LineNumberDisplay *lnTextEditHFiles{};
    
// interface to parent widget
    ApplicationWindow* app();
    void toResLog( QString text );

    // Numbers in the Compile interface (parameter values and ranges, x/y range) follow the application locale;
    // .fif files, generated code and included-function headers stay C
    QLocale uiLocale() const;
    double uiToDouble(const QString &text, bool *ok = nullptr) const;
    QString uiNumber(double value, char format = 'g', int precision = 6) const;
    QString uiToC(const QString &text) const;
    QString cToUi(const QString &text) const;
    QString rangeUiToC(const QString &text) const;
    QString rangeCToUi(const QString &text) const;
    // The fit-algorithm parameters ("SD=11 REL=1e-12 DER=0.001"): only the decimal point differs
    QString algorithmUiToC(const QString &text) const;
    QString algorithmCToUi(const QString &text) const;
    void setPathExtern(QString path);
    
    bool boolYN{};
    QString pathFIF;
    QString batFileMSVC;
    QString lastCompileErrors;

    EditableLinkBrowser *textEditDescription{};

// init
    static void changeFixedSizeH(QWidget *obj, int H);
    static void changeFixedSizeHW(QWidget *obj, int H, int W);
    void setFontForce(const QFont &font) const;
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);    
    static bool find(QTextEdit *qte, QLineEdit *qle);
    QStringList groupFunctions(const QString &groupName);
    QString functionName() const;
    void setFunctionName(const QString &name);
    QString yName() const;
    void setYName(const QString &name);
    QString xName() const;
    void setXName(const QString &name);
    int parameterCount() const;
    QString code() const;
    void setCode(const QString &code);
    QString info() const;
    void setInfo(const QString &text) const;
    QString hHeaders() const;
    void setHHeaders(const QString &text);
    QString includedFunctions() const;
    void setIncludedFunctions(const QString &text);
    QStringList paramNames() const;
    QString paramName(int i) const;
    void setParamName(int i, const QString &name);
    void setParamNamesAll(const QStringList &names);
    double paramInit(int i) const;
    void setParamInit(int i, double value);
    void setParamInitsAll(const QVector<double> &values);
    QString paramLimit(int i) const;
    void setParamLimit(int i, const QString &limit);
    void setParamLimitsAll(const QStringList &limits);
    bool paramVaries(int i) const;
    void setParamVaries(int i, bool varies);
    void setParamVariesAll(const QVector<bool> &varies);
    QString paramDescr(int i) const;
    void setParamDescr(int i, const QString &descr);
    void setParamDescrsAll(const QStringList &descrs);
    QString group() const;
    void setGroup(const QString &name);
    QStringList groups() const;
    // fit.control: x-range
    void setXRange(bool uniform, double xMin, double xMax, int nPoints, bool logStep, double yMin = 0.0);
    bool customXrange() const;
    void setCustomXrange(bool on);
    double xMin() const;
    void setXMin(double v);
    double xMax() const;
    void setXMax(double v);
    int xPoints() const;
    void setXPoints(int n);
    double yMin() const;
    void setYMin(double v);
    bool logStep() const;
    void setLogStep(bool on);
    // fit.control: fit options
    bool eFitEnabled() const;
    void setEFitEnabled(bool on);
    QString eFit() const;
    void setEFit(const QString &text);
    bool weight() const;
    void setWeight(bool on);
    int weightMethod() const;
    void setWeightMethod(int idx);
    bool superpositional() const;
    void setSuperpositional(bool on);
    int subFitCount() const;
    void setSubFitCount(int n);
    bool useAlgorithm() const;
    void setUseAlgorithm(bool on);
    int fitMethod() const;
    void setFitMethod(int idx);
    QString fitMethodPara() const;
    void setFitMethodPara(const QString &text);
    QString category() const;
    QStringList categories() const;
    bool selectCategory(const QString &name);
    QStringList functionNames(QString categoryName);
    bool openFunction(const QString &name);
    void categoryChanged(const QString &category = QString());

public slots:

// compile-init
    void initCompile();
    void connectSlot();
    void defaultOptions();
    void showMenu();
    void expandExpl( bool YN );
    void compilerLocal();
    //
    void expandParaTrue();
    void expandParaFalse();
    void expandPara( bool YN );
    void expandCodeTrue();
    void expandCodeFalse();
    void expandCode( bool YN );
    void expandExplTrue();
    void expandExplFalse();
    //
    void setPath(const QString &path = QString());
    void selectionBatFileMSVC();
    void openFortranFilePath();
    void extructFortranFunctions(const QString &fileName);
// compile-settings
    void readSettings();
    void saveSettings();
// compile-explorer
    void newFunctionName();
    void newCategoryName();
    void stot1Dto2D();
    void newFIF();
    void downloadFIF(const QString &repoUrl = "https://iffgit.fz-juelich.de/qtisas/fitfunctions.git",
                     bool silent = false);
    QString makeFIF();
    void deleteFIF();
    void pathChanged();
    void scanGroups();
    void scanIncludedFunctions();
    void groupChanged(const QModelIndex &c, const QModelIndex &p);
    void openFIFfileSimple();
    void openFIFfileSimple(const QModelIndex &current, const QModelIndex &prev);
    void openFIFfile(QString fifName);
    QString save(const QString &fn, bool askYN);
    QString saveFIF(const QString &name = QString(), const QString &customPath = QString(), bool askYN = false);
    void deleteFIF(const QString &name, const QString &customPath = QString());
    QString makeCPP();
    QString saveAsCPP(const QString &fn);
    QString saveAsCPP1d(const QString &fn);
    static QString saveAsCPP2d(const QString &fn);
    QString makeCompileScript();
    bool buildSharedLibrary(bool compileAll = false);
    void checkFunctionName();
// compile-parameters
    void changedFXYinfo();
    void setNumberparameters( int paraNumber );
    void changedNumberIndepvar( int newNumber );
    void moveParaLine( int line );
    void selectRowsTableMultiFit();
    void addHeaderFile();
    void addIncludedFunction(const QString &fn);
    void addIncludedFunction(const QModelIndex &);
    void openHeaderInNote();
    void openInNote( QString fn );
    void makeIncluded();
    bool saveAsIncluded(const QString &fn);
    void deleteIncluded();
    void readFromStdout();
    void compileAll();
    void openFortranFileInNote();
    void updateFiles();
    void compileTest();
    void saveTest();
    void openInNoteCPP();
    void openSasViewPy();
    void readTextFormatting();
// Menu Filling Functions
    void stdMenu();
    void flagsMenu();
    void dataMenu();
    void mathMenu();
    void multiMenu();
    void sasviewMenu();
    void qtisasMenu();
    void fortranMenu();
// Menu Action Slots
    void mathAction(QAction*);
    void multimenuAction(QAction*);
    void includeAction(QAction*);
    void bgdMenuSelected(QAction*);
    void functionMenuSelected(QAction*);
    void fortranMenuSelected(QAction*);
// Find
    void findInCode();
    void findInIncluded();

protected:
    QProcess *procc{};
    // Re-formats the numbers shown in the widgets when the numerical format changes
    void changeEvent(QEvent *event) override;
    QLocale d_widgetLocale;
};

#endif
