/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wrapper around QtColorPicker from QtSolutions
 ******************************************************************************/

#include "ColorButton.h"
#include "ColorBox.h"
#include <QTimer>
#include <QScreen>

void ColorButton::repositionPopup()
{
    auto popups = this->findChildren<QWidget *>();
    for (QWidget *popup : popups)
    {
        if (popup->windowFlags() & Qt::Popup)
        {
            if (QScreen *scr = this->screen())
            {
                QRect screenGeom = scr->availableGeometry();
                QPoint pos = this->mapToGlobal(this->rect().bottomLeft());

                QSize popupSize = popup->sizeHint();
                int x = qBound(screenGeom.left(), pos.x(), screenGeom.right() - popupSize.width());
                int y = qBound(screenGeom.top(), pos.y(), screenGeom.bottom() - popupSize.height());
                popup->move(x, y);
            }
            break;
        }
    }
}

ColorButton::ColorButton(QWidget *parent) : QtColorPicker(parent)
{
    // Ensure the popup always opens on the same screen
    connect(this, &ColorButton::clicked, this,
            [this]() { QTimer::singleShot(0, this, &ColorButton::repositionPopup); });

    // Insert default colors
    QStringList color_names = ColorBox::defaultColorNames();
    QList<QColor> defaultColors = ColorBox::defaultColors();
    for (int i = 0; i < ColorBox::numPredefinedColors(); i++)
        insertColor(defaultColors[i], color_names[i]);

    // Insert user-defined colors
    QList<QColor> colors = ColorBox::colorList();
    color_names = ColorBox::colorNames();
    for (int i = 0; i < colors.count(); i++)
    {
        QColor c = colors[i];
        if (!defaultColors.contains(c))
            insertColor(c, color_names[i]);
    }

    // Forward colorChanged signal
    connect(this, &QtColorPicker::colorChanged, this, &ColorButton::colorChanged);
}