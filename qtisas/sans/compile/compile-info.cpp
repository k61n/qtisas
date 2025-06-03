/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Info editor functions  of compile interface
 ******************************************************************************/

#include <QClipboard>

#include "compile18.h"

// +++ clear formatting
void compile18::deleteFormat()
{
    textEditDescription->selectAll();
    textLeft();
    textEditDescription->setAcceptRichText(false);
    textEditDescription->setAcceptRichText(false);
    textEditDescription->selectAll();
    textEditDescription->cut();
    textEditDescription->paste();
    textEditDescription->setAcceptRichText(true);
    textEditDescription->toPlainText();
}
// +++ set text family
void compile18::textFamily(const QString &f)
{
    textEditDescription->setFontFamily(f);
    textEditDescription->viewport()->setFocus();
}
// +++ set text size
void compile18::textSize(const QString &p)
{
    textEditDescription->setFontPointSize(p.toInt());
    textEditDescription->viewport()->setFocus();
}
// +++ bold/thin
void compile18::textBold()
{
    if (textEditDescription->fontWeight() < QFont::Black)
        textEditDescription->setFontWeight(QFont::Black);
    else
        textEditDescription->setFontWeight(QFont::Thin);
}
// +++ underline
void compile18::textUnderline()
{
    textEditDescription->setFontUnderline(!textEditDescription->fontUnderline());
}
// +++ italic
void compile18::textItalic()
{
    textEditDescription->setFontItalic(!textEditDescription->fontItalic());
}
// +++ alighment: left
void compile18::textLeft()
{
    textEditDescription->setAlignment(Qt::AlignLeft);
    readTextFormatting();
}
// +++ alighment: right
void compile18::textRight()
{
    textEditDescription->setAlignment(Qt::AlignRight);
    readTextFormatting();
}
// +++ alighment: center
void compile18::textCenter()
{
    textEditDescription->setAlignment(Qt::AlignCenter);
    readTextFormatting();
}
// +++ alighment: justified
void compile18::textJust()
{
    textEditDescription->setAlignment(Qt::AlignJustify);
    readTextFormatting();
}
// +++ insert html
void insertHtmlAndSelect(QTextEdit *textEdit, const QString &html)
{
    QTextCursor cursor = textEdit->textCursor();
    int startPos = cursor.position();
    cursor.insertHtml(html);

    int endPos = cursor.position();

    cursor.setPosition(startPos, QTextCursor::MoveAnchor);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);

    textEdit->setTextCursor(cursor);
}
// +++ html: sup
void compile18::textEXP()
{
    textEditDescription->blockSignals(true);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    textEditDescription->cut();
    if (pushButtonEXP->isChecked() )
        insertHtmlAndSelect(textEditDescription, "<SUP>" + clipboard->text() + "</SUP>");
    else
        insertHtmlAndSelect(textEditDescription, clipboard->text());
    textEditDescription->blockSignals(false);
}
// +++ html: sub
void compile18::textIndex()
{
    textEditDescription->blockSignals(true);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    textEditDescription->cut();
    QTextCursor cursor = textEditDescription->textCursor();
    if (pushButtonSub->isChecked())
        insertHtmlAndSelect(textEditDescription, "<SUB>" + clipboard->text() + "</SUB>");
    else
        insertHtmlAndSelect(textEditDescription, clipboard->text());
    textEditDescription->blockSignals(false);
}
// +++ html: link
void compile18::textLINK()
{
    textEditDescription->blockSignals(true);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    textEditDescription->cut();
    if (pushButtonLINK->isChecked())
        insertHtmlAndSelect(textEditDescription, "<a href='" + clipboard->text() + "'>" + clipboard->text() + "</a>");
    else
        insertHtmlAndSelect(textEditDescription, clipboard->text());
    textEditDescription->blockSignals(false);
}
// +++ set greek
void compile18::textGreek()
{
    QTextCursor cursor = textEditDescription->textCursor();
    QString selected = cursor.selectedText();
    selected.remove(QRegularExpression("<.*?>"));

    for (QChar &ch : selected)
    {
        ushort uc = ch.unicode();

        if (uc >= 'a' && uc <= 'z')
            ch = QChar(0x03B1 + (uc - 'a')); // a–z → α–ω
        else if (uc >= 'A' && uc <= 'Z')
            ch = QChar(0x0391 + (uc - 'A')); // A–Z → Α–Ω
        else if (uc >= 0x03B1 && uc <= 0x03C9)
            ch = QChar('a' + (uc - 0x03B1)); // α–ω → a–z
        else if (uc >= 0x0391 && uc <= 0x03A9 && uc != 0x03A2)
            ch = QChar('A' + (uc - 0x0391)); // Α–Ω → A–Z
    }

    cursor.removeSelectedText();
    cursor.insertText(selected);
}
// +++ is greek?
bool containsGreek(const QString &text)
{
    for (QChar ch : text)
    {
        ushort uc = ch.unicode();
        if ((uc >= 0x0391 && uc <= 0x03A9 && uc != 0x03A2) || // Α–Ω
            (uc >= 0x03B1 && uc <= 0x03C9))                   // α–ω
            return true;
    }
    return false;
}
// +++ read formatting
void compile18::readTextFormatting()
{
    const QTextCursor cursor = textEditDescription->textCursor();
    const Qt::Alignment alignment = cursor.blockFormat().alignment();
    const QTextCharFormat format = cursor.charFormat();

    pushButtonBold->setChecked(textEditDescription->fontWeight() >= QFont::Bold);
    pushButtonItal->setChecked(textEditDescription->fontItalic());
    pushButtonUnder->setChecked(textEditDescription->fontUnderline());

    pushButtonLINK->setChecked(format.isAnchor());
    pushButtonEXP->setChecked(format.verticalAlignment() == QTextCharFormat::AlignSuperScript);
    pushButtonSub->setChecked(format.verticalAlignment() == QTextCharFormat::AlignSubScript);
    pushButtonGreek->setChecked(cursor.hasSelection() && containsGreek(cursor.selectedText()));

    pushButtonLeft->setChecked(alignment == Qt::AlignLeft);
    pushButtonRight->setChecked(alignment == Qt::AlignRight);
    pushButtonCenter->setChecked(alignment == Qt::AlignCenter);
    pushButtonJust->setChecked(alignment == Qt::AlignJustify);

    QString fontFamily;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QVariant fontFamiliesVariant = format.fontFamilies();
    if (fontFamiliesVariant.isValid())
    {
        QStringList fontFamilies = fontFamiliesVariant.toStringList();
        if (!fontFamilies.isEmpty())
            fontFamily = fontFamilies.first();
    }
#else
    fontFamily = format.fontFamily();
#endif
    comboBoxFont->setCurrentIndex(comboBoxFont->findText(fontFamily));
    comboBoxFontSize->setCurrentIndex(comboBoxFontSize->findText(QString::number(format.font().pointSize())));
}