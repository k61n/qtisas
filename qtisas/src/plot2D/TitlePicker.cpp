/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Title picker
 ******************************************************************************/

#include <QMouseEvent>
#include <QPen>

#include <qwt/qwt_text_label.h>

#include "Graph.h"
#include "TitlePicker.h"

TitlePicker::TitlePicker(Graph *plot):
	QObject(plot)
{
    d_selected = false;
	title = (QwtTextLabel *)plot->titleLabel();
	title->setFocusPolicy(Qt::StrongFocus);
	if (title)
		title->installEventFilter(this);
}

bool TitlePicker::eventFilter(QObject *object, QEvent *e)
{
	if (object != (QObject *)title)
		return false;

    if ( object->inherits("QwtTextLabel") && e->type() == QEvent::MouseButtonDblClick){
        emit doubleClicked();
		d_selected = true;
        return true;
    }

	 if ( object->inherits("QwtTextLabel") &&  e->type() == QEvent::MouseButtonPress ){
		 const QMouseEvent *me = (const QMouseEvent *)e;

		 emit clicked();

		 if (me->button() == Qt::RightButton)
			emit showTitleMenu();
         else if (me->button() == Qt::LeftButton)
		 	setSelected();

		 return !(me->modifiers() & Qt::ShiftModifier);
    }

	if ( object->inherits("QwtTextLabel") &&
        e->type() == QEvent::KeyPress){
		switch (((const QKeyEvent *)e)->key())
			{
			case Qt::Key_Delete:
			emit removeTitle();
            return true;
			}
		}

    return QObject::eventFilter(object, e);
}

void TitlePicker::setSelected(bool select)
{
    if (!title || d_selected == select)
        return;

    d_selected = select;

    QwtText text = title->text();
    if (select)
        text.setBackgroundPen(QPen(Qt::blue));
    else
        text.setBackgroundPen(QPen(Qt::NoPen));

    ((QwtPlot *)parent())->setTitle(text);
}
