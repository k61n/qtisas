/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Common functions for Fit.Function Explorer: compile/fittable
 ******************************************************************************/

#include <QMessageBox>
#include <QTextStream>

#include "fit-function-explorer.h"

//*********************************************************************
//+++  scan files with extension ext: in path & sub-folders
//*********************************************************************
QStringList FunctionsExplorer::scanFiles(const QString &path, const QString &ext, bool removeExt)
{
    QStringList lst;
    QDirIterator it(path, QStringList() << ext, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QString relativePath = QDir(path).relativeFilePath(filePath);
        if (relativePath.left(4) == ".tmp")
            continue;
        if (removeExt)
            relativePath.chop(ext.length() - 1);
        lst.append(relativePath);
    }

    return lst;
}
//*********************************************************************
//+++  list of files if path/subFolder with exension ext
//*********************************************************************
QStringList FunctionsExplorer::listFilesInSubfolder(const QString &path, const QString &ext, const QString &subFolder)
{
    QStringList files;
    QString subDir = QDir(path).filePath(subFolder);
    QDirIterator it(subDir, QStringList() << ext, QDir::Files);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QString fileName = QFileInfo(filePath).fileName();
        fileName.chop(ext.length() - 1);
        files.append(subFolder + fileName);
    }

    return files;
}
//*******************************************
//+++  list of subfolders if path
//*******************************************
QStringList FunctionsExplorer::scanSubfolders(const QString &path, const QString &ext)
{
    QStringList lst;
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

    while (it.hasNext())
    {
        QString subdirPath = it.next();
        QDir subdir(subdirPath);

        QDirIterator fileIt(subdirPath, QStringList() << ext, QDir::Files, QDirIterator::NoIteratorFlags);
        if (fileIt.hasNext())
            lst.append(subdir.dirName());
    }
    for (QString &dir : lst)
        dir.append("/");
    return lst;
}
//*******************************************
//+++  subFolder in name
//*******************************************
QString FunctionsExplorer::subFolder(const QString &s)
{
    return s.contains("/") ? s.section("/", 0, 0) : "";
}
//*******************************************
//+++  shared Filter
//*******************************************
QString FunctionsExplorer::filterShared()
{
    QString filter;
#if defined(Q_OS_WIN) // +++ WIN
    filter = "*.dll";
#elif defined(Q_OS_MAC) // +++  MAC
    filter = "*.dylib";
#else                   // +++ UNIX
    filter = "*.so";
#endif
    return filter;
}
// +++  read fif functilon as a string
QString FunctionsExplorer::readFifFileContent(const QString &fifName)
{
    if (fifName.isEmpty() || !QFile::exists(fifName))
    {
        QMessageBox::warning(nullptr, "QtiSAS", "Error: FIF file does not exist.");
        return {};
    }

    QFile f(fifName);

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(nullptr, "QtiSAS",
                              QString("Could not open file:<br><h4>%1</h4>"
                                      "<p>Please check read permissions.")
                                  .arg(fifName));
        return {};
    }

    QTextStream in(&f);
    QString content = in.readAll();
    f.close();

    return content;
}
// +++  shared Filter
QStringList FunctionsExplorer::getFifFileBlocks(const QString &contentOriginal)
{
    QString content = contentOriginal;
    content.replace("\r\n", "\n");

    if (!content.contains("\n\n[included functions]")) // to support reading of old fif-files
        content.replace("\n[included functions]", "\n\n[included functions]");

    content.replace("\n\n[", "\n\n..:Splitterr:..[");

    return content.split("..:Splitterr:..", Qt::SkipEmptyParts);
}
// +++ read block
QStringList FunctionsExplorer::readBlock(const QStringList &lst, const QString &codeName)
{
    for (int i = 0; i < lst.count(); i++)
        if (lst[i].left(codeName.length()) == codeName)
            return lst[i].trimmed().split("\n");

    return {};
}