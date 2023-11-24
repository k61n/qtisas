/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Header::Map Parser used in DAN-SANS interface
 ******************************************************************************/

#include "dan-parser-header.h"

ParserHeader::ParserHeader(FilesManager *filesManagerDan, QTableWidget *tableHeaderDan, QComboBox *HeaderFormatDan,
                           QGroupBox *XMLgroupDan, QLineEdit *XMLbaseDan, QGroupBox *FLEXIgroupDan,
                           QCheckBox *FLEXIcheckDan, QLineEdit *FLEXIlineDan, QSpinBox *HEADERlengthDan,
                           QSpinBox *HEADER2NDlengthDan, QSpinBox *HEADERDATAlengthDan)
{
    // getting live-control of the Header::Map related widgets
    filesManager = filesManagerDan;
    tableHeader = tableHeaderDan;
    HeaderFormat = HeaderFormatDan;
    XMLgroup = XMLgroupDan;
    XMLbase = XMLbaseDan;
    FLEXIgroup = FLEXIgroupDan;
    FLEXIcheck = FLEXIcheckDan;
    FLEXIline = FLEXIlineDan;
    HEADERlength = HEADERlengthDan;
    HEADER2NDlength = HEADER2NDlengthDan;
    HEADERDATAlength = HEADERDATAlengthDan;
    // init list of Headers
    initListOfHeaders();
    // init Header Table
    tableHeader->setRowCount(56);
    tableHeader->setVerticalHeaderLabels(listOfHeaders);
    for (int i = 0; i < 56; i++)
    {
        tableHeader->setItem(i, 0, new QTableWidgetItem);
        tableHeader->setItem(i, 1, new QTableWidgetItem);
    }

    connect(HeaderFormat, SIGNAL(activated(int)), this, SLOT(dataFormatChanged(int)));
}
//+++ read a string from run Number with  pos and  num codes
QString ParserHeader::readNumberString(const QString &Number, const QString &headerName, const QStringList &lst)
{
    int indexInHeader = listOfHeaders.indexOf(headerName);
    if (indexInHeader < 0)
        return "";

    QString pos = tableHeader->item(indexInHeader, 0)->text();
    QString num = tableHeader->item(indexInHeader, 1)->text();

    return readNumberString(Number, pos, num, lst);
}

