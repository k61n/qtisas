/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2009 masantos <masantos@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Editor widget for scripting code
 ******************************************************************************/

#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>
#include <QShortcut>
#include <QStringListModel>
#include <QTextStream>

#include "ApplicationWindow.h"
#include "FindReplaceDialog.h"
#include "MyParser.h"
#include "Note.h"
#include "PythonSyntaxHighlighter.h"
#include "ScriptEdit.h"

#ifdef QTISAS
#include "compile18.h"
#endif

ScriptEdit::ScriptEdit(ScriptingEnv *env, QWidget *parent, const char *name)
    : CompatQTextEdit(parent), scripted(env), d_error(false), d_completer(nullptr), d_highlighter(nullptr),
      d_file_name(QString()), d_search_string(QString()), d_output_widget(nullptr)
{
    setObjectName(name);
	myScript = scriptEnv->newScript("", this, name);
	connect(myScript, SIGNAL(error(const QString&, const QString&, int)), this, SLOT(insertErrorMsg(const QString&)));
	connect(myScript, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));
	connect(myScript, SIGNAL(error(const QString&, const QString&, int)),
			this, SIGNAL(error(const QString&, const QString&, int)));

	setLineWrapMode(NoWrap);
	setUndoRedoEnabled(true);
	setAcceptRichText (false);
	setFocusPolicy(Qt::StrongFocus);

	rehighlight();

	d_fmt_default.setBackground(palette().brush(QPalette::Base));

	//Init completer based on parser built-in functions
	QStringList functions = MyParser::functionNamesList();
	functions.sort();
	QCompleter *completer = new QCompleter(this);
	completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->setModel(new QStringListModel(functions, completer));
	setCompleter(completer);

	printCursor = textCursor();
	scriptsDirPath = qApp->applicationDirPath();

	actionExecute = new QAction(tr("E&xecute"), this);
	actionExecute->setShortcut( tr("Ctrl+J") );
	connect(actionExecute, SIGNAL(triggered()), this, SLOT(execute()));

	actionExecuteAll = new QAction(QIcon(":/play.png"), tr("Execute &All"), this);
	actionExecuteAll->setShortcut( tr("Ctrl+Shift+J") );
	connect(actionExecuteAll, SIGNAL(triggered()), this, SLOT(executeAll()));

	actionEval = new QAction(tr("&Evaluate Expression"), this);
	actionEval->setShortcut( tr("Ctrl+Return") );
	connect(actionEval, SIGNAL(triggered()), this, SLOT(evaluate()));

	actionPrint = new QAction(QIcon(":/fileprint.png"), tr("&Print"), this);
	connect(actionPrint, SIGNAL(triggered()), this, SLOT(print()));

	actionImport = new QAction(QIcon(":/fileopen.png"), tr("&Import..."), this);
    actionImport->setShortcut(QKeySequence(Qt::CTRL, Qt::ALT, Qt::Key_O));
	connect(actionImport, SIGNAL(triggered()), this, SLOT(importASCII()));

	actionSave = new QAction(QIcon(":/filesave.png"), tr("&Save"), this);
    actionSave->setShortcut(QKeySequence(Qt::CTRL, Qt::ALT, Qt::Key_S));
	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));

	actionExport = new QAction(QIcon(":/filesaveas.png"), tr("Sa&ve as..."), this);
	connect(actionExport, SIGNAL(triggered()), this, SLOT(exportASCII()));

    
    //*
    actionSaveIncluded = new QAction("Save As &Included Function", this);
    connect(actionSaveIncluded, SIGNAL(triggered()), this, SLOT(saveIncluded()));
    
    actionSaveAsFortranFunction = new QAction("Save As &Fortran Function", this);
    connect(actionSaveAsFortranFunction, SIGNAL(triggered()), this, SLOT(saveAsFortranFunction()));
    
    actionSaveAsFunctionCode = new QAction("Save As Fi&t-Function C-code", this);
    connect(actionSaveAsFunctionCode, SIGNAL(triggered()), this, SLOT(saveAsFunctionCode()));
    //-
    
	actionFind = new QAction(QIcon(":/find.png"), tr("&Find..."), this);
    actionFind->setShortcut(QKeySequence(Qt::CTRL, Qt::ALT, Qt::Key_F));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(showFindDialog()));

	actionReplace = new QAction(QIcon(":/replace.png"), tr("&Replace..."), this);
    actionReplace->setShortcut(QKeySequence(Qt::CTRL, Qt::Key_R));
	connect(actionReplace, SIGNAL(triggered()), this, SLOT(replace()));

	actionFindNext = new QAction(QIcon(":/find_next.png"), tr("&Find next"), this);
	actionFindNext->setShortcut(QKeySequence(Qt::Key_F3));
	connect(actionFindNext, SIGNAL(triggered()), this, SLOT(findNext()));

	actionFindPrevious = new QAction(QIcon(":/find_previous.png"), tr("&Find previous"), this);
	actionFindPrevious->setShortcut(QKeySequence(Qt::Key_F4));
	connect(actionFindPrevious, SIGNAL(triggered()), this, SLOT(findPrevious()));

	functionsMenu = new QMenu(this);
	Q_CHECK_PTR(functionsMenu);
	connect(functionsMenu, SIGNAL(triggered(QAction *)), this, SLOT(insertFunction(QAction *)));

	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(matchParentheses()));
}

