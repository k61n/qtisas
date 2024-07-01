/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: QtiSAS version
 ******************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QRegularExpression>

namespace REGEXPS
{
static const QRegularExpression nonalphanumeric("\\W");
static const QRegularExpression nonnumeric("\\D");
static const QRegularExpression whitespaces("\\s");
} // namespace REGEXPS

#endif
