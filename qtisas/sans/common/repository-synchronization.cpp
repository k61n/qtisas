/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: a repository synchronization with a local folder
 ******************************************************************************/

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStack>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>

#include <QMessageBox>

#include "repository-synchronization.h"

//************************************************************************
//+++ remove a folder recursively (local)
//************************************************************************
bool removeFolderRecursive(const QString &folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists())
        return true;

    return dir.removeRecursively();
}
//************************************************************************
//+++ download zipped file
//************************************************************************
bool RepositorySynchronization::downloadZipFile(const QUrl &zipFileUrl, const QString &savePath,
                                                const QString &localSubFolder)
{
    QNetworkRequest request(zipFileUrl);
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Download failed:" << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QFile file(savePath + zipFileUrl.fileName());
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot save file to" << savePath;
        reply->deleteLater();
        return false;
    }

    file.write(reply->readAll());
    file.close();

    reply->deleteLater();

    return true;
}
//************************************************************************
//+++ unzip a file
//************************************************************************
bool RepositorySynchronization::unzipFile(const QString &zipPath, const QString &destPath)
{
    if (!QFileInfo::exists(zipPath))
        return false;

    QProcess unzipProcess;
    QString program;
    QStringList args;

#if defined(Q_OS_WIN)
    program = "powershell";
    args << "-Command" << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(zipPath, destPath);
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    program = "unzip";
    args << "-o" << zipPath << "-d" << destPath;
#endif

    unzipProcess.start(program, args);
    if (!unzipProcess.waitForStarted())
        return false;

    unzipProcess.waitForFinished(-1);

    return unzipProcess.exitCode() == 0;
}
//************************************************************************
//+++ is git installed?
//************************************************************************
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
                                          const QString &repoUrl, const QString &repoZipUrl,
                                          const QStringList &extensions)
{
    const QString gitsDir = localPath + ".gits/";

    removeFolderRecursive(gitsDir + localSubFolder);

    if (isGitInstalled())
        cloneOrUpdateRepo(repoUrl, localPath, localSubFolder);
    else
    {
        const QString zipFile = QUrl(repoZipUrl).fileName();
        const QString zipFileBase = QUrl(repoZipUrl).fileName().remove(".zip", Qt::CaseInsensitive);

        downloadZipFile(repoZipUrl, localPath + ".gits/", localSubFolder);

        unzipFile(gitsDir + zipFile, localPath + ".gits/");

        QFile::rename(gitsDir + zipFileBase, gitsDir + localSubFolder);

        if (QFile::exists(gitsDir + zipFile))
            QFile::remove(gitsDir + zipFile);
    }

    copyNonExistingFilesAndFolders(gitsDir + localSubFolder, localPath + localSubFolder, extensions);
}