/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2007 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python script window
 ******************************************************************************/

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFile>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrintPreviewDialog>
#include <QTextStream>

#include "ApplicationWindow.h"
#include "LineNumberDisplay.h"
#include "ScriptEdit.h"
#include "ScriptWindow.h"


ScriptWindow::ScriptWindow(ScriptingEnv *env, ApplicationWindow *app)
: QMainWindow(),
d_app(app)
{
	initMenu();

    fileName = QString();

	te = new ScriptEdit(env, this, objectName().toLocal8Bit().constData());
	te->setContext(this);
	te->setDirPath(d_app->scriptsDirPath);
	connect(te, SIGNAL(dirPathChanged(const QString& )), d_app, SLOT(scriptsDirPathChanged(const QString&)));

	d_line_number = new LineNumberDisplay(te, this);
	d_frame = new QWidget(this);

	QHBoxLayout *hbox = new QHBoxLayout(d_frame);
    hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(0);
	hbox->addWidget(d_line_number);
	hbox->addWidget(te);

	setCentralWidget(d_frame);

	consoleWindow = new QDockWidget(this);
	consoleWindow->setAllowedAreas(Qt::BottomDockWidgetArea);
	consoleWindow->setWindowTitle(tr("Script Output Panel"));
	addDockWidget( Qt::BottomDockWidgetArea, consoleWindow );
	console = new QTextEdit(consoleWindow);
	console->setReadOnly(true);
	consoleWindow->setWidget(console);
	connect(te, SIGNAL(error(const QString&, const QString&, int)), console, SLOT(setPlainText(const QString&)));
	connect(te, SIGNAL(error(const QString&, const QString&, int)), consoleWindow, SLOT(show()));
	connect(te, SIGNAL(textChanged ()), this, SLOT(enableActions()));

	initActions();
	enableActions();

	setWindowIcon(QIcon(":/logo.png"));
	setWindowTitle(tr("QtiSAS - Script Window") + " - " + tr("untitled"));
	setFocusProxy(te);
	setFocusPolicy(Qt::StrongFocus);
	resize(QSize(500, 300));
}

void ScriptWindow::initMenu()
{
	file = new QMenu(tr("&File"), this);
	menuBar()->addMenu(file);

	edit = new QMenu(tr("&Edit"), this);
	menuBar()->addMenu(edit);

	run = new QMenu(tr("E&xecute"), this);
	menuBar()->addMenu(run);

	windowMenu = new QMenu(tr("&Window"), this);
	menuBar()->addMenu(windowMenu);
}

