/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dialog for opening project files
 ******************************************************************************/

#include "OpenProjectDialog.h"
#include "ApplicationWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCloseEvent>

OpenProjectDialog::OpenProjectDialog(QWidget *parent, bool extended, Qt::WindowFlags flags)
	: ExtensibleFileDialog(parent, extended, flags)
{
	setWindowTitle(tr("QTISAS - Open Project"));
	setFileMode(ExistingFile);
	QStringList filters;
	filters << tr("QtiPlot project") + " (*.qti)"
		<< tr("Compressed QtiPlot project") + " (*.qti.gz)"
#ifdef ORIGIN_IMPORT
    << tr("Origin project") + " (*.opj *.OPJ)"
    << tr("Origin matrix") + " (*.ogm *.OGM)"
    << tr("Origin worksheet") + " (*.ogw *.OGW)"
    << tr("Origin graph") + " (*.ogg *.OGG)"
    << tr("Origin 3.5 project") + " (*.org *.ORG)"
#endif
		<< tr("Backup files") + " (*.qti~)";

	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(parent);

	setNameFilters(filters);

	QWidget *advanced_options = new QWidget();
	QHBoxLayout *advanced_layout = new QHBoxLayout();
	advanced_options->setLayout(advanced_layout);
	advanced_layout->addWidget(new QLabel(tr("Open As")));
	d_open_mode = new QComboBox();
	// Important: Keep this is sync with enum OpenMode.
	d_open_mode->addItem(tr("New Project Window"));
	d_open_mode->addItem(tr("New Folder"));
	advanced_layout->addWidget(d_open_mode);
	setExtensionWidget(advanced_options);

	connect(this, SIGNAL(filterSelected ( const QString & )),
			this, SLOT(updateAdvancedOptions ( const QString & )));

	if (app){
		selectNameFilter(app->d_open_project_filter);
		updateAdvancedOptions(app->d_open_project_filter);
	}
}

void OpenProjectDialog::updateAdvancedOptions (const QString & filter)
{
	d_extension_toggle->setEnabled(true);
}

void OpenProjectDialog::closeEvent(QCloseEvent* e)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (app){
		if (isExtendable())
			app->d_extended_open_dialog = this->isExtended();
		app->d_open_project_filter = selectedNameFilter();
	}

	e->accept();
}
