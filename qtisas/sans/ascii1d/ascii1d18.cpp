/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SAS data (radially averaged) Import/Export interface
 ******************************************************************************/

#include "ascii1d18.h"
#include "Folder.h"

#include <QDockWidget>
#include <QDebug>
#include <PlotCurve.h>

ascii1d18::ascii1d18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    
    //+++ Read Settings
    readSettings();
    
    //+++ connections
    connectSlot();
    
    //+++
    findASCII1DFormats();
    
    //+++
    sourceSelected();
    
    //+++
    asciiFormatSelected();
    
    //+++
    asciiTab->setCurrentIndex(0);
    toolBoxChange->setCurrentIndex(1);
    
    //buttonGroupResoSAXS->hide();
    //radioButtonSAXS->hide();
}

// *******************************************
// *** connectSlot()
// *******************************************
void ascii1d18::connectSlot()
{
    connect( ASCIIpushButton, SIGNAL( clicked() ), this, SLOT( loadASCIIall() ) );
    
    //+++ options
    connect( comboBoxFormat, SIGNAL( activated(int) ), this, SLOT( asciiFormatSelected() ) );
    
    //+++ path
    connect( pushButtonPath, SIGNAL( clicked() ), this, SLOT( buttonPath() ) );
    connect( pushButtonPathOut, SIGNAL( clicked() ), this, SLOT( buttonPathOut() ) );
    
    //+++ session
    connect( comboBoxStructureASCII1D, SIGNAL( activated(int) ), this, SLOT( readCurrentASCII1D() ) );
    connect( pushButtonNewASCII1D, SIGNAL( clicked() ), this, SLOT( saveNewASCII1D() ) );
    connect( pushButtonDeleteASCII1D, SIGNAL( clicked() ), this, SLOT( deleteASCII1D()) );
    connect( pushButtonSaveCurrentASCII1D, SIGNAL( clicked() ), this, SLOT( saveCurrentASCII1D() ) );
    
    //+++ reso
    connect( checkBoxLenses, SIGNAL( stateChanged(int) ), this, SLOT( checkBoxLensesChanged() ) );
    connect( lineEditResoSamAper, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoSamAper2, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoPixelSize, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoLambda, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoFocus, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoDet, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoDeltaLambda, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoColAper, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoColAper2, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( lineEditResoCol, SIGNAL( editingFinished() ), this, SLOT( detColDistanceValidator() ) );
    connect( checkBoxReso, SIGNAL( stateChanged(int) ), this, SLOT( checkBoxResoSlot() ) );
    
    //+++ modify
    connect( comboBoxSelectPresentationFrom, SIGNAL( activated(int) ), this, SLOT( presentationFromChanged() ) );
    connect( comboBoxSelectPresentationTo, SIGNAL( activated(int) ), this, SLOT( presentationToChanged() ) );
    
    connect( checkBoxMath, SIGNAL( clicked() ), this, SLOT( mathControl() ) );
    connect( checkBoxMath2, SIGNAL( clicked() ), this, SLOT( mathControl2() ) );
    connect( checkBoxMath, SIGNAL( clicked() ), this, SLOT( slotMathYN() ) );
    
    connect( checkBoxRemoveRange, SIGNAL( clicked() ), this, SLOT( removeRangeControl() ) );
    connect( checkBoxRemoveFirst, SIGNAL( clicked() ), this, SLOT( removeFirstControl() ) );
    connect( checkBoxRemoveLast, SIGNAL( clicked() ), this, SLOT( removeLastControl() ) );
    
    connect( checkBox1DCalculator, SIGNAL( clicked() ), this, SLOT( slot1DcalcYN() ) );

    //+++ tools
    connect( lineEditFastPlot, SIGNAL( returnPressed()), this, SLOT( filterChangedFastPlotting() ) );
    connect( comboBoxDestination, SIGNAL( activated(int) ), this, SLOT( filterChangedFastPlotting() ) );
    connect( comboBoxFastPlot, SIGNAL( activated(int) ), this, SLOT( fastPlot() ) );
}

ascii1d18::~ascii1d18()
{
    writeSettings();
}

// *******************************************
// +++ app
// *******************************************
ApplicationWindow* ascii1d18::app()
{
    if (this->parentWidget()->parent())
    {
        QDockWidget *docWin=(QDockWidget*)this->parentWidget();
        return (ApplicationWindow *)docWin->parent();
    }
    return (ApplicationWindow *)this->parent();
}

// *******************************************
// *** Log-output
// *******************************************
void ascii1d18::toResLog(QString text)
{
    if (text=="") return;
    QString info =text;
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else app()->showResults(text, true);
}


// *******************************************
// *** readSettings()
// *******************************************
void ascii1d18::readSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif

    //+++
    QString ss;
    //+++
    settings.beginGroup("/ASCII1D");
    //+++
    bool ok = settings.contains("/lineEditPath");
    ss = settings.value("/lineEditPath", 0).toString();
    if (ok && ss!="home") lineEditPath->setText(ss);
    //+++
    ok = settings.contains("/lineEditPathOut");
    ss = settings.value("/lineEditPathOut", 0).toString();
    if (ok && ss!="home") lineEditPathOut->setText(ss);
    //+++
    settings.endGroup();
}
// *******************************************
// *** readSettings()
// *******************************************
void ascii1d18::writeSettings()
{
#ifdef Q_OS_MACOS
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "JCNS", "QtiSAS");
#endif

    //+++
    settings.beginGroup("/ASCII1D");
    //+++
    if (lineEditPath->text()!="home")
        settings.setValue("/lineEditPath",  lineEditPath->text());
    if (lineEditPathOut->text()!="home")
        settings.setValue("/lineEditPathOut",  lineEditPathOut->text());
    //+++
    settings.endGroup();
}

// *******************************************
// *** buttonPath()
// *******************************************
void ascii1d18::buttonPath()
{
    QString path=lineEditPath->text();
    
    if (path=="home") path = QDir::home().path();
    
    QString s = QFileDialog::getExistingDirectory(this, "get Input Folder - Choose a directory", path);
    if (s!="") lineEditPath->setText(s);
}

// *******************************************
// *** buttonPathOut()
// *******************************************
void ascii1d18::buttonPathOut()
{
    QString path=lineEditPathOut->text();
    
    if (path=="home") path = QDir::home().path();
    
    QString s = QFileDialog::getExistingDirectory(this, "get Output Folder - Choose a directory", path);
    if (s!="") lineEditPathOut->setText(s);
}

// *******************************************
// *** sourceSelected()
// *******************************************
void ascii1d18::sourceSelected()
{
    filterChangedFastPlotting();
    
    if (comboBoxSource->currentIndex() == 0) {
        groupFormat->show();
        //lineinput->show();
    }
    else
    {
        groupFormat->hide();
        //lineinput->hide();
    }
}

// *******************************************
// *** asciiFormatSelected()
// *******************************************
void ascii1d18::asciiFormatSelected()
{
    switch (comboBoxFormat->currentIndex()) {
        case 0: lineEditSkipHeader->hide(); // ASCII
            lineEditSkipHeader->setEnabled(false);
            lineEditSkipHeader->setText("0");
            textLabelHeaderLineInfo->hide();
            break;
        case 1: lineEditSkipHeader->show();// ASCII-HEADER
            lineEditSkipHeader->setEnabled(true);
            lineEditSkipHeader->setText("0");
            textLabelHeaderLineInfo->show();
            textLabelHeaderLineInfo->setText("[Header Length]");
            break;
        case 2: lineEditSkipHeader->hide();// cansas
            lineEditSkipHeader->setEnabled(false);
            lineEditSkipHeader->setText("0");
            textLabelHeaderLineInfo->hide();
            break;
        case 3: lineEditSkipHeader->show();//nist
            lineEditSkipHeader->setEnabled(false);
            lineEditSkipHeader->setText("{The 6 columns are}");
            textLabelHeaderLineInfo->show();
            textLabelHeaderLineInfo->setText("[Last Line of the Header]");
            break;
    }
    
    
}

// *******************************************
// *** checkBoxLensesChanged()
// *******************************************
void ascii1d18::checkBoxLensesChanged()
{
    
    if (checkBoxLenses->isChecked())
    {
        textLabelResoFocus->show();
        lineEditResoFocus->show();
        textLabelResoFocus2->show();
    }
    else
    {
        textLabelResoFocus->hide();
        lineEditResoFocus->hide();
        textLabelResoFocus2->hide();
    }
    
}

// *******************************************
// *** mathControl()
// *******************************************
void ascii1d18::mathControl()
{
    mathControl2();
    if (!checkBoxMath->isChecked())
    {
        checkBoxMath2->setChecked(false);
        checkBoxMath2->setEnabled(false);
        lineEditMath2->setEnabled(false);
        comboBoxMath2->setEnabled(false);
        comboBoxMath2->setCurrentIndex(2);
        lineEditMath2->setText("1");
        
        comboBoxMath->setEnabled(false);
        lineEditMath->setEnabled(false);
        comboBoxMath->setCurrentIndex(2);
        lineEditMath->setText("1");
        
        textLabelMath->setText("I(Q)");
    }
    else
    {
        checkBoxMath2->setEnabled(true);
        
        comboBoxMath->setEnabled(true);
        lineEditMath->setEnabled(true);
        
        textLabelMath->setText("I<sub>1</sub> ( Q )");
    }
    
}

// *******************************************
// *** mathControl2()
// *******************************************
void ascii1d18::mathControl2()
{
    
    if (checkBoxMath->isChecked())
    {
        if (!checkBoxMath2->isChecked())
        {
            comboBoxMath2->setEnabled(false);
            lineEditMath2->setText("1");
            lineEditMath2->setEnabled(false);
            
            textLabelMath->setText("I<sub>1</sub> ( Q )");
            
        }
        else
        {
            
            comboBoxMath2->setEnabled(true);
            lineEditMath2->setEnabled(true);
            
            textLabelMath->setText("I<sub>2</sub> ( Q )");
        }
    }
}

// *******************************************
// *** removeRangeControl()
// *******************************************
void ascii1d18::removeRangeControl()
{
    if (checkBoxRemoveRange->isChecked())
    {
        lineEditRemoveFrom->setEnabled(true);
        lineEditRemoveTo->setEnabled(true);
    }
    else
    {
        lineEditRemoveFrom->setEnabled(false);
        lineEditRemoveTo->setEnabled(false);
        lineEditRemoveFrom->setText("0");
        lineEditRemoveTo->setText("0");
    }
}


// *******************************************
// *** removeLastControl()
// *******************************************
void ascii1d18::removeLastControl()
{
    if (checkBoxRemoveLast->isChecked())
    {
        lineEditRemoveLast->setEnabled(true);
    }
    else
    {
        lineEditRemoveLast->setEnabled(false);
        lineEditRemoveLast->setText("0");
    }
}

// *******************************************
// *** removeFirstControl()
// *******************************************
void ascii1d18::removeFirstControl()
{
    if (checkBoxRemoveFirst->isChecked())
    {
        lineEditRemoveFirst->setEnabled(true);
    }
    else
    {
        lineEditRemoveFirst->setEnabled(false);
        lineEditRemoveFirst->setText("0");
    }
}

