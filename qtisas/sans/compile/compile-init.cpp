/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Init functions of compile interface
 ******************************************************************************/

#include <QFontDatabase>
#include <QSizePolicy>

#include "compile18.h"
#include "fittable18.h"
#include "highlighter.h"
#include <gsl/gsl_version.h>

//+++  CONNECTION SLOT
void compile18::connectSlot()
{
    connect(fitPath, &QLineEdit::textChanged, this, &compile18::pathChanged);
    connect(pushButtonMenu, &QToolButton::clicked, this, &compile18::showMenu);
    connect(pushButtonParaDown, &QToolButton::clicked, this, &compile18::expandParaTrue);
    connect(pushButtonParaUp, &QToolButton::clicked, this, &compile18::expandParaFalse);
    connect(pushButtonCodeDown, &QToolButton::clicked, this, &compile18::expandCodeTrue);
    connect(pushButtonCodeUp, &QToolButton::clicked, this, &compile18::expandCodeFalse);
    connect(pushButtonExplDown, &QToolButton::clicked, this, &compile18::expandExplTrue);
    connect(pushButtonExplUp, &QToolButton::clicked, this, &compile18::expandExplFalse);

// compile-explorer
    connect(lineEditFunctionName, &QLineEdit::textChanged, this, &compile18::newFunctionName);
    connect(lineEditGroupName, &QLineEdit::textChanged, this, &compile18::newCategoryName);
    connect(textLabelGroupName, &QToolButton::clicked, this, &compile18::newFIF);
    connect(radioButton1D, &QRadioButton::clicked, this, &compile18::stot1Dto2D);
    connect(radioButton2D, &QRadioButton::clicked, this, &compile18::stot1Dto2D);
    connect(pushButtonDownload, &QToolButton::clicked, this, &compile18::downloadFIF);
    connect(pushButtonNew, &QToolButton::clicked, this, &compile18::newFIF);
    connect(pushButtonSave, &QToolButton::clicked, this, &compile18::makeFIF);
    connect(pushButtonDelete, &QToolButton::clicked, this, &compile18::deleteFIF);
    connect(pushButtonBuild, &QToolButton::clicked, this, &compile18::buildSharedLibrary);
    connect(pushButtonCompileAll, &QToolButton::clicked, this, &compile18::compileAll);
    connect(lineEditFunctionName, &QLineEdit::editingFinished, this, &compile18::checkFunctionName);

// compile-parameters
    connect(spinBoxP, QOverload<int>::of(&QSpinBox::valueChanged), this, &compile18::setNumberparameters);
    connect(spinBoxXnumber, QOverload<int>::of(&QSpinBox::valueChanged), this, &compile18::changedNumberIndepvar);
    connect(tableParaNames->verticalHeader(), &QHeaderView::sectionClicked, this, &compile18::moveParaLine);
    connect(tableParaNames, &QTableWidget::itemSelectionChanged, this, &compile18::selectRowsTableMultiFit);
    connect(lineEditY, &QLineEdit::textChanged, this, &compile18::changedFXYinfo);
    connect(lineEditXXX, &QLineEdit::textChanged, this, &compile18::changedFXYinfo);
    connect(pushButtonPath, &QToolButton::clicked, this, &compile18::setPath);
    connect(pushButtonFortranFunction, &QToolButton::clicked, this, &compile18::openFortranFilePath);
    connect(pushButtonBatFileMSVC, &QToolButton::clicked, this, &compile18::selectionBatFileMSVC);
    connect(pushButtonAddHeader, &QToolButton::clicked, this, &compile18::addHeaderFile);
    connect(pushButtonOpenInNote, &QToolButton::clicked, this, &compile18::openHeaderInNote);
    connect(pushButtonFortranFunctionView, &QToolButton::clicked, this, &compile18::openFortranFileInNote);
    connect(pushButtonMakeIncluded, &QToolButton::clicked, this, &compile18::makeIncluded);
    connect(pushButtonIncludedDelete, &QToolButton::clicked, this, &compile18::deleteIncluded);
    connect(radioButtonBAT, &QRadioButton::clicked, this, &compile18::updateFiles);
    connect(radioButtonFIF, &QRadioButton::clicked, this, &compile18::updateFiles);
    connect(radioButtonCPP, &QRadioButton::clicked, this, &compile18::updateFiles);
    connect(pushButtonTestSave, &QToolButton::clicked, this, &compile18::saveTest);
    connect(pushButtonTestCompile, &QToolButton::clicked, this, &compile18::compileTest);
    connect(pushButtonInNoteFiles, &QToolButton::clicked, this, &compile18::openInNoteCPP);

// compile-info
    connect(toolButtonDeleteFormat, &QToolButton::clicked, this, &compile18::deleteFormat);
    connect(pushButtonBold, &QToolButton::clicked, this, &compile18::textBold);
    connect(pushButtonEXP, &QToolButton::clicked, this, &compile18::textEXP);
    connect(pushButtonSub, &QToolButton::clicked, this, &compile18::textIndex);
    connect(pushButtonLINK, &QToolButton::clicked, this, &compile18::textLINK);
    connect(pushButtonJust, &QToolButton::clicked, this, &compile18::textJust);
    connect(pushButtonRight, &QToolButton::clicked, this, &compile18::textRight);
    connect(pushButtonCenter, &QToolButton::clicked, this, &compile18::textCenter);
    connect(pushButtonLeft, &QToolButton::clicked, this, &compile18::textLeft);
    connect(pushButtonItal, &QToolButton::clicked, this, &compile18::textItalic);
    connect(pushButtonUnder, &QToolButton::clicked, this, &compile18::textUnderline);
    connect(pushButtonGreek, &QToolButton::clicked, this, &compile18::textGreek);
    connect(checkBoxAddFortran, &QCheckBox::toggled, textEditForwardFortran, &QWidget::setEnabled);

    connect(comboBoxFontSize, &QComboBox::textActivated, this, &compile18::textSize);
    connect(comboBoxFont, &QComboBox::textActivated, this, &compile18::textFamily);
    connect(textEditDescription, &QTextEdit::cursorPositionChanged, this, &compile18::readTextFormatting);

// compile-menu
    connect(pushButtonMenuSTD, &QToolButton::clicked, this, &compile18::stdMenu);
    connect(pushButtonMenuFlags, &QToolButton::clicked, this, &compile18::flagsMenu);
    connect(pushButtonMenuData, &QToolButton::clicked, this, &compile18::dataMenu);
    connect(pushButtonMenuMath, &QToolButton::clicked, this, &compile18::mathMenu);
    connect(pushButtonMenuMULTI, &QToolButton::clicked, this, &compile18::multiMenu);
    connect(pushButtonMenuSASVIEW, &QToolButton::clicked, this, &compile18::sasviewMenu);
    connect(pushButtonMenuQTIKWS, &QToolButton::clicked, this, &compile18::qtisasMenu);
    connect(pushButtonMenuFORTRAN, &QToolButton::clicked, this, &compile18::fortranMenu);

    //+++ find
    connect(lineEditFind, &QLineEdit::returnPressed, this, &compile18::findInCode);
    connect(lineEditFindIncluded, &QLineEdit::returnPressed, this, &compile18::findInIncluded);
    connect(pushButtonOpenSasViewPy, &QToolButton::clicked, this, &compile18::openSasViewPy);

    //+++ textEditHFiles
    connect(textEditHFiles, &QTextEdit::textChanged, this, [this]() {
        int offset = static_cast<int>(textEditHFiles->toPlainText().count("#include")) + 163 + spinBoxP->value() * 2;
        lnTextEditFunctions->firstLineIncrement = offset;
        lnTextEditFunctions->updateLineNumbers(true);
        textEditFunctions->textChanged();
    });

    //+++ textEditFunctions
    connect(textEditFunctions, &QTextEdit::textChanged, this, [this]() {
        int offset = lnTextEditFunctions->firstLineIncrement + textEditFunctions->document()->blockCount() + 14;
        if (checkBoxAddFortran->isChecked())
            offset += 8 + textEditForwardFortran->document()->blockCount();
        lnTextEditCode->firstLineIncrement = offset;
        lnTextEditCode->updateLineNumbers(true);
    });

    //+++ pushButtonPythonCall
    connect(pushButtonPythonCall, &QToolButton::clicked, this, [this]() {
        if (checkBoxAfterFitPython->isChecked())
            app()->scriptCaller(textEditAfterFitPython->toPlainText());
    });
}
//+++ set Font / ForceD
void compile18::setFontForce(const QFont &font) const
{
    //+++ ln TextEditHFiles
    lnTextEditHFiles->firstLineIncrement = 17;
    lnTextEditHFiles->setCurrentFont(font);
    lnTextEditHFiles->updateLineNumbers(true);

    //+++ ln TextEditFunctions
    lnTextEditFunctions->setCurrentFont(font);
    lnTextEditFunctions->updateLineNumbers(true);

    //+++ ln TextEditCode
    lnTextEditCode->setCurrentFont(font);
    lnTextEditCode->updateLineNumbers(true);
}
//+++ changeFixedSizeH
void compile18::changeFixedSizeH(QWidget *obj, int H)
{
    obj->setMinimumSize(0, H);
    obj->setMaximumSize(32767, H);
}
//+++ changeFixedSizeH
void compile18::changeFixedSizeHW(QWidget *obj, int H, int W)
{
    obj->setMinimumSize(W, H);
    obj->setMaximumSize(W, H);
}
//+++ init Screen Resolusion Dependent Parameters
void compile18::initScreenResolusionDependentParameters(int hResolusion, double sasResoScale)
{
    int labelHight = int(hResolusion * sasResoScale / 50);
    int fontIncr = app()->sasFontIncrement + 1;
    int newH = labelHight;
    int newW = int(double(labelHight) / 1.5);

    if (labelHight < 30)
    {
        newH = 30;
        newW = 15;
    }

    //+++ QTableWidget
    foreach (QTableWidget *obj, this->findChildren<QTableWidget *>())
    {
        obj->verticalHeader()->setDefaultSectionSize(labelHight + 5);
        obj->verticalHeader()->setMinimumWidth(labelHight + 5);
    }

    //+++ Labels
    foreach (QLabel *obj, this->findChildren<QLabel *>())
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }

    //+++ QLineEdit
    foreach (QLineEdit *obj, this->findChildren<QLineEdit *>())
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }

    //+++ QPushButton
    foreach (QPushButton *obj, this->findChildren<QPushButton *>())
    {
        if (obj->baseSize().width() == 25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            obj->setIconSize(QSize(int(0.75 * labelHight), int(0.75 * labelHight)));
        }
        else if (obj->baseSize().width() == 14)
        {
            QFont fonts = obj->font();

            obj->setMaximumWidth(fonts.pointSize() + 5);
            obj->setMaximumWidth(fonts.pointSize() + 5);
            obj->setMinimumHeight(fonts.pointSize() + 5);
            obj->setMaximumHeight(fonts.pointSize() + 5);
        }
        else if (obj->baseSize().width() == 10)
        {
            obj->setMinimumHeight(newH);
            obj->setMaximumHeight(newH);
        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            obj->setIconSize(QSize(int(0.75 * labelHight), int(0.75 * labelHight)));
        }
    }

    //+++ QToolButton
    foreach (QToolButton *obj, this->findChildren<QToolButton *>())
    {
        if (obj->baseSize().width() == 25)
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);
            obj->setMinimumWidth(labelHight);
            obj->setMaximumWidth(labelHight);
            obj->setIconSize(QSize(int(0.75 * labelHight), int(0.75 * labelHight)));
        }
        else if (obj->baseSize().width() == 14)
        {
            QFont fonts = obj->font();

            obj->setMaximumWidth(fonts.pointSize() + 5);
            obj->setMaximumWidth(fonts.pointSize() + 5);
            obj->setMinimumHeight(fonts.pointSize() + 5);
            obj->setMaximumHeight(fonts.pointSize() + 5);
        }
        else
        {
            obj->setMinimumHeight(labelHight);
            obj->setMaximumHeight(labelHight);

            if (obj->sizeIncrement().width() > 0)
            {
                int baseWidth = obj->sizeIncrement().width();
                if (fontIncr > 0)
                    baseWidth += static_cast<int>(2.0 * fontIncr * static_cast<double>(obj->text().length()));

                obj->setMinimumWidth(baseWidth);
                obj->setMaximumWidth(baseWidth);
            }

            obj->setIconSize(QSize(int(0.75 * labelHight), int(0.75 * labelHight)));
        }
    }

    //+++ QCheckBox
    foreach (QCheckBox *obj, this->findChildren<QCheckBox *>())
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }

    //+++ QComboBox
    foreach (QComboBox *obj, this->findChildren<QComboBox *>())
    {
        obj->setMinimumHeight(labelHight + 5);
        obj->setMaximumHeight(labelHight + 5);
    }

    //+++ QSpinBox
    foreach (QSpinBox *obj, this->findChildren<QSpinBox *>())
    {
        obj->setMinimumHeight(labelHight);
        obj->setMaximumHeight(labelHight);
    }

    changeFixedSizeH(textLabelInfoSAS, labelHight);
    changeFixedSizeH(textLabelInfoVersion, labelHight);
    changeFixedSizeH(textLabelInfoSASauthor, labelHight);

    changeFixedSizeH(frameControl, labelHight + 24);

    changeFixedSizeHW(spinBoxP, newH + 2, newW + 2);
    changeFixedSizeHW(spinBoxParaHun, newH, newW);
    changeFixedSizeHW(spinBoxParaDec, newH, newW);
    changeFixedSizeHW(spinBoxPara,  newH, newW);

    tableParaNames->horizontalHeader()->setMinimumHeight(labelHight);

