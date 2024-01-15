/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A widget displaying ellipses/circles in 2D plots
 ******************************************************************************/

#ifndef ELLIPSE_WIDGET_H
#define ELLIPSE_WIDGET_H

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
