/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant — sends script/note text to an external or local LLM
             and returns generated Python code to the script console.
             API format is auto-detected from the base URL:
             URLs containing "anthropic.com" use the Anthropic Messages API;
             all others use the OpenAI-compatible Chat Completions API.
 ******************************************************************************/

#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSettings>
#include <QString>

#include "AiScriptHost.h"

class QTimer;

class AiAssistant : public QObject
{
    Q_OBJECT

  public:
    explicit AiAssistant(QObject *parent = nullptr);

    // Compact: send the terse .pyi stubs (~2500 tokens total). Full: send the full .md docs
    // (~16500 tokens) for large-context models. Integrated: send NO API doc content at all — for
    // a model/deployment that already has QtiSAS's API knowledge baked in (fine-tuned, an
    // uploaded knowledge base, or a custom server-side system prompt), so resending it would be
    // pure waste; the doc-selection pre-request is skipped entirely in this mode too (see
    // startRequest()), since asking "which topics do you need" is pointless when none are ever
    // attached regardless of the answer.
    enum class DocsMode
    {
        Compact,
        Full,
        Integrated
    };

    void configure(const QString &apiKey, const QString &baseUrl, const QString &model, int maxTokens = 4096,
                   DocsMode docsMode = DocsMode::Compact);

    // Part of the eventual split into a standalone library: the host is whatever actually runs
    // scripts and displays output (ApplicationWindow today). Must be set once, right after
    // construction, before any response-handling method below is used.
    void setHost(AiScriptHost *host)
    {
        m_host = host;
    }

    // docTopics: when non-empty, only *-python-api.md/.pyi files whose name contains one of these
    // (case-insensitive) substrings are sent, instead of the full doc set — see loadSystemPrompt().
    // addToHistory: false for internal plumbing exchanges (doc-topic selection, self-review) whose
    // answer is consumed immediately and never needs to be recalled later — see the call sites in
    // ApplicationWindow::askAi() for why keeping those out of m_history matters.
    void ask(const QString &userText, bool includeDocs = true, const QString &systemPrompt = QString(),
             const QStringList &docTopics = QStringList(), bool addToHistory = true);
    static QString docTopicIndex();

    // Reads a preset's docs-mode setting from its "AI/DocsMode" int key. Migrates presets saved
    // before this setting existed (only the older "AI/Unlimited" bool present) — true maps to
    // Full, false/absent to Compact — so existing .ai preset files keep behaving the same way
    // after upgrading. Shared by ApplicationWindow::askAi()'s lazy preset load and ConfigDialog's
    // preset picker so both agree on the same migration rule.
    static DocsMode docsModeFromSettings(const QSettings &settings);

    void stop();
    void clearHistory();
    QString historyText() const;

    static QString userRulesFilePath();
    static QStringList userRules();
    static bool addUserRule(const QString &text, int maxRules, QString *errorOut = nullptr);
    static bool removeUserRule(int index, QString *errorOut = nullptr);
    static bool clearUserRules();

    // Pure helpers used by ApplicationWindow::askAi()'s response handling — no QtiSAS-specific
    // dependencies (script execution, UI), so they live here rather than in ApplicationWindow.cpp.
    static QString selfReviewChecklist();
    static QString extractAvailableBlock(const QString &err);
    static QString normalizeErrorForDedup(const QString &err);

    // Handles the doc-topic-selection round-trip's answer (parses topics, sets
    // selectedDocTopics, kicks off the real request). Call only when awaitingDocSelection is
    // true; it resets that flag itself. Needs setHost() to have been called first (uses it to
    // print the "AI doc selection: ..." line).
    void handleDocSelectionResponse(const QString &code);

    // Restates the original-request anchor (see originalRequestText's declaration) and fires the
    // real request with `docTopics` attached. Shared by handleDocSelectionResponse() (after a
    // normal doc-selection answer) and by ApplicationWindow::askAi() directly when docsMode() is
    // Integrated — that mode never attaches any doc content regardless of topics, so the
    // doc-selection round-trip is skipped entirely and this is called with an empty topic list.
    void startRequest(const QStringList &docTopics);

    // Call only when `code` is confirmed to start with "# EXPLORE" (the caller keeps that check,
    // matching handleDocSelectionResponse's pattern); this method assumes it's definitely
    // handling one. Needs setHost() first (uses it for both print() and runScript()).
    void handleExploreResponse(const QString &code);

    // Sets `compilable` (via AiScriptHost::canCompile()) and returns true if self-review was
    // just dispatched (caller should return immediately without doing anything else with
    // `code`). Returns false otherwise — caller should proceed based on `compilable`: run it if
    // true (self-review already approved it, or this is its own returned correction), or report
    // "not runnable" if false. Needs setHost() first.
    bool maybeDispatchSelfReview(const QString &code, bool &compilable);

