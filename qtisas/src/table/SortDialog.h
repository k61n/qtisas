/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Sort table dialog
 ******************************************************************************/

#ifndef SORTDIALOG_H
#define SORTDIALOG_H

#include <QDialog>

class QPushButton;
class QComboBox;

//! Sorting options dialog
class SortDialog : public QDialog
{
    Q_OBJECT

public:
    SortDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    void insertColumnsList(const QStringList& cols);

private slots:
	void accept();
	void changeType(int index);

signals:
	void sort(int, int, const QString&);

private:
    QPushButton* buttonOk;
	QPushButton* buttonCancel;
	QPushButton* buttonHelp;
	QComboBox* boxType;
	QComboBox* boxOrder;
	QComboBox *columnsList;
};

#endif
