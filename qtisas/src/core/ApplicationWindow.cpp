/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2008 roemmic <roemmic@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2009 Jonas Bähr <jonas@fs.ei.tum.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiSAS's main window
 ******************************************************************************/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <zlib.h>

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontComboBox>
#include <QImageReader>
#include <QImageWriter>
#include <QInputDialog>
#include <QHideEvent>
#include <QKeySequence>
#include <QList>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPixmapCache>
#include <QPluginLoader>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStringListModel>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QTextDocumentWriter>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QUndoView>
#include <QUrl>

#include <gsl/gsl_sort.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_plot_magnifier.h>
#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_symbol.h>

#include "AddWidgetTool.h"
#include "AnovaDialog.h"
#include "ApplicationWindow.h"
#include "AxesDialog.h"
#include "BaselineDialog.h"
#include "ColorBox.h"
#include "ColorButton.h"
#include "ColorMapDialog.h"
#include "ColorMapEditor.h"
#include "ConfigDialog.h"
#include "Convolution.h"
#include "Correlation.h"
#include "CreateBinMatrixDialog.h"
#include "CurvesDialog.h"
#include "CustomActionDialog.h"
#include "DataPickerTool.h"
#include "Differentiation.h"
#include "EllipseWidget.h"
#include "EnrichmentDialog.h"
#include "ErrDialog.h"
#include "ErrorBarsCurve.h"
#include "ExpDecayDialog.h"
#include "ExtractDataDialog.h"
#include "FFTDialog.h"
#include "FFTFilter.h"
#include "FilterDialog.h"
#include "FindDialog.h"
#include "Fit.h"
#include "FitDialog.h"
#include "fitting.h"
#include "FrequencyCountDialog.h"
#include "FunctionCurve.h"
#include "FunctionDialog.h"
#include "globals.h"
#include "GriddingDialog.h"
#include "ImageExportDialog.h"
#include "ImageWidget.h"
#include "ImportASCIIDialog.h"
#include "IntDialog.h"
#include "Integration.h"
#include "IntegrationDialog.h"
#include "InterpolationDialog.h"
#include "LayerDialog.h"
#include "LineDialog.h"
#include "LineProfileTool.h"
#include "LogisticFit.h"
#include "MatrixCommand.h"
#include "MatrixDialog.h"
#include "MatrixModel.h"
#include "MatrixResamplingDialog.h"
#include "MatrixSizeDialog.h"
#include "MatrixValuesDialog.h"
#include "MdiSubWindow.h"
#include "MultiPeakFit.h"
#include "MultiPeakFitTool.h"
#include "NonLinearFit.h"
#include "OpenProjectDialog.h"
#include "PatternBox.h"
#include "PenStyleBox.h"
#include "PieCurve.h"
#include "PlotDialog.h"
#include "PlotToolInterface.h"
#include "PlotWizard.h"
#include "PolynomFitDialog.h"
#include "PolynomialFit.h"
#include "PythonSyntaxHighlighter.h"
#ifdef SCRIPTING_PYTHON
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#pragma pop_macro("slots")
#include "PythonScripting.h"
#endif
#include "QtiPlotApplication.h"
#include "QwtHistogram.h"
#include "RectangleWidget.h"
#include "RenameWindowDialog.h"
#include "ScaleDraw.h"
#include "ScaleEngine.h"
#include "ScreenPickerTool.h"
#include "ScriptingLangDialog.h"
#include "SelectionMoveResizer.h"
#include "SetColValuesDialog.h"
#include "settings.h"
#include "SigmoidalFit.h"
#include "SmartCompleter.h"
#include "SmoothCurveDialog.h"
#include "SmoothFilter.h"
#include "Spectrogram.h"
#include "StudentTestDialog.h"
#include "SubtractDataDialog.h"
#include "SubtractLineTool.h"
#include "SurfaceDialog.h"
#include "SymbolBox.h"
#include "SymbolDialog.h"
#include "TableDialog.h"
#include "TextDialog.h"
#include "TexWidget.h"
#include "version.h"

#ifdef QTISAS
#include "ascii1d18.h"
#include "jnse18.h"
#include "svd.h"
#include "dan18.h"
#include "dan-python.h"
#include "compile18.h"
#include "fittable18.h"
#endif

using namespace std;
using namespace Qwt3D;

QString currentVersionInfo(bool txtYN)
{
    QString h1 = (txtYN) ? "%1\n" : "<h1>%1</h1>";
    QString par = (txtYN) ? "%1\n" : "<p>%1</p>";
    QString bold = (txtYN) ? "%1" : "<b>%1</b>";
    QString norm = (txtYN) ? "%1" : "<span style=\"font-weight:normal\">%1</span>";
    QString link = (txtYN) ? "    https://%1" : "<a href=\"https://%1\">%1</a>";
    QString list = (txtYN) ? "\n%1" : "<ul>%1</ul>";
    QString listentry = (txtYN) ? "    %1\n" : "<li>%1</li>";
    QString text = "";
    text += h1.arg(QString("QtiSAS v%1.%2.%3").arg(maj_version).arg(min_version).arg(patch_version));
    text += par.arg("Copyright (C) Forschungszentrum Jülich Gmbh and Authors.");
    text += par.arg("QtiSAS is an analysis software for SAS (small-angle scattering) measurements based on QtiPlot.");
    QString authors =
        list.arg(QString("%1%2").arg(listentry.arg("Vitaliy Pipich")).arg(listentry.arg("Konstantin Kholostov")));
    text += par.arg(QString("Authors:%1").arg(authors));
    text += par.arg(bold.arg("QtiSAS git:"));
    text += par.arg(norm.arg(link.arg("iffgit.fz-juelich.de/qtisas/qtisas")));
    text += par.arg(bold.arg("QtiSAS Web-Page:"));
    text += par.arg(norm.arg(link.arg("qtisas.com")));
    text += par.arg(bold.arg("News:"));
    text += par.arg(norm.arg(link.arg("qtisas.com/news")));
    text += par.arg(bold.arg("Download Page:"));
    text += par.arg(norm.arg(link.arg("iffgit.fz-juelich.de/qtisas/qtisas/-/releases")));
    return text;
}

void ApplicationWindow::newVersionCheck()
{
    const QString current = QVersionNumber(maj_version, min_version, patch_version).toString();

    QString suffix;

#if defined(Q_OS_WIN)
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    suffix = (arch.contains("arm") || arch.contains("aarch64")) ? "-arm64.exe" : "-x64.exe";
#elif defined(Q_OS_MACOS)
    suffix = ".dmg";
#else
    suffix = ".zip";
#endif

    auto *manager = new QNetworkAccessManager(this);

    connect(manager, &QNetworkAccessManager::finished, this, [=, this](QNetworkReply *reply) {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            QMessageBox::warning(this, "Version check", "Network error:\n" + reply->errorString());
            return;
        }

        QVersionNumber best;
        QString latestTag;

        const QJsonArray tags = QJsonDocument::fromJson(reply->readAll()).array();

        for (const auto &t : tags)
        {
            const QString name = t.toObject()["name"].toString();
            if (!name.startsWith("v"))
                continue;

            QVersionNumber v = QVersionNumber::fromString(name.mid(1));

            if (!v.isNull() && v > best)
            {
                best = v;
                latestTag = name.mid(1);
            }
        }

        const QVersionNumber currentV = QVersionNumber::fromString(current);

        QDialog dialog(this);
        dialog.setWindowTitle("QtiSAS Version Check");

        auto *layout = new QVBoxLayout(&dialog);

        auto *icon = new QLabel(&dialog);
        icon->setPixmap(QIcon(":/qtisas_logo.png").pixmap(64, 64));
        icon->setAlignment(Qt::AlignHCenter);
        layout->addWidget(icon);

        auto *info = new QLabel(&dialog);
        info->setWordWrap(true);
        info->setAlignment(Qt::AlignHCenter);

        if (best.isNull())
            info->setText("No valid version tags found.\nCurrent version: " + current);
        else
            info->setText("Current version: " + current + "\nLatest version: " + latestTag);

        layout->addWidget(info);

        auto *closeBtn = new QPushButton("Close", &dialog);
        layout->addWidget(closeBtn);
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        if (!best.isNull() && best > currentV)
        {
            auto *downloadBtn = new QPushButton("Download", &dialog);
            layout->insertWidget(2, downloadBtn);

            connect(downloadBtn, &QPushButton::clicked, this, [=, this, &dialog]() {
                downloadBtn->setEnabled(false);

                const QString url =
                    QString(
                        "https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v%1/qtisas-v%1%2")
                        .arg(QString(QUrl::toPercentEncoding(latestTag)))
                        .arg(suffix);

                const QString downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
                                             "/qtisas-v" + latestTag + suffix;

                auto *dlManager = new QNetworkAccessManager(this);
                QNetworkReply *dlReply = dlManager->get(QNetworkRequest(QUrl(url)));

                auto *file = new QFile(downloadPath);
                if (!file->open(QIODevice::WriteOnly))
                {
                    QMessageBox::warning(this, "Download Error", "Cannot save file to:\n" + downloadPath);

                    dlReply->deleteLater();
                    dlManager->deleteLater();
                    file->deleteLater();
                    downloadBtn->setEnabled(true);
                    return;
                }

                auto *progress = new QProgressBar(&dialog);
                progress->setRange(0, 100);
                layout->replaceWidget(downloadBtn, progress);
                downloadBtn->hide();

                connect(dlReply, &QNetworkReply::readyRead, [=] { file->write(dlReply->readAll()); });

                connect(dlReply, &QNetworkReply::downloadProgress, progress, [=](qint64 r, qint64 t) {
                    if (t > 0)
                        progress->setValue(int((r * 100) / t));
                });

                connect(dlReply, &QNetworkReply::finished, this, [=, &dialog]() {
                    file->close();
                    file->deleteLater();
                    dlReply->deleteLater();
                    dlManager->deleteLater();

#if defined(Q_OS_WIN)

                    auto *btn = new QPushButton("Show Downloaded File", &dialog);

                    layout->replaceWidget(progress, btn);
                    progress->hide();

                    connect(btn, &QPushButton::clicked, [=] {
                        QProcess::startDetached("explorer",
                                                QStringList() << ("/select," + QDir::toNativeSeparators(downloadPath)));

                        btn->setEnabled(false);
                    });

#elif defined(Q_OS_MACOS)

                    auto *installBtn = new QPushButton("Close QtiSAS && Install", &dialog);

                    layout->replaceWidget(progress, installBtn);
                    progress->hide();

                    QObject::connect(installBtn, &QPushButton::clicked, &dialog, [installBtn, downloadPath]()
                    {
                        installBtn->setEnabled(false);

                        QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [downloadPath]()
                        {
                            QProcess::startDetached("open", QStringList() << downloadPath);
                        },
                        Qt::SingleShotConnection);

                        qApp->closeAllWindows();
                    });

#endif

                    dialog.adjustSize();
                });
            });
        }

#endif

        dialog.setMinimumWidth(400);
        dialog.adjustSize();
        dialog.exec();
    });

    manager->get(
        QNetworkRequest(QUrl("https://iffgit.fz-juelich.de/api/v4/projects/1655/repository/tags?per_page=50")));
}

extern "C"
{
void file_compress(char  *file, char  *mode);
void file_uncompress(char  *file);
}

ApplicationWindow::ApplicationWindow(bool factorySettings)
: QMainWindow(), scripted(ScriptingLangManager::newEnv(this))
{
	setAttribute(Qt::WA_DeleteOnClose);
	init(factorySettings);
	((QtiPlotApplication *)QCoreApplication::instance ())->append(this);
}

void ApplicationWindow::init(bool factorySettings)
{
    d_terminal_script = nullptr;

    bool thousandsSep = false;
    QLocale loc = QLocale::c();
    if (!thousandsSep)
        loc.setNumberOptions(QLocale::OmitGroupSeparator);
    setLocale(loc);
    QLocale::setDefault(loc);

#ifdef Q_OS_WIN
    sasPath = QDir::homePath() + "/AppData/Local/qtisas/";
#else
    sasPath = QDir::homePath() + "/.config/qtisas/";
#endif

    if (!QDir(sasPath).exists())
        QDir().mkdir(sasPath);

    templatesDir = sasPath + "templates/";
    if (!QDir(templatesDir).exists())
        QDir().mkdir(templatesDir);

    screenResoHight = QGuiApplication::primaryScreen()->availableGeometry().height();

    sasResoScale = 1.0;
    sasDefaultInterface = 0;
    currentColorMap = 5;
    currentSymbolColorList = 0;
    imageFormat = "PDF";
    imageRes = 150;

    projectname = "untitled";
    setWindowTitle("QtiSAS - untitled");
    setObjectName("QtiSAS");
    setDefaultOptions();
    QPixmapCache::setCacheLimit(20 * QPixmapCache::cacheLimit());

    tablesDepend = new QMenu(this);

    logWindow = new QDockWidget(this);
    logWindow->setObjectName("logWindow");
    logWindow->setWindowTitle(tr("Results Log"));
    if (screenResoHight < 910)
    {
        addDockWidget(Qt::RightDockWidgetArea, logWindow);
        logWindow->setMinimumHeight(90);
    }
    else
    {
        addDockWidget(Qt::BottomDockWidgetArea, logWindow);
        logWindow->setMinimumHeight(150);
    }

    results = new QTextEdit(logWindow);
    results->setReadOnly(true);
    logWindow->setWidget(results);
    logWindow->hide();

    explorerWindow = new QDockWidget(this);
	explorerWindow->setWindowTitle(tr("Project Explorer"));
    explorerWindow->setObjectName("explorerWindow");
    if (screenResoHight < 910)
    {
        addDockWidget(Qt::RightDockWidgetArea, explorerWindow);
        explorerWindow->setMinimumHeight(90);
    }
    else
    {
        addDockWidget(Qt::BottomDockWidgetArea, explorerWindow);
        explorerWindow->setMinimumHeight(150);
    }

    tabifyDockWidget(explorerWindow, logWindow);
    
#ifdef QTISAS
    danWindow = new QDockWidget(this);
    danWindow->setWindowTitle(tr("DAN.SANS"));
    danWindow->setObjectName("danWindow");
    addDockWidget(Qt::RightDockWidgetArea, danWindow);
    danWidget = new dan18(this);
    danWindow->setWidget(danWidget);
    danWindow->hide();
    danWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);
    danWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);

    compileWindow = new QDockWidget(this);
    compileWindow->setWindowTitle(tr("FIT.COMPILE"));
    compileWindow->setObjectName("compileWindow");
    addDockWidget(Qt::RightDockWidgetArea, compileWindow);
    compileWidget = new compile18(this);
    compileWindow->setWidget(compileWidget);
    compileWindow->hide();
    compileWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);
    compileWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);

    fittableWindow = new QDockWidget(this);
    fittableWindow->setWindowTitle(tr("FIT.CURVE(S)"));
    fittableWindow->setObjectName("fittableWindow");
    addDockWidget(Qt::RightDockWidgetArea, fittableWindow);
    fittableWidget = new fittable18(this);
    fittableWindow->setWidget(fittableWidget);
    fittableWindow->hide();
    fittableWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);
    fittableWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);

    svdWindow = new QDockWidget(this);
    svdWindow->setWindowTitle(tr("SANS.SVD"));
    svdWindow->setObjectName("svdWindow");
    svdWindow->setMinimumHeight(150);
    addDockWidget(Qt::RightDockWidgetArea, svdWindow);
    svdWidget = new svd(this);
    svdWindow->setWidget(svdWidget);
    svdWindow->hide();
    svdWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);

    jnseWindow = new QDockWidget( this );
    jnseWindow->setWindowTitle(tr("JNSE"));
    jnseWindow->setObjectName("jnseWindow");
    jnseWindow->setMinimumHeight(150);
    addDockWidget(Qt::RightDockWidgetArea, jnseWindow);
    jnseWidget = new jnse18(this);
    jnseWindow->setWidget(jnseWidget);
    jnseWindow->hide();
    jnseWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);

    ascii1dWindow = new QDockWidget(this);
    ascii1dWindow->setWindowTitle(tr("ASCII.SANS.1D"));
    ascii1dWindow->setObjectName("ascii1dWindow");
    ascii1dWindow->setMinimumHeight(150);
    addDockWidget(Qt::RightDockWidgetArea, ascii1dWindow);
    ascii1dWidget = new ascii1d18(this);
    ascii1dWindow->setWidget(ascii1dWidget);
    ascii1dWindow->hide();
    ascii1dWindow->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea|Qt::NoDockWidgetArea);

    if (screenResoHight < 910)
        tabifyDockWidget(logWindow, fittableWindow);
    fittableWindow->hide();

    tabifyDockWidget(fittableWindow, compileWindow);
    compileWindow->hide();

    tabifyDockWidget(compileWindow, danWindow);
    danWindow->hide();
    compileWindow->hide();

    tabifyDockWidget(danWindow,svdWindow);
    svdWindow->hide();

    tabifyDockWidget(svdWindow,jnseWindow);
    jnseWindow->hide();

    tabifyDockWidget(jnseWindow,ascii1dWindow);
    ascii1dWindow->hide();

#endif
    
	actionSaveProject = nullptr;

    folders = new FolderListView(this);
    folders->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    folders->setContextMenuPolicy(Qt::CustomContextMenu);
    folders->setHeaderLabels(QStringList() << tr("Folder") << QString());
    folders->setRootIsDecorated(true);
    folders->header()->setSectionsClickable(false);
    folders->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    folders->header()->hide();
    folders->setSelectionMode(QTreeWidget::SingleSelection);

    connect(folders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this,
            SLOT(folderItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(folders, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(renameFolder(QTreeWidgetItem *, int)));
    connect(folders, SIGNAL(customContextMenuRequested(const QPoint &)), this,
            SLOT(showFolderPopupMenu(const QPoint &)));
    connect(folders, SIGNAL(dragItems(QList<QTreeWidgetItem *>)), this,
            SLOT(dragFolderItems(QList<QTreeWidgetItem *>)));
    connect(folders, SIGNAL(dropItems(QTreeWidgetItem *)), this, SLOT(dropFolderItems(QTreeWidgetItem *)));
    connect(folders, SIGNAL(renameItem(QTreeWidgetItem *)), this, SLOT(startRenameFolder(QTreeWidgetItem *)));
    connect(folders, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
    connect(folders, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));

    current_folder = new Folder(nullptr, tr("UNTITLED"));
    auto fli = new FolderListItem(folders, current_folder);
	current_folder->setFolderListItem(fli);
	fli->setExpanded(true);

    lv = new FolderListView();
    lv->setRootIsDecorated(false);
    lv->setContextMenuPolicy(Qt::CustomContextMenu);
    lv->setHeaderLabels(QStringList() << tr("Name") << tr("Type") << tr("View") << tr("Created") << tr("Label"));
    lv->header()->setStretchLastSection(true);
    lv->setSortingEnabled(true);
    lv->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    lv->setMinimumHeight(80);
    lv->setSelectionMode(QTreeWidget::ExtendedSelection);

    explorerSplitter = new QSplitter(Qt::Horizontal, explorerWindow);
    explorerSplitter->addWidget(folders);
    explorerSplitter->addWidget(lv);
    explorerWindow->setWidget(explorerSplitter);

    QList<int> splitterSizes;
    explorerSplitter->setSizes(splitterSizes << 45 << 45);
    explorerWindow->hide();

#ifdef SCRIPTING_CONSOLE
    consoleWindow = new QDockWidget(this);
    consoleWindow->setObjectName("consoleWindow");
    consoleWindow->setWindowTitle(tr("Scripting Console"));
    if (screenResoHight < 910)
    {
        addDockWidget(Qt::RightDockWidgetArea, consoleWindow);
        consoleWindow->setMinimumHeight(90);
    }
    else
    {
        addDockWidget(Qt::BottomDockWidgetArea, consoleWindow);
        consoleWindow->setMinimumHeight(150);
    }
    console = new QTextEdit(consoleWindow);
    console->setReadOnly(true);
    console->setContextMenuPolicy(Qt::CustomContextMenu);
    new PythonSyntaxHighlighter(console->document());
    {
        // Ctrl+X clear: QShortcut with WidgetShortcut fires before the window-level actionCutSelection
        auto *clrSC = new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_X), console);
        clrSC->setContext(Qt::WidgetShortcut);
        connect(clrSC, &QShortcut::activated, console, &QTextEdit::clear);

        // Context menu with visual shortcut labels (Ctrl+C/Ctrl+A handled natively by QTextEdit)
        auto *actCopy = new QAction(tr("Copy"), console);
        actCopy->setShortcut(QKeySequence::Copy);
        actCopy->setShortcutVisibleInContextMenu(true);
        connect(actCopy, &QAction::triggered, console, &QTextEdit::copy);

        auto *actSelAll = new QAction(tr("Select All"), console);
        actSelAll->setShortcut(QKeySequence::SelectAll);
        actSelAll->setShortcutVisibleInContextMenu(true);
        connect(actSelAll, &QAction::triggered, console, &QTextEdit::selectAll);

        auto *actClear = new QAction(tr("Clear"), console);
        actClear->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_X));
        actClear->setShortcutVisibleInContextMenu(true);
        connect(actClear, &QAction::triggered, console, &QTextEdit::clear);

        connect(console, &QTextEdit::customContextMenuRequested, this,
                [this, actCopy, actSelAll, actClear](const QPoint &pos) {
                    actCopy->setEnabled(console->textCursor().hasSelection());
                    const bool hasText = !console->document()->isEmpty();
                    actSelAll->setEnabled(hasText);
                    actClear->setEnabled(hasText);
                    QMenu menu;
                    menu.addAction(actCopy);
                    menu.addAction(actSelAll);
                    menu.addSeparator();
                    menu.addAction(actClear);
                    menu.exec(console->mapToGlobal(pos));
                });
    }
    consoleWindow->setWidget(console);
    consoleWindow->hide();
    tabifyDockWidget(explorerWindow, consoleWindow);
#endif

    undoStackWindow = new QDockWidget(this);
    undoStackWindow->setObjectName("undoStackWindow");
    undoStackWindow->setWindowTitle(tr("Undo Stack"));
    addDockWidget(Qt::RightDockWidgetArea, undoStackWindow);

    d_undo_view = new QUndoView(undoStackWindow);
    d_undo_view->setCleanIcon(QIcon(":/filesave.png"));

    undoStackWindow->setWidget(d_undo_view);
    undoStackWindow->hide();

    // Needs to be done after initialization of dock windows,
    // because we now use QDockWidget::toggleViewAction()
    createActions();
    initToolBars();
    initMainMenu();

    d_workspace = new QMdiArea();
    d_workspace->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
    d_workspace->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d_workspace->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d_workspace->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    setCentralWidget(d_workspace);

    setAcceptDrops(true);

    hiddenWindows = new QList<QWidget *>();

    scriptWindow = nullptr;
    d_text_editor = nullptr;
    d_default_2D_grid = new Grid();
    renamedTables = QStringList();

    if (!factorySettings)
        readSettings();

    insertTranslatedStrings();
    disableToolbars();

    connect(tablesDepend, SIGNAL(triggered(QAction *)), this, SLOT(showTable(QAction *)));

    connect(actionNextWindow, SIGNAL(triggered()), d_workspace, SLOT(activateNextSubWindow()));
    connect(actionPrevWindow, SIGNAL(triggered()), d_workspace, SLOT(activatePreviousSubWindow()));

    connect(this, SIGNAL(modified()), this, SLOT(modifiedProject()));

    connect(d_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(windowActivated(QMdiSubWindow *)));

    connect(lv, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(maximizeWindow(QTreeWidgetItem *)));
    connect(lv, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(folderItemDoubleClicked(QTreeWidgetItem *)));
    connect(lv, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showWindowPopupMenu(const QPoint &)));
    connect(lv, SIGNAL(dragItems(QList<QTreeWidgetItem *>)), this, SLOT(dragFolderItems(QList<QTreeWidgetItem *>)));
    connect(lv, SIGNAL(dropItems(QTreeWidgetItem *)), this, SLOT(dropFolderItems(QTreeWidgetItem *)));
    connect(lv, SIGNAL(renameItem(QTreeWidgetItem *)), this, SLOT(startRenameFolder(QTreeWidgetItem *)));
    connect(lv, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
    connect(lv, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));
    connect(lv, SIGNAL(itemRenamed(QTreeWidgetItem *, int, const QString &)), this,
            SLOT(renameWindow(QTreeWidgetItem *, int, const QString &)));

    connect(scriptEnv, SIGNAL(error(const QString &, const QString &, int)), this,
            SLOT(scriptError(const QString &, const QString &, int)));
    connect(scriptEnv, SIGNAL(print(const QString &)), this, SLOT(scriptPrint(const QString &)));

    connect(recent, &QMenu::triggered, this, &ApplicationWindow::openRecentProject);

    connect(explorerWindow, &QDockWidget::dockLocationChanged, this, &ApplicationWindow::updateExplorerWindowLayout);

	scriptEnv->initialize();

    updateAppFonts();

    scriptFitRunning = false;
    d_fitFromScriptWindow = false;
    setScriptingLanguage(defaultScriptingLang);
    setAppColors(workspaceColor, panelsColor, panelsTextColor, true);

    loadCustomActions();
    initCompleter();
    loadPlugins();

    if (defaultScriptingLang.isEmpty())
    {
#ifdef SCRIPTING_PYTHON
        defaultScriptingLang = "Python";
#else
        defaultScriptingLang = "muParser";
#endif
        setScriptingLanguage(defaultScriptingLang);
    }

#ifdef SCRIPTING_PYTHON
    if (defaultScriptingLang == QString("Python"))
        executeStartupScripts();
#endif

    changeFontSasWidgets();

    if (!factorySettings)
    {
        changeSasReso();

#ifdef QTISAS
        switch (int(fabs(sasDefaultInterface)))
        {
        case 1:
            bringToFront(compileWindow, actionShowCompile);
            break;
        case 2:
            bringToFront(fittableWindow, actionShowFittable);
            break;
        case 3:
            bringToFront(svdWindow, actionShowSvd);
            break;
        case 4:
            bringToFront(danWindow, actionShowDan);
            break;
        case 5:
            bringToFront(jnseWindow, actionShowJnse);
            break;
        case 6:
            bringToFront(ascii1dWindow, actionShowAscii1d);
            break;
        default:
            break;
        }
#endif
    }

    findColorMaps();

    qApp->installEventFilter(this);
    connect(qApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) { recoverOffscreenTopLevelWidgets(state); });
}

void ApplicationWindow::recoverOffscreenTopLevelWidgets(Qt::ApplicationState state, QWidget *w)
{
    if (state != Qt::ApplicationActive)
        return;

    auto fixPosition = [this](QWidget *widget) {
        if (!widget)
            return;

        QScreen *ownScreen = widget->screen();

        // Widget has a valid screen and intersects its available area.
        if (ownScreen && ownScreen->availableGeometry().intersects(widget->frameGeometry()))
            return;

        QScreen *mainScreen = this->screen();
        if (!mainScreen)
            return;

        const QRect available = mainScreen->availableGeometry();

        // Center the widget on the main screen.
        QPoint newPos = available.center() - QPoint(widget->width() / 2, widget->height() / 2);

        // Make sure at least the widget's top-left is inside
        // the available screen area.
        newPos.setX(qBound(available.left(), newPos.x(), available.right() - widget->width() + 1));
        newPos.setY(qBound(available.top(), newPos.y(), available.bottom() - widget->height() + 1));

        widget->move(newPos);
    };

    if (w)
    {
        fixPosition(w);
        return;
    }

    for (QWidget *tw : QApplication::topLevelWidgets())
    {
        if (!tw || tw == this || !tw->isVisible())
            continue;

        const Qt::WindowType wtype = tw->windowType();

        if (wtype == Qt::Popup || wtype == Qt::ToolTip)
            continue;

        QPointer<QWidget> guarded(tw);

        QTimer::singleShot(0, this, [guarded, fixPosition]() {
            if (guarded)
                fixPosition(guarded);
        });
    }
}

void ApplicationWindow::updateExplorerWindowLayout(Qt::DockWidgetArea area)
{
	switch(area){
		case Qt::LeftDockWidgetArea:
		case Qt::RightDockWidgetArea:
			explorerSplitter->setOrientation(Qt::Vertical);
		break;

		case Qt::TopDockWidgetArea:
		case Qt::BottomDockWidgetArea:
		case Qt::NoDockWidgetArea:
			explorerSplitter->setOrientation(Qt::Horizontal);
		break;

		default:
			break;
	}
}

void ApplicationWindow::initWindow()
{
	switch(d_init_window_type){
		case TableWindow:
			newTable();
		break;
		case MatrixWindow:
			newMatrix();
		break;
		case MultiLayerWindow:
			newGraph();
		break;
		case NoteWindow:
			newNote();
		break;
		default:
			break;
	}
	savedProject();
}

QString ApplicationWindow::bundledPythonFolder()
{
    const QString aux = qApp->applicationDirPath();
#ifdef Q_OS_MACOS
    return aux + "/../Resources/python";
#elif defined(Q_OS_WIN)
    return aux + "/python";
#elif defined(Q_OS_LINUX)
    for (const QString &candidate : {aux + "/../qtisas/python", QString("/usr/share/qtisas/python")})
        if (QDir(candidate).exists())
            return candidate;
#endif
    return {};
}

void ApplicationWindow::copyPythonConfigurationFiles(bool forceInit) const
{
    QDir pythonDir(bundledPythonFolder());

    QDir().mkpath(d_startup_scripts_folder);

    QDir destinationDir(d_python_config_folder);
    destinationDir.mkpath(".");

    if (forceInit)
    {
        for (const QString &file : destinationDir.entryList(QDir::Files))
            QFile::remove(destinationDir.filePath(file));
    }

    const QStringList files = pythonDir.entryList(QDir::Files);
    for (const QString &file : files)
    {
        const QString sourceFilePath = pythonDir.filePath(file);
        const QString destinationFilePath = destinationDir.filePath(file);
        // A bundled file newer than the copy replaces it, also without a version change: a rebuilt QtiSAS with the
        // same version number otherwise kept running an old qtisasrc.py (confirmed: a one-argument scriptPrint()
        // after the bundle had the print()-like one). A copy the user edited later is newer and stays.
        const QFileInfo destination(destinationFilePath);
        if (destination.exists() && destination.lastModified() < QFileInfo(sourceFilePath).lastModified())
        {
            QFile::remove(destinationFilePath);
            // its compiled .pyc can be newer than the replacement and would be run instead (loadInitFile())
            if (file.endsWith(".py"))
                QFile::remove(destinationFilePath + "c");
        }
        QFile::copy(sourceFilePath, destinationFilePath);
    }
}

void ApplicationWindow::setDefaultOptions()
{
    bool thousandsSep = false;
    QLocale loc = QLocale::c();
    if (!thousandsSep) loc.setNumberOptions(QLocale::OmitGroupSeparator);
    setLocale(loc);
    QLocale::setDefault(loc);

    sasFontIncrement = 0;
    sasResoScale = 1.0;
    sasDefaultInterface = 0;

    d_fft_norm_amp = false;
    d_fft_shift_res = true;
    d_fft_power2 = true;

    d_int_sort_data = false;
    d_int_show_plot = true;
    d_int_results_table = true;

    d_show_empty_cell_gap = true;
    d_stats_significance_level = 0.05;
    d_stats_result_table = false;
    d_stats_result_log = true;
    d_stats_result_notes = false;
    d_descriptive_stats = true;
    d_stats_confidence = true;
    d_stats_power = true;
    d_stats_output = true;

    d_confirm_modif_2D_points = true;
    d_ask_web_connection = false;
    d_open_last_project = false;
    d_force_muParser = true;
    d_indexed_colors = ColorBox::defaultColors();
    d_indexed_color_names = ColorBox::defaultColorNames();

    d_symbols_list = SymbolBox::defaultSymbols();

    d_latex_compiler = CodeCogs;
    d_mdi_windows_area = true;
    d_open_project_filter = QString();

    d_comment_highlight_color = Qt::red;
    d_class_highlight_color = Qt::darkMagenta;
    d_numeric_highlight_color = Qt::darkGreen;
    d_keyword_highlight_color = Qt::darkBlue;
    d_function_highlight_color = Qt::blue;
    d_quotation_highlight_color = Qt::darkYellow;

    d_notes_tab_length = 20;
    d_completer = nullptr;
    d_completion = true;
    d_note_line_numbers = true;

	d_auto_update_table_values = true;
	d_show_table_paste_dialog = true;
	d_active_window = nullptr;
    d_matrix_undo_stack_size = 10;

	d_opening_file = false;
	d_is_appending_file = false;
    d_in_place_editing = true;

	d_matrix_tool_bar = true;
	d_file_tool_bar = true;
	d_table_tool_bar = true;
	d_column_tool_bar = true;
	d_edit_tool_bar = true;
	d_notes_tool_bar = true;
	d_plot_tool_bar = true;
	d_plot3D_tool_bar = true;
	d_display_tool_bar = false;
	d_format_tool_bar = true;

    appStyle = qApp->style()->objectName();

    d_app_rect = QRect();

	lastCopiedLayer = 0;
	d_enrichement_copy = nullptr;
	d_arrow_copy = nullptr;

	savingTimerId = 0;

	//show_windows_policy = ActiveFolder;
    show_windows_policy =HideAll;
	d_script_win_on_top = false;
	d_script_win_rect = QRect(0, 0, 500, 300);
	d_init_window_type = TableWindow;

	QString aux = qApp->applicationDirPath();
	workingDir = aux;

#ifdef Q_WS_X11
	d_latex_compiler_path = "/usr/bin/latex";
#endif
#ifdef Q_OS_MACOS
    d_latex_compiler_path = "/Library/TeX/texbin/latex";
#endif

	fitPluginsPath = aux;
	if ( QDir(aux+"/fitPlugins").exists())fitPluginsPath = aux+"/fitPlugins";
	if ( QDir(aux+"/../fitPlugins").exists())fitPluginsPath = aux+"/../fitPlugins";

    customActionsDirPath = QString();

#ifdef SCRIPTING_PYTHON
#ifdef Q_OS_WIN
    d_python_config_folder = QDir::homePath() + "/AppData/Local/qtisas/python";
    d_startup_scripts_folder = QDir::homePath() + "/AppData/Local/qtisas/python-scripts";
    customActionsDirPath = QDir::homePath() + "/AppData/Local/qtisas/python-actions";
#else
    d_python_config_folder = QDir::homePath() + "/.config/qtisas/python";
    d_startup_scripts_folder = QDir::homePath() + "/.config/qtisas/python-scripts";
    customActionsDirPath = QDir::homePath() + "/.config/qtisas/python-actions";
#endif
    QDir().mkpath(customActionsDirPath);
    {
        QSettings *s = Settings::DefaultSettings();
        s->beginGroup("/General");
        const bool newVersion = QVersionNumber(maj_version, min_version, patch_version) !=
                                QVersionNumber::fromString(s->value("/LastUsedVersion", "0.0.0").toString());
        s->endGroup();
        copyPythonConfigurationFiles(newVersion);
    }
#endif

    fitModelsPath = QString();
	asciiDirPath = aux;
	imagesDirPath = aux;
	scriptsDirPath = aux;

	appFont = QFont();
#ifdef Q_OS_MACOS
    appFont.setFamily("Helvetica Neue");
#endif

    d_notes_font = appFont;

	QString family = appFont.family();
	int pointSize = appFont.pointSize();

	tableTextFont = appFont;
	tableHeaderFont = appFont;
	plotAxesFont = QFont(family, pointSize + 2, QFont::Bold, false);
	plotNumbersFont = QFont(family, pointSize + 2 );
	plotLegendFont = QFont(family, pointSize + 2 );
	plotTitleFont = QFont(family, pointSize + 4, QFont::Bold,false);

	appLanguage = QLocale::system().name().section('_',0,0);
	show_windows_policy = ApplicationWindow::ActiveFolder;

	workspaceColor = QColor("darkGray");
	panelsColor = QColor("#ffffff");
	panelsTextColor = QColor("#000000");
	tableBkgdColor = QColor("#ffffff");
	tableTextColor = QColor("#000000");
	tableHeaderColor = QColor("#000000");
    tableHeaderColorRows = QColor("#000000");

	d_graph_background_color = Qt::white;
	d_graph_canvas_color = Qt::white;
	d_graph_border_color = Qt::black;
	d_graph_background_opacity = 0;
	d_graph_canvas_opacity = 0;
	d_graph_border_width = 0;
	d_graph_tick_labels_dist = 4;
	d_graph_axes_labels_dist = 2;

	autoSave = true;
	autoSaveTime = 15;
	d_backup_files = true;

	d_decimal_digits = 13;
    d_muparser_c_locale = true;

	d_extended_open_dialog = true;
	d_extended_export_dialog = true;
	d_extended_import_ASCII_dialog = true;
	d_extended_plot_dialog = true;

	d_add_curves_dialog_size = QSize(700, 400);
    d_add_curves_dialog_filter = "*";
    d_add_curves_dialog_style = int(Graph::LineSymbols);
	d_show_current_folder = false;

	d_confirm_overwrite = true;
	confirmCloseFolder = true;
	confirmCloseTable = true;
	confirmCloseMatrix = true;
	confirmClosePlot2D = true;
	confirmClosePlot3D = true;
	confirmCloseNotes = true;
	d_inform_rename_table = true;

	d_show_table_comments = false;

	titleOn = true;
	d_show_axes = QVector<bool> (QwtPlot::axisCnt, true);
	d_show_axes_labels = QVector<bool> (QwtPlot::axisCnt, true);
    d_show_axes_labels[1] = false;
    d_show_axes_labels[3] = false;

    canvasFrameWidth = 2;
	d_canvas_frame_color = Qt::black;
	defaultPlotMargin = 0;
    drawBackbones = false;
    axesLineWidth = 2;
	autoscale2DPlots = true;
    autoScaleFonts = false;
    autoResizeLayers = false;
	antialiasing2DPlots = false;
	d_scale_plots_on_print = false;
	d_print_cropmarks = false;
	d_graph_legend_display = Graph::Auto;
    d_graph_attach_policy = (FrameWidget::AttachPolicy)0;
    d_graph_clipboard_format = 1;
	d_graph_axis_labeling = Graph::Default;
	d_synchronize_graph_scales = true;
    d_print_paper_size = QPageSize::A4;
    d_printer_orientation = QPageLayout::Landscape;
	defaultCurveStyle = int(Graph::LineSymbols);
    defaultCurveLineWidth = 2;
    d_curve_line_style = 0;
	defaultCurveBrush = 0;
	defaultCurveAlpha = 100;
	defaultSymbolSize = 7;
	defaultSymbolEdge = 1.0;
	d_fill_symbols = true;
	d_symbol_style = 0;
	d_indexed_symbols = true;
    d_indexed_symbol_colors = true;
    d_symbol_color = 0;
	d_keep_aspect_ration = true;

	d_disable_curve_antialiasing = true;
	d_curve_max_antialising_size = 1000;

    majTicksStyle = int(ScaleDraw::In);
    minTicksStyle = int(ScaleDraw::In);
	minTicksLength = 5;
	majTicksLength = 9;

	legendFrameStyle = int(LegendWidget::Line);
    d_frame_widget_pen = QPen(Qt::black, 2, Qt::SolidLine);
	legendTextColor = Qt::black;
	legendBackground = Qt::white;
    legendBackground.setAlpha(255);
	d_legend_default_angle = 0;
	d_frame_geometry_unit = (int)FrameWidget::Scale;
	d_layer_geometry_unit = (int)FrameWidget::Pixel;

    QRect rec = QGuiApplication::primaryScreen()->availableGeometry();
    int sHintHight = rec.height();
    d_layer_canvas_width = int(double(sHintHight) / 2.0);
    d_layer_canvas_height = d_layer_canvas_width;

	d_rect_default_background = Qt::white;
	d_rect_default_brush = QBrush(Qt::white);

    defaultArrowLineWidth = 2;
	defaultArrowColor = Qt::black;
	defaultArrowHeadLength = 4;
	defaultArrowHeadAngle = 45;
	defaultArrowHeadFill = true;
	defaultArrowLineStyle = Graph::getPenStyle("SolidLine");

	d_3D_legend = true;
	d_3D_projection = NOFLOOR;
	d_3D_smooth_mesh = true;
	d_3D_resolution = 1;
	d_3D_orthogonal = false;
	d_3D_autoscale = true;
	d_3D_axes_font = QFont(family, pointSize, QFont::Normal, false);
	d_3D_numbers_font = QFont(family, pointSize);
	d_3D_title_font = QFont(family, pointSize + 2, QFont::Normal, false);
    currentColorMap = 5;
    d_3D_color_map = ColorMapEditor::presetColorMap(5, sasPath);
	d_3D_mesh_color = Qt::black;
	d_3D_axes_color = Qt::black;
	d_3D_numbers_color = Qt::black;
	d_3D_labels_color = Qt::black;
	d_3D_background_color = Qt::white;
	d_3D_grid_color = Qt::blue;
	d_3D_minor_grid_color = Qt::lightGray;
	d_3D_minor_grids = true;
	d_3D_major_grids = true;
	d_3D_major_style = Qwt3D::SOLID;
	d_3D_minor_style = Qwt3D::DASH;
	d_3D_major_width = 1.0;
	d_3D_minor_width = 0.8;

	fit_output_precision = 13;
	pasteFitResultsToPlot = false;
	writeFitResultsToLog = true;
	generateUniformFitPoints = true;
	fitPoints = 100;
	generatePeakCurves = true;
	peakCurvesColor = Qt::green;
	fit_scale_errors = true;
	d_2_linear_fit_points = true;
	d_multi_peak_messages = true;

	columnSeparator = "\t";
	ignoredLines = 0;
	renameColumns = false;
	strip_spaces = true;
	simplify_spaces = true;
	d_ASCII_file_filter = "*";
    d_ASCII_import_locale = QLocale::c(); // QLocale::system().name();
	d_ASCII_import_mode = int(ImportASCIIDialog::NewTables);
    d_ASCII_import_first_row_role = 0;
	d_ASCII_comment_string = "#";
	d_ASCII_import_comments = false;
	d_ASCII_import_read_only = false;
	d_ASCII_import_preview = true;
	d_preview_lines = 10;
	d_import_ASCII_dialog_size = QSize();
    d_ASCII_end_line = LF;
	d_eol = LF;

	d_export_col_separator = "\t";
	d_export_col_names = false;
    d_export_col_comment = false;
	d_export_table_selection = false;
	d_export_ASCII_file_filter = ".dat";

	d_scale_fonts_factor = 0.0;
	d_image_export_filter = ".png";
	d_export_transparency = false;
	d_export_quality = 100;
	d_export_compression = 1;
	d_export_raster_size = QSizeF();
	d_export_size_unit = FrameWidget::Pixel;
	d_export_vector_resolution = QWidget().logicalDpiX();
	d_export_bitmap_resolution = d_export_vector_resolution;
	d_export_color = true;
	d_export_escape_tex_strings = true;
	d_export_tex_font_sizes = true;
    d_3D_export_text_mode = 1;
    d_3D_export_sort = 1;
}

void ApplicationWindow::setupActionIcon(QAction *action, const QIcon &icon)
{
    if (!icon.isNull())
        action->setIcon(icon);
#ifdef Q_OS_MACOS
    action->setIconVisibleInMenu(true);
#endif
}

void ApplicationWindow::initToolBars()
{
	initPlot3DToolBar();

	setWindowIcon(QIcon(":/logo.png"));

	QPixmap openIcon, saveIcon;

    fileTools = new QToolBar(tr("File"), this);
    fileTools->setObjectName("fileTools");
    addToolBar(Qt::TopToolBarArea, fileTools);
#ifdef Q_OS_MACOS
    fileTools->setIconSize(QSize(20, 20));
#endif
	fileTools->addAction(actionNewProject);
	fileTools->addAction(actionNewFolder);
	fileTools->addAction(actionNewTable);
	fileTools->addAction(actionNewMatrix);
	fileTools->addAction(actionNewNote);
	fileTools->addAction(actionNewGraph);
	fileTools->addAction(actionNewFunctionPlot);
	fileTools->addAction(actionNewSurfacePlot);
	fileTools->addSeparator ();
	fileTools->addAction(actionOpen);
	fileTools->addAction(actionOpenTemplate);
	fileTools->addAction(actionAppendProject);
	fileTools->addAction(actionSaveProject);
	fileTools->addAction(actionSaveTemplate);
	fileTools->addSeparator ();
	fileTools->addAction(actionLoad);
	fileTools->addSeparator ();
	fileTools->addAction(actionCopyWindow);
	fileTools->addAction(actionPrint);
	fileTools->addAction(actionPrintPreview);
	fileTools->addAction(actionExportPDF);
	fileTools->addSeparator();
	fileTools->addAction(actionShowExplorer);
	fileTools->addAction(actionShowLog);
#ifdef SCRIPTING_PYTHON
	fileTools->addAction(actionShowScriptWindow);
#endif
    editTools = new QToolBar(tr("Edit"), this);
    editTools->setObjectName("editTools");
    addToolBar(editTools);
#ifdef Q_OS_MACOS
    editTools->setIconSize(QSize(20, 20));
#endif
	editTools->addAction(actionUndo);
	editTools->addAction(actionRedo);
	editTools->addAction(actionCutSelection);
	editTools->addAction(actionCopySelection);
	editTools->addAction(actionPasteSelection);
	editTools->addAction(actionClearSelection);

    noteTools = new QToolBar(tr("Notes"), this);
    noteTools->setObjectName("noteTools");
#ifdef Q_OS_MACOS
    noteTools->setIconSize(QSize(20, 20));
#endif
#ifdef SCRIPTING_PYTHON
	noteTools->addAction(actionCommentSelection);
	noteTools->addAction(actionUncommentSelection);
#endif
	noteTools->addAction(actionDecreaseIndent);
	noteTools->addAction(actionIncreaseIndent);
	noteTools->addAction(actionFind);
	noteTools->addAction(actionFindPrev);
	noteTools->addAction(actionFindNext);
	noteTools->addAction(actionReplace);
	addToolBar( noteTools );

    plotTools = new QToolBar(tr("Plot"), this);
    plotTools->setObjectName("plotTools");
#ifdef Q_OS_MACOS
    plotTools->setIconSize(QSize(20, 20));
#endif
    addToolBar(Qt::LeftToolBarArea, plotTools);

	plotTools->addAction(actionAddLayer);
	plotTools->addAction(actionAddInsetLayer);
	plotTools->addAction(actionAddInsetCurveLayer);
	plotTools->addSeparator();
	plotTools->addAction(actionShowLayerDialog);
	plotTools->addAction(actionAutomaticLayout);
    plotTools->addAction(actionLogLog);
    plotTools->addAction(actionLinLin);
	plotTools->addSeparator();
	plotTools->addAction(actionExtractLayers);
	plotTools->addAction(actionExtractGraphs);
	plotTools->addSeparator();
	plotTools->addAction(actionAddErrorBars);
	plotTools->addAction(actionShowCurvesDialog);
	plotTools->addAction(actionAddFunctionCurve);
	plotTools->addAction(actionNewLegend);
	plotTools->addSeparator ();
    plotTools->addAction(actionSaveGraphAsProject);
    plotTools->addSeparator ();
	plotTools->addAction(actionUnzoom);

	dataTools = new QActionGroup( this );
	dataTools->setExclusive( true );

	btnPointer = new QAction(tr("Disable &Tools"), this);
	btnPointer->setActionGroup(dataTools);
	btnPointer->setCheckable( true );
	btnPointer->setIcon(QIcon(":/pointer.png"));
	btnPointer->setChecked(true);
	plotTools->addAction(btnPointer);

	actionMagnify->setActionGroup(dataTools);
	actionMagnify->setCheckable( true );

	actionMagnifyHor->setActionGroup(dataTools);
	actionMagnifyHor->setCheckable( true );

	actionMagnifyVert->setActionGroup(dataTools);
	actionMagnifyVert->setCheckable( true );

	btnZoomIn = new QAction(tr("&Zoom In"), this);
	btnZoomIn->setShortcut( tr("Ctrl++") );
	btnZoomIn->setActionGroup(dataTools);
	btnZoomIn->setCheckable( true );
	btnZoomIn->setIcon(QIcon(":/zoom.png"));

	btnZoomOut = new QAction(tr("&Zoom Out"), this);
	btnZoomOut->setShortcut( tr("Ctrl+-") );
	btnZoomOut->setActionGroup(dataTools);
	btnZoomOut->setCheckable( true );
	btnZoomOut->setIcon(QIcon(":/zoomOut.png"));

	QMenu *menu_zoom = new QMenu(this);
	menu_zoom->addAction(actionMagnify);
	menu_zoom->addAction(actionMagnifyHor);
	menu_zoom->addAction(actionMagnifyVert);
	menu_zoom->addAction(btnZoomIn);
	menu_zoom->addAction(btnZoomOut);

	QToolButton *btn_zoom = new QToolButton(this);
	btn_zoom->setMenu(menu_zoom);
	btn_zoom->setPopupMode(QToolButton::MenuButtonPopup);
	btn_zoom->setDefaultAction(actionMagnify);
	connect(menu_zoom, SIGNAL(triggered(QAction *)), btn_zoom, SLOT(setDefaultAction(QAction *)));

	plotTools->addWidget(btn_zoom);

	btnCursor = new QAction(tr("&Data Reader"), this);
	btnCursor->setShortcut( tr("CTRL+D") );
	btnCursor->setActionGroup(dataTools);
	btnCursor->setCheckable( true );
	btnCursor->setIcon(QIcon(":/select.png"));
	plotTools->addAction(btnCursor);

	btnSelect = new QAction(tr("&Select Data Range"), this);
	btnSelect->setShortcut(QKeySequence(tr("Alt+Shift+S")));
	btnSelect->setActionGroup(dataTools);
	btnSelect->setCheckable( true );
	btnSelect->setIcon(QIcon(":/cursors.png"));
	plotTools->addAction(btnSelect);

	btnPicker = new QAction(tr("S&creen Reader"), this);
	btnPicker->setActionGroup(dataTools);
	btnPicker->setCheckable( true );
	btnPicker->setIcon(QIcon(":/cursor_16.png"));
	plotTools->addAction(btnPicker);

	actionDrawPoints = new QAction(tr("&Draw Data Points"), this);
	actionDrawPoints->setActionGroup(dataTools);
	actionDrawPoints->setCheckable( true );
	actionDrawPoints->setIcon(QIcon(":/draw_points.png")); // no xpm
	plotTools->addAction(actionDrawPoints);

	btnMovePoints = new QAction(tr("&Move Data Points..."), this);
	btnMovePoints->setShortcut( tr("Ctrl+ALT+M") );
	btnMovePoints->setActionGroup(dataTools);
	btnMovePoints->setCheckable( true );
	btnMovePoints->setIcon(QIcon(":/hand.png"));
	plotTools->addAction(btnMovePoints);

	btnRemovePoints = new QAction(tr("Remove &Bad Data Points..."), this);
	btnRemovePoints->setShortcut( tr("Alt+B") );
	btnRemovePoints->setActionGroup(dataTools);
	btnRemovePoints->setCheckable( true );
	btnRemovePoints->setIcon(QIcon(":/delete.png"));
	plotTools->addAction(btnRemovePoints);

	actionDragCurve = new QAction(tr("Dra&g Curve"), this);
	actionDragCurve->setActionGroup(dataTools);
	actionDragCurve->setCheckable( true );
	actionDragCurve->setIcon(QIcon(":/drag_curve.png"));
	plotTools->addAction(actionDragCurve);

	connect( dataTools, SIGNAL( triggered( QAction* ) ), this, SLOT( pickDataTool( QAction* ) ) );
	plotTools->addSeparator ();

	actionAddFormula = new QAction(tr("Add E&quation"), this);
	actionAddFormula->setShortcut( tr("ALT+Q") );
	actionAddFormula->setCheckable(true);
    setupActionIcon(actionAddFormula, QIcon(":/formula.png"));
	connect(actionAddFormula, SIGNAL(triggered()), this, SLOT(addTexFormula()));
	plotTools->addAction(actionAddFormula);

	actionAddText = new QAction(tr("Add &Text"), this);
	actionAddText->setShortcut(QKeySequence(tr("Shift+T")));
	actionAddText->setCheckable(true);
    setupActionIcon(actionAddText, QIcon(":/text.png"));
	connect(actionAddText, SIGNAL(triggered()), this, SLOT(addText()));
	plotTools->addAction(actionAddText);

	btnArrow = new QAction(tr("Draw &Arrow"), this);
	btnArrow->setShortcut( tr("CTRL+ALT+A") );
	btnArrow->setActionGroup(dataTools);
	btnArrow->setCheckable( true );
    setupActionIcon(btnArrow, QIcon(":/arrow.png"));
	plotTools->addAction(btnArrow);

	btnLine = new QAction(tr("Draw &Line"), this);
	btnLine->setShortcut( tr("CTRL+ALT+L") );
	btnLine->setActionGroup(dataTools);
	btnLine->setCheckable( true );
    setupActionIcon(btnLine, QIcon(":/lPlot.png"));
	plotTools->addAction(btnLine);

	QPixmap pix = QPixmap(16, 16);
	pix.fill(Qt::transparent);
	QPainter p;
	p.begin(&pix);
	p.setBrush(Qt::lightGray);
	p.drawRect(QRect(1, 2, 12, 10));

    actionAddRectangle = new QAction(tr("Add &Rectangle"), this);
	actionAddRectangle->setShortcut( tr("CTRL+ALT+R") );
	actionAddRectangle->setCheckable(true);
    setupActionIcon(actionAddRectangle, QIcon(pix));
	connect(actionAddRectangle, SIGNAL(triggered()), this, SLOT(addRectangle()));
	plotTools->addAction(actionAddRectangle);

	pix.fill(Qt::transparent);
	p.setRenderHint(QPainter::Antialiasing);
	p.drawEllipse(QRect(1, 2, 14, 12));
	p.end();

	actionAddEllipse = new QAction(tr("Add &Ellipse"), this);
	actionAddEllipse->setShortcut( tr("CTRL+ALT+E") );
	actionAddEllipse->setCheckable(true);
    setupActionIcon(actionAddEllipse, QIcon(pix));
	connect(actionAddEllipse, SIGNAL(triggered()), this, SLOT(addEllipse()));
	plotTools->addAction(actionAddEllipse);

	plotTools->addAction(actionTimeStamp);
	plotTools->addAction(actionAddImage);

	plotTools->addSeparator();
	plotTools->addAction(actionRaiseEnrichment);
	plotTools->addAction(actionLowerEnrichment);
	plotTools->addAction(actionAlignLeft);
	plotTools->addAction(actionAlignRight);
	plotTools->addAction(actionAlignTop);
	plotTools->addAction(actionAlignBottom);

	plotTools->hide();

    tableTools = new QToolBar(tr("Table"), this);
    tableTools->setObjectName("tableTools");
    addToolBar(Qt::TopToolBarArea, tableTools);
#ifdef Q_OS_MACOS
    tableTools->setIconSize(QSize(20, 20));
#endif
	QMenu *menuPlotLine = new QMenu(this);
	menuPlotLine->addAction(actionPlotL);
	menuPlotLine->addAction(actionPlotHorSteps);
	menuPlotLine->addAction(actionPlotVertSteps);

	QToolButton *btnPlotLine = new QToolButton(this);
	btnPlotLine->setMenu(menuPlotLine);
	btnPlotLine->setPopupMode(QToolButton::MenuButtonPopup);
	btnPlotLine->setDefaultAction(actionPlotL);
	connect(menuPlotLine, SIGNAL(triggered(QAction *)), btnPlotLine, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnPlotLine);

	QMenu *menuPlotScatter = new QMenu(this);
	menuPlotScatter->addAction(actionPlotP);
	menuPlotScatter->addAction(actionPlotVerticalDropLines);

	QToolButton *btnPlotScatter = new QToolButton(this);
	btnPlotScatter->setMenu(menuPlotScatter);
	btnPlotScatter->setPopupMode(QToolButton::MenuButtonPopup);
	btnPlotScatter->setDefaultAction(actionPlotP);
	connect(menuPlotScatter, SIGNAL(triggered(QAction *)), btnPlotScatter, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnPlotScatter);

	QMenu *menuPlotLineSymbol = new QMenu(this);
	menuPlotLineSymbol->addAction(actionPlotLP);
	menuPlotLineSymbol->addAction(actionPlotSpline);

	QToolButton *btnPlotLineSymbol = new QToolButton(this);
	btnPlotLineSymbol->setMenu(menuPlotLineSymbol);
	btnPlotLineSymbol->setPopupMode(QToolButton::MenuButtonPopup);
	btnPlotLineSymbol->setDefaultAction(actionPlotLP);
	connect(menuPlotLineSymbol, SIGNAL(triggered(QAction *)), btnPlotLineSymbol, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnPlotLineSymbol);

	QMenu *menuPlotBars = new QMenu(this);
	menuPlotBars->addAction(actionPlotVerticalBars);
	menuPlotBars->addAction(actionPlotHorizontalBars);
	menuPlotBars->addAction(actionStackColumns);
	menuPlotBars->addAction(actionStackBars);

	QToolButton *btnPlotBars = new QToolButton(this);
	btnPlotBars->setMenu(menuPlotBars);
	btnPlotBars->setPopupMode(QToolButton::MenuButtonPopup);
	btnPlotBars->setDefaultAction(actionPlotVerticalBars);
	connect(menuPlotBars, SIGNAL(triggered(QAction *)), btnPlotBars, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnPlotBars);

	tableTools->addAction(actionPlotArea);
	tableTools->addAction(actionPlotPie);

	QMenu *menuStatisticPlots = new QMenu(this);
	menuStatisticPlots->addAction(actionBoxPlot);
	menuStatisticPlots->addAction(actionPlotHistogram);
	menuStatisticPlots->addAction(actionPlotStackedHistograms);
	menuStatisticPlots->addAction(actionStemPlot);

	QToolButton *btnStatisticPlots = new QToolButton(this);
	btnStatisticPlots->setMenu(menuStatisticPlots);
	btnStatisticPlots->setPopupMode(QToolButton::MenuButtonPopup);
	btnStatisticPlots->setDefaultAction(actionBoxPlot);
	connect(menuStatisticPlots, SIGNAL(triggered(QAction *)), btnStatisticPlots, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnStatisticPlots);

	QMenu *menuVectorPlots = new QMenu(this);
	menuVectorPlots->addAction(actionPlotVectXYXY);
	menuVectorPlots->addAction(actionPlotVectXYAM);

	QToolButton *btnVectorPlots = new QToolButton(this);
	btnVectorPlots->setMenu(menuVectorPlots);
	btnVectorPlots->setPopupMode(QToolButton::MenuButtonPopup);
	btnVectorPlots->setDefaultAction(actionPlotVectXYXY);
	connect(menuVectorPlots, SIGNAL(triggered(QAction *)), btnVectorPlots, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnVectorPlots);

	QMenu *menuPlotSpecial = new QMenu(this);
	menuPlotSpecial->addAction(actionPlotDoubleYAxis);
	menuPlotSpecial->addAction(actionWaterfallPlot);
	menuPlotSpecial->addAction(actionAddZoomPlot);
	menuPlotSpecial->addAction(actionPlot2VerticalLayers);
	menuPlotSpecial->addAction(actionPlot2HorizontalLayers);
	menuPlotSpecial->addAction(actionPlot4Layers);
	menuPlotSpecial->addAction(actionPlotStackedLayers);
	menuPlotSpecial->addAction(actionCustomSharedAxisLayers);

	QToolButton *btnPlotSpecial = new QToolButton(this);
	btnPlotSpecial->setMenu(menuPlotSpecial);
	btnPlotSpecial->setPopupMode(QToolButton::MenuButtonPopup);
	btnPlotSpecial->setDefaultAction(actionPlotDoubleYAxis);
	connect(menuPlotSpecial, SIGNAL(triggered(QAction *)), btnPlotSpecial, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btnPlotSpecial);

	tableTools->addSeparator ();

	QMenu *menu3DPlots = new QMenu(this);
	menu3DPlots->addAction(actionPlot3DBars);
	menu3DPlots->addAction(actionPlot3DRibbon);
	menu3DPlots->addAction(actionPlot3DScatter);
	menu3DPlots->addAction(actionPlot3DTrajectory);

	QToolButton *btn3DPlots = new QToolButton(this);
	btn3DPlots->setMenu(menu3DPlots);
	btn3DPlots->setPopupMode(QToolButton::MenuButtonPopup);
#ifdef Q_OS_MACOS
    btn3DPlots->setIconSize(QSize(20, 20));
#endif
	btn3DPlots->setDefaultAction(actionPlot3DBars);
	connect(menu3DPlots, SIGNAL(triggered(QAction *)), btn3DPlots, SLOT(setDefaultAction(QAction *)));
	tableTools->addWidget(btn3DPlots);

	tableTools->setEnabled(false);
    tableTools->hide();

    columnTools = new QToolBar(tr("Column"), this);
    columnTools->setObjectName("columnTools");
    addToolBar(Qt::TopToolBarArea, columnTools);
#ifdef Q_OS_MACOS
    columnTools->setIconSize(QSize(20, 20));
#endif
	columnTools->addAction(actionShowColumnValuesDialog);
	columnTools->addAction(actionSetAscValues);
	columnTools->addAction(actionSetRandomValues);
	columnTools->addAction(actionSetRandomNormalValues);
	columnTools->addSeparator();
	columnTools->addAction(actionSortTable);
	columnTools->addAction(actionShowColStatistics);
	columnTools->addAction(actionShowRowStatistics);
	columnTools->addSeparator();
	columnTools->addAction(actionSetXCol);
	columnTools->addAction(actionSetYCol);
	columnTools->addAction(actionSetZCol);
	columnTools->addAction(actionSetYErrCol);
	columnTools->addAction(actionSetLabelCol);
	columnTools->addAction(actionDisregardCol);
	columnTools->addSeparator();
	columnTools->addAction(actionMoveColFirst);
	columnTools->addAction(actionMoveColLeft);
	columnTools->addAction(actionMoveColRight);
	columnTools->addAction(actionMoveColLast);
	columnTools->addAction(actionSwapColumns);
	columnTools->addSeparator();
	columnTools->addAction(actionAdjustColumnWidth);
	columnTools->addAction(actionMoveRowUp);
	columnTools->addAction(actionMoveRowDown);
	columnTools->addSeparator();
	columnTools->addAction(actionAddColToTable);
    columnTools->setEnabled(false);
	columnTools->hide();

	displayBar = new QToolBar( tr( "Data Display" ), this );
    displayBar->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    displayBar->setObjectName("displayBar");
#ifdef Q_OS_MACOS
    displayBar->setIconSize(QSize(20, 20));
#endif
	info = new QLineEdit( this );
	displayBar->addWidget( info );
	info->setReadOnly(true);
    QPalette palette;
    palette.setColor(QPalette::Text, QColor(Qt::green));
	palette.setColor(QPalette::HighlightedText, QColor(Qt::darkGreen));
	palette.setColor(QPalette::Base, QColor(Qt::black));
	info->setPalette(palette);

	addToolBar( Qt::TopToolBarArea, displayBar );
	displayBar->hide();

    insertToolBarBreak (displayBar);

    qtisasToolBar = new QToolBar(tr("QtiSAS"), this);
    qtisasToolBar->setObjectName("QtiSAS");
#ifdef Q_OS_MACOS
    qtisasToolBar->setIconSize(QSize(20, 20));
#endif
    addToolBar(Qt::RightToolBarArea, qtisasToolBar);

#ifdef QTISAS
    qtisasToolBar->addSeparator();
	qtisasToolBar->addAction(actionShowDan);
    qtisasToolBar->addSeparator();
	qtisasToolBar->addAction(actionShowCompile);
    qtisasToolBar->addSeparator();
    qtisasToolBar->addAction(actionShowFittable);
    qtisasToolBar->addSeparator();
    qtisasToolBar->addAction(actionShowSvd);
    qtisasToolBar->addSeparator();
    qtisasToolBar->addAction(actionShowJnse);
    qtisasToolBar->addSeparator();
    qtisasToolBar->addAction(actionShowAscii1d);
    qtisasToolBar->addSeparator();
#endif

    d_status_info = new QLabel(this);
    d_status_info->setFrameShape(QFrame::StyledPanel);
    d_status_info->setFrameShadow(QFrame::Sunken);
    d_status_info->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d_status_info->setContextMenuPolicy(Qt::CustomContextMenu);
    d_status_info->setStyleSheet("color: grey");
    // Cap to exactly one text line — frame margins added on top of the font's own line height so
    // the sunken border doesn't clip the glyphs.
    d_status_info->setMaximumHeight(d_status_info->fontMetrics().height() + 6);
    d_status_info->setMinimumWidth(250);
    connect(d_status_info, SIGNAL(customContextMenuRequested(const QPoint &)), this,
            SLOT(showStatusBarContextMenu(const QPoint &)));

    terminal_line = new QLineEdit(this);
    QPalette palette1;
    palette1.setColor(terminal_line->backgroundRole(), QColor(220, 235, 255));
    terminal_line->setPalette(palette1);
    terminal_line->setPlaceholderText(
        "type command here [ '?' = help | /ai | /ai-run | /ai-chat | /python | /muparser ]");
    terminal_line->setReadOnly(0);
    terminal_line->setMinimumWidth(250);

    m_terminalModeLabel = new QLabel(scriptEnv->objectName().toLower(), terminal_line);
    m_terminalModeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_terminalModeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_terminalModeLabel->setStyleSheet("color: grey; background: transparent;");
    const int suffixWidth = qMax(m_terminalModeLabel->fontMetrics().horizontalAdvance("muparser"),
                                 qMax(m_terminalModeLabel->fontMetrics().horizontalAdvance("python"),
                                      m_terminalModeLabel->fontMetrics().horizontalAdvance("ai-chat")));
    m_terminalModeLabel->setFixedWidth(suffixWidth);
    auto *terminalLayout = new QHBoxLayout(terminal_line);
    terminalLayout->setContentsMargins(0, 0, 6, 0);
    terminalLayout->addStretch();
    terminalLayout->addWidget(m_terminalModeLabel);
    terminal_line->setTextMargins(0, 0, suffixWidth + 12, 0);

    connect(terminal_line, &QLineEdit::returnPressed, this,
            [this]() { submitTerminalLine(QApplication::keyboardModifiers()); });

    auto terminalPlus = new QSplitter(Qt::Horizontal, this);

    terminalPlus->addWidget(terminal_line);
    terminalPlus->addWidget(d_status_info);
    statusBar()->addWidget(terminalPlus, 1);

    QList<int> splitterSizes;
    terminalPlus->setSizes( splitterSizes << 60 << 40);

	plotMatrixBar = new QToolBar( tr( "Matrix Plot" ), this);
	plotMatrixBar->setObjectName("plotMatrixBar");
	addToolBar(Qt::BottomToolBarArea, plotMatrixBar);
#ifdef Q_OS_MACOS
    plotMatrixBar->setIconSize(QSize(20, 20));
#endif
	QMenu *menu3DMatrix = new QMenu(this);
	menu3DMatrix->addAction(actionPlot3DWireFrame);
	menu3DMatrix->addAction(actionPlot3DHiddenLine);
	menu3DMatrix->addAction(actionPlot3DWireSurface);
	menu3DMatrix->addAction(actionPlot3DPolygons);

	QToolButton *btn3DMatrix = new QToolButton(this);
	btn3DMatrix->setMenu(menu3DMatrix);
	btn3DMatrix->setPopupMode(QToolButton::MenuButtonPopup);
	btn3DMatrix->setDefaultAction(actionPlot3DWireSurface);
	connect(menu3DMatrix, SIGNAL(triggered(QAction *)), btn3DMatrix, SLOT(setDefaultAction(QAction *)));
	plotMatrixBar->addWidget(btn3DMatrix);

    plotMatrixBar->addAction(actionPlot3DBars);
    plotMatrixBar->addAction(actionPlot3DScatter);

	plotMatrixBar->addSeparator();

	QMenu *menuContourPlot = new QMenu(this);
	menuContourPlot->addAction(actionColorMap);
	menuContourPlot->addAction(actionContourMap);
	menuContourPlot->addAction(actionGrayMap);

	QToolButton *btnContourPlot = new QToolButton(this);
	btnContourPlot->setMenu(menuContourPlot);
	btnContourPlot->setPopupMode(QToolButton::MenuButtonPopup);
	btnContourPlot->setDefaultAction(actionColorMap);
	connect(menuContourPlot, SIGNAL(triggered(QAction *)), btnContourPlot, SLOT(setDefaultAction(QAction *)));
	plotMatrixBar->addWidget(btnContourPlot);

    plotMatrixBar->addAction(actionImagePlot);
    plotMatrixBar->addAction(actionImageProfilesPlot);
    plotMatrixBar->addAction(actionPlotHistogram);
	plotMatrixBar->addSeparator();
    plotMatrixBar->addAction(actionSetMatrixValues);
    plotMatrixBar->addAction(actionFlipMatrixHorizontally);
    plotMatrixBar->addAction(actionFlipMatrixVertically);
    plotMatrixBar->addAction(actionRotateMatrix);
    plotMatrixBar->addAction(actionRotateMatrixMinus);
	plotMatrixBar->addSeparator();
	plotMatrixBar->addAction(actionIncreasePrecision);
	plotMatrixBar->addAction(actionDecreasePrecision);
	plotMatrixBar->hide();

	formatToolBar = new QToolBar(tr( "Format" ), this);
	formatToolBar->setObjectName("formatToolBar");
    if (screenResoHight<960) addToolBar(Qt::BottomToolBarArea, formatToolBar);
	else addToolBar(Qt::TopToolBarArea, formatToolBar);
#ifdef Q_OS_MACOS
    formatToolBar->setIconSize(QSize(20, 20));
#endif
	QFontComboBox *fb = new QFontComboBox();
	connect(fb, SIGNAL(currentFontChanged(const QFont &)), this, SLOT(setFontFamily(const QFont &)));
	actionFontBox = formatToolBar->addWidget(fb);

	QSpinBox *sb = new QSpinBox();
	connect(sb, SIGNAL(valueChanged(int)), this, SLOT(setFontSize(int)));
	actionFontSize = formatToolBar->addWidget(sb);

    formatToolBar->addAction(actionFontBold);
    formatToolBar->addAction(actionFontItalic);

    formatToolBar->addAction(actionUnderline);
    formatToolBar->addAction(actionSuperscript);
    formatToolBar->addAction(actionSubscript);
    formatToolBar->addAction(actionGreekSymbol);
    formatToolBar->addAction(actionGreekMajSymbol);
    formatToolBar->addAction(actionMathSymbol);
    formatToolBar->addAction(actionUnicodeSymbol);

    auto *cBtn = new ColorButton(formatToolBar);
    connect(cBtn, &ColorButton::colorChanged, this, &ApplicationWindow::setTextColor);
	actionTextColor = formatToolBar->addWidget(cBtn);

	formatToolBar->setEnabled(false);
	formatToolBar->hide();

	QList<QToolBar *> toolBars = toolBarsList();
	foreach (QToolBar *t, toolBars)
		connect(t, SIGNAL(actionTriggered(QAction *)), this, SLOT(performCustomAction(QAction *)));
}

void ApplicationWindow::showStatusBarContextMenu(const QPoint &pos)
{
    QMenu cm(this);
    cm.addAction(actionCopyStatusBarText);
    cm.exec(d_status_info->mapToGlobal(pos));
}

void ApplicationWindow::insertTranslatedStrings()
{
	if (projectname == "untitled")
		setWindowTitle("QTISAS - untitled");

	lv->headerItem()->setText(0, tr("Name"));
	lv->headerItem()->setText(1, tr("Type"));
	lv->headerItem()->setText(2, tr("View"));
	//lv->setColumnText (3, tr("Size"));
	lv->headerItem()->setText(3, tr("Created"));
	lv->headerItem()->setText(4, tr("Label"));

	if (scriptWindow)
		scriptWindow->setWindowTitle("QTISAS - Script Window");
	explorerWindow->setWindowTitle(tr("Project Explorer"));
	logWindow->setWindowTitle(tr("Results Log"));
	undoStackWindow->setWindowTitle(tr("Undo Stack"));
#ifdef SCRIPTING_CONSOLE
	consoleWindow->setWindowTitle(tr("Scripting Console"));
#endif
	displayBar->setWindowTitle(tr("Data Display"));
	tableTools->setWindowTitle(tr("Table"));
	columnTools->setWindowTitle(tr("Column"));
	plotTools->setWindowTitle(tr("Plot"));
	fileTools->setWindowTitle(tr("File"));
	editTools->setWindowTitle(tr("Edit"));
	noteTools->setWindowTitle(tr("Notes"));
	plotMatrixBar->setWindowTitle(tr("Matrix Plot"));
	plot3DTools->setWindowTitle(tr("3D Surface"));
	formatToolBar->setWindowTitle(tr("Format"));

    if (recentMenuID >= 0 && recentMenuID < fileMenu->actions().count())
        fileMenu->actions().at(recentMenuID)->setText(tr("&Recent Projects"));
    else fileMenu->addAction(tr("&Recent Projects"));
	fileMenu->setTitle(tr("&File"));
	newMenu->setTitle(tr("&New"));
	exportPlotMenu->setTitle(tr("&Export Graph"));
	importMenu->setTitle(tr("&Import"));
#ifdef Q_OS_MACOS
    // An invisible character: macOS does not take the menu for the Edit menu and adds no AutoFill, Dictation, Emoji
    edit->setTitle(tr("&Edit") + QChar(0x200B));
#else
    edit->setTitle(tr("&Edit"));
#endif
	view->setTitle(tr("&View"));
	graphMenu->setTitle(tr("&Graph"));
	plot3DMenu->setTitle(tr("3D &Plot"));
	matrixMenu->setTitle(tr("&Matrix"));
    plot2DMenu->setTitle(tr("&Plot"));
    plotDataMenu->setTitle(tr("&Data"));
	normMenu->setTitle(tr("&Normalize"));
	fillMenu->setTitle(tr("&Fill Columns With"));
	tableMenu->setTitle(tr("&Table"));
	smoothMenu->setTitle(tr("&Smooth"));
	filterMenu->setTitle(tr("&FFT filter"));
	decayMenu->setTitle(tr("Fit E&xponential Decay"));
	multiPeakMenu->setTitle(tr("Fit &Multi-peak"));
	analysisMenu->setTitle(tr("&Analysis"));
	format->setTitle(tr("F&ormat"));
	scriptingMenu->setTitle(tr("&Scripting"));
	windowsMenu->setTitle(tr("&Windows"));

	help->setTitle(tr("&Help"));

	translateActionsStrings();
	customMenu(activeWindow());
}

void ApplicationWindow::initMainMenu()
{
	menuBar()->setObjectName("menuBar");

	fileMenu = new QMenu(this);
	fileMenu->setObjectName("fileMenu");
	connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));
	menuBar()->addMenu(fileMenu);

	recent = new QMenu(this);
	newMenu = new QMenu(this);
	newMenu->setObjectName("newMenu");
	exportPlotMenu = new QMenu(this);
	exportPlotMenu->setObjectName("exportPlotMenu");
	importMenu = new QMenu(this);
	importMenu->setObjectName("importMenu");

	edit = new QMenu(this);
	edit->setObjectName("editMenu");
	menuBar()->addMenu(edit);

	edit->addAction(actionUndo);
	edit->addAction(actionRedo);
	edit->addSeparator();
	edit->addAction(actionCopySelection);
	edit->addAction(actionPasteSelection);
	edit->addAction(actionClearSelection);
	edit->addSeparator();
	edit->addAction(actionDeleteFitTables);
	edit->addAction(actionClearLogInfo);
	edit->addSeparator();
	edit->addAction(actionShowConfigureDialog);

	connect(edit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

	view = new QMenu(this);
	view->setObjectName("viewMenu");
	menuBar()->addMenu(view);

	view->addAction(actionToolBars);
	view->addAction(actionShowPlotWizard);
	view->addAction(actionShowExplorer);
	view->addAction(actionShowLog);
	view->addAction(actionShowUndoStack);
#ifdef SCRIPTING_CONSOLE
	view->addAction(actionShowConsole);
#endif

	scriptingMenu = new QMenu(this);
	scriptingMenu->setObjectName("scriptingMenu");
	connect(scriptingMenu, SIGNAL(aboutToShow()), this, SLOT(scriptingMenuAboutToShow()));
    menuBar()->addMenu(scriptingMenu);

	graphMenu = new QMenu(this);
	graphMenu->setObjectName("graphMenu");
	menuBar()->addMenu(graphMenu);

	graphMenu->addAction(actionShowCurvesDialog);
	graphMenu->addAction(actionAddFunctionCurve);
	graphMenu->addAction(actionAddErrorBars);
	graphMenu->addAction(actionNewLegend);
    graphMenu->addSeparator();
    graphMenu->addAction(actionSaveGraphAsProject);
    graphMenu->addSeparator ();
	graphMenu->addAction(actionAddFormula);
	graphMenu->addAction(actionAddText);
	graphMenu->addAction(btnArrow);
	graphMenu->addAction(btnLine);
	graphMenu->addAction(actionAddRectangle);
	graphMenu->addAction(actionAddEllipse);
	graphMenu->addAction(actionTimeStamp);
	graphMenu->addAction(actionAddImage);
	graphMenu->addSeparator();//layers section
	graphMenu->addAction(actionAddLayer);
	graphMenu->addAction(actionAddInsetLayer);
	graphMenu->addAction(actionAddInsetCurveLayer);
	graphMenu->addAction(actionShowLayerDialog);
	graphMenu->addAction(actionAutomaticLayout);
    graphMenu->addAction(actionLogLog);
    graphMenu->addAction(actionLinLin);
	graphMenu->addSeparator();
	graphMenu->addAction(actionExtractLayers);
	graphMenu->addAction(actionExtractGraphs);
	graphMenu->addSeparator();
	graphMenu->addAction(actionDeleteLayer);
    graphMenu->addSeparator();

	plot3DMenu = new QMenu(this);
	plot3DMenu->setObjectName("plot3DMenu");
	plot3DMenu->addAction(actionPlot3DWireFrame);
	plot3DMenu->addAction(actionPlot3DHiddenLine);
	plot3DMenu->addAction(actionPlot3DPolygons);
	plot3DMenu->addAction(actionPlot3DWireSurface);
	plot3DMenu->addSeparator();
	plot3DMenu->addAction(actionPlot3DBars);
	plot3DMenu->addAction(actionPlot3DScatter);
	plot3DMenu->addSeparator();
	plot3DMenu->addAction(actionColorMap);
	plot3DMenu->addAction(actionContourMap);
	plot3DMenu->addAction(actionGrayMap);
	plot3DMenu->addSeparator();
	plot3DMenu->addAction(actionImagePlot);
	plot3DMenu->addAction(actionImageProfilesPlot);
	plot3DMenu->addSeparator();
	plot3DMenu->addAction(actionPlotHistogram);
	menuBar()->addMenu(plot3DMenu);

	matrixMenu = new QMenu(this);
	matrixMenu->setObjectName("matrixMenu");
	connect(matrixMenu, SIGNAL(aboutToShow()), this, SLOT(matrixMenuAboutToShow()));
	menuBar()->addMenu(matrixMenu);

    plot2DMenu = new QMenu(this);
	plot2DMenu->setObjectName("plot2DMenu");
    connect(plot2DMenu, SIGNAL(aboutToShow()), this, SLOT(plotMenuAboutToShow()));
    menuBar()->addMenu(plot2DMenu);

    plotDataMenu = new QMenu(this);
	plotDataMenu->setObjectName("plotDataMenu");
    connect(plotDataMenu, SIGNAL(aboutToShow()), this, SLOT(plotDataMenuAboutToShow()));
    menuBar()->addMenu(plotDataMenu);

	normMenu = new QMenu(this);
	normMenu->setObjectName("normMenu");

	fillMenu = new QMenu();
	fillMenu->setObjectName("fillMenu");

	analysisMenu = new QMenu(this);
	analysisMenu->setObjectName("analysisMenu");
    connect(analysisMenu, SIGNAL(aboutToShow()), this, SLOT(analysisMenuAboutToShow()));
    menuBar()->addMenu(analysisMenu);

	tableMenu = new QMenu(this);
	tableMenu->setObjectName("tableMenu");
    connect(tableMenu, SIGNAL(aboutToShow()), this, SLOT(tableMenuAboutToShow()));
    menuBar()->addMenu(tableMenu);

	smoothMenu = new QMenu(this);
	smoothMenu->setObjectName("smoothMenu");

	filterMenu = new QMenu(this);
	filterMenu->setObjectName("filterMenu");

	decayMenu = new QMenu(this);
	decayMenu->setObjectName("decayMenu");

	multiPeakMenu = new QMenu(this);
	multiPeakMenu->setObjectName("multiPeakMenu");

	format = new QMenu(this);
	format->setObjectName("formatMenu");
	menuBar()->addMenu(format);

	windowsMenu = new QMenu(this);
	windowsMenu->setObjectName("windowsMenu");
	connect(windowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));
	menuBar()->addMenu(windowsMenu);

	foldersMenu = new QMenu(this);

#ifdef QTISAS
    qtisasMenu = new QMenu(this);
    qtisasMenu->setObjectName("qtisasMenu");
    menuBar()->addMenu(qtisasMenu);
    qtisasMenu->setTitle(tr("SAS-Tools"));
    qtisasMenu->addAction(actionShowDan);
    qtisasMenu->addAction(actionShowCompile);
    qtisasMenu->addAction(actionShowFittable);
    qtisasMenu->addAction(actionShowJnse);
    qtisasMenu->addAction(actionShowSvd);
    qtisasMenu->addAction(actionShowAscii1d);
#endif

    const QString qs = "https://www.qtisas.com";
    const QString gp = "https://iffgit.fz-juelich.de/qtisas";

    const QString ql = ":qtisas_logo.png";
    const QString gl = ":gitlab_logo.png";
    const QString fl = ":fileopen.png";
    const QString nl = ":network.png";

	help = new QMenu(this);
	help->setObjectName("helpMenu");
	menuBar()->addMenu(help);

    help->addAction(openWebPageAction("Homepage - www.qtisas.com", qs + "/", ql));
    help->addAction(openWebPageAction("News - www.qtisas.com/news", qs + "/news", ql));
    help->addAction(openWebPageAction("Download - www.qtisas.com/downloads", qs + "/downloads", ql));

    help->addSeparator();

    help->addAction(openWebPageAction("Source @ IFF-GitLab", gp + "/qtisas/", gl));

    auto *gitgetMenu = new QMenu(this);
    gitgetMenu->setTitle(tr("Git-Get - Repositories"));
    gitgetMenu->addAction(openWebPageAction("Fitting Functions", gp + "/fitfunctions", gl));
    gitgetMenu->addAction(openWebPageAction("Python Actions", gp + "/python-actions", gl));
    help->addMenu(gitgetMenu);

    auto *gitgetFolderMenu = new QMenu(this);
    gitgetFolderMenu->setTitle(tr("Git-Get - Local Folders"));
    gitgetFolderMenu->addAction(
        openWebPageAction("Fitting Functions", QUrl::fromLocalFile(sasPath + "FitFunctions/").toString(), fl));
    gitgetFolderMenu->addAction(
        openWebPageAction("Python Actions", QUrl::fromLocalFile(sasPath + "python-actions/").toString(), fl));
    help->addMenu(gitgetFolderMenu);

    help->addSeparator();

    help->addAction(openWebPageAction("QtiSAS[QtiPlot] - Online Handbook", qs + "/manual", nl));

    auto *sasHelpMenu = new QMenu(this);
    sasHelpMenu->setTitle(tr("SAS-Tools-Help"));
    sasHelpMenu->addAction(openWebPageAction("DAN.SANS: Online Info", qs + "/dan-sans", nl));
    sasHelpMenu->addAction(openWebPageAction("Fit.Compile: Online Info", qs + "/compile", nl));
    sasHelpMenu->addAction(openWebPageAction("Fit.Curve(s): Online Info", qs + "/fittable", nl));
    sasHelpMenu->addAction(openWebPageAction("JNSE: Online Info", qs + "/jnse", nl));
    sasHelpMenu->addAction(openWebPageAction("SANS.SVD: Online Info", qs + "/svd", nl));
    sasHelpMenu->addAction(openWebPageAction("ASCII.SANS.1D: Online Info", qs + "/ascii1d", nl));
    help->addMenu(sasHelpMenu);

#ifdef SCRIPTING_PYTHON
    auto *pythonApiMenu = new QMenu(tr("Python API"), this);
    {
        QStringList docDirs = {
#ifdef Q_OS_MACOS
            QCoreApplication::applicationDirPath() + "/../Resources/python/doc"
#elif defined(Q_OS_WIN)
            QCoreApplication::applicationDirPath() + "/python/doc"
#else
            QCoreApplication::applicationDirPath() + "/../share/qtisas/python/doc",
            QCoreApplication::applicationDirPath() + "/../qtisas/python/doc",
            QCoreApplication::applicationDirPath() + "/python/doc",
#endif
        };
        QString docDir;
        for (const QString &d : docDirs)
            if (QDir(d).exists())
            {
                docDir = d;
                break;
            }
        if (!docDir.isEmpty())
        {
            auto showApiDocWindow = [this](const QString &title, QString md) {
                // grey background extends above and below the python code text.
                {
                    QStringList lines = md.split('\n');
                    QStringList out;
                    bool inPython = false;
                    for (const QString &ln : lines)
                    {
                        if (!inPython && ln.startsWith("```python"))
                        {
                            out << ln << "";
                            inPython = true;
                        }
                        else if (inPython && ln.trimmed() == "```")
                        {
                            out << "" << ln;
                            inPython = false;
                        }
                        else
                        {
                            out << ln;
                        }
                    }
                    md = out.join('\n');
                }

                auto *w = new QWidget(this, Qt::Tool);
                w->setWindowTitle(title + " - Python API");
                w->setAttribute(Qt::WA_DeleteOnClose);
                auto *browser = new QTextBrowser(w);
                browser->document()->setDocumentMargin(16);
                browser->setMarkdown(md);
                browser->setOpenExternalLinks(true);
                new PythonSyntaxHighlighter(browser->document(), true);
                auto *layout = new QVBoxLayout(w);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->addWidget(browser);
                w->resize(900, 650);
                w->show();
            };

            QStringList files = QDir(docDir).entryList({"*-python-api.md"}, QDir::Files, QDir::Name);
            for (const QString &fname : files)
            {
                QString title = fname;
                title.remove("-python-api.md");
                title[0] = title[0].toUpper();
                auto *act = new QAction(title, this);
                QString path = docDir + "/" + fname;
                connect(act, &QAction::triggered, this, [path, title, showApiDocWindow] {
                    QFile f(path);
                    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                        return;
                    showApiDocWindow(title, QString::fromUtf8(f.readAll()));
                });
                pythonApiMenu->addAction(act);
            }

            pythonApiMenu->addSeparator();
            auto *userRulesAct = new QAction(tr("User Rules"), this);
            connect(userRulesAct, &QAction::triggered, this, [showApiDocWindow, this] {
                const QStringList rules = AiAssistant::userRules();
                QString md = QString("# User Rules (%1/%2)\n\n").arg(rules.size()).arg(aiSettings.maxUserRules) +
                             "Manually curated with `addRule()` / `removeRule()` / `removeAllRules()`, "
                             "sent to the AI alongside the documentation on every request. See the "
                             "**Ai** page for the full API.\n\n---\n\n";
                if (rules.isEmpty())
                    md += "*No rules defined yet.*";
                else
                    for (const QString &r : rules)
                        md += "- " + r + "\n";
                showApiDocWindow("User Rules", md);
            });
            pythonApiMenu->addAction(userRulesAct);
        }
    }
    help->addMenu(pythonApiMenu);
#endif

    help->addSeparator();
    auto *checkForUpdateAction = new QAction("Check for a new version ...", this);
    connect(checkForUpdateAction, &QAction::triggered, this, [this] { newVersionCheck(); });
    help->addAction(checkForUpdateAction);
    help->addSeparator();

	help->addAction(actionAbout);
    help->addAction(actionAboutQt);
    
	QList<QMenu *> menus;
	menus << windowsMenu << view << graphMenu << fileMenu << format << edit;
	menus << help << plot2DMenu << analysisMenu;
	menus << matrixMenu << plot3DMenu << plotDataMenu << scriptingMenu;
	menus << tableMenu << newMenu << exportPlotMenu << importMenu;

#ifdef QTISAS
    menus << qtisasMenu;
#endif

	foreach (QMenu *m, menus)
    	connect(m, SIGNAL(triggered(QAction *)), this, SLOT(performCustomAction(QAction *)));

	disableActions();
}

void ApplicationWindow::tableMenuAboutToShow()
{
    tableMenu->clear();
	fillMenu->clear();

    QMenu *setAsMenu = tableMenu->addMenu(tr("Set Columns &As"));
	setAsMenu->addAction(actionSetXCol);
	setAsMenu->addAction(actionSetYCol);
	setAsMenu->addAction(actionSetZCol);
	setAsMenu->addSeparator();
	setAsMenu->addAction(actionSetLabelCol);
	setAsMenu->addAction(actionDisregardCol);
	setAsMenu->addSeparator();
	setAsMenu->addAction(actionSetXErrCol);
	setAsMenu->addAction(actionSetYErrCol);
	setAsMenu->addSeparator();
	setAsMenu->addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
	setAsMenu->addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));

	tableMenu->addAction(actionShowColumnOptionsDialog);
	tableMenu->addSeparator();

	tableMenu->addAction(actionShowColumnValuesDialog);
	tableMenu->addAction(actionTableRecalculate);

	fillMenu->addAction(actionSetAscValues);
	fillMenu->addAction(actionSetRandomValues);
	fillMenu->addAction(actionSetRandomNormalValues);
	tableMenu->addMenu (fillMenu);

	tableMenu->addAction(actionClearTable);
	tableMenu->addSeparator();
	tableMenu->addAction(actionAddColToTable);
	tableMenu->addAction(actionShowColsDialog);
	tableMenu->addSeparator();
	tableMenu->addAction(actionHideSelectedColumns);
	tableMenu->addAction(actionShowAllColumns);
	tableMenu->addAction(actionAdjustColumnWidth);
	tableMenu->addSeparator();
	tableMenu->addAction(actionMoveColFirst);
	tableMenu->addAction(actionMoveColLeft);
	tableMenu->addAction(actionMoveColRight);
	tableMenu->addAction(actionMoveColLast);
	tableMenu->addAction(actionSwapColumns);
	tableMenu->addSeparator();
	tableMenu->addAction(actionShowRowsDialog);
	tableMenu->addAction(actionDeleteRows);

	QMenu *moveRowMenu = tableMenu->addMenu(tr("Move Row"));
	moveRowMenu->addAction(actionMoveRowUp);
	moveRowMenu->addAction(actionMoveRowDown);

	tableMenu->addSeparator();
	tableMenu->addAction(actionGoToRow);
	tableMenu->addAction(actionGoToColumn);
	tableMenu->addAction(actionExtractTableData);
    tableMenu->addAction(actionTransposeTable);
	tableMenu->addSeparator();

	QMenu *convertToMatrixMenu = tableMenu->addMenu(tr("Convert to &Matrix"));
	convertToMatrixMenu->addAction(actionConvertTableDirect);
	convertToMatrixMenu->addAction(actionConvertTableBinning);
	convertToMatrixMenu->addAction(actionConvertTableRegularXYZ);
	convertToMatrixMenu->addAction(actionConvertTableRandomXYZ);

    reloadCustomActions();
}

void ApplicationWindow::plotDataMenuAboutToShow()
{
    plotDataMenu->clear();
	plotDataMenu->addAction(btnPointer);
	plotDataMenu->addSeparator();
	plotDataMenu->addAction(actionMagnify);
	plotDataMenu->addAction(actionMagnifyHor);
	plotDataMenu->addAction(actionMagnifyVert);
	plotDataMenu->addAction(btnZoomIn);
	plotDataMenu->addAction(btnZoomOut);
	plotDataMenu->addAction(actionUnzoom);
	plotDataMenu->addSeparator();
	plotDataMenu->addAction(btnCursor);
	plotDataMenu->addAction(btnSelect);
	plotDataMenu->addAction(btnPicker);
	plotDataMenu->addSeparator();
	plotDataMenu->addAction(actionDrawPoints);
	plotDataMenu->addAction(btnMovePoints);
	plotDataMenu->addAction(btnRemovePoints);
	plotDataMenu->addAction(actionDragCurve);

    reloadCustomActions();
}

void ApplicationWindow::plotMenuAboutToShow()
{
	plot2DMenu->clear();

	plot2DMenu->addAction(actionPlotL);
	plot2DMenu->addAction(actionPlotP);
	plot2DMenu->addAction(actionPlotLP);

    QMenu *specialPlotMenu = plot2DMenu->addMenu (tr("Special Line/Symb&ol"));
	specialPlotMenu->addAction(actionPlotVerticalDropLines);
	specialPlotMenu->addAction(actionPlotSpline);
	specialPlotMenu->addAction(actionPlotVertSteps);
	specialPlotMenu->addAction(actionPlotHorSteps);
	specialPlotMenu->addSeparator();
	specialPlotMenu->addAction(actionPlotDoubleYAxis);
	specialPlotMenu->addAction(actionWaterfallPlot);
	specialPlotMenu->addAction(actionAddZoomPlot);

	plot2DMenu->addSeparator();
	plot2DMenu->addAction(actionPlotVerticalBars);
	plot2DMenu->addAction(actionPlotHorizontalBars);
	QMenu *specialBarMenu = plot2DMenu->addMenu (tr("Spec&ial Bar/Column"));
	specialBarMenu->addAction(actionStackBars);
	specialBarMenu->addAction(actionStackColumns);

	plot2DMenu->addAction(actionPlotArea);
	plot2DMenu->addAction(actionPlotPie);
	plot2DMenu->addAction(actionPlotVectXYXY);
	plot2DMenu->addAction(actionPlotVectXYAM);
	plot2DMenu->addSeparator();

	QMenu *statMenu = plot2DMenu->addMenu (tr("Statistical &Graphs"));
	statMenu->addAction(actionBoxPlot);
	statMenu->addAction(actionPlotHistogram);
	statMenu->addAction(actionPlotStackedHistograms);
	statMenu->addSeparator();
	statMenu->addAction(actionStemPlot);

    QMenu *panelsMenu = plot2DMenu->addMenu (tr("Pa&nel"));
	panelsMenu->addAction(actionPlot2VerticalLayers);
	panelsMenu->addAction(actionPlot2HorizontalLayers);
	panelsMenu->addAction(actionPlot4Layers);
	panelsMenu->addAction(actionPlotStackedLayers);
	panelsMenu->addAction(actionCustomLayout);

	QMenu *gridMenu = plot2DMenu->addMenu (tr("Shared A&xes Panel"));
	gridMenu->addAction(actionVertSharedAxisLayers);
	gridMenu->addAction(actionHorSharedAxisLayers);
	gridMenu->addAction(actionSharedAxesLayers);
	gridMenu->addAction(actionStackSharedAxisLayers);
	gridMenu->addAction(actionCustomSharedAxisLayers);

	QMenu *plot3D = plot2DMenu->addMenu (tr("3&D Plot"));
	plot3D->addAction(actionPlot3DRibbon);
	plot3D->addAction(actionPlot3DBars);
	plot3D->addAction(actionPlot3DScatter);
	plot3D->addAction(actionPlot3DTrajectory);

    reloadCustomActions();
}

void ApplicationWindow::customMenu(QMdiSubWindow* w)
{
    analysisMenu->menuAction()->setVisible(false);
    tableMenu->menuAction()->setVisible(false);
    plotDataMenu->menuAction()->setVisible(false);
    graphMenu->menuAction()->setVisible(false);
    matrixMenu->menuAction()->setVisible(false);
	format->menuAction()->setVisible(false);
    plot2DMenu->menuAction()->setVisible(false);
    plot3DMenu->menuAction()->setVisible(false);
#ifndef SCRIPTING_PYTHON
	scriptingMenu->menuAction()->setVisible(false);
#endif

	fileMenuAboutToShow();
	windowsMenuAboutToShow();
    scriptingMenuAboutToShow();

	// these use the same keyboard shortcut (Ctrl+Return) and should not be enabled at the same time
	actionTableRecalculate->setEnabled(false);
	// these use the same keyboard shortcut (Alt+C) and should not be enabled at the same time
	actionShowCurvesDialog->setEnabled(false);
	actionAddColToTable->setEnabled(false);
	// these use the same keyboard shortcut (Alt+Q) and should not be enabled at the same time
	actionAddFormula->setEnabled(false);
	actionShowColumnValuesDialog->setEnabled(false);
	actionSetMatrixValues->setEnabled(false);
	// these use the same keyboard shortcut (Ctrl+Alt+F) and should not be enabled at the same time
	actionAddFunctionCurve->setEnabled(false);
	actionFind->setEnabled(false);
	// these use the same keyboard shortcut (Ctrl+Alt+G) and should not be enabled at the same time
    actionExportLayer->setEnabled(false);
    actionExportGraph->setEnabled(false);
	actionGoToRow->setEnabled(false);

	// clear undo stack view (in case window is not a matrix)
	d_undo_view->setStack(0);
	actionUndo->setEnabled(false);
	actionRedo->setEnabled(false);

	actionCopyWindow->setEnabled(w);
	actionPrint->setEnabled(w);
	actionPrintPreview->setEnabled(w);
	actionExportPDF->setEnabled(w);

	if(w){
		analysisMenuAboutToShow();

		actionPrintAllPlots->setEnabled(projectHas2DPlots());
		actionPrint->setEnabled(true);
		actionCutSelection->setEnabled(true);
		actionCopySelection->setEnabled(true);
		actionPasteSelection->setEnabled(true);
		actionClearSelection->setEnabled(true);
		actionSaveTemplate->setEnabled(true);
		actionSaveWindow->setEnabled(true);
		QStringList tables = tableNames() + matrixNames();
		if (!tables.isEmpty())
			actionShowExportASCIIDialog->setEnabled(true);
		else
			actionShowExportASCIIDialog->setEnabled(false);

		if (QString(w->metaObject()->className()) == "MultiLayer") {
			actionAddFunctionCurve->setEnabled(true);
			actionShowCurvesDialog->setEnabled(true);
			actionAddFormula->setEnabled(true);
            actionExportLayer->setEnabled(true);
            actionExportGraph->setEnabled(true);

			graphMenu->menuAction()->setVisible(true);
			plotDataMenu->menuAction()->setVisible(true);
			analysisMenu->menuAction()->setVisible(true);
			format->menuAction()->setVisible(true);
			format->clear();
			format->addAction(actionShowPlotDialog);
			format->addSeparator();
            format->addAction(actionShowScaleDialog);
            format->addAction(actionShowAxisDialog);
            actionShowAxisDialog->setEnabled(true);
            format->addSeparator();
            format->addAction(actionShowGridDialog);
			format->addAction(actionShowTitleDialog);
		} else if (QString(w->metaObject()->className()) == "Graph3D") {
			disableActions();

			actionPrint->setEnabled(true);
			actionSaveTemplate->setEnabled(true);
			actionSaveWindow->setEnabled(true);
            actionExportLayer->setEnabled(true);
			actionExportGraph->setEnabled(true);

			format->menuAction()->setVisible(true);
			format->clear();
			format->addAction(actionShowPlotDialog);
			format->addAction(actionShowScaleDialog);
			format->addAction(actionShowAxisDialog);
			format->addAction(actionShowTitleDialog);
			if (((Graph3D*)w)->coordStyle() == Qwt3D::NOCOORD)
				actionShowAxisDialog->setEnabled(false);
		} else if (w->inherits("Table")) {
			tableMenuAboutToShow();

			plot2DMenu->menuAction()->setVisible(true);
			analysisMenu->menuAction()->setVisible(true);
			tableMenu->menuAction()->setVisible(true);

			actionTableRecalculate->setEnabled(true);
			actionAddColToTable->setEnabled(true);
			actionShowColumnValuesDialog->setEnabled(true);
			actionGoToRow->setEnabled(true);
		} else if (qobject_cast<Matrix*>(w)){
			matrixMenuAboutToShow();

			actionTableRecalculate->setEnabled(true);
			actionSetMatrixValues->setEnabled(true);
			actionGoToRow->setEnabled(true);

			plot3DMenu->menuAction()->setVisible(true);
			analysisMenu->menuAction()->setVisible(true);
			matrixMenu->menuAction()->setVisible(true);

			d_undo_view->setEmptyLabel(w->objectName() + ": " + tr("Empty Stack"));
			QUndoStack *stack = ((Matrix *)w)->undoStack();
			d_undo_view->setStack(stack);
		} else if (qobject_cast<Note*>(w)){
			#ifndef SCRIPTING_PYTHON
			scriptingMenu->menuAction()->setVisible(true);
			#endif
			actionSaveTemplate->setEnabled(false);
			actionFind->setEnabled(true);
		} else
			disableActions();
	} else
		disableActions();

	reloadCustomActions();
}

void ApplicationWindow::disableActions()
{
	actionSaveTemplate->setEnabled(false);
	actionSaveWindow->setEnabled(false);
	actionPrintAllPlots->setEnabled(false);
	actionPrint->setEnabled(false);

#ifdef SCRIPTING_CONSOLE
    actionCutSelection->setEnabled(this->console && this->console->hasFocus());
#else
    actionCutSelection->setEnabled(false);
#endif
	actionCopySelection->setEnabled(false);
	actionPasteSelection->setEnabled(false);
	actionClearSelection->setEnabled(false);
}

void ApplicationWindow::customColumnActions()
{
	actionAdjustColumnWidth->setEnabled(false);
    actionMoveColFirst->setEnabled(false);
    actionMoveColLeft->setEnabled(false);
    actionMoveColRight->setEnabled(false);
    actionMoveColLast->setEnabled(false);
    actionSetXCol->setEnabled(false);
    actionSetYCol->setEnabled(false);
    actionSetZCol->setEnabled(false);
    actionSetLabelCol->setEnabled(false);
    actionSetYErrCol->setEnabled(false);
    actionDisregardCol->setEnabled(false);
    actionSwapColumns->setEnabled(false);

	Table *t = (Table*)activeWindow(TableWindow);
    if (!t)
		return;

    int selectedCols = t->selectedColsNumber();
    if (selectedCols == 1){
        int col = t->selectedColumn();
        if (col > 0){
            actionMoveColFirst->setEnabled(true);
            actionMoveColLeft->setEnabled(true);
        }

        if (col < t->numCols() - 1){
            actionMoveColRight->setEnabled(true);
            actionMoveColLast->setEnabled(true);
        }
	}

	if (selectedCols >= 1){
        actionSetXCol->setEnabled(true);
        actionSetYCol->setEnabled(true);
        actionSetZCol->setEnabled(true);
        actionSetYErrCol->setEnabled(true);
        actionSetLabelCol->setEnabled(true);
        actionDisregardCol->setEnabled(true);
        actionAdjustColumnWidth->setEnabled(true);
	}

	if (selectedCols == 2)
	    actionSwapColumns->setEnabled(true);
}

void ApplicationWindow::customToolBars(QMdiSubWindow* w)
{
    disableToolbars();
	if (!w)
        return;

	actionTextColor->setVisible(false);

	if (qobject_cast<MultiLayer*>(w)){
		actionTextColor->setVisible(true);
		if (d_plot_tool_bar){
			if(!plotTools->isVisible())
				plotTools->show();
			plotTools->setEnabled (true);
			custom2DPlotTools((MultiLayer *)w);
		}
		if(d_format_tool_bar && !formatToolBar->isVisible()){
			formatToolBar->setEnabled (true);
            formatToolBar->show();
		}
    } else if (w->inherits("Table")){
        if(d_table_tool_bar){
            if(!tableTools->isVisible())
                tableTools->show();
            tableTools->setEnabled (true);
        }
        if (d_column_tool_bar){
            if(!columnTools->isVisible())
                columnTools->show();
            columnTools->setEnabled (true);
            customColumnActions();
        }
	} else if (qobject_cast<Matrix*>(w)){
		 if(d_matrix_tool_bar && !plotMatrixBar->isVisible())
            plotMatrixBar->show();
        plotMatrixBar->setEnabled (true);
	} else if (qobject_cast<Graph3D*>(w)){
		if(d_plot3D_tool_bar && !plot3DTools->isVisible())
			plot3DTools->show();

		plot3DTools->setEnabled(((Graph3D*)w)->plotStyle() != Qwt3D::NOPLOT);
		custom3DActions(w);
	} else if (qobject_cast<Note*>(w)){
		if(d_format_tool_bar && !formatToolBar->isVisible())
            formatToolBar->show();
		if(d_notes_tool_bar && !noteTools->isVisible())
            noteTools->show();

        formatToolBar->setEnabled (true);
        noteTools->setEnabled (true);
        setFormatBarFont(((Note*)w)->currentEditor()->currentFont());
    }
}

void ApplicationWindow::disableToolbars()
{
	plotTools->setEnabled(false);
	tableTools->setEnabled(false);
	columnTools->setEnabled(false);
	plot3DTools->setEnabled(false);
	plotMatrixBar->setEnabled(false);
	noteTools->setEnabled(false);
}

void ApplicationWindow::plot3DRibbon()
{
	MdiSubWindow *w = activeWindow(TableWindow);
    if (!w)
		return;

	Table *table = static_cast<Table*>(w);
	if(table->selectedColumns().count() == 1){
		if (!validFor3DPlot(table))
			return;
		plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Ribbon);
	} else
		QMessageBox::warning(this, tr("QTISAS - Plot error"), tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plot3DWireframe()
{
	plot3DMatrix (0, Qwt3D::WIREFRAME);
}

void ApplicationWindow::plot3DHiddenLine()
{
	plot3DMatrix (0, Qwt3D::HIDDENLINE);
}

void ApplicationWindow::plot3DPolygons()
{
	plot3DMatrix (0, Qwt3D::FILLED);
}

void ApplicationWindow::plot3DWireSurface()
{
	plot3DMatrix (0, Qwt3D::FILLEDMESH);
}

void ApplicationWindow::plot3DBars()
{
	MdiSubWindow *w = activeWindow();
    if (!w)
		return;

	if (w->inherits("Table")){
		Table *table = static_cast<Table *>(w);
		if (!validFor3DPlot(table))
			return;

		if(table->selectedColumns().count() == 1)
			plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Bars);
		else
			QMessageBox::warning(this, tr("QTISAS - Plot error"),tr("You must select exactly one column for plotting!"));
	}
	else if(w->inherits("Matrix"))
		plot3DMatrix(0, Qwt3D::USER);
}

void ApplicationWindow::plot3DScatter()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (w->inherits("Table"))
	{
		Table *table = static_cast<Table *>(w);
		if (!validFor3DPlot(table))
			return;

		if(table->selectedColumns().count() == 1)
			plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Scatter);
		else
			QMessageBox::warning(this, tr("QTISAS - Plot error"),tr("You must select exactly one column for plotting!"));
	}
	else if(w->inherits("Matrix"))
		plot3DMatrix (0, Qwt3D::POINTS);
}

void ApplicationWindow::plot3DTrajectory()
{
	Table *table = (Table *)activeWindow(TableWindow);
    if (!table)
		return;
    if (!validFor3DPlot(table))
        return;

    if(table->selectedColumns().count() == 1)
        plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Trajectory);
    else
        QMessageBox::warning(this, tr("QTISAS - Plot error"), tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plotBox()
{
    generate2DGraph(Graph::Box);
}

void ApplicationWindow::plotVerticalBars()
{
	generate2DGraph(Graph::VerticalBars);
}

void ApplicationWindow::plotHorizontalBars()
{
	generate2DGraph(Graph::HorizontalBars);
}

void ApplicationWindow::plotStackBar()
{
	generate2DGraph(Graph::StackBar);
}

void ApplicationWindow::plotStackColumn()
{
	generate2DGraph(Graph::StackColumn);
}

MultiLayer* ApplicationWindow::plotHistogram()
{
    return generate2DGraph(Graph::Histogram);
}

MultiLayer* ApplicationWindow::plotHistogram(Matrix *m)
{
	if (!m){
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m)
			return 0;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	MultiLayer* g = newGraph();
	if (g)
		g->activeLayer()->addHistogram(m);
	QApplication::restoreOverrideCursor();
	return g;
}

void ApplicationWindow::plotArea()
{
	generate2DGraph(Graph::Area);
}

void ApplicationWindow::plotPie()
{
	Table *table = (Table *)activeWindow(TableWindow);
    if (!table)
		return;

	if(table->selectedColumns().count() != 1){
		QMessageBox::warning(this, tr("QTISAS - Plot error"),
				tr("You must select exactly one column for plotting!"));
		return;
	}

	QStringList s = table->selectedColumns();
	if (s.count()>0){
        MySelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::Pie, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select a column to plot!"));
}

void ApplicationWindow::plotL()
{
	generate2DGraph(Graph::Line);
}

void ApplicationWindow::plotP()
{
	generate2DGraph(Graph::Scatter);
}

void ApplicationWindow::plotLP()
{
	generate2DGraph(Graph::LineSymbols);
}

void ApplicationWindow::plotVerticalDropLines()
{
	generate2DGraph(Graph::VerticalDropLines);
}

void ApplicationWindow::plotSpline()
{
	generate2DGraph(Graph::Spline);
}

void ApplicationWindow::plotVertSteps()
{
	generate2DGraph(Graph::VerticalSteps);
}

void ApplicationWindow::plotHorSteps()
{
	generate2DGraph(Graph::HorizontalSteps);
}

void ApplicationWindow::plotVectXYXY()
{
	Table *table = (Table *)activeWindow(TableWindow);
    if (!table)
		return;
	if (!validFor2DPlot(table, Graph::VectXYXY))
		return;

	QStringList s = table->selectedColumns();
	if (s.count() == 4) {
        MySelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::VectXYXY, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select four columns for this operation!"));
}

void ApplicationWindow::plotVectXYAM()
{
    Table *table = (Table *)activeWindow(TableWindow);
    if (!table)
		return;
	if (!validFor2DPlot(table, Graph::VectXYAM))
		return;

	QStringList s = table->selectedColumns();
	if (s.count() == 4){
        MySelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::VectXYAM, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select four columns for this operation!"));
}

void ApplicationWindow::renameListViewItem(const QString& oldName,const QString& newName)
{
    if (!lv->findItems(oldName, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty()) {
        QTreeWidgetItem *it = lv->findItems(oldName, Qt::MatchExactly | Qt::MatchCaseSensitive).first();
        it->setText(0, newName);
    }
}

void ApplicationWindow::setListViewLabel(const QString& caption, const QString& label)
{
    if (!lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty()) {
        QTreeWidgetItem *it = lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).first();
		QString s = label;
		it->setText(4, s.replace("\n", " "));
	}
}

void ApplicationWindow::setListViewDate(const QString& caption,const QString& date)
{
    if (!lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty()) {
        QTreeWidgetItem *it = lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).first();
        it->setText(3, date);
    }
}

void ApplicationWindow::setListView(const QString& caption,const QString& view)
{
    if (!lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty()) {
        QTreeWidgetItem *it = lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).first();
        it->setText(2, view);
    }
}

QString ApplicationWindow::listViewDate(const QString& caption)
{
    if (!lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty()) {
        QTreeWidgetItem *it = lv->findItems(caption, Qt::MatchExactly | Qt::MatchCaseSensitive).first();
        return it->text(3);
    }
	else return {};
}

void ApplicationWindow::updateTableNames(const QString& oldName, const QString& newName)
{
	QList<MdiSubWindow *> windows = windowsList();
	foreach (MdiSubWindow *w, windows) {
		if (QString(w->metaObject()->className()) == "MultiLayer") {
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers)
				g->updateCurveNames(oldName, newName);
		} else if (QString(w->metaObject()->className()) == "Graph3D") {
			QString name = ((Graph3D*)w)->formula();
			if (name.contains(oldName, Qt::CaseSensitive)) {
				name.replace(oldName,newName);
				((Graph3D*)w)->setPlotAssociation(name);
			}
		}
	}
}

void ApplicationWindow::updateColNames(const QString& oldName, const QString& newName)
{
	QList<MdiSubWindow *> windows = windowsList();
	foreach (MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "MultiLayer"){
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers)
				g->updateCurveNames(oldName, newName, false);
		} else if (QString(w->metaObject()->className()) == "Graph3D"){
			QString name = ((Graph3D*)w)->formula();
			if (name.contains(oldName)){
				name.replace(oldName,newName);
				((Graph3D*)w)->setPlotAssociation(name);
			}
		}
	}
}

void ApplicationWindow::changeMatrixName(const QString& oldName, const QString& newName)
{
	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "Graph3D")
		{
			QString s = ((Graph3D*)w)->formula();
			if (s.contains(oldName))
			{
				s.replace(oldName, newName);
				((Graph3D*)w)->setPlotAssociation(s);
			}
		}
		else if (QString(w->metaObject()->className()) == "MultiLayer")
		{
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers){
				for (int i=0; i<g->curveCount(); i++){
					QwtPlotItem *sp = (QwtPlotItem *)g->plotItem(i);
					if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->title().text() == oldName)
						sp->setTitle(newName);
				}
			}
		}
	}
}

void ApplicationWindow::remove3DMatrixPlots(Matrix *m)
{
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "Graph3D" && ((Graph3D*)w)->matrix() == m)
			((Graph3D*)w)->clearData();
		else if (QString(w->metaObject()->className()) == "MultiLayer"){
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers){
				bool update = false;
				QList<QwtPlotItem *> curvesList = g->curvesList();
				foreach (QwtPlotItem *it, curvesList){
					if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && ((Spectrogram *)it)->matrix() == m){
						g->removeCurve(it);
						update = true;
					} else if (((PlotCurve *)it)->type() == Graph::Histogram && ((QwtHistogram *)it)->matrix() == m){
						g->removeCurve(it);
						update = true;
					}
				}
				if (update)
					g->updatePlot();
			}
		}
	}
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateMatrixPlots(Matrix *m)
{
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "Graph3D" && ((Graph3D*)w)->matrix() == m)
			((Graph3D*)w)->updateMatrixData(m);
		else if (QString(w->metaObject()->className()) == "MultiLayer"){
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers){
				bool update = false;
				QList<QwtPlotItem *> curvesList = g->curvesList();
				foreach (QwtPlotItem *it, curvesList){
					if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
						Spectrogram *sp = (Spectrogram *)it;
						if (sp->matrix() == m){
							sp->updateData();
							update = true;
						}
					} else if (((PlotCurve *)it)->type() == Graph::Histogram){
						QwtHistogram *h = (QwtHistogram *)it;
						if (h->matrix() == m){
							h->loadData();
							update = true;
						}
					}
				}
				if (update)
					g->updatePlot();
			}
		}
	}
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::add3DData()
{
	if (!hasTable()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no tables available in this project.</h4>"
					"<p><h4>Please create a table and try again!</h4>"));
		return;
	}

	QStringList zColumns = columnsList(Table::Z);
	if ((int)zColumns.count() <= 0){
		QMessageBox::critical(this,tr("QTISAS - Warning"),
				tr("There are no available columns with plot designation set to Z!"));
		return;
	}

	bool ok;
	QString column = QInputDialog::getItem(this, tr("QTISAS - Choose data set"),
									tr("Column") + ": ", zColumns, 0, false, &ok);
	if (ok && !column.isEmpty())
		insertNew3DData(column);
}

void ApplicationWindow::change3DData()
{
	bool ok;
	QString column = QInputDialog::getItem(this, tr("QTISAS - Choose data set"),
									tr("Column") + ": ", columnsList(Table::Z), 0, false, &ok);
	if (ok && !column.isEmpty())
		change3DData(column);
}

void ApplicationWindow::change3DMatrix()
{
	QStringList matrices = matrixNames();
	int currentIndex = 0;
	Graph3D* g = (Graph3D*)activeWindow(Plot3DWindow);
	if (g && g->matrix())
		currentIndex = matrices.indexOf(g->matrix()->objectName());

	bool ok;
	QString matrixName = QInputDialog::getItem(this, tr("QTISAS - Choose matrix to plot"),
							tr("Matrix") + ": ", matrices, currentIndex, false, &ok);
	if (ok && !matrixName.isEmpty())
		change3DMatrix(matrixName);
}

void ApplicationWindow::change3DMatrix(const QString& matrix_name)
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
    if (!g)
		return;

	Matrix *m = matrix(matrix_name);
	if (!m)
        return;

	if (d_3D_autoscale)
        g->addMatrixData(m);
    else
		g->addMatrixData(m, g->xStart(), g->xStop(), g->yStart(), g->yStop(), g->zStart(), g->zStop());

	emit modified();
}

void ApplicationWindow::add3DMatrixPlot()
{
	QStringList matrices = matrixNames();
	if ((int)matrices.count() <= 0){
		QMessageBox::warning(this, tr("QTISAS - Warning"),
				tr("<h4>There are no matrices available in this project.</h4>"
					"<p><h4>Please create a matrix and try again!</h4>"));
		return;
	}

	bool ok;
	QString matrixName = QInputDialog::getItem(this, tr("QTISAS - Choose matrix to plot"),
							tr("Matrix") + ": ", matrices, 0, false, &ok);
	if (ok && !matrixName.isEmpty())
		insert3DMatrixPlot(matrixName);
}

void ApplicationWindow::insert3DMatrixPlot(const QString& matrix_name)
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
    if (!g)
		return;

	g->addMatrixData(matrix(matrix_name));
	emit modified();
}

void ApplicationWindow::insertNew3DData(const QString& colName)
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
    if (!g)
		return;

	g->insertNewData(table(colName),colName);
	emit modified();
}

void ApplicationWindow::change3DData(const QString& colName)
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
    if (!g)
		return;

	g->changeDataColumn(table(colName), colName, g->tablePlotType());
	emit modified();
}

void ApplicationWindow::editSurfacePlot()
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
    if (!g)
		return;

	SurfaceDialog* sd = new SurfaceDialog(this);
	sd->setAttribute(Qt::WA_DeleteOnClose);

	if (g->hasData() && g->userFunction())
		sd->setFunction(g);
	else if (g->hasData() && g->parametricSurface())
		sd->setParametricSurface(g);
	else
		sd->setGraph(g);
	sd->exec();
}

void ApplicationWindow::newSurfacePlot()
{
	SurfaceDialog* sd = new SurfaceDialog(this);
	sd->setAttribute(Qt::WA_DeleteOnClose);
	sd->exec();
}

Graph3D* ApplicationWindow::plotSurface(const QString& formula, double xl, double xr,
		double yl, double yr, double zl, double zr, int columns, int rows)
{
	Graph3D *plot = newPlot3D();
	if(!plot)
		return 0;

	plot->addFunction(formula, xl, xr, yl, yr, zl, zr, columns, rows);
    plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
	plot->update();

	emit modified();
	return plot;
}

// Same folder-tree search plot3D(name)'s own SIP %MethodCode performs (qtimod.sip), exposed here
// as a reusable C++ helper for the newPlot3D(name, ...) upsert overloads below.
Graph3D *ApplicationWindow::findPlot3D(const QString &name)
{
    if (name.isEmpty())
        return nullptr;
    Graph3D *plot = current_folder->plot3D(name, false);
    if (!plot)
        plot = projectFolder()->plot3D(name, true);
    return plot;
}

QString ApplicationWindow::plot3DNameCollisionError(const QString &name)
{
    if (name.isEmpty() || findPlot3D(name) || !alreadyUsedName(name))
        return {};
    return QString("newPlot3D: name '%1' already exists but is not a Graph3D window — cannot "
                   "create or update it as one.")
        .arg(name);
}

Graph3D *ApplicationWindow::newPlot3D(const QString &name, const QString &formula, double xl, double xr, double yl,
                                      double yr, double zl, double zr, int columns, int rows)
{
    Graph3D *plot = findPlot3D(name);
    if (plot)
    {
        plot->addFunction(formula, xl, xr, yl, yr, zl, zr, columns, rows);
        plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
        plot->update();
        emit modified();
        return plot;
    }
    plot = plotSurface(formula, xl, xr, yl, yr, zl, zr, columns, rows);
    if (plot && !name.isEmpty())
        setWindowName(plot, name);
    return plot;
}

Graph3D* ApplicationWindow::plotParametricSurface(const QString& xFormula, const QString& yFormula,
		const QString& zFormula, double ul, double ur, double vl, double vr,
		int columns, int rows, bool uPeriodic, bool vPeriodic)
{
	Graph3D *plot = newPlot3D();
	if(!plot)
		return 0;
	plot->addParametricSurface(xFormula, yFormula, zFormula, ul, ur, vl, vr,
								columns, rows, uPeriodic, vPeriodic);
    plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
	plot->update();

	emit modified();
	return plot;
}

Graph3D *ApplicationWindow::newPlot3D(const QString &name, const QString &xFormula, const QString &yFormula,
                                      const QString &zFormula, double ul, double ur, double vl, double vr, int columns,
                                      int rows, bool uPeriodic, bool vPeriodic)
{
    Graph3D *plot = findPlot3D(name);
    if (plot)
    {
        plot->addParametricSurface(xFormula, yFormula, zFormula, ul, ur, vl, vr, columns, rows, uPeriodic, vPeriodic);
        plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
        plot->update();
        emit modified();
        return plot;
    }
    plot = plotParametricSurface(xFormula, yFormula, zFormula, ul, ur, vl, vr, columns, rows, uPeriodic, vPeriodic);
    if (plot && !name.isEmpty())
        setWindowName(plot, name);
    return plot;
}

void ApplicationWindow::updateSurfaceFuncList(const QString& s)
{
	surfaceFunc.removeAll(s);
	surfaceFunc.push_front(s);
	while ((int)surfaceFunc.size() > 10)
		surfaceFunc.pop_back();
}

Graph3D* ApplicationWindow::newPlot3D(const QString& title)
{
	QString label = title;
	if (label.isEmpty() || alreadyUsedName(label))
		label = generateUniqueName(tr("Graph"));

    auto plot = new Graph3D("", this, nullptr);
	plot->setWindowTitle(label);
	plot->setName(label);

	initPlot3D(plot);

	emit modified();
	return plot;
}

Graph3D* ApplicationWindow::plotXYZ(Table* table, const QString& zColName, int type)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    int zCol = table->colIndex(zColName);
    int yCol = table->colY(zCol);
    int xCol = table->colX(zCol);
	if (zCol < 0)
		return 0;

	Graph3D *plot = newPlot3D();
	if(!plot)
		return 0;

    if (type == Graph3D::Ribbon)
        plot->addRibbon(table, table->colName(xCol), zColName);
    else
        plot->addData(table, xCol, yCol, zCol, type);

    plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
	plot->update();
    plot->setMaximized(table);

	emit modified();
	QApplication::restoreOverrideCursor();
	return plot;
}

Graph3D *ApplicationWindow::newPlot3D(const QString &name, Table *table, const QString &zColName, int type)
{
    if (!table)
        return nullptr;
    Graph3D *plot = findPlot3D(name);
    if (plot)
    {
        int zCol = table->colIndex(zColName);
        if (zCol < 0)
            return nullptr;
        int yCol = table->colY(zCol);
        int xCol = table->colX(zCol);
        if (type == Graph3D::Ribbon)
            plot->addRibbon(table, table->colName(xCol), zColName);
        else
            plot->addData(table, xCol, yCol, zCol, type);
        plot->setDataColorMap(new LinearColorMap(d_3D_color_map.get()));
        plot->update();
        emit modified();
        return plot;
    }
    plot = plotXYZ(table, zColName, type);
    if (plot && !name.isEmpty())
        setWindowName(plot, name);
    return plot;
}

void ApplicationWindow::initPlot3D(Graph3D *plot)
{
	if (d_mdi_windows_area)
		d_workspace->addSubWindow(plot);
	else
		plot->setParent(0);

	connectSurfacePlot(plot);

	plot->setWindowIcon(QIcon(":/trajectory.png"));
	plot->show();
    // delays this command because something is not ready to call it right now; keeps a view a script set meanwhile
    QTimer::singleShot(30, plot, [plot] { plot->findBestLayoutKeepingExplicitView(); });
    addListViewItem(plot);

    if (!plot3DTools->isVisible())
        plot3DTools->show();

    if (!plot3DTools->isEnabled())
		plot3DTools->setEnabled(true);

	windowActivated(plot);
}

void ApplicationWindow::exportMatrix(const QString& exportFilter)
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	ImageExportDialog *ied = new ImageExportDialog(m, this, d_extended_export_dialog);
	ied->setDirectory(imagesDirPath);
	ied->selectFile(m->objectName());
	if (exportFilter.isEmpty())
    	ied->selectNameFilter(d_image_export_filter);
	else
		ied->selectNameFilter(exportFilter);

	if ( ied->exec() != QDialog::Accepted )
		return;
	imagesDirPath = ied->directory().path();

    QString selected_filter = ImageExportDialog::filterSuffix(ied->selectedNameFilter());
	QString file_name = ied->selectedFiles()[0];
	if(!file_name.endsWith(selected_filter, Qt::CaseInsensitive))
		file_name.append(selected_filter);

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
		m->exportVector(file_name, ied->vectorResolution(), ied->color());
	else if (selected_filter.contains(".svg"))
		m->exportSVG(file_name);
	else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i = 0; i < list.count(); i++){
			if (selected_filter.contains("." + (list[i]).toLower()))
				m->exportRasterImage(file_name, ied->quality(), ied->bitmapResolution(), ied->compression());
		}
	}
}

Matrix* ApplicationWindow::importImage(const QString& fileName, bool newWindow)
{
	QString fn = fileName;
	if (fn.isEmpty()){
        fn = getFileName(this, tr("QTISAS - Import image from file"), imagesDirPath, imageFilter(), nullptr, false);
		if ( !fn.isEmpty() ){
			QFileInfo fi(fn);
			imagesDirPath = fi.absolutePath();
		}
	}

    QImage image(fn);
    if (image.isNull())
        return 0;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    MdiSubWindow *w = activeWindow(MatrixWindow);
    Matrix* m = nullptr;
    if (w && !newWindow){
        m = (Matrix *)w;
        m->importImage(fn);
    } else {
        m = new Matrix(scriptEnv, image, "", this);
        initMatrix(m, generateUniqueName(tr("Matrix")));
        m->show();
        m->setWindowLabel(fn);
        m->setCaptionPolicy(MdiSubWindow::Both);
    }

    QApplication::restoreOverrideCursor();
    return m;
}

QString ApplicationWindow::imageFilter()
{
	QList<QByteArray> list = QImageReader::supportedImageFormats();
	QString filter = tr("Images") + " (", aux1, aux2;
	for (int i = 0; i < (int)list.count(); i++){
		aux1 = " *." + list[i] + " ";
		aux2 += " *." + list[i] + ";;";
		filter += aux1;
	}
	filter += ");;" + aux2;
	return filter;
}

void ApplicationWindow::loadImage()
{
    QString fn = getFileName(this, tr("QTISAS - Load image from file"), imagesDirPath, imageFilter(), nullptr, false);
	if ( !fn.isEmpty() ){
		loadImage(fn);
		QFileInfo fi(fn);
		imagesDirPath = fi.absolutePath();
	}
}

void ApplicationWindow::loadImage(const QString& fn)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer *plot = multilayerPlot(generateUniqueName(tr("Graph")));
	plot->setWindowLabel(fn);
	plot->setCaptionPolicy(MdiSubWindow::Both);

	Graph *g = plot->activeLayer();
	g->setTitle("");
	for (int i=0; i<4; i++)
		g->enableAxis(i, false);
	g->addImage(fn);
	QApplication::restoreOverrideCursor();
}

MultiLayer *ApplicationWindow::multilayerPlot(const QString &caption, int layers, int rows, int cols,
                                              bool applyPreferences)
{
    auto *ml = new MultiLayer(this, layers, rows, cols);
    initMultilayerPlot(ml, caption);
    if (applyPreferences)
        for (Graph *g : ml->layersList())
            setPreferences(g);
    return ml;
}

MultiLayer *ApplicationWindow::newGraph(const QString &caption)
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

    QString name = caption;

    while (alreadyUsedName(name))
        name = generateUniqueName(tr("Graph"));

    MultiLayer *ml = multilayerPlot(name);
    if (!ml)
        return nullptr;

    ml->setMaximized(prevActiveWindow, d_mdi_windows_area);

    Graph *g = ml->activeLayer();
    if (g)
        setPreferences(g);

    ml->arrangeLayers(false, true);
    updateWindowLists(ml);

    return ml;
}

MultiLayer *ApplicationWindow::multilayerPlot(Table *w, const QStringList &colList, int style, int startRow, int endRow)
{
    if (!w)
        return nullptr;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    MultiLayer *ml = multilayerPlot(generateUniqueName(tr("Graph")));
    if (!ml)
        return nullptr;

    Graph *g = ml->activeLayer();
    if (!g)
        return nullptr;

    setPreferences(g);
    g->addCurves(w, colList, style, defaultCurveLineWidth, defaultSymbolSize, startRow, endRow);

    ml->arrangeLayers(false, true);
    g->newLegend();

    ml->setMaximized(w);
    updateWindowLists(ml);

    QApplication::restoreOverrideCursor();
    return ml;
}

MultiLayer* ApplicationWindow::multilayerPlot(int c, int r, int style, const MultiLayer::AlignPolicy& align)
{//used when plotting from the panel menu
	Table *t = (Table *)activeWindow(TableWindow);
    if (!t)
		return 0;

	if (!validFor2DPlot(t, (Graph::CurveType)style))
		return 0;

	QStringList list = t->drawableColumnSelection();
	if((int)list.count() < 1) {
		QMessageBox::warning(this, tr("QTISAS - Plot error"), tr("Please select a Y column to plot!"));
		return 0;
	}

	int curves = list.count();
	if (r < 0)
		r = curves;

	int layers = c*r;
	MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")), layers, r, c);
	QList<Graph *> layersList = g->layersList();
	int i = 0;
	foreach(Graph *ag, layersList){
		setPreferences(ag);
		if (i < curves){
			QStringList lst = QStringList() << list[i];
			for (int j = 0; j < curves; j++){
				int col = t->colIndex(list[j]);
				if (t->colPlotDesignation(col) == Table::xErr ||
					t->colPlotDesignation(col) == Table::yErr ||
					t->colPlotDesignation(col) == Table::Label){
					lst << list[j];
				}
			}
			ag->addCurves(t, lst, style, defaultCurveLineWidth, defaultSymbolSize);
		}
		i++;
	}

	if (align == MultiLayer::AlignCanvases){
		g->setAlignPolicy(align);
		g->setSpacing(0, 0);
		g->setCommonLayerAxes();
		connect(layersList.last(), SIGNAL(updatedLayout(Graph *)), g, SLOT(updateLayersLayout(Graph *)));
	} else {
		g->arrangeLayers(false, true);
		foreach(Graph *ag, layersList){
			if (ag->curveCount())
				ag->newLegend();
		}
	}
	return g;
}

MultiLayer* ApplicationWindow::waterfallPlot()
{
	Table *t = (Table *)activeWindow(TableWindow);
    if (!t)
		return 0;

	return waterfallPlot(t, t->selectedYColumns());
}

MultiLayer *ApplicationWindow::waterfallPlot(Table *t, const QStringList &list, bool interactive)
{
	if (!t)
		return 0;

	if(list.count() < 1){
        if (interactive)
            QMessageBox::warning(this, tr("QTISAS - Plot error"), tr("Please select a Y column to plot!"));
		return 0;
	}

	MultiLayer* ml = new MultiLayer(this);

	Graph *g = ml->activeLayer();
	setPreferences(g);
	g->enableAxis(QwtPlot::xTop, false);
	g->enableAxis(QwtPlot::yRight, false);
	g->setCanvasFrame(0);
    g->setTitle(QString());
    g->plotLayout()->setCanvasMargin(0);
	g->setFrame(0);
	g->addCurves(t, list, Graph::Line);
	g->setWaterfallOffset(10, 20);

	initMultilayerPlot(ml);
	ml->arrangeLayers(false, true);
	ml->setWaterfallLayout();

	g->newLegend()->move(QPoint(g->x() + g->canvas()->x() + 5, 5));

	return ml;
}

void ApplicationWindow::initMultilayerPlot(MultiLayer* g, const QString& name)
{
	QString label = name;
	while(alreadyUsedName(label))
		label = generateUniqueName(tr("Graph"));

	g->setWindowTitle(label);
	g->setName(label);
	g->setWindowIcon(QIcon(":/graph.png"));
	g->setScaleLayersOnPrint(d_scale_plots_on_print);
	g->printCropmarks(d_print_cropmarks);

	if (d_mdi_windows_area)
		d_workspace->addSubWindow(g);
	else
		g->setParent(0);

	connectMultilayerPlot(g);
        g->showNormal();

	addListViewItem(g);
    windowActivated(g);
}

void ApplicationWindow::setAutoUpdateTableValues(bool on)
{
	if (d_auto_update_table_values == on)
		return;

	d_auto_update_table_values = on;

	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
            if (w->inherits("Table"))
            	((Table *)w)->setAutoUpdateValues(d_auto_update_table_values);
		}
		f = f->folderBelow();
	}
}

void ApplicationWindow::customTable(Table* w)
{
	QPalette pal;
	pal.setColor(QPalette::Base, tableBkgdColor);
	pal.setColor(QPalette::Text, tableTextColor);
	w->setPalette(pal);

	w->setHeaderColor(tableHeaderColor);
    w->setHeaderColorRows(tableHeaderColorRows);

	w->setTextFont(tableTextFont);
	w->setHeaderFont(tableHeaderFont);
	w->showComments(d_show_table_comments);
	w->setNumericPrecision(d_decimal_digits);
}

void ApplicationWindow::setPreferences(Graph* g)
{
    if (!g)
        return;

    if (!g->isPiePlot())
    {
        for (int i = 0; i < QwtPlot::axisCnt; i++)
        {
            bool show = d_show_axes[i];
            g->enableAxis(i, show);
            if (show)
            {
                auto *sd = (ScaleDraw *)g->axisScaleDraw(i);
                sd->enableComponent(QwtAbstractScaleDraw::Labels, d_show_axes_labels[i]);
                sd->setSpacing(d_graph_tick_labels_dist);
                if (i == QwtPlot::yRight && !d_show_axes_labels[i])
                    g->setAxisTitle(i, tr(" "));
            }
        }

        g->grid()->copy(d_default_2D_grid);
        g->showMissingDataGap(d_show_empty_cell_gap);

        g->updateSecondaryAxis(QwtPlot::xTop);
        g->updateSecondaryAxis(QwtPlot::yRight);

        QList<int> ticksList;
        ticksList << majTicksStyle << majTicksStyle << majTicksStyle << majTicksStyle;
        g->setMajorTicksType(ticksList);
        ticksList.clear();
        ticksList << minTicksStyle << minTicksStyle << minTicksStyle << minTicksStyle;
        g->setMinorTicksType(ticksList);

        g->setTicksLength(minTicksLength, majTicksLength);
        g->setAxesLinewidth(axesLineWidth);
        g->drawAxesBackbones(drawBackbones);
        g->setCanvasFrame(canvasFrameWidth, d_canvas_frame_color);
        for (int i = 0; i < QwtPlot::axisCnt; i++)
        {
            g->setAxisTitleDistance(i, d_graph_axes_labels_dist);
            g->setAxisMargin(i, 0);
            g->setAxisMaxMajor(i, 10);
            g->setAxisMaxMinor(i, 4);
        }
    }

    g->setAxisTitlePolicy(d_graph_axis_labeling);
    g->setSynchronizedScaleDivisions(d_synchronize_graph_scales);
    g->initFonts(plotAxesFont, plotNumbersFont);
    g->initTitle(titleOn, plotTitleFont);

    g->plotLayout()->setCanvasMargin(defaultPlotMargin);
    g->enableAutoscaling(autoscale2DPlots);
    g->setAutoscaleFonts(autoScaleFonts);
    g->setAntialiasing(antialiasing2DPlots);
    g->disableCurveAntialiasing(d_disable_curve_antialiasing, d_curve_max_antialising_size);
    g->setFrame(d_graph_border_width, d_graph_border_color);

    QColor c = d_graph_background_color;
    c.setAlphaF(0.01 * d_graph_background_opacity);
    g->setBackgroundColor(c);

    c = d_graph_canvas_color;
    c.setAlphaF(0.01 * d_graph_canvas_opacity);
    g->setCanvasBackground(c);

    g->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignHCenter | Qt::AlignBottom);
    g->setAxisLabelAlignment(QwtPlot::xTop, Qt::AlignHCenter | Qt::AlignTop);
    g->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);
}

// return the current Plot (used for the Python interface)
MultiLayer* ApplicationWindow::currentPlot()
{
    auto p = dynamic_cast<MultiLayer *>(activeWindow());
	return p;
}

Table *ApplicationWindow::currentTable()
{
    auto *t = (Table *)activeWindow(TableWindow);
    return t;
}

Table *ApplicationWindow::newTable()
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

    auto *t = new Table(scriptEnv, 30, 2, "", this, nullptr);
    if (!t)
        return nullptr;

    initTable(t, generateUniqueName(tr("Table")));
    t->setMaximized(prevActiveWindow, d_mdi_windows_area);
    updateWindowLists(t);

    return t;
}
// used when opening a project file
Table *ApplicationWindow::newTable(const QString &caption, int r, int c)
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

    auto *t = new Table(scriptEnv, r, c, "", this, nullptr);
    if (!t)
        return nullptr;

    initTable(t, caption);

    if (d_is_appending_file && t->objectName() != caption)
    {
        // the table was renamed
        renamedTables << caption << t->objectName();
        if (d_inform_rename_table)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(
                this, tr("QTISAS - Renamed Window"),
                tr("The table '%1' already exists. It has been renamed '%2'.").arg(caption).arg(t->objectName()));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        }
    }

    t->setMaximized(prevActiveWindow, d_mdi_windows_area);
    updateWindowLists(t);

    return t;
}

Table* ApplicationWindow::newTable(int r, int c, const QString& name, const QString& legend)
{
	Table* w = new Table(scriptEnv, r, c, legend, this, 0);
	initTable(w, name);
	return w;
}

Table* ApplicationWindow::newHiddenTable(const QString& name, const QString& label, int r, int c, const QString& text)
{
	Table* w = new Table(scriptEnv, r, c, label, this, 0);

	if (!text.isEmpty())
    {
        QStringList rows = text.split("\n", Qt::SkipEmptyParts);
		QStringList list = rows[0].split("\t");
		w->setHeader(list);

		QString rlist;
		for (int i=0; i<r; i++)
        {
			rlist=rows[i+1];
			list = rlist.split("\t");
			for (int j=0; j<c; j++)
				w->setText(i, j, list[j]);
		}
	}

    initTable(w, name);

    hideWindow(w);

	return w;
}

void ApplicationWindow::initTable(Table* w, const QString& caption)
{
	QString name = caption;

	while(name.isEmpty() || alreadyUsedName(name))
        if (w->objectName()=="" || caption=="") name = generateUniqueName(tr("Table")); else name = generateUniqueName(caption);

	if (d_mdi_windows_area)
		d_workspace->addSubWindow(w);
	else
		w->setParent(0);

	connectTable(w);
	customTable(w);

	w->setName(name);

	w->setWindowIcon(QIcon(":/worksheet.png") );
	addListViewItem(w);
}

TableStatistics *ApplicationWindow::newTableStatistics(Table *base, int type, QList<int> target, int start, int end,
                                                       const QString &caption)
{
    auto *s = new TableStatistics(scriptEnv, this, base, (TableStatistics::Type)type, std::move(target), start, end);

    s->setObjectName("temp-Stat");

    if (caption.isEmpty())
        initTable(s, s->objectName());
    else
        initTable(s, caption);

    s->showNormal();
    return s;
}

// return the current note (used for the Python interface)
Note* ApplicationWindow::currentNote()
{
	Note* m = (Note*)activeWindow(NoteWindow);
	return m;
}

// creates a new empty note window
Note* ApplicationWindow::newNote(const QString& caption)
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

#ifdef SCRIPTING_PYTHON
    Note *m = new Note(scriptEnv, console, "", this);
#else
    Note *m = new Note(scriptEnv, "", this);
#endif

	QString name = caption;
	while(name.isEmpty() || alreadyUsedName(name)) name = generateUniqueName(tr("Notes"));

	m->setName(name);
	m->setWindowIcon(QIcon(":/note.png"));
	m->askOnCloseEvent(confirmCloseNotes);

	if (d_mdi_windows_area) d_workspace->addSubWindow(m);
	else m->setParent(0);

	addListViewItem(m);

	connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
	connect(m, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
	connect(m, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
	connect(m, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
	connect(m, SIGNAL(statusChanged(MdiSubWindow*)), this, SLOT(updateWindowStatus(MdiSubWindow*)));
	connect(m, SIGNAL(dirPathChanged(const QString&)), this, SLOT(scriptsDirPathChanged(const QString&)));
	connect(m, SIGNAL(currentEditorChanged()), this, SLOT(scriptingMenuAboutToShow()));

    m->setMaximized(prevActiveWindow, d_mdi_windows_area);
    updateWindowLists(m);

    return m;
}

Note* ApplicationWindow::newNoteText(const QString& caption, QString text)
{
#ifdef SCRIPTING_PYTHON
    Note *m = new Note(scriptEnv, console, "", this);
#else
    Note *m = new Note(scriptEnv, "", this);
#endif

    QString name = caption;
    while(name.isEmpty() || alreadyUsedName(name))
        name = generateUniqueName(tr("Notes"));

    m->setName(name);
    m->setWindowIcon(QIcon(":/note.png"));
    m->askOnCloseEvent(confirmCloseNotes);

    if (d_mdi_windows_area)
        d_workspace->addSubWindow(m);
    else
        m->setParent(0);

    addListViewItem(m);

    connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
    connect(m, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
    connect(m, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
    connect(m, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
    connect(m, SIGNAL(statusChanged(MdiSubWindow*)), this, SLOT(updateWindowStatus(MdiSubWindow*)));
    connect(m, SIGNAL(dirPathChanged(const QString&)), this, SLOT(scriptsDirPathChanged(const QString&)));
    connect(m, SIGNAL(currentEditorChanged()), this, SLOT(scriptingMenuAboutToShow()));

    m->setText ( text );
    m->showMaximized();
    return m;
}

void ApplicationWindow::connectScriptEditor(ScriptEdit *editor)
{
	if (!editor)
		return;
	QTextDocument *doc = editor->document();
	actionUndo->setEnabled(doc->isUndoAvailable());
	actionRedo->setEnabled(doc->isRedoAvailable());
	connect(editor, SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
	connect(editor, SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));
}

// return the current Matrix (used for the Python interface)
Matrix* ApplicationWindow::currentMatrix()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	return m;
}

Matrix* ApplicationWindow::newMatrix(int rows, int columns)
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

	Matrix* m = new Matrix(scriptEnv, rows, columns, "", this, 0);
	initMatrix(m, generateUniqueName(tr("Matrix")));

    m->setCoordinates(1,columns,1,rows);
    m->setCoordinates(1,columns,1,rows);

    m->setMaximized(prevActiveWindow, d_mdi_windows_area);
    updateWindowLists(m);

    return m;
}

Matrix* ApplicationWindow::newMatrix(const QString& caption, int r, int c)
{
    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();

	Matrix* w = new Matrix(scriptEnv, r, c, "", this, 0);
	initMatrix(w, caption);
	if (d_is_appending_file && w->objectName() != caption)
    {
        //the matrix was renamed
		renamedTables << caption << w->objectName();
		if (d_inform_rename_table)
        {
			QApplication::restoreOverrideCursor();
			QMessageBox:: warning(this, tr("QTISAS - Renamed Window"),
			tr("The table '%1' already exists. It has been renamed '%2'.").arg(caption).arg(w->objectName()));
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		}
	}
    w->setCoordinates(1,c,1,r);
    w->setCoordinates(1,c,1,r);

    w->setMaximized(prevActiveWindow, d_mdi_windows_area);
    updateWindowLists(w);

    return w;
}

void ApplicationWindow::viewMatrixImage()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->undoStack()->push(new MatrixSetViewCommand(m, m->viewType(), Matrix::ImageView, tr("Set Image Mode")));
	m->setViewType(Matrix::ImageView);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixTable()
{
	Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetViewCommand(m, m->viewType(), Matrix::TableView, tr("Set Data Mode")));
	m->setViewType(Matrix::TableView);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixXY()
{
    Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetHeaderViewCommand(m, m->headerViewType(), Matrix::XY, tr("Show X/Y")));
	m->setHeaderViewType(Matrix::XY);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixColumnRow()
{
    Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetHeaderViewCommand(m, m->headerViewType(), Matrix::ColumnRow, tr("Show Column/Row")));
	m->setHeaderViewType(Matrix::ColumnRow);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixDefaultScale()
{
	Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetColorMapCommand(m, m->colorMapType(), m->colorMap(), Matrix::Default,
                                                      new LinearColorMap(), tr("Set Default Palette")));
	m->setDefaultColorMap();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixGrayScale()
{
	Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetColorMapCommand(m, m->colorMapType(), m->colorMap(), Matrix::GrayScale,
                                                      new LinearColorMap(), tr("Set Gray Scale Palette")));
	m->setGrayScale();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixRainbowScale()
{
	Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m->undoStack()->push(new MatrixSetColorMapCommand(m, m->colorMapType(), m->colorMap(), Matrix::Rainbow,
                                                      new LinearColorMap(), tr("Set Rainbow Palette")));
	m->setRainbowColorMap();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::showColorMapDialog()
{
	Matrix* m = static_cast<Matrix*>(activeWindow(MatrixWindow));
	if (!m)
		return;

	ColorMapDialog *cmd = new ColorMapDialog(this);
	cmd->setMatrix(m);
	cmd->exec();
}

void ApplicationWindow::transposeMatrix()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->transpose();
}

void ApplicationWindow::flipMatrixVertically()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->flipVertically();
}

void ApplicationWindow::flipMatrixHorizontally()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->flipHorizontally();
}

void ApplicationWindow::rotateMatrix90()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->rotate90();
}

void ApplicationWindow::rotateMatrixMinus90()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->rotate90(false);
}

void ApplicationWindow::matrixDeterminant()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	QDateTime dt = QDateTime::currentDateTime ();
    QString info = locale().toString(dt, QLocale::ShortFormat);
	info+= "\n" + tr("Determinant of ") + QString(m->objectName()) + ":\t";
	info+= "det = " + QString::number(m->determinant()) + "\n";
	info+="-------------------------------------------------------------\n";

	current_folder->appendLogInfo(info);

	showResults(true);
}

void ApplicationWindow::invertMatrix()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->invert();
}

Table* ApplicationWindow::convertMatrixToTableDirect()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return 0;

	return matrixToTable(m, Direct);
}

Table* ApplicationWindow::convertMatrixToTableXYZ()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return 0;

	return matrixToTable(m, XYZ);
}

Table* ApplicationWindow::convertMatrixToTableYXZ()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return 0;

	return matrixToTable(m, YXZ);
}

Table* ApplicationWindow::matrixToTable(Matrix* m, MatrixToTableConversion conversionType)
{
	if (!m)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = m->numRows();
	int cols = m->numCols();
	MatrixModel *mModel = m->matrixModel();

	Table* w = nullptr;
	if (conversionType == Direct){
		w = new Table(scriptEnv, rows, cols, "", this, 0);
		for (int i = 0; i<rows; i++){
			for (int j = 0; j<cols; j++)
				w->setCell(i, j, m->cell(i,j));
		}
	} else if (conversionType == XYZ){
		int tableRows = rows*cols;
		w = new Table(scriptEnv, tableRows, 3, "", this, 0);
		for (int i = 0; i<rows; i++){
			for (int j = 0; j<cols; j++){
				int cell = i*cols + j;
				w->setCell(cell, 0, mModel->x(j));
				w->setCell(cell, 1, mModel->y(i));
				w->setCell(cell, 2, mModel->cell(i, j));
			}
		}
	} else if (conversionType == YXZ){
		int tableRows = rows*cols;
		w = new Table(scriptEnv, tableRows, 3, "", this, 0);
		for (int i = 0; i<cols; i++){
			for (int j = 0; j<rows; j++){
				int cell = i*rows + j;
				w->setCell(cell, 0, mModel->x(i));
				w->setCell(cell, 1, mModel->y(j));
				w->setCell(cell, 2, mModel->cell(i, j));
			}
		}
	}

    if (conversionType == YXZ || conversionType == XYZ)
    {
        w->setColPlotDesignation(2, Table::Z);
        w->setHeaderColType();
    }

	initTable(w, generateUniqueName(tr("Table")));
	w->setWindowLabel(m->windowLabel());
	w->setCaptionPolicy(m->captionPolicy());
	w->resize(m->size());

    w->setMaximized(m);

	QApplication::restoreOverrideCursor();
	return w;
}

void ApplicationWindow::initMatrix(Matrix* m, const QString& caption)
{
	QString name = caption;
	while(alreadyUsedName(name)){name = generateUniqueName(tr("Matrix"));}

	m->setWindowTitle(name);
	m->setName(name);
	m->setWindowIcon(QIcon(":/matrix.png") );
	m->askOnCloseEvent(confirmCloseMatrix);
	m->setNumericPrecision(d_decimal_digits);

	if (d_mdi_windows_area)
		d_workspace->addSubWindow(m);
	else
		m->setParent(0);

	addListViewItem(m);

	QUndoStack *stack = m->undoStack();
	connect(stack, SIGNAL(canUndoChanged(bool)), actionUndo, SLOT(setEnabled(bool)));
	connect(stack, SIGNAL(canRedoChanged(bool)), actionRedo, SLOT(setEnabled(bool)));
	connect(m, SIGNAL(modifiedWindow(MdiSubWindow*)), this, SLOT(modifiedProject(MdiSubWindow*)));
	connect(m, SIGNAL(modifiedData(Matrix*)), this, SLOT(updateMatrixPlots(Matrix *)));
	connect(m, SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
	connect(m, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
	connect(m, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
	connect(m, SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
	connect(m, SIGNAL(showContextMenu()), this, SLOT(showWindowContextMenu()));

	emit modified();
}

void ApplicationWindow::showBinMatrixDialog()
{
	Table* t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

    MySelection sel = t->getSelection();
	if (t->selectedYColumns().size() != 1 || fabs(sel.topRow() - sel.bottomRow()) < 2){
        QMessageBox::warning(this, tr("QTISAS - Column selection error"),
			tr("You must select a single Y column that has an associated X column!"));
		return;
	}

	int ycol = t->colIndex(t->selectedYColumns()[0]);
	if (t->isEmptyColumn(ycol)){
		QMessageBox::critical(this, tr("QTISAS - Error"), tr("The selected Y column is empty, operation aborted!"));
		return;
	} else if (t->isEmptyColumn(t->colX(ycol))){
		QMessageBox::critical(this, tr("QTISAS - Error"), tr("The associated X column is empty, operation aborted!"));
		return;
	}

	CreateBinMatrixDialog *cbmd = new CreateBinMatrixDialog(t, sel.topRow(), sel.bottomRow(), this);
	cbmd->exec();
}

void ApplicationWindow::showNoDataMessage()
{
	QMessageBox::critical(this, tr("QTISAS"), tr("Input error: empty data set!"));
}

void ApplicationWindow::smoothMatrix()
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	m->smooth();
}

void ApplicationWindow::expandMatrix()
{
	showMatrixResamplingDialog();
}

void ApplicationWindow::shrinkMatrix()
{
	showMatrixResamplingDialog(true);
}

void ApplicationWindow::showMatrixResamplingDialog(bool shrink)
{
	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	if (!m->isEmpty()){
		MatrixResamplingDialog *mrd = new MatrixResamplingDialog(m, shrink, this);
		mrd->exec();
	} else
		showNoDataMessage();
}

void ApplicationWindow::convertTableToMatrixRandomXYZ()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList selection = t->selectedColumns();
    MySelection sel = t->getSelection();
	if (selection.size() != 1 || t->colPlotDesignation(t->colIndex(selection[0])) != Table::Z ||
		fabs(sel.topRow() - sel.bottomRow()) < 2){
		QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("You must select exactly one Z column!"));
		return;
	}

	int startRow = sel.topRow();
	int endRow = sel.bottomRow();
	int zcol = t->colIndex(selection[0]);
	if (zcol < 0 || zcol >= t->numCols())
		return;

	int ycol = t->colY(zcol);
	int xcol = t->colX(ycol);

	int cells = 0;
	for (int i = startRow; i <= endRow; i++){
		QString xs = t->text(i, xcol);
		QString ys = t->text(i, ycol);
		QString zs = t->text(i, zcol);
		if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty())
			cells++;
	}
	if (!cells)
		return;

	GriddingDialog *gd = new GriddingDialog(t, selection[0], cells, this);
	gd->exec();
}

void ApplicationWindow::showChiSquareTestDialog()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	StudentTestDialog *std = new StudentTestDialog(StatisticTest::ChiSquareTest, t, false, this);
	std->show();
}

void ApplicationWindow::showStudentTestDialog(bool twoSamples)
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	StudentTestDialog *std = new StudentTestDialog(StatisticTest::StudentTest, t, twoSamples, this);
	std->show();
}

void ApplicationWindow::testNormality()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	AnovaDialog *ad = new AnovaDialog(this, t, StatisticTest::NormalityTest);
	ad->show();
}

void ApplicationWindow::showANOVADialog(bool twoWay)
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	AnovaDialog *ad = new AnovaDialog(this, t, StatisticTest::AnovaTest, twoWay);
	ad->show();
}

Matrix* ApplicationWindow::tableToMatrixRegularXYZ(Table* t, const QString& colName)
{
	if (!t)
		t = (Table*)activeWindow(TableWindow);
	if (!t)
		return 0;

	int startRow = 0;
	int endRow = t->numRows() - 1;

	int zcol = -1;
	if (colName.isEmpty()){
        MySelection sel = t->getSelection();
		if (t->selectedColumns().size() != 1 ||
			t->colPlotDesignation(t->colIndex(t->selectedColumns()[0])) != Table::Z ||
			fabs(sel.topRow() - sel.bottomRow()) < 2){
			QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("You must select exactly one Z column!"));
			return 0;
		}
		zcol = t->colIndex(t->selectedColumns()[0]);
		startRow = sel.topRow();
		endRow = sel.bottomRow();
	} else
		zcol = t->colIndex(colName);

	if (zcol < 0 || zcol >= t->numCols())
		return 0;

	int ycol = t->colY(zcol);
	int xcol = t->colX(ycol);

	int cells = 0;
	for (int i = startRow; i <= endRow; i++){
		QString xs = t->text(i, xcol);
		QString ys = t->text(i, ycol);
		QString zs = t->text(i, zcol);
		if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty())
			cells++;
	}

	if (!cells)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QLocale locale = this->locale();
	bool xVariesFirst = false;
	int firstValidRow = startRow;
	double x0 = 0.0, y0 = 0.0, xstart = 0.0, ystart = 0.0;
	double tolerance = 0.15;
	for (int i = startRow; i <= endRow; i++){
		QString xs = t->text(i, xcol);
		QString ys = t->text(i, ycol);
		QString zs = t->text(i, zcol);
		if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty()){
			x0 = locale.toDouble(xs);
			y0 = locale.toDouble(ys);
			xstart = x0;
			ystart = y0;
			firstValidRow = i;
			for (int j = i + 1; j <= endRow; j++){
				xs = t->text(j, xcol);
				ys = t->text(j, ycol);
				zs = t->text(j, zcol);
				if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty()){
					double x = locale.toDouble(xs);
					double y = locale.toDouble(ys);
					if (fabs(x - x0) > fabs(tolerance*x0) && fabs(y - y0) <= fabs(tolerance*y0))
						xVariesFirst = true;
					break;
				}
			}
			break;
		}
	}

	int rows = 0;
	int cols = 0;
	for (int i = firstValidRow; i <= endRow; i++){
		QString xs = t->text(i, xcol);
		QString ys = t->text(i, ycol);
		QString zs = t->text(i, zcol);
		if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty()){
			double x = locale.toDouble(xs);
			double y = locale.toDouble(ys);
			if (xVariesFirst){
				if (fabs(y - y0) <= tolerance){
					cols++;
					y0 = y;
				} else
					break;
			} else {
				if (fabs(x - x0) <= tolerance){
					rows++;
					x0 = x;
				} else
					break;
			}
		}
	}

	if (rows)
		cols = cells/rows;
	else
		rows = cells/cols;

	double xend = xstart, yend = ystart;
	Matrix* m = newMatrix(rows, cols);
	for (int i = startRow; i <= endRow; i++){
		QString xs = t->text(i, xcol);
		QString ys = t->text(i, ycol);
		QString zs = t->text(i, zcol);
		if (!xs.isEmpty() && !ys.isEmpty() && !zs.isEmpty()){
			int row = 0, col = 0;
			if (xVariesFirst){
				col = i%cols;
				row = i/cols;
			} else {
				row = i%rows;
				col = i/rows;
			}

			double x = locale.toDouble(xs);
			double y = locale.toDouble(ys);
			if (x > xend)
				xend = x;
			if (y > yend)
				yend = y;

			m->setCell(row, col, locale.toDouble(zs));
		}
	}

	m->setCoordinates(qMin(xstart, xend), qMax(xstart, xend), qMin(ystart, yend), qMax(ystart, yend));

	QApplication::restoreOverrideCursor();
	return m;
}

Matrix* ApplicationWindow::convertTableToMatrix()
{
	Table* t = (Table*)activeWindow(TableWindow);
	if (!t)
		return 0;

	return tableToMatrix (t);
}

Matrix* ApplicationWindow::tableToMatrix(Table* t)
{
	if (!t)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = t->numRows();
	int cols = t->numCols();
        for (int i = cols - 1; i >= 0; i--){
                if (t->isEmptyColumn(i))
                    cols--;
        }

        if (!cols){
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, tr("QTISAS - Error"), tr("The selected table is empty, operation aborted!"));
            return 0;
        }

	Matrix* m = newMatrix(rows, cols);
	for (int i = 0; i<rows; i++){
		for (int j = 0; j<cols; j++)
			m->setCell(i, j, t->cell(i, j));
	}

	m->showNormal();

	QApplication::restoreOverrideCursor();
	return m;
}

MdiSubWindow* ApplicationWindow::window(const QString& name, bool label)
{
	QList<MdiSubWindow *> windows = windowsList();
	if (label){
		foreach(MdiSubWindow *w, windows){
			if (w->windowLabel() == name)
				return w;
		}
	} else {
		foreach(MdiSubWindow *w, windows){
			if (w->objectName() == name)
				return w;
		}
	}
	return  nullptr;
}

Table* ApplicationWindow::table(const QString& name, bool justTableName)
{
    QString caption;
    if (justTableName) caption = name;
    else caption = name.left(name.lastIndexOf("_"));

	Folder *f = projectFolder();
	while (f){
		foreach(MdiSubWindow *w, f->windowsList()){
			if (w->inherits("Table") && w->objectName() == caption)
				return qobject_cast<Table*>(w);
		}
		f = f->folderBelow();
	}
	return  0;
}

Matrix* ApplicationWindow::matrix(const QString& name)
{
	QString caption = name;
	if (d_is_appending_file && !renamedTables.isEmpty() && renamedTables.contains(caption)){
		int index = renamedTables.indexOf(caption);
		caption = renamedTables[index + 1];
	}

	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if (QString(w->metaObject()->className()) == "Matrix" && w->objectName() == caption)
				return (Matrix*)w;
		}
		f = f->folderBelow();
	}
	return  0;
}

MdiSubWindow *ApplicationWindow::activeWindow(WindowType type)
{
	if (!d_active_window){
		QList<MdiSubWindow *> windows = current_folder->windowsList();
		if (!current_folder->activeWindow() && windows.size() > 0){
			d_active_window = windows[0];
		} else
			return nullptr;
	}

	switch(type){
		case NoWindow:
		break;

		case TableWindow:
			if (d_active_window->inherits("Table"))
				return d_active_window;
			else
				return nullptr;
		break;

		case MatrixWindow:
			return qobject_cast<Matrix *>(d_active_window);
		break;

		case MultiLayerWindow:
			return qobject_cast<MultiLayer *>(d_active_window);
		break;

		case NoteWindow:
			return qobject_cast<Note *>(d_active_window);
		break;

		case Plot3DWindow:
			return qobject_cast<Graph3D *>(d_active_window);
		break;
	}
	return d_active_window;
}

void ApplicationWindow::windowActivated(QMdiSubWindow *w)
{
    if (d_closing)
        return;

	MdiSubWindow *window = qobject_cast<MdiSubWindow *>(w);
	if (!window) return;

	if (d_active_window && d_active_window == window) return;

	d_active_window = window;

	customToolBars(window);
	customMenu(window);

	if (d_opening_file) return;

    d_active_window = window;

	Folder *f = window->folder();
	if (f) f->setActiveWindow(window);

	d_workspace->setActiveSubWindow(0);
	d_workspace->setActiveSubWindow(window);
	window->raise();
	emit modified();
}

void ApplicationWindow::addErrorBars()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (!g->curveCount())
    {
		QMessageBox::warning(this, tr("QTISAS - Warning"), tr("There are no curves available on this plot!"));
		return;
	}

    if (g->isPiePlot())
    {
		QMessageBox::warning(this, tr("QTISAS - Warning"), tr("This functionality is not available for pie plots!"));
		return;
	}

    auto ed = new ErrDialog(this);
	ed->setCurveNames(g->analysableCurvesList());
	ed->setSrcTables(tableList());
	ed->exec();
}

void ApplicationWindow::removeCurves(const QString &name, bool updatePlot)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "MultiLayer"){
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers)
                g->removeCurves(name, updatePlot);
		} else if (QString(w->metaObject()->className()) == "Graph3D"){
			if ( (((Graph3D*)w)->formula()).contains(name) )
				((Graph3D*)w)->clearData();
		}
	}

	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateCurves(Table *t, const QString &name)
{
    const auto windows = windowsList();
    foreach (MdiSubWindow *w, windows)
    {
        if (QString(w->metaObject()->className()) == "MultiLayer")
        {
            const auto layers = ((MultiLayer *)w)->layersList();
            foreach (Graph *g, layers)
                g->updateCurvesData(t, name);
        }
        else if (QString(w->metaObject()->className()) == "Graph3D")
        {
            auto g = (Graph3D *)w;
            if ((g->formula()).contains(name))
                g->updateData(t);
        }
    }
}

void ApplicationWindow::showPreferencesDialog()
{
	ConfigDialog* cd = new ConfigDialog(this);
	cd->setColumnSeparator(columnSeparator);
	cd->exec();
}

void ApplicationWindow::setSaveSettings(bool autoSaving, int min)
{
	if (autoSave==autoSaving && autoSaveTime==min)
		return;

	autoSave=autoSaving;
	autoSaveTime=min;

	killTimer(savingTimerId);

	if (autoSave)
		savingTimerId=startTimer(autoSaveTime*60000);
	else
		savingTimerId=0;
}

void ApplicationWindow::changeAppStyle(const QString& s)
{

	qApp->setStyle(s);
	appStyle = qApp->style()->objectName();

	QPalette pal = qApp->palette();

	pal.setColor ( QPalette::Active, QPalette::Base, QColor ( panelsColor ) );
    pal.setColor(QPalette::Active, QPalette::Window, QColor(239, 239, 239));
	pal.setColor ( QPalette::Active, QPalette::Button, QColor(225, 225, 225) );
    pal.setColor(QPalette::Inactive, QPalette::Window, QColor(239, 239, 239));
	pal.setColor ( QPalette::Inactive, QPalette::Button, QColor(225, 225, 225) );

	qApp->setPalette(pal);

}

void ApplicationWindow::changeAppFont(const QFont& f)
{
	if (appFont == f)
		return;

	appFont = f;
	updateAppFonts();
}

void ApplicationWindow::updateAppFonts()
{
	qApp->setFont(appFont);
	this->setFont(appFont);
	info->setFont(QFont(appFont.family(), 2 + appFont.pointSize(), QFont::Bold,false));
}

void ApplicationWindow::updateConfirmOptions(bool askTables, bool askMatrices, bool askPlots2D,
		bool askPlots3D, bool askNotes)
{
	QList<MdiSubWindow *> windows = windowsList();

	if (confirmCloseTable != askTables){
		confirmCloseTable=askTables;
		foreach(MdiSubWindow *w, windows){
			if (w->inherits("Table"))
				w->askOnCloseEvent(confirmCloseTable);
		}
	}

	if (confirmCloseMatrix != askMatrices){
		confirmCloseMatrix = askMatrices;
		foreach(MdiSubWindow *w, windows){
			if (QString(QString(w->metaObject()->className())) == "Matrix")
				w->askOnCloseEvent(confirmCloseMatrix);
		}
	}

	if (confirmClosePlot2D != askPlots2D){
		confirmClosePlot2D=askPlots2D;
		foreach(MdiSubWindow *w, windows){
			if (QString(QString(w->metaObject()->className())) == "MultiLayer")
				w->askOnCloseEvent(confirmClosePlot2D);
		}
	}

	if (confirmClosePlot3D != askPlots3D){
		confirmClosePlot3D=askPlots3D;
		foreach(MdiSubWindow *w, windows){
			if (QString(w->metaObject()->className()) == "Graph3D")
				w->askOnCloseEvent(confirmClosePlot3D);
		}
	}

	if (confirmCloseNotes != askNotes){
		confirmCloseNotes = askNotes;
		foreach(MdiSubWindow *w, windows){
			if (QString(w->metaObject()->className()) == "Note")
				w->askOnCloseEvent(confirmCloseNotes);
		}
	}
}

void ApplicationWindow::setGraphDefaultSettings(bool autoscale, bool scaleFonts, bool resizeLayers, bool antialiasing)
{
    if (autoscale2DPlots == autoscale && autoScaleFonts == scaleFonts && autoResizeLayers != resizeLayers &&
        antialiasing2DPlots == antialiasing)
        return;

    autoscale2DPlots = autoscale;
    autoScaleFonts = scaleFonts;
    autoResizeLayers = !resizeLayers;
    antialiasing2DPlots = antialiasing;
}

void ApplicationWindow::setArrowDefaultSettings(double lineWidth,  const QColor& c, Qt::PenStyle style,
		int headLength, int headAngle, bool fillHead)
{
	if (defaultArrowLineWidth == lineWidth &&
		defaultArrowColor == c &&
		defaultArrowLineStyle == style &&
		defaultArrowHeadLength == headLength &&
		defaultArrowHeadAngle == headAngle &&
		defaultArrowHeadFill == fillHead)
		return;

	defaultArrowLineWidth = lineWidth;
	defaultArrowColor = c;
	defaultArrowLineStyle = style;
	defaultArrowHeadLength = headLength;
	defaultArrowHeadAngle = headAngle;
	defaultArrowHeadFill = fillHead;
	saveSettings();
}

ApplicationWindow * ApplicationWindow::plotFile(const QString& fn)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	ApplicationWindow *app = new ApplicationWindow();
	app->restoreApplicationGeometry();

    QList<QByteArray> lst = QImageReader::supportedImageFormats() << "JPG";
    QStringList tempList;
    foreach(QByteArray temp,lst)// convert QList<QByteArray> to QStringList to be able to 'filter'
        tempList.append(QString(temp));

    QFileInfo fi(fn);
    QStringList l = tempList.filter(fi.suffix(), Qt::CaseInsensitive);
    if (l.count() > 0)
        app->importImage(fn);
    else {
        Table* t = app->newTable();
        if (!t) {
            QApplication::restoreOverrideCursor();
            return nullptr;
        }
        t->importASCII(fn, app->columnSeparator, 0, app->renameColumns, app->strip_spaces, app->simplify_spaces,
                app->d_ASCII_import_comments, app->d_ASCII_comment_string,
				app->d_ASCII_import_read_only, Table::Overwrite, app->d_ASCII_import_locale, app->d_ASCII_end_line);
        t->setCaptionPolicy(MdiSubWindow::Both);
        app->multilayerPlot(t, t->YColumns(), defaultCurveStyle);
    }

	QApplication::restoreOverrideCursor();
    savedProject();//force saved state
    close();
	return app;
}

Table * ApplicationWindow::importDatabase(const QString& fileName, int table)
{
	QString fn = fileName;
	if (fn.isEmpty()){
		QStringList filters;
		filters << tr("dBase") + " (*.dbf)";
	#ifdef Q_OS_WIN
		filters << tr("Microsoft Access") + " (*.mdb *accdb)";
	#else
		filters << tr("Microsoft Access") + " (*.mdb)";
	#endif
		filters << tr("SQLite 3") + " (*.db)";

        fn = getFileName(this, tr("Open Database"), QString(), filters.join(";"), nullptr, false);
		if (fn.isEmpty())
			return 0;
	}

	ImportExportPlugin *plugin = importPlugin(fn);
	if (plugin)
		return plugin->import(fn, table);

	return 0;
}

Table * ApplicationWindow::importWaveFile()
{
    QString fn = getFileName(this, tr("Open File"), QString(), "*.wav", nullptr, false);
	if (fn.isEmpty())
		return nullptr;

    QString log = locale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat) + " - ";
	log += tr("Imported sound file") + ": " + fn + "\n";
	ifstream file(fn.toLocal8Bit().constData(), ios::in | ios::binary);

	file.seekg(4, ios::beg);
	int chunkSize;
	file.read( (char*) &chunkSize, 4 ); // read the ChunkSize

	file.seekg(16, ios::beg);
	int	subChunk1Size;
	file.read( (char*) &subChunk1Size, 4 ); // read the SubChunk1Size

	short format;
	file.read( (char*) &format, sizeof(short) ); // read the file format.  This should be 1 for PCM
	log += tr("Format") + ": ";
	if (format != 1){
		QMessageBox::information(this, tr("QtiSAS"),
		tr("This is not a PCM type WAV file, operation aborted!"));
		log +=  QString::number(format) + "\n";
		showResults(log, true);
		return nullptr;
	} else
		log += tr("PCM") + "\n";

	short channels;
	file.read( (char*) &channels, sizeof(short) ); // read the # of channels (1 or 2)
	QStringList header;
	if (channels == 2)
		header << tr("Time") << tr("Left") << tr("Right");
	else if (channels == 1)
		header << tr("Time") << tr("Data");
	log += tr("Channels") + ": " + QString::number(channels) + "\n";

	int sampleRate;
	file.read( (char*) &sampleRate, sizeof(int) );
	log += tr("Sample Rate") + ": " + QString::number(sampleRate) + "\n";

	int byteRate;
	file.read( (char*) &byteRate, sizeof(int) );
	log += tr("Byte Rate") + ": " + QString::number(byteRate) + "\n";

	short blockAlign;
	file.read( (char*) &blockAlign, sizeof(short) );
	log += tr("Block Align") + ": " + QString::number(blockAlign) + "\n";

	short bitsPerSample;
	file.read( (char*) &bitsPerSample, sizeof(short) );
	log += tr("Bits Per Sample") + ": " + QString::number(bitsPerSample) + "\n";
	log += "__________________________________\n";
	showResults(log, true);

	int rows = (chunkSize - 36)/blockAlign;
	Table *t = newTable(rows, int(channels + 1), QFileInfo(fn).baseName(), fn);
	if (!t)
		return nullptr;

	t->setHeader(header);

	file.seekg(44, ios::beg);

	double dt = 1.0/(double)sampleRate;
	int aux = 1;
	if (bitsPerSample == 8){
		if (channels == 2)
			file.seekg(46, ios::beg);//???Why???
		int size = 1;
		uchar ch;
		while(!file.eof()){
			int i = aux - 1;
			t->setCell(i, 0, dt*aux);
			for (int j = 1; j <= channels; j++){
				file.read((char*) &ch, size);
				t->setText(i, j, QString::number(ch));
			}
			aux++;
			if (aux>rows) break;
		}
	} else if (bitsPerSample == 16){
		int size = sizeof(short);
		short ch;
		while(!file.eof()){
			int i = aux - 1;
			t->setCell(i, 0, dt*aux);
			for (int j = 1; j <= channels; j++){
				file.read((char*) &ch, size);
				t->setText(i, j, QString::number(ch));
			}
			aux++;
			if (aux>rows) break;
		}
	}

	file.close();
	t->show();
	return t;
}

void ApplicationWindow::importASCII(const QString& fileName)
{
	ImportASCIIDialog *import_dialog = new ImportASCIIDialog(!activeWindow(TableWindow) && !activeWindow(MatrixWindow), this, d_extended_import_ASCII_dialog);
	import_dialog->setDirectory(asciiDirPath);
	import_dialog->selectNameFilter(d_ASCII_file_filter);
	if (!fileName.isEmpty()){
		import_dialog->selectFile(fileName);
		import_dialog->setCurrentPath(fileName);
	}
	if (d_import_ASCII_dialog_size.isValid())
		import_dialog->resize(d_import_ASCII_dialog_size);
	if (import_dialog->exec() != QDialog::Accepted)
		return;

	asciiDirPath = import_dialog->directory().path();
    d_ASCII_import_mode = import_dialog->importMode();
    columnSeparator = import_dialog->columnSeparator();
    ignoredLines = import_dialog->ignoredLines();
    renameColumns = import_dialog->useFirstRow();
    strip_spaces = import_dialog->stripSpaces();
    simplify_spaces = import_dialog->simplifySpaces();
    d_ASCII_import_locale = import_dialog->decimalSeparators();
    d_ASCII_comment_string = import_dialog->commentString();
    d_ASCII_import_comments = import_dialog->useSecondRow();
    d_ASCII_import_read_only = import_dialog->readOnly();
    d_ASCII_end_line = (EndLineChar)import_dialog->endLineChar();
    saveSettings();

    importASCII(import_dialog->selectedFiles(), import_dialog->importMode(), import_dialog->columnSeparator(),
                import_dialog->ignoredLines(), import_dialog->renameColumns(), import_dialog->stripSpaces(),
                import_dialog->simplifySpaces(), import_dialog->importComments(), import_dialog->decimalSeparators(),
                import_dialog->commentString(), import_dialog->readOnly(), import_dialog->endLineChar(),
                import_dialog->columnTypes(), import_dialog->columnFormats(), import_dialog->columnDestinations());
}

void ApplicationWindow::importASCII(const QStringList& files, int import_mode, const QString& local_column_separator,
                                    int local_ignored_lines, bool local_rename_columns, bool local_strip_spaces,
                                    bool local_simplify_spaces, bool local_import_comments,
                                    const QLocale &local_separators, const QString &local_comment_string,
                                    bool import_read_only, int endLineChar, const QList<int> &colTypes,
                                    const QStringList &colFormats, const QList<int> &colDestinations)
{
	if (files.isEmpty())
		return;

	switch(import_mode) {
		case ImportASCIIDialog::NewTables:
			{
				int dx = 0, dy = 0;
				QStringList sorted_files = files;
				sorted_files.sort();
				int filesCount = sorted_files.size();
				for (int i=0; i<filesCount; i++){
					Table *w = newTable();
					if (!w)
						continue;

            w->importASCII(sorted_files[i], local_column_separator, local_ignored_lines, local_rename_columns,
                           local_strip_spaces, local_simplify_spaces, local_import_comments, local_comment_string,
                           import_read_only, Table::Overwrite, local_separators, endLineChar, -1, colTypes, colFormats,
                           colDestinations);
					if (!w) continue;
					w->setWindowLabel(sorted_files[i]);
					w->setCaptionPolicy(MdiSubWindow::Both);

					QString name = QFileInfo(sorted_files[i]).baseName();

                    QString nameNoMinus=name;
                    nameNoMinus=nameNoMinus.remove("-");
            if (!alreadyUsedName(name) && !nameNoMinus.contains(REGEXPS::nonalphanumeric))
						setWindowName(w, name);

					if (i == 0){
						dx = w->verticalHeaderWidth();
						dy = w->frameGeometry().height() - w->widget()->height();
					}
					if (filesCount > 1)
						w->move(QPoint(i*dx, i*dy));
				}
				modifiedProject();
				break;
			}
        case ImportASCIIDialog::NewMatrices:
			{
				int dx = 0, dy = 0;
				QStringList sorted_files = files;
				sorted_files.sort();
				int filesCount = sorted_files.size();
				for (int i=0; i<filesCount; i++){
					Matrix *w = newMatrix();
					if (!w)
                        continue;
					w->importASCII(sorted_files[i], local_column_separator, local_ignored_lines,
                                local_strip_spaces, local_simplify_spaces, local_comment_string,
								Matrix::Overwrite, local_separators, endLineChar);
					w->setWindowLabel(sorted_files[i]);
					w->setCaptionPolicy(MdiSubWindow::Both);

					QString name = QFileInfo(sorted_files[i]).baseName();
            if (!alreadyUsedName(name) && !name.contains(REGEXPS::nonalphanumeric))
						setWindowName(w, name);

					if (i == 0){
						dx = w->verticalHeaderWidth();
						dy = w->frameGeometry().height() - w->widget()->height();
					}
					if (filesCount > 1)
						w->move(QPoint(i*dx,i*dy));
				}
				modifiedProject();
				break;
			}

		case ImportASCIIDialog::NewColumns:
		case ImportASCIIDialog::NewRows:
			{
				MdiSubWindow *w = activeWindow();
				if (!w)
                    return;

        if (w->inherits("Table"))
        {
            auto *t = (Table *)w;
            for (int i = 0; i < files.size(); i++)
            {
                t->importASCII(files[i], local_column_separator, local_ignored_lines, local_rename_columns,
                               local_strip_spaces, local_simplify_spaces, local_import_comments, local_comment_string,
                               import_read_only, (Table::ImportMode)(import_mode - 2), local_separators, endLineChar,
                               -1, colTypes, colFormats, colDestinations);
            }
					t->notifyChanges();
					emit modifiedProject(t);
				} else if (QString(w->metaObject()->className()) == "Matrix"){
					Matrix *m = (Matrix *)w;
					for (int i=0; i<files.size(); i++){
						m->importASCII(files[i], local_column_separator, local_ignored_lines,
                         local_strip_spaces, local_simplify_spaces, local_comment_string,
						 (Matrix::ImportMode)(import_mode - 2), local_separators, endLineChar);
					}
				}
				w->setWindowLabel(files.join("; "));
				w->setCaptionPolicy(MdiSubWindow::Name);
				break;
			}
		case ImportASCIIDialog::Overwrite:
			{
				MdiSubWindow *w = activeWindow();
				if (!w)
                    return;

        if (w->inherits("Table"))
        {
            auto *t = (Table *)w;
            t->importASCII(files[0], local_column_separator, local_ignored_lines, local_rename_columns,
                           local_strip_spaces, local_simplify_spaces, local_import_comments, local_comment_string,
                           import_read_only, Table::Overwrite, local_separators, endLineChar, -1, colTypes, colFormats,
                           colDestinations);
            t->notifyChanges();
				} else if (QString(w->metaObject()->className()) == "Matrix"){
				    Matrix *m = (Matrix *)w;
					m->importASCII(files[0], local_column_separator, local_ignored_lines,
                          local_strip_spaces, local_simplify_spaces, local_comment_string,
						  Matrix::Overwrite, local_separators, endLineChar);
				}

                w->setWindowLabel(files[0]);
				w->setCaptionPolicy(MdiSubWindow::Both);

				QString name = QFileInfo(files[0]).baseName();
        if (!alreadyUsedName(name) && !name.contains(REGEXPS::nonalphanumeric))
					setWindowName(w, name);

                modifiedProject();
				break;
			}
	}
}

void ApplicationWindow::open()
{
	if (showSaveProjectMessage() == QMessageBox::Cancel)
		return;

	OpenProjectDialog *open_dialog = new OpenProjectDialog(this, d_extended_open_dialog);
	open_dialog->setDirectory(workingDir);

	if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
		return;
	workingDir = open_dialog->directory().path();
	d_open_project_filter = open_dialog->selectedNameFilter();

	switch(open_dialog->openMode()) {
		case OpenProjectDialog::NewProject:
			{
				QString fn = open_dialog->selectedFiles()[0];
				QFileInfo fi(fn);

				if (projectname != "untitled"){
					QFileInfo fi(projectname);
					QString pn = fi.absoluteFilePath();
					if (fn == pn){
						QMessageBox::warning(this, tr("QTISAS - File openning error"),
								tr("The file: <b>%1</b> is the current file!").arg(fn));
						return;
					}
				}

				if (!fi.exists ()){
					QMessageBox::critical(this, tr("QTISAS - File openning error"),
					tr("The file: <b>%1</b> doesn't exist!").arg(fn));
					return;
				}

				ApplicationWindow *a = open (fn);
				if (a){
					a->workingDir = workingDir;
					a->d_open_project_filter = d_open_project_filter;
					if (isProjectFile(fn)){
						recentProjects = a->recentProjects;//the recent projects must be saved
						this->close();
					} else
						saveSettings();//the recent projects must be saved
				}
				break;
			}
		case OpenProjectDialog::NewFolder:
			appendProject(open_dialog->selectedFiles()[0]);
			break;
	}
}

bool ApplicationWindow::isProjectFile(const QString& fn)
{
    if (fn.endsWith(".qti", Qt::CaseInsensitive) || fn.endsWith(".qti.gz", Qt::CaseInsensitive) ||
        fn.endsWith(".qti.svg", Qt::CaseInsensitive) || fn.endsWith(".qti~", Qt::CaseInsensitive) ||
        fn.endsWith(".opj", Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive))
        return true;
    return false;
}

bool ApplicationWindow::isFileReadable(const QString& file_name)
{
	QFileInfo fi(file_name);
	if (fi.isDir()){
		QMessageBox::critical(this, tr("QTISAS - File openning error"),
				tr("<b>%1</b> is a directory, please specify a file name!").arg(file_name));
		return false;
	} else if (!fi.exists()){
		QMessageBox::critical(this, tr("QTISAS - File openning error"),
				tr("The file: <b>%1</b> doesn't exist!").arg(file_name));
		return false;
	} else if (fi.exists() && !fi.isReadable()){
		QMessageBox::critical(this, tr("QTISAS - File openning error"),
				tr("You don't have the permission to open this file: <b>%1</b>").arg(file_name));
		return false;
	}
	return true;
}

ApplicationWindow* ApplicationWindow::open(const QString& fn, bool factorySettings, bool newProject)
{
	if (!this->isFileReadable(fn)){
		if (recentProjects.contains(fn)){
			recentProjects.removeAll(fn);
			updateRecentProjectsList();
		}
		return nullptr;
	}

	if (fn.endsWith(".db", Qt::CaseInsensitive) || fn.endsWith(".dbf", Qt::CaseInsensitive) ||
		fn.endsWith(".mdb", Qt::CaseInsensitive) || fn.endsWith(".accdb", Qt::CaseInsensitive)){
		importDatabase(fn);
		return this;
	} else if (fn.endsWith(".py", Qt::CaseInsensitive))
		return loadScript(fn);
    else if (fn.endsWith(".qti.svg", Qt::CaseInsensitive))
    {
        const QString tempQtiPath = extractEmbeddedProjectFromSvg(fn);
        if (tempQtiPath.isEmpty())
        {
            QMessageBox::critical(this, tr("QTISAS - File opening error"),
                                  tr("The file: <b>%1</b> doesn't contain an embedded QtiSAS project.").arg(fn));
            return nullptr;
        }

        ApplicationWindow *app = open(tempQtiPath, factorySettings, newProject);
        QFile::remove(tempQtiPath);
        if (app)
        {
            QString qtiPath = fn;
            qtiPath.chop(4); // ".svg"
            app->projectname = qtiPath;
            app->setWindowTitle(tr("QtiSAS") + " - " + qtiPath);
            app->recentProjects.removeAll(QDir::toNativeSeparators(tempQtiPath));
            app->updateRecentProjectsList(fn);
        }
        return app;
    }

	QString fname = fn;
	if (fn.endsWith(".qti.gz", Qt::CaseInsensitive)){//decompress using zlib
        file_uncompress((char *)fname.toLocal8Bit().constData());
		fname = fname.left(fname.size() - 3);
	}

	QFile f(fname);
	QTextStream t( &f );
    (void)f.open(QIODevice::ReadOnly);
	QString s = t.readLine();
	f.close();

    QStringList lst = s.split(REGEXPS::whitespaces, Qt::SkipEmptyParts);
	bool qtiProject = (lst.count() < 2 || lst[0] != "QtiPlot") ? false : true;
	if (!qtiProject){
		if (QFile::exists(fname + "~")){
            QMessageBox::StandardButton choice =
                QMessageBox::question(this, tr("QTISAS - File opening error"),
                                      tr("The file <b>%1</b> is corrupted, but there exists a backup copy."
                                         "<br>Do you want to open the backup instead?")
                                          .arg(fn),
                                      QMessageBox::Yes | QMessageBox::No);
            if (choice == QMessageBox::Yes)
                return open(fname + "~");
            else
                QMessageBox::critical(this, tr("QTISAS - File opening error"),
                                      tr("The file: <b>%1</b> was not created using QTISAS!").arg(fn));
            return nullptr;
		}

		return plotFile(fn);
	}

    QStringList vl = lst[1].split(".", Qt::SkipEmptyParts);
    d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();

	ApplicationWindow* app = openProject(fname, factorySettings, newProject);

	f.close();
	return app;
}

void ApplicationWindow::openRecentProject(QAction *action)
{
	QString fn = action->text();
	int pos = fn.indexOf(" ", 0);
	fn = fn.right(fn.length() - pos - 1);

	if (projectname != "untitled"){
		QFileInfo fi(projectname);
		QString pn = fi.absoluteFilePath();

		if (QDir::toNativeSeparators(fn) == QDir::toNativeSeparators(pn)){
			QMessageBox::warning(this, tr("QTISAS - File openning error"),
					tr("The file: <p><b> %1 </b><p> is the current file!").arg(QDir::toNativeSeparators(fn)));
			return;
		}
	}

	if (isProjectFile(fn)){
		if (showSaveProjectMessage() == QMessageBox::Cancel)
			return;
	}

	QFile f(fn);
	if (!f.exists()){
		QMessageBox::critical(this, tr("QTISAS - File Open Error"),
				tr("The file: <b> %1 </b> <p>does not exist anymore!"
					"<p>It will be removed from the list.").arg(fn));

		for (int i = 0; i < recentProjects.size(); i++){
			if (QDir::toNativeSeparators(recentProjects[i]) == fn){
				recentProjects.removeAt(i);
				break;
			}
		}
        updateRecentProjectsList();
		return;
	}

	if (!fn.isEmpty()){
		saveSettings();//the recent projects must be saved

		bool isSaved = saved;
		ApplicationWindow * a = open (fn);
		if (a){
			if (isSaved)
				savedProject();//force saved state
			if (isProjectFile(fn))
				close();
			else
				modifiedProject();
		}
	}
}

ApplicationWindow* ApplicationWindow::openProject(const QString& fn, bool factorySettings, bool newProject)
{
	ApplicationWindow *app = this;
	if (newProject)
		app = new ApplicationWindow(factorySettings);

	app->projectname = fn;
	app->d_file_version = d_file_version;
	app->setWindowTitle(tr("QtiSAS") + " - " + fn);
	app->d_opening_file = true;
	app->d_workspace->blockSignals(true);

	QFile f(fn);
	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    (void)f.open(QIODevice::ReadOnly);

	QFileInfo fi(fn);
	QString baseName = fi.fileName();

	t.readLine();
	if (d_file_version < 73) t.readLine();

	QString s = t.readLine();
    QStringList list = s.split("\t", Qt::SkipEmptyParts);
	if (list[0] == "<scripting-lang>")
    {
		if (!app->setScriptingLanguage(list[1]))
			QMessageBox::warning(app, tr("QTISAS - File opening error"),
					tr("The file \"%1\" was created using \"%2\" as scripting language.\n\n"\
						"Initializing support for this language FAILED; I'm using \"%3\" instead.\n"\
						"Various parts of this file may not be displayed as expected.")\
					.arg(fn).arg(list[1]).arg(scriptEnv->objectName()));

		s = t.readLine();
        list = s.split("\t", Qt::SkipEmptyParts);
	}
	int aux=0,widgets=list[1].toInt();

	QString titleBase = tr("Window") + ": ";
	QString title = titleBase + "1/" + QString::number(widgets) + "  ";

    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setRange(0, widgets);
    progress.setMinimumWidth(this->width() / 2);
    progress.setWindowTitle(tr("QTISAS - Opening file") + ": " + baseName);
    progress.setLabelText(title);
    progress.show();
    QApplication::processEvents();

	Folder *cf = app->projectFolder();
	app->folders->blockSignals (true);
	app->blockSignals (true);

	//rename project folder item
	FolderListItem *item = (FolderListItem *)app->folders->topLevelItem(0);
	item->setText(0, fi.baseName());
	item->folder()->setObjectName(fi.baseName());

	//process tables and matrix information
	while (!t.atEnd() && !progress.wasCanceled()) {
		s = t.readLine();
		list.clear();
		if (s.left(8) == "<folder>") {
			list = s.split("\t");
			Folder *f = new Folder(app->current_folder, list[1]);
			f->setBirthDate(list[2]);
            f->setModificationDate(list[2]);
            if(list.count() >= 4) if (s.right(7) == "current") { cf = f; };

			FolderListItem *fli = new FolderListItem(app->current_folder->folderListItem(), f);
			f->setFolderListItem(fli);

			app->current_folder = f;
		}
        else if (s.contains("<open>")) {
			app->current_folder->folderListItem()->setExpanded(s.remove("<open>").remove("</open>").toInt());
		}
        else if (s == "<table>") {
			title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			QStringList lst;
			while (!t.atEnd() && s!="</table>") {
				s = t.readLine();
				lst<<s;
			}
            lst.pop_back();

            QStringList list = lst[0].split("\t");

            MdiSubWindow *wactive = nullptr;
            if (auto *w = app->current_folder->activeWindow(); w && w->status() == 2)
                wactive = w;

            if (list[2].toInt() > 0)
            {
                if (auto *w = openTable(app, lst); w && lst[1].contains("maximized"))
                    wactive = w;

                if (wactive)
                {
                    app->current_folder->setActiveWindow(wactive);
                    wactive->setMaximized();
                }
            }
            progress.setValue(aux - 1);
		}
        else if (s.left(17)=="<TableStatistics>")
        {
			QStringList lst;
			while ( s!="</TableStatistics>" )
            {
				s=t.readLine();
				lst<<s;
			}
			lst.pop_back();

            QStringList list = lst[1].split("\t");

            MdiSubWindow *wactive = nullptr;
            if (auto *w = app->current_folder->activeWindow(); w && w->status() == 2)
                wactive = w;

            if (list.count() > 1)
            {
                if (auto *w = app->openTableStatistics(lst); w && lst.count() > 3 && lst[3].contains("maximized"))
                    wactive = w;

                if (wactive)
                {
                    app->current_folder->setActiveWindow(wactive);
                    wactive->setMaximized();
                }
            }
		}
        else if  (s == "<matrix>")
        {
			title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			QStringList lst;
			while ( s != "</matrix>" )
            {
				s=t.readLine();
				lst<<s;
			}
			lst.pop_back();

            MdiSubWindow *wactive = nullptr;
            if (auto *w = app->current_folder->activeWindow(); w && w->status() == 2)
                wactive = w;

            if (auto *w = openMatrix(app, lst); w && lst[1].contains("maximized"))
                wactive = w;

            if (wactive)
            {
                app->current_folder->setActiveWindow(wactive);
                wactive->setMaximized();
            }

            progress.setValue(aux - 1);
		}
        else if  (s == "<note>")
        {
			title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
            QStringList lst;
			for (int i=0; i<3; i++){
				s = t.readLine();
				lst << s;
			}

            MdiSubWindow *wactive = nullptr;
            if (auto *w = app->current_folder->activeWindow(); w && w->status() == 2)
                wactive = w;

            auto *n = openNote(app, lst);
			QStringList cont;
			while ( s != "</note>" ){
				s = t.readLine();
				cont << s;
			}
			cont.pop_back();
            n->restore(cont);

           if (n && lst[1].contains("maximized"))
                wactive = n;

            if (wactive)
            {
                app->current_folder->setActiveWindow(wactive);
                wactive->setMaximized();
            }

            progress.setValue(aux - 1);
		}
        else if  (s == "</folder>") app->goToParentFolder();

        QApplication::processEvents();
	}
	f.close();

    foreach (MdiSubWindow *w, app->tableList())
    {
        auto *ts = qobject_cast<TableStatistics *>(w);
        if (ts)
            ts->setBase(app->table(ts->baseName(), true));
    }

	if (progress.wasCanceled()){
		app->saved = true;
		app->close();
		return 0;
	}

	//process the rest
    (void)f.open(QIODevice::ReadOnly);

	MultiLayer *plot=0;
	while (!t.atEnd() && !progress.wasCanceled())
    {
		s = t.readLine();
        if  (s.left(8) == "<folder>")
        {
			list = s.split("\t");
			if (app->current_folder && list.size() >= 2) app->current_folder = app->current_folder->findSubfolder(list[1]);
		}
        else if  (s == "<multiLayer>")
        {
            //process multilayers information
			title = titleBase + QString::number(++aux) + "/" + QString::number(widgets);
			progress.setLabelText(title);

			s = t.readLine();
			QStringList graph = s.split("\t");
			QString caption = graph[0];

            MdiSubWindow *wactive = nullptr;
            if (auto *w = app->current_folder->activeWindow(); w && w->status() == 2)
                wactive = w;

			plot = app->multilayerPlot(caption, 0,  graph[2].toInt(), graph[1].toInt());
			app->setListViewDate(caption, graph[3]);
			plot->setBirthDate(graph[3]);

            restoreWindowGeometry(app, plot, t.readLine(), wactive);
			plot->blockSignals(true);

			if (d_file_version > 71)
            {
				QStringList lst = t.readLine().split("\t");
				if (lst.size() > 1) plot->setWindowLabel(lst[1]);
				if (lst.size() > 2) plot->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
			}
			if (d_file_version > 83)
            {

                QStringList lst = t.readLine().split("\t", Qt::SkipEmptyParts);
				if (lst.size() >= 5) plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());

                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
				if (lst.size() >= 3) plot->setSpacing(lst[1].toInt(),lst[2].toInt());

                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
				if (lst.size() >= 3) plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());

                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
				if (lst.size() >= 3) plot->setAlignement(lst[1].toInt(),lst[2].toInt());
			}

            int keepAlignedFromFile = 0;
			while ( s != "</multiLayer>" )
            {
                //open layers
				s = t.readLine();
				if (s.contains("<waterfall>"))
                {
					QStringList lst = s.trimmed().remove("<waterfall>").remove("</waterfall>").split(",");
					Graph *ag = plot->activeLayer();
					if (ag && lst.size() >= 2)
                    {
						ag->setWaterfallOffset(lst[0].toInt(), lst[1].toInt());
						if (lst.size() >= 3) ag->setWaterfallSideLines(lst[2].toInt());
					}
					plot->setWaterfallLayout();
				}

				if (s.left(7) == "<graph>")
                {
					list.clear();
					while ( s != "</graph>" )
                    {
						s = t.readLine();
						list<<s;
					}
					openGraph(app, plot, list);
				}

				if (s.contains("<LinkXAxes>"))
					plot->linkXLayerAxes(s.trimmed().remove("<LinkXAxes>").remove("</LinkXAxes>").toInt());
				else if (s.contains("<AlignPolicy>"))
					plot->setAlignPolicy((MultiLayer::AlignPolicy)s.trimmed().remove("<AlignPolicy>").remove("</AlignPolicy>").toInt());
				else if (s.contains("<CommonAxes>"))
					plot->setCommonAxesLayout(s.trimmed().remove("<CommonAxes>").remove("</CommonAxes>").toInt());
				else if (s.contains("<ScaleLayers>"))
					plot->setScaleLayersOnResize(s.trimmed().remove("<ScaleLayers>").remove("</ScaleLayers>").toInt());
                else if (s.contains("<KeepAligned>"))
                    keepAlignedFromFile = s.trimmed().remove("<KeepAligned>").remove("</KeepAligned>").toInt();
			}
            if (keepAlignedFromFile)
                plot->keepAligned(true);
			if (plot->status() == MdiSubWindow::Minimized) plot->showMinimized();
			plot->blockSignals(false);

            progress.setValue(aux - 1);
		}
        else if  (s == "<SurfacePlot>")
        {
            //process 3D plots information
			list.clear();
			title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			while ( s != "</SurfacePlot>" ){
				s = t.readLine();
				list << s;
			}

            MdiSubWindow *w = Graph3D::restore(app, list, d_file_version);
            if (w->status() == MdiSubWindow::Minimized)
                w->showMinimized();

            progress.setValue(aux - 1);
		}
        else if (s == "</folder>") app->goToParentFolder();
		else if (s == "<log>")
        {
            //process analysis information
			s = t.readLine();
			QString log = s + "\n";
			while(!s.contains("</log>"))
            {
				s = t.readLine();
				log += s + "\n";
			}
			if (app->current_folder) app->current_folder->appendLogInfo(log.remove("</log>"));
		}

        QApplication::processEvents();
	}
	f.close();

    if (progress.wasCanceled())
    {
		app->saved = true;
		app->close();
		return 0;
	}

	QFileInfo fi2(f);
	QString fileName = fi2.absoluteFilePath();

	app->updateRecentProjectsList(fileName);
	app->folders->setCurrentItem(cf->folderListItem());
	app->folders->blockSignals (false);
	app->changeFolder(cf, true);//change folder to user defined current folder
	app->blockSignals (false);
	app->renamedTables.clear();
	app->executeNotes();
	app->d_workspace->blockSignals(false);
	app->addWindowsListToCompleter();
	app->restoreApplicationGeometry();
	app->d_opening_file = false;
	app->savedProject();
    app->changeFolder(cf, true);
    QApplication::processEvents(); // allow setMaximized() to settle before show()
    app->show();

    // LegendWidget placeholders only resolve/resize in paintEvent(); force that catch-up now that restore is complete.
    foreach (MdiSubWindow *w, app->multilayerList())
    {
        auto *ml = qobject_cast<MultiLayer *>(w);
        if (!ml)
            continue;
        foreach (Graph *g, ml->layersList())
        {
            // Deferred catch-up: updateMarkersBoundingRect() skips its work while d_opening_file was set.
            g->updateMarkersBoundingRect();
            foreach (FrameWidget *fw, g->enrichmentsList())
            {
                auto *l = qobject_cast<LegendWidget *>(fw);
                if (l)
                    l->updateSize();
            }
        }
    }

	return app;
}

void ApplicationWindow::executeNotes()
{
	QList<MdiSubWindow *> lst = projectFolder()->windowsList();
	foreach(MdiSubWindow *widget, lst)
		if (QString(widget->metaObject()->className()) == "Note" && ((Note*)widget)->autoexec())
			((Note*)widget)->executeAll();
}


void ApplicationWindow::scriptError(const QString &message, const QString &scriptName, int lineNumber)
{
    Q_UNUSED(scriptName);
    Q_UNUSED(lineNumber);

    // During an AI run this comes from a script the AI script ran (a column or matrix formula): the AI script's own
    // errors use runScript()'s connection. Information for the model, not a failure of its script — a formula
    // failure that matters reaches the AI script as its own exception (e.g. recalculate()'s ValueError).
    if (m_aiCapturing && !message.trimmed().isEmpty())
        m_aiCaptureBuffer += message + "\n";

#ifdef SCRIPTING_CONSOLE
    auto appendAndScrollToHeader = [](QTextEdit *w, const QString &text) {
        ScriptEdit::ensureOneTrailingBlank(w);
        QTextCursor c = w->textCursor();
        c.movePosition(QTextCursor::End);
        const int anchor = c.position();
        w->append(text);
        c.setPosition(anchor > 0 ? anchor + 1 : 0);
        w->setTextCursor(c);
        w->ensureCursorVisible();
    };
    const QString errText = "▸▸▸ " + scriptEnv->objectName() + " script Error\n\n" + message + "\n";
    if (d_fitFromScriptWindow)
    {
        scriptWindow->showOutputPanel();
        appendAndScrollToHeader(scriptWindow->outputConsole(), errText);
    }
    else
    {
        consoleWindow->show();
        consoleWindow->raise();
        if (explorerWindow && tabifiedDockWidgets(explorerWindow).contains(consoleWindow))
            tabifyDockWidget(explorerWindow, consoleWindow);
        else if (logWindow && tabifiedDockWidgets(logWindow).contains(consoleWindow))
            tabifyDockWidget(logWindow, consoleWindow);
        appendAndScrollToHeader(console, errText);
    }
#else
    QMessageBox::critical(this, tr("QtiSAS") + " - " + tr("Script Error"), message);
#endif
}

#ifdef SCRIPTING_PYTHON
void ApplicationWindow::askAiConfigure(const QString &apiKey, const QString &baseUrl, const QString &model,
                                       int maxTokens, AiAssistant::DocsMode docsMode)
{
    if (!m_ai)
    {
        m_ai = new AiAssistant(this);
        m_ai->setHost(this);
        m_ai->setContentProvider(&m_aiContent);
    }
    m_ai->configure(apiKey, baseUrl, model, maxTokens, docsMode);
}

bool ApplicationWindow::addRule(const QString &text, QString *errorOut) const
{
    return AiAssistant::addUserRule(text, aiSettings.maxUserRules, errorOut);
}

bool ApplicationWindow::removeRule(int index, QString *errorOut)
{
    return AiAssistant::removeUserRule(index, errorOut);
}

bool ApplicationWindow::removeAllRules()
{
    return AiAssistant::clearUserRules();
}

QStringList ApplicationWindow::showRules()
{
    const QStringList rules = AiAssistant::userRules();
    if (rules.isEmpty())
        scriptPrint("No user rules defined.");
    else
        for (int i = 0; i < rules.size(); ++i)
            scriptPrint(QString::number(i + 1) + ". " + rules[i]);
    return rules;
}

// LIST-mode status line? Used only for spacing decisions below — the actual coloring is owned
// entirely by PythonSyntaxHighlighter's own matching check, same as it owns "▸▸▸ " lines.
static bool isAiListMessage(const QString &message)
{
    return message.startsWith("▸    AI request: LIST") || message.contains("▸▸  AI LIST:");
}

// AiScriptHost implementation — part of the eventual split of AI orchestration logic into a
// standalone library. print() is called directly by askAi()'s responseReady lambda for plain-text
// answers, and internally by runScript() below for the two output channels that don't already
// auto-display themselves (see runScript()'s doc comment).
void ApplicationWindow::print(const QString &message)
{
    m_aiTranscript << message;
    // Invalidated on every print() call — see printHttpRetryStatus()'s doc comment.
    // Anything printed between retries means the last line isn't ours, so the next one starts fresh, never overwrites.
    m_retryStatusPos = -1;
    m_retryStatusWidget = nullptr;
    const bool isHeader = message.startsWith("▸▸▸ ") || message.startsWith("▸▸ ") || message.startsWith("▸ ");
    // A LIST item line wants a blank before it EXCEPT right after "▸ AI request: LIST" itself
    // (item 1) — passing that as headerPrefix here reuses ensureOneTrailingBlank()'s own
    // "skip if last line already starts with this" check for exactly that one case.
    const QString blankSkipPrefix = isAiListMessage(message) ? "▸    AI request: LIST" : "▸▸▸ ";
    if (!m_aiForceConsole && scriptWindow && scriptWindow->isVisible())
    {
        scriptWindow->showOutputPanel();
        QTextEdit *out = scriptWindow->outputConsole();
        if (isHeader)
            ScriptEdit::ensureOneTrailingBlank(out, blankSkipPrefix);
        QTextCursor c = out->textCursor();
        c.movePosition(QTextCursor::End);
        const int anchor = c.position();
        out->append(message);
        if (isHeader)
        {
            c.setPosition(anchor > 0 ? anchor + 1 : 0);
            out->setTextCursor(c);
            out->ensureCursorVisible();
        }
        return;
    }
#ifdef SCRIPTING_CONSOLE
    {
        QTextEdit *sc = scriptingConsole();
        int anchor = 0;
        if (sc)
        {
            QTextCursor c = sc->textCursor();
            c.movePosition(QTextCursor::End);
            anchor = c.position();
        }
        if (isHeader && sc)
            ScriptEdit::ensureOneTrailingBlank(sc, blankSkipPrefix);
        scriptPrint(message);
        if (isHeader && sc)
        {
            QTextCursor c = sc->textCursor();
            c.setPosition(anchor > 0 ? anchor + 1 : 0);
            sc->setTextCursor(c);
            sc->ensureCursorVisible();
        }
    }
#else
    scriptPrint(message);
#endif
}

// Same "which panel" selection as print(), for retry-backoff: first attempt appends, later ones replace in place.
// So N backoff attempts show as one updating line, not N. Falls back to print() with no QTextEdit target to overwrite.
void ApplicationWindow::printHttpRetryStatus(const QString &message, bool isFirstAttempt)
{
    const QString full = message.startsWith("▸▸▸ ") ? message : "▸▸▸ " + message;
    QTextEdit *target = nullptr;
#ifdef SCRIPTING_PYTHON
    if (!m_aiForceConsole && scriptWindow && scriptWindow->isVisible())
    {
        scriptWindow->showOutputPanel();
        target = scriptWindow->outputConsole();
    }
#endif
#ifdef SCRIPTING_CONSOLE
    if (!target)
        target = scriptingConsole();
#endif
    if (!target)
    {
        print(full);
        return;
    }
    m_aiTranscript << full; // see aiTranscript(): a retry is part of the run's story (print() above records its own)
    if (!isFirstAttempt && target == m_retryStatusWidget && m_retryStatusPos >= 0)
    {
        QTextCursor c(target->document());
        c.setPosition(m_retryStatusPos);
        c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        c.insertText(full);
        target->setTextCursor(c);
        target->ensureCursorVisible();
        return;
    }
    // Position recorded AFTER append() — its own break means an earlier position sits at the previous line's end.
    // Confirmed live: capturing early let the next overwrite delete that break too, merging onto the previous line.
    ScriptEdit::ensureOneTrailingBlank(target);
    target->append(full);
    QTextCursor c = target->textCursor();
    c.movePosition(QTextCursor::StartOfBlock);
    m_retryStatusPos = c.position();
    m_retryStatusWidget = target;
    target->ensureCursorVisible();
}

// Runs `code` under `scriptName`, capturing everything printed and any raised exception —
// including a SyntaxError, which is just run and captured like any other exception, matching
// how EXPLORE scripts have always behaved (no separate pre-check). Callers needing to decide
// "is this even worth attempting" (the action-script branch's "not runnable" case) must do that
// check themselves before calling this — see AiScriptHost.h's comment on AiScriptResult. Display
// happens live during execution here — the RETURNED
// AiScriptResult is a batched summary for the caller's own use (feeding the model, retry/dedup
// logic), not something to print again — see the note further down on why re-printing it double-
// displays scriptPrint() output.
//
// Uses the shared m_aiCaptureBuffer/m_aiErrorBuffer/m_aiCapturing members rather than fully-local
// variables — NOT a stylistic choice. AI-generated code overwhelmingly reports output via the
// scriptPrint() PYTHON FUNCTION (a QtiSAS API call: `sipCpp->app()->scriptPrint(...)`), which is
// a completely separate channel from Script::print (raw stdout/print()) — it calls straight into
// ApplicationWindow::scriptPrint()/scriptError(), which can only feed these member buffers (they
// are member-function slots, not lambdas, so they cannot write into a caller's local variables).
// Skipping this and capturing only Script::print/Script::error would silently drop nearly all AI
// output, since almost every AI script reports results via scriptPrint(), not bare print().
//
// IMPORTANT — callers must NOT re-print result.output/result.error after this returns. Confirmed
// live: an earlier version of this method left display entirely to the caller (batched, after
// the fact), and every scriptPrint() line came out DOUBLED — because ApplicationWindow::
// scriptPrint() already displays itself live, correctly routed to the Script Output Panel,
// whenever m_aiCapturing is true (see its own body) — it is not a passive buffer-filler. Only
// two channels have NO auto-display of their own and are printed live here instead: raw
// Script::print (bare stdout/print(), rarely used by AI code) and Script::error (auto-displayed
// only via scriptEnv's global relay to ApplicationWindow::scriptError(), which lands AI runs in
// the wrong panel since d_fitFromScriptWindow is never true for them).
AiScriptResult ApplicationWindow::runScript(const QString &code, const QString &scriptName)
{
    if (console && console->isVisible())
        console->viewport()->repaint();
    if (scriptWindow && scriptWindow->isVisible() && scriptWindow->outputConsole())
        scriptWindow->outputConsole()->viewport()->repaint();
    AiScriptResult result;
    m_aiErrorBuffer.clear();
    m_aiCaptureBuffer.clear();
    m_aiCapturing = true;
    Script *s = scriptEnv->newScript(code, this, scriptName);
    connect(s, &Script::error, this, [this](const QString &message) {
        if (m_aiCapturing && !message.trimmed().isEmpty())
        {
            m_aiCaptureBuffer += message + "\n";
            m_aiErrorBuffer += message + "\n";
        }
        print("▸▸▸ " + scriptEnv->objectName() + " script Error\n\n" + message + "\n");
    });
    connect(s, &Script::print, this, [this](const QString &msg) {
        m_aiCaptureBuffer += msg + "\n";
        print(msg);
    });
    {
#ifdef SCRIPTING_PYTHON
        // AI scripts run in the main thread with no limit of their own: confirmed live, "while n.tabs() > 0:
        // n.removeTab(0)" kept QtiSAS at 100% CPU for an hour. A real fit or compile finishes far below this
        std::unique_ptr<PythonTimeLimit> limit;
        if (qobject_cast<PythonScripting *>(scriptEnv))
            limit = std::make_unique<PythonTimeLimit>(
                300, "the script ran longer than 300 s and was stopped — an endless loop? Check every while loop: "
                     "does its condition ever change (e.g. removeTab() keeps a note's last tab)?");
#endif
        s->exec();
    }
    delete s;
    m_aiCapturing = false;
    result.output = m_aiCaptureBuffer.trimmed();
    result.error = m_aiErrorBuffer.trimmed();
    return result;
}

// Answers the AI's "does this name already exist, and as what?" question straight from live state
// — see AiScriptHost.h's entityKinds() comment for why this is not a generated script. Labels match
// the ones AiAssistant's own existence-check parsing already uses ("table"/"matrix"/"note"/"graph"/
// "3D plot"/"compile"/"fittable"), so both paths speak one vocabulary.
QList<QStringList> ApplicationWindow::entityKinds(const QStringList &names)
{
    // Snapshot each registry ONCE, not per name: groupFunctions() rescans and re-reads every .fif
    // file on disk, so asking it per name would multiply that by the number of names checked.
    QHash<QString, QString> windowKinds, moduleKinds;
    const QList<MdiSubWindow *> windows = windowsList();
    for (MdiSubWindow *w : windows)
    {
        if (!w)
            continue;
        QString kind = "window";
        if (w->inherits("Table"))
            kind = "table";
        else if (w->inherits("Matrix"))
            kind = "matrix";
        else if (w->inherits("Note"))
            kind = "note";
        else if (w->inherits("MultiLayer"))
            kind = "graph";
        else if (w->inherits("Graph3D"))
            kind = "3D plot";
        windowKinds.insert(w->objectName(), kind);
        // a table a module made is also that module's object: its kind routes a request naming it to the module's
        // docs, and a script must not replace it with newTable()
        static const QHash<QString, QString> moduleTables{{"NSE Headers", "JNSE header table"},
                                                          {"NSE Dataset", "JNSE dataset table"},
                                                          {"SVD Table", "SVD table"},
                                                          {"DAN::Script::Table", "DAN script table"},
                                                          {"DAN::Merging::Template", "DAN merging template"},
                                                          {"DAN::Settings::Table", "DAN settings table"},
                                                          {"Info::Extractor", "DAN header table"}};
        if (kind == "table")
            if (const QString module = moduleTables.value(w->windowLabel()); !module.isEmpty())
                moduleKinds.insert(w->objectName(), module);
    }
    // groupFunctions("ALL") rather than functionNames(): the latter calls selectCategory(), moving
    // the Compile widget's own list selection — a visible UI side effect for what must be a purely
    // passive question. It reports whichever dimensionality the widget is currently set to (1D vs
    // 2D), so a name can be absent here and still exist as the other kind; a name reported present
    // is always genuinely present, which is the direction that matters for a collision warning.
    const QStringList compiled = compileWidget ? compileWidget->groupFunctions("ALL") : QStringList();
    const QStringList fitFuncs = fittableWidget ? fittableWidget->fitFunctions() : QStringList();

    QList<QStringList> result;
    for (const QString &name : names)
    {
        QStringList kinds;
        const auto it = windowKinds.constFind(name);
        if (it != windowKinds.constEnd())
            kinds << it.value();
        if (moduleKinds.contains(name))
            kinds << moduleKinds.value(name);
        if (compiled.contains(name))
            kinds << "compile";
        if (fitFuncs.contains(name))
            kinds << "fittable";
        result << kinds;
    }
    return result;
}

QString ApplicationWindow::tableShape(const QString &name)
{
    for (MdiSubWindow *w : windowsList())
    {
        auto *t = qobject_cast<Table *>(w);
        if (!t || t->objectName() != name)
            continue;
        static const QStringList roles = {"None", "X", "Y", "Z", "xErr", "yErr", "Label"};
        const QStringList labels = t->colNames();
        QStringList columns;
        for (int c = 0; c < t->numCols() && c < labels.size(); ++c)
        {
            const int role = t->colPlotDesignation(c);
            columns << QString("%1 \"%2\" %3")
                           .arg(c + 1)
                           .arg(labels[c], role >= 0 && role < roles.size() ? roles[role] : "?");
        }
        return QString("%1 rows; columns: %2").arg(t->numRows()).arg(columns.join(", "));
    }
    return {};
}

QString ApplicationWindow::formulaLanguage() const
{
    return scriptEnv ? QString(scriptEnv->objectName()) : QString();
}

// Windows by kind, and the fit functions of the "ai/" folder (a directory listing: cheap enough to run before and after
// every AI script, unlike groupFunctions(), which re-reads every .fif file).
QStringList ApplicationWindow::liveObjects()
{
    QStringList objects;
    for (MdiSubWindow *w : windowsList())
    {
        if (!w)
            continue;
        QString kind = "window";
        if (w->inherits("Table"))
            kind = "table";
        else if (w->inherits("Matrix"))
            kind = "matrix";
        else if (w->inherits("Note"))
            kind = "note";
        else if (w->inherits("MultiLayer"))
            kind = "graph";
        else if (w->inherits("Graph3D"))
            kind = "3D plot";
        objects << kind + ":" + w->objectName();
    }
    if (compileWidget)
        for (const QFileInfo &fif : QDir(compileWidget->pathFIF + "/ai").entryInfoList({"*.fif"}, QDir::Files))
            objects << "function:ai/" + fif.completeBaseName();
    return objects;
}

QStringList ApplicationWindow::moduleStates()
{
    QStringList states;
    if (danWidget && danWidget->danOptions)
        if (const QString dan = danWidget->danOptions->state(); !dan.isEmpty())
            states << "DAN-SANS session (Dan): " + dan;
    return states;
}

QStringList ApplicationWindow::closeObjects(const QStringList &objects)
{
    QStringList closed;
    const QStringList live = liveObjects(); // the same "kind:name" spelling, so the kind must match too
    for (const QString &object : objects)
    {
        const QString kind = object.section(':', 0, 0);
        const QString name = object.section(':', 1);
        if (kind == "function" || !live.contains(object))
            continue;
        for (MdiSubWindow *w : windowsList())
            if (w && w->objectName() == name)
            {
                closeWindow(w, true, false);
                closed << object;
                break;
            }
    }
    return closed;
}

// Checks syntax only — WITHOUT running `code` — so the caller can decide "not runnable" (plain
// English instead of code) vs. "worth running/self-reviewing" before ever touching scriptEnv. A
// separate AiScriptHost method specifically so the orchestration logic that decides what to do
// with the result (in AiAssistant) never needs the Python C API directly — see AiScriptHost.h's
// comment on canCompile().
bool ApplicationWindow::canCompile(const QString &code, QString *errorOut) const
{
    bool compilable = false;
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *obj = Py_CompileString(code.toUtf8().constData(), "<ai>", Py_file_input);
    if (obj)
    {
        Py_DECREF(obj);
        compilable = true;
    }
    else
    {
        // PyErr_Fetch() already clears the error indicator as a side effect — the explicit
        // PyErr_Clear() the old code relied on for that is redundant once this runs, but kept
        // below (as a harmless no-op) so the "always clear on failure" invariant stays obvious
        // even if this errorOut-capturing block is ever removed.
        if (errorOut)
        {
            PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
            PyErr_Fetch(&type, &value, &traceback);
            PyErr_NormalizeException(&type, &value, &traceback);
            if (value)
            {
                PyObject *str = PyObject_Str(value);
                if (str)
                {
                    *errorOut = QString::fromUtf8(PyUnicode_AsUTF8(str));
                    Py_DECREF(str);
                }
            }
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
        }
        PyErr_Clear();
    }
    PyGILState_Release(gstate);
    return compilable;
}

// Lazy-constructs m_ai (with the last-selected preset's API key/base URL/model/max tokens/docs
// mode loaded, if any) — a no-op if askAiConfigure() already pre-created it, or if this isn't the
// first askAi() call in the session.
QStringList ApplicationWindow::aiAgents() const
{
    QStringList names;
    for (const QFileInfo &fi : QDir(sasPath + "ai/").entryInfoList({"*.ai"}, QDir::Files))
        names << fi.completeBaseName();
    // by the names, not the file names: "X-pyi.ai" sorts before "X.ai" ('-' < '.')
    names.sort();
    return names;
}

QString ApplicationWindow::aiAgent()
{
    QSettings *s = Settings::DefaultSettings();
    const QString name = s->value("/AI/SelectedPreset").toString();
    delete s;
    return name;
}

bool ApplicationWindow::setAiAgent(const QString &name)
{
    if (!aiAgents().contains(name))
        return false;
    // as AiConfig::apply(): the preset's fields into the live assistant, its name remembered for the next start
    QSettings sf(sasPath + "ai/" + name + ".ai", QSettings::IniFormat);
    askAiConfigure(sf.value("AI/ApiKey").toString(), sf.value("AI/BaseUrl").toString(),
                   sf.value("AI/Model", "claude-opus-4-8").toString(), sf.value("AI/MaxTokens", 4096).toInt(),
                   AiAssistant::docsModeFromSettings(sf));
    m_ai->setBigModel(AiAssistant::bigModelFromSettings(sf));
    QSettings *s = Settings::DefaultSettings();
    s->setValue("/AI/SelectedPreset", name);
    delete s;
    return true;
}

void ApplicationWindow::aiClearHistory()
{
    if (m_ai)
        m_ai->clearHistory();
}

bool ApplicationWindow::aiIdle() const
{
    return !m_ai || m_ai->isIdle();
}

void ApplicationWindow::aiStop()
{
    if (m_ai)
        m_ai->stop();
}

void ApplicationWindow::ensureAiAssistant()
{
    if (m_ai)
        return;
    m_ai = new AiAssistant(this);
    m_ai->setHost(this);
    m_ai->setContentProvider(&m_aiContent);
    QSettings *s = Settings::DefaultSettings();
    const QString presetName = s->value("/AI/SelectedPreset").toString();
    delete s;
    if (!presetName.isEmpty())
    {
        QSettings sf(sasPath + "ai/" + presetName + ".ai", QSettings::IniFormat);
        m_ai->configure(sf.value("AI/ApiKey").toString(), sf.value("AI/BaseUrl").toString(),
                        sf.value("AI/Model", "claude-opus-4-8").toString(), sf.value("AI/MaxTokens", 4096).toInt(),
                        AiAssistant::docsModeFromSettings(sf));
        m_ai->setBigModel(AiAssistant::bigModelFromSettings(sf));
    }
}

// True if every code line is just scriptPrint()/print() — a MESSAGE wrapped as valid Python, not real work.
// canCompile() can't tell these apart: a real action script and a bare scriptPrint() greeting are equally valid Python.
// Textual heuristic, not a parse — multi-line scriptPrint() falls through as real code, only costs an eager highlight.
static bool isInformationalOnlyScript(const QString &code)
{
    static const QRegularExpression printCallRe(R"(^\s*(scriptPrint|print)\s*\(.*\)\s*$)");
    const auto lines = code.split('\n');
    bool sawStatement = false;
    for (const QString &line : lines)
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith("import "))
            continue;
        if (!printCallRe.match(trimmed).hasMatch())
            return false;
        sawStatement = true;
    }
    return sawStatement;
}

// Extracts an informational script's plain text (see isInformationalOnlyScript()) to show as a normal chat reply.
// Conservative: only a bare string literal, no f/r/b prefix, no concatenation — an f-string embeds a variable.
// Anything unmatched returns empty: "show raw code instead." Multiple scriptPrint()/print() lines join with newlines.
static QString tryExtractInformationalMessage(const QString &code)
{
    static const QRegularExpression literalCallRe(
        R"(^\s*(?:scriptPrint|print)\s*\(\s*(['"])((?:[^\\]|\\.)*?)\1\s*\)\s*$)");
    QStringList messages;
    for (const QString &line : code.split('\n'))
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith("import "))
            continue;
        const QRegularExpressionMatch m = literalCallRe.match(trimmed);
        if (!m.hasMatch())
            return {};
        QString text = m.captured(2);
        text.replace("\\n", "\n").replace("\\t", "\t").replace("\\\"", "\"").replace("\\'", "'").replace("\\\\", "\\");
        messages << text;
    }
    return messages.join('\n');
}

// Wires m_ai's responseReady/errorOccurred/infoMessage signals — guarded by m_aiConnected so a
// re-entrant askAi() (or one following askAiConfigure()'s own pre-creation of m_ai) never
// double-connects.
void ApplicationWindow::connectAiSignalsOnce()
{
    if (m_aiConnected)
        return;
    m_aiConnected = true;
    connect(m_ai, &AiAssistant::chatResponseReady, this,
            [this](const QString &text) { print("▸▸▸ AI chat:\n\n" + text); });
    connect(m_ai, &AiAssistant::responseReady, this, [this](const QString &code) {
        // "Existing X" verification script round-trip — see AiAssistant::beginRequest()'s doc
        // comment. Checked before EVERYTHING else below, including the continuation-check — this
        // is the earliest gate a request passes through.
        if (m_ai->awaitingExistenceCheckScript)
        {
            m_ai->handleExistenceCheckScriptResponse(code);
            return;
        }
        // Split-plan creation-name collision-check round-trip — see
        // AiAssistant::handleSplitPlanResponse()'s doc comment. Same early priority as the
        // "existing X" verification check above (it's the same kind of bare check-script
        // round-trip), checked right after it.
        if (m_ai->awaitingSplitPlanCreationCheckScript)
        {
            m_ai->handleSplitPlanCreationCheckScriptResponse(code);
            return;
        }
        // New-task-vs-continuation round-trip — see AiAssistant::handleContinuationCheckResponse()'s
        // doc comment. Checked before everything else below: its own answer must never be
        // misinterpreted as a doc-selection/split-plan response.
        if (m_ai->awaitingContinuationCheck)
        {
            m_ai->handleContinuationCheckResponse(code);
            return;
        }
        // Split-mode decomposition/merge round-trips — see AiAssistant::handleSplitPlanResponse()/
        // handleSplitMergeResponse()'s doc comments.
        if (m_ai->awaitingSplitPlan)
        {
            m_ai->handleSplitPlanResponse(code);
            return;
        }
        if (m_ai->awaitingSplitMerge)
        {
            m_ai->handleSplitMergeResponse(code);
            return;
        }

        // EXPLORE-round handling checked outside autoRun (read-only, always safe) — confirmed live, previously inside
        // autoRun only, a non-auto-run script dead-ended as "AI answer:" with no error. Marker matches as a FIRST-LINE
        // PREFIX, not exact — a strict check missed a same-line description and stalled as "not runnable".
        static const QRegularExpression exploreMarkerRe(R"(^#\s*EXPLORE\b)");
        const int firstLineEnd = static_cast<int>(code.indexOf('\n'));
        const QString firstLine = (firstLineEnd < 0 ? code : code.left(firstLineEnd));
        if (exploreMarkerRe.match(firstLine).hasMatch())
        {
            m_ai->handleExploreResponse(code);
            return;
        }
        if (m_ai->autoRun)
        {
            // Compilable-check + self-review dispatch — see maybeDispatchSelfReview(). Mutable copy: the call may strip
            // a leading commentary paragraph — execution below must use the corrected version, not the original `code`.
            QString correctedCode = code;
            bool compilable = false;
            QString syntaxError;
            if (m_ai->maybeDispatchSelfReview(correctedCode, compilable, syntaxError))
                return;

            // Action-script running + error-retry dispatch — see
            // AiAssistant::runActionScript()'s doc comment.
            m_ai->runActionScript(correctedCode, compilable, syntaxError);
        }
        else
        {
            // "AI answer:" tells the highlighter to skip Python until the next header — but this branch also shows
            // not-yet-run scripts (confirmed live, unhighlighted). Route through a distinct header like "AI code
            // (running)" — canCompile() can't tell scriptPrint() prose from real code, see isInformationalOnlyScript()
            const bool isRealActionScript = canCompile(code) && !isInformationalOnlyScript(code);
            if (isRealActionScript)
            {
                // Lets a follow-up "run it"/"execute that" run this exact script directly instead
                // of going back through the model — see askAi()'s own "run it" interception.
                m_ai->markLastScriptUnrun();
                print("▸▸▸ AI code (not run):\n\n" + code);
                // Reported, never retried — see AiAssistant::generatedScriptWarning()'s doc
                // comment. Printed AFTER the script so it reads as a note on what you just saw.
                const QString warning = m_ai->generatedScriptWarning(code);
                if (!warning.isEmpty())
                    print("▸▸▸ AI: not retried (generate-only): " + warning);
            }
            else
            {
                // Show it like a normal chat reply when safely extractable (see tryExtractInformationalMessage()) —
                // scriptPrint(...) is plumbing for "must be valid Python", not something the user asked to see.
                const QString extracted = tryExtractInformationalMessage(code);
                print("▸▸▸ AI answer:\n\n" + (extracted.isEmpty() ? code : extracted));
            }
            // A LIST in generate-only mode still has to advance. advanceListItem() is normally
            // reached from runActionScript()'s post-execution hook, which never runs here — so
            // without this the list generated item 1, printed it, and silently stopped, leaving
            // items 2..N undispatched AND listModeActive stuck true. That stale flag is the live
            // cross-request bug the cancel path already guards against: the next unrelated
            // auto-run request would reach the same hook, see listModeActive, and dispatch item 2
            // of an abandoned list as if it were that request's own continuation.
            //
            // Deliberately AFTER printing this item's script, and independent of
            // isRealActionScript: a plain-text/informational answer is still that item's answer,
            // and the list must move on either way rather than stalling on one non-script reply.
            if (m_ai->listModeActive)
                m_ai->advanceListItem();
        }
    });
    connect(m_ai, &AiAssistant::errorOccurred, this,
            [this](const QString &err) { print(err.startsWith("▸▸▸") ? err : "▸▸▸ " + err); });
    connect(m_ai, &AiAssistant::infoMessage, this,
            [this](const QString &msg) { print(msg.startsWith("▸▸▸") ? msg : "▸▸▸ " + msg); });
    connect(m_ai, &AiAssistant::httpRetryStatus, this,
            [this](const QString &msg, bool isFirstAttempt) { printHttpRetryStatus(msg, isFirstAttempt); });
}

// Echoes the request's first line to wherever the user is watching (Script Window output panel or
// the Scripting Console), mirroring print()'s own "which panel" logic since this happens before
// any AI call — and thus before print() itself would ever be invoked for this request.
void ApplicationWindow::echoAiRequest(const QString &effectiveText)
{
    // A LIST request (see AiAssistant::parseListRequest()) gets its own, single-arrow prefix and
    // scriptOutputColor green — visually distinct from a normal "▸▸▸ AI request: ..." line, and
    // consistent with the "▸▸ AI LIST: ..." status lines beginListRequest()/startNextListItem()
    // print as it runs — coloring for both is owned entirely by PythonSyntaxHighlighter, not here
    // (see its own matching check). Reuses parseListRequest() itself
    // rather than re-deriving the same check, so this echo and the actual dispatch decision in
    // askAi() can never disagree about what counts as a LIST request.
    const bool isList = AiAssistant::parseListRequest(effectiveText).isList;
    const QString headerText = (isList ? "▸    AI request: " : "▸▸▸ AI request: ") + effectiveText.split("\n").first();
    const QColor normalFg = qApp->palette().color(QPalette::Text);
    if (!m_aiForceConsole && scriptWindow && scriptWindow->isVisible())
    {
        scriptWindow->showOutputPanel();
        QTextEdit *out = scriptWindow->outputConsole();
        ScriptEdit::ensureOneTrailingBlank(out);
        // append() inherits whatever format the document's LAST character happens to have — if
        // the previous line was scriptPrintHighlighted()'s own green script-output color, append()
        // can silently continue in that same color instead of the normal default, confirmed live
        // (this exact header line rendered green right after a script's colored output). Force an
        // explicit, definite foreground instead of trusting append()'s inherited-format behavior.
        QTextCursor cursor(out->document());
        cursor.movePosition(QTextCursor::End);
        QTextCharFormat fmt;
        fmt.setForeground(normalFg);
        // Newline+text in ONE insertText(), not insertBlock() then insertText() — see the matching
        // comment in scriptPrintHighlighted()/scriptPrint(): the latter let
        // PythonSyntaxHighlighter's isScriptOutput probe fire on the still-EMPTY new block first,
        // leaving Python highlighting on top of what should be plain text.
        cursor.insertText((out->document()->isEmpty() ? QString() : QString("\n")) + headerText, fmt);
        out->setTextCursor(cursor);
        out->setCurrentCharFormat(fmt);
        out->ensureCursorVisible();
    }
    else
    {
#ifdef SCRIPTING_CONSOLE
        QTextEdit *sc = scriptingConsole();
        if (sc)
            ScriptEdit::ensureOneTrailingBlank(sc);
        scriptPrint(headerText);
#else
        scriptPrint(headerText);
#endif
    }
}

// Plain-text commands answered locally, without an API call ("stop"/"history"/"model"/"url"/
// "status"/"clear history" and their aliases). Returns true if `trimmed` matched one (askAi()
// must return right after), false to continue on to a real request.
bool ApplicationWindow::handleAiTextCommand(const QString &trimmed)
{
    if (trimmed == "stop" || trimmed == "ai stop" || trimmed == "cancel" || trimmed == "ai cancel")
    {
        // Checked here, before ask()'s own m_busy guard, specifically so this works WHILE a
        // request is in flight — ask() itself refuses to run at all while busy ("previous request
        // still in progress"), which would otherwise make it impossible to type this mid-request.
        m_ai->stop();
        // Every pending round-trip, not a hand-picked few: a reply arriving after this cancel is
        // routed by these flags alone, and one left set sends it into a handler whose state has
        // just been cleared below. Confirmed live as a SIGSEGV — see resetPendingRoundTrips().
        m_ai->resetPendingRoundTrips();
        // Not covered by that reset (it waits for the user, not the network) but a cancel does
        // drop the pending question, so clear it here.
        m_ai->awaitingContinuationClarification = false;
        m_ai->autoContinueCount = 0;
        m_ai->splitModeActive = false;
        m_ai->splitPlan.clear();
        m_ai->splitPartIndex = -1;
        m_ai->splitPartScripts.clear();
        // Same reasoning as the split-mode fields above — without this, a stale listModeActive
        // left over from a stopped LIST could make a later, completely unrelated successful
        // request get mistaken for "the next list item completing" by runActionScript()'s
        // completion hook.
        m_ai->listModeActive = false;
        m_ai->listItems.clear();
        m_ai->listItemIndex = -1;
        m_ai->clearUserCorrections();
        print("▸▸▸ AI: request cancelled.");
        return true;
    }
    if (trimmed == "history" || trimmed == "history list" || trimmed == "list history" || trimmed == "show history" ||
        trimmed == "show request history" || trimmed == "request history")
    {
        print("▸▸▸ AI answer:\n\n" + m_ai->historyText());
        return true;
    }
    if (trimmed == "ai model" || trimmed == "model")
    {
        print("▸▸▸ AI answer:\n\n" + (m_ai->model().isEmpty() ? "(not set)" : m_ai->model()));
        return true;
    }
    if (trimmed == "ai url" || trimmed == "url")
    {
        print("▸▸▸ AI answer:\n\n" + (m_ai->baseUrl().isEmpty() ? "(not set)" : m_ai->baseUrl()));
        return true;
    }
    if (trimmed == "ai status" || trimmed == "status")
    {
        QString docs;
        switch (m_ai->docsMode())
        {
        case AiAssistant::DocsMode::Full:
            docs = "full .md docs";
            break;
        case AiAssistant::DocsMode::Integrated:
            docs = "integrated (model has API knowledge — no docs sent)";
            break;
        case AiAssistant::DocsMode::Compact:
        default:
            docs = "compact .pyi stubs";
            break;
        }
        const QString status = "Model:    " + (m_ai->model().isEmpty() ? "(not set)" : m_ai->model()) +
                               "\n"
                               "URL:      " +
                               (m_ai->baseUrl().isEmpty() ? "(not set)" : m_ai->baseUrl()) +
                               "\n"
                               "Docs:     " +
                               docs +
                               "\n"
                               "History:  " +
                               QString::number(m_ai->historySize()) +
                               " turns\n"
                               "Max resp: " +
                               QString::number(m_ai->maxTokens()) + " tokens";
        print("▸▸▸ AI answer:\n\n" + status);
        return true;
    }
    if (trimmed == "clear history" || trimmed == "clean history" || trimmed == "reset history")
    {
        m_ai->clearHistory();
        m_ai->originalRequestText.clear();
        print("▸▸▸ AI: conversation history cleared.");
        return true;
    }
    return false;
}

bool ApplicationWindow::askAiChat(const QString &text, bool forceConsole)
{
    if (scriptEnv->objectName() != "Python")
    {
        d_status_info->setText(tr("AI requires Python. Select /python first."));
        return false;
    }
    const QString command = text.trimmed();
    if (command.isEmpty())
        return false;
    ensureAiAssistant();
    connectAiSignalsOnce();
    if (command.compare("stop", Qt::CaseInsensitive) == 0 || command.compare("cancel", Qt::CaseInsensitive) == 0)
        m_ai->stop();
    else if (!m_ai->isIdle())
    {
        d_status_info->setText(tr("Finish or stop the current AI request before chatting."));
        return false;
    }
    else
    {
        m_aiForceConsole = forceConsole;
        m_ai->setRequestTimeoutSec(aiSettings.requestTimeoutSec);
        m_ai->setDocTiers(aiSettings.docTiers);
        if (command.compare("clear history", Qt::CaseInsensitive) == 0)
            m_ai->clearChatHistory();
        else
        {
            print("▸▸▸ AI chat request:\n\n" + command);
            m_ai->askChat(command);
        }
    }
    return true;
}

void ApplicationWindow::askAi(const QString &text, bool autoRun, bool forceConsole)
{
    if (scriptEnv->objectName() != "Python")
    {
        d_status_info->setText(tr("AI requires Python. Select /python first."));
        return;
    }
    // Creates m_ai on first use (no-op if it already exists)
    ensureAiAssistant();

    if (m_ai->isChatPending())
    {
        const QString command = text.trimmed().toLower();
        if (command == "stop" || command == "cancel")
            m_ai->stop();
        else
            d_status_info->setText(tr("Finish or stop the chat request before using script AI."));
        return;
    }

    // Answers the pending "new task or continuation?" question
    // Reuses handleContinuationCheckResponse()'s own dispatch logic.
    if (m_ai->awaitingContinuationClarification)
    {
        m_ai->awaitingContinuationClarification = false;
        m_ai->handleContinuationCheckResponse(AiAssistant::looksLikeNewTaskReply(text) ? "NEW" : "CONTINUE");
        return;
    }

    // Guards the run-it/run-it-again/run-history fast paths against firing while busy — confirmed live without
    // this, one ran concurrently with an in-flight error-retry and interleaved garbled output.
    if (m_ai->isBusy() && (AiAssistant::looksLikeRunItRequest(text) || AiAssistant::looksLikeRunItAgainRequest(text) ||
                           AiAssistant::looksLikeRunHistoryIndexRequest(text, nullptr)))
    {
        echoAiRequest(text);
        print("▸▸▸ AI: previous request still in progress.");
        return;
    }

    // A request is running: what the user types now corrects it, from its next model call on, or at once with a
    // leading "!" — see AiAssistant::addUserCorrection(). Local commands ("stop", "status", ...) still work.
    if (!m_ai->isIdle())
    {
        if (handleAiTextCommand(text.trimmed().toLower()))
            return;
        bool now = false;
        const QString correction = AiAssistant::parseCorrection(text, &now);
        if (correction.isEmpty())
            return;
        print("▸▸▸ AI correction: " + correction);
        const QString scope =
            m_ai->listModeActive ? QString("LIST item %1").arg(m_ai->listItemIndex + 1) : QString("this request");
        print(m_ai->addUserCorrection(correction, now)
                  ? "▸▸▸ AI: correction noted for " + scope + " — the current model call starts again with it."
                  : "▸▸▸ AI: correction noted for " + scope + " — applied from its next step.");
        return;
    }

    // "run it" for the just-shown script — handled directly (confirmed live: re-asking the model hallucinated an
    // unrelated answer). Must run before anything else appends to m_history this turn.
    if (m_ai->hasUnrunLastScript() && AiAssistant::looksLikeRunItRequest(text))
    {
        // echoAiRequest() needed here or the request never appears in the log (confirmed live). The confirmation
        // message prints inside runLastScript() instead, since it depends on the mistake-check running first.
        echoAiRequest(text);
        m_ai->runLastScript("▸▸▸ AI: running the previously shown script directly (no re-generation needed).");
        return;
    }

    // "run it again" wants a repeat regardless of whether the last script already ran,
    // so it only needs a last script to exist, not hasUnrunLastScript() specifically.
    if (m_ai->historySize() > 0 && AiAssistant::looksLikeRunItAgainRequest(text))
    {
        echoAiRequest(text);
        m_ai->runLastScript("▸▸▸ AI: running the previous script again directly (no re-generation needed).");
        return;
    }

    // "run Q4"/"run A4" — an explicit history index, valid even before the context boundary
    // (see m_contextHistoryStart's doc comment).
    int historyIndex = 0;
    if (AiAssistant::looksLikeRunHistoryIndexRequest(text, &historyIndex))
    {
        echoAiRequest(text);
        // Success message (if any) is printed by runHistoryScript() itself, same timing reasoning
        // as runLastScript() above — only the "invalid index" case is decidable here up front.
        if (!m_ai->runHistoryScript(historyIndex))
            print(QString("▸▸▸ AI: no turn #%1 in history (%2 turn(s) recorded).")
                      .arg(historyIndex)
                      .arg(m_ai->historySize()));
        return;
    }

    m_ai->clearUserCorrections(); // a new request: the previous one's corrections are in history now
    m_ai->setMaxUserRules(aiSettings.maxUserRules);
    m_ai->setMaxAutoContinue(aiSettings.maxAutoContinue);
    m_ai->setMaxSplitParts(aiSettings.maxSplitParts);
    m_ai->setSplitMergeEnabled(aiSettings.splitRequestMerge);
    // BIGMODEL preset: split, self-review and EXPLORE are off whatever the global settings say;
    // the request's SPLIT / REVIEW / EXPLORE markers still switch a step back on.
    const bool bigModel = m_ai->bigModel();
    const bool splitDefault = aiSettings.splitByDefault && !bigModel;
    const bool reviewDefault = aiSettings.selfReviewEnabled && !bigModel;
    const bool exploreDefault = aiSettings.exploreEnabled && !bigModel;
    m_ai->setSelfReviewEnabled(reviewDefault);
    m_ai->setRequestTimeoutSec(aiSettings.requestTimeoutSec);
    m_ai->setDocTiers(aiSettings.docTiers);

    m_aiForceConsole = forceConsole;

    // LIST mode (see parseListRequest()) is a separate dispatch path — checked first so a match never falls
    // through to parseSplitMarker()/beginRequest() below.
    const AiAssistant::ParsedListRequest parsedList = AiAssistant::parseListRequest(text);
    if (parsedList.isList)
    {
        connectAiSignalsOnce();
        echoAiRequest(text);
        // splitByDefault forced OFF in generate-only mode, mirroring the plain-request fallback a
        // few lines below: split mode advances between PARTS only when a part actually executes,
        // so with no execution every item would emit part 1's script and lose parts 2..N (the
        // next item then resets the split state, hiding it). One script per item is what
        // generate-only can honestly deliver.
        m_ai->beginListRequest(parsedList, autoRun && splitDefault, reviewDefault, exploreDefault, autoRun);
        return;
    }

    // Split-mode trigger — see AiAssistant.h's splitModeActive doc comment and
    // AiAssistant::parseSplitMarker()'s doc comment for the "SPLIT"/"NOSPLIT" marker rules.
    QString effectiveText;
    bool wantsSplit = AiAssistant::parseSplitMarker(text, splitDefault, &effectiveText);
    if (wantsSplit && !autoRun)
    {
        // Split mode needs execution to advance, so can't honor "generate only" — previously force-enabled autoRun,
        // overriding the no-run choice. Now falls back to a plain, non-split request: autoRun=false is always honored
        // (losing split decomposition here), no print since this is now normal behavior, not worth announcing.
        wantsSplit = false;
    }

    // Chained onto effectiveText (already stripped of SPLIT/NOSPLIT), so REVIEW/NOREVIEW work
    // together with either marker in any order/position.
    m_ai->setSelfReviewEnabled(AiAssistant::parseReviewMarker(effectiveText, reviewDefault, &effectiveText));
    // Chained onto effectiveText the same way, so EXPLORE/NOEXPLORE combines with the SPLIT and
    // REVIEW markers in any order/position.
    m_ai->setExploreEnabled(AiAssistant::parseExploreMarker(effectiveText, exploreDefault, &effectiveText));

    connectAiSignalsOnce();
    echoAiRequest(effectiveText);

    // Client-side intercepts — answered locally without an API call.
    if (handleAiTextCommand(effectiveText.trimmed().toLower()))
        return;

    // Suffix wording lives in QtiSasAiContent (host-specific — names this host's own scriptPrint()
    // function), not here — see AiContentProvider.h.
    const QString aiText = effectiveText + m_aiContent.requestSuffix(autoRun);
    // State reset, originalRequestText anchoring, and dispatch (existence gate, continuation
    // check, split-plan vs. doc-selection preflight) are all encapsulated in AiAssistant now —
    // see beginRequest()'s doc comment.
    m_ai->beginRequest(aiText, autoRun, wantsSplit);
}
#endif // SCRIPTING_PYTHON

void ApplicationWindow::scriptPrint(const QString &text)
{
    if (m_aiCapturing && !text.trimmed().isEmpty())
    {
        m_aiCaptureBuffer += text + "\n";
        m_aiTranscript << text;
    }

#ifdef SCRIPTING_CONSOLE
    // Duplicates print()'s Script Output Panel check since AI scripts call scriptPrint() directly,
    // bypassing askAi()'s routing; d_fitFromScriptWindow alone misses AI runs.
    // Explicit foreground (via qApp's palette) since append() would otherwise inherit a leftover
    // green from prior script output.
    QTextCharFormat fmt;
    fmt.setForeground(qApp->palette().color(QPalette::Text));
    if (d_fitFromScriptWindow || (m_aiCapturing && !m_aiForceConsole && scriptWindow && scriptWindow->isVisible()))
    {
        scriptWindow->showOutputPanel();
        QTextEdit *out = scriptWindow->outputConsole();
        if (!text.trimmed().isEmpty())
        {
            QTextCursor cursor(out->document());
            cursor.movePosition(QTextCursor::End);
            // Newline+text in ONE insertText(), not insertBlock() then insertText() — see the
            // matching comment in scriptPrintHighlighted(): the latter let
            // PythonSyntaxHighlighter's isScriptOutput probe fire on the still-EMPTY new block
            // first, missing this format and leaving Python highlighting (e.g. its numeric-color
            // rule) on top of what should be plain text.
            cursor.insertText((out->document()->isEmpty() ? QString() : QString("\n")) + text, fmt);
            out->setTextCursor(cursor);
            out->setCurrentCharFormat(fmt);
            out->ensureCursorVisible();
        }
    }
    else
    {
        consoleWindow->show();
        if (explorerWindow && tabifiedDockWidgets(explorerWindow).contains(consoleWindow))
            tabifyDockWidget(explorerWindow, consoleWindow);
        else if (logWindow && tabifiedDockWidgets(logWindow).contains(consoleWindow))
            tabifyDockWidget(logWindow, consoleWindow);
        if (!text.trimmed().isEmpty())
        {
            QTextCursor cursor(console->document());
            cursor.movePosition(QTextCursor::End);
            // See the matching comment in the scriptWindow branch above — same fix, same reason.
            cursor.insertText((console->document()->isEmpty() ? QString() : QString("\n")) + text, fmt);
            console->setTextCursor(cursor);
            console->setCurrentCharFormat(fmt);
            console->ensureCursorVisible();
        }
    }
#else
    qDebug() << text;
#endif
}

void ApplicationWindow::scriptPrintReplace(const QString &text)
{
#ifdef SCRIPTING_CONSOLE
    // Same AI-routing gap as scriptPrint() above — see that comment.
    if (d_fitFromScriptWindow || (m_aiCapturing && !m_aiForceConsole && scriptWindow && scriptWindow->isVisible()))
    {
        QTextEdit *out = scriptWindow->outputConsole();
        QTextCursor cursor = out->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(text);
        out->setTextCursor(cursor);
    }
    else
    {
        consoleWindow->show();
        QTextCursor cursor = console->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(text);
        console->setTextCursor(cursor);
    }
#else
    qDebug() << text;
#endif
}

// scriptOutputColor is shared with PythonSyntaxHighlighter — see its doc comment in
// PythonSyntaxHighlighter.h (already #included above) for why it lives there instead of here.

void ApplicationWindow::scriptPrintHighlighted(const QString &text)
{
    if (m_aiCapturing && !text.trimmed().isEmpty())
    {
        m_aiCaptureBuffer += text + "\n";
        m_aiTranscript << text;
    }

#ifdef SCRIPTING_CONSOLE
    // Mirrors scriptPrint() above line-for-line (including its own comment on why this routing
    // check is duplicated here) — only the final append is replaced with a colored insert.
    QTextCharFormat fmt;
    fmt.setForeground(scriptOutputColor);
    if (d_fitFromScriptWindow || (m_aiCapturing && !m_aiForceConsole && scriptWindow && scriptWindow->isVisible()))
    {
        scriptWindow->showOutputPanel();
        QTextEdit *out = scriptWindow->outputConsole();
        if (!text.trimmed().isEmpty())
        {
            // Captured BEFORE touching the cursor at all — currentCharFormat() is "whatever
            // format text typed here right now would use", i.e. the real previous appearance,
            // not an assumed/blank default. Restored below so later plain-colored lines pick up
            // exactly where this one's surroundings left off, instead of stayng green.
            const QTextCharFormat previousFormat = out->currentCharFormat();
            QTextCursor cursor(out->document());
            cursor.movePosition(QTextCursor::End);
            // Newline+text in ONE insertText(), not insertBlock() then insertText() — the latter
            // let PythonSyntaxHighlighter's isScriptOutput probe fire on the still-EMPTY new
            // block first, missing the green format and leaving Python highlighting on top.
            cursor.insertText((out->document()->isEmpty() ? QString() : QString("\n")) + text, fmt);
            // Without this, insertText() leaves its format as the cursor's "current" format —
            // append() (used by the plain, uncolored scriptPrint()/print() for every later line)
            // continues from whatever format the document last ended with, so every subsequent
            // line would silently render green too, confirmed live.
            cursor.setCharFormat(previousFormat);
            out->setTextCursor(cursor);
            out->setCurrentCharFormat(previousFormat);
            out->ensureCursorVisible();
        }
    }
    else
    {
        consoleWindow->show();
        if (explorerWindow && tabifiedDockWidgets(explorerWindow).contains(consoleWindow))
            tabifyDockWidget(explorerWindow, consoleWindow);
        else if (logWindow && tabifiedDockWidgets(logWindow).contains(consoleWindow))
            tabifyDockWidget(logWindow, consoleWindow);
        if (!text.trimmed().isEmpty())
        {
            const QTextCharFormat previousFormat = console->currentCharFormat();
            QTextCursor cursor(console->document());
            cursor.movePosition(QTextCursor::End);
            // See the matching comment in the scriptWindow branch above — same fix, same reason.
            cursor.insertText((console->document()->isEmpty() ? QString() : QString("\n")) + text, fmt);
            // See the matching comment in the scriptWindow branch above — restores the format
            // that was actually in effect before this call, not an assumed default.
            cursor.setCharFormat(previousFormat);
            console->setTextCursor(cursor);
            console->setCurrentCharFormat(previousFormat);
            console->ensureCursorVisible();
        }
    }
#else
    qDebug() << text;
#endif
}

void ApplicationWindow::scriptPrintReplaceHighlighted(const QString &text)
{
#ifdef SCRIPTING_CONSOLE
    QTextCharFormat fmt;
    fmt.setForeground(scriptOutputColor);
    if (d_fitFromScriptWindow || (m_aiCapturing && !m_aiForceConsole && scriptWindow && scriptWindow->isVisible()))
    {
        QTextEdit *out = scriptWindow->outputConsole();
        // See scriptPrintHighlighted()'s comment — captured before the selection below exists,
        // so a PRIOR call's own green line (about to be replaced) is never mistaken for "the
        // previous format".
        const QTextCharFormat previousFormat = out->currentCharFormat();
        QTextCursor cursor = out->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(text, fmt);
        cursor.setCharFormat(previousFormat);
        out->setTextCursor(cursor);
        out->setCurrentCharFormat(previousFormat);
    }
    else
    {
        consoleWindow->show();
        const QTextCharFormat previousFormat = console->currentCharFormat();
        QTextCursor cursor = console->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(text, fmt);
        cursor.setCharFormat(previousFormat);
        console->setTextCursor(cursor);
        console->setCurrentCharFormat(previousFormat);
    }
#else
    qDebug() << text;
#endif
}

bool ApplicationWindow::scriptInterrupted()
{
#ifdef SCRIPTING_PYTHON
    return PyErr_CheckSignals() != 0;
#else
    return false;
#endif
}

void ApplicationWindow::setScriptFitRunning(bool on)
{
    scriptFitRunning = on;
}

void ApplicationWindow::setFitFromScriptWindow(bool on)
{
    d_fitFromScriptWindow = on;
}

ApplicationWindow::TerminalMode ApplicationWindow::effectiveTerminalMode(Qt::KeyboardModifiers modifiers) const
{
#ifdef SCRIPTING_PYTHON
    if (scriptEnv->objectName() != "Python")
        return TerminalMode::Script;
    const bool ctrl = modifiers & (Qt::ControlModifier | Qt::MetaModifier);
    if (m_terminalMode == TerminalMode::AiChat)
        return !ctrl ? TerminalMode::AiChat : (modifiers & Qt::ShiftModifier ? TerminalMode::AiRun : TerminalMode::Ai);
    if (m_terminalMode == TerminalMode::AiRun)
        return !ctrl ? TerminalMode::AiRun : (modifiers & Qt::ShiftModifier ? TerminalMode::Script : TerminalMode::Ai);
    if (m_terminalMode == TerminalMode::Ai)
        return !ctrl ? TerminalMode::Ai : (modifiers & Qt::ShiftModifier ? TerminalMode::AiRun : TerminalMode::Script);
    if (ctrl)
        return modifiers & Qt::ShiftModifier ? TerminalMode::AiRun : TerminalMode::Ai;
#else
    Q_UNUSED(modifiers);
#endif
    return TerminalMode::Script;
}

void ApplicationWindow::updateTerminalModeSuffix(Qt::KeyboardModifiers modifiers)
{
    if (!m_terminalModeLabel)
        return;
    const TerminalMode mode = effectiveTerminalMode(modifiers);
    if (m_terminalCompleter)
        m_terminalCompleter->setKeywordCompletionEnabled(d_completion && mode == TerminalMode::Script);
    const QString label = mode == TerminalMode::AiChat  ? "ai-chat"
                          : mode == TerminalMode::Ai    ? "ai"
                          : mode == TerminalMode::AiRun ? "ai-run"
                                                        : scriptEnv->objectName().toLower();
    m_terminalModeLabel->setText(label);
}

void ApplicationWindow::submitTerminalLine(Qt::KeyboardModifiers modifiers)
{
    const QString text = terminal_line->text();
    const QString command = text.trimmed();
    // Modifier shortcuts bypass returnPressed, and AI dispatch clears the input.
    if (m_terminalCompleter)
    {
        const TerminalMode mode = effectiveTerminalMode(modifiers);
        const QString label = mode == TerminalMode::AiChat  ? "ai-chat"
                              : mode == TerminalMode::Ai    ? "ai"
                              : mode == TerminalMode::AiRun ? "ai-run"
                                                            : scriptEnv->objectName().toLower();
        m_terminalCompleter->addHistoryEntry(text, label);
    }
    if (command == "/python" || command == "/muparser")
    {
        const QString language = command == "/python" ? "Python" : "muParser";
        if (setScriptingLanguage(language))
        {
            m_terminalMode = TerminalMode::Script;
            terminal_line->clear();
        }
        else
            d_status_info->setText(tr("Could not select scripting language: %1").arg(language));
    }
    else if (command == "/ai" || command == "/ai-run" || command == "/ai-chat")
    {
#ifdef SCRIPTING_PYTHON
        if (scriptEnv->objectName() != "Python")
            d_status_info->setText(tr("AI requires Python. Select /python first."));
        else
        {
            m_terminalMode = command == "/ai-chat" ? TerminalMode::AiChat
                             : command == "/ai"    ? TerminalMode::Ai
                                                   : TerminalMode::AiRun;
            terminal_line->clear();
        }
#else
        d_status_info->setText(tr("AI requires a build with Python support."));
#endif
    }
    else if (command == "?")
        terminal(command);
    else if (!command.isEmpty())
    {
        const TerminalMode mode = effectiveTerminalMode(modifiers);
        if (mode == TerminalMode::Script)
            terminal(text);
#ifdef SCRIPTING_PYTHON
        else if (mode == TerminalMode::AiChat)
        {
            if (!askAiChat(command, true))
                return;
            terminal_line->clear();
        }
        else
        {
            const bool run = mode == TerminalMode::AiRun;
            d_status_info->setText((run ? "▸▸▸ AI (running): " : "▸▸▸ AI: ") + command);
            askAi(command, run, true);
            terminal_line->clear();
        }
#endif
    }
    terminal_line->setFocus();
    updateTerminalModeSuffix(modifiers);
}

bool ApplicationWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == terminal_line)
    {
        if (m_terminalModeLabel && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ||
                                    event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
        {
            Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
            if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
            {
                auto *ke = dynamic_cast<QKeyEvent *>(event);
                modifiers = ke->modifiers();
                Qt::KeyboardModifier changed = Qt::NoModifier;
                if (ke->key() == Qt::Key_Control)
                    changed = Qt::ControlModifier;
                else if (ke->key() == Qt::Key_Meta)
                    changed = Qt::MetaModifier;
                else if (ke->key() == Qt::Key_Shift)
                    changed = Qt::ShiftModifier;
                if (event->type() == QEvent::KeyPress)
                    modifiers |= changed;
                else
                    modifiers &= ~Qt::KeyboardModifiers(changed);
            }
            if (event->type() == QEvent::FocusOut)
                modifiers = Qt::NoModifier;
            updateTerminalModeSuffix(modifiers);
        }
        if (event->type() == QEvent::FocusIn)
            actionTableRecalculate->setShortcut(QKeySequence());
        else if (event->type() == QEvent::FocusOut)
            actionTableRecalculate->setShortcut(QKeySequence(tr("Ctrl+Return")));
    }
#ifdef SCRIPTING_PYTHON
    auto *se = qobject_cast<ScriptEdit *>(obj);
    if (se && event->type() == QEvent::KeyPress)
    {
        auto *ke = dynamic_cast<QKeyEvent *>(event);
        const bool ctrl = ke->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
        const bool shift = ke->modifiers() & Qt::ShiftModifier;
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) && ctrl)
        {
            QString q = se->textCursor().selectedText().trimmed();
            if (q.isEmpty())
                q = se->toPlainText().trimmed();
            if (!q.isEmpty())
                askAi(q, shift);
            return true;
        }
    }
#endif
    if (scriptFitRunning && event->type() == QEvent::KeyPress)
    {
        auto *ke = dynamic_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers() == Qt::ControlModifier)
        {
#ifdef SCRIPTING_PYTHON
            PyErr_SetInterrupt();
#endif
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

bool ApplicationWindow::setScriptingLanguage(const QString &lang, bool force)
{
	if (!force && lang == scriptEnv->objectName())return true;
	if (lang.isEmpty()) return false;

    delete d_terminal_script;
    d_terminal_script = nullptr;

	ScriptingEnv *newEnv = ScriptingLangManager::newEnv(lang.toLocal8Bit().constData(), this);

	if (!newEnv)
		return false;

	connect(newEnv, SIGNAL(error(const QString&,const QString&,int)),
			this, SLOT(scriptError(const QString&,const QString&,int)));
	connect(newEnv, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));

	if (!newEnv->initialize()){
		delete newEnv;
		return false;
	}

	// notify everyone who might be interested
	ScriptingChangeEvent *sce = new ScriptingChangeEvent(newEnv);
	QApplication::sendEvent(this, sce);
	delete sce;

	initCompleter();

	foreach(QObject *i, findChildren<QObject*>())
		QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));
    if (scriptWindow)
    {
        foreach (QObject *i, scriptWindow->findChildren<QObject *>())
            QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));
    }

#ifdef SCRIPTING_PYTHON
	bool python = (lang == QString("Python"));
	actionCommentSelection->setEnabled(python);
	actionUncommentSelection->setEnabled(python);
#endif

	return true;
}

void ApplicationWindow::showScriptingLangDialog()
{
	ScriptingLangDialog* d = new ScriptingLangDialog(scriptEnv, this);
	d->exec();
}

void ApplicationWindow::restartScriptingEnv()
{
	if (setScriptingLanguage(scriptEnv->objectName(), true))
		executeNotes();
	else
		QMessageBox::critical(this, tr("QTISAS - Scripting Error"),
				tr("Scripting language \"%1\" failed to initialize.").arg(scriptEnv->objectName()));
}

void ApplicationWindow::openTemplate()
{
	QString filter = tr("QtiPlot 2D Graph Template") + " (*.qpt);";
	filter += tr("QtiPlot 3D Surface Template") + " (*.qst);";
	filter += tr("QtiPlot Table Template") + " (*.qtt);";
	filter += tr("QtiPlot Matrix Template") + " (*.qmt)";

    QString fn = getFileName(this, tr("QTISAS - Open Template File"), templatesDir, filter, nullptr, false);
	if (!fn.isEmpty()){
		QFileInfo fi(fn);
		if (fn.contains(".qmt") || fn.contains(".qpt") || fn.contains(".qtt") || fn.contains(".qst"))
			openTemplate(fn);
		else {
			QMessageBox::critical(this,tr("QTISAS - File opening error"),
					tr("The file: <b>%1</b> is not a QtiPlot template file!").arg(fn));
			return;
		}
	}
}

MdiSubWindow *ApplicationWindow::openTemplate(const QString &fn)
{
    if (fn.isEmpty())
    {
        QMessageBox::critical(this, "QTISAS - File opening error", "The file is empty");
        return nullptr;
    }

    QString fullName = QFileInfo(fn).isAbsolute() ? fn : templatesDir + fn;

    if (!QFile::exists(fullName))
    {
        QMessageBox::critical(this, tr("QTISAS - File opening error"),
                              tr("The file: <b>%1</b> doesn't exist!").arg(fn));
        return nullptr;
    }

    QFile f(fullName);
    QTextStream t(&f);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif

    (void)f.open(QIODevice::ReadOnly);
    QStringList l = t.readLine().split(REGEXPS::whitespaces, Qt::SkipEmptyParts);
    const QString &fileType = l[0];
    if (fileType != "QtiPlot")
    {
        QMessageBox::critical(this, tr("QTISAS - File opening error"),
                              tr("The file: <b> %1 </b> was not created using QtiPlot!").arg(fn));
        return nullptr;
    }

    QStringList vl = l[1].split(".", Qt::SkipEmptyParts);
    d_file_version = 100 * (vl[0]).toInt() + 10 * (vl[1]).toInt() + (vl[2]).toInt();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    MdiSubWindow *w = nullptr;
    QString templateType;
    t >> templateType;

    if (templateType == "<SurfacePlot>")
    {
        t.skipWhiteSpace();
        QStringList lst;
        while (!t.atEnd())
            lst << t.readLine();
        w = Graph3D::restore(this, lst, d_file_version);
        if (w)
            ((Graph3D *)w)->clearData();
    }
    else
    {
        int rows, cols;
        t >> rows;
        t >> cols;
        t.skipWhiteSpace();
        QString geometry = t.readLine();

        if (templateType == "<multiLayer>")
        {
            w = multilayerPlot(generateUniqueName(tr("Graph")), 0, rows, cols);
            if (w)
            {
                auto ml = qobject_cast<MultiLayer *>(w);
                restoreWindowGeometry(this, w, geometry);
                if (d_file_version > 83)
                {
                    QStringList lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                    ml->setMargins(lst[1].toInt(), lst[2].toInt(), lst[3].toInt(), lst[4].toInt());
                    lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                    ml->setSpacing(lst[1].toInt(), lst[2].toInt());
                    lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                    ml->setLayerCanvasSize(lst[1].toInt(), lst[2].toInt());
                    lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                    ml->setAlignement(lst[1].toInt(), lst[2].toInt());
                }
                int keepAlignedFromFile = 0;
                while (!t.atEnd())
                { // open layers
                    QString s = t.readLine();
                    if (s.contains("<waterfall>"))
                    {
                        QStringList lst = s.trimmed().remove("<waterfall>").remove("</waterfall>").split(",");
                        Graph *ag = ml->activeLayer();
                        if (ag && lst.size() >= 2)
                        {
                            ag->setWaterfallOffset(lst[0].toInt(), lst[1].toInt());
                            if (lst.size() >= 3)
                                ag->setWaterfallSideLines(lst[2].toInt());
                        }
                        ml->setWaterfallLayout();
                    }
                    if (s.left(7) == "<graph>")
                    {
                        QStringList lst;
                        while (s != "</graph>")
                        {
                            s = t.readLine();
                            lst << s;
                        }
                        openGraph(this, ml, lst);
                    }
                    if (s.contains("<LinkXAxes>"))
                        ml->linkXLayerAxes(s.trimmed().remove("<LinkXAxes>").remove("</LinkXAxes>").toInt());
                    else if (s.contains("<AlignPolicy>"))
                        ml->setAlignPolicy((MultiLayer::AlignPolicy)s.trimmed()
                                               .remove("<AlignPolicy>")
                                               .remove("</AlignPolicy>")
                                               .toInt());
                    else if (s.contains("<CommonAxes>"))
                        ml->setCommonAxesLayout(s.trimmed().remove("<CommonAxes>").remove("</CommonAxes>").toInt());
                    else if (s.contains("<ScaleLayers>"))
                        ml->setScaleLayersOnResize(
                            s.trimmed().remove("<ScaleLayers>").remove("</ScaleLayers>").toInt());
                    else if (s.contains("<KeepAligned>"))
                        keepAlignedFromFile = s.trimmed().remove("<KeepAligned>").remove("</KeepAligned>").toInt();
                }
                if (keepAlignedFromFile)
                    ml->keepAligned(true);
            }
        }
        else
        {
            if (templateType == "<table>")
                w = newTable(tr("Table1"), rows, cols);
            else if (templateType == "<matrix>")
                w = newMatrix(rows, cols);
            if (w)
            {
                QStringList lst;
                while (!t.atEnd())
                    lst << t.readLine();
                w->restore(lst);
                restoreWindowGeometry(this, w, geometry);
            }
        }
    }

    f.close();
    if (w)
    {
        w->show();
        customMenu(w);
        customToolBars(w);
    }

    QApplication::restoreOverrideCursor();
    return w;
}

void ApplicationWindow::readSettings()
{
    setDefaultOptions();

    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/General");
    settings->beginGroup("/ApplicationGeometry");

    if (!settings->contains("/x"))
    {
        // FIRST START OPTIONS (no ini file)
        QStringList styles = QStyleFactory::keys();
        styles.sort();

#if defined(Q_OS_MACOS)
        if (styles.indexOf("macOS") >= 0)
            appStyle = styles[styles.indexOf("macOS")];
#elif defined(Q_OS_WIN)
        if (styles.indexOf("windows11") >= 0)
            appStyle = styles[styles.indexOf("windows11")];
        else if (styles.indexOf("windowsvista") >= 0)
            appStyle = styles[styles.indexOf("windowsvista")];
        else if (styles.indexOf("Windows") >= 0)
            appStyle = styles[styles.indexOf("Windows")];
#else
        if (styles.indexOf("Fusion") >= 0)
            appStyle = styles[styles.indexOf("Fusion")];
#endif
        else
            appStyle = styles[0];

        changeAppStyle(appStyle);

        updatePathesInInterfaces();

        actionShowExplorer->setChecked(true);
        showExplorerDialog();
        return;
    }
    d_app_rect = QRect(settings->value("/x", 0).toInt(), settings->value("/y", 0).toInt(),
                       settings->value("/width", 0).toInt(), settings->value("/height", 0).toInt());
    settings->endGroup(); // ApplicationGeometry

    bool firstRunOfNewVersion = false;
    QVersionNumber prevRunVersion = QVersionNumber::fromString(settings->value("/LastUsedVersion", "0.8.9").toString());
    QVersionNumber currentRunVersion(maj_version, min_version, patch_version);
    if (currentRunVersion > prevRunVersion)
        firstRunOfNewVersion = true;

    appLanguage = settings->value("/Language", QLocale::system().name().section('_', 0, 0)).toString();
    show_windows_policy =
        (ShowWindowsPolicy)settings->value("/ShowWindowsPolicy", ApplicationWindow::ActiveFolder).toInt();

    recentProjects = settings->value("/RecentProjects").toStringList();
    //Follows an ugly hack added by Ion in order to fix Qt4 porting issues
    //(only needed on Windows due to a Qt bug?)
#ifdef Q_OS_WIN
    if (!recentProjects.isEmpty() && recentProjects[0].contains("^e"))
        recentProjects = recentProjects[0].split("^e", Qt::SkipEmptyParts);
    else if (recentProjects.count() == 1)
    {
        QString s = recentProjects[0];
        if (s.remove(REGEXPS::whitespaces).isEmpty())
            recentProjects = QStringList();
    }
#endif

    updateRecentProjectsList();

    changeAppStyle(settings->value("/Style", appStyle).toString());
    autoSave = settings->value("/AutoSave", true).toBool();
    autoSaveTime = settings->value("/AutoSaveTime", 15).toInt();
    d_backup_files = settings->value("/BackupProjects", true).toBool();
    d_init_window_type = (WindowType)settings->value("/InitWindow", TableWindow).toInt();
    d_completion = settings->value("/Completion", true).toBool();
    d_open_last_project = settings->value("/OpenLastProject", d_open_last_project).toBool();
    defaultScriptingLang = settings->value("/ScriptingLang", "muParser").toString();

    // The numerical format of Preferences; C without group separators when none is saved
    QLocale loc(settings->value("/Locale", QLocale::c().name()).toString());
    const bool thousandsSep = settings->value("/ThousandsSeparator", false).toBool();
    loc.setNumberOptions(thousandsSep ? QLocale::DefaultNumberOptions : QLocale::OmitGroupSeparator);

    setLocale(loc);
    QLocale::setDefault(loc);

    d_decimal_digits = settings->value("/DecimalDigits", 13).toInt();
    d_clipboard_locale = QLocale(settings->value("/ClipboardLocale", QLocale::c().name()).toString());
    // Not "/MuParserCLocale": that key was saved as false while the checkbox was frozen
    d_muparser_c_locale = settings->value("/MuParserUsesCLocale", true).toBool();
    d_force_muParser = settings->value("/ForceMuParser", d_force_muParser).toBool();
    d_matrix_undo_stack_size = settings->value("/MatrixUndoStackSize", 10).toInt();
    d_eol = (EndLineChar)settings->value("/EndOfLine", d_eol).toInt();

    // restore dock windows and tool bars
    restoreState(settings->value("/DockWindows").toByteArray());
    explorerSplitter->restoreState(settings->value("/ExplorerSplitter").toByteArray());
    QList<int> lst = explorerSplitter->sizes();
    for (int i = 0; i < lst.count(); i++)
    {
        if (lst[i] == 0)
        {
            lst[i] = 45;
            explorerSplitter->setSizes(lst);
        }
    }

    QString applicationFont = settings->value("/Font").toString();
    if (!applicationFont.isEmpty())
        appFont.fromString(applicationFont);

    QStringList colors = settings->value("/IndexedColors").toStringList();
    if (!colors.isEmpty())
    {
        d_indexed_colors.clear();
        for (int i = 0; i < colors.size(); i++)
            d_indexed_colors << QColor(colors[i]);
    }
    d_indexed_color_names = settings->value("/IndexedColorNames", d_indexed_color_names).toStringList();
    QStringList symbols = settings->value("/IndexedSymbolsList").toStringList();
    if (!symbols.isEmpty())
    {
        d_symbols_list.clear();
        for (int i = 0; i < symbols.size(); i++)
            d_symbols_list << symbols[i].toInt();
    }

    settings->beginGroup("/Dialogs");
    d_extended_open_dialog = settings->value("/ExtendedOpenDialog", true).toBool();
    d_extended_export_dialog = settings->value("/ExtendedExportDialog", true).toBool();
    d_extended_import_ASCII_dialog = settings->value("/ExtendedImportAsciiDialog", true).toBool();
    d_extended_plot_dialog = settings->value("/ExtendedPlotDialog", true).toBool(); // used by PlotDialog

    settings->beginGroup("/AddRemoveCurves");
    d_add_curves_dialog_size = QSize(settings->value("/Width", 700).toInt(), settings->value("/Height", 400).toInt());
    d_add_curves_dialog_filter = settings->value("/Filter", "*").toString();
    d_add_curves_dialog_style = settings->value("/CurveStyle", int(Graph::LineSymbols)).toInt();
    d_show_current_folder = settings->value("/ShowCurrentFolder", false).toBool();
    settings->endGroup(); // AddRemoveCurves

    settings->beginGroup("/Statistics");
    d_stats_significance_level = settings->value("/SignificanceLevel", d_stats_significance_level).toDouble();
    d_stats_result_table = settings->value("/ResultTable", d_stats_result_table).toBool();
    d_stats_result_log = settings->value("/ResultLog", d_stats_result_log).toBool();
    d_stats_result_notes = settings->value("/Notes", d_stats_result_notes).toBool();
    d_descriptive_stats = settings->value("/DescriptiveStats", d_descriptive_stats).toBool();
    d_stats_confidence = settings->value("/ConfidenceIntervals", d_stats_confidence).toBool();
    d_stats_power = settings->value("/PowerAnalysis", d_stats_power).toBool();
    d_stats_output = settings->value("/OutputSettings", d_stats_output).toBool();
    settings->endGroup(); // Statistics

    settings->beginGroup("/Integration");
    d_int_sort_data = settings->value("/SortData", d_int_sort_data).toBool();
    d_int_show_plot = settings->value("/ShowPlot", d_int_show_plot).toBool();
    d_int_results_table = settings->value("/ResultsTable", d_int_results_table).toBool();
    settings->endGroup(); // Integration

    settings->beginGroup("/FFT");
    d_fft_norm_amp = settings->value("/NormalizeAmplitude", d_fft_norm_amp).toBool();
    d_fft_shift_res = settings->value("/ShiftResults", d_fft_shift_res).toBool();
    d_fft_power2 = settings->value("/Power2", d_fft_power2).toBool();
    settings->endGroup(); // FFT Dialog

    settings->endGroup(); // Dialogs

    settings->beginGroup("/Colors");
    workspaceColor = settings->value("/Workspace", "darkGray").value<QColor>();
    panelsColor = settings->value("/Panels", "#ffffff").value<QColor>();
    panelsTextColor = settings->value("/PanelsText", "#000000").value<QColor>();
    settings->endGroup(); // Colors

    settings->beginGroup("/Paths");
    QString appPath = qApp->applicationDirPath();
    workingDir = settings->value("/WorkingDir", appPath).toString();
    fitPluginsPath = settings->value("/FitPlugins", fitPluginsPath).toString();
#ifdef Q_OS_WIN
    asciiDirPath = settings->value("/ASCII", appPath).toString();
    imagesDirPath = settings->value("/Images", appPath).toString();
#else
    asciiDirPath = settings->value("/ASCII", QDir::homePath()).toString();
    imagesDirPath = settings->value("/Images", QDir::homePath()).toString();
    workingDir = settings->value("/WorkingDir", QDir::homePath()).toString();
#endif
    scriptsDirPath = settings->value("/ScriptsDir", appPath).toString();
    fitModelsPath = settings->value("/FitModelsDir", "").toString();
    d_python_config_folder = settings->value("/PythonConfigDir", d_python_config_folder).toString();
    d_latex_compiler_path = settings->value("/LaTeXCompiler", d_latex_compiler_path).toString();
    d_startup_scripts_folder = settings->value("/StartupScripts", d_startup_scripts_folder).toString();
    settings->endGroup(); // Paths

    d_open_project_filter = settings->value("/OpenProjectFilter", d_open_project_filter).toString();
    d_latex_compiler = settings->value("/TeXCompilerPolicy", d_latex_compiler).toInt();
    settings->endGroup(); // General

    settings->beginGroup("/UserFunctions");
    if (100 * maj_version + 10 * min_version + patch_version == 91 && settings->contains("/FitFunctions"))
    {
        saveFitFunctions(settings->value("/FitFunctions").toStringList());
        settings->remove("/FitFunctions");
    }
    surfaceFunc = settings->value("/SurfaceFunctions").toStringList();
    xFunctions = settings->value("/xFunctions").toStringList();
    yFunctions = settings->value("/yFunctions").toStringList();
    rFunctions = settings->value("/rFunctions").toStringList();
    thetaFunctions = settings->value("/thetaFunctions").toStringList();
    d_param_surface_func = settings->value("/ParametricSurfaces").toStringList();
    d_recent_functions = settings->value("/Functions").toStringList();
    settings->endGroup(); // UserFunctions

    settings->beginGroup("/Confirmations");
    confirmCloseFolder = settings->value("/Folder", true).toBool();
    confirmCloseTable = settings->value("/Table", true).toBool();
    confirmCloseMatrix = settings->value("/Matrix", true).toBool();
    confirmClosePlot2D = settings->value("/Plot2D", true).toBool();
    confirmClosePlot3D = settings->value("/Plot3D", true).toBool();
    confirmCloseNotes = settings->value("/Note", true).toBool();
    d_inform_rename_table = settings->value("/RenameTable", true).toBool();
    d_confirm_overwrite = settings->value("/Overwrite", true).toBool();
    d_ask_web_connection = settings->value("/WebConnection", d_ask_web_connection).toBool();
    d_confirm_modif_2D_points = settings->value("/ModifyDataPoints", d_confirm_modif_2D_points).toBool();
    settings->endGroup(); // Confirmations

    settings->beginGroup("/Tables");
    d_show_table_comments = settings->value("/DisplayComments", false).toBool();
    d_auto_update_table_values = settings->value("/AutoUpdateValues", true).toBool();
    d_show_table_paste_dialog = settings->value("/EnablePasteDialog", d_show_table_paste_dialog).toBool();
    settings->beginGroup("/Fonts");
    QString textFont = settings->value("/Text").toString();
    if (!textFont.isEmpty())
        tableTextFont.fromString(textFont);
    QString headerFont = settings->value("/Header").toString();
    if (!headerFont.isEmpty())
        tableHeaderFont.fromString(headerFont);
    settings->endGroup(); // Fonts
    settings->beginGroup("/Colors");
    tableBkgdColor = settings->value("/Background", "#ffffff").value<QColor>();
    tableTextColor = settings->value("/Text", "#000000").value<QColor>();
    tableHeaderColor = settings->value("/Header", "#000000").value<QColor>();
    tableHeaderColorRows = settings->value("/HeaderBgd", "#000000").value<QColor>();

    settings->endGroup(); // Colors
    settings->endGroup(); // Tables

    settings->beginGroup("/2DPlots");
    settings->beginGroup("/General");
    titleOn = settings->value("/Title", true).toBool();
    canvasFrameWidth = settings->value("/CanvasFrameWidth", 0).toDouble();
    defaultPlotMargin = settings->value("/Margin", 0).toInt();
    drawBackbones = settings->value("/AxesBackbones", true).toBool();
    axesLineWidth = settings->value("/AxesLineWidth", 1).toDouble();
    autoscale2DPlots = settings->value("/Autoscale", true).toBool();
    autoScaleFonts = settings->value("/AutoScaleFonts", true).toBool();
    autoResizeLayers = settings->value("/AutoResizeLayers", true).toBool();
    antialiasing2DPlots = settings->value("/Antialiasing", antialiasing2DPlots).toBool();
    d_scale_plots_on_print = settings->value("/ScaleLayersOnPrint", false).toBool();
    d_print_cropmarks = settings->value("/PrintCropmarks", false).toBool();
    d_layer_geometry_unit = settings->value("/GeometryUnit", d_layer_geometry_unit).toInt();
    d_layer_canvas_width = settings->value("/LayerCanvasWidth", d_layer_canvas_width).toInt();
    d_layer_canvas_height = settings->value("/LayerCanvasHeight", d_layer_canvas_height).toInt();
    settings->beginGroup("/Fonts");
    QString axesFont = settings->value("/Axes").toString();
    if (!axesFont.isEmpty())
        plotAxesFont.fromString(axesFont);
    QString numbersFont = settings->value("/Numbers").toString();
    if (!numbersFont.isEmpty())
        plotNumbersFont.fromString(numbersFont);
    QString legendFont = settings->value("/Legend").toString();
    if (!legendFont.isEmpty())
        plotLegendFont.fromString(legendFont);
    QString titleFont = settings->value("/Title").toString();
    if (!titleFont.isEmpty())
        plotTitleFont.fromString(titleFont);
    settings->endGroup(); // Fonts
    d_in_place_editing = settings->value("/InPlaceEditing", true).toBool();
    d_graph_background_color = settings->value("/BackgroundColor", d_graph_background_color).value<QColor>();
    d_graph_canvas_color = settings->value("/CanvasColor", d_graph_canvas_color).value<QColor>();
    d_graph_border_color = settings->value("/FrameColor", d_graph_border_color).value<QColor>();
    d_graph_background_opacity = settings->value("/BackgroundOpacity", d_graph_background_opacity).toInt();
    d_graph_canvas_opacity = settings->value("/BackgroundOpacity", d_graph_canvas_opacity).toInt();
    d_graph_border_width = settings->value("/FrameWidth", d_graph_border_width).toInt();
    d_canvas_frame_color = settings->value("/FrameColor", QColor(Qt::black)).value<QColor>();
    d_graph_axes_labels_dist = settings->value("/LabelsAxesDist", d_graph_axes_labels_dist).toInt();
    d_graph_tick_labels_dist = settings->value("/TickLabelsDist", d_graph_tick_labels_dist).toInt();
    int size = settings->beginReadArray("EnabledAxes");
    for (int i = 0; i < size; ++i)
    {
        settings->setArrayIndex(i);
        d_show_axes[i] = settings->value("enabled", true).toBool();
        d_show_axes_labels[i] = settings->value("labels", true).toBool();
    }
    settings->endArray();
    d_graph_legend_display =
        (Graph::LegendDisplayMode)settings->value("/LegendDisplayMode", d_graph_legend_display).toInt();
    d_graph_axis_labeling = (Graph::AxisTitlePolicy)settings->value("/AxisTitlePolicy", d_graph_axis_labeling).toInt();
    d_keep_aspect_ration = settings->value("/KeepAspectRatio", d_keep_aspect_ration).toBool();
    d_synchronize_graph_scales = settings->value("/SynchronizeScales", d_synchronize_graph_scales).toBool();
    d_show_empty_cell_gap = settings->value("/ShowEmptyCellGap", d_show_empty_cell_gap).toBool();
    d_graph_attach_policy = settings->value("/AttachPolicy", d_graph_attach_policy).toInt();
    d_graph_clipboard_format = settings->value("/ClipboardFormat", d_graph_clipboard_format).toInt();
    settings->endGroup(); // General

    settings->beginGroup("/Curves");
    defaultCurveStyle = settings->value("/Style", Graph::LineSymbols).toInt();
    defaultCurveLineWidth = settings->value("/LineWidth", 1).toFloat();
    d_curve_line_style = settings->value("/LineType", d_curve_line_style).toInt();
    defaultSymbolSize = settings->value("/SymbolSize", 7).toInt();
    defaultSymbolEdge = settings->value("/SymbolEdge", defaultSymbolEdge).toDouble();
    d_fill_symbols = settings->value("/FillSymbols", d_fill_symbols).toBool();
    d_symbol_style = settings->value("/SymbolStyle", d_symbol_style).toInt();
    d_symbol_color = settings->value("/SymbolColor", d_symbol_color).toInt();
    d_indexed_symbols = settings->value("/IndexedSymbols", d_indexed_symbols).toBool();
    d_indexed_symbol_colors = settings->value("/IndexedColors", d_indexed_symbol_colors).toBool();
    currentSymbolColorList = settings->value("/currentSymbolColorList", currentSymbolColorList).toInt();
    defaultCurveBrush = settings->value("/BrushStyle", defaultCurveBrush).toInt();
    defaultCurveAlpha = settings->value("/BrushAlpha", defaultCurveAlpha).toInt();
    d_disable_curve_antialiasing = settings->value("/DisableAntialiasing", d_disable_curve_antialiasing).toBool();
    d_curve_max_antialising_size = settings->value("/MaxCurveAntialisingSize", d_curve_max_antialising_size).toInt();
    settings->endGroup(); // Curves

    settings->beginGroup("/Ticks");
    majTicksStyle = settings->value("/MajTicksStyle", ScaleDraw::Out).toInt();
    minTicksStyle = settings->value("/MinTicksStyle", ScaleDraw::Out).toInt();
    minTicksLength = settings->value("/MinTicksLength", 5).toInt();
    majTicksLength = settings->value("/MajTicksLength", 9).toInt();
    settings->endGroup(); // Ticks

    settings->beginGroup("/Legend");
    legendFrameStyle = settings->value("/FrameStyle", LegendWidget::Line).toInt();
    d_frame_widget_pen.setColor(settings->value("/FrameColor", QColor(Qt::black)).value<QColor>());
    d_frame_widget_pen.setWidthF(settings->value("/FrameWidth", 1).toDouble());
    d_frame_widget_pen.setStyle(PenStyleBox::penStyle(settings->value("/FramePenStyle", 0).toInt()));
    legendTextColor = settings->value("/TextColor", "#000000").value<QColor>(); // default color Qt::black
    legendBackground =
        settings->value("/BackgroundColor", QColor(Qt::white)).value<QColor>(); // default color Qt::white
    legendBackground.setAlpha(settings->value("/Transparency", 255).toInt());   // opaque by default;
    d_legend_default_angle = settings->value("/Angle", 0).toInt();
    d_frame_geometry_unit = settings->value("/DefaultGeometryUnit", FrameWidget::Scale).toInt();
    settings->endGroup(); // Legend

    settings->beginGroup("/Arrows");
    defaultArrowLineWidth = settings->value("/Width", 1).toDouble();
    defaultArrowColor = settings->value("/Color", "#000000").value<QColor>(); // default color Qt::black
    defaultArrowHeadLength = settings->value("/HeadLength", 4).toInt();
    defaultArrowHeadAngle = settings->value("/HeadAngle", 45).toInt();
    defaultArrowHeadFill = settings->value("/HeadFill", true).toBool();
    defaultArrowLineStyle = Graph::getPenStyle(settings->value("/LineStyle", "SolidLine").toString());
    settings->endGroup(); // Arrows

    settings->beginGroup("/Rectangle");
    d_rect_default_background = settings->value("/BackgroundColor", QColor(Qt::white)).value<QColor>();
    d_rect_default_background.setAlpha(settings->value("/Transparency", 255).toInt());
    d_rect_default_brush.setColor(settings->value("/BrushColor", d_rect_default_brush).value<QColor>());
    d_rect_default_brush.setStyle(PatternBox::brushStyle(settings->value("/Pattern", 0).toInt()));
    settings->endGroup(); // Rectangle

    settings->beginGroup("/Grid");
    d_default_2D_grid->setRenderHint(QwtPlotItem::RenderAntialiased, settings->value("/Antialiased", false).toBool());
    d_default_2D_grid->enableX(settings->value("/MajorX", d_default_2D_grid->xEnabled()).toBool());

    QPen pen = d_default_2D_grid->majPenX();
    pen.setColor(settings->value("/MajorXColor", pen.color()).value<QColor>());
    pen.setStyle(PenStyleBox::penStyle(settings->value("/MajorXStyle", PenStyleBox::styleIndex(pen.style())).toInt()));
    pen.setWidthF(settings->value("/MajorXThickness", pen.widthF()).toDouble());
    d_default_2D_grid->setMajPenX(pen);
    d_default_2D_grid->enableXMin(settings->value("/MinorX", d_default_2D_grid->xMinEnabled()).toBool());

    pen = d_default_2D_grid->minPenX();
    pen.setColor(settings->value("/MinorXColor", pen.color()).value<QColor>());
    pen.setStyle(PenStyleBox::penStyle(settings->value("/MinorXStyle", PenStyleBox::styleIndex(pen.style())).toInt()));
    pen.setWidthF(settings->value("/MinorXThickness", pen.widthF()).toDouble());
    d_default_2D_grid->setMinPenX(pen);

    pen = d_default_2D_grid->majPenY();
    d_default_2D_grid->enableY(settings->value("/MajorY", d_default_2D_grid->yEnabled()).toBool());
    pen.setColor(settings->value("/MajorYColor", pen.color()).value<QColor>());
    pen.setStyle(PenStyleBox::penStyle(settings->value("/MajorYStyle", PenStyleBox::styleIndex(pen.style())).toInt()));
    pen.setWidthF(settings->value("/MajorYThickness", pen.widthF()).toDouble());
    d_default_2D_grid->setMajPenY(pen);
    d_default_2D_grid->enableYMin(settings->value("/MinorY", d_default_2D_grid->yMinEnabled()).toBool());

    pen = d_default_2D_grid->minPenY();
    pen.setColor(settings->value("/MinorYColor", pen.color()).value<QColor>());
    pen.setStyle(PenStyleBox::penStyle(settings->value("/MinorYStyle", PenStyleBox::styleIndex(pen.style())).toInt()));
    pen.setWidthF(settings->value("/MinorYThickness", pen.widthF()).toDouble());
    d_default_2D_grid->setMinPenY(pen);
    settings->endGroup(); // Grid

    settings->endGroup(); // 2D Plots

    settings->beginGroup("/3DPlots");
    d_3D_legend = settings->value("/Legend", true).toBool();
    d_3D_projection = settings->value("/Projection", d_3D_projection).toInt();
    d_3D_smooth_mesh = settings->value("/Antialiasing", true).toBool();
    d_3D_resolution = settings->value("/Resolution", 1).toInt();
    d_3D_orthogonal = settings->value("/Orthogonal", false).toBool();
    d_3D_autoscale = settings->value("/Autoscale", true).toBool();
    settings->beginGroup("/Fonts");
    axesFont = settings->value("/Axes").toString();
    if (!axesFont.isEmpty())
        d_3D_axes_font.fromString(axesFont);
    numbersFont = settings->value("/Numbers").toString();
    if (!numbersFont.isEmpty())
        d_3D_numbers_font.fromString(numbersFont);
    titleFont = settings->value("/Title").toString();
    if (!titleFont.isEmpty())
        d_3D_title_font.fromString(titleFont);
    settings->endGroup(); // Fonts
    settings->beginGroup("/Colors");
    auto max_color = settings->value("/MaxData", QColor(Qt::red)).value<QColor>();
    d_3D_labels_color = settings->value("/Labels", d_3D_labels_color).value<QColor>();
    d_3D_mesh_color = settings->value("/Mesh", d_3D_mesh_color).value<QColor>();
    auto min_color = settings->value("/MinData", QColor(Qt::blue)).value<QColor>();
    d_3D_numbers_color = settings->value("/Numbers", d_3D_numbers_color).value<QColor>();
    d_3D_axes_color = settings->value("/Axes", d_3D_axes_color).value<QColor>();
    d_3D_background_color = settings->value("/Background", d_3D_background_color).value<QColor>();
    d_3D_color_map = std::make_unique<LinearColorMap>(min_color, max_color);
    d_3D_color_map->setMode(
        (QwtLinearColorMap::Mode)settings->value("/ColorMapMode", QwtLinearColorMap::ScaledColors).toInt());
    QList<QVariant> stop_values = settings->value("/ColorMapStops").toList();
    QStringList stop_colors = settings->value("/ColorMapColors").toStringList();
    if (stop_values.size() == stop_colors.size())
    {
        for (int i = 0; i < stop_colors.size(); i++)
            d_3D_color_map->addColorStop(stop_values[i].toDouble(), QColor(stop_colors[i]));
    }
    settings->endGroup(); // Colors

    settings->beginGroup("/Grids");
    d_3D_major_grids = settings->value("/EnableMajor", d_3D_major_grids).toBool();
    d_3D_grid_color = settings->value("/MajorColor", d_3D_grid_color).value<QColor>();
    d_3D_major_style = settings->value("/MajorStyle", d_3D_major_style).toInt();
    d_3D_major_width = settings->value("/MajorWidth", d_3D_major_width).toDouble();
    d_3D_minor_grids = settings->value("/EnableMinor", d_3D_minor_grids).toBool();
    d_3D_minor_grid_color = settings->value("/MinorColor", d_3D_minor_grid_color).value<QColor>();
    d_3D_minor_style = settings->value("/MinorStyle", d_3D_minor_style).toInt();
    d_3D_minor_width = settings->value("/MinorWidth", d_3D_minor_width).toDouble();
    settings->endGroup(); // Grids

    settings->endGroup(); // 3D Plots

    settings->beginGroup("/Fitting");
    fit_output_precision = settings->value("/OutputPrecision", 15).toInt();
    pasteFitResultsToPlot = settings->value("/PasteResultsToPlot", false).toBool();
    writeFitResultsToLog = settings->value("/WriteResultsToLog", true).toBool();
    generateUniformFitPoints = settings->value("/GenerateFunction", true).toBool();
    fitPoints = settings->value("/Points", 100).toInt();
    generatePeakCurves = settings->value("/GeneratePeakCurves", true).toBool();
    peakCurvesColor = QColor(settings->value("/PeakColor", peakCurvesColor.name()).toString()); // green color
    fit_scale_errors = settings->value("/ScaleErrors", true).toBool();
    d_2_linear_fit_points = settings->value("/TwoPointsLinearFit", true).toBool();
    d_multi_peak_messages = settings->value("/MultiPeakToolMsg", d_multi_peak_messages).toBool();
    settings->endGroup(); // Fitting

#ifdef SCRIPTING_PYTHON
    AiReadSettings(settings, aiSettings);
#endif

    settings->beginGroup("/ImportASCII");
    columnSeparator = settings->value("/ColumnSeparator", "\\t").toString();
    columnSeparator.replace("\\t", "\t").replace("\\s", " ");
    ignoredLines = settings->value("/IgnoreLines", 0).toInt();
    renameColumns = settings->value("/RenameColumns", true).toBool();
    strip_spaces = settings->value("/StripSpaces", false).toBool();
    simplify_spaces = settings->value("/SimplifySpaces", false).toBool();
    d_ASCII_file_filter = settings->value("/AsciiFileTypeFilter", "*").toString();

    // d_ASCII_import_locale = settings->value("/AsciiImportLocale", QLocale::system().name()).toString();
    d_ASCII_import_locale = QLocale::c();
    d_ASCII_import_locale.setNumberOptions(QLocale::OmitGroupSeparator);
    // if (settings->value("/OmitGroupSeparator", false).toBool())
    //    d_ASCII_import_locale.setNumberOptions(QLocale::OmitGroupSeparator);

    d_ASCII_import_mode = settings->value("/ImportMode", ImportASCIIDialog::NewTables).toInt();
    d_ASCII_comment_string = settings->value("/CommentString", "#").toString();
    d_ASCII_import_comments = settings->value("/ImportComments", false).toBool();
    d_ASCII_import_read_only = settings->value("/ImportReadOnly", false).toBool();
    d_ASCII_import_preview = settings->value("/Preview", true).toBool();
    d_preview_lines = settings->value("/PreviewLines", 100).toInt();
    d_ASCII_end_line = (EndLineChar)settings->value("/EndLineCharacter", d_ASCII_end_line).toInt();
    d_ASCII_import_first_row_role = settings->value("/FirstLineRole", 0).toInt();
    d_import_ASCII_dialog_size = settings->value("/DialogSize", d_import_ASCII_dialog_size).toSize();
    settings->endGroup(); // Import ASCII

    settings->beginGroup("/ExportASCII");
    d_export_col_separator = settings->value("/ColumnSeparator", "\\t").toString();
    d_export_col_separator.replace("\\t", "\t").replace("\\s", " ");
    d_export_col_names = settings->value("/ExportLabels", false).toBool();
    d_export_col_comment = settings->value("/ExportComments", false).toBool();
    d_export_table_selection = settings->value("/ExportSelection", false).toBool();
    d_export_ASCII_file_filter = settings->value("/ExportAsciiFilter", d_export_ASCII_file_filter).toString();
    settings->endGroup(); // ExportASCII

    settings->beginGroup("/ExportImage");
    d_image_export_filter = settings->value("/ImageFileTypeFilter", ".png").toString();
    d_export_transparency = settings->value("/ExportTransparency", false).toBool();
    d_export_quality = settings->value("/ImageQuality", 100).toInt();
    d_export_vector_resolution = settings->value("/Resolution", d_export_vector_resolution).toInt();
    d_export_color = settings->value("/ExportColor", true).toBool();
    d_3D_export_text_mode = settings->value("/3DTextMode", d_3D_export_text_mode).toInt();
    d_3D_export_sort = settings->value("/3DSortMode", d_3D_export_sort).toInt();
    d_export_bitmap_resolution = settings->value("/BitmapResolution", d_export_bitmap_resolution).toInt();
    d_export_raster_size = settings->value("/RasterSize", d_export_raster_size).toSizeF();
    d_export_size_unit = settings->value("/SizeUnit", d_export_size_unit).toInt();
    d_scale_fonts_factor = settings->value("/ScaleFontsFactor", d_scale_fonts_factor).toDouble();
    d_export_escape_tex_strings = settings->value("/EscapeTeXStrings", true).toBool();
    d_export_tex_font_sizes = settings->value("/ExportTeXFontSize", true).toBool();
    d_export_compression = settings->value("/Compression", d_export_compression).toBool();
    settings->endGroup(); // ExportImage

    settings->beginGroup("/ScriptWindow");
    d_script_win_on_top = settings->value("/AlwaysOnTop", false).toBool();
    d_script_win_rect = QRect(settings->value("/x", 0).toInt(), settings->value("/y", 0).toInt(),
                              settings->value("/width", 500).toInt(), settings->value("/height", 300).toInt());
    settings->endGroup(); // ScriptWindow

    settings->beginGroup("/ToolBars");
    d_file_tool_bar = settings->value("/FileToolBar", true).toBool();
    d_edit_tool_bar = settings->value("/EditToolBar", true).toBool();
    d_table_tool_bar = settings->value("/TableToolBar", true).toBool();
    d_column_tool_bar = settings->value("/ColumnToolBar", true).toBool();
    d_matrix_tool_bar = settings->value("/MatrixToolBar", true).toBool();
    d_plot_tool_bar = settings->value("/PlotToolBar", true).toBool();
    d_plot3D_tool_bar = settings->value("/Plot3DToolBar", true).toBool();
    d_display_tool_bar = settings->value("/DisplayToolBar", false).toBool();
    d_format_tool_bar = settings->value("/FormatToolBar", true).toBool();
    d_notes_tool_bar = settings->value("/NotesToolBar", true).toBool();
    settings->endGroup(); // ToolBars

    settings->beginGroup("/Notes");
    d_note_line_numbers = settings->value("/LineNumbers", true).toBool();
    d_notes_tab_length = settings->value("/TabLength", d_notes_tab_length).toInt();
    d_notes_font.setFamily(settings->value("/FontFamily", d_notes_font.family()).toString());
    d_notes_font.setPointSize(settings->value("/FontSize", d_notes_font.pointSize()).toInt());
    d_notes_font.setBold(settings->value("/FontBold", d_notes_font.bold()).toBool());
    d_notes_font.setItalic(settings->value("/FontItalic", d_notes_font.italic()).toBool());

    settings->beginGroup("/SyntaxHighlighting");
    d_comment_highlight_color = settings->value("/Comments", d_comment_highlight_color).value<QColor>();
    d_keyword_highlight_color = settings->value("/Keywords", d_keyword_highlight_color).value<QColor>();
    d_quotation_highlight_color = settings->value("/Quotations", d_quotation_highlight_color).value<QColor>();
    d_numeric_highlight_color = settings->value("/Numbers", d_numeric_highlight_color).value<QColor>();
    d_function_highlight_color = settings->value("/Functions", d_function_highlight_color).value<QColor>();
    d_class_highlight_color = settings->value("/QtClasses", d_class_highlight_color).value<QColor>();
    settings->endGroup(); // end group SyntaxHighlighting
    settings->endGroup(); // end group Notes

    settings->beginGroup("/PrintPreview");
    d_print_paper_size = static_cast<QPageSize::PageSizeId>(settings->value("/PaperSize", d_print_paper_size).toInt());
    d_printer_orientation =
        static_cast<QPageLayout::Orientation>(settings->value("/Orientation", d_printer_orientation).toInt());
    settings->endGroup(); // PrintPreview

    settings->beginGroup("/sasWidgets");
    sasFontIncrement = settings->value("/sasFontIncrement", sasFontIncrement).toInt();
    sasResoScale = settings->value("/sasResoScale", sasResoScale).toDouble();
    sasDefaultInterface = settings->value("/sasDefaultInterface", sasDefaultInterface).toInt();

    magicTemplateFile = settings->value("/magicTemplateFile", magicTemplateFile).toString();
    if (magicTemplateFile == "")
        magicTemplateFile = sasPath + "/templates/graph.qpt";
    magicTemplateFile.replace("\\","/");

    QDir d(sasPath + "/templates/");
    magicList = d.entryList(QStringList() << "*.qpt");
    currentColorMap = settings->value("/currentColorMap", currentColorMap).toInt();
    imageFormat = settings->value("/imageFormat", imageFormat).toString();
    imageRes = settings->value("/imageRes", imageRes).toInt(); // sasWidgets"

    updatePathesInInterfaces();
    delete settings;

#ifdef QTISAS
    danWidget->findCalibrators();
    danWidget->findSANSinstruments();
    danWidget->readSettings();
    ascii1dWidget->findASCII1DFormats();
    ascii1dWidget->readSettings();
#endif
}

void ApplicationWindow::saveSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/General");

    settings->setValue("/LastUsedVersion", QVersionNumber(maj_version, min_version, patch_version).toString());

    settings->beginGroup("/ApplicationGeometry");
    d_app_rect = QRect(this->pos(), this->size());
    if (this->isMaximized())
        d_app_rect = QRect();

    settings->setValue("/x", d_app_rect.x());
    settings->setValue("/y", d_app_rect.y());
    settings->setValue("/width", d_app_rect.width());
    settings->setValue("/height", d_app_rect.height());
    settings->endGroup();

    settings->setValue("/Language", appLanguage);
    settings->setValue("/ShowWindowsPolicy", show_windows_policy);
    settings->setValue("/RecentProjects", recentProjects);
    settings->setValue("/Style", appStyle);
    settings->setValue("/AutoSave", autoSave);
    settings->setValue("/AutoSaveTime", autoSaveTime);
    settings->setValue("/BackupProjects", d_backup_files);
    settings->setValue("/InitWindow", int(d_init_window_type));
    settings->setValue("/Completion", d_completion);
    settings->setValue("/OpenLastProject", d_open_last_project);
    settings->setValue("/ScriptingLang", defaultScriptingLang);

    bool thousandsSep = (locale().numberOptions() & QLocale::OmitGroupSeparator) ? false : true;
    settings->setValue("/ThousandsSeparator", thousandsSep);

    settings->setValue("/Locale", locale().name());
    settings->setValue("/DecimalDigits", d_decimal_digits);
    settings->setValue("/ClipboardLocale", d_clipboard_locale.name());
    settings->setValue("/MuParserUsesCLocale", d_muparser_c_locale);
    settings->setValue("/ForceMuParser", d_force_muParser);
    settings->setValue("/MatrixUndoStackSize", d_matrix_undo_stack_size);
    settings->setValue("/EndOfLine", (int)d_eol);
    settings->setValue("/DockWindows", saveState());
    settings->setValue("/ExplorerSplitter", explorerSplitter->saveState());

    settings->setValue("/Font", appFont.toString());

    QStringList indexedColors;
    for (int i = 0; i < d_indexed_colors.size(); i++)
        indexedColors << d_indexed_colors[i].name();
    settings->setValue("/IndexedColors", indexedColors);
    settings->setValue("/IndexedColorNames", d_indexed_color_names);

    QStringList indexedSymbols;
    for (int i = 0; i < d_symbols_list.size(); i++)
        indexedSymbols << QString::number(d_symbols_list[i]);
    settings->setValue("/IndexedSymbolsList", indexedSymbols);

    settings->beginGroup("/Dialogs");
    settings->setValue("/ExtendedOpenDialog", d_extended_open_dialog);
    settings->setValue("/ExtendedExportDialog", d_extended_export_dialog);
    settings->setValue("/ExtendedImportAsciiDialog", d_extended_import_ASCII_dialog);
    settings->setValue("/ExtendedPlotDialog", d_extended_plot_dialog);

    settings->beginGroup("/AddRemoveCurves");
    settings->setValue("/Width", d_add_curves_dialog_size.width());
    settings->setValue("/Height", d_add_curves_dialog_size.height());
    settings->setValue("/Filter", d_add_curves_dialog_filter);
    settings->setValue("/CurveStyle", d_add_curves_dialog_style);
    settings->setValue("/ShowCurrentFolder", d_show_current_folder);
    settings->endGroup(); // AddRemoveCurves

    settings->beginGroup("/Statistics");
    settings->setValue("/SignificanceLevel", d_stats_significance_level);
    settings->setValue("/ResultTable", d_stats_result_table);
    settings->setValue("/ResultLog", d_stats_result_log);
    settings->setValue("/Notes", d_stats_result_notes);
    settings->setValue("/DescriptiveStats", d_descriptive_stats);
    settings->setValue("/ConfidenceIntervals", d_stats_confidence);
    settings->setValue("/PowerAnalysis", d_stats_power);
    settings->setValue("/OutputSettings", d_stats_output);
    settings->endGroup(); // Statistics

    settings->beginGroup("/Integration");
    settings->setValue("/SortData", d_int_sort_data);
    settings->setValue("/ShowPlot", d_int_show_plot);
    settings->setValue("/ResultsTable", d_int_results_table);
    settings->endGroup(); // Integration

    settings->beginGroup("/FFT");
    settings->setValue("/NormalizeAmplitude", d_fft_norm_amp);
    settings->setValue("/ShiftResults", d_fft_shift_res);
    settings->setValue("/Power2", d_fft_power2);
    settings->endGroup(); // FFT

    settings->endGroup(); // Dialogs

    settings->beginGroup("/Colors");
    settings->setValue("/Workspace", workspaceColor);
    settings->setValue("/Panels", panelsColor);
    settings->setValue("/PanelsText", panelsTextColor);
    settings->endGroup(); // Colors

    settings->beginGroup("/Paths");
    settings->setValue("/WorkingDir", workingDir);
    settings->setValue("/FitPlugins", fitPluginsPath);
    settings->setValue("/ASCII", asciiDirPath);
    settings->setValue("/Images", imagesDirPath);
    settings->setValue("/ScriptsDir", scriptsDirPath);
    settings->setValue("/FitModelsDir", fitModelsPath);
    settings->setValue("/PythonConfigDir", d_python_config_folder);
    settings->setValue("/LaTeXCompiler", d_latex_compiler_path);
    settings->setValue("/StartupScripts", d_startup_scripts_folder);
    settings->endGroup(); // Paths

    settings->setValue("/OpenProjectFilter", d_open_project_filter);
    settings->setValue("/TeXCompilerPolicy", d_latex_compiler);
    settings->endGroup(); // General

    settings->beginGroup("/UserFunctions");
    settings->setValue("/SurfaceFunctions", surfaceFunc);
    settings->setValue("/xFunctions", xFunctions);
    settings->setValue("/yFunctions", yFunctions);
    settings->setValue("/rFunctions", rFunctions);
    settings->setValue("/thetaFunctions", thetaFunctions);
    settings->setValue("/ParametricSurfaces", d_param_surface_func);
    settings->setValue("/Functions", d_recent_functions);
    settings->endGroup(); // UserFunctions

    settings->beginGroup("/Confirmations");
    settings->setValue("/Folder", confirmCloseFolder);
    settings->setValue("/Table", confirmCloseTable);
    settings->setValue("/Matrix", confirmCloseMatrix);
    settings->setValue("/Plot2D", confirmClosePlot2D);
    settings->setValue("/Plot3D", confirmClosePlot3D);
    settings->setValue("/Note", confirmCloseNotes);
    settings->setValue("/RenameTable", d_inform_rename_table);
    settings->setValue("/Overwrite", d_confirm_overwrite);
    settings->setValue("/WebConnection", d_ask_web_connection);
    settings->setValue("/ModifyDataPoints", d_confirm_modif_2D_points);
    settings->endGroup(); // Confirmations

    settings->beginGroup("/Tables");
    settings->setValue("/DisplayComments", d_show_table_comments);
    settings->setValue("/AutoUpdateValues", d_auto_update_table_values);
    settings->setValue("/EnablePasteDialog", d_show_table_paste_dialog);
    settings->beginGroup("/Fonts");
    settings->setValue("/Header", tableHeaderFont.toString());
    settings->setValue("/Text", tableTextFont.toString());
    settings->endGroup(); // Fonts
    settings->beginGroup("/Colors");
    settings->setValue("/Background", tableBkgdColor);
    settings->setValue("/Text", tableTextColor);
    settings->setValue("/Header", tableHeaderColor);
    settings->setValue("/HeaderBgd", tableHeaderColorRows);
    settings->endGroup(); // Colors
    settings->endGroup(); // Tables

    settings->beginGroup("/2DPlots");
    settings->beginGroup("/General");
    settings->setValue("/Title", titleOn);
    settings->setValue("/CanvasFrameWidth", canvasFrameWidth);
    settings->setValue("/Margin", defaultPlotMargin);
    settings->setValue("/AxesBackbones", drawBackbones);
    settings->setValue("/AxesLineWidth", axesLineWidth);
    settings->setValue("/Autoscale", autoscale2DPlots);
    settings->setValue("/AutoScaleFonts", autoScaleFonts);
    settings->setValue("/AutoResizeLayers", autoResizeLayers);
    settings->setValue("/Antialiasing", antialiasing2DPlots);
    settings->setValue("/ScaleLayersOnPrint", d_scale_plots_on_print);
    settings->setValue("/PrintCropmarks", d_print_cropmarks);
    settings->setValue("/GeometryUnit", d_layer_geometry_unit);
    settings->setValue("/LayerCanvasWidth", d_layer_canvas_width);
    settings->setValue("/LayerCanvasHeight", d_layer_canvas_height);
    settings->setValue("/AxesFont", plotAxesFont.toString());
    settings->beginGroup("/Fonts");
    settings->setValue("/Numbers", plotNumbersFont.toString());
    settings->setValue("/Legend", plotLegendFont.toString());
    settings->setValue("/Title", plotTitleFont.toString());
    settings->endGroup(); // Fonts
    settings->setValue("/InPlaceEditing", d_in_place_editing);
    settings->setValue("/InPlaceEditing", d_in_place_editing);
    settings->setValue("/BackgroundColor", d_graph_background_color);
    settings->setValue("/CanvasColor", d_graph_canvas_color);
    settings->setValue("/FrameColor", d_graph_border_color);
    settings->setValue("/BackgroundOpacity", d_graph_background_opacity);
    settings->setValue("/BackgroundOpacity", d_graph_canvas_opacity);
    settings->setValue("/FrameWidth", d_graph_border_width);
    settings->setValue("/FrameColor", d_canvas_frame_color);
    settings->setValue("/LabelsAxesDist", d_graph_axes_labels_dist);
    settings->setValue("/TickLabelsDist", d_graph_tick_labels_dist);
    settings->beginWriteArray("EnabledAxes");
    for (int i = 0; i < QwtPlot::axisCnt; ++i)
    {
        settings->setArrayIndex(i);
        settings->setValue("axis", i);
        settings->setValue("enabled", d_show_axes[i]);
        settings->setValue("labels", d_show_axes_labels[i]);
    }
    settings->endArray();
    settings->setValue("/LegendDisplayMode", d_graph_legend_display);
    settings->setValue("/AxisTitlePolicy", d_graph_axis_labeling);
    settings->setValue("/KeepAspectRatio", d_keep_aspect_ration);
    settings->setValue("/SynchronizeScales", d_synchronize_graph_scales);
    settings->setValue("/ShowEmptyCellGap", d_show_empty_cell_gap);
    settings->setValue("/AttachPolicy", d_graph_attach_policy);
    settings->setValue("/ClipboardFormat", d_graph_clipboard_format);
    settings->endGroup(); // General

    settings->beginGroup("/Curves");
    settings->setValue("/Style", defaultCurveStyle);
    settings->setValue("/LineWidth", defaultCurveLineWidth);
    settings->setValue("/LineType", d_curve_line_style);
    settings->setValue("/SymbolSize", defaultSymbolSize);
    settings->setValue("/SymbolEdge", defaultSymbolEdge);
    settings->setValue("/FillSymbols", d_fill_symbols);
    settings->setValue("/SymbolStyle", d_symbol_style);
    settings->setValue("/SymbolColor", d_symbol_color);
    settings->setValue("/IndexedSymbols", d_indexed_symbols);
    settings->setValue("/IndexedColors", d_indexed_symbol_colors);
    settings->setValue("/BrushStyle", defaultCurveBrush);
    settings->setValue("/BrushAlpha", defaultCurveAlpha);
    settings->setValue("/DisableAntialiasing", d_disable_curve_antialiasing);
    settings->setValue("/MaxCurveAntialisingSize", d_curve_max_antialising_size);
    settings->setValue("/currentSymbolColorList", currentSymbolColorList);
    settings->endGroup(); // Curves

    settings->beginGroup("/Ticks");
    settings->setValue("/MajTicksStyle", majTicksStyle);
    settings->setValue("/MinTicksStyle", minTicksStyle);
    settings->setValue("/MinTicksLength", minTicksLength);
    settings->setValue("/MajTicksLength", majTicksLength);
    settings->endGroup(); // Ticks

    settings->beginGroup("/Legend");
    settings->setValue("/FrameStyle", legendFrameStyle);
    settings->setValue("/FrameColor", d_frame_widget_pen.color().name());
    settings->setValue("/FrameWidth", d_frame_widget_pen.widthF());
    settings->setValue("/FramePenStyle", PenStyleBox::styleIndex(d_frame_widget_pen.style()));
    settings->setValue("/TextColor", legendTextColor);
    settings->setValue("/BackgroundColor", legendBackground);
    settings->setValue("/Transparency", legendBackground.alpha());
    settings->setValue("/Angle", d_legend_default_angle);
    settings->setValue("/DefaultGeometryUnit", d_frame_geometry_unit);
    settings->endGroup(); // Legend

    settings->beginGroup("/Arrows");
    settings->setValue("/Width", defaultArrowLineWidth);
    settings->setValue("/Color", defaultArrowColor.name());
    settings->setValue("/HeadLength", defaultArrowHeadLength);
    settings->setValue("/HeadAngle", defaultArrowHeadAngle);
    settings->setValue("/HeadFill", defaultArrowHeadFill);
    settings->setValue("/LineStyle", Graph::penStyleName(defaultArrowLineStyle));
    settings->endGroup(); // Arrows

    settings->beginGroup("/Rectangle");
    settings->setValue("/BackgroundColor", d_rect_default_background);
    settings->setValue("/Transparency", d_rect_default_background.alpha());
    settings->setValue("/BrushColor", d_rect_default_brush.color());
    settings->setValue("/Pattern", PatternBox::patternIndex(d_rect_default_brush.style()));
    settings->endGroup(); // Rectangle

    settings->beginGroup("/Grid");
    settings->setValue("/Antialiased", d_default_2D_grid->testRenderHint(QwtPlotItem::RenderAntialiased));
    settings->setValue("/MajorX", d_default_2D_grid->xEnabled());
    settings->setValue("/MajorXColor", d_default_2D_grid->majPenX().color());
    settings->setValue("/MajorXStyle", PenStyleBox::styleIndex(d_default_2D_grid->majPenX().style()));
    settings->setValue("/MajorXThickness", d_default_2D_grid->majPenX().widthF());
    settings->setValue("/MinorX", d_default_2D_grid->xMinEnabled());
    settings->setValue("/MinorXColor", d_default_2D_grid->minPenX().color());
    settings->setValue("/MinorXStyle", PenStyleBox::styleIndex(d_default_2D_grid->minPenX().style()));
    settings->setValue("/MinorXThickness", d_default_2D_grid->minPenX().widthF());
    settings->setValue("/MajorY", d_default_2D_grid->yEnabled());
    settings->setValue("/MajorYColor", d_default_2D_grid->majPenY().color());
    settings->setValue("/MajorYStyle", PenStyleBox::styleIndex(d_default_2D_grid->majPenY().style()));
    settings->setValue("/MajorYThickness", d_default_2D_grid->majPenY().widthF());
    settings->setValue("/MinorY", d_default_2D_grid->yMinEnabled());
    settings->setValue("/MinorYColor", d_default_2D_grid->minPenY().color());
    settings->setValue("/MinorYStyle", PenStyleBox::styleIndex(d_default_2D_grid->minPenY().style()));
    settings->setValue("/MinorYThickness", d_default_2D_grid->minPenY().widthF());
    settings->endGroup(); // Grid
    settings->endGroup(); // 2D Plots

    settings->beginGroup("/3DPlots");
    settings->setValue("/Legend", d_3D_legend);
    settings->setValue("/Projection", d_3D_projection);
    settings->setValue("/Antialiasing", d_3D_smooth_mesh);
    settings->setValue("/Resolution", d_3D_resolution);
    settings->setValue("/Orthogonal", d_3D_orthogonal);
    settings->setValue("/Autoscale", d_3D_autoscale);
    settings->beginGroup("/Fonts");
    settings->setValue("/Title", d_3D_title_font.toString());
    settings->setValue("/Numbers", d_3D_numbers_font.toString());
    settings->setValue("/Axes", d_3D_axes_font.toString());
    settings->endGroup(); // Fonts
    settings->beginGroup("/Colors");
    settings->setValue("/MaxData", d_3D_color_map->color2());
    settings->setValue("/Labels", d_3D_labels_color);
    settings->setValue("/Mesh", d_3D_mesh_color);
    settings->setValue("/MinData", d_3D_color_map->color1());
    settings->setValue("/Numbers", d_3D_numbers_color);
    settings->setValue("/Axes", d_3D_axes_color);
    settings->setValue("/Background", d_3D_background_color);
    settings->setValue("/ColorMapMode", d_3D_color_map->mode());
    QList<QVariant> stop_values;
    QStringList stop_colors;
    QVector<double> colors = d_3D_color_map->colorStops();
    int stops = (int)colors.size() - 1;
    for (int i = 1; i < stops; i++)
    {
        stop_values << QVariant(colors[i]);
        stop_colors << d_3D_color_map->color(i).name();
    }
    settings->setValue("/ColorMapStops", QVariant(stop_values));
    settings->setValue("/ColorMapColors", stop_colors);
    settings->endGroup(); // Colors

    settings->beginGroup("/Grids");
    settings->setValue("/EnableMajor", d_3D_major_grids);
    settings->setValue("/MajorColor", d_3D_grid_color);
    settings->setValue("/MajorStyle", d_3D_major_style);
    settings->setValue("/MajorWidth", d_3D_major_width);
    settings->setValue("/EnableMinor", d_3D_minor_grids);
    settings->setValue("/MinorColor", d_3D_minor_grid_color);
    settings->setValue("/MinorStyle", d_3D_minor_style);
    settings->setValue("/MinorWidth", d_3D_minor_width);
    settings->endGroup(); // Grids

    settings->endGroup(); // 3D Plots

    settings->beginGroup("/Fitting");
    settings->setValue("/OutputPrecision", fit_output_precision);
    settings->setValue("/PasteResultsToPlot", pasteFitResultsToPlot);
    settings->setValue("/WriteResultsToLog", writeFitResultsToLog);
    settings->setValue("/GenerateFunction", generateUniformFitPoints);
    settings->setValue("/Points", fitPoints);
    settings->setValue("/GeneratePeakCurves", generatePeakCurves);
    settings->setValue("/PeakColor", peakCurvesColor.name());
    settings->setValue("/ScaleErrors", fit_scale_errors);
    settings->setValue("/TwoPointsLinearFit", d_2_linear_fit_points);
    settings->setValue("/MultiPeakToolMsg", d_multi_peak_messages);
    settings->endGroup(); // Fitting

#ifdef SCRIPTING_PYTHON
    AiWriteSettings(settings, aiSettings);
#endif

    settings->beginGroup("/ImportASCII");
    QString sep = columnSeparator;
    settings->setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
    settings->setValue("/IgnoreLines", ignoredLines);
    settings->setValue("/RenameColumns", renameColumns);
    settings->setValue("/StripSpaces", strip_spaces);
    settings->setValue("/SimplifySpaces", simplify_spaces);
    settings->setValue("/AsciiFileTypeFilter", d_ASCII_file_filter);
    settings->setValue("/AsciiImportLocale", d_ASCII_import_locale.name());

    bool omitGroupSep = (d_ASCII_import_locale.numberOptions() & QLocale::OmitGroupSeparator) ? true : false;
    settings->setValue("/OmitGroupSeparator", omitGroupSep);

    settings->setValue("/ImportMode", d_ASCII_import_mode);
    settings->setValue("/CommentString", d_ASCII_comment_string);
    settings->setValue("/ImportComments", d_ASCII_import_comments);
    settings->setValue("/ImportReadOnly", d_ASCII_import_read_only);
    settings->setValue("/Preview", d_ASCII_import_preview);
    settings->setValue("/PreviewLines", d_preview_lines);
    settings->setValue("/EndLineCharacter", (int)d_ASCII_end_line);
    settings->setValue("/FirstLineRole", d_ASCII_import_first_row_role);
    settings->setValue("/DialogSize", d_import_ASCII_dialog_size);
    settings->endGroup(); // ImportASCII

    settings->beginGroup("/ExportASCII");
    sep = d_export_col_separator;
    settings->setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
    settings->setValue("/ExportLabels", d_export_col_names);
    settings->setValue("/ExportComments", d_export_col_comment);
    settings->setValue("/ExportSelection", d_export_table_selection);
    settings->setValue("/ExportAsciiFilter", d_export_ASCII_file_filter);
    settings->endGroup(); // ExportASCII

    settings->beginGroup("/ExportImage");
    settings->setValue("/ImageFileTypeFilter", d_image_export_filter);
    settings->setValue("/ExportTransparency", d_export_transparency);
    settings->setValue("/ImageQuality", d_export_quality);
    settings->setValue("/Resolution", d_export_vector_resolution);
    settings->setValue("/ExportColor", d_export_color);
    settings->setValue("/3DTextMode", d_3D_export_text_mode);
    settings->setValue("/3DSortMode", d_3D_export_sort);
    settings->setValue("/BitmapResolution", d_export_bitmap_resolution);
    settings->setValue("/RasterSize", d_export_raster_size);
    settings->setValue("/SizeUnit", d_export_size_unit);
    settings->setValue("/ScaleFontsFactor", d_scale_fonts_factor);
    settings->setValue("/EscapeTeXStrings", d_export_escape_tex_strings);
    settings->setValue("/ExportTeXFontSize", d_export_tex_font_sizes);
    settings->setValue("/Compression", d_export_compression);
    settings->endGroup(); // ExportImage

    settings->beginGroup("/ScriptWindow");
    settings->setValue("/AlwaysOnTop", d_script_win_on_top);
    settings->setValue("/x", d_script_win_rect.x());
    settings->setValue("/y", d_script_win_rect.y());
    settings->setValue("/width", d_script_win_rect.width());
    settings->setValue("/height", d_script_win_rect.height());
    settings->endGroup(); // ScriptWindow

    settings->beginGroup("/ToolBars");
    settings->setValue("/FileToolBar", d_file_tool_bar);
    settings->setValue("/EditToolBar", d_edit_tool_bar);
    settings->setValue("/TableToolBar", d_table_tool_bar);
    settings->setValue("/ColumnToolBar", d_column_tool_bar);
    settings->setValue("/MatrixToolBar", d_matrix_tool_bar);
    settings->setValue("/PlotToolBar", d_plot_tool_bar);
    settings->setValue("/Plot3DToolBar", d_plot3D_tool_bar);
    settings->setValue("/DisplayToolBar", d_display_tool_bar);
    settings->setValue("/FormatToolBar", d_format_tool_bar);
    settings->setValue("/NotesToolBar", d_notes_tool_bar);
    settings->endGroup(); // ToolBars

    settings->beginGroup("/Notes");
    settings->setValue("/LineNumbers", d_note_line_numbers);
    settings->setValue("/TabLength", d_notes_tab_length);
    settings->setValue("/FontFamily", d_notes_font.family());
    settings->setValue("/FontSize", d_notes_font.pointSize());
    settings->setValue("/FontBold", d_notes_font.bold());
    settings->setValue("/FontItalic", d_notes_font.italic());
    settings->beginGroup("/SyntaxHighlighting");
    settings->setValue("/Comments", d_comment_highlight_color.name());
    settings->setValue("/Keywords", d_keyword_highlight_color.name());
    settings->setValue("/Quotations", d_quotation_highlight_color.name());
    settings->setValue("/Numbers", d_numeric_highlight_color.name());
    settings->setValue("/Functions", d_function_highlight_color.name());
    settings->setValue("/QtClasses", d_class_highlight_color.name());
    settings->endGroup(); // SyntaxHighlighting
    settings->endGroup(); // Notes

    settings->beginGroup("/PrintPreview");
    settings->setValue("/PaperSize", static_cast<int>(d_print_paper_size));
    settings->setValue("/Orientation", static_cast<int>(d_printer_orientation));
    settings->endGroup(); // PrintPreview

    settings->beginGroup("/sasWidgets");
    settings->setValue("/sasFontIncrement", sasFontIncrement);
    settings->setValue("/sasPath", sasPath);
    settings->setValue("/sasResoScale", sasResoScale);
    settings->setValue("/sasDefaultInterface", sasDefaultInterface);
    settings->setValue("/magicTemplateFile", magicTemplateFile);
    settings->setValue("/currentColorMap", currentColorMap);
    settings->setValue("/imageFormat", imageFormat);
    settings->setValue("/imageRes", imageRes);
    settings->endGroup(); // sasWidgets

    delete settings;

#ifdef QTISAS
    compileWidget->saveSettings();
#endif
}

void ApplicationWindow::exportGraph(const QString& exportFilter)
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	MultiLayer *plot2D = qobject_cast<MultiLayer *>(w);
	Graph3D *plot3D = qobject_cast<Graph3D *>(w);
	if(plot2D && plot2D->isEmpty()){
		QMessageBox::critical(this, tr("QTISAS - Export Error"),
					tr("<h4>There are no plot layers available in this window!</h4>"));
		return;
	}

	if (!plot2D && !plot3D)
		return;

	ImageExportDialog *ied = new ImageExportDialog(w, this, d_extended_export_dialog);
	ied->setDirectory(imagesDirPath);
	ied->selectFile(w->objectName());
    if (exportFilter.isEmpty())
    	ied->selectNameFilter(d_image_export_filter);
	else
		ied->selectNameFilter(exportFilter);

	if ( ied->exec() != QDialog::Accepted )
		return;
	imagesDirPath = ied->directory().path();

    QString selected_filter = ImageExportDialog::filterSuffix(ied->selectedNameFilter());
	QString file_name = ied->selectedFiles()[0];
	if(!file_name.endsWith(selected_filter, Qt::CaseInsensitive))
		file_name.append(selected_filter);

    if (plot3D && selected_filter.contains(".pgf")){
        plot3D->exportVector(file_name, ied->textExportMode(), ied->sortMode());
        return;
    }

	if (plot2D && selected_filter.contains(".tex")){
		plot2D->exportTeX(file_name, ied->color(), ied->escapeStrings(), ied->exportFontSizes(), ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
		return;
	}

    if (plot2D && selected_filter.contains(".py"))
    {
        plot2D->exportMatplotlibScript(file_name);
        return;
    }

    if (plot2D && selected_filter.contains(".qti.svg"))
    {
        exportGraphAsSelfContainedSvg(plot2D, file_name);
        return;
    }

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") ||
		selected_filter.contains(".ps") || selected_filter.contains(".svg")) {
		if (plot3D)
			plot3D->exportVector(file_name, ied->textExportMode(), ied->sortMode(),
					ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
		else if (plot2D){
			if (selected_filter.contains(".svg"))
                plot2D->exportSVG(file_name);
			else
				plot2D->exportVector(file_name, true, ied->vectorResolution(), ied->color(),
						ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
		}
	} else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i = 0; i < list.count(); i++){
			if (selected_filter.contains("." + (list[i]).toLower())){
				if (plot2D)
					plot2D->exportImage(file_name, ied->quality(), ied->transparency(), ied->bitmapResolution(),
							ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor(), ied->compression());
				else if (plot3D){
					plot3D->exportImage(file_name, ied->quality(), ied->transparency(), ied->bitmapResolution(),
						ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor(), ied->compression());
				}
			}
		}
	}
}

void ApplicationWindow::exportLayer()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

    auto ied = new ImageExportDialog(w, this, d_extended_export_dialog, g);
	ied->setDirectory(imagesDirPath);
	ied->selectFile(w->objectName());
	ied->selectNameFilter(d_image_export_filter);
	if ( ied->exec() != QDialog::Accepted )
		return;
	imagesDirPath = ied->directory().path();

	QString file_name = ied->selectedFiles()[0];
    QString selected_filter = ImageExportDialog::filterSuffix(ied->selectedNameFilter());
	if(!file_name.endsWith(selected_filter, Qt::CaseInsensitive))
		file_name.append(selected_filter);

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
		g->exportVector(file_name, true, ied->vectorResolution(), ied->color(),
			ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
	else if (selected_filter.contains(".svg"))
        g->exportSVG(file_name);
	else if (selected_filter.contains(".tex"))
		g->exportTeX(file_name, ied->color(), ied->escapeStrings(), ied->exportFontSizes(), ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
    else if (selected_filter.contains(".py"))
        g->exportMatplotlibScript(file_name);
    else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i = 0; i < list.count(); i++){
			if (selected_filter.contains("." + (list[i]).toLower()))
				g->exportImage(file_name, ied->quality(), ied->transparency(), ied->bitmapResolution(),
							ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor(), ied->compression());
		}
	}
}

void ApplicationWindow::exportAllGraphs()
{
	ImageExportDialog *ied = new ImageExportDialog(nullptr, this, d_extended_export_dialog);
	ied->setWindowTitle(tr("Choose a directory to export the graphs to"));
	QStringList tmp = ied->nameFilters();
	ied->setFileMode(QFileDialog::Directory);
	ied->setNameFilters(tmp);
	ied->setLabelText(QFileDialog::FileType, tr("Output format:"));
	ied->setLabelText(QFileDialog::FileName, tr("Directory:"));
	ied->setDirectory(imagesDirPath);
    ied->selectNameFilter(d_image_export_filter);

	if ( ied->exec() != QDialog::Accepted )
		return;
	imagesDirPath = ied->directory().path();
	if (ied->selectedFiles().isEmpty())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QString output_dir = ied->selectedFiles()[0];
    QString file_suffix = ImageExportDialog::filterSuffix(ied->selectedNameFilter()).toLower();

	bool confirm_overwrite = d_confirm_overwrite;
	MultiLayer *plot2D;
	Graph3D *plot3D;

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "MultiLayer") {
			plot3D = 0;
			plot2D = (MultiLayer *)w;
			if (plot2D->isEmpty()) {
				QApplication::restoreOverrideCursor();
				QMessageBox::warning(this, tr("QTISAS - Warning"),
						tr("There are no plot layers available in window <b>%1</b>.<br>"
							"Graph window not exported!").arg(plot2D->objectName()));
				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				continue;
			}
		} else if (QString(w->metaObject()->className()) == "Graph3D") {
			plot2D = 0;
			plot3D = (Graph3D *)w;
		} else
			continue;

        if (plot3D && (file_suffix.contains(".py") || file_suffix.contains(".qti.svg")))
            continue;

		QString file_name = output_dir + "/" + w->objectName() + file_suffix;
		QFile f(file_name);
		if (f.exists() && confirm_overwrite) {
			QApplication::restoreOverrideCursor();

			QString msg = tr("A file called: <p><b>%1</b><p>already exists. ""Do you want to overwrite it?").arg(file_name);
			QMessageBox msgBox(QMessageBox::Question, tr("QTISAS - Overwrite file?"), msg,
							  QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel,
							  (ApplicationWindow *)this);
 			msgBox.exec();
			switch(msgBox.standardButton(msgBox.clickedButton())){
				case QMessageBox::Yes:
					QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				break;
				case QMessageBox::YesToAll:
					confirm_overwrite = false;
				break;
				case QMessageBox::No:
					confirm_overwrite = true;
					continue;
				break;
				case QMessageBox::Cancel:
					return;
				break;
				default:
					break;
			}
		}
		if ( !f.open( QIODevice::WriteOnly ) ) {
			QApplication::restoreOverrideCursor();
			QMessageBox::critical(this, tr("QTISAS - Export error"),
					tr("Could not write to file: <br><h4>%1</h4><p>"
						"Please verify that you have the right to write to this location!").arg(file_name));
			return;
		}
		f.close();

        if (plot2D && file_suffix.contains(".tex"))
        {
            plot2D->exportTeX(file_name, ied->color(), ied->escapeStrings(), ied->exportFontSizes(),
                              ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
            continue;
        }

        if (plot2D && file_suffix.contains(".py"))
        {
            plot2D->exportMatplotlibScript(file_name);
            continue;
        }

        if (plot2D && file_suffix.contains(".qti.svg"))
        {
            exportGraphAsSelfContainedSvg(plot2D, file_name);
            continue;
        }

		if (file_suffix.contains(".eps") || file_suffix.contains(".pdf") ||
			file_suffix.contains(".ps") || file_suffix.contains(".svg")) {
			if (plot3D)
				plot3D->exportVector(file_name, ied->textExportMode(), ied->sortMode(),
					ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
			else if (plot2D){
				if (file_suffix.contains(".svg"))
                    plot2D->exportSVG(file_name);
				else
					plot2D->exportVector(file_name, true, ied->vectorResolution(), ied->color(),
							ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor());
			}
		} else {
			QList<QByteArray> list = QImageWriter::supportedImageFormats();
			for (int i = 0; i < list.count(); i++){
				if (file_suffix.contains("." + (list[i]).toLower())) {
					if (plot2D)
						plot2D->exportImage(file_name, ied->quality(), ied->transparency(),
						ied->bitmapResolution(), ied->customExportSize(), ied->sizeUnit(),
						ied->scaleFontsFactor(), ied->compression());
					else if (plot3D)
						plot3D->exportImage(file_name, ied->quality(), ied->transparency(), ied->bitmapResolution(),
							ied->customExportSize(), ied->sizeUnit(), ied->scaleFontsFactor(), ied->compression());
				}
			}
		}
	}
	QApplication::restoreOverrideCursor();
}

QString ApplicationWindow::windowGeometryInfo(MdiSubWindow *w)
{
    bool maxMultiLayer= (w->status() == MdiSubWindow::Maximized && QString(w->metaObject()->className()) == "MultiLayer");

    int maxw=w->width();
    int maxh=w->height();

    if (QString(w->metaObject()->className()) == "MultiLayer")
    {
        //  geometry of Mul...
        maxw=0;
        maxh=0;
        foreach(Graph *g, ((MultiLayer *)w)->layersList())
        {
            if (g->pos().x()+g->geometry().width() > maxw) maxw=g->pos().x()+g->geometry().width();
            if (g->pos().y()+g->geometry().height() > maxh) maxh=g->pos().y()+g->geometry().height();

            foreach(LegendWidget *l, g->textsList())
            {
                if (l->pos().x()+l->geometry().width() > maxw) maxw=l->pos().x()+l->geometry().width();
                if (l->pos().y()+l->geometry().height() > maxh) maxh=l->pos().y()+l->geometry().height();
            }

            foreach(FrameWidget *f, g->enrichmentsList())
            {
                if (f->pos().x()+f->geometry().width() > maxw) maxw=f->pos().x()+f->geometry().width();
                if (f->pos().y()+f->geometry().height() > maxh) maxh=f->pos().y()+f->geometry().height();
            }
        }
    }

    QString s = "geometry\t";

    if (w->status() == MdiSubWindow::Maximized && !maxMultiLayer)
    {
		if (w == w->folder()->activeWindow())
			return s + "maximized\tactive\n";
		else
			return s + "maximized\n";
	}

    if (maxMultiLayer)
    {
        s += QString::number(0) + "\t";
        s += QString::number(0) + "\t";
    }
    else
    {
        s += QString::number(w->x()) + "\t";
        s += QString::number(w->y()) + "\t";
    }
    QSize minRestoreSize = w->minRestoreSize();
    if (w->status() == MdiSubWindow::Hidden &&
        minRestoreSize.width() > maxw &&
        minRestoreSize.height() > maxh)
    {
        // the window was minimized and afterwards hidden
        s += QString::number(minRestoreSize.width()) + "\t";
        s += QString::number(minRestoreSize.height()) + "\t";
	} else if (w->status() != MdiSubWindow::Minimized)
    {
        s += QString::number(maxw) + "\t";
        s += QString::number(maxh) + "\t";
    } else
    {
        s += QString::number(minRestoreSize.width()) + "\t";
        s += QString::number(minRestoreSize.height()) + "\t";
        s += "minimized\t";
    }

    bool hide = hidden(w);
    if (w == w->folder()->activeWindow() && !hide)
    {
        if (maxMultiLayer) s += "maximized\tactive\n";
        else  s += "active\n";
    }
    else if(hide)
        s += "hidden\n";
    else
        s += "\n";
	return s;
}

void ApplicationWindow::restoreWindowGeometry(ApplicationWindow *app, MdiSubWindow *w, const QString &s,
                                              MdiSubWindow *previouslyMaximized)
{
	if (qobject_cast<Graph3D *>(w)) w->hide();

	QString caption = w->objectName();

	if (s.contains ("minimized"))
    {
		QStringList lst = s.split("\t");
		if (lst.count() > 4)
        {
			int width = lst[3].toInt();
			int height = lst[4].toInt();
			if(width > 0 && height > 0)
				w->resize(width, height);
		}
		w->setStatus(MdiSubWindow::Minimized);
		app->setListView(caption, tr("Minimized"));
	}
    else if (s.contains ("maximized"))
    {
        w->folder()->setActiveWindow(w);
        w->setMaximized();
    }
    else
    {
		QStringList lst = s.split("\t");
		if (lst.count() > 4)
        {
			w->resize(lst[3].toInt(), lst[4].toInt());
			w->move(lst[1].toInt(), lst[2].toInt());
		}
		w->setStatus(MdiSubWindow::Normal);
		if (lst.count() > 5)
        {
			if (lst[5] == "hidden") app->hideWindow(w);
		}
	}

	if (s.contains ("active"))
    {
		Folder *f = w->folder();
		if (f) f->setActiveWindow(w);
	}

    if (previouslyMaximized)
    {
        previouslyMaximized->folder()->setActiveWindow(previouslyMaximized);
        previouslyMaximized->setMaximized();
    }
}

Folder* ApplicationWindow::projectFolder()
{
	return ((FolderListItem *)folders->topLevelItem(0))->folder();
}

bool ApplicationWindow::saveProject(bool compress)
{
	if (projectname == "untitled" || projectname.endsWith(".opj", Qt::CaseInsensitive) ||
		projectname.endsWith(".ogm", Qt::CaseInsensitive) || projectname.endsWith(".ogw", Qt::CaseInsensitive)
		|| projectname.endsWith(".ogg", Qt::CaseInsensitive)){
		saveProjectAs();
		return false;
	}

	saveFolder(projectFolder(), projectname, compress);
	savedProject();

	if (autoSave){
		if (savingTimerId)
			killTimer(savingTimerId);
		savingTimerId=startTimer(autoSaveTime*60000);
	} else
		savingTimerId=0;

	QApplication::restoreOverrideCursor();
	return true;
}

QString ApplicationWindow::getFileName(QWidget *parent, const QString &caption, const QString &dir,
                                       const QString &filter, QString *selectedFilter, bool save)
{
	QFileDialog fd(parent, caption, dir, filter);
	if (filter.contains(";"))
		fd.setNameFilters(filter.split(";"));

	if (save)
		fd.setAcceptMode(QFileDialog::AcceptSave);
	else
		fd.setAcceptMode(QFileDialog::AcceptOpen);

	fd.setFileMode(QFileDialog::AnyFile);

	if (fd.exec() != QDialog::Accepted )
		return QString();

	if (fd.selectedFiles().isEmpty())
		return QString();

	if (selectedFilter)
		*selectedFilter = fd.selectedNameFilter();

    QString file_name = fd.selectedFiles()[0];

    if (save)
    {
        // Extract the extension from a "*.ext" pattern in the filter text
        QString selected_filter = fd.selectedNameFilter();
        static const QRegularExpression extRe(R"(\*(\.[\w.]+))");
        QRegularExpressionMatch match = extRe.match(selected_filter);
        if (match.hasMatch())
        {
            QString ext = match.captured(1);
            if (!file_name.endsWith(ext, Qt::CaseInsensitive))
            {
                // macOS's save panel can mangle the auto-appended extension
                // (e.g. "a" -> "a.qtif" instead of "a.qti") -- cut at "." + ext
                int cut = static_cast<int>(file_name.lastIndexOf(ext, -1, Qt::CaseInsensitive));
                QString base = cut >= 0 ? file_name.left(cut) : file_name;
                file_name = base + ext;
            }
        }

		QFile file(file_name);
		if(!file.open(QIODevice::WriteOnly)){
			QMessageBox::critical(parent, tr("QTISAS - Export error"),
			tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
			return QString();
		}
		file.close();
		file.remove();
	}

    return file_name;
}

QString ApplicationWindow::getSaveProjectName(const QString& fileName, bool *compress, int scope)
{
	QString fn = fileName;
	if (fileName.isEmpty()){
        QString filter = tr("QtiSAS[QtiPlot] project") + " (*.qti);";
        filter += tr("Compressed QtiSAS[QtiPlot] project") + " (*.qti.gz)";

		QString windowTitle = tr("Save Project As");
		if (scope == 1)
			windowTitle = tr("Save Folder As");
		else if (scope == 2)
			windowTitle = tr("Save Window As");

		QString selectedFilter;
        fn = getFileName(this, windowTitle, workingDir, filter, &selectedFilter, true);

		if (selectedFilter.contains(".gz"))
			*compress = true;
	}

	if (!fn.isEmpty()){
		QFileInfo fi(fn);
		workingDir = fi.absolutePath();
		if (fn.endsWith(".qti.gz", Qt::CaseInsensitive))
			fn.remove(".gz");
		if (!fn.endsWith(".qti", Qt::CaseInsensitive))
			fn.append(".qti");
	}
	return fn;
}

void ApplicationWindow::saveProjectAs(const QString& fileName, bool compress)
{
	QString fn = getSaveProjectName(fileName, &compress);
	if (!fn.isEmpty()){
		projectname = fn;
		if (saveProject(compress)){
			updateRecentProjectsList(projectname);

			QString baseName = QFileInfo(fn).baseName();
			FolderListItem *item = (FolderListItem *)folders->topLevelItem(0);
			item->setText(0, baseName);
			item->folder()->setObjectName(baseName);
		}
	}
}

void ApplicationWindow::saveWindowAs(const QString& fileName, bool compress)
{
	MdiSubWindow *w = this->activeWindow();
	if (!w)
		return;

	QString fn = getSaveProjectName(fileName, &compress, 2);
	if (!fn.isEmpty()){
		/*if (w->inherits("Table") && fn.endsWith(".ogw")){
			ImportExportPlugin *ep = exportPlugin("ogw");
			if (!ep)
				return;
			ep->exportTable((Table *)w, fn, true, true, false);
			return;
		}*/
		if (saveWindow(w, fn, compress))
			updateRecentProjectsList(fn);
	}
}

bool ApplicationWindow::saveWindow(MdiSubWindow *w, const QString& fn, bool compress)
{
	if (!w)
		return false;

	QFile f(fn);
	if ( !f.open( QIODevice::WriteOnly ) ){
		QMessageBox::about(this, tr("QTISAS - File save error"), tr("The file: <br><b>%1</b> is opened in read-only mode").arg(fn));
		return false;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << "QtiPlot 0.9.8 project file structure saved in QtiSAS v.";
    t << QString::number(maj_version) + "." + QString::number(min_version) + "." + QString::number(patch_version);
    t << "\n";
	t << "<scripting-lang>\t" + QString(scriptEnv->objectName()) + "\n";

	int windows = 1;
	QStringList tbls;

    QList<MdiSubWindow *> tables = tableList();

    QList<MdiSubWindow *> notes = noteList();
    QStringList nts;

    if (qobject_cast<MultiLayer *>(w))
    {
        tbls = multilayerDependencies(w);

        QStringList lst;
        for (int mw = 0; mw < tables.count(); mw++)
            lst << tables[mw]->objectName();

        QStringList lstSession;
        for (int i = 0; i < tbls.count(); i++)
        {
            QString base = tbls[i];
            base = base.split("-global-", Qt::SkipEmptyParts)[0];
            if (tbls.contains(base + "-session"))
                continue;
            if (lst.contains(base + "-session") && !lstSession.contains(base + "-session"))
                lstSession << base + "-session";
        }

        for (int i = 0; i < tbls.count(); i++)
        {
            QString base = tbls[i];
            base = base.split("-global-", Qt::SkipEmptyParts)[0];
            if (nts.contains(base + "-statistics"))
                continue;
            if (checkNoteExistence(base + "-statistics"))
                nts << base + "-statistics";
        }

        tbls << lstSession;
        windows = tbls.count() + 1 + nts.count();
    }

    QStringList matrixls;
    if (qobject_cast<MultiLayer *>(w))
    {
        QList<Graph *> layers = qobject_cast<MultiLayer *>(w)->layersList();
        foreach(Graph *g, layers)
        {
            if (!g) continue;
            if (g->curvesList().size()==0)  continue;
            for (int i=0; i<g->curvesList().size();i++)
            {
                if( g->curvesList()[i]->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
                {
                    Spectrogram *sp = (Spectrogram *)g->curvesList()[i];
                    matrixls<<sp->matrix()->objectName();
                }
            }
        }
    }
    windows += matrixls.count();

	Graph3D *g = qobject_cast<Graph3D *>(w);
	if (g && (g->table() || g->matrix()))
		windows++;

	t << "<windows>\t" + QString::number(windows) + "\n";
	f.close();

	if (!f.isOpen())
        (void)f.open(QIODevice::Append);

    foreach (QString s, tbls)
        foreach (MdiSubWindow *t, tables)
            if (t->objectName() == s)
                t->save(fn, windowGeometryInfo(t));

    foreach (QString s, nts)
        foreach (MdiSubWindow *n, notes)
            if (n->objectName() == s)
                n->save(fn, windowGeometryInfo(n));

	if (g)
    {
		Matrix *m = g->matrix();

		if (m)
			m->save(fn, windowGeometryInfo(m));
		Table *t = g->table();
		if (t)
			t->save(fn, windowGeometryInfo(t));
	}

    if (matrixls.size()>0) for (int i=0; i<matrixls.size();i++)
    {
        Matrix *m = matrix(matrixls[i]);
        if (m) m->save(fn, windowGeometryInfo(m));
    }

	w->save(fn, windowGeometryInfo(w));
	f.close();

	if (compress)
        file_compress(fn.toLocal8Bit().data(), QString("wb9").toLocal8Bit().data());

	QApplication::restoreOverrideCursor();
	return true;
}

void ApplicationWindow::saveNoteAs()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;
	w->exportASCII();
}

void ApplicationWindow::increaseNoteIndent()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

    w->setTabStopDistance(w->currentEditor()->tabStopDistance() + 5);
	modifiedProject();
}

void ApplicationWindow::decreaseNoteIndent()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

    w->setTabStopDistance(w->currentEditor()->tabStopDistance() - 5);
	modifiedProject();
}

void ApplicationWindow::showNoteLineNumbers(bool show)
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;
	w->showLineNumbers(show);
	modifiedProject();
}

void ApplicationWindow::noteFindDialogue()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

	w->currentEditor()->showFindDialog();
}

void ApplicationWindow::noteFindNext()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

	w->currentEditor()->findNext();
}

void ApplicationWindow::noteFindPrev()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

	w->currentEditor()->findPrevious();
}

void ApplicationWindow::noteReplaceDialogue()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;

	w->currentEditor()->showFindDialog(true);
}

void ApplicationWindow::renameCurrentNoteTab()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;
	w->renameCurrentTab();
	modifiedProject();
}

void ApplicationWindow::addNoteTab()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;
	w->addTab();
	modifiedProject();
}

void ApplicationWindow::closeNoteTab()
{
	Note* w = (Note*)activeWindow(NoteWindow);
	if (!w)
		return;
	w->removeTab();
	modifiedProject();
}

void ApplicationWindow::saveAsTemplate(MdiSubWindow *w, const QString &fileName)
{
    if (!w)
    {
        w = activeWindow();
        if (!w)
            return;
    }

    QString fn = fileName;
    if (fn.isEmpty())
    {
        QString filter;
        if (QString(w->metaObject()->className()) == "Matrix")
            filter = tr("QtiPlot Matrix Template") + " (*.qmt)";
        else if (QString(w->metaObject()->className()) == "MultiLayer")
            filter = tr("QtiPlot 2D Graph Template") + " (*.qpt)";
        else if (w->inherits("Table"))
            filter = tr("QtiPlot Table Template") + " (*.qtt)";
        else if (QString(w->metaObject()->className()) == "Graph3D")
            filter = tr("QtiPlot 3D Surface Template") + " (*.qst)";

        QString selectedFilter;
        fn = getFileName(this, tr("Save Window As Template"), templatesDir + w->objectName(), filter, &selectedFilter,
                         true);
        if (!fn.isEmpty())
        {
            QFileInfo fi(fn);
            QString baseName = fi.fileName();
            if (!baseName.contains("."))
            {
                selectedFilter = selectedFilter.right(5).left(4);
                fn.append(selectedFilter);
            }
        }
        else
            return;
    }

    if (!QFileInfo(fn).isAbsolute())
        fn = templatesDir + fn;

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly))
    {
		QMessageBox::critical(this, tr("QTISAS - Export error"),
            tr("Could not write to file:<br><h4>%1</h4><p>Check write permissions for this location").arg(fn));
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QTextStream t(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << "QtiPlot " + QString::number(0) + "." + QString::number(8) + "." + QString::number(9) + " template file\n";
    f.close();
    w->save(fn, windowGeometryInfo(w), true);
    QApplication::restoreOverrideCursor();

    QDir d(templatesDir);
    magicList = d.entryList(QStringList() << "*.qpt");
}

void ApplicationWindow::rename()
{
	MdiSubWindow* m = activeWindow();
	if (!m)
		return;

	RenameWindowDialog *rwd = new RenameWindowDialog(this);
	rwd->setWidget(m);
	rwd->exec();
}

void ApplicationWindow::renameWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MdiSubWindow *w = it->window();
	if (!w)
		return;

	RenameWindowDialog *rwd = new RenameWindowDialog(this);
	rwd->setWidget(w);
	rwd->exec();
}

void ApplicationWindow::renameWindow(QTreeWidgetItem *item, int, const QString &text)
{
	if (!item)
		return;

	MdiSubWindow *w = ((WindowListItem *)item)->window();
	if (!w || text == w->objectName())
		return;

	if(!setWindowName(w, text))
        item->setText(0, w->objectName());
}

bool ApplicationWindow::setWindowName(MdiSubWindow *w, const QString &text, bool interactive)
{
	if (!w)
		return false;

	QString name = w->objectName();
	if (name == text)
		return true;

	QString newName = text;
    if (newName.isEmpty())
    {
        if (interactive)
            QMessageBox::critical(this, tr("QTISAS - Error"), tr("Please enter a valid name!"));
		return false;
    }
    else if (QString(newName).remove("-").contains(REGEXPS::nonalphanumeric))
    {
        if (interactive)
            QMessageBox::critical(this, tr("QTISAS - Error"),
                                  tr("The name you chose is not valid: only letters and digits are allowed!") + "<p>" +
                                      tr("Please choose another name!"));
		return false;
	}

	while(alreadyUsedName(newName)){
        if (interactive)
            QMessageBox::critical(this, tr("QTISAS - Error"),
                                  tr("Name <b>%1</b> already exists!").arg(newName) + "<p>" +
                                      tr("Please choose another name!"));
		return false;
	}

	if (w->inherits("Table"))
		updateTableNames(name, newName);
	else if (QString(w->metaObject()->className()) == "Matrix")
		changeMatrixName(name, newName);

	w->setName(newName);
	renameListViewItem(name, newName);
	updateCompleter(name, false, newName);
	emit modified();
	return true;
}

QStringList ApplicationWindow::columnsList(Table::PlotDesignation plotType)
{
	QStringList list;
	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (!w->inherits("Table"))
			continue;

		Table *t = (Table *)w;
		for (int i=0; i < t->numCols(); i++)
		{
			if (t->colPlotDesignation(i) == plotType || plotType == Table::All)
				list << QString(t->objectName()) + "_" + t->colLabel(i);
		}
	}
	return list;
}

void ApplicationWindow::showCurvesDialog()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    if (w->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Error"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

    if (g->isPiePlot())
    {
		QMessageBox::warning(this,tr("QTISAS - Error"),
				tr("This functionality is not available for pie plots!"));
    }
    else
    {
        auto crvDialog = new CurvesDialog(this);
		crvDialog->setAttribute(Qt::WA_DeleteOnClose);
		crvDialog->setGraph(g);
		crvDialog->resize(d_add_curves_dialog_size);
		crvDialog->setModal(true);
		crvDialog->show();
	}
}

bool ApplicationWindow::hasTable()
{
    Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
            if (w->inherits("Table"))
                return true;
		}
		f = f->folderBelow();
	}
	return false;
}

QStringList ApplicationWindow::tableNames()
{
    QStringList lst = QStringList();
    Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
            if (w->inherits("Table"))
                lst << w->objectName();
		}
		f = f->folderBelow();
	}
	return lst;
}

QList<MdiSubWindow *> ApplicationWindow::tableList()
{
    QList<MdiSubWindow *> lst;
    Folder *f = projectFolder();
    while (f)
    {
        foreach (MdiSubWindow *w, f->windowsList())
            if (w->inherits("Table"))
                lst << w;
        f = f->folderBelow();
    }
    return lst;
}

QList<MdiSubWindow *> ApplicationWindow::matrixList()
{
    QList<MdiSubWindow *> lst;
    Folder *f = projectFolder();
    while (f)
    {
        foreach (MdiSubWindow *w, f->windowsList())
            if (w->inherits("Matrix"))
                lst << w;
        f = f->folderBelow();
    }
    return lst;
}

QList<MdiSubWindow *> ApplicationWindow::noteList()
{
    QList<MdiSubWindow *> lst;
    Folder *f = projectFolder();
    while (f)
    {
        foreach (MdiSubWindow *w, f->windowsList())
            if (w->inherits("Note"))
                lst << w;
        f = f->folderBelow();
    }
    return lst;
}

QList<MdiSubWindow *> ApplicationWindow::multilayerList()
{
    QList<MdiSubWindow *> lst;
    Folder *f = projectFolder();
    while (f)
    {
        foreach (MdiSubWindow *w, f->windowsList())
            if (w->inherits("MultiLayer"))
                lst << w;
        f = f->folderBelow();
    }
    return lst;
}

QStringList ApplicationWindow::findTableListByLabel(const QString &tableLabel)
{
    QStringList list;
    foreach (MdiSubWindow *w, tableList())
        if (w->windowLabel().contains(tableLabel))
            list << w->name();
    return list;
}

QStringList ApplicationWindow::findMatrixListByLabel(const QString &matrixLabel)
{
    QStringList list;
    foreach (MdiSubWindow *w, matrixList())
        if (w->windowLabel().contains(matrixLabel))
            list << w->name();
    return list;
}

AssociationsDialog* ApplicationWindow::showPlotAssociations(int curve)
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return nullptr;

    Graph *g = w->activeLayer();
    if (!g)
        return nullptr;

    auto ad = new AssociationsDialog(this);
    ad->setAttribute(Qt::WA_DeleteOnClose);
    ad->setGraph(g);
    ad->initTablesList(tableList(), curve);
    ad->show();
    return ad;
}

void ApplicationWindow::showTitleDialog()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (QString(w->metaObject()->className()) == "MultiLayer"){
		Graph* g = ((MultiLayer*)w)->activeLayer();
		if (g){
            auto *td = new TextDialog(TextDialog::LayerTitle, this, Qt::WindowFlags());
			td->setGraph(g);
			td->exec();
		}
	} else if (QString(w->metaObject()->className()) == "Graph3D") {
		Plot3DDialog* pd = (Plot3DDialog*)showPlot3dDialog();
		if (pd)
			pd->showTitleTab();
	}
}

void ApplicationWindow::showAxisTitleDialog()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

    auto *td = new TextDialog(TextDialog::AxisTitle, this, Qt::WindowFlags());
    td->setGraph(g);
    td->exec();
}

ExportDialog* ApplicationWindow::showExportASCIIDialog()
{
    MdiSubWindow* t = activeWindow();
    if (!t)
		return 0;
	if (!qobject_cast<Matrix*>(t) && !t->inherits("Table"))
		return 0;

    ExportDialog* ed = new ExportDialog(t, this, true);
	ed->open();
	return ed;
}

void ApplicationWindow::exportAllTables(const QString& dir, const QString& filter, const QString& sep, bool colNames, bool colComments, bool expSelection, QString wildCard)
{
	if (dir.isEmpty())
		return;

    QRegularExpression rx0 = REGEXPS::wildcardToRegex(wildCard);

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	workingDir = dir;

	bool confirmOverwrite = d_confirm_overwrite;
	bool success = true;
	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (w->inherits("Table") || QString(w->metaObject()->className()) == "Matrix"){
            if (!rx0.match(w->objectName()).hasMatch())
                continue;
			QString fileName = dir + "/" + w->objectName() + filter;
			QFile f(fileName);
			if (f.exists(fileName) && confirmOverwrite){
				QApplication::restoreOverrideCursor();
                switch (QMessageBox::question(this, tr("QTISAS - Overwrite file?"),
                                              tr("A file called: <p><b>%1</b><p>already exists. "
                                                 "Do you want to overwrite it?")
                                                  .arg(fileName),
                                              QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::Cancel))
				{
                case QMessageBox::Yes:
						if (w->inherits("Table"))
							success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
						else if (QString(w->metaObject()->className()) == "Matrix")
							success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);
						break;
                case QMessageBox::YesAll:
						confirmOverwrite = false;
						if (w->inherits("Table"))
							success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
						else if (QString(w->metaObject()->className()) == "Matrix")
							success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);
						break;
                default:
						return;
						break;
				}
			} else if (w->inherits("Table"))
				success = ((Table*)w)->exportASCII(fileName, sep, colNames, colComments, expSelection);
			  else if (QString(w->metaObject()->className()) == "Matrix")
				success = ((Matrix*)w)->exportASCII(fileName, sep, expSelection);

			if (!success)
				break;
		}
	}
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::showRowsDialog()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	bool ok;
	int rows = QInputDialog::getInt(this, tr("QTISAS - Enter rows number"), tr("Rows"),
			t->numRows(), 0, 1000000, 1, &ok);
	if ( ok )
		t->resizeRows(rows);
}

void ApplicationWindow::showDeleteRowsDialog()
{
    Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	bool ok;
	int start_row = QInputDialog::getInt(this, tr("QTISAS - Delete rows"), tr("Start row"),
                    1, 1, t->numRows(), 1, &ok);
    if (ok){
        int end_row = QInputDialog::getInt(this, tr("QTISAS - Delete rows"), tr("End row"),
                        t->numRows(), 1, t->numRows(), 1, &ok);
        if (ok)
            t->deleteRows(start_row, end_row);
	}
}

void ApplicationWindow::showColsDialog()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	bool ok;
	int cols = QInputDialog::getInt(this, tr("QTISAS - Enter columns number"), tr("Columns"),
			t->numCols(), 0, 1000000, 1, &ok);
	if ( ok )
		t->resizeCols(cols);
}

void ApplicationWindow::showColumnValuesDialog()
{
	Table *w = (Table*)activeWindow(TableWindow);
	if (!w)
		return;

    if (w->selectedColumns().count()>0 || !w->table()->currentSelection().isEmpty()) {
        SetColValuesDialog* vd = new SetColValuesDialog(scriptEnv, this);
        vd->setTable(w);
        if (d_completion)
            vd->setCompleter(d_completer);
        vd->exec();
    } else
        QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a column first!"));
}

void ApplicationWindow::showExtractDataDialog()
{
	Table *w = (Table*)activeWindow(TableWindow);
	if (!w)
		return;

	ExtractDataDialog* edd = new ExtractDataDialog(scriptEnv, this);
	edd->setTable(w);
	if (d_completion)
		edd->setCompleter(d_completer);
	edd->exec();
}

void ApplicationWindow::recalculateTable()
{
	MdiSubWindow* w = activeWindow();
	if (!w)
            return;

        if (qobject_cast<TableStatistics *>(w))
            ((TableStatistics*)w)->update();
        else if (qobject_cast<Table *>(w))
            ((Table*)w)->calculate();
        else if (qobject_cast<Matrix *>(w))
            ((Matrix*)w)->calculate();
}

void ApplicationWindow::sortActiveTable()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	t->sortTableDialog();
}

void ApplicationWindow::sortSelection()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	t->sortColumnsDialog();
}

void ApplicationWindow::normalizeActiveTable()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	if (int(t->selectedColumns().count())>0)
		t->normalize();
	else
		QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a column first!"));
}

void ApplicationWindow::normalizeSelection()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	if (int(t->selectedColumns().count())>0)
		t->normalizeSelection();
	else
		QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a column first!"));
}

void ApplicationWindow::correlate()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2){
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select two columns for this operation!"));
		return;
	}

	Correlation *cor = new Correlation(this, t, s[0], s[1]);
	cor->run();
	delete cor;
}

void ApplicationWindow::autoCorrelate()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList s = t->selectedColumns();
	if ((int)s.count() != 1)
	{
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select exactly one columns for this operation!"));
		return;
	}

	Correlation *cor = new Correlation(this, t, s[0], s[0]);
	cor->run();
	delete cor;
}

void ApplicationWindow::convolute()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2)
	{
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));
		return;
	}

	Convolution *cv = new Convolution(this, t, s[0], s[1]);
	cv->run();
	delete cv;
}

void ApplicationWindow::deconvolute()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2)
	{
		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));
		return;
	}

	Deconvolution *dcv = new Deconvolution(this, t, s[0], s[1]);
	dcv->run();
	delete dcv;
}

void ApplicationWindow::showColStatistics()
{
    auto *t = (Table *)activeWindow(TableWindow);
    if (!t)
        return;

    QList<int> targets;
    for (int i = 0; i < t->numCols(); i++)
        if (t->isColumnSelected(i))
            targets << i;

    MySelection select = t->getSelection();
    newTableStatistics(t, TableStatistics::column, targets, select.topRow(), select.bottomRow(),
                       t->objectName() + "-ColStats")
        ->showNormal();
}

void ApplicationWindow::showRowStatistics()
{
    auto *t = (Table *)activeWindow(TableWindow);
    if (!t)
        return;

    QList<int> targets;
    for (int i = 0; i < t->numRows(); i++)
        if (t->isRowSelected(i))
            targets << i;

    MySelection select = t->getSelection();
    newTableStatistics(t, TableStatistics::row, targets, select.leftColumn(), select.rightColumn(),
                       t->objectName() + "-RowStats")
        ->showNormal();
}

void ApplicationWindow::showColMenu(int c)
{
	Table *w = (Table*)activeWindow(TableWindow);
	if (!w)
		return;

	QMenu contextMenu(this);
	QMenu plot(this);
	QMenu specialPlot(this);
	QMenu specialBarMenu(this);
	QMenu fill(this);
	QMenu sorting(this);
	QMenu colType(this);
	QMenu panels(this);
	QMenu gridMenu(this);
	QMenu stat(this);
	QMenu norm(this);

	if (w->selectedColumns().count() == 1){
		w->setSelectedCol(c);
		plot.addAction(actionPlotL);
		plot.addAction(actionPlotP);
		plot.addAction(actionPlotLP);

		specialPlot.addAction(actionPlotVerticalDropLines);
		specialPlot.addAction(actionPlotSpline);
		specialPlot.addAction(actionPlotVertSteps);
		specialPlot.addAction(actionPlotHorSteps);
		specialPlot.addSeparator();
        specialPlot.addAction(actionAddZoomPlot);
		specialPlot.setTitle(tr("Special Line/Symb&ol"));
		plot.addMenu(&specialPlot);
		plot.addSeparator();
		plot.addAction(actionPlotVerticalBars);
		plot.addAction(actionPlotHorizontalBars);
		specialBarMenu.addAction(actionStackBars);
		specialBarMenu.addAction(actionStackColumns);
		specialBarMenu.setTitle(tr("Spec&ial Bar/Column"));
		plot.addMenu (&specialBarMenu);
		plot.addAction(actionPlotArea);
		plot.addAction(actionPlotPie);
		plot.addSeparator();

		plot.addAction(actionPlot3DRibbon);
		plot.addAction(actionPlot3DBars);
		plot.addAction(actionPlot3DScatter);
		plot.addAction(actionPlot3DTrajectory);

		plot.addSeparator();

		stat.addAction(actionBoxPlot);
		stat.addAction(actionPlotHistogram);
		stat.addAction(actionPlotStackedHistograms);
		stat.addSeparator();
		stat.addAction(actionStemPlot);
		stat.setTitle(tr("Statistical &Graphs"));
		plot.addMenu(&stat);

		plot.setTitle(tr("&Plot"));
		contextMenu.addMenu(&plot);
		contextMenu.addSeparator();

		contextMenu.addAction(QIcon(":/cut.png"),tr("Cu&t"), w, SLOT(cutSelection()));
		contextMenu.addAction(QIcon(":/copy.png"),tr("&Copy"), w, SLOT(copySelection()));
		contextMenu.addAction(QIcon(":/paste.png"),tr("Past&e"), w, SLOT(pasteSelection()));
		contextMenu.addSeparator();

		QAction * xColID=colType.addAction(QIcon(":/x_col.png"), tr("&X"), this, SLOT(setXCol()));
		xColID->setCheckable(true);
		QAction * yColID=colType.addAction(QIcon(":/y_col.png"), tr("&Y"), this, SLOT(setYCol()));
        yColID->setCheckable(true);
		QAction * zColID=colType.addAction(QIcon(":/z_col.png"), tr("&Z"), this, SLOT(setZCol()));
        zColID->setCheckable(true);
        colType.addSeparator();
		QAction * labelID = colType.addAction(QIcon(":/set_label_col.png"), tr("&Label"), this, SLOT(setLabelCol()));
        labelID->setCheckable(true);
		QAction * noneID=colType.addAction(QIcon(":/disregard_col.png"), tr("&None"), this, SLOT(disregardCol()));
        noneID->setCheckable(true);
        colType.addSeparator();
        QAction * xErrColID =colType.addAction(tr("X E&rror"), this, SLOT(setXErrCol()));
        xErrColID->setCheckable(true);
		QAction * yErrColID = colType.addAction(QIcon(":/errors.png"), tr("Y &Error"), this, SLOT(setYErrCol()));
        yErrColID->setCheckable(true);
        colType.addSeparator();

        if (w->colPlotDesignation(c) == Table::X)
            xColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::Y)
            yColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::Z)
            zColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::xErr)
            xErrColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::yErr)
            yErrColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::Label)
            labelID->setChecked(true);
        else
            noneID->setChecked(true);

        colType.addAction(actionReadOnlyCol);
        actionReadOnlyCol->setCheckable(true);
        actionReadOnlyCol->setChecked(w->isReadOnlyColumn(c));

		colType.setTitle(tr("Set As"));
		contextMenu.addMenu(&colType);

		if (w){
			contextMenu.addSeparator();

			contextMenu.addAction(actionShowColumnValuesDialog);
			contextMenu.addAction(actionTableRecalculate);
			fill.addAction(actionSetAscValues);
			fill.addAction(actionSetRandomValues);
			fill.addAction(actionSetRandomNormalValues);
			fill.setTitle(tr("&Fill Column With"));
			contextMenu.addMenu(&fill);

			norm.addAction(tr("&Column"), w, SLOT(normalizeSelection()));
			norm.addAction(actionNormalizeTable);
			norm.setTitle(tr("&Normalize"));
			contextMenu.addMenu(& norm);

			contextMenu.addSeparator();
			contextMenu.addAction(actionFrequencyCount);
			contextMenu.addAction(actionShowColStatistics);

			contextMenu.addSeparator();

			contextMenu.addAction(QIcon(":/erase.png"), tr("Clea&r"), w, SLOT(clearSelection()));
			contextMenu.addAction(QIcon(":/delete_column.png"), tr("&Delete"), w, SLOT(removeCol()));
			contextMenu.addAction(actionHideSelectedColumns);
			contextMenu.addAction(actionShowAllColumns);
			contextMenu.addSeparator();
			contextMenu.addAction(QIcon(":/insert_column.png"), tr("&Insert"), w, SLOT(insertCol()));
			contextMenu.addAction(actionAddColToTable);
			contextMenu.addSeparator();

			sorting.addAction(QIcon(":/sort_ascending.png"), tr("&Ascending"), w, SLOT(sortColAsc()));
			sorting.addAction(QIcon(":/sort_descending.png"), tr("&Descending"), w, SLOT(sortColDesc()));
			sorting.setTitle(tr("Sort Colu&mn"));
			contextMenu.addMenu(&sorting);

			contextMenu.addAction(actionSortTable);
		}
		contextMenu.addSeparator();
		contextMenu.addAction(actionAdjustColumnWidth);
		contextMenu.addAction(actionShowColumnOptionsDialog);
	} else if (w->selectedColumns().count() > 1){
		plot.addAction(actionPlotL);
		plot.addAction(actionPlotP);
		plot.addAction(actionPlotLP);

		specialPlot.addAction(actionPlotVerticalDropLines);
		specialPlot.addAction(actionPlotSpline);
		specialPlot.addAction(actionPlotVertSteps);
		specialPlot.addAction(actionPlotHorSteps);
		specialPlot.addSeparator();
        specialPlot.addAction(actionPlotDoubleYAxis);
        specialPlot.addAction(actionWaterfallPlot);
        specialPlot.addAction(actionAddZoomPlot);
		specialPlot.setTitle(tr("Special Line/Symb&ol"));
		plot.addMenu(&specialPlot);
		plot.addSeparator();
		plot.addAction(actionPlotVerticalBars);
		plot.addAction(actionPlotHorizontalBars);

		specialBarMenu.addAction(actionStackBars);
		specialBarMenu.addAction(actionStackColumns);
		specialBarMenu.setTitle(tr("Spec&ial Bar/Column"));
		plot.addMenu (&specialBarMenu);;

		plot.addAction(actionPlotArea);
		plot.addAction(actionPlotVectXYXY);
		plot.addAction(actionPlotVectXYAM);
		plot.addSeparator();
		stat.addAction(actionBoxPlot);
		stat.addAction(actionPlotHistogram);
		stat.addAction(actionPlotStackedHistograms);
		stat.addSeparator();
		stat.addAction(actionStemPlot);
		stat.setTitle(tr("Statistical &Graphs"));
		plot.addMenu(&stat);

		panels.addAction(actionPlot2VerticalLayers);
		panels.addAction(actionPlot2HorizontalLayers);
		panels.addAction(actionPlot4Layers);
		panels.addAction(actionPlotStackedLayers);
		panels.addAction(actionCustomLayout);
		panels.setTitle(tr("Pa&nel"));
		plot.addMenu(&panels);

		gridMenu.addAction(actionVertSharedAxisLayers);
		gridMenu.addAction(actionHorSharedAxisLayers);
		gridMenu.addAction(actionSharedAxesLayers);
		gridMenu.addAction(actionStackSharedAxisLayers);
		gridMenu.addAction(actionCustomSharedAxisLayers);
		gridMenu.setTitle(tr("Shared A&xes Panel"));
		plot.addMenu(&gridMenu);

		plot.setTitle(tr("&Plot"));
		contextMenu.addMenu(&plot);
		contextMenu.addSeparator();
		contextMenu.addAction(QIcon(":/cut.png"),tr("Cu&t"), w, SLOT(cutSelection()));
		contextMenu.addAction(QIcon(":/copy.png"),tr("&Copy"), w, SLOT(copySelection()));
		contextMenu.addAction(QIcon(":/paste.png"),tr("Past&e"), w, SLOT(pasteSelection()));
		contextMenu.addSeparator();

		if (w){
			contextMenu.addAction(QIcon(":/erase.png"),tr("Clea&r"), w, SLOT(clearSelection()));
			contextMenu.addAction(QIcon(":/close.png"),tr("&Delete"), w, SLOT(removeCol()));
			contextMenu.addAction(actionHideSelectedColumns);
			contextMenu.addAction(actionShowAllColumns);
			contextMenu.addSeparator();
			contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
			contextMenu.addAction(actionAddColToTable);
			contextMenu.addSeparator();
		}

		colType.addAction(actionSetXCol);
		colType.addAction(actionSetYCol);
		colType.addAction(actionSetZCol);
		colType.addSeparator();
		colType.addAction(actionSetLabelCol);
		colType.addAction(actionDisregardCol);
		colType.addSeparator();
		colType.addAction(actionSetXErrCol);
		colType.addAction(actionSetYErrCol);
		colType.addSeparator();
		colType.addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
		colType.addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));
		colType.setTitle(tr("Set As"));
		contextMenu.addMenu(&colType);

		if (w){
			contextMenu.addSeparator();

			fill.addAction(actionSetAscValues);
			fill.addAction(actionSetRandomValues);
			fill.addAction(actionSetRandomNormalValues);
			fill.setTitle(tr("&Fill Columns With"));
			contextMenu.addMenu(&fill);

			norm.addAction(actionNormalizeSelection);
			norm.addAction(actionNormalizeTable);
			norm.setTitle(tr("&Normalize"));
			contextMenu.addMenu(&norm);

			contextMenu.addSeparator();
			sorting.addAction(QIcon(":/sort_ascending.png"), tr("&Ascending"), w, SLOT(sortColAsc()));
			sorting.addAction(QIcon(":/sort_descending.png"), tr("&Descending"), w, SLOT(sortColDesc()));
			sorting.addAction(actionSortSelection);
			sorting.setTitle("&" + tr("Sort Columns"));
			contextMenu.addMenu(&sorting);
			contextMenu.addAction(actionSortTable);
			contextMenu.addSeparator();
			contextMenu.addAction(actionShowColStatistics);
			contextMenu.addAction(actionShowRowStatistics);
			contextMenu.addSeparator();
			contextMenu.addAction(actionAdjustColumnWidth);
		}
	}

	QPoint posMouse=QCursor::pos();
	contextMenu.exec(posMouse);
}

void ApplicationWindow::plotVerticalSharedAxisLayers()
{
	multilayerPlot(1, 2, defaultCurveStyle, MultiLayer::AlignCanvases);
}

void ApplicationWindow::plotHorizontalSharedAxisLayers()
{
	multilayerPlot(2, 1, defaultCurveStyle, MultiLayer::AlignCanvases);
}

void ApplicationWindow::plotSharedAxesLayers()
{
	multilayerPlot(2, 2, defaultCurveStyle, MultiLayer::AlignCanvases);
}

void ApplicationWindow::plotStackSharedAxisLayers()
{
	multilayerPlot(1, -1, defaultCurveStyle, MultiLayer::AlignCanvases);
}

void ApplicationWindow::plotCustomLayout(bool sharedAxes)
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t || !validFor2DPlot(t, (Graph::CurveType)defaultCurveStyle))
		return;

	QStringList list = t->drawableColumnSelection();
	int curves = list.count();
	if(curves < 1){
		QMessageBox::warning(this, tr("QTISAS - Plot error"), tr("Please select a Y column to plot!"));
		return;
	}

	LayerDialog *id = new LayerDialog(this, true);
	id->setLayers(curves);
	id->setRows(curves);
	id->setMargins(5, 5, 5, 5);
	id->setLayerCanvasSize(d_layer_canvas_width, d_layer_canvas_height, d_layer_geometry_unit);
	if (sharedAxes)
		id->setSharedAxes();
	id->exec();
}

void ApplicationWindow::plot2VerticalLayers()
{
	multilayerPlot(1, 2, defaultCurveStyle);
}

void ApplicationWindow::plot2HorizontalLayers()
{
	multilayerPlot(2, 1, defaultCurveStyle);
}

void ApplicationWindow::plot4Layers()
{
	multilayerPlot(2, 2, defaultCurveStyle);
}

void ApplicationWindow::plotStackedLayers()
{
	multilayerPlot(1, -1, defaultCurveStyle);
}

void ApplicationWindow::plotStackedHistograms()
{
	multilayerPlot(1, -1, Graph::Histogram);
}

void ApplicationWindow::zoomRectanglePlot()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

    QStringList lst = t->selectedYColumns();
    int cols = lst.size();
	if (cols < 1){
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("You need to select at least one Y column for this operation!"));
		return;
	}

    MySelection sel = t->getSelection();
    MultiLayer *ml = multilayerPlot(t, lst, Graph::LineSymbols, sel.topRow(), sel.bottomRow());
    if (ml){
        Graph *ag = ml->activeLayer();
        ag->setTitle("");
        ag->setAxisTitle(QwtPlot::xBottom, " ");
        ag->setAxisTitle(QwtPlot::yLeft, " ");
        ag->setCanvasFrame();
        ag->drawAxesBackbones(false);
        ag->showGrid();
        ag->removeLegend();

        RectangleWidget *r = new RectangleWidget(ag);
        QColor c = Qt::yellow;
        c.setAlpha(100);
        r->setBackgroundColor(c);
        r->setFrameColor(Qt::blue);
        ag->add(r, false);

		Graph *g = ml->addLayer();
        if (!g)
            return;

		setPreferences(g);
        g->setTitle("");
        g->setAxisTitle(QwtPlot::xBottom, " ");
        g->setAxisTitle(QwtPlot::yLeft, " ");
        g->copyCurves(ag);
        g->drawAxesBackbones(false);
        g->setCanvasFrame();
        g->setCanvasBackground(c);

        ml->setRows(2);
        ml->setCols(1);
        ml->arrangeLayers(false);

        QRect canvasRect = ag->canvas()->geometry();
        r->setRect(canvasRect.x(), canvasRect.bottom() - 50, 100, 50);

        g->setCanvasCoordinates(r->boundingRect());
        r->setLinkedLayer(1);
    }
}

void ApplicationWindow::plotDoubleYAxis()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	QStringList lst = t->selectedYColumns();
	int cols = lst.size();
	if (cols < 2){
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("You need at least two columns for this operation!"));
		return;
	}

    MySelection sel = t->getSelection();
	MultiLayer *ml = multilayerPlot(t, lst, Graph::LineSymbols, sel.topRow(), sel.bottomRow());
	if (ml){
		Graph *g = ml->activeLayer();
		g->enableAxis(QwtPlot::yRight);
		QwtPlotCurve *c = g->curve(cols - 1);
		if (c){
			c->setYAxis(QwtPlot::yRight);
			g->setAutoScale();
		}
		g->updateAxisTitle(QwtPlot::yRight);
	}
}

void ApplicationWindow::showMatrixDialog()
{
	Matrix *m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	MatrixDialog* md = new MatrixDialog(this);
	md->setMatrix (m);
	md->exec();
}

void ApplicationWindow::showMatrixSizeDialog()
{
	Matrix *m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	MatrixSizeDialog* md = new MatrixSizeDialog(m, this);
	md->exec();
}

void ApplicationWindow::showMatrixValuesDialog()
{
	Matrix *m = nullptr;
    auto ml = dynamic_cast<MultiLayer *>(activeWindow());
    if (ml)
    {
		int curveIndex = actionSetMatrixValues->data().toInt();
		Graph *g = ml->activeLayer();
		if (g){
			Spectrogram *sp = (Spectrogram *)g->plotItem(curveIndex);
			if (sp)
				m = sp->matrix();
		}
	} else
		m = (Matrix*)activeWindow(MatrixWindow);

	if (!m)
		return;

	MatrixValuesDialog* md = new MatrixValuesDialog(scriptEnv, this);
	md->setMatrix(m);
    if (d_completion)
        md->setCompleter(d_completer);
	md->exec();
}

void ApplicationWindow::showColumnOptionsDialog()
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	if(t->selectedColumns().count()>0) {
		TableDialog* td = new TableDialog(t, this);
		td->setAttribute(Qt::WA_DeleteOnClose);
		td->exec();
	}
}

void ApplicationWindow::showGeneralPlotDialog()
{
	MdiSubWindow* plot = activeWindow();
	if (!plot)
		return;

	if (QString(plot->metaObject()->className()) == "MultiLayer" && ((MultiLayer*)plot)->numLayers())
		showPlotDialog();
	else if (QString(plot->metaObject()->className()) == "Graph3D"){
	    QDialog* gd = showScaleDialog();
		((Plot3DDialog*)gd)->showGeneralTab();
	}
}

void ApplicationWindow::showAxisDialog()
{
	MdiSubWindow* plot = activeWindow();
	if (!plot)
		return;

	QDialog* gd = showScaleDialog();
	if (gd && QString(plot->metaObject()->className()) == "MultiLayer" && ((MultiLayer*)plot)->numLayers())
		((AxesDialog*)gd)->showAxesPage();
	else if (gd && QString(plot->metaObject()->className()) == "Graph3D")
		((Plot3DDialog*)gd)->showAxisTab();
}

void ApplicationWindow::showGridDialog()
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd)
		gd->showGridPage();
}

QDialog* ApplicationWindow::showScaleDialog()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return 0;

	if (QString(w->metaObject()->className()) == "MultiLayer"){
		if (((MultiLayer*)w)->isEmpty())
			return 0;

		Graph* g = ((MultiLayer*)w)->activeLayer();
		if (g->isPiePlot()){
            QMessageBox::warning(this, tr("QTISAS - Warning"), tr("This functionality is not available for pie plots!"));
		    return 0;
		}

		AxesDialog* ad = new AxesDialog(this);
        ad->setGraph(g);
        ad->exec();
        return ad;
	} else if (QString(w->metaObject()->className()) == "Graph3D")
		return showPlot3dDialog();

	return 0;
}

AxesDialog* ApplicationWindow::showScalePageFromAxisDialog(int axisPos)
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd)
		gd->setCurrentScale(axisPos);

	return gd;
}

AxesDialog* ApplicationWindow::showAxisPageFromAxisDialog(int axisPos)
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd){
		gd->showAxesPage();
		gd->setCurrentScale(axisPos);
	}
	return gd;
}

QDialog* ApplicationWindow::showPlot3dDialog()
{
	Graph3D *g = (Graph3D*)activeWindow(Plot3DWindow);
	if (!g)
		return 0;

	if (!g->hasData()){
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, tr("QTISAS - Warning"),
					tr("Not available for empty 3D surface plots!"));
		return 0;
	}

	Plot3DDialog* pd = new Plot3DDialog(this);
	pd->setPlot(g);
	pd->show();
	return pd;
}

void ApplicationWindow::showPlotDialog(int curveIndex)
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto pd = new PlotDialog(d_extended_plot_dialog, this);
	pd->insertColumnsList(columnsList(Table::All));
	pd->setMultiLayer(w);
    if (curveIndex >= 0)
    {
        auto g = (Graph *)w->activeLayer();
		if (g)
			pd->selectCurve(curveIndex);
    }
    else if (curveIndex == -100)
		pd->selectMultiLayerItem();

    pd->initFonts(plotTitleFont, plotAxesFont, plotNumbersFont, plotLegendFont);
	pd->showAll(d_extended_plot_dialog);
	pd->show();
}

void ApplicationWindow::showCurvePlotDialog()
{
	showPlotDialog(actionShowCurvePlotDialog->data().toInt());
}

void ApplicationWindow::showCurveContextMenu(QwtPlotItem *cv)
{
    if (!cv || !cv->isVisible())
        return;

    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	int curveIndex = g->curveIndex(cv);
	if (curveIndex < 0 || curveIndex >= g->curveCount())
		return;

	QMenu curveMenu(this);
	curveMenu.addAction(cv->title().text(), this, SLOT(showCurvePlotDialog()));
	curveMenu.addSeparator();

	curveMenu.addAction(actionHideCurve);
	actionHideCurve->setData(curveIndex);

	int type = ((PlotCurve *)cv)->type();
	bool spectrogram = (cv->rtti() == QwtPlotItem::Rtti_PlotSpectrogram) ? true : false;
    if (g->visibleCurves() > 1 && (type == Graph::Function || spectrogram)){
        curveMenu.addAction(actionHideOtherCurves);
        actionHideOtherCurves->setData(curveIndex);
    } else if (type != Graph::Function && !spectrogram) {
        if ((g->visibleCurves() - ((DataCurve *)cv)->errorBarsList().count()) > 1) {
            curveMenu.addAction(actionHideOtherCurves);
            actionHideOtherCurves->setData(curveIndex);
        }
    }

	if (g->visibleCurves() != g->curveCount())
		curveMenu.addAction(actionShowAllCurves);
	curveMenu.addSeparator();

	if (g->rangeSelectorsEnabled() || (g->activeTool() &&
		g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker))
		curveMenu.addAction(actionCopySelection);
	if (spectrogram){
		curveMenu.addSeparator();
		curveMenu.addAction(actionSetMatrixValues);
		actionSetMatrixValues->setData(curveIndex);
	} else if (type != Graph::Function && type != Graph::ErrorBars){
		if (g->rangeSelectorsEnabled() || (g->activeTool() &&
			g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)){
			curveMenu.addAction(actionCutSelection);
			curveMenu.addAction(actionPasteSelection);
			if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker){
				DataPickerTool *dpt = (DataPickerTool *)g->activeTool();
				if (dpt){
					QAction *act = new QAction(tr("Paste Selection as Te&xt"), this);
					connect(act, SIGNAL(triggered()), dpt, SLOT(pasteSelectionAsLayerText()));
					curveMenu.addAction(act);
				}
			}

			curveMenu.addAction(actionClearSelection);
			curveMenu.addSeparator();
			if (g->rangeSelectorsEnabled()){
				QAction *act = new QAction(tr("Set Display Range"), this);
				connect(act, SIGNAL(triggered()), g->rangeSelectorTool(), SLOT(setCurveRange()));
				curveMenu.addAction(act);
			}
		}

		curveMenu.addAction(actionEditCurveRange);
		actionEditCurveRange->setData(curveIndex);

        auto *dcfr = dynamic_cast<DataCurve *>(cv);
        if (dcfr && dcfr->table())
        {
            curveMenu.addAction(actionCurveFullRange);
            if (dcfr->isFullRange())
                actionCurveFullRange->setDisabled(true);
            else
                actionCurveFullRange->setEnabled(true);
            actionCurveFullRange->setData(curveIndex);
        }

		curveMenu.addSeparator();
	}

	curveMenu.addAction(actionShowCurveWorksheet);
	actionShowCurveWorksheet->setData(curveIndex);

	curveMenu.addAction(actionShowCurvePlotDialog);
	actionShowCurvePlotDialog->setData(curveIndex);

    curveMenu.addAction(actionExportLayer);
    curveMenu.addAction(QPixmap(":/copy.png"), tr("&Copy Layer"), this, SLOT(copyActiveLayer()));
	curveMenu.addSeparator();

	curveMenu.addAction(actionRemoveCurve);
	actionRemoveCurve->setData(curveIndex);
	curveMenu.exec(QCursor::pos());
}

void ApplicationWindow::showAllCurves()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	for(int i=0; i< g->curveCount(); i++)
		g->showCurve(i);
	g->replot();
}

void ApplicationWindow::hideOtherCurves()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	for(int i=0; i< g->curveCount(); i++)
		g->showCurve(i, false);

	g->showCurve(actionHideOtherCurves->data().toInt());
	g->replot();
}

void ApplicationWindow::hideCurve()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	g->showCurve(actionHideCurve->data().toInt(), false);
}

void ApplicationWindow::removeCurve()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	g->removeCurve(actionRemoveCurve->data().toInt());
	g->updatePlot();
}

void ApplicationWindow::showCurveWorksheet(Graph *g, int curveIndex)
{
    if (!g)
        return;

    MultiLayer *ml = g->multiLayer();
    if (!ml)
        return;

    const QwtPlotItem *it = g->plotItem(curveIndex);
    if (!it)
        return;

    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
        Matrix *m = ((Spectrogram *)it)->matrix();
        if (!m)
            return;

        m->setMaximized(ml);
    }
    else if (((PlotCurve *)it)->type() == Graph::Function)
        g->createTable((PlotCurve *)it);
    else
    {
        Table *t = ((DataCurve *)it)->table();
        if (!t)
            return;

        showTable(t, it->title().text());

        if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
            ((DataPickerTool *)g->activeTool())->selectTableRow();

        t->setMaximized(ml);
    }
}

void ApplicationWindow::showCurveWorksheet()
{
    auto w = dynamic_cast<MultiLayer *>(activeWindow());
    if (!w)
        return;

    auto g = (Graph *)w->activeLayer();
    if (!g)
        return;

	showCurveWorksheet(g, actionShowCurveWorksheet->data().toInt());
}

void ApplicationWindow::magnify(int mode)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this, tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers)
		g->enablePanningMagnifier(true, mode);
}

void ApplicationWindow::zoomIn()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	if (plot->isEmpty())
	{
		QMessageBox::warning(this, tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	if ((Graph*)plot->activeLayer()->isPiePlot())
	{
		if (btnZoomIn->isChecked())
			QMessageBox::warning(this,tr("QTISAS - Warning"),
					tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers){
		if (!g->isPiePlot())
			g->zoom(true);
	}
}

void ApplicationWindow::zoomOut()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	if (plot->isEmpty() || (Graph*)plot->activeLayer()->isPiePlot())
		return;

	((Graph*)plot->activeLayer())->zoomOut();
	btnPointer->setChecked(true);
}

void ApplicationWindow::setAutoScale()
{
    MdiSubWindow *w = this->activeWindow();
    if (!w)
        return;

    if (QString(w->metaObject()->className()) == "Graph3D")
        return;

    auto *plot = dynamic_cast<MultiLayer *>(w);
    if (!plot || plot->isEmpty())
        return;

    auto *g = static_cast<Graph *>(plot->activeLayer());
    if (!g)
        return;

    if (g && !g->curvesList().empty() && g->curvesList()[0]->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
        auto *sp = (Spectrogram *)g->curvesList()[0];
        return g->spLogLinSwitcher(g->axisLabelFormat(sp->colorScaleAxis()) != 0);
    }

    for (int i = 0; i <= 3; i++)
    {
        int axisType = ((ScaleEngine *)g->axisScaleEngine(i))->type();

        if (axisType == 0)
            g->setLinOrLogAxis(i, false, false, true);
        else if (axisType == 1)
            g->setLinOrLogAxis(i, true, false, true);
        else
            g->setAxisAutoScale(i);
    }
    g->replot();

    if (!plot->hasLinkedXLayerAxes())
        return;

    double gMinX, gMaxX, gDummy1, gDummy2;
    g->minmaxPositiveXY(2, gMinX, gMaxX, gDummy1, gDummy2, false, false);

    for (Graph *l : plot->layersList())
    {
        if (l == g)
            continue;
        bool changed = false;
        for (int i = 2; i <= 3; i++)
        {
            if (!l->axisEnabled(i))
                continue;
            if (l->curveCount() > 0)
            {
                int axisType = ((ScaleEngine *)l->axisScaleEngine(i))->type();
                if (axisType == 0)
                    l->setLinOrLogAxis(i, false, false, true);
                else if (axisType == 1)
                    l->setLinOrLogAxis(i, true, false, true);
                else
                    l->setAxisAutoScale(i);
            }
            else if (gMinX < gMaxX)
            {
                int axisType = ((ScaleEngine *)l->axisScaleEngine(i))->type();
                bool logAxis = (axisType == ScaleTransformation::Log10);
                double lo = logAxis ? gMinX : gMinX - fabs(gMaxX - gMinX) * 0.025;
                double hi = logAxis ? gMaxX : gMaxX + fabs(gMaxX - gMinX) * 0.025;
                l->setScale(i, lo, hi, l->axisStep(i), l->axisMaxMajor(i), l->axisMaxMinor(i), axisType, false);
            }
            changed = true;
        }
        if (changed)
            l->replot();
    }
}

void ApplicationWindow::setLinOrLog(bool rescaleAll, bool logYN)
{
    MdiSubWindow *w = this->activeWindow();
    if (!w)
        return;

    if (QString(w->metaObject()->className()) == "Graph3D")
        return;

    auto *plot = dynamic_cast<MultiLayer *>(w);
    if (!plot)
        return;

    auto *g = static_cast<Graph *>(plot->activeLayer());
    if (!g)
        return;

    if (!g->curvesList().empty() && g->curvesList()[0]->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
        return g->spLogLinSwitcher(logYN);

    bool oneAxis = (!rescaleAll && g->selectedScale());

    int iStart = oneAxis ? g->selectedAxis() : 0;
    int iFinish = oneAxis ? g->selectedAxis() : 3;

    for (int i = iStart; i <= iFinish; i++)
    {
        if (logYN)
            g->setLinOrLogAxis(i, true);
        else
            g->setLinOrLogAxis(i, false);
    }

    g->replot();
}

void ApplicationWindow::removePoints()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}
    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
    {
        btnPointer->setChecked(true);
        return;
    }

	if (g->isPiePlot()){
		QMessageBox::warning(this,tr("QTISAS - Warning"), tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}

	if (d_confirm_modif_2D_points){
		QMessageBox msgBox(QMessageBox::Question, tr("QTISAS"),
		tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"));

		QPushButton *yesButton = msgBox.addButton(tr("Yes, don't ask me again"), QMessageBox::YesRole);
		msgBox.addButton(QMessageBox::Cancel);
		msgBox.setDefaultButton(msgBox.addButton(QMessageBox::Yes));
		msgBox.setEscapeButton(QMessageBox::Cancel);
		msgBox.setWindowIcon(this->windowIcon());
		msgBox.exec();
		if (msgBox.clickedButton() == yesButton || msgBox.clickedButton() == msgBox.defaultButton()){
			g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Remove, info, SLOT(setText(const QString&))));
			displayBar->show();
			if (msgBox.clickedButton() == yesButton)
				d_confirm_modif_2D_points = false;
		} else
			btnPointer->setChecked(true);
	} else {
		g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Remove, info, SLOT(setText(const QString&))));
		displayBar->show();
	}
}

void ApplicationWindow::movePoints(bool wholeCurve)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
    {
        btnPointer->setChecked(true);
        return;
	}

	if (g->isPiePlot()){
		QMessageBox::warning(this, tr("QTISAS - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	}

	if (d_confirm_modif_2D_points){
		QMessageBox msgBox(QMessageBox::Question, tr("QTISAS"),
		tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"));

		QPushButton *yesButton = msgBox.addButton(tr("Yes, don't ask me again"), QMessageBox::YesRole);
		msgBox.addButton(QMessageBox::Cancel);
		msgBox.setDefaultButton(msgBox.addButton(QMessageBox::Yes));
		msgBox.setEscapeButton(QMessageBox::Cancel);
		msgBox.setWindowIcon(this->windowIcon());
		msgBox.exec();
		if (msgBox.clickedButton() == yesButton || msgBox.clickedButton() == msgBox.defaultButton()){
			DataPickerTool *tool = new DataPickerTool(g, this, DataPickerTool::Move, info, SLOT(setText(const QString&)));
			if (wholeCurve)
				tool->setMode(DataPickerTool::MoveCurve);
			g->setActiveTool(tool);
			displayBar->show();
			if (msgBox.clickedButton() == yesButton)
				d_confirm_modif_2D_points = false;
		} else
			btnPointer->setChecked(true);
	} else {
		DataPickerTool *tool = new DataPickerTool(g, this, DataPickerTool::Move, info, SLOT(setText(const QString&)));
		if (wholeCurve)
			tool->setMode(DataPickerTool::MoveCurve);
		g->setActiveTool(tool);
		displayBar->show();
	}

	/*switch(QMessageBox::warning (this, tr("QTISAS"),
				tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"),
				tr("Continue"), tr("Cancel"), 0, 1))
	{
		case 0:
			if (g){
				DataPickerTool *tool = new DataPickerTool(g, this, DataPickerTool::Move, info, SLOT(setText(const QString&)));
				if (wholeCurve)
					tool->setMode(DataPickerTool::MoveCurve);
				g->setActiveTool(tool);
				displayBar->show();
			}
			break;

		case 1:
			btnPointer->setChecked(true);
			break;
	}*/
}

void ApplicationWindow::exportPDF()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if(qobject_cast<MultiLayer *>(w) && ((MultiLayer *)w)->isEmpty()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
			tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}

	if (qobject_cast<MultiLayer *>(w) || qobject_cast<Graph3D *>(w)){
		exportGraph("*.pdf");
		return;
	} else if (qobject_cast<Matrix *>(w)){
		exportMatrix("*.pdf");
		return;
	}

    QString fname = getFileName(this, tr("Choose a filename to save under"), imagesDirPath + "/" + w->objectName(),
                                "*.pdf", nullptr, true);

	if (!fname.isEmpty() ){
		QFileInfo fi(fname);
		QString baseName = fi.fileName();
		if (!baseName.contains("."))
			fname.append(".pdf");

        imagesDirPath = fi.absolutePath();

        QFile f(fname);
        if (!f.open(QIODevice::WriteOnly)){
            QMessageBox::critical(this, tr("QTISAS - Export error"),
            tr("Could not write to file: <h4>%1</h4><p>Please verify that you have the right to write to this location or that the file is not being used by another application!").arg(fname));
            return;
        }

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        w->exportPDF(fname);
		QApplication::restoreOverrideCursor();
	}
}

//print active window
void ApplicationWindow::print()
{
	MdiSubWindow* w = activeWindow();
	if (!w)
		return;

    if (QString(w->metaObject()->className()) == "MultiLayer" && ((MultiLayer *)w)->isEmpty()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}
	w->print();
}

//print preview for active window
void ApplicationWindow::printPreview()
{
	MdiSubWindow* w = activeWindow();
	if (!w)
		return;

	if (QString(w->metaObject()->className()) == "MultiLayer" && ((MultiLayer *)w)->isEmpty()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}

	QPrinter p;
    p.setPageSize(QPageSize(d_print_paper_size));
    p.setPageOrientation(d_printer_orientation);

	QPrintPreviewDialog *preview = new QPrintPreviewDialog(&p, this, Qt::Window);
	preview->setWindowTitle(tr("QTISAS") + " - " + "Print preview of window: " + w->objectName());
	connect(preview, SIGNAL(paintRequested(QPrinter *)), w, SLOT(print(QPrinter *)));
	connect(preview, SIGNAL(paintRequested(QPrinter *)), this, SLOT(setPrintPreviewOptions(QPrinter *)));

	preview->exec();
}

void ApplicationWindow::setPrintPreviewOptions(QPrinter *printer)
{
	if (!printer)
		return;
    d_print_paper_size = printer->pageLayout().pageSize().id();
    d_printer_orientation = printer->pageLayout().orientation();
}

void ApplicationWindow::printAllPlots()
{
    auto *printer = new QPrinter();
    printer->setPageOrientation(QPageLayout::Landscape);
	printer->setColorMode (QPrinter::Color);
	printer->setFullPage(true);

    QPrintDialog dialog(printer);
	if (dialog.exec()){
		QPainter *paint = new QPainter(printer);

		int plots = 0;
		QList<MdiSubWindow *> windows = windowsList();
		foreach(MdiSubWindow *w, windows){
			if (qobject_cast<MultiLayer*>(w))
				plots++;
		}

        dialog.setMinMax(0, plots);
		printer->setFromTo (0, plots);

		foreach(MdiSubWindow *w, windows){
			MultiLayer *ml = qobject_cast<MultiLayer*>(w);
			if (ml){
				ml->printAllLayers(paint);
				if (w != windows.last())
					printer->newPage();
			}
		}
		paint->end();
		delete paint;
	}
}

void ApplicationWindow::showExpGrowthDialog()
{
	showExpDecayDialog(-1);
}

void ApplicationWindow::showExpDecayDialog()
{
	showExpDecayDialog(1);
}

void ApplicationWindow::showExpDecayDialog(int type)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto edd = new ExpDecayDialog(type, this);
    edd->setGraph(g);
    edd->show();
}

void ApplicationWindow::showTwoExpDecayDialog()
{
	showExpDecayDialog(2);
}

void ApplicationWindow::showExpDecay3Dialog()
{
	showExpDecayDialog(3);
}

void ApplicationWindow::showFitDialog()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	MultiLayer* plot = 0;
	if(QString(w->metaObject()->className()) == "MultiLayer")
		plot = (MultiLayer*)w;
	else if(w->inherits("Table")){
		QStringList columnsLst = ((Table *)w)->drawableColumnSelection();
		if (columnsLst.isEmpty()){
			QMessageBox::warning(this, tr("QTISAS - Column selection error"),
			tr("Please select a 'Y' column first!"));
			return;
		}
		plot = multilayerPlot((Table *)w, columnsLst, Graph::LineSymbols);
	}

	if (!plot)
		return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto fd = new FitDialog(g, this);
	connect (plot, SIGNAL(destroyed()), fd, SLOT(close()));

	fd->setSrcTables(tableList());
	fd->show();
	fd->resize(fd->minimumSize());
}

void ApplicationWindow::showFilterDialog(int filter)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (g && g->validCurvesDataSize())
    {
        auto fd = new FilterDialog(filter, this);
        fd->setGraph(g);
        fd->exec();
	}
}

void ApplicationWindow::lowPassFilterDialog()
{
	showFilterDialog(FFTFilter::LowPass);
}

void ApplicationWindow::highPassFilterDialog()
{
	 showFilterDialog(FFTFilter::HighPass);
}

void ApplicationWindow::bandPassFilterDialog()
{
	showFilterDialog(FFTFilter::BandPass);
}

void ApplicationWindow::bandBlockFilterDialog()
{
	showFilterDialog(FFTFilter::BandBlock);
}

void ApplicationWindow::showFFTDialog()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	FFTDialog *sd = 0;
	if (qobject_cast<MultiLayer *>(w)){
		Graph* g = ((MultiLayer*)w)->activeLayer();
		if ( g && g->validCurvesDataSize() ){
			sd = new FFTDialog(FFTDialog::onGraph, this);
			sd->setGraph(g);
		}
	} else if (w->inherits("Table")){
		sd = new FFTDialog(FFTDialog::onTable, this);
		sd->setTable((Table*)w);
	} else if (qobject_cast<Matrix *>(w)){
		if (!((Matrix *)w)->isEmpty()){
			sd = new FFTDialog(FFTDialog::onMatrix, this);
			sd->setMatrix((Matrix *)w);
		} else
			showNoDataMessage();
	}

	if (sd)
        sd->exec();
}

void ApplicationWindow::showSmoothDialog(int m)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto sd = new SmoothCurveDialog(m, this);
    sd->setGraph(g);
    sd->exec();
}

void ApplicationWindow::showSmoothSavGolDialog()
{
    showSmoothDialog(SmoothFilter::SavitzkyGolay);
}

void ApplicationWindow::showSmoothFFTDialog()
{
	showSmoothDialog(SmoothFilter::FFT);
}

void ApplicationWindow::showSmoothAverageDialog()
{
	showSmoothDialog(SmoothFilter::Average);
}

void ApplicationWindow::showSmoothLowessDialog()
{
	showSmoothDialog(SmoothFilter::Lowess);
}

void ApplicationWindow::showInterpolationDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto id = new InterpolationDialog(this);
    id->setGraph(g);
    id->show();
}

void ApplicationWindow::showFitPolynomDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto pfd = new PolynomFitDialog(this);
    pfd->setGraph(g);
    pfd->show();
}

void ApplicationWindow::updateLog(const QString& result)
{
	if ( !result.isEmpty() ){
		current_folder->appendLogInfo(result);
		showResults(true);
		emit modified();
	}
}

void ApplicationWindow::showFunctionIntegrationDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    auto id = new IntDialog(this, g);
    id->exec();
}

void ApplicationWindow::showResults(bool ok)
{
	if (ok){
		if (!current_folder->logInfo().isEmpty())
			results->setText(current_folder->logInfo());
		else
			results->setText(tr("Sorry, there are no results to display!"));

		logWindow->show();
		QTextCursor cur = results->textCursor();
		cur.movePosition(QTextCursor::End);
		results->setTextCursor(cur);
	} else
		logWindow->hide();
}

void ApplicationWindow::showResults(const QString& s, bool ok)
{
	current_folder->appendLogInfo(s);

	QString logInfo = current_folder->logInfo();
	if (!logInfo.isEmpty())
		results->setText(logInfo);
	showResults(ok);
}

void ApplicationWindow::showScreenReader()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers)
		g->setActiveTool(new ScreenPickerTool(g, info, SLOT(setText(const QString&))));

	displayBar->show();
}

void ApplicationWindow::drawPoints()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers)
        g->setActiveTool(new DrawPointTool(this, g, info, SLOT(setText(const QString&))));

	displayBar->show();
}

void ApplicationWindow::showRangeSelectors()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this, tr("QTISAS - Warning"), tr("There are no plot layers available in this window!"));
		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

	if (!g->curveCount()){
		QMessageBox::warning(this, tr("QTISAS - Warning"), tr("There are no curves available on this plot!"));
		btnPointer->setChecked(true);
		return;
	} else if (g->isPiePlot()) {
		QMessageBox::warning(this, tr("QTISAS - Warning"), tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}

	displayBar->show();
	g->enableRangeSelectors(info, SLOT(setText(const QString&)));
}

void ApplicationWindow::showCursor()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	if ((Graph*)plot->activeLayer()->isPiePlot()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers){
		if (g->isPiePlot() || !g->curveCount())
            continue;
        if (g->validCurvesDataSize())
			g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Display, info, SLOT(setText(const QString&))));
    }
	displayBar->show();
}

void ApplicationWindow::newLegend()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
        QMessageBox::warning(this, tr("QTISAS - Warning"),
                             tr("<h4>There are no plot layers available in this window.</h4>"
                                "<p><h4>Please add a layer and try again!</h4>"));
        return;
    }

    auto g = (Graph *)plot->activeLayer();
    if (g)
        g->newLegend();
}

void ApplicationWindow::addTimeStamp()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = (Graph*)plot->activeLayer();
	if ( g )
		g->addTimeStamp();
}

void ApplicationWindow::addRectangle()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
    {
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("There are no layers available on this plot. Operation aborted!"));
		actionAddRectangle->setChecked(false);
		return;
	}

    g->setActiveTool(new AddWidgetTool(AddWidgetTool::Rectangle, g, actionAddRectangle, info, SLOT(setText(const QString&))));
	btnPointer->setChecked(false);
}

void ApplicationWindow::addEllipse()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
    {
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("There are no layers available on this plot. Operation aborted!"));
		actionAddEllipse->setChecked(false);
		return;
	}

    g->setActiveTool(new AddWidgetTool(AddWidgetTool::Ellipse, g, actionAddEllipse, info, SLOT(setText(const QString&))));
	btnPointer->setChecked(false);
}

void ApplicationWindow::addTexFormula()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
    {
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("There are no layers available on this plot. Operation aborted!"));
		actionAddFormula->setChecked(false);
		return;
	}

	g->setActiveTool(new AddWidgetTool(AddWidgetTool::TexEquation, g, actionAddFormula, info, SLOT(setText(const QString&))));
	btnPointer->setChecked(false);
}

void ApplicationWindow::addText()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
    {
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("There are no layers available on this plot. Operation aborted!"));
		actionAddText->setChecked(false);
		return;
	}

	g->setActiveTool(new AddWidgetTool(AddWidgetTool::Text, g, actionAddText, info, SLOT(setText(const QString&))));
	btnPointer->setChecked(false);
}

void ApplicationWindow::addImage()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    QString fn = getFileName(this, tr("QTISAS - Insert image from file"), imagesDirPath, imageFilter(), nullptr, false);
	if ( !fn.isEmpty() ){
		QFileInfo fi(fn);
		imagesDirPath = fi.absolutePath();

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		g->addImage(fn);
		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::drawLine()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));

		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (g)
    {
        g->drawLine(true);
        emit modified();
	}
}

void ApplicationWindow::drawArrow()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));

		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (g)
    {
        g->drawLine(true, true);
        emit modified();
    }
}

void ApplicationWindow::showLayerDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
        QMessageBox::warning(this, tr("QTISAS - Warning"), tr("There are no plot layers available in this window."));
        return;
    }

    auto id = new LayerDialog(this);
    id->setMultiLayer(plot);
    id->exec();
}

void ApplicationWindow::showEnrichementDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

	FrameWidget *w = g->activeEnrichment();
	EnrichmentDialog::WidgetType wt = EnrichmentDialog::Text;
	if (qobject_cast<RectangleWidget *>(w))
		wt = EnrichmentDialog::Frame;
	else if (qobject_cast<EllipseWidget *>(w))
		wt = EnrichmentDialog::Ellipse;
	else if (qobject_cast<ImageWidget *>(w))
		wt = EnrichmentDialog::Image;
	else if (qobject_cast<TexWidget *>(w))
		wt = EnrichmentDialog::Tex;

    auto ed = new EnrichmentDialog(wt, g, this, this);
    ed->setWidget(w);
    ed->exec();
}

void ApplicationWindow::showLineDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (g)
    {
		ArrowMarker *lm = g->selectedArrow();
		if (!lm)
			return;

        auto ld = new LineDialog(lm, this);
        ld->exec();
        g->deselectMarker();
	}
}

void ApplicationWindow::addColToTable()
{
	Table* m = (Table*)activeWindow(TableWindow);
	if ( m )
    {
		m->addCol();
        m->setColumnWidth(m->numCols()-1, m->colWidth);
    }
}

void ApplicationWindow::clearSelection()
{
	if (results->hasFocus()){
		clearLogInfo();
		return;
	} else if(lv->hasFocus()){
		deleteSelectedItems();
		return;
	}

	MdiSubWindow* m = activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->clearSelection();
	else if (QString(m->metaObject()->className()) == "Matrix")
		((Matrix*)m)->clearSelection();
	else if (QString(m->metaObject()->className()) == "MultiLayer"){
		Graph* g = ((MultiLayer*)m)->activeLayer();
		if (!g)
			return;

        if (((MultiLayer*)m)->hasSelectedLayers()){
            ((MultiLayer*)m)->confirmRemoveLayer();
            emit modified();
            return;
        }

		if (g->rangeSelectorsEnabled())
			g->rangeSelectorTool()->clearSelection();
		else if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
			((DataPickerTool *)g->activeTool())->removePoint();
        else if (g->titleSelected())
			g->clearTitle();
		else if (g->axisTitleSelected()){
			QwtScaleWidget *axis = g->currentScale();
			if (axis)
				axis->setTitle(" ");
		} else if (g->selectedCurveLabels()){
			QwtPlotItem *i = g->selectedCurveLabels();
			if(i->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
				((Spectrogram *)i)->showContourLineLabels(false);
			else if(i->rtti() == QwtPlotItem::Rtti_PlotCurve && ((PlotCurve *)i)->type() != Graph::Function)
				((DataCurve *)i)->clearLabels();
			g->replot();
		} else
			g->removeMarker();
	}
	else if (QString(m->metaObject()->className()) == "Note")
		((Note*)m)->currentEditor()->textCursor().removeSelectedText();
	emit modified();
}

void ApplicationWindow::copySelection()
{
	if(results->hasFocus()){
		results->copy();
		return;
	} else if(info->hasFocus()) {
		info->copy();
		return;
	}

	MdiSubWindow* m = activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->copySelection();
	else if (QString(m->metaObject()->className()) == "Matrix")
		((Matrix*)m)->copySelection();
	else if (QString(m->metaObject()->className()) == "MultiLayer"){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot || plot->numLayers() == 0)
			return;

		Graph* g = (Graph*)plot->activeLayer();
		if (!g)
            return;

		if (g->rangeSelectorsEnabled())
			g->rangeSelectorTool()->copySelection();
		else if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
			((DataPickerTool *)g->activeTool())->copySelection();
		else if (g->markerSelected())
			copyMarker();
		else
			copyActiveLayer();

	} else if (QString(m->metaObject()->className()) == "Note")
		((Note*)m)->currentEditor()->copy();
}

void ApplicationWindow::cutSelection()
{
	MdiSubWindow* m = activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->cutSelection();
	else if (QString(m->metaObject()->className()) == "Matrix")
		((Matrix*)m)->cutSelection();
	else if(QString(m->metaObject()->className()) == "MultiLayer"){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot || plot->numLayers() == 0)
			return;

		Graph* g = (Graph*)plot->activeLayer();
		if (!g)
            return;

		if (g->rangeSelectorsEnabled())
			g->rangeSelectorTool()->cutSelection();
		else if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
			((DataPickerTool *)g->activeTool())->cutSelection();
		else {
            copyMarker();
            g->removeMarker();
        }
	} else if (QString(m->metaObject()->className()) == "Note")
		((Note*)m)->currentEditor()->cut();

	emit modified();
}

void ApplicationWindow::copyMarker()
{
    lastCopiedLayer = nullptr;

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (g && g->markerSelected())
    {
		d_enrichement_copy = nullptr;
		d_arrow_copy = nullptr;
		if (g->activeEnrichment())
			d_enrichement_copy = g->activeEnrichment();
		else if (g->arrowMarkerSelected())
			d_arrow_copy = g->selectedArrow();
        QGuiApplication::clipboard()->clear(QClipboard::Clipboard);
	}
}

void ApplicationWindow::pasteSelection()
{
    if (QApplication::activeModalWidget())
        return;

	MdiSubWindow* m = activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->pasteSelection();
	else if (QString(m->metaObject()->className()) == "Matrix")
		((Matrix*)m)->pasteSelection();
	else if (QString(m->metaObject()->className()) == "Note")
		((Note*)m)->currentEditor()->paste();
	else if (QString(m->metaObject()->className()) == "MultiLayer"){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot)
			return;
		plot->deselect();

        if (lastCopiedLayer)
        {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            Graph *g = plot->addLayer();
            g->copy(lastCopiedLayer);
            QPoint pos = plot->canvas()->mapFromGlobal(QCursor::pos());
            g->setCanvasGeometry(pos.x(), pos.y(), lastCopiedLayer->canvas()->width(),
                                 lastCopiedLayer->canvas()->height());
            if (g->isWaterfallPlot())
                g->updateDataCurves();
            QApplication::restoreOverrideCursor();
            lastCopiedLayer = nullptr;
        }
        else if (!QGuiApplication::clipboard()->image().isNull())
        {
            auto g = (Graph *)plot->activeLayer();
            if (!g)
                return;

            ImageWidget *iw = g->addImage(QGuiApplication::clipboard()->image());
            if (iw)
            {
                iw->setSize(iw->pixmap().size());
                g->multiLayer()->notifyChanges();
                iw->setAttachPolicy((FrameWidget::AttachPolicy)0);
            }
        }
        else
        {
			if (plot->numLayers() == 0)
				return;

			Graph* g = (Graph*)plot->activeLayer();
			if (!g)
				return;

			if (g->rangeSelectorsEnabled())
				g->rangeSelectorTool()->pasteSelection();
            else if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
				((DataPickerTool *)g->activeTool())->pasteSelection();
            else if (d_enrichement_copy){
				FrameWidget *t = g->add(d_enrichement_copy);
				QPoint pos = g->mapFromGlobal(QCursor::pos());
				if (g->geometry().contains(pos))
					t->move(pos);
				else
					t->move(g->pos());
			} else if (d_arrow_copy){
                ArrowMarker *a = g->addArrow(d_arrow_copy);
                a->setStartPoint(d_arrow_copy->startPointCoord().x(), d_arrow_copy->startPointCoord().y());
                a->setEndPoint(d_arrow_copy->endPointCoord().x(), d_arrow_copy->endPointCoord().y());
                //translate the new arrow 10 pixels to the right;
                a->setStartPoint(a->startPoint() + QPoint(10, 0));
                a->setEndPoint(a->endPoint() + QPoint(10, 0));
                g->replot();
                g->deselectMarker();
			}
		}
	}
	emit modified();
}

MdiSubWindow *ApplicationWindow::clone(MdiSubWindow *w)
{
    if (!w)
    {
        w = activeWindow();
        if (!w)
        {
            QMessageBox::critical(this, tr("QTISAS - Duplicate window error"),
                                  tr("There are no windows available in this folder!"));
            return nullptr;
        }
    }

    MdiSubWindow *nw = nullptr;
    MdiSubWindow::Status status = w->status();
    const QString caption = generateUniqueName(w->objectName() + "-copy");
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (QString(w->metaObject()->className()) == "MultiLayer")
    {
        auto *g = (MultiLayer *)w;
        nw = multilayerPlot(caption, 0, g->getRows(), g->getCols());
        ((MultiLayer *)nw)->copy(g);
    }
    else if (w->inherits("Table"))
    {
        auto *t = (Table *)w;
        nw = newTable(caption, t->numRows(), t->numCols());
        ((Table *)nw)->copy(t);
    }
    else if (QString(w->metaObject()->className()) == "Graph3D")
    {
        auto *g = (Graph3D *)w;
        if (!g->hasData())
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(this, tr("QTISAS - Duplicate error"),
                                 tr("Empty 3D surface plots cannot be duplicated!"));
            return nullptr;
        }
        nw = newPlot3D(caption);
        if (!nw)
        {
            QApplication::restoreOverrideCursor();
            return nullptr;
        }
        if (status == MdiSubWindow::Maximized)
            nw->hide();
        ((Graph3D *)nw)->copy(g);
        customToolBars(nw);
    }
    else if (QString(w->metaObject()->className()) == "Matrix")
    {
        auto *m = (Matrix *)w;
        nw = newMatrix(caption, m->numRows(), m->numCols());
        ((Matrix *)nw)->copy(m);
    }
    else if (QString(w->metaObject()->className()) == "Note")
    {
        nw = newNote(caption);
        if (nw)
        {
            ((Note *)nw)->setText(((Note *)w)->text());
            ((Note *)nw)->showLineNumbers(((Note *)w)->hasLineNumbers());
        }
    }

    if (nw)
    {
        auto *g3d = qobject_cast<Graph3D *>(nw);
        if (g3d)
            g3d->setIgnoreFonts(true);

        // Only the copy stays maximized, for every window type; setMaximized() also sets the status
        // shown in the project explorer (newTable()/newMatrix()/newNote() may have restored the source already)
        if (status == MdiSubWindow::Maximized)
        {
            w->showNormal();
            nw->setMaximized();
        }
        else
        {
            nw->resize(w->size());
            nw->showNormal();
        }

        if (g3d)
            g3d->setIgnoreFonts(false);

        nw->setWindowLabel(w->windowLabel());
        nw->setCaptionPolicy(w->captionPolicy());
    }
    QApplication::restoreOverrideCursor();
    return nw;
}

void ApplicationWindow::undo()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (qobject_cast<Note*>(w))
		((Note*)w)->currentEditor()->undo();
	else if (qobject_cast<Matrix*>(w)){
	    QUndoStack *stack = ((Matrix *)w)->undoStack();
	    if (stack && stack->canUndo())
			stack->undo();
	}
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::redo()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	if (qobject_cast<Note*>(w))
		((Note*)w)->currentEditor()->redo();
	else if (qobject_cast<Matrix*>(w)){
	    QUndoStack *stack = ((Matrix *)w)->undoStack();
	    if (stack && stack->canRedo())
			stack->redo();
	}
	QApplication::restoreOverrideCursor();
}

bool ApplicationWindow::hidden(QWidget* window)
{
	if (hiddenWindows->contains(window))
		return true;

	return false;
}

void ApplicationWindow::updateWindowStatus(MdiSubWindow* w)
{
    if (!w) return;

    setListView(w->objectName(), w->aspect());

    if (w->status() == MdiSubWindow::Maximized)
    {
		QList<MdiSubWindow *> windows = current_folder->windowsList();
		foreach(MdiSubWindow *oldMaxWindow, windows)
        {
			if (oldMaxWindow != w && oldMaxWindow->status() == MdiSubWindow::Maximized)
				oldMaxWindow->setStatus(MdiSubWindow::Normal);
		}
	}
	modifiedProject();
}

void ApplicationWindow::hideActiveWindow()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	hideWindow(w);
}

void ApplicationWindow::hideWindow(MdiSubWindow *w)
{
    bool maximized = w->isMaximized();

    // The window activated before this one: the most recent shown window of the same folder
    MdiSubWindow *previous = nullptr;
    const QList<QMdiSubWindow *> history = d_workspace->subWindowList(QMdiArea::ActivationHistoryOrder);
    for (auto it = history.crbegin(); it != history.crend() && !previous; ++it)
    {
        auto *sw = qobject_cast<MdiSubWindow *>(*it);
        if (sw && sw != w && sw->folder() == w->folder() && !hiddenWindows->contains(sw) && sw->isVisible())
            previous = sw;
    }

    hiddenWindows->append(w);
    w->setHidden();
    emit modified();

    // a maximized window hands the maximized state back to that window
    if (maximized && previous)
        maximizeWindow(previous);

    // A hidden window is not the active one: the folder's active window decides whether the next new window
    // opens maximized, so it must be a shown window, not this one
    if (Folder *f = w->folder(); f && f->activeWindow() == w)
        f->setActiveWindow(previous);
    if (d_active_window == w)
        d_active_window = previous;
}

void ApplicationWindow::hideWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MdiSubWindow *w = it->window();
	if (!w)
		return;

	hideWindow(w);
}

void ApplicationWindow::resizeActiveWindow()
{
	MdiSubWindow* w = activeWindow();
	if (!w)
		return;

	EnrichmentDialog *ed = new EnrichmentDialog(EnrichmentDialog::MDIWindow, nullptr, this, this);
    ed->setWidget(w);
    ed->exec();
}

void ApplicationWindow::resizeWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MdiSubWindow *w = it->window();
	if (!w)
		return;

	d_workspace->setActiveSubWindow(w);

	EnrichmentDialog *ed = new EnrichmentDialog(EnrichmentDialog::MDIWindow, nullptr, this, this);
    ed->setWidget(w);
    ed->exec();
}

void ApplicationWindow::activateWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();

    foreach (MdiSubWindow *ow, it->window()->folder()->windowsList())
        if (ow != it->window() && ow->status() == MdiSubWindow::Maximized)
        {
            ow->setNormal();
            break;
        }

	activateWindow(it->window());
}

void ApplicationWindow::activateWindow(MdiSubWindow *w)
{
	if (!w)
		return;

	w->setNormal();
	d_workspace->setActiveSubWindow(w);

    updateWindowLists(w);
	emit modified();
}

bool ApplicationWindow::activateWindow(QString name)
{
    if (name == "")
        return false;

    foreach (MdiSubWindow *w, windowsList())
        if (w && w->objectName() == name)
        {
            activateWindow(w);
            return true;
        }

    return false;
}

void ApplicationWindow::maximizeWindow(QTreeWidgetItem *lbi)
{
    if (!lbi)
        lbi = lv->currentItem();

    if (!lbi || lbi->type() == FolderListItem::itemType)
        return;

    maximizeWindow(((WindowListItem *)lbi)->window());
}

void ApplicationWindow::maximizeWindow(MdiSubWindow *w)
{
    if (!w || w->status() == MdiSubWindow::Maximized)
        return;

    w->showNormal();

    MdiSubWindow *prevActiveWindow = current_folder->activeWindow();
    if (prevActiveWindow == w)
        prevActiveWindow = nullptr;

    activateWindow(w);
    w->setMaximized(nullptr);
    updateWindowLists(w);

    emit modified();
}

void ApplicationWindow::maximizeWindow(const QString &name)
{
    foreach (MdiSubWindow *w, windowsList())
        if (w->name() == name)
            return maximizeWindow(w);
}

void ApplicationWindow::minimizeWindow(MdiSubWindow *w)
{
	if (!w)
		w = ((WindowListItem *)lv->currentItem())->window();

	if (!w)
		return;

	w->setMinimized();
    updateWindowLists(w);

    emit modified();
}

void ApplicationWindow::updateWindowLists(MdiSubWindow *w)
{
    if (!w || !hiddenWindows->contains(w))
    {
        if (lv && d_active_window)
            lv->ensureItemVisibility(d_active_window->objectName());
    }
    else if (!w->isHidden())
        hiddenWindows->takeAt(hiddenWindows->indexOf(w));
}

void ApplicationWindow::closeActiveWindow()
{
	MdiSubWindow *w = activeWindow();
	if (w)
		w->close();
}

void ApplicationWindow::removeWindowFromLists(MdiSubWindow *w, bool updatePlot)
{
	if (!w)
		return;

	QString caption = w->objectName();
	if (w->inherits("Table")){
		Table* m = (Table*)w;
		for (int i = 0; i < m->numCols(); i++)
            removeCurves(m->colName(i), updatePlot);
	} else if (QString(w->metaObject()->className()) == "MultiLayer"){
		MultiLayer *ml = (MultiLayer*)w;
		Graph *g = ml->activeLayer();
		if (g)
			btnPointer->setChecked(true);
	} else if (QString(w->metaObject()->className()) == "Matrix")
		remove3DMatrixPlots((Matrix*)w);

	if (hiddenWindows->contains(w))
		hiddenWindows->takeAt(hiddenWindows->indexOf(w));

	updateCompleter(caption, true);
}

void ApplicationWindow::closeWindow(MdiSubWindow *window, bool updatePlot, bool interactive)
{
	if (!window)
		return;

    bool maximized = window->isMaximized();

	if (d_active_window == window)
		d_active_window = nullptr;

    removeWindowFromLists(window, updatePlot);

	Folder *f = window->folder();
	f->removeWindow(window);

	//update list view in project explorer
    if (lv->findItems(window->objectName(), Qt::MatchExactly | Qt::MatchCaseSensitive).count() > 0)
        lv->takeTopLevelItem(lv->indexOfTopLevelItem(
            lv->findItems(window->objectName(), Qt::MatchExactly | Qt::MatchCaseSensitive).first()));

    window->blockSignals(true);
    if (!interactive)
        window->askOnCloseEvent(false);
	window->close();

	if (show_windows_policy == ActiveFolder && !f->windowsList().count()){
		customMenu(0);
		customToolBars(0);
	} else if (show_windows_policy == SubFolders && !(current_folder->children()).isEmpty()){
		FolderListItem *fi = current_folder->folderListItem();
		FolderListItem *item = (FolderListItem *)fi->child(0);
		int initial_depth = item->childCount();
		bool emptyFolder = true;
		while (item && item->childCount() >= initial_depth) {
			QList<MdiSubWindow *> lst = item->folder()->windowsList();
			if (lst.count() > 0) {
				emptyFolder = false;
				break;
			}
			item = (FolderListItem *)fi->treeWidget()->itemBelow(item);
		}
		if (emptyFolder){
			customMenu(0);
			customToolBars(0);
		}
	}
	emit modified();

    if (maximized)
    {
        d_workspace->activateNextSubWindow();
        maximizeWindow(activeWindow());
    }
}

QMessageBox * ApplicationWindow::about(bool dialog)
{
    if (dialog)
    {
        QString text = currentVersionInfo(!dialog);
        auto *mb = new QMessageBox();
        mb->setAttribute(Qt::WA_DeleteOnClose);
        mb->setWindowTitle(tr("About QtiSAS"));
        mb->setWindowIcon(QIcon(":/logo.png"));
        mb->setIconPixmap(QPixmap(":/logo.png"));
        mb->setText(text);
        mb->exec();
        return mb;
    }
    else
    {
        QString text = currentVersionInfo(!dialog);
        printf("%s\n", text.toLocal8Bit().constData());
        exit(0);
    }
}

void ApplicationWindow::aboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void ApplicationWindow::scriptingMenuAboutToShow()
{
    scriptingMenu->clear();
#ifdef SCRIPTING_PYTHON
	scriptingMenu->addAction(actionScriptingLang);
	scriptingMenu->addAction(actionRestartScripting);
    scriptingMenu->addAction(actionCustomActionDialog);
    scriptingMenu->addAction(actionOpenQtDesignerUi);
#endif

	Note *note = (Note *)activeWindow(NoteWindow);
    if (note){
		scriptingMenu->addSeparator();

        bool noteHasText = !note->text().isEmpty();
    	noteTools->setEnabled(noteHasText);
		if (noteHasText){
			if (scriptEnv->objectName() == QString("Python")){
				scriptingMenu->addAction(actionNoteExecuteAll);
			}

			#ifdef SCRIPTING_PYTHON
			if (scriptEnv->objectName() == QString("Python") && note->currentEditor() && note->currentEditor()->textCursor().hasSelection()){
				scriptingMenu->addSeparator();
				scriptingMenu->addAction(actionCommentSelection);
				scriptingMenu->addAction(actionUncommentSelection);
			}
			#endif

			scriptingMenu->addSeparator();
			scriptingMenu->addAction(actionIncreaseIndent);
			scriptingMenu->addAction(actionDecreaseIndent);
			scriptingMenu->addSeparator();
			scriptingMenu->addAction(actionFind);
			scriptingMenu->addAction(actionFindNext);
			scriptingMenu->addAction(actionFindPrev);
			scriptingMenu->addAction(actionReplace);
			scriptingMenu->addSeparator();
		}
		scriptingMenu->addAction(actionRenameNoteTab);
		scriptingMenu->addAction(actionAddNoteTab);
		if (note->tabs() > 1)
			scriptingMenu->addAction(actionCloseNoteTab);
		scriptingMenu->addSeparator();
		actionShowNoteLineNumbers->setChecked(note->hasLineNumbers());
		scriptingMenu->addAction(actionShowNoteLineNumbers);
    }

	reloadCustomActions();
}

void ApplicationWindow::analysisMenuAboutToShow()
{
    analysisMenu->clear();
    MdiSubWindow *w = activeWindow();
    if (!w)
        return;

	if (QString(w->metaObject()->className()) == "MultiLayer"){
        QMenu *translateMenu = analysisMenu->addMenu (tr("&Translate"));
        translateMenu->addAction(actionTranslateVert);
        translateMenu->addAction(actionTranslateHor);

		QMenu *subtractMenu = analysisMenu->addMenu(tr("S&ubtract"));
		subtractMenu->addAction(actionBaseline);
		subtractMenu->addAction(actionSubtractReference);
		subtractMenu->addAction(actionSubtractLine);

        analysisMenu->addSeparator();
        analysisMenu->addAction(actionDifferentiate);
		actionIntegrate->setText(tr("&Integrate") + "...");
		analysisMenu->addAction(actionIntegrate);
        analysisMenu->addAction(actionShowIntDialog);
        analysisMenu->addSeparator();

		smoothMenu->clear();
        analysisMenu->addMenu(smoothMenu);
        smoothMenu->addAction(actionSmoothSavGol);
        smoothMenu->addAction(actionSmoothAverage);
        smoothMenu->addAction(actionSmoothLowess);
        smoothMenu->addAction(actionSmoothFFT);

		filterMenu->clear();
		analysisMenu->addMenu(filterMenu);
        filterMenu->addAction(actionLowPassFilter);
        filterMenu->addAction(actionHighPassFilter);
        filterMenu->addAction(actionBandPassFilter);
        filterMenu->addAction(actionBandBlockFilter);

        analysisMenu->addSeparator();
        analysisMenu->addAction(actionInterpolate);
        analysisMenu->addAction(actionFFT);
        analysisMenu->addSeparator();
        analysisMenu->addAction(actionFitSlope);
        analysisMenu->addAction(actionFitLinear);
        analysisMenu->addAction(actionShowFitPolynomDialog);
        analysisMenu->addSeparator();

		decayMenu->clear();
		analysisMenu->addMenu(decayMenu);
        decayMenu->addAction(actionShowExpDecayDialog);
        decayMenu->addAction(actionShowTwoExpDecayDialog);
        decayMenu->addAction(actionShowExpDecay3Dialog);

        analysisMenu->addAction(actionFitExpGrowth);
        analysisMenu->addAction(actionFitSigmoidal);
        analysisMenu->addAction(actionFitGauss);
        analysisMenu->addAction(actionFitLorentz);

		multiPeakMenu->clear();
		analysisMenu->addMenu(multiPeakMenu);
        multiPeakMenu->addAction(actionMultiPeakGauss);
        multiPeakMenu->addAction(actionMultiPeakLorentz);
        analysisMenu->addSeparator();
        analysisMenu->addAction(actionShowFitDialog);
        analysisMenu->addSeparator();
#ifdef QTISAS
        analysisMenu->addAction(actionShowCompileLocal);
        analysisMenu->addAction(actionShowFittableLocal);

        auto *efitMenu = analysisMenu->addMenu("eFit");
        QStringList lstEfitGroups = fittableWidget->scanGroupEfit();
        for (int i = 0; i < lstEfitGroups.count(); i++)
        {
            auto *efitSubMenu = efitMenu->addMenu(lstEfitGroups[i]);
            QStringList lstEfitFunctions = fittableWidget->groupFunctions(lstEfitGroups[i], true);
            for (int ii = 0; ii < lstEfitFunctions.count(); ii++)
            {
                auto *efitAction = new QAction(lstEfitFunctions[ii], this);
                efitSubMenu->addAction(efitAction);
            }
            connect(efitSubMenu, &QMenu::triggered, this, &ApplicationWindow::eFitAction);
        }
#endif
	} else if (QString(w->metaObject()->className()) == "Matrix"){
		actionIntegrate->setText(tr("&Integrate"));
        analysisMenu->addAction(actionIntegrate);
        analysisMenu->addSeparator();
        analysisMenu->addAction(actionFFT);
        analysisMenu->addAction(actionMatrixFFTDirect);
        analysisMenu->addAction(actionMatrixFFTInverse);
	} else if (w->inherits("Table")){
		QMenu *statsMenu = analysisMenu->addMenu (tr("Descriptive S&tatistics"));
		statsMenu->addAction(actionShowColStatistics);
		statsMenu->addAction(actionShowRowStatistics);
		statsMenu->addAction(actionFrequencyCount);
		statsMenu->addAction(actionShapiroWilk);

		QMenu *tTestMenu = analysisMenu->addMenu (tr("&Hypothesis Testing"));
		tTestMenu->addAction(actionOneSampletTest);
		tTestMenu->addAction(actionTwoSampletTest);
		tTestMenu->addAction(actionChiSquareTest);

		QMenu *anovaMenu = analysisMenu->addMenu (tr("ANO&VA"));
		anovaMenu->addAction(actionOneWayANOVA);
		anovaMenu->addAction(actionTwoWayANOVA);

        analysisMenu->addSeparator();
		if (((Table *)w)->selectedColumns().count() > 1){
			QMenu *sortMenu = analysisMenu->addMenu("&" + tr("Sort Columns"));
			sortMenu->addAction(QIcon(":/sort_ascending.png"), tr("&Ascending"), w, SLOT(sortColAsc()));
			sortMenu->addAction(QIcon(":/sort_descending.png"), tr("&Descending"), w, SLOT(sortColDesc()));
			sortMenu->addAction(actionSortSelection);
			analysisMenu->addMenu(sortMenu);
		}
        analysisMenu->addAction(actionSortTable);

		normMenu->clear();
		analysisMenu->addMenu(normMenu);
        normMenu->addAction(actionNormalizeSelection);
        normMenu->addAction(actionNormalizeTable);

        analysisMenu->addSeparator();
		analysisMenu->addAction(actionDifferentiate);
		actionIntegrate->setText(tr("&Integrate") + "...");
		analysisMenu->addAction(actionIntegrate);
        analysisMenu->addSeparator();
		analysisMenu->addAction(actionFFT);
		analysisMenu->addSeparator();
        analysisMenu->addAction(actionCorrelate);
        analysisMenu->addAction(actionAutoCorrelate);
        analysisMenu->addSeparator();
        analysisMenu->addAction(actionConvolute);
        analysisMenu->addAction(actionDeconvolute);
        analysisMenu->addSeparator();
        analysisMenu->addAction(actionFitSlope);
		analysisMenu->addAction(actionFitLinear);
        analysisMenu->addAction(actionShowFitDialog);
        analysisMenu->addSeparator();
#ifdef QTISAS
        analysisMenu->addAction(actionShowCompileLocal);
        analysisMenu->addAction(actionShowFittableLocal);
#endif
	}
    reloadCustomActions();
}

void ApplicationWindow::matrixMenuAboutToShow()
{
	matrixMenu->clear();
	matrixMenu->addAction(actionSetMatrixProperties);
	matrixMenu->addAction(actionSetMatrixDimensions);
	matrixMenu->addSeparator();
	matrixMenu->addAction(actionSetMatrixValues);
	matrixMenu->addAction(actionTableRecalculate);
	matrixMenu->addSeparator();
	matrixMenu->addAction(actionRotateMatrix);
	matrixMenu->addAction(actionRotateMatrixMinus);
	matrixMenu->addAction(actionFlipMatrixVertically);
	matrixMenu->addAction(actionFlipMatrixHorizontally);
	matrixMenu->addSeparator();

	matrixMenu->addAction(actionExpandMatrix);
	matrixMenu->addAction(actionShrinkMatrix);
	matrixMenu->addAction(actionSmoothMatrix);
	matrixMenu->addSeparator();

	matrixMenu->addAction(actionTransposeMatrix);
	matrixMenu->addAction(actionInvertMatrix);
	matrixMenu->addAction(actionMatrixDeterminant);
	matrixMenu->addSeparator();
	matrixMenu->addAction(actionGoToRow);
	matrixMenu->addAction(actionGoToColumn);
	matrixMenu->addSeparator();
	QMenu *matrixViewMenu = matrixMenu->addMenu (tr("Vie&w"));
	matrixViewMenu->addAction(actionViewMatrixImage);
	matrixViewMenu->addAction(actionViewMatrix);
    QMenu *matrixPaletteMenu = matrixMenu->addMenu (tr("&Palette"));
	matrixPaletteMenu->addAction(actionMatrixDefaultScale);
	matrixPaletteMenu->addAction(actionMatrixGrayScale);
	matrixPaletteMenu->addAction(actionMatrixRainbowScale);
	matrixPaletteMenu->addAction(actionMatrixCustomScale);
	matrixMenu->addSeparator();
	matrixMenu->addAction(actionMatrixColumnRow);
    matrixMenu->addAction(actionMatrixXY);
	matrixMenu->addSeparator();
	QMenu *convertToTableMenu = matrixMenu->addMenu (tr("&Convert to Spreadsheet"));
	convertToTableMenu->addAction(actionConvertMatrixDirect);
	convertToTableMenu->addAction(actionConvertMatrixXYZ);
	convertToTableMenu->addAction(actionConvertMatrixYXZ);

	Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	bool empty = m->isEmpty();
	actionFlipMatrixVertically->setDisabled(empty);
	actionFlipMatrixHorizontally->setDisabled(empty);

	actionSmoothMatrix->setDisabled(empty);

	actionInvertMatrix->setDisabled(empty);
	actionMatrixDeterminant->setDisabled(empty);
	convertToTableMenu->setDisabled(empty);

	actionViewMatrixImage->setChecked(m->viewType() == Matrix::ImageView);
	actionViewMatrix->setChecked(m->viewType() == Matrix::TableView);
	actionMatrixColumnRow->setChecked(m->headerViewType() == Matrix::ColumnRow);
	actionMatrixColumnRow->setEnabled(m->viewType() == Matrix::TableView);
	actionMatrixXY->setChecked(m->headerViewType() == Matrix::XY);
	actionMatrixXY->setEnabled(m->viewType() == Matrix::TableView);

	actionMatrixDefaultScale->setChecked(m->colorMapType() == Matrix::Default);
    actionMatrixGrayScale->setChecked(m->colorMapType() == Matrix::GrayScale);
	actionMatrixRainbowScale->setChecked(m->colorMapType() == Matrix::Rainbow);
	actionMatrixCustomScale->setChecked(m->colorMapType() == Matrix::Custom);

    reloadCustomActions();
}

void ApplicationWindow::fileMenuAboutToShow()
{
	if (fileMenu)
		fileMenu->clear();
	if (newMenu)
		newMenu->clear();
	if (exportPlotMenu)
		exportPlotMenu->clear();
	if (importMenu)
		importMenu->clear();

	fileMenu->addMenu(newMenu);
	newMenu->addAction(actionNewProject);
    newMenu->addAction(actionNewFolder);
	newMenu->addAction(actionNewTable);
	newMenu->addAction(actionNewMatrix);
	newMenu->addAction(actionNewNote);
	newMenu->addAction(actionNewGraph);
	newMenu->addAction(actionNewFunctionPlot);
	newMenu->addAction(actionNewSurfacePlot);
	fileMenu->addAction(actionOpen);
	fileMenu->addAction(actionLoadImage);
	fileMenu->addAction(actionAppendProject);
    recent->setTitle(tr("&Recent Projects"));
    fileMenu->insertMenu(actionAppendProject, recent);
	recentMenuID = fileMenu->actions().indexOf(recent->menuAction());
	fileMenu->addAction(actionCloseProject);
	fileMenu->addSeparator();
	fileMenu->addAction(actionSaveProject);
	fileMenu->addAction(actionSaveProjectAs);
	fileMenu->addSeparator();
	fileMenu->addAction(actionSaveWindow);
	fileMenu->addAction(actionOpenTemplate);
	fileMenu->addAction(actionSaveTemplate);
	fileMenu->addSeparator();
	fileMenu->addAction(actionPrint);
	fileMenu->addAction(actionPrintPreview);
	fileMenu->addAction(actionPrintAllPlots);
	fileMenu->addSeparator();

	MdiSubWindow *w = activeWindow();
	if (w){
		if (QString(w->metaObject()->className()) == "MultiLayer" || QString(w->metaObject()->className()) == "Graph3D"){
			fileMenu->addMenu (exportPlotMenu);
			exportPlotMenu->addAction(actionExportGraph);
            exportPlotMenu->addAction(actionExportLayer);
			exportPlotMenu->addAction(actionExportAllGraphs);
            exportPlotMenu->addAction(actionSaveGraphAsProject);
		} else if (w->inherits("Table") || QString(w->metaObject()->className()) == "Matrix"){
			QMenu *exportMenu = fileMenu->addMenu(tr("Export"));
			exportMenu->addAction(actionShowExportASCIIDialog);
            exportMenu->addAction(actionShowAscii1dLocal);
			exportMenu->addAction(actionExportPDF);
			if (QString(w->metaObject()->className()) == "Matrix")
				exportMenu->addAction(actionExportMatrix);
		}
	}

	fileMenu->addMenu(importMenu);
	importMenu->addAction(actionLoad);
    importMenu->addAction(actionShowAscii1dLocal);
	importMenu->addAction(actionImportSound);
	importMenu->addAction(actionImportImage);
	importMenu->addAction(actionImportDatabase);

	fileMenu->addSeparator();
	fileMenu->addAction(actionCloseAllWindows);

	reloadCustomActions();
}

void ApplicationWindow::editMenuAboutToShow()
{
	MdiSubWindow *w = activeWindow();
	if (!w){
		actionUndo->setEnabled(false);
		actionRedo->setEnabled(false);
		return;
	}

	if (qobject_cast<Note *>(w)){
		QTextDocument *doc = ((Note *)w)->currentEditor()->document();
		actionUndo->setEnabled(doc->isUndoAvailable());
		actionRedo->setEnabled(doc->isRedoAvailable());
	} else if (qobject_cast<Matrix *>(w)){
		QUndoStack *stack = ((Matrix *)w)->undoStack();
		actionUndo->setEnabled(stack->canUndo());
		actionRedo->setEnabled(stack->canRedo());
	} else {
		actionUndo->setEnabled(false);
		actionRedo->setEnabled(false);
	}
}

void ApplicationWindow::windowsMenuAboutToShow()
{
	windowsMenu->clear();
	foldersMenu->clear();

	int folder_param = 0;
	Folder *f = projectFolder();
	while (f){
		if (folder_param < 9)
			foldersMenu->addAction("&" + QString::number(folder_param+1) + " " + f->path(), this, SLOT(foldersMenuActivated(bool)));
		else
			foldersMenu->addAction(f->path(), this, SLOT(foldersMenuActivated(bool)));
		foldersMenu->actions().last()->setData(folder_param);
		foldersMenu->actions().last()->setChecked(f == current_folder);
        folder_param++;
		f = f->folderBelow();
	}

    foldersMenu->setTitle(tr("&Folders"));
	windowsMenu->addMenu(foldersMenu);
	windowsMenu->addSeparator();

	QList<MdiSubWindow *> windows = current_folder->windowsList();
	int n = int(windows.count());
	if (!n){
		#ifdef SCRIPTING_PYTHON
			windowsMenu->addAction(actionShowScriptWindow);
		#endif
		return;
	}

	windowsMenu->addAction(tr("&Cascade"), this, SLOT(cascade()));
	windowsMenu->addAction(tr("&Tile"), d_workspace, SLOT(tileSubWindows()));
	windowsMenu->addSeparator();
	windowsMenu->addAction(actionNextWindow);
	windowsMenu->addAction(actionPrevWindow);
	windowsMenu->addSeparator();
	windowsMenu->addAction(actionFindWindow);
	windowsMenu->addAction(actionRename);
	windowsMenu->addAction(actionCopyWindow);
	windowsMenu->addSeparator();
#ifdef SCRIPTING_PYTHON
	windowsMenu->addAction(actionShowScriptWindow);
	windowsMenu->addSeparator();
#endif

	windowsMenu->addAction(actionResizeActiveWindow);
	windowsMenu->addAction(actionHideActiveWindow);
	windowsMenu->addAction(actionCloseWindow);

	if (n > 0)
		windowsMenu->addSeparator();

	bool moreWindows = (n >= 10);
	if (moreWindows)
		n = 9;

	for (int i = 0; i < n; ++i){
		MdiSubWindow *w = windows.at(i);
		if (!w)
			continue;

		windowsMenu->addAction("&" + QString::number(i + 1) + " " + w->windowTitle(), this, SLOT(windowsMenuActivated(bool)));
		windowsMenu->actions().last()->setData(i);
		windowsMenu->actions().last()->setChecked(current_folder->activeWindow() == w);
	}

	if (moreWindows){
		windowsMenu->addSeparator();
		windowsMenu->addAction(tr("More windows..."), this, SLOT(showMoreWindows()));
	}

	reloadCustomActions();
}

void ApplicationWindow::showMarkerPopupMenu()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    QMenu markerMenu(this);

	if (g->imageMarkerSelected()){
		markerMenu.addAction(QIcon(":/pixelProfile.png"),tr("&View Pixel Line profile"),this, SLOT(pixelLineProfile()));
		markerMenu.addAction(tr("&Intensity Matrix"),this, SLOT(intensityTable()));
		markerMenu.addSeparator();
	}
	if (!g->activeEnrichment())
		markerMenu.addAction(QIcon(":/cut.png"),tr("&Cut"),this, SLOT(cutSelection()));
	markerMenu.addAction(QIcon(":/copy.png"), tr("&Copy"),this, SLOT(copySelection()));
	markerMenu.addAction(QIcon(":/delete.png"), tr("&Delete"),this, SLOT(clearSelection()));
	markerMenu.addSeparator();

	if (g->activeEnrichment()){
		markerMenu.addAction(actionRaiseEnrichment);
		markerMenu.addAction(actionLowerEnrichment);
		markerMenu.addSeparator();
	}

	if (g->arrowMarkerSelected())
		markerMenu.addAction(tr("&Properties..."),this, SLOT(showLineDialog()));
	else
		markerMenu.addAction(tr("&Properties..."), this, SLOT(showEnrichementDialog()));

	markerMenu.exec(QCursor::pos());
}

void ApplicationWindow::lowerActiveEnrichment()
{
	raiseActiveEnrichment(false);
}

void ApplicationWindow::raiseActiveEnrichment(bool on)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (g->selectionMoveResizer())
        g->selectionMoveResizer()->raiseTargets(on);
}

void ApplicationWindow::alignTop()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	Graph* g = plot->activeLayer();
	if (!g)
		return;

	if (g->selectionMoveResizer())
		g->selectionMoveResizer()->alignTargetsTop();
}

void ApplicationWindow::alignBottom()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (g->selectionMoveResizer())
        g->selectionMoveResizer()->alignTargetsBottom();
}

void ApplicationWindow::alignLeft()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (g->selectionMoveResizer())
        g->selectionMoveResizer()->alignTargetsLeft();
}

void ApplicationWindow::alignRight()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (g->selectionMoveResizer())
        g->selectionMoveResizer()->alignTargetsRight();
}

void ApplicationWindow::graphSelectionChanged(SelectionMoveResizer *s)
{
	if (s){
		actionRaiseEnrichment->setEnabled(true);
		actionLowerEnrichment->setEnabled(true);
		if (s->widgetsList().size() > 1){
			actionAlignTop->setEnabled(true);
			actionAlignBottom->setEnabled(true);
			actionAlignLeft->setEnabled(true);
			actionAlignRight->setEnabled(true);
		}
	} else {
		actionRaiseEnrichment->setEnabled(false);
		actionLowerEnrichment->setEnabled(false);
		actionAlignTop->setEnabled(false);
		actionAlignBottom->setEnabled(false);
		actionAlignLeft->setEnabled(false);
		actionAlignRight->setEnabled(false);
	}
}

void ApplicationWindow::showMoreWindows()
{
	if (explorerWindow->isVisible())
		QMessageBox::information(this, "QtiSAS",tr("Please use the project explorer to select a window!"));
	else
		explorerWindow->show();
}

void ApplicationWindow::windowsMenuActivated(bool id)
{
	QList<MdiSubWindow *> windows = current_folder->windowsList();
	MdiSubWindow* w = windows.at(int(id));
	if ( w ){
		MdiSubWindow* aw = (MdiSubWindow*)d_workspace->activeSubWindow();
		bool maximize = aw && aw->isMaximized();

		d_workspace->setActiveSubWindow(w);
        if (maximize) w->showMaximized();
		else w->showNormal();
		w->setFocus();

		if(hidden(w)) hiddenWindows->takeAt(hiddenWindows->indexOf(w));
	}

}

void ApplicationWindow::foldersMenuActivated(bool id)
{
	int folder_param = 0;
	Folder *f = projectFolder();
	while (f){
		if (folder_param == int(id)){
			changeFolder (f);
			return;
		}

		folder_param++;
		f = f->folderBelow();
	}
}

void ApplicationWindow::newProject()
{
    if (showSaveProjectMessage() == QMessageBox::Cancel)
		return;

    saveSettings();//the recent projects must be saved

	ApplicationWindow *ed = new ApplicationWindow();
	ed->restoreApplicationGeometry();
	ed->initWindow();
	close();
}

void ApplicationWindow::savedProject()
{
	//QCoreApplication::processEvents();

	setWindowTitle(tr("QTISAS") + " - " + projectname);
	actionSaveProject->setEnabled(false);
	saved = true;

	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if (QString(w->metaObject()->className()) == "Matrix")
				((Matrix *)w)->undoStack()->setClean();
		}
		f = f->folderBelow();
	}
}

void ApplicationWindow::modifiedProject()
{
	if (!windowTitle().contains("*"))
		setWindowTitle(tr("QTISAS") + " - " + projectname + " *");

	if (saved == false)
		return;

	if (actionSaveProject)
		actionSaveProject->setEnabled(true);

	saved = false;
}

void ApplicationWindow::modifiedProject(MdiSubWindow *w)
{
	if (!w)
		return;

	modifiedProject();
}

void ApplicationWindow::timerEvent ( QTimerEvent *e)
{
	if (e->timerId() == savingTimerId)
		saveProject();
	else
		QWidget::timerEvent(e);
}

void ApplicationWindow::dropEvent( QDropEvent* e )
{
	if (!e->mimeData()->hasImage() && !e->mimeData()->hasUrls())
		return;

	MdiSubWindow *destWindow = nullptr;
	QList<QMdiSubWindow *> windows = d_workspace->subWindowList(QMdiArea::StackingOrder);
	QListIterator<QMdiSubWindow *> it(windows);
	it.toBack();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = d_workspace->mapFromGlobal(e->pos());
#else
    QPoint pos = d_workspace->mapFromGlobal(e->position().toPoint());
#endif
	while (it.hasPrevious()){
		QMdiSubWindow *w = it.previous();
		if (w->frameGeometry().contains(pos)){
			destWindow = (MdiSubWindow*)w;
			break;
		}
	}

	if (e->mimeData()->hasImage()){
		QImage image = qvariant_cast<QImage>(e->mimeData()->imageData());
		MultiLayer *ml = qobject_cast<MultiLayer *>(destWindow);
        if (ml){
            Graph *l = ml->layerAt(pos);
            if (l)
                l->addImage(image);
            else if (ml->activeLayer())
                ml->activeLayer()->addImage(image);
            else
                ml->addLayer()->addImage(image);
            return;
        }

		Matrix *m = qobject_cast<Matrix *>(destWindow);
        if (m)
			m->importImage(image);
		else {
			m = new Matrix(scriptEnv, image, "", this);
        	initMatrix(m, generateUniqueName(tr("Matrix")));
        	m->show();
		}
		return;
	}

	if (e->mimeData()->hasUrls()){
		QList<QUrl> urls = e->mimeData()->urls();
		QStringList fileNames;
		foreach(QUrl url, urls)
			fileNames << url.toLocalFile();

		QList<QByteArray> lst = QImageReader::supportedImageFormats() << "JPG";
		QStringList asciiFiles;

		for(int i = 0; i<(int)fileNames.count(); i++){
			QString fn = fileNames[i];
			QFileInfo fi (fn);
			QString ext = fi.suffix();
			QStringList tempList;
			// convert QList<QByteArray> to QStringList to be able to 'filter'
			foreach(QByteArray temp,lst)
				tempList.append(QString(temp));
			QStringList l = tempList.filter(ext, Qt::CaseInsensitive);
			if (l.count() > 0){
			    MultiLayer *ml = qobject_cast<MultiLayer *>(destWindow);
			    if (ml){
			        Graph *l = ml->layerAt(pos);
			        if (l)
                        l->addImage(fn);
			        else if (ml->activeLayer())
                        ml->activeLayer()->addImage(fn);
                    else
                        ml->addLayer()->addImage(fn);
			    } else {
			        Matrix *m = qobject_cast<Matrix *>(destWindow);
                    if (m)
                        m->importImage(fn);
                    else
                        importImage(fn, true);
			    }
			} else if ( ext == "opj" || ext == "qti")
				open(fn);
			else
				asciiFiles << fn;
		}

		if (asciiFiles.count() == 1)
			return importASCII(asciiFiles[0]);
		else
			importASCII(asciiFiles, ImportASCIIDialog::NewTables, columnSeparator, ignoredLines,
					renameColumns, strip_spaces, simplify_spaces, d_ASCII_import_comments,
					d_ASCII_import_locale, d_ASCII_comment_string, d_ASCII_import_read_only, d_ASCII_end_line);
	}
}

void ApplicationWindow::dragEnterEvent( QDragEnterEvent* e )
{
	if (e->source()){
		e->ignore();
		return;
	}

    if (e->mimeData()->hasUrls())
	    e->acceptProposedAction();
}

void ApplicationWindow::closeEvent( QCloseEvent* ce )
{
	switch(showSaveProjectMessage()){
		case QMessageBox::Yes:
			if (!saveProject()){
				ce->ignore();
				break;
			}
			saveSettings();
			ce->accept();
			break;

		case QMessageBox::No:
		default:
			saveSettings();
			ce->accept();
			break;

		case QMessageBox::Cancel:
			ce->ignore();
			break;
	}
}

#ifdef Q_OS_MACOS
void ApplicationWindow::hideEvent(QHideEvent *event)
{
    if (event->spontaneous())
        ((QtiPlotApplication *)QCoreApplication::instance())->updateDockMenu();
    event->accept();
}
#endif

QMessageBox::StandardButton ApplicationWindow::showSaveProjectMessage()
{
	if (!saved){
		QString s = tr("Save changes to project: <p><b> %1 </b> ?").arg(projectname);
		switch(QMessageBox::information(this, tr("QTISAS"), s, QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes)){
			case QMessageBox::Yes:
			saveProject();
			return QMessageBox::Yes;
			break;
			case QMessageBox::No:
			default:
				savedProject();
				return QMessageBox::No;
			break;
			case QMessageBox::Cancel:
				return QMessageBox::Cancel;
			break;
		}
	}
	return QMessageBox::No;
}

bool ApplicationWindow::closeProject(int savePolicy)
{
    if (savePolicy == 0)
        savedProject(); // discard the changes
    else if ((savePolicy == 1 && !saved && !saveProject()) ||
             (savePolicy < 0 && showSaveProjectMessage() == QMessageBox::Cancel))
        return false;

    blockSignals(true);

    Folder *f = projectFolder();
    f->folderListItem()->setText(0, tr("UNTITLED"));
    current_folder = f;
    projectname = "untitled";

    foreach (MdiSubWindow *w, f->windowsList())
    {
        w->askOnCloseEvent(false);
        closeWindow(w);
    }

    if (!(f->children()).isEmpty())
    {
        Folder *subFolder = f->folderBelow();
        int initial_depth = f->depth();
        while (subFolder && subFolder->depth() > initial_depth)
        {
            foreach (MdiSubWindow *w, subFolder->windowsList())
            {
                removeWindowFromLists(w);
                subFolder->removeWindow(w);
                delete w;
            }
            delete subFolder->folderListItem();
            delete subFolder;

            subFolder = f->folderBelow();
        }
    }

    blockSignals(false);
    savedProject();
    setWindowTitle("QTISAS - untitled");
    return true;
}

bool ApplicationWindow::openProjectInPlace(const QString &fn)
{
    return open(fn, false, false) == this;
}

void ApplicationWindow::customEvent(QEvent *e)
{
    if (e->type() == SCRIPTING_CHANGE_EVENT)
    {
        scriptingChangeEvent((ScriptingChangeEvent *)e);
        if (scriptEnv->objectName() != "Python")
            m_terminalMode = TerminalMode::Script;
        // Refresh after the environment changes, including directly delivered language-change events.
        if (m_terminalModeLabel)
            updateTerminalModeSuffix(terminal_line->hasFocus() ? QApplication::keyboardModifiers() : Qt::NoModifier);
    }
}

#ifdef Q_OS_MACOS
// A fullscreen-Space switch doesn't fire applicationStateChange
// this window's own activation does, so it's a second trigger for the same recovery
void ApplicationWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange && isActiveWindow())
        recoverOffscreenTopLevelWidgets(Qt::ApplicationActive);
}
#endif

void ApplicationWindow::deleteSelectedItems()
{
	if (folders->hasFocus() && folders->currentItem() != folders->topLevelItem(0))
	{//we never allow the user to delete the project folder item
		deleteFolder();
		return;
	}
	QList<Folder *> folderList;
	QList<MdiSubWindow *> windowList;
    QTreeWidgetItemIterator iter(lv);
	while (*iter) {
		if ((*iter)->isSelected()){
			if ((*iter)->type() == FolderListItem::itemType)
				folderList << ((FolderListItem *)(*iter))->folder();
			else
				windowList << ((WindowListItem *)(*iter))->window();
		}
		iter++;
	}

	folders->blockSignals(true);
	foreach(MdiSubWindow *w, windowList)
		w->close();
	foreach(Folder *f, folderList)
		deleteFolder(f);
	folders->blockSignals(false);
}

void ApplicationWindow::showListViewSelectionMenu(const QPoint &p)
{
	QMenu cm(this);
	cm.addAction(tr("&Show All Windows"), this, SLOT(showSelectedWindows()));
	cm.addAction(tr("&Hide All Windows"), this, SLOT(hideSelectedWindows()));
	cm.addSeparator();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cm.addAction(tr("&Delete Selection"), this, SLOT(deleteSelectedItems()), Qt::Key_F8);
#else
    cm.addAction(tr("&Delete Selection"), Qt::Key_F8, this, SLOT(deleteSelectedItems()));
#endif
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showListViewPopupMenu(const QPoint &p)
{
	QMenu cm(this);
	QMenu window(this);

	window.addAction(actionNewTable);
	window.addAction(actionNewMatrix);
	window.addAction(actionNewNote);
	window.addAction(actionNewGraph);
	window.addAction(actionNewFunctionPlot);
	window.addAction(actionNewSurfacePlot);
    window.setTitle(tr("New &Window"));
	cm.addMenu(&window);

	cm.addAction(actionNewFolder);
	cm.addSeparator();
	cm.addAction(tr("Auto &Column Width"), lv, SLOT(adjustColumns()));
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showWindowPopupMenu(const QPoint &pos)
{
	if (folders->isRenaming())
		return;

    QTreeWidgetItem *it = lv->itemAt(pos);
	if (!it) {
		showListViewPopupMenu(pos);
		return;
	}

	int selected = 0;
    QTreeWidgetItemIterator iter(lv);
	while (*iter) {
		if ((*iter)->isSelected())
			selected++;
        iter++;
	}
    if (selected > 1) {
        showListViewSelectionMenu(pos);
        return;
    }
	if (it->type() == FolderListItem::itemType) {
		current_folder = ((FolderListItem *)it)->folder();
		showFolderPopupMenu(pos, false);
		return;
	}

	MdiSubWindow *w = ((WindowListItem *)it)->window();
	if (w){
		QMenu cm(this);
		QMenu plots(this);

		cm.addAction(actionActivateWindow);
		cm.addAction(actionMinimizeWindow);
		cm.addAction(actionMaximizeWindow);
		cm.addSeparator();
        if (w->inherits("MultiLayer"))
        {
            cm.addAction(actionSaveGraphAsProject);
            cm.addSeparator();
        }
		if (!hidden(w))
			cm.addAction(actionHideWindow);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        cm.addAction(QIcon(":/close.png"), tr("&Delete Window"), w, SLOT(close()), Qt::Key_F8);
#else
        cm.addAction(QIcon(":/close.png"), tr("&Delete Window"), Qt::Key_F8, w, SLOT(close()));
#endif
		cm.addSeparator();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        cm.addAction(tr("&Rename Window"), this, SLOT(renameWindow()), Qt::Key_F2);
#else
        cm.addAction(tr("&Rename Window"), Qt::Key_F2, this, SLOT(renameWindow()));
#endif
		cm.addAction(actionResizeWindow);
		cm.addSeparator();
		cm.addAction(QIcon(":/fileprint.png"), tr("&Print Window"), w, SLOT(print()));
		cm.addSeparator();
		cm.addAction(tr("&Properties..."), this, SLOT(windowProperties()));

		if (w->inherits("Table")){
			QStringList graphs = dependingPlots(w->objectName());
			if (int(graphs.count())>0){
				cm.addSeparator();
				for (int i=0;i<int(graphs.count());i++)
					plots.addAction(graphs[i], window(graphs[i]), SLOT(showMaximized()));
                plots.setTitle(tr("D&epending Graphs"));
				cm.addMenu(&plots);
			}
		} else if (QString(w->metaObject()->className()) == "Matrix"){
			QStringList graphs = depending3DPlots((Matrix*)w);
			if (int(graphs.count())>0){
				cm.addSeparator();
				for (int i=0;i<int(graphs.count());i++)
					plots.addAction(graphs[i], window(graphs[i]), SLOT(showMaximized()));
                plots.setTitle(tr("D&epending 3D Graphs"));
				cm.addMenu(&plots);
			}
		} else if (QString(w->metaObject()->className()) == "MultiLayer") {
			tablesDepend->clear();
			QStringList tbls = multilayerDependencies(w);
			int n = int(tbls.count());
			if (n > 0){
				cm.addSeparator();
				for (int i=0; i<n; i++)
					tablesDepend->addAction(tbls[i]);
                tablesDepend->setTitle(tr("D&epends on"));
				cm.addMenu(tablesDepend);
			}
		} else if (QString(w->metaObject()->className()) == "Graph3D"){
			cm.addSeparator();
			Graph3D *sp = qobject_cast<Graph3D*>(w);
			Matrix *m = sp->matrix();
			if (m){
				plots.addAction(m->objectName(), m, SLOT(showMaximized()));
                plots.setTitle(tr("D&epends on"));
				cm.addMenu(&plots);
			} else if (sp->table()){
				tablesDepend->clear();
				tablesDepend->addAction(sp->table()->objectName());
                tablesDepend->setTitle(tr("D&epends on"));
				cm.addMenu(tablesDepend);
			} else if (!sp->formula().isEmpty()){
				plots.addAction(sp->formula());
                plots.setTitle(tr("Function"));
				cm.addMenu(&plots);
			}
		}
		cm.exec(QCursor::pos());
	}
}

void ApplicationWindow::showTable(QAction *action)
{
	Table *t = table(action->text());
	if (!t) return;

	t->showMaximized();
    updateWindowLists(t);
    auto items = lv->findItems(t->objectName(), Qt::MatchExactly | Qt::MatchCaseSensitive);
    QTreeWidgetItem *it = items.isEmpty() ? nullptr : items.first();
    if (it)
        it->setText(2, tr("Maximized"));
}

void ApplicationWindow::showTable(Table *w, const QString& curve)
{
	if (!w) return;

	int colIndex = w->colIndex(curve);
	w->setSelectedCol(colIndex);
	w->table()->clearSelection();
	w->table()->selectColumn(colIndex);
    QTableWidgetItem *item = w->table()->item(0, colIndex);
	w->table()->scrollToItem(item, QAbstractItemView::EnsureVisible);
    w->showNormal();
    updateWindowLists(w);
    auto items = lv->findItems(w->objectName(), Qt::MatchExactly | Qt::MatchCaseSensitive);
    QTreeWidgetItem *it = items.isEmpty() ? nullptr : items.first();
    if (it)
        it->setText(2, tr("Normal"));
	emit modified();
}

QStringList ApplicationWindow::depending3DPlots(Matrix *m)
{
	QStringList plots;
	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "Graph3D" && ((Graph3D *)w)->matrix() == m)
			plots << w->objectName();
	}
	return plots;
}

QStringList ApplicationWindow::dependingPlots(const QString& name)
{
	QStringList plots;

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows)
    {
		if (QString(w->metaObject()->className()) == "MultiLayer")
        {
			QList<Graph *> layers = ((MultiLayer*)w)->layersList();
			foreach(Graph *g, layers)
            {
				QStringList onPlot = g->curveNamesList();
				onPlot = onPlot.filter(name, Qt::CaseSensitive);
                if (int(onPlot.count()) && !plots.contains(w->objectName()))
                    plots << w->objectName();
			}
		}else if (QString(w->metaObject()->className()) == "Graph3D")
        {
            if ((((Graph3D*)w)->formula()).contains(name, Qt::CaseSensitive) && !plots.contains(w->objectName()))
				plots << w->objectName();
		}
	}
	return plots;
}

bool ApplicationWindow::showFullRangeAllPlots(const QString& tableName)
{
    QStringList graphsContainedTable=dependingPlots(tableName);
    if (graphsContainedTable.count()==0) return false;

    QList<MdiSubWindow *> windows = windowsList();

    foreach(MdiSubWindow *w, windows)
    {
        if (QString(w->metaObject()->className()) == "MultiLayer" && graphsContainedTable.contains(w->objectName()))
        {
            QList<Graph *> layers = ((MultiLayer*)w)->layersList();
            foreach(Graph *g, layers)
            {
                QStringList onPlot = g->curveNamesList();
                onPlot = onPlot.filter(tableName, Qt::CaseSensitive);

                for(int cn=0; cn<int(onPlot.count()); cn++)
                {
                    g->setCurveFullRange(g->curveIndex(onPlot[cn]));
                }

            }
        }
    }

    return true;
}

QStringList ApplicationWindow::multilayerDependencies(QWidget *w)
{
	QStringList tables;
	MultiLayer *g=(MultiLayer*)w;
	QList<Graph *> layers = g->layersList();
    foreach(Graph *ag, layers){
		QStringList onPlot = ag->curveNamesList();
		for (int j=0; j<onPlot.count(); j++)
		{
            QString tableName=onPlot[j].left(onPlot[j].lastIndexOf("_"));
            if (tableName!="" && !tables.contains(tableName) ) tables <<tableName;
		}
	}
	return tables;
}

void ApplicationWindow::showGraphContextMenu()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto ag = (Graph *)plot->activeLayer();
    if (!ag)
        return;

	QMenu cm(this);
	if (plot->isLayerSelected(ag)){
		cm.addAction(QPixmap(":/copy.png"), tr("&Copy"), this, SLOT(copyActiveLayer()));
		if (lastCopiedLayer)
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Layer"), this, SLOT(pasteSelection()));
		else if (d_enrichement_copy)
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste"), plot, SIGNAL(pasteMarker()));
		else if (d_arrow_copy)
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Line/Arrow"), plot, SIGNAL(pasteMarker()));
		cm.addAction(actionDeleteLayer);
		cm.addSeparator();
		cm.addAction(tr("P&roperties..."), this, SLOT(showGeneralPlotDialog()));
		cm.exec(QCursor::pos());
		return;
	}

	QMenu addMenu(this);
	if (ag->isPiePlot()){
		cm.addAction(tr("Re&move Pie Curve"),ag, SLOT(removePie()));
		addMenu.addAction(actionNewLegend);
	} else {
		if (ag->visibleCurves() != ag->curveCount()){
			cm.addAction(actionShowAllCurves);
			cm.addSeparator();
		}

		addMenu.addAction(actionShowCurvesDialog);
		addMenu.addAction(actionAddFunctionCurve);
		addMenu.addAction(actionAddErrorBars);
		addMenu.addAction(actionNewLegend);
		addMenu.addSeparator();
	}

	addMenu.addAction(actionAddFormula);
	addMenu.addAction(actionAddText);
	addMenu.addAction(btnArrow);
	addMenu.addAction(btnLine);
	addMenu.addAction(actionAddRectangle);
	addMenu.addAction(actionAddEllipse);
	addMenu.addAction(actionTimeStamp);
	addMenu.addAction(actionAddImage);
	addMenu.addSeparator();
	addMenu.addAction(actionAddLayer);
	addMenu.addAction(actionAddInsetLayer);
	addMenu.addAction(actionAddInsetCurveLayer);
    addMenu.setTitle(tr("&Add"));
	cm.addMenu(&addMenu);

	QMenu paletteMenu(this);
	if (!ag->isPiePlot()){
        analysisMenu->setTitle(tr("Anal&yze"));
		cm.addMenu(analysisMenu);
        plotDataMenu->setTitle(tr("&Data"));
		cm.addMenu(plotDataMenu);

		paletteMenu.addAction(tr("&Gray Scale"), ag, SLOT(setGrayScale()));
		paletteMenu.addAction(tr("&Indexed Colors"), ag, SLOT(setIndexedColors()));
        paletteMenu.setTitle(tr("Pale&tte"));
		cm.addMenu(&paletteMenu);
		cm.addSeparator();
	}

	QMenu copy(this);
	copy.addAction(tr("&Layer"), this, SLOT(copyActiveLayer()));
	copy.addAction(tr("&Window"), plot, SLOT(copyAllLayers()));
    copy.setIcon(QIcon(":/copy.png"));
    copy.setTitle(tr("&Copy"));
	cm.addMenu(&copy);

	if (lastCopiedLayer)
		cm.addAction(QPixmap(":/paste.png"), tr("&Paste Layer"), this, SLOT(pasteSelection()));
	else if (d_enrichement_copy){
		if (qobject_cast<LegendWidget *>(d_enrichement_copy))
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Text"), plot, SIGNAL(pasteMarker()));
		else if (qobject_cast<TexWidget *>(d_enrichement_copy))
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Tex Formula"), plot, SIGNAL(pasteMarker()));
		else if (qobject_cast<ImageWidget *>(d_enrichement_copy))
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Image"), plot, SIGNAL(pasteMarker()));
		else if (qobject_cast<RectangleWidget *>(d_enrichement_copy))
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Rectangle"), plot, SIGNAL(pasteMarker()));
		else if (qobject_cast<EllipseWidget *>(d_enrichement_copy))
			cm.addAction(QPixmap(":/paste.png"), tr("&Paste Ellipse"), plot, SIGNAL(pasteMarker()));
	} else if (d_arrow_copy)
		cm.addAction(QPixmap(":/paste.png"), tr("&Paste Line/Arrow"), plot, SIGNAL(pasteMarker()));

	QMenu exports(this);
	exports.addAction(tr("&Layer"), this, SLOT(exportLayer()));
	exports.addAction(tr("&Window"), this, SLOT(exportGraph()));
    exports.setTitle(tr("E&xport"));
	cm.addMenu(&exports);

	QMenu prints(this);
	prints.addAction(tr("&Layer"), plot, SLOT(printActiveLayer()));
	prints.addAction(tr("&Window"), plot, SLOT(print()));
    prints.setIcon(QIcon(":/fileprint.png"));
    prints.setTitle(tr("&Print"));
	cm.addMenu(&prints);

	cm.addSeparator();

	cm.addAction(tr("P&roperties..."), this, SLOT(showGeneralPlotDialog()));
	cm.addSeparator();
	cm.addAction(actionDeleteLayer);
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showWindowContextMenu()
{
	MdiSubWindow* w = activeWindow();
	if (!w)
		return;

	QMenu cm(this);
	QMenu plot3D(this);
	if (qobject_cast<MultiLayer *>(w)){
		MultiLayer *g = (MultiLayer*)w;
		if (!g->geometry().contains(d_workspace->mapFromGlobal(QCursor::pos())))
			return;

		if (lastCopiedLayer){
			cm.addAction(QIcon(":/paste.png"), tr("&Paste Layer"), this, SLOT(pasteSelection()));
			cm.addSeparator();
		}

		cm.addAction(actionAddLayer);
		if (g->numLayers() != 0)
			cm.addAction(actionDeleteLayer);

		cm.addAction(actionShowLayerDialog);
		cm.addSeparator();
		cm.addAction(actionRename);
		cm.addAction(actionCopyWindow);
		cm.addSeparator();
		cm.addAction(QIcon(":/copy.png"),tr("&Copy Page"), g, SLOT(copyAllLayers()));
		cm.addAction(tr("E&xport Page"), this, SLOT(exportGraph()));
		cm.addAction(actionPrint);
		cm.addSeparator();
		cm.addAction(tr("&Properties..."), this, SLOT(showGeneralPlotDialog()));
		cm.addSeparator();
		cm.addAction(actionCloseWindow);
	} else if (QString(w->metaObject()->className()) == "Graph3D"){
		Graph3D *g = (Graph3D*)w;
		if (!g->hasData()){
            plot3D.setTitle(tr("3D &Plot"));
			cm.addMenu(&plot3D);
			if (hasTable())
				plot3D.addAction(actionAdd3DData);
			if (matrixNames().count())
				plot3D.addAction(tr("&Matrix..."), this, SLOT(add3DMatrixPlot()));
			plot3D.addAction(actionEditSurfacePlot);
		} else {
			if (g->table())
				cm.addAction(tr("Choose &Data Set..."), this, SLOT(change3DData()));
			else if (g->matrix())
				cm.addAction(tr("Choose &Matrix..."), this, SLOT(change3DMatrix()));
			else if (g->userFunction() || g->parametricSurface())
				cm.addAction(actionEditSurfacePlot);
			cm.addAction(QPixmap(":/erase.png"), tr("C&lear"), g, SLOT(clearData()));
		}
		cm.addMenu(format);
		cm.addSeparator();
		cm.addAction(actionRename);
		cm.addAction(actionCopyWindow);
		cm.addSeparator();
		cm.addAction(tr("&Copy Graph"), g, SLOT(copyImage()));
		cm.addAction(tr("&Export") + "...", this, SLOT(exportGraph()));
		cm.addAction(actionPrint);
		cm.addSeparator();
		cm.addAction(actionCloseWindow);
	} else if (qobject_cast<Matrix *>(w)){
		Matrix *t = (Matrix *)w;
		cm.addMenu(plot3DMenu);
		cm.addSeparator();
		cm.addAction(actionSetMatrixProperties);
		cm.addAction(actionSetMatrixDimensions);
		cm.addSeparator();
		cm.addAction(actionSetMatrixValues);
		cm.addAction(actionTableRecalculate);
		cm.addSeparator();

		if (t->viewType() == Matrix::TableView){
			cm.addAction(QIcon(":/cut.png"),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.addAction(QIcon(":/copy.png"),tr("&Copy"), t, SLOT(copySelection()));
			cm.addAction(QIcon(":/paste.png"),tr("&Paste"), t, SLOT(pasteSelection()));
            cm.addSeparator();
			cm.addAction(QIcon(":/insert_row.png"), tr("&Insert Row"), t, SLOT(insertRow()));
			cm.addAction(QIcon(":/insert_column.png"), tr("&Insert Column"), t, SLOT(insertColumn()));
            if (t->numSelectedRows() > 0)
				cm.addAction(QIcon(":/delete_row.png"), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
            else if (t->numSelectedColumns() > 0)
				cm.addAction(QIcon(":/delete_column.png"), tr("&Delete Columns"), t, SLOT(deleteSelectedColumns()));

			cm.addAction(QIcon(":/erase.png"),tr("Clea&r"), t, SLOT(clearSelection()));
            cm.addSeparator();
            cm.addAction(actionViewMatrixImage);
		} else if (t->viewType() == Matrix::ImageView){
		    cm.addAction(actionImportImage);
            cm.addAction(actionExportMatrix);
            cm.addSeparator();
            cm.addAction(actionRotateMatrix);
            cm.addAction(actionRotateMatrixMinus);
            cm.addSeparator();
            cm.addAction(actionFlipMatrixVertically);
            cm.addAction(actionFlipMatrixHorizontally);
            cm.addSeparator();

			cm.addAction(actionExpandMatrix);
			cm.addAction(actionShrinkMatrix);
			cm.addAction(actionSmoothMatrix);
			cm.addSeparator();

            cm.addAction(actionTransposeMatrix);
            cm.addAction(actionInvertMatrix);
            cm.addSeparator();

			actionViewMatrix->setChecked(false);
			cm.addAction(actionViewMatrix);

			QMenu *paletteMenu = cm.addMenu (tr("&Palette"));
			paletteMenu->addAction(actionMatrixDefaultScale);
			paletteMenu->addAction(actionMatrixGrayScale);
			paletteMenu->addAction(actionMatrixRainbowScale);
			paletteMenu->addAction(actionMatrixCustomScale);

			actionMatrixDefaultScale->setChecked(t->colorMapType() == Matrix::Default);
			actionMatrixGrayScale->setChecked(t->colorMapType() == Matrix::GrayScale);
			actionMatrixRainbowScale->setChecked(t->colorMapType() == Matrix::Rainbow);
			actionMatrixCustomScale->setChecked(t->colorMapType() == Matrix::Custom);
		}
	}
	cm.exec(QCursor::pos());
}

void ApplicationWindow::customWindowTitleBarMenu(MdiSubWindow *w, QMenu *menu)
{
	menu->addSeparator();

	menu->addAction(actionSaveWindow);
	menu->addAction(actionPrint);
	menu->addSeparator();
	menu->addAction(actionCopyWindow);
	menu->addAction(actionRename);
	menu->addSeparator();

	if (w->inherits("Table") || QString(w->metaObject()->className()) == "Matrix") {
		menu->addAction(actionLoad);
		QMenu *exportMenu = menu->addMenu(tr("Export"));
		exportMenu->addAction(actionShowExportASCIIDialog);
        exportMenu->addAction(actionShowAscii1dLocal);
		exportMenu->addAction(actionExportPDF);
		if (QString(w->metaObject()->className()) == "Matrix")
			exportMenu->addAction(actionExportMatrix);
		menu->addSeparator();
	}

	if (QString(w->metaObject()->className()) == "Note")
		menu->addAction(actionSaveNote);
	else
		menu->addAction(actionSaveTemplate);
	menu->addSeparator();
	menu->addAction(actionHideActiveWindow);
}

void ApplicationWindow::showTableContextMenu(bool selection)
{
	Table *t = (Table*)activeWindow(TableWindow);
	if (!t)
		return;

	QMenu cm(this);
	QMenu moveRow(this);
	if (selection){
		if ((int)t->selectedColumns().count() > 0){
			showColMenu(t->firstSelectedColumn());
			return;
		} else if (t->numSelectedRows() == 1) {
			cm.addAction(actionShowColumnValuesDialog);
			cm.addAction(actionTableRecalculate);
			cm.addSeparator();
			cm.addAction(QIcon(":/cut.png"),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.addAction(QIcon(":/copy.png"),tr("&Copy"), t, SLOT(copySelection()));
			cm.addAction(QIcon(":/paste.png"),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.addSeparator();
			moveRow.addAction(actionMoveRowUp);
			moveRow.addAction(actionMoveRowDown);
			moveRow.setTitle(tr("Move Row"));
			cm.addMenu (&moveRow);
			cm.addAction(QIcon(":/insert_row.png"), tr("&Insert Row"), t, SLOT(insertRow()));
			cm.addAction(QIcon(":/delete_row.png"), tr("&Delete Row"), t, SLOT(deleteSelectedRows()));
			cm.addAction(QIcon(":/erase.png"), tr("Clea&r Row"), t, SLOT(clearSelection()));
		} else if (t->numSelectedRows() > 1) {
			cm.addAction(actionShowColumnValuesDialog);
			cm.addAction(QIcon(":/cut.png"),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.addAction(QIcon(":/copy.png"),tr("&Copy"), t, SLOT(copySelection()));
			cm.addAction(QIcon(":/paste.png"),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.addSeparator();
			cm.addAction(actionTableRecalculate);
			cm.addAction(QIcon(":/delete_row.png"), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
			cm.addAction(QIcon(":/erase.png"),tr("Clea&r Rows"), t, SLOT(clearSelection()));
		} else if (t->numRows() > 0 && t->numCols() > 0){
			cm.addAction(actionShowColumnValuesDialog);
			cm.addAction(QIcon(":/cut.png"),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.addAction(QIcon(":/copy.png"),tr("&Copy"), t, SLOT(copySelection()));
			cm.addAction(QIcon(":/paste.png"),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.addSeparator();
			cm.addAction(actionTableRecalculate);
			cm.addAction(QIcon(":/erase.png"),tr("Clea&r"), t, SLOT(clearSelection()));
		}
		cm.addSeparator();
		cm.addAction(actionShowColStatistics);
		cm.addAction(actionShowRowStatistics);
	} else {
		cm.addAction(actionShowExportASCIIDialog);
		cm.addSeparator();
		cm.addAction(actionAddColToTable);
		cm.addAction(actionClearTable);
		cm.addSeparator();
		cm.addAction(actionGoToRow);
		cm.addAction(actionGoToColumn);
	}
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showPlotWizard()
{
    QStringList lst = tableNames();
	if (lst.count() > 0){
		PlotWizard* pw = new PlotWizard(this);
		pw->insertTablesList(lst);
		pw->setColumnsList(columnsList(Table::All));
		pw->changeColumnsList(lst[0]);
		pw->exec();
	} else
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no tables available in this project.</h4>"
					"<p><h4>Please create a table and try again!</h4>"));
}

void ApplicationWindow::setCurveFullRange()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	Graph* g = plot->activeLayer();
	if (!g)
		return;

	g->setCurveFullRange(actionCurveFullRange->data().toInt());
}

void ApplicationWindow::showCurveRangeDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	Graph* g = plot->activeLayer();
	if (!g)
		return;

	showCurveRangeDialog(g, actionEditCurveRange->data().toInt());
}

CurveRangeDialog* ApplicationWindow::showCurveRangeDialog(Graph *g, int curve)
{
	if (!g)
		return 0;

	CurveRangeDialog* crd = new CurveRangeDialog(this);
	crd->setCurveToModify(g, curve);
	crd->exec();
	return crd;
}

FunctionDialog* ApplicationWindow::showFunctionDialog(Graph *g, int curve)
{
	if ( !g )
		return 0;

	FunctionDialog* fd = functionDialog();
	fd->setWindowTitle("QTISAS - Edit function");
	fd->setCurveToModify(g, curve);
	return fd;
}

FunctionDialog* ApplicationWindow::functionDialog()
{
	FunctionDialog* fd = new FunctionDialog(this);
	fd->setModal(true);
	fd->show();
	fd->activateWindow();
	return fd;
}

void ApplicationWindow::addFunctionCurve()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (g)
    {
		FunctionDialog* fd = functionDialog();
		if (fd)
			fd->setGraph(g);
	}
}

void ApplicationWindow::updateFunctionLists(int type, QStringList &formulas)
{
	int maxListSize = 10;
	if (type == 2 && formulas.size() >= 2){
		rFunctions.removeAll(formulas[0]);
		rFunctions.push_front(formulas[0]);

		thetaFunctions.removeAll(formulas[1]);
		thetaFunctions.push_front(formulas[1]);

		while ((int)rFunctions.size() > maxListSize)
			rFunctions.pop_back();
		while ((int)thetaFunctions.size() > maxListSize)
			thetaFunctions.pop_back();
	} else if (type == 1 && formulas.size() >= 2){
		xFunctions.removeAll(formulas[0]);
		xFunctions.push_front(formulas[0]);

		yFunctions.removeAll(formulas[1]);
		yFunctions.push_front(formulas[1]);

		while ((int)xFunctions.size() > maxListSize)
			xFunctions.pop_back();
		while ((int)yFunctions.size() > maxListSize)
			yFunctions.pop_back();
	} else if (type == 0 && formulas.size() >= 1){
		d_recent_functions.removeAll(formulas[0]);
		d_recent_functions.push_front(formulas[0]);

		while ((int)d_recent_functions.size() > maxListSize)
			d_recent_functions.pop_back();
	}
}

MultiLayer* ApplicationWindow::newFunctionPlot(QStringList &formulas, double start, double end, int points, const QString& var, int type)
{
	MultiLayer *ml = newGraph();
	if (ml){
		Graph *g = ml->activeLayer();
		if (g){
			g->enableAutoscaling();
			g->addFunction(formulas, start, end, points, var, type);
			g->enableAutoscaling(false);
		}
	}

	updateFunctionLists(type, formulas);
	return ml;
}

void ApplicationWindow::clearLogInfo()
{
	if (!current_folder->logInfo().isEmpty()){
		current_folder->clearLogInfo();
		results->setText("");
		emit modified();
	}
}

void ApplicationWindow::setFramed3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFramed();
	actionShowAxisDialog->setEnabled(true);
}

void ApplicationWindow::setBoxed3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setBoxed();
	actionShowAxisDialog->setEnabled(true);
}

void ApplicationWindow::removeAxes3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setNoAxes();
	actionShowAxisDialog->setEnabled(false);
}

void ApplicationWindow::removeGrid3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setPolygonStyle();
}

void ApplicationWindow::setHiddenLineGrid3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setHiddenLineStyle();
}

void ApplicationWindow::setPoints3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setDotStyle();
}

void ApplicationWindow::setCones3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setConeStyle();
}

void ApplicationWindow::setCrosses3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setCrossStyle();
}

void ApplicationWindow::setBars3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setBarStyle();
}

void ApplicationWindow::setLineGrid3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setWireframeStyle();
}

void ApplicationWindow::setFilledMesh3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFilledMeshStyle();
}

void ApplicationWindow::setFloorData3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFloorData();
}

void ApplicationWindow::setFloorIso3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFloorIsolines();
}

void ApplicationWindow::setEmptyFloor3DPlot()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setEmptyFloor();
}

void ApplicationWindow::setFrontGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFrontGrid(on);
}

void ApplicationWindow::setBackGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setBackGrid(on);
}

void ApplicationWindow::setFloorGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setFloorGrid(on);
}

void ApplicationWindow::setCeilGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setCeilGrid(on);
}

void ApplicationWindow::setRightGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setRightGrid(on);
}

void ApplicationWindow::setLeftGrid3DPlot(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setLeftGrid(on);
}

void ApplicationWindow::pickPlotStyle( QAction* action )
{
	if (!action )
		return;

	if (action == polygon)
		removeGrid3DPlot();
	else if (action == filledmesh)
		setFilledMesh3DPlot();
	else if (action == wireframe)
		setLineGrid3DPlot();
	else if (action == hiddenline)
		setHiddenLineGrid3DPlot();
	else if (action == pointstyle)
		setPoints3DPlot();
	else if (action == conestyle)
		setCones3DPlot();
	else if (action == crossHairStyle)
		setCrosses3DPlot();
	else if (action == barstyle)
		setBars3DPlot();

	emit modified();
}

void ApplicationWindow::pickCoordSystem( QAction* action)
{
	if (!action)
		return;

	if (action == Box || action == Frame)
	{
		if (action == Box)
			setBoxed3DPlot();
		if (action == Frame)
			setFramed3DPlot();
		grids->setEnabled(true);
	}
	else if (action == None)
	{
		removeAxes3DPlot();
		grids->setEnabled(false);
	}

	emit modified();
}

void ApplicationWindow::pickFloorStyle( QAction* action )
{
	if (!action)
		return;

	if (action == floordata)
		setFloorData3DPlot();
	else if (action == flooriso)
		setFloorIso3DPlot();
	else
		setEmptyFloor3DPlot();

	emit modified();
}

void ApplicationWindow::custom3DActions(QMdiSubWindow *w)
{
	if (w && QString(w->metaObject()->className()) == "Graph3D")
	{
		Graph3D* plot = (Graph3D*)w;
		actionAnimate->setChecked(plot->isAnimated());
		actionPerspective->setChecked(!plot->isOrthogonal());
		switch(plot->plotStyle())
		{
			case FILLEDMESH:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( true );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case FILLED:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( true );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case Qwt3D::USER:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( false );

				if (plot->pointType() == Graph3D::VerticalBars)
				{
					pointstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( false );
					barstyle->setChecked( true );
				}
				else if (plot->pointType() == Graph3D::Dots)
				{
					pointstyle->setChecked( true );
					barstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( false );
				}
				else if (plot->pointType() == Graph3D::HairCross)
				{
					pointstyle->setChecked( false );
					barstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( true );
				}
				else if (plot->pointType() == Graph3D::Cones)
				{
					pointstyle->setChecked( false );
					barstyle->setChecked( false );
					conestyle->setChecked( true );
					crossHairStyle->setChecked( false );
				}
				break;

			case WIREFRAME:
				wireframe->setChecked( true );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case HIDDENLINE:
				wireframe->setChecked( false );
				hiddenline->setChecked( true );
				polygon->setChecked( false );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			default:
				break;
		}

		switch(plot->coordStyle())
		{
			case Qwt3D::NOCOORD:
				None->setChecked( true );
				Box->setChecked( false );
				Frame->setChecked( false );
				break;

			case Qwt3D::BOX:
				None->setChecked( false );
				Box->setChecked( true );
				Frame->setChecked( false );
				break;

			case Qwt3D::FRAME:
				None->setChecked(false );
				Box->setChecked( false );
				Frame->setChecked(true );
				break;
		}

		switch(plot->floorStyle())
		{
			case NOFLOOR:
				floornone->setChecked( true );
				flooriso->setChecked( false );
				floordata->setChecked( false );
				break;

			case FLOORISO:
				floornone->setChecked( false );
				flooriso->setChecked( true );
				floordata->setChecked( false );
				break;

			case FLOORDATA:
				floornone->setChecked(false );
				flooriso->setChecked( false );
				floordata->setChecked(true );
				break;
		}
		custom3DGrids(plot->grids());
	}
}

void ApplicationWindow::custom3DGrids(int grids)
{
	if (Qwt3D::BACK & grids)
		back->setChecked(true);
	else
		back->setChecked(false);

	if (Qwt3D::FRONT & grids)
		front->setChecked(true);
	else
		front->setChecked(false);

	if (Qwt3D::CEIL & grids)
		ceil->setChecked(true);
	else
		ceil->setChecked(false);

	if (Qwt3D::FLOOR & grids)
		floor->setChecked(true);
	else
		floor->setChecked(false);

	if (Qwt3D::RIGHT & grids)
		right->setChecked(true);
	else
		right->setChecked(false);

	if (Qwt3D::LEFT & grids)
		left->setChecked(true);
	else
		left->setChecked(false);
}

void ApplicationWindow::initPlot3DToolBar()
{
    plot3DTools = new QToolBar(tr("3D Surface"), this);
    plot3DTools->setObjectName("plot3DTools");
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(Qt::TopToolBarArea, plot3DTools);
#ifdef Q_OS_MACOS
    plot3DTools->setIconSize(QSize(20, 20));
#endif
	coord = new QActionGroup( this );
	Box = new QAction( coord );
	Box->setIcon(QIcon(":/box.png"));
	Box->setCheckable(true);

	Frame = new QAction( coord );
	Frame->setIcon(QIcon(":/free_axes.png"));
	Frame->setCheckable(true);

	None = new QAction( coord );
	None->setIcon(QIcon(":/no_axes.png"));
	None->setCheckable(true);

	plot3DTools->addAction(Frame);
	plot3DTools->addAction(Box);
	plot3DTools->addAction(None);
	Box->setChecked( true );

	plot3DTools->addSeparator();

	// grid actions
	grids = new QActionGroup( this );
	grids->setEnabled( true );
	grids->setExclusive( false );
	front = new QAction( grids );
	front->setCheckable( true );
	front->setIcon(QIcon(":/frontGrid.png"));
	back = new QAction( grids );
	back->setCheckable( true );
	back->setIcon(QIcon(":/backGrid.png"));
	right = new QAction( grids );
	right->setCheckable( true );
	right->setIcon(QIcon(":/leftGrid.png"));
	left = new QAction( grids );
	left->setCheckable( true );
	left->setIcon(QIcon(":/rightGrid.png"));
	ceil = new QAction( grids );
	ceil->setCheckable( true );
	ceil->setIcon(QIcon(":/ceilGrid.png"));
	floor = new QAction( grids );
	floor->setCheckable( true );
	floor->setIcon(QIcon(":/floorGrid.png"));

	plot3DTools->addAction(front);
	plot3DTools->addAction(back);
	plot3DTools->addAction(right);
	plot3DTools->addAction(left);
	plot3DTools->addAction(ceil);
	plot3DTools->addAction(floor);

	plot3DTools->addSeparator();

	actionPerspective = new QAction( this );
	actionPerspective->setCheckable(true);
	actionPerspective->setIcon(QIcon(":/perspective.png"));
    plot3DTools->addAction(actionPerspective);
	actionPerspective->setChecked(!d_3D_orthogonal);
	connect(actionPerspective, SIGNAL(toggled(bool)), this, SLOT(togglePerspective(bool)));

	actionResetRotation = new QAction( this );
	actionResetRotation->setCheckable(false);
	actionResetRotation->setIcon(QIcon(":/reset_rotation.png"));
    plot3DTools->addAction(actionResetRotation);
	connect(actionResetRotation, SIGNAL(triggered()), this, SLOT(resetRotation()));

	actionFitFrame = new QAction( this );
	actionFitFrame->setCheckable(false);
	actionFitFrame->setIcon(QIcon(":/fit_frame.png"));
    plot3DTools->addAction(actionFitFrame);
	connect(actionFitFrame, SIGNAL(triggered()), this, SLOT(fitFrameToLayer()));

	plot3DTools->addSeparator();

	//plot style actions
	plotstyle = new QActionGroup( this );
	wireframe = new QAction( plotstyle );
	wireframe->setCheckable( true );
	wireframe->setEnabled( true );
	wireframe->setIcon(QIcon(":/lineMesh.png"));
	hiddenline = new QAction( plotstyle );
	hiddenline->setCheckable( true );
	hiddenline->setEnabled( true );
	hiddenline->setIcon(QIcon(":/grid_only.png"));
	polygon = new QAction( plotstyle );
	polygon->setCheckable( true );
	polygon->setEnabled( true );
	polygon->setIcon(QIcon(":/no_grid.png"));
	filledmesh = new QAction( plotstyle );
	filledmesh->setCheckable( true );
	filledmesh->setIcon(QIcon(":/grid_poly.png"));
	pointstyle = new QAction( plotstyle );
	pointstyle->setCheckable( true );
	pointstyle->setIcon(QIcon(":/pointsMesh.png"));

	conestyle = new QAction( plotstyle );
	conestyle->setCheckable( true );
	conestyle->setIcon(QIcon(":/cones.png"));

	crossHairStyle = new QAction( plotstyle );
	crossHairStyle->setCheckable( true );
	crossHairStyle->setIcon(QIcon(":/crosses.png"));

	barstyle = new QAction( plotstyle );
	barstyle->setCheckable( true );
	barstyle->setIcon(QIcon(":/plot_bars.png"));

	plot3DTools->addAction(barstyle);
	plot3DTools->addAction(pointstyle);

	plot3DTools->addAction(conestyle);
	plot3DTools->addAction(crossHairStyle);
	plot3DTools->addSeparator();

	plot3DTools->addAction(wireframe);
	plot3DTools->addAction(hiddenline);
	plot3DTools->addAction(polygon);
	plot3DTools->addAction(filledmesh);
	filledmesh->setChecked( true );

	plot3DTools->addSeparator();

	//floor actions
	floorstyle = new QActionGroup( this );
	floordata = new QAction( floorstyle );
	floordata->setCheckable( true );
	floordata->setIcon(QIcon(":/floor.png"));
	flooriso = new QAction( floorstyle );
	flooriso->setCheckable( true );
	flooriso->setIcon(QIcon(":/isolines.png"));
	floornone = new QAction( floorstyle );
	floornone->setCheckable( true );
	floornone->setIcon(QIcon(":/no_floor.png"));

	plot3DTools->addAction(floordata);
	plot3DTools->addAction(flooriso);
	plot3DTools->addAction(floornone);
	floornone->setChecked( true );

	plot3DTools->addSeparator();

	actionAnimate = new QAction( this );
	actionAnimate->setCheckable(true);
	actionAnimate->setIcon(QPixmap(":/movie.png"));
	plot3DTools->addAction(actionAnimate);

	plot3DTools->hide();

	connect(actionAnimate, SIGNAL(toggled(bool)), this, SLOT(toggle3DAnimation(bool)));
	connect( coord, SIGNAL( triggered( QAction* ) ), this, SLOT( pickCoordSystem( QAction* ) ) );
	connect( floorstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickFloorStyle( QAction* ) ) );
	connect( plotstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickPlotStyle( QAction* ) ) );

	connect( left, SIGNAL( triggered( bool ) ), this, SLOT( setLeftGrid3DPlot(bool) ));
	connect( right, SIGNAL( triggered( bool ) ), this, SLOT( setRightGrid3DPlot( bool ) ) );
	connect( ceil, SIGNAL( triggered( bool ) ), this, SLOT( setCeilGrid3DPlot( bool ) ) );
	connect( floor, SIGNAL( triggered( bool ) ), this, SLOT(setFloorGrid3DPlot( bool ) ) );
	connect( back, SIGNAL( triggered( bool ) ), this, SLOT(setBackGrid3DPlot( bool ) ) );
	connect( front, SIGNAL( triggered( bool ) ), this, SLOT( setFrontGrid3DPlot( bool ) ) );
}

void ApplicationWindow::pixelLineProfile()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

	bool ok;
	int res = QInputDialog::getInt(this, tr("QTISAS - Set the number of pixels to average"),
                                       tr("Number of averaged pixels"), 1, 1, 2000, 2, &ok);
	if ( !ok )
		return;

    auto lpt = new LineProfileTool(g, this, res);
    g->setActiveTool(lpt);
}

void ApplicationWindow::intensityTable()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (g)
    {
        auto im = qobject_cast<ImageWidget *>(g->activeEnrichment());
        if (im)
        {
            QString fn = im->fileName();
            if (!fn.isEmpty())
                importImage(fn);
        }
    }
}

void ApplicationWindow::autoArrangeLayers()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;
    plot->autoArrangeLayers();
}

void ApplicationWindow::extractGraphs()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->numLayers() < 2){
        QMessageBox::critical(this, tr("QTISAS - Error"),
        tr("You must have more than one layer in the active window!"));
		return;
    }

    QList<Graph *> lst = plot->layersList();
    foreach(Graph *g, lst){
		MultiLayer *nw = multilayerPlot(generateUniqueName(tr("Graph")), 0, plot->getRows(), plot->getCols());
        nw->resize(plot->size());
		Graph *ng = nw->addLayer(g->pos().x(), g->pos().y(), g->width(), g->height());
		if (ng)
            ng->copy(g);
    }
}

void ApplicationWindow::extractLayers()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    int curves = g->curveCount();
    if (curves < 2){
        QMessageBox::critical(this, tr("QTISAS - Error"),
        tr("You must have more than one dataset in the active layer!"));
		return;
    }

    for(int i = 0; i < curves; i++){
		Graph *ng = plot->addLayer(g->pos().x(), g->pos().y(), g->width(), g->height());
		if (ng){
            ng->copy(g);
            for(int j = 0; j < curves; j++){
                if (j != i)
                    ng->removeCurve(j);
            }
		}
    }
    plot->removeLayer(g);
    plot->arrangeLayers(true, false);
}

void ApplicationWindow::addInsetLayer(bool curves)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto al = (Graph *)plot->activeLayer();
    if (!al)
        return;

    QRect r = al->geometry();
	Graph *g = plot->addLayer(r.x() + r.width()/2, al->canvas()->y(), r.width()/2, r.height()/2, true);
    if (g){
        g->setTitle("");
        g->setAxisTitle(QwtPlot::xBottom, "");
        g->setAxisTitle(QwtPlot::yLeft, "");
        g->enableAxis(QwtPlot::yRight, false);
        g->enableAxis(QwtPlot::xTop, false);

        QColor c = Qt::white;
        c.setAlpha(0);
        g->setBackgroundColor(c);
        g->setCanvasBackground(c);
        if (curves)
            g->copyCurves(al);
    }
}

void ApplicationWindow::addInsetCurveLayer()
{
    addInsetLayer(true);
}

void ApplicationWindow::addLayer()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->numLayers() == 0)
    {
		setPreferences(plot->addLayer());
		return;
	}

    switch (QMessageBox::information(this, tr("QTISAS - Guess best origin for the new layer?"),
                                     tr("Do you want QtiSAS to guess the best position for the new layer?\n"
                                        "Warning: this will rearrange existing layers!"),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
    {
    case QMessageBox::Yes:
				setPreferences(plot->addLayer());
				plot->arrangeLayers(true, true);
		break;
    case QMessageBox::No:
			setPreferences(plot->addLayer(0, 0, plot->canvasRect().width(), plot->canvasRect().height()));
		break;
    default:
			break;
	}
}

void ApplicationWindow::deleteLayer()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    plot->confirmRemoveLayer();
}

Note* ApplicationWindow::openNote(ApplicationWindow* app, const QStringList &flist)
{
    QStringList lst = flist[0].split("\t", Qt::SkipEmptyParts);
	QString caption = lst[0];
	Note* w = app->newNote(caption);
	if (lst.count() == 2){
		app->setListViewDate(caption, lst[1]);
		w->setBirthDate(lst[1]);
	}

	if (flist.size() >= 2)
		restoreWindowGeometry(app, w, flist[1]);

	if (flist.size() >= 3){
		lst = flist[2].split("\t");
		if (lst.size() >= 3){
			w->setWindowLabel(lst[1]);
			w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
		}
	}
	return w;
}

Matrix* ApplicationWindow::openMatrix(ApplicationWindow* app, const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list=(*line).split("\t");
	QString caption=list[0];
	int rows = list[1].toInt();
	int cols = list[2].toInt();

	Matrix* w = app->newMatrix(caption, rows, cols);
	app->setListViewDate(caption,list[3]);
	w->setBirthDate(list[3]);

	for (line++; line!=flist.end(); line++){
		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry") {
			restoreWindowGeometry(app, w, *line);
		} else if (fields[0] == "ColWidth") {
			w->setColumnsWidth(fields[1].toInt());
		} else if (fields[0] == "Formula") {
			w->setFormula(fields[1]);
		} else if (fields[0] == "<formula>") {
			QString formula;
			for (line++; line!=flist.end() && *line != "</formula>"; line++)
				formula += *line + "\n";
			formula.truncate(formula.length()-1);
			w->setFormula(formula);
		} else if (fields[0] == "TextFormat") {
			if (fields[1] == "f")
				w->setTextFormat('f', fields[2].toInt());
			else
				w->setTextFormat('e', fields[2].toInt());
		} else if (fields[0] == "WindowLabel" && fields.size() >= 3) { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
		} else if (fields[0] == "Coordinates") { // d_file_version > 81
			w->setCoordinates(fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble(), fields[4].toDouble());
		} else if (fields[0] == "ViewType") { // d_file_version > 90
			w->setViewType((Matrix::ViewType)fields[1].toInt());
		} else if (fields[0] == "HeaderViewType") { // d_file_version > 90
			w->setHeaderViewType((Matrix::HeaderViewType)fields[1].toInt());
		} else if (fields[0] == "ColorPolicy"){// d_file_version > 90
			w->setColorMapType((Matrix::ColorMapType)fields[1].toInt());
		} else if (fields[0] == "<ColorMap>"){// d_file_version > 90
			QStringList lst;
			while ( *line != "</ColorMap>" )
				lst << *(++line);
			lst.pop_back();
			w->setColorMap(LinearColorMap::fromXmlStringList(lst));
		} else // <data> or values
			break;
	}
	if (*line == "<data>") line++;

	//read and set table values
	for (; line!=flist.end() && *line != "</data>"; line++){
		QStringList fields = (*line).split("\t");
		int row = fields[0].toInt();
		for (int col=0; col<cols; col++){
		    QString cell = fields[col+1];
		    if (cell.isEmpty())
                continue;

		    if (d_file_version < 90)
                w->setCell(row, col, QLocale::c().toDouble(cell));
            else if (d_file_version == 90)
                w->setText(row, col, cell);
			else
				w->setCell(row, col, cell.toDouble());
		}
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	w->resetView();

	return w;
}

Table* ApplicationWindow::openTable(ApplicationWindow* app, const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list = (*line).split("\t");
	QString caption = list[0];
	int rows = list[1].toInt();
	int cols = list[2].toInt();

	Table* w = app->newTable(caption, rows, cols);
	app->setListViewDate(caption, list[3]);
    w->setBirthDate(list[3]);

    for (line++; line!=flist.end(); line++)
    {

		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry" || fields[0] == "tgeometry")
        {
			restoreWindowGeometry(app, w, *line);
		}
        else if (fields[0] == "header")
        {
            if (cols == 0)
                continue;

			fields.pop_front();
			if (d_file_version >= 78)
            {
                w->loadHeader(fields);
            }
			else if (list.size() >= 7)
            {
				w->setColPlotDesignation(list[4].toInt(), Table::X);
				w->setColPlotDesignation(list[6].toInt(), Table::Y);
				w->setHeader(fields);
			}
		}
        else if (fields[0] == "ColWidth")
        {
            if (cols == 0)
                continue;

			fields.pop_front();
			w->setColWidths(fields);
		}
        else if (fields[0] == "com")
        { // legacy code
			w->setCommands(*line);
		}
        else if (fields[0] == "<com>")
        {
			for (line++; line!=flist.end() && *line != "</com>"; line++){
				int col = (*line).mid(9,(*line).length()-11).toInt();
				QString formula;
				for (line++; line!=flist.end() && *line != "</col>"; line++)
					formula += *line + "\n";
				formula.truncate(formula.length()-1);
				w->setCommand(col,formula);
			}
		} else if (fields[0] == "ColType") { // d_file_version > 65
            if (cols == 0)
                continue;

			fields.pop_front();
			w->setColumnTypes(fields);
		} else if (fields[0] == "Comments") { // d_file_version > 71
            if (cols == 0)
                continue;

			fields.pop_front();
			w->setColComments(fields);
			w->setHeaderColType();
		} else if (fields[0] == "WindowLabel" && fields.size() >= 3) { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
		} else if (fields[0] == "ReadOnlyColumn") { // d_file_version > 91
            if (cols == 0)
                continue;

			fields.pop_front();
			for (int i=0; i < w->numCols(); i++)
				w->setReadOnlyColumn(i, fields[i] == "1");
		} else if (fields[0] == "HiddenColumn") { // d_file_version >= 93
            if (cols == 0)
                continue;

			fields.pop_front();
			for (int i=0; i < w->numCols(); i++)
				w->hideColumn(i, fields[i] == "1");
		} else // <data> or values
			break;
	}

    QApplication::setOverrideCursor(Qt::WaitCursor);
	w->table()->blockSignals(true);
	for (line++; line!=flist.end() && *line != "</data>"; line++){//read and set table values
		QStringList fields = (*line).split("\t");
		int row = fields[0].toInt();
		for (int col=0; col<cols; col++){
		    if (fields.count() >= col+2){
		        QString cell = fields[col+1];
		        if (cell.isEmpty())
                    continue;

				if (w->columnType(col) == Table::Numeric){
		        	if (d_file_version < 90)
                    	w->setCell(row, col, QLocale::c().toDouble(cell.replace(",", ".")));
					else if (d_file_version == 90)
						w->setText(row, col, cell);
					else if (d_file_version >= 91)
						w->setCell(row, col, cell.toDouble());
		        } else
                    w->setText(row, col, cell);
		    }
		}
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
    QApplication::restoreOverrideCursor();

	w->table()->blockSignals(false);

    return w;
}

TableStatistics* ApplicationWindow::openTableStatistics(const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list=(*line++).split("\t");
	QString caption=list[0];

	QList<int> targets;
	for (int i = 1; i <= (*line).count('\t'); i++)
		targets << (*line).section('\t',i,i).toInt();

	TableStatistics* w = newTableStatistics(0, list[2] == "row" ? TableStatistics::row : TableStatistics::column, targets, 0, -1, list[0]);
	w->setBaseName(list[1]);

	setListViewDate(caption, list[3]);
	w->setBirthDate(list[3]);

	for (line++; line!=flist.end(); line++){
		QStringList fields = (*line).split("\t");

		if (fields[0] == "ColStatType"){
			QList <int> colStatTypes;
			for (int i = 1; i < fields.size(); i++)
				colStatTypes << fields[i].toInt();
			w->setColumnStatsTypes(colStatTypes);
		} else if (fields[0] == "Range"){
			w->setRange(fields[1].toInt(), fields[2].toInt());
		} else if (fields[0] == "geometry")
			restoreWindowGeometry(this, w, *line);
		else if (fields[0] == "header")
        {
			fields.pop_front();

			if (w->numCols() != fields.size())
				w->setNumCols(fields.size());

			if (d_file_version >= 78)
				w->loadHeader(fields);
			else
            {
				w->setColPlotDesignation(list[4].toInt(), Table::X);
				w->setColPlotDesignation(list[6].toInt(), Table::Y);
				w->setHeader(fields);
			}
		}
        else if (fields[0] == "ColWidth")
        {
			fields.pop_front();
			w->setColWidths(fields);
		}
        else if (fields[0] == "com")
        { // legacy code
			w->setCommands(*line);
		}
        else if (fields[0] == "<com>")
        {
			for (line++; line!=flist.end() && *line != "</com>"; line++)
			{
				int col = (*line).mid(9,(*line).length()-11).toInt();
				QString formula;
				for (line++; line!=flist.end() && *line != "</col>"; line++)
					formula += *line + "\n";
				formula.truncate(formula.length()-1);
				w->setCommand(col,formula);
			}
		}
        else if (fields[0] == "ColType")
        { // d_file_version > 65
			fields.pop_front();
			w->setColumnTypes(fields);
		}
        else if (fields[0] == "Comments")
        { // d_file_version > 71
			fields.pop_front();
			w->setColComments(fields);
		}
        else if (fields[0] == "WindowLabel" && fields.size() >= 3) { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
		}
	}
	return w;
}

Graph* ApplicationWindow::openGraph(ApplicationWindow* app, MultiLayer *plot, const QStringList &list)
{
    Graph* ag = 0;
    openGraph(app, plot, list, ag, true);
    return ag;
}

void ApplicationWindow::openGraph(ApplicationWindow* app, MultiLayer *plot, const QStringList &list, Graph* &ag, bool newYN)
{
	int curveID = 0;
	QList<int> mcIndexes;
	QList<ErrorBarsCurve *> errBars;
    QStringList fListGeometry, lstGeometry;
    QRect savedCanvasGeometry;
	for (int j = 0; j < list.count() - 1; j++)
    {
		QString s = list[j];
		if (s.contains ("ggeometry"))
        {
			fListGeometry = s.split("\t");

			if (newYN) ag = (Graph*)plot->addLayer(fListGeometry[1].toInt(), fListGeometry[2].toInt(), fListGeometry[3].toInt(), fListGeometry[4].toInt());
            else
            {
                ag->setGeometry(fListGeometry[1].toInt(), fListGeometry[2].toInt(), fListGeometry[3].toInt(), fListGeometry[4].toInt());
            }

            ag->blockSignals(true);
			ag->setAxisTitlePolicy(d_graph_axis_labeling);
		}
		else if (s.startsWith ("<PageGeometry>") && s.endsWith ("</PageGeometry>"))
		{
            lstGeometry = s.remove("<PageGeometry>").remove("</PageGeometry>").split("\t", Qt::SkipEmptyParts);
			ag->setPageGeometry(QRectF(lstGeometry[0].toDouble(), lstGeometry[1].toDouble(), lstGeometry[2].toDouble(), lstGeometry[3].toDouble()));
		}
		else if (s.left(10) == "Background"){
			QStringList fList = s.split("\t");
			QColor c = QColor(fList[1]);
			if (fList.count() == 3)
				c.setAlpha(fList[2].toInt());
			ag->setBackgroundColor(c);
		}
		else if (s.contains ("Margin")){
			QStringList fList=s.split("\t");
            ag->plotLayout()->setCanvasMargin(fList[1].toInt());
		}
		else if (s.contains ("Border")){
			QStringList fList=s.split("\t");
			ag->setFrame(fList[1].toInt(), QColor(fList[2]));
		}
		else if (s.contains ("EnabledAxes")){
			QStringList fList=s.split("\t");
			fList.pop_front();
			for (int i=0; i<(int)fList.count(); i++)
				ag->enableAxis(i, fList[i].toInt());
		}
		else if (s.contains ("AxesBaseline")){
            QStringList fList = s.split("\t", Qt::SkipEmptyParts);
			fList.pop_front();
			for (int i=0; i<(int)fList.count(); i++)
				ag->setAxisMargin(i, fList[i].toInt());
		}
		else if (s.contains ("EnabledTicks"))
		{//version < 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			fList.replaceInStrings("-1", "3");
			ag->setMajorTicksType(fList);
			ag->setMinorTicksType(fList);
		}
		else if (s.contains ("MajorTicks"))
		{//version >= 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setMajorTicksType(fList);
		}
		else if (s.contains ("MinorTicks"))
		{//version >= 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setMinorTicksType(fList);
		}
		else if (s.contains ("TicksLength")){
			QStringList fList=s.split("\t");
			ag->setTicksLength(fList[1].toInt(), fList[2].toInt());
		}
		else if (s.contains ("EnabledTickLabels")){
			QStringList fList=s.split("\t");
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->enableAxisLabels(i, fList[i].toInt());
		}
		else if (s.contains ("AxesColors")){
			QStringList fList = s.split("\t");
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->setAxisColor(i, QColor(fList[i]));
		}
		else if (s.contains ("AxesNumberColors")){
            QStringList fList = s.split("\t");
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->setAxisLabelsColor(i, QColor(fList[i]));
		}
		else if (s.left(5)=="grid\t"){
			ag->grid()->load(s.split("\t"));
		}
		else if (s.startsWith ("<Antialiasing>") && s.endsWith ("</Antialiasing>")){
			bool antialiasing = s.remove("<Antialiasing>").remove("</Antialiasing>").toInt();
			ag->setAntialiasing(antialiasing);
		}
		else if (s.startsWith ("<Autoscaling>") && s.endsWith ("</Autoscaling>"))
			ag->enableAutoscaling(s.remove("<Autoscaling>").remove("</Autoscaling>").toInt());
		else if (s.startsWith ("<ScaleFonts>") && s.endsWith ("</ScaleFonts>"))
			ag->setAutoscaleFonts(s.remove("<ScaleFonts>").remove("</ScaleFonts>").toInt());
		else if (s.startsWith ("<GridOnTop>") && s.endsWith ("</GridOnTop>"))
			ag->setGridOnTop(s.remove("<GridOnTop>").remove("</GridOnTop>").toInt(), false);
		else if (s.startsWith ("<MissingDataGap>") && s.endsWith ("</MissingDataGap>"))
			ag->showMissingDataGap(s.remove("<MissingDataGap>").remove("</MissingDataGap>").toInt(), false);
		else if (s.contains ("PieCurve")){
			QStringList curve=s.split("\t");
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[1]).left((curve[1]).lastIndexOf("_"));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.indexOf(caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}
			QPen pen = QPen(QColor(curve[3]), curve[2].toDouble(),Graph::getPenStyle(curve[4]));

			Table *table = app->table(curve[1]);
			if (table){
				int startRow = 0;
				int endRow = table->numRows() - 1;
				int first_color = curve[7].toInt();
				bool visible = true;
				if (d_file_version >= 90){
					startRow = curve[8].toInt();
					endRow = curve[9].toInt();
					visible = curve[10].toInt();
				}

				if (d_file_version <= 89)
					first_color = convertOldToNewColorIndex(first_color);

				if (curve.size() >= 22){//version 0.9.3-rc3
					ag->plotPie(table, curve[1], pen, curve[5].toInt(),
						curve[6].toInt(), first_color, startRow, endRow, visible,
						curve[11].toDouble(), curve[12].toDouble(), curve[13].toDouble(),
						curve[14].toDouble(), curve[15].toDouble(), curve[16].toInt(),
						curve[17].toInt(), curve[18].toInt(), curve[19].toInt(),
						curve[20].toInt(), curve[21].toInt());
				} else
					ag->plotPie(table, curve[1], pen, curve[5].toInt(),
						curve[6].toInt(), first_color, startRow, endRow, visible);
			}
		} else if (s.left(6) == "curve\t"){
            QStringList curve = s.split("\t", Qt::SkipEmptyParts);
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[2]).left((curve[2]).lastIndexOf("_"));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.indexOf(caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}

			if (curve.size() < 5)
				continue;

			CurveLayout cl;
            cl.connectType = curve[4].toInt() - 1;
			cl.lCol = readColorFromProject(curve[5]);
			cl.lStyle = curve[6].toInt();
			cl.lWidth = curve[7].toDouble();
			cl.sSize = curve[8].toInt();
			if (d_file_version <= 78)
				cl.sType=Graph::obsoleteSymbolStyle(curve[9].toInt());
			else
				cl.sType=curve[9].toInt();

			cl.symCol = readColorFromProject(curve[10]);
			cl.fillCol = readColorFromProject(curve[11]);
			cl.filledArea = curve[12].toDouble();
			cl.aCol = readColorFromProject(curve[13]);
			cl.aStyle = curve[14].toInt();
			if(curve.count() < 16)
				cl.penWidth = cl.lWidth;
			else if ((d_file_version >= 79) && (curve[3].toInt() == Graph::Box))
				cl.penWidth = curve[15].toDouble();
			else if ((d_file_version >= 78) && (curve[3].toInt() <= Graph::LineSymbols))
				cl.penWidth = curve[15].toDouble();
			else
				cl.penWidth = cl.lWidth;

			int plotType = curve[3].toInt();
			int size = curve.count();
			Table *w = app->table(curve[2]);
			Table *xt = app->table(curve[1]);
			PlotCurve *c = nullptr;
			if (xt && w && xt != w){
				c = (PlotCurve *)ag->insertCurve(xt, curve[1], w, curve[2], plotType, curve[size - 3].toInt(), curve[size - 2].toInt());
				ag->updateCurveLayout(c, &cl);
				if (c && c->rtti() == QwtPlotItem::Rtti_PlotCurve){
                    c->setAxes(curve[size - 5].toInt(), curve[size - 4].toInt());
					c->setVisible(curve.last().toInt());
				}
			} else if (w){
				if (plotType == Graph::VectXYXY || plotType == Graph::VectXYAM){
					QStringList colsList;
					colsList<<curve[2]; colsList<<curve[20]; colsList<<curve[21];
					if (d_file_version < 72)
						colsList.prepend(w->colName(curve[1].toInt()));
					else
                        colsList.prepend(curve[1]);

					int startRow = 0;
					int endRow = -1;
					if (d_file_version >= 90){
						startRow = curve[size - 3].toInt();
						endRow = curve[size - 2].toInt();
					}

					c = (PlotCurve *)ag->plotVectors(w, colsList, plotType, startRow, endRow);

					if (d_file_version <= 77){
						int temp_index = convertOldToNewColorIndex(curve[15].toInt());
						ag->updateVectorsLayout(curveID, ColorBox::defaultColor(temp_index), curve[16].toDouble(), curve[17].toInt(),
								curve[18].toInt(), curve[19].toInt(), 0, curve[20], curve[21]);
					} else {
						if(plotType == Graph::VectXYXY)
							ag->updateVectorsLayout(curveID, curve[15], curve[16].toDouble(),
								curve[17].toInt(), curve[18].toInt(), curve[19].toInt(), 0);
						else
							ag->updateVectorsLayout(curveID, curve[15], curve[16].toDouble(), curve[17].toInt(),
									curve[18].toInt(), curve[19].toInt(), curve[22].toInt());
					}
				} else if (plotType == Graph::Box)
					c = (PlotCurve *)ag->openBoxDiagram(w, curve, d_file_version);
				else {
					if (d_file_version < 72)
						c = (PlotCurve *)ag->insertCurve(w, curve[1].toInt(), curve[2], plotType);
					else if (d_file_version < 90)
						c = (PlotCurve *)ag->insertCurve(w, curve[1], curve[2], plotType);
					else
						c = (PlotCurve *)ag->insertCurve(w, curve[1], curve[2], plotType, curve[size - 3].toInt(), curve[size - 2].toInt());
				}

				if(plotType == Graph::Histogram){
				    QwtHistogram *h = (QwtHistogram *)ag->curve(curveID);
					if (d_file_version <= 76)
                        h->setBinning(curve[16].toInt(),curve[17].toDouble(),curve[18].toDouble(),curve[19].toDouble());
					else
						h->setBinning(curve[17].toInt(),curve[18].toDouble(),curve[19].toDouble(),curve[20].toDouble());
                    h->loadData();
				}

				if(plotType == Graph::VerticalBars || plotType == Graph::HorizontalBars ||
						plotType == Graph::Histogram){
					if (d_file_version <= 76)
						ag->setBarsGap(curveID, curve[15].toInt(), 0);
					else
						ag->setBarsGap(curveID, curve[15].toInt(), curve[16].toInt());
				}
				ag->updateCurveLayout(c, &cl);
				if (d_file_version >= 88){
					if (c && c->rtti() == QwtPlotItem::Rtti_PlotCurve){
						if (d_file_version < 90)
                            c->setAxes(curve[size - 2].toInt(), curve[size - 1].toInt());
						else {
                            c->setAxes(curve[size - 5].toInt(), curve[size - 4].toInt());
							c->setVisible(curve.last().toInt());
						}
					}
				}
			} else if(plotType == Graph::Histogram){//histograms from matrices
                Matrix *m = app->matrix(curve[2]);
                QwtHistogram *h = ag->restoreHistogram(m, curve);
                ag->updateCurveLayout(h, &cl);
			}
			curveID++;
		} else if (s == "<CurveLabels>"){
			QStringList lst;
			while ( s!="</CurveLabels>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			if (newYN) ag->restoreCurveLabels(curveID - 1, lst);
		} else if (s.contains("<SkipPoints>")){
			PlotCurve *c = (PlotCurve *)ag->curve(curveID - 1);
			if (c)
				c->setSkipSymbolsCount(s.remove("<SkipPoints>").remove("</SkipPoints>").toInt());
		} else if (s.contains("<StackWhiteOut>")){
			QwtBarCurve *b = (QwtBarCurve *)ag->curve(curveID - 1);
			if (b)
				b->setStacked();
		} else if (s == "<Function>"){//version 0.9.5
			curveID++;
			QStringList lst;
			while ( s != "</Function>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			FunctionCurve::restore(ag, lst);
		} else if (s.contains ("FunctionCurve")){
			QStringList curve = s.split("\t");
			CurveLayout cl;
			cl.connectType = curve[6].toInt();
			cl.lCol = readColorFromProject(curve[7]);
			cl.lStyle = curve[8].toInt();
			cl.lWidth=curve[9].toDouble();
			cl.sSize=curve[10].toInt();
			cl.sType=curve[11].toInt();
			cl.symCol = readColorFromProject(curve[12]);
			cl.fillCol = readColorFromProject(curve[13]);
			cl.filledArea = curve[14].toDouble();
			cl.aCol = readColorFromProject(curve[15]);
			cl.aStyle=curve[16].toInt();
			int current_index = 17;
			if(curve.count() < 16)
				cl.penWidth = cl.lWidth;
			else if ((d_file_version >= 79) && (curve[5].toInt() == Graph::Box))
				{
					cl.penWidth = curve[17].toDouble();
					current_index++;
				}
			else if ((d_file_version >= 78) && (curve[5].toInt() <= Graph::LineSymbols))
				{
					cl.penWidth = curve[17].toDouble();
					current_index++;
				}
			else
				cl.penWidth = cl.lWidth;

			PlotCurve *c = (PlotCurve *)ag->insertFunctionCurve(curve[1], curve[2].toInt(), d_file_version);
			c->setPlotStyle(curve[5].toInt());
			ag->updateCurveLayout(c, &cl);
			if (d_file_version >= 88){
				QwtPlotCurve *c = ag->curve(curveID);
				if (c){
                    if(current_index + 1 < curve.size())
                        c->setAxes(curve[current_index].toInt(), curve[current_index + 1].toInt());
					if (d_file_version >= 90 && current_index+2 < curve.size())
						c->setVisible(curve.last().toInt());
                    else
                        c->setVisible(true);
				}

			}
			curveID++;
		} else if (s.contains ("ErrorBars")){
            QStringList curve = s.split("\t", Qt::SkipEmptyParts);
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[4]).left((curve[4]).lastIndexOf("_"));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.indexOf(caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}
			Table *w = app->table(curve[3]);
			Table *errTable = app->table(curve[4]);
			if (w && errTable){
				bool useMasterIndex = (curve.size() >= 12);
				DataCurve *mc = useMasterIndex ? ag->dataCurve(curve[11].toInt()) : ag->masterCurve(curve[2], curve[3]);
				ErrorBarsCurve *err = ag->addErrorBars(mc, errTable, curve[4], curve[1].toInt(),
					curve[5].toDouble(), curve[6].toInt(), QColor(curve[7]),
					curve[8].toInt(), curve[10].toInt(), curve[9].toInt());

				if (!mc && useMasterIndex){
					mcIndexes << curve[11].toInt();
					errBars << err;
				}
			}
			curveID++;
		}
		else if (s == "<spectrogram>"){
			curveID++;
			QStringList lst;
			while ( s!="</spectrogram>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			if (newYN) ag->restoreSpectrogram(app, lst);
		}
		else if (s.left(6) == "scale\t"){
			QStringList scl = s.split("\t");
			scl.pop_front();
			int size = scl.count();
			if (d_file_version < 88){
				double step = scl[2].toDouble();
				if (scl[5] == "0")
					step = 0.0;
				ag->setScale(QwtPlot::xBottom, scl[0].toDouble(), scl[1].toDouble(), step,
						scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));
				ag->setScale(QwtPlot::xTop, scl[0].toDouble(), scl[1].toDouble(), step,
						scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));

				step = scl[10].toDouble();
				if (scl[13] == "0")
					step = 0.0;
				ag->setScale(QwtPlot::yLeft, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
						scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
				ag->setScale(QwtPlot::yRight, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
						scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
			} else if (size == 8){

				if (d_file_version == 89 && scl[6].toInt()==1 && scl[3].toDouble()>0) scl[3]="0"; //*    LOG-BLACK-LINE-SOLUSION
				if (newYN) ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(), scl[4].toInt(), scl[5].toInt(), scl[6].toInt(), bool(scl[7].toInt()));
                else
                {
                    const auto &scDiv = ag->axisScaleDiv(scl[0].toInt());
                    ag->setScale(scl[0].toInt(), qMin(scDiv.lowerBound(), scDiv.upperBound()),
                                 qMax(scDiv.lowerBound(), scDiv.upperBound()), scl[3].toDouble(), scl[4].toInt(),
                                 scl[5].toInt(), scl[6].toInt(), bool(scl[7].toInt()));
                }

			} else if (size == 18){
                ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(), scl[4].toInt(),
                             scl[5].toInt(), scl[6].toInt(), bool(scl[7].toInt()), scl[8].toDouble(), scl[9].toDouble(),
                             scl[10].toInt(), scl[11].toDouble(), scl[12].toDouble(), scl[13].toInt(), scl[14].toInt(),
                             scl[15].toInt(), scl[16].toInt(), bool(scl[17].toInt()));
			}
		}
		else if (s.contains ("PlotTitle")){
			QStringList fList=s.split("\t");
			 /*if (newYN)*/ ag->setTitle(fList[1]);
			ag->setTitleColor(QColor(fList[2]));
			ag->setTitleAlignment(fList[3].toInt());
		}
		else if (s.contains ("TitleFont")){
			QStringList fList=s.split("\t");
			QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());
			ag->setTitleFont(fnt);
		}
		else if (s.contains ("AxesTitles")){
			QStringList lst=s.split("\t");
			lst.pop_front();
			for (int i=0; i<4; i++){
			    if (lst.count() > i)
                    /*if (newYN)*/ ag->setScaleTitle(i, lst[i]);
			}
		}
		else if (s.contains ("AxesTitleColors")){
            QStringList colors = s.split("\t", Qt::SkipEmptyParts);
			colors.pop_front();
			for (int i=0; i<int(colors.count()); i++)
				ag->setAxisTitleColor(i, colors[i]);
		}else if (s.contains ("AxesTitleAlignment")){
            QStringList align = s.split("\t", Qt::SkipEmptyParts);
			align.pop_front();
			for (int i=0; i<(int)align.count(); i++)
				ag->setAxisTitleAlignment(i, align[i].toInt());
		} else if (s.contains ("AxesTitleDistance")){
            QStringList align = s.split("\t", Qt::SkipEmptyParts);
			align.pop_front();
			for (int i = 0; i < align.count(); i++)
				ag->setAxisTitleDistance(i, align[i].toInt());
		} else if (s.contains ("<InvertedRightTitle>")){
			QwtScaleWidget *scale = ag->axisWidget(QwtPlot::yRight);
			if (scale)
				scale->setLayoutFlag(QwtScaleWidget::TitleInverted, true);
		} else if (s.contains ("InvertedTitle")){
            QStringList invertTitles = s.split("\t", Qt::SkipEmptyParts);
			invertTitles.pop_front();
			for (int i = 0; i < invertTitles.count(); i++){
				QwtScaleWidget *scale = ag->axisWidget(i);
				if (scale)
					scale->setLayoutFlag(QwtScaleWidget::TitleInverted, invertTitles[i].toInt());
			}
		} else if (s.contains ("ScaleFont")){
			QStringList fList = s.split("\t");
			QFont fnt = QFont (fList[1], fList[2].toInt(), fList[3].toInt(), fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());

			int axis=(fList[0].right(1)).toInt();
			ag->setAxisTitleFont(axis,fnt);
		}else if (s.contains ("AxisFont")){
			QStringList fList=s.split("\t");
			QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());

			int axis=(fList[0].right(1)).toInt();
			ag->setAxisFont(axis,fnt);
		}
		else if (s.contains ("AxesFormulas"))
		{
			QStringList fList=s.split("\t");
			fList.removeAll(fList.first());
			for (int i=0; i<(int)fList.count(); i++)
				ag->setAxisFormula(i, fList[i]);
		}
		else if (s.startsWith("<AxisFormula "))
		{
			int axis = s.mid(18,s.length()-20).toInt();
			QString formula;
			for (j++; j<(int)list.count() && list[j] != "</AxisFormula>"; j++)
				formula += list[j] + "\n";
			formula.truncate(formula.length()-1);
			ag->setAxisFormula(axis, formula);
		}
		else if (s.contains ("LabelsFormat"))
		{
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setLabelsNumericFormat(fList);
		}
		else if (s.contains ("LabelsRotation"))
		{
			QStringList fList = s.split("\t");
			ag->setAxisLabelRotation(QwtPlot::xBottom, fList[1].toInt());
			ag->setAxisLabelRotation(QwtPlot::xTop, fList[2].toInt());
			if (fList.size() == 5){
				ag->setAxisLabelRotation(QwtPlot::yLeft, fList[3].toInt());
				ag->setAxisLabelRotation(QwtPlot::yRight, fList[4].toInt());
			}
		}
		else if (s.contains ("DrawAxesBackbone"))
		{
			ag->loadAxesOptions(s.split("\t"));
		}
		else if (s.contains ("AxesLineWidth"))
		{
			QStringList fList=s.split("\t");
            ag->loadAxesLinewidth(fList[1].toDouble());
		}
		else if (s.contains ("TickLabelsSpace"))
		{
			QStringList lst = s.split("\t");
			lst.removeFirst();
			for (int i = 0; i < QwtPlot::axisCnt; i++){
				QwtScaleDraw *sd = ag->axisScaleDraw (i);
				if (sd && lst.count() > i)
					sd->setSpacing(lst[i].toInt());
			}
		}
		else if (s.contains ("ShowTicksPolicy"))
		{
			QStringList lst = s.split("\t");
			lst.removeFirst();
			for (int i = 0; i < QwtPlot::axisCnt; i++){
				ScaleDraw *sd = (ScaleDraw *)ag->axisScaleDraw (i);
				if (sd && lst.count() > i)
					sd->setShowTicksPolicy((ScaleDraw::ShowTicksPolicy)lst[i].toInt());
			}
		}
		else if (s.contains ("LabelsPrefix")){
			QStringList lst = s.split("\t");
			lst.removeFirst();
			for (int i = 0; i < QwtPlot::axisCnt && i < lst.size(); i++){
				QString prefix = lst[i];
				if (prefix.isEmpty())
					continue;
				ScaleDraw *sd = (ScaleDraw *)ag->axisScaleDraw(i);
				if (sd){
					sd->setPrefix(prefix);
					ag->axisWidget(i)->setScaleDraw(new ScaleDraw(ag, sd));
				}
			}
		} else if (s.contains ("LabelsSuffix")){
			QStringList lst = s.split("\t");
			lst.removeFirst();
			for (int i = 0; i < QwtPlot::axisCnt && i < lst.size(); i++){
				QString s = lst[i];
				if (s.isEmpty())
					continue;
				ScaleDraw *sd = (ScaleDraw *)ag->axisScaleDraw(i);
				if (sd){
					sd->setSuffix(s);
					ag->axisWidget(i)->setScaleDraw(new ScaleDraw(ag, sd));
				}
			}
		}
		else if (s.contains ("CanvasFrame")){
			QStringList lst = s.split("\t");
            //if (!newYN) ag->setCanvasFrame(0);
            ag->setCanvasFrame(lst[1].toDouble(), QColor(lst[2]));
		}
		else if (s.contains ("CanvasBackground"))
		{
			QStringList list = s.split("\t");
			QColor c = QColor(list[1]);
			if (list.count() == 3)
				c.setAlpha(list[2].toInt());
			ag->setCanvasBackground(c);
		}
		else if (s.startsWith ("Legend"))
		{// version <= 0.8.9
            QStringList fList = s.split("\t");
			ag->insertLegend(fList, d_file_version);
		}
		else if (s.startsWith ("<legend>") && s.endsWith ("</legend>"))
		{
            QStringList fList = s.remove("</legend>").split("\t");
			ag->insertLegend(fList, d_file_version);
		}
		else if (s.contains ("textMarker"))
		{// version <= 0.8.9
            QStringList fList = s.split("\t");
			ag->insertText(fList, d_file_version);
		}
		else if (s.startsWith ("<text>") && s.endsWith ("</text>"))
		{
            QStringList fList = s.remove("</text>").split("\t");
			ag->insertText(fList, d_file_version);

		}
		else if (s.startsWith ("<PieLabel>") && s.endsWith ("</PieLabel>"))
		{
            QStringList fList = s.remove("</PieLabel>").split("\t");
			ag->insertText(fList, d_file_version);
		} else if (s == "<PieText>"){//version 0.9.7
			QStringList lst;
			while ( s != "</PieText>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			PieLabel::restore(ag, lst);
		}
		else if (s.contains ("lineMarker"))
		{// version <= 0.8.9
			QStringList fList=s.split("\t");
			ag->addArrow(fList, d_file_version);
		}
		else if (s.startsWith ("<line>") && s.endsWith ("</line>"))
		{
			QStringList fList=s.remove("</line>").split("\t");
			ag->addArrow(fList, d_file_version);
		}
		else if (s.contains ("ImageMarker") || (s.startsWith ("<image>") && s.endsWith ("</image>")))
		{
			QStringList fList=s.remove("</image>").split("\t");
			ag->insertImageMarker(fList, d_file_version);
		} else if (s == "<TexFormula>"){//version 0.9.7
			QStringList lst;
			while ( s != "</TexFormula>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			TexWidget::restore(ag, lst);
		} else if (s == "<Image>"){//version 0.9.7
			QStringList lst;
			while ( s != "</Image>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			ImageWidget::restore(ag, lst);
		} else if (s == "<Legend>"){//version 0.9.7
			QStringList lst;
			while ( s != "</Legend>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			if (newYN) LegendWidget::restore(ag, lst);///
		} else if (s == "<Rectangle>"){//version 0.9.7
			QStringList lst;
			while ( s != "</Rectangle>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			RectangleWidget::restore(ag, lst);
		} else if (s == "<Ellipse>"){//version 0.9.7.2
			QStringList lst;
			while ( s != "</Ellipse>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			EllipseWidget::restore(ag, lst);
		} else if (s == "<BackgroundImage>"){//version 0.9.8.4
			QStringList lst;
			while ( s != "</BackgroundImage>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			ag->restoreBackgroundImage(lst);
		} else if (s == "<SymbolImage>"){//version 0.9.8.4
			QStringList lst;
			while ( s != "</SymbolImage>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			ag->restoreSymbolImage(curveID - 1, lst);
		} else if (s.contains("AxisType")) {
			QStringList fList = s.split("\t");
			for (int i = 0; i < 4; i++){
                QStringList lst = fList[i + 1].split(";", Qt::SkipEmptyParts);
				int format = lst[0].toInt();
				if (format == ScaleDraw::Numeric)
					continue;
				if (format == ScaleDraw::Day)
					ag->setLabelsDayFormat(i, lst[1].toInt());
				else if (format == ScaleDraw::Month)
					ag->setLabelsMonthFormat(i, lst[1].toInt());
				else if (format == ScaleDraw::Time || format == ScaleDraw::Date){
					bool obsoleteDateTime = lst.size() > 2;
					ag->setLabelsDateTimeFormat(i, format, obsoleteDateTime ? lst[2] : lst[1]);
					if (obsoleteDateTime)
						ag->recoverObsoleteDateTimeScale(i, format, lst[1], lst[2]);
				} else if (lst.size() > 1)
					ag->setLabelsTextFormat(i, format, lst[1], app->table(lst[1]));
			}
		}
		else if (d_file_version < 69 && s.contains ("AxesTickLabelsCol"))
		{
			QStringList fList = s.split("\t");
			for (int i = 0; i < 4; i++){
				QString colName = fList[i+1];
				Table *nw = app->table(colName);
				ag->setLabelsTextFormat(i, ag->axisType(i), colName, nw);
			}
		} else if (s.startsWith ("<SpeedMode>") && s.endsWith ("</SpeedMode>")){
			QStringList lst = s.remove("<SpeedMode>").remove("</SpeedMode>").split("\t");
			if (lst.size() == 2)
				ag->enableDouglasPeukerSpeedMode(lst[0].toDouble(), lst[1].toInt());
		} else if (s.startsWith ("<ImageProfileTool>") && s.endsWith ("</ImageProfileTool>")){
			QStringList lst = s.remove("<ImageProfileTool>").remove("</ImageProfileTool>").split("\t");
			if (!lst.isEmpty()){
				Table *hort = 0, *vert = 0;
				if (lst.size() >= 2)
					hort = app->table(lst[1]);
				if (lst.size() >= 3)
					vert = app->table(lst[2]);
				ag->setActiveTool(new ImageProfilesTool(app, ag, app->matrix(lst[0]), hort, vert));
                if (ag->multiLayer())
                {
                    ag->multiLayer()->setActiveLayer(ag);
                    ag->multiLayer()->keepAligned(true);
                }
			}
		} else if (s.startsWith ("<ImageProfileValues>") && s.endsWith ("</ImageProfileValues>")){
			QStringList lst = s.remove("<ImageProfileValues>").remove("</ImageProfileValues>").split("\t");
			ImageProfilesTool *ipt = ag->imageProfilesTool();
			if (ipt){
				if (lst.size() == 3)
					ipt->setAveragePixels(lst[2].toInt());
				if (lst.size() >= 2)
                    ipt->append(QPointF(lst[0].toDouble(), lst[1].toDouble()));
                if (ag->multiLayer())
                    ag->multiLayer()->keepAligned(false);
			}
		} else if (s.contains("<waterfall>")){
			QStringList lst = s.trimmed().remove("<waterfall>").remove("</waterfall>").split(",");
			if (lst.size() >= 2)
				ag->setWaterfallOffset(lst[0].toInt(), lst[1].toInt());
			if (lst.size() >= 3)
				ag->setWaterfallSideLines(lst[2].toInt());
			ag->updateDataCurves();
        }
        else if (s.startsWith("<CanvasGeometry>") && s.endsWith("</CanvasGeometry>"))
        {
            QStringList lst = s.remove("<CanvasGeometry>").remove("</CanvasGeometry>").split("\t");
            if (lst.size() == 4)
                savedCanvasGeometry = QRect(lst[0].toInt(), lst[1].toInt(), lst[2].toInt(), lst[3].toInt());
        }
	}

	if (ag){
		ag->loadErrorBars(errBars, mcIndexes);
		ag->disableCurveAntialiasing(app->d_disable_curve_antialiasing, app->d_curve_max_antialising_size);
		ag->updateAxesTitles();
		ag->updateLayout();
		ag->setSynchronizedScaleDivisions(d_synchronize_graph_scales);
        ag->setGeometry(fListGeometry[1].toInt(), fListGeometry[2].toInt(), fListGeometry[3].toInt(), fListGeometry[4].toInt());
        ag->updateLayout();
        ag->blockSignals(true);
        if (lstGeometry.count()>0) ag->setPageGeometry(QRectF(lstGeometry[0].toDouble(), lstGeometry[1].toDouble(), lstGeometry[2].toDouble(), lstGeometry[3].toDouble()));
        if (savedCanvasGeometry.isValid())
        {
            ag->setCanvasGeometry(savedCanvasGeometry);
            ag->updateLayout();
        }
        ag->blockSignals(false);
	}
    return;
}

void ApplicationWindow::copyActiveLayer()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

	lastCopiedLayer = g;
	connect (g, SIGNAL(destroyed()), this, SLOT(closedLastCopiedLayer()));
	g->copyImage();
}

void ApplicationWindow::showDataSetDialog(Analysis operation)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

	bool ok;
	QStringList curves = g->analysableCurvesList();
	QString txt = QInputDialog::getItem(this, tr("QTISAS - Choose data set"),
					tr("Curve") + ": ", curves, 0, false, &ok);
	if (ok && !txt.isEmpty())
		analyzeCurve(g, g->curve(txt.left(txt.indexOf(" ["))), operation);
}

void ApplicationWindow::analyzeCurve(Graph *g,  QwtPlotCurve *c, Analysis operation)
{
	if (!g || !c)
		return;

	Fit *fitter = 0;
	switch(operation){
	    case NoAnalysis:
	    break;
		case Integrate:
		{
			Integration *i = new Integration(this, c);
			i->run();
			delete i;
		}
		break;
		case Diff:
		{
			Differentiation *diff = new Differentiation(this, c);
			diff->enableGraphicsDisplay(true);
			diff->run();
			delete diff;
		}
		break;
		case FitLinear:
			fitter = new LinearFit (this, g);
		break;
		case FitLorentz:
			fitter = new LorentzFit(this, g);
		break;
		case FitGauss:
			fitter = new GaussFit(this, g);
		break;
		case FitSigmoidal:
		{
			ScaleEngine *se = (ScaleEngine *)g->axisScaleEngine(c->xAxis());
			if(se->type() == ScaleTransformation::Log10)
				fitter = new LogisticFit (this, g);
			else
				fitter = new SigmoidalFit (this, g);
		}
		break;
		case FitSlope:
			fitter = new LinearSlopeFit (this, g);
		break;
	}

	if (!fitter)
		return;

	if (fitter->setDataFromCurve(c)){
		if (operation != FitLinear && operation != FitSlope){
			fitter->guessInitialValues();
			fitter->scaleErrors(fit_scale_errors);
			fitter->generateFunction(generateUniformFitPoints, fitPoints);
		} else if (d_2_linear_fit_points)
			fitter->generateFunction(generateUniformFitPoints, 2);
		fitter->setOutputPrecision(fit_output_precision);
		fitter->fit();
		if (pasteFitResultsToPlot)
			fitter->showLegend();
		delete fitter;
	}
}

void ApplicationWindow::analysis(Analysis operation)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

	if (g->rangeSelectorsEnabled()){
		analyzeCurve(g, g->rangeSelectorTool()->selectedCurve(), operation);
		return;
	}

	QStringList lst = g->analysableCurvesList();
	if (lst.count() == 1)
		analyzeCurve(g, g->curve(0), operation);
	else
		showDataSetDialog(operation);
}

void ApplicationWindow::integrate()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (QString(w->metaObject()->className()) == "MultiLayer"){
		Graph* g = ((MultiLayer *)w)->activeLayer();
		if (!g)
			return;
		IntegrationDialog *id = new IntegrationDialog(g, this);
		id->show();
	} else if (QString(w->metaObject()->className()) == "Matrix"){
		if (!((Matrix *)w)->isEmpty()){
			QDateTime dt = QDateTime::currentDateTime ();
            QString info = locale().toString(dt, QLocale::ShortFormat);
			info += "\n" + tr("Integration of %1 from zero is").arg(QString(w->objectName())) + ":\t";
			info += QString::number(((Matrix *)w)->integrate()) + "\n";
			info += "-------------------------------------------------------------\n";
			current_folder->appendLogInfo(info);
			showResults(true);
		} else
			showNoDataMessage();
	} else if (w->inherits("Table")){
		Table *t = (Table *)w;
		QStringList lst = t->selectedYColumns();
		int cols = lst.size();
        MySelection sel = t->getSelection();
		if (!cols || sel.topRow() == sel.bottomRow()){
			QMessageBox::warning(this, tr("QTISAS - Column selection error"),
			tr("Please select a 'Y' column first!"));
			return;
		}

		IntegrationDialog *id = new IntegrationDialog(t, this);
		id->show();
	}
}

void ApplicationWindow::differentiate()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (qobject_cast<MultiLayer *>(w))
		analysis(Diff);
	else if (w->inherits("Table")){
		Table *t = qobject_cast<Table *>(w);
		QStringList lst = t->selectedYColumns();
		int cols = lst.size();
		if (!cols){
			QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a 'Y' column first!"));
			return;
		}

		Differentiation *diff = new Differentiation(this, nullptr, "", "");
		diff->setUpdateOutputGraph(false);
		int aux = 0;
		foreach (QString yCol, lst){
			int xCol = t->colX(t->colIndex(yCol));
			diff->setDataFromTable(t, t->colName(xCol), yCol);
			diff->run();
			Graph *g = diff->outputGraph();
			if (!g)
				continue;

			QwtPlotCurve *c = g->curve(aux);
			if (c){
				if (aux < d_indexed_colors.size()){
					QPen pen = c->pen();
					pen.setColor(d_indexed_colors[aux]);
					c->setPen(pen);
				}
				aux++;
			}
		}

		Graph *g = diff->outputGraph();
		if (g){
			g->newLegend();
			g->updatePlot();
		}

		delete diff;
	}
}

void ApplicationWindow::fitLinear()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (qobject_cast<MultiLayer *>(w))
		analysis(FitLinear);
	else if (w->inherits("Table")){
		Table *t = (Table *)w;
		QStringList lst = t->selectedYColumns();
		int cols = lst.size();
		if (!cols){
        	QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a 'Y' column first!"));
			return;
		}

		MultiLayer* g = multilayerPlot(t, t->drawableColumnSelection(), Graph::LineSymbols);
		if (!g)
			return;

		QString legend = tr("Linear Regression of %1").arg(t->objectName());
		g->setWindowLabel(legend);

		QApplication::setOverrideCursor(Qt::WaitCursor);

		Table *result = newTable(cols, 5, "", legend);
		result->setColName(0, tr("Column"));
		result->setColName(1, tr("Slope"));
		result->setColName(2, tr("Intercept"));
		result->setColName(3, tr("Chi^2"));
		result->setColName(4, tr("R^2"));

		LinearFit *lf = new LinearFit (this, g->activeLayer());
		lf->setUpdateOutputGraph(false);
		if (d_2_linear_fit_points)
			lf->generateFunction(generateUniformFitPoints, 2);
		lf->setOutputPrecision(fit_output_precision);

		int aux = 0;
		foreach (QString yCol, lst){
			if (!lf->setDataFromCurve(yCol))
				continue;

			lf->setColor(aux);
			lf->fit();
			double *res = lf->results();
			result->setText(aux, 0, yCol);
			result->setCell(aux, 1, res[1]);
			result->setCell(aux, 2, res[0]);
			result->setCell(aux, 3, lf->chiSquare());
			result->setCell(aux, 4, lf->rSquare());
			aux++;
		}
        result->adjustColumnsWidth(false);
		result->show();

		Graph *og = lf->outputGraph();
		if (og)
			og->updatePlot();
		delete lf;

		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::fitSlope()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;

	if (qobject_cast<MultiLayer *>(w))
		analysis(FitSlope);
	else if (w->inherits("Table")){
		Table *t = (Table *)w;
		QStringList lst = t->selectedYColumns();
		int cols = lst.size();
		if (!cols){
        	QMessageBox::warning(this, tr("QTISAS - Column selection error"), tr("Please select a 'Y' column first!"));
			return;
		}

		MultiLayer* g = multilayerPlot(t, t->drawableColumnSelection(), Graph::LineSymbols);
		if (!g)
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);

		QString legend = tr("Linear Regression of %1").arg(t->objectName());
		g->setWindowLabel(legend);

		Table *result = newTable(cols, 4, "", legend);
		result->setColName(0, tr("Column"));
		result->setColName(1, tr("Slope"));
		result->setColName(2, tr("Chi^2"));
		result->setColName(3, tr("R^2"));

		LinearSlopeFit *lf = new LinearSlopeFit (this, g->activeLayer());
		lf->setUpdateOutputGraph(false);
		if (d_2_linear_fit_points)
			lf->generateFunction(generateUniformFitPoints, 2);
		lf->setOutputPrecision(fit_output_precision);

		int aux = 0;
		foreach (QString yCol, lst){
			if (!lf->setDataFromCurve(yCol))
				continue;

			lf->setColor(aux);
			lf->fit();
			double *res = lf->results();
			result->setText(aux, 0, yCol);
			result->setCell(aux, 1, res[0]);
			result->setCell(aux, 2, lf->chiSquare());
			result->setCell(aux, 3, lf->rSquare());
			aux++;
		}
        result->adjustColumnsWidth(false);
		result->show();

		Graph *og = lf->outputGraph();
		if (og)
			og->updatePlot();
		delete lf;

		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::fitSigmoidal()
{
	analysis(FitSigmoidal);
}

void ApplicationWindow::fitGauss()
{
	analysis(FitGauss);
}

void ApplicationWindow::fitLorentz()

{
	analysis(FitLorentz);
}

void ApplicationWindow::pickPointerCursor()
{
	btnPointer->setChecked(true);
}

void ApplicationWindow::disableTools()
{
	if (displayBar && displayBar->isVisible())
		displayBar->hide();

	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		MultiLayer *ml = qobject_cast<MultiLayer *>(w);
		if (ml){
			ml->deselect();
			QList<Graph *> layers = ml->layersList();
			foreach(Graph *g, layers)
				g->disableTools();
		}
	}
}

void ApplicationWindow::pickDataTool( QAction* action )
{
	if (!action)
		return;

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

	plot->deselect();

	QList<Graph *> layers = plot->layersList();
	foreach(Graph *g, layers)
		g->disableTools();

	if (action == btnCursor)
		showCursor();
	else if (action == btnSelect)
		showRangeSelectors();
	else if (action == btnPicker)
		showScreenReader();
	else if (action == btnMovePoints)
		movePoints();
	else if (action == actionDragCurve)
		movePoints(true);
	else if (action == btnRemovePoints)
		removePoints();
	else if (action == actionDrawPoints)
		drawPoints();
	else if (action == btnZoomIn)
		zoomIn();
	else if (action == btnZoomOut)
		zoomOut();
	else if (action == btnArrow)
		drawArrow();
	else if (action == btnLine)
		drawLine();
	else if (action == actionMagnify)
		magnify();
	else if (action == actionMagnifyHor)
		magnify(2);
	else if (action == actionMagnifyVert)
		magnify(1);
}

void ApplicationWindow::custom2DPlotTools(MultiLayer *plot)
{
	if (!plot)
		return;

	actionAddText->setChecked(false);
	actionAddFormula->setChecked(false);
	actionAddRectangle->setChecked(false);
	actionAddEllipse->setChecked(false);

	if (plot->activeLayer())
		graphSelectionChanged(plot->activeLayer()->selectionMoveResizer());

	QList<Graph *> layers = plot->layersList();
    foreach(Graph *g, layers){
    	PlotToolInterface *active_tool = g->activeTool();
    	if (active_tool){
			if (active_tool->rtti() == PlotToolInterface::Rtti_PlotTool){
				btnPicker->setChecked(true);
				return;
			} else if (active_tool->rtti() == PlotToolInterface::Rtti_DataPicker){
				switch(((DataPickerTool *)active_tool)->mode()){
					case DataPickerTool::Display:
						btnCursor->setChecked(true);
					break;
					case DataPickerTool::Move:
						btnMovePoints->setChecked(true);
					break;
					case DataPickerTool::Remove:
						btnRemovePoints->setChecked(true);
					break;
					case DataPickerTool::MoveCurve:
						actionDragCurve->setChecked(true);
					break;
				}
				return;
			} else if (active_tool->rtti() == PlotToolInterface::Rtti_DrawDataPoints){
				actionDrawPoints->setChecked(true);
				return;
			} else if (active_tool->rtti() == PlotToolInterface::Rtti_AddWidgetTool){
				switch(((AddWidgetTool *)active_tool)->widgetType()){
					case AddWidgetTool::Text:
						actionAddText->setChecked(true);
					break;
					case AddWidgetTool::TexEquation:
						actionAddFormula->setChecked(true);
					break;
					case AddWidgetTool::Rectangle:
						actionAddRectangle->setChecked(true);
					break;
					case AddWidgetTool::Ellipse:
						actionAddEllipse->setChecked(true);
					break;
					default:
						break;
				}
				return;
			}
		} else if (g->hasPanningMagnifierEnabled()){
			QwtPlotMagnifier *magnifier = g->magnifyTool();
			if (!magnifier->isAxisEnabled(QwtPlot::xBottom) && !magnifier->isAxisEnabled(QwtPlot::xTop))
				actionMagnifyVert->setChecked(true);
			else if (!magnifier->isAxisEnabled(QwtPlot::yLeft) && !magnifier->isAxisEnabled(QwtPlot::yRight))
				actionMagnifyHor->setChecked(true);
			else
				actionMagnify->setChecked(true);
			return;
		} else if (g->drawArrow()){
			btnArrow->setChecked(true);
			return;
    	} else if (g->drawLineActive()){
			btnLine->setChecked(true);
			return;
    	} else if (g->rangeSelectorsEnabled()){
			btnSelect->setChecked(true);
			return;
		} else if (g->zoomOn()){
    		btnZoomIn->setChecked(true);
    		return;
    	}
    }
	btnPointer->setChecked(true);
}

void ApplicationWindow::connectSurfacePlot(Graph3D *plot)
{
	connect (plot, SIGNAL(showContextMenu()), this,SLOT(showWindowContextMenu()));
	connect (plot, SIGNAL(showOptionsDialog()), this,SLOT(showPlot3dDialog()));
	connect (plot, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeWindow(MdiSubWindow*)));
	connect (plot, SIGNAL(hiddenWindow(MdiSubWindow*)), this, SLOT(hideWindow(MdiSubWindow*)));
	connect (plot, SIGNAL(statusChanged(MdiSubWindow*)), this, SLOT(updateWindowStatus(MdiSubWindow*)));
	connect (plot, SIGNAL(modified()), this, SIGNAL(modified()));

	plot->askOnCloseEvent(confirmClosePlot3D);
}

void ApplicationWindow::connectMultilayerPlot(MultiLayer *g)
{
	connect (g,SIGNAL(showEnrichementDialog()),this,SLOT(showEnrichementDialog()));
	connect (g,SIGNAL(showCurvesDialog()),this,SLOT(showCurvesDialog()));
	connect (g,SIGNAL(drawLineEnded(bool)), btnPointer, SLOT(setChecked(bool)));
	connect (g,SIGNAL(showMarkerPopupMenu()),this,SLOT(showMarkerPopupMenu()));
	connect (g,SIGNAL(closedWindow(MdiSubWindow*)),this, SLOT(closeWindow(MdiSubWindow*)));
	connect (g,SIGNAL(hiddenWindow(MdiSubWindow*)),this, SLOT(hideWindow(MdiSubWindow*)));
	connect (g,SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
	connect (g,SIGNAL(cursorInfo(const QString&)),info,SLOT(setText(const QString&)));
	connect (g,SIGNAL(modifiedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
	connect (g,SIGNAL(modifiedPlot()), this, SLOT(modifiedProject()));
	connect (g,SIGNAL(showLineDialog()),this, SLOT(showLineDialog()));
	connect (g,SIGNAL(pasteMarker()),this,SLOT(pasteSelection()));
	connect (g,SIGNAL(setPointerCursor()),this, SLOT(pickPointerCursor()));
	connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SLOT(setFormatBarFont(const QFont&)));
	connect (g,SIGNAL(currentColorChanged(const QColor&)), this, SLOT(setFormatBarColor(const QColor&)));

	g->askOnCloseEvent(confirmClosePlot2D);
}

void ApplicationWindow::connectTable(Table* w)
{
    connect (w->table(), SIGNAL(itemSelectionChanged()), this, SLOT(customColumnActions()));
	connect (w,SIGNAL(statusChanged(MdiSubWindow*)),this, SLOT(updateWindowStatus(MdiSubWindow*)));
	connect (w,SIGNAL(hiddenWindow(MdiSubWindow*)),this, SLOT(hideWindow(MdiSubWindow*)));
	connect (w,SIGNAL(closedWindow(MdiSubWindow*)),this, SLOT(closeWindow(MdiSubWindow*)));
	connect (w,SIGNAL(removedCol(const QString&)),this,SLOT(removeCurves(const QString&)));
	connect (w,SIGNAL(removedCol(const QString&)),this,SLOT(removeColumnNameFromCompleter(const QString&)));
	connect (w,SIGNAL(addedCol(const QString&)),this,SLOT(addColumnNameToCompleter(const QString&)));
	connect (w,SIGNAL(modifiedData(Table *, const QString&)),
			this, SLOT(updateCurves(Table *, const QString&)));
	connect (w,SIGNAL(resizedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
	connect (w,SIGNAL(modifiedWindow(MdiSubWindow*)),this,SLOT(modifiedProject(MdiSubWindow*)));
	connect (w,SIGNAL(optionsDialog()),this,SLOT(showColumnOptionsDialog()));
	connect (w,SIGNAL(colValuesDialog()),this,SLOT(showColumnValuesDialog()));
	connect (w,SIGNAL(showContextMenu(bool)),this,SLOT(showTableContextMenu(bool)));
	connect (w,SIGNAL(changedColHeader(const QString&,const QString&)),this,SLOT(updateColNames(const QString&,const QString&)));

	w->askOnCloseEvent(confirmCloseTable);
}

void ApplicationWindow::setAppColors(const QColor& wc, const QColor& pc, const QColor& tpc, bool force)
{
	if (force || workspaceColor != wc){
		workspaceColor = wc;
		d_workspace->setBackground(wc);
	}

	if (!force && panelsColor == pc && panelsTextColor == tpc)
		return;

	panelsColor = pc;
	panelsTextColor = tpc;

	QPalette palette;
	palette.setColor(QPalette::Base, QColor(panelsColor));
	qApp->setPalette(palette);

	palette.setColor(QPalette::Text, QColor(panelsTextColor));
	palette.setColor(QPalette::WindowText, QColor(panelsTextColor));

	lv->setPalette(palette);
	results->setPalette(palette);
	folders->setPalette(palette);
}

void ApplicationWindow::setPlot3DOptions()
{
	foreach(MdiSubWindow *w, windowsList()){
		Graph3D *g = qobject_cast<Graph3D*>(w);
		if (g){
			g->setAutoscale(d_3D_autoscale);
			g->setAntialiasing(d_3D_smooth_mesh);
		}
	}
}

void ApplicationWindow::createActions()
{
    actionCustomActionDialog = new QAction(tr("Add &Custom Script Action..."), this);
	connect(actionCustomActionDialog, SIGNAL(triggered()), this, SLOT(showCustomActionDialog()));

	actionNewProject = new QAction(QIcon(":/new.png"), tr("New &Project"), this);
	actionNewProject->setShortcut( tr("Ctrl+N") );
	connect(actionNewProject, SIGNAL(triggered()), this, SLOT(newProject()));

	actionAppendProject = new QAction(QIcon(":/append_file.png"), tr("App&end Project..."), this);
	connect(actionAppendProject, SIGNAL(triggered()), this, SLOT(appendProject()));

	actionNewFolder = new QAction(QIcon(":/newfolder.png"), tr("New F&older"), this);
	actionNewProject->setShortcut(Qt::Key_F7);
	connect(actionNewFolder, SIGNAL(triggered()), this, SLOT(addFolder()));

	actionNewGraph = new QAction(QIcon(":/new_graph.png"), tr("New &Graph"), this);
	actionNewGraph->setShortcut( tr("Ctrl+G") );
	connect(actionNewGraph, SIGNAL(triggered()), this, SLOT(newGraph()));

	actionNewNote = new QAction(QIcon(":/new_note.png"), tr("New &Note"), this);
	connect(actionNewNote, SIGNAL(triggered()), this, SLOT(newNote()));

	actionNewTable = new QAction(QIcon(":/table.png"), tr("New &Table"), this);
	actionNewTable->setShortcut( tr("Ctrl+T") );
	connect(actionNewTable, SIGNAL(triggered()), this, SLOT(newTable()));

	actionNewMatrix = new QAction(QIcon(":/new_matrix.png"), tr("New &Matrix"), this);
	actionNewMatrix->setShortcut( tr("Ctrl+M") );
	connect(actionNewMatrix, SIGNAL(triggered()), this, SLOT(newMatrix()));

	actionNewFunctionPlot = new QAction(QIcon(":/newF.png"), tr("New &Function Plot") + "...", this);
	actionNewFunctionPlot->setShortcut( tr("Ctrl+F") );
	connect(actionNewFunctionPlot, SIGNAL(triggered()), this, SLOT(functionDialog()));

	actionNewSurfacePlot = new QAction(QIcon(":/newFxy.png"), tr("New 3D &Surface Plot") + "...", this);
	actionNewSurfacePlot->setShortcut( tr("Ctrl+ALT+Z") );
	connect(actionNewSurfacePlot, SIGNAL(triggered()), this, SLOT(newSurfacePlot()));

	actionOpen = new QAction(QIcon(":/fileopen.png"), tr("&Open..."), this);
	actionOpen->setShortcut( tr("Ctrl+O") );
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));

	actionLoadImage = new QAction(tr("Open Image &File..."), this);
	actionLoadImage->setShortcut( tr("Ctrl+I") );
	connect(actionLoadImage, SIGNAL(triggered()), this, SLOT(loadImage()));

	actionImportImage = new QAction(QIcon(":/monalisa.png"), tr("Import I&mage..."), this);
	connect(actionImportImage, SIGNAL(triggered()), this, SLOT(importImage()));

	actionSaveProject = new QAction(QIcon(":/filesave.png"), tr("&Save Project"), this);

	actionSaveProject->setShortcut( tr("Ctrl+S") );
	connect(actionSaveProject, SIGNAL(triggered()), this, SLOT(saveProject()));

	actionSaveProjectAs = new QAction(QIcon(":/filesaveas.png"), tr("Save Project &As..."), this);
	actionSaveProjectAs->setShortcut( tr("Ctrl+Shift+S") );
	connect(actionSaveProjectAs, SIGNAL(triggered()), this, SLOT(saveProjectAs()));

	actionOpenTemplate = new QAction(QIcon(":/open_template.png"),tr("Open Temp&late..."), this);
	connect(actionOpenTemplate, SIGNAL(triggered()), this, SLOT(openTemplate()));

	actionSaveTemplate = new QAction(QIcon(":/save_template.png"), tr("Save As &Template..."), this);
	connect(actionSaveTemplate, SIGNAL(triggered()), this, SLOT(saveAsTemplate()));

	actionSaveWindow = new QAction(tr("Save &Window As..."), this);
	connect(actionSaveWindow, SIGNAL(triggered()), this, SLOT(saveWindowAs()));

	actionSaveNote = new QAction(QIcon(":/filesaveas.png"), tr("Save Note As..."), this);
	connect(actionSaveNote, SIGNAL(triggered()), this, SLOT(saveNoteAs()));

	actionLoad = new QAction(QIcon(":/import.png"), tr("&Import ASCII..."), this);
	connect(actionLoad, SIGNAL(triggered()), this, SLOT(importASCII()));

	actionImportSound = new QAction(tr("&Sound (WAV)..."), this);
	connect(actionImportSound, SIGNAL(triggered()), this, SLOT(importWaveFile()));

	actionImportDatabase = new QAction(tr("&Database..."), this);
	connect(actionImportDatabase, SIGNAL(triggered()), this, SLOT(importDatabase()));

	actionUndo = new QAction(QIcon(":/undo.png"), tr("&Undo"), this);
	actionUndo->setShortcut( tr("Ctrl+Z") );
    setupActionIcon(actionUndo);
	connect(actionUndo, SIGNAL(triggered()), this, SLOT(undo()));

	actionRedo = new QAction(QIcon(":/redo.png"), tr("&Redo"), this);
    actionRedo->setShortcut(QKeySequence(Qt::ControlModifier, Qt::SHIFT, Qt::Key_Z));
    setupActionIcon(actionRedo);
	connect(actionRedo, SIGNAL(triggered()), this, SLOT(redo()));

	actionCopyWindow = new QAction(QIcon(":/duplicate.png"), tr("&Duplicate"), this);
	actionCopyWindow->setShortcut(tr("Ctrl+Alt+D"));
	connect(actionCopyWindow, SIGNAL(triggered()), this, SLOT(clone()));

	actionCutSelection = new QAction(QIcon(":/cut.png"), tr("Cu&t Selection"), this);
	actionCutSelection->setShortcut( tr("Ctrl+X") );
    setupActionIcon(actionCutSelection);
    connect(actionCutSelection, &QAction::triggered, this, [this]() {
#ifdef SCRIPTING_CONSOLE
        if (console && console->hasFocus())
        {
            console->clear();
            return;
        }
#endif
        cutSelection();
    });

	actionCopySelection = new QAction(QIcon(":/copy.png"), tr("&Copy Selection"), this);
	actionCopySelection->setShortcut( tr("Ctrl+C") );
    setupActionIcon(actionCopySelection);
    connect(actionCopySelection, &QAction::triggered, this, [this]() {
#ifdef SCRIPTING_CONSOLE
        if (console && console->hasFocus())
        {
            console->copy();
            return;
        }
#endif
        copySelection();
    });

	actionPasteSelection = new QAction(QIcon(":/paste.png"), tr("&Paste Selection"), this);
	actionPasteSelection->setShortcut( tr("Ctrl+V") );
    setupActionIcon(actionPasteSelection);
	connect(actionPasteSelection, SIGNAL(triggered()), this, SLOT(pasteSelection()));

	actionClearSelection = new QAction(QIcon(":/erase.png"), tr("&Delete Selection"), this);
	actionClearSelection->setShortcut( tr("Del","delete key") );
    setupActionIcon(actionClearSelection);
	connect(actionClearSelection, SIGNAL(triggered()), this, SLOT(clearSelection()));

	actionRaiseEnrichment = new QAction(QIcon(":/raise.png"), tr("&Front"), this);
	connect(actionRaiseEnrichment, SIGNAL(triggered()), this, SLOT(raiseActiveEnrichment()));
	actionRaiseEnrichment->setEnabled(false);

	actionLowerEnrichment = new QAction(QIcon(":/lower.png"), tr("&Back"), this);
	connect(actionLowerEnrichment, SIGNAL(triggered()), this, SLOT(lowerActiveEnrichment()));
	actionLowerEnrichment->setEnabled(false);

	actionAlignTop = new QAction(QIcon(":/align_top.png"), tr("Align &Top"), this);
	connect(actionAlignTop, SIGNAL(triggered()), this, SLOT(alignTop()));
	actionAlignTop->setEnabled(false);

	actionAlignBottom = new QAction(QIcon(":/align_bottom.png"), tr("Align &Bottom"), this);
	connect(actionAlignBottom, SIGNAL(triggered()), this, SLOT(alignBottom()));
	actionAlignBottom->setEnabled(false);

	actionAlignLeft = new QAction(QIcon(":/align_left.png"), tr("Align &Left"), this);
	connect(actionAlignLeft, SIGNAL(triggered()), this, SLOT(alignLeft()));
	actionAlignLeft->setEnabled(false);

	actionAlignRight = new QAction(QIcon(":/align_right.png"), tr("Align &Right"), this);
	connect(actionAlignRight, SIGNAL(triggered()), this, SLOT(alignRight()));
	actionAlignRight->setEnabled(false);

	actionShowExplorer = explorerWindow->toggleViewAction();
    setupActionIcon(actionShowExplorer, QIcon(":/folder.png"));
	actionShowExplorer->setShortcut( tr("Ctrl+E") );
    connect(actionShowExplorer, SIGNAL(changed()), this, SLOT(showExplorerDialog()));

#ifdef QTISAS
    actionShowDan = danWindow->toggleViewAction();
    setupActionIcon(actionShowDan, QIcon(":/dan.png"));
    connect(actionShowDan, SIGNAL(triggered()), this, SLOT(showDanDialog()));
    actionShowCompile = compileWindow->toggleViewAction();
    setupActionIcon(actionShowCompile, QIcon(":/compile.png"));
    connect(actionShowCompile, SIGNAL(triggered()), this, SLOT(showCompileDialog()));

    actionShowCompileLocal = new QAction(this);
    actionShowCompileLocal->setText(tr("Fit.Compile - Fitting Function Compiler"));
    setupActionIcon(actionShowCompileLocal, QIcon(":/compile.png"));
    connect(actionShowCompileLocal, &QAction::triggered, this, [this]() {
        if (!actionShowCompile->isChecked())
            actionShowCompile->setChecked(true);
        showCompileDialog();
    });

    actionShowFittableLocal = new QAction(this);
    actionShowFittableLocal->setText(tr("Fit.Curve(s) - Fitting Interface"));
    setupActionIcon(actionShowFittableLocal, QIcon(":/fittable.png"));
    connect(actionShowFittableLocal, &QAction::triggered, this, [this]() {
        if (!actionShowFittable->isChecked())
            actionShowFittable->setChecked(true);
        showFittableDialog();
    });

    actionShowFittable = fittableWindow->toggleViewAction();
    setupActionIcon(actionShowFittable, QIcon(":/fittable.png"));
    connect(actionShowFittable, SIGNAL(triggered()), this, SLOT(showFittableDialog()));
    actionShowJnse = jnseWindow->toggleViewAction();
    setupActionIcon(actionShowJnse, QIcon(":/jnse.png"));
    connect(actionShowJnse, SIGNAL(triggered()), this, SLOT(showJnseDialog()));
    actionShowAscii1d = ascii1dWindow->toggleViewAction();
    setupActionIcon(actionShowAscii1d, QIcon(":/ascii1d.png"));
    connect(actionShowAscii1d, SIGNAL(triggered()), this, SLOT(showAscii1dDialog()));

    actionShowAscii1dLocal = new QAction(this);
    actionShowAscii1dLocal->setText(tr("Import/Export ASCII ... via ASCII.SANS.1D"));
    setupActionIcon(actionShowAscii1dLocal, QIcon(":/ascii1d.png"));
    connect(actionShowAscii1dLocal, &QAction::triggered, this, [this]() {
        if (!actionShowAscii1d->isChecked())
            actionShowAscii1d->setChecked(true);
        showAscii1dDialog();
    });

    actionShowSvd = svdWindow->toggleViewAction();
    setupActionIcon(actionShowSvd, QIcon(":/svd.png"));
    connect(actionShowSvd, SIGNAL(triggered()), this, SLOT(showSvdDialog()));
#endif

	actionFindWindow = new QAction(QIcon(":/find.png"), tr("&Find..."), this);
	connect(actionFindWindow, SIGNAL(triggered()), this, SLOT(showFindDialogue()));

	actionShowLog = logWindow->toggleViewAction();
    setupActionIcon(actionShowLog, QIcon(":/log.png"));
    connect(actionShowLog, SIGNAL(changed()), this, SLOT(showLogDialog()));

    actionShowUndoStack = undoStackWindow->toggleViewAction();

#ifdef SCRIPTING_CONSOLE
	actionShowConsole = consoleWindow->toggleViewAction();
#endif

	actionAddLayer = new QAction(QIcon(":/newLayer.png"), tr("Add La&yer"), this);
	actionAddLayer->setShortcut( tr("ALT+L") );
    setupActionIcon(actionAddLayer);
	connect(actionAddLayer, SIGNAL(triggered()), this, SLOT(addLayer()));

	actionShowLayerDialog = new QAction(QIcon(":/arrangeLayers.png"), tr("Arran&ge Layers"), this);
	actionShowLayerDialog->setShortcut( tr("Shift+A") );
    setupActionIcon(actionShowLayerDialog);
	connect(actionShowLayerDialog, SIGNAL(triggered()), this, SLOT(showLayerDialog()));

    actionAutomaticLayout = new QAction(QIcon(":/auto_layout.png"), tr("Automatic Layout"), this);
    setupActionIcon(actionAutomaticLayout);
    connect(actionAutomaticLayout, SIGNAL(triggered()), this, SLOT(autoArrangeLayers()));

    actionLogLog = new QAction(QIcon(":/log-log.png"), "Log-Log(Log) Presentation", this);
    setupActionIcon(actionLogLog);
    actionLogLog->setToolTip("- \"Log-Log\" Presentation:\tif no axis is selected\n"
                             "- \"Log\" Presentation:\tfor selected axis\n"
                             "- \"Log\" Presentation of Spectrograms Color-Fill:\tfor map and bar scale");
    connect(actionLogLog, &QAction::triggered, this, [this]() { setLinOrLog(false, true); });

    actionLinLin = new QAction(QIcon(":/lin-lin.png"), "Lin-Lin(Lin) Presentation", this);
    setupActionIcon(actionLinLin);
    actionLinLin->setToolTip("- \"Lin-Lin\" Presentation:\tif no axis is selected\n"
                             "- \"Lin\" Presentation:\tfor selected axis\n"
                             "- \"Lin\" Presentation of Spectrograms Color-Fill:\tfor map and bar scale");
    connect(actionLinLin, &QAction::triggered, this, [this]() { setLinOrLog(false, false); });

	actionExportGraph = new QAction(tr("&Current"), this);
	actionExportGraph->setShortcut( tr("Ctrl+Alt+G") );
	connect(actionExportGraph, SIGNAL(triggered()), this, SLOT(exportGraph()));

    actionExportLayer = new QAction(tr("Export current layer"), this);
    connect(actionExportLayer, SIGNAL(triggered()), this, SLOT(exportLayer()));

	actionExportAllGraphs = new QAction(tr("&All") + "...", this);
	actionExportAllGraphs->setShortcut( tr("Alt+X") );
	connect(actionExportAllGraphs, SIGNAL(triggered()), this, SLOT(exportAllGraphs()));

	actionExportPDF = new QAction(QIcon(":/pdf.png"), tr("&Export PDF") + "...", this);
	actionExportPDF->setShortcut( tr("Ctrl+Alt+P") );
	connect(actionExportPDF, SIGNAL(triggered()), this, SLOT(exportPDF()));

	actionPrint = new QAction(QIcon(":/fileprint.png"), tr("&Print..."), this);
	actionPrint->setShortcut( tr("Ctrl+P") );
	connect(actionPrint, SIGNAL(triggered()), this, SLOT(print()));

	actionPrintPreview = new QAction(QIcon(":/preview.png"), tr("Print Pre&view..."), this);
	connect(actionPrintPreview, SIGNAL(triggered()), this, SLOT(printPreview()));

	actionPrintAllPlots = new QAction(tr("Print All Plo&ts"), this);
	actionPrintAllPlots->setShortcut(tr("Ctrl+Shift+P"));
	connect(actionPrintAllPlots, SIGNAL(triggered()), this, SLOT(printAllPlots()));

	actionShowExportASCIIDialog = new QAction(tr("E&xport ASCII..."), this);
	connect(actionShowExportASCIIDialog, SIGNAL(triggered()), this, SLOT(showExportASCIIDialog()));

	actionCloseAllWindows = new QAction(QIcon(":/quit.png"), tr("&Quit"), this);
	actionCloseAllWindows->setShortcut( tr("Ctrl+Q") );
	connect(actionCloseAllWindows, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

	actionCloseProject = new QAction(QIcon(":/delete.png"), tr("&Close"), this);

	connect(actionCloseProject, SIGNAL(triggered()), this, SLOT(newProject()));

	actionClearLogInfo = new QAction(tr("Clear &Log Information"), this);
	connect(actionClearLogInfo, SIGNAL(triggered()), this, SLOT(clearLogInfo()));

	actionDeleteFitTables = new QAction(QIcon(":/close.png"), tr("Delete &Fit Tables"), this);
    setupActionIcon(actionDeleteFitTables);
	connect(actionDeleteFitTables, SIGNAL(triggered()), this, SLOT(deleteFitTables()));

	actionShowPlotWizard = new QAction(QIcon(":/wizard.png"), tr("Plot &Wizard") + "...", this);
	actionShowPlotWizard->setShortcut( tr("Ctrl+Alt+W") );
	connect(actionShowPlotWizard, SIGNAL(triggered()), this, SLOT(showPlotWizard()));

	actionShowConfigureDialog = new QAction(QIcon(":/configure.png"), tr("&Preferences..."), this);
	connect(actionShowConfigureDialog, SIGNAL(triggered()), this, SLOT(showPreferencesDialog()));
#ifdef Q_OS_MACOS
    actionShowConfigureDialog->setText("Settings...");
    actionShowConfigureDialog->setMenuRole(QAction::NoRole);
    setupActionIcon(actionShowConfigureDialog, QIcon(":/configure.png"));
#endif

	actionShowCurvesDialog = new QAction(QIcon(":/curves.png"), tr("Add/Remove &Curve..."), this);
	actionShowCurvesDialog->setShortcut( tr("ALT+C") );
    setupActionIcon(actionShowCurvesDialog);
	connect(actionShowCurvesDialog, SIGNAL(triggered()), this, SLOT(showCurvesDialog()));

	actionAddErrorBars = new QAction(QIcon(":/errors.png"), tr("Add &Error Bars..."), this);
	actionAddErrorBars->setShortcut( tr("Ctrl+B") );
    setupActionIcon(actionAddErrorBars);
	connect(actionAddErrorBars, SIGNAL(triggered()), this, SLOT(addErrorBars()));

	actionAddFunctionCurve = new QAction(QIcon(":/fx.png"), tr("Add &Function..."), this);
	actionAddFunctionCurve->setShortcut( tr("Ctrl+Alt+F") );
    setupActionIcon(actionAddFunctionCurve);
	connect(actionAddFunctionCurve, SIGNAL(triggered()), this, SLOT(addFunctionCurve()));

	actionUnzoom = new QAction(QIcon(":/unzoom.png"), tr("&Rescale to Show All"), this);
	actionUnzoom->setShortcut( tr("Ctrl+Shift+R") );
	connect(actionUnzoom, SIGNAL(triggered()), this, SLOT(setAutoScale()));

	actionMagnify = new QAction(QIcon(":/magnifier.png"), tr("Zoom &In/Out and Drag Canvas"), this);
	connect(actionMagnify, SIGNAL(triggered()), this, SLOT(magnify()));

	actionMagnifyHor = new QAction(QIcon(":/magnifier_hor.png"), tr("Zoom/Drag Canvas &Horizontally"), this);
	actionMagnifyVert = new QAction(QIcon(":/magnifier_vert.png"), tr("Zoom/Drag Canvas &Vertically"), this);

	actionNewLegend = new QAction(QIcon(":/legend.png"), tr("New &Legend"), this);
	actionNewLegend->setShortcut( tr("Ctrl+L") );
    setupActionIcon(actionNewLegend);
	connect(actionNewLegend, SIGNAL(triggered()), this, SLOT(newLegend()));

	actionTimeStamp = new QAction(QIcon(":/clock.png"), tr("Add Time Stamp"), this);
	actionTimeStamp->setShortcut( tr("Ctrl+ALT+T") );
    setupActionIcon(actionTimeStamp);
	connect(actionTimeStamp, SIGNAL(triggered()), this, SLOT(addTimeStamp()));

	actionAddImage = new QAction(QIcon(":/monalisa.png"), tr("Add &Image"), this);
	actionAddImage->setShortcut( tr("ALT+I") );
    setupActionIcon(actionAddImage);
	connect(actionAddImage, SIGNAL(triggered()), this, SLOT(addImage()));

	actionPlotL = new QAction(QIcon(":/lPlot.png"), tr("&Line"), this);
	connect(actionPlotL, SIGNAL(triggered()), this, SLOT(plotL()));

	actionPlotP = new QAction(QIcon(":/pPlot.png"), tr("&Scatter"), this);
	connect(actionPlotP, SIGNAL(triggered()), this, SLOT(plotP()));

	actionPlotLP = new QAction(QIcon(":/lpPlot.png"), tr("Line + S&ymbol"), this);
	connect(actionPlotLP, SIGNAL(triggered()), this, SLOT(plotLP()));

	actionPlotVerticalDropLines = new QAction(QIcon(":/dropLines.png"), tr("Vertical &Drop Lines"), this);
	connect(actionPlotVerticalDropLines, SIGNAL(triggered()), this, SLOT(plotVerticalDropLines()));

	actionPlotSpline = new QAction(QIcon(":/spline.png"), tr("&Spline"), this);
	connect(actionPlotSpline, SIGNAL(triggered()), this, SLOT(plotSpline()));

	actionPlotHorSteps = new QAction(QIcon(":/hor_steps.png"), tr("&Horizontal Steps"), this);
	connect(actionPlotHorSteps, SIGNAL(triggered()), this, SLOT(plotHorSteps()));

	actionPlotVertSteps = new QAction(QIcon(":/vert_steps.png"), tr("&Vertical Steps"), this);
	connect(actionPlotVertSteps, SIGNAL(triggered()), this, SLOT(plotVertSteps()));

	actionPlotVerticalBars = new QAction(QIcon(":/vertBars.png"), tr("&Columns"), this);
	connect(actionPlotVerticalBars, SIGNAL(triggered()), this, SLOT(plotVerticalBars()));

	actionPlotHorizontalBars = new QAction(QIcon(":/hBars.png"), tr("&Rows"), this);
	connect(actionPlotHorizontalBars, SIGNAL(triggered()), this, SLOT(plotHorizontalBars()));

	actionStackBars = new QAction(QIcon(":/stack_bar.png"), tr("Stack &Bar"), this);
	connect(actionStackBars, SIGNAL(triggered()), this, SLOT(plotStackBar()));

	actionStackColumns = new QAction(QIcon(":/stack_column.png"), tr("Stack &Column"), this);
	connect(actionStackColumns, SIGNAL(triggered()), this, SLOT(plotStackColumn()));

	actionPlotArea = new QAction(QIcon(":/area.png"), tr("&Area"), this);
	connect(actionPlotArea, SIGNAL(triggered()), this, SLOT(plotArea()));

	actionPlotPie = new QAction(QIcon(":/pie.png"), tr("&Pie"), this);
	connect(actionPlotPie, SIGNAL(triggered()), this, SLOT(plotPie()));

	actionPlotVectXYAM = new QAction(QIcon(":/vectXYAM.png"), tr("Vectors XY&AM"), this);
	connect(actionPlotVectXYAM, SIGNAL(triggered()), this, SLOT(plotVectXYAM()));

	actionPlotVectXYXY = new QAction(QIcon(":/vectXYXY.png"), tr("&Vectors &XYXY"), this);
	connect(actionPlotVectXYXY, SIGNAL(triggered()), this, SLOT(plotVectXYXY()));

	actionPlotHistogram = new QAction(QIcon(":/histogram.png"), tr("&Histogram"), this);
	connect(actionPlotHistogram, SIGNAL(triggered()), this, SLOT(plotHistogram()));

	actionPlotStackedHistograms = new QAction(QIcon(":/stacked_hist.png"), tr("&Stacked Histogram"), this);
	connect(actionPlotStackedHistograms, SIGNAL(triggered()), this, SLOT(plotStackedHistograms()));

	actionStemPlot = new QAction(QIcon(":/leaf.png"), tr("Stem-and-&Leaf Plot"), this);
	connect(actionStemPlot, SIGNAL(triggered()), this, SLOT(newStemPlot()));

	actionPlot2VerticalLayers = new QAction(QIcon(":/panel_v2.png"), tr("&Vertical 2 Layers"), this);
	connect(actionPlot2VerticalLayers, SIGNAL(triggered()), this, SLOT(plot2VerticalLayers()));

	actionPlot2HorizontalLayers = new QAction(QIcon(":/panel_h2.png"), tr("&Horizontal 2 Layers"), this);
	connect(actionPlot2HorizontalLayers, SIGNAL(triggered()), this, SLOT(plot2HorizontalLayers()));

	actionPlot4Layers = new QAction(QIcon(":/panel_4.png"), tr("&4 Layers"), this);
	connect(actionPlot4Layers, SIGNAL(triggered()), this, SLOT(plot4Layers()));

	actionPlotStackedLayers = new QAction(QIcon(":/stacked.png"), tr("&Stacked Layers"), this);
	connect(actionPlotStackedLayers, SIGNAL(triggered()), this, SLOT(plotStackedLayers()));

	actionVertSharedAxisLayers = new QAction(QIcon(":/panel_v2.png"), tr("&Vertical 2 Layers"), this);
	connect(actionVertSharedAxisLayers, SIGNAL(triggered()), this, SLOT(plotVerticalSharedAxisLayers()));

	actionHorSharedAxisLayers = new QAction(QIcon(":/panel_h2.png"), tr("&Horizontal 2 Layers"), this);
	connect(actionHorSharedAxisLayers, SIGNAL(triggered()), this, SLOT(plotHorizontalSharedAxisLayers()));

	actionSharedAxesLayers = new QAction(QIcon(":/panel_4.png"), tr("&4 Layers"), this);
	connect(actionSharedAxesLayers, SIGNAL(triggered()), this, SLOT(plotSharedAxesLayers()));

	actionStackSharedAxisLayers = new QAction(QIcon(":/stacked.png"), tr("&Stacked Layers"), this);
	connect(actionStackSharedAxisLayers, SIGNAL(triggered()), this, SLOT(plotStackSharedAxisLayers()));

	actionCustomSharedAxisLayers = new QAction(QIcon(":/arrangeLayers.png"), tr("&Custom Layout..."), this);
	connect(actionCustomSharedAxisLayers, SIGNAL(triggered()), this, SLOT(plotCustomLayoutSharedAxes()));

	actionCustomLayout = new QAction(QIcon(":/arrangeLayers.png"), tr("&Custom Layout..."), this);
	connect(actionCustomLayout, SIGNAL(triggered()), this, SLOT(plotCustomLayout()));

	actionPlotDoubleYAxis = new QAction(QIcon(":/plot_double_y.png"), tr("D&ouble-Y"), this);
	connect(actionPlotDoubleYAxis, SIGNAL(triggered()), this, SLOT(plotDoubleYAxis()));

	actionAddZoomPlot = new QAction(QIcon(":/add_zoom_plot.png"), tr("&Zoom"), this);
	connect(actionAddZoomPlot, SIGNAL(triggered()), this, SLOT(zoomRectanglePlot()));

	actionWaterfallPlot = new QAction(QIcon(":/waterfall_plot.png"), tr("&Waterfall Plot"), this);
	connect(actionWaterfallPlot, SIGNAL(triggered()), this, SLOT(waterfallPlot()));

	actionExtractGraphs = new QAction(QIcon(":/extract_graphs.png"), tr("E&xtract to Graphs"), this);
    setupActionIcon(actionExtractGraphs);
	connect(actionExtractGraphs, SIGNAL(triggered()), this, SLOT(extractGraphs()));

	actionExtractLayers = new QAction(QIcon(":/extract_layers.png"), tr("Extract to &Layers"), this);
    setupActionIcon(actionExtractLayers);
	connect(actionExtractLayers, SIGNAL(triggered()), this, SLOT(extractLayers()));

	actionAddInsetLayer = new QAction(QIcon(":/add_inset_layer.png"), tr("Add Inset Layer"), this);
    setupActionIcon(actionAddInsetLayer);
	connect(actionAddInsetLayer, SIGNAL(triggered()), this, SLOT(addInsetLayer()));

	actionAddInsetCurveLayer = new QAction(QIcon(":/add_inset_curve_layer.png"), tr("Add Inset Layer"), this);
    setupActionIcon(actionAddInsetCurveLayer);
	connect(actionAddInsetCurveLayer, SIGNAL(triggered()), this, SLOT(addInsetCurveLayer()));

	actionPlot3DRibbon = new QAction(QIcon(":/ribbon.png"), tr("&Ribbon"), this);
	connect(actionPlot3DRibbon, SIGNAL(triggered()), this, SLOT(plot3DRibbon()));

	actionPlot3DBars = new QAction(QIcon(":/bars.png"), tr("&Bars"), this);
	connect(actionPlot3DBars, SIGNAL(triggered()), this, SLOT(plot3DBars()));

	actionPlot3DScatter = new QAction(QIcon(":/scatter.png"), tr("&Scatter"), this);
	connect(actionPlot3DScatter, SIGNAL(triggered()), this, SLOT(plot3DScatter()));

	actionPlot3DTrajectory = new QAction(QIcon(":/trajectory.png"), tr("&Trajectory"), this);
	connect(actionPlot3DTrajectory, SIGNAL(triggered()), this, SLOT(plot3DTrajectory()));

	actionShowColStatistics = new QAction(QIcon(":/col_stat.png"), tr("Statistics on &Columns"), this);
	connect(actionShowColStatistics, SIGNAL(triggered()), this, SLOT(showColStatistics()));

	actionShowRowStatistics = new QAction(QIcon(":/stat_rows.png"), tr("Statistics on &Rows"), this);
	connect(actionShowRowStatistics, SIGNAL(triggered()), this, SLOT(showRowStatistics()));

	actionIntegrate = new QAction(tr("&Integrate") + "...", this);
	connect(actionIntegrate, SIGNAL(triggered()), this, SLOT(integrate()));

	actionShowIntDialog = new QAction(tr("Integr&ate Function..."), this);
	connect(actionShowIntDialog, SIGNAL(triggered()), this, SLOT(showFunctionIntegrationDialog()));

	actionInterpolate = new QAction(tr("Inte&rpolate ..."), this);
	connect(actionInterpolate, SIGNAL(triggered()), this, SLOT(showInterpolationDialog()));

	actionLowPassFilter = new QAction(tr("&Low Pass..."), this);
	connect(actionLowPassFilter, SIGNAL(triggered()), this, SLOT(lowPassFilterDialog()));

	actionHighPassFilter = new QAction(tr("&High Pass..."), this);
	connect(actionHighPassFilter, SIGNAL(triggered()), this, SLOT(highPassFilterDialog()));

	actionBandPassFilter = new QAction(tr("&Band Pass..."), this);
	connect(actionBandPassFilter, SIGNAL(triggered()), this, SLOT(bandPassFilterDialog()));

	actionBandBlockFilter = new QAction(tr("&Band Block..."), this);
	connect(actionBandBlockFilter, SIGNAL(triggered()), this, SLOT(bandBlockFilterDialog()));

	actionFFT = new QAction(tr("&FFT..."), this);
	connect(actionFFT, SIGNAL(triggered()), this, SLOT(showFFTDialog()));

	actionSmoothSavGol = new QAction(tr("&Savitzky-Golay..."), this);
	connect(actionSmoothSavGol, SIGNAL(triggered()), this, SLOT(showSmoothSavGolDialog()));

	actionSmoothFFT = new QAction(tr("&FFT Filter..."), this);
	connect(actionSmoothFFT, SIGNAL(triggered()), this, SLOT(showSmoothFFTDialog()));

	actionSmoothAverage = new QAction(tr("Moving Window &Average..."), this);
	connect(actionSmoothAverage, SIGNAL(triggered()), this, SLOT(showSmoothAverageDialog()));

	actionSmoothLowess = new QAction(tr("&Lowess..."), this);
	connect(actionSmoothLowess, SIGNAL(triggered()), this, SLOT(showSmoothLowessDialog()));

	actionDifferentiate = new QAction(tr("&Differentiate"), this);
	connect(actionDifferentiate, SIGNAL(triggered()), this, SLOT(differentiate()));

	actionFitSlope = new QAction(tr("Fit Slop&e"), this);
	connect(actionFitSlope, SIGNAL(triggered()), this, SLOT(fitSlope()));

	actionFitLinear = new QAction(tr("Fit &Linear"), this);
	connect(actionFitLinear, SIGNAL(triggered()), this, SLOT(fitLinear()));

	actionShowFitPolynomDialog = new QAction(tr("Fit &Polynomial ..."), this);
	connect(actionShowFitPolynomDialog, SIGNAL(triggered()), this, SLOT(showFitPolynomDialog()));

	actionShowExpDecayDialog = new QAction(tr("&First Order ..."), this);
	connect(actionShowExpDecayDialog, SIGNAL(triggered()), this, SLOT(showExpDecayDialog()));

	actionShowTwoExpDecayDialog = new QAction(tr("&Second Order ..."), this);
	connect(actionShowTwoExpDecayDialog, SIGNAL(triggered()), this, SLOT(showTwoExpDecayDialog()));

	actionShowExpDecay3Dialog = new QAction(tr("&Third Order ..."), this);
	connect(actionShowExpDecay3Dialog, SIGNAL(triggered()), this, SLOT(showExpDecay3Dialog()));

	actionFitExpGrowth = new QAction(tr("Fit Exponential Gro&wth ..."), this);
	connect(actionFitExpGrowth, SIGNAL(triggered()), this, SLOT(showExpGrowthDialog()));

	actionFitSigmoidal = new QAction(tr("Fit &Boltzmann (Sigmoidal)"), this);
	connect(actionFitSigmoidal, SIGNAL(triggered()), this, SLOT(fitSigmoidal()));

	actionFitGauss = new QAction(tr("Fit &Gaussian"), this);
	connect(actionFitGauss, SIGNAL(triggered()), this, SLOT(fitGauss()));

	actionFitLorentz = new QAction(tr("Fit Lorent&zian"), this);
	connect(actionFitLorentz, SIGNAL(triggered()), this, SLOT(fitLorentz()));

    actionShowFitDialog = new QAction(tr("QtiPlot Fit &Wizard..."), this);
	actionShowFitDialog->setShortcut( tr("Ctrl+Y") );
	connect(actionShowFitDialog, SIGNAL(triggered()), this, SLOT(showFitDialog()));

	actionShowPlotDialog = new QAction(tr("&Plot ..."), this);
	connect(actionShowPlotDialog, SIGNAL(triggered()), this, SLOT(showGeneralPlotDialog()));

	actionShowScaleDialog = new QAction(tr("&Scales..."), this);
	connect(actionShowScaleDialog, SIGNAL(triggered()), this, SLOT(showScaleDialog()));

	actionShowAxisDialog = new QAction(tr("&Axes..."), this);
	connect(actionShowAxisDialog, SIGNAL(triggered()), this, SLOT(showAxisDialog()));

	actionShowGridDialog = new QAction(tr("&Grid ..."), this);
	connect(actionShowGridDialog, SIGNAL(triggered()), this, SLOT(showGridDialog()));

	actionShowTitleDialog = new QAction(tr("&Title ..."), this);
	connect(actionShowTitleDialog, SIGNAL(triggered()), this, SLOT(showTitleDialog()));

	actionShowColumnOptionsDialog = new QAction(tr("Column &Options ..."), this);
	actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
	connect(actionShowColumnOptionsDialog, SIGNAL(triggered()), this, SLOT(showColumnOptionsDialog()));

	actionShowColumnValuesDialog = new QAction(QIcon(":/formula.png"), tr("Set Column &Values ..."), this);
	connect(actionShowColumnValuesDialog, SIGNAL(triggered()), this, SLOT(showColumnValuesDialog()));
	actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));

	actionExtractTableData = new QAction(tr("&Extract Data..."), this);
	connect(actionExtractTableData, SIGNAL(triggered()), this, SLOT(showExtractDataDialog()));

    actionTransposeTable = new QAction(tr("Transpose Table ..."), this);
    connect(actionTransposeTable, SIGNAL(triggered()), this, SLOT(transposeTable()));

	actionTableRecalculate = new QAction(tr("Recalculate"), this);
	actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
	connect(actionTableRecalculate, SIGNAL(triggered()), this, SLOT(recalculateTable()));

	actionHideSelectedColumns = new QAction(tr("&Hide Selected"), this);
	connect(actionHideSelectedColumns, SIGNAL(triggered()), this, SLOT(hideSelectedColumns()));

	actionShowAllColumns = new QAction(tr("Sho&w All Columns"), this);
	connect(actionShowAllColumns, SIGNAL(triggered()), this, SLOT(showAllColumns()));

	actionSwapColumns = new QAction(QIcon(":/swap_columns.png"), tr("&Swap columns"), this);
	connect(actionSwapColumns, SIGNAL(triggered()), this, SLOT(swapColumns()));

	actionMoveColRight = new QAction(QIcon(":/move_col_right.png"), tr("Move &Right"), this);
	connect(actionMoveColRight, SIGNAL(triggered()), this, SLOT(moveColumnRight()));

	actionMoveColLeft = new QAction(QIcon(":/move_col_left.png"), tr("Move &Left"), this);
	connect(actionMoveColLeft, SIGNAL(triggered()), this, SLOT(moveColumnLeft()));

	actionMoveColFirst = new QAction(QIcon(":/move_col_first.png"), tr("Move to F&irst"), this);
	connect(actionMoveColFirst, SIGNAL(triggered()), this, SLOT(moveColumnFirst()));

	actionMoveColLast = new QAction(QIcon(":/move_col_last.png"), tr("Move to Las&t"), this);
	connect(actionMoveColLast, SIGNAL(triggered()), this, SLOT(moveColumnLast()));

	actionAdjustColumnWidth = new QAction(QIcon(":/adjust_col_width.png"), tr("Ad&just Column Width"), this);
	connect(actionAdjustColumnWidth, SIGNAL(triggered()), this, SLOT(adjustColumnWidth()));

	actionShowColsDialog = new QAction(tr("&Columns..."), this);
	connect(actionShowColsDialog, SIGNAL(triggered()), this, SLOT(showColsDialog()));

	actionShowRowsDialog = new QAction(tr("&Rows..."), this);
	connect(actionShowRowsDialog, SIGNAL(triggered()), this, SLOT(showRowsDialog()));

    actionDeleteRows = new QAction(tr("&Delete Rows Interval..."), this);
	connect(actionDeleteRows, SIGNAL(triggered()), this, SLOT(showDeleteRowsDialog()));

	actionMoveRowUp = new QAction(QIcon(":/move_row_up.png"), tr("&Upward"), this);
	connect(actionMoveRowUp, SIGNAL(triggered()), this, SLOT(moveTableRowUp()));

	actionMoveRowDown = new QAction(QIcon(":/move_row_down.png"), tr("&Downward"), this);
	connect(actionMoveRowDown, SIGNAL(triggered()), this, SLOT(moveTableRowDown()));

	actionAbout = new QAction(tr("&About QtiSAS"), this);
	actionAbout->setShortcut( tr("F1") );
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    actionAboutQt = new QAction(tr("&About Qt"), this);
    connect(actionAboutQt, &QAction::triggered, this, &ApplicationWindow::aboutQt);

#ifdef Q_OS_MACOS
    actionAbout->setMenuRole(QAction::NoRole);
    actionAboutQt->setMenuRole(QAction::NoRole);
#endif

	actionRename = new QAction(tr("&Rename Window") + "...", this);
	connect(actionRename, SIGNAL(triggered()), this, SLOT(rename()));

	actionNextWindow = new QAction(QIcon(":/next.png"), tr("&Next","next window"), this);
	actionNextWindow->setShortcut( tr("F5","next window shortcut") );

	actionPrevWindow = new QAction(QIcon(":/prev.png"), tr("&Previous","previous window"), this);
	actionPrevWindow->setShortcut( tr("F6","previous window shortcut") );

	actionCloseWindow = new QAction(QIcon(":/close.png"), tr("Close &Window"), this);
	connect(actionCloseWindow, SIGNAL(triggered()), this, SLOT(closeActiveWindow()));

	actionAddColToTable = new QAction(QIcon(":/addCol.png"), tr("Add Column"), this);
	connect(actionAddColToTable, SIGNAL(triggered()), this, SLOT(addColToTable()));

	actionGoToRow = new QAction(tr("&Go to Row..."), this);
	actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));
	connect(actionGoToRow, SIGNAL(triggered()), this, SLOT(goToRow()));

    actionGoToColumn = new QAction(tr("Go to Colum&n..."), this);
	actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));
	connect(actionGoToColumn, SIGNAL(triggered()), this, SLOT(goToColumn()));

	actionClearTable = new QAction(QIcon(":/erase.png"), tr("Clear"), this);
	connect(actionClearTable, SIGNAL(triggered()), this, SLOT(clearTable()));

	actionDeleteLayer = new QAction(QIcon(":/delete.png"), tr("&Remove Layer"), this);
	actionDeleteLayer->setShortcut( tr("Alt+R") );
    setupActionIcon(actionDeleteLayer);
	connect(actionDeleteLayer, SIGNAL(triggered()), this, SLOT(deleteLayer()));

	actionResizeActiveWindow = new QAction(QIcon(":/resize.png"), tr("Window &Geometry..."), this);
	connect(actionResizeActiveWindow, SIGNAL(triggered()), this, SLOT(resizeActiveWindow()));

	actionHideActiveWindow = new QAction(tr("&Hide Window"), this);
	actionHideActiveWindow->setShortcut(tr("Ctrl+Alt+H"));
	connect(actionHideActiveWindow, SIGNAL(triggered()), this, SLOT(hideActiveWindow()));

	actionShowMoreWindows = new QAction(tr("More windows..."), this);
	connect(actionShowMoreWindows, SIGNAL(triggered()), this, SLOT(showMoreWindows()));

	actionPixelLineProfile = new QAction(QIcon(":/pixelProfile.png"), tr("&View Pixel Line Profile"), this);
	connect(actionPixelLineProfile, SIGNAL(triggered()), this, SLOT(pixelLineProfile()));

	actionIntensityTable = new QAction(tr("&Intensity Table"), this);
	connect(actionIntensityTable, SIGNAL(triggered()), this, SLOT(intensityTable()));

	actionShowLineDialog = new QAction(tr("&Properties"), this);
	connect(actionShowLineDialog, SIGNAL(triggered()), this, SLOT(showLineDialog()));

	actionShowTextDialog = new QAction(tr("&Properties"), this);
	connect(actionShowTextDialog, SIGNAL(triggered()), this, SLOT(showEnrichementDialog()));

	actionActivateWindow = new QAction(tr("&Activate Window"), this);
	connect(actionActivateWindow, SIGNAL(triggered()), this, SLOT(activateWindow()));

	actionMinimizeWindow = new QAction(tr("Mi&nimize Window"), this);
	connect(actionMinimizeWindow, SIGNAL(triggered()), this, SLOT(minimizeWindow()));

	actionMaximizeWindow = new QAction(tr("Ma&ximize Window"), this);
	connect(actionMaximizeWindow, SIGNAL(triggered()), this, SLOT(maximizeWindow()));

	actionHideWindow = new QAction(tr("&Hide Window"), this);
	actionHideWindow->setShortcut(tr("Ctrl+Alt+H"));
	connect(actionHideWindow, SIGNAL(triggered()), this, SLOT(hideWindow()));

	actionResizeWindow = new QAction(QIcon(":/resize.png"), tr("Re&size Window..."), this);
	connect(actionResizeWindow, SIGNAL(triggered()), this, SLOT(resizeWindow()));

	actionEditSurfacePlot = new QAction(tr("&Surface..."), this);
	connect(actionEditSurfacePlot, SIGNAL(triggered()), this, SLOT(editSurfacePlot()));

	actionAdd3DData = new QAction(tr("&Data Set..."), this);
	connect(actionAdd3DData, SIGNAL(triggered()), this, SLOT(add3DData()));

	actionSetMatrixProperties = new QAction(tr("Set &Properties..."), this);
	connect(actionSetMatrixProperties, SIGNAL(triggered()), this, SLOT(showMatrixDialog()));

	actionSetMatrixDimensions = new QAction(tr("Set &Dimensions..."), this);
	connect(actionSetMatrixDimensions, SIGNAL(triggered()), this, SLOT(showMatrixSizeDialog()));
	actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));

	actionSetMatrixValues = new QAction(QIcon(":/formula.png"), tr("Set &Values..."), this);
	connect(actionSetMatrixValues, SIGNAL(triggered()), this, SLOT(showMatrixValuesDialog()));
	actionSetMatrixValues->setShortcut(tr("Alt+Q"));

	actionImagePlot = new QAction(QIcon(":/image_plot.png"), tr("&Image Plot"), this);
	connect(actionImagePlot, SIGNAL(triggered()), this, SLOT(plotImage()));

	actionImageProfilesPlot = new QAction(QIcon(":/image_profiles.png"), tr("&Image Profiles"), this);
	connect(actionImageProfilesPlot, SIGNAL(triggered()), this, SLOT(plotImageProfiles()));

	actionTransposeMatrix = new QAction(tr("&Transpose"), this);
	connect(actionTransposeMatrix, SIGNAL(triggered()), this, SLOT(transposeMatrix()));

	actionFlipMatrixVertically = new QAction(QIcon(":/flip_vertical.png"), tr("Flip &V"), this);
	actionFlipMatrixVertically->setShortcut(tr("Ctrl+Shift+V"));
	connect(actionFlipMatrixVertically, SIGNAL(triggered()), this, SLOT(flipMatrixVertically()));

	actionFlipMatrixHorizontally = new QAction(QIcon(":/flip_horizontal.png"), tr("Flip &H"), this);
	actionFlipMatrixHorizontally->setShortcut(tr("Ctrl+Shift+H"));
	connect(actionFlipMatrixHorizontally, SIGNAL(triggered()), this, SLOT(flipMatrixHorizontally()));

    actionRotateMatrix = new QAction(QIcon(":/rotate_clockwise.png"), tr("R&otate 90°"), this);
	actionRotateMatrix->setShortcut(tr("Ctrl+Shift+R"));
	connect(actionRotateMatrix, SIGNAL(triggered()), this, SLOT(rotateMatrix90()));

    actionRotateMatrixMinus = new QAction(QIcon(":/rotate_counterclockwise.png"), tr("Rotate &-90°"), this);
	actionRotateMatrixMinus->setShortcut(tr("Ctrl+Alt+R"));
	connect(actionRotateMatrixMinus, SIGNAL(triggered()), this, SLOT(rotateMatrixMinus90()));

	actionInvertMatrix = new QAction(tr("&Invert"), this);
	connect(actionInvertMatrix, SIGNAL(triggered()), this, SLOT(invertMatrix()));

	actionMatrixDeterminant = new QAction(tr("&Determinant"), this);
	connect(actionMatrixDeterminant, SIGNAL(triggered()), this, SLOT(matrixDeterminant()));

	actionViewMatrixImage = new QAction(tr("&Image mode"), this);
	actionViewMatrixImage->setShortcut(tr("Ctrl+Shift+I"));
	connect(actionViewMatrixImage, SIGNAL(triggered()), this, SLOT(viewMatrixImage()));
	//actionViewMatrixImage->setCheckable(true);

	actionViewMatrix = new QAction(tr("&Data mode"), this);
	actionViewMatrix->setShortcut(tr("Ctrl+Shift+D"));
	connect(actionViewMatrix, SIGNAL(triggered()), this, SLOT(viewMatrixTable()));
	actionViewMatrix->setCheckable(true);

    actionMatrixXY = new QAction(tr("Show &X/Y"), this);
	actionMatrixXY->setShortcut(tr("Ctrl+Shift+X"));
	connect(actionMatrixXY, SIGNAL(triggered()), this, SLOT(viewMatrixXY()));
	actionMatrixXY->setCheckable(true);

    actionMatrixColumnRow = new QAction(tr("Show &Column/Row"), this);
	actionMatrixColumnRow->setShortcut(tr("Ctrl+Shift+C"));
	connect(actionMatrixColumnRow, SIGNAL(triggered()), this, SLOT(viewMatrixColumnRow()));
	actionMatrixColumnRow->setCheckable(true);

    actionMatrixGrayScale = new QAction(tr("&Gray Scale"), this);
	connect(actionMatrixGrayScale, SIGNAL(triggered()), this, SLOT(setMatrixGrayScale()));
	actionMatrixGrayScale->setCheckable(true);

	actionMatrixDefaultScale = new QAction(tr("&Default"), this);
	connect(actionMatrixDefaultScale, SIGNAL(triggered()), this, SLOT(setMatrixDefaultScale()));
	actionMatrixDefaultScale->setCheckable(true);

	actionMatrixRainbowScale = new QAction(tr("&Rainbow"), this);
	connect(actionMatrixRainbowScale, SIGNAL(triggered()), this, SLOT(setMatrixRainbowScale()));
	actionMatrixRainbowScale->setCheckable(true);

	actionMatrixCustomScale = new QAction(tr("&Custom"), this);
	connect(actionMatrixCustomScale, SIGNAL(triggered()), this, SLOT(showColorMapDialog()));
	actionMatrixCustomScale->setCheckable(true);

	actionExportMatrix = new QAction(QIcon(":/monalisa.png"), tr("&Export Image ..."), this);
	connect(actionExportMatrix, SIGNAL(triggered()), this, SLOT(exportMatrix()));

	actionConvertMatrixDirect = new QAction(tr("&Direct"), this);
	connect(actionConvertMatrixDirect, SIGNAL(triggered()), this, SLOT(convertMatrixToTableDirect()));

	actionConvertMatrixXYZ = new QAction(tr("&XYZ Columns"), this);
	connect(actionConvertMatrixXYZ, SIGNAL(triggered()), this, SLOT(convertMatrixToTableXYZ()));

	actionConvertMatrixYXZ = new QAction(tr("&YXZ Columns"), this);
	connect(actionConvertMatrixYXZ, SIGNAL(triggered()), this, SLOT(convertMatrixToTableYXZ()));

    actionMatrixFFTDirect = new QAction(tr("&Forward FFT"), this);
	connect(actionMatrixFFTDirect, SIGNAL(triggered()), this, SLOT(matrixDirectFFT()));

	actionMatrixFFTInverse = new QAction(tr("&Inverse FFT"), this);
	connect(actionMatrixFFTInverse, SIGNAL(triggered()), this, SLOT(matrixInverseFFT()));

	actionConvertTableDirect= new QAction(tr("&Direct"), this);
	connect(actionConvertTableDirect, SIGNAL(triggered()), this, SLOT(convertTableToMatrix()));

	actionConvertTableBinning = new QAction(tr("2D &Binning"), this);
	connect(actionConvertTableBinning, SIGNAL(triggered()), this, SLOT(showBinMatrixDialog()));

	actionConvertTableRegularXYZ = new QAction(tr("&Regular XYZ"), this);
	connect(actionConvertTableRegularXYZ, SIGNAL(triggered()), this, SLOT(tableToMatrixRegularXYZ()));

	actionConvertTableRandomXYZ = new QAction(tr("Random &XYZ..."), this);
	connect(actionConvertTableRandomXYZ, SIGNAL(triggered()), this, SLOT(convertTableToMatrixRandomXYZ()));

	actionExpandMatrix = new QAction(tr("&Expand..."), this);
	connect(actionExpandMatrix, SIGNAL(triggered()), this, SLOT(expandMatrix()));

	actionShrinkMatrix = new QAction(tr("&Shrink..."), this);
	connect(actionShrinkMatrix, SIGNAL(triggered()), this, SLOT(shrinkMatrix()));

	actionSmoothMatrix = new QAction(tr("S&mooth"), this);
	connect(actionSmoothMatrix, SIGNAL(triggered()), this, SLOT(smoothMatrix()));

	actionPlot3DWireFrame = new QAction(QIcon(":/lineMesh.png"), tr("3D &Wire Frame"), this);
	connect(actionPlot3DWireFrame, SIGNAL(triggered()), this, SLOT(plot3DWireframe()));

	actionPlot3DHiddenLine = new QAction(QIcon(":/grid_only.png"), tr("3D &Hidden Line"), this);
	connect(actionPlot3DHiddenLine, SIGNAL(triggered()), this, SLOT(plot3DHiddenLine()));

	actionPlot3DPolygons = new QAction(QIcon(":/no_grid.png"), tr("3D &Polygons"), this);
	connect(actionPlot3DPolygons, SIGNAL(triggered()), this, SLOT(plot3DPolygons()));

	actionPlot3DWireSurface = new QAction(QIcon(":/grid_poly.png"), tr("3D Wire &Surface"), this);
	connect(actionPlot3DWireSurface, SIGNAL(triggered()), this, SLOT(plot3DWireSurface()));

	actionColorMap = new QAction(QIcon(":/color_map.png"), tr("Contour - &Color Fill"), this);
	connect(actionColorMap, SIGNAL(triggered()), this, SLOT(plotColorMap()));

	actionContourMap = new QAction(QIcon(":/contour_map.png"), tr("Contour &Lines"), this);
	connect(actionContourMap, SIGNAL(triggered()), this, SLOT(plotContour()));

	actionGrayMap = new QAction(QIcon(":/gray_map.png"), tr("&Gray Scale Map"), this);
	connect(actionGrayMap, SIGNAL(triggered()), this, SLOT(plotGrayScale()));

	actionSortTable = new QAction(QIcon(":/sort.png"), tr("Sort Ta&ble") + "...", this);
	connect(actionSortTable, SIGNAL(triggered()), this, SLOT(sortActiveTable()));

	actionSortSelection = new QAction(QIcon(":/sort.png"), tr("&Custom") + "...", this);
	connect(actionSortSelection, SIGNAL(triggered()), this, SLOT(sortSelection()));

	actionNormalizeTable = new QAction(tr("&Table"), this);
	connect(actionNormalizeTable, SIGNAL(triggered()), this, SLOT(normalizeActiveTable()));

	actionNormalizeSelection = new QAction(tr("&Columns"), this);
	connect(actionNormalizeSelection, SIGNAL(triggered()), this, SLOT(normalizeSelection()));

	actionCorrelate = new QAction(tr("Co&rrelate"), this);
	connect(actionCorrelate, SIGNAL(triggered()), this, SLOT(correlate()));

	actionAutoCorrelate = new QAction(tr("&Autocorrelate"), this);
	connect(actionAutoCorrelate, SIGNAL(triggered()), this, SLOT(autoCorrelate()));

	actionConvolute = new QAction(tr("&Convolute"), this);
	connect(actionConvolute, SIGNAL(triggered()), this, SLOT(convolute()));

	actionDeconvolute = new QAction(tr("&Deconvolute"), this);
	connect(actionDeconvolute, SIGNAL(triggered()), this, SLOT(deconvolute()));

	actionTranslateHor = new QAction(tr("&Horizontal"), this);
	connect(actionTranslateHor, SIGNAL(triggered()), this, SLOT(translateCurveHor()));

	actionTranslateVert = new QAction(tr("&Vertical"), this);
	connect(actionTranslateVert, SIGNAL(triggered()), this, SLOT(translateCurve()));

	actionSetAscValues = new QAction(QIcon(":/rowNumbers.png"),tr("Ro&w Numbers"), this);
	connect(actionSetAscValues, SIGNAL(triggered()), this, SLOT(setAscValues()));

	actionSetRandomValues = new QAction(QIcon(":/randomNumbers.png"),tr("&Random Values"), this);
	connect(actionSetRandomValues, SIGNAL(triggered()), this, SLOT(setRandomValues()));

	actionSetRandomNormalValues = new QAction(QIcon(":/normalRandomNumbers.png"), tr("&Normal Random Numbers"), this);
	connect(actionSetRandomNormalValues, SIGNAL(triggered()), this, SLOT(setNormalRandomValues()));

	actionFrequencyCount = new QAction(tr("&Frequency Count ..."), this);
	connect(actionFrequencyCount, SIGNAL(triggered()), this, SLOT(showFrequencyCountDialog()));

	actionShapiroWilk = new QAction(tr("&Normality Test (Shapiro - Wilk)") + "...", this);
	connect(actionShapiroWilk, SIGNAL(triggered()), this, SLOT(testNormality()));

	actionChiSquareTest = new QAction(tr("Chi-square Test for &Variance..."), this);
	connect(actionChiSquareTest, SIGNAL(triggered()), this, SLOT(showChiSquareTestDialog()));

	actionOneSampletTest = new QAction(tr("&One Sample t-Test..."), this);
	connect(actionOneSampletTest, SIGNAL(triggered()), this, SLOT(showStudentTestDialog()));

	actionTwoSampletTest = new QAction(tr("&Two Sample t-Test..."), this);
	connect(actionTwoSampletTest, SIGNAL(triggered()), this, SLOT(showTwoSampleStudentTestDialog()));

	actionOneWayANOVA = new QAction(tr("&One-Way ANOVA..."), this);
	connect(actionOneWayANOVA, SIGNAL(triggered()), this, SLOT(showANOVADialog()));

	actionTwoWayANOVA = new QAction(tr("&Two-Way ANOVA..."), this);
	connect(actionTwoWayANOVA, SIGNAL(triggered()), this, SLOT(showTwoWayANOVADialog()));

    actionReadOnlyCol = new QAction(tr("&Read Only"), this);
    connect(actionReadOnlyCol, SIGNAL(triggered()), this, SLOT(setReadOnlyCol()));

	actionSetXCol = new QAction(QIcon(":/x_col.png"), tr("&X"), this);
	connect(actionSetXCol, SIGNAL(triggered()), this, SLOT(setXCol()));

	actionSetYCol = new QAction(QIcon(":/y_col.png"), tr("&Y"), this);
	connect(actionSetYCol, SIGNAL(triggered()), this, SLOT(setYCol()));

	actionSetZCol = new QAction(QIcon(":/z_col.png"), tr("&Z"), this);
	connect(actionSetZCol, SIGNAL(triggered()), this, SLOT(setZCol()));

	actionSetXErrCol = new QAction(tr("X E&rror"), this);
	connect(actionSetXErrCol, SIGNAL(triggered()), this, SLOT(setXErrCol()));

	actionSetYErrCol = new QAction(QIcon(":/errors.png"), tr("Y &Error"), this);
	connect(actionSetYErrCol, SIGNAL(triggered()), this, SLOT(setYErrCol()));

	actionDisregardCol = new QAction(QIcon(":/disregard_col.png"), tr("&Disregard"), this);
	connect(actionDisregardCol, SIGNAL(triggered()), this, SLOT(disregardCol()));

	actionSetLabelCol = new QAction(QIcon(":/set_label_col.png"), tr("&Label"), this);
	connect(actionSetLabelCol, SIGNAL(triggered()), this, SLOT(setLabelCol()));

	actionBoxPlot = new QAction(QIcon(":/boxPlot.png"),tr("&Box Plot"), this);
	connect(actionBoxPlot, SIGNAL(triggered()), this, SLOT(plotBox()));

	actionMultiPeakGauss = new QAction(tr("&Gaussian..."), this);
	connect(actionMultiPeakGauss, SIGNAL(triggered()), this, SLOT(fitMultiPeakGauss()));

	actionMultiPeakLorentz = new QAction(tr("&Lorentzian..."), this);
	connect(actionMultiPeakLorentz, SIGNAL(triggered()), this, SLOT(fitMultiPeakLorentz()));

	actionSubtractLine = new QAction(tr("&Straight Line..."), this);
	connect(actionSubtractLine, SIGNAL(triggered()), this, SLOT(subtractStraightLine()));

	actionSubtractReference = new QAction(tr("&Reference Data..."), this);
	connect(actionSubtractReference, SIGNAL(triggered()), this, SLOT(subtractReferenceData()));

	actionBaseline = new QAction(tr("&Baseline..."), this);
	connect(actionBaseline, SIGNAL(triggered()), this, SLOT(baselineDialog()));

    actionSaveGraphAsProject = new QAction(QIcon(":/project_pdf.png"), tr("Save Graph as Project & Image(s)"), this);
    setupActionIcon(actionSaveGraphAsProject);
    connect(actionSaveGraphAsProject, SIGNAL(triggered()), this, SLOT(saveGraphAsProject()));

#ifdef SCRIPTING_PYTHON
	actionScriptingLang = new QAction(tr("Scripting &language") + "...", this);
	connect(actionScriptingLang, SIGNAL(triggered()), this, SLOT(showScriptingLangDialog()));

	actionCommentSelection = new QAction(QIcon(":/comment.png"), tr("Commen&t Selection"), this);
	actionCommentSelection->setEnabled(false);
	connect(actionCommentSelection, SIGNAL(triggered()), this, SLOT(commentSelection()));

	actionUncommentSelection = new QAction(QIcon(":/uncomment.png"), tr("&Uncomment Selection"), this);
	actionUncommentSelection->setEnabled(false);
	connect(actionUncommentSelection, SIGNAL(triggered()), this, SLOT(uncommentSelection()));
#endif

	actionRestartScripting = new QAction(tr("&Restart scripting"), this);
	connect(actionRestartScripting, SIGNAL(triggered()), this, SLOT(restartScriptingEnv()));

	actionNoteExecuteAll = new QAction(QIcon(":/play.png"), tr("Execute &All"), this);
    actionNoteExecuteAll->setShortcut(tr("Ctrl+R"));
	connect(actionNoteExecuteAll, SIGNAL(triggered()), this, SLOT(executeAll()));

	actionShowNoteLineNumbers = new QAction(tr("Show Line &Numbers"), this);
	actionShowNoteLineNumbers->setCheckable(true);
	connect(actionShowNoteLineNumbers, SIGNAL(toggled(bool)), this, SLOT(showNoteLineNumbers(bool)));

	actionFind = new QAction(QIcon(":/find.png"), tr("&Find..."), this);
    actionFind->setShortcut(tr("Ctrl+Shift+F"));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(noteFindDialogue()));

	actionFindNext = new QAction(QIcon(":/find_next.png"), tr("Find &Next"), this);
	actionFindNext->setShortcut(tr("F3"));
	connect(actionFindNext, SIGNAL(triggered()), this, SLOT(noteFindNext()));

	actionFindPrev = new QAction(QIcon(":/find_previous.png"), tr("Find &Previous"), this);
	actionFindPrev->setShortcut(tr("F4"));
	connect(actionFindPrev, SIGNAL(triggered()), this, SLOT(noteFindPrev()));

	actionReplace = new QAction(QIcon(":/replace.png"), tr("&Replace..."), this);
	connect(actionReplace, SIGNAL(triggered()), this, SLOT(noteReplaceDialogue()));

	actionIncreaseIndent = new QAction(QIcon(":/increase_indent.png"), tr("Increase Indent"), this);
	connect(actionIncreaseIndent, SIGNAL(triggered()), this, SLOT(increaseNoteIndent()));

	actionDecreaseIndent = new QAction(QIcon(":/decrease_indent.png"),tr("Decrease Indent"), this);
	connect(actionDecreaseIndent, SIGNAL(triggered()), this, SLOT(decreaseNoteIndent()));

	actionRenameNoteTab = new QAction(tr("Rena&me Tab..."), this);
	connect(actionRenameNoteTab, SIGNAL(triggered()), this, SLOT(renameCurrentNoteTab()));

	actionAddNoteTab = new QAction(QIcon(":/plus.png"), tr("A&dd Tab"), this);
	connect(actionAddNoteTab, SIGNAL(triggered()), this, SLOT(addNoteTab()));

	actionCloseNoteTab = new QAction(QIcon(":/delete.png"), tr("C&lose Tab"), this);
	connect(actionCloseNoteTab, SIGNAL(triggered()), this, SLOT(closeNoteTab()));

#ifdef SCRIPTING_PYTHON
	actionShowScriptWindow = new QAction(QIcon(":/python.png"), tr("&Script Window"), this);
    actionShowScriptWindow->setShortcut(QKeySequence(Qt::ALT, Qt::Key_F3));
	actionShowScriptWindow->setCheckable(true);
	connect(actionShowScriptWindow, SIGNAL(triggered()), this, SLOT(showScriptWindow()));

	actionOpenQtDesignerUi = new QAction(tr("Load Custom User &Interface..."), this);
	connect(actionOpenQtDesignerUi, SIGNAL(triggered()), this, SLOT(openQtDesignerUi()));
#endif

	actionShowCurvePlotDialog = new QAction(tr("&Plot details..."), this);
	connect(actionShowCurvePlotDialog, SIGNAL(triggered()), this, SLOT(showCurvePlotDialog()));

	actionShowCurveWorksheet = new QAction(tr("&Worksheet"), this);
	connect(actionShowCurveWorksheet, SIGNAL(triggered()), this, SLOT(showCurveWorksheet()));

	actionCurveFullRange = new QAction(tr("&Reset to Full Range"), this);
	connect(actionCurveFullRange, SIGNAL(triggered()), this, SLOT(setCurveFullRange()));

	actionEditCurveRange = new QAction(tr("Edit &Range..."), this);
	connect(actionEditCurveRange, SIGNAL(triggered()), this, SLOT(showCurveRangeDialog()));

	actionRemoveCurve = new QAction(QIcon(":/close.png"), tr("&Delete"), this);
	connect(actionRemoveCurve, SIGNAL(triggered()), this, SLOT(removeCurve()));

	actionHideCurve = new QAction(tr("&Hide"), this);
	connect(actionHideCurve, SIGNAL(triggered()), this, SLOT(hideCurve()));

	actionHideOtherCurves = new QAction(tr("Hide &Other Curves"), this);
	connect(actionHideOtherCurves, SIGNAL(triggered()), this, SLOT(hideOtherCurves()));

	actionShowAllCurves = new QAction(tr("&Show All Curves"), this);
	connect(actionShowAllCurves, SIGNAL(triggered()), this, SLOT(showAllCurves()));

	actionToolBars = new QAction(tr("&Toolbars..."), this);
	actionToolBars->setShortcut(tr("Ctrl+Shift+T"));
	connect(actionToolBars, SIGNAL(triggered()), this, SLOT(showToolBarsMenu()));

	actionFontBold = new QAction("B", this);
	actionFontBold->setToolTip(tr("Bold"));
	QFont font = appFont;
	font.setBold(true);
	actionFontBold->setFont(font);
	actionFontBold->setCheckable(true);
	connect(actionFontBold, SIGNAL(toggled(bool)), this, SLOT(setBoldFont(bool)));

	actionFontItalic = new QAction("It", this);
	actionFontItalic->setToolTip(tr("Italic"));
	font = appFont;
	font.setItalic(true);
	actionFontItalic->setFont(font);
	actionFontItalic->setCheckable(true);
	connect(actionFontItalic, SIGNAL(toggled(bool)), this, SLOT(setItalicFont(bool)));

	actionSuperscript = new QAction(QIcon(":/exp.png"), tr("Superscript"), this);
	connect(actionSuperscript, SIGNAL(triggered()), this, SLOT(insertSuperscript()));
    actionSuperscript->setEnabled(false);

	actionSubscript = new QAction(QIcon(":/index.png"), tr("Subscript"), this);
	connect(actionSubscript, SIGNAL(triggered()), this, SLOT(insertSubscript()));
	actionSubscript->setEnabled(false);

	actionUnderline = new QAction("U", this);
	actionUnderline->setToolTip(tr("Underline (Ctrl+U)"));
	actionUnderline->setShortcut(tr("Ctrl+U"));
    font = appFont;
	font.setUnderline(true);
	actionUnderline->setFont(font);
	connect(actionUnderline, SIGNAL(triggered()), this, SLOT(underline()));
	actionUnderline->setEnabled(false);

	actionGreekSymbol = new QAction(QString(QChar(0x3B1)) + QString(QChar(0x3B2)), this);
	actionGreekSymbol->setToolTip(tr("Greek"));
	connect(actionGreekSymbol, SIGNAL(triggered()), this, SLOT(insertGreekSymbol()));

    actionGreekMajSymbol = new QAction(QString(QChar(0x393)), this);
	actionGreekMajSymbol->setToolTip(tr("Greek"));
	connect(actionGreekMajSymbol, SIGNAL(triggered()), this, SLOT(insertGreekMajSymbol()));

	actionMathSymbol = new QAction(QString(QChar(0x222B)), this);
	actionMathSymbol->setToolTip(tr("Mathematical Symbols"));
	connect(actionMathSymbol, SIGNAL(triggered()), this, SLOT(insertMathSymbol()));

    actionUnicodeSymbol = new QAction("Unicode", this);
    actionUnicodeSymbol->setToolTip(tr("Insert Unicode Symbol"));
    connect(actionUnicodeSymbol, SIGNAL(triggered()), this, SLOT(insertUnicodeSymbol()));

	actionIncreasePrecision = new QAction(QIcon(":/increase_decimals.png"), tr("Increase Precision"), this);
	connect(actionIncreasePrecision, SIGNAL(triggered()), this, SLOT(increasePrecision()));

	actionDecreasePrecision = new QAction(QIcon(":/decrease_decimals.png"), tr("Decrease Precision"), this);
	connect(actionDecreasePrecision, SIGNAL(triggered()), this, SLOT(decreasePrecision()));

    actionCopyStatusBarText = new QAction(tr("&Copy status bar text"), this);
    connect(actionCopyStatusBarText, SIGNAL(triggered()), this, SLOT(copyStatusBarText()));
}
 
void ApplicationWindow::translateActionsStrings()
{
    actionFontBold->setToolTip(tr("Bold"));
    actionFontItalic->setToolTip(tr("Italic"));
    actionUnderline->setStatusTip(tr("Underline (Ctrl+U)"));
	actionUnderline->setShortcut(tr("Ctrl+U"));
	actionGreekSymbol->setToolTip(tr("Greek"));
	actionGreekMajSymbol->setToolTip(tr("Greek"));
	actionMathSymbol->setToolTip(tr("Mathematical Symbols"));
    actionUnicodeSymbol->setToolTip(tr("Insert Unicode Symbol"));
	actionShowCurvePlotDialog->setText(tr("&Plot details..."));
	actionShowCurveWorksheet->setText(tr("&Worksheet"));
	actionRemoveCurve->setText(tr("&Delete"));

	actionCurveFullRange->setText(tr("&Reset to Full Range"));
	actionEditCurveRange->setText(tr("Edit &Range..."));
	actionHideCurve->setText(tr("&Hide"));
	actionHideOtherCurves->setText(tr("Hide &Other Curves"));
	actionShowAllCurves->setText(tr("&Show All Curves"));

	actionNewProject->setText(tr("New &Project"));
	actionNewProject->setToolTip(tr("Open a new project"));
	actionNewProject->setShortcut(tr("Ctrl+N"));

	actionAppendProject->setText(tr("App&end Project..."));
	actionAppendProject->setToolTip(tr("Append a project to the current folder"));
	actionAppendProject->setShortcut(tr("Ctrl+Alt+A"));

	actionNewFolder->setText(tr("New F&older"));
	actionNewFolder->setToolTip(tr("Create a new folder"));
	actionNewFolder->setShortcut(Qt::Key_F7);

	actionNewGraph->setText(tr("New &Graph"));
	actionNewGraph->setToolTip(tr("Create an empty 2D plot"));
	actionNewGraph->setShortcut(tr("Ctrl+G"));

	actionNewNote->setText(tr("New &Note"));
	actionNewNote->setToolTip(tr("Create an empty note window"));

	actionNewTable->setText(tr("New &Table"));
	actionNewTable->setShortcut(tr("Ctrl+T"));
	actionNewTable->setToolTip(tr("New table"));

	actionNewMatrix->setText(tr("New &Matrix"));
	actionNewMatrix->setShortcut(tr("Ctrl+M"));
	actionNewMatrix->setToolTip(tr("New matrix"));

	actionNewFunctionPlot->setText(tr("New &Function Plot") + "...");
	actionNewFunctionPlot->setToolTip(tr("Create a new 2D function plot"));
	actionNewFunctionPlot->setShortcut(tr("Ctrl+F"));

	actionNewSurfacePlot->setText(tr("New 3D &Surface Plot") + "...");
	actionNewSurfacePlot->setToolTip(tr("Create a new 3D surface plot"));
	actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));

	actionOpen->setText(tr("&Open..."));
	actionOpen->setShortcut(tr("Ctrl+O"));
	actionOpen->setToolTip(tr("Open project"));

	actionLoadImage->setText(tr("Open Image &File..."));
	actionLoadImage->setShortcut(tr("Ctrl+I"));

	actionImportDatabase->setText(tr("&Database..."));
	actionImportSound->setText(tr("&Sound (WAV)..."));
	actionImportImage->setText(tr("Import I&mage..."));

	actionSaveProject->setText(tr("&Save Project"));
	actionSaveProject->setToolTip(tr("Save project"));
	actionSaveProject->setShortcut(tr("Ctrl+S"));

	actionSaveProjectAs->setText(tr("Save Project &As..."));
	actionSaveProjectAs->setShortcut( tr("Ctrl+Shift+S") );

	actionOpenTemplate->setText(tr("Open Te&mplate..."));
	actionOpenTemplate->setToolTip(tr("Open template"));

	actionSaveTemplate->setText(tr("Save As &Template..."));
	actionSaveTemplate->setToolTip(tr("Save window as template"));

	actionSaveWindow->setText(tr("Save &Window As..."));

	actionLoad->setText(tr("&Import ASCII..."));
	actionLoad->setToolTip(tr("Import data file(s)"));
	actionLoad->setShortcut(tr("Ctrl+K"));

	actionUndo->setText(tr("&Undo"));
	actionUndo->setToolTip(tr("Undo changes"));
	actionUndo->setShortcut(tr("Ctrl+Z"));

	actionRedo->setText(tr("&Redo"));
	actionRedo->setToolTip(tr("Redo changes"));

	actionCopyWindow->setText(tr("&Duplicate"));
	actionCopyWindow->setToolTip(tr("Duplicate window"));
	actionCopyWindow->setShortcut(tr("Ctrl+Alt+D"));

	actionCutSelection->setText(tr("Cu&t Selection"));
	actionCutSelection->setToolTip(tr("Cut selection"));
	actionCutSelection->setShortcut(tr("Ctrl+X"));

	actionCopySelection->setText(tr("&Copy Selection"));
	actionCopySelection->setToolTip(tr("Copy selection"));
	actionCopySelection->setShortcut(tr("Ctrl+C"));

	actionPasteSelection->setText(tr("&Paste Selection"));
	actionPasteSelection->setToolTip(tr("Paste selection"));
	actionPasteSelection->setShortcut(tr("Ctrl+V"));

	actionClearSelection->setText(tr("&Delete Selection"));
	actionClearSelection->setToolTip(tr("Delete selection"));
	actionClearSelection->setShortcut(tr("Del","delete key"));

	actionRaiseEnrichment->setText(tr("&Front"));
	actionRaiseEnrichment->setToolTip(tr("Raise object on top"));

	actionLowerEnrichment->setText(tr("&Back"));
	actionLowerEnrichment->setToolTip(tr("Lower object to the bottom"));

	actionAlignTop->setToolTip(tr("Align Top"));
	actionAlignBottom->setToolTip(tr("Align Bottom"));
	actionAlignLeft->setToolTip(tr("Align Left"));
	actionAlignRight->setToolTip(tr("Align Right"));

	actionShowExplorer->setText(tr("Project &Explorer"));
	actionShowExplorer->setShortcut(tr("Ctrl+E"));
	actionShowExplorer->setToolTip(tr("Show project explorer"));

	actionFindWindow->setText(tr("&Find..."));

	actionNextWindow->setText(tr("&Next","next window"));
	actionNextWindow->setShortcut(tr("F5","next window shortcut"));

	actionPrevWindow->setText(tr("&Previous","previous window"));
	actionPrevWindow->setShortcut(tr("F6","previous window shortcut"));

	actionShowLog->setText(tr("Results &Log"));
	actionShowLog->setToolTip(tr("Show analysis results"));

    actionShowUndoStack->setText(tr("&Undo/Redo Stack"));
	actionShowUndoStack->setToolTip(tr("Show available undo/redo commands"));

#ifdef SCRIPTING_CONSOLE
	actionShowConsole->setText(tr("&Console"));
	actionShowConsole->setToolTip(tr("Show Scripting console"));
#endif

#ifdef SCRIPTING_PYTHON
	actionShowScriptWindow->setText(tr("&Script Window"));
	actionShowScriptWindow->setToolTip(tr("Script Window"));
	actionOpenQtDesignerUi->setText(tr("Load Custom User &Interface..."));
#endif

	actionCustomActionDialog->setText(tr("Add &Custom Script Action..."));

	actionAddLayer->setText(tr("Add La&yer"));
	actionAddLayer->setToolTip(tr("Add Layer"));
	actionAddLayer->setShortcut(tr("ALT+L"));

	actionShowLayerDialog->setText(tr("Arran&ge Layers") + "...");
	actionShowLayerDialog->setToolTip(tr("Arrange Layers"));
	actionShowLayerDialog->setShortcut(tr("Shift+A"));

	actionAutomaticLayout->setText(tr("Automatic Layout"));
	actionAutomaticLayout->setToolTip(tr("Automatic Layout"));

    actionLogLog->setText("Log-Log(Log) Presentation");
    actionLogLog->setToolTip("- \"Log-Log\" Presentation:\tif no axis is selected\n"
                             "- \"Log\" Presentation:\tfor selected axis\n"
                             "- \"Log\" Presentation of Spectrograms Color-Fill:\tfor map and bar scale");

    actionLinLin->setText("Lin-Lin (Lin) Presentation");
    actionLinLin->setToolTip("- \"Lin-Lin\" Presentation:\tif no axis is selected\n"
                             "- \"Lin\" Presentation:\tfor selected axis\n"
                             "- \"Lin\" Presentation of Spectrograms Color-Fill:\tfor map and bar scale");

	actionExportGraph->setText(tr("&Current") + "...");
	actionExportGraph->setShortcut(tr("Ctrl+Alt+G"));
	actionExportGraph->setToolTip(tr("Export current graph"));

    actionExportLayer->setText(tr("Export current layer") + "...");
    actionExportLayer->setToolTip(tr("Export current layer"));

	actionExportAllGraphs->setText(tr("&All") + "...");
	actionExportAllGraphs->setShortcut(tr("Alt+X"));
	actionExportAllGraphs->setToolTip(tr("Export all graphs"));
	actionExportPDF->setText(tr("&Export PDF") + "...");
	actionExportPDF->setShortcut(tr("Ctrl+Alt+P"));
	actionExportPDF->setToolTip(tr("Export to PDF"));

	actionPrint->setText(tr("&Print..."));
	actionPrint->setShortcut(tr("Ctrl+P"));
	actionPrint->setToolTip(tr("Print window"));

	actionPrintPreview->setText(tr("Print Pre&view..."));
	actionPrintPreview->setToolTip(tr("Print preview"));

	actionPrintAllPlots->setText(tr("Print All Plo&ts") + "...");
	actionPrintAllPlots->setShortcut(tr("Ctrl+Shift+P"));
	actionShowExportASCIIDialog->setText(tr("E&xport ASCII..."));

	actionCloseAllWindows->setText(tr("&Quit"));
	actionCloseAllWindows->setShortcut(tr("Ctrl+Q"));
	actionCloseProject->setText(tr("&Close"));

	actionClearLogInfo->setText(tr("Clear &Log Information"));
	actionDeleteFitTables->setText(tr("Delete &Fit Tables"));

    actionToolBars->setText(tr("&Toolbars..."));
	actionToolBars->setShortcut(tr("Ctrl+Shift+T"));

	actionShowPlotWizard->setText(tr("Plot &Wizard") + "...");
	actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));

#ifdef Q_OS_MACOS
    actionShowConfigureDialog->setText(tr("Settings..."));
#else
	actionShowConfigureDialog->setText(tr("&Preferences..."));
#endif
	actionShowCurvesDialog->setText(tr("Add/Remove &Curve..."));
	actionShowCurvesDialog->setShortcut(tr("ALT+C"));
	actionShowCurvesDialog->setToolTip(tr("Add curve to graph"));

	actionAddErrorBars->setText(tr("Add &Error Bars..."));
	actionAddErrorBars->setToolTip(tr("Add Error Bars..."));
	actionAddErrorBars->setShortcut(tr("Ctrl+B"));

	actionAddFunctionCurve->setText(tr("Add &Function..."));
	actionAddFunctionCurve->setToolTip(tr("Add Function..."));
	actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));

	actionUnzoom->setText(tr("&Rescale to Show All"));
	actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
	actionUnzoom->setToolTip(tr("Best fit"));

	actionNewLegend->setText( tr("New &Legend"));
	actionNewLegend->setShortcut(tr("Ctrl+L"));
	actionNewLegend->setToolTip(tr("Add new legend"));

	actionTimeStamp->setText(tr("Add Time Stamp"));
	actionTimeStamp->setShortcut(tr("Ctrl+ALT+T"));
	actionTimeStamp->setToolTip(tr("Date & time "));

	actionAddImage->setText(tr("Add &Image"));
	actionAddImage->setToolTip(tr("Add Image"));
	actionAddImage->setShortcut(tr("ALT+I"));

	actionPlotL->setText(tr("&Line"));
	actionPlotL->setToolTip(tr("Plot as line"));

	actionPlotP->setText(tr("&Scatter"));
	actionPlotP->setToolTip(tr("Plot as symbols"));

	actionPlotLP->setText(tr("Line + S&ymbol"));
	actionPlotLP->setToolTip(tr("Plot as line + symbols"));

	actionPlotVerticalDropLines->setText(tr("Vertical &Drop Lines"));

	actionPlotSpline->setText(tr("&Spline"));
	actionPlotVertSteps->setText(tr("&Vertical Steps"));
	actionPlotHorSteps->setText(tr("&Horizontal Steps"));

	actionPlotVerticalBars->setText(tr("&Columns"));
	actionPlotVerticalBars->setToolTip(tr("Plot with vertical bars"));

	actionPlotHorizontalBars->setText(tr("&Rows"));
	actionPlotHorizontalBars->setToolTip(tr("Plot with horizontal bars"));

	actionStackBars->setText(tr("Stack &Bar"));
	actionStackBars->setToolTip(tr("Plot stack bar"));

	actionStackColumns->setText(tr("Stack &Column"));
	actionStackColumns->setToolTip(tr("Plot stack column"));

	actionPlotArea->setText(tr("&Area"));
	actionPlotArea->setToolTip(tr("Plot area"));

	actionPlotPie->setText(tr("&Pie"));
	actionPlotPie->setToolTip(tr("Plot pie"));

	actionPlotVectXYXY->setText(tr("&Vectors XYXY"));
	actionPlotVectXYXY->setToolTip(tr("Vectors XYXY"));

	actionPlotVectXYAM->setText(tr("Vectors XY&AM"));
	actionPlotVectXYAM->setToolTip(tr("Vectors XYAM"));

	actionPlotHistogram->setText( tr("&Histogram"));
	actionPlotStackedHistograms->setText(tr("&Stacked Histogram"));

	actionPlot2VerticalLayers->setText(tr("&Vertical 2 Layers"));
	actionPlot2HorizontalLayers->setText(tr("&Horizontal 2 Layers"));
	actionPlot4Layers->setText(tr("&4 Layers"));
	actionPlotStackedLayers->setText(tr("&Stacked Layers"));

	actionVertSharedAxisLayers->setText(tr("&Vertical 2 Layers"));
	actionHorSharedAxisLayers->setText(tr("&Horizontal 2 Layers"));
	actionSharedAxesLayers->setText(tr("&4 Layers"));
	actionStackSharedAxisLayers->setText(tr("&Stacked Layers"));
	actionCustomSharedAxisLayers->setText(tr("&Custom Layout..."));
	actionCustomLayout->setText(tr("&Custom Layout..."));

	actionStemPlot->setText(tr("Stem-and-&Leaf Plot"));
	actionStemPlot->setToolTip(tr("Stem-and-Leaf Plot"));

    actionPlotDoubleYAxis->setText(tr("D&ouble-Y"));
    actionPlotDoubleYAxis->setToolTip(tr("Double Y Axis"));

    actionAddZoomPlot->setText(tr("&Zoom"));
    actionAddZoomPlot->setToolTip(tr("Zoom"));

	actionWaterfallPlot->setText(tr("&Waterfall Plot"));
	actionWaterfallPlot->setToolTip(tr("Waterfall Plot"));

    actionExtractGraphs->setText(tr("E&xtract to Graphs"));
    actionExtractGraphs->setToolTip(tr("Extract to Graphs"));

    actionExtractLayers->setText(tr("Extract to Layer&s"));
    actionExtractLayers->setToolTip(tr("Extract to Layers"));

	actionPlot3DRibbon->setText(tr("&Ribbon"));
	actionPlot3DRibbon->setToolTip(tr("Plot 3D ribbon"));

	actionPlot3DBars->setText(tr("&Bars"));
	actionPlot3DBars->setToolTip(tr("Plot 3D bars"));

	actionPlot3DScatter->setText(tr("&Scatter"));
	actionPlot3DScatter->setToolTip(tr("Plot 3D scatter"));

	actionPlot3DTrajectory->setText(tr("&Trajectory"));
	actionPlot3DTrajectory->setToolTip(tr("Plot 3D trajectory"));

	actionColorMap->setText(tr("&Color Fill"));
    actionColorMap->setToolTip(tr("Color Fill"));

	actionContourMap->setText(tr("Contour &Lines"));
	actionContourMap->setToolTip(tr("Contour Lines"));

	actionGrayMap->setText(tr("&Gray Scale Map"));
	actionGrayMap->setToolTip(tr("Gray Scale Map"));

	actionShowColStatistics->setText(tr("Statistics on &Columns"));
	actionShowColStatistics->setToolTip(tr("Selected columns statistics"));

	actionShowRowStatistics->setText(tr("Statistics on &Rows"));
	actionShowRowStatistics->setToolTip(tr("Selected rows statistics"));
	actionShowIntDialog->setText(tr("Integr&ate Function..."));
	actionIntegrate->setText(tr("&Integrate") + "...");
	actionInterpolate->setText(tr("Inte&rpolate ..."));
	actionLowPassFilter->setText(tr("&Low Pass..."));
	actionHighPassFilter->setText(tr("&High Pass..."));
	actionBandPassFilter->setText(tr("&Band Pass..."));
	actionBandBlockFilter->setText(tr("&Band Block..."));
	actionFFT->setText(tr("&FFT..."));
	actionSmoothSavGol->setText(tr("&Savitzky-Golay..."));
	actionSmoothFFT->setText(tr("&FFT Filter..."));
	actionSmoothAverage->setText(tr("Moving Window &Average..."));
	actionSmoothLowess->setText(tr("&Lowess..."));
	actionDifferentiate->setText(tr("&Differentiate"));
	actionFitLinear->setText(tr("Fit &Linear"));
	actionFitSlope->setText(tr("Fit Slop&e"));
	actionShowFitPolynomDialog->setText(tr("Fit &Polynomial ..."));
	actionShowExpDecayDialog->setText(tr("&First Order ..."));
	actionShowTwoExpDecayDialog->setText(tr("&Second Order ..."));
	actionShowExpDecay3Dialog->setText(tr("&Third Order ..."));
	actionFitExpGrowth->setText(tr("Fit Exponential Gro&wth ..."));
	actionFitSigmoidal->setText(tr("Fit &Boltzmann (Sigmoidal)"));
	actionFitGauss->setText(tr("Fit &Gaussian"));
	actionFitLorentz->setText(tr("Fit Lorent&zian"));

    actionShowFitDialog->setText(tr("QtiPlot Fit &Wizard..."));
	actionShowFitDialog->setShortcut(tr("Ctrl+Y"));

	actionShowPlotDialog->setText(tr("&Plot ..."));
	actionShowScaleDialog->setText(tr("&Scales..."));
	actionShowAxisDialog->setText(tr("&Axes..."));
	actionShowGridDialog->setText(tr("&Grid ..."));
	actionShowTitleDialog->setText(tr("&Title ..."));
	actionShowColumnOptionsDialog->setText(tr("Column &Options ..."));
	actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
	actionShowColumnValuesDialog->setText(tr("Set Column &Values ..."));
	actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));
	actionTableRecalculate->setText(tr("Recalculate"));
	actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
	actionHideSelectedColumns->setText(tr("&Hide Selected"));
	actionHideSelectedColumns->setToolTip(tr("Hide selected columns"));
	actionShowAllColumns->setText(tr("Sho&w All Columns"));
	actionHideSelectedColumns->setToolTip(tr("Show all table columns"));
	actionSwapColumns->setText(tr("&Swap columns"));
	actionSwapColumns->setToolTip(tr("Swap selected columns"));
	actionMoveColRight->setText(tr("Move &Right"));
    actionMoveColRight->setToolTip(tr("Move Right"));
	actionMoveColLeft->setText(tr("Move &Left"));
    actionMoveColLeft->setToolTip(tr("Move Left"));
	actionMoveColFirst->setText(tr("Move to F&irst"));
	actionMoveColFirst->setToolTip(tr("Move to First"));
	actionMoveColLast->setText(tr("Move to Las&t"));
    actionMoveColLast->setToolTip(tr("Move to Last"));
	actionShowColsDialog->setText(tr("&Columns..."));
	actionShowRowsDialog->setText(tr("&Rows..."));
	actionDeleteRows->setText(tr("&Delete Rows Interval..."));
	actionMoveRowUp->setText(tr("&Upward"));
	actionMoveRowUp->setToolTip(tr("Move current row upward"));
	actionMoveRowDown->setText(tr("&Downward"));
	actionMoveRowDown->setToolTip(tr("Move current row downward"));
	actionAdjustColumnWidth->setText(tr("Ad&just Column Width"));
	actionAdjustColumnWidth->setToolTip(tr("Set optimal column width"));

	actionExtractTableData->setText(tr("&Extract Data..."));

    actionTransposeTable->setText(tr("Transpose Table..."));

	actionAbout->setText(tr("&About QtiSAS"));
	actionAbout->setShortcut(tr("F1"));

	actionRename->setText(tr("&Rename Window") + "...");

	actionCloseWindow->setText(tr("Close &Window"));

	actionAddColToTable->setText(tr("Add Column"));
	actionAddColToTable->setToolTip(tr("Add Column"));
	actionAddColToTable->setShortcut(tr("Alt+C"));

	actionClearTable->setText(tr("Clear"));
	actionGoToRow->setText(tr("&Go to Row..."));
	actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));

    actionGoToColumn->setText(tr("Go to Colum&n..."));
	actionGoToColumn->setShortcut(tr("Ctrl+Alt+C"));

	actionDeleteLayer->setText(tr("&Remove Layer"));
	actionDeleteLayer->setShortcut(tr("Alt+R"));

	actionResizeActiveWindow->setText(tr("Window &Geometry..."));
	actionHideActiveWindow->setText(tr("&Hide Window"));
	actionHideActiveWindow->setShortcut(tr("Ctrl+Alt+H"));
	actionShowMoreWindows->setText(tr("More Windows..."));
	actionPixelLineProfile->setText(tr("&View Pixel Line Profile"));
	actionIntensityTable->setText(tr("&Intensity Table"));
	actionShowLineDialog->setText(tr("&Properties"));
	actionShowTextDialog->setText(tr("&Properties"));
	actionActivateWindow->setText(tr("&Activate Window"));
	actionMinimizeWindow->setText(tr("Mi&nimize Window"));
	actionMaximizeWindow->setText(tr("Ma&ximize Window"));
	actionHideWindow->setText(tr("&Hide Window"));
	actionHideWindow->setShortcut(tr("Ctrl+Alt+H"));
	actionResizeWindow->setText(tr("Re&size Window..."));
	actionEditSurfacePlot->setText(tr("&Surface..."));
	actionAdd3DData->setText(tr("&Data Set..."));
	actionSetMatrixProperties->setText(tr("Set &Properties..."));
	actionSetMatrixDimensions->setText(tr("Set &Dimensions..."));
	actionSetMatrixDimensions->setShortcut(tr("Ctrl+D"));
	actionSetMatrixValues->setText(tr("Set &Values..."));
	actionSetMatrixValues->setToolTip(tr("Set Matrix Values"));
    actionSetMatrixValues->setShortcut(tr("Alt+Q"));
    actionImagePlot->setText(tr("&Image Plot"));
    actionImagePlot->setToolTip(tr("Image Plot"));

	actionImageProfilesPlot->setText(tr("&Image Profiles"));
	actionImageProfilesPlot->setToolTip(tr("Image Profiles"));

	actionTransposeMatrix->setText(tr("&Transpose"));
    actionRotateMatrix->setText(tr("R&otate 90°"));
    actionRotateMatrix->setToolTip(tr("Rotate 90° Clockwise"));
    actionRotateMatrixMinus->setText(tr("Rotate &-90°"));
    actionRotateMatrixMinus->setToolTip(tr("Rotate 90° Counterclockwise"));
	actionFlipMatrixVertically->setText(tr("Flip &V"));
	actionFlipMatrixVertically->setToolTip(tr("Flip Vertically"));
	actionFlipMatrixHorizontally->setText(tr("Flip &H"));
	actionFlipMatrixHorizontally->setToolTip(tr("Flip Horizontally"));

    actionMatrixXY->setText(tr("Show &X/Y"));
    actionMatrixColumnRow->setText(tr("Show &Column/Row"));
	actionViewMatrix->setText(tr("&Data mode"));
	actionViewMatrixImage->setText(tr("&Image mode"));
	actionMatrixDefaultScale->setText(tr("&Default"));
    actionMatrixGrayScale->setText(tr("&Gray Scale"));
	actionMatrixRainbowScale->setText(tr("&Rainbow"));
	actionMatrixCustomScale->setText(tr("&Custom"));
	actionInvertMatrix->setText(tr("&Invert"));
	actionMatrixDeterminant->setText(tr("&Determinant"));
	actionConvertMatrixDirect->setText(tr("&Direct"));
	actionConvertMatrixXYZ->setText(tr("&XYZ Columns"));
	actionConvertMatrixYXZ->setText(tr("&YXZ Columns"));
	actionExportMatrix->setText(tr("&Export Image ..."));

	actionConvertTableDirect->setText(tr("&Direct"));
	actionConvertTableBinning->setText(tr("2D &Binning"));
	actionConvertTableRegularXYZ->setText(tr("&Regular XYZ"));

	actionConvertTableRandomXYZ->setText(tr("Random &XYZ..."));
	actionExpandMatrix->setText(tr("&Expand..."));
	actionShrinkMatrix->setText(tr("&Shrink..."));
	actionSmoothMatrix->setText(tr("S&mooth"));

	actionPlot3DWireFrame->setText(tr("3D &Wire Frame"));
	actionPlot3DHiddenLine->setText(tr("3D &Hidden Line"));
	actionPlot3DPolygons->setText(tr("3D &Polygons"));
	actionPlot3DWireSurface->setText(tr("3D Wire &Surface"));
	actionSortTable->setText(tr("Sort Ta&ble") + "...");
	actionSortSelection->setText(tr("&Custom") + "...");
	actionNormalizeTable->setText(tr("&Table"));
	actionNormalizeSelection->setText(tr("&Columns"));
	actionCorrelate->setText(tr("Co&rrelate"));
	actionAutoCorrelate->setText(tr("&Autocorrelate"));
	actionConvolute->setText(tr("&Convolute"));
	actionDeconvolute->setText(tr("&Deconvolute"));
	actionTranslateHor->setText(tr("&Horizontal"));
	actionTranslateVert->setText(tr("&Vertical"));
	actionSetAscValues->setText(tr("Ro&w Numbers"));
	actionSetAscValues->setToolTip(tr("Fill selected columns with row numbers"));
	actionSetRandomValues->setText(tr("&Random Values"));
	actionSetRandomValues->setToolTip(tr("Fill selected columns with random numbers"));
	actionSetRandomNormalValues->setText(tr("&Normal Random Numbers"));
	actionSetRandomNormalValues->setToolTip(tr("Fill selected columns with normal random numbers"));
	actionChiSquareTest->setText(tr("Chi-square Test for &Variance..."));
	actionFrequencyCount->setText(tr("&Frequency Count ..."));
	actionOneSampletTest->setText(tr("&One Sample t-Test..."));
	actionTwoSampletTest->setText(tr("&Two Sample t-Test..."));
	actionShapiroWilk->setText(tr("&Normality Test (Shapiro - Wilk)") + "...");

	actionOneWayANOVA->setText(tr("&One-Way ANOVA..."));
	actionTwoWayANOVA->setText(tr("&Two-Way ANOVA..."));

	actionSetXCol->setText(tr("&X"));
	actionSetXCol->setToolTip(tr("Set column as X"));
	actionSetYCol->setText(tr("&Y"));
	actionSetYCol->setToolTip(tr("Set column as Y"));
	actionSetZCol->setText(tr("&Z"));
	actionSetZCol->setToolTip(tr("Set column as Z"));
	actionSetXErrCol->setText(tr("X E&rror"));
	actionSetYErrCol->setText(tr("Y &Error"));
	actionSetYErrCol->setToolTip(tr("Set as Y Error Bars"));
	actionSetLabelCol->setText(tr("&Label"));
	actionSetLabelCol->setToolTip(tr("Set as Labels"));
	actionDisregardCol->setText(tr("&Disregard"));
	actionDisregardCol->setToolTip(tr("Disregard Columns"));
	actionReadOnlyCol->setText(tr("&Read Only"));

	actionBoxPlot->setText(tr("&Box Plot"));
	actionBoxPlot->setToolTip(tr("Box and whiskers plot"));

	actionSubtractReference->setText(tr("&Reference Data..."));
	actionSubtractLine->setText(tr("&Straight Line..."));
	actionMultiPeakGauss->setText(tr("&Gaussian..."));
	actionMultiPeakLorentz->setText(tr("&Lorentzian..."));

#ifdef SCRIPTING_PYTHON
	actionScriptingLang->setText(tr("Scripting &language") + "...");
	actionCommentSelection->setText(tr("Commen&t Selection"));
	actionCommentSelection->setToolTip(tr("Comment Selection"));
	actionCommentSelection->setShortcut(tr("Ctrl+Shift+O"));

	actionUncommentSelection->setText(tr("&Uncomment Selection"));
	actionUncommentSelection->setToolTip(tr("Uncomment Selection"));
	actionUncommentSelection->setShortcut(tr("Ctrl+Shift+U"));
#endif
	actionRestartScripting->setText(tr("&Restart scripting"));

	actionNoteExecuteAll->setText(tr("Execute &All"));
    actionNoteExecuteAll->setShortcut(tr("Ctrl+R"));

	actionShowNoteLineNumbers->setText(tr("Show Line &Numbers"));
	actionRenameNoteTab->setText(tr("Rena&me Tab..."));
	actionAddNoteTab->setText(tr("A&dd Tab"));
	actionCloseNoteTab->setText(tr("C&lose Tab"));

	actionFind->setText(tr("&Find..."));
	actionFind->setToolTip(tr("Show find dialog"));
    actionFind->setShortcut(tr("Ctrl+Shift+F"));

	actionFindNext->setText(tr("Find &Next"));
	actionFindNext->setToolTip(tr("Find Next"));
	actionFindNext->setShortcut(tr("F3"));

	actionFindPrev->setText(tr("Find &Previous"));
	actionFindPrev->setToolTip(tr("Find Previous"));
	actionFindPrev->setShortcut(tr("F4"));

	actionReplace->setText(tr("&Replace..."));
	actionReplace->setToolTip(tr("Show replace dialog"));
    actionReplace->setShortcut(tr("Ctrl+Shift+R"));

	actionIncreaseIndent->setToolTip(tr("Increase Indent"));
	actionDecreaseIndent->setToolTip(tr("Decrease Indent"));

	btnPointer->setText(tr("Disable &tools"));
	btnPointer->setToolTip( tr( "Pointer" ) );

	actionMagnify->setText(tr("Zoom &In/Out and Drag Canvas"));
	actionMagnify->setToolTip(tr("Zoom In (Shift++) or Out (-) and Drag Canvas"));

	actionMagnifyHor->setText(tr("Zoom/Drag Canvas &Horizontally"));
	actionMagnifyVert->setText(tr("Zoom/Drag Canvas &Vertically"));

	actionMagnifyHor->setToolTip(tr("Zoom In/Out and Drag Canvas Horizontally"));
	actionMagnifyVert->setToolTip(tr("Zoom In/Out and Drag Canvas Vertically"));

	btnZoomIn->setText(tr("&Zoom In"));
	btnZoomIn->setShortcut(tr("Ctrl++"));
	btnZoomIn->setToolTip(tr("Zoom In"));

	btnZoomOut->setText(tr("Zoom &Out"));
	btnZoomOut->setShortcut(tr("Ctrl+-"));
	btnZoomOut->setToolTip(tr("Zoom Out"));

	btnCursor->setText(tr("&Data Reader"));
	btnCursor->setShortcut(tr("CTRL+D"));
	btnCursor->setToolTip(tr("Data reader"));

	btnSelect->setText(tr("&Select Data Range"));
	btnSelect->setShortcut(QKeySequence(tr("Alt+Shift+S")));
	btnSelect->setToolTip(tr("Select data range"));

	btnPicker->setText(tr("S&creen Reader"));
	btnPicker->setToolTip(tr("Screen reader"));

    actionDrawPoints->setText(tr("&Draw Data Points"));
    actionDrawPoints->setToolTip(tr("Draw Data Points"));

	btnMovePoints->setText(tr("&Move Data Points..."));
	btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
	btnMovePoints->setToolTip(tr("Move data points"));

	actionDragCurve->setText(tr("Dra&g Curve"));
	actionDragCurve->setToolTip(tr("Drag Curve"));

	btnRemovePoints->setText(tr("Remove &Bad Data Points..."));
	btnRemovePoints->setShortcut(tr("Alt+B"));
	btnRemovePoints->setToolTip(tr("Remove data points"));

	actionAddText->setText(tr("Add &Text"));
	actionAddText->setToolTip(tr("Add Text"));
	actionAddText->setShortcut(QKeySequence(tr("Shift+T")));

	actionAddFormula->setText(tr("Add E&quation"));
	actionAddFormula->setToolTip(tr("Add Equation"));
	actionAddFormula->setShortcut( tr("ALT+Q") );

    actionAddRectangle->setText(tr("Add &Rectangle"));
    actionAddRectangle->setToolTip(tr("Add Rectangle"));
	actionAddRectangle->setShortcut( tr("CTRL+ALT+R") );

	actionAddEllipse->setText(tr("Add &Ellipse"));
    actionAddEllipse->setToolTip(tr("Add Ellipse/Circle"));
	actionAddEllipse->setShortcut( tr("CTRL+ALT+E") );

	btnArrow->setText(tr("Draw &Arrow"));
	btnArrow->setShortcut(tr("CTRL+ALT+A"));
	btnArrow->setToolTip(tr("Draw arrow"));

	btnLine->setText(tr("Draw &Line"));
	btnLine->setShortcut(tr("CTRL+ALT+L"));
	btnLine->setToolTip(tr("Draw line"));

	// FIXME: is setText necessary for action groups?
	//	coord->setText( tr( "Coordinates" ) );
	//	coord->setText( tr( "&Coord" ) );
	//  coord->setStatusTip( tr( "Coordinates" ) );
	Box->setText( tr( "Box" ) );
	Box->setText( tr( "Box" ) );
	Box->setToolTip( tr( "Box" ) );
	Box->setStatusTip( tr( "Box" ) );
	Frame->setText( tr( "Frame" ) );
	Frame->setText( tr( "&Frame" ) );
	Frame->setToolTip( tr( "Frame" ) );
	Frame->setStatusTip( tr( "Frame" ) );
	None->setText( tr( "No Axes" ) );
	None->setText( tr( "No Axes" ) );
	None->setToolTip( tr( "No axes" ) );
	None->setStatusTip( tr( "No axes" ) );

	front->setToolTip( tr( "Front grid" ) );
	back->setToolTip( tr( "Back grid" ) );
	right->setToolTip( tr( "Right grid" ) );
	left->setToolTip( tr( "Left grid" ) );
	ceil->setToolTip( tr( "Ceiling grid" ) );
	floor->setToolTip( tr( "Floor grid" ) );

	wireframe->setText( tr( "Wireframe" ) );
	wireframe->setText( tr( "Wireframe" ) );
	wireframe->setToolTip( tr( "Wireframe" ) );
	wireframe->setStatusTip( tr( "Wireframe" ) );
	hiddenline->setText( tr( "Hidden Line" ) );
	hiddenline->setText( tr( "Hidden Line" ) );
	hiddenline->setToolTip( tr( "Hidden line" ) );
	hiddenline->setStatusTip( tr( "Hidden line" ) );
	polygon->setText( tr( "Polygon Only" ) );
	polygon->setText( tr( "Polygon Only" ) );
	polygon->setToolTip( tr( "Polygon only" ) );
	polygon->setStatusTip( tr( "Polygon only" ) );
	filledmesh->setText( tr( "Mesh & Filled Polygons" ) );
	filledmesh->setText( tr( "Mesh & Filled Polygons" ) );
	filledmesh->setToolTip( tr( "Mesh & filled Polygons" ) );
	filledmesh->setStatusTip( tr( "Mesh & filled Polygons" ) );
	pointstyle->setText( tr( "Dots" ) );
	pointstyle->setText( tr( "Dots" ) );
	pointstyle->setToolTip( tr( "Dots" ) );
	pointstyle->setStatusTip( tr( "Dots" ) );
	barstyle->setText( tr( "Bars" ) );
	barstyle->setText( tr( "Bars" ) );
	barstyle->setToolTip( tr( "Bars" ) );
	barstyle->setStatusTip( tr( "Bars" ) );
	conestyle->setText( tr( "Cones" ) );
	conestyle->setText( tr( "Cones" ) );
	conestyle->setToolTip( tr( "Cones" ) );
	conestyle->setStatusTip( tr( "Cones" ) );
	crossHairStyle->setText( tr( "Crosshairs" ) );
	crossHairStyle->setText( tr( "Crosshairs" ) );
	crossHairStyle->setToolTip( tr( "Crosshairs" ) );
	crossHairStyle->setStatusTip( tr( "Crosshairs" ) );

	//floorstyle->setText( tr( "Floor Style" ) );
	//floorstyle->setText( tr( "Floor Style" ) );
	//floorstyle->setStatusTip( tr( "Floor Style" ) );
	floordata->setText( tr( "Floor Data Projection" ) );
	floordata->setText( tr( "Floor Data Projection" ) );
	floordata->setToolTip( tr( "Floor data projection" ) );
	floordata->setStatusTip( tr( "Floor data projection" ) );
	flooriso->setText( tr( "Floor Isolines" ) );
	flooriso->setText( tr( "Floor Isolines" ) );
	flooriso->setToolTip( tr( "Floor isolines" ) );
	flooriso->setStatusTip( tr( "Floor isolines" ) );
	floornone->setText( tr( "Empty Floor" ) );
	floornone->setText( tr( "Empty Floor" ) );
	floornone->setToolTip( tr( "Empty floor" ) );
	floornone->setStatusTip( tr( "Empty floor" ) );

	actionAnimate->setText( tr( "Animation" ) );
	actionAnimate->setText( tr( "Animation" ) );
	actionAnimate->setToolTip( tr( "Animation" ) );
	actionAnimate->setStatusTip( tr( "Animation" ) );

	actionPerspective->setText( tr( "Enable perspective" ) );
	actionPerspective->setText( tr( "Enable perspective" ) );
	actionPerspective->setToolTip( tr( "Enable perspective" ) );
	actionPerspective->setStatusTip( tr( "Enable perspective" ) );

	actionResetRotation->setText( tr( "Reset rotation" ) );
	actionResetRotation->setText( tr( "Reset rotation" ) );
	actionResetRotation->setToolTip( tr( "Reset rotation" ) );
	actionResetRotation->setStatusTip( tr( "Reset rotation" ) );

	actionFitFrame->setText( tr( "Fit frame to window" ) );
	actionFitFrame->setText( tr( "Fit frame to window" ) );
	actionFitFrame->setToolTip( tr( "Fit frame to window" ) );
	actionFitFrame->setStatusTip( tr( "Fit frame to window" ) );
}

Graph3D * ApplicationWindow::plot3DMatrix(Matrix *m, int style)
{
	if (!m)
    {
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m) return 0;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	Graph3D *plot = newPlot3D();
	if(!plot) return 0;

	plot->addMatrixData(m);
	plot->customPlotStyle(style);
    plot->setDataColorMap(new LinearColorMap(m->colorMap()));
	plot->update();

	custom3DActions(plot);
    
    double xShift = (m->xEnd() - m->xStart()) / (m->numCols() - 1) * 0.5;
    double yShift = (m->yEnd() - m->yStart()) / (m->numRows() - 1) * 0.5;

    plot->setScale(0, m->xStart() - xShift, m->xEnd() + xShift, 5, 5, (Qwt3D::SCALETYPE)0);
    plot->setScale(1, m->yStart() - yShift, m->yEnd() + yShift, 5, 5, (Qwt3D::SCALETYPE)0);

    double min, max;
    m->range(&min, &max);
    min = qMin(0.0, min);

    plot->setScale(2, min, max, 5, 5, (Qwt3D::SCALETYPE)0);
    plot->customPlotStyle(style);
    plot->setMaximized(m);
    updateWindowLists();

    emit modified();
    QApplication::restoreOverrideCursor();
    return plot;
}

Graph3D *ApplicationWindow::plot3DMatrix(const QString &matrixName, int style)
{
    Matrix *m = matrix(matrixName);
    if (!m)
        return nullptr;
    return plot3DMatrix(m, style);
}

Graph3D *ApplicationWindow::newPlot3D(const QString &name, Matrix *m, int style)
{
    if (!m)
        return nullptr;
    Graph3D *plot = findPlot3D(name);
    if (plot)
    {
        plot->addMatrixData(m);
        plot->customPlotStyle(style);
        plot->setDataColorMap(new LinearColorMap(m->colorMap()));
        plot->update();
        emit modified();
        return plot;
    }
    plot = plot3DMatrix(m, style);
    if (plot && !name.isEmpty())
        setWindowName(plot, name);
    return plot;
}

Graph3D *ApplicationWindow::newPlot3D(const QString &name, const QString &matrixName, int style)
{
    Matrix *m = matrix(matrixName);
    if (!m)
        return nullptr;
    return newPlot3D(name, m, style);
}

MultiLayer* ApplicationWindow::plotGrayScale(Matrix *m)
{
	if (!m) {
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m)
			return 0;
	}
    
    MultiLayer* g=plotSpectrogram(m, Graph::GrayScale);
    emit modified();
    maximizeWindow(g);
    setAutoScale();
    g->activeLayer()->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);

    updateWindowLists(g);

    return g;
}

MultiLayer* ApplicationWindow::plotContour(Matrix *m)
{
	if (!m) {
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m)
			return 0;
	}

    MultiLayer* g=plotSpectrogram(m, Graph::Contour);
    emit modified();
    maximizeWindow(g);

    setAutoScale();
    g->activeLayer()->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);

    updateWindowLists(g);

    return g;
}

MultiLayer *ApplicationWindow::plotColorMap(Matrix *m)
{
    if (!m)
    {
        m = (Matrix *)activeWindow(MatrixWindow);
        if (!m)
            return nullptr;
    }

    m->setDefaultColorMap();

    MultiLayer *ml = plotSpectrogram(m, Graph::ColorMap);
    if (!ml)
        return nullptr;


    ml->setMaximized(m);

    setAutoScale();
    autoArrangeLayers();
    ml->activeLayer()->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);

    updateWindowLists(ml);

    return ml;
}

MultiLayer* ApplicationWindow::plotImage(Matrix *m)
{
    if (!m) {
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m)
			return 0;
	}

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
	Graph* plot = g->activeLayer();
	setPreferences(plot);
	Spectrogram *s = plot->plotSpectrogram(m, Graph::GrayScale);
	if (!s)
		return 0;

    s->setAxes(QwtPlot::xTop, QwtPlot::yLeft);
	plot->enableAxis(QwtPlot::xTop, true);
	plot->setScale(QwtPlot::xTop, qMin(m->xStart(), m->xEnd()), qMax(m->xStart(), m->xEnd()));
	plot->setScale(QwtPlot::xBottom, qMin(m->xStart(), m->xEnd()), qMax(m->xStart(), m->xEnd()));
	plot->enableAxis(QwtPlot::xBottom, false);
	plot->enableAxis(QwtPlot::yRight, false);
	plot->setScale(QwtPlot::yLeft, qMin(m->yStart(), m->yEnd()), qMax(m->yStart(), m->yEnd()),
					0.0, 5, 5, Graph::Linear, true);
    plot->setAxisTitle(QwtPlot::yLeft, QString());
    plot->setAxisTitle(QwtPlot::xTop, QString());
    plot->setTitle(QString());

    g->arrangeLayers(false, true);
    emit modified();
    QApplication::restoreOverrideCursor();
    g->activeLayer()->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);
    return g;
}

MultiLayer* ApplicationWindow::plotSpectrogram(Matrix *m, Graph::CurveType type)
{
	if (type == Graph::ImagePlot)
		return plotImage(m);
	else if (type == Graph::Histogram)
		return plotHistogram(m);

	if (!m)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
	Graph* plot = g->activeLayer();
	setPreferences(plot);

	Spectrogram *sp = plot->plotSpectrogram(m, type);
	if (sp && type == Graph::ColorMap )
    {
        sp->setCustomColorMap(new LinearColorMap(m->colorMap()));

        int numCols = sp->matrix()->numCols();
        int numRows = sp->matrix()->numRows();

        double xStart = sp->matrix()->xStart();
        double xEnd = sp->matrix()->xEnd();
        double yStart = sp->matrix()->yStart();
        double yEnd = sp->matrix()->yEnd();

        double xDelta = fabs((xEnd - xStart) / double(numCols - 1) / 2.0);
        double yDelta = fabs((yEnd - yStart) / double(numRows - 1) / 2.0);

        plot->setScale(QwtPlot::xBottom, xStart - xDelta, xEnd + xDelta, 0, 8, 4, 0, false);
        plot->setScale(QwtPlot::yLeft, yStart - yDelta, yEnd + yDelta, 0, 8, 4, 0, false);
        plot->setScale(QwtPlot::xTop, xStart - xDelta, xEnd + xDelta, 0, 8, 4, 0, false);
        
        ScaleDraw *sd = (ScaleDraw *)plot->axisScaleDraw(QwtPlot::yRight);
        sd->enableComponent (QwtAbstractScaleDraw::Backbone, true);
        
        plot->setAxisTicksLength(QwtPlot::xTop, 0, 0, plot->minorTickLength(), plot->majorTickLength());
        plot->setAxisTicksLength(QwtPlot::yRight, 1, 1, plot->minorTickLength(), plot->majorTickLength());
        
        sp->clearLabels();
        plot->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);
    }
	g->arrangeLayers(false, true);
	QApplication::restoreOverrideCursor();
	return g;
}

MultiLayer* ApplicationWindow::plotImageProfiles(Matrix *m)
{
    bool maximizeYN = false;
    if (!m) {
		m = (Matrix*)activeWindow(MatrixWindow);
		if (!m)
			return 0;
        if (m->isMaximized())
            maximizeYN = true;
	}

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    MultiLayer* g = multilayerPlot(generateUniqueName(tr("Profiles")), 0);
    g->resize(650, 600);
    g->plotProfiles(m);

    Table *horTable = newHiddenTable(tr("Horizontal"), QString(), m->numCols(), 2);
    Table *verTable = newHiddenTable(tr("Vertical"), QString(), m->numRows(), 2);

	Graph *sg = g->layer(1);
	if (sg){
		ImageProfilesTool *ipt = new ImageProfilesTool(this, sg, m, horTable, verTable);
		ipt->connectPlotLayers();
		sg->setActiveTool(ipt);
        g->setActiveLayer(sg);
	}

    g->keepAligned(false);

	QApplication::restoreOverrideCursor();

    if (maximizeYN)
        maximizeWindow(g);

    if (g)
        updateWindowLists(g);

    return g;
}

void ApplicationWindow::deleteFitTables()
{
	QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		MultiLayer *ml = qobject_cast<MultiLayer*>(w);
		if (!ml)
			continue;
		QList<Graph *> layers = ml->layersList();
		foreach(Graph *g, layers){
			QList<QwtPlotCurve *> curves = g->fitCurvesList();
			foreach(QwtPlotCurve *c, curves){
				if (((PlotCurve *)c)->type() != Graph::Function){
					Table *t = ((DataCurve *)c)->table();
					if (!t)
						continue;
					t->askOnCloseEvent(false);
					t->close();
				}
			}
		}
	}
}

QList<MdiSubWindow *> ApplicationWindow::windowsList()
{
	QList<MdiSubWindow *> lst;

    Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows)
			lst << w;
		f = f->folderBelow();
	}
	return lst;
}

void ApplicationWindow::updateRecentProjectsList(const QString& fn)
{
	QString nativeFileName = QDir::toNativeSeparators(fn);
	if (!nativeFileName.isEmpty()){
		recentProjects.removeAll(nativeFileName);
		recentProjects.push_front(nativeFileName);
	}

	if (recentProjects.isEmpty())
		return;

	while ((int)recentProjects.size() > MaxRecentProjects)
		recentProjects.pop_back();

	recent->clear();

	for (int i = 0; i<(int)recentProjects.size(); i++ )
		recent->addAction("&" + QString::number(i+1) + " " + QDir::toNativeSeparators(recentProjects[i]));
}

void ApplicationWindow::translateCurveHor()
{
	translateCurve(TranslateCurveTool::Horizontal);
}

void ApplicationWindow::translateCurve(TranslateCurveTool::Direction direction)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g)
        return;

    if (g->isPiePlot())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	} else if (g->validCurvesDataSize()) {
		btnPointer->setChecked(true);
		g->setActiveTool(new TranslateCurveTool(g, this, direction, info, SLOT(setText(const QString&))));
		displayBar->show();
	}
}

void ApplicationWindow::setReadOnlyCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]), actionReadOnlyCol->isChecked());
}

void ApplicationWindow::setReadOnlyColumns()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]));
}

void ApplicationWindow::setReadWriteColumns()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]), false);
}

void ApplicationWindow::setAscValues()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

    t->setAscValues();
}

void ApplicationWindow::setRandomValues()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

    t->setRandomValues();
}

void ApplicationWindow::setNormalRandomValues()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setNormalRandomValues();
}

void ApplicationWindow::setXErrCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::xErr);
}

void ApplicationWindow::setYErrCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::yErr);
}

void ApplicationWindow::setXCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::X);
}

void ApplicationWindow::setYCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::Y);
}

void ApplicationWindow::setZCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::Z);
}

void ApplicationWindow::setLabelCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::Label);
}

void ApplicationWindow::disregardCol()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

	t->setPlotDesignation(Table::None);
}

void ApplicationWindow::fitMultiPeakGauss()
{
	fitMultiPeak((int)MultiPeakFit::Gauss);
}

void ApplicationWindow::fitMultiPeakLorentz()
{
	fitMultiPeak((int)MultiPeakFit::Lorentz);
}

void ApplicationWindow::fitMultiPeak(int profile)
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    if (g->isPiePlot())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("This functionality is not available for pie plots!"));
		return;
	} else {
		bool ok;
		int peaks = QInputDialog::getInt(this, tr("QTISAS - Enter the number of peaks"),
                                             tr("Peaks"), 2, 2, 1000000, 1, &ok);
		if (ok && peaks){
			g->setActiveTool(new MultiPeakFitTool(g, this, (MultiPeakFit::PeakProfile)profile, peaks, info, SLOT(setText(const QString&))));
			displayBar->show();
		}
	}
}

void ApplicationWindow::subtractStraightLine()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    if (plot->isEmpty())
    {
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

	if (g->isPiePlot()){
		QMessageBox::warning(this,tr("QTISAS - Warning"),
				tr("This functionality is not available for pie plots!"));
		return;
	} else {
		g->setActiveTool(new SubtractLineTool(g, this, info, SLOT(setText(const QString&))));
		displayBar->show();
	}
}

void ApplicationWindow::subtractReferenceData()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto sdd = new SubtractDataDialog(this);
    sdd->setGraph(g);
    sdd->exec();
}

void ApplicationWindow::baselineDialog()
{
    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (!g || !g->validCurvesDataSize())
        return;

    auto bd = new BaselineDialog(this);
    bd->setGraph(g);
    bd->show();
}

QAction *ApplicationWindow::openWebPageAction(const QString &text, const QString &url, const QString &icon)
{
    auto *action = new QAction(text);
    setupActionIcon(action, QIcon(icon));
    connect(action, &QAction::triggered, this, [=]() { QDesktopServices::openUrl(QUrl(url)); });
    return action;
}

void ApplicationWindow::parseCommandLineArguments(const QStringList &args)
{
    int num_args = args.count();
    if (num_args == 0)
    {
        if (d_open_last_project && !recentProjects.isEmpty())
        {
            ApplicationWindow *app = open(recentProjects[0]);
            if (app && app != this)
            {
                savedProject();
                close();
            }
            else
                savedProject();
        }
        else
            initWindow();
        return;
    }

    QString str;
    bool exec = false;
    bool noGui = false;
    bool quitAfter = false;
    // -X and -S have no visible window: their startup errors go to stderr instead of a dialog nobody can close.
    const auto isServerOption = [](const QString &arg, bool offscreen) {
        const QString shortName = offscreen ? "-S" : "-s";
        const QString longName = offscreen ? "--server-offscreen" : "--server";
        return arg == shortName || arg.startsWith(shortName + "=") || arg == longName || arg.startsWith(longName + "=");
    };
    const bool noGuiRequested = args.contains("-X") || std::any_of(args.begin(), args.end(), [&](const QString &arg) {
                                    return isServerOption(arg, true);
                                });
    QString serverInstance;
    bool serverOffscreen = false;
    bool default_settings = false;
    bool console = false;
    foreach (str, args)
    {
        if ((str == "-a" || str == "--about") || (str == "-m" || str == "--manual"))
        {
            QMessageBox::critical(
                this, tr("QTISAS - Error"),
                tr("<b> %1 </b>: This command line option must be used without other arguments!").arg(str));
        }
        else if (str == "-c" || str == "--console")
        {
            d_mdi_windows_area = false;
            showScriptWindow(false);
            setScriptingLanguage("Python");
            hide();
            console = true;
        }
        else if ((str == "-d" || str == "--default-settings"))
            default_settings = true;
        else if (str == "-v" || str == "--version")
        {
            QString text = currentVersionInfo(true);
#ifdef Q_OS_WIN
            hide();
            QMessageBox::information(this, tr("QTISAS"), text);
#else
            std::wcout << text.toStdWString();
#endif
            exit(0);
        }
        else if (str == "-h" || str == "--help")
        {
            QString s = "\n" + tr("Usage") + ": ";
            s += "qtisas[-py] [" + tr("options") + "] [" + tr("file") + "_" + tr("name") + "]\n\n";
            s += tr("Valid options are") + ":\n";
            s += "-a " + tr("or") + " --about: " + tr("show about dialog and exit") + "\n";
            s += "-c " + tr("or") + " --console: " + tr("show standalone scripting window") + "\n";
            s += "-d " + tr("or") + " --default-settings: " + tr("start QTISAS with the default settings") + "\n";
            s += "-h " + tr("or") + " --help: " + tr("show command line options") + "\n";
            s += "-v " + tr("or") + " --version: " + tr("print QTISAS version") + "\n";
            s += "-x " + tr("or") + " --execute: " + tr("execute the script file given as argument") + "\n";
            s += "--quit: " + tr("with -x, quit after the script (exit code 1 if it failed)") + "\n";
            s += "-s[=NAME] " + tr("or") + " --server[=NAME]: " +
                 tr("start the script server as instance NAME (default: main); send it code with qtisas_send.py") +
                 "\n";
            s += "-S[=NAME] " + tr("or") + " --server-offscreen[=NAME]: " +
                 tr("the same without any window; dialogs are closed automatically, errors go to stderr") + "\n";
            s += "-X: " +
                 tr("execute the script file given as argument without displying the user interface. Warning: 2D plots "
                    "are not correctly handled in this functioning mode!") +
                 "\n\n";
            s += "'" + tr("file") + "_" + tr("name") + "' " + tr("can be any .qti, qti.gz, .py or ASCII file") + "\n";
#ifdef Q_OS_WIN
            hide();
            QMessageBox::information(this, tr("QTISAS") + " - " + tr("Help"), s);
#else
            std::wcout << s.toStdWString();
#endif
            exit(0);
        }
        else if (str.startsWith("--lang=") || str.startsWith("-l="))
        {
            QString locale = str.mid(str.indexOf('=') + 1);
            if (locales.contains(locale))
                switchToLanguage(locale);
            if (!locales.contains(locale))
                QMessageBox::critical(this, tr("QTISAS - Error"),
                                      tr("<b> %1 </b>: Wrong locale option or no translation available!").arg(locale));
        }
        else if (str == "-x" || str == "--execute")
            exec = true;
        else if (str == "-X")
            noGui = true;
        else if (str == "--quit")
            quitAfter = true;
        else if (isServerOption(str, false) || isServerOption(str, true))
        {
            serverOffscreen = isServerOption(str, true);
            serverInstance = str.contains('=') ? str.mid(str.indexOf('=') + 1) : QString("main");
        }
        else if (str.startsWith("-") && noGuiRequested)
            std::cerr << "QtiSAS: unknown command line option " << str.toStdString() << std::endl;
        else if (str.startsWith("-") || str.startsWith("--"))
        {
            QMessageBox::critical(this, tr("QTISAS - Error"),
                                  tr("<b> %1 </b> unknown command line option!").arg(str) + "\n" +
                                      tr("Type %1 to see the list of the valid options.").arg("'qtisas[-py] -h'"));
        }
    }

#ifdef SCRIPTING_PYTHON
    if (!serverInstance.isEmpty())
        startScriptServer(serverInstance, serverOffscreen);
#endif

    const QString &file_name = args[num_args - 1]; // last argument
    if (file_name.startsWith("-"))
    { // no file name given
        if (console)
            return;

        if (d_open_last_project && !recentProjects.isEmpty())
        {
            ApplicationWindow *app = open(recentProjects[0], default_settings);
            if (app && app != this)
            {
                savedProject();
                close();
            }
        }
        else
            initWindow();
        return;
    }

    savedProject();

    if (!file_name.isEmpty())
    {
        bool directScript = !QFileInfo(file_name).isReadable();
        // A missing script file, not Python code to run: report it instead of executing its name.
        if (directScript && (console || exec || noGui) && file_name.endsWith(".py", Qt::CaseInsensitive))
        {
            if (noGui)
            {
                std::cerr << "QtiSAS: script file not found or not readable: " << file_name.toStdString() << std::endl;
                exit(1);
            }
            QMessageBox::critical(this, tr("QTISAS - Error"),
                                  tr("The script file <b>%1</b> doesn't exist or isn't readable!").arg(file_name));
            initWindow();
            return;
        }
        if (directScript && !(console || exec || noGui))
        {
            this->isFileReadable(file_name);
            return;
        }

        workingDir = QFileInfo(file_name).absolutePath();
        // Batch runs (-X, -x --quit) leave the user's settings alone
        if (!noGui && !quitAfter)
            saveSettings(); // the recent projects must be saved

        if (console)
        {
            directScript ? scriptWindow->newScript(file_name) : scriptWindow->open(file_name);
            if (exec)
                scriptWindow->executeAll();
        }
        else if (exec || noGui)
            loadScript(file_name, exec, noGui, quitAfter);
        else
        {
            ApplicationWindow *app = open(file_name, default_settings);
            if (app && app != this)
            {
                savedProject();
                close();
            }
        }
    }
}

#ifdef SCRIPTING_PYTHON
bool ApplicationWindow::startScriptServer(const QString &instance, bool offscreen)
{
    // Without a window nobody could close a dialog: -S reports to stderr and exits
    const auto fail = [this, offscreen](const QString &message) {
        if (offscreen)
        {
            std::cerr << "QtiSAS: " << message.toStdString() << std::endl;
            exit(1);
        }
        QMessageBox::critical(this, tr("QtiSAS") + " - " + tr("Script server"), message);
        return false;
    };
    static const QRegularExpression nameRe("^[A-Za-z0-9_-]+$");
    if (!nameRe.match(instance).hasMatch())
        return fail(tr("script server: the instance name may use letters, digits, '-' and '_' only: %1").arg(instance));
    if (QString(scriptEnv->objectName()) != "Python")
        return fail(tr("script server: needs Python as the scripting language"));

    // Loaded from the folder shipped with QtiSAS, never from a copy in the user's python folder that may be stale
    const QString folder = bundledPythonFolder() + "/server";
    if (!QFileInfo::exists(folder + "/qtisas_server.py"))
        return fail(tr("script server: %1 not found").arg(folder + "/qtisas_server.py"));
    const auto quoted = [](QString text) { return "'" + text.replace("\\", "\\\\").replace("'", "\\'") + "'"; };
    const QString code = QString("import sys\nsys.path.insert(0, %1)\nimport qtisas_server\n"
                                 "qtisas_server.start(%2, dialogs=%3)\n")
                             .arg(quoted(QDir(folder).absolutePath()), quoted(instance),
                                  offscreen ? QString("'auto'") : QString("'leave'"));
    QString error;
    Script *script = scriptEnv->newScript(code, this, "<qtisas-server>");
    connect(script, &Script::error, this, [&error](const QString &message) { error = message; });
    script->exec();
    delete script;
    if (!error.isEmpty()) // the exception's own line: "RuntimeError: ... already used by QtiSAS pid ..."
        return fail(error.trimmed().section('\n', 0, 0));
    return true;
}
#endif

void ApplicationWindow::switchToLanguage(int param)
{
	if (param < (int)locales.size())
		switchToLanguage(locales[param]);
}

void ApplicationWindow::switchToLanguage(const QString& locale)
{
  
	if (!locales.contains(locale) || appLanguage == locale)
		return;

	appLanguage = locale;
//	if (locale == "en")
	{
		qApp->removeTranslator(appTranslator);
		qApp->removeTranslator(qtTranslator);
		delete appTranslator;
		delete qtTranslator;
		appTranslator = new QTranslator(this);
		qtTranslator = new QTranslator(this);
		qApp->installTranslator(appTranslator);
		qApp->installTranslator(qtTranslator);
	}
/*	
	else
	{
		QString qmPath = d_translations_folder;
		appTranslator->load("qtiplot_" + locale, qmPath);
		qtTranslator->load("qt_" + locale, qmPath+"/qt");
	}
*/
	insertTranslatedStrings();

}

QStringList ApplicationWindow::matrixNames()
{
	QStringList names;
	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if (QString(w->metaObject()->className()) == "Matrix")
				names << w->objectName();
		}
		f = f->folderBelow();
	}
	return names;
}

bool ApplicationWindow::alreadyUsedName(const QString& label)
{
	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if (w->objectName() == label)
				return true;
		}
		f = f->folderBelow();
	}
	return false;
}

bool ApplicationWindow::projectHas2DPlots()
{
	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if (QString(w->metaObject()->className()) == "MultiLayer")
				return true;
		}
		f = f->folderBelow();
	}
	return false;
}

void ApplicationWindow::appendProject()
{
	OpenProjectDialog *open_dialog = new OpenProjectDialog(this, false);
	open_dialog->setDirectory(workingDir);
	open_dialog->setExtensionWidget(0);
	if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
		return;
	workingDir = open_dialog->directory().path();
	appendProject(open_dialog->selectedFiles()[0]);
}

Folder* ApplicationWindow::appendProject(const QString& fn, Folder* parentFolder)
{
	if (fn.isEmpty())
		return 0;

	QFileInfo fi(fn);
	workingDir = fi.absolutePath();

    bool selfContainedSvg = fn.endsWith(".qti.svg", Qt::CaseInsensitive);
    if (fn.endsWith(".qti") || selfContainedSvg)
    {
		QFileInfo f(fn);
		if (!f.exists ()){
			QMessageBox::critical(this, tr("QTISAS - File opening error"), tr("The file: <b>%1</b> doesn't exist!").arg(fn));
			return 0;
		}
	} else {
		QMessageBox::critical(this,tr("QTISAS - File opening error"), tr("The file: <b>%1</b> is not a QtiPlot or Origin project file!").arg(fn));
		return 0;
	}

	d_is_appending_file = true;

	if (fn != projectname)
		updateRecentProjectsList(fn);

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QString fname = fn;
    QString tempExtractedQti; // only set for selfContainedSvg
    if (selfContainedSvg)
    {
        fname = extractEmbeddedProjectFromSvg(fn);
        if (fname.isEmpty())
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, tr("QTISAS - File opening error"),
                                  tr("The file: <b>%1</b> doesn't contain an embedded QtiSAS project.").arg(fn));
            return nullptr;
        }
        tempExtractedQti = fname;
    }
    else if (fn.contains(".qti.gz"))
    { // decompress using zlib
        file_uncompress((char *)fname.toLocal8Bit().constData());
		fname.remove(".gz");
	}

	Folder *cf = current_folder;
	if (parentFolder) changeFolder(parentFolder, true);

	FolderListItem *item = (FolderListItem *)current_folder->folderListItem();
	folders->blockSignals (true);
	blockSignals (true);

	QString baseName = fi.baseName();
	QStringList lst = current_folder->subfolders();
	int n = lst.count(baseName);
	if (n)
    {
        //avoid identical subfolder names
		while (lst.count(baseName + QString::number(n))) n++;
		baseName += QString::number(n);
	}

	Folder *new_folder;
	if (parentFolder)
		new_folder = new Folder(parentFolder, baseName);
	else
		new_folder = new Folder(current_folder, baseName);

	current_folder = new_folder;
	FolderListItem *fli = new FolderListItem(item, current_folder);
	current_folder->setFolderListItem(fli);

	QFile f(fname);
	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    (void)f.open(QIODevice::ReadOnly);

	QString s = t.readLine();
    lst = s.split(REGEXPS::whitespaces, Qt::SkipEmptyParts);
	QString version = lst[1];
    lst = version.split(".", Qt::SkipEmptyParts);
	d_file_version =100*(lst[0]).toInt()+10*(lst[1]).toInt()+(lst[2]).toInt();

    qDebug() << QString::number(d_file_version);
    
	t.readLine();
	
	if (d_file_version < 73) t.readLine();

    
//process tables and matrix information
	while ( !t.atEnd())
    {
        s = t.readLine();
        
        lst.clear();
        if  (s.left(8) == "<folder>")
        {
            lst = s.split("\t");
            Folder *f = new Folder(current_folder, lst[1]);
            f->setBirthDate(lst[2]);
            f->setModificationDate(lst[3]);
            if(lst.count() > 4) if (lst[4] == "current") cf = f;
            
            FolderListItem *fli = new FolderListItem(current_folder->folderListItem(), f);
            fli->setText(0, lst[1]);
            f->setFolderListItem(fli);
            
            current_folder = f;
            
        } else if  (s == "<table>")
        {
            while ( s!="</table>" )
            {
                s=t.readLine();
                lst<<s;
            }
            lst.pop_back();

            QStringList list = lst[0].split("\t");
            if (list[2].toInt()>0) openTable(this,lst);
        }else if  (s == "<matrix>")
        {
            while ( s != "</matrix>" )
            {
                s=t.readLine();
                lst<<s;
            }
            lst.pop_back();
            openMatrix(this, lst);
            
        }else if  (s == "<note>")
        {
            for (int i=0; i<3; i++)
            {
                s = t.readLine();
                lst << s;
            }
            Note* m = openNote(this, lst);
            QStringList cont;
            while ( s != "</note>" )
            {
                s = t.readLine();
                cont << s;
            }
            cont.pop_back();
            m->restore(cont);
        } else if  (s == "</folder>") goToParentFolder();
        
    }
    f.close();

    //process the rest
    (void)f.open(QIODevice::ReadOnly);

    MultiLayer *plot=0;
    while ( !t.atEnd())
    {
        s=t.readLine();
        if  (s.left(8) == "<folder>")
        {
            lst = s.split("\t");
            if (current_folder && lst.size() >= 2)
                current_folder = current_folder->findSubfolder(lst[1]);
        }
        else if  (s == "<multiLayer>")
        {//process multilayers information
            s=t.readLine();
            QStringList graph=s.split("\t");
            QString caption=graph[0];
            plot = multilayerPlot(caption, 0, graph[2].toInt(), graph[1].toInt());
            setListViewDate(caption, graph[3]);
            plot->setBirthDate(graph[3]);
            plot->blockSignals(true);
            
            restoreWindowGeometry(this, plot, t.readLine());
            
            if (d_file_version > 71)
            {
                QStringList lst = t.readLine().split("\t");
                if (lst.size() >= 3){
                    plot->setWindowLabel(lst[1]);
                    plot->setCaptionPolicy((MdiSubWindow::CaptionPolicy)lst[2].toInt());
                }
            }
            
            if (d_file_version > 83)
            {
                QStringList lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                plot->setSpacing(lst[1].toInt(),lst[2].toInt());
                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
                lst = t.readLine().split("\t", Qt::SkipEmptyParts);
                plot->setAlignement(lst[1].toInt(),lst[2].toInt());
            }
            
            int keepAlignedFromFile = 0;
            while ( s != "</multiLayer>" )
            {//open layers
                s = t.readLine();
                if (s.contains("<waterfall>")){
                    QStringList lst = s.trimmed().remove("<waterfall>").remove("</waterfall>").split(",");
                    Graph *ag = plot->activeLayer();
                    if (ag && lst.size() >= 2){
                        ag->setWaterfallOffset(lst[0].toInt(), lst[1].toInt());
                        if (lst.size() >= 3)
                            ag->setWaterfallSideLines(lst[2].toInt());
                    }
                    plot->setWaterfallLayout();
                }
                if (s.left(7) == "<graph>")
                {
                    lst.clear();
                    while ( s != "</graph>" ){
                        s = t.readLine();
                        lst << s;
                    }
                    openGraph(this, plot, lst);
                }
                if (s.contains("<LinkXAxes>"))
                    plot->linkXLayerAxes(s.trimmed().remove("<LinkXAxes>").remove("</LinkXAxes>").toInt());
                else if (s.contains("<AlignPolicy>"))
                    plot->setAlignPolicy((MultiLayer::AlignPolicy)s.trimmed().remove("<AlignPolicy>").remove("</AlignPolicy>").toInt());
                else if (s.contains("<CommonAxes>"))
                    plot->setCommonAxesLayout(s.trimmed().remove("<CommonAxes>").remove("</CommonAxes>").toInt());
                else if (s.contains("<ScaleLayers>"))
                    plot->setScaleLayersOnResize(s.trimmed().remove("<ScaleLayers>").remove("</ScaleLayers>").toInt());
                else if (s.contains("<KeepAligned>"))
                    keepAlignedFromFile = s.trimmed().remove("<KeepAligned>").remove("</KeepAligned>").toInt();
            }
            if (keepAlignedFromFile)
                plot->keepAligned(true);
            if (plot->status() == MdiSubWindow::Minimized)
                plot->showMinimized();
            plot->blockSignals(false);
        } else if (s == "<SurfacePlot>")
        {//process 3D plots information
            lst.clear();
            while ( s!="</SurfacePlot>" )
            {
                s = t.readLine();
                lst<<s;
            }
            Graph3D::restore(this, lst, d_file_version);
        } else if  (s == "</folder>")
            goToParentFolder();
    }
		f.close();
	

	folders->blockSignals (false);
	//change folder to user defined current folder
	changeFolder(cf);
	blockSignals (false);
	renamedTables = QStringList();

    // Clear before the catch-up loop below -- updateMarkersBoundingRect() checks this flag and needs it false.
    d_is_appending_file = false;

    // Same restore-time enrichment-visibility gap as in openProject() -- see
    // the comment there.
    foreach (MdiSubWindow *w, multilayerList())
    {
        auto *ml = qobject_cast<MultiLayer *>(w);
        if (!ml)
            continue;
        foreach (Graph *g, ml->layersList())
        {
            g->updateMarkersBoundingRect();
            foreach (FrameWidget *fw, g->enrichmentsList())
            {
                auto *l = qobject_cast<LegendWidget *>(fw);
                if (l)
                    l->updateSize();
            }
        }
    }

    if (!tempExtractedQti.isEmpty())
        QFile::remove(tempExtractedQti);

	QApplication::restoreOverrideCursor();
	return new_folder;
}

void ApplicationWindow::saveFolder(Folder *folder, const QString& fn, bool compress)
{
	QFile f( fn );
	if (d_backup_files && f.exists())
	{// make byte-copy of current file so that there's always a copy of the data on disk
		while (!f.open(QIODevice::ReadOnly)){
			if (f.isOpen())
				f.close();
            QMessageBox::StandardButton choice =
                QMessageBox::warning(this, tr("QTISAS - File backup error"),
                                     tr("Cannot make a backup copy of <b>%1</b> (to %2)."
                                        "<br>If you ignore this, you run the risk of <b>data loss</b>.")
                                         .arg(projectname, projectname + "~"),
                                     QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Ignore);
			if (choice == QMessageBox::Abort)
				return;
			if (choice == QMessageBox::Ignore)
				break;
		}

		if (f.isOpen()){
			QString bfn = fn + "~";
			QFile::remove(bfn);//remove any existing backup
            QFile::copy(fn, bfn);
			f.close();
		}
	}

	if ( !f.open( QIODevice::WriteOnly ) ){
		QMessageBox::about(this, tr("QTISAS - File save error"), tr("The file: <br><b>%1</b> is opened in read-only mode").arg(fn));
		return;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QList<MdiSubWindow *> lst = folder->windowsList();
	int windows = lst.count();
	int initial_depth = folder->depth();
	Folder *dir = folder->folderBelow();
	while (dir && dir->depth() > initial_depth)
    {
		windows += dir->windowsList().count();
		dir = dir->folderBelow();
	}

	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    t << "QtiPlot 0.9.8 project file structure saved in QtiSAS v.";
    t << QString::number(maj_version) + "." + QString::number(min_version) + "." + QString::number(patch_version);
    t << "\n";
	t << "<scripting-lang>\t" + QString(scriptEnv->objectName()) + "\n";
	t << "<windows>\t" + QString::number(windows) + "\n";
	f.close();

	foreach(MdiSubWindow *w, lst)
		w->save(fn, windowGeometryInfo(w));

	initial_depth = folder->depth();
	dir = folder->folderBelow();
	while (dir && dir->depth() > initial_depth)
    {
		if (!f.isOpen())
            (void)f.open(QIODevice::Append);

		t << "<folder>\t" + QString(dir->objectName()) + "\t" + dir->birthDate() + "\t" + dir->modificationDate();
		if (dir == current_folder)
			t << "\tcurrent\n";
		else
			t << "\n";  // FIXME: Having no 5th string here is not a good idea
		t << "<open>" + QString::number(dir->folderListItem()->isExpanded()) + "</open>\n";
		f.close();

		lst = dir->windowsList();
		foreach(MdiSubWindow *w, lst)
			w->save(fn, windowGeometryInfo(w));

		if (!f.isOpen())
            (void)f.open(QIODevice::Append);

		if (!dir->logInfo().isEmpty() )
			t << "<log>\n" + dir->logInfo() + "</log>\n" ;

		if ( (dir->children()).isEmpty() )
			t << "</folder>\n";

		int depth = dir->depth();
		dir = dir->folderBelow();
		if (dir){
		    int next_dir_depth = dir->depth();
		    if (next_dir_depth < depth){
		        int diff = depth - next_dir_depth;
		        for (int i = 0; i < diff; i++)
                    t << "</folder>\n";
		    }
		} else {
		    int diff = depth - initial_depth - 1;
            for (int i = 0; i < diff; i++)
                t << "</folder>\n";
		}
	}

	if (!f.isOpen())
        (void)f.open(QIODevice::Append);

	t << "<open>" + QString::number(folder->folderListItem()->isExpanded()) + "</open>\n";
	if (!folder->logInfo().isEmpty())
		t << "<log>\n" + folder->logInfo() + "</log>" ;

	f.close();

	if (compress)
        file_compress(fn.toLocal8Bit().data(), QString("wb9").toLocal8Bit().data());

	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::saveAsProject()
{
	saveFolderAsProject(current_folder);
}

void ApplicationWindow::saveFolderAsProject(Folder *f)
{
	bool compress = false;
	QString fn = getSaveProjectName("", &compress, 1);
	if (!fn.isEmpty())
		saveFolder(f, fn, compress);
}

void ApplicationWindow::showFolderPopupMenu(const QPoint &pos, bool fromFolders)
{
    QTreeWidgetItem *it = folders->itemAt(pos);
	if (!it || folders->isRenaming())
		return;

	QMenu cm(this);
	QMenu window(this);
	QMenu viewWindowsMenu(this);

	cm.addAction(actionFindWindow);
	cm.addSeparator();
	cm.addAction(actionAppendProject);
	if (((FolderListItem *)it)->folder()->parent())
		cm.addAction(QIcon(":/filesaveas.png"), tr("Save &As Project..."), this, SLOT(saveAsProject()));
	else
		cm.addAction(actionSaveProjectAs);
	cm.addSeparator();

	if (fromFolders && show_windows_policy != HideAll)
	{
		cm.addAction(tr("&Show All Windows"), this, SLOT(showAllFolderWindows()));
		cm.addAction(tr("&Hide All Windows"), this, SLOT(hideAllFolderWindows()));
		cm.addSeparator();
	}

	if (((FolderListItem *)it)->folder()->parent())
	{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        cm.addAction(QIcon(":/close.png"), tr("&Delete Folder"), this, SLOT(deleteFolder()), Qt::Key_F8);
        cm.addAction(tr("&Rename"), this, SLOT(startRenameFolder()), Qt::Key_F2);
#else
        cm.addAction(QIcon(":/close.png"), tr("&Delete Folder"), Qt::Key_F8, this, SLOT(deleteFolder()));
        cm.addAction(tr("&Rename"), Qt::Key_F2, this, SLOT(startRenameFolder()));
#endif
		cm.addSeparator();
	}

	if (fromFolders)
	{
		window.addAction(actionNewTable);
		window.addAction(actionNewMatrix);
		window.addAction(actionNewNote);
		window.addAction(actionNewGraph);
		window.addAction(actionNewFunctionPlot);
		window.addAction(actionNewSurfacePlot);
        window.setTitle(tr("New &Window"));
		cm.addMenu(&window);
	}

	cm.addAction(actionNewFolder);
	cm.addSeparator();

	QStringList lst;
	lst << tr("&None") << tr("&Windows in Active Folder") << tr("Windows in &Active Folder && Subfolders");

    auto *lstActions = new QActionGroup(this);
    lstActions->setExclusive(false);

    for (int i = 0; i < 3; i++)
    {
        lstActions->addAction(viewWindowsMenu.addAction(lst[i]))->setData(i);
        lstActions->actions()[i]->setCheckable(true);
        if (show_windows_policy == (ShowWindowsPolicy)i)
            lstActions->actions()[i]->setChecked(true);
    }

    connect(lstActions, SIGNAL(triggered(QAction *)), SLOT(setShowWindowsPolicy(QAction *)));

    viewWindowsMenu.setTitle(tr("&View Windows"));
	cm.addMenu(&viewWindowsMenu);
	cm.addSeparator();
	cm.addAction(tr("&Properties..."), this, SLOT(folderProperties()));
	cm.exec(QCursor::pos());
}
void ApplicationWindow::setShowWindowsPolicy(QAction *triggeredAction)
{
    int p = triggeredAction->data().toInt();

	if (show_windows_policy == (ShowWindowsPolicy)p)
		return;

	show_windows_policy = (ShowWindowsPolicy)p;
	if (show_windows_policy == HideAll){
		QList<MdiSubWindow *> windows = windowsList();
		foreach(MdiSubWindow *w, windows){
			hiddenWindows->append(w);
			w->hide();
			setListView(w->objectName(), tr("Hidden"));
		}
	} else
		showAllFolderWindows();
}

void ApplicationWindow::showFindDialogue()
{
	FindDialog *fd = new FindDialog(this);
	fd->exec();
}

void ApplicationWindow::startRenameFolder()
{
	FolderListItem *fi = current_folder->folderListItem();
	if (!fi)
		return;

    disconnect(folders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this,
               SLOT(folderItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
	fi->setFlags(fi->flags() | Qt::ItemIsEditable);
	fi->treeWidget()->editItem(fi, 0);
}

void ApplicationWindow::startRenameFolder(QTreeWidgetItem *item)
{
	if (!item || item == folders->topLevelItem(0))
		return;

	if (item->treeWidget() == lv && item->type() == FolderListItem::itemType) {
        disconnect(folders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this,
                   SLOT(folderItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
		current_folder = ((FolderListItem *)item)->folder();
		FolderListItem *it = current_folder->folderListItem();
		it->setFlags(it->flags() | Qt::ItemIsEditable);
		it->treeWidget()->editItem(it, 0);
	}
    else {
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->treeWidget()->editItem(item, 0);
	}
}

void ApplicationWindow::renameFolder(QTreeWidgetItem *it, int col)
{
    if (!it)
        return;

    if (!it->isSelected())
        return;

    QString newText = it->text(col);
    QString oldText = ((FolderListItem *)it)->folder()->objectName();

    if (oldText == newText)
        return;

	Folder *parent = (Folder *)current_folder->parent();
	if (!parent)//the parent folder is the project folder (it always exists)
		parent = projectFolder();

    if (newText.isEmpty())
    {
        QMessageBox::critical(this, tr("QTISAS - Error"), tr("Please enter a valid name!"));
        folders->blockSignals(true);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        it->setText(0, oldText);
        folders->blockSignals(false);
        return;
    }

	QStringList lst = parent->subfolders();
	lst.removeAll(current_folder->objectName());
    if (lst.contains(newText))
    {
        QMessageBox::critical(this, tr("QTISAS - Error"),
                              tr("Name already exists!") + "\n" + tr("Please choose another name!"));
        folders->blockSignals(true);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        it->setText(0, oldText);
        folders->blockSignals(false);
        return;
    }

    current_folder->setObjectName(newText);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    connect(folders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this,
            SLOT(folderItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
	folders->setCurrentItem(parent->folderListItem());//update the list views
}

void ApplicationWindow::showAllFolderWindows()
{
	QList<MdiSubWindow *> lst = current_folder->windowsList();
	foreach(MdiSubWindow *w, lst){//force show all windows in current folder
		if (w){
            updateWindowLists(w);
			w->restoreWindow();
		}
	}

	if ((current_folder->children()).isEmpty()) return;

	FolderListItem *fi = current_folder->folderListItem();
	FolderListItem *item = (FolderListItem *)fi->child(0);
	int initial_depth = item->childCount();
	while (item && item->childCount() >= initial_depth) {//show/hide windows in all subfolders
		lst = ((Folder *)item->folder())->windowsList();
		foreach(MdiSubWindow *w, lst) {
			if (w && show_windows_policy == SubFolders) {
                updateWindowLists(w);
				w->restoreWindow();
			}
            else
				w->hide();
		}
		item = (FolderListItem *)fi->treeWidget()->itemBelow(item);
	}
}

void ApplicationWindow::hideAllFolderWindows()
{
	QList<MdiSubWindow *> lst = current_folder->windowsList();
	foreach(MdiSubWindow *w, lst) hideWindow(w);

	if ((current_folder->children()).isEmpty()) return;

	if (show_windows_policy == SubFolders)
    {
		FolderListItem *fi = current_folder->folderListItem();
		FolderListItem *item = (FolderListItem *)fi->child(0);
		int initial_depth = item->childCount();
        while (item && item->childCount() >= initial_depth) {
			lst = item->folder()->windowsList();
			foreach(MdiSubWindow *w, lst) hideWindow(w);
			item = (FolderListItem *)fi->treeWidget()->itemBelow(item);
		}
	}
}

void ApplicationWindow::projectProperties()
{
	QString s = QString(current_folder->objectName()) + "\n\n";
	s += "\n\n\n";
	s += tr("Type") + ": " + tr("Project")+"\n\n";
	if (projectname != "untitled")
	{
		s += tr("Path") + ": " + projectname + "\n\n";

		QFileInfo fi(projectname);
		s += tr("Size") + ": " + QString::number(fi.size()) + " " + tr("bytes")+ "\n\n";
	}

	s += tr("Contents") + ": " + QString::number(windowsList().size()) + " " + tr("windows");
	s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
	s += "\n\n\n";

	if (projectname != "untitled")
	{
		QFileInfo fi(projectname);
        s += tr("Created") + ": " + locale().toString(fi.birthTime(), QLocale::ShortFormat) + "\n\n";
        s += tr("Modified") + ": " + locale().toString(fi.lastModified(), QLocale::ShortFormat) + "\n\n";
	}
	else
		s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";

    QMessageBox::information(this, tr("Properties"), s, QMessageBox::Ok);
}

void ApplicationWindow::folderProperties()
{
	if (!current_folder->parent())
	{
		projectProperties();
		return;
	}

	QString s = QString(current_folder->objectName()) + "\n\n";
	s += "\n\n\n";
	s += tr("Type") + ": " + tr("Folder")+"\n\n";
	s += tr("Path") + ": " + current_folder->path() + "\n\n";
	s += tr("Size") + ": " + current_folder->sizeToString() + "\n\n";
	s += tr("Contents") + ": " + QString::number(current_folder->windowsList().count()) + " " + tr("windows");
	s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
	//s += "\n\n\n";
	s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";
	//s += tr("Modified") + ": " + current_folder->modificationDate() + "\n\n";

    QMessageBox::information(this, tr("Properties"), s, QMessageBox::Ok);
}

void ApplicationWindow::addFolder()
{
    if (!explorerWindow->isVisible())
		explorerWindow->show();

	QStringList lst = current_folder->subfolders();
	QString name =  tr("New Folder");
	lst = lst.filter(name);
	if (!lst.isEmpty())
		name += " ("+ QString::number(lst.size()+1)+")";

	Folder *f = new Folder(current_folder, name);
	addFolderListViewItem(f);

	FolderListItem *fi = new FolderListItem(current_folder->folderListItem(), f);
	if (fi) {
		f->setFolderListItem(fi);
        fi->treeWidget()->setCurrentItem(fi, 0);
        fi->setFlags(fi->flags() | Qt::ItemIsEditable);
        fi->treeWidget()->editItem(fi, 0);
	}
}

Folder* ApplicationWindow::addFolder(QString name, Folder* parent)
{
    if(!parent){
		if (current_folder)
			parent = current_folder;
		else
        	parent = projectFolder();
	}

    QStringList lst = parent->subfolders();

    if (lst.contains(name))
    {
        int index = 1;
        while (lst.contains(name + " (" + QString::number(index) + ")"))
            index++;
        name += " (" + QString::number(index) + ")";
    }

    Folder *f = new Folder(parent, name);
    addFolderListViewItem(f);

    FolderListItem *fi = new FolderListItem(parent->folderListItem(), f);
    if (fi)
        f->setFolderListItem(fi);

    return f;
}

bool ApplicationWindow::deleteFolder(Folder *f)
{
    // The project folder holds everything else; deleting it left no folder to return to — confirmed live, an AI
    // script's deleteFolder(rootFolder()) crashed QtiSAS in changeFolder()
    if (!f || f == projectFolder())
        return false;

    if (confirmCloseFolder &&
        QMessageBox::information(this, tr("QTISAS - Delete folder?"),
                                 tr("Delete folder '%1' and all the windows it contains?").arg(f->objectName()),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		return false;
	else {
        // where to go afterwards: the current folder, unless it is f or below f — then f's parent, which survives f
        Folder *parent = current_folder ? current_folder : projectFolder();
        for (QObject *o = parent; o; o = o->parent())
            if (o == f)
            {
                parent = dynamic_cast<Folder *>(f->parent());
                break;
            }

		folders->blockSignals(true);

		FolderListItem *fi = f->folderListItem();
		foreach(MdiSubWindow *w, f->windowsList()){
			w->askOnCloseEvent(false);
            closeWindow(w);
		}

		if (!(f->children()).isEmpty()){
			Folder *subFolder = f->folderBelow();
			int initial_depth = f->depth();
			while (subFolder && subFolder->depth() > initial_depth){
			    foreach(MdiSubWindow *w, subFolder->windowsList()){
					removeWindowFromLists(w);
					subFolder->removeWindow(w);
					delete w;
				}
				delete subFolder->folderListItem();
				delete subFolder;

				subFolder = f->folderBelow();
			}
		}

		delete f;
		delete fi;

		current_folder = parent;
		folders->setCurrentItem(parent->folderListItem());
		changeFolder(parent, true);

		folders->blockSignals(false);
		folders->setFocus();
		return true;
	}
}

void ApplicationWindow::deleteFolder()
{
	Folder *parent = (Folder *)current_folder->parent();
	if (!parent)
		parent = projectFolder();

	folders->blockSignals(true);

	if (deleteFolder(current_folder)){
		current_folder = parent;
		folders->setCurrentItem(parent->folderListItem());
		changeFolder(parent, true);
	}

	folders->blockSignals(false);
	folders->setFocus();
}

void ApplicationWindow::folderItemDoubleClicked(QTreeWidgetItem *it)
{
	if (!it || it->type() != FolderListItem::itemType)
		return;

	FolderListItem *item = ((FolderListItem *)it)->folder()->folderListItem();
	folders->setCurrentItem(item);
}

void ApplicationWindow::folderItemChanged(QTreeWidgetItem *it, QTreeWidgetItem *)
{
	if (!it) return;

	changeFolder (((FolderListItem *)it)->folder());
	folders->setFocus();
}

void ApplicationWindow::hideFolderWindows(Folder *f)
{
	if (!f) return;

	QList<MdiSubWindow *> lst = f->windowsList();
	foreach(MdiSubWindow *w, lst)
		w->hide();

	if ((f->children()).isEmpty())
		return;

	Folder *dir = f->folderBelow();
	int initial_depth = f->depth();
	while (dir && dir->depth() > initial_depth){
		lst = dir->windowsList();
		foreach(MdiSubWindow *w, lst)
			w->hide();

		dir = dir->folderBelow();
	}
}

void ApplicationWindow::goToParentFolder()
{
	if (current_folder &&  current_folder->parent())
		current_folder = (Folder *)current_folder->parent();
	else
		current_folder = projectFolder();
}

bool ApplicationWindow::changeFolder(Folder *newFolder, bool force)
{
    if (!newFolder)
        return false;
    
    if (current_folder == newFolder && !force)
        return false;

    disconnect(d_workspace, &QMdiArea::subWindowActivated, this, &ApplicationWindow::windowActivated);

    desactivateFolders();

    newFolder->folderListItem()->setActive(true);

    folders->setCurrentItem(newFolder->folderListItem());

    Folder *oldFolder = current_folder;
    MdiSubWindow::Status old_active_window_state = MdiSubWindow::Normal;
    MdiSubWindow *old_active_window = oldFolder->activeWindow();
    if (old_active_window)
        old_active_window_state = old_active_window->status();

    MdiSubWindow::Status active_window_state = MdiSubWindow::Normal;
    MdiSubWindow *active_window = newFolder->activeWindow();

    QList<MdiSubWindow *> lst = newFolder->windowsList();
    foreach (MdiSubWindow *w, lst)
        if (w->status() == MdiSubWindow::Maximized)
            active_window = w;
    
    if (active_window)
        active_window_state = active_window->status();

    hideFolderWindows(oldFolder);
    current_folder = newFolder;

    results->setText(current_folder->logInfo());
    lv->clear();

    QObjectList folderLst = newFolder->children();
    if (!folderLst.isEmpty())
    {
        foreach (QObject *f, folderLst)
            addFolderListViewItem(dynamic_cast<Folder *>(f));
    }

    foreach (MdiSubWindow *w, lst)
    {
        if (!hiddenWindows->contains(w) && show_windows_policy != HideAll)
            w->restoreWindow(); // show only windows in the current folder which are not hidden by the user
        else
            w->setStatus(MdiSubWindow::Hidden);

        addListViewItem(w);
    }

    if (!(newFolder->children()).isEmpty())
    {
        Folder *f = newFolder->folderBelow();
        int initial_depth = newFolder->depth();
        while (f && f->depth() > initial_depth) // show/hide windows in subfolders
        {
            foreach (MdiSubWindow *w, f->windowsList())
                if (!hiddenWindows->contains(w))
                {
                    if (show_windows_policy == SubFolders)
                    {
                        if (w->status() == MdiSubWindow::Normal || w->status() == MdiSubWindow::Maximized)
                            w->showNormal();
                        else if (w->status() == MdiSubWindow::Minimized)
                            w->showMinimized();
                    }
                    else
                        w->hide();
                }
            f = f->folderBelow();
        }
    }

    if (active_window)
    {
        d_active_window = active_window;
        d_workspace->setActiveSubWindow(active_window);

        if (active_window_state == MdiSubWindow::Minimized)
            active_window->showMinimized();
        else if (active_window_state == MdiSubWindow::Maximized)
        {
            if (QString(active_window->metaObject()->className()) == "Graph3D")
                ((Graph3D *)active_window)->setIgnoreFonts(true);

            active_window->setMaximized();

            auto ml = qobject_cast<MultiLayer *>(active_window);

            if (QString(active_window->metaObject()->className()) == "Graph3D")
                ((Graph3D *)active_window)->setIgnoreFonts(false);
        }

        auto items = lv->findItems(active_window->objectName(), Qt::MatchExactly | Qt::MatchCaseSensitive);
        QTreeWidgetItem *it = items.isEmpty() ? nullptr : items.first();
        if (it)
        {
            it->setSelected(true);
            lv->scrollToItem(it, QAbstractItemView::EnsureVisible);
        }
    }
    else
        d_active_window = (MdiSubWindow *)d_workspace->activeSubWindow();

    customMenu(d_active_window);
    customToolBars(d_active_window);
    
    if (old_active_window)
    {
        old_active_window->setStatus(old_active_window_state);
        oldFolder->setActiveWindow(old_active_window);
    }

    connect(d_workspace, &QMdiArea::subWindowActivated, this, &ApplicationWindow::windowActivated);

    if (!d_opening_file)
        modifiedProject();

    return true;
}

// the project's subfolder name (made when there is none) becomes the current folder, the explorer and the windows
// shown with it: before, an existing subfolder that was not current only became current_folder, so new windows went
// into it while the explorer still showed (and listed them under) the folder before
bool ApplicationWindow::changeFolder(const QString &name)
{
    if (!projectFolder()->findSubfolder(name))
        addFolder(name, projectFolder());
    changeFolder(projectFolder()->findSubfolder(name), true);
    folders->setFocus();
    return true;
}

void ApplicationWindow::desactivateFolders()
{
	FolderListItem *item = (FolderListItem *)folders->topLevelItem(0);
	while (item) {
		item->setActive(false);
		item = (FolderListItem *)folders->itemBelow(item);
	}
}

void ApplicationWindow::addListViewItem(MdiSubWindow *w)
{
	if (!w) return;

	WindowListItem* it = new WindowListItem(lv, w);

	if (QString(w->metaObject()->className()) == "Matrix")
    {
		it->setIcon(0, QIcon(":/matrix.png"));
		it->setText(1, tr("Matrix"));
	}
	else if (w->inherits("Table")){
		it->setIcon(0, QIcon(":/worksheet.png"));
		it->setText(1, tr("Table"));
	}
	else if (QString(w->metaObject()->className()) == "Note"){
		it->setIcon(0, QIcon(":/note.png"));
		it->setText(1, tr("Note"));
	}
	else if (QString(w->metaObject()->className()) == "MultiLayer"){
		it->setIcon(0, QIcon(":/graph.png"));
		it->setText(1, tr("Graph"));
	}
	else if (QString(w->metaObject()->className()) == "Graph3D"){
		it->setIcon(0, QIcon(":/trajectory.png"));
		it->setText(1, tr("3D Graph"));
	}

	it->setText(0, w->objectName());
    it->setText(2, w->aspect());
	it->setText(3, w->birthDate());
	it->setText(4, w->windowLabel().replace("\n", " "));

    // updateCompleter(w->objectName()); // 2022: to speed up large projects cancel compleater
}

void ApplicationWindow::windowProperties()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MdiSubWindow *w = it->window();
	if (!w)
		return;

    auto *mbox = new QMessageBox(QMessageBox::NoIcon, tr("Properties"), QString(), QMessageBox::Ok, this);

	QString s = QString(w->objectName()) + "\n\n";
	s += "\n\n\n";

	s += tr("Label") + ": " + ((MdiSubWindow *)w)->windowLabel() + "\n\n";

	if (QString(w->metaObject()->className()) == "Matrix"){
		mbox->setIconPixmap(QPixmap(":/matrix.png"));
		s +=  tr("Type") + ": " + tr("Matrix") + "\n\n";
	}else if (w->inherits("Table")){
		mbox->setIconPixmap(QPixmap(":/worksheet.png"));
		s +=  tr("Type") + ": " + tr("Table") + "\n\n";
	}else if (QString(w->metaObject()->className()) == "Note"){
		mbox->setIconPixmap(QPixmap(":/note.png"));
		s +=  tr("Type") + ": " + tr("Note") + "\n\n";
	}else if (QString(w->metaObject()->className()) == "MultiLayer"){
		mbox->setIconPixmap(QPixmap(":/graph.png"));
		s +=  tr("Type") + ": " + tr("Graph") + "\n\n";
	}else if (QString(w->metaObject()->className()) == "Graph3D"){
		mbox->setIconPixmap(QPixmap(":/trajectory.png"));
		s +=  tr("Type") + ": " + tr("3D Graph") + "\n\n";
	}
	s += tr("Path") + ": " + current_folder->path() + "\n\n";
	//s += tr("Size") + ": " + w->sizeToString() + "\n\n";
	s += tr("Created") + ": " + w->birthDate() + "\n\n";
	s += tr("Status") + ": " + it->text(2) + "\n\n";
	mbox->setText(s);
	mbox->show();
}

void ApplicationWindow::addFolderListViewItem(Folder *f)
{
	if (!f)
		return;

	FolderListItem* it = new FolderListItem(lv, f);
	it->setActive(false);
	it->setText(0, f->objectName());
	it->setText(1, tr("Folder"));
	it->setText(3, f->birthDate());
}

void ApplicationWindow::find(const QString& s, bool windowNames, bool labels,
		bool folderNames, bool caseSensitive, bool partialMatch, bool subfolders)
{
	if (windowNames || labels){
		MdiSubWindow *w = current_folder->findWindow(s, windowNames, labels, caseSensitive, partialMatch);
		if (w){
			activateWindow(w);
			return;
		}

		if (subfolders){
			FolderListItem *item = (FolderListItem *)folders->currentItem()->child(0);
			while (item) {
				Folder *f = item->folder();
				MdiSubWindow *w = f->findWindow(s,windowNames,labels,caseSensitive,partialMatch);
				if (w) {
					folders->setCurrentItem(f->folderListItem());
					activateWindow(w);
					return;
				}
				item = (FolderListItem *)folders->itemBelow(item);
			}
		}
	}

	if (folderNames){
		Folder *f = current_folder->findSubfolder(s, caseSensitive, partialMatch);
		if (f){
			folders->setCurrentItem(f->folderListItem());
			return;
		}

		if (subfolders) {
			FolderListItem *item = (FolderListItem *)folders->currentItem()->child(0);
			while (item) {
				Folder *f = item->folder()->findSubfolder(s, caseSensitive, partialMatch);
				if (f) {
					folders->setCurrentItem(f->folderListItem());
					return;
				}
				item = (FolderListItem *)folders->itemBelow(item);
			}
		}
	}

	QMessageBox::warning(this, tr("QTISAS - No match found"),
			tr("Sorry, no match found for string: '%1'").arg(s));
}

void ApplicationWindow::dropFolderItems(QTreeWidgetItem *dest)
{
	if (!dest || draggedItems.isEmpty ())
		return;

	Folder *dest_f = ((FolderListItem *)dest)->folder();

	QTreeWidgetItem *it;
	QStringList subfolders = dest_f->subfolders();

	foreach(it, draggedItems){
		if (it->type() == FolderListItem::itemType){
			Folder *f = ((FolderListItem *)it)->folder();
			FolderListItem *src = f->folderListItem();
			if (dest_f == f){
				QMessageBox::critical(this, "QTISAS - Error", tr("Cannot move an object to itself!"));
				return;
			}

			if (((FolderListItem *)dest)->isChildOf(src)){
				QMessageBox::critical(this,"QTISAS - Error",tr("Cannot move a parent folder into a child folder!"));
				draggedItems.clear();
				folders->setCurrentItem(current_folder->folderListItem());
				return;
			}

			Folder *parent = (Folder *)f->parent();
			if (!parent)
				parent = projectFolder();
			if (dest_f == parent)
				return;

			if (subfolders.contains(f->objectName())){
				QMessageBox::critical(this, tr("QTISAS") +" - " + tr("Skipped moving folder"),
						tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(f->objectName()));
			} else
				moveFolder(src, (FolderListItem *)dest);
		} else {
			if (dest_f == current_folder)
				return;

			hideFolderWindows(current_folder);

			MdiSubWindow *w = ((WindowListItem *)it)->window();
			if (w){
				current_folder->removeWindow(w);
				w->hide();
				dest_f->addWindow(w);
				dest_f->setActiveWindow(w);
				delete it;
			}
		}
	}

	draggedItems.clear();
	current_folder = dest_f;
	folders->setCurrentItem(dest_f->folderListItem());
	changeFolder(dest_f, true);
	folders->setFocus();
}

void ApplicationWindow::moveFolder(FolderListItem *src, FolderListItem *dest)
{
	folders->blockSignals(true);
	if (copyFolder(src->folder(), dest->folder())){
		delete src->folder();
		delete src;
	}
	folders->blockSignals(false);
}

bool ApplicationWindow::copyFolder(Folder *src, Folder *dest)
{
    if (!src || !dest)
        return false;

	if (dest->subfolders().contains(src->objectName())){
		QMessageBox::critical(this, tr("QTISAS") + " - " + tr("Error"),
		tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(src->objectName()));
		return false;
	}

	Folder *dest_f = new Folder(dest, src->objectName());
	dest_f->setBirthDate(src->birthDate());
	dest_f->setModificationDate(src->modificationDate());

	FolderListItem *copy_item = new FolderListItem(dest->folderListItem(), dest_f);
	copy_item->setText(0, src->objectName());
	copy_item->setExpanded(src->folderListItem()->isExpanded());
	dest_f->setFolderListItem(copy_item);

	QList<MdiSubWindow *> lst = QList<MdiSubWindow *>(src->windowsList());
	foreach(MdiSubWindow *w, lst)
		dest_f->addWindow(w);

	if (!(src->children()).isEmpty()){
		int initial_depth = src->depth();
		Folder *parentFolder = dest_f;
        src = src->folderBelow();
		while (src && parentFolder && src->depth() > initial_depth){
			dest_f = new Folder(parentFolder, src->objectName());
			dest_f->setBirthDate(src->birthDate());
			dest_f->setModificationDate(src->modificationDate());

			copy_item = new FolderListItem(parentFolder->folderListItem(), dest_f);
			copy_item->setText(0, src->objectName());
			copy_item->setExpanded(src->folderListItem()->isExpanded());
			dest_f->setFolderListItem(copy_item);

			lst = QList<MdiSubWindow *>(src->windowsList());
			foreach(MdiSubWindow *w, lst)
				dest_f->addWindow(w);

            int depth = src->depth();
			src = src->folderBelow();
			if (src){
				int next_folder_depth = src->depth();
            	if (next_folder_depth > depth)
                	parentFolder = dest_f;
				else if (next_folder_depth < depth && next_folder_depth > initial_depth)
                	parentFolder = (Folder*)parentFolder->parent();
			}
		}
	}
	return true;
}

void ApplicationWindow::toggle3DAnimation(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->animate(on);
}

QString ApplicationWindow::generateUniqueName(const QString& name, bool increment)
{
	int index = 0;
	QStringList lst;
	Folder *f = projectFolder();
    QString sss;
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
            sss=QString(w->objectName());
			
			if (sss.startsWith(name))
            {
                sss=sss.right(sss.length() - name.length());
                if (sss.toInt()>0 || sss=="0")
                {
                    lst << QString(w->objectName());
                    index++;
                }
            }
		}
		f = f->folderBelow();
	}

	QString newName = name;
	if (increment)//force return of a different name
		newName += QString::number(++index);
	else if (index>0)
		newName += QString::number(index);

	while(lst.contains(newName))
		newName = name + QString::number(++index);

	return newName;
}

void ApplicationWindow::clearTable()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

    if (QMessageBox::question(this, tr("QTISAS - Warning"),
                              tr("This will clear the contents of all the data associated with the table."
                                 "Are you sure?"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		return;
	else
		t->clear();
}

void ApplicationWindow::goToRow()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;
	if (!w->inherits("Table") && QString(w->metaObject()->className()) != "Matrix")
		return;

	int rows = 0;
	if (w->inherits("Table"))
		rows = ((Table *)w)->numRows();
	else if (QString(w->metaObject()->className()) == "Matrix")
		rows = ((Matrix *)w)->numRows();

	bool ok;
	int row = QInputDialog::getInt(this, tr("QTISAS - Enter row number"), tr("Row"),
			1, 1, rows, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint );
	if ( !ok )
		return;

	if (w->inherits("Table"))
		((Table *)w)->goToRow(row);
	else if (QString(w->metaObject()->className()) == "Matrix")
		((Matrix *)w)->goToRow(row);
}

void ApplicationWindow::goToColumn()
{
	MdiSubWindow *w = activeWindow();
	if (!w)
		return;
	if (!w->inherits("Table") && QString(w->metaObject()->className()) != "Matrix")
		return;

	int columns = 0;
	if (w->inherits("Table"))
		columns = ((Table *)w)->numCols();
	else if (QString(w->metaObject()->className()) == "Matrix")
		columns = ((Matrix *)w)->numCols();

	bool ok;
	int col = QInputDialog::getInt(this, tr("QTISAS - Enter column number"), tr("Column"),
			1, 1, columns, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint );
	if ( !ok )
		return;

	if (w->inherits("Table"))
		((Table *)w)->goToColumn(col);
	else if (QString(w->metaObject()->className()) == "Matrix")
		((Matrix *)w)->goToColumn(col);
}

void ApplicationWindow::showScriptWindow(bool parent)
{
	if (!scriptWindow){
        Qt::WindowFlags flags = parent ? Qt::Tool : Qt::Window;
        if (d_script_win_on_top)
            flags |= Qt::WindowStaysOnTopHint;
        scriptWindow = new ScriptWindow(scriptEnv, this, flags);
		if (d_completion && d_completer)
            scriptWindow->editor()->setCompleter(d_completer);
        scriptWindow->showLineNumbers(d_note_line_numbers);
        scriptWindow->editor()->setTabStopDistance(d_notes_tab_length);
        scriptWindow->editor()->setCurrentFont(d_notes_font);
        scriptWindow->editor()->document()->setDefaultFont(d_notes_font);
		scriptWindow->resize(d_script_win_rect.size());
		scriptWindow->move(d_script_win_rect.topLeft());
		connect(scriptWindow, SIGNAL(visibilityChanged(bool)), actionShowScriptWindow, SLOT(setChecked(bool)));
	}

	if (!parent)
		scriptWindow->setAttribute(Qt::WA_DeleteOnClose);

	if (!scriptWindow->isVisible()){
		scriptWindow->show();
        if (parent && scriptWindow->windowHandle() && windowHandle())
            scriptWindow->windowHandle()->setTransientParent(windowHandle());
		scriptWindow->setFocus();
	} else
		scriptWindow->hide();
}

/*!
  Turns perspective mode on or off
  */
void ApplicationWindow::togglePerspective(bool on)
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setOrthogonal(!on);
}

/*!
  Resets rotation of 3D plots to default values
  */
void ApplicationWindow::resetRotation()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->setRotation(30, 0, 15);
}

/*!
  Finds best layout for the 3D plot
  */
void ApplicationWindow::fitFrameToLayer()
{
	Graph3D *g = (Graph3D *)activeWindow(Plot3DWindow);
	if (!g)
		return;

	g->findBestLayout();
}

ApplicationWindow::~ApplicationWindow()
{
    d_closing = true;
    disconnect(d_workspace, &QMdiArea::subWindowActivated, this, &ApplicationWindow::windowActivated);

	disableTools();//avoids crash if a plot tol is still active

    QList<MdiSubWindow *> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
	    MultiLayer *ml = qobject_cast<MultiLayer *>(w);
		if (ml && ml->hasSelectedLayers())
			delete ml;
	}

	delete hiddenWindows;

	if (scriptWindow)
		scriptWindow->close();

    if (d_text_editor)
		delete d_text_editor;

	QApplication::clipboard()->clear(QClipboard::Clipboard);
	QApplication::restoreOverrideCursor();

#ifdef Q_OS_MACOS
	((QtiPlotApplication *)QCoreApplication::instance ())->remove(this);
#endif
}

int ApplicationWindow::convertOldToNewColorIndex(int cindex)
{
	if( (cindex == 13) || (cindex == 14) ) // white and light gray
		return cindex + 4;

	if(cindex == 15) // dark gray
		return cindex + 8;

	return cindex;
}

void ApplicationWindow::cascade()
{
    const int xoffset = 13;
    const int yoffset = 20;
    int x = 0;
    int y = 0;
	QList<QMdiSubWindow*> windows = d_workspace->subWindowList(QMdiArea::StackingOrder);
    foreach (QMdiSubWindow *w, windows){
		if (!w->isVisible())
			continue;

        w->activateWindow();
		((MdiSubWindow *)w)->setNormal();
		w->setGeometry(x, y, w->geometry().width(), w->geometry().height());
        w->raise();
        x += xoffset;
        y += yoffset;
    }
    modifiedProject();
}

ApplicationWindow *ApplicationWindow::loadScript(const QString &fn, bool execute, bool noGui, bool quitAfter)
{
#ifdef SCRIPTING_PYTHON
    bool directScript = !QFileInfo(fn).isReadable();
	if (noGui){
		hide();
		setScriptingLanguage("Python");

		ScriptEdit *se = new ScriptEdit(scriptEnv, this);

        if (directScript)
            se->setText(fn);
        else
        {
            se->importASCII(fn);
            se->setScriptString("__file__", QFileInfo(fn).absoluteFilePath());
        }

		se->executeAll();
        // a window the script closed is deleted later: before exit(), which would otherwise crash on it
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // Exit status and error text for batch use: nothing else reports a failure without a GUI.
        if (se->lastRunFailed())
        {
            std::cerr << se->errorMessage().toStdString() << std::endl;
            exit(1);
        }
		exit(0);
	} else {
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		setScriptingLanguage("Python");
		restoreApplicationGeometry();

		showScriptWindow();
        directScript ? scriptWindow->newScript(fn) : scriptWindow->open(fn);
        if (!directScript)
            scriptWindow->editor()->setScriptString("__file__", QFileInfo(fn).absoluteFilePath());

		QApplication::restoreOverrideCursor();

		if (execute){
			scriptWindow->hide();
    		scriptWindow->executeAll();
            const bool failed = scriptWindow->editor()->lastRunFailed();
            // --quit: end with the script's result, for test runners (exit() as -X does: the event loop
            // hasn't started yet, and a normal close could stop at a "save project?" dialog).
            if (quitAfter)
            {
                // a window the script closed is deleted later: before exit(), which would otherwise crash on it
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                if (failed)
                    std::cerr << scriptWindow->editor()->errorMessage().toStdString() << std::endl;
                exit(failed ? 1 : 0);
            }
            if (failed)
				showScriptWindow();
		}
		return this;
	}
#else
    QMessageBox::critical(this, tr("QTISAS") + " - " + tr("Error"),
    tr("QtiSAS was not built with Python scripting support included!"));
#endif
	return 0;
}

bool ApplicationWindow::validFor2DPlot(Table *table, Graph::CurveType type)
{
	if (!table->selectedYColumns().count()){
  		QMessageBox::warning(this, tr("QTISAS - Error"), tr("Please select a Y column to plot!"));
  	    return false;
  	} else if (type != Graph::Box && type != Graph::Histogram && type != Graph::Pie){
		if (table->numCols() < 2) {
			QMessageBox::critical(this, tr("QTISAS - Error"),tr("You need at least two columns for this operation!"));
			return false;
		} else if (table->noXColumn()) {
			QMessageBox::critical(this, tr("QTISAS - Error"), tr("Please set a default X column for this table, first!"));
			return false;
		}
	}
	return true;
}

MultiLayer *ApplicationWindow::generate2DGraph(Graph::CurveType type)
{
    auto *w = activeWindow();
    if (!w)
        return nullptr;

    MultiLayer *ml = nullptr;

    if (w->inherits("Table"))
    {
        auto *table = dynamic_cast<Table *>(w);
        if (!validFor2DPlot(table, type))
            return nullptr;

        const auto sel = table->getSelection();
        ml = multilayerPlot(table, table->drawableColumnSelection(), type, sel.topRow(), sel.bottomRow());
    }
    else if (w->inherits("Matrix"))
    {
        auto *m = dynamic_cast<Matrix *>(w);
        ml = plotHistogram(m);
    }
    else
    {
        return nullptr;
    }

    autoArrangeLayers();
    return ml;
}

bool ApplicationWindow::validFor3DPlot(Table *table)
{
	if (table->numCols()<2){
		QMessageBox::critical(0,tr("QTISAS - Error"),tr("You need at least two columns for this operation!"));
		return false;
	}
	if (table->selectedColumn() < 0 || table->colPlotDesignation(table->selectedColumn()) != Table::Z){
		QMessageBox::critical(0,tr("QTISAS - Error"),tr("Please select a Z column for this operation!"));
		return false;
	}
	if (table->noXColumn()){
		QMessageBox::critical(0,tr("QTISAS - Error"),tr("You need to define a X column first!"));
		return false;
	}
	if (table->noYColumn()){
		QMessageBox::critical(0,tr("QTISAS - Error"),tr("You need to define a Y column first!"));
		return false;
	}
	return true;
}

void ApplicationWindow::hideSelectedWindows()
{
	QList<QTreeWidgetItem *> lst;
    QTreeWidgetItemIterator iter(lv);
	while (*iter) {
        if ((*iter)->isSelected())
            lst.append((*iter));
        iter++;
    }

	folders->blockSignals(true);
	foreach(QTreeWidgetItem *item, lst){
		if (item->type() != FolderListItem::itemType)
			hideWindow(((WindowListItem *)item)->window());
	}
	folders->blockSignals(false);
}

void ApplicationWindow::showSelectedWindows()
{
	QList<QTreeWidgetItem *> lst;
    QTreeWidgetItemIterator iter(lv);
    while (*iter) {
		if ((*iter)->isSelected())
			lst.append((*iter));
        iter++;
	}

	folders->blockSignals(true);
	foreach(QTreeWidgetItem *item, lst){
		if (item->type() != FolderListItem::itemType)
			activateWindow(((WindowListItem *)item)->window());
	}
	folders->blockSignals(false);
}

void ApplicationWindow::swapColumns()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;
	QStringList lst = t->selectedColumns();
	if(lst.count() != 2)
        return;

	t->swapColumns(t->colIndex(lst[0]), t->colIndex(lst[1]));
}

void ApplicationWindow::moveColumnRight()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveColumnBy(1);
}

void ApplicationWindow::moveColumnLeft()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveColumnBy(-1);
}

void ApplicationWindow::moveColumnFirst()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveColumnBy(-t->selectedColumn());
}

void ApplicationWindow::moveColumnLast()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveColumnBy(t->numCols() - t->selectedColumn() - 1);
}

void ApplicationWindow::adjustColumnWidth()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->adjustColumnsWidth();
}

void ApplicationWindow::moveTableRowUp()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveRow();
}

void ApplicationWindow::moveTableRowDown()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
    	t->moveRow(false);
}

void ApplicationWindow::restoreApplicationGeometry()
{
	if (d_app_rect.isNull())
    {
		showMaximized();
	}
    else
    {
		resize(d_app_rect.size());
		move(d_app_rect.topLeft());
		show();
	}
}

void ApplicationWindow::scriptsDirPathChanged(const QString& path)
{
	scriptsDirPath = path;

	QList<MdiSubWindow*> windows = windowsList();
	foreach(MdiSubWindow *w, windows){
		if (QString(w->metaObject()->className()) == "Note")
			((Note*)w)->setDirPath(path);
	}
}

void ApplicationWindow::showToolBarsMenu()
{
	QMenu toolBarsMenu;

	QAction *actionFileTools = new QAction(fileTools->windowTitle(), this);
	actionFileTools->setCheckable(true);
	actionFileTools->setChecked(fileTools->isVisible());
	connect(actionFileTools, SIGNAL(toggled(bool)), fileTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionFileTools);

	QAction *actionEditTools = new QAction(editTools->windowTitle(), this);
	actionEditTools->setCheckable(true);
	actionEditTools->setChecked(editTools->isVisible());
	connect(actionEditTools, SIGNAL(toggled(bool)), editTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionEditTools);

	QAction *actionNoteTools = new QAction(noteTools->windowTitle(), this);
	actionNoteTools->setCheckable(true);
	actionNoteTools->setChecked(noteTools->isVisible());
	connect(actionNoteTools, SIGNAL(toggled(bool)), noteTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionNoteTools);

	QAction *actionTableTools = new QAction(tableTools->windowTitle(), this);
	actionTableTools->setCheckable(true);
	actionTableTools->setChecked(tableTools->isVisible());
	connect(actionTableTools, SIGNAL(toggled(bool)), tableTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionTableTools);

	QAction *actionColumnTools = new QAction(columnTools->windowTitle(), this);
	actionColumnTools->setCheckable(true);
	actionColumnTools->setChecked(columnTools->isVisible());
	connect(actionColumnTools, SIGNAL(toggled(bool)), columnTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionColumnTools);

	QAction *actionPlotTools = new QAction(plotTools->windowTitle(), this);
	actionPlotTools->setCheckable(true);
	actionPlotTools->setChecked(plotTools->isVisible());
	connect(actionPlotTools, SIGNAL(toggled(bool)), plotTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionPlotTools);

	QAction *actionMatrixTools = new QAction(plotMatrixBar->windowTitle(), this);
	actionMatrixTools->setCheckable(true);
	actionMatrixTools->setChecked(plotMatrixBar->isVisible());
	connect(actionMatrixTools, SIGNAL(toggled(bool)), plotMatrixBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionMatrixTools);

	QAction *actionPlot3DTools = new QAction(plot3DTools->windowTitle(), this);
	actionPlot3DTools->setCheckable(true);
	actionPlot3DTools->setChecked(plot3DTools->isVisible());
	connect(actionPlot3DTools, SIGNAL(toggled(bool)), plot3DTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionPlot3DTools);

	QAction *actionDisplayBar = new QAction(displayBar->windowTitle(), this);
	actionDisplayBar->setCheckable(true);
	actionDisplayBar->setChecked(displayBar->isVisible());
	connect(actionDisplayBar, SIGNAL(toggled(bool)), displayBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionDisplayBar);
    
	QAction *actionQtisasToolBar = new QAction(qtisasToolBar->windowTitle(), this);
	actionQtisasToolBar->setCheckable(true);
	actionQtisasToolBar->setChecked(qtisasToolBar->isVisible());
	connect(actionQtisasToolBar, SIGNAL(toggled(bool)), qtisasToolBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionQtisasToolBar);
    
	QAction *actionFormatToolBar = new QAction(formatToolBar->windowTitle(), this);
	actionFormatToolBar->setCheckable(true);
	actionFormatToolBar->setChecked(formatToolBar->isVisible());
	connect(actionFormatToolBar, SIGNAL(toggled(bool)), formatToolBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionFormatToolBar);

	QAction *action = toolBarsMenu.exec(QCursor::pos());
	if (!action)
		return;

	MdiSubWindow *w = activeWindow();

	if (action->text() == plotMatrixBar->windowTitle()){
		d_matrix_tool_bar = action->isChecked();
		plotMatrixBar->setEnabled(w && QString(w->metaObject()->className()) == "Matrix");
	} else if (action->text() == tableTools->windowTitle()){
		d_table_tool_bar = action->isChecked();
		tableTools->setEnabled(w && w->inherits("Table"));
	} else if (action->text() == columnTools->windowTitle()){
		d_column_tool_bar = action->isChecked();
		columnTools->setEnabled(w && w->inherits("Table"));
	} else if (action->text() == plotTools->windowTitle()){
		d_plot_tool_bar = action->isChecked();
		plotTools->setEnabled(w && QString(w->metaObject()->className()) == "MultiLayer");
	} else if (action->text() == plot3DTools->windowTitle()){
		d_plot3D_tool_bar = action->isChecked();
		plot3DTools->setEnabled(w && QString(w->metaObject()->className()) == "Graph3D");
	} else if (action->text() == fileTools->windowTitle()){
		d_file_tool_bar = action->isChecked();
	} else if (action->text() == editTools->windowTitle()){
		d_edit_tool_bar = action->isChecked();
	} else if (action->text() == displayBar->windowTitle()){
		d_display_tool_bar = action->isChecked();
	} else if (action->text() == formatToolBar->windowTitle()){
		d_format_tool_bar = action->isChecked();
	} else if (action->text() == noteTools->windowTitle()){
		d_notes_tool_bar = action->isChecked();
	}
}

void ApplicationWindow::saveFitFunctions(const QStringList& lst)
{
	if (!lst.count())
		return;

    QString explain = tr("Starting with version 0.9.1 QtiPlot stores the user defined fit models to a different location.");
    explain += " " + tr("If you want to save your already defined models, please choose a destination folder.");
    if (QMessageBox::Ok != QMessageBox::information(this, tr("QTISAS") + " - " + tr("Import fit models"), explain,
                            QMessageBox::Ok, QMessageBox::Cancel)) return;

	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to export the fit models to"), fitModelsPath, QFileDialog::ShowDirsOnly);
	if (!dir.isEmpty()){
	    fitModelsPath = dir;

        for (int i = 0; i<lst.count(); i++){
            QString s = lst[i].simplified();
            if (!s.isEmpty()){
                NonLinearFit *fit = new NonLinearFit(this, (Graph*)0);

                QStringList l = s.split("=");
                if (l.count() == 2)
                    fit->setFormula(l[1]);

                fit->save(fitModelsPath + "/" + fit->objectName() + ".fit");
            }
        }
	}
}

void ApplicationWindow::matrixDirectFFT()
{
    Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	if (!m->isEmpty())
		m->fft();
	else
		showNoDataMessage();
}

void ApplicationWindow::matrixInverseFFT()
{
    Matrix* m = (Matrix*)activeWindow(MatrixWindow);
	if (!m)
		return;

	if (!m->isEmpty())
		m->fft(true);
	else
		showNoDataMessage();
}

void ApplicationWindow::setFormatBarColor(const QColor& color)
{
	formatToolBar->setEnabled(true);

	ColorButton *cb = (ColorButton *)formatToolBar->widgetForAction(actionTextColor);
	cb->blockSignals(true);
	cb->setColor(color);
	cb->blockSignals(false);
}

void ApplicationWindow::setFormatBarFont(const QFont& font)
{
	formatToolBar->setEnabled(true);

	QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	fb->blockSignals(true);
	fb->setCurrentFont(font);
	fb->blockSignals(false);
	fb->setEnabled(true);

	QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	sb->blockSignals(true);
	sb->setValue(font.pointSize());
	sb->blockSignals(false);
	sb->setEnabled(true);

    actionFontBold->blockSignals(true);
	actionFontBold->setChecked(font.bold());
	actionFontBold->blockSignals(false);

	actionFontItalic->blockSignals(true);
	actionFontItalic->setChecked(font.italic());
    actionFontItalic->blockSignals(false);

    actionSubscript->setEnabled(false);
    actionSuperscript->setEnabled(false);
    actionUnderline->setEnabled(false);
    actionGreekSymbol->setEnabled(false);
    actionGreekMajSymbol->setEnabled(false);
    actionMathSymbol->setEnabled(false);
    actionUnicodeSymbol->setEnabled(false);
}

void ApplicationWindow::setTextColor()
{
    auto cb = (ColorButton *)formatToolBar->widgetForAction(actionTextColor);

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (!plot)
        return;

    auto g = (Graph *)plot->activeLayer();
    if (g)
        g->setCurrentColor(cb->color());
}

void ApplicationWindow::setFontSize(int size)
{
    auto fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	QFont f(fb->currentFont().family(), size);
	f.setBold(actionFontBold->isChecked());
	f.setItalic(actionFontItalic->isChecked());

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (plot)
    {
        auto g = (Graph *)plot->activeLayer();
        if (g)
            g->setCurrentFont(f);
    }

	Note *n = (Note *)activeWindow(NoteWindow);
	if (n){
	    d_notes_font = f;
        QList<MdiSubWindow *> windows = windowsList();
        foreach(MdiSubWindow *w, windows){
            Note *m = qobject_cast<Note *>(w);
            if (m)
                m->setFont(f);
        }
	}
}

void ApplicationWindow::setFontFamily(const QFont& font)
{
    auto sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
    QFont f(font.family(), sb->value());
    f.setBold(actionFontBold->isChecked());
    f.setItalic(actionFontItalic->isChecked());

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (plot)
    {
        auto g = (Graph *)plot->activeLayer();
        if (g)
            g->setCurrentFont(f);
    }

	Note *n = (Note *)activeWindow(NoteWindow);
	if (n){
	    d_notes_font = f;
        QList<MdiSubWindow *> windows = windowsList();
        foreach(MdiSubWindow *w, windows){
            Note *m = qobject_cast<Note *>(w);
            if (m)
                m->setFont(f);
        }
	}
}

void ApplicationWindow::setItalicFont(bool italic)
{
    auto fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
    auto sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	QFont f(fb->currentFont().family(), sb->value());
	f.setBold(actionFontBold->isChecked());
	f.setItalic(italic);

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (plot)
    {
        auto g = (Graph *)plot->activeLayer();
        if (g)
            g->setCurrentFont(f);
    }

    Note *n = (Note *)activeWindow(NoteWindow);
	if (n){
	    d_notes_font = f;
        QList<MdiSubWindow *> windows = windowsList();
        foreach(MdiSubWindow *w, windows){
            Note *m = qobject_cast<Note *>(w);
            if (m)
                m->setFont(f);
        }
	}
}

void ApplicationWindow::setBoldFont(bool bold)
{
    auto fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
    auto sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	QFont f(fb->currentFont().family(), sb->value());
	f.setBold(bold);
	f.setItalic(actionFontItalic->isChecked());

    auto plot = dynamic_cast<MultiLayer *>(activeWindow());
    if (plot)
    {
        auto g = (Graph *)plot->activeLayer();
        if (g)
            g->setCurrentFont(f);
    }

    Note *n = (Note *)activeWindow(NoteWindow);
	if (n){
	    d_notes_font = f;
        QList<MdiSubWindow *> windows = windowsList();
        foreach(MdiSubWindow *w, windows){
            Note *m = qobject_cast<Note *>(w);
            if (m)
                m->setFont(f);
        }
	}
}

void ApplicationWindow::enableTextEditor(Graph *g)
{
	if (!g){
        formatToolBar->setEnabled(false);
	    if (d_text_editor){
            d_text_editor->close();
            d_text_editor = nullptr;
	    }
	} else if (g) {
	    if (!g->activeText() && !g->selectedScale() && !g->titleSelected())
            return;

        d_text_editor = new TextEditor(g);

        formatToolBar->setEnabled(true);
        actionSubscript->setEnabled(true);
        actionSuperscript->setEnabled(true);
        actionUnderline->setEnabled(true);
        actionGreekSymbol->setEnabled(true);
        actionGreekMajSymbol->setEnabled(true);
        actionMathSymbol->setEnabled(true);
        actionUnicodeSymbol->setEnabled(true);
	}
}

void ApplicationWindow::insertSuperscript()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<sup>","</sup>");
}

void ApplicationWindow::insertSubscript()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<sub>","</sub>");
}

void ApplicationWindow::underline()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<u>","</u>");
}

void ApplicationWindow::insertGreekSymbol()
{
    if (!d_text_editor)
        return;

    SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::lowerGreek, this);
	connect(greekLetters, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
	greekLetters->exec();
}

void ApplicationWindow::insertGreekMajSymbol()
{
    if (!d_text_editor)
        return;

    SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::upperGreek, this);
	connect(greekLetters, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
	greekLetters->exec();
}

void ApplicationWindow::insertMathSymbol()
{
    if (!d_text_editor)
        return;

    SymbolDialog *ms = new SymbolDialog(SymbolDialog::mathSymbols, this);
	connect(ms, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
	ms->exec();
}

void ApplicationWindow::insertUnicodeSymbol()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Generation of a symbol from its Unicode"),
                                         tr("Please insert Unicode:"), QLineEdit::Normal,
                                         "", &ok);
    if (!ok && text.isEmpty()) return;

    char *str = (char *)malloc(10);
    QByteArray ba = text.toLocal8Bit();
    strcpy(str, ba.data());
    auto s = new char[text.length()];
    strcpy(s, str);
    int num = (int)strtol(s, nullptr, 16);
    delete[] s;
    
    d_text_editor->addSymbol(QChar(num));
}

void ApplicationWindow::showCustomActionDialog()
{
    CustomActionDialog *ad = new CustomActionDialog(this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	ad->show();
	ad->setFocus();
}

void ApplicationWindow::addCustomAction(QAction *action, const QString& parentName, int index)
{
    if (!action)
        return;

	QList<QToolBar *> toolBars = toolBarsList();
    foreach (QToolBar *t, toolBars){
        if (t->objectName() == parentName){
			if (action->icon().isNull()){
				QPixmap icon = QPixmap(16, 16);
				QRect r = QRect(0, 0, 15, 15);
				icon.fill (Qt::white);
				QPainter p;
				p.begin(&icon);
				p.setPen(QPen(Qt::red, 2));
				p.drawLine(0, 0, 15, 15);
				p.drawLine(0, 15, 15, 0);
				p.setPen(QPen(Qt::black));
				p.drawRect(r);
				action->setIcon(QIcon(icon));
				action->setIconText(tr("No Icon"));
			}
            t->addAction(action);
			if (index < 0)
            	d_user_actions << action;
			else if (index >= 0 && index < d_user_actions.size())
				d_user_actions.replace(index, action);
            return;
        }
    }

    QList<QMenu *> menus = customizableMenusList();
    menus += d_user_menus;
    foreach (QMenu *m, menus){
        if (m->objectName() == parentName){
			if (action->icon().isNull())
				action->setIconText(tr("No Icon"));
            m->addAction(action);
			if (index < 0)
            	d_user_actions << action;
			else if (index >= 0 && index < d_user_actions.size())
				d_user_actions.replace(index, action);
            return;
        }
    }
}

void ApplicationWindow::reloadCustomActions()
{
	reloadCustomMenus();

    QList<QMenu *> menus = customizableMenusList();
	foreach(QAction *a, d_user_actions){
		if (!a->statusTip().isEmpty()){
    		foreach (QMenu *m, menus){
        		if (m->objectName() == a->statusTip()){
        		    QList<QAction *> lst = m->actions();
        		    if (!lst.contains(a))
                        m->addAction(a);

					break;
        		}
        	}
		}
	}
}

void ApplicationWindow::removeAllCustomActions()
{
    foreach (QAction *a, d_user_actions)
    {
        d_user_actions.removeAll(a);
        delete a;
    }
}

void ApplicationWindow::removeCustomAction(QAction *action)
{
    int index = d_user_actions.indexOf(action);
    if (index >= 0 && index < d_user_actions.count()){
        d_user_actions.removeAt(index);
        delete action;
    }
}

static QString normalizedCustomActionPath(const QString &path);
static QString managedCustomActionPath(const QString &base, const QString &subfolder, const QString &entry);

void ApplicationWindow::removeCustomActionFiles(QAction *action) const
{
    if (!action)
        return;

    const QString descriptorPath = normalizedCustomActionPath(customActionsDirPath + "/" + action->text() + ".qca");
    const QString scriptPath = managedCustomActionPath(customActionsDirPath, "scripts", action->data().toString());
    const QString iconPath = managedCustomActionPath(customActionsDirPath, "icons", action->iconText());
    QFile::remove(descriptorPath);

    const QDir folder(customActionsDirPath);
    const auto isReferencedByAnotherAction = [&](const QString &path, const QString &element) {
        if (path.isEmpty())
            return true;

        for (const QString &name : folder.entryList({"*.qca"}, QDir::Files | QDir::NoSymLinks, QDir::Name))
        {
            const QString candidate = normalizedCustomActionPath(folder.filePath(name));
            if (candidate == descriptorPath)
                continue;

            XMLParser parser(candidate);
            if (!parser.errorString().isEmpty() || !parser.hasElement("action") || !parser.hasElement(element))
                continue;
            const QString subfolder = element == "file" ? "scripts" : "icons";
            if (managedCustomActionPath(customActionsDirPath, subfolder, parser.readElement(element).last()) == path)
                return true;
        }
        return false;
    };

    if (!scriptPath.isEmpty() && !isReferencedByAnotherAction(scriptPath, "file"))
        QFile::remove(scriptPath);
    if (!iconPath.isEmpty() && !isReferencedByAnotherAction(iconPath, "icon"))
        QFile::remove(iconPath);
}

// Write (or overwrite) the .qca descriptor from the action's current state.
// scriptRelPath and iconEntry are already relative to customActionsDirPath.
static void writeCustomActionQca(const QString &base, QAction *a, const QString &scriptRelPath,
                                 const QString &iconEntry)
{
    QFile f(base + "/" + a->text() + ".qca");
    if (!f.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE action>\n"
        << "<action version=\"1.0\">\n"
        << "<text>" + a->text() + "</text>\n"
        << "<file>" + scriptRelPath + "</file>\n"
        << "<icon>" + iconEntry + "</icon>\n"
        << "<tooltip>" + a->toolTip() + "</tooltip>\n"
        << "<shortcut>" + a->shortcut().toString() + "</shortcut>\n"
        << "<location>" + a->statusTip() + "</location>\n"
        << "</action>\n";
}

// Extract the current icon entry (relative path or "No Icon") from an action.
static QString actionIconEntry(const QString &base, QAction *a)
{
    const QString t = a->iconText();
    return t.contains("No Icon") ? t : QDir(base).relativeFilePath(t);
}

static QString normalizedCustomActionPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return QDir::cleanPath(canonical.isEmpty() ? info.absoluteFilePath() : canonical);
}

static QString managedCustomActionPath(const QString &base, const QString &subfolder, const QString &entry)
{
    if (entry.isEmpty() || entry.contains("No Icon"))
        return {};

    QString path = normalizedCustomActionPath(QDir::isAbsolutePath(entry) ? entry : QDir(base).absoluteFilePath(entry));
    const QString relative = QDir(normalizedCustomActionPath(base)).relativeFilePath(path);
    const QString prefix = subfolder + "/";
    if (!relative.startsWith(prefix) && !relative.startsWith(subfolder + "\\"))
        return {};
    return path;
}

bool ApplicationWindow::addCustomAction(const QString &name, const QString &scriptName, const QString &menuName,
                                        const QString &iconName, const QString &shortcut, const QString &tooltip)
{
    if (name.isEmpty() || scriptName.isEmpty())
        return false;

    QDir().mkpath(customActionsDirPath + "/scripts");
    QDir().mkpath(customActionsDirPath + "/icons");

    const QString scriptPath = customActionsDirPath + "/scripts/" + scriptName;
    const QString iconPath = iconName.isEmpty() ? QString() : customActionsDirPath + "/icons/" + iconName;

    auto *action = new QAction(this);
    action->setText(name);
    action->setData(scriptPath);
    action->setToolTip(tooltip.isEmpty() ? name : tooltip);
    if (!shortcut.isEmpty())
        action->setShortcut(QKeySequence(shortcut));

    if (!iconPath.isEmpty() && QFile::exists(iconPath))
    {
        action->setIcon(QIcon(iconPath));
        action->setIconText(iconPath);
    }
    else
    {
        action->setIconText(tr("No Icon"));
    }

    // Resolve parent menu: try objectName, then visible title, then fall back to scriptingMenu
    auto resolveParent = [&](const QString &hint) -> QString {
        if (hint.isEmpty())
            return scriptingMenu->objectName();
        const QList<QMenu *> menus = customizableMenusList() + d_user_menus;
        for (QMenu *m : menus)
            if (m->objectName() == hint)
                return hint;
        for (QMenu *m : menus)
            if (m->title().remove("&") == hint)
                return m->objectName();
        return scriptingMenu->objectName();
    };
    const QString parentName = resolveParent(menuName);
    action->setStatusTip(parentName);
    addCustomAction(action, parentName);

    const QString iconEntry = (!iconPath.isEmpty() && QFile::exists(iconPath)) ? "icons/" + iconName : tr("No Icon");
    writeCustomActionQca(customActionsDirPath, action, "scripts/" + scriptName, iconEntry);
    return true;
}

bool ApplicationWindow::updateCustomActionScript(const QString &name, const QString &newScriptName)
{
    for (QAction *a : d_user_actions)
    {
        if (a->text() != name)
            continue;
        QDir().mkpath(customActionsDirPath + "/scripts");
        a->setData(customActionsDirPath + "/scripts/" + newScriptName);
        writeCustomActionQca(customActionsDirPath, a, "scripts/" + newScriptName,
                             actionIconEntry(customActionsDirPath, a));
        return true;
    }
    return false;
}

bool ApplicationWindow::updateCustomActionIcon(const QString &name, const QString &newIconName)
{
    for (QAction *a : d_user_actions)
    {
        if (a->text() != name)
            continue;
        QDir().mkpath(customActionsDirPath + "/icons");
        const QString iconPath = newIconName.isEmpty() ? QString() : customActionsDirPath + "/icons/" + newIconName;
        if (!iconPath.isEmpty() && QFile::exists(iconPath))
        {
            a->setIcon(QIcon(iconPath));
            a->setIconText(iconPath);
        }
        else
        {
            a->setIcon(QIcon());
            a->setIconText(tr("No Icon"));
        }
        const QString iconEntry =
            (!iconPath.isEmpty() && QFile::exists(iconPath)) ? "icons/" + newIconName : tr("No Icon");
        const QString relScript = QDir(customActionsDirPath).relativeFilePath(a->data().toString());
        writeCustomActionQca(customActionsDirPath, a, relScript, iconEntry);
        return true;
    }
    return false;
}

bool ApplicationWindow::updateCustomActionMenu(const QString &name, const QString &newMenuName)
{
    for (QAction *a : d_user_actions)
    {
        if (a->text() != name)
            continue;

        // Remove from every menu and toolbar that currently holds it
        for (QMenu *m : customizableMenusList() + d_user_menus)
            m->removeAction(a);
        for (QToolBar *tb : toolBarsList())
            tb->removeAction(a);

        // Resolve new parent and add to it (without touching d_user_actions)
        auto resolveParent = [&](const QString &hint) -> QString {
            if (hint.isEmpty())
                return scriptingMenu->objectName();
            const QList<QMenu *> menus = customizableMenusList() + d_user_menus;
            for (QMenu *m : menus)
                if (m->objectName() == hint)
                    return hint;
            for (QMenu *m : menus)
                if (m->title().remove("&") == hint)
                    return m->objectName();
            return scriptingMenu->objectName();
        };
        const QString newParent = resolveParent(newMenuName);
        a->setStatusTip(newParent);

        for (QMenu *m : customizableMenusList() + d_user_menus)
            if (m->objectName() == newParent)
            {
                m->addAction(a);
                break;
            }
        for (QToolBar *tb : toolBarsList())
            if (tb->objectName() == newParent)
            {
                tb->addAction(a);
                break;
            }

        const QString relScript = QDir(customActionsDirPath).relativeFilePath(a->data().toString());
        writeCustomActionQca(customActionsDirPath, a, relScript, actionIconEntry(customActionsDirPath, a));
        return true;
    }
    return false;
}


bool ApplicationWindow::updateCustomActionName(const QString &name, const QString &newName)
{
    if (newName.isEmpty())
        return false;
    for (QAction *a : d_user_actions)
    {
        if (a->text() != name)
            continue;
        QFile::remove(customActionsDirPath + "/" + name + ".qca");
        a->setText(newName);
        const QString relScript = QDir(customActionsDirPath).relativeFilePath(a->data().toString());
        writeCustomActionQca(customActionsDirPath, a, relScript, actionIconEntry(customActionsDirPath, a));
        return true;
    }
    return false;
}

bool ApplicationWindow::removeCustomActionByName(const QString &name)
{
    const auto it =
        std::find_if(d_user_actions.begin(), d_user_actions.end(), [&](QAction *a) { return a->text() == name; });
    if (it == d_user_actions.end())
        return false;
    removeCustomActionFiles(*it);
    removeCustomAction(*it);
    return true;
}

bool ApplicationWindow::runCustomActionByName(const QString &name)
{
    const auto it =
        std::find_if(d_user_actions.begin(), d_user_actions.end(), [&](QAction *a) { return a->text() == name; });
    if (it == d_user_actions.end())
        return false;
    performCustomAction(*it);
    return true;
}

QStringList ApplicationWindow::customActionNames() const
{
    QStringList names;
    for (const QAction *a : d_user_actions)
        names << a->text();
    return names;
}


void ApplicationWindow::performCustomAction(QAction *action)
{
	if (!action || !d_user_actions.contains(action))
		return;

#ifdef SCRIPTING_PYTHON
	QString lang = scriptEnv->objectName();
	setScriptingLanguage("Python");

    ScriptEdit *script = new ScriptEdit(scriptEnv, 0);
    script->importASCII(action->data().toString());
    script->executeAll();
    delete script;

    setScriptingLanguage(lang);// reset old scripting language
#else
    QMessageBox::critical(this, tr("QTISAS") + " - " + tr("Error"),
    tr("QtiSAS was not built with Python scripting support included!"));
#endif
}

void ApplicationWindow::loadCustomActions()
{
    QString path = customActionsDirPath + "/";
    QDir dir(path);
    QStringList lst = dir.entryList(QDir::Files | QDir::NoSymLinks, QDir::Name);

    for (int i = 0; i < lst.count(); i++)
    {
        QString fileName = path + lst[i];
        if (fileName.split(".").last() != "qcm")
            continue; // @TODO this needs some log output
        XMLParser parser(fileName);
        if (!parser.errorString().isEmpty())
            continue; // @TODO this needs some log output
        if (!parser.hasElement("menu"))
            continue; // @TODO this needs some log output
        if (parser.hasElement("title") && parser.hasElement("location"))
            addCustomMenu(parser.readElement("title").last(), parser.readElement("location").last());
    }

    for (int i = 0; i < lst.count(); i++)
    {
        QString fileName = path + lst[i];
        if (fileName.split(".").last() != "qca")
            continue; // @TODO this needs some log output
        XMLParser parser(fileName);
        if (!parser.errorString().isEmpty())
            continue; // @TODO this needs some log output
        if (!parser.hasElement("action"))
            continue; // @TODO this needs some log output
        auto *action = new QAction(this);
        if (!parser.readElement("text").isEmpty() && !parser.readElement("file").isEmpty() &&
            !parser.readElement("icon").isEmpty() && !parser.readElement("tooltip").isEmpty() &&
            parser.hasElement("shortcut") && !parser.readElement("location").isEmpty())
        {
            action->setText(parser.readElement("text").last());

            QString file = parser.readElement("file").last();
            file = QFile(file).exists() ? file : customActionsDirPath + '/' + file;
            action->setData(file);

            QString icon = parser.readElement("icon").last();
            icon = QFile(icon).exists() || icon.contains("No Icon") ? icon : customActionsDirPath + '/' + icon;
            action->setIcon(QIcon(icon));

            action->setIconText(parser.readElement("icon").last());
            action->setToolTip(parser.readElement("tooltip").last());
            if (!parser.readElement("shortcut").isEmpty())
                action->setShortcut(parser.readElement("shortcut").last());
            action->setStatusTip(parser.readElement("location").last());
            addCustomAction(action, parser.readElement("location").last());
        }
    }
}

QList<QMenu *> ApplicationWindow::customizableMenusList()
{
	QList<QMenu *> lst;
	lst << windowsMenu << view << graphMenu << fileMenu << format << edit;
	lst << help << plot2DMenu;
	lst << analysisMenu << multiPeakMenu  << smoothMenu << filterMenu << decayMenu  << normMenu;
	lst << matrixMenu << plot3DMenu << plotDataMenu << scriptingMenu;
	lst << tableMenu << fillMenu << newMenu << exportPlotMenu << importMenu;

#ifdef QTISAS
    lst << qtisasMenu;
#endif
    return lst;
}

QList<QMenu *> ApplicationWindow::menusList()
{
	QList<QMenu *> lst;
	QObjectList children = this->children();
	foreach (QObject *w, children){
        if (QString(w->metaObject()->className()) == "QMenu")
            lst << (QMenu *)w;
    }
	return lst;
}

QList<QToolBar *> ApplicationWindow::toolBarsList()
{
	QList<QToolBar *> lst;
	QObjectList children = this->children();
	foreach (QObject *w, children){
        if (QString(w->metaObject()->className()) == "QToolBar")
            lst << (QToolBar *)w;
    }
	return lst;
}

void ApplicationWindow::hideSelectedColumns()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
		t->hideSelectedColumns();
}

void ApplicationWindow::showAllColumns()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (t)
		t->showAllColumns();
}

void ApplicationWindow::setMatrixUndoStackSize(int size)
{
    if (d_matrix_undo_stack_size == size)
        return;

    d_matrix_undo_stack_size = size;
    Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
		    if (QString(w->metaObject()->className()) == "Matrix"){
				QUndoStack *stack = ((Matrix *)w)->undoStack();
				if (!stack->count())// undo limit can only be changed for empty stacks
                	stack->setUndoLimit(size);
			}
		}
		f = f->folderBelow();
	}
}

QString ApplicationWindow::guessEndOfLine(const QString& sample)
{//Try to guess which end-of-line character is used:
    if (sample.indexOf("\r\n") != -1)//Try \r\n first
        return "\r\n";
    else if (sample.indexOf("\r") != -1)//then look for \r
        return "\r";
	// use \n if neither \r\n nor \r have been found
    return "\n";
}

QString ApplicationWindow::endOfLine()
{
	switch(d_eol){
		case LF:
			return "\n";
		break;
		case CRLF:
			return "\r\n";
		break;
		case CR:
			return "\r";
		break;
	}
	return "\n";
}

QStringList ApplicationWindow::windowsNameList()
{
	QStringList names;
	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows)
			names << w->objectName();
		f = f->folderBelow();
	}
	return names;
}

void ApplicationWindow::initCompleter()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QStringList words;
	words.append("col");
	words.append("tablecol");
	words.append("cell");
#ifdef SCRIPTING_PYTHON
	if (scriptEnv->objectName() == QString("Python")){
		QString fn = d_python_config_folder + "/qti_wordlist.txt";
		QFile file(fn);
		if (!file.open(QFile::ReadOnly)){
			QMessageBox::critical(this, tr("QTISAS - Warning"),
			tr("Couldn't load file: %1.\nAutocompletion will not be available!").arg(QFileInfo(file).absoluteFilePath()));
		} else {
			while (!file.atEnd()){
				QByteArray line = file.readLine();
				if (!line.isEmpty()){
					QString s = line.trimmed();
					if (!words.contains(s))
						words << s;
				}
			}
			file.close();
		}
		words.append(PythonSyntaxHighlighter::keywordsList());
	}
#endif

	words.append(windowsNameList());
    
    QList<MdiSubWindow*> lst = tableList();
    foreach (MdiSubWindow* mw, lst){
        Table *t = (Table*)mw;
        for (int i = 0; i < t->numCols(); i++)
            words.append(t->colName(i));
    }
    
	QStringList functions = scriptEnv->mathFunctions();
	foreach(QString s, functions)
		words.append(s);

    words.sort();

	if (!d_completer){
		d_completer = new QCompleter(this);
		d_completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
		d_completer->setCompletionMode(QCompleter::PopupCompletion);
	}
	d_completer->setModel(new QStringListModel(words, d_completer));

    // Keep history across language changes; only the keyword vocabulary changes.
    if (!m_terminalCompleter)
    {
        m_terminalCompleter = new SmartCompleter(terminal_line, {}, this);
        connect(m_terminalCompleter, &SmartCompleter::modifiedReturnPressed, this,
                &ApplicationWindow::submitTerminalLine);
        m_terminalCompleter->setMemberProvider(
            [this](const QString &owner) { return scriptEnv ? scriptEnv->memberNames(owner) : QStringList(); });
    }
    m_terminalCompleter->setAutomaticHistoryEnabled(false);
    m_terminalCompleter->setKeywords(words);
    updateTerminalModeSuffix(terminal_line->hasFocus() ? QApplication::keyboardModifiers() : Qt::NoModifier);

	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::enableCompletion(bool on)
{
    if (!d_completer || d_completion == on)
        return;

    d_completion = on;

    updateTerminalModeSuffix(terminal_line->hasFocus() ? QApplication::keyboardModifiers() : Qt::NoModifier);

    if (scriptWindow){
		if (d_completion)
            scriptWindow->editor()->setCompleter(d_completer);
        else
            scriptWindow->editor()->setCompleter(0);
    }

    Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			if(QString(w->metaObject()->className()) == "Note"){
                if (d_completion)
                    ((Note *)w)->currentEditor()->setCompleter(d_completer);
                else
                    ((Note *)w)->currentEditor()->setCompleter(0);
			}
        }
		f = f->folderBelow();
	}
}

void ApplicationWindow::showFrequencyCountDialog()
{
    Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return;

    int validRows = 0;
    MySelection sel = t->table()->currentSelection();
//    qDebug() << sel.topRow() << sel.bottomRow();
    if (!sel.isEmpty())
        if (sel.rowCount() > 1 && sel.columnCount() == 1) {
            int col = sel.leftColumn();
            for (int i = sel.topRow(); i <= sel.bottomRow(); i++) {
                if (!t->text(i, col).isEmpty())
                   validRows++;
                if (validRows > 1){
                    FrequencyCountDialog *fcd = new FrequencyCountDialog(t, this);
                    fcd->exec();
                    break;
                }
            }
        }
    if (validRows < 2)
        QMessageBox::warning(this, tr("QTISAS - Column selection error"),
        tr("Please select exactly one column and more than one non empty cell!"));
}

Note * ApplicationWindow::newStemPlot()
{
	Table *t = (Table *)activeWindow(TableWindow);
	if (!t)
		return nullptr;

    MySelection sel = t->table()->currentSelection();
    if (sel.isEmpty())
		return nullptr;

	Note *n = newNote();
	if (!n)
		return nullptr;
	n->hide();

	ScriptEdit* editor = n->currentEditor();
	QStringList lst = t->selectedColumns();
	if (lst.isEmpty()){
		for (int i = sel.leftColumn(); i <= sel.rightColumn(); i++)
			editor->insertPlainText(stemPlot(t, t->colName(i), 1001, sel.topRow() + 1, sel.bottomRow() + 1) + "\n");
	} else {
		for (int i = 0; i < lst.count(); i++)
			editor->insertPlainText(stemPlot(t, lst[i], 1001) + "\n");
	}

	n->show();
	return n;
}

QString ApplicationWindow::stemPlot(Table *t, const QString& colName, int power, int startRow, int endRow)
{
	if (!t)
		return QString();

	int col = t->colIndex(colName);
	if (col < 0){
		QMessageBox::critical(this, tr("QTISAS - Error"),
		tr("Data set: %1 doesn't exist!").arg(colName));
		return QString();
	}

	startRow--;
	endRow--;
	if (startRow < 0 || startRow >= t->numRows())
		startRow = 0;
	if (endRow < 0 || endRow >= t->numRows())
		endRow = t->numRows() - 1;

	QString result = tr("Stem and leaf plot of dataset") + ": " + colName + " ";
	result += tr("from row") + ": " + QString::number(startRow + 1) + " ";
	result += tr("to row") + ": " + QString::number(endRow + 1) + "\n";

	int rows = 0;
	for (int j = startRow; j <= endRow; j++){
		if (!t->text(j, col).isEmpty())
		   rows++;
	}

	if (rows >= 1){
		double *data = (double *)malloc(rows * sizeof (double));
		if (!data){
			result += tr("Not enough memory for this dataset!") + "\n";
			return result;
		}

		result += "\n" + tr("Stem") + " | " + tr("Leaf");
		result += "\n---------------------\n";

		int row = 0;
		for (int j = startRow; j <= endRow; j++){
			if (!t->text(j, col).isEmpty()){
				data[row] = t->cell(j, col);
				row++;
			}
		}
		gsl_sort (data, 1, rows);

		if (power > 1e3){
			power = std::ceil(log10(data[rows - 1] - data[0]) - log10(rows - 1));
			bool ok;
			int input = QInputDialog::getInt(this, tr("Please confirm the stem unit!"),
                                      tr("Data set") + ": " + colName + ", " + tr("stem unit") + " = 10<sup>n</sup>, n = ",
                                      power, -1000, 1000, 1, &ok);
			if (ok)
				power = input;
		}

		double stem_unit = pow(10.0, power);
		double leaf_unit = stem_unit/10.0;

		int prev_stem = int(data[0]/stem_unit);
		result += "      " + QString::number(prev_stem) + " | ";

		for (int j = 0; j <rows; j++){
			double val = data[j];
			int stem = int(val/stem_unit);
			int leaf = int(qRound((val - stem*stem_unit)/leaf_unit));
			for (int k = prev_stem + 1; k < stem + 1; k++)
			  result += "\n      " + QString::number(k) + " | ";
			result += QString::number(leaf);
			prev_stem = stem;
		}

		result += "\n---------------------\n";
		result += tr("Stem unit") + ": " + locale().toString(stem_unit) + "\n";
		result += tr("Leaf unit") + ": " + locale().toString(leaf_unit) + "\n";

		QString legend = tr("Key") + ": " + QString::number(prev_stem) + " | ";
		int leaf = int(qRound((data[rows - 1] - prev_stem*stem_unit)/leaf_unit));
		legend += QString::number(leaf);
		legend += " " + tr("means") + ": " + locale().toString(prev_stem*stem_unit + leaf*leaf_unit) + "\n";

		result += legend + "---------------------\n";
		free(data);
	} else
		result += "\t" + tr("Input error: empty data set!") + "\n";
	return result;
}

QMenu* ApplicationWindow::addCustomMenu(const QString& title, const QString& parentName)
{
	if (parentName == menuBar()->objectName()){
		QMenu *menu = menuBar()->addMenu(title);
		if (menu){
			menu->setObjectName(title);
			d_user_menus.append(menu);
			connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(performCustomAction(QAction *)));
			return menu;
		}
	}

	QList<QMenu *> menus = customizableMenusList();
	foreach (QMenu *m, menus){
		if (m->objectName() == parentName){
            foreach (QAction *existingAction, m->actions())
            {
                QMenu *existingMenu = existingAction->menu();
                if (existingMenu && existingMenu->title().remove("&") == title)
                    return existingMenu;
            }
			QMenu *menu = m->addMenu(title);
			if (menu){
				menu->setObjectName(title);
				d_user_menus.append(menu);
				return menu;
			}
		}
	}
	return nullptr;
}

void ApplicationWindow::removeCustomMenu(const QString& title)
{
	foreach (QMenu *m, d_user_menus){
		if (m->objectName() == title){
			int index = d_user_menus.indexOf(m);
			if (index >= 0 && index < d_user_menus.count()){
				d_user_menus.removeAt(index);
				delete m;
				return;
			}
		}
	}
}

void ApplicationWindow::reloadCustomMenus()
{
	foreach(QMenu *m, d_user_menus){
        if (auto *parentMenu = qobject_cast<QMenu *>(parent()))
        {
            if (!parentMenu->actions().contains(m->menuAction()))
                parentMenu->addAction(m->menuAction());
        }
        else if (auto *parentBar = qobject_cast<QMenuBar *>(parent()))
        {
            if (!parentBar->actions().contains(m->menuAction()))
                parentBar->addAction(m->menuAction());
        }
	}
}

QColor ApplicationWindow::readColorFromProject(const QString& name)
{
	QColor c = QColor();

	if (name.contains(",")){
		QStringList lst = name.split(",");
		if (lst.size() == 2){
			c = QColor(lst[0]);
			c.setAlphaF(lst[1].toDouble());
		}
		return c;
	}

	bool canConvertToInt = false;
	int fillColIndex = name.toInt(&canConvertToInt);
	if (canConvertToInt){
		if (fillColIndex < 0)
			return c;
		if (d_file_version <= 89)
			fillColIndex = convertOldToNewColorIndex(fillColIndex);
		c = ColorBox::defaultColor(fillColIndex);
	} else
		c = QColor(name);

	return c;
}

void ApplicationWindow::enableMdiArea(bool on)
{
	if (d_mdi_windows_area == on)
		return;

	d_mdi_windows_area = on;

	if (on){
		QList<MdiSubWindow *> windows = windowsList();
		foreach(MdiSubWindow *w, windows)
			d_workspace->addSubWindow(w);
	} else {
		QList<QMdiSubWindow *> windows = d_workspace->subWindowList();
		foreach(QMdiSubWindow *w, windows){
			d_workspace->removeSubWindow(w);
			w->show();
		}
	}
}

void ApplicationWindow::memoryAllocationError()
{
	QMessageBox::critical(0, tr("QTISAS") + " - " + tr("Memory Allocation Error"),
		tr("Not enough memory, operation aborted!"));
}

void ApplicationWindow::increasePrecision()
{
	Matrix *m = (Matrix *)activeWindow(MatrixWindow);
	if (!m)
		return;

	int oldPrec = m->precision();
	if (oldPrec == 13)
		return;

	int prec = oldPrec + 1;
	QChar format = m->textFormat();
	m->undoStack()->push(new MatrixSetPrecisionCommand(m, format, format,
					oldPrec, prec, tr("Set Precision %1 digits").arg(prec)));
	m->setNumericPrecision(prec);
}

void ApplicationWindow::decreasePrecision()
{
	Matrix *m = (Matrix *)activeWindow(MatrixWindow);
	if (!m)
		return;

	int oldPrec = m->precision();
	if (oldPrec == 0)
		return;

	int prec = oldPrec - 1;
	QChar format = m->textFormat();
	m->undoStack()->push(new MatrixSetPrecisionCommand(m, format, format,
					oldPrec, prec, tr("Set Precision %1 digits").arg(prec)));
	m->setNumericPrecision(prec);
}

void ApplicationWindow::commentSelection()
{
	Note *note = (Note *)activeWindow(NoteWindow);
	if (!note || !note->currentEditor())
		return;

	note->currentEditor()->commentSelection();
}

void ApplicationWindow::uncommentSelection()
{
	Note *note = (Note *)activeWindow(NoteWindow);
	if (!note || !note->currentEditor())
		return;

	note->currentEditor()->uncommentSelection();
}

void ApplicationWindow::executeAll()
{
	Note *note = (Note *)activeWindow(NoteWindow);
    if (!note)
		return;

	note->executeAll();
}

void ApplicationWindow::addWindowsListToCompleter()
{
#ifdef SCRIPTING_PYTHON
	if (scriptEnv->objectName() != QString("Python"))
		return;

	if (!d_completer)
		return;

	QStringListModel *model = qobject_cast<QStringListModel *> (d_completer->model());
	if (!model)
		return;

	QStringList lst = model->stringList();
	Folder *f = projectFolder();
	while (f){
		QList<MdiSubWindow *> folderWindows = f->windowsList();
		foreach(MdiSubWindow *w, folderWindows){
			lst << w->objectName();

			if (w->inherits("Table")){
				Table *t = (Table*)w;
				for (int i = 0; i < t->numCols(); i++)
					lst.append(t->colName(i));
			}
		}
		f = f->folderBelow();
	}
	lst.sort();
	model->setStringList(lst);
#endif
}

void ApplicationWindow::updateCompleter(const QString& windowName, bool remove, const QString& newName)
{
	if (!d_completer || d_is_appending_file || d_opening_file) return;
    
	QStringListModel *model = qobject_cast<QStringListModel *> (d_completer->model());
    if (!model) return;
    
	QStringList lst = model->stringList();
    
	if (newName.isEmpty())
    {
		Table *t = table(windowName);
		if (remove)
        {
			lst.removeAll(windowName);
			if (t)
            {
				for (int i = 0; i < t->numCols(); i++) lst.removeAll(t->colName(i));
			}
		}
        else
        {
            /* 2021-03
			lst.append(windowName);
			if (t)
            {
				for (int i = 0; i < t->numCols(); i++) lst.append(t->colName(i));
			}
             */
            if (lst.indexOf(windowName) < 0) lst.append(windowName);
            if (t)
            {
                for (int i = 0; i < t->numCols(); i++) if (lst.indexOf(t->colName(i)) < 0) lst.append(t->colName(i));
            }
		}
	}
    else
    {
		int index = lst.indexOf(windowName);
		if (index >= 0) lst.replace(index, newName);

		Table *t = table(newName);
		if (t){
			for (int i = 0; i < t->numCols(); i++){
				int index = lst.indexOf(windowName + "_" + t->colLabel(i));
				if (index >= 0)
					lst.replace(index, t->colName(i));
			}
		}
	}

	lst.sort();
	model->setStringList(lst);
    
}

void ApplicationWindow::addColumnNameToCompleter(const QString& colName, bool remove)
{
	if (!d_completer || d_is_appending_file || d_opening_file)
		return;

	QStringListModel *model = qobject_cast<QStringListModel *> (d_completer->model());
	if (!model)
		return;

	QStringList lst = model->stringList();

	if (remove)
		lst.removeAll(colName);
	else
		lst.append(colName);

	lst.sort();
	model->setStringList(lst);
}

#ifdef SCRIPTING_PYTHON
void ApplicationWindow::openQtDesignerUi()
{
    QString fn = getFileName(this, tr("QTISAS") + " - " + tr("Choose custom user interface"), workingDir, "*.ui",
                             nullptr, false);
	if (!fn.isEmpty()){
		QFileInfo fi(fn);
		workingDir = fi.absolutePath();
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

		setScriptingLanguage("Python");

        QString script = R"("""
Custom user interfaces can be created using QtDesigner from Qt Framework https://www.qt.io
See documentation https://www.riverbankcomputing.co.uk/static/Docs/PyQt$V
"""

from PyQt$V import uic

global ui
ui = uic.loadUi('$FN')
ui.show()
)";
        script.replace("$V", QString::number(QT_VERSION_MAJOR));
        script.replace("$FN", fn);

		Note *note = newNote();
		note->setText(script);
		note->executeAll();

		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::executeStartupScripts()
{
 
	QDir dir(d_startup_scripts_folder);
	if (!dir.exists() || !dir.isReadable())
		return;

	QFileInfoList lst = dir.entryInfoList(QStringList("*.py"), QDir::Files|QDir::NoSymLinks|QDir::Readable);
	if (lst.isEmpty())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	ScriptEdit *se = new ScriptEdit(scriptEnv, this);
	for (int i = 0; i < lst.count(); i++){
		se->importASCII(lst[i].absoluteFilePath());
		se->executeAll();
	}
	delete se;

	QApplication::restoreOverrideCursor();
}
#endif

ImportExportPlugin * ApplicationWindow::exportPlugin(const QString& suffix)
{
	foreach (ImportExportPlugin *plugin, d_import_export_plugins){
		if (plugin->exportFormats().contains(suffix))
			return plugin;
	}
	return 0;
}

ImportExportPlugin * ApplicationWindow::importPlugin(const QString& fileName)
{
	foreach (ImportExportPlugin *plugin, d_import_export_plugins){
		if (plugin->importFormats().contains(QFileInfo(fileName).suffix()))
			return plugin;
	}
	return 0;
}

void ApplicationWindow::loadPlugins()
{
	foreach (QObject *plugin, QPluginLoader::staticInstances()){
		ImportExportPlugin *p = qobject_cast<ImportExportPlugin *>(plugin);
		if (p){
			p->setApplicationWindow(this);
			d_import_export_plugins << p;
		}
	}

	QDir pluginsDir = QDir(qApp->applicationDirPath());
	pluginsDir.cd("plugins");

	foreach (QString fileName, pluginsDir.entryList(QDir::Files)){
		QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
		QObject *plugin = loader.instance();
		if (plugin){
			ImportExportPlugin *p = qobject_cast<ImportExportPlugin *>(plugin);
			if (p){
				p->setApplicationWindow(this);
				d_import_export_plugins << p;
			}
		}
	}
}

void ApplicationWindow::changeFontSasWidgets()
{
    QFont fonts = appFont;

    int pointSize = appFont.pointSize();
    fonts.setPointSize(pointSize + sasFontIncrement);

    QString styleSheet = QString("font-size:%1pt;").arg(pointSize + sasFontIncrement);

#ifdef QTISAS
    ascii1dWidget->setFont(fonts);
    ascii1dWidget->setStyleSheet(styleSheet);
    jnseWidget->setFont(fonts);
    jnseWidget->setStyleSheet(styleSheet);
	svdWidget->setFont(fonts);
    svdWidget->setStyleSheet(styleSheet);
    danWidget->setFont(fonts);
    danWidget->setStyleSheet(styleSheet);
    fittableWidget->setFont(fonts);
    fittableWidget->setStyleSheet(styleSheet);
    compileWidget->setFont(fonts);
    compileWidget->setStyleSheet(styleSheet);
    compileWidget->setFontForce(fonts);
#endif
}

void ApplicationWindow::changeSasReso()
{
    screenResoHight = screen()->availableGeometry().height();

#ifdef QTISAS
    danWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);
    compileWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);
    fittableWidget->initScreenResolusionDependentParameters(screenResoHight, sasResoScale);
#endif
}

void ApplicationWindow::updatePathesInInterfaces()
{
    QString s;
    s=sasPath+"/FitFunctions/";
    s=s.replace("//","/");
    s=s.replace("\\","/");

#ifdef QTISAS
    compileWidget->setPathExtern(s);
    compileWidget->newFIF();
    fittableWidget->setPathExtern(s);
    fittableWidget->scanGroup();
#endif

    findColorMaps();
}

double ApplicationWindow::sigma ( double Q )
{
#ifdef QTISAS
    return ascii1dWidget->sigma ( Q );
#endif
    return 0.0;
};

#ifdef QTISAS
void ApplicationWindow::showCompileDialog()
{
    bringToFront(compileWindow, actionShowCompile);
    if (actionShowCompile->isChecked() && sasDefaultInterface<=0) sasDefaultInterface=-1;
}

void ApplicationWindow::showFittableDialog()
{
    bringToFront(fittableWindow, actionShowFittable);
    if (sasDefaultInterface<=0) sasDefaultInterface=-2;
}

void ApplicationWindow::showJnseDialog()
{
    bringToFront(jnseWindow, actionShowJnse);
    if (sasDefaultInterface<=0) sasDefaultInterface=-5;
}

void ApplicationWindow::showAscii1dDialog()
{
    bringToFront(ascii1dWindow, actionShowAscii1d);
    if (sasDefaultInterface<=0) sasDefaultInterface=-6;
}

void ApplicationWindow::showSvdDialog()
{
    bringToFront(svdWindow, actionShowSvd);
    if (sasDefaultInterface<=0) sasDefaultInterface=-3;
}

void ApplicationWindow::showDanDialog()
{
    bringToFront(danWindow, actionShowDan);
    if (sasDefaultInterface<=0) sasDefaultInterface=-4;
}

void ApplicationWindow::eFitAction(QAction* action)
{
    QString fName=action->text();
    
    if (fittableWidget->widgetStackFit->currentIndex()!=0) fittableWidget->slotStackFitPrev();
    if (fittableWidget->widgetStackFit->currentIndex()!=0) fittableWidget->slotStackFitPrev();
    fittableWidget->scanGroup();

    QModelIndex index = fittableWidget->listViewGroup->model()->index(0, 0);
    if (index.isValid())
    {
        fittableWidget->listViewGroup->selectionModel()->clear();
        fittableWidget->listViewGroup->selectionModel()->select(index, QItemSelectionModel::Select);
    }

    const QModelIndexList indexesAll = fittableWidget->listViewGroup->model()->match(
        fittableWidget->listViewGroup->model()->index(0, 0), Qt::DisplayRole, "ALL", 1, Qt::MatchExactly);

    fittableWidget->groupChanged(indexesAll[0], indexesAll[0]);
    
    const QAbstractItemModel *model = fittableWidget->listViewFunctions->model();
    const QModelIndexList indexes = model->match(
                                                 model->index(0,0),
                                                 Qt::DisplayRole,
                                                 fName,
                                                 1, // first hit only
                                                 Qt::MatchExactly
                                                 );
    
    if (indexes.size() <1) return;
    fittableWidget->listViewFunctions->setCurrentIndex(indexes.at(0));

    fittableWidget->openSharedLibrary(fName);

    fittableWidget->eFit(false);
}
#endif

void ApplicationWindow::showExplorerDialog() {bringToFront(explorerWindow, actionShowExplorer);};
void ApplicationWindow::showLogDialog() {bringToFront(logWindow, actionShowLog);};

void ApplicationWindow::bringToFront( QDockWidget* dockIn, QAction* action )
{
    if ( !action->isChecked() ) return;

    dockIn->show();
    QList<QDockWidget*> docks = tabifiedDockWidgets( dockIn ) ;
    foreach( QDockWidget* dock, docks )
    {
        // Move second dock on top of first dock widget.
        tabifyDockWidget( dock, dockIn ) ;
    }
}

void ApplicationWindow::findColorMaps()
{
    QString colorPath;

    if (!QDir(sasPath + "/colorMaps/").exists())
        QDir().mkdir(sasPath + "/colorMaps/");

    colorPath = sasPath + "/colorMaps/";

    colorPath = colorPath.replace("//", "/");
    
    colorMapList.clear();
    colorMapList = QDir(colorPath).entryList(QStringList() << "*.MAP");
    colorMapList.replaceInStrings(".MAP", "");
    colorMapList.prepend("default #6: royal");
    colorMapList.prepend("default #5: jet-white");
    colorMapList.prepend("default #4: jet");
    colorMapList.prepend("default #3: white|black");
    colorMapList.prepend("default #2: gray scale");
    colorMapList.prepend("default #1: rainbow");
    colorMapList.prepend("default: blue-red");
}

void ApplicationWindow::setMagicTemplate()
{
    setMagicTemplate(magicTemplateFile);
}
    
void ApplicationWindow::setMagicTemplate(QString fn)
{
    if (fn=="") fn=sasPath+"/templates/graph.qpt";
    
    if (fn.isEmpty() || !QFile::exists(fn)){
        QMessageBox::critical(this, tr("QTISAS - File opening error"),
                              tr("The file: <b>%1</b> doesn't exist!").arg(fn));
        return;
    }
    
    QFile f(fn);
    QTextStream t(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
    (void)f.open(QIODevice::ReadOnly);
    QStringList l = t.readLine().split(REGEXPS::whitespaces, Qt::SkipEmptyParts);
    QString fileType=l[0];
    if (fileType != "QtiPlot"){
        QMessageBox::critical(this,tr("QTISAS - File opening error"),
                              tr("The file: <b> %1 </b> was not created using QtiPlot!").arg(fn));
        return;
    }
    
    QStringList vl = l[1].split(".", Qt::SkipEmptyParts);
    d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();
    
   
    
    QString templateType;
    t>>templateType;
    int rows, cols;
    t>>rows; t>>cols;
    t.skipWhiteSpace();
    QString geometry = t.readLine();
    
    if (templateType != "<multiLayer>") return;
    

    
    if (windowsList().count()==0 || !activeWindow() || QString(activeWindow()->metaObject()->className()) != "MultiLayer")  return;
    MultiLayer* plot = (MultiLayer*)activeWindow();
    if (plot->isEmpty()) return;
    Graph * g = (Graph*)plot->activeLayer();
    
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    if (d_file_version > 83)
    {
        QStringList lst = t.readLine().split("\t", Qt::SkipEmptyParts);
        lst = t.readLine().split("\t", Qt::SkipEmptyParts);
        lst = t.readLine().split("\t", Qt::SkipEmptyParts);
        lst = t.readLine().split("\t", Qt::SkipEmptyParts);
    }
    
    QStringList lstAll;
    int graphCount=0;
    QString s;
    while (!t.atEnd())
    {
        s = t.readLine();
        if (s.contains("<ImageProfileTool>"))
            continue;
        if (s.contains("<ImageProfileValues>"))
            continue;
        lstAll << s;
        if (s.left(7) == "<graph>") graphCount++;
    }
    f.close();
    
    QStringList *lst=new QStringList[graphCount];
    int currentGraph=0;
    for(int i=0; i<lstAll.count();i++)
    {

        if (lstAll[i].left(7) == "<graph>")
        {
            i++;
            while ( lstAll[i] != "</graph>" )
            {
                lst[currentGraph] << lstAll[i];
                i++;
                if (i>=lstAll.count()) break;
            }
            currentGraph++;
            i--;
        }
    }
    
    
    if (graphCount<1) return;
    if (graphCount==1)
    {
        openGraph(this, plot, lst[0], g, false);

        QwtPlotCurve *c = g->curve(0);
        
       if (g->curve(0)) if (g->curve(0)->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
       {
           //g->replot();
           //g->setAutoScale();
           //g->replot();
       }
    }
    if (graphCount>1)
    {
        int maxCountGraphsToUpdate=plot->numLayers();
        if (graphCount<maxCountGraphsToUpdate) maxCountGraphsToUpdate=graphCount;
        
        for (int i=maxCountGraphsToUpdate-1; i>=0;i--)
        {
            Graph * gg = (Graph*)plot->layer(i+1);
            openGraph(this, plot, lst[i], gg, false);
//            gg->replot();
//            gg->setAutoScale();
//            gg->replot();
        }
    }
    QApplication::restoreOverrideCursor();

}

void ApplicationWindow::saveGraphAsProject()
{
    MdiSubWindow *w = this->activeWindow();
    if (!w || QString(w->metaObject()->className()) != "MultiLayer")
        return;

    bool compress = false;
    QString fn;
    if (imageFormat.contains("Self-Contained SVG"))
    {
        QString filter = tr("QtiSAS Self-Contained SVG project") + " (*.qti.svg)";
        QString windowTitle = tr("Save Graph as Project");
        QString selectedFilter;
        fn = getFileName(this, windowTitle, workingDir, filter, &selectedFilter, true);
        if (fn.isEmpty())
            return;
        QFileInfo fi(fn);
        const QString dirPath = fi.absolutePath();
        QString stem = fi.fileName();
        bool stripped = true;
        while (stripped)
        {
            stripped = false;
            if (stem.endsWith(".qti.svg", Qt::CaseInsensitive))
            {
                stem.chop(8);
                stripped = true;
            }
            else if (stem.endsWith(".svg", Qt::CaseInsensitive) || stem.endsWith(".qti", Qt::CaseInsensitive))
            {
                stem.chop(4);
                stripped = true;
            }
        }
        fn = dirPath + "/" + stem + ".qti.svg";
        workingDir = dirPath;
    }
    else
    {
        fn = getSaveProjectName("", &compress, 1);
        if (fn.isEmpty())
            return;
    }

    QString baseName = QFileInfo(fn).fileName();
    if (baseName.endsWith(".qti.svg", Qt::CaseInsensitive))
        baseName.chop(8);
    else if (baseName.endsWith(".qti.gz", Qt::CaseInsensitive))
        baseName.chop(7);
    else if (baseName.endsWith(".qti", Qt::CaseInsensitive))
        baseName.chop(4);

    saveGraphAsProject(qobject_cast<MultiLayer *>(w), QFileInfo(fn).absolutePath(), baseName, compress);
}

void ApplicationWindow::saveGraphAsProject(MultiLayer *plot, const QString &path, const QString &name, bool compress,
                                           const QString &format, int resolution)
{
    if (!plot || path.isEmpty() || name.isEmpty())
        return;

    const QString fmt = format.isEmpty() ? imageFormat : format;
    const int res = resolution < 0 ? imageRes : resolution;
    QString fn = path + "/" + name + ".qti";

    if (!fmt.contains("Self-Contained SVG"))
        saveWindow(plot, fn, compress);
    if (plot->layersList().count() == 0)
        return;

    if (fmt == "SVG")
    {
        fn.replace(".qti.gz", ".svg").replace(".qti", ".svg");

        if (plot->layersList().count() > 1)
            plot->exportSVG(fn);
        else if (plot->layersList().count() == 1)
            plot->activeLayer()->exportSVG(fn);
    }
    else if (fmt.contains("Self-Contained SVG"))
    {
        // .qti.svg, not plain .svg -- makes a project-carrying file recognizable at a glance and lets
        // the file-open side fast-path on the extension alone instead of sniffing every .svg's content.
        fn.replace(".qti.gz", ".qti.svg").replace(".qti", ".qti.svg");
        exportGraphAsSelfContainedSvg(plot, fn);
    }
    else if (fmt == "TEX")
    {
        fn.replace(".qti.gz", ".tex").replace(".qti", ".tex");

        if (plot->layersList().count() > 1)
            plot->exportTeX(fn);
        else if (plot->layersList().count() == 1)
            plot->activeLayer()->exportTeX(fn);
    }
    else if (fmt == "PS" || fmt == "EPS" || fmt == "PDF")
    {
        QString str = "." + fmt.toLower();
        fn.replace(".qti.gz", str).replace(".qti", str);

        if (plot->layersList().count() > 1)
            plot->exportVector(fn, true, plot->logicalDpiY(), true);
        else if (plot->layersList().count() == 1)
            plot->activeLayer()->exportVector(fn, true, plot->activeLayer()->logicalDpiY(), true);
    }
    else if (fmt.contains("Archive Graph"))
    {
        fn = fn.remove(".qti.gz").remove(".qti");

        if (plot->layersList().count() == 1)
        {
            Graph *g = plot->activeLayer();
            g->exportVector(fn + ".pdf", true, g->logicalDpiY(), true);
            g->exportSVG(fn + ".svg");
            g->exportImage(fn + ".png", 30, true, 600, QSizeF(), 4, 1, 0);
            g->exportImage(fn + ".tiff", 100, false, 600, QSizeF(), 4, 1, 1);
        }
        else
        {
            plot->exportVector(fn + ".pdf", true, plot->logicalDpiY(), true);
            plot->exportSVG(fn + ".svg");
            plot->exportImage(fn + ".png", 30, true, 600, QSizeF(), 4, 1, 0);
            plot->exportImage(fn + ".tiff", 100, false, 600, QSizeF(), 4, 1, 1);
        }
    }
    else
    {
        QString iformat = "." + fmt.toLower();
        QString fnOUT = fn;
        fnOUT.replace(".qti.gz", iformat).replace(".qti", iformat);
        bool alpha = (fmt == "PNG" || fmt == "XPM" || fmt == "XBM" || fmt == "PPM");

        if (plot->layersList().count() > 1)
            plot->exportImage(fnOUT, 100, alpha, res, QSizeF(), 4, 1, 0);
        else if (plot->layersList().count() == 1)
            plot->activeLayer()->exportImage(fnOUT, 100, alpha, res, QSizeF(), 4, 1, 0);
    }
}

namespace
{
//! Gzip (not raw zlib deflate) decompression entirely in memory -- windowBits 15+16 tells zlib to
//! expect/parse a gzip header+trailer instead of a plain zlib one, matching file_compress()'s output.
QByteArray gzipInflate(const QByteArray &compressed)
{
    if (compressed.isEmpty())
        return {};

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, 15 + 16) != Z_OK)
        return {};

    strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.constData()));
    strm.avail_in = static_cast<uInt>(compressed.size());

    QByteArray out;
    char buf[16384];
    int ret;
    do
    {
        strm.next_out = reinterpret_cast<Bytef *>(buf);
        strm.avail_out = sizeof(buf);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            inflateEnd(&strm);
            return {};
        }
        out.append(buf, static_cast<int>(sizeof(buf) - strm.avail_out));
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return out;
}
} // namespace

QString ApplicationWindow::extractEmbeddedProjectFromSvg(const QString &svgFileName)
{
    QFile svgFile(svgFileName);
    if (!svgFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QString svgContent = QString::fromUtf8(svgFile.readAll());
    svgFile.close();

    const QRegularExpression re(QStringLiteral("qtisas:project=\"([^\"]+)\""));
    const QRegularExpressionMatch m = re.match(svgContent);
    if (!m.hasMatch())
        return {};

    const QByteArray projectBytes = QByteArray::fromBase64(m.captured(1).toLatin1());
    if (projectBytes.isEmpty())
        return {};

    const QByteArray projectText = gzipInflate(projectBytes);
    if (projectText.isEmpty())
        return {};

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
        return {};
    tempDir.setAutoRemove(false); // caller removes the extracted .qti once done with it

    const QString baseName = QFileInfo(svgFileName).baseName();
    QString qtiPath = tempDir.filePath(baseName + ".qti");

    QFile qtiFile(qtiPath);
    if (!qtiFile.open(QIODevice::WriteOnly))
        return {};
    qtiFile.write(projectText);
    qtiFile.close();

    return qtiPath;
}

bool ApplicationWindow::exportGraphAsSelfContainedSvg(MultiLayer *plot, const QString &fileName)
{
    if (!plot || fileName.isEmpty() || plot->layersList().count() == 0)
        return false;

    if (plot->layersList().count() > 1)
        plot->exportSVG(fileName);
    else
        plot->activeLayer()->exportSVG(fileName);

    // Reuse the existing, proven project writer (saveWindow()) rather than teaching it to write to
    // memory -- it and everything it calls (Table::save(), Matrix::save(), ...) are path-based, so
    // going through a real temp file is far less invasive than reworking that whole chain.
    QTemporaryFile tempProject(QDir::temp().filePath("qtisas_embed_XXXXXX.qti"));
    if (!tempProject.open())
        return false;
    const QString tempPath = tempProject.fileName();
    tempProject.close(); // saveWindow() reopens this path itself

    if (!saveWindow(plot, tempPath, true)) // true: gzip via file_compress(), same as any other project
    {
        QFile::remove(tempPath);
        return false;
    }

    const QString gzPath = tempPath + ".gz";
    QFile projectFile(gzPath);
    if (!projectFile.open(QIODevice::ReadOnly))
    {
        QFile::remove(gzPath);
        return false;
    }
    const QByteArray projectBytes = projectFile.readAll();
    projectFile.close();
    QFile::remove(gzPath);

    QFile svgFile(fileName);
    if (!svgFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QString svgContent = QString::fromUtf8(svgFile.readAll());
    svgFile.close();

    const int svgTagStart = static_cast<int>(svgContent.indexOf(QLatin1String("<svg")));
    if (svgTagStart < 0)
        return false;
    const int svgTagEnd = static_cast<int>(svgContent.indexOf(QLatin1Char('>'), svgTagStart));
    if (svgTagEnd < 0)
        return false;

    const QString injected = QStringLiteral(" xmlns:qtisas=\"https://www.qtisas.com/ns\" qtisas:project=\"%1\"")
                                 .arg(QString::fromLatin1(projectBytes.toBase64()));
    svgContent.insert(svgTagEnd, injected);

    if (!svgFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    svgFile.write(svgContent.toUtf8());
    svgFile.close();

    return true;
}

// findActivePlot
bool ApplicationWindow::findActivePlot(MultiLayer *&plot)
{
    if (windowsList().count() == 0 || !activeWindow() ||
        QString(activeWindow()->metaObject()->className()) != "MultiLayer")
        return false;

    plot = (MultiLayer *)activeWindow();
    if (plot->isEmpty())
        return false;

    return true;
}

bool ApplicationWindow::findActiveGraph(Graph *&g)
{
    MultiLayer *plot;
    if (!findActivePlot(plot) || plot->isEmpty())
        return false;

    g = (Graph *)plot->activeLayer();

    return true;
}

bool ApplicationWindow::checkTableExistence(const QString &tableName, Table *&w)
{
    foreach (MdiSubWindow *t, tableList())
        if (t->objectName() == tableName)
        {
            w = (Table *)t;
            return true;
        }
    return false;
}

bool ApplicationWindow::checkTableExistence(const QString &tableName)
{
    foreach (MdiSubWindow *t, tableList())
        if (t->objectName() == tableName)
            return true;
    return false;
}

bool ApplicationWindow::checkNoteExistence(const QString &noteName)
{
    foreach (MdiSubWindow *w, windowsList())
        if (QString(w->metaObject()->className()) == "Note" && w->name() == noteName)
            return true;
    return false;
}

// existMatrix()/existGraph() in Python, next to existTable()/existNote(): confirmed live, AI scripts reach for them by
// analogy, and the blocked or failing call cost the request its correct first answer
bool ApplicationWindow::checkMatrixExistence(const QString &matrixName)
{
    foreach (MdiSubWindow *w, windowsList())
        if (QString(w->metaObject()->className()) == "Matrix" && w->name() == matrixName)
            return true;
    return false;
}

bool ApplicationWindow::checkGraphExistence(const QString &graphName)
{
    foreach (MdiSubWindow *w, windowsList())
        if (QString(w->metaObject()->className()) == "MultiLayer" && w->name() == graphName)
            return true;
    return false;
}

// existGraph()'s counterpart for 3D plots, which it does not count (it knows 2D graph windows only). Confirmed live
// (benchmark LIST 5, item 4): existGraph("Saddle3D") right after newPlot3D("Saddle3D", ...) printed False.
bool ApplicationWindow::checkPlot3DExistence(const QString &plotName)
{
    foreach (MdiSubWindow *w, windowsList())
        if (QString(w->metaObject()->className()) == "Graph3D" && w->name() == plotName)
            return true;
    return false;
}

bool ApplicationWindow::existWindow(const QString &name)
{
    foreach (MdiSubWindow *w, windowsList())
        if (w->name() == name)
            return true;
    return false;
}

void ApplicationWindow::copyStatusBarText()
{
    QApplication::clipboard()->setText(d_status_info->text());
}

QVariant ApplicationWindow::scriptCaller(const QString &scriptCode)
{
    QVariant result = QVariant();

    if (scriptCode.isEmpty())
        return result;

    if (!d_terminal_script)
    {
        d_terminal_script = scriptEnv->newScript("", this, "script caller");
        connect(d_terminal_script, SIGNAL(error(const QString &, const QString &, int)), this,
                SLOT(scriptError(const QString &, const QString &, int)));
        connect(d_terminal_script, SIGNAL(print(const QString &)), scriptEnv, SIGNAL(print(const QString &)));
    }

    d_terminal_script->setCode(scriptCode);
    result = d_terminal_script->replEval();
    return result;
}

void ApplicationWindow::terminal(QString str)
{
    str = str.simplified();
    d_status_info->setText(str);

    if (str.left(1) == "?")
    {
        QMessageBox::information(this, tr("QtiSAS :: Command Line"),
                                 "Keys:\n\n"
                                 " /python or /muparser — select global language and script mode\n"
                                 " /ai                 — select AI mode (generate only)\n"
                                 " /ai-run             — select AI mode with auto-run\n"
                                 " /ai-chat            — conversation only; no scripts or execution\n"
                                 " In AI-chat mode: Enter = chat; Ctrl+Enter = AI; Ctrl+Shift+Enter = AI+run\n"
                                 " Chat commands: stop / cancel; clear history (chat only)\n"
                                 " AI modes require Python; with muParser, all Enter shortcuts run muParser.\n"
                                 " Modes remain selected until another mode command is entered.\n\n"
                                 " Enter               — use the selected mode\n"
                                 " In Python script mode: Ctrl+Enter = AI; Ctrl+Shift+Enter = AI+run\n"
                                 " In AI mode: Ctrl+Enter = global script language; Ctrl+Shift+Enter = AI+run\n"
                                 " In AI-run mode: Ctrl+Enter = AI; Ctrl+Shift+Enter = global script language\n"
                                 " Hold modifiers to preview the mode in the suffix.\n\n\n"
                                 "AI meta commands (answered locally, no API call):\n\n"
                                 " status              — model, URL, docs mode, history turns\n"
                                 " model               — active model name\n"
                                 " url                 — base URL\n"
                                 " history             — list conversation turns\n"
                                 " clear history       — reset conversation context\n"
                                 " stop / cancel       — abort a request currently in flight\n\n\n"
                                 "Built-in commands:\n\n"
                                 "rm [-project -label] *wildcard*\n\n"
                                 " ••• delete widgets selected with wildcard\n"
                                 " ••• by widget names or labels (option '-label')\n"
                                 " ••• in current folder or in entire project (option '-project')\n\n\n"
                                 "rn [-project] *wildcard* abc aXYZc\n\n"
                                 " ••• change names of widgets selected with wildcard\n"
                                 " ••• in current folder or in entire project (option '-project')\n\n\n"
                                 "mc Result = (Sample/1.5 - 0.7 * EB / 1.2 - e^pi) * 0.1\n\n"
                                 " ••• 'mc' starts a Matrix Calculator\n"
                                 " ••• Result, Sample, EB — matrixes of identical dimension\n"
                                 " ••• Result[i,j] = function(Sample[i,j], EB[i,j], ...)\n\n\n"
                                 "radial?\n\n"
                                 " ••• information about the 'radial' command\n"
                                 " ••• 'radial' is for radial averaging of a matrix\n\n\n"
                                 "newTable(\"TableName\")\n\n"
                                 " ••• type any Python script command here");
        return;
    }
    if (str.left(7) == "radial?")
    {
        QMessageBox::information(this, tr("QtiSAS :: Command Line"),
                                 "radial - radial averaging of a matrix:\n\n\n\n"
                                 "radial MATRIX MASK Xc Yc Q_factor I_factor [Integral]\n\n\n\n"
                                 "Radial averaging command. Parameters are below:\n\n"
                                 " ••• MATRIX and MASK should have the same dimension\n\n"
                                 " ••• Xc and Yc define the beam center position\n\n"
                                 " ••• first pixel on a matrix is [1,1]\n\n"
                                 " ••• Q/x value is in [pixels]\n\n"
                                 " ••• to scale Q to a desired dimension, use Q_factor parameter\n\n"
                                 " ••• intensity scaling factor is I_factor\n\n"
                                 " ••• 'Integral' option: intensity will be integrated for all pixels;\n"
                                 " ••• otherwise, intensity is normalized to the number of pixels");
        return;
    }
    if (str.left(7) == "radial ")
    {
        d_status_info->setText("▸ radial: " + str.mid(7).simplified());
        return radialAveragingMatrix(str.right(str.length() - 7).simplified());
    }
    if (str.left(3) == "rm ")
    {
        d_status_info->setText("▸ rm: " + str.mid(3));
        return removeWindows(str.right(str.length() - 3).remove(" "));
    }
    if (str.left(3) == "rn ")
    {
        d_status_info->setText("▸ rn: " + str.mid(3).simplified());
        return renameWindows(str.simplified().right(str.length() - 3));
    }
    if (str.left(3) == "mc ")
    {
        QString expr = str.mid(3).simplified();
        QString result = matrixCalculator(expr);
        d_status_info->setText("▸ MC: " + expr + "  →  " + result);
        return;
    }

    const QString envLabel = "▸▸ " + scriptEnv->objectName() + ": ";
    d_status_info->setText(envLabel + str);
    QVariant res = scriptCaller(str);
    if (res.userType() == QMetaType::Double || res.userType() == QMetaType::LongLong)
    {
        double val = res.toDouble();
        d_status_info->setText(envLabel + str + "  →  " + QString::number(val, 'g', d_decimal_digits));
    }
    else if (res.userType() == QMetaType::QString)
    {
        QString val = res.toString().section('\n', 0, 0);
        d_status_info->setText(envLabel + str + "  →  " + val);
    }
}

void ApplicationWindow::removeWindows(QString pattern)
{
    bool byLabel=false;
    if (pattern.contains("-label"))
    {
        byLabel=true;
        pattern=pattern.remove("-label");
    }
    
    bool removeInFolderYN=false;
    if (pattern.contains("-project"))
    {
        removeInFolderYN=true;
        pattern=pattern.remove("-project");
    }
    
    pattern=pattern.simplified();

    QRegularExpression rx = REGEXPS::wildcardToRegex(pattern);

    QList<MdiSubWindow *> windows;
    if (removeInFolderYN) windows = current_folder->windowsList();
    else windows = windowsList();
    
    foreach(MdiSubWindow *w, windows)
    {
        if ((!byLabel && rx.match(w->objectName()).hasMatch()) || (byLabel && rx.match(w->windowLabel()).hasMatch()))
        {
            w->askOnCloseEvent(false);
            w->close();
        }

    }
}

void ApplicationWindow::renameWindows(QString pattern)
{
    bool renameInFolderYN=false;
    if (pattern.contains("-project"))
    {
        renameInFolderYN=true;
        pattern=pattern.remove("-project");
    }
    
    pattern=pattern.simplified();
    
    QStringList lst0;
    lst0 = pattern.split(" ", Qt::SkipEmptyParts);
    
    if(lst0.count()!=3) return;
    
    QList<MdiSubWindow *> windows;
    if (renameInFolderYN) windows = current_folder->windowsList();
    else windows = windowsList();

    QRegularExpression rx = REGEXPS::wildcardToRegex(lst0[0]);

    foreach(MdiSubWindow *w, windows)
    {
        if (rx.match(w->objectName()).hasMatch())
        {
            QString s0=w->objectName();
            QString s=s0; s=s.replace(lst0[1],lst0[2]);
            setWindowName(w, s);
        }
        
    }
}

QString ApplicationWindow::matrixCalculator(QString script)
{
    if (!script.contains("=")) return "MC:  string contains no symbol '='";
    QStringList lst;
    lst = script.split("=", Qt::SkipEmptyParts);
    if (lst.count()!=2) return "MC:  string contains more than once symbol '='";
    
    QString resultMatrixName=lst[0].remove(" ");
    QString resultScript=lst[1];
    
    if (resultMatrixName=="") return "MC:  result name is empty";

    bool resultMatrixExist=false;
    int rows = 32;
    int cols = 32;
    
    Matrix* mResult;
    QStringList matrixNamesAll=matrixNames();
    if (matrixNamesAll.contains(resultMatrixName))
    {
        resultMatrixExist=true;

        mResult=matrix(resultMatrixName);
    
        rows = mResult->numRows();
        cols = mResult->numCols();
    }
    
    
    QStringList matrixNames;
    QList<Matrix *> matrixes;
    foreach(QString s, matrixNamesAll)
    {
        if (resultScript.contains(s))
        {
            matrixNames<<s;
            matrixes<<matrix(s);
        }
    }
    
    if (!resultMatrixExist && matrixNames.count()>0)
    {
        rows = matrix(matrixNames[0])->numRows();
        cols = matrix(matrixNames[0])->numCols();
    }
    
    foreach(QString s, matrixNames) if (rows != matrix(s)->numRows() || cols != matrix(s)->numCols() )
        return "MC: input/output matrixes have different dimentions";
    
    
    if(!resultMatrixExist)
    {
            mResult=newMatrix(resultMatrixName, rows, cols);
            
            mResult->setWindowLabel("Matrix-Calculator");
            setListViewLabel(resultMatrixName, "Matrix-Calculator");
    }
    
    
    
    int mmMax=matrixNames.count();
    
    QStringList matrixNamesCorrected=matrixNames;
    for(int mm=0; mm<mmMax;mm++)
    {
        matrixNamesCorrected[mm]=matrixNamesCorrected[mm].remove("-").remove("+").remove("/").remove("*");
        resultScript=resultScript.replace(matrixNames[mm],matrixNamesCorrected[mm]);
    }
    
    // MuParser @ Matrix Calculator
    muParserScript *mup = new muParserScript(mResult->scriptingEnv(), resultScript,
                                             mResult, QString("<%1>").arg(mResult->objectName()));
    
    double** mnames= new double* [mmMax];
    for(int mm=0; mm<mmMax;mm++)
    {
        mnames[mm]=mup->defineVariable(matrixNamesCorrected[mm].toLocal8Bit().constData());
        *mnames[mm]=1;
    }
    double *ri = mup->defineVariable("i");
    double *cj = mup->defineVariable("j");
    
    *ri =1.0; *cj =1.0;
    
    if (!mup->compile())
    {
        return "MC compilation error";
    }
    

    for (int i = 0; i<rows; i++)
    {
        for (int j = 0; j<cols; j++)
        {
            for(int mm=0; mm<mmMax;mm++) *mnames[mm]=matrixes[mm]->cell(i,j);
            *ri = (double)i+1.0; *cj = (double)j+1.0;
            QVariant res =mup->evalSingleLine();
            if (res.userType() == QMetaType::Double)
                mResult->setCell(i, j, res.toDouble());
            else
            {
                QMessageBox::warning(this, tr("QTISAS - MATRIX CALCULATOR"), tr("Equation error!"));
                return "MC equation error";
            }
        }
    }
    
    mResult->setFormula(resultScript);
    mResult->notifyModifiedData();
    
    
    return "MC: ok";
}

void ApplicationWindow::radialAveragingMatrix(QString pattern)
{
#ifndef QTISAS
    QMessageBox::warning(this, tr("QTISAS - Radial Averaging of a Matrix"), tr("DAN interface is not compiled"));
    return;
#else
    bool integralYN=false;
    if (pattern.contains(" Integral"))
    {
        integralYN=true;
        pattern=pattern.remove(" Integral");
    }
 
    QStringList lst;
    lst = pattern.split(" ", Qt::SkipEmptyParts);
    
    double qScale=1.0;
    if (lst.count()>4) qScale=lst[4].toDouble();
    double iScale=1.0;
    if (lst.count()>5) iScale=lst[5].toDouble();
    
    if (lst.count()<4)
    {
        QMessageBox::warning(this, tr("QTISAS - Radial Averaging of a Matrix"), tr("not enougph parameters!"));
        return;
    }
    
    QString matrixName=lst[0]; if (matrixName=="") return;
    QString maskName=lst[1]; if (maskName=="") return;
    
    bool matrixExist=false;
    bool maskExist=false;
    Matrix* matrix;
    Matrix* mask;
    QList<MdiSubWindow *> windows = windowsList();
    foreach(MdiSubWindow *w, windows)
    {
        if ( w->objectName()==matrixName && QString(w->metaObject()->className()) == "Matrix")
        {
            matrix=(Matrix*)w;
            matrixExist=true;
        }
        if ( w->objectName()==maskName && QString(w->metaObject()->className()) == "Matrix")
        {
            mask=(Matrix*)w;
            maskExist=true;
        }
        if (matrixExist && maskExist) break;
    }
    
    if (!matrixExist)
    {
        QMessageBox::warning(this, tr("QTISAS - Radial Averaging of a Matrix"), tr("Matrix with given name does not exist!"));
        return;
    }
    if (!maskExist)
    {
        QMessageBox::warning(this, tr("QTISAS - Radial Averaging of a Matrix"), tr("Mask with given name does not exist! All pxels will be used!"));
    }
    
    int chanellNumberX=matrix->numCols();
    int chanellNumberY=matrix->numRows();
    
    if (maskExist && (chanellNumberX!=mask->numCols() || chanellNumberY!=mask->numRows()) )
    {
        QMessageBox::warning(this, tr("QTISAS - Radial Averaging of a Matrix"), tr("Mask matrix has other dimension as matrix! All pxels will be used!"));
        maskExist=false;
    }
    
    QString label;
    
    gsl_matrix *Sample=gsl_matrix_alloc(chanellNumberY,chanellNumberX);
    danWidget->make_GSL_Matrix_Uni(matrixName,Sample,chanellNumberX,chanellNumberY,label);
    gsl_matrix *SampleErr=gsl_matrix_alloc(chanellNumberY,chanellNumberX);
    
    double value;
    for (int i = 0; i < chanellNumberY; i++)
        for (int j = 0; j < chanellNumberX; j++)
        {
            value = gsl_matrix_get(Sample, i, j);
            if (value != 0.0)
                gsl_matrix_set(SampleErr, i, j, 1 / value);
            else
                gsl_matrix_set(SampleErr, i, j, 0.0);
        }

    QString maskLabel;
    gsl_matrix *Mask=gsl_matrix_alloc(chanellNumberY,chanellNumberX);
    if (maskExist) danWidget->make_GSL_Matrix_Uni(maskName,Mask,chanellNumberX,chanellNumberY,maskLabel);
    else gsl_matrix_set_all(Mask,1.0);
        
    double XYcenter=lst[2].toDouble()-1;
    double YXcenter=lst[3].toDouble()-1;
        
    QString sampleMatrix=matrixName;
    label="Radial averaged matrix :: " + matrixName;
        
    radUniHF(chanellNumberX, chanellNumberY, Sample, SampleErr, Mask, XYcenter, YXcenter, matrixName, label, qScale, iScale, !integralYN);
        
    gsl_matrix_free(Sample);
    gsl_matrix_free(SampleErr);
    gsl_matrix_free(Mask);
#endif
}

void ApplicationWindow::radUniHF
(
 int chanellNumberX,int chanellNumberY,
 gsl_matrix *Sample, gsl_matrix *SampleErr,  gsl_matrix *mask,
 double XYcenter, double YXcenter,
 QString sampleMatrix,
 QString label, double qScale, double iScale,  bool normalizeYN
 )
{
    double Xcenter=XYcenter;
    double Ycenter=YXcenter;
    
    int numRowsOut=0;
    
    QString tableOUT;
    tableOUT="RadialAvareging-"+sampleMatrix+"-";
    tableOUT= generateUniqueName(tableOUT);
    
    Table *wOut;
    
    wOut=newHiddenTable(tableOUT,label, 0, 3);
    
    wOut->setColName(0,"Channel");
    wOut->setColName(1,"I");
    wOut->setColName(2,"dI");
    
    wOut->setWindowLabel(label);
    setListViewLabel(wOut->objectName(), label);
    updateWindowLists(wOut);
    
    wOut->setColPlotDesignation(2,Table::yErr);
    wOut->setHeaderColType();

    // calculation of radial averaging

    double Xc=(double(chanellNumberX)-1)/2;
    double Yc=(double(chanellNumberY)-1)/2;
    
    double R;
    
    double rmax =  sqrt( (Xcenter)*(Xcenter) + ( Ycenter)*(Ycenter));
    rmax = GSL_MAX(rmax, (sqrt(((chanellNumberX-1) -
                                Xcenter)*((chanellNumberX-1) - Xcenter) + (Ycenter)*(Ycenter))));
    rmax = GSL_MAX(rmax,sqrt(((chanellNumberX-1) -
                              Xcenter)*((chanellNumberX-1) - Xcenter) + ((chanellNumberY-1) -
                                                                         Ycenter)*((chanellNumberY-1) - Ycenter)));
    rmax = GSL_MAX(rmax,sqrt(( Xcenter)*(Xcenter) + ((chanellNumberY-1)
                                                     - Ycenter)*((chanellNumberY-1) - Ycenter)));
    
    int ir,ii;                  //iR iPhi...
    int ix1,iy1,ix2,iy2;                //...
    int msk, phisteps;          //mask &
    
    double qr,phi,xs,ys,rex,rey;
    double wt;
    
    double pi=M_PI;
    double  twt ;                       // totales Gewicht    >>>RESET<<<
    double avg;                 // Mittel
    double avge;                        //Fehler
    
    
    double sigmaErr;
    
    QString streamSTR="";
    
    for(ir=0;ir<=int(rmax-0.5);ir++)
    {
        //      qr  = 2.*pi/lambda *  (1.0/radInter*ir*detelem/detdist);
        qr  = ir;  // test
        
        phisteps =int( GSL_MAX( int(4*pi*ir+0.5), 4) );
        
        twt = 0.0;      //      ! totales Gewicht    >>>RESET<<<
        avg = 0.0;      //      ! Mittel
        avge= 0.0;      //      ! Fehler
        
        
        
        for(ii=0;ii<=(phisteps-1);ii++)
        {
            phi = 2.0 * pi * ii / phisteps; // Phi
            
            xs  = Xcenter + ir * cos(phi);
            ys  = Ycenter + ir * sin(phi);
            
            R=sqrt((xs-Xc)*(xs-Xc)+(ys-Yc)*(ys-Yc));
            
            ix1 = int(xs);      //         ! Eckpunkte
            iy1 = int(ys);
            ix2 = ix1 + 1;
            iy2 = iy1 + 1;
            
            rex = xs - ix1;     //        ! Reste f"ur Gewichtung sp"ater
            rey = ys - iy1;
            
            msk = 0;    //        ! linker unterer Punkt
            
            
            if (ix1>=0 && ix1<chanellNumberX && iy1>=0 && iy1<chanellNumberY)
            {
                msk=gsl_matrix_get(mask,iy1,ix1); if (msk>0) msk=1;
                wt  = msk*(1.0-rex)*(1.0-rey);
                twt += wt;
                avg += wt *gsl_matrix_get(Sample,iy1,ix1);
                
                avge += wt*fabs(gsl_matrix_get(Sample,iy1,ix1));
            }
            
            msk=0;
            if (ix2>=0 && ix2<chanellNumberX && iy1>=0 && iy1<chanellNumberY)
            {
                msk=gsl_matrix_get(mask,iy1,ix2);if (msk>0) msk=1;
                wt  = msk*(rex)*(1.0-rey);
                twt +=  wt;
                avg += wt*gsl_matrix_get(Sample,iy1,ix2);
                
                avge += wt * fabs(gsl_matrix_get(Sample, iy1, ix2)); // err
            }
            
            msk=0;
            if (ix1>=0 && ix1<chanellNumberX && iy2>=0 && iy2<chanellNumberY)
            {
                msk=gsl_matrix_get(mask,iy2,ix1);if (msk>0) msk=1;
                wt  = msk* (1.0-rex)*(rey);
                twt += wt;
                avg += wt*gsl_matrix_get(Sample,iy2,ix1);
                
                avge += wt * fabs(gsl_matrix_get(Sample, iy2, ix1)); // err
            }
            
            msk=0;
            if (ix2>=0 && ix2<chanellNumberX && iy2>=0 && iy2<chanellNumberY)
            {
                msk=gsl_matrix_get(mask,iy2,ix2);if (msk>0) msk=1;
                wt  = msk* (rex)*(rey);
                twt += wt;
                avg += wt * gsl_matrix_get(Sample,iy2,ix2);
                
                avge += wt * fabs(gsl_matrix_get(Sample, iy2, ix2)); // erri
            }
        }
        avge=sqrt(avg);

        if (twt>0.0  && normalizeYN)
        { // Mean Value
            avge=avge  / twt;
            avg = avg / twt;
        }

        if (twt>0.0)
        {
            double Q,I,dI;
            double QQ, II, dII;
            Q=qr;
            I=avg;
            dI=avge;
            
            QQ=qr;
            II=avg;
            dII=avge;
            
            wOut->setNumRows(numRowsOut+1);
            wOut->setText(numRowsOut,0,QString::number(QQ*qScale,'e',5));
            wOut->setText(numRowsOut,1,QString::number(II*iScale,'e',5));
            wOut->setText(numRowsOut,2,QString::number(dII*iScale,'e',5));
            numRowsOut++;
        }
        
    }
}

void ApplicationWindow::transposeTable()
{
    auto *t = (Table *)activeWindow();
    if (!t)
        return;

    if (t->numCols() == 0 || t->numRows() == 0)
        return;

    int columnTypesAll = Table::Text;
    auto colTypes = t->columnTypes();
    if (QSet<int>(colTypes.begin(), colTypes.end()).size() <= 1)
        columnTypesAll = colTypes[0];

    QList<int> colTypesNew;
    for (int i = 0; i < t->numRows(); ++i)
        colTypesNew.append(columnTypesAll);

    QString columnFormatAll = "0/16";
    const auto colFormats = t->getColumnsFormat();
    if (QSet<QString>(colFormats.begin(), colFormats.end()).size() <= 1)
        columnFormatAll = colFormats[0];

    QStringList colFormatsNew;
    for (int i = 0; i < t->numRows(); ++i)
        colFormatsNew.append(columnFormatAll);

    Table *tNew = newTable(t->numCols(), t->numRows(), generateUniqueName(t->objectName() + "-transposed"),
                           t->windowLabel() + " (transposed)");

    tNew->setColumnTypes(colTypesNew);
    tNew->setColumnsFormat(colFormatsNew);

    for (int i = 0; i < t->numRows(); i++)
        for (int j = 0; j < t->numCols(); j++)
            tNew->setText(j, i, t->text(i, j));

    tNew->adjustColumnsWidth(false);

    setListViewLabel(tNew->name(), t->windowLabel() + " (transposed)");
    tNew->showMaximized();
    updateWindowLists(tNew);
}
