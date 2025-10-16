/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Command completion and historization class for a QLineEdit Widget
 ******************************************************************************/

#include "SmartCompleter.h"

SmartCompleter::SmartCompleter(QLineEdit *edit, const QStringList &keywords, QObject *parent)
    : QObject(parent), m_lineEdit(edit)
{
    Q_ASSERT(edit);

    m_keywordModel = new QStringListModel(keywords, this);
    m_historyModel = new QStringListModel(this);

    // --- Keyword completer ---
    m_completer = new QCompleter(m_keywordModel, this);
    m_completer->setWidget(edit);
    m_completer->setCaseSensitivity(Qt::CaseSensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setFilterMode(Qt::MatchStartsWith);

    // --- History completer ---
    m_historyCompleter = new QCompleter(m_historyModel, this);
    m_historyCompleter->setWidget(edit);
    m_historyCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_historyCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_historyCompleter->setFilterMode(Qt::MatchStartsWith);

    // --- Event filters ---
    edit->installEventFilter(this);
    if (m_completer && m_completer->popup())
        m_completer->popup()->installEventFilter(this);
    if (m_historyCompleter && m_historyCompleter->popup())
        m_historyCompleter->popup()->installEventFilter(this);

    // Global event filter for Enter blocking and TAB
    qApp->installEventFilter(this);

    // --- Connections ---
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated), this,
            [this](const QString &s) { selectCompletion(m_completer, s); });
    connect(m_historyCompleter, QOverload<const QString &>::of(&QCompleter::activated), this,
            [this](const QString &s) { selectCompletion(m_historyCompleter, s); });

    connect(edit, &QLineEdit::textEdited, this, &SmartCompleter::updateCompletionPrefix);

    // Save history when Enter pressed (only if no popup is visible)
    connect(edit, &QLineEdit::returnPressed, this, [this]() {
        if (!m_blockReturn)
        {
            QString text = m_lineEdit->text().trimmed();
            if (text.isEmpty())
                return;

            QStringList hist = m_historyModel->stringList();
            if (!hist.contains(text))
            {
                hist << text;
                m_historyModel->setStringList(hist);
            }
        }
    });
}

void SmartCompleter::addKeywords(const QStringList &list)
{
    QStringList current = m_keywordModel->stringList();
    current.append(list);
    current.removeDuplicates();
    m_keywordModel->setStringList(current);
}

bool SmartCompleter::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_lineEdit)
        return QObject::eventFilter(obj, event);

    // --- Popup show/hide detection ---
    if ((obj == m_completer->popup() || obj == m_historyCompleter->popup()) &&
        (event->type() == QEvent::Show || event->type() == QEvent::Hide))
    {
        m_blockReturn = (event->type() == QEvent::Show);
        return QObject::eventFilter(obj, event);
    }

    // --- Global key press interception ---
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        int key = keyEvent->key();

        // ENTER handling → select completion
        if ((key == Qt::Key_Return || key == Qt::Key_Enter) && m_lineEdit->hasFocus() && m_blockReturn)
        {
            QCompleter *active = nullptr;
            if (m_completer->popup()->isVisible())
                active = m_completer;
            else if (m_historyCompleter->popup()->isVisible())
                active = m_historyCompleter;

            if (active && active->popup()->currentIndex().isValid())
            {
                QString completion = active->popup()->currentIndex().data().toString();
                selectCompletion(active, completion);
                active->popup()->hide();
            }

            keyEvent->accept();
            return true;
        }

        // TAB → show full history popup
        if (key == Qt::Key_Tab && m_lineEdit->hasFocus() && !m_blockReturn && !m_historyModel->stringList().isEmpty())
        {
            QTimer::singleShot(0, [this]() {
                m_historyCompleter->setCompletionPrefix("");
                m_historyCompleter->complete();
            });

            keyEvent->accept();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void SmartCompleter::updateCompletionPrefix()
{
    if (!m_lineEdit)
        return;

    int cursorPos = m_lineEdit->cursorPosition();
    QString beforeCursor = m_lineEdit->text().left(cursorPos);
    if (beforeCursor.isEmpty())
        return;

    int lastDot = static_cast<int>(beforeCursor.lastIndexOf('.'));
    int lastSpace = static_cast<int>(beforeCursor.lastIndexOf(' '));
    int sepPos = qMax(lastDot, lastSpace);

    QString lastSegment = (sepPos >= 0) ? beforeCursor.mid(sepPos + 1) : beforeCursor;
    lastSegment = lastSegment.trimmed();
    if (lastSegment.isEmpty())
        return;

    m_completer->setCompletionPrefix(lastSegment);
    m_completer->complete();
}

void SmartCompleter::selectCompletion(QCompleter *active, const QString &completion)
{
    if (!m_lineEdit || !active)
        return;

    QString full;

    if (active == m_completer)
    {
        // --- keyword completer behavior (replace last segment) ---
        QString text = m_lineEdit->text();
        int cursorPos = m_lineEdit->cursorPosition();
        QString before = text.left(cursorPos);

        int lastDot = static_cast<int>(before.lastIndexOf('.'));
        int lastSpace = static_cast<int>(before.lastIndexOf(' '));
        int sepPos = qMax(lastDot, lastSpace);

        QString prefix = (sepPos >= 0) ? before.left(sepPos + 1) : "";
        full = prefix + completion;

        bool afterSpace = (lastSpace > lastDot);
        if (!completion.isEmpty() && !afterSpace)
        {
            QChar start = completion[0];
            if (start.isUpper())
                full += '.';
            else if (start.isLower())
            {
                full += "()";
                m_lineEdit->setText(full);
                m_lineEdit->setCursorPosition(static_cast<int>(full.length()) - 1);
                if (m_completer->popup())
                    m_completer->popup()->hide();
                goto add_history;
            }
        }
        if (m_completer->popup())
            m_completer->popup()->hide();
    }
    else if (active == m_historyCompleter)
    {
        // --- history completer replaces entire line ---
        full = completion;
        if (m_historyCompleter->popup())
            m_historyCompleter->popup()->hide();
    }

    m_lineEdit->setText(full);
    m_lineEdit->setCursorPosition(static_cast<int>(full.length()));

add_history: // Add to history

    QStringList hist = m_historyModel->stringList();
    if (!hist.contains(full))
    {
        hist << full;
        m_historyModel->setStringList(hist);
    }
}

