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

#include <QColor>
#include <QHash>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>

#include "ScriptEdit.h"

// Distinct color for a script's OWN reported results (via the scriptPrint()/scriptPrintReplace()
// Python API calls), so they stand out from surrounding AI status/header lines and error dumps,
// which are printed via the plain, uncolored scriptPrint()/scriptPrintReplace() above. Applied via
// QTextCharFormat + QTextCursor::insertText(), not raw HTML markup — scriptPrintReplace()'s
// cursor.insertText() never interprets HTML, so a format-based approach is the only one that works
// identically for both scriptPrint() and scriptPrintReplace(). Shared with PythonSyntaxHighlighter
// (see highlightBlock()'s script-output detection), which is why this lives here rather than in
// ApplicationWindow.cpp alongside its callers.
inline const QColor scriptOutputColor(0x0f, 0x9d, 0x58);

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(ScriptEdit * parent);
    explicit SyntaxHighlighter(QTextDocument *doc);

protected:
    void highlightBlock(const QString &text);

	struct HighlightingRule
	{
        QRegularExpression pattern;
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
    explicit PythonSyntaxHighlighter(QTextDocument *doc, bool docBrowserMode = false);

    static QStringList keywordsList(){return d_keywords;};

protected:
    void highlightBlock(const QString &text);

private:
    bool m_docBrowserMode = false;
	QVector<HighlightingRule> pythonHighlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;

    static const QStringList d_keywords;
};

#endif
