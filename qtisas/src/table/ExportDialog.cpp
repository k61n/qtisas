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

#include <iostream>

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>

#include "ApplicationWindow.h"
#include "ExportDialog.h"
#include "Matrix.h"
#include "MdiSubWindow.h"

ExportDialog::ExportDialog(MdiSubWindow *window, QWidget * parent, bool extended, Qt::WindowFlags flags)
: ExtensibleFileDialog( parent, extended, flags ), d_window(window)
{
	setWindowTitle( tr( "QtiSAS - Export ASCII" ) );
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );
	setAcceptMode(QFileDialog::AcceptSave);

	initAdvancedOptions();
	setExtensionWidget((QWidget *)d_advanced_options);

	setFileTypeFilters();
	setFileMode(QFileDialog::AnyFile);
	if (d_window){
		boxTable->setCurrentIndex(boxTable->findText(d_window->objectName()));
		selectFile(d_window->objectName());
	}

	connect(this, SIGNAL(filterSelected ( const QString & )),
			this, SLOT(updateAdvancedOptions ( const QString & )));

	selectNameFilter(((ApplicationWindow *)parent)->d_export_ASCII_file_filter);
	updateAdvancedOptions(selectedNameFilter());
}

void ExportDialog::initAdvancedOptions()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	d_advanced_options = new QGroupBox();

	QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("Table")), 0, 0);
	boxTable = new QComboBox();
	QStringList tables = app->tableNames() + app->matrixNames();
	boxTable->addItems(tables);

	boxTable->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	gl1->addWidget(boxTable, 0, 1);

    
    QHBoxLayout *h1 = new QHBoxLayout;
	boxAllTables = new QCheckBox(tr( "&All" ));
    boxAllTables->setChecked(false);
    boxAllTables->setMaximumWidth(40);
    h1->addWidget( boxAllTables );

    fileWildCard = new QLineEdit("*");
    fileWildCard->setEnabled(false);
    fileWildCard->setHidden(true);
    fileWildCard->setMaximumWidth(55);
    
    QString helpWildCard = "Wild Card Filter to select Tables/MAtrixes for export.";
    helpWildCard += "\n"+tr("For example:");
    helpWildCard += "\n"+tr("QI-*: to export all radial averaged tables generated in DAN");
    helpWildCard += "\n"+tr("I-*:  to export all Matrixes generated in DAN");
    helpWildCard += "\n"+tr("...");
    fileWildCard->setWhatsThis(helpWildCard);
    fileWildCard->setToolTip(helpWildCard);
    h1->addWidget( fileWildCard );
    
    gl1->addLayout(h1, 0, 2);
    
    separatorLbl = new QLabel( tr( "Separator" ) );
	gl1->addWidget(separatorLbl, 1, 0);

    boxSeparator = new QComboBox();
	boxSeparator->addItem(tr("TAB"));
    boxSeparator->addItem(tr("SPACE"));
	boxSeparator->addItem(";" + tr("TAB"));
	boxSeparator->addItem("," + tr("TAB"));
	boxSeparator->addItem(";" + tr("SPACE"));
	boxSeparator->addItem("," + tr("SPACE"));
    boxSeparator->addItem(";");
    boxSeparator->addItem(",");
	boxSeparator->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	boxSeparator->setEditable( true );
	gl1->addWidget(boxSeparator, 1, 1);
	setColumnSeparator(app->d_export_col_separator);

	buttonHelp = new QPushButton(tr( "&Help" ));
    buttonHelp->setMaximumWidth(100);
	gl1->addWidget( buttonHelp, 1, 2);

	QString help = tr("The column separator can be customized. The following special codes can be used:\n\\t for a TAB character \n\\s for a SPACE");
	help += "\n"+tr("The separator must not contain the following characters: 0-9eE.+-");

	boxSeparator->setWhatsThis(help);
	separatorLbl->setWhatsThis(help);
	boxSeparator->setToolTip(help);
	separatorLbl->setToolTip(help);

	boxNames = new QCheckBox(tr( "Include Column &Names" ));
    boxNames->setChecked( app->d_export_col_names );
	boxNames->setVisible(d_window && d_window->inherits("Table"));

	boxComments = new QCheckBox(tr( "Include Column Co&mments" ));
    boxComments->setChecked( app->d_export_col_comment );
	boxComments->setVisible(d_window && d_window->inherits("Table"));

    boxSelection = new QCheckBox(tr( "Export &Selection" ));
    boxSelection->setChecked( app->d_export_table_selection );

	QVBoxLayout *vl1 = new QVBoxLayout(d_advanced_options);
	vl1->addLayout( gl1 );
	vl1->addWidget( boxNames );
	vl1->addWidget( boxComments );
	vl1->addWidget( boxSelection );

    // signals and slots connections
    connect(boxTable, &QComboBox::textActivated, this, &ExportDialog::updateOptions);
    connect( buttonHelp, SIGNAL(clicked()), this, SLOT(help()));
	connect( boxAllTables, SIGNAL(toggled(bool)), this, SLOT( enableTableName(bool)));
}

void ExportDialog::updateAdvancedOptions (const QString & filter)
{
	bool on = !filter.contains(".tex") && !filter.contains(".odf") && !filter.contains(".html") &&
			  !filter.contains(".xls") && !filter.contains(".ods") && !filter.contains(".csv");
	separatorLbl->setVisible(on);
	boxSeparator->setVisible(on);
	buttonHelp->setVisible(on);
}

