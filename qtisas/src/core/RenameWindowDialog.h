/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Rename window dialog
 ******************************************************************************/

#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <qvariant.h>
#include <qdialog.h>

#include "MdiSubWindow.h"

class QGroupBox;
class QPushButton;
class QLineEdit;
class QRadioButton;
class QTextEdit;
class MdiSubWindow;
class QButtonGroup;

//! Rename window dialog
class RenameWindowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameWindowDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

private:
    QPushButton * buttonOk;
	QPushButton * buttonCancel;
    QGroupBox * groupBox1;
	QButtonGroup * buttons;
	QRadioButton * boxName;
	QRadioButton * boxLabel;
	QRadioButton * boxBoth;
	QLineEdit * boxNameLine;
	QTextEdit * boxLabelEdit;

public slots:
	void setWidget(MdiSubWindow *w);
	MdiSubWindow::CaptionPolicy getCaptionPolicy();
	void accept();

signals:

private:
	MdiSubWindow *window;
};

#endif
