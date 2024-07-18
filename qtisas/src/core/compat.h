/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compatibility layer with previous Qt versions
 ******************************************************************************/

#ifndef COMPAT_H
#define COMPAT_H

#include <QString>
#include <QtGlobal>

namespace Qt
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
extern QString::SplitBehavior SkipEmptyParts;
#endif
} // namespace Qt

#endif
