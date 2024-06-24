/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying ellipses/circles in 2D plots
 ******************************************************************************/

#ifndef ELLIPSEWIDGET_H
#define ELLIPSEWIDGET_H

#include "FrameWidget.h"

class EllipseWidget: public FrameWidget
{
	Q_OBJECT

public:
	EllipseWidget(Graph *);

	virtual QString saveToString();

	void clone(EllipseWidget* t);
	static void restore(Graph *g, const QStringList& lst);

private:
	void drawFrame(QPainter *p, const QRect& rect);
};

#endif
