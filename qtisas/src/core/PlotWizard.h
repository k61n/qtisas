/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wizard type dialog to create new plots
 ******************************************************************************/

#ifndef PLOTWIZARD_H
#define PLOTWIZARD_H

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>

//! A wizard type dialog class to create new plots
class PlotWizard : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget
	 * \param fl Qt window flags
	 */
    explicit PlotWizard(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

private:
				//! Button "Plot"
    QPushButton* buttonOk,
				//! Button "<->xErr"
				*buttonXErr,
				//! Button "<->yErr"
				*buttonYErr,
				//! Button "Delete curve"
				*buttonDelete;
				//! Button "cancel"
	QPushButton* buttonCancel,
				//! Button "<->X"
				*buttonX,
				//! Button "<->Y"
				*buttonY,
				//! Button "New curve"
				*buttonNew,
				//! Button "<->Z"
				*buttonZ;
				//! Button group defining the layout
    QGroupBox*  groupBox1,
				//! Button group defining the layout
				*groupBox2,
				//! Button group defining the layout
				*groupBox3;
				//! Combo box to select the table
    QComboBox* boxTables;
				//! List of the columns in the selected table
	QListWidget *columnsList,
				//! List of the plots to generate
			 *plotAssociations;

	//! Internal list of columns (contains all columns in all tables)
	QStringList columns;

public slots:
	//! Update the columns list box to contain the columns of 'table'
	void changeColumnsList(const QString &table);
	//! Insert a list of tables into the tables combo box
	void insertTablesList(const QStringList& tables);
	//! Set the contents of the columns list box to the strings in 'cols'
	void setColumnsListBoxContents(const QStringList& cols);
	//! Set the internal columns list to 'cols'
	void setColumnsList(const QStringList& cols);
	//! Add new curve
	void addCurve();
	//! Delete selected curve
	void removeCurve();
	//! Add column as X
	void addXCol();
	//! Add column as Y
	void addYCol();
	//! Add column as X error
	void addXErrCol();
	//! Add column as Y error
	void addYErrCol();
	//! Accept settings, close the dialog
	void accept();
	//! Add column as Z
	void addZCol();

private:
	void plot2D(const QStringList&);
	void plot3D(const QStringList&);
	void plot3DRibbon(const QStringList&);
	//! Display a warning that a new curve must be added first before the selection of the columns
	bool noCurves();
    QSize sizeHint() const;
};

#endif
