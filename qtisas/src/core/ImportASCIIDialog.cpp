/***************************************************************************
    File                 : ImportASCIIDialog.cpp
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright /QtiSAS/  : (C) 2012-2021 by Vitaliy Pipich
    Copyright /QtiPlot/ : (C) 2004-2011 by Ion Vasilief
                          (C) 2006 - June 2007 by Knut Franke
 
    Email (use @ for *)  : v.pipich*gmail.com, ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Import ASCII file(s) dialog

 ***************************************************************************/


/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#include <QLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QRegExp>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QHeaderView>
#include <QInputDialog>
#include <QCloseEvent>

#include <gsl/gsl_math.h>

#include "ImportASCIIDialog.h"
#include "ApplicationWindow.h"
#include "Table.h"
#include "Matrix.h"
#include "MatrixModel.h"

ImportASCIIDialog::ImportASCIIDialog(bool new_windows_only, QWidget * parent, bool extended, Qt::WindowFlags flags )
: ExtensibleFileDialog(parent, extended, flags )
{
	setWindowTitle(tr("QtiSAS - Import ASCII File(s)"));
	setFileMode( QFileDialog::ExistingFiles );

	d_current_path = QString::null;

	initAdvancedOptions();
	setNewWindowsOnly(new_windows_only);
	setExtensionWidget(d_advanced_options);
	setEditableFilter();

	// get rembered option values
	ApplicationWindow *app = (ApplicationWindow *)parent;
	setLocale(app->locale());

	d_strip_spaces->setChecked(app->strip_spaces);
	d_simplify_spaces->setChecked(app->simplify_spaces);
	d_ignored_lines->setValue(app->ignoredLines);
	d_rename_columns->setChecked(app->renameColumns);
	setColumnSeparator(app->columnSeparator);
    d_comment_string->setText(app->d_ASCII_comment_string);
    d_import_comments->setChecked(app->d_ASCII_import_comments);
    d_first_line_role->setCurrentIndex(app->d_ASCII_import_first_row_role);
    d_read_only->setChecked(app->d_ASCII_import_read_only);

	if (app->d_ASCII_import_locale.name() == QLocale::c().name())
        boxDecimalSeparator->setCurrentIndex(1);
    else if (app->d_ASCII_import_locale.name() == QLocale(QLocale::German).name())
        boxDecimalSeparator->setCurrentIndex(2);
    else if (app->d_ASCII_import_locale.name() == QLocale(QLocale::French).name())
        boxDecimalSeparator->setCurrentIndex(3);

	QLocale::NumberOptions groupSep = app->d_ASCII_import_locale.numberOptions();
    d_omit_thousands_sep->setChecked(true);//+++groupSep & QLocale::OmitGroupSeparator);

	connect(d_import_mode, SIGNAL(currentIndexChanged(int)), this, SLOT(updateImportMode(int)));
	if (app->d_ASCII_import_mode < d_import_mode->count())
		d_import_mode->setCurrentIndex(app->d_ASCII_import_mode);

	d_preview_lines_box->setValue(app->d_preview_lines);
	d_preview_button->setChecked(app->d_ASCII_import_preview);

    boxEndLine->setCurrentIndex((int)app->d_ASCII_end_line);

	if (!app->d_ASCII_import_preview)
		d_preview_stack->hide();

	initPreview(d_import_mode->currentIndex());

    connect(d_preview_lines_box, SIGNAL(valueChanged(int)), this, SLOT(preview()));
    connect(d_rename_columns, SIGNAL(clicked()), this, SLOT(preview()));
    connect(d_import_comments, SIGNAL(clicked()), this, SLOT(preview()));
    connect(d_strip_spaces, SIGNAL(clicked()), this, SLOT(preview()));
    connect(d_simplify_spaces, SIGNAL(clicked()), this, SLOT(preview()));
    connect(d_ignored_lines, SIGNAL(valueChanged(int)), this, SLOT(preview()));
    connect(d_omit_thousands_sep, SIGNAL(clicked()), this, SLOT(preview()));
    connect(d_column_separator, SIGNAL(currentIndexChanged(int)), this, SLOT(preview()));
    connect(boxDecimalSeparator, SIGNAL(currentIndexChanged(int)), this, SLOT(preview()));
    connect(d_comment_string, SIGNAL(textChanged(const QString&)), this, SLOT(preview()));
    connect(this, SIGNAL(currentChanged(const QString&)), this, SLOT(changePreviewFile(const QString&)));
	connect(this, SIGNAL(filterSelected(const QString &)), this, SLOT(selectFilter(const QString &)));
}

