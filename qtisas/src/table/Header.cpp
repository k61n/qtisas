/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QHeaderView class with mouse events passed to parent
 ******************************************************************************/

#include <QMouseEvent>

#include "Header.h"

Header::Header(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
}

Header::~Header()
{
}

void Header::mousePressEvent(QMouseEvent *event)
{
    QHeaderView::mousePressEvent(event);
    parent()->eventFilter(this, event);
}

void Header::mouseReleaseEvent(QMouseEvent *event)
{
    QHeaderView::mouseReleaseEvent(event);
    parent()->eventFilter(this, event);
}

void Header::mouseDoubleClickEvent(QMouseEvent *event)
{
    QHeaderView::mouseDoubleClickEvent(event);
    parent()->eventFilter(this, event);
}

void Header::mouseMoveEvent(QMouseEvent *event)
{
    QHeaderView::mouseMoveEvent(event);
    parent()->eventFilter(this, event);
}
