/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
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
    explicit FindDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
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
