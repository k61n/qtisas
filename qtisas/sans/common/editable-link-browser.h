/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: QTextBrowser subclass that is both editable and has clickable links
 ******************************************************************************/

#pragma once

#include <QTextBrowser>

class QMouseEvent;

class EditableLinkBrowser : public QTextBrowser
{
  public:
    explicit EditableLinkBrowser(QWidget *parent = nullptr);

    // Load HTML from storage: replace canonical font-weight:16 with the runtime widget weight (Qt5 compat)
    void setNormalizedHtml(const QString &html);

    // Serialize to canonical HTML: CSS head, font-weight:16, no redundant span/p styles, &#160; blank lines
    QString toNormalizedHtml() const;

    void clearFormatting();

    // Inline markup
    void insertSup(bool active);
    void insertSub(bool active);
    void insertLink(bool active);

    // Greek toggling
    void toggleGreek();
    static bool containsGreek(const QString &text);

  private:
    void insertHtmlAndSelect(const QString &html);

  protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
};