void ImportASCIIDialog::initAdvancedOptions()
{
	d_advanced_options = new QGroupBox();
	QVBoxLayout *main_layout = new QVBoxLayout(d_advanced_options);
	QGridLayout *advanced_layout = new QGridLayout();
	main_layout->addLayout(advanced_layout);

	advanced_layout->addWidget(new QLabel(tr("Import each file as: ")), 0, 0);
	d_import_mode = new QComboBox();
	d_import_mode->addItem(tr("New Table"));
	d_import_mode->addItem(tr("New Matrix"));
	d_import_mode->addItem(tr("New Columns"));
	d_import_mode->addItem(tr("New Rows"));
	d_import_mode->addItem(tr("Overwrite Current Window"));
	advanced_layout->addWidget(d_import_mode, 0, 1);

	QLabel *label_column_separator = new QLabel(tr("Separator:"));
	advanced_layout->addWidget(label_column_separator, 1, 0);
	d_column_separator = new QComboBox();
	d_column_separator->addItem(tr("TAB"));
	d_column_separator->addItem(tr("SPACE"));
	d_column_separator->addItem(";" + tr("TAB"));
	d_column_separator->addItem("," + tr("TAB"));
	d_column_separator->addItem(";" + tr("SPACE"));
	d_column_separator->addItem("," + tr("SPACE"));
	d_column_separator->addItem(";");
	d_column_separator->addItem(",");
	d_column_separator->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	d_column_separator->setEditable( true );
	advanced_layout->addWidget(d_column_separator, 1, 1);
	// context-sensitive help
	QString help_column_separator = tr("The column separator can be customized. \nThe following special codes can be used:\n\\t for a TAB character \n\\s for a SPACE");
	help_column_separator += "\n"+tr("The separator must not contain the following characters: \n0-9eE.+-");
	d_column_separator->setWhatsThis(help_column_separator);
	label_column_separator->setToolTip(help_column_separator);
	d_column_separator->setToolTip(help_column_separator);
	label_column_separator->setWhatsThis(help_column_separator);

	QLabel *label_ignore_lines = new QLabel(tr( "Ignore first" ));
	advanced_layout->addWidget(label_ignore_lines, 2, 0);
	d_ignored_lines = new QSpinBox();
	d_ignored_lines->setRange( 0, 10000 );
	d_ignored_lines->setSuffix(" " + tr("lines"));
	d_ignored_lines->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	advanced_layout->addWidget(d_ignored_lines, 2, 1);

	advanced_layout->addWidget(new QLabel(tr("Ignore lines containing")), 3, 0);
	d_comment_string = new QLineEdit();
    advanced_layout->addWidget(d_comment_string, 3, 1);

	QHBoxLayout *renameBox = new QHBoxLayout;
	d_rename_columns = new QCheckBox(tr("Use first row &as"));
	connect(d_rename_columns, SIGNAL(toggled(bool)), this, SLOT(enableComments()));
	renameBox->addWidget(d_rename_columns);

	d_first_line_role = new QComboBox();
	d_first_line_role->addItem(tr("Column Names"));
	d_first_line_role->addItem(tr("Column Comments"));
	connect(d_first_line_role, SIGNAL(activated(int)), this, SLOT(enableComments()));
	connect(d_first_line_role, SIGNAL(activated(int)), this, SLOT(preview()));
	renameBox->addWidget(d_first_line_role);
	advanced_layout->addLayout(renameBox, 0, 2, 1, 2);

    d_import_comments = new QCheckBox(tr("Use second row as &comments"));
	advanced_layout->addWidget(d_import_comments, 1, 2, 1, 2);

	d_strip_spaces = new QCheckBox(tr("&Remove white spaces from line ends"));
	advanced_layout->addWidget(d_strip_spaces, 2, 2, 1, 2);
	// context-sensitive help
	QString help_strip_spaces = tr("By checking this option all white spaces will be \nremoved from the beginning and the end of \nthe lines in the ASCII file.","when translating this check the what's this functions and tool tips to place the '\\n's correctly");
	help_strip_spaces +="\n\n"+tr("Warning: checking this option leads to column \noverlaping if the columns in the ASCII file don't \nhave the same number of rows.");
	help_strip_spaces +="\n"+tr("To avoid this problem you should precisely \ndefine the column separator using TAB and \nSPACE characters.","when translating this check the what's this functions and tool tips to place the '\\n's correctly");
	d_strip_spaces->setWhatsThis(help_strip_spaces);
	d_strip_spaces->setToolTip(help_strip_spaces);

	d_simplify_spaces = new QCheckBox(tr("&Simplify white spaces" ));
	advanced_layout->addWidget(d_simplify_spaces, 3, 2, 1, 2);
	// context-sensitive help
	QString help_simplify_spaces = tr("By checking this option all white spaces will be \nremoved from the beginning and the end of the \nlines and each sequence of internal \nwhitespaces (including the TAB character) will \nbe replaced with a single space.","when translating this check the what's this functions and tool tips to place the '\\n's correctly");
	help_simplify_spaces +="\n\n"+tr("Warning: checking this option leads to column \noverlaping if the columns in the ASCII file don't \nhave the same number of rows.","when translating this check the what's this functions and tool tips to place the '\\n's correctly");
	help_simplify_spaces +="\n"+tr("To avoid this problem you should precisely \ndefine the column separator using TAB and \nSPACE characters.","when translating this check the what's this functions and tool tips to place the '\\n's correctly");
	d_simplify_spaces->setWhatsThis(help_simplify_spaces);
	d_simplify_spaces->setToolTip(help_simplify_spaces);

	advanced_layout->addWidget(new QLabel(tr("Decimal Separators")), 4, 0);
	boxDecimalSeparator = new QComboBox();
	boxDecimalSeparator->addItem(tr("System Locale Setting"));
	boxDecimalSeparator->addItem("1,000.0");
	boxDecimalSeparator->addItem("1.000,0");
	boxDecimalSeparator->addItem("1 000,0");
	advanced_layout->addWidget(boxDecimalSeparator, 4, 1);

	d_omit_thousands_sep = new QCheckBox(tr("Omit &thousands separator"));
    //+++
    d_omit_thousands_sep->setEnabled(false);
    //---
	advanced_layout->addWidget(d_omit_thousands_sep, 4, 2, 1, 2);

	advanced_layout->addWidget(new QLabel(tr("Endline character")), 5, 0);
	boxEndLine = new QComboBox();
	boxEndLine->addItem(tr("LF (Unix)"));
	boxEndLine->addItem(tr("CRLF (Windows)"));
	boxEndLine->addItem(tr("CR (Mac)"));
    connect(boxEndLine, SIGNAL(activated(int)), this, SLOT(preview()));
	advanced_layout->addWidget(boxEndLine, 5, 1);

    d_read_only = new QCheckBox(tr("Import as &read-only"));
	advanced_layout->addWidget(d_read_only, 5, 2);

	d_preview_button = new QCheckBox(tr("&Preview Lines"));
	connect(d_preview_button, SIGNAL(clicked()), this, SLOT(preview()));
	advanced_layout->addWidget(d_preview_button, 6, 0);

	d_preview_lines_box = new QSpinBox();
	d_preview_lines_box->setMaximum (INT_MAX);
	d_preview_lines_box->setValue(100);
	d_preview_lines_box->setSingleStep(10);
	d_preview_lines_box->setSpecialValueText(tr("All"));
	advanced_layout->addWidget(d_preview_lines_box, 6, 1);

	QHBoxLayout *btnsLayout = new QHBoxLayout();
	d_col_types_button = new QPushButton(tr("Column T&ypes..."));
	connect(d_col_types_button, SIGNAL(clicked()), this, SLOT(showColTypeDialog()));
	d_col_types_button->hide();
	btnsLayout->addWidget(d_col_types_button);

	d_help_button = new QPushButton(tr("&Help"));
	connect(d_help_button, SIGNAL(clicked()), this, SLOT(displayHelp()));
	btnsLayout->addWidget(d_help_button);
	advanced_layout->addLayout(btnsLayout, 6, 2);

	d_preview_table = nullptr;
	d_preview_matrix = nullptr;
	d_preview_stack = new QStackedWidget();
	main_layout->addWidget(d_preview_stack);
}

