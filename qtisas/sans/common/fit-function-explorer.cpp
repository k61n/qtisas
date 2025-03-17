/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Common functions for Fit.Function Explorer: compile/fittable
 ******************************************************************************/

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