void ScriptEdit::enableShortcuts()
{
	QShortcut *accelFindNext = new QShortcut(actionFindNext->shortcut(), this);
	connect(accelFindNext, SIGNAL(activated()), this, SLOT(findNext()));

	QShortcut *accelReplace = new QShortcut(actionReplace->shortcut(), this);
	connect(accelReplace, SIGNAL(activated()), this, SLOT(replace()));

	QShortcut *accelFindPrevious = new QShortcut(actionFindPrevious->shortcut(), this);
	connect(accelFindPrevious, SIGNAL(activated()), this, SLOT(findPrevious()));

	QShortcut *accelFind = new QShortcut(actionFind->shortcut(), this);
	connect(accelFind, SIGNAL(activated()), this, SLOT(showFindDialog()));

	QShortcut *accelSave = new QShortcut(actionSave->shortcut(), this);
	connect(accelSave, SIGNAL(activated()), this, SLOT(save()));

	QShortcut *accelImport = new QShortcut(actionImport->shortcut(), this);
	connect(accelImport, SIGNAL(activated()), this, SLOT(importASCII()));

	QShortcut *accelEval = new QShortcut(actionEval->shortcut(), this);
	connect(accelEval, SIGNAL(activated()), this, SLOT(evaluate()));
}

void ScriptEdit::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT)
	{
		scriptingChangeEvent((ScriptingChangeEvent*)e);
		delete myScript;
		myScript = scriptEnv->newScript("", this, objectName());
		connect(myScript, SIGNAL(error(const QString&, const QString&, int)), this, SLOT(insertErrorMsg(const QString&)));
		connect(myScript, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));

		rehighlight();
	}
}

void ScriptEdit::focusInEvent(QFocusEvent *e)
{
     if (d_completer)
         d_completer->setWidget(this);

	 activated(this);
	 return QTextEdit::focusInEvent(e);
}

void ScriptEdit::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_QuoteDbl){
		QTextCursor cursor = textCursor();
		cursor.insertText("\"\"");
		cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, 1);
		setTextCursor(cursor);
		return;
	}

	if (d_completer && d_completer->popup()->isVisible()){
         // The following keys are forwarded by the completer to the widget
		switch (e->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				e->ignore();
				return; // let the completer do default behavior
			default:
				break;
		}
	 }

    QTextEdit::keyPressEvent(e);
    if (d_completer && !d_completer->popup()->isVisible() && e->key() == Qt::Key_Return)
        updateIndentation();

     bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_U); // CTRL+U
     const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
     if (!d_completer || (ctrlOrShift && e->text().isEmpty()))
         return;

	 static QString eow("~!@#$%^&*()+{}|:\"<>?,./;'[]\\-="); // end of word
     bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
     QString completionPrefix = textUnderCursor();

	 if (!isShortcut && (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 2
		 || eow.contains(e->text().right(1)))){
         d_completer->popup()->hide();
         return;
	 }

     if (completionPrefix != d_completer->completionPrefix()){
         d_completer->setCompletionPrefix(completionPrefix);
         d_completer->popup()->setCurrentIndex(d_completer->completionModel()->index(0, 0));
     }

     QRect cr = cursorRect();
     cr.setWidth(d_completer->popup()->sizeHintForColumn(0)
                 + d_completer->popup()->verticalScrollBar()->sizeHint().width());
     d_completer->complete(cr); // popup it up!
}

