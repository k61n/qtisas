/***************************************************************************
 File                   : compile-init.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2017-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : compile interface: init functions
 
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
#include "compile18.h"
#include <QSizePolicy>
#include "highlighter.h"



void compile18::connectSlot()
{
//
    connect( fitPath, SIGNAL( textChanged(const QString&) ), this, SLOT( pathChanged() ) );
    
    connect(pushButtonMenu, SIGNAL(clicked()), this, SLOT(showMenu()));
    connect( pushButtonDefaultOptions, SIGNAL( clicked() ), this, SLOT( defaultOptions() ) );
    connect( tabWidgetCode, SIGNAL( selected(const QString&) ), this, SLOT( pathUpdate() ) );
    
    //+++
    connect( pushButtonParaDown, SIGNAL( clicked() ), this, SLOT( expandParaTrue() ) );
    connect( pushButtonParaUp, SIGNAL( clicked() ), this, SLOT( expandParaFalse() ) );
    
    //+++
    connect( pushButtonCodeDown, SIGNAL( clicked() ), this, SLOT( expandCodeTrue() ) );
    connect( pushButtonCodeUp, SIGNAL( clicked() ), this, SLOT( expandCodeFalse() ) );
    
    //+++
    connect( pushButtonExplDown, SIGNAL( clicked() ), this, SLOT( expandExplTrue() ) );
    connect( pushButtonExplUp, SIGNAL( clicked() ), this, SLOT( expandExplFalse() ) );

// compile-explorer
    connect( lineEditFunctionName, SIGNAL( textChanged(const QString&) ), this, SLOT( newFunctionName() ) );
    connect( lineEditGroupName, SIGNAL( textChanged(const QString&) ), this, SLOT( newCategoryName() ) );
    
    connect( textLabelGroupName, SIGNAL( clicked() ), this, SLOT( newFIF() ) );
    
    connect( radioButton1D, SIGNAL( clicked() ), this, SLOT( stot1Dto2D() ) );
    connect( radioButton2D, SIGNAL( clicked() ), this, SLOT( stot1Dto2D() ) );
    connect( pushButtonNew, SIGNAL( clicked() ), this, SLOT( newFIF() ) );
    connect( pushButtonSave, SIGNAL( clicked() ), this, SLOT( makeFIF() ) );
    connect( pushButtonDelete, SIGNAL( clicked() ), this, SLOT( deleteFIF() ) );
    connect( pushButtonMakeDLL, SIGNAL( clicked() ), this, SLOT( compileSingleFunction() ) );
    
    connect( pushButtonOpenFIF, SIGNAL( clicked() ), this, SLOT( openFIFfile() ) );
    connect( pushButtonSaveAs, SIGNAL( clicked() ), this, SLOT( saveasFIF() ) );
    connect( pushButtonCompileAll, SIGNAL( clicked() ), this, SLOT( compileAll() ) );
    connect( lineEditFunctionName, SIGNAL( lostFocus() ), this, SLOT( checkFunctionName() ) );
    
// compile-parameters
    connect( spinBoxP, SIGNAL( valueChanged(int) ), this, SLOT( setNumberparameters(int) ) );
    connect( spinBoxXnumber, SIGNAL( valueChanged ( int)), this, SLOT(changedNumberIndepvar(int)));
    connect(tableParaNames->verticalHeader(), SIGNAL(sectionClicked (int)), this, SLOT(moveParaLine(int)));
    connect(tableParaNames, SIGNAL(itemSelectionChanged()), this, SLOT(selectRowsTableMultiFit()));
    
    connect( lineEditY, SIGNAL( textChanged(const QString&) ), this, SLOT( changedFXYinfo() ) );
    connect( lineEditXXX, SIGNAL( textChanged(const QString&) ), this, SLOT( changedFXYinfo() ) );
//
    connect( pushButtonPath, SIGNAL( clicked() ), this, SLOT( setPath() ) );
    connect( pushButtonFortranFunction, SIGNAL( clicked() ), this, SLOT( openFortranFilePath() ) );
    connect( pushButtonPathMingw, SIGNAL( clicked() ), this, SLOT( mingwPath() ) );
    connect( pushButtonPathGSL, SIGNAL( clicked() ), this, SLOT( gslPath() ) );
    
    connect( pushButtonAddHeader, SIGNAL( clicked() ), this, SLOT( addHeaderFile() ) );
    connect( pushButtonOpenInNote, SIGNAL( clicked() ), this, SLOT( openHeaderInNote() ) );
    connect( pushButtonFortranFunctionView, SIGNAL( clicked() ), this, SLOT( openFortranFileInNote() ) );
    connect( pushButtonMakeIncluded, SIGNAL( clicked() ), this, SLOT( makeIncluded() ) );
    connect( pushButtonIncludedDelete, SIGNAL( clicked() ), this, SLOT( deleteIncluded() ) );
    
    connect( checkBoxGSLstatic, SIGNAL( toggled(bool) ),this, SLOT( gslStatic(bool) ) );
    connect( checkBoxGSLlocal, SIGNAL( toggled(bool) ),this, SLOT( gslLocal(bool) ) );
    connect( checkBoxCompilerLocal, SIGNAL( toggled(bool) ),this, SLOT( compilerLocal(bool) ) );

    connect( radioButtonBAT, SIGNAL( clicked() ), this, SLOT( updateFiles() ) );
    connect( radioButtonFIF, SIGNAL( clicked() ), this, SLOT( updateFiles() ) );
    connect( radioButtonCPP, SIGNAL( clicked() ), this, SLOT( updateFiles() ) );
    
    connect( pushButtonTestSave, SIGNAL( clicked() ), this, SLOT( saveTest() ) );
    connect( pushButtonTestCompile, SIGNAL( clicked() ), this, SLOT( compileTest() ) );
    connect(pushButtonInNoteFiles, SIGNAL(clicked()), this, SLOT(openInNoteCPP()));

// compile-info
    connect( toolButtonDeleteFormat, SIGNAL( clicked() ), this, SLOT( deleteFormat() ) );
    connect( pushButtonBold, SIGNAL( clicked() ), this, SLOT( textBold() ) );
    connect(pushButtonEXP, SIGNAL( clicked() ), this, SLOT( textEXP() ) );
    connect( pushButtonJust, SIGNAL( clicked() ), this, SLOT( textJust() ) );
    connect( pushButtonRight, SIGNAL( clicked() ), this, SLOT( textRight() ) );
    connect( pushButtonCenter, SIGNAL( clicked() ), this, SLOT( textCenter() ) );
    connect( pushButtonLeft, SIGNAL( clicked() ), this, SLOT( textLeft() ) );
    connect( pushButtonItal, SIGNAL( clicked() ), this, SLOT( textItalic() ) );
    connect( pushButtonUnder, SIGNAL( clicked() ), this, SLOT( textUnderline() ) );
    
    connect( comboBoxFontSize, SIGNAL( activated(const QString&) ), this, SLOT( textSize(const QString&) ) );
    connect( comboBoxFont, SIGNAL( activated(const QString&) ), this, SLOT( textFamily(const QString&) ) );
    connect( checkBoxAddFortran, SIGNAL( toggled(bool) ), textEditForwardFortran, SLOT( setEnabled(bool) ) );
    connect( pushButtonGreek, SIGNAL( clicked() ), this, SLOT( textGreek() ) );
    connect( pushButtonSub, SIGNAL( clicked() ), this, SLOT( textIndex() ) );
    
    connect( textEditDescription, SIGNAL( cursorPositionChanged() ), this, SLOT( readTextFormatting() ) );


// compile-menu
    connect(pushButtonMenuSTD, SIGNAL(clicked()), this, SLOT(stdMenu()));
    connect(pushButtonMenuFlags, SIGNAL(clicked()), this, SLOT(flagsMenu()));
    connect(pushButtonMenuData, SIGNAL(clicked()), this, SLOT(dataMenu()));
    connect(pushButtonMenuMath, SIGNAL(clicked()), this, SLOT(mathMenu()));
    connect(pushButtonMenuSANS, SIGNAL(clicked()), this, SLOT(sansMenu()));
    
    
    connect(pushButtonMenuMULTI, SIGNAL(clicked()), this, SLOT(multiMenu()));
    
    connect(pushButtonMenuSASVIEW, SIGNAL(clicked()), this, SLOT(sasviewMenu()));
    connect(pushButtonMenuQTIKWS, SIGNAL(clicked()), this, SLOT(qtikwsMenu()));
    connect(pushButtonMenuFORTRAN, SIGNAL(clicked()), this, SLOT(fortranMenu()));
    
    //+++ find
    connect( lineEditFind, SIGNAL( returnPressed() ), this, SLOT( findInCode() ) );
    connect( lineEditFindIncluded, SIGNAL( returnPressed() ), this, SLOT( findInIncluded() ) );
    
}


//*******************************************
//*setFontForce
//*******************************************
void compile18::setFontForce(QFont font)
{
    QFontMetrics fm(font);
   

     listBoxGroupNew->setFont(font);
     listBoxFunctionsNew->setFont(font);
     listBoxIncludeFunctionsNew->setFont(font);
     textEditDescription->setFont(font);
    pushButtonBold->setFont(font);
    pushButtonItal->setFont(font);
    pushButtonUnder->setFont(font);
    
    
    foreach( QWidget *obj, tableParaNames->findChildren< QWidget * >( ) )
    {
        obj->setFont(font);
    }
    
    foreach( QWidget *obj, frameControl->findChildren< QWidget * >( ) )
    {
        obj->setFont(font);
    }
    foreach( QWidget *obj, groupBoxMinGw->findChildren< QWidget * >( ) )
    {
        obj->setFont(font);
    }
    foreach( QWidget *obj, groupBoxOptions->findChildren< QWidget * >( ) )
    {
        obj->setFont(font);
    }
    groupBoxMinGw->setFont(font);
    groupBoxOptions->setFont(font);
    
    
    QFont fonts=font;
    
    int pointSize=fonts.pointSize();
    fonts.setPointSize(pointSize-2);
    frameMenu->setFont(fonts);
    pushButtonMenu->setFont(fonts);
    pushButtonMenuSASVIEW->setFont(fonts);
    pushButtonMenuQTIKWS->setFont(fonts);
    pushButtonMenuFORTRAN->setFont(fonts);
    
    textEditCode->setFont(font);
    textEditFunctions->setFont(font);
    textEditHFiles->setFont(font);

    tableParaNames->setFont(font);
    
    //+++ ln TextEditCode
    lnTextEditCode->setCurrentFont(font);
    lnTextEditCode->updateLineNumbers(true);


    QFontMetrics fm2(lnTextEditCode->currentFont());
    int length=fm2.width("99999")+5;
    lnTextEditCode->setMinimumWidth(length);
    lnTextEditCode->setMaximumWidth(length);
    
    //+++ ln TextEditFunctions
    lnTextEditFunctions->setCurrentFont(font);
    lnTextEditFunctions->updateLineNumbers(true);

    lnTextEditFunctions->setMinimumWidth(length);
    lnTextEditFunctions->setMaximumWidth(length);

    //+++ ln TextEditHFiles
    lnTextEditHFiles->setCurrentFont(font);
    lnTextEditHFiles->updateLineNumbers(true);

    lnTextEditHFiles->setMinimumWidth(length);
    lnTextEditHFiles->setMaximumWidth(length);
	
    //+++ ln TextEditCode 2nd
    lnTextEditCode->setCurrentFont(font);
    lnTextEditCode->updateLineNumbers(true);
    
    lnTextEditCode->setMinimumWidth(length);
    lnTextEditCode->setMaximumWidth(length);
}


void compile18::changeFixedSizeH(QWidget *obj, int H)
{
    obj->setMinimumSize(0,H);
    obj->setMaximumSize(32767,H);
}

void compile18::changeFixedSizeHW(QWidget *obj, int H, int W)
{
    obj->setMinimumSize(W,H);
    obj->setMaximumSize(W,H);
}


//*******************************************
//*initScreenResolusionDependentParameters
//*******************************************
void compile18::initScreenResolusionDependentParameters(int hResolusion, double sasResoScale)
{
    
    int labelHight= int(hResolusion*sasResoScale/50);
    int fontIncr=app()->sasFontIncrement+1;
    
    int newH=labelHight;
    int newW=int(double(labelHight)/1.5);
    
    
    if (labelHight<30) {newH=30; newW=15;};
    
    foreach( QTableWidget *obj, this->findChildren< QTableWidget * >( ) )
    {
        obj->verticalHeader()->setDefaultSectionSize(labelHight+5);
        obj->verticalHeader()->setMinimumWidth(labelHight+5);
    }
    
    //+++ Labels
    foreach( QLabel *obj, this->findChildren< QLabel * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
        
        //obj->setMinimumWidth(3*labelHight);
    }
    
    //+++ QLineEdit
    foreach( QLineEdit *obj, this->findChildren< QLineEdit * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    //+++ QPushButton
    foreach( QPushButton *obj, this->findChildren< QPushButton * >( ) )
    {
        if(obj->baseSize().width()==25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else if(obj->baseSize().width()==14)
        {
            QFont fonts=obj->font();
            /*
            obj->setMaximumWidth(int(0.6*labelHight));
            obj->setMaximumWidth(int(0.6*labelHight));
            obj->setMinimumHeight(int(0.6*labelHight));
            obj->setMaximumHeight(int(0.6*labelHight));
             */
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMinimumHeight(fonts.pointSize()+5);
            obj->setMaximumHeight(fonts.pointSize()+5);
        }
        else if(obj->baseSize().width()==10)
        {
            obj->setMinimumHeight(newH);
            obj->setMaximumHeight(newH);
        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
    }
    //+++ QToolButton
    foreach( QToolButton *obj, this->findChildren< QToolButton * >( ) )
    {
        if(obj->baseSize().width()==25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
        else if(obj->baseSize().width()==14)
        {
            QFont fonts=obj->font();
            /*
             obj->setMaximumWidth(int(0.6*labelHight));
             obj->setMaximumWidth(int(0.6*labelHight));
             obj->setMinimumHeight(int(0.6*labelHight));
             obj->setMaximumHeight(int(0.6*labelHight));
             */
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMaximumWidth(fonts.pointSize()+5);
            obj->setMinimumHeight(fonts.pointSize()+5);
            obj->setMaximumHeight(fonts.pointSize()+5);
        }
        else if(obj->baseSize().width()==10)
        {
            //obj->setMinimumHeight(int(0.8*labelHight));
            //obj->setMaximumHeight(int(0.8*labelHight));

        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            

            if (obj->sizeIncrement().width()>0)
            {
                int baseWidth=obj->sizeIncrement().width();
                if (fontIncr>0)  baseWidth=baseWidth + 2*fontIncr*obj->text().length();
                
                obj->setMinimumWidth(baseWidth);
                obj->setMaximumWidth(baseWidth);
            }
            
                
            obj->setIconSize(QSize(int(0.75*labelHight),int(0.75*labelHight)));
        }
    }
        

    //+++ QCheckBox
    foreach( QCheckBox *obj, this->findChildren< QCheckBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    //+++ QComboBox
    foreach(QComboBox *obj, this->findChildren< QComboBox * >( ) )
    {
        obj->setMinimumHeight(labelHight+5);
        obj->setMaximumHeight(labelHight+5);
    }
    //+++ QSpinBox
    foreach(QSpinBox *obj, this->findChildren< QSpinBox * >( ) )
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }
    
    //+++
    //+++
    changeFixedSizeH(textLabelInfoSAS, labelHight);
    changeFixedSizeH(textLabelInfoVersion, labelHight);
    changeFixedSizeH(textLabelInfoSASauthor, labelHight);
    
    changeFixedSizeH(frameControl, labelHight+24);
    

    changeFixedSizeHW(spinBoxP, newH+2, newW+2);
    changeFixedSizeHW(spinBoxParaHun, newH, newW);
    changeFixedSizeHW(spinBoxParaDec, newH, newW);
    changeFixedSizeHW(spinBoxPara,  newH, newW);
 
    tableParaNames->horizontalHeader()->setMinimumHeight(labelHight);

    
#ifdef Q_OS_WIN
    spinBoxP->setStyleSheet(QString("QSpinBox::up-arrow {width: %1;} QSpinBox::up-button {width: %2;}QSpinBox::down-arrow {width: %1;} QSpinBox::down-button {width: %2;}").arg(QString::number(newW)).arg(newW+2));
#endif
    

    if (pushButtonMenu->text()==QChar(0x226A)) frameMenu->setMaximumWidth(3000);
    else frameMenu->setMaximumWidth(0);
    
    
    QFont font=pushButtonMenu->font();
    QFontMetrics fm(font);

    
    //+++ frameMenu
    foreach( QToolButton *obj, tabCode->findChildren< QToolButton * >( ) )
    {
        obj->setMinimumWidth(fm.width(obj->text())+10);
        obj->setMaximumWidth(fm.width(obj->text())+10);
        
       obj->setMinimumHeight(fm.height()+8);
       obj->setMaximumHeight(fm.height()+8);
    }
    frameMenu->setMinimumHeight(fm.height()+8);
    frameMenu->setMaximumHeight(fm.height()+8);

}