void ImportASCIIDialog::showColTypeDialog()
{
	if (d_preview_table){
		d_preview_table->setSelectedColumn(0);
		d_preview_table->showColTypeDialog();
	}
}

void ImportASCIIDialog::initPreview(int previewMode)
{
	if (previewMode < NewTables || previewMode > Overwrite)
		return;

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!app)
		return;

	if (d_preview_table){
		delete d_preview_table;
		d_preview_table = nullptr;
	}

	if (d_preview_matrix){
		delete d_preview_matrix;
		d_preview_matrix = nullptr;
	}

	switch(previewMode){
		case NewTables:
			d_preview_table = new PreviewTable(30, 2, this);
			d_preview_table->setNumericPrecision(app->d_decimal_digits);
			d_preview_stack->addWidget(d_preview_table);
			connect(d_preview_table, SIGNAL(modifiedColumnType()), this, SLOT(preview()));
			enableTableOptions(true);
		break;

		case NewMatrices:
			d_preview_matrix = new PreviewMatrix(app);
			d_preview_stack->addWidget(d_preview_matrix);
			enableTableOptions(false);
		break;

		case NewColumns:
		case NewRows:
		case Overwrite:
			MdiSubWindow *w = app->activeWindow();
			if (!w)
				return;

			if (w->inherits("Table")){
				d_preview_table = new PreviewTable(30, ((Table*)w)->numCols(), this);
				d_preview_table->setNumericPrecision(app->d_decimal_digits);
				d_preview_stack->addWidget(d_preview_table);
				connect(d_preview_table, SIGNAL(modifiedColumnType()), this, SLOT(preview()));
				enableTableOptions(true);
			} else if (w->metaObject()->className() == "Matrix"){
				d_preview_matrix = new PreviewMatrix(app, (Matrix *)w);
				d_preview_stack->addWidget(d_preview_matrix);
				enableTableOptions(false);
			}
		break;
	}
	preview();
}