// *******************************************
// *** slot1DcalcYN()
// *******************************************
void ascii1d18::slot1DcalcYN()
{
    QString currentItem=comboBox1DclacTables->currentText();
    QStringList yTables=app()->columnsList(Table::Y);
    comboBox1DclacTables->clear();
    comboBox1DclacTables->addItems(yTables);
    if (yTables.indexOf(currentItem) >= 0)
        comboBox1DclacTables->setItemText(comboBox1DclacTables->currentIndex(), currentItem);
    
    if (checkBox1DCalculator->isChecked())
    {
        comboBoxMath1D->setEnabled(true);
        comboBox1DclacTables->setEnabled(true);
    }
    else
    {
        comboBoxMath1D->setEnabled(false);
        comboBox1DclacTables->setEnabled(false);
    }
}

// *******************************************
// *** defaultASCII1D()
// *******************************************
void ascii1d18::defaultASCII1D()
{
    comboBoxSource->setCurrentIndex(0);
    comboBoxDestination->setCurrentIndex(0);
    spinBoxForceNumCols->setValue(4);
    comboBoxFormat->setCurrentIndex(1);
    lineEditSkipHeader->setText("0");
    spinBoxFindIndex->setCurrentIndex(0);
    comboBoxPrefixASCII1D->setCurrentIndex(1);
    lineEditExt->setText("RAD");
    
    checkBoxIndexing->setChecked(false);
    checkBoxPlotInActive->setChecked(false);
    checkBoxActions->setChecked(false);
    checkBoxReso->setChecked(false);
    
    // reso tab
    radioButtonSANS->setChecked(true);
    radioButtonSAXS->setChecked(false);
    
    lineEditResoCol->setText("2000");
    lineEditResoDet->setText("2000");
    lineEditResoPixelSize->setText("0.525");
    lineEditResoSamAper->setText("10");
    lineEditResoSamAper2->setText("10");
    lineEditResoColAper->setText("30");
    lineEditResoColAper2->setText("30");
    lineEditResoDeltaLambda->setText("0.2");
    lineEditResoFocus->setText("1000");
    checkBoxLenses->setChecked(false);
    
    // convert tab
    checkBoxConvert->setChecked(false);
    comboBoxSelectPresentationFrom->setCurrentIndex(0);
    comboBoxSelectPresentationTo->setCurrentIndex(0);
    // tab bad points
    checkBoxRemoveRange->setChecked(false);
    lineEditRemoveFrom->setText("0");
    lineEditRemoveTo->setText("0");
    checkBoxRemoveFirst->setChecked(false);
    lineEditRemoveFirst->setText("0");
    checkBoxRemoveLast->setChecked(false);
    lineEditRemoveLast->setText("0");
    checkBoxNoNegative->setChecked(false);
    // tab math
    checkBoxMath->setChecked(false);
    comboBoxMath->setCurrentIndex(2);
    lineEditMath->setText("1");
    checkBoxMath2->setChecked(false);
    comboBoxMath2->setCurrentIndex(2);
    lineEditMath2->setText("1");
    // Q-Binning
    checkBoxBinningLinear->setChecked(false);
    spinBoxBinningLinear->setValue(1);
    checkBoxBinningProgressive->setChecked(false);
    lineEditBinningProgressive->setValue(1.000);
    
    
    //textLabelSASdesc->setHidden(checkBoxConvert->isChecked());
    //frameSASpresentation->setShown(checkBoxConvert->isChecked());
    mathControl();
    mathControl2();
    removeRangeControl();
    removeFirstControl();
    removeLastControl();
    //textLabelHelp->setHidden(checkBoxActions->isChecked());
    //toolBoxChange->setShown(checkBoxActions->isChecked());
    //mCalcSelected1();
    //mCalcSelected2();
    
    
    if (comboBoxStructureASCII1D->currentIndex()==0)
    {
        pushButtonSaveCurrentASCII1D->setEnabled(false);
        pushButtonDeleteASCII1D->setEnabled(false);
    }
    else
    {
        pushButtonSaveCurrentASCII1D->setEnabled(true);
        pushButtonDeleteASCII1D->setEnabled(true);
    }
    
    checkBox1DCalculator->setChecked(false);
    
    checkBoxFastPlot->setChecked(true);
    checkBoxShowTableNameInGraph->setChecked(true);
    lineEditFastPlot->setText("Q*");
    checkBoxXerror->setChecked(false);
    checkBoxYerror->setChecked(true);
}