void ScriptWindow::initActions()
{
	actionNew = new QAction(QIcon(":/new.png"), tr("&New"), this);
	actionNew->setShortcut( tr("Ctrl+N") );
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newScript()));
	file->addAction(actionNew);

	actionOpen = new QAction(QIcon(":/fileopen.png"), tr("&Open..."), this);
	actionOpen->setShortcut( tr("Ctrl+O") );
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	file->addAction(actionOpen);

	file->addSeparator();

	actionSave = new QAction(QIcon(":/filesave.png"), tr("&Save"), this);
	actionSave->setShortcut( tr("Ctrl+S") );
	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
	file->addAction(actionSave);

	actionSaveAs = new QAction(QIcon(":/filesaveas.png"), tr("Save &As..."), this);
	connect(actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
	file->addAction(actionSaveAs);

	file->addSeparator();

	actionPrintPreview = new QAction(QIcon(":/preview.png"), tr("Print Pre&view..."), this);
	connect(actionPrintPreview, SIGNAL(triggered()), this, SLOT(printPreview()));
	file->addAction(actionPrintPreview);

	actionPrint = new QAction(QIcon(":/fileprint.png"), tr("&Print"), this);
	actionPrint->setShortcut( tr("Ctrl+P") );
	connect(actionPrint, SIGNAL(triggered()), te, SLOT(print()));
	file->addAction(actionPrint);

	actionUndo = new QAction(QIcon(":/undo.png"), tr("&Undo"), this);
	actionUndo->setShortcut( tr("Ctrl+Z") );
	connect(actionUndo, SIGNAL(triggered()), te, SLOT(undo()));
	edit->addAction(actionUndo);
	actionUndo->setEnabled(false);

	actionRedo = new QAction(QIcon(":/redo.png"), tr("&Redo"), this);
	actionRedo->setShortcut( tr("Ctrl+Y") );
	connect(actionRedo, SIGNAL(triggered()), te, SLOT(redo()));
	edit->addAction(actionRedo);
	actionRedo->setEnabled(false);
	edit->addSeparator();

	actionCut = new QAction(QIcon(":/cut.png"), tr("&Cut"), this);
	actionCut->setShortcut( tr("Ctrl+x") );
	connect(actionCut, SIGNAL(triggered()), te, SLOT(cut()));
	edit->addAction(actionCut);
	actionCut->setEnabled(false);

	actionCopy = new QAction(QIcon(":/copy.png"), tr("&Copy"), this);
	actionCopy->setShortcut( tr("Ctrl+C") );
	connect(actionCopy, SIGNAL(triggered()), te, SLOT(copy()));
	edit->addAction(actionCopy);
	actionCopy->setEnabled(false);

	actionPaste = new QAction(QIcon(":/paste.png"), tr("&Paste"), this);
	actionPaste->setShortcut( tr("Ctrl+V") );
	connect(actionPaste, SIGNAL(triggered()), te, SLOT(paste()));
	edit->addAction(actionPaste);

	edit->addSeparator();

	actionIncreaseIndent = new QAction(QIcon(":/increase_indent.png"), tr("Increase Indent"), this);
	connect(actionIncreaseIndent, SIGNAL(triggered()), this, SLOT(increaseIndent()));
	edit->addAction(actionIncreaseIndent);

	actionDecreaseIndent = new QAction(QIcon(":/decrease_indent.png"),tr("Decrease Indent"), this);
	connect(actionDecreaseIndent, SIGNAL(triggered()), this, SLOT(decreaseIndent()));
	edit->addAction(actionDecreaseIndent);

	edit->addSeparator();

	actionFind = new QAction(QIcon(":/find.png"), tr("&Find..."), this);
	actionFind->setShortcut(tr("Ctrl+Alt+F"));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(find()));
	edit->addAction(actionFind);

	actionFindNext = new QAction(QIcon(":/find_next.png"), tr("Find &Next"), this);
	actionFindNext->setShortcut(tr("F3"));
	connect(actionFindNext, SIGNAL(triggered()), this, SLOT(findNext()));
	edit->addAction(actionFindNext);

	actionFindPrev = new QAction(QIcon(":/find_previous.png"), tr("Find &Previous"), this);
	actionFindPrev->setShortcut(tr("F4"));
	connect(actionFindPrev, SIGNAL(triggered()), this, SLOT(findPrevious()));
	edit->addAction(actionFindPrev);

	actionReplace = new QAction(QIcon(":/replace.png"), tr("&Replace..."), this);
	connect(actionReplace, SIGNAL(triggered()), this, SLOT(replace()));
	edit->addAction(actionReplace);

	edit->addSeparator();

	actionShowLineNumbers = new QAction(tr("Show &Line Numbers"), this);
	actionShowLineNumbers->setCheckable(true);
	actionShowLineNumbers->setChecked(d_app->d_note_line_numbers);
	connect(actionShowLineNumbers, SIGNAL(toggled(bool)), d_line_number, SLOT(setVisible(bool)));
	edit->addAction(actionShowLineNumbers);

	actionExecute = new QAction(tr("E&xecute"), this);
	actionExecute->setShortcut( tr("CTRL+J") );
	connect(actionExecute, SIGNAL(triggered()), te, SLOT(execute()));
	run->addAction(actionExecute);

	actionExecuteAll = new QAction(QIcon(":/play.png"), tr("Execute &All"), this);
	actionExecuteAll->setShortcut( tr("CTRL+SHIFT+J") );
	connect(actionExecuteAll, SIGNAL(triggered()), te, SLOT(executeAll()));
	run->addAction(actionExecuteAll);

	actionEval = new QAction(tr("&Evaluate Expression"), this);
	actionEval->setShortcut( tr("CTRL+Return") );
	connect(actionEval, SIGNAL(triggered()), te, SLOT(evaluate()));
	run->addAction(actionEval);

	run->addSeparator();

	actionShowConsole = consoleWindow->toggleViewAction();
	actionShowConsole->setText(tr("Show Script &Output Panel"));
	run->addAction(actionShowConsole);

	actionRedirectOutput = new QAction(tr("Ouput on Next &Line"), this);
	actionRedirectOutput->setCheckable(true);
	actionRedirectOutput->setChecked(true);
	connect(actionRedirectOutput, SIGNAL(toggled(bool)), this, SLOT(redirectOutput(bool)));
	run->addAction(actionRedirectOutput);

	actionAlwaysOnTop = new QAction(tr("Always on &Top"), this);
	actionAlwaysOnTop->setCheckable(true);
	if (d_app)
		actionAlwaysOnTop->setChecked (d_app->d_script_win_on_top);
	windowMenu->addAction(actionAlwaysOnTop);
	connect(actionAlwaysOnTop, SIGNAL(toggled(bool)), this, SLOT(setAlwaysOnTop(bool)));

	actionShowWorkspace = new QAction(tr("Show &Workspace"), this);
	actionShowWorkspace->setCheckable(true);
	actionShowWorkspace->setChecked(d_app->isMdiAreaEnabled());
	connect(actionShowWorkspace, SIGNAL(toggled(bool)), this, SLOT(showWorkspace(bool)));
	windowMenu->addAction(actionShowWorkspace);

	actionHide = new QAction(tr("&Close"), this);
	connect(actionHide, SIGNAL(triggered()), this, SLOT(close()));
	windowMenu->addAction(actionHide);

	connect(te, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
	connect(te, SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));
}

