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
#include <QUrl>

class RepositorySynchronization
{
  public:
    static bool removeFolderRecursive(const QString &folderPath);
    static bool cloneOrUpdateRepo(const QString &repoUrl, const QString &fullPath);
    static bool downloadZipFile(const QUrl &zipFileUrl, const QString &savePath);
    static bool unzipFile(const QString &zipPath, const QString &destPath);
    static QString convertGitRepoToZipUrl(const QString &repoUrl, const QString &branch = "main");
    static void copyNonExistingFilesAndFolders(const QString &localFolder, const QString &localSubFolder,
                                               const QStringList &extensions);
    static bool updateGit(const QString &fullPath, const QString &repoUrl, const QStringList &extensions);
};
#endif
