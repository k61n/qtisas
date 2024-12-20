/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Notes window class
 ******************************************************************************/

#include <QFile>
#include <QInputDialog>
#include <QLayout>
#include <QPushButton>
#include <QTextStream>

#include "ApplicationWindow.h"
#include "Note.h"
#include "ScriptEdit.h"

Note::Note(ScriptingEnv *env, QTextEdit *output, const QString &label, ApplicationWindow *parent, const QString &name,
           Qt::WindowFlags f)
    : MdiSubWindow(label, parent, name, f)
{
    d_env = env;
    d_line_number_enabled = parent->d_note_line_numbers;
    d_output = output;
    init();
}

Note::Note(ScriptingEnv *env, const QString &label, ApplicationWindow *parent, const QString &name, Qt::WindowFlags f)
    : MdiSubWindow(label, parent, name, f)
{
    d_env = env;
    d_line_number_enabled = parent->d_note_line_numbers;
    d_output = nullptr;
    init();
}

void Note::init()
{
	autoExec = false;

	d_tab_widget = new QTabWidget;
	d_tab_widget->setTabsClosable(true);
	d_tab_widget->setDocumentMode(true);
    setWidget(d_tab_widget);
	connect(d_tab_widget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));
	connect(d_tab_widget, SIGNAL(currentChanged(int)), this, SLOT(notifyChanges()));
	connect(d_tab_widget, SIGNAL(currentChanged(int)), this, SIGNAL(currentEditorChanged()));

    auto cornerBtns = new QWidget;
    auto cornerBtnsLyt = new QHBoxLayout(cornerBtns);
    cornerBtnsLyt->setContentsMargins(0, 0, 0, 0);
    cornerBtnsLyt->setSpacing(0);
    d_tab_widget->setCornerWidget(cornerBtns, Qt::TopRightCorner);

#ifdef SCRIPTING_PYTHON
    auto btnRun = new QToolButton;
    btnRun->setToolTip(tr("Run"));
    btnRun->setIcon(QIcon(":/play.png"));
    btnRun->setPopupMode(QToolButton::InstantPopup);
    btnRun->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnRun->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cornerBtnsLyt->addWidget(btnRun);
    connect(btnRun, &QToolButton::clicked, this, &Note::executeAll);
#endif

    auto btnAdd = new QToolButton;
    btnAdd->setToolTip(tr("Add tab"));
    btnAdd->setIcon(QIcon(":/plus.png"));
    btnAdd->setPopupMode(QToolButton::InstantPopup);
    btnAdd->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnAdd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cornerBtnsLyt->addWidget(btnAdd);
    connect(btnAdd, &QToolButton::clicked, this, &Note::addTab);

	addTab();
	resize(500, 200);
}

void Note::showLineNumbers(bool show)
{
	if (d_line_number_enabled == show)
		return;

	d_line_number_enabled = show;

	for (int i = 0; i < d_tab_widget->count(); i++){
		QWidget *w = d_tab_widget->widget(i);
		if (!w)
			continue;

		QObjectList lst = w->children();
		foreach (QObject *obj, lst){
			LineNumberDisplay *display = qobject_cast<LineNumberDisplay *>(obj);
			if (display){
				display->setVisible(show);
				break;
			}
		}
	}
}

void Note::renameCurrentTab()
{
	bool ok;
	int index = d_tab_widget->currentIndex();
	QString title = QInputDialog::getText(this, tr("Please, enter new title:"), tr("Title"),
					QLineEdit::Normal, d_tab_widget->tabText(index), &ok);
	if (ok && !title.isEmpty())
		d_tab_widget->setTabText(index, title);
}

void Note::renameTab(int index, const QString& title)
{
	d_tab_widget->setTabText(index, title);
}

void Note::removeTab(int index)
{
	if (index < 0)
		index = d_tab_widget->currentIndex();
	if (d_tab_widget->count() == 1)
		return;
	d_tab_widget->removeTab(index);
	d_tab_widget->setTabsClosable(d_tab_widget->count() != 1);
}