void ImportASCIIDialog::enableTableOptions(bool on)
{
	d_rename_columns->setEnabled(on);
	d_import_comments->setEnabled(on && d_rename_columns->isChecked());
	d_read_only->setEnabled(on);
	d_col_types_button->setVisible(on);
}

void ImportASCIIDialog::setColumnSeparator(const QString& sep)
{
	if (sep=="\t")
		d_column_separator->setCurrentIndex(0);
	else if (sep==" ")
		d_column_separator->setCurrentIndex(1);
	else if (sep==";\t")
		d_column_separator->setCurrentIndex(2);
	else if (sep==",\t")
		d_column_separator->setCurrentIndex(3);
	else if (sep=="; ")
		d_column_separator->setCurrentIndex(4);
	else if (sep==", ")
		d_column_separator->setCurrentIndex(5);
	else if (sep==";")
		d_column_separator->setCurrentIndex(6);
	else if (sep==",")
		d_column_separator->setCurrentIndex(7);
	else {
		QString separator = sep;
		d_column_separator->setEditText(separator.replace(" ","\\s").replace("\t","\\t"));
	}
}

const QString ImportASCIIDialog::columnSeparator() const
{
	QString sep = d_column_separator->currentText();

	if (d_simplify_spaces->isChecked())
		sep.replace(tr("TAB"), " ", Qt::CaseInsensitive);
	else
		sep.replace(tr("TAB"), "\t", Qt::CaseInsensitive);

	sep.replace(tr("SPACE"), " ", Qt::CaseInsensitive);
	sep.replace("\\s", " ");
	sep.replace("\\t", "\t");

	/* TODO
	if (sep.contains(QRegExp("[0-9.eE+-]")))
		QMessageBox::warning(this, tr("QtiSAS - Import options error"),
				tr("The separator must not contain the following characters: 0-9eE.+-"));
	*/

	return sep;
}

void ImportASCIIDialog::displayHelp()
{
	QString s = tr("The column separator can be customized. The following special codes can be used:\n\\t for a TAB character \n\\s for a SPACE");
	s += "\n"+tr("The separator must not contain the following characters: 0-9eE.+-") + "\n\n";
	s += tr( "Remove white spaces from line ends" )+ ":\n";
	s += tr("By checking this option all white spaces will be removed from the beginning and the end of the lines in the ASCII file.") + "\n\n";
	s += tr( "Simplify white spaces" )+ ":\n";
	s += tr("By checking this option each sequence of internal whitespaces (including the TAB character) will be replaced with a single space.");
	s += tr("By checking this option all white spaces will be removed from the beginning and the end of the lines and each sequence of internal whitespaces (including the TAB character) will be replaced with a single space.");

	s +="\n\n"+tr("Warning: using these two last options leads to column overlaping if the columns in the ASCII file don't have the same number of rows.");
	s +="\n"+tr("To avoid this problem you should precisely define the column separator using TAB and SPACE characters.");

	QMessageBox::about(this, tr("QtiSAS - Help"), s);
}

void ImportASCIIDialog::updateImportMode(int mode)
{
	setUpdatesEnabled(false);

	if (mode == Overwrite)
		setFileMode( QFileDialog::ExistingFile );
	else
		setFileMode( QFileDialog::ExistingFiles );

	initPreview(mode);

	setUpdatesEnabled(true);
	repaint();
}

void ImportASCIIDialog::closeEvent(QCloseEvent* e)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (app){
		app->d_extended_import_ASCII_dialog = this->isExtended();
		app->d_ASCII_file_filter = this->selectedNameFilter();
		app->d_ASCII_import_preview = d_preview_button->isChecked();
		app->d_preview_lines = d_preview_lines_box->value();
		app->d_ASCII_import_first_row_role = d_first_line_role->currentIndex();
		app->d_import_ASCII_dialog_size = size();
	}

	e->accept();
}

QLocale ImportASCIIDialog::decimalSeparators()
{
	QLocale locale;
    switch (boxDecimalSeparator->currentIndex()){
        case 0:
            locale = QLocale::system();
        break;
        case 1:
            locale = QLocale::c();
        break;
        case 2:
            locale = QLocale(QLocale::German);
        break;
        case 3:
            locale = QLocale(QLocale::French);
        break;
    }
//+++
	//if (d_omit_thousands_sep->isChecked())
	//	locale.setNumberOptions(QLocale::OmitGroupSeparator);
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
//---
	return locale;
}

