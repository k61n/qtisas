/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Find/Replace dialog for ScriptEdit
 ******************************************************************************/

#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextDocument>

class QPushButton;
class QCheckBox;
class QComboBox;
class ScriptEdit;

//! Find/Replace dialog
class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(ScriptEdit *editor, bool replace = false, QWidget *parent = nullptr,
                               Qt::WindowFlags fl = Qt::WindowFlags());

private:
	QTextDocument::FindFlags searchFlags();

	ScriptEdit *d_editor;

	QPushButton* buttonNext;
	QPushButton* buttonPrevious;
	QPushButton* buttonReplace;
	QPushButton* buttonReplaceAll;
	QPushButton* buttonCancel;

	QComboBox* boxFind;
	QComboBox* boxReplace;

	QCheckBox* boxCaseSensitive;
	QCheckBox* boxWholeWords;

protected slots:
	bool find(bool previous = false);
	bool findPrevious(){return find(true);};
	void replace();
	void replaceAll();
};

#endif
