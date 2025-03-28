/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: a repository synchronization with a local folder
 ******************************************************************************/

#include "repository-synchronization.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QStack>

bool isGitInstalled()
{
    QProcess process;
    process.start("git", QStringList() << "--version");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();
    return output.startsWith("git version");
}

//************************************************************************
//+++ clone Or Update Git-Repo
//************************************************************************
void RepositorySynchronization::cloneOrUpdateRepo(const QString &repoUrl, const QString &localFolder,
                                                  const QString &localSubFolder)
{
    QString folderInGit = localFolder + "/.gits/" + localSubFolder;
    QDir dir(folderInGit);

    if (dir.exists(".git"))
    {
        QProcess process;
        QStringList arguments;
        arguments << "fetch" << "--all";

        process.setWorkingDirectory(folderInGit);
        process.start("git", arguments);
        if (!process.waitForFinished())
            qDebug() << "Error updating repo:" << process.errorString();

        arguments.clear();
        arguments << "reset" << "--hard";

        process.start("git", arguments);
        if (!process.waitForFinished())
            qDebug() << "Error updating repo:" << process.errorString();
    }
    else
    {
        QDir().mkpath(folderInGit);

        QProcess process;
        QStringList arguments;
        arguments << "clone" << repoUrl << folderInGit;

        process.start("git", arguments);
        if (!process.waitForFinished())
            qDebug() << "Error cloning repo:" << process.errorString();
    }
}
//************************************************************************
//+++ copy Non-Existing-Files-And-Folders between local git and destFolder
//************************************************************************
void RepositorySynchronization::copyNonExistingFilesAndFolders(const QString &sourceFolder, const QString &destFolder,
                                                               const QStringList &extensions)
{
    QStack<QPair<QString, QString>> stack;
    stack.push(QPair<QString, QString>(sourceFolder, destFolder));

    while (!stack.isEmpty())
    {
        QPair<QString, QString> current = stack.pop();
        QDir sourceDir(current.first);
        QDir destDir(current.second);

        if (!sourceDir.exists())
        {
            qDebug() << "Source folder does not exist!";
            continue;
        }

        if (!destDir.exists())
            destDir.mkpath(current.second);

        QFileInfoList entries = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &entry : entries)
        {
            QString sourcePath = entry.absoluteFilePath();
            QString destPath = destDir.absoluteFilePath(entry.fileName());

            if (entry.isDir())
            {
                stack.push(qMakePair(sourcePath, destPath)); // Process subdirectories iteratively
                if (!QDir(destPath).exists())
                    QDir().mkpath(destPath);
            }
            else if (entry.isFile() && extensions.contains(entry.suffix(), Qt::CaseInsensitive) &&
                     !QFile::exists(destPath))
            {
                if (!QFile::copy(sourcePath, destPath))
                    qDebug() << "Failed to copy:" << sourcePath;
            }
        }
    }
}

//*********************************************************************
//+++  a repository synchronization with a local folder
//*********************************************************************
void RepositorySynchronization::updateGit(const QString &localPath, const QString &localSubFolder,
                                          const QString &repoUrl, const QStringList &extensions)
{
    if (!isGitInstalled())
    {
        QMessageBox::critical(nullptr, "QtiSAS: Compile", "Install first GIT");
        return;
    }

    cloneOrUpdateRepo(repoUrl, localPath, localSubFolder);

    copyNonExistingFilesAndFolders(localPath + ".gits/" + localSubFolder, localPath + localSubFolder, extensions);
}