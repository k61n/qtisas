/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Command completion and historization class for a QLineEdit Widget
 ******************************************************************************/

#include <QRegularExpression>

#include "SmartCompleter.h"

SmartCompleter::SmartCompleter(QLineEdit *edit, const QStringList &keywords, QObject *parent)
    : QObject(parent), m_lineEdit(edit)
{
    Q_ASSERT(edit);

    m_keywordModel = new QStringListModel(keywords, this);
    m_historyModel = new QStandardItemModel(this);

    // --- Keyword completer ---
    m_completer = new QCompleter(m_keywordModel, this);
    m_completer->setObjectName("keywords");
    m_completer->setWidget(edit);
    m_completer->setCaseSensitivity(Qt::CaseSensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setFilterMode(Qt::MatchStartsWith);

    // --- History completer ---
    m_historyCompleter = new QCompleter(m_historyModel, this);
    m_historyCompleter->setObjectName("history");
    m_historyCompleter->setWidget(edit);
    m_historyCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_historyCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_historyCompleter->setFilterMode(Qt::MatchStartsWith);
    m_historyCompleter->setCompletionRole(Qt::UserRole);

    // --- Member completer (after "Name.") ---
    m_memberCompleter = new QCompleter(new QStringListModel(this), this);
    m_memberCompleter->setObjectName("members");
    m_memberCompleter->setWidget(edit);
    m_memberCompleter->setCaseSensitivity(Qt::CaseSensitive);
    m_memberCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_memberCompleter->setFilterMode(Qt::MatchStartsWith);

    // --- Event filters ---
    edit->installEventFilter(this);
    m_memberCompleter->popup()->installEventFilter(this);
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
    connect(m_memberCompleter, QOverload<const QString &>::of(&QCompleter::activated), this,
            [this](const QString &s) { selectCompletion(m_memberCompleter, s); });

    connect(edit, &QLineEdit::textEdited, this, &SmartCompleter::updateCompletionPrefix);

    // Save history when Enter pressed (only if no popup is visible)
    connect(edit, &QLineEdit::returnPressed, this, [this]() {
        if (m_automaticHistory && !m_blockReturn)
            addHistoryEntry(m_lineEdit->text());
    });
}

void SmartCompleter::addHistoryEntry(const QString &text, const QString &mode)
{
    const QString entry = text.trimmed();
    if (entry.isEmpty())
        return;
    for (int row = 0; row < m_historyModel->rowCount(); ++row)
        if (m_historyModel->item(row)->data(Qt::UserRole).toString() == entry &&
            m_historyModel->item(row)->data(Qt::UserRole + 1).toString() == mode)
            return;
    auto *item = new QStandardItem(mode.isEmpty() ? entry : "[" + mode + "] " + entry);
    item->setData(entry, Qt::UserRole);
    item->setData(mode, Qt::UserRole + 1);
    m_historyModel->appendRow(item);
}

void SmartCompleter::setKeywords(const QStringList &list)
{
    m_keywordModel->setStringList(list);
}

void SmartCompleter::setKeywordCompletionEnabled(bool enabled)
{
    m_keywordEnabled = enabled;
    if (!enabled && m_completer->popup()->isVisible())
        m_completer->popup()->hide();
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

    const bool terminalTarget = obj == m_lineEdit || obj == m_completer->popup() ||
                                obj == m_historyCompleter->popup() || obj == m_memberCompleter->popup();
    if (terminalTarget && (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress))
    {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        const bool ctrl = keyEvent->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
        if (ctrl && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter))
        {
            // Modified Enter submits the typed command, even while a completion popup owns focus.
            // Claim ShortcutOverride too, before a window QAction can consume the shortcut.
            event->accept();
            if (event->type() == QEvent::KeyPress)
            {
                m_completer->popup()->hide();
                m_historyCompleter->popup()->hide();
                m_memberCompleter->popup()->hide();
                m_blockReturn = false;
                emit modifiedReturnPressed(keyEvent->modifiers());
            }
            return true;
        }
    }

    // --- Popup show/hide detection ---
    if ((obj == m_completer->popup() || obj == m_historyCompleter->popup() || obj == m_memberCompleter->popup()) &&
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
            if (m_memberCompleter->popup()->isVisible())
                active = m_memberCompleter;
            else if (m_completer->popup()->isVisible())
                active = m_completer;
            else if (m_historyCompleter->popup()->isVisible())
                active = m_historyCompleter;

            if (active && active->popup()->currentIndex().isValid())
            {
                QString completion = active->popup()->currentIndex().data(active->completionRole()).toString();
                selectCompletion(active, completion);
                active->popup()->hide();
            }

            keyEvent->accept();
            return true;
        }

        // Ctrl → hide popup if visible
        if ((key == Qt::Key_Control || key == Qt::Key_Meta) && terminalTarget)
        {
            m_memberCompleter->popup()->hide();
            if (m_completer->popup()->isVisible())
                m_completer->popup()->hide();
            else if (m_historyCompleter->popup()->isVisible())
                m_historyCompleter->popup()->hide();
        }

        // ESC → hide popup if visible, else clear line
        if (key == Qt::Key_Escape && m_lineEdit->hasFocus())
        {
            if (m_memberCompleter->popup()->isVisible())
                m_memberCompleter->popup()->hide();
            else if (m_completer->popup()->isVisible())
                m_completer->popup()->hide();
            else if (m_historyCompleter->popup()->isVisible())
                m_historyCompleter->popup()->hide();
            else
                m_lineEdit->clear();
            keyEvent->accept();
            return true;
        }

        // TAB → show full history popup
        if (key == Qt::Key_Tab && m_lineEdit->hasFocus() && !m_blockReturn && m_historyModel->rowCount() > 0)
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
    if (!m_lineEdit || !m_keywordEnabled)
        return;

    int cursorPos = m_lineEdit->cursorPosition();
    QString beforeCursor = m_lineEdit->text().left(cursorPos);
    if (beforeCursor.isEmpty())
        return;

    if (memberCompletion(beforeCursor))
    {
        m_completer->popup()->hide();
        return;
    }
    m_memberCompleter->popup()->hide();

    int lastDot = static_cast<int>(beforeCursor.lastIndexOf('.'));
    int lastSpace = static_cast<int>(beforeCursor.lastIndexOf(' '));
    int sepPos = qMax(lastDot, lastSpace);

    QString lastSegment = (sepPos >= 0) ? beforeCursor.mid(sepPos + 1) : beforeCursor;
    lastSegment = lastSegment.trimmed();
    if (lastSegment.isEmpty())
        return;

    m_completer->setCompletionPrefix(lastSegment);
    m_completer->complete();
    if (m_completer->completionCount() > 0)
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
}

