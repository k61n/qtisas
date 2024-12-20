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

#ifndef NOTE_H
#define NOTE_H

#include <QTabWidget>
#include <QTextEdit>

#include "LineNumberDisplay.h"
#include "MdiSubWindow.h"
#include "ScriptEdit.h"
#include "ScriptingEnv.h"

/*!\brief Notes window class.
 *
 */
class Note: public MdiSubWindow
{
    Q_OBJECT

public:
    Note(ScriptingEnv *env, QTextEdit *output, const QString &label, ApplicationWindow *parent,
         const QString &name = QString(), Qt::WindowFlags f = Qt::WindowFlags());
    Note(ScriptingEnv *env, const QString &label, ApplicationWindow *parent, const QString &name = QString(),
         Qt::WindowFlags f = Qt::WindowFlags());
    ~Note() override = default;

    void init();
	void setName(const QString& name);
    void setTabStopDistance(qreal length);
	int indexOf(ScriptEdit* editor);
    ScriptEdit *editorAt(int index);
	ScriptEdit* currentEditor();
    int tabs();
    void renameTab(int, const QString&);

    void save(const QString &fn, const QString &info, bool = false) override;
    void restore(const QStringList &) override;

public slots:
    bool autoexec() const;
	void setAutoexec(bool);
	void modifiedNote();

	// ScriptEdit methods
    QString text();
    void setText(const QString &s);
    void print() override;
    void print(QPrinter *printer) override;
    void exportPDF(const QString &fileName) override;
    QString exportASCII(const QString &file = QString());
    QString importASCII(const QString &file = QString());
    void executeAll();
    void setDirPath(const QString &path);

	//! Enables/Disables the line number display
	void showLineNumbers(bool show = true);
	bool hasLineNumbers(){return d_line_number_enabled;};

    void setFont(const QFont& f);
    void addTab();
    void removeTab(int = -1);
    void renameCurrentTab();

  signals:
	void dirPathChanged(const QString& path);
	void currentEditorChanged();

private:
	void saveTab(int index, const QString &fn);
	ScriptingEnv *d_env;
    QTabWidget *d_tab_widget{};
	bool d_line_number_enabled;
    bool autoExec{};
    QTextEdit *d_output;
};

#endif
