/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QHeaderView class with mouse events passed to parent
 ******************************************************************************/

#ifndef HEADER_H
#define HEADER_H

#include <QHeaderView>

class Header : public QHeaderView
{
public:
    Header(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~Header();

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
};

#endif
