/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Common functions for Fit.Function Explorer: compile/fittable
 ******************************************************************************/

#pragma once
#include <QDirIterator>

namespace FunctionsExplorer
{
    QStringList scanFiles(const QString &path, const QString &ext, bool removeExt);
    QStringList listFilesInSubfolder(const QString &path, const QString &ext, const QString &subFolder);
    QStringList scanSubfolders(const QString &path, const QString &ext);
    QString subFolder(const QString &s);
    QString filterShared();
    QString readFifFileContent(const QString &fifName);
    QStringList getFifFileBlocks(const QString &content);
    QStringList readBlock(const QStringList &lst, const QString &codeName);
}