// after "Name." or "Name.Sub.pre": the members of that object starting with "pre" (Dan.M -> Mask, Merge); false
// when there is no such chain or it names no object (then the keyword list completes)
bool SmartCompleter::memberCompletion(const QString &beforeCursor)
{
    static const QRegularExpression chain(R"(([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)\.(\w*)$)");
    const QRegularExpressionMatch m = chain.match(beforeCursor);
    if (!m_memberProvider || !m.hasMatch())
        return false;
    const QStringList members = m_memberProvider(m.captured(1));
    if (members.isEmpty())
        return false;
    QStringList matching;
    for (const QString &name : members)
        if (name.startsWith(m.captured(2)) && name != m.captured(2))
            matching << name;
    if (matching.isEmpty())
    {
        m_memberCompleter->popup()->hide();
        return true;
    }
    qobject_cast<QStringListModel *>(m_memberCompleter->model())->setStringList(matching);
    m_memberCompleter->setCompletionPrefix(QString());
    m_memberCompleter->complete();
    m_memberCompleter->popup()->setCurrentIndex(m_memberCompleter->completionModel()->index(0, 0));
    return true;
}

void SmartCompleter::selectCompletion(QCompleter *active, const QString &completion)
{
    if (!m_lineEdit || !active)
        return;

    QString full;

    if (active == m_completer || active == m_memberCompleter)
    {
        // --- keyword and member completer behavior (replace last segment) ---
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
                active->popup()->hide();
                goto add_history;
            }
        }
        active->popup()->hide();
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
    if (full.endsWith('.')) // "Dan.Mask.": its members at once
        QTimer::singleShot(0, this, &SmartCompleter::updateCompletionPrefix);

add_history: // Add to history

    if (m_automaticHistory)
        addHistoryEntry(full);
}
