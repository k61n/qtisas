/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A collapsive QGroupBox
 ******************************************************************************/

#include "CollapsiveGroupBox.h"

CollapsiveGroupBox::CollapsiveGroupBox(const QString & title, QWidget *parent) : QGroupBox(title, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
}

void CollapsiveGroupBox::setCollapsed(bool collapsed)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(collapsed);
	}

	setFlat(collapsed);
}

void CollapsiveGroupBox::setExpanded(bool expanded)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(expanded);
	}

	setFlat(!expanded);
}