// *******************************************
// *** findASCII1DFormats()
// *******************************************
void ascii1d18::findASCII1DFormats()
{
    //+++
    QDir dd;
    QString asciiPath=app()->sasPath+"/ascii1dFormats";
    asciiPath=asciiPath.replace("//","/");
    if (!dd.cd(asciiPath))
    {
        asciiPath=QDir::homePath()+"/ascii1dFormats";
        asciiPath=asciiPath.replace("//","/");
        
        if (!dd.cd(asciiPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/ascii1dFormats");
            dd.cd("./qtiSAS/ascii1dFormats");
        }
    };
    asciiPath=dd.absolutePath();
    
    QStringList lst = dd.entryList(QStringList() << "*.ASCII1D");
    lst.replaceInStrings(".ASCII1D", "");
    lst.prepend("default");
    
    QString ct=comboBoxStructureASCII1D->currentText();
    
    comboBoxStructureASCII1D->clear();
    comboBoxStructureASCII1D->addItems(lst);
    if (lst.contains(ct))
        comboBoxStructureASCII1D->setItemText(comboBoxStructureASCII1D->currentIndex(), ct);
}

// *******************************************
// *** readCurrentASCII1D()
// *******************************************
void ascii1d18::readCurrentASCII1D()
{
    defaultASCII1D();
    
    if (comboBoxStructureASCII1D->currentIndex()==0) return;
    
    QString fileName=comboBoxStructureASCII1D->currentText();
    
    //+++
    QDir dd;
    QString asciiPath=app()->sasPath+"/ascii1dFormats";
    asciiPath=asciiPath.replace("//","/");
    if (!dd.cd(asciiPath))
    {
        asciiPath=QDir::homePath()+"/ascii1dFormats";
        asciiPath=asciiPath.replace("//","/");
        
        if (!dd.cd(asciiPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/ascii1dFormats");
            dd.cd("./qtiSAS/ascii1dFormats");
        }
    };
    asciiPath=dd.absolutePath();
    
    QFile f(asciiPath+"/"+fileName+".ASCII1D");
    
    
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        //*************************************Log Window Output
        QMessageBox::warning(this,"Could not read file", tr("QtiSAS::ASCII.SANS.1D"));
        //*************************************Log Window Output
        return;
    }
    
    
    QTextStream t( &f );
    
    QRegExp rx("((\\-)?\\d+)");
    int pos, indexCurrent;
    
    QString s;
    
    
    while(!t.atEnd())
    {
        s=t.readLine().trimmed();
        
        if (s.contains("{Data-Source-From} Table")) comboBoxSource->setCurrentIndex(1);
        if (s.contains("{Data-Source-From} File")) comboBoxSource->setCurrentIndex(0);
        if (s.contains("{Data-Destination-To} Table")) comboBoxSource->setCurrentIndex(0);
        if (s.contains("{Data-Destination-To} File")) comboBoxSource->setCurrentIndex(1);
        if (s.contains("{Data-Number-Columns} ")) spinBoxForceNumCols->setValue(s.remove("Data-Number-Columns} ").trimmed().toInt());
        if (s.contains("{Data-Format} ")) comboBoxFormat->setCurrentIndex(s.remove("{Data-Format} ").trimmed().toInt());
        if (s.contains("{Data-Skip-Header} ")) lineEditSkipHeader->setText(s.remove("{Data-Skip-Header} "));
        if (s.contains("{Name-Structure} ")) spinBoxFindIndex->setCurrentIndex(s.remove("{Name-Structure} ").trimmed().toInt());
        if (s.contains("{Name-Prefix} ")) comboBoxPrefixASCII1D->setCurrentIndex(s.remove("{Name-Prefix} ").trimmed().toInt());
        if (s.contains("{File-Extension} ")) lineEditExt->setText(s.remove("{File-Extension} "));
        if (s.contains("{Indexing-Yes-No} Yes"))
            checkBoxIndexing->setChecked(true);
        if (s.contains("{Indexing-Yes-No} No"))
            checkBoxIndexing->setChecked(false);
        if (s.contains("{Plot-Yes-No} Yes"))
            checkBoxPlotInActive->setChecked(true);
        if (s.contains("{Plot-Yes-No} No"))
            checkBoxPlotInActive->setChecked(false);
        if (s.contains("{Modify-Yes-No} Yes"))
            checkBoxActions->setChecked(true);
        if (s.contains("{Modify-Yes-No} No"))
            checkBoxActions->setChecked(false);
        // reso tab
        if (s.contains("{Reso-Yes-No} Yes"))
            checkBoxReso->setChecked(true);
        if (s.contains("{Reso-Yes-No} No"))
            checkBoxReso->setChecked(false);
        // reso sans/saxs
        if (s.contains("{Reso-SANS-Yes-No} "))
        {
            if (s.contains(" Yes"))
            {
                radioButtonSANS->setChecked(true);
                radioButtonSAXS->setChecked(false);
            }
            else
            {
                radioButtonSANS->setChecked(false);
                radioButtonSAXS->setChecked(true);
            }
        }
        if (s.contains("{Reso-Colimation} ")) lineEditResoCol->setText(s.remove("{Reso-Colimation} "));
        if (s.contains("{Reso-SDD} ")) lineEditResoDet->setText(s.remove("{Reso-SDD} "));
        if (s.contains("{Reso-PixelSize} ")) lineEditResoPixelSize->setText(s.remove("{Reso-PixelSize} "));
        if (s.contains("{Reso-Sample-Aperture-W} ")) lineEditResoSamAper->setText(s.remove("{Reso-Sample-Aperture-W} "));
        if (s.contains("{Reso-Sample-Aperture-H} ")) lineEditResoSamAper2->setText(s.remove("{Reso-Sample-Aperture-H} "));
        if (s.contains("{Reso-Sample-Source-W} ")) lineEditResoColAper->setText(s.remove("{Reso-Sample-Source-W} "));
        if (s.contains("{Reso-Sample-Source-H} ")) lineEditResoColAper2->setText(s.remove("{Reso-Sample-Source-H} "));
        if (s.contains("{Reso-Delta-Lambda} ")) lineEditResoDeltaLambda->setText(s.remove("{Reso-Delta-Lambda} "));
        if (s.contains("{Reso-Focus} ")) lineEditResoFocus->setText(s.remove("{Reso-Focus} "));
        if (s.contains("{Reso-Lens-Yes-No} Yes"))
            checkBoxLenses->setChecked(true);
        if (s.contains("{Reso-Lens-Yes-No} No"))
            checkBoxLenses->setChecked(false);
        // convert tab
        if (s.contains("{Convert-Yes-No} Yes"))
            checkBoxConvert->setChecked(true);
        if (s.contains("Convert-Yes-No} No"))
            checkBoxConvert->setChecked(false);
        if (s.contains("{Convert-From} "))  comboBoxSelectPresentationFrom->setCurrentIndex(s.remove("{Convert-From} ").trimmed().toInt());
        if (s.contains("{Convert-To} "))    comboBoxSelectPresentationTo->setCurrentIndex(s.remove("{Convert-To} ").trimmed().toInt());
        // tab bad points
        if (s.contains("{Remove-Range-Yes-No} Yes"))
            checkBoxRemoveRange->setChecked(true);
        if (s.contains("{Remove-Range-Yes-No} No"))
            checkBoxRemoveRange->setChecked(false);
        if (s.contains("{Remove-Range-From} ")) lineEditRemoveFrom->setText(s.remove("{Remove-Range-From} "));
        if (s.contains("{Remove-Range-To} "))   lineEditRemoveTo->setText(s.remove("{Remove-Range-To} "));
        if (s.contains("{Remove-First-Yes-No} Yes"))
            checkBoxRemoveFirst->setChecked(true);
        if (s.contains("{Remove-First-Yes-No} No"))
            checkBoxRemoveFirst->setChecked(false);
        if (s.contains("{Remove-First} ")) lineEditRemoveFirst->setText(s.remove("{Remove-First} "));
        if (s.contains("{Remove-Last-Yes-No} Yes"))
            checkBoxRemoveLast->setChecked(true);
        if (s.contains("{Remove-Last-Yes-No} No"))
            checkBoxRemoveLast->setChecked(false);
        if (s.contains("{Remove-Last} ")) lineEditRemoveLast->setText(s.remove("{Remove-Last} "));
        if (s.contains("{Remove-Negative-Yes-No} Yes"))
            checkBoxNoNegative->setChecked(true);
        if (s.contains("{Remove-Negative-Yes-No} No"))
            checkBoxNoNegative->setChecked(false);
        // tab math
        if (s.contains("{Math1-Yes-No} Yes"))
            checkBoxMath->setChecked(true);
        if (s.contains("{Math1-Yes-No} No"))
            checkBoxMath->setChecked(false);
        if (s.contains("{Math1-Action} ")) comboBoxMath->setCurrentIndex(s.remove("{Math1-Action} ").trimmed().toInt());
        if (s.contains("{Math1-Factor} ")) lineEditMath->setText(s.remove("{Math1-Factor} "));
        if (s.contains("{Math2-Yes-No} Yes"))
            checkBoxMath2->setChecked(true);
        if (s.contains("{Math2-Yes-No} No"))
            checkBoxMath2->setChecked(false);
        if (s.contains("{Math2-Action} ")) comboBoxMath2->setCurrentIndex(s.remove("{Math2-Action} ").trimmed().toInt());
        if (s.contains("{Math2-Factor} ")) lineEditMath2->setText(s.remove("{Math2-Factor} "));
        // Q-Binning
        if (s.contains("{Binning-Linear-Yes-No} Yes"))
            checkBoxBinningLinear->setChecked(true);
        if (s.contains("{Binning-Linear-Yes-No} No"))
            checkBoxBinningLinear->setChecked(false);
        if (s.contains("{Binning-Linear-Value} ")) spinBoxBinningLinear->setValue(s.remove("{Binning-Linear-Value} ").trimmed().toInt());
        if (s.contains("{Binning-Progressive-Yes-No} Yes"))
            checkBoxBinningProgressive->setChecked(true);
        if (s.contains("{Binning-Progressive-Yes-No} No"))
            checkBoxBinningProgressive->setChecked(false);
        if (s.contains("{Binning-Progressive-Value} ")) lineEditBinningProgressive->setValue(s.remove("{Binning-Progressive-Value} ").trimmed().toDouble());
        
        
        if (s.contains("{FastPlot-TableActive-Yes-No} "))
        {
            if (s.contains(" Yes")) checkBoxFastPlot->setChecked(true);
            else checkBoxFastPlot->setChecked(false);
        }
        if (s.contains("{FastPlot-ShowName-Yes-No} "))
        {
            if (s.contains(" Yes")) checkBoxShowTableNameInGraph->setChecked(true);
            else checkBoxShowTableNameInGraph->setChecked(false);
        }
        if (s.contains("{Plot-xError-Yes-No} "))
        {
            if (s.contains(" Yes")) checkBoxXerror->setChecked(true);
            else checkBoxXerror->setChecked(false);
        }
        if (s.contains("{Plot-yError-Yes-No} "))
        {
            if (s.contains(" Yes")) checkBoxYerror->setChecked(true);
            else checkBoxYerror->setChecked(false);
        }
        if (s.contains("{FastPlot-Filter} ")) lineEditFastPlot->setText(s.remove("{FastPlot-Filter} "));
        
        continue;
    }
    
    
    f.close();
    findASCII1DFormats();
    
    
    //textLabelSASdesc->setHidden(checkBoxConvert->isChecked());
    //frameSASpresentation->setShown(checkBoxConvert->isChecked());
    mathControl();
    mathControl2();
    removeRangeControl();
    removeFirstControl();
    removeLastControl();
    //textLabelHelp->setHidden(checkBoxActions->isChecked());
    //toolBoxChange->setShown(checkBoxActions->isChecked());
    //mCalcSelected1();
    //mCalcSelected2();
    
    return;
}

// *******************************************
// *** saveNewASCII1D()
// *******************************************
void ascii1d18::saveNewASCII1D()
{
    QString newTableName="newformat";
    
    bool ok;
    newTableName = QInputDialog::getText(this,
                                         "New Structure of ASCII.SANS.1D", "Enter name of new Format:",
                                         QLineEdit::Normal,
                                         newTableName, &ok);
    
    if ( !ok ||  newTableName.isEmpty() ) return;
    
    saveCurrentASCII1D(newTableName);
    
    return;
}

// *******************************************
// *** saveCurrentASCII1D()
// *******************************************
void ascii1d18::saveCurrentASCII1D()
{
    if (comboBoxStructureASCII1D->currentIndex()==0) return;
    else saveCurrentASCII1D(comboBoxStructureASCII1D->currentText());
    readCurrentASCII1D();
    return;
}

// *******************************************
// *** saveCurrentASCII1D(QString fileName)
// *******************************************
void ascii1d18::saveCurrentASCII1D(QString fileName)
{
    //+++
    QDir dd;
    QString asciiPath=app()->sasPath+"/ascii1dFormats";
    asciiPath=asciiPath.replace("//","/");
    if (!dd.cd(asciiPath))
    {
        asciiPath=QDir::homePath()+"/ascii1dFormats";
        asciiPath=asciiPath.replace("//","/");
        
        if (!dd.cd(asciiPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/ascii1dFormats");
            dd.cd("./qtiSAS/ascii1dFormats");
        }
    };
    asciiPath=dd.absolutePath();
    
    QFile f(asciiPath+"/"+fileName+".ASCII1D");
    
    
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        //*************************************Log Window Output
        QMessageBox::warning(this,"Could not write to file", tr("QtiSAS::ASCII.SANS.1D"));
        //*************************************Log Window Output
        return;
    }
    QTextStream stream( &f );
    
    stream<<"{Data-Source-From} "   <<comboBoxSource->currentText()         <<"\n";
    stream<<"{Data-Destination-To} "<<comboBoxDestination->currentText()    <<"\n";
    stream<<"{Data-Number-Columns} "<<spinBoxForceNumCols->value()          <<"\n";
    stream<<"{Data-Format} "        <<comboBoxFormat->currentIndex()         <<"\n";
    stream<<"{Data-Skip-Header} "   <<lineEditSkipHeader->text()            <<"\n";
    stream<<"{Name-Structure} "     <<spinBoxFindIndex->currentIndex()       <<"\n";
    stream<<"{Name-Prefix} "        <<comboBoxPrefixASCII1D->currentIndex()  <<"\n";
    stream<<"{File-Extension} "     <<lineEditExt->text()                   <<"\n";
    
    if (checkBoxIndexing->isChecked())
        stream<<"{Indexing-Yes-No} Yes\n";
    else
        stream<<"{Indexing-Yes-No} No\n";
    if (checkBoxPlotInActive->isChecked())
        stream<<"{Plot-Yes-No} Yes\n";
    else
        stream<<"{Plot-Yes-No} No\n";
    if (checkBoxActions->isChecked())
        stream<<"{Modify-Yes-No} Yes\n";
    else
        stream<<"{Modify-Yes-No} No\n";
    // reso tab
    if (checkBoxReso->isChecked())
        stream<<"{Reso-Yes-No} Yes\n";
    else
        stream<<"{Reso-Yes-No} No\n";
    
    if (radioButtonSANS->isChecked())
        stream<<"{Reso-SANS-Yes-No} Yes\n";
    else
        stream<<"{Reso-SANS-Yes-No} No\n";
    
    stream<<"{Reso-Colimation} "            <<lineEditResoCol->text()           <<"\n";
    stream<<"{Reso-SDD} "                   <<lineEditResoDet->text()           <<"\n";
    stream<<"{Reso-PixelSize} "             <<lineEditResoPixelSize->text()     <<"\n";
    stream<<"{Reso-Sample-Aperture-W} "     <<lineEditResoSamAper->text()       <<"\n";
    stream<<"{Reso-Sample-Aperture-H} "     <<lineEditResoSamAper2->text()      <<"\n";
    stream<<"{Reso-Sample-Source-W} "       <<lineEditResoColAper->text()       <<"\n";
    stream<<"{Reso-Sample-Source-H} "       <<lineEditResoColAper2->text()      <<"\n";
    stream<<"{Reso-Delta-Lambda} "          <<lineEditResoDeltaLambda->text()   <<"\n";
    stream<<"{Reso-Focus} "                 <<lineEditResoFocus->text()         <<"\n";
    if (checkBoxLenses->isChecked())
        stream<<"{Reso-Lens-Yes-No} Yes\n";
    else
        stream<<"{Reso-Lens-Yes-No} No\n";
    // convert tab
    if (checkBoxConvert->isChecked())
        stream<<"{Convert-Yes-No} Yes\n";
    else
        stream<<"{Convert-Yes-No} No\n";
    stream<<"{Convert-From} "       <<comboBoxSelectPresentationFrom->currentIndex()  <<"\n";
    stream<<"{Convert-To} "         <<comboBoxSelectPresentationTo->currentIndex()  <<"\n";
    // tab bad points
    if (checkBoxRemoveRange->isChecked())
        stream<<"{Remove-Range-Yes-No} Yes\n";
    else
        stream<<"{Remove-Range-Yes-No} No\n";
    stream<<"{Remove-Range-From} "     <<lineEditRemoveFrom->text()         <<"\n";
    stream<<"{Remove-Range-To} "       <<lineEditRemoveTo->text()         <<"\n";
    if (checkBoxRemoveFirst->isChecked())
        stream<<"{Remove-First-Yes-No} Yes\n";
    else
        stream<<"{Remove-First-Yes-No} No\n";
    stream<<"{Remove-First} "     <<lineEditRemoveFirst->text()         <<"\n";
    if (checkBoxRemoveLast->isChecked())
        stream<<"{Remove-Last-Yes-No} Yes\n";
    else
        stream<<"{Remove-Last-Yes-No} No\n";
    stream<<"{Remove-Last} "     <<lineEditRemoveLast->text()         <<"\n";
    if (checkBoxNoNegative->isChecked())
        stream<<"{Remove-Negative-Yes-No} Yes\n";
    else
        stream<<"{Remove-Negative-Yes-No} No\n";
    // tab math
    if (checkBoxMath->isChecked())
        stream<<"{Math1-Yes-No} Yes\n";
    else
        stream<<"{Math1-Yes-No} No\n";
    stream<<"{Math1-Action} "  <<comboBoxMath->currentIndex()  <<"\n";
    stream<<"{Math1-Factor} "  <<lineEditMath->text()         <<"\n";
    if (checkBoxMath2->isChecked())
        stream<<"{Math2-Yes-No} Yes\n";
    else
        stream<<"{Math2-Yes-No} No\n";
    stream<<"{Math2-Action} "  <<comboBoxMath2->currentIndex()  <<"\n";
    stream<<"{Math2-Factor} "  <<lineEditMath2->text()         <<"\n";
    // Q-Binning
    if (checkBoxBinningLinear->isChecked())
        stream<<"{Binning-Linear-Yes-No} Yes\n";
    else
        stream<<"{Binning-Linear-Yes-No} No\n";
    stream<<"{Binning-Linear-Value} "     <<spinBoxBinningLinear->value()       <<"\n";
    if (checkBoxBinningProgressive->isChecked())
        stream<<"{Binning-Progressive-Yes-No} Yes\n";
    else
        stream<<"{Binning-Progressive-Yes-No} No\n";
    stream<<"{Binning-Progressive-Value} "     <<lineEditBinningProgressive->text()       <<"\n";
    
    
    if (checkBoxFastPlot->isChecked())
        stream<<"{FastPlot-TableActive-Yes-No} Yes\n";
    else
        stream<<"{FastPlot-TableActive-Yes-No} No\n";
    if (checkBoxShowTableNameInGraph->isChecked())
        stream<<"{FastPlot-ShowName-Yes-No} Yes\n";
    else
        stream<<"{FastPlot-ShowName-Yes-No} No\n";
    if (checkBoxXerror->isChecked())
        stream<<"{Plot-xError-Yes-No} Yes\n";
    else
        stream<<"{Plot-xError-Yes-No} No\n";
    if (checkBoxYerror->isChecked())
        stream<<"{Plot-yError-Yes-No} Yes\n";
    else
        stream<<"{Plot-yError-Yes-No} No\n";
    
    stream<<"{FastPlot-Filter} "     <<lineEditFastPlot->text()       <<"\n";

    
    f.close();
    findASCII1DFormats();
    
    comboBoxStructureASCII1D->setItemText(comboBoxStructureASCII1D->currentIndex(), fileName);
    
    return;
}

// *******************************************
// *** deleteASCII1D()
// *******************************************
void ascii1d18::deleteASCII1D()
{
    if (comboBoxStructureASCII1D->currentIndex()==0)
    {
        return;
    }
    
    QString fileName=comboBoxStructureASCII1D->currentText();
    
    //+++
    QDir dd;
    QString headerPath=app()->sasPath+"/ascii1dFormats";
    headerPath=headerPath.replace("//","/");
    if (!dd.cd(headerPath))
    {
        headerPath=QDir::homePath()+"/ascii1dFormats";
        headerPath=headerPath.replace("//","/");
        
        if (!dd.cd(headerPath))
        {
            dd.cd(QDir::homePath());
            dd.mkdir("./qtiSAS/ascii1dFormats");
            dd.cd("./qtiSAS/ascii1dheaderFormats");
        }
    };
    headerPath=dd.absolutePath();
    
    dd.remove(fileName+".ASCII1D");
    
    findASCII1DFormats();
    defaultASCII1D();
    readCurrentASCII1D();
}


//*******************************************
//*** filterChangedFastPlotting
//*******************************************
void ascii1d18::filterChangedFastPlotting()
{
    
    if(comboBoxDestination->currentIndex()==1)
    {
        buttonGroupFastPlot->hide();
        return;
    } else buttonGroupFastPlot->show();
    
    
    QStringList activeNumberList;
    QString oldItem=comboBoxFastPlot->currentText();
    
    if (comboBoxSource->currentIndex()==0)
    {
        
        
        QDir dir( lineEditPath->text() );
        
        QStringList activeFileList;
        activeFileList= dir.entryList(QStringList() << lineEditFastPlot->text());
        
        for(int i=0;i<activeFileList.count(); i++) activeNumberList << activeFileList[i].remove(lineEditPath->text()).remove("/");
        
    }
    
    if (comboBoxSource->currentIndex()==1)
    {
        QRegExp rx(lineEditFastPlot->text() );
        rx.setPatternSyntax(QRegExp::Wildcard);
        
        QList<MdiSubWindow *> tableList=app()->tableList();
        foreach (MdiSubWindow *t, tableList) if ( rx.exactMatch(t->name())) activeNumberList<<t->name();
    }
    
    comboBoxFastPlot->clear();
    comboBoxFastPlot->insertItems(0, activeNumberList);
    if (oldItem!="" && comboBoxFastPlot->count() > 0)
        comboBoxFastPlot->setCurrentIndex(activeNumberList.indexOf(oldItem));
}

//*******************************************
//*** fastPlot
//*******************************************
void ascii1d18::fastPlot()
{
    if (comboBoxSource->currentIndex()==0)
    {
        QString str=lineEditPath->text()+"/"+comboBoxFastPlot->currentText();
        QStringList lst; lst<<str.replace("//","/");
        loadASCIIfromFiles(lst, true);
    }
    
    if (comboBoxSource->currentIndex()==1)
    {
        QStringList lst; lst<<comboBoxFastPlot->currentText();
        loadASCIIfromTables(lst, true);
    }
}


//*******************************************
//---Proceed:: SANS | 1D
//*******************************************
void ascii1d18::loadASCIIall()
{
    if (comboBoxSource->currentIndex()==0) loadASCIIfromFiles();
    if (comboBoxSource->currentIndex()==1) loadASCIIfromTables();
}

//*******************************************
//*** loadASCIIfromFiles()
//*******************************************
void ascii1d18::loadASCIIfromFiles()
{
    QString Dir=lineEditPath->text();
    QString ext="."+lineEditExt->text(); ext=ext.trimmed().remove(" ");
    if (ext=="" || ext==".") ext=".RAD";
    
    QFileDialog *fd = new QFileDialog(this,"Getting File Information",Dir,"Rawdata (*"+ext+" *)");
    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setWindowTitle(tr("QtiSAS - Reading ASCII.1D files"));
    //fd->setNameFilter(filter+";;"+textEditPattern->text());

    if (fd->exec() == QDialog::Rejected)
        return;
    
    QStringList fileNames=fd->selectedFiles();
    
    if (fileNames.count()==0)     return;
    loadASCIIfromFiles(fileNames, false);
}

//*******************************************
//*** loadASCIIfromFiles(QStringList fileNames, bool fastPlotYN)
//*******************************************
void ascii1d18::loadASCIIfromFiles(QStringList fileNames, bool fastPlotYN)
{
    int files = fileNames.count();
    QString Dir=lineEditPath->text();
    
    int i;
    int N, Nfinal;
    bool sigmaExist;
    
    // active graph
    Graph *g;
    Table* w;
    QStringList listToPlot;
    QString currentName;
    bool plotData=checkBoxPlotInActive->isChecked();
    if (comboBoxDestination->currentIndex()>0) plotData=false;
    if (fastPlotYN ) plotData=true;
    
    if (plotData)
    {
        plotData=app()->findActiveGraph(g);
        bool maximaizedYN=false;
        MdiSubWindow *ww;
        
        if ( plotData )
        {
            ww= ( MdiSubWindow *  ) app()->activeWindow();
            if (ww->status() == MdiSubWindow::Maximized && QString(ww->metaObject()->className()) == "MultiLayer")
            {
                maximaizedYN=true;
                // w->showMinimized();
            }
        }
    }
    
    for (i=0;i<files;i++)
    {
        //loadASCII(fileNames[i]); //+ call  loadASCII
        sigmaExist=false;
        N=linesNumber(fileNames[i]);
        gsl_matrix* data=gsl_matrix_alloc(N,4);
        
        
        if ( loadASCIIfromFile(fileNames[i], data, N, sigmaExist) )
        {
            Nfinal=N;
            if (checkBoxActions->isChecked())
            {
                // remove points
                Nfinal=removePoints(data,N);
                if (Nfinal==0)
                {
                    QMessageBox::warning(this,tr("QtiSAS::ASCII.SANS.1D"),
                                         "File "+fileNames[i]+" contains no data, and is skipped");
                    break;
                }
                // convert from
                convertFromQI(data, N, Nfinal);
                //sigma
                sigmaCalculation(data, N);
                // math 1....
                if (checkBoxMath->isChecked() && checkBoxActions->isChecked())
                    applyMath(data, comboBoxMath->currentText(), lineEditMath->text().toDouble(), N);
                // math 2....
                if (checkBoxMath2->isChecked() && checkBoxActions->isChecked())
                    applyMath(data, comboBoxMath2->currentText(), lineEditMath2->text().toDouble(),N);
                // convert to
                convertToQI(data, N, Nfinal);
                // column calculation
                if (checkBox1DCalculator->isChecked())
                    columnCalculation(data, N,Nfinal,comboBoxMath1D->currentText(), comboBox1DclacTables->currentText() );
                // Linear Binning
                if(checkBoxBinningLinear->isChecked()) linearBinning(data, N, Nfinal);
                // Log Binning
                if(checkBoxBinningProgressive->isChecked()) logBinning(data, N, Nfinal);
            }
            else sigmaCalculation(data, N);
            
            // save ascii as ..
            currentName=fileNames[i];
            if (fastPlotYN && checkBoxFastPlot->isChecked()) currentName="Table-Active";
            
            dataMatrixSave(currentName,data,N,Nfinal, sigmaExist,w );
            
            if (plotData)
            {
                plotTable(g,w, currentName);
                if (fastPlotYN && checkBoxFastPlot->isChecked() && checkBoxShowTableNameInGraph->isChecked()) g->setTitle(comboBoxFastPlot->currentText());
                app()->setAutoScale();
            }
            
        }
        gsl_matrix_free (data);
        
        toResLog(currentName+"\n");
    }
}

//*******************************************
//+++   Load ASCII files
//*******************************************
void ascii1d18::loadASCIIfromTables()
{
    bool ok;
    QString text = QInputDialog::getText(this, "ASCII.SANS.1D", "Input pattern to select tables:", QLineEdit::Normal,
                                         QString(), &ok);
    if ( !ok || text.isEmpty() ) return;

    QRegExp rx(text);
    rx.setPatternSyntax(QRegExp::Wildcard);
    QStringList tableNames;
    
    QList<MdiSubWindow *> tableList=app()->tableList();
    foreach (MdiSubWindow *t, tableList) if ( rx.exactMatch(t->name())) tableNames<<t->name();
    

    if (tableNames.count()==0)
    {
        QMessageBox::warning(this,tr("QtiSAS::ASCII.SANS.1D"),"No table is selected");
        return;
    }
    
    loadASCIIfromTables(tableNames, false);
}





//*******************************************
//+++   Load ASCII files
//*******************************************
void ascii1d18::loadASCIIfromTables(QStringList tableNames, bool fastPlotYN)
{
    bool ok;
    
    int tables=tableNames.count();
    
    
    bool sigmaExist;
    int N,Nfinal;
    
    // active graph
    Graph *g;
    Table* w;
    QString currentName;
    bool plotData=checkBoxPlotInActive->isChecked();
    if (fastPlotYN ) plotData=true;
    
    if (comboBoxDestination->currentIndex()>0) plotData=false;
    
    if (plotData)
    {
        plotData=app()->findActiveGraph(g);
        bool maximaizedYN=false;
        MdiSubWindow *ww;
        
        if ( plotData )
        {
            ww= ( MdiSubWindow *) app()->activeWindow();
            if (ww->status() == MdiSubWindow::Maximized && QString(ww->metaObject()->className()) == "MultiLayer")
            {
                maximaizedYN=true;
                // w->showMinimized();
            }
        }
    }
    
    for (int i=0;i<tables;i++)
    {
        //loadASCII(fileNames[i]); //+ call  loadASCII
        sigmaExist=false;
        N=0;
        if (rowsNumber(tableNames[i])==0) continue;
        gsl_matrix* data=gsl_matrix_alloc(rowsNumber(tableNames[i]),4);
        
        if ( loadASCIIfromTable(tableNames[i], data, N, sigmaExist) )
        {
            if (N==0) continue;
            Nfinal=N;
            if (checkBoxActions->isChecked())
            {
                // remove points
                Nfinal=removePoints(data,N);
                if (Nfinal==0)
                {
                    QMessageBox::warning(this,tr("QtiSAS::ASCII.SANS.1D"),
                                         "File "+tableNames[i]+" contains no data, and is skipped");
                    break;
                }
                // convert from
                convertFromQI(data, N, Nfinal);
                //sigma
                sigmaCalculation(data, N);
                // math 1....
                if (checkBoxMath->isChecked())
                    applyMath(data, comboBoxMath->currentText(), lineEditMath->text().toDouble(), N);
                // math 2....
                if (checkBoxMath2->isChecked())
                    applyMath(data, comboBoxMath2->currentText(), lineEditMath2->text().toDouble(),N);
                // convert to
                convertToQI(data, N, Nfinal);
                // column calculation
                if (checkBox1DCalculator->isChecked()) columnCalculation(data, N,Nfinal,comboBoxMath1D->currentText(), comboBox1DclacTables->currentText() );
                // Linear Binning
                if(checkBoxBinningLinear->isChecked()) linearBinning(data, N, Nfinal);
                // Log Binning
                if(checkBoxBinningProgressive->isChecked()) logBinning(data, N, Nfinal);
                
            }
            else sigmaCalculation(data, N);
            
            // save ascii as ...
            currentName=tableNames[i];
            if (fastPlotYN && checkBoxFastPlot->isChecked()) currentName="Table-Active";
            
            dataMatrixSave(currentName,data,N,Nfinal, sigmaExist,w );
            
            if (plotData)
            {
                plotTable(g,w, currentName);
                if (fastPlotYN && checkBoxFastPlot->isChecked() && checkBoxShowTableNameInGraph->isChecked()    ) g->setTitle(comboBoxFastPlot->currentText());
            }
            
            //
        }
        gsl_matrix_free(data);
    }
}

//*******************************************
//+++   linesNumber
//*******************************************
int ascii1d18::linesNumber(const QString fn)
{
    //scan No1
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) return 0;
    
    QTextStream t( &f );
    int N=0;
    QString s;
    
    QString ss=lineEditSkipHeader->text().trimmed();
    
    if (ss.left(1)=="{" && ss.right(1)=="}")
    {
        ss=ss.mid(1,ss.length()-2);
        while (!t.atEnd() && !s.contains(ss) ) s=t.readLine();
    }
    else
    {
        for (int i=0; i<ss.toInt(); i++) t.readLine();
    }
    
    if (comboBoxFormat->currentText().contains("cansas1d"))
    {
        while (!t.atEnd())
        {
            s=t.readLine();
            
            if (s.contains("<Idata>")) N++;
        }
    }
    else while (!t.atEnd())
    {
        s=t.readLine();
        N++;
    }
    f.close();
    return N;
}