void ImportASCIIDialog::preview()
{
    if (!d_preview_button->isChecked()){
		d_preview_stack->hide();
        return;
    } else
		d_preview_stack->show();

	if (d_preview_table)
		previewTable();
	else if (d_preview_matrix)
		previewMatrix();
}

void ImportASCIIDialog::previewTable()
{
	if (!d_preview_table)
		return;

	if (!d_preview_table->isVisible())
		d_preview_table->show();

	if (d_current_path.trimmed().isEmpty()){
		d_preview_table->clear();
		d_preview_table->resetHeader();
		return;
	}

	int importMode = d_import_mode->currentIndex();
	if (importMode == NewTables)
		importMode = Table::Overwrite;
	else
		importMode -= 2;

	d_preview_table->clear();
	d_preview_table->resetHeader();

	d_preview_table->importASCII(d_current_path, columnSeparator(), d_ignored_lines->value(),
						renameColumns(), d_strip_spaces->isChecked(),
						d_simplify_spaces->isChecked(), importComments(),
						d_comment_string->text(), (Table::ImportMode)importMode, decimalSeparators(),
						boxEndLine->currentIndex(), d_preview_lines_box->value());

    if (!d_preview_table->isVisible())
        d_preview_table->show();
}

void ImportASCIIDialog::previewMatrix()
{
	if (!d_preview_matrix)
		return;

	if (d_current_path.trimmed().isEmpty()){
		d_preview_matrix->clear();
        return;
    }

	int importMode = d_import_mode->currentIndex();
	if (importMode == NewMatrices)
		importMode = Matrix::Overwrite;
	else
		importMode -= 2;

	d_preview_matrix->importASCII(d_current_path, columnSeparator(), d_ignored_lines->value(),
							d_strip_spaces->isChecked(), d_simplify_spaces->isChecked(),
                            d_comment_string->text(), importMode, decimalSeparators(),
                            boxEndLine->currentIndex(), d_preview_lines_box->value());
	d_preview_matrix->resizeColumnsToContents();
}

void ImportASCIIDialog::setCurrentPath(const QString& path)
{
	d_current_path = path;
	changePreviewFile(path);
}

void ImportASCIIDialog::changePreviewFile(const QString& path)
{
	if (path.isEmpty())
		return;

	QFileInfo fi(path);
	if (!fi.exists() || fi.isDir() || !fi.isFile())
		return;

	if (!fi.isReadable()){
		QMessageBox::critical(this, tr("QtiSAS - File openning error"),
		tr("You don't have the permission to open this file: <b>%1</b>").arg(path));
		return;
	}

	d_current_path = path;
	preview();
}

void ImportASCIIDialog::setNewWindowsOnly(bool on)
{
    if (on){
        d_import_mode->clear();
        d_import_mode->addItem(tr("New Table"));
        d_import_mode->addItem(tr("New Matrice"));
    }

    d_preview_button->setChecked(false);
}

bool ImportASCIIDialog::importComments()
{
	if (!d_rename_columns->isChecked())
		return false;

	return (d_first_line_role->currentIndex() == 1) ||
			(d_import_comments->isChecked() && d_import_comments->isEnabled());

}

void ImportASCIIDialog::enableComments()
{
	d_import_comments->setEnabled(d_rename_columns->isChecked() &&
								!d_first_line_role->currentIndex());
}

void ImportASCIIDialog::selectFilter(const QString & filter)
{
	QStringList filters;
	filters << tr("All files") + " (*)";
	filters << tr("Text files") + " (*.TXT *.txt)";
	filters << tr("Data files") + " (*.DAT *.dat)";
//	filters << tr("Comma Separated Values") + " (*.CSV *.csv)";

	if (!filters.contains(filter))
		filters << filter;

	blockSignals(true);
	setNameFilters(filters);
	selectNameFilter(filter);
	blockSignals(false);
}

/*****************************************************************************
 *
 * Class PreviewTable
 *
 *****************************************************************************/

PreviewTable::PreviewTable(int numRows, int numCols, QWidget * parent)
: QTableWidget(numRows, numCols, parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setSelectionMode(QAbstractItemView::NoSelection);
	verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	horizontalHeader()->installEventFilter(this);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

	for (int i = 0; i < numCols; i++){
		comments << "";
		col_label << QString::number(i + 1);
		colTypes << Table::Numeric;
		d_col_format << QString();
	}

	d_start_col = numCols;
	setHeader();
	setMinimumHeight(2*horizontalHeader()->height());
	connect(horizontalHeader(), SIGNAL(sectionResized(int, int, int)), this, SLOT(setHeader()));
}

