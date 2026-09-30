/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: rules for numbers written as text in a given locale
 ******************************************************************************/

#ifndef NUMERICLOCALE_H
#define NUMERICLOCALE_H

#include <QLocale>
#include <QString>

// One policy for every place that reads or writes numbers as text: tables, matrices, FitTable widgets, the clipboard
// and the Python bindings. Numbers stored or exchanged as text (files, settings, Text table columns, Python) are C.
namespace NumericLocale
{
// The locale that rejects group separators when it omits them on output ("1,234" is not 1234 in C)
QLocale strict(QLocale locale);

// Parses text with strict(locale); false (value unchanged) for text that is not a number
bool toDouble(const QString &text, const QLocale &locale, double &value);

// Text that the locale accepts as a number only because of its group separators, e.g. "1,333" in C
bool isGroupedWhenOmitted(const QString &text, const QLocale &locale);

// The number written in another locale; a leading "±" is kept, text that is not a number (---, labels) is unchanged
QString convert(const QString &text, const QLocale &from, const QLocale &to, char format = 'g',
                int precision = QLocale::FloatingPointShortest);

// Both ends of a "left..right" or "center+-width" range
QString convertRange(const QString &text, const QLocale &from, const QLocale &to);

// A number in a Text column or other shared text: C first, then the table's own locale
bool toDoubleCOrLocale(const QString &text, const QLocale &locale, double &value);
} // namespace NumericLocale

#endif
