/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: a repository synchronization with a local folder
 ******************************************************************************/

#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHash>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSslSocket>
#include <QStack>
#include <QTextEdit>
#include <QVBoxLayout>

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

    unzipProcess.waitForFinished(30000);

    return unzipProcess.exitCode() == 0;
}
// +++ is git installed?
bool isGitInstalled()
{
    QProcess process;
    process.start("git", QStringList() << "--version");
    process.waitForFinished(3000);
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
bool RepositorySynchronization::cloneOrUpdateRepo(const QString &repoUrl, const QString &fullPath,
                                                  QProgressDialog *progress)
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

    auto waitGit = [progress](QProcess &p, const char *what) -> bool {
        while (!p.waitForFinished(200))
        {
            QCoreApplication::processEvents();
            if (progress && progress->wasCanceled())
            {
                p.kill();
                return false;
            }
        }
        if (p.exitStatus() != QProcess::NormalExit)
            qWarning() << "git" << what << "error:" << p.errorString();
        return true;
    };

    if (QDir(gitDirPath).exists())
    {
        qDebug() << "Updating existing repository:" << repoPath;
        process.start("git", {"fetch", "--all"});
        if (!waitGit(process, "fetch"))
            return false;
        process.start("git", {"reset", "--hard"});
        if (!waitGit(process, "reset"))
            return false;
    }
    else
    {
        qDebug() << "Cloning new repository to:" << repoPath;
        process.setWorkingDirectory(fullPath);
        process.start("git", {"clone", repoUrl, repoName});
        if (!waitGit(process, "clone"))
            return false;
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
    removeFolderRecursive(tmpDir);
    return true;
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
                    const bool isAi = entry.suffix().compare("ai", Qt::CaseInsensitive) == 0;
                    QString savedApiKey;
                    if (isAi && QFile::exists(dest))
                        savedApiKey = QSettings(dest, QSettings::IniFormat).value("AI/ApiKey").toString();
                    QFile::remove(dest);
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
QString RepositorySynchronization::downloadRepoToTmp(const QString &fullPath, const QString &repoUrl,
                                                     QProgressDialog *progress)
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
        cloneOrUpdateRepo(repoUrl, tmpDir, progress);
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
// +++ categorize what would happen: new / update / remove (recurses into subdirectories)
// Keys in out-lists: fileName() for top-level files; "subDir/fileName" for nested files.
void RepositorySynchronization::categorizeSyncPlan(const QString &tmpRepoDir, const QString &destFolder,
                                                   const QStringList &extensions, QStringList &outNew,
                                                   QStringList &outUpdate, QStringList &outRemove)
{
    outNew.clear();
    outUpdate.clear();
    outRemove.clear();

    // stack holds {srcDir, dstDir, relPrefix}
    QStack<std::tuple<QString, QString, QString>> stack;
    stack.push({tmpRepoDir, destFolder, ""});

    while (!stack.isEmpty())
    {
        const auto [srcDir, dstDir, relPrefix] = stack.pop();
        const QFileInfoList entries = QDir(srcDir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &entry : entries)
        {
            if (entry.isDir())
            {
                const QString sub = entry.fileName();
                const QString rel = relPrefix.isEmpty() ? sub : relPrefix + "/" + sub;
                stack.push({entry.absoluteFilePath(), QDir(dstDir).filePath(sub), rel});
                continue;
            }
            if (!extensions.contains(entry.suffix(), Qt::CaseInsensitive))
                continue;

            const QString key = relPrefix.isEmpty() ? entry.fileName() : relPrefix + "/" + entry.fileName();
            const QString dest = QDir(dstDir).filePath(entry.fileName());
            const bool localExists = QFile::exists(dest);

            if (entry.size() == 0)
            {
                if (localExists)
                    outRemove << key;
            }
            else if (localExists)
            {
                bool different = false;
                if (entry.suffix().compare("ai", Qt::CaseInsensitive) == 0)
                {
                    // AI preset (INI format): compare via QSettings, excluding the user's API key
                    QSettings src(entry.absoluteFilePath(), QSettings::IniFormat);
                    QSettings dst(dest, QSettings::IniFormat);
                    for (const QString &k : src.allKeys())
                    {
                        if (k.compare("AI/ApiKey", Qt::CaseInsensitive) == 0)
                            continue;
                        if (src.value(k) != dst.value(k))
                        {
                            different = true;
                            break;
                        }
                    }
                    if (!different)
                    {
                        for (const QString &k : dst.allKeys())
                        {
                            if (k.compare("AI/ApiKey", Qt::CaseInsensitive) == 0)
                                continue;
                            if (!src.contains(k))
                            {
                                different = true;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    QFile srcFile(entry.absoluteFilePath());
                    QFile dstFile(dest);
                    if (srcFile.open(QIODevice::ReadOnly) && dstFile.open(QIODevice::ReadOnly))
                        different = (srcFile.readAll() != dstFile.readAll());
                }
                if (different)
                    outUpdate << key;
            }
            else
                outNew << key;
        }
    }
}
// +++ apply sync only for the names in selectedNames (recurses into subdirectories)
// selectedNames uses the same key format as categorizeSyncPlan: fileName() or "subDir/fileName".
void RepositorySynchronization::applySyncSelective(const QString &tmpRepoDir, const QString &destFolder,
                                                   const QStringList &extensions, const QSet<QString> &selectedNames)
{
    // stack holds {srcDir, dstDir, relPrefix}
    QStack<std::tuple<QString, QString, QString>> stack;
    stack.push({tmpRepoDir, destFolder, ""});

    while (!stack.isEmpty())
    {
        const auto [srcDir, dstDir, relPrefix] = stack.pop();
        const QFileInfoList entries = QDir(srcDir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &entry : entries)
        {
            if (entry.isDir())
            {
                const QString sub = entry.fileName();
                const QString rel = relPrefix.isEmpty() ? sub : relPrefix + "/" + sub;
                stack.push({entry.absoluteFilePath(), QDir(dstDir).filePath(sub), rel});
                continue;
            }
            if (!extensions.contains(entry.suffix(), Qt::CaseInsensitive))
                continue;

            const QString key = relPrefix.isEmpty() ? entry.fileName() : relPrefix + "/" + entry.fileName();
            if (!selectedNames.contains(key))
                continue;

            const QString dest = QDir(dstDir).filePath(entry.fileName());
            QDir().mkpath(dstDir);

            if (entry.size() == 0)
            {
                QFile::remove(dest);
            }
            else
            {
                const bool isAi = entry.suffix().compare("ai", Qt::CaseInsensitive) == 0;
                QString savedApiKey;
                if (isAi && QFile::exists(dest))
                    savedApiKey = QSettings(dest, QSettings::IniFormat).value("AI/ApiKey").toString();
                QFile::remove(dest);
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
// +++ interactive sync: download → show New/Modified selection dialog → apply
void RepositorySynchronization::updateGitInteractive(QWidget *parent, const QString &fullPath, const QString &repoUrl,
                                                     const QStringList &extensions, bool silent)
{
    const QString tmpCleanup = QDir::cleanPath(fullPath) + "/.tmp";

    // Step 1: download
    QProgressDialog busyDlg("Checking repository…", "Cancel", 0, 0, parent);
    if (!silent)
    {
        busyDlg.setWindowModality(Qt::ApplicationModal);
        busyDlg.setMinimumDuration(0);
        busyDlg.setWindowTitle("QtiSAS — git :: get");
        busyDlg.show();
        QCoreApplication::processEvents();
    }

    const QString repoDir = downloadRepoToTmp(fullPath, repoUrl, silent ? nullptr : &busyDlg);
    if (!silent)
        busyDlg.hide();
    if (repoDir.isEmpty())
    {
        removeFolderRecursive(tmpCleanup);
        if (silent || !busyDlg.wasCanceled())
            QMessageBox::warning(parent, "QtiSAS", "Failed to download repository.");
        return;
    }

    // Step 2: categorize
    QStringList toNew, toUpdate, toRemove;
    categorizeSyncPlan(repoDir, fullPath, extensions, toNew, toUpdate, toRemove);
    if (toNew.isEmpty() && toUpdate.isEmpty() && toRemove.isEmpty())
    {
        removeFolderRecursive(tmpCleanup);
        if (!silent)
            QMessageBox::information(parent, "QtiSAS — git :: get", "Everything is up to date.");
        return;
    }

    // Silent mode: auto-apply only new files (safe — no overwrites) and return
    if (silent)
    {
        if (!toNew.isEmpty())
            applySyncSelective(repoDir, fullPath, extensions, QSet<QString>(toNew.cbegin(), toNew.cend()));
        removeFolderRecursive(tmpCleanup);
        return;
    }

    // Build path map for modified files: key → {srcAbsPath, dstAbsPath}
    using PathPair = QPair<QString, QString>;
    QHash<QString, PathPair> modifiedPaths;
    if (!toUpdate.isEmpty())
    {
        const QSet<QString> updateSet(toUpdate.cbegin(), toUpdate.cend());
        QStack<std::tuple<QString, QString, QString>> scanStack;
        scanStack.push({repoDir, fullPath, ""});
        while (!scanStack.isEmpty())
        {
            const auto [sDir, dDir, relPfx] = scanStack.pop();
            for (const QFileInfo &e : QDir(sDir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            {
                if (e.isDir())
                {
                    const QString sub = e.fileName();
                    const QString rel = relPfx.isEmpty() ? sub : relPfx + "/" + sub;
                    scanStack.push({e.absoluteFilePath(), QDir(dDir).filePath(sub), rel});
                    continue;
                }
                if (!extensions.contains(e.suffix(), Qt::CaseInsensitive))
                    continue;
                const QString key = relPfx.isEmpty() ? e.fileName() : relPfx + "/" + e.fileName();
                if (updateSet.contains(key))
                    modifiedPaths[key] = {e.absoluteFilePath(), QDir(dDir).filePath(e.fileName())};
            }
        }
    }

    // Compute unified diff tooltip (local vs repository version)
    auto diffTooltip = [](const QString &localPath, const QString &repoPath) -> QString {
        QProcess proc;
        proc.start("git", {"diff", "--no-index", "-U5", "--src-prefix=local/", "--dst-prefix=repository/", localPath,
                           repoPath});
        // git diff --no-index exits 1 when files differ (normal), 2 on error
        if (!proc.waitForFinished(3000) || proc.exitCode() == 2 || proc.exitCode() > 2)
            return {};
        const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (out.isEmpty())
            return {};
        const QStringList lines = out.split('\n');
        QString html = "<pre style='white-space:pre;font-family:monospace;font-size:9pt'>";
        for (const QString &line : lines)
        {
            if (line.startsWith("---") || line.startsWith("+++"))
                continue;
            else if (line.startsWith('+'))
                html += "<span style='color:green'>" + line.toHtmlEscaped() + "</span>\n";
            else if (line.startsWith('-'))
                html += "<span style='color:red'>" + line.toHtmlEscaped() + "</span>\n";
            else if (line.startsWith("@@"))
                html += "<span style='color:gray'>" + line.toHtmlEscaped() + "</span>\n";
            else
                html += line.toHtmlEscaped() + "\n";
        }
        return html + "</pre>";
    };

    // Pre-compute display HTML for all files
    // Modified: unified diff; New: plain file content
    auto fileHtml = [](const QString &path) -> QString {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        const QStringList lines = QString::fromLocal8Bit(f.readAll()).split('\n');
        QString html = "<pre style='white-space:pre;font-family:monospace;font-size:9pt'>";
        for (const QString &line : lines)
            html += line.toHtmlEscaped() + "\n";
        return html + "</pre>";
    };

    QHash<QString, QString> modifiedHtml, newHtml;
    for (const QString &name : toUpdate)
    {
        const auto it = modifiedPaths.constFind(name);
        if (it != modifiedPaths.constEnd())
        {
            const QString diff = diffTooltip(it->second, it->first);
            modifiedHtml[name] = diff.isEmpty() ? fileHtml(it->first) : diff;
        }
    }
    for (const QString &name : toNew)
    {
        const QString path = repoDir + name;
        newHtml[name] = fileHtml(path);
    }
    QHash<QString, QString> removeHtml;
    for (const QString &name : toRemove)
        removeHtml[name] = fileHtml(QDir(fullPath).filePath(name));

    // Step 3: build selection dialog
    QDialog dlg(parent);
    dlg.setWindowTitle("git :: get — Select Files to Sync");
    auto *dlgLay = new QVBoxLayout(&dlg);

    // Diff viewer — shared, always visible, below the file sections
    auto *diffView = new QTextEdit(&dlg);
    diffView->setReadOnly(true);
    diffView->setFrameShape(QFrame::StyledPanel);
    diffView->setMinimumHeight(150);
    {
        QFont mono;
        mono.setFamily("Courier");
        mono.setStyleHint(QFont::Monospace);
        mono.setPointSize(9);
        diffView->setFont(mono);
    }
    diffView->setHtml("<i style='color:gray'>Click a file to preview</i>");

    // helper: build a QGroupBox with a checkable QListWidget and a "select all" header
    QListWidget *lwUpdate = nullptr, *lwNew = nullptr, *lwRemove = nullptr;
    auto addSection = [&](const QString &title, const QStringList &names, QListWidget *&outLW, bool checkedDefault,
                          const QHash<QString, QString> &contentMap) -> QGroupBox * {
        if (names.isEmpty())
            return nullptr;
        auto *grp = new QGroupBox(title, &dlg);
        auto *grpLay = new QVBoxLayout(grp);

        auto *cbAll = new QCheckBox("(select all)", grp);
        cbAll->setCheckState(checkedDefault ? Qt::Checked : Qt::Unchecked);
        QFont f = cbAll->font();
        f.setItalic(true);
        cbAll->setFont(f);
        grpLay->addWidget(cbAll);

        auto *lw = new QListWidget(grp);
        lw->setSelectionMode(QAbstractItemView::SingleSelection);
        for (const QString &name : names)
        {
            auto *item = new QListWidgetItem(name, lw);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(checkedDefault ? Qt::Checked : Qt::Unchecked);
            const auto it = contentMap.constFind(name);
            if (it != contentMap.constEnd())
                item->setData(Qt::UserRole, it.value());
        }
        outLW = lw;
        grpLay->addWidget(lw);

        // "select all" clicked → sync all items
        QObject::connect(cbAll, &QCheckBox::clicked, grp, [lw](bool checked) {
            for (int i = 0; i < lw->count(); ++i)
                lw->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        });
        // any item toggled → update "select all" header state
        QObject::connect(lw, &QListWidget::itemChanged, grp, [cbAll, lw]() {
            int n = 0;
            for (int i = 0; i < lw->count(); ++i)
                if (lw->item(i)->checkState() == Qt::Checked)
                    ++n;
            QSignalBlocker bl(cbAll);
            cbAll->setTristate(n > 0 && n < lw->count());
            cbAll->setCheckState(n == 0 ? Qt::Unchecked : (n == lw->count() ? Qt::Checked : Qt::PartiallyChecked));
        });
        // row selected → show content in shared viewer; mark that a new selection just happened
        QObject::connect(
            lw, &QListWidget::currentItemChanged, &dlg, [diffView, lw](QListWidgetItem *cur, QListWidgetItem *) {
                lw->setProperty("justSelected", cur != nullptr);
                const QString html = cur ? cur->data(Qt::UserRole).toString() : QString();
                diffView->setHtml(html.isEmpty() ? "<i style='color:gray'>No preview available</i>" : html);
            });
        // click already-selected item → deselect and clear viewer (not for single-item lists)
        // currentItemChanged does NOT fire on a re-click (current didn't change), so
        // justSelected stays false and itemClicked knows to deselect.
        if (names.size() > 1)
            QObject::connect(lw, &QListWidget::itemClicked, lw, [lw](QListWidgetItem *) {
                if (lw->property("justSelected").toBool())
                {
                    lw->setProperty("justSelected", false);
                    return;
                }
                lw->clearSelection();
                lw->setCurrentItem(nullptr);
            });
        // single item → auto-show immediately, no deselection possible
        if (names.size() == 1)
            lw->setCurrentRow(0);

        return grp;
    };

    auto *grpModified = addSection("Modified", toUpdate, lwUpdate, false, modifiedHtml);
    auto *grpNew = addSection("New", toNew, lwNew, true, newHtml);
    auto *grpRemove = addSection("Empty files", toRemove, lwRemove, false, removeHtml);

    // clicking in one list clears the others
    auto clearOthers = [](QListWidget *changed, const QList<QListWidget *> &others) {
        if (!changed)
            return;
        QObject::connect(changed, &QListWidget::currentItemChanged, changed,
                         [others](QListWidgetItem *cur, QListWidgetItem *) {
                             if (!cur)
                                 return;
                             for (QListWidget *lw : others)
                             {
                                 if (lw)
                                 {
                                     QSignalBlocker b(lw);
                                     lw->clearSelection();
                                     lw->setCurrentItem(nullptr);
                                 }
                             }
                         });
    };
    clearOthers(lwUpdate, {lwNew, lwRemove});
    clearOthers(lwNew, {lwUpdate, lwRemove});
    clearOthers(lwRemove, {lwUpdate, lwNew});

    auto *sectionsRow = new QHBoxLayout();
    if (grpModified)
        sectionsRow->addWidget(grpModified);
    if (grpNew)
        sectionsRow->addWidget(grpNew);
    if (grpRemove)
        sectionsRow->addWidget(grpRemove);
    dlgLay->addLayout(sectionsRow);

    dlgLay->addWidget(diffView);

    auto *btnRow = new QHBoxLayout();
    auto *btnApply = new QPushButton("Apply selected", &dlg);
    auto *btnCancel = new QPushButton("Cancel", &dlg);
    btnApply->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(btnApply);
    btnRow->addWidget(btnCancel);
    dlgLay->addLayout(btnRow);
    QObject::connect(btnApply, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
    {
        removeFolderRecursive(tmpCleanup);
        return;
    }

    // Step 4: collect checked names and apply
    QSet<QString> selected;
    auto collectLW = [&](QListWidget *lw, const QStringList &names) {
        if (!lw)
            return;
        for (int i = 0; i < lw->count() && i < names.size(); ++i)
            if (lw->item(i)->checkState() == Qt::Checked)
                selected.insert(names[i]);
    };
    collectLW(lwUpdate, toUpdate);
    collectLW(lwNew, toNew);
    collectLW(lwRemove, toRemove);

    if (!selected.isEmpty())
        applySyncSelective(repoDir, fullPath, extensions, selected);

    removeFolderRecursive(tmpCleanup);
}
