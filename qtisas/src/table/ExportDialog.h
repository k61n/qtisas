/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Export ASCII dialog
 ******************************************************************************/

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "ExtensibleFileDialog.h"
#include "MdiSubWindow.h"

//! Export ASCII dialog
class ExportDialog : public ExtensibleFileDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param window window to be exported
	 * \param parent parent widget
	 * \param extended flag: show/hide the advanced options on start-up
	 * \param fl window flags
	 */
    explicit ExportDialog(MdiSubWindow *window = nullptr, QWidget *parent = nullptr, bool extended = true,
                          Qt::WindowFlags fl = Qt::WindowFlags());

private:
	//! Pointer to the window to be exported.
	MdiSubWindow *d_window;

	void closeEvent(QCloseEvent*);
	//! Create #d_advanced_options and everything it contains.
	void initAdvancedOptions();
	void setFileTypeFilters();
	//! Container widget for all advanced options.
	QGroupBox *d_advanced_options;

	QPushButton* buttonHelp;
    QCheckBox* boxNames;
    QCheckBox* boxComments;
    QCheckBox* boxSelection;
	QCheckBox* boxAllTables;
    QComboBox* boxSeparator;
	QComboBox* boxTable;
	QLabel *separatorLbl;
    QLineEdit *fileWildCard;
    
public slots:
	//! Set the column delimiter
	void setColumnSeparator(const QString& sep);
	//! Enable/disable export options depending if the selected window is a Table or a Matrix.
	void updateOptions(const QString & name);
	//! Update which options are visible and enabled based on the output format.
	void updateAdvancedOptions (const QString &filter);

private slots:
	//! Enable/disable the tables combox box
	/**
	 * The tables combo box is disabled when
	 * the checkbox "all" is selected.
	 */
	void enableTableName(bool ok);

protected slots:
	//! Accept changes
	void accept();
	//! Display help
	void help();
};

#endif
