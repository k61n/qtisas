/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Info tab functions  of compile interface
 ******************************************************************************/

#include <QClipboard>

#include "compile18.h"

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

void compile18::textFamily(const QString &f)
{
    
    textEditDescription->setFontFamily(f);
    textEditDescription->viewport()->setFocus();
}

void compile18::textSize(const QString &p)
{
    textEditDescription->setFontPointSize(p.toInt());
    textEditDescription->viewport()->setFocus();
}

void compile18::textBold()
{
    if (textEditDescription->fontWeight() < QFont::Black)
        textEditDescription->setFontWeight(QFont::Black);
    else
        textEditDescription->setFontWeight(QFont::Thin);
}

void compile18::textUnderline()
{
    textEditDescription->setFontUnderline(!textEditDescription->fontUnderline());
}

void compile18::textItalic()
{
    textEditDescription->setFontItalic(!textEditDescription->fontItalic());
}

void compile18::textLeft()
{
    textEditDescription->setAlignment(Qt::AlignLeft);
    readTextFormatting();
}

void compile18::textRight()
{
    textEditDescription->setAlignment(Qt::AlignRight);
    readTextFormatting();
}

void compile18::textCenter()
{
    textEditDescription->setAlignment(Qt::AlignCenter);
    readTextFormatting();
}

void compile18::textJust()
{
    textEditDescription->setAlignment(Qt::AlignJustify);
    readTextFormatting();
}

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

void compile18::textGreek()
{
    QString selected = textEditDescription->textCursor().selectedText();
    textEditDescription->cut();
    static const QRegularExpression re("<.*>");
    selected.remove(re);
    
    int length=selected.length();
    
    for (int i=0; i<length;i++)
    {
        if (selected[i].unicode() >= 0x0061 && selected[i].unicode() <= 0x007A) selected[i]=QChar(selected[i].unicode()-0x0061+0x03B1);
        else if (selected[i].unicode() >= 0x03B1 && selected[i].unicode() <= 0x03C9) selected[i]=QChar(selected[i].unicode()-0x03B1+0x0061);
    }

    insertHtmlAndSelect(textEditDescription, selected);
}

bool containsGreek(const QString &text)
{
    for (int i = 0; i < text.length(); ++i)
    {
        QChar ch = text[i];
        if (ch.unicode() >= 0x0370 && ch.unicode() <= 0x03FF)
            return true;
    }
    return false;
}

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