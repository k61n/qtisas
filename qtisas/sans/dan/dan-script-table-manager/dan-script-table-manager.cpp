/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN script table manager
 ******************************************************************************/

#include "dan-script-table-manager.h"

ScriptTableManager::ScriptTableManager(QComboBox *modeDAN, QComboBox *polModeDAN, Table *wDAN)
{
    mode = modeDAN;
    polMode = polModeDAN;
    if (wDAN != nullptr)
    {
        w = wDAN;
        update(w);
    }
}
bool ScriptTableManager::update(Table *wDAN)
{
    if (!wDAN)
        return false;

    w = wDAN;
    cols = w->numCols();

    QString modeCurrentText = mode->currentText();
    bool modePN = modeCurrentText.contains("(PN)");
    bool modeBS = modeCurrentText.contains("(BS");
    bool modeBSsens = modeCurrentText.contains("(BS-SENS)");

    //+++
    QStringList scriptColList = w->colNames();

    //+++ Run-info
    if (!readColumnIndex(indexInfo, "Run-info", scriptColList))
        return false;
    //+++ #-Run : Sample Run Number
    if (!readColumnIndex(indexSample, "#-Run", scriptColList))
        return false;
    //+++ #-Condition
    if (!readColumnIndex(indexCondition, "#-Condition", scriptColList))
        return false;
    //+++ Polarization
    if (modePN && !readColumnIndex(indexPolarization, "Polarization", scriptColList))
        return false;
    //+++ C
    if (!readColumnIndex(indexC, "C", scriptColList))
        return false;
    //+++ C
    if (!readColumnIndex(indexD, "D", scriptColList))
        return false;
    //+++ Lambda
    if (!readColumnIndex(indexLambda, "Lambda", scriptColList))
        return false;
    //+++ Beam Size
    if (!readColumnIndex(indexBeamSize, "Beam Size", scriptColList))
        return false;
    //+++ #-BC
    if (!readColumnIndex(indexBC, "#-BC", scriptColList))
        return false;
    //+++ #-EC [EB]
    if (!readColumnIndex(indexEC, "#-EC [EB]", scriptColList))
        return false;
    //+++ #-Buffer
    if (modeBS && !readColumnIndex(indexBuffer, "#-Buffer", scriptColList))
        return false;
    //+++ Thickness
    if (!readColumnIndex(indexThickness, "Thickness", scriptColList))
        return false;
    //+++ Transmission-Sample
    if (!readColumnIndex(indexTransmission, "Transmission-Sample", scriptColList))
        return false;
    //+++ Transmission-Buffer
    if (modeBS && !readColumnIndex(indexTrBuffer, "Transmission-Buffer", scriptColList))
        return false;
    //+++ Buffer-Fraction
    if (modeBS && !readColumnIndex(indexBufferFraction, "Buffer-Fraction", scriptColList))
        return false;
    //+++ Factor
    if (!readColumnIndex(indexFactor, "Factor", scriptColList))
        return false;
    //+++ X-center
    if (!readColumnIndex(indexXC, "X-center", scriptColList))
        return false;
    //+++ Y-center
    if (!readColumnIndex(indexYC, "Y-center", scriptColList))
        return false;
    //+++ Mask
    if (!readColumnIndex(indexMask, "Mask", scriptColList))
        return false;
    //+++ Sens
    if (!readColumnIndex(indexSens, "Sens", scriptColList))
        return false;
    //+++ Use-Buffer-as-Sensitivity
    if (modeBSsens && !readColumnIndex(indexUseSensFromBuffer, "Use-Buffer-as-Sensitivity", scriptColList))
        return false;
    //+++ Status
    readColumnIndex(indexStatus, "Status", scriptColList);
    //+++ Analyzer Transmission
    if (modePN && !readColumnIndex(indexAnalyzerTransmission, "Analyzer-Transmission", scriptColList))
        return false;
    //+++ Analyzer Efficiency
    if (modePN && !readColumnIndex(indexAnalyzerEfficiency, "Analyzer-Efficiency", scriptColList))
        return false;

    //+++ Scale
    readColumnIndex(indexScale, "Scale", scriptColList, true);
    //+++ Background
    readColumnIndex(indexBackgroundConst, "Background", scriptColList, true);
    //+++ VShift
    readColumnIndex(indexVShift, "VShift", scriptColList, true);
    //+++ HShift
    readColumnIndex(indexHShift, "HShift", scriptColList, true);
    //+++ Suffix
    readColumnIndex(indexSuffix, "Suffix", scriptColList, true);
    //+++ TrDet
    readColumnIndex(indexTrDet, "TrDet", scriptColList, true);
    //+++ maskDB
    readColumnIndex(indexMaskDB, "MaskDB", scriptColList, true);
    //+++ removeFirst
    readColumnIndex(indexRemoveFirst, "RemoveFirst", scriptColList, true);
    //+++ removeLast
    readColumnIndex(indexRemoveLast, "RemoveLast", scriptColList, true);
    return true;
}

