/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Add error bars dialog
 ******************************************************************************/

#ifndef ERRDIALOG_H
#define ERRDIALOG_H

#include <QDialog>
#include <QList>
class QLabel;
class QComboBox;
class QRadioButton;
class DoubleSpinBox;
class QPushButton;
class QGroupBox;
class QButtonGroup;
class MdiSubWindow;

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
    ErrDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );

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

#endif // ERRDIALOG_H
