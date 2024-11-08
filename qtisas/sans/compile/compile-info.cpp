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
}

void compile18::textRight()
{
    textEditDescription->setAlignment(Qt::AlignRight);
}

void compile18::textCenter()
{
    textEditDescription->setAlignment(Qt::AlignCenter);
}

void compile18::textJust()
{
    textEditDescription->setAlignment(Qt::AlignJustify);
}

void insertHtmlAndSelect(QTextEdit *textEdit, const QString &html)
{
    // Get the current text cursor
    QTextCursor cursor = textEdit->textCursor();

    // Remember the initial position before inserting the HTML
    int startPos = cursor.position();

    // Insert the HTML content at the cursor position
    cursor.insertHtml(html);

    // Position after the HTML insertion
    int endPos = cursor.position();

    // Move cursor to the start position and then select the inserted HTML
    cursor.setPosition(startPos, QTextCursor::MoveAnchor);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);

    // Apply the modified cursor with the selection
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


void compile18::readTextFormatting()
{
    if (textEditDescription->fontWeight() < QFont::Bold)
        pushButtonBold->setChecked(false);
    else
        pushButtonBold->setChecked(true);

    if (textEditDescription->fontItalic()) pushButtonItal->setChecked(true);
    else pushButtonItal->setChecked(false);
    
    if (textEditDescription->fontUnderline()) pushButtonUnder->setChecked(true);
    else pushButtonUnder->setChecked(false);
    
    if (textEditDescription->fontUnderline()) pushButtonUnder->setChecked(true);
    else pushButtonUnder->setChecked(false);
    
    //+++
    pushButtonLeft->setChecked(false);
    pushButtonCenter->setChecked(false);
    pushButtonRight->setChecked(false);
    pushButtonJust->setChecked(false);
    
    
    switch (textEditDescription->alignment())
    {
        case Qt::AlignLeft: pushButtonLeft->setChecked(true); break;
        case Qt::AlignRight: pushButtonRight->setChecked(true); break;
        case Qt::AlignJustify: pushButtonJust->setChecked(true); break;
        case Qt::AlignHCenter: pushButtonCenter->setChecked(true); break;
    }

    //+++ font
    comboBoxFont->setCurrentIndex(comboBoxFont->findText(textEditDescription->fontFamily()));
    comboBoxFontSize->setCurrentIndex(
        comboBoxFontSize->findText(QString::number(lround(textEditDescription->fontPointSize()))));

    QTextCursor cursor = textEditDescription->textCursor();
    if (cursor.hasSelection())
    {
        QString selectedHtml = cursor.selection().toHtml();

        if (selectedHtml.contains("vertical-align:sub"))
            pushButtonSub->setChecked(true);
        else
            pushButtonSub->setChecked(false);

        if (selectedHtml.contains("vertical-align:sup"))
            pushButtonEXP->setChecked(true);
        else
            pushButtonEXP->setChecked(false);
    }
    else
    {
        pushButtonSub->setChecked(false);
        pushButtonEXP->setChecked(false);
    }
}