void Note::addTab()
{
	ApplicationWindow *app = applicationWindow();
	if (!app)
		return;
	d_tab_widget->setTabsClosable(d_tab_widget->count() != 0);
	QFont f = app->d_notes_font;

	ScriptEdit *editor = new ScriptEdit(d_env, this, name().toLocal8Bit().constData());
	editor->setContext(this);
	editor->setCurrentFont(f);
	editor->document()->setDefaultFont(f);
    editor->setTabStopDistance(app->d_notes_tab_length);
	editor->setCompleter(app->completer());
	editor->setDirPath(app->scriptsDirPath);
    if (d_output)
        editor->redirectOutputTo(d_output);
	app->connectScriptEditor(editor);
	d_tab_widget->setFocusProxy(editor);
    connect(editor, &QTextEdit::textChanged, this, &Note::modifiedNote);
    connect(editor, &QTextEdit::textChanged, this, &Note::currentEditorChanged);
    connect(editor, &ScriptEdit::dirPathChanged, this, &Note::dirPathChanged);

	LineNumberDisplay *ln = new LineNumberDisplay(editor, this);
    ln->setVisible(d_line_number_enabled);

    auto frame = new QWidget;
	QHBoxLayout *hbox = new QHBoxLayout(frame);
    hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(0);
	hbox->addWidget(ln);
	hbox->addWidget(editor);

    std::vector<int> tabnums = {};
    for (int i = 0; i < d_tab_widget->count(); i++)
    {
        int tabnum = d_tab_widget->tabText(i).split("Tab ").last().toInt();
        tabnums.push_back(tabnum);
    }
    std::sort(tabnums.begin(), tabnums.end());
    if (tabnums.empty())
        d_tab_widget->setCurrentIndex(d_tab_widget->addTab(frame, QString("Tab 1")));
    else
    {
        bool inserted = false;
        for (int i = 1; i <= tabnums.back(); i++)
            if (i != tabnums.at(i - 1))
            {
                d_tab_widget->setCurrentIndex(d_tab_widget->addTab(frame, QString("Tab %1").arg(i)));
                inserted = true;
                break;
            }
        if (!inserted)
            d_tab_widget->setCurrentIndex(d_tab_widget->addTab(frame, QString("Tab %1").arg(tabnums.size() + 1)));
    }
}

int Note::indexOf(ScriptEdit* editor)
{
	if (!editor)
		return -1;

	for (int i = 0; i < d_tab_widget->count(); i++){
		QWidget *w = d_tab_widget->widget(i);
		if (!w)
			continue;

		QObjectList lst = w->children();
		foreach (QObject *obj, lst){
			ScriptEdit *edit = qobject_cast<ScriptEdit *>(obj);
			if (edit && edit == editor){
				return i;
			}
		}
	}

	return -1;
}

ScriptEdit *Note::editorAt(int index)
{
	if (index < 0 || index >= d_tab_widget->count())
        return nullptr;

	QWidget *w = d_tab_widget->widget(index);
	if (!w)
        return nullptr;

	QObjectList lst = w->children();
	foreach (QObject *obj, lst){
		ScriptEdit *edit = qobject_cast<ScriptEdit *>(obj);
		if (edit)
			return edit;
	}
    return nullptr;
}

ScriptEdit* Note::currentEditor()
{
	QWidget *w = d_tab_widget->currentWidget();
	if (!w)
		return 0;

	QObjectList lst = w->children();
	foreach (QObject *obj, lst){
		ScriptEdit *editor = qobject_cast<ScriptEdit *>(obj);
		if (editor)
			return editor;
	}
	return 0;
}

int Note::tabs()
{
    return d_tab_widget->count();
}

void Note::setTabStopDistance(qreal length)
{
	for (int i = 0; i < d_tab_widget->count(); i++){
		QWidget *w = d_tab_widget->widget(i);
		if (!w)
			continue;

		QObjectList lst = w->children();
		foreach (QObject *obj, lst){
			ScriptEdit *edit = qobject_cast<ScriptEdit *>(obj);
			if (edit){
                edit->setTabStopDistance(length);
				break;
			}
		}
	}
}

void Note::setName(const QString& name)
{
	currentEditor()->setObjectName(name);
	MdiSubWindow::setName(name);
}

void Note::modifiedNote()
{
    emit modifiedWindow(this);
}

QString Note::text()
{
    if (currentEditor())
        return currentEditor()->toPlainText();
    return {};
}

void Note::setText(const QString &s)
{
    if (currentEditor())
        currentEditor()->setText(s);
}

void Note::print()
{
    if (currentEditor())
        currentEditor()->print();
}

void Note::print(QPrinter *printer)
{
    if (currentEditor())
        currentEditor()->print(printer);
}

void Note::exportPDF(const QString &fileName)
{
    if (currentEditor())
        currentEditor()->exportPDF(fileName);
}

QString Note::exportASCII(const QString &file)
{
    if (currentEditor())
        return currentEditor()->exportASCII(file);
    return {};
}

QString Note::importASCII(const QString &file)
{
    if (currentEditor())
        return currentEditor()->importASCII(file);
    return {};
}

void Note::save(const QString &fn, const QString &info, bool)
{
	QFile f(fn);
	if (!f.isOpen()){
		if (!f.open(QIODevice::Append))
			return;
	}
	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
	t << "<note>\n";
	t << QString(name()) + "\t" + birthDate() + "\n";
	t << info;
	t << "WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
	t << "AutoExec\t" + QString(autoExec ? "1" : "0") + "\n";
	t << "<LineNumbers>" + QString::number(d_line_number_enabled) + "</LineNumbers>\n";

	f.close();
	for (int i = 0; i < tabs(); i++)
		saveTab(i, fn);

	if (!f.open(QIODevice::Append))
		return;

	t << "</note>\n";
}

