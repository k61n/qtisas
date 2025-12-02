/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: a repository synchronization with a local folder
 ******************************************************************************/

#pragma once
#include <QString>
#include <QStringList>
#include <QUrl>

namespace RepositorySynchronization
{
    bool removeFolderRecursive(const QString &folderPath);
    bool cloneOrUpdateRepo(const QString &repoUrl, const QString &fullPath);
    bool downloadZipFile(const QUrl &zipFileUrl, const QString &savePath);
    bool unzipFile(const QString &zipPath, const QString &destPath);
    QString convertGitRepoToZipUrl(const QString &repoUrl, const QString &branch = "main");
    void copyNonExistingFilesAndFolders(const QString &localFolder, const QString &localSubFolder,
                                        const QStringList &extensions);
    bool updateGit(const QString &fullPath, const QString &repoUrl, const QStringList &extensions);
}
