/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/

#include "dan-files-manager.h"

FilesManager::FilesManager(QLineEdit *pathInDan, QLineEdit *wildCardInDan, QCheckBox *subFoldersActiveDan,
                           QToolButton *buttonPathInDan, QLineEdit *pathOutDan, QToolButton *buttonPathOutDan,
                           QLineEdit *wildCardDan, QLineEdit *wildCard2ndDan, QCheckBox *wildCard2ndActiveDan)
{
    pathIn = pathInDan;
    wildCardIn = wildCardInDan;
    subFoldersActive = subFoldersActiveDan;
    buttonPathIn = buttonPathInDan;
    pathOut = pathOutDan;
    buttonPathOut = buttonPathOutDan;
    wildCard = wildCardDan;
    wildCard2nd = wildCard2ndDan;
    wildCard2ndActive = wildCard2ndActiveDan;

    // connect
    connect(buttonPathIn, SIGNAL(clicked()), this, SLOT(pushedPathIn()));
    connect(buttonPathOut, SIGNAL(clicked()), this, SLOT(pushedPathOut()));
}

//+++ SLOT: pathIn
bool FilesManager::pushedPathIn()
{
    QString path = pathIn->text();
    if (path.left(4) == "home")
        path = QDir::homePath();

    QString s = "";
    s = QFileDialog::getExistingDirectory(nullptr, "get 2D-data directory - Choose a directory", path);
    if (s == "")
        return false;
    if (s.right(1) != "/")
        s = s + "/";
    s = s.replace("\\", "/");

    pathIn->setText(s);

    QDir dd;
    if (dd.cd(s))
    {
        dd.cdUp();
        s = dd.absolutePath();
        if (s.right(1) != "/")
            s = s + "/";
    }
    pathOut->setText(s);
    return true;
}
//+++  SLOT: pathOut
bool FilesManager::pushedPathOut()
{
    QString path = pathOut->text();

    if (path.left(4) == "home")
        path = QDir::homePath();

    QString s = "";
    s = QFileDialog::getExistingDirectory(nullptr, "get 2D-data directory - Choose a directory", path);
    if (s == "")
        return false;

    if (s.right(1) != "/")
        s = s + "/";
    s = s.replace("\\", "/");

    pathOut->setText(s);
    return true;
}

QString FilesManager::wildCardHeader()
{
    if (wildCard2ndActive->isChecked())
        return wildCard2nd->text();
    return wildCard->text();
}