void Note::executeAll()
{
    if (currentEditor())
        currentEditor()->executeAll();
    d_output->parentWidget()->raise();
}

void Note::setDirPath(const QString &path)
{
    if (currentEditor())
        currentEditor()->setDirPath(path);
}

void Note::saveTab(int index, const QString &fn)
{
    ScriptEdit *editor = editorAt(index);
    if (editor)
    {
        QFile f(fn);
        if (!f.open(QIODevice::Append))
            return;

        QTextStream t(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        t.setCodec("UTF-8");
#endif
        t << "<tab>\n";
        if (d_tab_widget->currentIndex() == index)
            t << "<active>1</active>\n";

        t << "<title>" + d_tab_widget->tabText(index) + "</title>\n";
        t << "<content>\n" + editor->toPlainText().trimmed() + "\n</content>";
        t << "\n</tab>\n";
        f.close();
    }
}

void Note::restore(const QStringList& data)
{
	QStringList::ConstIterator line = data.begin();
	QStringList fields;

	fields = (*line).split("\t");
	if (fields[0] == "AutoExec"){
		setAutoexec(fields[1] == "1");
		line++;
	}

	bool lineNumbers = true;
	if ((*line).startsWith("<LineNumbers>")){
	  QString s = *line;
	  lineNumbers = s.remove("<LineNumbers>").remove("</LineNumbers>").toInt();
	  line++;
	}

	d_tab_widget->clear();

	int activeTab = 0;
	while (line != data.end() && *line != "</note>"){
		QString s = *line;
		if (s == "<tab>"){
			addTab();

			line++;
			s = *line;

			if (s.startsWith("<active>")){
				activeTab = d_tab_widget->currentIndex();
				line++;
				s = *line;
			}

			if (s.startsWith("<title>"))
				d_tab_widget->setTabText(d_tab_widget->currentIndex(), s.remove("<title>").remove("</title>"));

			line++;
			if (*line == "<content>")
				line++;

			currentEditor()->setUndoRedoEnabled(false);
			while (line != data.end() && *line != "</content>")
				currentEditor()->insertPlainText((*line++)+"\n");
			currentEditor()->setUndoRedoEnabled(true);

			line++;
			if (*line == "</content>")
				line++;
			if (*line == "</tab>")
				line++;
		} else if (s == "<content>"){//old style single editor notes
			addTab();
			line++;

			currentEditor()->setUndoRedoEnabled(false);
			while (line != data.end() && *line != "</content>")
				currentEditor()->insertPlainText((*line++)+"\n");
			currentEditor()->setUndoRedoEnabled(true);

			if (*line == "</content>")
				line++;
		} else {//even older style, no <content> tag, versions < 0.8.5
			if (!currentEditor())
				addTab();
			currentEditor()->insertPlainText((*line++) + "\n");
		}
	}

	showLineNumbers(lineNumbers);
	d_tab_widget->setCurrentIndex(activeTab);
	currentEditor()->moveCursor(QTextCursor::Start);
}

bool Note::autoexec() const
{
    return autoExec;
}

void Note::setAutoexec(bool exec)
{
    autoExec = exec;
    QPalette palette;
    palette.setColor(backgroundRole(), QColor(255, 239, 185));
    if (autoExec)
        currentEditor()->setPalette(palette);
    else
        currentEditor()->setPalette(QPalette());
}

void Note::setFont(const QFont& f)
{
	for (int i = 0; i < d_tab_widget->count(); i++){
		QWidget *w = d_tab_widget->widget(i);
		if (!w)
			continue;

		LineNumberDisplay *display = 0;
		ScriptEdit *editor = 0;

		QObjectList lst = w->children();
		foreach (QObject *obj, lst){
			display = qobject_cast<LineNumberDisplay *>(obj);
			if (display)
				break;
		}

		foreach (QObject *obj, lst){
			editor = qobject_cast<ScriptEdit *>(obj);
			if (editor)
				break;
		}

		if (!editor || !display)
			continue;

		if (editor->toPlainText().isEmpty()){
			editor->setCurrentFont(f);
			editor->document()->setDefaultFont(f);
			display->setCurrentFont(f);
			return;
		}

		editor->selectAll();
		editor->setCurrentFont(f);
		editor->document()->setDefaultFont(f);
		display->selectAll();
		display->setCurrentFont(f);
		display->updateLineNumbers(true);

		QTextCursor cursor = editor->textCursor();
		cursor.clearSelection();
		editor->setTextCursor(cursor);
	}
}
