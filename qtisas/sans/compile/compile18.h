/***************************************************************************
 File                   : compile18.h
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2017 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : compile interface: header file
 
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

#ifndef COMPILE18_H
#define COMPILE18_H

#include "ApplicationWindow.h"
#include "ui_compile.h"

#include <qaction.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qdir.h>
#include <qdebug.h>
#include <qfontdatabase.h>
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qmenubar.h>
#include <qradiobutton.h>
#include <qprocess.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qsettings.h>
#include <qsyntaxhighlighter.h>
#include <qslider.h>
#include <qsplitter.h>
#include <qtextedit.h>
#include <qtooltip.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qtextstream.h>
#include <qwaitcondition.h>

#include <LineNumberDisplay.h>
#include <QStringListModel>

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
    QString pathGSL;
    QString pathFIF;
    QString pathMinGW;

// init
    void changeFixedSizeH(QWidget *obj, int H);
    void changeFixedSizeHW(QWidget *obj, int H, int W);
    void setFontForce(QFont font);
    void initScreenResolusionDependentParameters(int hResolusion, double sasResoScale);    
    bool find(QTextEdit *qte, QLineEdit *qle);

public slots:

// compile-init
    void initCompile();
    void connectSlot();
    void defaultOptions();
    void showMenu();
    void expandExpl( bool YN );
    void gslLocal( bool YN );
    void compilerLocal( bool YN );
    void gslStatic( bool YN );
    void pathUpdate();
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
    void gslPath();
    void mingwPath();
    void openFortranFilePath();
    void extructFortranFunctions( QString fileName );
// compile-settings
    void readSettings();
    void saveSettings();
// compile-explorer
    void newFunctionName();
    void newCategoryName();
    void stot1Dto2D();
    void newFIF();
    void makeFIF();
    void deleteFIF();
    void compileSingleFunction();
    void pathChanged();
    void scanGroups();
    void scanIncludedFunctions();
    QStringList groupFunctions( const QString &groupName);
    void groupFunctions(const QModelIndex &c, const QModelIndex &p);
    void openFIFfileSimple();
    void openFIFfileSimple(const QModelIndex &current, const QModelIndex &prev);
    void openFIFfile();
    void openFIFfile( const QString & fifName );
    void updateFiles2();
    void saveasFIF();
    bool save( QString fn, bool askYN );
    void makeCPP();
    void saveAsCPP( QString fn );
    void saveAsCPP1d( QString fn );
    void saveAsCPP2d( QString fn );
    void makeBATnew();
    void openOrigin( QString fdfName );
    void parseOrigin( QStringList lst );
    void makeDLL();
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
    bool saveAsIncluded( QString fn );
    void deleteIncluded();
    void readFromStdout();
    void compileAll();
    void openFortranFileInNote();
    void updateFiles();
    void compileTest();
    void saveTest();
    void openInNoteCPP();
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
    void qtikwsMenu();
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
    bool boolCompileAll;
};

#endif