void ScriptEdit::contextMenuEvent(QContextMenuEvent *e)
{
	QMenu *menu = createStandardContextMenu();
	Q_CHECK_PTR(menu);

	menu->addSeparator();
	bool emptyText = toPlainText().isEmpty();
	if (!emptyText){
		menu->addAction(actionFind);
		menu->addAction(actionFindNext);
		menu->addAction(actionFindPrevious);
		menu->addAction(actionReplace);
		menu->addSeparator();
	}
	menu->addAction(actionPrint);
	menu->addAction(actionImport);
	menu->addSeparator();
	menu->addAction(actionSave);
	menu->addAction(actionExport);
    
    //*
    menu->addAction(actionSaveIncluded);
    menu->addAction(actionSaveAsFortranFunction);
    menu->addAction(actionSaveAsFunctionCode);
    //-
    
	menu->addSeparator();

	Note *sp = qobject_cast<Note*>(myScript->context());
	if (sp){
		QAction *actionRenameTab = new QAction(tr("Rena&me Tab..."), menu);
		connect(actionRenameTab, SIGNAL(triggered()), sp, SLOT(renameCurrentTab()));
		menu->addAction(actionRenameTab);

		QAction *actionAddTab = new QAction(QIcon(QPixmap(":/plus.png")), tr("A&dd Tab"), menu);
		connect(actionAddTab, SIGNAL(triggered()), sp, SLOT(addTab()));
		menu->addAction(actionAddTab);

		if (sp->tabs() > 1){
			QAction *actionRemoveTab = new QAction(QIcon(QPixmap(":/delete.png")), tr("C&lose Tab"), menu);
			connect(actionRemoveTab, SIGNAL(triggered()), sp, SLOT(removeTab()));
			menu->addAction(actionRemoveTab);
		}

		menu->addSeparator();
	}

	bool python = myScript->scriptingEnv()->objectName() == QString("Python");
	if (!emptyText){
		if (python){
			menu->addAction(actionExecute);
			menu->addAction(actionExecuteAll);
		}
		menu->addAction(actionEval);
	}

	if (sp && python){
		QAction *actionAutoexec = new QAction(tr("Auto&exec"), menu);
		actionAutoexec->setCheckable(true);
		actionAutoexec->setChecked(sp->autoexec());
		connect(actionAutoexec, SIGNAL(toggled(bool)), sp, SLOT(setAutoexec(bool)));
		menu->addAction(actionAutoexec);
		menu->addSeparator();
	}

	functionsMenu->clear();
	functionsMenu->setTearOffEnabled(true);
	QStringList flist = scriptEnv->mathFunctions();
	QMenu *submenu=nullptr;
	for (int i=0; i<flist.size(); i++)
	{
		QAction *newAction;
		QString menupart;
		// group by text before "_" (would make sense if we renamed several functions...)
		/*if (flist[i].contains("_") || (i<flist.size()-1 && flist[i+1].split("_")[0]==flist[i]))
			menupart = flist[i].split("_")[0];
		else
			menupart = "";*/
		// group by first letter, avoiding submenus with only one entry
		if ((i==0 || flist[i-1][0] != flist[i][0]) && (i==flist.size()-1 || flist[i+1][0] != flist[i][0]))
			menupart = "";
		else
			menupart = flist[i].left(1);
		if (!menupart.isEmpty()) {
			if (!submenu || menupart != submenu->title())
				submenu = functionsMenu->addMenu(menupart);
			newAction = submenu->addAction(flist[i]);
		} else
			newAction = functionsMenu->addAction(flist[i]);
		newAction->setData(i);
		newAction->setWhatsThis(scriptEnv->mathFunctionDoc(flist[i]));
	}
	functionsMenu->setTitle(tr("&Functions"));
	menu->addMenu(functionsMenu);

	menu->exec(e->globalPos());
	delete menu;
}