//*******************************************
//+++   rowsNumber
//*******************************************
int ascii1d18::rowsNumber(const QString table)
{
    Table *w;
    if (app()->checkTableExistence(table,w)) return w->numRows();
    else return 0;
}

//*******************************************
//+++   loadASCIIfromFile
//*******************************************
bool ascii1d18::loadASCIIfromFile(const QString fn, gsl_matrix* &data, int &N, bool &sigmaExist)
{
    //scan No1
    int maxN=N;
    N=0;
    
    QFile f(fn);
    
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"File "+fn+" could not be opened, and was skipped", tr("QtiSAS::ASCII.SANS.1D"));
        return false;
    }
    
    QTextStream t( &f );
    int pos,NN;
    double Q,I,dI,sigma;
    QString s;
    
    QRegExp rx("((\\-|\\+)?\\d\\d*(\\.\\d*)?(((e|E)\\-|(e|E)\\+)\\d\\d?\\d?\\d?)?)");
    
    QString ss=lineEditSkipHeader->text().trimmed();
    
    if (ss.left(1)=="{" && ss.right(1)=="}")
    {
        ss=ss.mid(1,ss.length()-2);
        while (!t.atEnd() && !s.contains(ss) ) s=t.readLine();
    }
    else
    {
        for (int i=0; i<ss.toInt(); i++) t.readLine();
    }
    int n;
    if (comboBoxFormat->currentText().contains("cansas1d"))
    {
        while (!t.atEnd() && !s.contains("<SASdata") ) s=t.readLine();
        while (!t.atEnd() && !s.contains("</SASdata>") && N<maxN )
        {
            Q=0;
            I=0;
            dI=-99.99;
            sigma=-99.99;
            n=0;
            while (!t.atEnd() && (!s.contains("</Idata>") || n==0) )
            {
                n++;
                s=t.readLine();
                if (s.contains("<Q ") && s.contains("</Q>"))
                {
                    ss=s.left(s.indexOf("</Q>"));
                    ss=ss.right(ss.length()-ss.indexOf(">",ss.indexOf("<Q "))-1);
                    ss=ss.trimmed();
                    Q=ss.toDouble();
                }
                if (s.contains("<I ") && s.contains("</I>"))
                {
                    ss=s.left(s.indexOf("</I>"));
                    ss=ss.right(ss.length()-ss.indexOf(">",ss.indexOf("<I "))-1);
                    ss=ss.trimmed();
                    I=ss.toDouble();
                }
                if (s.contains("<Idev ") && s.contains("</Idev>"))
                {
                    ss=s.left(s.indexOf("</Idev>"));
                    ss=ss.right(ss.length()-ss.indexOf(">",ss.indexOf("<Idev "))-1);
                    ss=ss.trimmed();
                    dI=ss.toDouble();
                }
                if (s.contains("<Qdev ") && s.contains("</Qdev>"))
                {
                    ss=s.left(s.indexOf("</Qdev>"));
                    ss=ss.right(ss.length()-ss.indexOf(">",ss.indexOf("<Qdev "))-1);
                    ss=ss.trimmed();
                    sigma=ss.toDouble();
                    sigmaExist=true;
                }
            }
            gsl_matrix_set(data,N,0,Q);
            gsl_matrix_set(data,N,1,I);
            gsl_matrix_set(data,N,2,dI);
            gsl_matrix_set(data,N,3,sigma);
            N++;
            if (!s.contains("<Idata>")) s=t.readLine();
        }
    }
    else while (!t.atEnd() && N<maxN)
    {
        Q=0;
        I=0;
        dI=-99.99;
        sigma=-99.99;
        
        s=t.readLine().trimmed();
        
        s.replace(",",".");
        s.replace("e","E");
        s.replace("E","E0");
        s.replace("E0+","E+0");
        s.replace("E0-","E-0");
        s.replace("E0","E+0");
        
        
        pos = 0;
        NN=0;
        while ( pos >= 0 )
        {
            pos = rx.indexIn( s, pos );
            
            if (NN==0) Q=rx.cap( 1 ).toDouble();
            if (NN==1) I=rx.cap( 1 ).toDouble();
            if (NN==2) dI=rx.cap( 1 ).toDouble();
            if (NN==3 && pos > -1 )
            {
                sigma=rx.cap( 1 ).toDouble();
                sigmaExist=true;
            }
            
            if ( pos > -1 )
            {
                pos  += rx.matchedLength();
                NN++;
            }
            if (NN>3) break;
        }
        
        if (NN>=2)
        {
            gsl_matrix_set(data,N,0,Q);
            gsl_matrix_set(data,N,1,I);
            gsl_matrix_set(data,N,2,dI);
            gsl_matrix_set(data,N,3,sigma);
            N++;
        }
    }
    
    if (N==0)
    {
        QMessageBox::warning(this,"File "+fn+" contains no data, and is skipped", tr("QtiSAS::ASCii.SANS.1D"));
        return false;
    }
    
    f.close();
    return true;
}