#ifdef Q_OS_WIN
    spinBoxP->setStyleSheet(QString("QSpinBox::up-arrow { width: %1; } "
                                    "QSpinBox::up-button { width: %2; } "
                                    "QSpinBox::down-arrow { width: %1; } "
                                    "QSpinBox::down-button { width: %2; };")
                                .arg(QString::number(newW))
                                .arg(newW + 2));
#endif

    if (pushButtonMenu->text() == QChar(0x226A))
        frameMenu->setMaximumWidth(3000);
    else
        frameMenu->setMaximumWidth(0);

    QFont font = pushButtonMenu->font();
    QFontMetrics fm(font);

    //+++ frameMenu
    foreach (QToolButton *obj, tabCode->findChildren<QToolButton *>())
    {
        obj->setMinimumWidth(fm.horizontalAdvance(obj->text()) + 10);
        obj->setMaximumWidth(fm.horizontalAdvance(obj->text()) + 10);
        obj->setMinimumHeight(fm.height() + 8);
        obj->setMaximumHeight(fm.height() + 8);
    }

    frameMenu->setMinimumHeight(fm.height() + 8);
    frameMenu->setMaximumHeight(fm.height() + 8);

}
//+++ initiation
void compile18::initCompile()
{
    radioButton1D->hide();
    radioButton2D->hide();

    lnTextEditCode = new LineNumberDisplay(textEditCode, this);
    lnTextEditCode->setVisible(true);
    horizontalLayoutEditCode->insertWidget(0, lnTextEditCode);
    lnTextEditCode->setStyleSheet("background-color: transparent; "
                                  "color: gray; "
                                  "selection-color: white; "
                                  "selection-background-color: rgb(137, 137, 183);");

    lnTextEditFunctions = new LineNumberDisplay(textEditFunctions, this);
    lnTextEditFunctions->setVisible(true);
    horizontalLayoutTextEditFunctions->insertWidget(0,lnTextEditFunctions);
    lnTextEditFunctions->setStyleSheet("background-color: transparent; "
                                       "color: gray; "
                                       "selection-color: white; "
                                       "selection-background-color: rgb(137, 137, 183);");

    //+++ TextEditHFiles
    lnTextEditHFiles = new LineNumberDisplay(textEditHFiles, this);
    lnTextEditHFiles->setVisible(true);
    horizontalLayoutTextEditHFiles->insertWidget(0,lnTextEditHFiles);
    lnTextEditHFiles->setStyleSheet("background-color: transparent; "
                                    "color: gray; "
                                    "selection-color: white; "
                                    "selection-background-color: rgb(137, 137, 183);");

    spinBoxP->findChild<QLineEdit*>()->setMaximumWidth(0);

    stot1Dto2D();
    defaultOptions();
    readSettings ();
    scanGroups();
    scanIncludedFunctions();

    textEditDescription->setFontFamily("Arial");
    textEditDescription->setFontPointSize(12);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    comboBoxFont->addItems(QFontDatabase().families());
#else
    comboBoxFont->addItems(QFontDatabase::families());
#endif

    QList<int> sizes = QFontDatabase::standardSizes();
    QList<int>::Iterator it = sizes.begin();
    for (; it != sizes.end(); ++it)
        comboBoxFontSize->addItem(QString::number(*it));
    boolYN = false;

    expandExpl(false);

    tableCPP->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    pushButtonParaUp->hide();
    pushButtonExplUp->hide();
    pushButtonCodeUp->hide();

    QString toolText = "Main Code Window";
    textEditCode->setToolTip(toolText);

    pushButtonMenu->setText(QChar(0x226A));
    showMenu();

    spinBoxParaHun->hide();
    spinBoxParaDec->hide();
    
    auto highlighter1 = new Highlighter(textEditCode->document());
    auto highlighter2 = new Highlighter(textEditFunctions->document());
    auto highlighter3 = new Highlighter(textEditHFiles->document());
    auto highlighter4 = new Highlighter(textEditForwardFortran->document());
    auto highlighter5 = new Highlighter(textEditAfterFitPython->document());

    tabWidgetCode->setCurrentIndex(0);

    tableParaNames->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    tableParaNames->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tableParaNames->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
}
//+++ called when "default" button pressed
void compile18::defaultOptions()
{
    pathFIF = app()->sasPath + "FitFunctions/";

    if (!QDir(pathFIF).exists())
    {
        QDir().mkdir(pathFIF);
        QDir().mkdir(pathFIF + "IncludedFunctions");
    }

    fitPath->setText(pathFIF);
    pathChanged();

    compilerLocal();

    QString gslVersion = QString::number(GSL_MAJOR_VERSION) + QString::number(GSL_MINOR_VERSION);
    QString gslcblasVersion = QString::number(0);

    // Link Flags
    QString compileFlag;
    QString linkFlag;

#if defined(Q_OS_MAC) // MAC
    compileFlag = "clang -fPIC -w -c -I.";
    linkFlag = "clang -lc++ -Wall -shared -lgsl." + gslVersion + " -lgslcblas." + gslcblasVersion + " -o";
#elif defined(Q_OS_WIN) // WIN
    compileFlag = "cl /nologo /W3 /c /I$GSL";
    linkFlag = "link /NOLOGO /DLL $GSL\\gsl.lib $GSL\\gslcblas.lib";
#else                   // LINUX
    compileFlag = "g++ -fPIC -w -c -I.";
    linkFlag = "g++ -Wall -shared -lgsl -lgslcblas -o";
#endif

    lineEditCompileFlags->setText(compileFlag);
    lineEditLinkFlags->setText(linkFlag);
}
//+++ showMenu
void compile18::showMenu()
{
    if (pushButtonMenu->text() == QChar(0x226A))
    {
        pushButtonMenu->setText(QChar(0x226B));
        frameMenu->setMaximumWidth(0);
    }
    else
    {
        pushButtonMenu->setText(QChar(0x226A));
        frameMenu->setMaximumWidth(3000);
    }
}
//+++ expandExpl
void compile18::expandExpl(bool YN)
{
    QList<int> lst;
    
    if (YN)
    {
        lst << 100 << 0 << 0 << 0;
        splitterMain->setSizes(lst);
    }
    else
    {
        if (textLabelInfoVersion->isHidden())
            lst << 135 << 85 << 135 << 0;
        else
            lst << 135 << 85 << 135 << 5;
        splitterMain->setSizes(lst);
    }
}
//+++ compiler-local options
void compile18::compilerLocal()
{
#ifdef Q_OS_WIN

    textLabelMSVC->show();
    lineEditBatFileMSVC->show();
    pushButtonBatFileMSVC->show();

    QString progFilesFolder =
        qEnvironmentVariable("PROGRAMFILES").replace('\\', '/') + "/Microsoft Visual Studio/2022/";
    QStringList msvcVersions = {"Preview", "Community", "Enterprise", "Professional"};

    QString arm = "";
#if defined(Q_PROCESSOR_ARM_64)
    arm = "arm";
#endif

    batFileMSVC = "";
    for (const QString &version : msvcVersions)
    {
        QString potentialVcsvar = progFilesFolder + version + "/VC/Auxiliary/Build/vcvars" + arm + "64.bat";
        if (QFileInfo(potentialVcsvar).exists())
        {
            batFileMSVC = potentialVcsvar;
            break;
        }
    }

    if (batFileMSVC.isEmpty())
        batFileMSVC = "Select MSVC Compiler bat-file!!!";

    lineEditBatFileMSVC->setText(batFileMSVC);
#else
    textLabelMSVC->hide();
    lineEditBatFileMSVC->hide();
    pushButtonBatFileMSVC->hide();
    lineEditBatFileMSVC->setText("");
#endif
}
//+++  Plus/Minus button: Expand Parameters Block
void compile18::expandParaTrue()
{
    expandPara(true);
    pushButtonParaUp->show();
    pushButtonParaDown->hide();
    tableParaNames->setFocus();
}
//+++ Plus/Minus button: Expand Parameters Block
void compile18::expandParaFalse()
{
    expandPara(false);
    pushButtonParaUp->hide();
    pushButtonParaDown->show();
}
//+++ Plus/Minus button: Expand Parameters Block
void compile18::expandPara(bool YN)
{
    QList<int> lst;

    if (YN)
    {
        lst << 0 << 350 << 0 << 5;
        splitterMain->setSizes(lst);
    }
    else
    {
        lst << 135 << 85 << 135 << 0;
        splitterMain->setSizes(lst);
    }
}
//+++ Plus/Minus button: Expand Code Block
void compile18::expandCodeTrue()
{
    expandCode(true);
    pushButtonCodeUp->show();
    pushButtonCodeDown->hide();
    tabWidgetCode->setFocus();
}
//+++ Plus/Minus button: Expand Code Block
void compile18::expandCodeFalse()
{
    expandCode(false);
    pushButtonCodeUp->hide();
    pushButtonCodeDown->show();
}
//+++ Plus/Minus button: Expand Code Block
void compile18::expandCode(bool YN)
{
    QList<int> lst;

    if (YN)
    {
        lst << 0 << 0 << 350 << 5;
        splitterMain->setSizes(lst);
        lst.clear();
        QList<int> lst22;
        lst22 << 40 << 320;
        splitterInc->setSizes(lst22);
    }
    else
    {
        lst << 135 << 85 << 135 << 0;
        splitterMain->setSizes(lst);
    }
}
//+++  Plus/Minus button: Expand Explorer Block
void compile18::expandExplTrue()
{
    expandExpl(true);
    pushButtonExplUp->show();
    pushButtonExplDown->hide();
}
//+++ Plus/Minus button: Expand Explorer Block
void compile18::expandExplFalse()
{
    expandExpl(false);
    pushButtonExplUp->hide();
    pushButtonExplDown->show();
}
//+++ Set Path
void compile18::setPath()
{
    QString dir = pathFIF;
    QDir dirOld(dir);

    if (!dirOld.exists())
        dirOld.setPath(QDir::homePath());

    QDir dirNew;
    dir = QFileDialog::getExistingDirectory(this, "Functions - Choose a directory - path to *.fif", dir);

    if (dir != "")
    {
        dirNew.setPath(dir);
        if (!dirNew.exists("/IncludedFunctions"))
            dirNew.mkdir(dir + "/IncludedFunctions");

        pathFIF = dirNew.path() + "/";
        fitPath->setText(pathFIF);

        app()->fittableWidget->setPathExtern(pathFIF);
        app()->fittableWidget->scanGroup();
    }
    newFIF();
}
//+++ Compiler Path
void compile18::selectionBatFileMSVC()
{
    QString lastPath = QFileInfo(batFileMSVC).absolutePath();
    if (!QDir(lastPath).exists())
        lastPath = QDir::homePath();

    QString selectedFile = QFileDialog::getOpenFileName(this, "Select MSVC bat-file", lastPath, "Batch Files (*.bat)");

    if (selectedFile.isEmpty())
    {
        lineEditBatFileMSVC->setText("select bat-file for MSVC compiler");
        batFileMSVC = "select bat-file for MSVC compiler";
    }
    else
    {
        lineEditBatFileMSVC->setText(selectedFile);
        batFileMSVC = selectedFile;
    }
}
//+++ Fortran Path
void compile18::openFortranFilePath()
{
    QString filter = tr("Fortran file") + " (*.f *.f90 *.F *.for *.FOR);;";

    QString fn = QFileDialog::getOpenFileName(this, "QtiSAS - Fortran - File", pathFIF, filter, nullptr,
                                              QFileDialog::DontResolveSymlinks);

    extructFortranFunctions(fn);
}
//+++ add Fortran Functions
void compile18::extructFortranFunctions(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QString name = fileName;
    name.replace("//", "/").remove(app()->sasPath).remove("FitFunctions/");
    if (!name.contains(".f", Qt::CaseInsensitive))
        return;

    QString fortranFiles = fortranFunction->text();
    if (fortranFiles.contains(name))
        return;

    if (fortranFiles.contains(".f", Qt::CaseInsensitive))
        fortranFiles += ", " + name;
    else
        fortranFiles = name;

    fortranFunction->setText(fortranFiles);

    checkBoxAddFortran->setChecked(true);
    tabWidgetCode->setCurrentIndex(2); // "Fortran" Tab

    QTextStream t(&f);

    QString s = "", ss = "", sFinal = "";

    while (!t.atEnd())
    {
        s = t.readLine();
        ss = "";
        if (s.left(1) != "!" && s.left(1) != "c" && s.left(1) != "C" && s.contains("function", Qt::CaseInsensitive) &&
            s.contains("("))
        {
            if (!s.contains(")"))
            {
                while (!s.contains(")") && !t.atEnd())
                    s += t.readLine();
            }
            s = s.simplified();
            s = s.right(s.length() - s.indexOf("function", 0, Qt::CaseInsensitive) - 8);
            ss = s.left(s.indexOf("(")).remove(" ").remove("\n").toLower();
            s = s.right(s.length() - s.indexOf("(") - 1);
            s = s.left(s.indexOf(")"));

            QStringList lst = s.split(",", Qt::SkipEmptyParts);
            sFinal += "double " + ss + "_(";
            for (int si = 0; si < lst.count(); si++)
                sFinal += "double*,";
            if (sFinal.right(1) == ",")
                sFinal = sFinal.left(sFinal.length() - 1);
            sFinal += ");\n";
        }

        if (s.left(1) != "!" && s.left(1) != "c" && s.left(1) != "C" && s.contains("subroutine", Qt::CaseInsensitive) &&
            s.contains("("))
        {
            if (!s.contains(")"))
                while (!s.contains(")") && !t.atEnd())
                    s += t.readLine();

            s = s.simplified();
            s = s.right(s.length() - s.indexOf("subroutine", 0, Qt::CaseInsensitive) - 10);
            ss = s.left(s.indexOf("(")).remove(" ").remove("\n").toLower();
            s = s.right(s.length() - s.indexOf("(") - 1);
            s = s.left(s.indexOf(")"));

            QStringList lst = s.split(",", Qt::SkipEmptyParts);
            sFinal += "void " + ss + "_(";

            for (int si = 0; si < lst.count(); si++)
                sFinal += "double*,";

            if (sFinal.right(1) == ",")
                sFinal = sFinal.left(sFinal.length() - 1);
            sFinal += ");\n";
        }
    }
    textEditForwardFortran->append(sFinal);
    f.close();
}
//+++ Add Header File
void compile18::addHeaderFile()
{
    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count() < 1)
        return;

    QString fn = listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();

    if (fn != "")
        addIncludedFunction(fn);
}
//+++add Included Functions
void compile18::addIncludedFunction(const QModelIndex &index)
{
    QString fn = index.data().toString();
    addIncludedFunction(fn);
}
//+++ Add Included Functions
void compile18::addIncludedFunction(const QString &fn)
{
    if (!QFile::exists(pathFIF + "/IncludedFunctions/" + fn))
    {
        scanIncludedFunctions();
        return;
    }

    if (fn.contains(".f", Qt::CaseInsensitive))
        extructFortranFunctions(pathFIF + "IncludedFunctions/" + fn);

    if (fn.contains(".h") || fn.contains(".cpp"))
    {
        QString sss = "#include ";
        sss += "\"";
        sss += "IncludedFunctions/" + fn;
        sss += "\"";
        sss += "";

        if (!textEditHFiles->toPlainText().contains(sss))
            textEditHFiles->append(sss);
    }
}
//+++ open Header In Note
void compile18::openHeaderInNote()
{
    app()->changeFolder("FIT :: COMPILE");

    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count() < 1)
        return;

    QString fn = listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();

    if (fn != "")
        openInNote(fn);

    app()->changeFolder("FIT :: COMPILE");
}
//+++ Open a File in None
void compile18::openInNote(QString fn)
{
    app()->changeFolder("FIT :: COMPILE");

    QFile f;
    if (tabWidgetCode->currentIndex() == 2)
        f.setFileName(pathFIF + "/" + fn);
    else
        f.setFileName(pathFIF + "/IncludedFunctions/" + fn);

    QString s = "";

    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, tr("QtiSAS"),
        tr(QString("Could not read from file: <br><h4>" + fn +
                "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
        return;
    }
    else
    {
        QTextStream t(&f);
        while (!t.atEnd())
            s += t.readLine() + "\n";
    }
    f.close();

    Note *note = app()->newNoteText(fn.replace(".", "-"), s);
}
//+++ make Included
void compile18::makeIncluded()
{
    QString fn = QFileDialog::getSaveFileName(
        this, "Create header file with Included Functions - Choose a filename to save under",
        fitPath->text() + "/IncludedFunctions/", "Headers (*.h)");

    fn.remove(".cpp");
    fn.remove(".c");
    fn.remove(".c++");

    if (!fn.contains(".h"))
        fn += ".h";

    if (saveAsIncluded(fn))
    {
        textEditFunctions->setText("");
        textEditHFiles->setText("");
        addIncludedFunction(fn.remove(fitPath->text() + "/IncludedFunctions/"));
    }
}
//+++ Save as Included a File
bool compile18::saveAsIncluded(const QString &fn)
{
    qDebug() << fn;
    qDebug() << fitPath->text();
    int i;
    QString text;
    QString s = fn;
    s = s.replace(fitPath->text(), "")
            .replace("IncludedFunctions", "")
            .replace('/', "")
            .replace(".", "_")
            .replace("-", "_");

    if (QFile::exists(fn) && QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "),
                                                   tr("A file called: <p><b>%1</b><p>already exists.\n"
                                                      "Do you want to overwrite it?")
                                                       .arg(fn),
                                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return false;
    else
    {
        text += "#ifndef " + s + "\n";
        text += "#define " + s + "\n\n";
        text += "//[add_start]\n";
        text += "//[name] " + lineEditFunctionName->text() + "\n";
        text += "//[parameter_names]";

        int paraNumber = spinBoxP->value();

        QString s;
        for (int p = 0; p < paraNumber; p++)
            text += " " + tableParaNames->item(p, 0)->text() + (p < paraNumber - 1 ? "," : "\n");

        text += "//[parameter_init_values]";

        for (int p = 0; p < paraNumber; p++)
        {
            s = tableParaNames->item(p, 1)->text();
            text += " " + s + (p < paraNumber - 1 ? "," : "\n");
        }

        text += "//[parameter_init_range]";
        for (int p = 0; p < paraNumber; p++)
        {
            s = tableParaNames->item(p, 2)->text();
            text += " [" + (s.contains("..") ? s : "..") + "]" + (p < paraNumber - 1 ? "," : "\n");
        }

        text += "//[parameter_units]";

        for (int p = 0; p < paraNumber; p++)
        {
            s = tableParaNames->item(p, 3)->text();
            text += " " + (s.contains("]") ? s.left(s.indexOf("]") + 1) : "[?]") + (p < paraNumber - 1 ? "," : "\n");
        }

        text += "//[parameter_info]";

        for (int p = 0; p < paraNumber; p++)
        {
            s = tableParaNames->item(p, 3)->text();
            s = (s.contains("]") ? s.right(s.length() - s.indexOf("]") - 1) : s);
            s = s.replace(",", ";");
            s = s.simplified();
            text += " [" + s + "]" + (p < paraNumber - 1 ? "," : "\n");
        }

        text += "//[info]";
        s = textEditDescription->toPlainText();
        QStringList lst = s.split("\n", Qt::SkipEmptyParts);

        for (int i = 0; i < lst.count(); i++)
            text += " \"" + lst[i] + "\"" + (i < lst.count() - 1 ? ",," : "\n");

        text += "//[add_finish]\n\n";

        text += "//[Included Headers]\n";
        text += textEditHFiles->toPlainText().remove("IncludedFunctions/") + "\n";

        text += "//[included Functions]\n";
        text += textEditFunctions->toPlainText();

        text += "\n//[end]\n\n";
        text += "#endif\n";

        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(nullptr, "QtiSAS - File Save Error",
                                  tr("Could not write to file: <br><h4>%1</h4><p>"
                                     "Please verify that you have the right to write to this location!")
                                      .arg(fn));
            return false;
        }

        QTextStream t(&f);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
        t << text;
        f.close();
        scanIncludedFunctions();
    }
    return true;
}
//+++ Delete FIF file
void compile18::deleteIncluded()
{
    if (listBoxIncludeFunctionsNew->selectionModel()->selectedRows().count() < 1)
        return;

    QString fn = listBoxIncludeFunctionsNew->selectionModel()->selectedRows()[0].data().toString();
    if (fn == "")
        return;

    if (QMessageBox::question(this, "QtiSAS :: Delete Included Function?",
                              tr("Do you want to delete Included Function %1?").arg(fn),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return;

    QDir d(pathFIF + "/IncludedFunctions");
    d.remove(fn);
    scanIncludedFunctions();
}
//+++ Open a Fortran File In Note
void compile18::openFortranFileInNote()
{
    app()->changeFolder("FIT :: COMPILE");

    QString fn = fortranFunction->text();
    if (fn != "")
        openInNote(fn);

    app()->changeFolder("FIT :: COMPILE");
}
//+++ Update Files
void compile18::updateFiles()
{
    if (radioButtonFIF->isChecked())
        makeFIF();

    if (radioButtonCPP->isChecked())
        makeCPP();

    if (radioButtonBAT->isChecked())
        makeCompileScript();
}
//+++ Open In Note a CPP file
void compile18::openInNoteCPP()
{
    app()->changeFolder("FIT :: COMPILE");

    QString fn = lineEditFunctionName->text();

    if (radioButtonCPP->isChecked()) 
        fn += ".cpp";
    if (radioButtonFIF->isChecked()) 
        fn += ".fif";
    if (radioButtonBAT->isChecked())
    {
        fn = "compile.script";
#if defined(Q_OS_WIN)
        fn += ".ps1";
#endif
    }

    QFile f(pathFIF + "/" + fn);
    QString s = "";

    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, tr("QtiSAS"),
                              tr(QString("Could not read from file: <br><h4>" + fn +
                                 "</h4><p>Please verify that you have the right to read from this location!").toLocal8Bit().constData()));
        return;
    }
    else
    {
        QTextStream t(&f);
        while (!t.atEnd())
            s += t.readLine() + "\n";
    }

    f.close();

    Note *note = app()->newNoteText(fn.replace(".", "-"), s);

    app()->changeFolder("FIT :: COMPILE");
}
//+++ Save Test
void compile18::saveTest()
{
    if (radioButtonFIF->isChecked()) 
        return;
    if (radioButtonCPP->text() == "*.cpp")
        return;

    QString fn;
    
    if (radioButtonCPP->isChecked()) 
        fn = radioButtonCPP->text();

    if (radioButtonBAT->isChecked())
    {
        fn = "compile.script";
#if defined(Q_OS_WIN)
        fn += ".ps1";
#endif
    }

    fn = fitPath->text() + "/" + fn;

    QString text;

    for (int i = 0; i < tableCPP->rowCount() - 1; i++)
        text += tableCPP->item(i, 0)->text() + "\n";

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(nullptr, "QtiSAS - File Save Error",
                              QString("Could not write to file:<br><h4>%1</h4>"
                                      "<p>Please verify that you have the right to write to this location!")
                                  .arg(fn));
        return;
    }

    QTextStream t(&f);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << text;
    f.close();
}