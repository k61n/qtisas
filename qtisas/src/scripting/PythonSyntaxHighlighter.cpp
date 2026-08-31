/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python syntax highlighting based on the Qt syntax highlighter example
 ******************************************************************************/

#include "ApplicationWindow.h"
#include "PythonSyntaxHighlighter.h"

const QStringList PythonSyntaxHighlighter::d_keywords = QStringList() << "and" << "assert" << "break"
					<< "class" << "continue"  << "def" << "del"
					<< "elif" << "else" << "except" << "exec"
					<< "finally" << "for" << "from" << "global"
					<< "if" << "import" << "in" << "is"
					<< "lambda" << "not" << "or" << "pass"
					<< "print" << "raise" << "return" << "try" << "while" << "True" << "False";

PythonSyntaxHighlighter::PythonSyntaxHighlighter(ScriptEdit *parent)
    : SyntaxHighlighter(parent)
{
    HighlightingRule rule;
	ApplicationWindow *app = parent->scriptingEnv()->application();

	keywordFormat.setForeground(app->d_keyword_highlight_color);
    keywordFormat.setFontWeight(QFont::Bold);

	foreach (QString pattern, d_keywords) {
        rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
        rule.format = keywordFormat;
        pythonHighlightingRules.append(rule);
    }

    classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(app->d_class_highlight_color);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
	pythonHighlightingRules.append(rule);
}

PythonSyntaxHighlighter::PythonSyntaxHighlighter(QTextDocument *doc, bool docBrowserMode)
    : SyntaxHighlighter(doc), m_docBrowserMode(docBrowserMode)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    foreach (QString pattern, d_keywords)
    {
        rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
        rule.format = keywordFormat;
        pythonHighlightingRules.append(rule);
    }

    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    pythonHighlightingRules.append(rule);
}

void PythonSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (m_docBrowserMode)
    {
        // code blocks — value is the language tag string (e.g. "python").
        const QString lang = currentBlock().blockFormat().stringProperty(QTextFormat::BlockCodeLanguage);
        if (lang != "python")
            return;

        // grey background — for python block
        static const QColor codeBackground(245, 245, 245);
        QTextBlockFormat blockFmt = currentBlock().blockFormat();
        if (blockFmt.background().color() != codeBackground)
        {
            blockFmt.setBackground(codeBackground);
            QTextCursor c(currentBlock());
            c.setBlockFormat(blockFmt);
        }
        QTextCharFormat bgFmt;
        bgFmt.setBackground(codeBackground);
        setFormat(0, static_cast<int>(text.length()), bgFmt);

        // base rules first: functions, numbers, strings, comments
        foreach (const HighlightingRule &rule, highlightingRules)
        {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext())
            {
                QRegularExpressionMatch m = it.next();
                setFormat(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), rule.format);
            }
        }

        // Python-specific rules on top: keywords and Qt class names
        foreach (const HighlightingRule &rule, pythonHighlightingRules)
        {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext())
            {
                QRegularExpressionMatch m = it.next();
                setFormat(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), rule.format);
            }
        }
        return;
    }

    static const QColor aiColor(220, 235, 255);
    // Marks blocks under a "▸▸▸ AI answer:" header as prose, not code, so they skip Python
    // highlighting until the next "▸▸▸ " header (other headers stay code/status as usual).
    static const int aiAnswerBlockState = 2;

    if (text.startsWith("▸▸▸ "))
    {
        QTextBlockFormat blockFmt = currentBlock().blockFormat();
        if (blockFmt.background().color() != aiColor)
        {
            blockFmt.setBackground(aiColor);
            QTextCursor c(currentBlock());
            c.setBlockFormat(blockFmt);
        }
        QTextCharFormat charFmt;
        charFmt.setBackground(aiColor);
        // setFormat() merges onto the existing char format, so an unset foreground here would
        // leave a bled-in color (e.g. green from prior script output) instead of resetting it.
        // Uses qApp's palette, not a hardcoded color, to respect the active light/dark theme.
        charFmt.setForeground(qApp->palette().color(QPalette::Text));
        setFormat(0, static_cast<int>(text.length()), charFmt);
        setCurrentBlockState(text.startsWith("▸▸▸ AI answer:") ? aiAnswerBlockState : 0);
        return;
    }

    // Script output is colored at insertion time (ApplicationWindow), not by this highlighter, so
    // we detect it by that color instead of a block state — read via a probe cursor since format(0)
    // only reflects what this highlighter itself set, not the document's real character format.
    bool isScriptOutput = false;
    if (!text.isEmpty())
    {
        QTextCursor probe(document());
        probe.setPosition(currentBlock().position());
        probe.setPosition(currentBlock().position() + 1, QTextCursor::KeepAnchor);
        isScriptOutput = probe.charFormat().foreground().color() == scriptOutputColor;
    }

    const bool isAiAnswerBody = previousBlockState() == aiAnswerBlockState;
    const bool suppressPythonHighlighting = isAiAnswerBody || isScriptOutput;

    // Strip AI background inherited by subsequent blocks
    if (currentBlock().blockFormat().background().color() == aiColor)
    {
        QTextBlockFormat blockFmt = currentBlock().blockFormat();
        blockFmt.setBackground(QBrush());
        QTextCursor c(currentBlock());
        c.setBlockFormat(blockFmt);
    }

    // Tint a cleaned "AI answer:" body dark blue, echoing header aiColor (see tryExtractInformationalMessage()).
    // Not applied to isScriptOutput lines — those already have their own scriptOutputColor.
    if (isAiAnswerBody && !text.isEmpty())
    {
        static const QColor aiAnswerTextColor(30, 60, 100);
        QTextCharFormat fmt;
        fmt.setForeground(aiAnswerTextColor);
        setFormat(0, static_cast<int>(text.length()), fmt);
    }

    QString s = text;
    static const QRegularExpression comment = QRegularExpression("\"{3}");
    s.replace(comment, "   ");

    if (!suppressPythonHighlighting)
    {
        foreach (HighlightingRule rule, pythonHighlightingRules)
        {
            QRegularExpression re(rule.pattern);
            QRegularExpressionMatchIterator i = re.globalMatch(s);
            while (i.hasNext())
            {
                QRegularExpressionMatch match = i.next();
                setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()),
                          rule.format);
            }
        }
    }

    // Markdown emphasis in AI answers (**bold**, `code`) — markers stay visible since a
    // QSyntaxHighlighter can't remove characters, but this still beats no styling at all.
    // Applied after the rules above so it wins over an incidental keyword-color match.
    static const QRegularExpression boldRe(R"(\*\*[^*]+\*\*)");
    QRegularExpressionMatchIterator boldIt = boldRe.globalMatch(s);
    while (boldIt.hasNext())
    {
        const QRegularExpressionMatch m = boldIt.next();
        QTextCharFormat boldFmt = format(static_cast<int>(m.capturedStart()));
        boldFmt.setFontWeight(QFont::Bold);
        setFormat(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), boldFmt);
    }

    static const QRegularExpression inlineCodeRe(R"(`[^`]+`)");
    QRegularExpressionMatchIterator codeIt = inlineCodeRe.globalMatch(s);
    while (codeIt.hasNext())
    {
        const QRegularExpressionMatch m = codeIt.next();
        QTextCharFormat codeFmt = format(static_cast<int>(m.capturedStart()));
        codeFmt.setFontFamily("monospace");
        setFormat(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), codeFmt);
    }

    if (suppressPythonHighlighting)
    {
        // Carry the suppressed state forward to the next block — skip the base class's
        // comment/string/number/function coloring (also code-oriented; a quoted word in prose
        // would otherwise be colored as if it were a Python string) and the trailing """
        // multi-line-comment state machine below, which would otherwise overwrite this state.
        setCurrentBlockState(aiAnswerBlockState);
        return;
    }

	SyntaxHighlighter::highlightBlock(text);//process common rules and parentheses matching

	int startIndex = text.indexOf(comment);
	int prevState = previousBlockState ();
	int comments = text.count(comment);

	if (comments > 1){
			int aux = 1;
			if (prevState == 1)
				setFormat(0, startIndex + 3, commentFormat);

			while (aux < comments) {
				int endIndex = text.indexOf(comment, startIndex + 3);
				aux++;
				if ((!prevState && (aux %2 == 0)) || (prevState == 1 && (aux %2 != 0)))
					setFormat(startIndex, endIndex - startIndex + 3, commentFormat);

				startIndex = endIndex;
			}

		int state = 0;
		if ((!prevState && (comments % 2 != 0)) || (prevState && (comments % 2 == 0))){
			state = 1;
			setFormat(startIndex, text.length() - startIndex, commentFormat);
		}
		setCurrentBlockState(state);
	} else if (comments == 1){
		if (prevState == 1){
			setCurrentBlockState(0);// end of comment block
			setFormat(0, startIndex + 3, commentFormat);
		} else {
			setCurrentBlockState(1);// start of comment block
			setFormat(startIndex, text.length() - startIndex, commentFormat);
		}
	} else {
		if (prevState == 1){
			setCurrentBlockState(1);// inside comment block
			setFormat(0, text.length(), commentFormat);
		} else
			setCurrentBlockState(0);// outside comment block
	}
}

