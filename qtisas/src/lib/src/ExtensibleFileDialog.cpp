/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QFileDialog plus generic extension support
 ******************************************************************************/

#include <QComboBox>
#include <QGridLayout>
#include <QUrl>

#include "ExtensibleFileDialog.h"

ExtensibleFileDialog::ExtensibleFileDialog(QWidget *parent, bool extended, Qt::WindowFlags flags)
	: QFileDialog(parent, flags)
{
	//+++ 2023:  force do not use Native Dialog: layout()=0 in the case of Native Dialog
	setOption(QFileDialog::DontUseNativeDialog, true);
	//---
	d_extension = 0;

	d_extension_toggle = new QPushButton();
	d_extension_toggle->setCheckable(true);
	d_extension_toggle->hide(); // show only for d_extension != 0
	setExtended(extended);

	QGridLayout *main_layout = qobject_cast<QGridLayout*>(layout());
	if (main_layout) {
		d_extension_row = main_layout->rowCount();
		main_layout->addWidget(d_extension_toggle, d_extension_row, main_layout->columnCount()-1);
		main_layout->setRowStretch(d_extension_row, 0);
		main_layout->setRowStretch(d_extension_row+1, 0);
	} else {
		// fallback in case QFileDialog uses a different layout in the future (=> main_layout==0)
		// would probably look a mess, but at least all functions would be accessible
		layout()->addWidget(d_extension_toggle);
	}

	connect(this, SIGNAL(accepted()), this, SLOT(close()));
    connect(this, SIGNAL(rejected()), this, SLOT(close()));
}

void ExtensibleFileDialog::setExtensionWidget(QWidget *extension)
{
	if (d_extension == extension)
		return;
	if (d_extension) {
		d_extension->hide();
		disconnect(d_extension_toggle, SIGNAL(toggled(bool)));
	}
	d_extension = extension;
	if (!d_extension) {
		d_extension_toggle->hide();
		return;
	}
	d_extension_toggle->show();

	QGridLayout *main_layout = qobject_cast<QGridLayout*>(layout());
	if (main_layout)
		main_layout->addWidget(d_extension, d_extension_row, 0, 2, main_layout->columnCount()-1);
	else
		layout()->addWidget(d_extension);

	d_extension->setVisible(d_extension_toggle->isChecked());
	connect(d_extension_toggle, SIGNAL(toggled(bool)), d_extension, SLOT(setVisible(bool)));
	connect(d_extension_toggle, SIGNAL(toggled(bool)), this, SLOT(updateToggleButtonText(bool)));
}

void ExtensibleFileDialog::setEditableFilter(bool on)
{
	QLayout *main_layout = layout();
	if (!main_layout)
		return;

	for (int i = 0; i < main_layout->count(); i++){
		QLayoutItem *item = main_layout->itemAt(i);
		if (!item)
			continue;
		QComboBox *filterBox = qobject_cast<QComboBox*>(item->widget());
		if (filterBox){
			filterBox->setEditable(on);
			connect(filterBox, SIGNAL(editTextChanged(const QString &)),
					this, SIGNAL(filterSelected(const QString &)));
			return;
		}
	}
}

void ExtensibleFileDialog::updateToggleButtonText(bool toggled)
{
	QString s = tr("&Advanced");
	if (toggled)
		s += " >>";
	else
		s = tr("<< &Advanced");
	d_extension_toggle->setText(s);
}

void ExtensibleFileDialog::setExtended(bool extended)
{
	updateToggleButtonText(extended);
	if (extended)
		d_extension_toggle->toggle();
}
