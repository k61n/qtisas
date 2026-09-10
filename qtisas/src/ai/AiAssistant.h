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

#include "AiContentProvider.h"
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

    // The other half of the same eventual-library split as setHost() above: this supplies
    // QtiSAS-specific KNOWLEDGE (self-review checklist, doc-topic descriptions, doc file loading)
    // rather than script EXECUTION — see AiContentProvider.h. Must be set once, right after
    // construction, before any response-handling method below is used.
    void setContentProvider(AiContentProvider *content)
    {
        m_content = content;
    }

    // docTopics: when non-empty, only *-python-api.md/.pyi files whose name contains one of these
    // (case-insensitive) substrings are sent, instead of the full doc set — see
    // AiContentProvider::loadDocs(). addToHistory: false for internal plumbing exchanges
    // (doc-topic selection, self-review) whose answer is consumed immediately and never needs to
    // be recalled later — see the call sites in ApplicationWindow::askAi() for why keeping those
    // out of m_history matters. includeHistory: false for a call whose prompt is already fully
    // self-contained (every fact it needs — the script under review, the original request, the
    // current message — is spelled out inline in userText itself) and gains nothing from also
    // seeing prior turns. Confirmed live: self-review, deep into a long split-mode conversation
    // (7-8 accumulated turns of prior parts' scripts resent as history on every call), twice
    // returned an entirely different, older script from that history instead of reviewing the
    // actual candidate placed in front of it — once reproducing a stale draft of the SAME script,
    // once reproducing a completely unrelated EARLIER part's script outright. Since self-review's
    // own prompt already inlines everything it needs, it never needed that history in the first
    // place — omitting it removes the only thing it could have latched onto by mistake.
    void ask(const QString &userText, bool includeDocs = true, const QString &systemPrompt = QString(),
             const QStringList &docTopics = QStringList(), bool addToHistory = true, bool includeHistory = true);
    // Needs setContentProvider() first — delegates to AiContentProvider::docTopicIndex().
    QString docTopicIndex() const;

    // Reads a preset's docs-mode setting from its "AI/DocsMode" int key. Migrates presets saved
    // before this setting existed (only the older "AI/Unlimited" bool present) — true maps to
    // Full, false/absent to Compact — so existing .ai preset files keep behaving the same way
    // after upgrading. Shared by ApplicationWindow::askAi()'s lazy preset load and ConfigDialog's
    // preset picker so both agree on the same migration rule.
    static DocsMode docsModeFromSettings(const QSettings &settings);

    // Parses a leading "SPLIT:"/"NOSPLIT:" marker off `text` and decides whether split-mode should
    // activate for this request, combined with `splitByDefault` (the host's persisted default:
    // "SPLIT:" opts in when it's off, "NOSPLIT:" opts out for this one request when it's on).
    // Purely generic text parsing with no host-specific knowledge, so it lives here rather than in
    // the host's own request-entry-point code. `effectiveTextOut`, if non-null, receives `text`
    // with the marker (if any) stripped — the form every later use of the request text should use.
    static bool parseSplitMarker(const QString &text, bool splitByDefault, QString *effectiveTextOut = nullptr);

    // Same convention as parseSplitMarker() above, for "REVIEW"/"NOREVIEW" controlling self-review
    // instead of split-mode. `reviewByDefault` is the host's persisted default.
    static bool parseReviewMarker(const QString &text, bool reviewByDefault, QString *effectiveTextOut = nullptr);

    // Classifies the REAL user's own free-form reply to the "is this a new task or a
    // continuation?" clarifying question (see awaitingContinuationClarification's doc comment) as
    // "new task" (true) or "continue" (false, the default) — a broader keyword set than
    // handleContinuationCheckResponse()'s model-facing one-word parsing, since a human answers in
    // natural language, not a constrained single word. Defaults to "continue" when nothing
    // matches — fail-open, same philosophy as every other verify-and-retry in this class, and
    // matches today's pre-existing behavior (keep history) for an answer that doesn't clearly say
    // otherwise. Purely generic text classification with no host-specific knowledge, so it lives
    // here (public static, like parseSplitMarker() above) rather than behind AiContentProvider.
    static bool looksLikeNewTaskReply(const QString &reply);

    // Classifies a short follow-up like "run it"/"execute that"/"run this" as a literal request to execute the
    // just-shown, not-yet-run candidate script — as opposed to a fresh request that merely happens to start with "run".
    // Deliberately narrow (whole-message match, not substring) so it only fires for the unambiguous case; a longer
    // message ("run it and then plot the results") falls through to normal dispatch instead. Same reasoning as
    // looksLikeNewTaskReply() above for living here.
    static bool looksLikeRunItRequest(const QString &text);

    // "run it again"/"do that again" — distinct from looksLikeRunItRequest() above: "again" is an
    // explicit signal the user knows (or doesn't care) whether the last script already ran, and
    // deliberately wants a repeat, side effects included. Callers gate this on the last script
    // simply EXISTING (historySize() > 0), not on hasUnrunLastScript() — an already-run script is
    // exactly the normal case "again" is for.
    static bool looksLikeRunItAgainRequest(const QString &text);

    // "run Q4"/"run A4"/"execute Q4" — re-runs a SPECIFIC numbered turn from historyText()'s own Q/A numbering
    // (1-based), not just the last one. Unlike the two checks above, the index is explicit in the text itself, so
    // there's no "which script did they mean" ambiguity to gate on — any existing index is fair game, including one
    // before m_contextHistoryStart (see its own doc comment). indexOut receives the parsed index only when true.
    static bool looksLikeRunHistoryIndexRequest(const QString &text, int *indexOut);

    // Runs m_history[oneBasedIndex - 1].second directly via runActionScript() — same reasoning as
    // runLastScript(), just addressed by explicit index instead of "the last turn". Returns false
    // (and runs nothing) when oneBasedIndex is out of [1, historySize()] range, so the caller can
    // report that instead of silently doing nothing.
    bool runHistoryScript(int oneBasedIndex);

    // True only in the narrow window between a real action script shown as "AI code (not run)" and either
    // runLastScript() consuming it or a new request superseding it (beginRequest() clears this unconditionally, since a
    // leftover script is no longer safely "the thing 'run it' refers to"). markLastScriptUnrun() sets it;
    // ApplicationWindow calls it right where it prints "AI code (not run)". Gates looksLikeRunItRequest()'s fast path
    // specifically; looksLikeRunItAgainRequest() deliberately does NOT check this.
    bool hasUnrunLastScript() const;
    void markLastScriptUnrun();

    // Runs the last turn's script via runActionScript(), skipping continuation-check/decomposition/generation — "run
    // it"/"run it again" mean one thing, and re-asking the model hallucinated instead (confirmed live). Call BEFORE
    // anything touches m_history this turn — reads m_history.last(), only correct while nothing from THIS turn is
    // appended. Clears m_lastScriptUnrun unconditionally; confirmationMessage prints only once detectKnownMistakes()
    // confirms it won't block — never beforehand, confirmed wrong right after "no re-generation needed" was shown.
    void runLastScript(const QString &confirmationMessage);

    void stop();
    void clearHistory();
    QString historyText() const;

    // Sets the config directory NAME userRulesFilePath()/requestTimingLogPath() build their path
    // under (e.g. "~/.config/<name>/" on Linux/macOS, "%LOCALAPPDATA%/<name>/" on Windows).
    // Defaults to "qtisas" — this app's own identity — so nothing needs to call this for QtiSAS
    // itself; exists so a different host embedding this class doesn't inherit QtiSAS's own config
    // directory. Call once at startup, before any of the static rule-management methods below run.
    static void setConfigDirName(const QString &name);
    static QString userRulesFilePath();
    static QStringList userRules();
    static bool addUserRule(const QString &text, int maxRules, QString *errorOut = nullptr);
    static bool removeUserRule(int index, QString *errorOut = nullptr);
    static bool clearUserRules();

    // Pure helpers used by ApplicationWindow::askAi()'s response handling — no QtiSAS-specific
    // dependencies (script execution, UI), so they live here rather than in ApplicationWindow.cpp.
    // selfReviewChecklist() needs setContentProvider() first — it's the one exception, since it
    // appends the generic user-rules mechanism after AiContentProvider's own (QtiSAS-specific)
    // checklist text.
    QString selfReviewChecklist() const;
    static QString extractAvailableBlock(const QString &err);
    static QString normalizeErrorForDedup(const QString &err);

    // Handles the doc-topic-selection round-trip's answer (parses topics, sets
    // selectedDocTopics, kicks off the real request). Call only when awaitingDocSelection is
    // true; it resets that flag itself. Needs setHost() to have been called first (uses it to
    // print the "AI doc selection: ..." line).
    void handleDocSelectionResponse(const QString &code);

    // Top-level entry point for a new request — replaces ApplicationWindow::askAi()'s own direct
    // dispatch to beginSplitPlanDecomposition()/the doc-selection preflight, so the "existing X"
    // verification gate (see extractExistingEntityNames()'s doc comment) stays fully encapsulated
    // here rather than leaking into ApplicationWindow. Also owns the per-request state reset
    // (autoRun/autoContinueCount/splitModeActive/splitPlan/... and pendingRequestText/
    // originalRequestText) that used to live inline in ApplicationWindow::askAi() — moved here so
    // that function stops reaching into AiAssistant's internal fields directly; it now only
    // computes `aiText` and `wantsSplit` (from the "SPLIT:"/"NOSPLIT:" marker) and hands both
    // straight to this one call. If `aiText` names anything as already EXISTING (via that exact
    // word, near a quoted name — e.g. `the existing "Sphere" model`, `existing table
    // "SAXS_data"`), dispatches a one-shot, read-only verification script BEFORE any split-plan/
    // generation call, and only proceeds (via dispatchAfterExistenceCheck()) once it reports every
    // name found. If nothing matches, proceeds immediately — zero overhead for the common case.
    // Rationale: by the time a split-plan/action script has already committed to a specific name,
    // silently discovering/substituting a different one (or worse, creating a fresh stand-in) has
    // repeatedly gone unannounced in this codebase's own history — see
    // unannouncedModelNameSubstitution()'s doc comment for the "Sphere" R²=-190 incident this
    // generalizes from a single Fittable-only check into a universal, kind-agnostic one.
    void beginRequest(const QString &aiText, bool autoRunValue, bool wantsSplit);

    // Entry point for starting split-mode decomposition of `aiText` — replaces a direct call to
    // ask(m_content->splitPlanPreflightPrompt(aiText), ...) so the new-task-vs-continuation check
    // (see awaitingContinuationCheck's doc comment) stays fully encapsulated here rather than
    // leaking into ApplicationWindow::askAi(). Sets awaitingDocSelection=false and then EITHER:
    // on a fresh conversation (m_history empty, nothing to conflict with) sets awaitingSplitPlan
    // and dispatches the split-plan preflight directly, unchanged from before; OR, when history
    // exists, sets awaitingContinuationCheck instead and dispatches the continuation-check
    // preflight first, deferring the split-plan preflight (and awaitingSplitPlan) until
    // handleContinuationCheckResponse() resolves it. Caller must NOT set awaitingSplitPlan itself
    // — exactly one of the two "awaiting" flags is live at a time, decided internally here.
    void beginSplitPlanDecomposition(const QString &aiText);

    // Handles the continuation-check pre-flight's answer — see beginSplitPlanDecomposition() and
    // AiContentProvider::continuationCheckPreflightPrompt(). Call only when awaitingContinuationCheck
    // is true (mirrors handleDocSelectionResponse()/handleSplitPlanResponse()'s own contract); it
    // resets that flag itself. CONTINUE or an unrecognized answer (fail-open, matching every other
    // verify-and-retry in this class) dispatches the split-plan preflight with
    // pendingSplitRequestText, history intact. NEW clears history/originalRequestText first,
    // printing a transparent notice, then does the same. UNCLEAR prints a clarifying question and
    // sets awaitingContinuationClarification instead of dispatching anything — the real user's own
    // next typed reply answers it (see ApplicationWindow::askAi()'s own interception of that flag).
    void handleContinuationCheckResponse(const QString &code);

    // Handles the "existing X" verification script's answer — see beginRequest()'s doc comment.
    // Call only when awaitingExistenceCheckScript is true; it resets that flag itself. Runs the
    // returned script for REAL via m_host->runScript() (it's read-only by construction — the
    // preflight prompt explicitly forbids anything else) and parses its printed "EXISTS: <name>"/
    // "MISSING: <name>" lines deterministically (missingEntityNames()) — never by trusting the
    // model's own summary of what it found. Any name reported MISSING stops the request entirely:
    // no split-plan, no generation, nothing created or run, with an explicit note that the fix
    // requires the user's own authorization, never a silently inferred one. If the script itself
    // doesn't compile or raises, fails OPEN (prints a note, proceeds anyway) — a broken safety
    // check must not become a new way to get permanently stuck, matching every other backstop in
    // this class.
    void handleExistenceCheckScriptResponse(const QString &code);

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
    // "not runnable" if false. `syntaxError` is set to canCompile()'s diagnostic text when
    // `compilable` comes back false — forwarded to runActionScript() so a syntax error can go
    // through the same retry machinery as a runtime error instead of dead-ending (see that
    // method's comment). Needs setHost() first.
    // `code` is a non-const reference: if the full text fails to compile but a leading paragraph
    // of natural-language commentary can be stripped to leave a compilable remainder (see
    // stripLeadingCommentaryParagraphs()'s comment), `code` is rewritten in place to that
    // remainder — the caller's copy (used for printing AND execution) picks up the fix too.
    bool maybeDispatchSelfReview(QString &code, bool &compilable, QString &syntaxError);

    // Call after maybeDispatchSelfReview() returns false, passing the `compilable`/`syntaxError`
    // it computed. `!compilable` no longer just reports "not runnable" and stops — it now feeds
    // `syntaxError` into the SAME dedup/give-up/retry-budget logic a runtime error already uses
    // below, so a malformed script (e.g. a stray leftover markdown-fence remnant, confirmed live)
    // gets a real chance at an automatic fix instead of silently stalling the conversation
    // forever with no further AI response. Needs setHost() first.
    void runActionScript(const QString &code, bool compilable, const QString &syntaxError = QString());

    // SPLIT MODE — opt-in decomposition of a compound request into an ordered sequence of
    // independently generated/reviewed/executed/retried parts (see ApplicationWindow::askAi()'s
    // "SPLIT:" marker). Each part uses the SAME handleExploreResponse()/maybeDispatchSelfReview()/
    // runActionScript() machinery as a normal request — the only new behavior is what happens
    // after a part's action script succeeds (advance to the next part, or merge). This is what
    // keeps a retry scoped to the CURRENT part only: splitPartIndex advances solely on success,
    // so a failing part's retries never re-touch earlier parts' already-completed work.

    // Handles the split-plan decomposition call's answer: parses "PART: ... | DOCS: ..." lines
    // into splitPlan, falling back to one synthetic part covering the whole request if nothing
    // parses (same fail-open philosophy as an unrecognized doc-selection answer). Prints the
    // plan, then starts part 0 via startSplitPart(). Call only when awaitingSplitPlan is true; it
    // resets that flag itself. Needs setHost() first.
    void handleSplitPlanResponse(const QString &code);

    // Starts generation for splitPlan[index]: resets the same per-request state a brand-new
    // whole request gets (autoContinueCount, awaitingSelfReview, lastAvailableBlock,
    // lastRetryError, isErrorRetryCorrection) so each part has its own full retry budget, then
    // sends that part's generation prompt scoped to only its own doc topics.
    void startSplitPart(int index);

    // Called by runActionScript() when splitModeActive and the CURRENT part's action script just
    // executed with no error. Records `code` into splitPartScripts, then either starts the next
    // part (startSplitPart()) or, if that was the last part, fires the merge (requestSplitMerge()).
    void advanceSplitPart(const QString &code);

    // One-shot, terminal call: asks the model to collapse all of splitPartScripts into ONE
    // standalone script reproducing the same end result from a fresh session. Sets
    // awaitingSplitMerge; handled by handleSplitMergeResponse(), which syntax-checks the answer
    // via canCompile() but NEVER runs it.
    void requestSplitMerge();

    // Handles the merge call's answer: syntax-checks via m_host->canCompile() (never runs it) and
    // prints it labeled "NOT executed — verify before reuse" either way. Ends split mode
    // (splitModeActive = false) regardless of outcome. Call only when awaitingSplitMerge is true;
    // it resets that flag itself. Needs setHost() first.
    void handleSplitMergeResponse(const QString &code);

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
    // Lets ApplicationWindow's "run it"/"run it again"/"run Q4" fast paths reject a concurrent
    // submission the same way beginRequest() does, instead of running while another request is
    // still in flight — see those call sites for why that matters.
    bool isBusy() const
    {
        return m_busy;
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
    // HTTP transfer timeout for loopback backends only — see ask()'s setTransferTimeout() call
    // for why this is separate from the cloud-API timeout branches.
    void setLocalTimeoutSec(int n)
    {
        m_localTimeoutSec = n;
    }
    int localTimeoutSec() const
    {
        return m_localTimeoutSec;
    }
    // Caps how many parts a single split-mode decomposition is allowed to produce — see
    // handleSplitPlanResponse()'s use of this for the rationale (bounds sequential HTTP
    // round-trips a pathologically over-fragmented plan would cause).
    void setMaxSplitParts(int n)
    {
        m_maxSplitParts = n;
    }
    int maxSplitParts() const
    {
        return m_maxSplitParts;
    }
    // When true (default), advanceSplitPart() requests a final, NEVER-EXECUTED merged reference
    // script after the last part completes (see requestSplitMerge()'s doc comment). When false,
    // split mode ends right after the last part runs — each part already ran independently, so
    // this only skips producing that standalone reference script.
    void setSplitMergeEnabled(bool on)
    {
        m_splitMergeEnabled = on;
    }
    bool splitMergeEnabled() const
    {
        return m_splitMergeEnabled;
    }
    // When true (default), maybeDispatchSelfReview() sends every compilable candidate script back
    // to the AI once for a mandatory pre-execution review pass (checklist + the 9 mechanical
    // regression backstops) before it ever runs. When false, that whole round-trip is skipped —
    // a compilable candidate goes straight to runActionScript(). detectKnownMistakes() and the
    // other pre-execution mechanical checks in runActionScript() itself still run either way; only
    // the SEPARATE review-and-correct pass is what this toggles.
    void setSelfReviewEnabled(bool on)
    {
        m_selfReviewEnabled = on;
    }
    bool selfReviewEnabled() const
    {
        return m_selfReviewEnabled;
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
    // Consumed by resetErrorRetryState() — declared here so that reset has a real counter to
    // clear; not yet incremented/read anywhere else in this commit.
    int consecutiveDocRetryCount = 0;
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
    // correction whose triggering error actually looked like a narrowed-availability substitution
    // (see the assignment site in runActionScript()) — NOT for a fresh, never-yet-run first
    // attempt (even one resembling an earlier, now-stale failure in a long conversation), and NOT
    // for a retry correcting a plain coding mistake (NameError/TypeError/AttributeError/...)
    // unrelated to that failure shape. Gates the "watch for scope reduction" self-review
    // instruction, which must never block a script that hasn't been given a chance to run, and
    // must never second-guess a correction to an unrelated mistake either — confirmed live,
    // severe: firing unconditionally on every retry let self-review discard an already-correct
    // fix (re-deriving a value from a table two other parts had just confirmed existed) for a
    // fabricated "table is missing" give-up message, silently dropping the task's entire final
    // plot.
    bool isErrorRetryCorrection = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN previous answer was caught fabricating an unverified "missing" claim (see
    // maybeDispatchSelfReview()'s use of AiContentProvider::isUnverifiedMissingClaim()) — prevents
    // that corrective retry from looping if the model repeats the same fabrication a second time;
    // its answer is accepted regardless of outcome, same fail-open philosophy as every other
    // one-shot verify-and-retry in this class.
    bool awaitingSelfReviewSkepticismRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN previous answer was caught RE-INTRODUCING a split-mode scope violation
    // (performing a different part's job) into an already-correct, violation-free draft — see
    // maybeDispatchSelfReview()'s use of AiContentProvider::splitPartScopeViolation() against both
    // the pre- and post-review script. Confirmed live, severe: given an already-correct,
    // plotting-free fit-comparison draft, self-review rewrote it into a much bigger script that
    // ALSO built an entire graph — content the reviewed draft never had, and something
    // runActionScript()'s own scope check catches too, but only AFTER burning the shared
    // error-retry/dedup budget as if it were an ordinary stuck-in-a-loop code error instead of
    // self-review's own repeatable mistake. Same one-shot, fail-open philosophy as
    // awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewScopeRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN previous answer changed the model-name argument to configure()/
    // setFunction() from what the pre-review draft had — see maybeDispatchSelfReview()'s use of
    // AiContentProvider::selfReviewFunctionNameSubstitution(). Self-review runs with
    // includeHistory=false (see ask()'s own call site), so it never sees an EXPLORE round's
    // printed fitFunctions() output earlier in the SAME conversation — it has no legitimate basis
    // to judge which function name is correct, unlike checklist items it IS grounded to check.
    // Confirmed live, severe: a correct, EXPLORE-verified "SphereSANS" was silently rewritten to
    // the plausible-sounding but never-discovered "Sphere", producing a guaranteed
    // "function not found" error and a wasted retry cycle. Same one-shot, fail-open philosophy as
    // awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewFunctionNameRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN previous answer falsely claimed this part's job is incomplete/not
    // performed, while the pre-review draft demonstrably contained a real creation/action call —
    // see maybeDispatchSelfReview()'s use of
    // AiContentProvider::selfReviewFalseIncompletenessClaim(). Confirmed live, severe: a part
    // correctly created and filled a table matching its own description; self-review's response —
    // self-contradictorily saying both "this script creates a table" AND "no table creation
    // occurs" in the same message — discarded it, judging it against a LATER part's job
    // (plotting) instead of its own. That false give-up ran clean (no exception) and was accepted
    // as a completed part, so the table was never actually created. Same one-shot, fail-open
    // philosophy as awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewIncompletenessRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit replaced a correct matrix(name)/table(name)/note(name)/
    // graphWindow(name) lookup with a from-scratch newMatrix(name, ...)/newTable(name, ...)/
    // newNote(name, ...)/newGraph(name, ...) recreation for the SAME name — see
    // maybeDispatchSelfReview()'s use of AiContentProvider::selfReviewLookupToCreationRegression().
    // Confirmed live: a correct `m = matrix("SphereMap")` (grounded in real cross-turn history the
    // draft-generation call could see but self-review cannot) was rewritten into
    // `m = newMatrix("SphereMap", -3.14159, 3.14159, 50, ...)` — an invented, nonexistent
    // constructor signature — raising TypeError on the very next run. Same one-shot, fail-open
    // philosophy as awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewLookupRegressionRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit replaced a correct enum-namespace constant argument (Qti3D.*, etc.)
    // with something else in the same call — see maybeDispatchSelfReview()'s use of
    // AiContentProvider::selfReviewEnumConstantRegression(). Confirmed live: a correct
    // setPlotStyle(Qti3D.FILLEDMESH) was rewritten into setPlotStyle("Filled Mesh") (TypeError),
    // and the runtime-error retry that followed it "fixed" the crash with a guessed bare int
    // (HIDDENLINE) instead of the requested FILLEDMESH — silently wrong output, no error. Same
    // one-shot, fail-open philosophy as awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewEnumRegressionRetry = false;

    // Split-mode: true for a request that opted in via the client-side "SPLIT:" marker (see
    // ApplicationWindow::askAi()). While true, a successful action-script run in
    // runActionScript() does not simply return — it calls advanceSplitPart() instead of ending
    // the exchange. Reset (together with the split fields below) on every new top-level askAi()
    // call, same lifetime as autoContinueCount et al.
    bool splitModeActive = false;
    // True only while beginRequest()'s "existing X" verification script is in flight — the
    // EARLIEST gate a request passes through, before even awaitingContinuationCheck/
    // awaitingSplitPlan/awaitingDocSelection. handleExistenceCheckScriptResponse() resets this.
    bool awaitingExistenceCheckScript = false;
    // The request text beginRequest() is gating behind the existence-check script — resumed with
    // (via dispatchAfterExistenceCheck()) once it reports every referenced name found, or if the
    // check itself failed open. Not consumed/cleared by the check itself.
    QString pendingExistenceCheckRequestText;
    // True only while beginSplitPlanDecomposition()'s continuation-check pre-flight (see
    // AiContentProvider::continuationCheckPreflightPrompt()) is in flight — mirrors
    // awaitingDocSelection/awaitingSplitPlan. Exactly one of this and awaitingSplitPlan is live at
    // a time; onReplyFinished() resets this and either dispatches the split-plan preflight itself
    // (CONTINUE/NEW answers) or pauses for the real user's own clarification (UNCLEAR answer, see
    // awaitingContinuationClarification). Confirmed live: without this check, an unrelated,
    // completely separate earlier request's own object names bled into a later, independent
    // request's split-mode decomposition and error-retry history, with no way to tell "this task's
    // own prior turn" apart from "an earlier, unrelated request's turn" in the same flat history.
    bool awaitingContinuationCheck = false;
    // True only while paused waiting for the REAL user's own typed reply clarifying whether their
    // new message is a new task or a continuation — set when continuationCheckPreflightPrompt()
    // answers UNCLEAR. ApplicationWindow::askAi() must check this BEFORE treating incoming text as
    // a new top-level request (mirrors how it already intercepts fixed text commands before normal
    // dispatch) — the reply answers this pending question, it is not itself a fresh request.
    bool awaitingContinuationClarification = false;
    // The split-mode request text beginSplitPlanDecomposition() is gating behind the
    // continuation-check (or the user's clarification) — resumed with once either resolves. Not
    // consumed/cleared by the check itself, only by whichever branch finally dispatches the
    // split-plan preflight.
    QString pendingSplitRequestText;
    // Mirrors awaitingDocSelection — true only while the decomposition pre-flight call is in
    // flight. handleSplitPlanResponse() resets it.
    bool awaitingSplitPlan = false;
    // Set when handleSplitPlanResponse() sends the plan BACK to the model once, pointing out which
    // part(s) failed to name an object they create despite the decomposition prompt already
    // instructing this explicitly — see that method's naming-completeness check. Cleared as soon
    // as the retry's own response is parsed, whether or not the naming gap is actually fixed this
    // time, so a plan that still fails proceeds anyway rather than retrying forever.
    bool awaitingSplitPlanNamingRetry = false;
    // Mirrors awaitingDocSelection/awaitingSplitPlan — true only while the final, NEVER-EXECUTED
    // merge call (see requestSplitMerge()) is in flight. handleSplitMergeResponse() resets it.
    bool awaitingSplitMerge = false;
    // Set when handleSplitMergeResponse() sends the merge back once, pointing out which part's
    // literal give-up fallback script didn't survive into the merged output (see that method's
    // fallback-content check). Cleared as soon as the retry's own response is parsed, whether or
    // not the substitution is actually fixed this time — same one-shot-then-accept-anyway
    // philosophy as awaitingSplitPlanNamingRetry above.
    bool awaitingSplitMergeRetry = false;

    // One decomposed part of a split-mode request — see handleSplitPlanResponse()'s "PART: ... |
    // DOCS: ..." parsing.
    struct SplitPart
    {
        QString description;   // self-contained, imperative description startSplitPart() builds
                               // that part's own generation prompt from
        QStringList docTopics; // this part's OWN doc topics, parallel to selectedDocTopics but
                               // scoped per-part; {"ai","app"} prepended by startSplitPart() the
                               // same way handleDocSelectionResponse() prepends them for a whole
                               // request
    };
    // Ordered plan from the decomposition call; empty until handleSplitPlanResponse() parses it
    // (or falls back to a single synthetic part covering the whole request — see that method).
    QList<SplitPart> splitPlan;
    // 0-based index into splitPlan of the part currently being generated/reviewed/executed/
    // retried; -1 before the first part starts. Advanced ONLY by advanceSplitPart(), on that
    // part's SUCCESSFUL run — a failing part's retries all resend with this SAME index unchanged,
    // which is exactly what keeps a retry scoped to the current part (runActionScript()'s
    // existing error-retry ask() call is untouched — it only ever regenerates whatever code it
    // was just given, never reaches back into earlier parts).
    int splitPartIndex = -1;
    // Final, actually-executed script text for each COMPLETED part, in order — captured by
    // advanceSplitPart() from runActionScript()'s own `code` parameter on success. Kept as a
    // dedicated list rather than reconstructed from m_history because self-review's corrected
    // script is NOT reliably the one stored in history (maybeDispatchSelfReview()'s own ask()
    // call uses addToHistory=false — a part that needed only a self-review fix, no runtime-error
    // retry, would leave just its PRE-review draft in m_history). This is the exact, final code
    // requestSplitMerge() feeds to the merge call.
    QStringList splitPartScripts;

  signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &error);
    void infoMessage(const QString &text);
    // Separate from infoMessage() specifically so its consecutive attempts can overwrite one
    // another in the UI (see ApplicationWindow::printHttpRetryStatus()) instead of each becoming
    // its own new line — infoMessage() also carries the per-call "sending..." diagnostic, which
    // must never be collapsed like this. isFirstAttempt distinguishes "append a new line" (this
    // retry sequence's first message) from "replace the previous line" (every attempt after).
    void httpRetryStatus(const QString &text, bool isFirstAttempt);

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

    // Dispatches `aiText` into split-mode decomposition or the plain doc-selection preflight,
    // whichever splitModeActive calls for — the shared resumption point for beginRequest() once
    // the existence-check gate (if any fired) has resolved. Mirrors beginSplitPlanDecomposition()'s
    // own null-m_content fallback.
    void dispatchAfterExistenceCheck(const QString &aiText);

    // The plain (non-split) half of dispatchAfterExistenceCheck() — lifted out of
    // ApplicationWindow::askAi() (which used to call this inline) so beginRequest()'s existence-
    // check gate can resume into it the same way it resumes into beginSplitPlanDecomposition().
    // Sets awaitingDocSelection=false and calls startRequest() directly when docsMode() is
    // Integrated (no doc content is ever attached regardless of topic, so asking which topics are
    // needed would be a wasted round-trip) or m_content is null; otherwise sets
    // awaitingDocSelection=true and dispatches the doc-selection preflight.
    void beginNonSplitRequest(const QString &aiText);

    // Shared by runActionScript()'s three "give up" branches and handleExploreResponse()'s
    // round-cap message: when splitModeActive, returns a note naming which part stalled and
    // that later parts/merge were never attempted, AND clears splitModeActive (so a later,
    // unrelated request doesn't inherit stale split state) — both as a side effect of one call.
    // Returns an empty string (no side effect) when not in split mode.
    QString splitModeGiveUpNote();

    // Clears the retry-loop tracking state runActionScript()'s error path reads/writes
    // (lastRetryError, lastAvailableBlock, isErrorRetryCorrection, consecutiveDocRetryCount) —
    // the subset of beginRequest()'s per-request reset that matters for a FRESH execution, as
    // opposed to conversation-level state (pendingRequestText, splitModeActive, ...) that doesn't
    // apply when no new model request is being made. beginRequest() calls this as part of its own
    // reset; runLastScript()/runHistoryScript() call it too, for the same reason — confirmed live:
    // without it, "run A2" re-running an old, already-abandoned script inherited lastRetryError
    // from that OLD, unrelated retry cycle, so hitting the identical error again falsely tripped
    // "same corrective explanation given twice in a row" — a check meant to detect a live retry
    // loop going nowhere, not a deliberate, standalone re-run of something already known to fail.
    void resetErrorRetryState();

    // Appends one line per HTTP response to a persistent, cross-session log — timestamp, latency,
    // request size, HTTP status, and any provider-reported rate-limit headers found (name contains
    // "ratelimit", case-insensitive; varies by provider, so collected generically rather than by a
    // hardcoded per-provider header list). Purpose: let the user correlate response latency against
    // their own account's reported remaining capacity over time — plain appended text, not surfaced
    // in the UI, since a live console line per request would add noise without helping that goal.
    static QString requestTimingLogPath();
    void logRequestTiming(QNetworkReply *reply) const;

    // Extracted pieces of ask() — see that method's own body for how they're assembled. Kept as
    // private members (not free functions) since each touches instance state (m_cachedDocs,
    // m_history, m_lastCallMs, ...), not just its own parameters.
    QString buildSystemPrompt(const QString &systemPrompt, bool includeDocs, const QStringList &docTopics);
    qsizetype windowedHistoryStart(bool includeHistory) const;
    QString buildSendingDiagnostic(qint64 payloadSize, qsizetype historyStart, const QStringList &docTopics) const;

    // Shared shape of self-review's three internal round-trips (the mandatory review itself, and
    // its two one-shot corrective backstops): no docs, no system-prompt override, and excluded
    // from history in both directions — each call's own prompt is fully self-contained and its
    // answer is consumed immediately, never resent or built upon. Replaces repeating
    // `ask(prompt, false, QString(), QStringList(), false, false)` at every one of those call
    // sites.
    void askSelfReviewRound(const QString &prompt);

    AiScriptHost *m_host = nullptr;
    AiContentProvider *m_content = nullptr;
    QNetworkAccessManager *m_nam;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
    int m_maxTokens = 4096;
    DocsMode m_docsMode = DocsMode::Compact;
    int m_maxUserRules = 20;
    int m_maxAutoContinue = 4;
    int m_maxSplitParts = 8;
    bool m_splitMergeEnabled = true;
    bool m_selfReviewEnabled = true;
    int m_localTimeoutSec = 600;
    QList<QPair<QString, QString>> m_history; // (user, assistant) pairs
    // See hasUnrunLastScript()/markLastScriptUnrun()/runLastScript()'s doc comments — true
    // only while m_history.last() is a shown-but-not-executed candidate script "run it" can still
    // unambiguously refer to. beginRequest() clears it unconditionally for any request that isn't
    // itself consumed as that fast path.
    bool m_lastScriptUnrun = false;
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