//*******************************************
//+++   removePoints
//*******************************************
int ascii1d18::removePoints(gsl_matrix* &data, int N)
{
    int i;
    if (checkBoxRemoveRange->isChecked() && checkBoxActions->isChecked())
    {
        //
        int from=lineEditRemoveFrom->text().toInt()-1;
        int to=lineEditRemoveTo->text().toInt()-1;
        
        if (from<0) from=0;
        if (to<0) to=0;
        
        if (to >= N) to=N-1;
        if (to<from) {to=0; from=0;};
        if (to!=from) for(i=from;i<=to;i++) gsl_matrix_set(data, i,0, -99.99);
    }
    //
    if (checkBoxRemoveFirst->isChecked() && checkBoxActions->isChecked())
    {
        int first=lineEditRemoveFirst->text().toInt();
        if (first>=N) return 0;
        for(i=0;i<first; i++) gsl_matrix_set(data, i,0, -99.99);
    }
    //
    if (checkBoxRemoveLast->isChecked() && checkBoxActions->isChecked())
    {
        int last=lineEditRemoveLast->text().toInt();
        if (last>=N) return 0;
        for(i=(N-last);i<N; i++) gsl_matrix_set(data, i,0, -99.99);
    }
    //
    if (checkBoxNoNegative->isChecked())
    {
        for(i=0;i<N; i++) if ( gsl_matrix_get(data, i,1) <=0 ) gsl_matrix_set(data, i,0, -99.99);
    }
    
    //
    int Nfinal=0;
    for(i=0;i<N; i++) if (gsl_matrix_get(data, i,0)!=-99.99) Nfinal++;
    
    //
    return Nfinal;
}

//*******************************************
//+++   convertFromQI
//*******************************************
void ascii1d18::convertFromQI(gsl_matrix* &data, int N, int &Nfinal)
{
    if (!checkBoxConvert->isChecked() || !checkBoxActions->isChecked()) return;
    if (comboBoxSelectPresentationFrom->currentIndex()==0) return;
    //
    QString oldPresentation=comboBoxSelectPresentationFrom->currentText();
    //
    // * ****** Change of Presentation  *****************************
    double QQ, II, dII;
    double Q, I, dI;
    for (int i=0; i<N; i++) if (gsl_matrix_get(data, i,0) != -99.99)
    {
        QQ=gsl_matrix_get(data, i,0);
        II=gsl_matrix_get(data, i,1);
        dII=gsl_matrix_get(data, i,2);
        
        if (oldPresentation=="Guinier")
        {
            if (QQ<0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                I=exp(II);
                if (dII>0) dI=I*dII;
            }
        }
        else if (oldPresentation=="Zimm")
        {
            if (QQ<0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                if (II<=0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=1/II;
                    if (dII>0) dI=dII/II/II;
                }
            }
        }
        else if (oldPresentation=="Porod")
        {
            Q=pow(10,QQ);
            I=pow(10,II);
            
            if (dII>0) dI=dII*I*log(10.0);
        }
        else if (oldPresentation=="Porod2")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II/pow(Q,4);
                    if (dII>0) dI=dII/pow(Q,4);
                }
            }
        }
        else if (oldPresentation=="logQ")
        {
            if (QQ<0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=pow(10,QQ);
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II;
                    if (dII>0) dI=dII;
                }
            }
        }
        else if (oldPresentation=="logI")
        {
            Q=QQ;
            I=pow(10,II);
            if (dII>0) dI=dII*I*log(10.0);
        }
        else if (oldPresentation=="Debye")
        {
            if (QQ<0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                if (II<=0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=1/II/II;
                    if (dII>0) dI=2*dII/II/II/II;
                }
            }
        }
        else if (oldPresentation=="1Moment")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=QQ;
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II/Q;
                    dI=dII/Q;
                }
            }
        }
        else if (oldPresentation=="2Moment")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=QQ;
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II/Q/Q;
                    if (dII>0) dI=dII/Q/Q;
                }
            }
        }
        else if (oldPresentation=="Kratky")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=QQ;
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II/Q/Q;
                    if (dII>0) dI=dII/Q/Q;
                }
            }
        }
        else if (oldPresentation=="GuinierRod")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                I=exp(II)/Q;
                if (dII>0) dI=I*dII/Q;
            }
        }
        else if (oldPresentation=="GuinierPlate")
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=sqrt(QQ);
                I=exp(II)/Q/Q;
                if (dII>0) dI=I*dII/Q/Q;
            }
        }
        else
        {
            if (QQ<=0) {Q=-99.99;Nfinal--;}
            else
            {
                Q=QQ;
                if (II<0) {Q=-99.99;Nfinal--;}
                else
                {
                    I=II;
                    if (dII>0)   dI=dII;
                }
            }
        }
        
        gsl_matrix_set(data, i,0, Q);
        gsl_matrix_set(data, i,1, I);
        gsl_matrix_set(data, i,2, dI);
    }
}