void ScriptEdit::insertErrorMsg(const QString &message)
{
	QString err = message;
	err.prepend("\n").replace("\n","\n#> ");

#ifdef SCRIPTING_CONSOLE
	QTextEdit *console = scriptEnv->application()->scriptingConsole();
	console->setPlainText(err);
	if (!console->isVisible())
		((QDockWidget *)console->parent())->show();
#else
	int start = printCursor.position();
	printCursor.insertText(err);
	printCursor.setPosition(start, QTextCursor::KeepAnchor);
	setTextCursor(printCursor);
#endif

	d_error = true;
	d_err_message = message;
}

void ScriptEdit::scriptPrint(const QString &text)
{
	if(lineNumber(printCursor.position()) == lineNumber(textCursor().selectionEnd()))
		printCursor.insertText("\n");
	printCursor.insertText(text);
}

void ScriptEdit::insertFunction(const QString &fname)
{
	QTextCursor cursor = textCursor();
	QString markedText = cursor.selectedText();
	cursor.insertText(fname+"("+markedText+")");
	if(markedText.isEmpty()){
		// if no text is selected, place cursor inside the ()
		// instead of after it
		cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,1);
		// the next line makes the selection visible to the user
		// (the line above only changes the selection in the
		// underlying QTextDocument)
		setTextCursor(cursor);
	}
}

void ScriptEdit::commentSelection()
{
	QTextCursor cursor = textCursor();
	QString markedText = cursor.selectedText();
	if (markedText.isEmpty())
		return;

	markedText.replace(QChar::ParagraphSeparator, "\n#");
	cursor.insertText("#" + markedText);

	undoAvailable(true);
}

void ScriptEdit::uncommentSelection()
{
	QTextCursor cursor = textCursor();
	QString markedText = cursor.selectedText();
	if (markedText.isEmpty() || !(markedText.contains("#") || markedText.contains("\"\"\"")))
		return;

	cursor.insertText(markedText.remove("#").remove("\"\"\""));
	undoAvailable(true);
}

void ScriptEdit::insertFunction(QAction *action)
{
	insertFunction(scriptEnv->mathFunctions()[action->data().toInt()]);
}

int ScriptEdit::lineNumber(int pos) const
{
	int n = 1;
	for(QTextBlock i=document()->begin(); !i.contains(pos) && i!=document()->end(); i=i.next())
		n++;
	return n;
}

void ScriptEdit::execute()
{
	clearErrorHighlighting();

	QString fname = "<%1:%2>";
	fname = fname.arg(objectName());
	QTextCursor codeCursor = textCursor();
	if (codeCursor.selectedText().isEmpty()){
		codeCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		codeCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
	}
	int startLineNumber = lineNumber(codeCursor.selectionStart());
	fname = fname.arg(startLineNumber);

	myScript->setName(fname);
	myScript->setCode(codeCursor.selectedText().replace(QChar::ParagraphSeparator,"\n"));
	printCursor.setPosition(codeCursor.selectionEnd(), QTextCursor::MoveAnchor);
	printCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
	myScript->exec();

	highlightErrorLine(startLineNumber - 1);
	d_error = false;
}

void ScriptEdit::executeAll()
{
	clearErrorHighlighting();

	QString fname = "<%1>";
	fname = fname.arg(objectName());
	myScript->setName(fname);
	myScript->setCode(toPlainText());
	myScript->exec();

	highlightErrorLine(0);
	d_error = false;
}

void ScriptEdit::evaluate()
{
	clearErrorHighlighting();

	QString fname = "<%1:%2>";
	fname = fname.arg(objectName());
	QTextCursor codeCursor = textCursor();
	if (codeCursor.selectedText().isEmpty()){
		codeCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
		codeCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
	}
	int startLineNumber = lineNumber(codeCursor.selectionStart());
	fname = fname.arg(startLineNumber);

	myScript->setName(fname);
	myScript->setCode(codeCursor.selectedText().replace(QChar::ParagraphSeparator,"\n"));
	QVariant res = myScript->eval();

	if (res.isValid() && !myScript->code().isEmpty())
		if (!res.isNull() && res.canConvert(QVariant::String)){
			QString strVal;
			if (myScript->scriptingEnv()->objectName() == QString("Python"))
				strVal = res.toString();
			else
				strVal = QLocale().toString(res.toDouble());

			strVal.replace("\n", "\n#> ");

			printCursor.insertText("\n");
			printCursor.mergeBlockFormat(d_fmt_default);
			if (!strVal.isEmpty())
				 printCursor.insertText("#> " + strVal + "\n");
		}

	highlightErrorLine(startLineNumber - 2);//we need to substract a line due to __doit__ line prepended to Python scripts.
	d_error = false;
}

