/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/

#include "dan-files-manager.h"

FilesManager::FilesManager(QLineEdit *pathInDan, QCheckBox *subFoldersActiveDan, QLineEdit *pathOutDan,
                           QLineEdit *wildCardDan, QLineEdit *wildCard2ndDan, QCheckBox *wildCard2ndActiveDan)
{
    pathIn = pathInDan;
    subFoldersActive = subFoldersActiveDan;
    pathOut = pathOutDan;
    wildCard = wildCardDan;
    wildCard2nd = wildCard2ndDan;
    wildCard2ndActive = wildCard2ndActiveDan;
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
        lst = Number.split("-", QString::SkipEmptyParts);
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
        lst = Number.split("-", QString::SkipEmptyParts);
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
        lst = Number.split("-", QString::SkipEmptyParts);
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