/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: validator of number fields that follows the numerical format
 ******************************************************************************/

#ifndef NUMBER_VALIDATOR_H
#define NUMBER_VALIDATOR_H

#include <optional>

#include <QDoubleValidator>

class QLineEdit;

// A number field's validator: numbers only (also negative and scientific), in the numerical format of its line edit.
// - When that format changes, the validator follows it and converts the field's number.
// - While typing, the other format's decimal point becomes this one's ("2.5" typed in German gives 2,5, not 25), as
//   long as the format omits group separators.
// - When editing ends with no acceptable number: an exponent alone means 1 ("E4" gives 1E4), a C number is rewritten
//   in the format ("2.5" with German group separators shown), a number outside the range (setBottom()/setTop())
//   becomes the nearest limit, else an empty or unreadable field gets the fallback value, if one is given.
// Usage: new NumberValidator(lineEdit, fallback) - one per field.
class NumberValidator : public QDoubleValidator
{
    Q_OBJECT

  public:
    explicit NumberValidator(QLineEdit *edit, std::optional<double> fallback = std::nullopt);

    State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    QLineEdit *d_edit = nullptr;
    std::optional<double> d_fallback{};
};

#endif