void ScriptEdit::exportPDF(const QString& fileName)
{
	QTextDocument *doc = document();
	QPrinter printer;
	printer.setCreator("QtiSAS");
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
	doc->print(&printer);
}

void ScriptEdit::print()
{
	QTextDocument *doc = document();
	QPrinter printer;
	QPrintDialog printDialog(&printer);
	// TODO: Write a dialog to use more features of Qt4's QPrinter class
	if (printDialog.exec() == QDialog::Accepted)
		doc->print(&printer);
}

void ScriptEdit::print(QPrinter *printer)
{
	if (!printer)
		return;

	document()->print(printer);
}

QString ScriptEdit::importASCII(const QString &filename)
{
	QString filter = scriptEnv->fileFilter();
	filter += tr("Text") + " (*.txt *.TXT);";
	filter += tr("All Files")+" (*)";

	QString f;
	if (filename.isEmpty())
		f = ApplicationWindow::getFileName(this, tr("QtiSAS - Import Text From File"), scriptsDirPath, filter, 0, false);
	else
		f = filename;
    if (f.isEmpty())
        return {};

	QFile file(f);
	if (!file.open(QIODevice::ReadOnly)){
		QMessageBox::critical(this, tr("QtiSAS - Error Opening File"), tr("Could not open file \"%1\" for reading.").arg(f));
        return {};
	}

	setFileName(f);

	QFileInfo fi(f);
	if (scriptsDirPath != fi.absolutePath()){
		scriptsDirPath = fi.absolutePath();
		emit dirPathChanged(scriptsDirPath);
	}

	clear();
	QTextStream ts(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    blockSignals(true);
    insertPlainText(ts.readAll());
    blockSignals(false);
    textChanged();

	file.close();
	QApplication::restoreOverrideCursor();
	return f;
}

QString ScriptEdit::save()
{
	return exportASCII(d_file_name);
}

QString ScriptEdit::exportASCII(const QString &filename)
{
	QString filter = scriptEnv->fileFilter();
	filter += tr("Text") + " (*.txt);";
	filter += tr("All Files") + " (*)";

	QString selectedFilter;
	QString fn;
	if (filename.isEmpty())
		fn = ApplicationWindow::getFileName(this, tr("Save Text to File"), scriptsDirPath, filter, &selectedFilter, true, scriptEnv->application()->d_confirm_overwrite);
	else
		fn = filename;

	if (!fn.isEmpty()){
		QFileInfo fi(fn);
		scriptsDirPath = fi.absolutePath();
		if (selectedFilter.contains(".txt") && !fn.endsWith(".txt", Qt::CaseInsensitive))
			fn.append(".txt");
		else if (selectedFilter.contains(".py") && !fn.endsWith(".py", Qt::CaseInsensitive))
			fn.append(".py");

		QFile f(fn);
		if (!f.open(QIODevice::WriteOnly)){
			QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
						tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
            return {};
		}

		QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
		t << toPlainText();
		f.close();

		setFileName(fn);
	}
	return fn;
}

void ScriptEdit::setFileName(const QString& fn)
{
	if (d_file_name == fn)
		return;

	d_file_name = fn;
	Note *note = qobject_cast<Note *>(myScript->context());
	if (note)
		note->renameTab(note->indexOf(this), QFileInfo(d_file_name).fileName());
}

void ScriptEdit::updateIndentation()
{
	QTextCursor cursor = textCursor();
	QTextBlock para = cursor.block();
	QString prev = para.previous().text();

	int i;
	for (i=0; prev[i].isSpace(); i++);
	QString indent = prev.mid(0, i);
	cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
	cursor.insertText(indent);
}

void ScriptEdit::setDirPath(const QString& path)
{
	QFileInfo fi(path);
	if (!fi.exists() || !fi.isDir())
		return;

	scriptsDirPath = path;
}

 void ScriptEdit::setCompleter(QCompleter *completer)
 {
	 if (d_completer)
		QObject::disconnect(d_completer, 0, this, 0);

	 d_completer = completer;

	 if (!d_completer)
		 return;

	 d_completer->setWidget(this);
	 QObject::connect(d_completer, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));
}

 void ScriptEdit::insertCompletion(const QString& completion)
 {
	if (!d_completer || d_completer->widget() != this)
		return;

	QTextCursor tc = textCursor();
	int extra = completion.length() - d_completer->completionPrefix().length();
	tc.movePosition(QTextCursor::Left);
	tc.movePosition(QTextCursor::EndOfWord);

	if (completion == "qti" || completion == "app" || completion == "self" || completion == "QtGui" || completion == "QtCore"){
		tc.insertText(completion.right(extra) + ".");
		setTextCursor(tc);
		return;
	}

	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(scriptEnv->application());
	if (app && (app->windowsNameList().contains(completion) || app->table(completion))){//window or column name?
		tc.insertText(completion.right(extra));
		setTextCursor(tc);
		return;
	}

	bool keyWord = (PythonSyntaxHighlighter::keywordsList().contains(completion));
	QChar startChar = completion[0];
	if (startChar.category() == QChar::Letter_Lowercase && !keyWord){
		tc.insertText(completion.right(extra) + "()");
		tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, 1);
	} else
		tc.insertText(completion.right(extra));

	setTextCursor(tc);
 }

 QString ScriptEdit::textUnderCursor() const
 {
	QTextCursor tc = textCursor();
	tc.select(QTextCursor::WordUnderCursor);

	QString s = tc.selectedText();
	if (s.contains(")")){
		tc.select(QTextCursor::LineUnderCursor);
		s = tc.selectedText();
		s.remove(")");
		if (s.endsWith("\""))
			s.chop(1);

		int pos1 = s.lastIndexOf("\"") + 1;
		int pos2 = s.lastIndexOf(".") + 1;
		int pos3 = s.lastIndexOf("(") + 1;
		int pos4 = s.lastIndexOf(",") + 1;

		if (pos4 > pos3 && pos4 > pos2 && pos4 > pos1)
			return s.mid(pos4, s.length() - pos4).trimmed();
		else if (pos3 > pos2 && pos3 > pos1)
			return s.mid(pos3, s.length() - pos3);
		else if (pos2 > pos1)
			return s.mid(pos2, s.length() - pos2);
		else if (pos1 != -1)
			return s.mid(pos1, s.length() - pos1);
	} else if (s.contains("\"")){
		tc.select(QTextCursor::LineUnderCursor);
		s = tc.selectedText();
		if (s.endsWith("\""))
			s.chop(1);
		int pos = s.lastIndexOf("\"") + 1;
		if (pos != -1)
			return s.mid(pos, s.length() - pos);
	}

	return s;
 }


