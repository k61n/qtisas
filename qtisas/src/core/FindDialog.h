/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Find dialog
 ******************************************************************************/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QLabel>

class QPushButton;
class QCheckBox;
class QComboBox;
class QLabel;

//! Find dialog
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    FindDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~FindDialog();

private:
	QPushButton* buttonFind;
	QPushButton* buttonCancel;
	QPushButton* buttonReset;

	QLabel *labelStart;
	QComboBox* boxFind;

    QCheckBox* boxWindowNames;
    QCheckBox* boxWindowLabels;
	QCheckBox* boxFolderNames;

	QCheckBox* boxCaseSensitive;
    QCheckBox* boxPartialMatch;
	QCheckBox* boxSubfolders;

public slots:

	//! Displays the project current folder path
	void setStartPath();

protected slots:

	void accept();
};

#endif // exportDialog_H
