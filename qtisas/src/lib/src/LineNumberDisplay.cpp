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

#include "LineNumberDisplay.h"

LineNumberDisplay::LineNumberDisplay(QTextEdit *te, QWidget *parent)
		 : QTextEdit(parent), d_text_edit(te)
{
    firstLineIncrement=0;
    addZeros=false;
	setReadOnly(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setMaximumWidth(0);
	setLineWidth(0);
	setFocusPolicy(Qt::NoFocus);
	setCurrentFont(te->currentFont());
	viewport()->setCursor(Qt::ArrowCursor);

	QPalette palette = this->palette();
	palette.setColor(QPalette::Highlight, palette.color(QPalette::Base));
	setPalette(palette);

	if (te){
		connect(this, SIGNAL(selectionChanged()), this, SLOT(updateDocumentSelection()));

		connect(te->document(), SIGNAL(contentsChanged()), this, SLOT(updateLineNumbers()));
		connect((QObject *)te->verticalScrollBar(), SIGNAL(valueChanged(int)),
			(QObject *)verticalScrollBar(), SLOT(setValue(int)));
        connect(te, SIGNAL(currentCharFormatChanged (const QTextCharFormat &)),
                this, SLOT(changeCharFormat (const QTextCharFormat &)));
	}
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
	if (!isVisible() || !d_text_edit)
		return;

	int lines = d_text_edit->document()->blockCount();
	if (document()->blockCount() - 1 == lines && !force)
		return;

	QString aux;
	for(int i = firstLineIncrement; i < lines+firstLineIncrement; i++)
    {
        if(addZeros)
        {
        aux += (i<9?"00":"");
        aux +=(i<99&&i>=9?"0":"");
        };
        aux += QString::number(i + 1) + "\n";
    }
		//aux += QString::number(i + 1) + "\n";

	setPlainText(aux);

	QFontMetrics fm(d_text_edit->currentFont(), this);
	setMaximumWidth(2*fm.boundingRect(QString::number(lines)).width());
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
}
