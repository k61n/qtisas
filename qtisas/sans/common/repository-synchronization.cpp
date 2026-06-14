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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSslSocket>
#include <QSettings>
#include <QStack>

#include "repository-synchronization.h"

// +++ remove a folder recursively (local)
bool RepositorySynchronization::removeFolderRecursive(const QString &folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists())
        return true;

    return dir.removeRecursively();
}
// +++ download zipped file
bool RepositorySynchronization::downloadZipFile(const QUrl &zipFileUrl, const QString &savePath)
{
    if (!QDir(savePath).exists())
        return false;

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

    QString filePath = QDir::cleanPath(savePath + "/" + zipFileUrl.fileName());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot save file to:" << filePath;
        reply->deleteLater();
        return false;
    }

    file.write(reply->readAll());
    file.close();
    reply->deleteLater();

    return true;
}
// +++ unzip a file
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
// +++ is git installed?
bool isGitInstalled()
{
    QProcess process;
    process.start("git", QStringList() << "--version");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();
    return output.startsWith("git version");
}
// +++ get repo name from url
QString getRepoNameFromUrl(const QString &repoUrl)
{
    QUrl url(repoUrl);
    QString path = url.path();
    QString repoName = QFileInfo(path).baseName();
    return repoName;
}
// +++ clone Or Update Git-Repo
bool RepositorySynchronization::cloneOrUpdateRepo(const QString &repoUrl, const QString &fullPath)
{
    if (!QDir(fullPath).exists())
    {
        qWarning() << "Path does not exist:" << fullPath;
        return false;
    }

    QString repoName = getRepoNameFromUrl(repoUrl);
    QString repoPath = QDir(fullPath).filePath(repoName);
    QString gitDirPath = QDir(repoPath).filePath(".git");

    QProcess process;
    process.setWorkingDirectory(repoPath);

    if (QDir(gitDirPath).exists())
    {
        qDebug() << "Updating existing repository:" << repoPath;

        process.start("git", {"fetch", "--all"});
        if (!process.waitForFinished())
            qWarning() << "Error fetching repo:" << process.errorString();

        process.start("git", {"reset", "--hard"});
        if (!process.waitForFinished())
            qWarning() << "Error resetting repo:" << process.errorString();
    }
    else
    {
        qDebug() << "Cloning new repository to:" << repoPath;

        process.setWorkingDirectory(fullPath);
        process.start("git", {"clone", repoUrl, repoName});
        if (!process.waitForFinished())
            qWarning() << "Error cloning repo:" << process.errorString();
    }

    return true;
}
// +++ copy Non-Existing-Files-And-Folders between local git and destFolder
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
// +++  convert Git Repo To Zip Url
QString RepositorySynchronization::convertGitRepoToZipUrl(const QString &repoUrl, const QString &branch)
{
    QUrl url(repoUrl);
    QString host = url.host();
    QString path = url.path();

    if (path.endsWith(".git"))
        path.chop(4);

    QString repoName = path.section('/', -1);

    QString zipPath;

    if (host.contains("github.com", Qt::CaseInsensitive))
        zipPath = path + "/archive/refs/heads/" + branch + ".zip";
    else
        zipPath = path + "/-/archive/" + branch + "/" + repoName + "-" + branch + ".zip";

    url.setPath(zipPath);
    return url.toString();
}
// +++  a repository synchronization with a local folder
bool RepositorySynchronization::updateGit(const QString &fullPath, const QString &repoUrl,
                                          const QStringList &extensions)
{
    QString cleanedFullPath = QDir::cleanPath(fullPath) + '/';
    if (!QDir(cleanedFullPath).exists())
    {
        qWarning() << "path does not exist:" << cleanedFullPath;
        return false;
    }

    const QString tmpDir = cleanedFullPath + ".tmp/";
    if (QDir(tmpDir).exists())
        removeFolderRecursive(tmpDir);
    QDir(cleanedFullPath).mkdir(".tmp");

    QString repoName = getRepoNameFromUrl(repoUrl);

    if (isGitInstalled())
        cloneOrUpdateRepo(repoUrl, tmpDir);
    else
    {
        const QString repoZipUrl = convertGitRepoToZipUrl(repoUrl);
        const QString zipFileName = QUrl(repoZipUrl).fileName();
        const QString zipFilePath = tmpDir + zipFileName;
        QString zipFileBase = QString(zipFileName).remove(".zip", Qt::CaseInsensitive);

        downloadZipFile(repoZipUrl, tmpDir);

        unzipFile(tmpDir + zipFileName, tmpDir);

        QDir(tmpDir).rename(zipFileBase, repoName);

        if (QFile::exists(zipFilePath))
            QFile::remove(zipFilePath);
    }

    copyNonExistingFilesAndFolders(tmpDir + repoName, cleanedFullPath, extensions);
    return false;
}
// +++ sync files: overwrite existing, delete local if repo file is empty
void RepositorySynchronization::syncFilesOverwrite(const QString &sourceFolder, const QString &destFolder,
                                                   const QStringList &extensions)
{
    QStack<QPair<QString, QString>> stack;
    stack.push({sourceFolder, destFolder});

    while (!stack.isEmpty())
    {
        const auto [srcDir, dstDir] = stack.pop();
        const QFileInfoList entries = QDir(srcDir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries)
        {
            if (entry.isDir())
            {
                stack.push({entry.absoluteFilePath(), QDir(dstDir).filePath(entry.fileName())});
            }
            else if (extensions.contains(entry.suffix(), Qt::CaseInsensitive))
            {
                const QString dest = QDir(dstDir).filePath(entry.fileName());
                if (entry.size() == 0)
                {
                    QFile::remove(dest); // empty repo file signals "model no longer works"
                }
                else
                {
                    QString savedApiKey;
                    if (QFile::exists(dest))
                    {
                        savedApiKey = QSettings(dest, QSettings::IniFormat).value("AI/ApiKey").toString();
                        QFile::remove(dest);
                    }
                    QFile::copy(entry.absoluteFilePath(), dest);
                    if (!savedApiKey.isEmpty())
                    {
                        QSettings updated(dest, QSettings::IniFormat);
                        updated.setValue("AI/ApiKey", savedApiKey);
                        updated.sync();
                    }
                }
            }
        }
    }
}
// +++ update git repo and sync with overwrite/delete semantics
bool RepositorySynchronization::updateGitOverwrite(const QString &fullPath, const QString &repoUrl,
                                                   const QStringList &extensions)
{
    const QString cleanedFullPath = QDir::cleanPath(fullPath) + '/';
    if (!QDir(cleanedFullPath).exists())
    {
        qWarning() << "path does not exist:" << cleanedFullPath;
        return false;
    }

    const QString tmpDir = cleanedFullPath + ".tmp/";
    if (QDir(tmpDir).exists())
        removeFolderRecursive(tmpDir);
    QDir(cleanedFullPath).mkdir(".tmp");

    const QString repoName = getRepoNameFromUrl(repoUrl);

    if (isGitInstalled())
    {
        cloneOrUpdateRepo(repoUrl, tmpDir);
    }
    else
    {
        const QString repoZipUrl = convertGitRepoToZipUrl(repoUrl);
        const QString zipFileName = QUrl(repoZipUrl).fileName();
        const QString zipFilePath = tmpDir + zipFileName;
        const QString zipFileBase = QString(zipFileName).remove(".zip", Qt::CaseInsensitive);
        downloadZipFile(repoZipUrl, tmpDir);
        unzipFile(tmpDir + zipFileName, tmpDir);
        QDir(tmpDir).rename(zipFileBase, repoName);
        if (QFile::exists(zipFilePath))
            QFile::remove(zipFilePath);
    }

    syncFilesOverwrite(tmpDir + repoName, cleanedFullPath, extensions);
    removeFolderRecursive(tmpDir);
    return true;
}
// +++ download repo to .tmp/ and return path to the repo dir inside it
QString RepositorySynchronization::downloadRepoToTmp(const QString &fullPath, const QString &repoUrl)
{
    const QString cleanedFullPath = QDir::cleanPath(fullPath) + '/';
    if (!QDir(cleanedFullPath).exists())
    {
        qWarning() << "path does not exist:" << cleanedFullPath;
        return {};
    }

    const QString tmpDir = cleanedFullPath + ".tmp/";
    if (QDir(tmpDir).exists())
        removeFolderRecursive(tmpDir);
    QDir(cleanedFullPath).mkdir(".tmp");

    const QString repoName = getRepoNameFromUrl(repoUrl);

    if (isGitInstalled())
    {
        cloneOrUpdateRepo(repoUrl, tmpDir);
    }
    else
    {
        const QString repoZipUrl = convertGitRepoToZipUrl(repoUrl);
        const QString zipFileName = QUrl(repoZipUrl).fileName();
        const QString zipFilePath = tmpDir + zipFileName;
        const QString zipFileBase = QString(zipFileName).remove(".zip", Qt::CaseInsensitive);
        downloadZipFile(repoZipUrl, tmpDir);
        unzipFile(tmpDir + zipFileName, tmpDir);
        QDir(tmpDir).rename(zipFileBase, repoName);
        if (QFile::exists(zipFilePath))
            QFile::remove(zipFilePath);
    }

    const QString repoDir = tmpDir + repoName + '/';
    return QDir(repoDir).exists() ? repoDir : QString();
}
// +++ categorize what would happen: new / update / remove
void RepositorySynchronization::categorizeSyncPlan(const QString &tmpRepoDir, const QString &destFolder,
                                                   const QStringList &extensions, QStringList &outNew,
                                                   QStringList &outUpdate, QStringList &outRemove)
{
    outNew.clear();
    outUpdate.clear();
    outRemove.clear();

    const QFileInfoList entries = QDir(tmpRepoDir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries)
    {
        if (!extensions.contains(entry.suffix(), Qt::CaseInsensitive))
            continue;
        const QString dest = QDir(destFolder).filePath(entry.fileName());
        const bool localExists = QFile::exists(dest);
        if (entry.size() == 0)
        {
            if (localExists)
                outRemove << entry.completeBaseName();
        }
        else if (localExists)
        {
            // Compare content excluding ApiKey — only report as Update if something changed
            QSettings src(entry.absoluteFilePath(), QSettings::IniFormat);
            QSettings dst(dest, QSettings::IniFormat);
            bool different = false;
            for (const QString &key : src.allKeys())
            {
                if (key.compare("AI/ApiKey", Qt::CaseInsensitive) == 0)
                    continue;
                if (src.value(key) != dst.value(key))
                {
                    different = true;
                    break;
                }
            }
            if (!different)
            {
                for (const QString &key : dst.allKeys())
                {
                    if (key.compare("AI/ApiKey", Qt::CaseInsensitive) == 0)
                        continue;
                    if (!src.contains(key))
                    {
                        different = true;
                        break;
                    }
                }
            }
            if (different)
                outUpdate << entry.completeBaseName();
        }
        else
            outNew << entry.completeBaseName();
    }
}
// +++ apply sync only for the names in selectedNames
void RepositorySynchronization::applySyncSelective(const QString &tmpRepoDir, const QString &destFolder,
                                                   const QStringList &extensions, const QSet<QString> &selectedNames)
{
    const QFileInfoList entries = QDir(tmpRepoDir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries)
    {
        if (!extensions.contains(entry.suffix(), Qt::CaseInsensitive))
            continue;
        if (!selectedNames.contains(entry.completeBaseName()))
            continue;
        const QString dest = QDir(destFolder).filePath(entry.fileName());
        if (entry.size() == 0)
        {
            QFile::remove(dest);
        }
        else
        {
            QString savedApiKey;
            if (QFile::exists(dest))
            {
                savedApiKey = QSettings(dest, QSettings::IniFormat).value("AI/ApiKey").toString();
                QFile::remove(dest);
            }
            QFile::copy(entry.absoluteFilePath(), dest);
            if (!savedApiKey.isEmpty())
            {
                QSettings updated(dest, QSettings::IniFormat);
                updated.setValue("AI/ApiKey", savedApiKey);
                updated.sync();
            }
        }
    }
}