//*******************************************
//*** sigma( double Q)
//*******************************************
double ascii1d18::sigma( double Q)
{
    bool qUnitsAng=true;
    bool lambdaUnitsAng=true;
    
    if (comboBoxQunits->currentIndex()==1) qUnitsAng=false;
    if (comboBoxLambdaUnits->currentIndex()==1) lambdaUnitsAng=false;
    
    const double PI =M_PI;
    double temp;
    
    double C          =lineEditResoCol->text().toDouble();                          //[cm]
    double D          =lineEditResoDet->text().toDouble();                          //[cm]
    double Rdet       =lineEditResoPixelSize->text().toDouble()/sqrt(PI);           //[cm]
    
    temp=lineEditResoSamAper->text().toDouble();
    temp*=lineEditResoSamAper2->text().toDouble();
    
    double r2         =0.1*sqrt(temp/PI);                                           //[cm]
    
    temp=lineEditResoColAper->text().toDouble();
    temp*=lineEditResoColAper2->text().toDouble();
    
    double r1         =0.1*sqrt(temp/PI);                                           //[cm]
    
    double Lambda     =lineEditResoLambda->text().toDouble();                       //[A]
    
    if (qUnitsAng!=lambdaUnitsAng)
    {
        if (qUnitsAng) Lambda*=10.0;
        else Lambda/=10.0;
    }
    
    double SigmaLambda=lineEditResoDeltaLambda->text().toDouble();                  //[1]
    double f            =lineEditResoFocus->text().toDouble();                      //[cm]
    
    
    double SigQ2,SigQ;
    double Theta0,cos2Theta02, a1, a2, beta1;
    
    
    if (checkBoxLenses    ->isChecked())
    {
        SigQ2=PI*PI/Lambda/Lambda/3*(3*r1*r1/C/C+2*r2*r2/f/f*SigmaLambda*SigmaLambda+Rdet*Rdet/D/D+Lambda*Lambda/2/2/PI/PI*Q*Q*SigmaLambda*SigmaLambda);
        SigQ=2*0.4246609*sqrt(SigQ2);
    }
    else
    {
        
        double K0                 = 2.0*PI/Lambda;   //[A-1]
        
        //
        Theta0          = asin(Q/2/K0);
        cos2Theta02     = cos(2*Theta0)*cos(2*Theta0);
        
        //+++ Sigma-D1  +++
        double SigmaDetector      = K0*cos(Theta0)*cos2Theta02*Rdet/D;       //[A-1]
        
        //
        if (cos2Theta02!=0) a1 = r1/(C+D/cos2Theta02); else a1 =0;
        a2 = r2*cos2Theta02/D;
        
        //
        if (a2>a1)
        {
            beta1   =2*r2*(1/C+cos2Theta02/D)-r1*r1*D/(2*r2*cos2Theta02*C*(C+D/cos2Theta02));
        }
        else
        {
            beta1   =2*r1/C-0.5*r2*r2/r1*cos2Theta02*cos2Theta02*(C+D/cos2Theta02)*(C+D/cos2Theta02)/C/D/D;
        }
        
        //+++ Sigma-W  +++
        SigQ2        =Q*Q*SigmaLambda*SigmaLambda;
        //+++ Sigma-D1  +++
        SigQ2       +=SigmaDetector*SigmaDetector;
        //+++ Sigma-C1  +++
        SigQ2       +=K0*K0*cos(Theta0)*cos(Theta0)*beta1*beta1;
        //+++  Sigma-AV  +++
        SigQ2       +=K0*K0*cos(Theta0)*cos(Theta0)*cos2Theta02*cos2Theta02*Rdet/D*Rdet/D;
        //
        SigQ         =0.4246609*sqrt(SigQ2); //[A-1];
    }
    
    return SigQ;
}

