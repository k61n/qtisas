/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/

#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDebug>

#include "dan-files-manager.h"

FilesManager::FilesManager(QLineEdit *pathInDan, QLineEdit *wildCardInDan, QCheckBox *subFoldersActiveDan,
                           QToolButton *buttonPathInDan, QLineEdit *pathOutDan, QToolButton *buttonPathOutDan,
                           QLineEdit *wildCardDan, QLineEdit *wildCard2ndDan, QCheckBox *wildCard2ndActiveDan,
                           QLineEdit *textEditPatternDan)
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
    textEditPattern = textEditPatternDan;

    connect(buttonPathIn, &QToolButton::clicked, this, [this]() { pushedPathIn(); });
    connect(buttonPathOut, &QToolButton::clicked, this, [this]() { pushedPathOut(); });
}
// +++ fileByStarWildcard
QString FilesManager::fileByStarWildcard(const QString &wildcard, const QString &subDir)
{
    QDir dir(pathIn->text() + "/" + subDir);
    if (!dir.exists())
    {
        qWarning() << "Directory does not exist:" << dir.path();
        return {};
    }

    dir.setNameFilters(QStringList() << wildcard);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QStringList matches = dir.entryList();
    if (matches.count() > 0)
        return matches[0];

    return {};
}
QString normalizePath(const QString &p)
{
    QString out = QDir::fromNativeSeparators(p).trimmed();
    if (!out.endsWith('/'))
        out += '/';
    return out;
}
// +++ pathIn
bool FilesManager::pushedPathIn(QString path)
{
    if (path.isEmpty() || !QDir(path).exists())
    {
        path = pathIn->text().trimmed();

        if (path.startsWith("home"))
            path = QDir::homePath();

        path = QFileDialog::getExistingDirectory(nullptr, tr("Get 2D-data directory - Choose a directory"), path);

        if (path.isEmpty())
            return false;
    }

    QDir dir(path);
    path = normalizePath(dir.absolutePath());
    pathIn->setText(path);

    if (dir.cdUp())
        path = normalizePath(dir.absolutePath());
    pathOut->setText(path);

    return true;
}
// +++ pathOut
bool FilesManager::pushedPathOut(QString path)
{
    if (path.isEmpty() || !QDir(path).exists())
    {
        path = pathOut->text().trimmed();

        if (path.startsWith("home"))
            path = QDir::homePath();

        path = QFileDialog::getExistingDirectory(nullptr, tr("Get 2D-data directory - Choose a directory"), path);

        if (path.isEmpty())
            return false;
    }

    QDir dir(path);
    path = normalizePath(dir.absolutePath());
    pathOut->setText(path);

    return true;
}
// +++ wildCardHeader
QString FilesManager::wildCardHeader()
{
    if (wildCard2ndActive->isChecked())
        return wildCard2nd->text();
    return wildCard->text();
}
// +++ wildCardDetector
QString FilesManager::wildCardDetector()
{
    return wildCard->text();
}
QString FilesManager::fileName(QString Number, QString wildCardLocal, QString &subDir)
{
    // +++ Files Names have indexing
    // +++ in this case Number and wildCard will have index within []
    QRegularExpression re(R"(\[([^\]]+)\])");
    auto m = re.match(Number);
    if (m.hasMatch())
    {
        QString index = m.captured(1);
        Number.remove(re);
        wildCardLocal.replace(re, index);
    }
    //+++ if subFolders are allowed
    subDir = QString();
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

    if (wildCardLocal.contains("*"))
        wildCardLocal = fileByStarWildcard(wildCardLocal, subDir);

    return wildCardLocal;
}
// +++ fileNameFull
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

        QRegularExpression rxF(wildCardLocal);
        if (!rxF.match(file).hasMatch())
            return {};
        file = rxF.match(file).captured(1);

        static const QRegularExpression rxF1("(\\d+)");
        if (!rxF1.match(file).hasMatch())
            return {};

        return subFolder + rxF1.match(file).captured(1);
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

        QRegularExpression rxF(wildCardLocal);
        if (!rxF.match(file).hasMatch())
            return {};

        file = rxF.match(file).captured(1);
        static const QRegularExpression rxF1("(\\d+)");
        if (!rxF1.match(file).hasMatch())
            return {};

        res = rxF1.match(file).captured(1);
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
        QRegularExpression rxF2nd(wildCardLocal);
        if (!rxF2nd.match(file).hasMatch())
            return {};

        file = rxF2nd.match(file).captured(1);
        static const QRegularExpression rxF12nd("(\\d+)");
        if (!rxF12nd.match(file).hasMatch())
            return {};

        res += "-" + rxF12nd.match(file).captured(1);

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
            QRegularExpression rx0(QRegularExpression::wildcardToRegularExpression(wildCard09));
            bool definedWildcard = false;
            int number = 4;
            if (rx0.match(file).hasMatch())
                definedWildcard = true;

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9][0-9][0-9]");
                rx0.setPattern(wildCard09);
                number = 3;
                if (rx0.match(file).hasMatch())
                    definedWildcard = true;
            }

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9][0-9]");
                rx0.setPattern(wildCard09);
                if (rx0.match(file).hasMatch())
                    definedWildcard = true;
                number = 2;
            }

            if (!definedWildcard)
            {
                wildCard09 = wildCardLocal;
                wildCard09 = wildCard09.replace("[0-9]", "[0-9]");
                rx0.setPattern(wildCard09);
                if (rx0.match(file).hasMatch())
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
            QRegularExpression rx0(QRegularExpression::wildcardToRegularExpression(wildCardLocal));
            if (rx0.match(file).hasMatch())
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
//+++
bool FilesManager::selectFile(QString &fileNumber)
{
    QString Dir = pathInString();
    QString filter = textEditPattern->text();
    QString wildCard = wildCardDetector();
    bool dirsInDir = subFoldersYN();

    auto *fd = new QFileDialog(nullptr, "Choose a file", Dir, "*");

    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFile);
    fd->setWindowTitle(tr("DAN - Getting File Information"));
    fd->setNameFilter(filter + ";;" + textEditPattern->text());
    foreach (QComboBox *obj, fd->findChildren<QComboBox *>())
        if (QString(obj->objectName()).contains("fileTypeCombo"))
            obj->setEditable(true);

    if (fd->exec() == QDialog::Rejected)
        return false;

    QStringList selectedDat = fd->selectedFiles();

    if (selectedDat.count() == 0)
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Nothing was selected");
        return false;
    }

    fileNumber = selectedDat[0];
    fileNumber = fileNumber.replace('\\', '/');
    if (fileNumber.contains(Dir))
    {
        if (Dir.right(1) == "/")
            fileNumber = fileNumber.remove(Dir);
        else
            fileNumber = fileNumber.remove(Dir + "/");

        if (!dirsInDir)
        {
            if (fileNumber.contains("/") || fileNumber.contains('\\'))
            {
                fileNumber = "";
                return false;
            }
        }
        else
        {
            if (fileNumber.count("/") > 1)
            {
                fileNumber = "";
                return false;
            }
        }
    }
    else
    {
        fileNumber = "";
        return false;
    }

    fileNumber = findFileNumberInFileName(wildCard, fileNumber);
    if (fileNumber == "")
        return false;

    return true;
}