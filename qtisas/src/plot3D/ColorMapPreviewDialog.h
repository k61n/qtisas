/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Open file dialog providing a color map preview
 ******************************************************************************/

#ifndef COLORMAPPREVIEWDIALOG_H
#define COLORMAPPREVIEWDIALOG_H

#include <QLabel>

#include "ExtensibleFileDialog.h"

class ColorMapPreviewDialog : public ExtensibleFileDialog
{
	Q_OBJECT
	public:
    explicit ColorMapPreviewDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    public slots:
		void updatePreview(const QString&);
	
	private:
		QLabel *d_preview_label;
};

#endif
