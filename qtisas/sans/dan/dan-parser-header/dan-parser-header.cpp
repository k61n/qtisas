/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Header::Map Parser used in DAN-SANS interface
 ******************************************************************************/

#include <QHeaderView>

#include <fstream>
#include <iostream>

#include "dan-parser-header.h"

ParserHeader::ParserHeader(FilesManager *filesManagerDan, QTableWidget *tableHeaderDan, QComboBox *HeaderFormatDan,
                           QGroupBox *XMLgroupDan, QLineEdit *XMLbaseDan, QGroupBox *FLEXIgroupDan,
                           QCheckBox *FLEXIcheckDan, QLineEdit *FLEXIlineDan, QSpinBox *HEADERlengthDan,
                           QSpinBox *HEADER2NDlengthDan, QSpinBox *HEADERDATAlengthDan, QCheckBox *removeNonePrintDan)
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
    removeNonePrint = removeNonePrintDan;
    // init list of Headers
    initListOfHeaders();
    // init Header Table
    tableHeader->setRowCount(listOfHeaders.count());
    tableHeader->setVerticalHeaderLabels(listOfHeaders);
    for (int i = 0; i < listOfHeaders.count(); i++)
    {
        tableHeader->setItem(i, 0, new QTableWidgetItem);
        tableHeader->setItem(i, 1, new QTableWidgetItem);
    }
    tableHeader->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect(HeaderFormat, SIGNAL(activated(int)), this, SLOT(dataFormatChanged(int)));
}
//+++ read a string from run Number
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
        num = "-1";
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
        std::cout << "file with run-Number: " << Number.toLocal8Bit().constData() << " does not exist!\n" << std::flush;
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
    listOfHeaders << "[Polarizer-Polarization]";
    listOfHeaders << "[Polarizer-Transmission]";
    listOfHeaders << "[Polarizer-Flipper-Efficiency]";
    listOfHeaders << "[Analyzer-Transmission]";
    listOfHeaders << "[Analyzer-Efficiency]";
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
//+++ read a Header as QStringList
bool ParserHeader::readHeader(const QString &fileName, const int &linesNumber, QStringList &header)
{
    bool removeNonePrintable = removeNonePrint->isChecked();
    bool flexiHeader = FLEXIcheck->isChecked();
    QStringList flexiStop = FLEXIline->text().split("|", Qt::SkipEmptyParts);

    header.clear();
    if (linesNumber <= 0)
        return true;

    QFile file(fileName);
    QTextStream t(&file);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    if (flexiHeader && flexiStop[0] != "")
    {
        QString s;
        int symbolsNumber;
        bool endReached = false;

        for (int i = 0; i < linesNumber; i++)
        {
            s = t.readLine();
            if (removeNonePrintable)
            {
                QString ss = s;
                s = "";
                int smax = ss.length();
                if (smax > 10000)
                    smax = 10000;
                for (int ii = 0; ii < smax; ii++)
                    if (ss[ii].isPrint())
                        s += ss[ii];
            }
            header << s;

            for (int iFlex = 0; iFlex < flexiStop.count(); iFlex++)
            {
                symbolsNumber = flexiStop[iFlex].length();
                if (s.left(symbolsNumber) == flexiStop[iFlex] || t.atEnd())
                {
                    endReached = true;
                    break; // skip header
                }
            }

            if (endReached)
                break;
            if (t.atEnd())
            {
                file.close();
                return false;
            }
        }
    }
    else
    {
        QString s = "";
        for (int i = 0; i < linesNumber; i++)
        {
            s = t.readLine();
            if (removeNonePrintable)
            {
                QString ss = s;
                s = "";
                int smax = ss.length();
                if (smax > 10000)
                    smax = 10000;
                for (int ii = 0; ii < smax; ii++)
                    if (ss[ii].isPrint())
                        s += ss[ii];
            }
            header << s;
            if (t.atEnd())
            {
                file.close();
                return true;
            }
        }
    }
    file.close();
    return true;
}
//+++ replaceHeaderValue
bool ParserHeader::replaceHeaderValue(const QString &Number, const QString &headerName, QStringList &lst,
                                      const QString &newValue)
{
    int indexInHeader = listOfHeaders.indexOf(headerName);
    if (indexInHeader < 0)
        return false;

    QString pos = tableHeader->item(indexInHeader, 0)->text();
    QString num = tableHeader->item(indexInHeader, 1)->text();

    if (pos == "const" || num == "#")
        return false;

    QString oldValue = readNumberString(Number, pos, num, lst);

    int numInt = num.toInt() - 1;
    int posInt = pos.toInt() - 1;
    if (numInt < 0 || posInt < 0 || posInt > lst.count())
        return false;

    QString oldLine = lst[posInt];
    numInt = oldLine.indexOf(oldValue, numInt);

    lst[posInt] = oldLine.replace(numInt, oldValue.length(), newValue);

    return true;
}
//+++ replace value
bool ParserHeader::replaceEntryYaml(const QString &fileNameString, const QString &headerName, bool numerical,
                                    const QString &newValue)
{
    int indexInHeader = listOfHeaders.indexOf(headerName);

    QString yamlCode = tableHeader->item(indexInHeader, 0)->text();
    yamlCode = yamlCode.remove(" ");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("::", ":");
    yamlCode = yamlCode.replace("||", "|");
    yamlCode = yamlCode.replace("||", "|");
    QStringList lst = yamlCode.split(":", Qt::SkipEmptyParts);
    int countLevels = lst.count();

    if (countLevels > 10)
        return "> 10 levels";

    QString postAction = tableHeader->item(indexInHeader, 1)->text();
    int listPos = 0;
    if (postAction.left(1) == "s")
    {
        listPos = postAction.right(postAction.length() - 2).toInt();
        listPos--;
    }

    std::ifstream fin(fileNameString.toLocal8Bit().constData());

    QString result = "";

    try
    {
        auto documents = YAML::LoadAll(fin);
        fin.close();
        std::ofstream fout(fileNameString.toLocal8Bit().constData());
        bool lastLevel;

        for (std::size_t docIndex = 0; docIndex < documents.size(); docIndex++)
        {
            YAML::Node doc = documents[docIndex];

            if (!ParserYAML::nodeModify(doc, lst, numerical, newValue, listPos))
                std::cout << "node does not exist: " << docIndex << " \n";

            fout << YAML::Dump(doc) << std::endl;

            continue;

            for (int levelIndex = 0; levelIndex < countLevels; levelIndex++)
            {
                lastLevel = (levelIndex == countLevels - 1) ? true : false;

                if (!ParserYAML::singleLevelParsing(doc, lst[levelIndex], lastLevel, result))
                    continue;

                if (countLevels == levelIndex + 1)
                    continue;

                fout << YAML::Dump(doc) << std::endl;
            }
        }
        fout.close();
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    return true;
}
