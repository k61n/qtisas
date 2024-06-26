/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Import ASCII file(s) dialog
 ******************************************************************************/

#ifndef IMPORTFILESDIALOG_H
#define IMPORTFILESDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>

#include "ExtensibleFileDialog.h"
#include "Matrix.h"
#include "MatrixModel.h"

class PreviewTable : public QTableWidget
{
	Q_OBJECT

public:
    PreviewTable(int numRows, int numCols, QWidget * parent = 0);

	void importASCII(const QString &fname, const QString &sep, int ignoredLines, bool renameCols,
		bool stripSpaces, bool simplifySpaces, bool importComments, const QString& commentString,
		int importMode, const QLocale& importLocale, int endLine, int maxRows);

	void resetHeader();
	void clear();
	void setNumericPrecision(int prec) {d_numeric_precision = prec;};
	QList<int> columnTypes(){return colTypes;};
	QStringList columnFormats(){return d_col_format;};
	void showColTypeDialog();
	void setSelectedColumn(int col);

signals:
	void modifiedColumnType();

private slots:
	void setColumnType(int);
	void setColumnFormat(const QString&);
	void setHeader();
	void prevColumn();
	void nextColumn();
	void updateColumn(int sc);
	bool eventFilter(QObject *object, QEvent *e);

private:
	void addColumns(int c);
	QStringList comments, col_label;
	int d_numeric_precision, d_start_col;
	QList<int> colTypes;
	QStringList d_col_format;
	int d_selected_column;

	QComboBox *formatBox, *typesBox;
	QLabel *formatLabel;
	QPushButton *buttonNext, *buttonPrev;
	QGroupBox *gb1;
};

class PreviewMatrix : public QTableView
{
public:
    PreviewMatrix(QWidget *parent, Matrix * m = 0);

	void importASCII(const QString &fname, const QString &sep, int ignoredLines,
		bool stripSpaces, bool simplifySpaces, const QString& commentString,
		int importAs, const QLocale& locale, int endLine, int maxRows);

	void clear();

private:
	 MatrixModel *d_matrix_model;
};

//! Import ASCII file(s) dialog
class ImportASCIIDialog: public ExtensibleFileDialog
{
	Q_OBJECT

public:
	//! Possible destinations for the imported data.
	/**
	 * Important: Keep this in sync with the initialization of #d_import_mode in initAdvancedOptions().
	 */
	enum ImportMode {
		NewTables, //!< create a new table for each file (default)
		NewMatrices, //!< create a new matrix for each file
		NewColumns, //!< add each file as new columns to the current table
		NewRows, //!< add each file as new rows to the current table
		Overwrite //!< replace content of current table with the selected file (like importing a single file in previous versions of QtiPlot)
	};

	//! Constructor
	/**
	 * \param importModeEnabled flag: enable/disable import mode combo box
	 * \param parent parent widget (only affects placement of the dialog)
	 * \param extended flag: show/hide the advanced options on start-up
	 * \param flags window flags
	 */
    explicit ImportASCIIDialog(bool new_windows_only, QWidget *parent = nullptr, bool extended = true,
                               Qt::WindowFlags flags = Qt::WindowFlags());

	//! Return the selected import mode
	/**
	 * \sa ImportMode
	 */
	ImportMode importMode() const { return (ImportMode) d_import_mode->currentIndex(); }
	//! Return the selected column separator.
	const QString columnSeparator() const;
	//! Return the number of lines to be skipped at the start of each file.
	int ignoredLines() const { return d_ignored_lines->value(); }
	//! Whether to rename columns based on the first (non-skipped) line.
	bool renameColumns() const { return d_rename_columns->isChecked() && !d_first_line_role->currentIndex(); };
	bool useFirstRow() const { return d_rename_columns->isChecked();};
	//! Whether to replace sequences of whitespace charecters with a single space.
	bool simplifySpaces() const { return d_simplify_spaces->isChecked(); };
	//! Whether to remove whitespace from beginning and end of lines.
	bool stripSpaces() const { return d_strip_spaces->isChecked(); };
	//! Whether the user wants the import options to be saved.
	bool readOnly() const {return d_read_only->isChecked(); };

	//! Set the selected columns separator.
	void setColumnSeparator(const QString &sep);

	//! Returns a locale having the decimal separators set to user custom settings.
	QLocale decimalSeparators();

    //! Returns a string used to comment lines when importing ASCII files
	QString commentString(){return d_comment_string->text();};

    //! Returns true if the second line of the ASCII file should be used to set comments in table
    bool importComments();
    bool useSecondRow() const { return d_import_comments->isChecked();};

	//! Returns the convention used for the end line character!
	inline int endLineChar(){return boxEndLine->currentIndex();};

	QList<int> columnTypes(){if (d_preview_table) return d_preview_table->columnTypes(); return QList<int>();};
	QStringList columnFormats(){if (d_preview_table) return d_preview_table->columnFormats(); return QStringList();};

	void setCurrentPath(const QString& path);

public slots:
	void selectFilter(const QString & filter);

private slots:
	//! Display help for advanced options.
	void displayHelp();
	void showColTypeDialog();
	//! For #Overwrite mode, allow only one file to be selected.
	void updateImportMode(int mode);
	void preview();
	void changePreviewFile(const QString& path);
	//! Enable/Disable options which are only available for tables.
	void enableTableOptions(bool on);
	void enableComments();

private:
	void initPreview(int previewMode);
	void previewTable();
	void previewMatrix();

	void closeEvent(QCloseEvent*);
	//! Initialise #d_advanced_options and everything it contains.
	void initAdvancedOptions();
	void setNewWindowsOnly(bool on);

	//! Container widget for all advanced options.
	QGroupBox *d_advanced_options;
	QCheckBox *d_read_only, *d_omit_thousands_sep;
	QPushButton *d_help_button, *d_col_types_button;
	// the actual options
	QComboBox *d_import_mode, *d_column_separator, *boxDecimalSeparator, *boxEndLine;
	QSpinBox *d_ignored_lines, *d_preview_lines_box;
	QCheckBox *d_rename_columns, *d_simplify_spaces, *d_strip_spaces, *d_import_comments;
	QLineEdit *d_comment_string;
	PreviewTable *d_preview_table;
	PreviewMatrix *d_preview_matrix;
	QCheckBox *d_preview_button;
	QStackedWidget *d_preview_stack;
	QString d_current_path;
	QComboBox *d_first_line_role;
};

#endif
