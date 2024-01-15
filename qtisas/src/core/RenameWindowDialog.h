/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
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
    RenameWindowDialog(QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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

#endif // EXPORTDIALOG_H
