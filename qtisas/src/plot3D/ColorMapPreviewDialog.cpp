/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Open file dialog providing a color map preview
 ******************************************************************************/

#include <QHBoxLayout>
#include <QLabel>

#include "ColorMapPreviewDialog.h"
#include "Graph3D.h"

ColorMapPreviewDialog::ColorMapPreviewDialog(QWidget *parent, Qt::WindowFlags flags)
	: ExtensibleFileDialog(parent, flags)
{
	setWindowTitle(tr("QtiSAS - Color Map Preview Dialog"));
	setFileMode(ExistingFile);
	QStringList filters;
	filters << tr("Colormap files") + " (*.map *.MAP)" << tr("All files") + " (*)";
	setNameFilters(filters);

	setExtentionToggleButtonText(tr("<< &Preview"));
	setExtended(true);
	
    auto advanced_options = new QWidget();
    auto advanced_layout = new QHBoxLayout();
	advanced_options->setLayout(advanced_layout);
	
	d_preview_label = new QLabel(tr("None"));
	d_preview_label->setScaledContents(true);
	d_preview_label->setFrameShape( QFrame::StyledPanel );
	d_preview_label->setFrameShadow( QFrame::Sunken );
	advanced_layout->addWidget(d_preview_label);
	
	setExtensionWidget(advanced_options);
	connect(this, SIGNAL(currentChanged(const QString&)), 
			this, SLOT(updatePreview(const QString&)));
}

void ColorMapPreviewDialog::updatePreview(const QString& fileName)
{
	if (fileName.isEmpty()){
		d_preview_label->setText(tr("None"));
   		return;
	}
	
	QFileInfo fi(fileName);
	if (!fi.isFile () || !fi.isReadable ()){
		d_preview_label->setText(tr("None"));
   		return;
	}
	
	ColorVector cv;
	if (!Graph3D::openColorMapFile(cv, fileName)){
		d_preview_label->setText(tr("None"));
   		return;
	}
		
	int height = 40;
	QPixmap pix;
    int width =
        (cv.size() > std::numeric_limits<int>::max()) ? std::numeric_limits<int>::max() : static_cast<int>(cv.size());
    pix = pix.copy(0, 0, width, height);
	QPainter p(&pix);
	for (unsigned i = 0; i != cv.size(); ++i){
		RGBA rgb = cv[i];
		p.setPen(GL2Qt(rgb.r, rgb.g, rgb.b));
		p.drawLine(QPoint(0, 0), QPoint(0, height));	
   		p.translate(1, 0);
	}
  	p.end();
	d_preview_label->setPixmap(pix);
}
