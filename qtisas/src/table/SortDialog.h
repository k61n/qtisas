/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
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
    explicit SortDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
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
