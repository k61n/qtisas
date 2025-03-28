/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: a repository synchronization with a local folder
 ******************************************************************************/

#ifndef REPOSITORYSYNCHRONIZATION_H
#define REPOSITORYSYNCHRONIZATION_H

#include <QString>
#include <QStringList>

class RepositorySynchronization
{
  public:
    static void cloneOrUpdateRepo(const QString &repoUrl, const QString &localFolder, const QString &localSubFolder);
    static void copyNonExistingFilesAndFolders(const QString &localFolder, const QString &localSubFolder,
                                               const QStringList &extensions);
    static void updateGit(const QString &localPath, const QString &localSubFolder, const QString &repoUrl,
                          const QStringList &filesExtension);
};
#endif
