/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Title picker
 ******************************************************************************/

#include "TitlePicker.h"
#include "Graph.h"

#include <qwt_text_label.h>

#include <QMouseEvent>
#include <QPen>

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