//*******************************************
//+++ Initiation
//*******************************************
void compile18::initCompile()
{
    //+++
    lnTextEditCode = new LineNumberDisplay(textEditCode, this);  lnTextEditCode->setVisible(true); lnTextEditCode->addZeros=true;
    horizontalLayoutEditCode->insertWidget(0,lnTextEditCode);
    lnTextEditCode->setStyleSheet("background-color: transparent; color: gray; selection-color: white; selection-background-color: rgb(137,137,183);");
    //+++
    lnTextEditFunctions = new LineNumberDisplay(textEditFunctions, this);  lnTextEditFunctions->setVisible(true); lnTextEditFunctions->addZeros=true;
    horizontalLayoutTextEditFunctions->insertWidget(0,lnTextEditFunctions);
    lnTextEditFunctions->setStyleSheet("background-color: transparent; color: gray; selection-color: white; selection-background-color: rgb(137, 137, 183);");//rgb(137, 137, 183);");

    //+++ TextEditHFiles
    lnTextEditHFiles = new LineNumberDisplay(textEditHFiles, this);  lnTextEditHFiles->setVisible(true);lnTextEditHFiles->addZeros=true;
    horizontalLayoutTextEditHFiles->insertWidget(0,lnTextEditHFiles);
    lnTextEditHFiles->setStyleSheet("background-color: transparent; color: gray; selection-color: white; selection-background-color: rgb(137, 137, 183);");//rgb(137, 137, 183);");
    
    spinBoxP->findChild<QLineEdit*>()->setMaximumWidth(0);
    
    //+++ 1D or 2D
    stot1Dto2D();
    
    
    defaultOptions();
    readSettings ();
    
    //+++
    scanGroups();
    scanIncludedFunctions();
    
    //+++
    QFontDatabase db;
    comboBoxFont->addItems(db.families());
    
    QList<int> sizes = db.standardSizes();
    QList<int>::Iterator it = sizes.begin();
    for ( ; it != sizes.end(); ++it )
        comboBoxFontSize->addItem(QString::number(*it));
    boolYN=false;
    
    expandExpl(false);
    
    tableCPP->horizontalHeader()->setResizeMode( 0, QHeaderView::Stretch );
    
    pushButtonParaUp->hide();
    pushButtonExplUp->hide();
    pushButtonCodeUp->hide();
    

    
    QString toolText="Main Code Window";
    QToolTip::add( textEditCode, toolText);
    
    pushButtonMenu->setText(QChar(0x226A));
    showMenu();
    
    //pushButtonMenuGSL->hide();
    
    spinBoxParaHun->hide();
    spinBoxParaDec->hide();
    
    Highlighter *highlighter1 = new Highlighter(textEditCode->document());
    Highlighter *highlighter2 = new Highlighter(textEditFunctions->document());
    Highlighter *highlighter3 = new Highlighter(textEditHFiles->document());
    Highlighter *highlighter4 = new Highlighter(textEditForwardFortran->document());
    
    //this->setMinimumWidth(tabWidgetCode->minimumSizeHint().width()+140);
    //this->setMinimumHeight(this->minimumSizeHint().height()-55);
    
    tabWidgetCode->setCurrentIndex(0);
    

    tableParaNames->horizontalHeader()->setResizeMode( 3, QHeaderView::Fixed );
    tableParaNames->horizontalHeader()->setResizeMode( 0, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setResizeMode( 1, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setResizeMode( 2, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setResizeMode( 3, QHeaderView::Stretch );

    pushButtonMenuSANS->hide();
}

void compile18::defaultOptions()
{
    //+++
    QDir dd;
    QString funPath;
    if (pathFIF=="") funPath=((ApplicationWindow *)this->parent())->sasPath+"/FitFunctions";
    else funPath=app()->sasPath+"/FitFunctions";
    funPath=funPath.replace("//","/");
    if (!dd.cd(funPath))
    {
        funPath = QDir::homePath() + "/FitFunctions";
        funPath=funPath.replace("//","/");
        
        if (!dd.cd(funPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/FitFunctions");
            dd.cd("./qtiSAS/FitFunctions");
        }
    };
    funPath = dd.absolutePath();
    
    pathFIF=funPath;
    fitPath->setText(funPath);
    
    
#ifdef Q_OS_WIN
    
    checkBoxGSLlocal->setChecked(true);
    checkBoxGSLstatic->setChecked(false);
    gslLocal(true);
    
    checkBoxCompilerLocal->setChecked(true);
    compilerLocal(true);
    
    checkBoxGSLstatic->setChecked(true);
    gslStatic(true);
    
#elif defined(Q_OS_MAC)
    
    checkBoxGSLlocal->setChecked(true);
    checkBoxGSLstatic->setChecked(true);
    gslLocal(true);
    
    checkBoxCompilerLocal->setChecked(false);
    compilerLocal(false);
    
    checkBoxGSLstatic->setChecked(true);
    gslStatic(true);
#else
    
    checkBoxGSLlocal->setChecked(true); //+++ 2019-10 checkBoxGSLlocal->setChecked(false);
    checkBoxGSLstatic->setChecked(true);//+++ 2019-10 checkBoxGSLstatic->setChecked(false);
    gslLocal(true); //+++ 2019-10 gslLocal(false);
    
    checkBoxCompilerLocal->setChecked(false);
    compilerLocal(false);
    
    checkBoxGSLstatic->setChecked(true);
    gslStatic(true);
#endif
    //+++
    
    
#if defined(Q_OS_WIN)
    //+++  gsl WIN
    dd.cd(qApp->applicationDirPath());
    
    pathGSL="Select GSL Directory!!!";
    
    if (dd.cd("../3rdparty/GSL"))
        pathGSL = dd.absolutePath();
    else
        if (dd.cd(funPath.remove("/FitFunctions")+"/3rdparty/GSL"))
            pathGSL = dd.absolutePath();
    
    gslPathline->setText(pathGSL);
    
#elif defined(Q_OS_MAC)
    
    //+++  gsl WIN
    dd.cd(qApp->applicationDirPath());
    if (!dd.cd("../3rdparty/GSL"))
    {
        if (dd.cd("/Applications/qtisas.app/Contents/3rdparty/GSL"))
        {
            pathGSL = dd.absolutePath();
        }
        else if (dd.cd(funPath.remove("/FitFunctions")+"/3rdparty/GSL"))
        {
            pathGSL = dd.absolutePath();
        }
        else if (dd.cd(QDir::homePath() + "/qtisas/3rdparty/GSL"))
        {
            pathGSL = dd.absolutePath();
        }
        else pathGSL="Select GSL Directory!!!";
    }
    else
    {
        pathGSL = dd.absolutePath();
    }
    gslPathline->setText(pathGSL);
    
#else
    
    //+++  linux x64
    dd.cd(qApp->applicationDirPath());
    if (!dd.cd("../3rdparty/gsl"))
    {
        if (dd.cd(funPath.remove("/FitFunctions")+"/3rdparty/GSL")) {
            pathGSL = dd.absolutePath();
        }
        else
            if (dd.cd(QDir::homePath()+"/qtisas/3rdparty/GSL")) {
                pathGSL=dd.absolutePath();
            }
            else
                pathGSL="Select GSL Directory!!!";
    }
    else
        pathGSL = dd.absolutePath();
    gslPathline->setText(pathGSL);
    
#endif
    
    
#if defined(Q_OS_WIN)
    
    //+++  MinGW WIN
    dd.cd(QDir::rootDirPath ());
    if (!dd.cd("./MinGW"))
    {
        pathMinGW="Select Compiler Directory!!!";
    }
    else
    {
        pathMinGW = dd.absolutePath();
    }
    mingwPathline->setText(pathMinGW);
#endif
    
    QString compileFlag="g++ -m32 -O2 -Wl,--no-as-needed -w";
    
#if defined(Q_OS_MAC)
    compileFlag="g++ -arch x86_64 -O2 -w";
#endif
    
#ifdef __x86_64__
    compileFlag="g++ -fPIC -O2 -w";
#endif
    
    pathChanged();
}

//*******************************************
//+++ showMenu
//*******************************************
void compile18::showMenu()
{
    if (pushButtonMenu->text()==QChar(0x226A))
    {
        pushButtonMenu->setText(QChar(0x226B));
        frameMenu->setMaximumWidth(0);
        //frameMenu->setMinimumHeight(2);
    }
    else
    {
        pushButtonMenu->setText(QChar(0x226A));
        frameMenu->setMaximumWidth(3000);
        //frameMenu->setMinimumHeight(50);
        
    }
    
}
//*******************************************
//+++ showMenu
//*******************************************
void compile18::expandExpl( bool YN )
{
    QList<int> lst;
    
    if (YN)
    {
        lst<<100<<0<<0<<0;
        splitterMain->setSizes(lst);
    }
    else
    {
        if (textLabelInfoVersion->isHidden()) lst<<135<<85<<135<<0;
        else lst<<135<<85<<135<<5;
        splitterMain->setSizes(lst);
    }
    
}


void compile18::gslLocal(bool YN)
{
    
    if (YN)
    {
        textLabelGSL->show();
        gslPathline->show();
        pushButtonPathGSL->show();
        checkBoxGSLstatic->show();
        
        lineEditCompileFlags->setText ("g++ -m32 -O2 -w -I$GSL -c");
        lineEditLinkFlags->setText	  ("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -L$GSL/bin -lgsl -lgslcblas -o");
        
#ifdef __x86_64__
        lineEditCompileFlags->setText  ("g++ -fpic -O2 -w -I$GSL  -c");
        lineEditLinkFlags->setText     ("g++ -O2 -Wall -shared -o -L$GSL/bin -lgsl -lgslcblas");  //+++ modified 2019-10-08
#endif
        
#ifdef Q_OS_MAC
        lineEditCompileFlags->setText  ("g++ -arch x86_64 -O2 -w -I$GSL  -c");
        lineEditLinkFlags->setText     ("g++ -arch x86_64 -O2 -Wall -shared -o -L$GSL/bin -lgsl -lgslcblas"); //+++ modified 2019-10-08
#endif
        
#ifdef Q_OS_WIN
        lineEditCompileFlags->setText  ("g++ -m32 -O2 -w -I$GSL -c");
        lineEditLinkFlags->setText     ("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -DGSL_DLL -L$GSL/bin -lgsl -lgslcblas -o");
#endif
    }
    else
    {
        checkBoxGSLstatic->setChecked(false);
        textLabelGSL->hide();
        gslPathline->hide();
        pushButtonPathGSL->hide();
        checkBoxGSLstatic->hide();
        
        lineEditCompileFlags->setText  ("g++ -m32 -O2 -w -c");
        lineEditLinkFlags->setText     ("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -lgsl -lgslcblas -o");
        
#ifdef __x86_64__
        lineEditCompileFlags->setText  ("g++ -fpic -O2 -w -c");
        lineEditLinkFlags->setText        ("g++ -O2 -Wl,--no-as-needed -Wall -shared -lgsl -lgslcblas -o");
#endif
        
        
#ifdef Q_OS_WIN
        lineEditCompileFlags->setText  ("g++ -m32 -O2 -w -c");
        lineEditLinkFlags->setText     ("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -DGSL_DLL -lgsl -lgslcblas -o");
#endif
        
#ifdef Q_OS_MAC
        lineEditCompileFlags->setText  ("g++ -arch x86_64 -O2 -w -c");
        lineEditLinkFlags->setText     ("g++ -arch x86_64 -O2  -Wall -shared -lgsl -lgslcblas -o");
#endif
        
    }
}

void compile18::compilerLocal(bool YN)
{
    if (YN)
    {
        textLabelMingw->show();
        mingwPathline->show();
        pushButtonPathMingw->show();
    }
    else
    {
        textLabelMingw->hide();
        mingwPathline->hide();
        pushButtonPathMingw->hide();
    }
}

void compile18::gslStatic(bool YN)
{
    if (YN)
    {
        lineEditLinkFlags->setText("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -o $GSL/bin/libgsl.a $GSL/bin/libgslcblas.a");

#ifdef __x86_64__
        lineEditLinkFlags->setText("g++ -O2 -Wall -shared -o $GSL/bin/libgsl.a $GSL/bin/libgslcblas.a");
#endif
        
#ifdef Q_OS_MAC
        lineEditLinkFlags->setText("g++ -arch x86_64 -O2 -Wall -shared -o $GSL/bin/libgsl.a $GSL/bin/libgslcblas.a");
#endif
        
    }
    else
    {
        lineEditLinkFlags->setText("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -L$GSL/bin -lgsl -lgslcblas -o");
        
#ifdef __x86_64__
        lineEditLinkFlags->setText("g++ -fpic -O2 -Wall -shared -o -L$GSL/bin -lgsl -lgslcblas");
#endif
        
#ifdef Q_OS_WIN
        lineEditLinkFlags->setText("g++ -m32 -O2 -Wl,--no-as-needed -Wall -shared -DGSL_DLL -L$GSL/bin -lgsl -lgslcblas -o");
#endif
        
#ifdef Q_OS_MAC
        lineEditLinkFlags->setText("g++ -arch x86_64 -O2 -Wall -shared -o  -L$GSL/bin -lgsl -lgslcblas");
#endif
    }
}


void compile18::pathUpdate()
{
    if (lineEditCompileFlags->text()!="-m32 -w") return;
    
    defaultOptions();
    readSettings();
    scanGroups();
    scanIncludedFunctions();
    
    if (lineEditCompileFlags->text()=="-m32 -w")     defaultOptions();
}

void compile18::expandParaTrue()
{
    expandPara(true);
    pushButtonParaUp->show();
    pushButtonParaDown->hide();
    tableParaNames->setFocus();
}


void compile18::expandParaFalse()
{
    expandPara(false);
    pushButtonParaUp->hide();
    pushButtonParaDown->show();
}

void compile18::expandPara( bool YN )
{
    QList<int> lst;
    
    if (YN)
    {
        lst<<0<<350<<0<<5;
        splitterMain->setSizes(lst);
    }
    else
    {
        lst<<135<<85<<135<<0;
        splitterMain->setSizes(lst);
    }
}


void compile18::expandCodeTrue()
{
    expandCode(true);
    pushButtonCodeUp->show();
    pushButtonCodeDown->hide();
    tabWidgetCode->setFocus();
}


void compile18::expandCodeFalse()
{
    expandCode(false);
    pushButtonCodeUp->hide();
    pushButtonCodeDown->show();
}


void compile18::expandCode( bool YN )
{
    QList<int> lst;
    
    if (YN)
    {
        lst<<0<<0<<350<<5;
        
        splitterMain->setSizes(lst);
        lst.clear();
        QList<int> lst22;
        lst22<<40<<320;
        splitterInc->setSizes(lst22);
    }
    else
    {
        lst<<135<<85<<135<<0;
        splitterMain->setSizes(lst);
    }
    
}

void compile18::expandExplTrue()
{
    expandExpl(true);
    pushButtonExplUp->show();
    pushButtonExplDown->hide();
}


void compile18::expandExplFalse()
{
    expandExpl(false);
    pushButtonExplUp->hide();
    pushButtonExplDown->show();
}

void compile18::setPath()
{
    //+++
    QString dir=pathFIF ;
    QDir dirOld(dir);
    //+++
    if (!dirOld.exists()) dirOld = QDir::homePath();
    
    QDir dirNew;
    dir = QFileDialog::getExistingDirectory(this, "Functions - Choose a directory - path to *.fif", dir);
    
    if (dir!="")
    {
        dirNew.setPath(dir);
        if (!dirNew.exists("/IncludedFunctions")) dirNew.mkdir(dir + "/IncludedFunctions");
        fitPath->setText(dirNew.path());
        pathFIF=dirNew.path();
    }
    newFIF();
}


void compile18::gslPath()
{
    //+++
    QString dir=pathGSL.stripWhiteSpace() ;
    QDir dirOld(dir);
    //+++
    if (!dirOld.exists()) dirOld = QDir::homePath();
    
    QDir dirNew;
    dir = QFileDialog::getExistingDirectory(this,  "set path to GSL directory - Choose a directory", dir);
    if (dir=="")
    {
        pathGSL="Set GSL Directory";
        gslPathline->setText("Set GSL Directory");
    }
    else
    {
        dirNew.setPath(dir);
        gslPathline->setText(dirNew.path());
        pathGSL=dirNew.path();
    }
}


void compile18::mingwPath()
{
    //+++
    QString dir=pathMinGW.stripWhiteSpace() ;
    QDir dirOld(dir);
    //+++
    if (!dirOld.exists()) dirOld = QDir::homePath();
    
    QDir dirNew;
    dir = QFileDialog::getExistingDirectory(this,"set path to MinGw->bin directory - Choose a directory", dir);
    if (dir=="")
    {
        mingwPathline->setText("set path to MinGw->bin directory");
        pathMinGW="set path to MinGw->bin directory";
    }
    else
    {
        dirNew.setPath(dir);
        mingwPathline->setText(dirNew.path());
        pathMinGW=dirNew.path();
    }
    
}


void compile18::openFortranFilePath()
{
    QString filter = tr("Fortran file") + " (*.f *.f90 *.F *.for *.FOR);;";
    
    //+++
    QString fn = QFileDialog::getOpenFileName(this,  "QtiSAS - Fortran - File", pathFIF, filter, 0, QFileDialog::DontResolveSymlinks);
    //QFileInfo fi(fn);
    //fortranFunction->setText(fi.fileName());
    QString name=fn; name=name.remove(pathFIF);
    fortranFunction->setText(name);
    
    extructFortranFunctions(fn);
}


void compile18::extructFortranFunctions(QString fileName)
{
    QFile f(fileName);
    
    
    QString s="";
    QString ss="";
    QString sFinal="";
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        return;
    }
    else
    {
        QTextStream t( &f );
        
        while ( !t.atEnd() )
        {
            s =t.readLine();
            ss="";
            if ( s.left(1)!="!" && s.left(1)!="c" && s.left(1)!="C" && s.contains("function", false) && s.contains("(") )
            {
                if (!s.contains(")"))
                {
                    while (!s.contains(")") && !t.atEnd())
                    {
                        s +=t.readLine();
                    }
                }
                s.simplifyWhiteSpace();
                s=s.right(s.length()-s.find("function",0, false)-8);
                ss=s.left(s.find("(")).remove(" ").remove("\n").lower();
                s=s.right(s.length()-s.find("(")-1);
                s=s.left(s.find(")"));
                QStringList lst=QStringList::split(",",s);
                sFinal+= "double "+ ss + "_(";
                for (int si=0; si<lst.count(); si++) sFinal+="double*,";
                if (sFinal.right(1)==",") sFinal=sFinal.left(sFinal.length()-1);
                sFinal+=");\n";
            }
            
            if ( s.left(1)!="!" && s.left(1)!="c" && s.left(1)!="C" && s.contains("subroutine", false) && s.contains("(") )
            {
                if (!s.contains(")"))
                {
                    while (!s.contains(")") && !t.atEnd())
                    {
                        s +=t.readLine();
                    }
                }
                s.simplifyWhiteSpace();
                s=s.right(s.length()-s.find("subroutine",0, false)-10);
                ss=s.left(s.find("(")).remove(" ").remove("\n").lower();
                s=s.right(s.length()-s.find("(")-1);
                s=s.left(s.find(")"));
                QStringList lst=QStringList::split(",",s);
                sFinal+= "void "+ ss + "_(";
                for (int si=0; si<lst.count(); si++) sFinal+="double*,";
                if (sFinal.right(1)==",") sFinal=sFinal.left(sFinal.length()-1);
                sFinal+=");\n";
            }
        }
        
    }
    textEditForwardFortran->setText(sFinal);
    f.close();
    
    
}

void compile18::addHeaderFile()
{
    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count()<1) return;
    QString fn=listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();
    
    if (fn!="") addIncludedFunction( fn );
}


//*******************************************
//+++ add Included Functions
//*******************************************
void compile18::addIncludedFunction(const QModelIndex &index)
{
    QString fn=index.data().toString();
    addIncludedFunction(fn);
}

//*******************************************
//+++ add Included Functions
//*******************************************
void compile18::addIncludedFunction( const QString &fn )
{
    
    //+++
    if (!QFile::exists (pathFIF+"/IncludedFunctions/"+fn))
    {
        scanIncludedFunctions();
        return;
    }
    
    if (fn.contains(".h") || fn.contains(".cpp"))
    {
        QString sss="#include ";
        sss+="\"";
        sss+="IncludedFunctions/"+fn;
        sss+="\"";
        sss+="";
        
        if (!textEditHFiles->text().contains(sss))
            textEditHFiles->append(sss);
        //	else openInNote(fn);
        
    }
    
}

void compile18::openHeaderInNote()
{
    app()->changeFolder("FIT :: COMPILE");
    
    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count()<1) return;
    QString fn=listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();
    
    if (fn!="") openInNote( fn );
    
    app()->changeFolder("FIT :: COMPILE");
    
}

void compile18::openInNote(QString fn)
{
    app()->changeFolder("FIT :: COMPILE");
    
    QFile f;

    if (tabWidgetCode->currentIndex() == 2) {
        f.setFileName(pathFIF + "/" + fn);
    }
    else
        f.setFileName(pathFIF + "/IncludedFunctions/" + fn);
    
    
    QString s="";
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QMessageBox::critical(0, tr("QtKws"),
                              tr("Could not read from file: <br><h4>"+fn+
                                 "</h4><p>Please verify that you have the right to read from this location!"));
        return;
    }
    else
    {
        QTextStream t( &f );
        
        while ( !t.atEnd() )
            s +=t.readLine() +"\n";
        
        
    }
    f.close();
    
    
    Note* note=app()->newNoteText(fn.replace(".","-"),s);
    
    //app()->changeFolder("FIT :: COMPILE");
    //	note->importASCII(file);
    //note->setMaximized();
}

