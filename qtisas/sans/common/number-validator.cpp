/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: validator of number fields that follows the numerical format
 ******************************************************************************/

#include <QEvent>
#include <QLineEdit>

#include "number-validator.h"
#include "numericLocale.h"

NumberValidator::NumberValidator(QLineEdit *edit, std::optional<double> fallback)
    : QDoubleValidator(edit), d_edit(edit), d_fallback(fallback)
{
    setNotation(QDoubleValidator::ScientificNotation);
    setLocale(NumericLocale::strict(edit->locale()));
    // Qt passes a locale change to the line edit before its parent's changeEvent()
    edit->installEventFilter(this);
    edit->setValidator(this);
}

bool NumberValidator::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d_edit && event->type() == QEvent::LocaleChange)
    {
        const QLocale from = locale();
        setLocale(NumericLocale::strict(d_edit->locale()));
        d_edit->setText(NumericLocale::convert(d_edit->text(), from, locale())); // text that is no number: unchanged
    }
    return QDoubleValidator::eventFilter(watched, event);
}

QValidator::State NumberValidator::validate(QString &input, int &pos) const
{
    if (locale().numberOptions() & QLocale::OmitGroupSeparator)
    {
        const QString point = locale().decimalPoint();
        input.replace(point == "," ? "." : ",", point);
    }
    return QDoubleValidator::validate(input, pos);
}

// Not QDoubleValidator::fixup() (Qt >= 6.3): it rewrites the number in its own notation, not as typed
void NumberValidator::fixup(QString &input) const
{
    // an exponent without mantissa means 1: "E4" is 1E4, "-e-3" is -1e-3
    QString text = input.trimmed();
    const qsizetype exponent = text.startsWith('+') || text.startsWith('-') ? 1 : 0;
    if (text.mid(exponent, 1).compare("e", Qt::CaseInsensitive) == 0)
        text.insert(exponent, '1');

    int pos = 0;
    if (validate(text, pos) == Acceptable) // as typed, only completed
    {
        input = text;
        return;
    }

    // a number of this format, else a C one; outside the range: the nearest limit
    double value = 0.0;
    const bool inFormat = NumericLocale::toDouble(text, locale(), value);
    if (inFormat || NumericLocale::toDouble(text, QLocale::c(), value))
    {
        const double clamped = qBound(bottom(), value, top());
        QString number = clamped != value ? locale().toString(clamped, 'g', QLocale::FloatingPointShortest)
                                          : NumericLocale::convert(text, inFormat ? locale() : QLocale::c(), locale());
        if (validate(number, pos) == Acceptable)
        {
            input = number;
            return;
        }
    }
    if (d_fallback)
        input = locale().toString(*d_fallback, 'g', QLocale::FloatingPointShortest);
}