QString FilesManager::wildCardDetector()
{
    return wildCard->text();
}
QString FilesManager::fileName(QString Number, QString wildCardLocal, QString &subDir)
{
    //+++ Files Names have indexing: like dining interactions
    //+++ in this case Number and wildCard will have index within []
    QString index = "";
    if (Number.contains("["))
    {
        index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]");
        Number = Number.left(Number.indexOf("["));
        if (wildCardLocal.contains("["))
            wildCardLocal = wildCardLocal.left(wildCardLocal.indexOf("[")) + index;
    }
    //+++ if subFolders are allowed
    subDir = "";
    if (Number.contains("/") == 1)
    {
        subDir = Number.left(Number.indexOf("/") + 1);
        Number = Number.remove(subDir);
    }
    //+++ wildCald contains
    if (wildCardLocal.count("#") == 1)
    {
        //+++ #: 12345 / run number correspons to a single number in the file name
        wildCardLocal = wildCardLocal.replace("#", Number);
    }
    else if (wildCardLocal.count("#") == 2)
    {
        //+++ #-#: 12345-1 / two-number file structure
        QStringList lst;
        lst = Number.split("-", Qt::SkipEmptyParts);
        if (lst.count() == 2)
        {
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[0]);
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[1]);
        }
        else
        {
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[0]);
            wildCardLocal = wildCardLocal.replace("#", "*");
        }
    }
    else if (wildCardLocal.count("*") == 1)
    {
        //+++ *: 12345 / run number correspons to a single STRING in the file name
        wildCardLocal = wildCardLocal.replace("*", Number);
    }
    else
        return "";
    return wildCardLocal;
}
QString FilesManager::fileNameFull(const QString &Number, const QString &wildCardLocal)
{
    QString subDir;
    QString Name = fileName(Number, wildCardLocal, subDir);

    //+++ local file-name
    QDir d(pathIn->text() + subDir);
    QStringList lst = d.entryList(QStringList() << Name);
    if (lst.count() == 0)
        return "";
    QString file = lst.last();

    //+++ file full-name
    return pathIn->text() + subDir + file;
}
//+++ formation of the new (full) file name (used for addition of files)
QString FilesManager::newFileNameFull(const QString &Number, const QString &wildCardLocal)
{
    QString subDir;
    QString Name = fileName(Number, wildCardLocal, subDir);
    //+++ file full-name
    return pathOut->text() + Name;
}
//+++ checkFileNumber
bool FilesManager::checkFileNumber(QString Number)
{
    QString subFolder = "";
    if (subFoldersActive->isChecked())
    {
        if (Number.count("/") == 1)
        {
            subFolder = Number.left(Number.indexOf("/") + 1);
            Number = Number.remove(subFolder);
        }
        else if (Number.count("/") > 1)
            return false;
    }

    QString wildCardLocal = wildCardDetector();

    if (wildCardLocal.count("#") == 1)
        wildCardLocal = wildCardLocal.replace("#", Number);
    else if (wildCardLocal.count("#") == 2)
    {
        QStringList lst;
        lst = Number.split("-", Qt::SkipEmptyParts);
        if (lst.count() == 2)
        {
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[0]);
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[1]);
        }
        else
            return false;
    }
    else if (wildCardLocal.count("#") > 2)
        return "";
    else if (wildCardLocal.count("*") == 1)
    {
        if (wildCardLocal.contains("["))
        {
            QString index = nullptr;
            if (Number.contains("["))
            {
                index = Number.right(Number.length() - Number.indexOf("[")).remove("[").remove("]");
                Number = Number.left(Number.indexOf("["));
            }
            else
                return false;
            wildCardLocal = wildCardLocal.left(wildCardLocal.indexOf("[") + 1);
            wildCardLocal = wildCardLocal.replace("[", index);
        }
        wildCardLocal = wildCardLocal.replace("*", Number);
    }
    else
        return false;

    QDir d(pathIn->text() + subFolder);
    QStringList lst = d.entryList(QStringList() << wildCardLocal);

    if (lst.count() < 1)
        return false;
    if (!wildCard2ndActive->isChecked())
        return true;

    wildCardLocal = wildCard2nd->text();

    if (wildCard->text().count("#") == 1)
    {
        if (wildCardLocal.count("#") == 1)
            wildCardLocal = wildCardLocal.replace("#", Number);
        else
            return false;
    }
    else if (wildCard->text().count("#") == 2)
    {
        QStringList lst;
        lst = Number.split("-", Qt::SkipEmptyParts);
        if (lst.count() == 2)
        {
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[0]);
            wildCardLocal = wildCardLocal.replace(wildCardLocal.indexOf("#"), 1, lst[1]);
        }
        else
            return false;
    }
    else if (wildCard->text().count("*") == 1 && wildCardLocal.count("*") == 1)
        wildCardLocal = wildCardLocal.replace("*", Number);
    else
        return false;

    lst.clear();
    QDir dd(pathIn->text() + subFolder);
    lst = dd.entryList(QStringList() << wildCardLocal);

    if (lst.count() < 1)
        return false;
    return true;
}
//+++ find-File-Number-In-File-Name
QString FilesManager::findFileNumberInFileName(QString wildCardLocal, QString file)
{
    QString subFolder = "";
    if (file.indexOf("/") > 0)
    {
        subFolder = file.left(file.indexOf("/") + 1);
        file = file.remove(subFolder);
    }

    if (wildCardLocal.count("#") == 1)
    {
        if (wildCardLocal.contains("."))
            wildCardLocal = wildCardLocal.left(wildCardLocal.indexOf(".") + 1);
        if (wildCardLocal.indexOf("#") < wildCardLocal.indexOf("*"))
            wildCardLocal = wildCardLocal.replace("*", "(.+)");
        else
            wildCardLocal = wildCardLocal.remove("*");
        wildCardLocal = wildCardLocal.replace("#", "(\\d+)");

        QRegExp rxF(wildCardLocal);
        int pos = 0;
        pos = rxF.indexIn(file, pos);
        if (pos < 0)
            return "";
        file = rxF.cap(1);

        QRegExp rxF1("(\\d+)");
        pos = 0;
        pos = rxF1.indexIn(file, pos);
        if (pos < 0)
            return "";

        return subFolder + rxF1.cap(1);
    }
    else if (wildCardLocal.count("#") == 2)
    {
        QString wildCardLocal2nd = wildCardLocal;
        QString file2nd = file;
        QStringList lst;
        QString res;

        wildCardLocal = wildCardLocal.left(wildCardLocal.lastIndexOf("#"));
        if (wildCardLocal.contains("*"))
        {
            lst = wildCardLocal.split("*", Qt::SkipEmptyParts);
            for (int i = 0; i < lst.count(); i++)
                if (lst[i].contains("#"))
                {
                    wildCardLocal = lst[i];
                    break;
                }
        }

        wildCardLocal = wildCardLocal.replace("#", "(\\d+)");

        QRegExp rxF(wildCardLocal);
        int pos = 0;
        pos = rxF.indexIn(file, pos);
        if (pos < 0)
            return "";

        file = rxF.cap(1);
        QRegExp rxF1("(\\d+)");
        pos = 0;
        pos = rxF1.indexIn(file, pos);
        if (pos < 0)
            return "";

        res = rxF1.cap(1);
        wildCardLocal = wildCardLocal2nd;
        file = file2nd;
        wildCardLocal = wildCardLocal.right(wildCardLocal.length() - wildCardLocal.indexOf("#") - 1);
        if (wildCardLocal.contains("*"))
        {
            lst = wildCardLocal.split("*", Qt::SkipEmptyParts);
            for (int i = 0; i < lst.count(); i++)
                if (lst[i].contains("#"))
                {
                    wildCardLocal = lst[i];
                    break;
                }
        }

        wildCardLocal = wildCardLocal.replace("#", "(\\d+)");
        QRegExp rxF2nd(wildCardLocal);
        pos = 0;
        pos = rxF2nd.indexIn(file, pos);
        if (pos < 0)
            return "";

        file = rxF2nd.cap(1);
        QRegExp rxF12nd("(\\d+)");
        pos = 0;
        pos = rxF12nd.indexIn(file, pos);
        if (pos < 0)
            return "";

        res += "-" + rxF12nd.cap(1);

        return subFolder + res;
    }
    else if (wildCardLocal.count("#") > 2)
        return "";

    if (wildCardLocal.count("*") == 1)
    {
        if (wildCardLocal.contains("[0-9]"))
        {
            QString wildCard09 = wildCardLocal;
            wildCard09 = wildCard09.replace("[0-9]", "[0-9][0-9][0-9][0-9]");
            QRegExp rx0(wildCard09);
            bool definedWildcard = false;
            int number = 4;
            rx0.setPatternSyntax(QRegExp::Wildcard);
            if (rx0.exactMatch(file))
                definedWildcard = true;

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9][0-9][0-9]");
                rx0.setPattern(wildCard09);
                number = 3;
                if (rx0.exactMatch(file))
                    definedWildcard = true;
            }

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9][0-9]");
                rx0.setPattern(wildCard09);
                if (rx0.exactMatch(file))
                    definedWildcard = true;
                number = 2;
            }

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9]");
                rx0.setPattern(wildCard09);
                if (rx0.exactMatch(file))
                    definedWildcard = true;
                number = 1;
            }

            if (!definedWildcard)
                return "";

            if (wildCard09.indexOf("*") < wildCard09.indexOf("[0-9]"))
            {
                file = file.right(file.length() - wildCard09.indexOf("*"));
                wildCard09 = wildCard09.right(wildCard09.length() - wildCard09.indexOf("*"));
            }
            else
            {
                file = file.right(file.length() - wildCard09.indexOf("[0-9]"));
                wildCard09 = wildCard09.right(wildCard09.length() - wildCard09.indexOf("[0-9]"));
            }

            if (wildCard09.lastIndexOf("[0-9]") < wildCard09.length() - 5)
            {
                if (wildCard09.indexOf("*") > wildCard09.lastIndexOf("[0-9]"))
                {
                    file = file.left(file.length() - (wildCard09.length() - wildCard09.indexOf("*") - 1));
                    wildCard09 = wildCard09.left(wildCard09.indexOf("*") + 1);
                }
                else
                {
                    file = file.left(file.length() - (wildCard09.length() - wildCard09.lastIndexOf("[0-9]") - 1 - 4));
                    wildCard09 = wildCard09.left(wildCard09.lastIndexOf("[0-9]") + 1 + 4);
                }
            }

            QString indexing;
            if (wildCard09.indexOf("*") == 0)
            {
                indexing = file.right(number);
                file = file.left(file.length() - number - wildCard09.remove("*").remove("[0-9]").length());
            }
            else
            {
                indexing = file.left(number);
                file = file.right(file.length() - number - wildCard09.remove("*").remove("[0-9]").length());
            }
            file = file.remove("*");
            return subFolder + file + "[" + indexing + "]";
        }
        else
        {
            QRegExp rx0(wildCardLocal);
            rx0.setPatternSyntax(QRegExp::Wildcard);
            if (rx0.exactMatch(file))
            {
                if (wildCardLocal.indexOf("*") > 0)
                    file = file.right(file.length() - wildCardLocal.indexOf("*"));
                wildCardLocal = wildCardLocal.right(wildCardLocal.length() - wildCardLocal.indexOf("*") - 1);
                file = file.left(file.length() - wildCardLocal.length());
                return subFolder + file;
            }
        }
    }
    return "";
}
