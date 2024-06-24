/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN script table manager
 ******************************************************************************/
#ifndef DAN_SCRIPT_TABLE_MANAGER_H
#define DAN_SCRIPT_TABLE_MANAGER_H

#include <iostream>
#include <QComboBox>
#include <QString>
#include <QStringList>
#include "Table.h"

class ScriptTableManager : public QObject
{
    Q_OBJECT
  protected:
    QComboBox *mode;
    QComboBox *polMode;
    Table *w;

    int cols = 0; // number cols

    int indexInfo = -1;                 //+++ Run-info
    int indexCondition = -1;            //+++ #-Condition
    int indexPolarization = -1;         //+++ Polarization
    int indexSample = -1;               //+++ #-Run
    int indexC = -1;                    //+++ C
    int indexD = -1;                    //+++ D:
    int indexLambda = -1;               //+++ Lambda
    int indexBeamSize = -1;             //+++ Beam Size
    int indexBC = -1;                   //+++ #-BC
    int indexEC = -1;                   //+++ #-EC [EB]
    int indexBuffer = -1;               //+++ #-Buffer
    int indexThickness = -1;            //+++ Thickness
    int indexTransmission = -1;         //+++ Transmission-Sample
    int indexTrBuffer = -1;             //+++ Transmission-Buffer
    int indexBufferFraction = -1;       //+++ Buffer-Fraction
    int indexFactor = -1;               //+++ Factor
    int indexXC = -1;                   //+++ X-center
    int indexYC = -1;                   //+++ Y-center
    int indexMask = -1;                 //+++ Mask
    int indexSens = -1;                 //+++ Sens
    int indexUseSensFromBuffer = -1;    //+++ Use sensitivity Local
    int indexStatus = -1;               //+++ Status
    int indexAnalyzerTransmission = -1; //+++ Analyzer Transmission
    int indexAnalyzerEfficiency = -1;   //+++ Analyzer Efficiency

    //+++ Hand-made columns
    int indexScale = -1;           //+++ Scale
    int indexBackgroundConst = -1; //+++ BackgroundConst
    int indexVShift = -1;          //+++ VShift
    int indexHShift = -1;          //+++ HShift
    int indexSuffix = -1;          //+++ Suffix
    int indexTrDet = -1;           //+++ TrDet
    int indexMaskDB = -1;          //+++ MaskDB
    int indexRemoveFirst = -1;     //+++ RemoveFirst
    int indexRemoveLast = -1;      //+++ RemoveLast

    QString read(int row, int index)
    {
        return (w && row >= 0 && row < w->numRows() && index >= 0 && index < cols) ? w->text(row, index) : "";
    }
    bool write(int row, int index, const QString &text)
    {
        if (!(w && row >= 0 && row < w->numRows() && index >= 0 && index < cols))
            return false;
        w->setText(row, index, text);
        return true;
    }
    static int readColumnIndex(int &index, const QString &colName, const QStringList &scriptColList,
                               bool blockOutput = false)
    {
        index = scriptColList.indexOf(colName);
        if (index < 0)
        {
            if (!blockOutput)
                std::cout << colName.toLatin1().constData() << " column is missing ...\n";
            return false;
        }
        else if (blockOutput)
            std::cout << colName.toLatin1().constData() << ": optional column exists\n";
        return true;
    }
  public:
    ScriptTableManager(QComboBox *mode, QComboBox *polMode, Table *w = nullptr);
    void emptyScriptTable(Table *newTable);
    bool update(Table *w);

    // read text(s)
    QString runSample(int row);
    QString runBuffer(int row);
    QString runEC(int row);
    QString runBC(int row);
    QString info(int row);
    QString condition(int row);
    QString polarization(int row);
    QString collimation(int row);
    QString distance(int row);
    QString lambda(int row);
    QString beamSize(int row);
    QString thickness(int row);
    QString transmission(int row);
    QString transmissionBuffer(int row);
    QString fractionBuffer(int row);
    QString absoluteFactor(int row);
    QString centerX(int row);
    QString centerY(int row);
    QString mask(int row);
    QString sens(int row);
    QString sensFromBuffer(int row);
    QString readStatus(int row);
    QString analyzerTransmission(int row);
    QString analyzerEfficiency(int row);
    QString scale(int row);
    QString backgroundConst(int row);
    QString VSchift(int row);
    QString HSchift(int row);
    QString suffix(int row);
    QString transmissionDetector(int row);
    QString maskDB(int row);
    QString removeFirst(int row);
    QString removeLast(int row);

    // write text(s)
    bool runSampleWrite(int row, const QString &txt);
    bool runBufferWrite(int row, const QString &txt);
    bool runECWrite(int row, const QString &txt);
    bool runBCWrite(int row, const QString &txt);
    bool infoWrite(int row, const QString &txt);
    bool conditionWrite(int row, const QString &txt);
    bool polarizationWrite(int row, const QString &txt);
    bool collimationWrite(int row, const QString &txt);
    bool distanceWrite(int row, const QString &txt);
    bool lambdaWrite(int row, const QString &txt);
    bool beamSizeWrite(int row, const QString &txt);
    bool thicknessWrite(int row, const QString &txt);
    bool transmissionWrite(int row, const QString &txt);
    bool transmissionBufferWrite(int row, const QString &txt);
    bool fractionBufferWrite(int row, const QString &txt);
    bool absoluteFactorWrite(int row, const QString &txt);
    bool centerXWrite(int row, const QString &txt);
    bool centerYWrite(int row, const QString &txt);
    bool maskWrite(int row, const QString &txt);
    bool sensWrite(int row, const QString &txt);
    bool sensFromBufferWrite(int row, const QString &txt);
    bool readStatusWrite(int row, const QString &txt);
    bool analyzerTransmissionWrite(int row, const QString &txt);
    bool analyzerEfficiencyWrite(int row, const QString &txt);
    bool scaleWrite(int row, const QString &txt);
    bool backgroundConstWrite(int row, const QString &txt);
    bool VSchiftWrite(int row, const QString &txt);
    bool HSchiftWrite(int row, const QString &txt);
    bool suffixWrite(int row, const QString &txt);
    bool transmissionDetectorWrite(int row, const QString &txt);
    bool maskDBWrite(int row, const QString &txt);
    bool removeFirstWrite(int row, const QString &txt);
    bool removeLastWrite(int row, const QString &txt);

  public slots:

};
#endif