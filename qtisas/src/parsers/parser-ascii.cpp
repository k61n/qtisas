/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: ASCII parser
 ******************************************************************************/

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

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