// read text(s)
QString ScriptTableManager::runSample(int row)
{
    return read(row, indexSample);
}
QString ScriptTableManager::runBuffer(int row)
{
    return read(row, indexBuffer);
}
QString ScriptTableManager::runEC(int row)
{
    return read(row, indexEC);
}
QString ScriptTableManager::runBC(int row)
{
    return read(row, indexBC);
}
QString ScriptTableManager::info(int row)
{
    return read(row, indexInfo);
}
QString ScriptTableManager::condition(int row)
{
    return read(row, indexCondition);
}
QString ScriptTableManager::polarization(int row)
{
    return read(row, indexPolarization);
}
QString ScriptTableManager::collimation(int row)
{
    return read(row, indexC);
}
QString ScriptTableManager::distance(int row)
{
    return read(row, indexD);
}
QString ScriptTableManager::lambda(int row)
{
    return read(row, indexLambda);
}
QString ScriptTableManager::beamSize(int row)
{
    return read(row, indexBeamSize);
}
QString ScriptTableManager::thickness(int row)
{
    return read(row, indexThickness);
}
QString ScriptTableManager::transmission(int row)
{
    return read(row, indexTransmission);
}
QString ScriptTableManager::transmissionBuffer(int row)
{
    return read(row, indexTrBuffer);
}
QString ScriptTableManager::fractionBuffer(int row)
{
    return read(row, indexBufferFraction);
}
QString ScriptTableManager::absoluteFactor(int row)
{
    return read(row, indexFactor);
}
QString ScriptTableManager::centerX(int row)
{
    return read(row, indexXC);
}
QString ScriptTableManager::centerY(int row)
{
    return read(row, indexYC);
}
QString ScriptTableManager::mask(int row)
{
    return read(row, indexMask);
}
QString ScriptTableManager::sens(int row)
{
    return read(row, indexSens);
}
QString ScriptTableManager::sensFromBuffer(int row)
{
    return read(row, indexUseSensFromBuffer);
}
QString ScriptTableManager::readStatus(int row)
{
    return read(row, indexStatus);
}
QString ScriptTableManager::analyzerTransmission(int row)
{
    return read(row, indexAnalyzerTransmission);
}
QString ScriptTableManager::analyzerEfficiency(int row)
{
    return read(row, indexAnalyzerEfficiency);
}
QString ScriptTableManager::scale(int row)
{
    return read(row, indexScale);
}
QString ScriptTableManager::backgroundConst(int row)
{
    return read(row, indexBackgroundConst);
}
QString ScriptTableManager::VSchift(int row)
{
    return read(row, indexVShift);
}
QString ScriptTableManager::HSchift(int row)
{
    return read(row, indexHShift);
}
QString ScriptTableManager::suffix(int row)
{
    return read(row, indexSuffix);
}
QString ScriptTableManager::transmissionDetector(int row)
{
    return read(row, indexTrDet);
}
QString ScriptTableManager::maskDB(int row)
{
    return read(row, indexMaskDB);
}
QString ScriptTableManager::removeFirst(int row)
{
    return read(row, indexRemoveFirst);
}
QString ScriptTableManager::removeLast(int row)
{
    return read(row, indexRemoveLast);
}
// wtire text(s)
bool ScriptTableManager::runSampleWrite(int row, const QString &txt)
{
    return write(row, indexSample, txt);
}
bool ScriptTableManager::runBufferWrite(int row, const QString &txt)
{
    return write(row, indexBuffer, txt);
}
bool ScriptTableManager::runECWrite(int row, const QString &txt)
{
    return write(row, indexEC, txt);
}
bool ScriptTableManager::runBCWrite(int row, const QString &txt)
{
    return write(row, indexBC, txt);
}
bool ScriptTableManager::infoWrite(int row, const QString &txt)
{
    return write(row, indexInfo, txt);
}
bool ScriptTableManager::conditionWrite(int row, const QString &txt)
{
    return write(row, indexCondition, txt);
}
bool ScriptTableManager::polarizationWrite(int row, const QString &txt)
{
    return write(row, indexPolarization, txt);
}
bool ScriptTableManager::collimationWrite(int row, const QString &txt)
{
    return write(row, indexC, txt);
}
bool ScriptTableManager::distanceWrite(int row, const QString &txt)
{
    return write(row, indexD, txt);
}
bool ScriptTableManager::lambdaWrite(int row, const QString &txt)
{
    return write(row, indexLambda, txt);
}
bool ScriptTableManager::beamSizeWrite(int row, const QString &txt)
{
    return write(row, indexBeamSize, txt);
}
bool ScriptTableManager::thicknessWrite(int row, const QString &txt)
{
    return write(row, indexThickness, txt);
}
bool ScriptTableManager::transmissionWrite(int row, const QString &txt)
{
    return write(row, indexTransmission, txt);
}
bool ScriptTableManager::transmissionBufferWrite(int row, const QString &txt)
{
    return write(row, indexTrBuffer, txt);
}
bool ScriptTableManager::fractionBufferWrite(int row, const QString &txt)
{
    return write(row, indexBufferFraction, txt);
}
bool ScriptTableManager::absoluteFactorWrite(int row, const QString &txt)
{
    return write(row, indexFactor, txt);
}
bool ScriptTableManager::centerXWrite(int row, const QString &txt)
{
    return write(row, indexXC, txt);
}
bool ScriptTableManager::centerYWrite(int row, const QString &txt)
{
    return write(row, indexYC, txt);
}
bool ScriptTableManager::maskWrite(int row, const QString &txt)
{
    return write(row, indexMask, txt);
}
bool ScriptTableManager::sensWrite(int row, const QString &txt)
{
    return write(row, indexSens, txt);
}
bool ScriptTableManager::sensFromBufferWrite(int row, const QString &txt)
{
    return write(row, indexUseSensFromBuffer, txt);
}
bool ScriptTableManager::readStatusWrite(int row, const QString &txt)
{
    return write(row, indexStatus, txt);
}
bool ScriptTableManager::analyzerTransmissionWrite(int row, const QString &txt)
{
    return write(row, indexAnalyzerTransmission, txt);
}
bool ScriptTableManager::analyzerEfficiencyWrite(int row, const QString &txt)
{
    return write(row, indexAnalyzerEfficiency, txt);
}
bool ScriptTableManager::scaleWrite(int row, const QString &txt)
{
    return write(row, indexScale, txt);
}
bool ScriptTableManager::backgroundConstWrite(int row, const QString &txt)
{
    return write(row, indexBackgroundConst, txt);
}
bool ScriptTableManager::VSchiftWrite(int row, const QString &txt)
{
    return write(row, indexVShift, txt);
}
bool ScriptTableManager::HSchiftWrite(int row, const QString &txt)
{
    return write(row, indexHShift, txt);
}
bool ScriptTableManager::suffixWrite(int row, const QString &txt)
{
    return write(row, indexSuffix, txt);
}
bool ScriptTableManager::transmissionDetectorWrite(int row, const QString &txt)
{
    return write(row, indexTrDet, txt);
}
bool ScriptTableManager::maskDBWrite(int row, const QString &txt)
{
    return write(row, indexMaskDB, txt);
}
bool ScriptTableManager::removeFirstWrite(int row, const QString &txt)
{
    return write(row, indexRemoveFirst, txt);
}
bool ScriptTableManager::removeLastWrite(int row, const QString &txt)
{
    return write(row, indexRemoveLast, txt);
}
//*******************************************
//+++  newScriptTable() [slot]
//*******************************************
void ScriptTableManager::emptyScriptTable(Table *newTable)
{
    w = newTable;
    w->setNumCols(24);
    w->setNumRows(0);

    QString modeCurrentText = mode->currentText();
    bool modePN = modeCurrentText.contains("(PN)");
    bool modeBS = modeCurrentText.contains("(BS");
    bool modeBSsens = modeCurrentText.contains("(BS-SENS)");

    //+++
    w->setWindowLabel("DAN::Script::Table");

    // Col-Names
    int colNumber = 0;

    indexInfo = colNumber;
    w->setColName(indexInfo, "Run-info");
    w->setColPlotDesignation(indexInfo, Table::None);
    w->setColumnType(indexInfo, Table::Text);
    colNumber++;

    indexSample = colNumber;
    w->setColName(indexSample, "#-Run");
    w->setColPlotDesignation(indexSample, Table::X);
    w->setColumnType(indexSample, Table::Text);
    colNumber++;

    indexCondition = colNumber;
    w->setColName(indexCondition, "#-Condition");
    w->setColPlotDesignation(indexCondition, Table::None);
    w->setColumnType(indexCondition, Table::Numeric);
    colNumber++;

    indexPolarization = colNumber;
    w->setColName(indexPolarization, "Polarization");
    w->setColPlotDesignation(indexPolarization, Table::None);
    w->setColumnType(indexPolarization, Table::Text);
    w->hideColumn(colNumber, !modePN);
    colNumber++;

    indexC = colNumber;
    w->setColName(indexC, "C");
    w->setColComment(indexC, "[m]");
    w->setColPlotDesignation(indexC, Table::None);
    w->setColumnType(indexC, Table::Numeric);
    colNumber++;

    indexD = colNumber;
    w->setColName(indexD, "D");
    w->setColComment(indexD, "[m]");
    w->setColPlotDesignation(indexD, Table::None);
    w->setColumnType(indexD, Table::Numeric);
    colNumber++;

    indexLambda = colNumber;
    w->setColName(indexLambda, "Lambda");
    QString sss = "[";
    sss += QChar(197);
    sss += "]";
    w->setColComment(indexLambda, sss);
    w->setColPlotDesignation(indexLambda, Table::None);
    w->setColumnType(indexLambda, Table::Numeric);
    colNumber++;

    indexBeamSize = colNumber;
    w->setColName(indexBeamSize, "Beam Size");
    w->setColComment(indexBeamSize, "[mm x mm]");
    w->setColPlotDesignation(indexBeamSize, Table::None);
    w->setColumnType(indexBeamSize, Table::Text);
    colNumber++;

    indexBC = colNumber;
    w->setColName(indexBC, "#-BC");
    w->setColPlotDesignation(indexBC, Table::None);
    w->setColumnType(indexBC, Table::Text);
    colNumber++;

    indexEC = colNumber;
    w->setColName(indexEC, "#-EC [EB]");
    w->setColPlotDesignation(indexEC, Table::None);
    w->setColumnType(indexEC, Table::Text);
    colNumber++;

    indexBuffer = colNumber;
    w->setColName(indexBuffer, "#-Buffer");
    w->setColPlotDesignation(indexBuffer, Table::None);
    w->setColumnType(indexBuffer, Table::Text);
    w->hideColumn(indexBuffer, !modeBS);
    colNumber++;

    indexThickness = colNumber;
    w->setColName(indexThickness, "Thickness");
    w->setColComment(indexThickness, "[cm]");
    w->setColPlotDesignation(indexThickness, Table::None);
    w->setColumnType(indexThickness, Table::Numeric);
    colNumber++;

    indexTransmission = colNumber;
    w->setColName(indexTransmission, "Transmission-Sample");
    w->setColPlotDesignation(indexTransmission, Table::None);
    w->setColumnType(indexTransmission, Table::Text);
    colNumber++;

    indexTrBuffer = colNumber;
    w->setColName(indexTrBuffer, "Transmission-Buffer");
    w->setColPlotDesignation(indexTrBuffer, Table::None);
    w->setColumnType(indexTrBuffer, Table::Text);
    w->hideColumn(indexTrBuffer, !modeBS);
    colNumber++;

    indexBufferFraction = colNumber;
    w->setColName(indexBufferFraction, "Buffer-Fraction");
    w->setColPlotDesignation(indexBufferFraction, Table::None);
    w->setColumnType(indexBufferFraction, Table::Numeric);
    w->hideColumn(indexBufferFraction, !modeBS);
    colNumber++;

    indexFactor = colNumber;
    w->setColName(indexFactor, "Factor");
    w->setColPlotDesignation(indexFactor, Table::None);
    w->setColumnType(indexFactor, Table::Numeric);
    colNumber++;

    indexXC = colNumber;
    w->setColName(indexXC, "X-center");
    w->setColPlotDesignation(indexXC, Table::Y);
    w->setColumnType(indexXC, Table::Numeric);
    colNumber++;

    indexYC = colNumber;
    w->setColName(indexYC, "Y-center");
    w->setColPlotDesignation(indexYC, Table::Y);
    w->setColumnType(indexYC, Table::Numeric);
    colNumber++;

    indexMask = colNumber;
    w->setColName(indexMask, "Mask");
    w->setColPlotDesignation(indexMask, Table::None);
    w->setColumnType(indexMask, Table::Text);
    colNumber++;

    indexSens = colNumber;
    w->setColName(indexSens, "Sens");
    w->setColPlotDesignation(indexSens, Table::None);
    w->setColumnType(indexSens, Table::Text);
    colNumber++;

    indexUseSensFromBuffer = colNumber;
    w->setColName(colNumber, "Use-Buffer-as-Sensitivity");
    w->setColPlotDesignation(indexUseSensFromBuffer, Table::None);
    w->setColumnType(indexUseSensFromBuffer, Table::Text);
    w->hideColumn(indexUseSensFromBuffer, !modeBSsens);
    colNumber++;

    indexStatus = colNumber;
    w->setColName(indexStatus, "Status");
    w->setColPlotDesignation(indexStatus, Table::None);
    w->setColumnType(indexStatus, Table::Text);
    colNumber++;

    indexAnalyzerTransmission = colNumber;
    w->setColName(indexAnalyzerTransmission, "Analyzer-Transmission");
    w->setColPlotDesignation(indexAnalyzerTransmission, Table::Y);
    w->setColumnType(indexAnalyzerTransmission, Table::Numeric);
    w->hideColumn(indexAnalyzerTransmission, !modePN);
    colNumber++;

    indexAnalyzerEfficiency = colNumber;
    w->setColName(indexAnalyzerEfficiency, "Analyzer-Efficiency");
    w->setColPlotDesignation(indexAnalyzerEfficiency, Table::Y);
    w->setColumnType(indexAnalyzerEfficiency, Table::Numeric);
    w->hideColumn(indexAnalyzerEfficiency, !modePN);
    colNumber++;

    // adjust columns
    w->adjustColumnsWidth(false);
}