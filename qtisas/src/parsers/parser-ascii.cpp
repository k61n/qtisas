/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: ASCII parser
 ******************************************************************************/

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <sstream>
#include <zlib.h>

#include "parser-ascii.h"

//++++++++++++++++++++++++++++++++++++
//+++ File: {Flexi Parser}
//++++++++++++++++++++++++++++++++++++
QString ParserASCII::readEntryFlexy(const QString &fileName, QString &code, int &startingPosition,
                                    int maxLinesNumberInHeader)
{
    startingPosition = 0;
    code = code.trimmed();
    int shiftLine = 0;

    if (code.contains("-{"))
        shiftLine--;
    if (code.contains("--{"))
        shiftLine--;
    if (code.contains("---{"))
        shiftLine--;

    code = code.remove("---{").remove("--{").remove("-{");

    if (code.contains("}+"))
        shiftLine++;
    if (code.contains("}++"))
        shiftLine++;
    if (code.contains("}+++"))
        shiftLine++;

    code = code.remove("}+++").remove("}++").remove("}+");

    code = code.remove("{").remove("}");
    int codeLength = code.length();

    if (code == "")
        return "";

    QFile file(fileName);
    QTextStream t(&file);

    if (!file.open(QIODevice::ReadOnly))
        return "no-file";

    QString s = "";
    QString sss = "";

    int position;
    QString lineMinus1 = "";
    QString lineMinus2 = "";
    QString lineMinus3 = "";

    QString resultLine = "";
    int skip = 0;
    while (skip < maxLinesNumberInHeader)
    {
        lineMinus3 = lineMinus2;
        lineMinus2 = lineMinus1;
        lineMinus1 = s;
        s = t.readLine();
        skip++;

        if (s.contains(code))
        {
            position = s.indexOf(code);
            sss = s.mid(position - 1, 1);

            if (position > 0 && sss != " " && sss != "," && sss != ";")
                continue;

            switch (shiftLine)
            {
            case -3:
                resultLine = lineMinus3;
                skip--;
                skip--;
                skip--;
                break;
            case -2:
                resultLine = lineMinus2;
                skip--;
                skip--;
                break;
            case -1:
                resultLine = lineMinus1;
                skip--;
                break;
            case 0:
                resultLine = s.right(s.length() - position - code.length());
                startingPosition = static_cast<int>(position + code.length());
                break;
            case 1:
                if (skip < maxLinesNumberInHeader)
                {
                    resultLine = t.readLine();
                    skip++;
                }
                else
                {
                    resultLine = s.right(s.length() - position - code.length());
                    startingPosition = static_cast<int>(position + code.length());
                }
                break;
            case 2:
                if (skip + 1 < maxLinesNumberInHeader)
                {
                    resultLine = t.readLine();
                    skip++;
                    resultLine = t.readLine();
                    skip++;
                }
                else
                {
                    resultLine = s.right(s.length() - position - code.length());
                    startingPosition = static_cast<int>(position + code.length());
                }
                break;
            case 3:
                if (skip + 2 < maxLinesNumberInHeader)
                {
                    resultLine = t.readLine();
                    skip++;
                    resultLine = t.readLine();
                    skip++;
                    resultLine = t.readLine();
                    skip++;
                }
                else
                {
                    resultLine = s.right(s.length() - position - code.length());
                    startingPosition = static_cast<int>(position + code.length());
                }
                break;
            default:
                break;
            }
            break;
        }
    }
    file.close();
    code = QString::number(skip--);
    return resultLine;
}
//++++++++++++++++++++++++++++++++++++
//+++ File: Standard parser
//++++++++++++++++++++++++++++++++++++
QString ParserASCII::readEntry(const QString &fileName, QString line, const QStringList &lst)
{
    line = line.trimmed();
    int lineInt = line.toInt();

    if (lineInt <= 0)
        lineInt = 1;

    if (lst.count() >= lineInt)
        return lst[lineInt - 1];

    QFile file(fileName);
    QTextStream t(&file);

    if (!file.open(QIODevice::ReadOnly))
        return "";

    for (int skip = 0; skip < lineInt; skip++)
        line = t.readLine();

    file.close();

    return line;
}
//++++++++++++++++++++++++++++++++++++
//+++ QString parser
//++++++++++++++++++++++++++++++++++++
QString ParserASCII::stringParser(QString initText, QString &action)
{
    if (initText == "")
        return "";
    if (action == "")
        return initText;

    QString res = initText;
    QString num = action;

    //--- initText=exam[11 22 33 44]ple; action=[s 2]; result=22
    if (num.contains("[s"))
    {
        num = num.remove("[").remove("]");
        res = res.mid(res.indexOf("[") + 1, res.indexOf("]") - res.indexOf("[") - 1);

        QString sep = num.mid(1, 1);
        num = num.right(num.length() - 2);
        if (num.toInt() > 0)
            return findStringInString(res, num.toInt(), sep, action);
        action = "-1";
        return "";
    }

    //--- initText=exam(11-22-33-44)ple; action=(s-3); result=33
    if (num.contains("(s"))
    {
        num = num.remove("(").remove(")");
        res = res.mid(res.indexOf("(") + 1, res.indexOf(")") - res.indexOf("(") - 1);
        QString sep = num.mid(1, 1);

        num = num.right(num.length() - 2);
        if (num.toInt() > 0)
            return findStringInString(res, num.toInt(), sep, action);
        action = "-1";
        return "";
    }
    //--- initText=exam{11!22!33!44}ple; action={s!4}; result=44
    if (num.contains("{s"))
    {
        num = num.remove("{").remove("}");
        res = res.mid(res.indexOf("{") + 1, res.indexOf("}") - res.indexOf("{") - 1);
        QString sep = num.mid(1, 1);
        num = num.right(num.length() - 2);
        if (num.toInt() > 0)
            return findStringInString(res, num.toInt(), sep, action);
        action = "-1";
        return "";
    }
    //--- initText=11 22 33 44; action=s 1; result=11
    if (num.contains("s"))
    {
        QString sep = num.mid(1, 1);
        num = num.right(num.length() - 2);
        if (num.toInt() > 0)
            return findStringInString(res, num.toInt(), sep, action);
        action = "-1";
        return "";
    }
    //--- initText=example; action=2-4; result=nit
    if (action.contains("-"))
    {
        return initText.mid(action.split("-")[0].toInt() - 1,
                            action.split("-")[1].toInt() - action.split("-")[0].toInt() + 1);
    }
    return findNumberInString(initText, num.toInt(), action);
}
//++++++++++++++++++++++++++++++++++++
//+++ Find a number (#digitNumber) in Qstring(line)
//++++++++++++++++++++++++++++++++++++
QString ParserASCII::findNumberInString(QString line, int digitNumber, QString &num)
{
    QString result;
    int posInline = 0;
    num = "0";
    int currentNumber = 0;
    static const QRegularExpression rx(R"(((\-|\+)?\d\d*(\.\d*)?((E\-|E\+)\d\d?\d?\d?)?))");

    if (line.contains(".") && line.contains(","))
        line.replace(",", ";");
    line.replace(",", ".");
    line.replace("e", "E");
    line.replace("E", "E0");
    line.replace("E0+", "E+0");
    line.replace("E0-", "E-0");
    line.replace("E0", "E+0");
    line.replace("E+00", "E+0");
    line.replace("E-00", "E-0");

    QString alloweddSymbols = " :;)([]_";
    alloweddSymbols += '\t';

    QRegularExpressionMatchIterator i = rx.globalMatch(line);
    while (i.hasNext() && currentNumber < digitNumber)
    {
        QRegularExpressionMatch match = i.next();
        posInline = static_cast<int>(match.capturedStart());
        result = match.captured(1);
        if ((match.capturedStart() == 0 || alloweddSymbols.contains(line[match.capturedStart() - 1])) &&
            (match.capturedEnd() == line.length() || (alloweddSymbols + '-').contains(line[match.capturedEnd()])))
            currentNumber++;
    }
    posInline++;
    num = QString::number(posInline);
    return result;
}
//++++++++++++++++++++++++++++++++++++
//+++ Find a number (#digitNumber) in Qstring(line)
//++++++++++++++++++++++++++++++++++++
QString ParserASCII::findStringInString(const QString &initialString, int stringPosisionInString,
                                        const QString &separationSymbol, QString &indexOfString)
{
    indexOfString = "";
    QStringList lst = initialString.split(separationSymbol, Qt::SkipEmptyParts);
    if (stringPosisionInString == 0)
        stringPosisionInString++;
    if (stringPosisionInString > lst.count() || stringPosisionInString < 0)
        return "";

    int sum = 1;
    for (int i = 0; i < stringPosisionInString - 1; i++)
        sum += lst[i].length() + 1;
    indexOfString = QString::number(sum); // position of the string

    return lst[stringPosisionInString - 1];
}

