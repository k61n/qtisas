/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A widget displaying rectangles in 2D plots
 ******************************************************************************/

#ifndef RECTANGLE_WIDGET_H
#define RECTANGLE_WIDGET_H

#include "FrameWidget.h"

class RectangleWidget: public FrameWidget
{
	Q_OBJECT

public:
	RectangleWidget(Graph *);

	virtual QString saveToString();

	void clone(RectangleWidget* t);
	static void restore(Graph *g, const QStringList& lst);

	void updateCoordinates();
	void setLinkedLayer(int layerIndex);

public slots:
    void closedLinkedLayer(QObject *);

private:
    //! Index of the linked layer using the rectangle coordinates as a zoom region
    int d_linked_layer;
};

#endif