void ScriptEdit::rehighlight()
{
	if (d_highlighter)
		delete d_highlighter;

#ifdef SCRIPTING_PYTHON
	if (scriptEnv->objectName() == QString("Python"))
		d_highlighter = new PythonSyntaxHighlighter(this);
	else
#endif
		d_highlighter = new SyntaxHighlighter(this);
}

void ScriptEdit::showFindDialog(bool replace)
{
	if (toPlainText().isEmpty())
		return;

	FindReplaceDialog *frd = new FindReplaceDialog(this, replace, (QWidget *)scriptingEnv()->application());
	frd->exec();
}

bool ScriptEdit::find(const QString& searchString, QTextDocument::FindFlags flags, bool previous)
{
	d_search_string = searchString;
	d_search_flags = flags;
	if (previous)
        flags |= QTextDocument::FindBackward;

	QTextCursor d_highlight_cursor = textCursor();
    bool stop = previous ? d_highlight_cursor.atStart() : d_highlight_cursor.atEnd();
	bool found = false;
	while (!d_highlight_cursor.isNull() && !stop){
		d_highlight_cursor = document()->find(searchString, d_highlight_cursor, flags);
		if (!d_highlight_cursor.isNull()){
			found = true;
			setTextCursor(d_highlight_cursor);
			return true;
		}
		stop = previous ? d_highlight_cursor.atStart() : d_highlight_cursor.atEnd();
	}

    if (!found)
        QMessageBox::information(this, tr("QtiSAS"), tr("QtiSAS has finished searching the document."));
	return found;
}