    // Call after maybeDispatchSelfReview() returns false, passing the `compilable` it computed —
    // this handles both outcomes ("not runnable" report, or run + error-retry/dedup logic).
    // Needs setHost() first.
    void runActionScript(const QString &code, bool compilable);

    QString baseUrl() const
    {
        return m_baseUrl;
    }
    QString model() const
    {
        return m_model;
    }
    QString apiKey() const
    {
        return m_apiKey;
    }
    int maxTokens() const
    {
        return m_maxTokens;
    }
    DocsMode docsMode() const
    {
        return m_docsMode;
    }
    int historySize() const
    {
        return static_cast<int>(m_history.size());
    }
    void setMaxUserRules(int n)
    {
        m_maxUserRules = n;
    }
    int maxUserRules() const
    {
        return m_maxUserRules;
    }
    void setMaxAutoContinue(int n)
    {
        m_maxAutoContinue = n;
    }
    int maxAutoContinue() const
    {
        return m_maxAutoContinue;
    }

    // Conversation-level orchestration state (doc-selection -> EXPLORE -> self-review ->
    // error-retry state machine). Each branch's handling has moved into the methods above, but
    // ApplicationWindow::askAi() still starts/resets most of this directly when a new request
    // comes in, and its responseReady lambda still does the top-level dispatch between branches
    // (though not each branch's actual handling). PUBLIC as an interim step: once askAi() itself
    // moves into AiAssistant, these become private implementation details instead of being poked
    // from outside.
    bool autoRun = false;
    int autoContinueCount = 0;
    bool awaitingSelfReview = false;
    bool awaitingDocSelection = false;
    QString pendingRequestText;
    // Set ONCE per conversation — the first askAi() call after history is empty/cleared — and
    // never overwritten by later short follow-ups ("try again", "noch mal", ...) in the same
    // conversation. pendingRequestText DOES get overwritten by those, which is exactly the
    // problem this field exists to avoid: conversation history is dominated by the model's own
    // recent turns (almost all code/errors), so a bare retry nudge without this explicit anchor
    // restated every time lets the model drift toward continuing its own last SCRIPT (e.g. a
    // substitute table it had already identified as wrong) instead of the original request.
    // Cleared together with pendingRequestText et al. by "clear history".
    QString originalRequestText;
    QStringList selectedDocTopics;
    QString lastAvailableBlock; // last-seen "Available X:\n  ..." block from a retry error
    QString lastRetryError;     // last retry's full error text, for exact-repeat detection
    // One-shot: true only for the self-review pass immediately following an error-retry
    // correction — NOT for a fresh, never-yet-run first attempt (even one resembling an earlier,
    // now-stale failure in a long conversation). Gates the "watch for scope reduction" self-review
    // instruction, which must never block a script that hasn't been given a chance to run.
    bool isErrorRetryCorrection = false;

  signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &error);
    void infoMessage(const QString &text);

  private slots:
    void onReplyFinished(QNetworkReply *reply);

  private:
    // Prefix reminding the model of the ORIGINAL request when this turn's own last user message
    // (pendingRequestText) differs from it — empty string otherwise. Prepend to EXPLORE-
    // continuation and error-retry prompts so a multi-round chain doesn't drift away from the
    // actual goal on nothing but the model's own history recall of it. See originalRequestText's
    // declaration for why a bare mention once, early in a growing, code-dominated history, isn't
    // enough by itself — confirmed live: a "create a table with X data" request drifted, several
    // EXPLORE rounds later, into repeatedly exploring an unrelated Fittable model and never
    // creating any table at all.
    QString originalRequestReminder() const;

    AiScriptHost *m_host = nullptr;
    QNetworkAccessManager *m_nam;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
    int m_maxTokens = 4096;
    DocsMode m_docsMode = DocsMode::Compact;
    int m_maxUserRules = 20;
    int m_maxAutoContinue = 4;
    QList<QPair<QString, QString>> m_history; // (user, assistant) pairs
    QString m_pendingUserText;
    bool m_pendingAddToHistory = true;
    QString m_cachedDocs; // loaded once; cleared by clearHistory() or configure()
    bool m_busy = false;
    QNetworkReply *m_currentReply = nullptr;
    QElapsedTimer m_requestTimer; // started right before post(); read in onReplyFinished()
    qint64 m_lastCallMs = -1;     // previous call's round-trip time; -1 until one call completes

    // Automatic retry for transient HTTP failures (503/502/504 "server unavailable", 429 rate
    // limit) — see onReplyFinished()'s emitHttpError for the retry decision and AiAssistant's
    // constructor for m_retryTimer's wiring. m_lastRequest/m_lastPayload hold the exact request
    // being retried, so a retry resends byte-for-byte rather than rebuilding (which could pick up
    // a different history window if time has passed).
    QNetworkRequest m_lastRequest;
    QByteArray m_lastPayload;
    int m_httpRetryCount = 0;
    QTimer *m_retryTimer;
};

#endif
