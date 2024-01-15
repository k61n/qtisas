/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Find/Replace dialog for ScriptEdit
 ******************************************************************************/

#ifndef FIND_REPLACE_DIALOG_H
#define FIND_REPLACE_DIALOG_H

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
    FindReplaceDialog(ScriptEdit *editor, bool replace = false, QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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