void PreviewTable::importASCII(const QString &fname, const QString &sep, int ignoredLines, bool renameCols,
    bool stripSpaces, bool simplifySpaces, bool importComments, const QString& commentString,
	int importMode, const QLocale& importLocale, int endLine, int maxRows)
{
	int rows;
	QString name = MdiSubWindow::parseAsciiFile(fname, commentString, endLine, ignoredLines, maxRows, rows);
	if (name.isEmpty())
		return;

	QFile f(name);
	if (!f.open(QIODevice::ReadOnly))
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QLocale locale = this->locale();
	bool updateDecimalSeparators = (importLocale != locale) ? true : false;

	QTextStream t(&f);
	QString s = t.readLine();//read first line
	if (simplifySpaces)
		s = s.simplified();
	else if (stripSpaces)
		s = s.trimmed();

	QStringList line = s.split(sep);
	int cols = line.size();

	bool allNumbers = true;
	for (int i=0; i<cols; i++)
	{//verify if the strings in the line used to rename the columns are not all numbers
		locale.toDouble(line[i], &allNumbers);
		if (!allNumbers)
			break;
	}

	if (renameCols && !allNumbers)
		rows--;
	if (importComments)
		rows--;

	int startRow = 0, startCol = 0;
	int c = columnCount();
	int r = rowCount();
	switch(importMode){
		case Table::Overwrite:
			if (rowCount() != rows)
				setRowCount(rows);

			if (c != cols){
				if (c < cols)
					addColumns(cols - c);
				else
					setColumnCount(cols);
			}
		break;
		case Table::NewColumns:
			startCol = d_start_col;
			//d_start_col += cols;
			if (abs(c - d_start_col) < cols)
				addColumns(cols);

			if (r < rows)
				setRowCount(rows);
		break;
		case Table::NewRows:
			startRow = r;
			if (c < cols)
				addColumns(cols - c);
			setRowCount(r + rows);
		break;
	}

	if (renameCols && !allNumbers){//use first line to set the table header
		for (int i = 0; i<cols; i++){
			int aux = i + startCol;
			col_label[aux] = QString::null;
			if (!importComments)
				comments[aux] = line[i];
			s = line[i].replace("-","_").remove(QRegExp("\\W")).replace("_","-");
			int n = col_label.count(s);
			if(n){//avoid identical col names
				while (col_label.contains(s + QString::number(n)))
					n++;
				s += QString::number(n);
			}
			col_label[aux] = s;
		}
	}

	if (importComments){//import comments
		if (renameCols && !allNumbers)
			s = t.readLine();//read 2nd line

		if (simplifySpaces)
			s = s.simplified();
		else if (stripSpaces)
			s = s.trimmed();
		line = s.split(sep, QString::KeepEmptyParts);
		for (int i=0; i<line.size(); i++){
			int aux = startCol + i;
			if (aux < comments.size())
				comments[aux] = line[i];
		}
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	if ((!renameCols || allNumbers)&& !importComments && rows > 0){
		//put values in the first line of the table
		for (int i = 0; i < cols; i++){
			QString cell = line[i];
			if (cell.isEmpty())
				continue;

			bool ok;
			double val = importLocale.toDouble(cell, &ok);
            QTableWidgetItem *item;
			if (colTypes[startCol + i] == Table::Numeric && (ok || updateDecimalSeparators))
                item = new QTableWidgetItem(locale.toString(val, 'g', d_numeric_precision));
            else
                item = new QTableWidgetItem(cell);
            setItem(startRow, startCol + i, item);
		}
		startRow++;
	}

	blockSignals(true);
	setHeader();

	QApplication::restoreOverrideCursor();

	int row = startRow;
	rows = rowCount();
	while (!t.atEnd() && row < rows){
		s = t.readLine();
		if (simplifySpaces)
			s = s.simplified();
		else if (stripSpaces)
			s = s.trimmed();
		line = s.split(sep);
		int lc = line.size();
		if (lc > cols) {
			addColumns(lc - cols);
			cols = lc;
		}
		for (int j = 0; j < cols && j < lc; j++){
			QString cell = line[j];
			if (cell.isEmpty())
				continue;

			bool ok;
			double val = importLocale.toDouble(cell, &ok);
            QTableWidgetItem *item;
			if (colTypes[startCol + j] == Table::Numeric && (ok || updateDecimalSeparators))
                item = new QTableWidgetItem(locale.toString(val, 'g', d_numeric_precision));
			else
                item = new QTableWidgetItem(cell);
            setItem(row, startCol + j, item);
		}

		row++;
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	blockSignals(false);
	f.remove();
}

void PreviewTable::resetHeader()
{
	int cols = columnCount();
	for (int i = 0; i < cols; i++){
		comments[i] = QString::null;
		col_label[i] = QString::number(i+1);
	}

	int size = colTypes.size();
	if (size > cols){
		for (int i = 0; i < (size - cols); i++)
			colTypes.pop_back();
	}

	size = d_col_format.size();
	if (size > cols){
		for (int i = 0; i < (size - cols); i++)
			d_col_format.pop_back();
	}
}

void PreviewTable::clear()
{
    QTableWidgetItem* item;
	for (int i=0; i<columnCount(); i++)
		for (int j=0; j<rowCount(); j++) {
            item = new QTableWidgetItem(QString::null);
            setItem(j, i, item);
        }
}

void PreviewTable::setHeader()
{
    QHeaderView *head = horizontalHeader();
    QStringList labels;
	for (int i=0; i<columnCount(); i++){
		QString s = col_label[i];
        int lines = columnWidth(i)/head->fontMetrics().boundingRect("_").width();
        labels.append(s.remove("\n") + "\n" + QString(lines, '_') + "\n" + comments[i]);
	}
    setHorizontalHeaderLabels(labels);
}

void PreviewTable::addColumns(int c)
{
	int max=0, cols = columnCount();
	for (int i=0; i<cols; i++){
		if (!col_label[i].contains(QRegExp ("\\D"))){
			int index=col_label[i].toInt();
			if (index>max)
				max=index;
		}
	}
	max++;
    for (int i = 0; i < c; i++)
	    insertColumn(cols);
	for (int i = 0; i < c; i++){
		comments << QString();
		col_label << QString::number(max+i);
		colTypes << Table::Numeric;
		d_col_format << QString();
	}
}

bool PreviewTable::eventFilter(QObject *object, QEvent *e)
{
	QHeaderView *hheader = horizontalHeader();
	if (e->type() == QEvent::MouseButtonPress && object == (QObject*)hheader) {
		const QMouseEvent *me = (const QMouseEvent *)e;
		clearSelection();
        int logicalIndex = hheader->logicalIndexAt(me->pos().x() + hheader->offset());
        int col = hheader->visualIndex(logicalIndex);
		if (col >= 0 && col < columnCount()) {
            int xPos = hheader->sectionViewportPosition(col);
            int width = hheader->sectionSize(col);
            QRect rect(xPos, 0, width, hheader->height());
			rect.setLeft(rect.left() + 2);
			rect.setRight(rect.right() - 2);
			if (rect.contains (me->pos())){
				selectColumn(col);
				d_selected_column = col;
				showColTypeDialog();
				return true;
			}
		}
	}
	return QTableWidget::eventFilter(object, e);
}

void PreviewTable::setSelectedColumn(int col)
{
	selectColumn(col);
	d_selected_column = col;
}

void PreviewTable::setColumnFormat(const QString& format)
{
	d_col_format[d_selected_column] = format;
}

void PreviewTable::setColumnType(int type)
{
	bool vis = type > Table::Text;
	formatBox->setVisible(vis);
	formatLabel->setVisible(vis);

	formatBox->blockSignals(true);
	formatBox->clear();
	formatBox->setEditable(false);
	switch (type){
		case Table::Date:
			formatBox->addItem("dd/MM/yyyy");
			formatBox->addItem("dd/MM/yyyy HH:mm");
			formatBox->addItem("dd/MM/yyyy HH:mm:ss");

			formatBox->addItem("dd.MM.yyyy");
			formatBox->addItem("dd.MM.yyyy HH:mm");
			formatBox->addItem("dd.MM.yyyy HH:mm:ss");

			formatBox->addItem("dd MM yyyy");
			formatBox->addItem("dd MM yyyy HH:mm");
			formatBox->addItem("dd MM yyyy HH:mm:ss");

			formatBox->addItem("yyyy-MM-dd");
			formatBox->addItem("yyyy-MM-dd HH:mm");
			formatBox->addItem("yyyy-MM-dd HH:mm:ss");

			formatBox->addItem("yyyyMMdd");
			formatBox->addItem("yyyyMMdd HH:mm");
			formatBox->addItem("yyyyMMdd HH:mm:ss");

			formatBox->setEditable(true);
			formatBox->setEditText(d_col_format[d_selected_column]);
		break;

		case Table::Time:
			formatBox->addItem("h");
			formatBox->addItem("h ap");
			formatBox->addItem("h AP");
			formatBox->addItem("h:mm");
			formatBox->addItem("h:mm ap");
			formatBox->addItem("hh:mm");
			formatBox->addItem("h:mm:ss");
			formatBox->addItem("h:mm:ss.zzz");
			formatBox->addItem("mm:ss");
			formatBox->addItem("mm:ss.zzz");
			formatBox->addItem("hmm");
			formatBox->addItem("hmmss");
			formatBox->addItem("hhmmss");

			formatBox->setEditable(true);
			formatBox->setEditText(d_col_format[d_selected_column]);
		break;

		default:
		break;
	}

	colTypes[d_selected_column] = type;
	formatBox->blockSignals(false);
	modifiedColumnType();
}

void PreviewTable::showColTypeDialog()
{
	QDialog *colTypeDialog = new QDialog(this);
	colTypeDialog->setAttribute(Qt::WA_DeleteOnClose);
	colTypeDialog->setSizeGripEnabled(true);
	colTypeDialog->setMinimumWidth(300);
	colTypeDialog->setWindowTitle(tr("Choose column type"));

	gb1 = new QGroupBox();
	QGridLayout *hl1 = new QGridLayout(gb1);
	hl1->addWidget(new QLabel(tr("Type")), 0, 0);
	typesBox = new QComboBox();
	typesBox->addItems(QStringList() << tr("Numeric") << tr("Text") << tr("Date") << tr("Time"));
	hl1->addWidget(typesBox, 0, 1);

	formatLabel = new QLabel(tr("Format"));
	hl1->addWidget(formatLabel, 1, 0);
	formatLabel->hide();

	formatBox = new QComboBox();
	hl1->addWidget(formatBox, 1, 1);
	formatBox->hide();
	hl1->setColumnStretch(1, 1);
	hl1->setRowStretch(2, 1);

	buttonPrev = new QPushButton("&<<");
	buttonPrev->setAutoDefault(false);
	buttonPrev->setMaximumWidth(40);

	buttonNext = new QPushButton("&>>");
	buttonNext->setAutoDefault(false);
	buttonNext->setMaximumWidth(40);

	QPushButton *closeBtn = new QPushButton(tr("&Close"));
	connect(closeBtn, SIGNAL(clicked()), colTypeDialog, SLOT(reject()));

	QHBoxLayout *hl2 = new QHBoxLayout();
	hl2->addStretch();
	hl2->addWidget(buttonPrev);
	hl2->addWidget(buttonNext);
	hl2->addWidget(closeBtn);

	QVBoxLayout *vl = new QVBoxLayout(colTypeDialog);
	vl->addWidget(gb1);
	vl->addLayout(hl2);

	updateColumn(d_selected_column);

	connect(typesBox, SIGNAL(currentIndexChanged (int)), this, SLOT(setColumnType(int)));
	connect(formatBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setColumnFormat(const QString&)));
	connect(formatBox, SIGNAL(editTextChanged(const QString&)), this, SLOT(setColumnFormat(const QString&)));
	connect(buttonPrev, SIGNAL(clicked()), this, SLOT(prevColumn()));
	connect(buttonNext, SIGNAL(clicked()), this, SLOT(nextColumn()));
	colTypeDialog->exec();
}

void PreviewTable::prevColumn()
{
	updateColumn(--d_selected_column);
}

void PreviewTable::nextColumn()
{
	updateColumn(++d_selected_column);
}

void PreviewTable::updateColumn(int sc)
{
	if (sc < 0 || sc >= columnCount())
		return;

	typesBox->setCurrentIndex(colTypes[sc]);
	setColumnType(typesBox->currentIndex());

	gb1->setTitle(tr("Column") + " " + QString::number(sc + 1));

	if (!sc)
		buttonPrev->setEnabled(false);
	else
		buttonPrev->setEnabled(true);

	if (sc >= columnCount() - 1)
		buttonNext->setEnabled(false);
	else
		buttonNext->setEnabled(true);

	clearSelection();
	selectColumn(sc);
}
/*****************************************************************************
 *
 * Class PreviewMatrix
 *
 *****************************************************************************/

PreviewMatrix::PreviewMatrix(QWidget *parent, Matrix * m):QTableView(parent)
{
	d_matrix_model = new MatrixModel(32, 32, m);
	if (!m){
		ApplicationWindow *app = (ApplicationWindow *)parent;
		if (app){
			d_matrix_model->setLocale(app->locale());
			d_matrix_model->setNumericFormat('f', app->d_decimal_digits);
		}
	}
	setModel(d_matrix_model);

	setAttribute(Qt::WA_DeleteOnClose);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFocusPolicy(Qt::NoFocus);

    QPalette pal = palette();
	pal.setColor(QPalette::Base, QColor(255, 255, 128));
	setPalette(pal);

	// set header properties
	horizontalHeader()->setSectionsMovable(false);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	for(int i=0; i<d_matrix_model->columnCount(); i++)
		setColumnWidth(i, 100);

	verticalHeader()->setSectionsMovable(false);
	verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	setMinimumHeight(4*horizontalHeader()->height());
}

void PreviewMatrix::importASCII(const QString &fname, const QString &sep, int ignoredLines,
    				bool stripSpaces, bool simplifySpaces, const QString& commentString,
					int importAs, const QLocale& locale, int endLine, int maxRows)
{
	d_matrix_model->importASCII(fname, sep, ignoredLines, stripSpaces,
		simplifySpaces, commentString, importAs, locale, endLine, maxRows);
	reset();
}

void PreviewMatrix::clear()
{
	d_matrix_model->clear();
	reset();
}