void ExportDialog::help()
{
	QString s = tr("The column separator can be customized. The following special codes can be used:\n\\t for a TAB character \n\\s for a SPACE");
	s += "\n"+tr("The separator must not contain the following characters: 0-9eE.+-");
	QMessageBox::about((ApplicationWindow *)parent(), tr("QtiSAS - Help"), s);
}

void ExportDialog::enableTableName(bool ok)
{
//+++2020.04
    selectFile(" \\.");
#ifdef Q_OS_MACOS

    setFocusProxy(boxSeparator);
    setFocus();
    selectFile("\\.");
#endif
#ifdef Q_OS_LINUX
    selectFile("\\.");
#endif
//---
    fileWildCard->setEnabled(ok);
    fileWildCard->setHidden(!ok);
	QString selected_filter = selectedNameFilter();
	boxTable->setEnabled(!ok);
    
	if (!ok)
    {
		setFileMode(QFileDialog::AnyFile);
		if (d_window)
        {
            selectFile(boxTable->currentText());
		}
	}
    else
    {
        setFileMode(QFileDialog::Directory);
        setDirectory(directory().path());
    }
	setFileTypeFilters();
	selectNameFilter(selected_filter);
}

void ExportDialog::setFileTypeFilters()
{
	QList<QByteArray> list;
    //	list << "CSV"; //+++2020 : not
	list << "DAT";
	list << "TXT";
	list << "TEX";
	list << "ODF";
	list << "HTML";
	list << "ODS";
	list << "XLS";

	QStringList filters;
	for(int i = 0 ; i < list.count() ; i++)
		filters << "*." + list[i].toLower();

	filters.sort();
	setNameFilters(filters);
}

void ExportDialog::accept()
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!app)
		return;

	QString sep = boxSeparator->currentText();
	sep.replace(tr("TAB"), "\t", Qt::CaseInsensitive);
	sep.replace(tr("SPACE"), " ");
	sep.replace("\\s", " ");
	sep.replace("\\t", "\t");

    static const QRegularExpression re("[0-9.eE+-]");
    if (sep.contains(re))
    {
		QMessageBox::warning(0, tr("QtiSAS - Import options error"),
				tr("The separator must not contain the following characters: 0-9eE.+-"));
		return;
	}

	app->asciiDirPath = directory().path();
    
	if (selectedFiles().isEmpty())
		return;

	QString selected_filter = selectedNameFilter().remove("*");
    if (boxAllTables->isChecked())
    {
		//app->exportAllTables(directory().absolutePath(), selected_filter, sep, boxNames->isChecked(), boxComments->isChecked(), boxSelection->isChecked(), fileWildCard->text());
        app->exportAllTables(selectedFiles()[0], selected_filter, sep, boxNames->isChecked(), boxComments->isChecked(), boxSelection->isChecked(), fileWildCard->text());

    }
	else {
		QString file_name = selectedFiles()[0];
		if(!file_name.endsWith(selected_filter, Qt::CaseInsensitive))
			file_name.append(selected_filter);

		if (app->d_confirm_overwrite && QFileInfo(file_name).exists() &&
			QMessageBox::warning(this, tr("QtiSAS") + " - " + tr("Overwrite file?"),
			tr("%1 already exists.").arg(file_name) + "\n" + tr("Do you want to replace it?"),
			QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
			return;

		QFile file(file_name);
		if ( !file.open( QIODevice::WriteOnly ) ){
			QMessageBox::critical(this, tr("QtiSAS - Export error"),
					tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
			return;
		}
		file.close();

		MdiSubWindow* w = app->window(boxTable->currentText());
		if (!w)
			return;

		if (w->inherits("Table"))
			((Table *)w)->exportASCII(file_name, sep, boxNames->isChecked(), boxComments->isChecked(), boxSelection->isChecked());
		else if (qobject_cast<Matrix *>(w))
			((Matrix *)w)->exportASCII(file_name, sep, boxSelection->isChecked());
	}

	close();
}

void ExportDialog::setColumnSeparator(const QString& sep)
{
	if (sep=="\t")
		boxSeparator->setCurrentIndex(0);
	else if (sep==" ")
		boxSeparator->setCurrentIndex(1);
	else if (sep==";\t")
		boxSeparator->setCurrentIndex(2);
	else if (sep==",\t")
		boxSeparator->setCurrentIndex(3);
	else if (sep=="; ")
		boxSeparator->setCurrentIndex(4);
	else if (sep==", ")
		boxSeparator->setCurrentIndex(5);
	else if (sep==";")
		boxSeparator->setCurrentIndex(6);
	else if (sep==",")
		boxSeparator->setCurrentIndex(7);
	else {
		QString separator = sep;
		boxSeparator->setEditText(separator.replace(" ","\\s").replace("\t","\\t"));
	}
}

void ExportDialog::closeEvent(QCloseEvent* e)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (app){
		app->d_export_col_names = boxNames->isChecked();
		app->d_export_table_selection = boxSelection->isChecked();
		app->d_export_col_comment = boxComments->isChecked();
		app->d_export_ASCII_file_filter = selectedNameFilter();

		QString sep = boxSeparator->currentText();
		sep.replace(tr("TAB"), "\t", Qt::CaseInsensitive);
		sep.replace(tr("SPACE"), " ");
		sep.replace("\\s", " ");
		sep.replace("\\t", "\t");
		app->d_export_col_separator = sep;
	}
	e->accept();
}

void ExportDialog::updateOptions(const QString & name)
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
        return;

    MdiSubWindow* w = app->window(name);
    if (!w)
		return;

    boxComments->setVisible(w->inherits("Table"));
    boxNames->setVisible(w->inherits("Table"));

    selectFile(boxTable->currentText());
}
