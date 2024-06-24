/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python syntax highlighting based on the Qt syntax highlighter example
 ******************************************************************************/

#ifndef PYTHONHIGHLIGHTER_H
#define PYTHONHIGHLIGHTER_H

#include <QHash>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>

#include "ScriptEdit.h"

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(ScriptEdit * parent);

protected:
    void highlightBlock(const QString &text);

	struct HighlightingRule
	{
		QRegExp pattern;
		QTextCharFormat format;
	};

	QVector<HighlightingRule> highlightingRules;

	QTextCharFormat commentFormat;
	QTextCharFormat quotationFormat;
	QTextCharFormat functionFormat;
	QTextCharFormat numericFormat;
};

class PythonSyntaxHighlighter : public SyntaxHighlighter
{
    Q_OBJECT

public:
    PythonSyntaxHighlighter(ScriptEdit *parent);

    static QStringList keywordsList(){return d_keywords;};

protected:
    void highlightBlock(const QString &text);

private:
	QVector<HighlightingRule> pythonHighlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;

    static const QStringList d_keywords;
};

#endif
