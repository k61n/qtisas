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
#include "LineNumberDisplay.h"

class compile18 : public QWidget, private Ui::compile
{
    Q_OBJECT
public:
    compile18(QWidget *parent = 0);
    ~compile18();
    
    LineNumberDisplay *lnTextEditCode;
    LineNumberDisplay *lnTextEditFunctions;
    LineNumberDisplay *lnTextEditHFiles;
    
// interface to parent widget
    ApplicationWindow* app();
    void toResLog( QString text );
    void setPathExtern(QString path);
    
    bool boolYN; 
    QString pathFIF;
    QString batFileMSVC;

// init
    static void changeFixedSizeH(QWidget *obj, int H);
    static void changeFixedSizeHW(QWidget *obj, int H, int W);
    void setFontForce(const QFont &font) const;
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);    
    bool find(QTextEdit *qte, QLineEdit *qle);

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
    void setPath();
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
    void downloadFIF();
    void makeFIF();
    void deleteFIF();
    void pathChanged();
    void scanGroups();
    void scanIncludedFunctions();
    QStringList groupFunctions( const QString &groupName);
    void groupFunctions(const QModelIndex &c, const QModelIndex &p);
    void openFIFfileSimple();
    void openFIFfileSimple(const QModelIndex &current, const QModelIndex &prev);
    void openFIFfile( const QString & fifName );
    bool save( QString fn, bool askYN );
    void makeCPP();
    void saveAsCPP( QString fn );
    void saveAsCPP1d( QString fn );
    void saveAsCPP2d( QString fn );
    void makeCompileScript();
    void buildSharedLibrary(bool compileAll = false);
    void checkFunctionName();
// compile-parameters
    void changedFXYinfo();
    void setNumberparameters( int paraNumber );
    void changedNumberIndepvar( int newNumber );
    void moveParaLine( int line );
    void selectRowsTableMultiFit();
    void addHeaderFile();
    void addIncludedFunction( const QString & fn  );
    void addIncludedFunction( const QModelIndex & );
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
// compile-info
    void textFamily( const QString & f );
    void textSize( const QString & p );
    void textBold();    
    void textUnderline();
    void textItalic();
    void textLeft();
    void textRight();
    void textCenter();
    void textJust();
    void textEXP();
    void textIndex();
    void textLINK();
    void textGreek();
    void readTextFormatting();
    void deleteFormat();
// Menu Filling Functions
    void stdMenu();
    void flagsMenu();
    void dataMenu();
    void mathMenu();
    void sansMenu();
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
    QProcess *procc;
};

#endif
