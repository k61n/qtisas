/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: QTextBrowser subclass that is both editable and has clickable links
 ******************************************************************************/

#pragma once

#include <QDesktopServices>
#include <QMouseEvent>
#include <QTextBrowser>
#include <QUrl>

class EditableLinkBrowser : public QTextBrowser
{
  public:
    explicit EditableLinkBrowser(QWidget *parent = nullptr) : QTextBrowser(parent)
    {
        setReadOnly(false);
        setOpenLinks(false);
    }

  protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        const QString anchor = anchorAt(e->pos());
        if (e->button() == Qt::LeftButton && !anchor.isEmpty())
        {
            QDesktopServices::openUrl(QUrl(anchor));
            return;
        }
        QTextBrowser::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        viewport()->setCursor(anchorAt(e->pos()).isEmpty() ? Qt::IBeamCursor : Qt::PointingHandCursor);
        QTextBrowser::mouseMoveEvent(e);
    }
};