SyntaxHighlighter::SyntaxHighlighter(ScriptEdit * parent) : QSyntaxHighlighter(parent->document())
{
	HighlightingRule rule;
	ApplicationWindow *app = parent->scriptingEnv()->application();

	functionFormat.setFontItalic(true);
	functionFormat.setForeground(app->d_function_highlight_color);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);

	numericFormat.setForeground(app->d_numeric_highlight_color);
    rule.pattern = QRegularExpression(R"(\b\d+[eE.,]*\d*\b)");
	rule.format = numericFormat;
	highlightingRules.append(rule);

	quotationFormat.setForeground(app->d_quotation_highlight_color);
    rule.pattern = QRegularExpression("\".*?\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);

	commentFormat.setForeground(app->d_comment_highlight_color);
    rule.pattern = QRegularExpression("#[^\n]*");
	rule.format = commentFormat;
	highlightingRules.append(rule);
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *doc) : QSyntaxHighlighter(doc)
{
    HighlightingRule rule;

    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    numericFormat.setForeground(QColor(0, 128, 0));
    rule.pattern = QRegularExpression(R"(\b\d+[eE.,]*\d*\b)");
    rule.format = numericFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(QColor(163, 21, 21));
    rule.pattern = QRegularExpression("\".*?\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    commentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

}

//! Parentheses matching code taken from Qt Quarterly Issue 31 - Q3 2009
void SyntaxHighlighter::highlightBlock(const QString &text)
{
	QString s = text;
	foreach (HighlightingRule rule, highlightingRules) {
        QRegularExpression re(rule.pattern);
        QRegularExpressionMatchIterator i = re.globalMatch(s);
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
		}
	}

	TextBlockData *data = new TextBlockData;

    int leftPos = text.indexOf('(');
    while (leftPos != -1) {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = '(';
        info->position = leftPos;

        data->insert(info);
        leftPos = text.indexOf('(', leftPos + 1);
    }

    int rightPos = text.indexOf(')');
    while (rightPos != -1) {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = ')';
        info->position = rightPos;

        data->insert(info);

		rightPos = text.indexOf(')', rightPos + 1);
    }

    setCurrentBlockUserData(data);
}
