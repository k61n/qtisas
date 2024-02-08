/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compatibility layer with previous Qt versions
 ******************************************************************************/

#ifndef COMPAT_H
#define COMPAT_H

#include <QtGlobal>
#include <QFontMetrics>
#include <QString>

namespace Qt
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
extern QString::SplitBehavior SkipEmptyParts;
#endif
} // namespace Qt

class CompatQFontMetrics : public QFontMetrics
{
  public:
    using QFontMetrics::QFontMetrics;

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    int horizontalAdvance(const QString &text, int len = -1) const
    {
        return width(text, len);
    }
    int horizontalAdvance(QChar ch) const
    {
        return width(ch);
    }
#endif
};

#endif // COMPAT_H
