/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Header::Map Parser used in DAN-SANS interface
 ******************************************************************************/

#include <QHeaderView>
#include <QSaveFile>

#include <cmath>

#include <fstream>
#include <iostream>

#include "dan-parser-header.h"
#include "parser-edf.h"

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
    int startingPosition = 0;
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
            result = ParserASCII::readEntryFlexy(fileName, FLEXYcode, startingPosition, maxLinesNumberInHeader);
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
    num = QString::number(postParserAction.toInt() + startingPosition);
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
//+++ the lines an ASCII header has (Flexi: the search goes that far)
int ParserHeader::linesInHeader() const
{
    if (filesManager->secondHeaderYN())
        return HEADER2NDlength->value();
    return HEADERlength->value() + HEADERDATAlength->value();
}
//+++ an ASCII/Flexi entry: its line (1, 2, ...), the column it starts at and the entry (Flexi: the rest of the line
//+++ after the searched text, or the whole line before/after it); false: not found
bool ParserHeader::asciiEntry(const QString &fileName, const QString &pos, const QStringList &lst, int &line,
                              int &start, QString &entry) const
{
    start = 0;
    if (pos.contains("{") && pos.contains("}"))
    {
        QString code = pos;
        entry = ParserASCII::readEntryFlexy(fileName, code, start, linesInHeader());
        line = code.toInt();
        return !entry.isEmpty() && line >= 1;
    }
    line = pos.trimmed().toInt();
    if (line <= 0)
        line = 1;
    entry = ParserASCII::readEntry(fileName, pos, lst);
    return true;
}
//+++ the same text or the same number
static bool sameValue(const QString &a, const QString &b)
{
    if (a.trimmed() == b.trimmed())
        return true;
    bool okA = false;
    bool okB = false;
    const double x = a.trimmed().toDouble(&okA);
    const double y = b.trimmed().toDouble(&okB);
    return okA && okB && std::fabs(x - y) <= 1e-12 * std::fmax(1.0, std::fabs(x));
}
//+++ readNumberString() the other way: the run's header file gets newValue where the header map reads headerName;
//+++ read back to check, the file as it was when that fails; error: why not
bool ParserHeader::setNumberString(const QString &Number, const QString &headerName, const QString &newValue,
                                   QString &error)
{
    const int index = static_cast<int>(listOfHeaders.indexOf(headerName));
    if (index < 0)
    {
        error = "no header item " + headerName;
        return false;
    }
    const QString pos = tableHeader->item(index, 0)->text().trimmed();
    const QString num = tableHeader->item(index, 1)->text().trimmed();
    if (num == "#" || headerName == "[Sample-Run-Number]")
    {
        error = headerName + " is the run number: it names the run's files";
        return false;
    }
    if (pos.isEmpty())
    {
        error = headerName + " is not in the instrument's header map";
        return false;
    }
    if (pos.contains("const"))
    {
        error = headerName + " is a constant of the instrument (Config.setParameters()), not in the header";
        return false;
    }
    if (Number.contains("["))
    {
        error = "run " + Number + ": a frame of a multi-frame run";
        return false;
    }
    const QString fileName = filesManager->fileNameFull(Number, filesManager->wildCardHeader());
    if (fileName.isEmpty())
    {
        error = "no header file of run " + Number;
        return false;
    }

    QFile original(fileName);
    if (!original.open(QIODevice::ReadOnly))
    {
        error = "cannot read " + fileName;
        return false;
    }
    const QByteArray originalData = original.readAll();
    original.close();

    bool written = false;
    bool ok = false;
    switch (HeaderFormat->currentIndex())
    {
    case 0: {
        int line = 0;
        int start = 0;
        QString entry;
        if (!asciiEntry(fileName, pos, QStringList(), line, start, entry))
        {
            error = headerName + " (" + pos + ") not found in " + fileName;
            return false;
        }
        const QString fullLine = ParserASCII::readEntry(fileName, QString::number(line));
        const QString newLine = fullLine.left(start) + ParserASCII::stringReplace(entry, num, newValue, ok);
        if (!ok)
        {
            error = headerName + ": \"" + num + "\" selects nothing in \"" + entry + "\"";
            return false;
        }
        // EDF: the header block of a binary file, its size kept (ParserEDF pads it)
        if (fileName.endsWith(".edf", Qt::CaseInsensitive))
            written = ParserEDF::replaceHeader(fileName, {newLine});
        else
            written = ParserASCII::setEntry(fileName, line, newLine);
        break;
    }
    case 1: {
        // a post-action ("s 1", ...): a part of the element's text
        QString value = newValue;
        if (!num.isEmpty())
        {
            const QString oldValue = ParserXML::readEntry(fileName, pos, XMLbase->text());
            value = ParserASCII::stringReplace(oldValue, num, newValue, ok);
            if (!ok)
            {
                error = headerName + ": \"" + num + "\" selects nothing in \"" + oldValue + "\"";
                return false;
            }
        }
        written = ParserXML::setEntry(fileName, pos, XMLbase->text(), value);
        break;
    }
    case 2: {
        // a list ("a; b; c") and "s ...": its element; another post-action: a part of the value
        const QString oldValue = ParserYAML::readEntry(fileName, pos);
        int element = 0;
        QString value = newValue;
        if (num.startsWith("s") && oldValue.contains("; "))
            element = num.mid(2).toInt() - 1;
        else if (!num.isEmpty())
        {
            value = ParserASCII::stringReplace(oldValue, num, newValue, ok);
            if (!ok)
            {
                error = headerName + ": \"" + num + "\" selects nothing in \"" + oldValue + "\"";
                return false;
            }
        }
        bool numberBefore = false;
        bool numberAfter = false;
        (void)oldValue.toDouble(&numberBefore);
        (void)value.toDouble(&numberAfter);
        written = ParserYAML::setEntry(fileName, pos, value, numberBefore && numberAfter, element < 0 ? 0 : element);
        break;
    }
    case 3: {
        // the element part of the action ("[2]") is ParserHDF5's; a post-action ("s 1") a part of the value
        QString postAction = num;
        const QString oldValue = ParserHDF5::readEntry(fileName, pos, postAction);
        QString value = newValue;
        if (!postAction.isEmpty())
        {
            value = ParserASCII::stringReplace(oldValue, postAction, newValue, ok);
            if (!ok)
            {
                error = headerName + ": \"" + postAction + "\" selects nothing in \"" + oldValue + "\"";
                return false;
            }
        }
        written = ParserHDF5::setEntry(fileName, pos, num, value);
        break;
    }
    default:
        error = "header values of the " + HeaderFormat->currentText().simplified() + " are not written yet";
        return false;
    }

    QString check = num;
    QString readPos = pos;
    if (written && sameValue(readNumberString(Number, readPos, check), newValue))
        return true;

    QSaveFile restore(fileName);
    if (restore.open(QIODevice::WriteOnly) && restore.write(originalData) == originalData.size())
        restore.commit();
    error = written ? headerName + ": the value read back is not " + newValue + "; " + fileName + " left as it was"
                    : headerName + " (" + pos + ") not found or not writable in " + fileName;
    return false;
}
//+++ replaceHeaderValue: in the lines of a header read before (a new file's)
bool ParserHeader::replaceHeaderValue(const QString &Number, const QString &headerName, QStringList &lst,
                                      const QString &newValue)
{
    const int index = static_cast<int>(listOfHeaders.indexOf(headerName));
    if (index < 0)
        return false;
    const QString pos = tableHeader->item(index, 0)->text().trimmed();
    const QString num = tableHeader->item(index, 1)->text().trimmed();
    if (pos.isEmpty() || pos.contains("const") || num == "#")
        return false;

    int line = 0;
    int start = 0;
    QString entry;
    const QString fileName = filesManager->fileNameFull(Number, filesManager->wildCardHeader());
    if (!asciiEntry(fileName, pos, lst, line, start, entry) || line > lst.count())
        return false;
    bool ok = false;
    const QString newLine = lst[line - 1].left(start) + ParserASCII::stringReplace(entry, num, newValue, ok);
    return ok && ParserASCII::setEntry(lst, line, newLine);
}
//+++ replace value: a YAML header file (a new file's)
bool ParserHeader::replaceEntryYaml(const QString &fileNameString, const QString &headerName, bool numerical,
                                    const QString &newValue)
{
    const int index = static_cast<int>(listOfHeaders.indexOf(headerName));
    if (index < 0)
        return false;
    const QString postAction = tableHeader->item(index, 1)->text();
    int listPos = 0;
    if (postAction.left(1) == "s")
        listPos = postAction.right(postAction.length() - 2).toInt() - 1;
    return ParserYAML::setEntry(fileNameString, tableHeader->item(index, 0)->text(), newValue, numerical,
                                listPos < 0 ? 0 : listPos);
}