//+++ Helper function to skip header
void ParserASCII::skipHeader(QTextStream &stream, int linesInHeader, bool flexiHeader, const QStringList &flexiStop)
{
    if (flexiHeader && !flexiStop.isEmpty() && !flexiStop[0].isEmpty())
    {
        for (int i = 0; i < linesInHeader; ++i)
        {
            QString line = stream.readLine();
            bool stopFound = std::any_of(flexiStop.begin(), flexiStop.end(),
                                         [&](const QString &stop) { return line.startsWith(stop); });
            if (stopFound || stream.atEnd())
                break;
        }
    }
    else
    {
        for (int i = 0; i < linesInHeader; ++i)
            stream.readLine();
    }
}
//+++ FUNCTIONS::Read-DAT-files:: Matrix ouble
bool ParserASCII::readMatrixByNameOne(const QString &fn, int DD, gsl_matrix *&data, int linesInHeader, bool flexiHeader,
                                      const QStringList &flexiStop)
{
    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Cannot open file:" << fn;
        return false;
    }
    QTextStream t(&file);
    skipHeader(t, linesInHeader, flexiHeader, flexiStop);

    int currentX = 0;
    int currentY = 0;
    QString s;

    // Read matrix
    for (int i = 0; i < DD * DD; i++)
    {
        s = t.readLine().simplified();
        gsl_matrix_set(data, currentY, currentX, s.toDouble());
        currentX++;

        if (currentX >= DD)
        {
            currentX = 0;
            currentY++;
        }
    }

    file.close();

    return true;
}
//+++ Read GSL-formatted matrix
bool ParserASCII::readMatrixByNameGSL(const QString &fileName, gsl_matrix *&data)
{
    if (!data)
    {
        qWarning() << "Null matrix passed to readMatrixByNameGSL()";
        return false;
    }

    FILE *f = fopen(fileName.toLocal8Bit().constData(), "r");
    if (!f)
    {
        qWarning() << "Cannot open file:" << fileName;
        return false;
    }

    gsl_set_error_handler_off();
    int status = gsl_matrix_fscanf(f, data);
    fclose(f);

    if (status != GSL_SUCCESS)
    {
        qWarning() << "gsl_matrix_fscanf failed for" << fileName << "GSL error:" << status;
        return false;
    }

    return true;
}
//+++ Read ascii matrix with specified pixels per line
bool ParserASCII::readMatrixByName(const QString &fn, int DD, int pixelPerLine, gsl_matrix *&data, int linesInHeader,
                                   bool flexiHeader, const QStringList &flexiStop)
{
    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Cannot open file:" << fn;
        return false;
    }
    QTextStream t(&file);
    skipHeader(t, linesInHeader, flexiHeader, flexiStop);

    int totalElements = DD * DD;
    int fullLines = totalElements / pixelPerLine;
    int remainder = totalElements % pixelPerLine;

    QStringList lst;

    int errorsCounter = 0;
    int currentX = 0;
    int currentY = 0;

    auto parseLine = [](const QString &line) -> QStringList {
        QString s = line;
        s.replace(",", " ");
        s = s.simplified();
        return s.split(" ", Qt::SkipEmptyParts);
    };

    for (int i = 0; i < fullLines; i++)
    {
        lst.clear();
        lst = parseLine(t.readLine());

        if (lst.size() != pixelPerLine)
        {
            qWarning() << "File:" << fn << "has error at line" << i << ". Numbers:" << lst.size();
            errorsCounter++;
            if (errorsCounter >= DD)
            {
                file.close();
                return false;
            }
            --i;
            continue;
        }

        for (const QString &numStr : lst)
        {
            gsl_matrix_set(data, currentY, currentX, numStr.toDouble());
            if (++currentX >= DD)
            {
                currentX = 0;
                ++currentY;
            }
        }
    }

    if (remainder > 0)
    {
        QStringList lst = parseLine(t.readLine());
        for (int j = 0; j < remainder; ++j)
            gsl_matrix_set(data, currentY, currentX++, lst[j].toDouble());
    }

    file.close();

    return true;
}
//+++ gzipped ascii matrix reader
bool ParserASCII::readMatrixByNameGZipped(const QString &fn, int DD, int ROI, gsl_matrix *&matrix)
{
    gzFile fd = gzopen(fn.toLocal8Bit().constData(), "rb");
    if (!fd)
    {
        qWarning("Failed to open gz file");
        return false;
    }

    const int chunkSize = 8192;
    char chunk[chunkSize];

    auto readLongLine = [&](std::string &out) -> bool {
        out.clear();
        for (;;)
        {
            if (!gzgets(fd, chunk, chunkSize))
                return false;
            out += chunk;
            if (out.empty())
                return false;
            if (out.back() == '\n')
                return true; // end of line
        }
    };

    int xOffset = (DD - ROI) / 2;
    int yOffset = (DD - ROI) / 2;

    std::string line;

    for (int yy = 0; yy < ROI; ++yy)
    {
        if (!readLongLine(line))
        {
            qWarning() << "Error reading long line" << yy;
            gzclose(fd);
            return false;
        }

        std::istringstream iss(line);
        for (int xx = 0; xx < ROI; ++xx)
        {
            double val;
            if (!(iss >> val))
            {
                qWarning() << "Error parsing number at row" << yy << "col" << xx;
                gzclose(fd);
                return false;
            }
            gsl_matrix_set(matrix, yOffset + yy, xOffset + xx, val);
        }
    }

    gzclose(fd);
    return true;
}