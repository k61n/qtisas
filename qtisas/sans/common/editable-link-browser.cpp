/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: QTextBrowser subclass that is both editable and has clickable links
 ******************************************************************************/

#include "editable-link-browser.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFont>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QUrl>

EditableLinkBrowser::EditableLinkBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setReadOnly(false);
    setOpenLinks(false);
    document()->setDefaultFont(QFont("Arial", 12));
}

void EditableLinkBrowser::setNormalizedHtml(const QString &html)
{
    if (html.isEmpty())
    {
        setHtml(html);
        return;
    }
    QString h = html;
    const QString rtWeight = QString::number(font().weight());
    if (rtWeight != "16")
        h.replace("font-weight:16", "font-weight:" + rtWeight);
    const QString rtFamily = document()->defaultFont().family();
    if (rtFamily != "Arial" && !rtFamily.isEmpty())
        h.replace("font-family:'Arial'", "font-family:'" + rtFamily + "'");
    setHtml(h);
}

QString EditableLinkBrowser::toNormalizedHtml() const
{
    QString html = toHtml();
    QString rtWeight;
    {
        const auto b0 = html.indexOf("<body"), b1 = html.indexOf('>', b0);
        if (b0 >= 0 && b1 > b0)
        {
            const auto m = QRegularExpression(R"(font-weight:(\d+))").match(html.mid(b0, b1 - b0 + 1));
            if (m.hasMatch())
                rtWeight = m.captured(1);
        }
    }
    html =
        R"(<html><head><meta name= "qrichtext " content= "1 " /><style type="text/css">p { margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; }body { font-family:'Arial'; font-size:12pt; font-weight:16; font-style:normal; }</style></head>)" +
        html.right(html.length() - html.indexOf("<body"));
    {
        const auto bs = html.indexOf("<body"), be = html.indexOf('>', bs);
        if (bs >= 0 && be > bs)
        {
            QString bodyTag = html.mid(bs, be - bs + 1);
            if (!rtWeight.isEmpty() && rtWeight != "16")
                bodyTag.replace("font-weight:" + rtWeight, "font-weight:16");
            bodyTag.replace(QRegularExpression(R"( font-family:[^;]+;)"), "");
            bodyTag.replace(QRegularExpression(R"( font-size:[^;]+;)"), "");
            bodyTag.replace(QRegularExpression(R"( font-style:normal;)"), "");
            bodyTag.replace(QRegularExpression(R"( style="\s*")"), "");
            html = html.left(bs) + bodyTag + html.mid(be + 1);
        }
    }
    // Strip defaults only from the body — leave the hardcoded <head> CSS intact
    const qsizetype bodyStart = html.indexOf("<body");
    if (bodyStart >= 0)
    {
        QString body = html.mid(bodyStart);
        body.replace(QRegularExpression(R"(<p style="[^"]*">)"), "<p>");
        const QString defaultFamily = document()->defaultFont().family();
        if (defaultFamily != "Arial" && !defaultFamily.isEmpty())
            body.replace(" font-family:'" + defaultFamily + "';", " font-family:'Arial';");
        body.replace(QRegularExpression(R"( font-family:'Arial';)"), "");
        body.replace(QRegularExpression(R"( font-size:12pt;)"), "");
        body.replace(QRegularExpression(R"( font-weight:(?:0|16|48|50);)"), "");
        body.replace(QRegularExpression(R"( style="\s*")"), "");
        body.replace("<span><br /></span>", "<br />");
        body.replace("<p><br /></p>", "<p>&#160;</p>");
        body.replace(QRegularExpression(R"(<p><span>\s*</span></p>)"), "<p>&#160;</p>");
        html = html.left(bodyStart) + body;
    }
    return html;
}

void EditableLinkBrowser::clearFormatting()
{
    selectAll();
    setAlignment(Qt::AlignLeft);
    setAcceptRichText(false);
    selectAll();
    cut();
    paste();
    setAcceptRichText(true);
}


void EditableLinkBrowser::insertHtmlAndSelect(const QString &html)
{
    QTextCursor cursor = textCursor();
    const int startPos = cursor.position();
    cursor.insertHtml(html);
    const int endPos = cursor.position();
    cursor.setPosition(startPos, QTextCursor::MoveAnchor);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void EditableLinkBrowser::insertSup(bool active)
{
    blockSignals(true);
    QClipboard *cb = QApplication::clipboard();
    cb->clear();
    cut();
    insertHtmlAndSelect(active ? "<SUP>" + cb->text() + "</SUP>" : cb->text());
    blockSignals(false);
}

void EditableLinkBrowser::insertSub(bool active)
{
    blockSignals(true);
    QClipboard *cb = QApplication::clipboard();
    cb->clear();
    cut();
    insertHtmlAndSelect(active ? "<SUB>" + cb->text() + "</SUB>" : cb->text());
    blockSignals(false);
}

void EditableLinkBrowser::insertLink(bool active)
{
    blockSignals(true);
    QClipboard *cb = QApplication::clipboard();
    cb->clear();
    cut();
    if (active)
        insertHtmlAndSelect("<a href='" + cb->text() + "'>" + cb->text() + "</a>");
    else
        insertHtmlAndSelect(cb->text());
    blockSignals(false);
}

void EditableLinkBrowser::toggleGreek()
{
    QTextCursor cursor = textCursor();
    QString selected = cursor.selectedText();
    selected.remove(QRegularExpression("<.*?>"));
    for (QChar &ch : selected)
    {
        const ushort uc = ch.unicode();
        if (uc >= 'a' && uc <= 'z')
            ch = QChar(0x03B1 + (uc - 'a'));
        else if (uc >= 'A' && uc <= 'Z')
            ch = QChar(0x0391 + (uc - 'A'));
        else if (uc >= 0x03B1 && uc <= 0x03C9)
            ch = QChar('a' + (uc - 0x03B1));
        else if (uc >= 0x0391 && uc <= 0x03A9 && uc != 0x03A2)
            ch = QChar('A' + (uc - 0x0391));
    }
    cursor.removeSelectedText();
    cursor.insertText(selected);
}

bool EditableLinkBrowser::containsGreek(const QString &text)
{
    return std::any_of(text.cbegin(), text.cend(), [](QChar ch) {
        const ushort uc = ch.unicode();
        return (uc >= 0x0391 && uc <= 0x03A9 && uc != 0x03A2) || (uc >= 0x03B1 && uc <= 0x03C9);
    });
}

void EditableLinkBrowser::mousePressEvent(QMouseEvent *e)
{
    const QString anchor = anchorAt(e->pos());
    if (e->button() == Qt::LeftButton && !anchor.isEmpty())
    {
        QDesktopServices::openUrl(QUrl(anchor));
        return;
    }
    QTextBrowser::mousePressEvent(e);
}

void EditableLinkBrowser::mouseMoveEvent(QMouseEvent *e)
{
    viewport()->setCursor(anchorAt(e->pos()).isEmpty() ? Qt::IBeamCursor : Qt::PointingHandCursor);
    QTextBrowser::mouseMoveEvent(e);
}
