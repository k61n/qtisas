/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: global functions
 ******************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QRegularExpression>

namespace REGEXPS
{

static const QRegularExpression nonalphanumeric("\\W");
static const QRegularExpression nonnumeric("\\D");
static const QRegularExpression whitespaces("\\s");

inline QString wildcardToRE(const QString &pattern)
{
    return QString(pattern).replace(".", "\\.").replace('?', '.').replace("*", ".*");
}

} // namespace REGEXPS

#endif