void ScriptWindow::languageChange()
{
	setWindowTitle(tr("QtiSAS - Script Window") + " - " + tr("untitled"));
	consoleWindow->setWindowTitle(tr("Script Output Panel"));

	menuBar()->clear();
	menuBar()->addMenu(file);
	menuBar()->addMenu(edit);
	menuBar()->addMenu(run);

	file->setTitle(tr("&File"));
	edit->setTitle(tr("&Edit"));
	run->setTitle(tr("E&xecute"));

	menuBar()->addAction(tr("&Close"), this, SLOT(close()));

	actionNew->setText(tr("&New"));
	actionNew->setShortcut(tr("Ctrl+N"));

	actionOpen->setText(tr("&Open..."));
	actionOpen->setShortcut(tr("Ctrl+O"));

	actionSave->setText(tr("&Save"));
	actionSave->setShortcut(tr("Ctrl+S"));

	actionSaveAs->setText(tr("Save &As..."));

	actionPrint->setText(tr("&Print"));
	actionPrint->setShortcut(tr("Ctrl+P"));

	actionPrintPreview->setText(tr("Print Pre&view..."));

	actionUndo->setText(tr("&Undo"));
	actionUndo->setShortcut(tr("Ctrl+Z"));

	actionRedo->setText(tr("&Redo"));
	actionRedo->setShortcut(tr("Ctrl+Y"));

	actionCut->setText(tr("&Cut"));
	actionCut->setShortcut(tr("Ctrl+x"));

	actionCopy->setText(tr("&Copy"));
	actionCopy->setShortcut(tr("Ctrl+C"));

	actionPaste->setText(tr("&Paste"));
	actionPaste->setShortcut(tr("Ctrl+V"));

	actionExecute->setText(tr("E&xecute"));
	actionExecute->setShortcut(tr("CTRL+J"));

	actionExecuteAll->setText(tr("Execute &All"));
	actionExecuteAll->setShortcut(tr("CTRL+SHIFT+J"));

	actionEval->setText(tr("&Evaluate Expression"));
	actionEval->setShortcut(tr("CTRL+Return"));

	actionShowConsole->setText(tr("Show Script &Output Panel"));
	actionShowConsole->setToolTip(tr("Show Script Output Panel"));

	actionRedirectOutput->setText(tr("Ouput on Next &Line"));
	actionShowWorkspace->setText(tr("Show &Workspace"));

	actionFind->setText(tr("&Find..."));
	actionFind->setShortcut(tr("Ctrl+Alt+F"));

	actionReplace->setText(tr("&Replace..."));

	actionFindNext->setText(tr("Find &Next"));
	actionFindNext->setShortcut(tr("F3"));

	actionFindPrev->setText(tr("Find &Previous"));
	actionFindPrev->setShortcut(tr("F4"));

	actionIncreaseIndent->setText(tr("Increase Indent"));
	actionDecreaseIndent->setText(tr("Decrease Indent"));
}


