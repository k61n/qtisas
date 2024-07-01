/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Highlighter
 ******************************************************************************/

#include "highlighter.h"

 Highlighter::Highlighter(QTextDocument *parent)
     : QSyntaxHighlighter(parent)
 {
     HighlightingRule rule;

     functionFormat.setForeground(Qt::darkBlue);
    rule.pattern = QRegularExpression(R"(\b(\d\d*(\.\d*)?((E\-|E\+|e\+|e\-|e|E)\d\d?\d?\d?)?)\b)");
     rule.format = functionFormat;
     highlightingRules.append(rule);
     
     functionFormat.setFontItalic(true);
     functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
     rule.format = functionFormat;
     highlightingRules.append(rule);
     
     keywordFormat.setForeground(Qt::darkBlue);
     keywordFormat.setFontWeight(QFont::Bold);
     QStringList keywordPatterns;
     keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
                     << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
                     << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
                     << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
                     << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
                     << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
                     << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
                     << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
                     << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
                     << "\\bvoid\\b" << "\\bvolatile\\b"<< "\\bif\\b"<< "\\belse\\b"
                     << "\\bclass\\b" << "\\bwhile\\b" << "\\bfor\\b" << "\\busing\\b"
                     << "\\breturn\\b" << "\\btrue\\b" << "\\bTRUE\\b" << "\\bfalse\\b"
                     << "\\bFALSE\\b" << "\\bnew\\b" << "\\bdelete\\b" << "\\bbool\\b"
     << "\\bend\\b" << "\\bextern\\b" << "\\bdo\\b"<< "\\bstd::\\b"<< "\\bstring\\b"<< "\\bcase\\b"<< "\\bswitch\\b";// << "\\binclude\\b" ;

     foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
         rule.format = keywordFormat;
         highlightingRules.append(rule);
     }

     classFormat.setFontWeight(QFont::Bold);
     classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression("#[A-Za-z]+\\b");
     rule.format = classFormat;
     highlightingRules.append(rule);

     singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("//[^\n]*");
     rule.format = singleLineCommentFormat;
     highlightingRules.append(rule);

     multiLineCommentFormat.setForeground(Qt::darkGreen);

     quotationFormat.setForeground(Qt::magenta);
    rule.pattern = QRegularExpression("\".*\"");
     rule.format = quotationFormat;
     highlightingRules.append(rule);

     quotationFormat.setForeground(Qt::magenta);
    rule.pattern = QRegularExpression(R"(\B\<\S+\>($|\;|\s+$|\s+\;))");
     rule.format = quotationFormat;
     highlightingRules.append(rule);

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
 }

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegularExpression re(rule.pattern);
        QRegularExpressionMatchIterator i = re.globalMatch(text);
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
        }
    }
     setCurrentBlockState(0);

    QRegularExpressionMatchIterator i = commentStartExpression.globalMatch(text);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()),
                  multiLineCommentFormat);
    }
    setCurrentBlockState(1);
}