//*******************************************
// __________ ASCII  | Table::  sigma...
//*******************************************
void ascii1d18::sigmaCalculation(gsl_matrix* &data, int N)
{
    if (checkBoxReso->isChecked() ) //+++  2021-07 && checkBoxActions->isChecked())
    {
        for (int i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
        {
            gsl_matrix_set(data,i,3,sigma(fabs(gsl_matrix_get(data,i,0))));
        }
    }
}


//*******************************************
// Apply Math...
//*******************************************
void ascii1d18::applyMath(gsl_matrix* &data, QString MathChar, double MathFactor, int N)
{
    double Q,I,dI;
    for (int i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
    {
        Q=gsl_matrix_get(data,i,0);
        I=gsl_matrix_get(data,i,1);
        dI=gsl_matrix_get(data,i,2);
        if (MathChar=="+")
        {
            I+=MathFactor;
        }
        else if (MathChar=="*")
        {
            I*=MathFactor;
            if (dI>0) dI*=MathFactor;
        }
        else if (MathChar=="/")
        {
            if (MathFactor!=0)
            {
                I/=MathFactor;
                if (dI>0) dI/=MathFactor;
            }
        }
        else if (MathChar=="-")
        {
            I-=MathFactor;
        }
        gsl_matrix_set(data,i,0,Q);
        gsl_matrix_set(data,i,1,I);
        gsl_matrix_set(data,i,2,dI);
    }
}

//*******************************************
// convertToQI ...
//*******************************************
void ascii1d18::convertToQI(gsl_matrix* &data, int N, int &Nfinal)
{
    if (!checkBoxConvert->isChecked() || !checkBoxActions->isChecked()) return;
    if (comboBoxSelectPresentationTo->currentIndex()==0) return;
    //
    QString newPresentation=comboBoxSelectPresentationTo->currentText();
    //
    // * ****** Change of Presentation  *****************************
    double QQ, II, dII;
    double Q, I, dI;
    
    for (int i=0; i<N; i++) if (gsl_matrix_get(data, i,0) != -99.99)
    {
        Q=gsl_matrix_get(data, i,0);
        I=gsl_matrix_get(data, i,1);
        dI=gsl_matrix_get(data, i,2);
        
        if (newPresentation=="Guinier")
        {
            QQ=Q*Q;
            if (I<=0) {QQ =-99.99;Nfinal--;}
            else
            {
                II=log(I);
                if (dI>0) dII=dI/I;
            }
        }
        else if (newPresentation=="Zimm")
        {
            QQ=Q*Q;
            
            if (I==0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=1/I;
                if (dI>0) dII=dI/I/I;
            }
        }
        else if (newPresentation=="Porod")
        {
            if (Q<=0) {QQ=-99.99;Nfinal--;}
            else
            {
                QQ=log10(Q);
                if (I<=0) {QQ=-99.99;Nfinal--;}
                else
                {
                    II=log10(I);
                    if (dI>0) dII=dI/I/log(10.0);
                }
            }
        }
        else if (newPresentation=="Porod2")
        {
            QQ=Q*Q;
            if (I<0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=I*pow(Q,4);
                if (dI>0) dII=dI*pow(Q,4);
            }
        }
        else if (newPresentation=="logQ")
        {
            if (Q<=0) {QQ=-99.99;Nfinal--;}
            else
            {
                QQ=log10(Q);
                
                II=I;
                if (dI>0) dII=dI;
            }
        }
        else if (newPresentation=="logI")
        {
            QQ=Q;
            if (I<=0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=log10(I);
                if (dI>0) dII=dI/I/log(10.0);
            }
        }
        else if (newPresentation=="Debye")
        {
            QQ=Q*Q;
            if (I<=0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=1/sqrt(I);
                if (dI>0) dII=dI/pow(I,1.5);
            }
        }
        else if (newPresentation=="1Moment")
        {
            QQ=Q;
            if (I<0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=Q*I;
                if (dI>0) dII=Q*dI;
            }
        }
        else if (newPresentation=="2Moment")
        {
            QQ=Q;
            if (I<0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=Q*Q*I;
                if (dI>0) dII=Q*Q*dI;
            }
        }
        else if (newPresentation=="Kratky")
        {
            QQ=Q;
            if (I<0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=Q*Q*I;
                if (dI>0) dII=Q*Q*dI;
            }
        }
        else if (newPresentation=="GuinierRod")
        {
            QQ=Q*Q;
            if (I<=0 || Q<=0 ) {QQ=-99.99;Nfinal--;}
            else
            {
                II=log(I*Q);
                if (dI>0) dII=Q*(dI/I);
            }
        }
        else if (newPresentation=="GuinierPlate")
        {
            if (Q<0) {QQ=-99.99;Nfinal--;}
            else
            {
                QQ=Q*Q;
                if (I<=0) {QQ=-99.99;Nfinal--;}
                else
                {
                    II=log(I*Q*Q);
                    if (dI>0) dII=Q*Q*(dI/I);
                }
            }
        }
        else
        {
            QQ=Q;
            if (I<0) {QQ=-99.99;Nfinal--;}
            else
            {
                II=I;
                if (dI>0) dII=dI;
            }
        }
        
        gsl_matrix_set(data, i,0, QQ);
        gsl_matrix_set(data, i,1, II);
        gsl_matrix_set(data, i,2, dII);
    }
}

//*******************************************
// columnCalculation ...
//*******************************************
int ascii1d18::columnCalculation(gsl_matrix* &data, int N, int Nfinal, QString action, QString columnName)
{
    int i;
    Table* w;
    QString tableName=columnName.left(columnName.indexOf("_"));
    QString colShortName=columnName.remove(tableName+"_");
    
    if (N>0 && comboBoxSelectPresentationFrom->currentText() == comboBoxSelectPresentationTo->currentText() && columnName!="" && app()->checkTableExistence(tableName,w))
    {
        int colIndex=w->colIndex(colShortName);
        int rowNumber=w->numRows();
        if (Nfinal!=rowNumber) return -1;
        
        double n1, n2;
        int j=0;
        for (int i=0; i<N; i++) if (gsl_matrix_get(data, i,0) != -99.99)
        {
            n1=gsl_matrix_get(data,i,1);
            n2=w->text(j,colIndex).toDouble();
            if (action=="+")
            {
                n1+=n2;
            }
            else if (action=="-")
            {
                n1-=n2;
            }
            else if (action=="/")
            {
                if ( n2!=0.0 ) n1/=n2;
            }
            else if (action=="*")
            {
                n1*=n2;
            }
            gsl_matrix_set(data,i,1,n1);
            j++;
        }
        
    }
}


//*******************************************
// linearBinning ...
//*******************************************
bool ascii1d18::linearBinning(gsl_matrix* &data, int N, int &Nfinal)
{
    int binning=spinBoxBinningLinear->value();
    
    if (binning==1) return true;
    if (Nfinal==1) return true;
    
    
    int Nnew=(int)(double(double(Nfinal)/double(binning)));
    
    int lastBinning=binning;
    
    if ((Nfinal-Nnew*binning)>0) lastBinning+=Nfinal-Nnew*binning;
    
    double qq, ii, di, sigma;
    int localBinning=binning;
    int nn=0;
    
    double q, qMin, qMax;
    
    for(int n=0;n<Nnew;n++)
    {
        
        qq=0;
        ii=0;
        di=0;
        sigma=0;
        qMin=100;
        qMax=0;
        
        if (n==Nnew-1) localBinning=lastBinning;
        
        for(int b=0;b<localBinning;b++)
        {
            if (gsl_matrix_get(data, nn,0) == -99.99)
            {
                b--;
                nn++;
                continue;
            }
            q=gsl_matrix_get(data, nn, 0);
            if (q<qMin) qMin=q;
            if (q>qMax) qMax=q;
            
            qq+=q;
            ii+=gsl_matrix_get(data, nn, 1);
            di+=gsl_matrix_get(data, nn, 2)*gsl_matrix_get(data, nn, 2);
            sigma+=gsl_matrix_get(data, nn, 3);
            nn++;
        }
        
        gsl_matrix_set(data,n,0,qq/localBinning);
        gsl_matrix_set(data,n,1,ii/localBinning);
        gsl_matrix_set(data,n,2,sqrt(fabs(di))/localBinning);
        gsl_matrix_set(data,n,3,fabs(qMax-qMin)/2+sigma/localBinning);
    }
    
    for(int n=Nnew; n<N; n++) gsl_matrix_set(data,n,0,-99.99);
    
    Nfinal=Nnew;
    return true;
}


//*******************************************
// logBinning ...
//*******************************************
bool ascii1d18::logBinning(gsl_matrix* &data, int N, int &Nfinal)
{
    double binningRatio=lineEditBinningProgressive->value();
    
    if (binningRatio==1.0) return true;
    if (Nfinal==1) return true;
    
    
    int Nnew=0;
    
    int currentBinning=1;
    
    double qq, ii, di, sigma;
    double q, qMin, qMax;
    
    for(int n=0;n<N;n++)
    {
        
        
        currentBinning=int( pow(binningRatio,Nnew));
        
        qq=0;
        ii=0;
        di=0;
        sigma=0;
        qMin=100;
        qMax=0;
        
        for(int b=0;b<currentBinning;b++)
        {
            if (n==N-1 || Nnew==Nfinal-1)
            {
                currentBinning=b;
                n++;
                break;
            }
            
            if (gsl_matrix_get(data, n,0) == -99.99)
            {
                n++;
                b--;
                continue;
            }
            
            q=gsl_matrix_get(data, n, 0);
            if (q<qMin) qMin=q;
            if (q>qMax) qMax=q;
            qq+=q;
            ii+=gsl_matrix_get(data, n, 1);
            di+=gsl_matrix_get(data, n, 2)*gsl_matrix_get(data, n, 2);
            sigma+=gsl_matrix_get(data, n, 3);
            
            n++;
        }
        
        n--;
        
        if(currentBinning==0) break;
        
        gsl_matrix_set(data,Nnew,0,qq/currentBinning);
        gsl_matrix_set(data,Nnew,1,ii/currentBinning);
        gsl_matrix_set(data,Nnew,2,sqrt(fabs(di))/currentBinning);
        gsl_matrix_set(data,Nnew,3,fabs(qMax-qMin)/2+sigma/currentBinning);
        
        
        Nnew++;
    }
    
    for(int n=Nnew; n<N; n++) gsl_matrix_set(data,n,0,-99.99);
    
    Nfinal=Nnew;
    return true;
}

//*******************************************
// Save Data ...
//*******************************************
void ascii1d18::dataMatrixSave(QString &fn, gsl_matrix* data, int N, int Nfinal, bool loadedReso, Table* &w)
{
    QFileInfo fi( fn );
    QString name=generateTableName(fi.baseName().replace( QRegExp("[^a-zA-Z\\d\\s]"), "-").remove(" "));
    
    bool calculateReso=checkBoxReso->isChecked();
    
    QString label="QI";
    if (checkBoxConvert->isChecked()) label=comboBoxSelectPresentationTo->currentText();
    
    int i;
    
    if (comboBoxDestination->currentText()=="Table")
    {
        //+++ create table
        
        bool indexing=checkBoxIndexing->isChecked();
        if(fn=="Table-Active")
        {
            name="Table-Active";
            indexing=false;
        }
        if ( indexing || !app()->checkTableExistence(name,w) )
        {
            if (calculateReso || loadedReso)
            {
                w=app()->newHiddenTable(name,label, 0 ,4);
                w->setColName(3,"Sigma");
                w->setColPlotDesignation(3,Table::xErr);
            }
            else w=app()->newHiddenTable(name,label, 0 ,3);
        }
        else
        {
            w->setNumRows(0);
            if (w->numCols()==2) w->addCol();
            if (w->numCols()<4 && (calculateReso || loadedReso) ) w->addCol(); //w->setNumCols(4);
            //        if (w->numCols()<3 && !(calculateReso || loadedReso) ) w->setNumCols(3);
        }
        
        w->setColName(0,"Q");
        w->setColName(1,"I");
        w->setColName(2,"dI");
        if (calculateReso || loadedReso ) w->setColName(3,"Sigma");
        
        w->setColPlotDesignation(0,Table::X);
        w->setColPlotDesignation(1,Table::Y);
        w->setColPlotDesignation(2,Table::yErr);
        if (calculateReso || loadedReso )     w->setColPlotDesignation(3,Table::xErr);
        
        w->setHeaderColType();
        
        w->setColNumericFormat(2, 8, true);
        
        app()->updateWindowLists(w);
        
        w->setWindowLabel(label);
        app()->setListViewLabel(w->name(),label);
        app()->updateWindowLists(w);
        
        int currentRow=0;
        
        for (i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
        {
            w->setNumRows(currentRow+1);
            //
            w->setText(currentRow,0, QString::number(gsl_matrix_get(data,i,0),'E'));
            //
            w->setText(currentRow,1, QString::number(gsl_matrix_get(data,i,1),'E'));
            //
            if (gsl_matrix_get(data, i,2) == -99.99) w->setText(currentRow,2, "---");
            else w->setText(currentRow,2, QString::number(gsl_matrix_get(data,i,2),'E'));
            //
            if (calculateReso || loadedReso)
            {
                if (gsl_matrix_get(data, i,3) == -99.99) w->setText(currentRow,3, "---");
                else w->setText(currentRow,3, QString::number(gsl_matrix_get(data,i,3),'E'));
            }
            currentRow++;
        }
        
        //+++ adjust cols
        for (int tt=0; tt<w->numCols(); tt++)
        {
            w->table()->resizeColumnToContents(tt);
            w->table()->setColumnWidth(tt, w->table()->columnWidth(tt)+10);
        }
        
        
        if (w->numCols()>spinBoxForceNumCols->value() && !checkBoxReso->isChecked()) w->setNumCols(spinBoxForceNumCols->value());
        
        if (w->numCols()>3 && spinBoxForceNumCols->value()!=3 &&  w->numCols()>spinBoxForceNumCols->value() && checkBoxReso->isChecked())
        {
            for (i=0; i<w->numRows();i++)  w->setText(i,2, "---");
        }
        w->notifyChanges();
        
    }
    else
    {
        QFile f( lineEditPathOut->text()+"/"+name);
        
        
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            //*************************************Log Window Output
            QMessageBox::warning(this,"Could not write to file::" +  fn, tr("QtiSAS::ASCII.SANS.1D"));
            //*************************************Log Window Output
            return;
        }
        QTextStream stream( &f );
        int currentRow=0;
        
        for (i=0; i<N;i++)  if (gsl_matrix_get(data, i,0) != -99.99)
        {
            stream<<QString::number(gsl_matrix_get(data,i,0),'E');
            //
            stream<<"   "+QString::number(gsl_matrix_get(data,i,1),'E');
            //
            if (spinBoxForceNumCols->value()>2)
            {
                if (gsl_matrix_get(data, i,2) == -99.99) stream<<"   ---";
                else stream<<"   "+QString::number(gsl_matrix_get(data,i,2),'E');
            }
            //
            if (spinBoxForceNumCols->value()>3)
            {
                if (calculateReso)
                {
                    stream<<"   "+QString::number(sigma(gsl_matrix_get(data,i,0)),'E');
                }
                else if (loadedReso)
                {
                    stream<<"   "+QString::number(gsl_matrix_get(data,i,3),'E');
                }
            }
            currentRow++;
            stream<<"\n";
        }
        f.close();
        
    }
    fn=name;
}

//*******************************************
// plotTable ...
//*******************************************
bool ascii1d18::plotTable(Graph *g,Table *table, QString tableName)
{
    int cols=table->numCols();
    bool xError=checkBoxXerror->isChecked();
    bool yError=checkBoxYerror->isChecked();
    
    int xColIndex=0;
    int yColIndex=1;
    QString curveName=tableName+"_I";
    
    // check Table
    int  nReal=0;
    for (int ii=0; ii<table->numRows(); ii++) if ((table->text(ii,0))!="") nReal++;
    //
    if (nReal<1) return false;
    
    QStringList contents;
    contents=g->curveNamesList();
    
    if (g && table && !contents.contains(curveName))
    {
        int scatterCounter=0;
        
        for (int i=0; i<contents.count();i++)
        {
            PlotCurve *c = (PlotCurve *)g->curve(i);
            if (!c) break;
            int curve_type = c->plotStyle();
            
            if ( curve_type == Graph::Scatter) scatterCounter++;
        }
        
        int style = Graph::Scatter;

        g->insertCurve(table, curveName, style);
        CurveLayout cl = Graph::initCurveLayout();
        
        int shape=shape=app()->d_symbol_style;
        if (app()->d_indexed_symbols) shape=app()->indexedSymbols()[(scatterCounter)%(app()->indexedSymbols().count()-1)+1];
        int color =(scatterCounter)%app()->indexedColors().count();


        cl.lCol=app()->indexedColors()[color];
        cl.symCol=cl.lCol;
        cl.fillCol = app()->d_fill_symbols ? cl.lCol: QColor();
        cl.aCol=cl.lCol;
        cl.lWidth = app()->defaultCurveLineWidth;
        cl.penWidth = app()->defaultSymbolEdge;
        cl.sSize = app()->defaultSymbolSize;
        cl.sType=shape;
        
        g->updateCurveLayout((PlotCurve *)g->curve(contents.count()), &cl);
        g->replot();
    }
    
    if (g && table && !contents.contains(tableName+"_dI") && cols>2 &&  yError)
    {

            DataCurve *curve = g->dataCurve(g->curveIndex(curveName));
            int direction = 1;
            g->addErrorBars(curve, table, tableName+"_dI", direction, app()->defaultCurveLineWidth, 0, curve->pen().color(),true,true, true);
            g->replot();
    }
    if (cols<=2 || !yError)  g->removeCurve(tableName+"_dI");

    if (g && table && !contents.contains(tableName+"_Sigma") && cols>3 &&  xError)
    {
        DataCurve *curve = g->dataCurve(g->curveIndex(curveName));
        int direction = 0;
        g->addErrorBars(curve, table, tableName+"_Sigma", direction, app()->defaultCurveLineWidth, 0, curve->pen().color(),true,true, true);
        g->replot();
    }
    if (cols<=3 || !xError) g->removeCurve(tableName+"_Sigma");
    
    return true;
}

//*******************************************
// generateTableName ...
//*******************************************
QString ascii1d18::generateTableName(QString fn)
{
    QString format;
    
    if (checkBoxConvert->isChecked()) format=comboBoxSelectPresentationTo->currentText()+"-";
    else format="QI-";
    
    int findNumberIndex=spinBoxFindIndex->currentIndex();
    
    bool indexing=checkBoxIndexing->isChecked();
    
    bool tableOutput=true;
    
    if (comboBoxDestination->currentText()=="File") tableOutput=false;
    
    
    return generateTableName(fn, format,  findNumberIndex, indexing, tableOutput);
}



//*******************************************
/*
 fn:         file name
 format:        prefix, like "QI-"
 findNumberIndex:    find cuber in file name and generate name with this number; 0: ne name = old one
 indexing:        indexing of tables[files]
 tableOutput:    outbut > table opposite file
 */
//*******************************************
QString ascii1d18::generateTableName(QString fn,   QString prefix,  int findNumberIndex, bool indexing, bool tableOutput)
{
    QString format=prefix;
    QString ext="."+lineEditExt->text(); ext=ext.trimmed().remove(" ");
    if (ext=="" || ext==".") ext=".RAD";
    
    fn.replace('_', '-');
    
    bool findNumber=true;
    if (findNumberIndex==0) findNumber=false;
    
    // fing number
    int number=0;
    if (findNumber)
    {
        
        QRegExp rx("(\\d+)");
        int pos=0;
        int indexCurrent=0;
        while ( pos >= 0 && indexCurrent<findNumberIndex)
        {
            pos = rx.indexIn( fn, pos );
            number=rx.cap( 1 ).toInt();
            if ( pos > -1 ) pos  += rx.matchedLength();
            indexCurrent++;
        }
        if (indexCurrent<findNumberIndex) findNumber=false;
    }
    
    if (findNumber)
    {
        format+=QString::number(number);
    }
    else
    {
        if (comboBoxPrefixASCII1D->currentIndex()==1) format="";
        format+=fn;
    }
    //
    if (!indexing)
    {
        if (!tableOutput) format+=ext;
    }
    else
    {
        format+="-v-";
        if (tableOutput)
        {
            format=app()->generateUniqueName(format);
        }
        else
        {
            int fff=0;
            bool fileExist=true;
            QString path=lineEditPath->text();
            
            while (fileExist)
            {
                if ( QFile::exists(path+format+QString::number(fff)+ext) ) fff++;
                else fileExist=false;
            }
            format+=QString::number(fff)+ext;
        }
    }
    return format;
}

//*******************************************
// loadASCIIfromTable...
//*******************************************
bool ascii1d18::loadASCIIfromTable(const QString table, gsl_matrix* &data, int &N, bool &sigmaExist)
{
    Table *w;
    if (!app()->checkTableExistence(table,w)) return false;
    
    if (w->numCols()<2 || w->numRows()==0) return false;
    
    int pos;
    double Q,I,dI,sigma;
    QRegExp rx("((\\-|\\+)?\\d\\d*(\\.\\d*)?(((e|E)\\-|(e|E)\\+)\\d\\d?\\d?\\d?)?)");  //very new
    
    
    // to change to
    sigmaExist=false;
    
    
    for (int i=0;i<w->numRows();i++)
    {
        pos=0;
        if ( rx.indexIn(w->text(i,0),pos)>-1 ) Q=rx.cap( 1 ).toDouble(); else Q=-99.99;
        pos=0;
        if ( rx.indexIn(w->text(i,1),pos)>-1 ) I=rx.cap( 1 ).toDouble(); else I=-99.99;
        pos=0;
        if ( w->numCols() > 2 && rx.indexIn(w->text(i,2),pos)>-1 ) dI=rx.cap( 1 ).toDouble(); else dI=-99.99;
        pos=0;
        if ( w->numCols() > 3 && rx.indexIn(w->text(i,3),pos)>-1 )
        {
            sigma=rx.cap( 1 ).toDouble();
            sigmaExist=true;
        }
        else sigma=-99.99;
        
        if (Q!=-99.99 && I!=-99.99)
        {
            gsl_matrix_set(data,N,0,Q);
            gsl_matrix_set(data,N,1,I);
            gsl_matrix_set(data,N,2,dI);
            gsl_matrix_set(data,N,3,sigma);
            N++;
        }
    }
    
    if (N==0)
    {
        QMessageBox::warning(this, tr("QtiSAS::ASCII.SANS.1D") ,"File "+table+" contains no data, and is skipped");
        return false;
    }
    
    return true;
}

//*******************************************
//+++ function:: SAS visualization
//*******************************************
void ascii1d18::sasPresentation(QString &presentation )
{
    if (presentation=="QI") presentation="I vs. Q";
    else if (presentation=="Guinier") presentation="ln[I] vs. Q<sup>2</sup>";
    else if (presentation=="Zimm") presentation="I<sup>-1</sup> vs. Q<sup>2</sup>";
    else if (presentation=="Porod") presentation="log[I] vs. log[Q]";
    else if (presentation=="Porod2") presentation=" IQ<sup>4</sup> vs. Q<sup>2</sup>";
    else if (presentation=="logQ") presentation=" I vs. log[Q]";
    else if (presentation=="logI") presentation=" log[I] vs. Q ";
    else if (presentation=="Debye") presentation="I<sup>-1/2</sup> vs. Q<sup>2</sup>";
    else if (presentation=="1Moment") presentation="IQ vs. Q";
    else if (presentation=="2Moment") presentation="IQ<sup>2</sup> vs. Q";
    else if (presentation=="Kratky") presentation="IQ<sup>2</sup> vs. Q";
    else if (presentation=="GuinierRod") presentation="ln[IQ] vs. Q<sup>2</sup>";
    else if (presentation=="GuinierPlate") presentation="ln[IQ<sup>2</sup>] vs. Q<sup>2</sup>";
}

//*******************************************
//+++ Presentation:: From Changed +++
//*******************************************
void ascii1d18::presentationFromChanged()
{
    QString presentation=comboBoxSelectPresentationFrom->currentText();
    sasPresentation(presentation );
    textLabelPresFrom->setText(presentation);
}


//*******************************************
//+++ Presentation:: To Changed +++
//*******************************************
void ascii1d18::presentationToChanged()
{
    QString presentation=comboBoxSelectPresentationTo->currentText();
    sasPresentation(presentation );
    textLabelPresTo->setText(presentation);
}

//*******************************************
//+++  Reso Validator
//*******************************************
void ascii1d18::detColDistanceValidator()
{
    double change;
    
    // Check Collimation Distance
    change = lineEditResoDet->text().toDouble();
    if (change<10.0)
        lineEditResoDet->setText(QString::number( 10.0, 'f', 2 ) );
    else if (change>10000.0)
        lineEditResoDet->setText( QString::number( 10000.0, 'f', 2 ) );
    else
        lineEditResoDet->setText( QString::number( change, 'f', 2 ) );
    
    // Check Detector Distance
    change = lineEditResoCol->text().toDouble();
    if (change<10.0)
        lineEditResoCol->setText( QString::number( 10.0, 'f', 2 ) );
    else if (change>10000.0)
        lineEditResoCol->setText( QString::number( 10000.0, 'f', 2 ) );
    else
        lineEditResoCol->setText( QString::number( change, 'f', 2 ) );
    
    // Check Focus Distance
    change = lineEditResoFocus->text().toDouble();
    if (change<10.0)
        lineEditResoFocus->setText( QString::number( 10.0, 'f', 2 ) );
    else if (change>10000.0)
        lineEditResoFocus->setText( QString::number( 10000.0, 'f', 2 ) );
    else
        lineEditResoFocus->setText( QString::number( change, 'f', 2 ) );
    // Check Detector Pixel Size
    change = lineEditResoPixelSize->text().toDouble();
    if (change<0.0001)
        lineEditResoPixelSize->setText( QString::number( 0.0001, 'f', 4 ) );
    else if (change>5.0)
        lineEditResoPixelSize->setText( QString::number( 5.0, 'f', 4 ) );
    else
        lineEditResoPixelSize->setText( QString::number( change, 'f', 4 ) );
    
    // Check Sample Aperture Size
    change = lineEditResoSamAper->text().toDouble();
    if (change<0.01)
        lineEditResoSamAper->setText( QString::number( 0.01, 'f', 2 ) );
    else if (change>100.0)
        lineEditResoSamAper->setText( QString::number( 100.0, 'f', 2 ) );
    else
        lineEditResoSamAper->setText( QString::number( change, 'f', 2 ) );
    
    // Check Sample Aperture Size
    change = lineEditResoSamAper2->text().toDouble();
    if (change<0.01)
        lineEditResoSamAper2->setText( QString::number( 0.01, 'f', 2 ) );
    else if (change>100.0)
        lineEditResoSamAper2->setText( QString::number( 100.0, 'f', 2 ) );
    else
        lineEditResoSamAper2->setText( QString::number( change, 'f', 2 ) );
    
    // Check Collimation Aperture Size
    change = lineEditResoColAper->text().toDouble();
    if (change<0.01)
        lineEditResoColAper->setText( QString::number( 0.01, 'f', 2 ) );
    else if (change>100.0)
        lineEditResoColAper->setText( QString::number( 100.0, 'f', 2 ) );
    else
        lineEditResoColAper->setText( QString::number( change, 'f', 2 ) );
    
    // Check Collimation Aperture Size
    change = lineEditResoColAper2->text().toDouble();
    if (change<0.01)
        lineEditResoColAper2->setText( QString::number( 0.01, 'f', 2 ) );
    else if (change>100.0)
        lineEditResoColAper2->setText( QString::number( 100.0, 'f', 2 ) );
    else
        lineEditResoColAper2->setText( QString::number( change, 'f', 2 ) );
    
    // Check Wave Length [A]
    change = lineEditResoLambda->text().toDouble();
    if (change<0.1)
        lineEditResoLambda->setText( QString::number( 0.1, 'f', 3 ) );
    else if (change>50.0)
        lineEditResoLambda->setText( QString::number( 50.0, 'f', 3 ) );
    else
        lineEditResoLambda->setText( QString::number( change, 'f', 3 ) );
    
    // Check delta lambda [1]
    change = lineEditResoDeltaLambda->text().toDouble();
    if (change<0.0001)
        lineEditResoDeltaLambda->setText( QString::number( 0.0001, 'f', 4 ) );
    else if (change>0.8)
        lineEditResoDeltaLambda->setText( QString::number( 0.8, 'f', 4 ) );
    else
        lineEditResoDeltaLambda->setText( QString::number( change, 'f', 4 ) );
}

//*******************************************
//+++   checkBoxResoSlot
//*******************************************
void ascii1d18::checkBoxResoSlot()
{
    if (checkBoxReso->isChecked())
    {
        checkBoxConvert->setChecked(false);
    }
}
 
//*******************************************
//+++   slotMathYN
//*******************************************
void ascii1d18::slotMathYN()
{
    double mathFactor=lineEditMath->text().toDouble();
    QString mathChar=comboBoxMath->currentText();
    if (mathChar=="/" && mathFactor==0)
    {
        mathFactor=1;
        lineEditMath->setText("1");
    }
}
