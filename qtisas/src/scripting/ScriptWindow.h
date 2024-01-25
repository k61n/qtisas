/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python script window
 ******************************************************************************/

#ifndef SCRIPTWINDOW_H
#define SCRIPTWINDOW_H

#include "ScriptEdit.h"

#include <QMainWindow>
#include <QMenu>
#include <QCloseEvent>
class ScriptingEnv;
class ApplicationWindow;
class LineNumberDisplay;
class QAction;

//! Python script window
class ScriptWindow: public QMainWindow
{
	Q_OBJECT

public:
		ScriptWindow(ScriptingEnv *env, ApplicationWindow *app);
        ~ScriptWindow(){exit(0);};

public slots:
		void newScript();
		void open(const QString& fn = QString());
		void save();
		void saveAs();
		void languageChange();
		virtual void setVisible(bool visible);

		ScriptEdit* editor(){return te;};
		void executeAll(){te->executeAll();};

		//! Enables/Disables the line number display
		void showLineNumbers(bool show = true);

private slots:
		void setAlwaysOnTop(bool on);
		void redirectOutput(bool);
		void printPreview();
		void showWorkspace(bool on = true);
		void find();
		void findNext();
		void findPrevious();
		void replace();
		void increaseIndent();
		void decreaseIndent();
		void enableActions();

signals:
		void visibilityChanged(bool visible);

private:
		void moveEvent( QMoveEvent* );
		void resizeEvent( QResizeEvent* );

		void initMenu();
		void initActions();
		ScriptEdit *te;
		ApplicationWindow *d_app;
		LineNumberDisplay *d_line_number;
		QWidget *d_frame;

		QString fileName;

		QMenu *file, *edit, *run, *windowMenu;
		QAction *actionNew, *actionUndo, *actionRedo, *actionCut, *actionCopy, *actionPaste;
		QAction *actionExecute, *actionExecuteAll, *actionEval, *actionPrint, *actionOpen;
		QAction *actionSave, *actionSaveAs;
		QAction *actionAlwaysOnTop, *actionHide, *actionShowLineNumbers;
		QAction *actionShowConsole, *actionRedirectOutput, *actionPrintPreview;
		QAction *actionShowWorkspace;
		QAction *actionFind, *actionFindNext, *actionFindPrev, *actionReplace;
		QAction *actionIncreaseIndent, *actionDecreaseIndent;
		QDockWidget *consoleWindow;
		QTextEdit *console;
};

#endif
