/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Compatibility layer with previous Qt versions
 ******************************************************************************/

#include "compat.h"

namespace Qt
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
#endif
}