void ScriptEdit::findNext()
{
	if (textCursor().hasSelection())
		d_search_string = textCursor().selectedText();

	if (!d_search_string.isEmpty())
		find(d_search_string, d_search_flags);
	else
		showFindDialog();
}

void ScriptEdit::findPrevious()
{
	if (textCursor().hasSelection())
		d_search_string = textCursor().selectedText();

	if (!d_search_string.isEmpty())
		find(d_search_string, d_search_flags, true);
	else
		showFindDialog();
}

void ScriptEdit::clearErrorHighlighting()
{
	QTextCursor codeCursor = textCursor();
	codeCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
	codeCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	codeCursor.mergeBlockFormat(d_fmt_default);

	if (d_output_widget)
		d_output_widget->clear();
}

void ScriptEdit::highlightErrorLine(int offset)
{
	if (!d_error)
		return;

	QTextCursor codeCursor = textCursor();
	codeCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

	if (scriptEnv->objectName() == QString("Python")){
		QRegExp rx("<*>:(\\d+)");
		rx.indexIn(d_err_message);
		QStringList list = rx.capturedTexts();
		int lineNumber = 0;
		if (!list.isEmpty())
			lineNumber = list.last().toInt();

		codeCursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, offset + lineNumber - 1);
		codeCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
	} else
		codeCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

	QTextBlockFormat d_fmt_failure;
	d_fmt_failure.setBackground(QBrush(QColor(255,128,128)));
	codeCursor.mergeBlockFormat(d_fmt_failure);
	setTextCursor(codeCursor);
}

void ScriptEdit::redirectOutputTo(QTextEdit *te)
{
	d_output_widget = te;
	if (d_output_widget)
		printCursor = QTextCursor(d_output_widget->textCursor());
	else
		printCursor = textCursor();
}

void ScriptEdit::matchParentheses()
{
    QList<QTextEdit::ExtraSelection> selections;
    setExtraSelections(selections);

    TextBlockData *data = static_cast<TextBlockData *>(textCursor().block().userData());

    if (data) {
        QVector<ParenthesisInfo *> infos = data->parentheses();

        int pos = textCursor().block().position();
        for (int i = 0; i < infos.size(); ++i) {
            ParenthesisInfo *info = infos.at(i);

            int curPos = textCursor().position() - textCursor().block().position();
            if (info->position == curPos - 1 && info->character == '(') {
                if (matchLeftParenthesis(textCursor().block(), i + 1, 0))
                    createParenthesisSelection(pos + info->position);
            } else if (info->position == curPos - 1 && info->character == ')') {
                if (matchRightParenthesis(textCursor().block(), i - 1, 0))
                    createParenthesisSelection(pos + info->position);
            }
        }
    }
}

bool ScriptEdit::matchLeftParenthesis(QTextBlock currentBlock, int i, int numLeftParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> infos = data->parentheses();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i) {
        ParenthesisInfo *info = infos.at(i);

        if (info->character == '(') {
            ++numLeftParentheses;
            continue;
        }

        if (info->character == ')' && numLeftParentheses == 0) {
            createParenthesisSelection(docPos + info->position);
            return true;
        } else
            --numLeftParentheses;
    }

    currentBlock = currentBlock.next();
    if (currentBlock.isValid())
        return matchLeftParenthesis(currentBlock, 0, numLeftParentheses);

    return false;
}

bool ScriptEdit::matchRightParenthesis(QTextBlock currentBlock, int i, int numRightParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> parentheses = data->parentheses();

    int docPos = currentBlock.position();
    for (; i > -1 && parentheses.size() > 0; --i) {
        ParenthesisInfo *info = parentheses.at(i);
        if (info->character == ')') {
            ++numRightParentheses;
            continue;
        }
        if (info->character == '(' && numRightParentheses == 0) {
            createParenthesisSelection(docPos + info->position);
            return true;
        } else
            --numRightParentheses;
    }

    currentBlock = currentBlock.previous();
    if (currentBlock.isValid())
        return matchRightParenthesis(currentBlock, 0, numRightParentheses);

    return false;
}

void ScriptEdit::createParenthesisSelection(int pos)
{
    QList<QTextEdit::ExtraSelection> selections = extraSelections();

    QTextEdit::ExtraSelection selection;
    QTextCharFormat format = selection.format;
	format.setBackground(Qt::green);
	format.setForeground (Qt::red);
    selection.format = format;

    QTextCursor cursor = textCursor();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    selection.cursor = cursor;

    selections.append(selection);

    setExtraSelections(selections);
}

