/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Highlighter
 ******************************************************************************/

#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Number format
    QTextCharFormat numberFormat;
    numberFormat.setForeground(Qt::darkBlue);
    rule.pattern = QRegularExpression(R"(\b\d+(\.\d*)?([eE][\+\-]?\d{1,3})?\b)");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Function call format
    QTextCharFormat functionFormat;
    functionFormat.setForeground(Qt::blue);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(R"(\b[A-Za-z0-9_]+(?=\())");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Keyword format
    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywordPatterns = {
        R"(\bchar\b)",      R"(\bclass\b)",    R"(\bconst\b)",   R"(\bdouble\b)",    R"(\benum\b)",
        R"(\bexplicit\b)",  R"(\bfriend\b)",   R"(\binline\b)",  R"(\bint\b)",       R"(\blong\b)",
        R"(\bnamespace\b)", R"(\boperator\b)", R"(\bprivate\b)", R"(\bprotected\b)", R"(\bpublic\b)",
        R"(\bshort\b)",     R"(\bsignals\b)",  R"(\bsigned\b)",  R"(\bslots\b)",     R"(\bstatic\b)",
        R"(\bstruct\b)",    R"(\btemplate\b)", R"(\btypedef\b)", R"(\btypename\b)",  R"(\bunion\b)",
        R"(\bunsigned\b)",  R"(\bvirtual\b)",  R"(\bvoid\b)",    R"(\bvolatile\b)",  R"(\bif\b)",
        R"(\belse\b)",      R"(\bwhile\b)",    R"(\bfor\b)",     R"(\busing\b)",     R"(\breturn\b)",
        R"(\btrue\b)",      R"(\bfalse\b)",    R"(\bnew\b)",     R"(\bdelete\b)",    R"(\bbool\b)",
        R"(\bend\b)",       R"(\bextern\b)",   R"(\bdo\b)",      R"(\bstd::\b)",     R"(\bstring\b)",
        R"(\bcase\b)",      R"(\bswitch\b)"};

    for (const QString &pattern : keywordPatterns)
    {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Preprocessor directives format
    classFormat.setForeground(Qt::darkMagenta);
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(R"(#[A-Za-z]+\b)");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Single-line comment format
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(R"(//[^\n]*)");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Quotation format (double quotes)
    quotationFormat.setForeground(Qt::magenta);
    rule.pattern = QRegularExpression(R"(".*?")");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Quotation format (angle brackets)
    rule.pattern = QRegularExpression(R"(\B<\S+>(\s*;)?\s*)");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Multi-line comment markers
    multiLineCommentFormat.setForeground(Qt::darkGreen);
    commentStartExpression = QRegularExpression(R"(/\*)");
    commentEndExpression = QRegularExpression(R"(\*/)");
}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules))
    {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = static_cast<int>(text.indexOf(commentStartExpression));

    while (startIndex >= 0)
    {
        QRegularExpressionMatch endMatch;
        int endIndex = static_cast<int>(text.indexOf(commentEndExpression, startIndex, &endMatch));
        int commentLength;

        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = static_cast<int>(text.length() - startIndex);
        }
        else
            commentLength = endIndex - startIndex + 2;

        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = static_cast<int>(text.indexOf(commentStartExpression, startIndex + commentLength));
    }
}