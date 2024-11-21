/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A collapsible QGroupBox
 ******************************************************************************/

#include "CollapsibleGroupBox.h"

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent) : QGroupBox(title, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(collapsed);
	}

	setFlat(collapsed);
}

void CollapsibleGroupBox::setExpanded(bool expanded)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(expanded);
	}

	setFlat(!expanded);
}
