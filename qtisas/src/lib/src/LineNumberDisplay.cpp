/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying line numbers for a QTextEdit
 ******************************************************************************/

#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextBlock>
#include <QTextStream>

#include "LineNumberDisplay.h"

LineNumberDisplay::LineNumberDisplay(QTextEdit *te, QWidget *parent) : QTextEdit(parent), d_text_edit(te)
{
    firstLineIncrement=0;
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setContentsMargins(0, 0, 0, 0);
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame | QFrame::Plain);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::NoWrap);
    setReadOnly(true);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setStyleSheet("QTextEdit {"
                  "    background: transparent;"
                  "    border: none;"
                  "    padding: 0px;"
                  "}");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setCursor(Qt::ArrowCursor);

    updateLineNumbers(false);
    changeCharFormat(d_text_edit->currentCharFormat());

    connect(this, &QTextEdit::selectionChanged, this, &LineNumberDisplay::updateDocumentSelection);
    connect(d_text_edit->document(), &QTextDocument::contentsChanged, this, [this]() { updateLineNumbers(false); });
    connect(d_text_edit->verticalScrollBar(), &QScrollBar::valueChanged, verticalScrollBar(), &QScrollBar::setValue);
    connect(d_text_edit, &QTextEdit::currentCharFormatChanged, this, &LineNumberDisplay::changeCharFormat);
}

void LineNumberDisplay::updateDocumentSelection()
{
	if (!isVisible() || !d_text_edit)
		return;

	QTextCursor c = textCursor();
	int selectionStart = document()->findBlock(c.selectionStart()).firstLineNumber();
	int selectionEnd = document()->findBlock(c.selectionEnd()).firstLineNumber();
	int selectedLines = abs(selectionEnd - selectionStart);

	QTextCursor cursor(d_text_edit->textCursor());
	cursor.movePosition(QTextCursor::Start);
	for (int i = 0; i < selectionStart; i++)
		cursor.movePosition(QTextCursor::Down);

	for (int i = 0; i < selectedLines; i++)
		cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);

	cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

	if (selectionEnd == d_text_edit->document()->blockCount() - 1)
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

	d_text_edit->setTextCursor(cursor);
}

void LineNumberDisplay::updateLineNumbers(bool force)
{
    if ((!isVisible() || !d_text_edit) && !force)
        return;
    int lines = d_text_edit->document()->blockCount();
    if (document()->blockCount() - 1 == lines && !force)
        return;

    QString nums;
    QTextStream stream(&nums, QIODevice::WriteOnly);
    for (int i = firstLineIncrement; i < lines + firstLineIncrement; i++)
        stream << i + 1 << '\n';
    setPlainText(nums);
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignRight);
    cursor.setBlockFormat(blockFormat);
    setFixedWidth(maxcharwidth * (int)(QString::number(lines + firstLineIncrement).length() + 2));
    verticalScrollBar()->setValue(d_text_edit->verticalScrollBar()->value());
}

void LineNumberDisplay::showEvent(QShowEvent *e)
{
    e->accept();
    if (isVisible())
        updateLineNumbers();
}

void LineNumberDisplay::changeCharFormat (const QTextCharFormat &f)
{
    setCurrentFont(f.font());
    QFontMetrics fm(f.font());
    maxcharwidth = 0;
    for (int i = 0; i < 10; i++)
    {
        int width = fm.boundingRect(QString::number(i)).width();
        maxcharwidth = (maxcharwidth < width) ? width : maxcharwidth;
    }
}