ScriptEdit::~ScriptEdit()
{
	if (d_highlighter)
		delete d_highlighter;

	if (d_completer){
        //+++2020 problem with closing projects ALARM
        /*
        d_completer->popup()->close();
		QObject::disconnect(d_completer, 0, this, 0);
         */
	}
}

//*
void ScriptEdit::saveIncluded()
{
    
	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(scriptEnv->application());

    QString filter = "h-files (*.h);;";
    filter += "cpp-files (*.cpp);;";
    filter += tr("All Files")+" (*)";
    
    QString fn=this->objectName();
    fn=fn.replace("-h",".h");
    fn =QFileDialog::getSaveFileName(this, tr("Save Included Functions to File"),
                                     app->sasPath+"/FitFunctions/IncludedFunctions/"+fn, tr(filter.toLocal8Bit().constData()));
    
    
    if ( !fn.isEmpty() )
    {
        QFileInfo fi(fn);
        QString baseName = fi.fileName();
        if (!baseName.contains(".")) fn.append(".h");
        {
            QFile f(fn);
            if ( !f.open( QIODevice::WriteOnly ) )
            {
                QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                      tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
                return;
            }
            QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            t.setCodec("UTF-8");
#endif
            t << toPlainText();
            f.close();
        }
    }
#ifdef QTISAS
    app->compileWidget->scanIncludedFunctions();
#endif
    return;
    
}

void ScriptEdit::saveAsFortranFunction()
{
	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(scriptEnv->application());
    
    QString filter = "Fortran f77-files (*.f *F);;";
    

    
    QString selectedFilter;
    QString fn=this->objectName();


    if (fn.right(4)=="-F90" || fn.right(4)=="-f90")
    {
        filter = "Fortran f90 (*.f90 F90);;"+filter;
        
        fn=fn.replace("-F90",".F90");
        fn=fn.replace("-f90",".f90");
    }
    else
    {
        filter += "Fortran f90 (*.f90 F90);;";

        fn=fn.replace("-F",".F");
        fn=fn.replace("-f",".f");
    }
    filter += tr("All Files")+" (*)";

    fn = QFileDialog::getSaveFileName(this, tr("Save Fortran Function to File"),
                                 app->sasPath+"/FitFunctions/"+fn, filter);
    
    if ( !fn.isEmpty() )
    {
        QFileInfo fi(fn);
        QString baseName = fi.fileName();
        if (!baseName.contains("."))fn.append(".f");
        {
            QFile f(fn);
            if ( !f.open( QIODevice::WriteOnly ) )
            {
                QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                      tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
                return;
            }
            QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            t.setCodec("UTF-8");
#endif
            t << toPlainText();
            f.close();
        }
    }
    return;
}

void ScriptEdit::saveAsFunctionCode()
{
	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(scriptEnv->application());
    
    QString filter = "cpp-code (*.cpp);;";
    filter += tr("All Files")+" (*)";
    
    QString fn = this->objectName();
    fn = fn.replace("-cpp",".cpp");
    fn = QFileDialog::getSaveFileName(this, tr("Save Fit-Function as C-code"),
                                      app->sasPath+"/FitFunctions/"+fn, tr(filter.toLocal8Bit().constData()));
    
    
    if ( !fn.isEmpty() )
    {
        QFileInfo fi(fn);
        QString baseName = fi.fileName();
        if (!baseName.contains(".")) fn.append(".cpp");
        
        if (QFile::exists(fn) && QMessageBox::question(this, tr("QtiSAS -- Overwrite File? "),
                                                       tr("A file called: <p><b>%1</b><p>already exists.\n"
                                                          "Do you want to overwrite it?")
                                                           .arg(fn),
                                                       tr("&Yes"), tr("&No"), QString(), 0, 1))
            return;
        else
        {
            QFile f(fn);
            if ( !f.open( QIODevice::WriteOnly ) )
            {
                QMessageBox::critical(0, tr("QtiSAS - File Save Error"),
                                      tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
                return;
            }
            QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            t.setCodec("UTF-8");
#endif
            t << toPlainText();
            f.close();
        }
    }
    
    return;

}

//-