//*******************************************
//+++  make Included
//*******************************************
void compile18::makeIncluded()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              "Create header file with Included Functions - Choose a filename to save under",
                                              fitPath->text()+"/IncludedFunctions/",
                                              "Headers (*.h)");
    
    fn.remove(".cpp");
    fn.remove(".c");
    fn.remove(".c++");
    
    if (!fn.contains(".h")) fn+=".h";
    
    if (saveAsIncluded(fn))
    {
        textEditFunctions->setText("");
        textEditHFiles->setText("");
        addIncludedFunction(fn.remove(fitPath->text()+"/IncludedFunctions/"));
    }
}





bool compile18::saveAsIncluded( QString fn )
{
    qDebug() << fn;
    qDebug() << fitPath->text();
    int i;
    QString text;
    QString s=fn;
    s=s.replace(fitPath->text(),"").replace("IncludedFunctions","").replace('\/',"").replace(".","_").replace("-","_");
    
    if ( QFile::exists(fn) &&  QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "), tr("A file called: <p><b>%1</b><p>already exists.\n"   "Do you want to overwrite it?").arg(fn), tr("&Yes"), tr("&No"),QString::null, 0, 1 ) ) return false;
    
    else
    {
        //+++
        text+="#ifndef "+s+"\n";
        text+="#define "+s+"\n\n";
        
        
        text+="//[add_start]\n";
        text+="//[name] "+lineEditFunctionName->text()+"\n";
        text+="//[parameter_names]";
        int paraNumber=spinBoxP->value(); QString s;
        for(int p=0;p<paraNumber;p++) text+=" " + tableParaNames->item(p,0)->text() + (p<paraNumber-1? "," : "\n");
        text+="//[parameter_init_values]";
        for(int p=0;p<paraNumber;p++)
        {
            s=tableParaNames->item(p,1)->text();
            text+=" " + s + (p<paraNumber-1? "," : "\n");
        }
        text+="//[parameter_init_range]";
        for(int p=0;p<paraNumber;p++)
        {
            s=tableParaNames->item(p,2)->text();
            text+=" [" + (s.contains("..")? s: ".." )+ "]" + (p<paraNumber-1? "," : "\n");
        }
        text+="//[parameter_units]";
        for(int p=0;p<paraNumber;p++)
        {
            s=tableParaNames->item(p,3)->text();
            //text+=(p<paraNumber-1? " ," : " \n");
            text+=" " + (s.contains("]")? s.left(s.find("]")+1): "[?]") + (p<paraNumber-1? "," : "\n");
        }
        text+="//[parameter_info]";
        for(int p=0;p<paraNumber;p++)
        {
            s=tableParaNames->item(p,3)->text();
            
            s=(s.contains("]")? s.right(s.length()-s.find("]")-1): s);
            s=s.replace(",",";");
            s=s.simplified();
            text+=" [" + s + "]"  + (p<paraNumber-1? "," : "\n");
        }
        text+="//[info]";
        s=textEditDescription->toPlainText();
        QStringList lst=QStringList::split("\n",s);
        
        for(int i=0;i<lst.count();i++)
        {

            text+=" \"" + lst[i] + "\""  + (i<lst.count()-1? ",," : "\n");
        }

        
        
        text+="//[add_finish]\n\n";
        //\n";
        
        
        text+="//[Included Headers]\n";
        text+=textEditHFiles->text().remove("IncludedFunctions/")+"\n";
        
        //+++
        text+="//[included Functions]\n";
        text+=textEditFunctions->text();
        
        //+++
        text+="\n//[end]\n\n";
        text+="#endif\n";
        
        QFile f(fn);
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                  tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
            return false;
        }
        QTextStream t( &f );
        t.setEncoding(QTextStream::UnicodeUTF8);
        t << text;
        f.close();
        scanIncludedFunctions();
    }
    return true;
}