void ScriptWindow::newScript()
{
    fileName = QString();
	te->clear();
	setWindowTitle(tr("QtiSAS - Script Window") + " - " + tr("untitled"));
}

void ScriptWindow::open(const QString& fn)
{
	QString s = te->importASCII(fn);
	if (!s.isEmpty()){
		fileName = s;
		setWindowTitle(tr("QtiSAS - Script Window") + " - " + fileName);
	}
}

void ScriptWindow::saveAs()
{
	QString fn = te->exportASCII();
	if (!fn.isEmpty()){
		fileName = fn;
		setWindowTitle(tr("QtiSAS - Script Window") + " - " + fileName);
	}
}

void ScriptWindow::save()
{
	if (!fileName.isEmpty()){
		QFile f(fileName);
		if ( !f.open( QIODevice::WriteOnly ) ){
			QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
					tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fileName));
			return;
		}
		QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
		t << te->toPlainText();
		f.close();
	} else
		saveAs();
}

void ScriptWindow::setVisible(bool visible)
{
	if (visible == isVisible())
		return;
	QMainWindow::setVisible(visible);
	emit visibilityChanged(visible);
}

void ScriptWindow::setAlwaysOnTop(bool on)
{
	if (!d_app)
		return;

	d_app->d_script_win_on_top = on;

	QString msg = tr("You need to close and reopen the script window before your changes become effective! Do you want to close it now?");
	if (QMessageBox::question(this, tr("QtiSAS"), msg, QMessageBox::Ok, QMessageBox::No) == QMessageBox::Ok)
		this->close();
}

void ScriptWindow::moveEvent( QMoveEvent* e )
{
	d_app->d_script_win_rect = QRect(pos(), size());
	e->accept();
}

void ScriptWindow::resizeEvent( QResizeEvent* e )
{
	d_app->d_script_win_rect = QRect(geometry().topLeft(), size());
	e->accept();
}

void ScriptWindow::showLineNumbers(bool show)
{
	d_line_number->setVisible(show);
	if (show)
		d_line_number->updateLineNumbers();
}

void ScriptWindow::redirectOutput(bool inside)
{
	if (inside)
		te->redirectOutputTo(0);
	else
		te->redirectOutputTo(console);
}

void ScriptWindow::printPreview()
{
	QPrintPreviewDialog *preview = new QPrintPreviewDialog(this);
	preview->setWindowTitle(tr("QtiSAS") + " - " + tr("Script print preview"));
	connect(preview, SIGNAL(paintRequested(QPrinter *)), te, SLOT(print(QPrinter *)));
	preview->exec();
}

void ScriptWindow::showWorkspace(bool on)
{
	if (!d_app)
		return;

	d_app->enableMdiArea(on);
	d_app->setVisible(on);
	setAttribute (Qt::WA_DeleteOnClose, !on);
}

void ScriptWindow::find()
{
	te->showFindDialog();
}

void ScriptWindow::findNext()
{
	te->findNext();
}

void ScriptWindow::findPrevious()
{
	te->findPrevious();
}

void ScriptWindow::replace()
{
	te->showFindDialog(true);
}

void ScriptWindow::increaseIndent()
{
    te->setTabStopDistance(te->tabStopDistance() + 5);
}

void ScriptWindow::decreaseIndent()
{
    te->setTabStopDistance(te->tabStopDistance() - 5);
}

void ScriptWindow::enableActions()
{
	bool hasText = !te->toPlainText().isEmpty();

	actionFind->setEnabled(hasText);
	actionFindNext->setEnabled(hasText);
	actionFindPrev->setEnabled(hasText);
	actionReplace->setEnabled(hasText);
	actionExecute->setEnabled(hasText);
	actionExecuteAll->setEnabled(hasText);
	actionEval->setEnabled(hasText);
}
