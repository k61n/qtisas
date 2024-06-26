/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Add error bars dialog
 ******************************************************************************/

#ifndef ERRDIALOG_H
#define ERRDIALOG_H

#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QRadioButton>

#include "DoubleSpinBox.h"
#include "MdiSubWindow.h"

//! Add error bars dialog
class ErrDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 * \param fl window flags
	 */
    explicit ErrDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

private:
    QLabel* textLabel1;
    QComboBox* nameLabel, *tableNamesBox, *colNamesBox;
    QGroupBox *groupBox2;
	QGroupBox *groupBox1, *groupBox3;
	QButtonGroup *buttonGroup1, *buttonGroup2;
    QRadioButton* standardBox, *columnBox;
    QRadioButton* percentBox;
    DoubleSpinBox* valueBox;
    QRadioButton* xErrBox;
    QRadioButton* yErrBox;
	QPushButton* buttonAdd;
    QPushButton* buttonCancel;
	QList<MdiSubWindow*> srcTables;

protected slots:
	//! Set all string in the current language
    virtual void languageChange();

public slots:
	//! Add a plot definition
	void add();
	//! Supply the dialog with a curves list
	void setCurveNames(const QStringList& names);
	//! Supply the dialog with a tables list
	void setSrcTables(QList<MdiSubWindow *> tables);
	//! Select a table
	void selectSrcTable(int tabnr);
    //+++
    void selectedDataset();
};

#endif
