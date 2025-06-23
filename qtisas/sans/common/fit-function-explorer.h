/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Common functions for Fit.Function Explorer: compile/fittable
 ******************************************************************************/

#ifndef FITFUNCTIONEXPLORER_H
#define FITFUNCTIONEXPLORER_H

#include <QDirIterator>

class FunctionsExplorer
{
  public:
    static QStringList scanFiles(const QString &path, const QString &ext, bool removeExt);
    static QStringList listFilesInSubfolder(const QString &path, const QString &ext, const QString &subFolder);
    static QStringList scanSubfolders(const QString &path, const QString &ext);
    static QString subFolder(const QString &s);
    static QString filterShared();
    static QString readFifFileContent(const QString &fifName);
    static QStringList getFifFileBlocks(const QString &content);
    static QStringList readBlock(const QStringList &lst, const QString &codeName);
};

#endif
