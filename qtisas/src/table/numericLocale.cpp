/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: rules for numbers written as text in a given locale
 ******************************************************************************/

#include "numericLocale.h"

namespace NumericLocale
{
QLocale strict(QLocale locale)
{
    if (locale.numberOptions() & QLocale::OmitGroupSeparator)
        locale.setNumberOptions(locale.numberOptions() | QLocale::RejectGroupSeparator);
    return locale;
}

bool toDouble(const QString &text, const QLocale &locale, double &value)
{
    bool ok = false;
    const double parsed = strict(locale).toDouble(text, &ok);
    if (ok)
        value = parsed;
    return ok;
}

bool isGroupedWhenOmitted(const QString &text, const QLocale &locale)
{
    if (!(locale.numberOptions() & QLocale::OmitGroupSeparator))
        return false;

    bool valid = false;
    locale.toDouble(text, &valid);
    if (!valid)
        return false; // expressions such as max(1,2) are no numbers at all

    double value = 0.0;
    return !toDouble(text, locale, value);
}

QString convert(const QString &text, const QLocale &from, const QLocale &to, char format, int precision)
{
    if (text.isEmpty() || from == to)
        return text;

    const bool pm = text.startsWith(QChar(0x00B1));
    double value = 0.0;
    if (!toDouble(pm ? text.mid(1) : text, from, value))
        return text;
    return (pm ? QString(QChar(0x00B1)) : QString()) + to.toString(value, format, precision);
}

QString convertRange(const QString &text, const QLocale &from, const QLocale &to)
{
    for (const QString &separator : {QStringLiteral(".."), QStringLiteral("+-"), QStringLiteral("-+")})
    {
        const qsizetype pos = text.indexOf(separator);
        if (pos >= 0)
            return convert(text.left(pos), from, to) + separator + convert(text.mid(pos + separator.size()), from, to);
    }
    return convert(text, from, to);
}

bool toDoubleCOrLocale(const QString &text, const QLocale &locale, double &value)
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    return toDouble(text, c, value) || toDouble(text, locale, value);
}
} // namespace NumericLocale
