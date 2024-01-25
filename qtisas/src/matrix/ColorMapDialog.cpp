/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtLinearColorMap editor dialog
 ******************************************************************************/

#include "ColorMapDialog.h"
#include "ColorMapEditor.h"
#include <Matrix.h>
#include <MatrixCommand.h>
#include <ApplicationWindow.h>

#include <QPushButton>
#include <QLayout>

ColorMapDialog::ColorMapDialog(QWidget* parent, Qt::WindowFlags fl)
	: QDialog(parent, fl)
{
	setObjectName( "ColorMapDialog" );
	setWindowTitle(tr("QtiSAS") + " - " + tr("Custom Color Map"));
	setSizeGripEnabled(true);
	setAttribute(Qt::WA_DeleteOnClose);

	ApplicationWindow *app = qobject_cast<ApplicationWindow *>(parent);
	if (app)
		editor = new ColorMapEditor(app->colorMapList, app->currentColorMap, false, app->sasPath, app->locale());
	else
		editor = new ColorMapEditor(app->colorMapList, app->currentColorMap, false, app->sasPath);

	applyBtn = new QPushButton(tr("&Apply"));
	connect(applyBtn, SIGNAL(clicked()), this, SLOT(apply()));

	closeBtn = new QPushButton(tr("&Close"));
	connect(closeBtn, SIGNAL(clicked()), this, SLOT(reject()));

	QHBoxLayout* hb = new QHBoxLayout();
	hb->setSpacing(5);
	hb->addStretch();
	hb->addWidget(applyBtn);
	hb->addWidget(closeBtn);
	hb->addStretch();

	QVBoxLayout* vl = new QVBoxLayout(this);
	vl->setSpacing(0);
	vl->addWidget(editor);
	vl->addLayout(hb);

	setMaximumWidth(editor->width() + 20);
}

void ColorMapDialog::setMatrix(Matrix *m)
{
	if (!m)
		return;

	d_matrix = m;
	editor->setRange(m->colorRange().minValue(), m->colorRange().maxValue());
	editor->setColorMap(m->colorMap());
}

void ColorMapDialog::apply()
{
	d_matrix->undoStack()->push(new MatrixSetColorMapCommand(d_matrix, d_matrix->colorMapType(),
						d_matrix->colorMap(), Matrix::Custom, editor->colorMap(), tr("Set Custom Palette")));
	d_matrix->setColorMap(editor->colorMap());
}