//*******************************************
//+++  delete FIF file
//*******************************************
void compile18::deleteIncluded()
{
    //+++
    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count()<1) return;
    QString fn=listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();
    
    if (fn=="") return;
    
    if ( QMessageBox::question(this, tr("QtiSAS :: Delete Included Function? "), tr( "Do you want to delete Included Function %1?").arg(fn), tr("&Yes"), tr("&No"),QString::null, 0, 1 ) ) return;
    
    QDir d(pathFIF+"/IncludedFunctions" );
    d.remove(fn);
    scanIncludedFunctions();
}

void compile18::openFortranFileInNote()
{
    app()->changeFolder("FIT :: COMPILE");
    
    QString fn=fortranFunction->text();
    if (fn!="") openInNote( fn );
    
    app()->changeFolder("FIT :: COMPILE");
}

void compile18::updateFiles()
{
    if ( radioButtonFIF->isChecked() && pushButtonSave->isEnabled())
    {
        makeFIF();
    }
    else
    {
        //toResLog("Function from Standard Library");
        openFIFfileSimple();
    }
    
    if ( radioButtonCPP->isChecked() ) makeCPP();
    if ( radioButtonBAT->isChecked() ) makeBATnew();
}

void compile18::openInNoteCPP()
{
    app()->changeFolder("FIT :: COMPILE");
    
    QString fn=lineEditFunctionName->text();
    
    if (radioButtonCPP->isChecked()) fn+=".cpp";
    if (radioButtonFIF->isChecked()) fn+=".fif";
    if (radioButtonBAT->isChecked()) fn="BAT.BAT";
    
    QFile f(pathFIF+"/"+fn);
    QString s="";
    
    //+++
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QMessageBox::critical(0, tr("QtKws"),
                              tr("Could not read from file: <br><h4>"+fn+
                                 "</h4><p>Please verify that you have the right to read from this location!"));
        return;
    }
    else
    {
        QTextStream t( &f );
        
        while ( !t.atEnd() )
            s +=t.readLine() +"\n";
        
        
    }
    f.close();
    
    Note* note=app()->newNoteText(fn.replace(".","-"),s);
    
    app()->changeFolder("FIT :: COMPILE");
}

//*******************************************
//+++  save Test
//*******************************************
void compile18::saveTest()
{
    if (radioButtonFIF->isChecked()) return;
    if (radioButtonCPP->text()=="*.cpp") return;
    
    QString fn;
    if (radioButtonCPP->isChecked()) fn=radioButtonCPP->text();
    if (radioButtonBAT->isChecked()) fn=radioButtonBAT->text();
    fn=fitPath->text()+"/"+fn;
    
    QString text;
    
    for(int i=0;i<tableCPP->rowCount()-1;i++) text+=tableCPP->item(i,0)->text()+"\n";
    
    QFile f(fn);
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        
        QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                              tr("Could not writ<e to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
        
         return;
    }

    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << text;
    f.close();
}

