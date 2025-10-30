/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2025 Vitaliy Pipich <v.pipich@gmail.com>
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

inline QRegularExpression wildcardToRegex(const QString &pattern)
{
    QString regex = QRegularExpression::escape(pattern);
    regex.replace("\\*", ".*");
    regex.replace("\\?", ".");
    return QRegularExpression("^" + regex + "$");
}

} // namespace REGEXPS

#endif