//+++ read a string from run Number with  pos and  num codes
QString ParserHeader::readNumberString(const QString &Number, QString &pos, QString &num, const QStringList &lst)
{
    //++++++++++++++++++++++++++++++++++++
    //+++ return: Run Number
    //++++++++++++++++++++++++++++++++++++
    if (num == "#")
        return Number;

    //++++++++++++++++++++++++++++++++++++
    //+++ return: "---" / not defined
    //++++++++++++++++++++++++++++++++++++
    if (pos == "")
    {
        pos = "0";
        num = "0";
        return "---";
    } // not defined

    //++++++++++++++++++++++++++++++++++++
    //+++ return: constant value num
    //++++++++++++++++++++++++++++++++++++
    if (pos.contains("const"))
    {
        pos = "0";
        return num;
    }

    //++++++++++++++++++++++++++++++++++++
    //+++ file for XML/YAML/HDF5 parsers
    //++++++++++++++++++++++++++++++++++++
    QString card = filesManager->wildCardHeader();
    QString fileName = filesManager->fileNameFull(Number, card);
    if (fileName == "")
    {
        std::cout << "file with run-Number: " << Number.toLatin1().constData() << " does not exist!\n" << std::flush;
        return "";
    }
    //++++++++++++++++++++++++++++++++++++
    //+++ XML/YAML/HDF5/Flexy/STD parsers
    //++++++++++++++++++++++++++++++++++++
    QString result = "---";
    QString postParserAction;
    switch (HeaderFormat->currentIndex())
    {
    case 1: {
        // QString xmlCode = pos;
        QString xmlBase = XMLbase->text();
        result = ParserXML::readEntry(fileName, pos, xmlBase);
        postParserAction = num;
        break;
    }
    case 2: {
        // const QString yamlCode = pos;
        result = ParserYAML::readEntry(fileName, pos);
        postParserAction = num;
        break;
    }
    case 3: {
        // QString hdf5code = pos;
        QString hdf5action = num;
        result = ParserHDF5::readEntry(fileName, pos, hdf5action);
        postParserAction = hdf5action;
        break;
    }
    default: {
        //++++++++++++++++++++++++++++++++++++
        //+++ ASCII/FLEXI header length
        //++++++++++++++++++++++++++++++++++++
        int maxLinesNumberInHeader = 0;
        if (filesManager->secondHeaderYN())
            maxLinesNumberInHeader = HEADER2NDlength->value();
        else
        {
            maxLinesNumberInHeader = HEADERlength->value();
            maxLinesNumberInHeader += HEADERDATAlength->value();
        }
        //++++++++++++++++++++++++++++++++++++
        //+++ FLEXI header
        //++++++++++++++++++++++++++++++++++++
        if (pos.contains("{") && pos.contains("}"))
        {
            //++++++++++++++++++++++++++++++++++++
            //+++ FLEXI header
            //++++++++++++++++++++++++++++++++++++
            QString FLEXYcode = pos;
            result = ParserASCII::readEntryFlexy(fileName, FLEXYcode, maxLinesNumberInHeader);
            pos = FLEXYcode;
            postParserAction = num;
        }
        else
        {
            //++++++++++++++++++++++++++++++++++++
            //+++ ASCII header
            //++++++++++++++++++++++++++++++++++++
            if (Number.contains("[") && pos.contains("["))
            {
                int index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]").toInt();
                pos = QString::number(pos.remove("[").remove("]").toInt() + index);
            }
            QString STDcode = pos;
            result = ParserASCII::readEntry(fileName, STDcode, lst);
            pos = STDcode;
            postParserAction = num;
        }
        break;
    }
    }
    result = ParserASCII::stringParser(result, postParserAction);
    num = postParserAction;

    return result;
}
void ParserHeader::initListOfHeaders()
{
    listOfHeaders.clear();
    listOfHeaders << "[Experiment-Title]";
    listOfHeaders << "[User-Name]";
    listOfHeaders << "[Sample-Run-Number]";
    listOfHeaders << "[Sample-Title]";
    listOfHeaders << "[Sample-Thickness]";
    listOfHeaders << "[Sample-Position-Number]";
    listOfHeaders << "[Date]";
    listOfHeaders << "[Time]";
    listOfHeaders << "[C]";
    listOfHeaders << "[D]";
    listOfHeaders << "[D-TOF]";
    listOfHeaders << "[C,D-Offset]";
    listOfHeaders << "[CA-X]";
    listOfHeaders << "[CA-Y]";
    listOfHeaders << "[SA-X]";
    listOfHeaders << "[SA-Y]";
    listOfHeaders << "[Sum]";
    listOfHeaders << "[Selector]";
    listOfHeaders << "[Lambda]";
    listOfHeaders << "[Delta-Lambda]";
    listOfHeaders << "[Duration]";
    listOfHeaders << "[Duration-Factor]";
    listOfHeaders << "[Monitor-1]";
    listOfHeaders << "[Monitor-2]";
    listOfHeaders << "[Monitor-3|Tr|ROI]";
    listOfHeaders << "[Comment1]";
    listOfHeaders << "[Comment2]";
    listOfHeaders << "[Detector-X || Beamcenter-X]";
    listOfHeaders << "[Detector-Y || Beamcenter-Y]";
    listOfHeaders << "[Detector-Angle-X]";
    listOfHeaders << "[Detector-Angle-Y]";
    listOfHeaders << "[Sample-Motor-1]";
    listOfHeaders << "[Sample-Motor-2]";
    listOfHeaders << "[Sample-Motor-3]";
    listOfHeaders << "[Sample-Motor-4]";
    listOfHeaders << "[Sample-Motor-5]";
    listOfHeaders << "[SA-Pos-X]";
    listOfHeaders << "[SA-Pos-Y]";
    listOfHeaders << "[Field-1]";
    listOfHeaders << "[Field-2]";
    listOfHeaders << "[Field-3]";
    listOfHeaders << "[Field-4]";
    listOfHeaders << "[RT-Number-Repetitions]";
    listOfHeaders << "[RT-Time-Factor]";
    listOfHeaders << "[RT-Current-Number]";
    listOfHeaders << "[Attenuator]";
    listOfHeaders << "[Polarization]";
    listOfHeaders << "[Lenses]";
    listOfHeaders << "[Slices-Count]";
    listOfHeaders << "[Slices-Duration]";
    listOfHeaders << "[Slices-Current-Number]";
    listOfHeaders << "[Slices-Current-Duration]";
    listOfHeaders << "[Slices-Current-Monitor1]";
    listOfHeaders << "[Slices-Current-Monitor2]";
    listOfHeaders << "[Slices-Current-Monitor3]";
    listOfHeaders << "[Slices-Current-Sum]";
}
void ParserHeader::dataFormatChanged(int format)
{
    HeaderFormat->setCurrentIndex(format);

    QStringList lst;
    switch (format)
    {
    case 0:
        lst << "#-Line"
            << "#-Pos";
        FLEXIgroup->show();
        XMLgroup->hide();
        break;
    case 1:
        lst << "XML-sequence"
            << "Attribute";
        FLEXIgroup->hide();
        XMLgroup->show();
        break;
    case 2:
        lst << "YAML-sequence"
            << "Attribute";
        FLEXIgroup->hide();
        XMLgroup->hide();
        break;
    case 3:
        lst << "HDF5-sequence"
            << "Attribute";
        FLEXIgroup->hide();
        XMLgroup->hide();
        break;
    default:
        break;
    }
    tableHeader->setHorizontalHeaderLabels(lst);
}