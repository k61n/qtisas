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
    static bool ensureSubfolderExists(const QString &basePath, const QString &subfolderName);
    static bool removeFolderRecursive(const QString &folderPath);
    static bool downloadZipFile(const QUrl &zipFileUrl, const QString &savePath, const QString &localSubFolder);
    static bool unzipFile(const QString &zipPath, const QString &destPath);
    static void cloneOrUpdateRepo(const QString &repoUrl, const QString &localFolder, const QString &localSubFolder);
    static void copyNonExistingFilesAndFolders(const QString &localFolder, const QString &localSubFolder,
                                               const QStringList &extensions);
    static void updateGit(const QString &localPath, const QString &localSubFolder, const QString &repoUrl,
                          const QString &repoZipUrl, const QStringList &filesExtension);
};
#endif
