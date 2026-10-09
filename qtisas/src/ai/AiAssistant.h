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

    // docTopics: non-empty limits to matching *-python-api.md/.pyi files instead of the full doc set. addToHistory:
    // false for internal plumbing (doc-topic selection, self-review) whose answer is consumed immediately, never
    // recalled later. includeHistory: false when userText is self-contained, gaining nothing from prior turns.
    // Confirmed live: self-review in a long split-mode conversation twice returned an unrelated OLDER script instead of
    // the actual candidate. expectsCode: false for a preflight whose answer is plain text (a PART/DOCS plan, a topic
    // list, ...), never Python — gates exploreModeReminder() out. Confirmed live: even a passing EXPLORE mention
    // measurably increased the odds of a script back instead of the plan. historyText: non-empty is stored as this
    // exchange's "Q" side in history instead of userText, for an internally-constructed prompt that would otherwise
    // leak scaffolding into history; userText is still what's actually sent either way.
    void ask(const QString &userText, bool includeDocs = true, const QString &systemPrompt = QString(),
             const QStringList &docTopics = QStringList(), bool addToHistory = true, bool includeHistory = true,
             bool expectsCode = true, const QString &historyText = QString());

    // Needs setContentProvider() first — delegates to AiContentProvider::docTopicIndex().
    QString docTopicIndex() const;

    // Reads a preset's docs-mode setting from its "AI/DocsMode" int key. Migrates presets saved
    // before this setting existed (only the older "AI/Unlimited" bool present) — true maps to
    // Full, false/absent to Compact — so existing .ai preset files keep behaving the same way
    // after upgrading. Shared by ApplicationWindow::askAi()'s lazy preset load and ConfigDialog's
    // preset picker so both agree on the same migration rule.
    static DocsMode docsModeFromSettings(const QSettings &settings);

    // Finds "SPLIT"/"NOSPLIT" anywhere in `text` as a standalone, ALL-CAPS word (not glued to
    // adjacent letters) and decides whether split-mode should activate, combined with
    // `splitByDefault` (the host's persisted default). `effectiveTextOut`, if non-null, receives
    // `text` with the marker removed and surrounding whitespace collapsed.
    static bool parseSplitMarker(const QString &text, bool splitByDefault, QString *effectiveTextOut = nullptr);

    // Same convention as parseSplitMarker() above, for "REVIEW"/"NOREVIEW" controlling self-review
    // instead of split-mode. `reviewByDefault` is the host's persisted default.
    static bool parseReviewMarker(const QString &text, bool reviewByDefault, QString *effectiveTextOut = nullptr);

    // Same convention again, for "EXPLORE"/"NOEXPLORE" controlling whether the model is allowed a
    // read-only probe round before writing the action script. `exploreByDefault` is the host's
    // persisted default.
    static bool parseExploreMarker(const QString &text, bool exploreByDefault, QString *effectiveTextOut = nullptr);

    // Result of parseListRequest() below.
    struct ParsedListRequest
    {
        bool isList = false;
        bool forceSplit = false;     // "SPLIT" found in the instruction_block region
        bool forceNoSplit = false;   // "NOSPLIT" found there
        bool forceReview = false;    // "REVIEW" found there
        bool forceNoReview = false;  // "NOREVIEW" found there
        bool forceExplore = false;   // "EXPLORE" found there
        bool forceNoExplore = false; // "NOEXPLORE" found there
        QString instructionBlock;    // whatever's left after stripping the six markers above
        QStringList items;           // each numbered item's own text, in order
    };
    // Detects a leading "LIST" marker (case-sensitive, capital-only — deliberately stricter than
    // parseSplitMarker()'s looser convention, since this bypasses the whole split-plan LLM
    // decomposition rather than just toggling it) followed by a numbered list (2+ lines matching
    // "N. text"/"N) text"). SPLIT/NOSPLIT/REVIEW/NOREVIEW/EXPLORE/NOEXPLORE, in any combination or
    // order, are found and
    // stripped from the region between "LIST" and the first numbered item — only genuinely
    // leftover text there becomes instruction_block, so a region that's markers-only never
    // becomes an instruction appended to every item. Returns
    // isList=false (all other fields empty/default) when no marker or fewer than 2 numbered items
    // are found — deliberately fails open to a normal request rather than misfiring on a request
    // that merely happens to contain a number. Purely generic text parsing with no host-specific
    // knowledge, so it lives here rather than in the host's own request-entry-point code, matching
    // parseSplitMarker()'s own reasoning above.
    static ParsedListRequest parseListRequest(const QString &text);

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

    // Extracts the missing name from a "Function 'X' not found"/"function 'x' not found." error
    // (Compiler/Fittable's own wording, case varies by call site) — empty if the error doesn't
    // match. Used to gate the compiled-function-history reminder below to the one error shape it
    // actually helps with, rather than injecting it into every unrelated retry.
    static QString functionNotFoundNameFromError(const QString &errContext);

    // Scans every assistant-side turn in `history` for setFunctionName("Y") calls, returning the
    // unique names in first-seen order — a record of every function this exact conversation has
    // already tried to compile, regardless of whether that particular attempt is known to have
    // succeeded. Confirmed live, severe: after two functions (GaussPeakModel, LorentzianPeakModel)
    // genuinely compiled in the first, doc-informed attempt, later retries progressively lost
    // track of them and fabricated THREE more distinct, never-compiled names in a row
    // (gauss_peak, lorentz_peak, ai/Gauss) instead of reusing or even re-checking the real ones —
    // m_history itself only ever stores generated code, never execution outcomes, so nothing
    // grounded the model back to its own prior work once retries stopped resending docs.
    static QStringList compiledFunctionNamesFromHistory(const QList<QPair<QString, QString>> &history);

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

    // Entry point for LIST mode — `parsed` is parseListRequest()'s own output. `splitByDefault`/
    // `reviewByDefault` are the host's persisted settings, used only when neither of a pair's
    // force flags is set. A non-empty parsed.instructionBlock is appended to every item as it is
    // (itemsWithInstruction()), with no model call.
    void beginListRequest(const ParsedListRequest &parsed, bool splitByDefault, bool reviewByDefault,
                          bool exploreByDefault, bool autoRunValue);

    // Extracts just the numbered items (no marker line) from arbitrary text — parseListRequest()'s
    // own item-extraction.
    static QStringList parseNumberedListItems(const QString &text);

    // Every item with the LIST's instruction_block appended, so the instruction reaches each item as written.
    static QStringList itemsWithInstruction(const QStringList &items, const QString &instruction);

    // Dispatches the split-plan decomposition preflight for `aiText` directly — no continuation-
    // check gating here (see dispatchAfterExistenceCheck()'s doc comment; that gate now lives one
    // level up and applies to both split and non-split requests). Sets awaitingSplitPlan=true.
    void beginSplitPlanDecomposition(const QString &aiText);

    // Handles the continuation-check pre-flight's answer — see dispatchAfterExistenceCheck() and
    // AiContentProvider::continuationCheckPreflightPrompt(). Call only when awaitingContinuationCheck
    // is true (mirrors handleSplitPlanResponse()'s own contract); it
    // resets that flag itself. CONTINUE or an unrecognized answer (fail-open, matching every other
    // verify-and-retry in this class) re-anchors originalRequestText to pendingDispatchRequestText
    // and calls dispatchByMode() with it, history intact. NEW clears history first, printing a
    // transparent notice, then does the same. UNCLEAR prints a clarifying question and sets
    // awaitingContinuationClarification instead of dispatching anything — the real user's own next
    // typed reply answers it (see ApplicationWindow::askAi()'s own interception of that flag).
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
    // real request with `docTopics` attached. Called by startRequestWithSelectedTopics() with the
    // topics matched from the request's keywords, by the split-mode paths with a part's topics,
    // and directly with an empty topic list from the handful of "no content provider" fallbacks
    // that can't phrase any preflight at all.
    void startRequest(const QStringList &docTopics, const QString &resolvedTaskDescription = QString());

    // "\n\nTOPICS: ..." line for an optional server-side RAG proxy to scope ITS OWN retrieval —
    // shared by every real code-generation call site (startRequest(), the EXPLORE follow-up, the
    // error-retry prompt; startSplitPart() builds its own equivalent inline since it also needs
    // the per-part noneOrAmbiguous distinction). Returns an empty string outside
    // DocsMode::Integrated or when there are no topics to hint at all. Always folds in
    // baselineDocTopics() and drops kNoDocsSentinel — see the .cpp definition's doc comment for
    // why (this is pure signal for an optional retrieval step, not the LOCAL doc-attachment
    // choice docTopics itself protects at its own call sites).
    QString topicsHintLine(const QStringList &docTopics) const;

    // "\n\nTOPICS: none" — the explicit-zero-retrieval counterpart to topicsHintLine() above, for
    // every split-mode preflight/retry call that only classifies, decomposes, or patches a plan
    // (continuation-check, split-plan decomposition, and its naming/coverage/fragment retries).
    // None of those need domain API doc content, but leaving them with no TOPICS: line at all
    // previously left qtisas-rag's files_from_topics() returning None, which retrieve() then
    // treats as "no signal — search the whole corpus" rather than "no docs wanted" (see core.py's
    // matching files=[] handling). Empty outside DocsMode::Integrated, same as topicsHintLine().
    QString noDocsHintLine() const;

    // Call only when `code` is confirmed to start with "# EXPLORE" (the caller keeps that check,
    // matching handleSplitPlanResponse's pattern); this method assumes it's definitely
    // handling one. Needs setHost() first (uses it for both print() and runScript()).
    void handleExploreResponse(const QString &code);

    // EXPLORE is split into a policy point plus three units carrying out its outcome — see
    // exploreDisposition()'s doc comment in the .cpp for why one 247-line function was the problem.
    enum class ExploreDisposition
    {
        Run,    // execute the read-only prefix and feed its output back
        Refuse, // show it, never run it, ask for the action script instead
    };
    // Every "may a probe run for this request" rule, in one place: the Preferences toggle, the
    // NOEXPLORE marker, and generate-only mode.
    ExploreDisposition exploreDisposition() const;
    // Invite a probe, forbid one, or say nothing — see the .cpp definition.
    // Warnings worth showing beside a script that is SHOWN and never run. Generate-only mode
    // bypasses runActionScript() entirely (ApplicationWindow's own autoRun branch), so none of the
    // pre-run analyses that mode's scripts would otherwise face — scope violation, known mistakes,
    // self-review — ever see them. This is the one hook that path has, so a finding is REPORTED
    // rather than retried: a retry would spend a round-trip in the one mode chosen for not spending
    // them, and the user is reading the script anyway. Empty when nothing is worth saying.
    QString generatedScriptWarning(const QString &code) const;

    // Descriptions of every split-mode part AFTER splitPartIndex, newline-joined (empty outside
    // split mode or on the last part) — lets detectKnownMistakes() tell "nobody creates this" apart
    // from "a LATER part creates this" (see AiContentProvider.h's doc comment).
    QString laterSplitPartDescriptions() const;

    QString exploreSlotText(bool expectsCode) const;

    // Builds the VERIFIED PROJECT STATE note for every name `requestText` calls existing, by asking
    // the host what it actually holds — no script, no round-trip. Empty when the request names
    // nothing, when there is no host/content provider, or when the host answers nothing.
    // `consoleLine`, if non-null, also receives a one-line human summary of the same facts: this
    // path replaces a check whose output the user USED to see scroll past, and silently deciding a
    // name is missing would otherwise be invisible right up until the script creates something.
    QString liveEntityFactsFor(const QString &requestText, QString *consoleLine = nullptr) const;
    // When scripts run: the request's input names (inputNamesInRequest()) that exist NOW, as a PROJECT STATE note
    // for the model, read from the host without a script. Confirmed live (local model): "In \"Squares\", give
    // column 3 ..." got newTable("Squares", ...), wiping the table an earlier item had filled, in every run.
    // existingTables receives the names that exist as tables (see existingTableRecreatedMistake()).
    QString existingInputNamesNote(const QString &requestText, QStringList *existingTables,
                                   QString *consoleLine = nullptr) const;
    // newTable(name) for a table this request only uses: newTable on an existing name wipes it.
    QString existingTableRecreatedMistake(const QString &code) const;
    // The script without 1, 2 or 3 trailing lines that may be no Python (a weak model's "expected output" after its
    // code), smallest cut first; empty if no line qualifies. The caller takes the first one that compiles.
    static QStringList withoutTrailingOutputLines(const QString &code);
    // The script without its last top-level lines of the form "R1: 385" / "R12: []" — valid Python (an annotated
    // name), so withoutTrailingOutputLines() never sees them; empty if there are none.
    static QString withoutTrailingAnnotationOutput(const QString &code);
    // The removed trailing text has a "label: values" line whose label no string in the script prints: the model
    // wrote its result line as text instead of printing it. Empty if not.
    static QString unprintedOutputLineMistake(const QString &code, const QString &removedText);
    // The script with that missing label put in front of its LAST line, when that line is a top-level print() or
    // scriptPrint() of the values; empty if it is not.
    static QString labeledLastPrintRepair(const QString &code, const QString &removedText);
    // A script that only prints string literals — a text answer, nothing for self-review to check.
    static bool isPureTextAnswer(const QString &code);
    // A short question with no action verb ("there are spheres?"): answered directly, never split into a plan.
    static bool isShortQuestion(const QString &requestText);
    // Charges one round against the shared EXPLORE/error-retry budget; true once it is spent (the
    // give-up has then already been reported and recorded).
    bool exploreRoundBudgetExhausted(bool refusing);
    void printExploreScript(const QString &code);
    void refuseExploreRound(const QString &code);
    // False means stop: no host, or the reply bundled an action script that has now already run.
    bool runExploreProbe(const QString &code, QString *output, QString *discardedDraft);
    void requestActionScriptAfterExplore(const QString &code, const QString &output, const QString &discardedDraft);

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
    // parses (same fail-open philosophy as an unrecognized doc-selection answer). Call only when
    // awaitingSplitPlan is true; it resets that flag itself. Needs setHost() first. Once the plan
    // is fully finalized (fixup rounds converged or exhausted), prints the plan summary — always,
    // even if the collision check below is about to halt the request; a halt must never leave the
    // user unable to see what the plan actually finalized to — then hands off to
    // finalizeSplitPlanAndLaunch(), either directly or after a round-trip verifying none of the
    // plan's to-be-created names collide with something that already exists (see
    // splitPlanCreationNames()/splitPlanCreationCollisions()).
    void handleSplitPlanResponse(const QString &code);

    // Resumption point once handleSplitPlanResponse() has a fully finalized, already-printed plan
    // with no pending creation-name collision check: collapses to a plain request if the plan
    // settled on a single part, otherwise starts part 0 via startSplitPart().
    void finalizeSplitPlanAndLaunch();

    // Handles the creation-name collision-check script's answer — see
    // handleSplitPlanResponse()'s doc comment. Call only when
    // awaitingSplitPlanCreationCheckScript is true; it resets that flag itself. A collision halts
    // the request outright (mirrors handleExistenceCheckScriptResponse()'s halt for a MISSING
    // claimed-existing name); otherwise proceeds via finalizeSplitPlanAndLaunch().
    void handleSplitPlanCreationCheckScriptResponse(const QString &code);

    // Starts generation for splitPlan[index]: resets the same per-request state a brand-new
    // whole request gets (autoContinueCount, awaitingSelfReview, lastAvailableBlock,
    // lastRetryError, isErrorRetryCorrection) so each part has its own full retry budget, then
    // sends that part's generation prompt scoped to only its own doc topics.
    void startSplitPart(int index);

    // Called by runActionScript() when splitModeActive and the CURRENT part's action script just
    // executed with no error. Records `code` into splitPartScripts, then either starts the next
    // part (startSplitPart()) or, if that was the last part, fires the merge (requestSplitMerge()).
    void advanceSplitPart(const QString &code);

    // " (took Xsec)" for the item that just finished (empty before any item has), same "Xsec"
    // style as m_lastCallMs's own formatting — read by startNextListItem() right before it
    // increments listItemIndex, so it always describes the PREVIOUS item, never the new one.
    QString listItemElapsedNote() const;

    // Dispatches listItems[listItemIndex] (already incremented by the caller) through the normal
    // single-request pipeline — dispatchByMode()-equivalent, honoring listItemForceSplit/
    // listItemForceNoSplit if set, otherwise the same per-request heuristic beginRequest() itself
    // would use. No-ops (sets listModeActive=false) once listItemIndex runs past the end.
    void startNextListItem();

    // Prints the parsed item list once, up front, in the plain AI color (see
    // PythonSyntaxHighlighter's matching "▸▸▸ AI: list " check).
    void printParsedListItems(const QStringList &items, bool wasTransformed);

    // Tail end of beginListRequest() — builds listItems
    // (mechanical per-item doc-topic keyword matching, see topicSynonymRe) from `items` and
    // starts the list running via startNextListItem().
    void startListItems(const QStringList &items, bool forceSplit, bool forceNoSplit, bool forceReview,
                        bool forceNoReview, bool forceExplore, bool forceNoExplore, bool splitByDefault,
                        bool reviewByDefault, bool exploreByDefault, bool autoRunValue);

    // The list-mode counterpart to advanceSplitPart() — called once a list item's OWN processing
    // is FULLY done (whether that item ran split or non-split; for a split item this is after its
    // own merge completes, not after each internal part). Increments listItemIndex and calls
    // startNextListItem() for the next item, or ends the list if that was the last one.
    void advanceListItem();

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
    //! No reply awaited, no HTTP retry scheduled and no LIST running. Between two steps of one request (a script
    //! runs, the next call is scheduled) it can be idle for an instant: whoever waits should see it idle for a while.
    //! The retry timer counts: m_busy is already false during a retry's backoff (onReplyFinished() resets it).
    bool isIdle() const;

    // Corrections typed while a request runs ("big brother" mode): each later model call of that request carries them
    // (correctionBlock()), until the next request starts — in a LIST, until the next item starts. `now` (a leading "!"
    // in the typed text, see parseCorrection()) cancels the model call in flight and sends it again with the
    // correction; otherwise it applies from the next call. Returns true when a call was restarted. While a correction
    // is active the scope and deliverable checks are skipped: they read the original request, which the correction may
    // contradict ("don't plot"); the known-mistake checks stay.
    bool addUserCorrection(const QString &text, bool now);
    void clearUserCorrections()
    {
        m_userCorrections.clear();
    }
    const QStringList &userCorrections() const
    {
        return m_userCorrections;
    }
    // The text appended to a prompt for these corrections; empty for none.
    static QString correctionBlock(const QStringList &corrections);
    // The correction in typed text: a leading "!" is stripped and sets *now.
    static QString parseCorrection(const QString &text, bool *now);

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
    // HTTP transfer timeout of one call, every backend; 0 = automatic — see ask()'s setTransferTimeout() call
    void setRequestTimeoutSec(int n)
    {
        m_requestTimeoutSec = n;
    }
    int requestTimeoutSec() const
    {
        return m_requestTimeoutSec;
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
    // When true (default), a "# EXPLORE" response enters the two-phase loop: the read-only script
    // runs, its real output is fed back, and only then is the action script requested. When false,
    // buildSystemPrompt() stops asking for probe rounds and handleExploreResponse() refuses one
    // that arrives anyway (see its own doc comment) — never silently running it, and never
    // dead-ending the request either. Does not affect the existence-check/collision-check
    // round-trips, which are QtiSAS's own machinery rather than model-initiated exploration.
    void setExploreEnabled(bool on)
    {
        m_exploreEnabled = on;
    }
    bool exploreEnabled() const
    {
        return m_exploreEnabled;
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
    // Consecutive # EXPLORE rounds with no intervening action-script attempt — a NARROWER,
    // smaller sub-limit than autoContinueCount/maxAutoContinue() (the overall round budget for
    // the whole request), specifically targeting "stuck exploring, never synthesizing a fix."
    // Reset to 0 both at the same per-request/per-part points as autoContinueCount AND every time
    // an action script actually gets attempted (see runActionScript()'s "AI code (running)"
    // print) — so it measures ONLY an unbroken streak of pure diagnostics, not total rounds used.
    // See handleExploreResponse()'s doc comment for the confirmed-live incident this closes: with
    // the full request history intact (no truncation — ruled out directly from that transcript's
    // own headers) and every fact already printed earlier in the SAME conversation, six straight
    // EXPLORE rounds re-verified already-known state instead of ever comparing it against the
    // string that had actually failed, burning the entire remaining autoContinueCount budget on
    // zero net progress.
    int consecutiveExploreCount = 0;
    // The most recent EXPLORE round's captured console output (empty outside an unbroken EXPLORE
    // streak) — shares consecutiveExploreCount's exact reset lifecycle (same three reset points:
    // beginRequest(), startSplitPart(), and runActionScript() right where that counter resets),
    // so it's valid precisely when consecutiveExploreCount > 0 is true. Confirmed live (twice) this
    // was needed: EXPLORE printed the real Compiler.groups() list, and the very next round's write
    // action called setGroup() with a name matching none of them — with nothing keeping the
    // printed output around, there was no way to mechanically compare the two. See
    // maybeDispatchSelfReview()'s exploreGroupsIgnoredMistake() check.
    QString lastExploreOutput;
    // Previous EXPLORE round's raw script, same reset lifecycle as lastExploreOutput above. Only
    // to detect a byte-identical repeat: confirmed live, an EXPLORE returned the exact same script
    // 3 rounds running at 37-50s each, so an unproductive loop can burn the whole retry budget.
    QString lastExploreCode;
    // Bounds how many CONSECUTIVE error/block retries in a row get full docs re-attached (see
    // runActionScript()'s doc comment on why they're attached at all). Unlike
    // consecutiveExploreCount, this must NOT reset on every action-script attempt — only on a
    // genuine SUCCESS (result.error.isEmpty() in runActionScript()) or a fresh request/part — since
    // it specifically counts an unbroken CHAIN of corrections that each still needed grounding, not
    // total attempts; resetting on every attempt (success or failure alike) would make it always
    // read 0 right when the next retry's inclusion decision is made. Caps the residual
    // context-window risk documented in buildSystemPrompt(): resending full docs on every one of
    // up to maxAutoContinue() retries, compounding with growing history, was the exact failure mode
    // (seen live: "maximum context length") that motivated the original includeDocs=false-for-
    // retries design this session's fix partially reverted — this keeps the benefit (grounding for
    // the first couple of corrections, where the confirmed hallucinations actually happened)
    // without letting a long chain compound indefinitely.
    int consecutiveDocRetryCount = 0;
    // Objects ("kind:name", see AiScriptHost::liveObjects()) that this request's failed scripts created and left
    // behind — a retry runs the whole script again and must reuse them instead of creating or extending them twice.
    // Confirmed live: each retry of a note task ran addTab() again (2, 3, then 4 tabs). Cleared on success and for
    // every new request.
    QStringList m_failedRunObjects;
    // The retry prompts' sentence naming m_failedRunObjects; empty when there are none.
    QString failedRunObjectsNote() const;
    // Clears every "a network reply for THIS is still coming" flag at once. Must be called by any
    // path that abandons or supersedes an in-flight request, because the flags are what route an
    // arriving reply to its handler — a stale one sends the NEXT reply into a handler whose state
    // has already been torn down. Confirmed live (crash, SIGSEGV): the cancel path in
    // ApplicationWindow::handleAiTextCommand() cleared splitPlan and four of these flags but not
    // awaitingSplitPlanCreationCheckScript, so a creation-check reply arriving after a cancel ran
    // handleSplitPlanCreationCheckScriptResponse() against an emptied plan, reached
    // finalizeSplitPlanAndLaunch() -> startSplitPart(0), and read splitPlan[0] out of bounds; the
    // garbage SplitPart's docTopics then faulted inside QList's growAppend(). beginRequest() had
    // the same hole. Enumerating a subset by hand is what caused this, hence one shared reset.
    //
    // Deliberately EXCLUDES awaitingContinuationClarification: that one waits for the real user's
    // own typed reply, not an HTTP response, so it is not this function's business — a caller that
    // means to drop a pending question to the user must clear it explicitly.
    void resetPendingRoundTrips();

    bool awaitingSelfReview = false;
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
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit dropped one or more required Compiler setup calls
    // (setFunctionName/setGroup/setParamCount/setParamNamesAll/setParamInitsAll/setParamLimitsAll/
    // setParamVariesAll) while still calling compile() — see maybeDispatchSelfReview()'s use of
    // AiContentProvider::selfReviewCompileSequenceRegression(). Confirmed live: a correct, complete
    // compile setup was replaced wholesale with old-style raw-array C-function code missing
    // setGroup()/parameter declarations entirely, raising ValueError: "compile: group is empty" on
    // the very next run. Same one-shot, fail-open philosophy as awaitingSelfReviewSkepticismRetry
    // above.
    bool awaitingSelfReviewCompileSequenceRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit changed an already-correct PyQt import's major version (PyQt5<->PyQt6)
    // for the same Qt class — see maybeDispatchSelfReview()'s use of
    // AiContentProvider::selfReviewPyQtVersionSubstitution(). Confirmed live: a correct
    // `from PyQt6.QtGui import QColor, QFont` draft (this build's real binding) was silently
    // rewritten to `from PyQt5.QtGui import QColor, QFont` while self-review fixed an unrelated
    // issue elsewhere in the same script — PyQt5 doesn't exist in this environment, and the script
    // reached actual execution and crashed with ModuleNotFoundError a full round-trip later. Same
    // one-shot, fail-open philosophy as awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewPyQtVersionRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit added a fabricated Compiler.setCode() call for a function the draft
    // correctly reused (open()+compile(), no setCode() of its own) — see
    // maybeDispatchSelfReview()'s use of AiContentProvider::selfReviewFabricatedSetCodeMistake().
    // Confirmed live, severe: EXPLORE had already printed the EXISTING, correct code for two
    // functions; the draft correctly did nothing but reopen and recompile them; self-review
    // rewrote this into a version with a brand-new, fabricated setCode() call for each — neither
    // function actually compiled. Same one-shot, fail-open philosophy as
    // awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewFabricatedSetCodeRetry = false;
    // One-shot: true only for the self-review pass answering the corrective retry fired when
    // self-review's OWN edit changed the name passed to a creation call (newTable/newMatrix/
    // newGraph/newNote) that the pre-review draft already had correct — see
    // maybeDispatchSelfReview()'s use of AiContentProvider::selfReviewCreationNameSubstitution().
    // Confirmed live: a request naming the table "SAXS_data" survived correctly into the pre-review
    // draft; self-review's rewrite silently dropped the underscore ("SAXSdata") in both the
    // newTable() call and an unrelated scriptPrint() confirmation string in the same script. Same
    // one-shot, fail-open philosophy as awaitingSelfReviewSkepticismRetry above.
    bool awaitingSelfReviewCreationNameRetry = false;
    // One-shot, but DIFFERENT IN KIND from the nine flags above (and gated the opposite way in
    // maybeDispatchSelfReview() — on the ORIGINAL candidate, not on self-review's own output): true
    // only for the self-review pass answering the corrective retry fired when a candidate's
    // setGroup(...) argument matches none of the groups an immediately-preceding EXPLORE round
    // actually printed — see maybeDispatchSelfReview()'s use of
    // AiContentProvider::exploreGroupsIgnoredMistake(). Confirmed live, twice: EXPLORE printed the
    // real Compiler.groups() list, and the next round's write action called
    // setGroup("custom:sans") anyway, matching none of them. Not a hard error (setGroup() allows
    // any new group), so — unlike the "revert this" framing of the flags above — the retry this
    // fires only asks the model to confirm or fix its choice once; it never re-fires on the
    // corrected reply even if a new group is still what comes back.
    bool awaitingSelfReviewExploreGroupRetry = false;

    // Split-mode: true for a request that opted in via the client-side "SPLIT:" marker (see
    // ApplicationWindow::askAi()). While true, a successful action-script run in
    // runActionScript() does not simply return — it calls advanceSplitPart() instead of ending
    // the exchange. Reset (together with the split fields below) on every new top-level askAi()
    // call, same lifetime as autoContinueCount et al.
    bool splitModeActive = false;
    // True only while beginRequest()'s "existing X" verification script is in flight — the
    // EARLIEST gate a request passes through, before even awaitingContinuationCheck/
    // awaitingSplitPlan. handleExistenceCheckScriptResponse() resets this.
    bool awaitingExistenceCheckScript = false;
    // The request text beginRequest() is gating behind the existence-check script — resumed with
    // (via dispatchAfterExistenceCheck()) once it reports every referenced name found, or if the
    // check itself failed open. Not consumed/cleared by the check itself.
    QString pendingExistenceCheckRequestText;
    // Set in beginRequest() when the existence-check round-trip is skipped (generate-only mode) and
    // appended to the outgoing request by startRequest(). Appended THERE rather than folded into
    // pendingRequestText because that string is also what gets stored as this turn's history text —
    // see startRequest()'s textForHistory comment for the two confirmed-live leaks that caused.
    QString pendingLiveFactsNote;
    QStringList m_existingInputTables; // see existingInputNamesNote()
    // Set by maybeDispatchSelfReview() when it removed trailing output lines; the flag allows one retry per request
    // for an output line written as text instead of printed (see unprintedOutputLineMistake()).
    QString m_removedTrailingOutput;
    // The description of a plan that settled on one part (startRequest()'s resolvedTaskDescription): it can name what
    // the request left unnamed ("Surface"), so the scope checks read it along with the request. Empty otherwise.
    QString m_singlePartDescription;
    // The compiled script sent to self-review: kept if the review answers with no runnable script at all.
    QString m_selfReviewDraftCode;
    bool m_unprintedOutputRetried = false;
    // True only while a finalized split plan's creation-name collision-check script (see
    // handleSplitPlanResponse()'s doc comment) is in flight — checked whether the plan's own
    // to-be-created names (splitPlanCreationNames()) already exist, so a silent recompile-over-an-
    // existing-function (or table/graph/note/matrix) never happens unnoticed. Resumes into
    // finalizeSplitPlanAndLaunch() once resolved. handleSplitPlanCreationCheckScriptResponse()
    // resets this.
    bool awaitingSplitPlanCreationCheckScript = false;
    // True only while beginSplitPlanDecomposition()'s continuation-check pre-flight (see
    // AiContentProvider::continuationCheckPreflightPrompt()) is in flight — mirrors
    // awaitingSplitPlan. Exactly one of this and awaitingSplitPlan is live at
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
    // The request text dispatchAfterExistenceCheck() is gating behind the continuation-check (or
    // the user's clarification) — resumed with once either resolves, via dispatchByMode(). Not
    // split-mode-specific (despite the similar name) — this gate now applies to every request,
    // split or not. Not consumed/cleared by the check itself, only by whichever branch finally
    // dispatches.
    QString pendingDispatchRequestText;
    // True only while the decomposition pre-flight call is in flight. handleSplitPlanResponse()
    // resets it.
    bool awaitingSplitPlan = false;
    // Combined, capped retry counter for handleSplitPlanResponse()'s three decomposition-fixup
    // checks (naming completeness, action coverage, unnecessary fragmentation). These used to be
    // three fully independent one-shot flags (awaitingSplitPlanNamingRetry/CoverageRetry/
    // FragmentRetry), each individually documented as "never retries a second time... so a plan
    // that still fails proceeds anyway rather than retrying forever" — but that guarantee only
    // held per individual check, not for the combined process. Confirmed live, severe: a plan
    // ping-ponged between "missing explicit name(s)" and "unnecessary fragmentation" THREE FULL
    // CYCLES (6 wasted round-trips) before converging — each retry prompt showed the model only
    // ONE class of complaint, so fixing it had no visibility into the OTHER constraint it was
    // simultaneously breaking, and nothing bounded the TOTAL round-trips across the three
    // independently-one-shot gates. handleSplitPlanResponse() now checks all three together every
    // round and folds every currently-detected complaint into ONE retry prompt, bounded by this
    // shared counter so the whole process is guaranteed to terminate (fail-open, accept the plan
    // as-is) even if the model still can't satisfy everything. Reset to 0 in beginRequest() for
    // every new top-level request.
    int splitPlanFixupRoundsUsed = 0;
    // The exact complaint list handed to the model on the most recent fixup retry — compared
    // against the NEXT round's freshly-computed complaints in handleSplitPlanResponse(). Confirmed
    // live, twice, independently (a matrix create+formula-fill plan, and an unrelated table
    // create+synthetic-data-fill plan): the model resent a plan with the identical unresolved
    // fragmentation complaint all 3 rounds straight through, so every round after the first was a
    // guaranteed-futile round-trip (one measured at 28s) before falling through to "proceeding
    // anyway" regardless. When this round's complaints exactly match this list, the process has
    // provably stopped converging — stop retrying immediately instead of spending the rest of
    // kMaxSplitPlanFixupRounds on rounds that cannot possibly do anything different. Reset
    // (cleared) in beginRequest() alongside splitPlanFixupRoundsUsed.
    QStringList lastSplitPlanFixupComplaints;
    // Bounds the SEPARATE, later-stage retry that fires when splitPlanCreationCollisions() flags a
    // name the fixup process itself invented (never mentioned in originalRequestText) — the model
    // is asked to invent a different placeholder instead of halting to ask the user for a name they
    // never chose in the first place (see buildSplitPlanCreationCollisionRenamePrompt()'s doc
    // comment). Deliberately independent of splitPlanFixupRoundsUsed: this check only ever runs
    // once the earlier fixup rounds already converged, so it needs its own small budget rather than
    // competing with (or being blocked by) that counter's own cap. Reset to 0 in beginRequest().
    int splitPlanCreationNameRetryRoundsUsed = 0;
    // Names confirmed Rule-6 sandbox-exempt (either pre-emptively skipped by
    // splitPlanCreationNamesNeedingCheck() or filtered out post-response by
    // splitPlanCollisionsAfterSandboxExemption()) — confirmed live: the model still re-derives
    // "does this already exist?" itself inside its own EXPLORE step for such a name, wasting a
    // round on a question the system already settled. startSplitPart() tells the part outright
    // instead. Reset (cleared) in beginRequest().
    QStringList splitPlanSandboxExemptNames;
    // Mirrors awaitingSplitPlan — true only while the final, NEVER-EXECUTED
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
                               // same way startRequestWithSelectedTopics() prepends them for a
                               // whole request
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

    // LIST mode: a purely mechanical alternative to split-mode decomposition — the user writes
    // the steps themselves as a numbered list (see ApplicationWindow::parseListRequest()), so
    // each item is parsed directly instead of asking the model to invent a decomposition, then
    // run one at a time as its own full, independent request via startRequest(). See
    // beginListRequest()/startNextListItem()/advanceListItem().
    bool listModeActive = false;
    struct ListItem
    {
        QString text;          // this item's own request text, run via startRequest() as-is
        QStringList docTopics; // mechanical keyword match against knownDocTopics() — see
                               // beginListRequest()'s doc comment
    };
    QList<ListItem> listItems;
    int listItemIndex = -1;
    // Started when an item's own request begins (startNextListItem()); read by
    // listItemElapsedNote() right before the NEXT item starts, to report how long the one that
    // just finished actually took.
    QElapsedTimer m_listItemTimer;
    // Started once when the whole LIST begins, never restarted per item — the per-item timer above
    // only says how long the last item took, which gives no sense of total cost on a long list.
    QElapsedTimer m_listTotalTimer;
    // From "SPLIT"/"NOSPLIT"/"REVIEW"/"NOREVIEW" found in the LIST request — forces every item's
    // own split-vs-single and review-vs-not choice, overriding the normal per-request heuristic.
    // Both false in a pair (bare "LIST") leaves that decision to the same heuristic any standalone
    // request already uses.
    bool listItemForceSplit = false;
    bool listItemForceNoSplit = false;
    bool listItemForceReview = false;
    bool listItemForceNoReview = false;
    bool listItemForceExplore = false;
    bool listItemForceNoExplore = false;
    // The host's persisted defaults, passed in once by beginListRequest() (same values
    // ApplicationWindow::askAi() would otherwise resolve itself per request) — used by
    // startNextListItem() for a bare "LIST" whose items get neither force flag above.
    bool listSplitByDefault = false;
    bool listReviewByDefault = false;
    bool listExploreByDefault = true;
    // Mirrors m_contextHistoryStart's exact mechanism (see its own doc comment), scoped to the
    // currently-running LIST call instead of the continuation-check's "new task" boundary —
    // windowedHistoryStart() takes the max of the two. Never trims m_history itself, only what
    // gets SENT for a request while this LIST is in progress; the full transcript (every item,
    // and anything before the LIST started) stays intact for "show history" regardless.
    qsizetype m_listHistoryStart = 0;

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

    // The shared resumption point for beginRequest() once the existence-check gate (if any fired)
    // has resolved. Runs the new-task-vs-continuation check (see awaitingContinuationCheck's doc
    // comment) regardless of splitModeActive — deliberately, since this gate and the
    // originalRequestText refresh it drives matter equally for a plain (non-split) follow-up, not
    // just a split one (confirmed live: see this method's .cpp doc comment for the incident that
    // exposed the gap when it was split-mode-only). On a fresh conversation (m_history empty,
    // nothing to conflict with) or no content provider, dispatches immediately via
    // dispatchByMode()/startRequest(); otherwise defers to handleContinuationCheckResponse().
    void dispatchAfterExistenceCheck(const QString &aiText);

    // Dispatches `aiText` into split-mode decomposition or the plain doc-selection preflight,
    // whichever splitModeActive calls for — called once the continuation-check gate (if any) has
    // already resolved, by dispatchAfterExistenceCheck() directly (fresh conversation) or by
    // handleContinuationCheckResponse() (history existed).
    void dispatchByMode(const QString &aiText);

    // The plain (non-split) half of dispatchByMode() — lifted out of ApplicationWindow::askAi()
    // (which used to call this inline) so beginRequest()'s existence-check gate can resume into it
    // the same way it resumes into beginSplitPlanDecomposition(). Selects the doc topics
    // mechanically from the request's keywords (docTopicsFromRequestText()) — no model call.
    void beginNonSplitRequest(const QString &aiText);

    // Shared by runActionScript()'s three "give up" branches and handleExploreResponse()'s
    // round-cap message: when splitModeActive, returns a note naming which part stalled and
    // that later parts/merge were never attempted, AND clears splitModeActive (so a later,
    // unrelated request doesn't inherit stale split state) — both as a side effect of one call.
    // Returns an empty string (no side effect) when not in split mode.
    QString splitModeGiveUpNote();

    // Called by every "give up" branch (runActionScript()'s three, handleExploreResponse()'s
    // round-cap) right before it returns. Appends a synthetic (user, assistant) pair to m_history
    // recording that the task did NOT complete — m_history otherwise contains only the model's own
    // generated code turns, never execution outcomes, so a give-up leaves no trace at all once the
    // final failing ask() is never sent. Confirmed live, severe: with nothing recorded, a later
    // "is it running?" status question in the SAME conversation had no signal to contradict the
    // shape of the original request, and fabricated a complete, specific, false success narrative
    // instead of reporting the actual unresolved error.
    void recordGiveUpInHistory(const QString &reason, const QString &lastError);

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
    // Doc topics matched mechanically from a request's own text — replaces the doc-selection LLM
    // pre-flight on the non-split path. See the .cpp definition for the under-selection incident
    // and for why the model's answer had become unable to change the result.
    QStringList docTopicsFromRequestText(const QString &text) const;

    // Reports the chosen topics under `label`, prepends the content provider's baseline topics,
    // and dispatches. Shared by beginNonSplitRequest() and the failed-split-plan fallback.
    void startRequestWithSelectedTopics(const QStringList &selected, const QString &label);

    void resetErrorRetryState();


    // Appends one line per HTTP response to a persistent, cross-session log — timestamp, latency,
    // request size, HTTP status, and any provider-reported rate-limit headers found (name contains
    // "ratelimit", case-insensitive; varies by provider, so collected generically rather than by a
    // hardcoded per-provider header list). Purpose: let the user correlate response latency against
    // their own account's reported remaining capacity over time — plain appended text, not surfaced
    // in the UI, since a live console line per request would add noise without helping that goal.
    static QString requestTimingLogPath();
    // One JSON line per correction (with the request, step, last script and last error at that moment) and one per
    // outcome of the next script (ran / failed / blocked): the material for turning corrections into checks and docs.
    static QString correctionLogPath();
    void logRequestTiming(QNetworkReply *reply) const;

    // Extracted pieces of ask() — see that method's own body for how they're assembled. Kept as
    // private members (not free functions) since each touches instance state (m_cachedDocs,
    // m_history, m_lastCallMs, ...), not just its own parameters.
    QString buildSystemPrompt(const QString &systemPrompt, bool includeDocs, const QStringList &docTopics,
                              bool expectsCode = true, const QString &userText = QString());
    qsizetype windowedHistoryStart(bool includeHistory) const;
    // Turns before this index belong to a task/list the conversation has already moved past, and
    // are excluded independently of the char budget — shared by windowedHistoryStart() and the
    // diagnostic so the two can attribute an exclusion to the right cause.
    qsizetype historyLowerBound() const;
    QString buildSendingDiagnostic(qint64 payloadSize, qsizetype historyStart, qsizetype contextBoundary,
                                   const QStringList &docTopics, bool includeHistory) const;

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
    bool m_exploreEnabled = true;
    int m_requestTimeoutSec = 0;
    QList<QPair<QString, QString>> m_history; // (user, assistant) pairs — the FULL transcript,
    // permanent; only an explicit user "clear history" command (ApplicationWindow.cpp) ever
    // shrinks it. historyText() ("show history") always reads all of it from index 0.
    // Index into m_history marking where the CURRENT task's context begins — turns before this
    // point still exist (in full, for historyText()) but are excluded from what windowedHistoryStart()
    // slices into a call's context, same idea as its own token-budget window, just a second,
    // independent reason to exclude old turns rather than a second history list. Set by
    // handleContinuationCheckResponse() on a NEW verdict — deliberately NOT by clearHistory()
    // itself, so an explicit user wipe and an automatic "that's a different task" boundary stay
    // distinct: the former also erases the transcript, the latter never does.
    qsizetype m_contextHistoryStart = 0;
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
    // See addUserCorrection(). m_lastAsk holds ask()'s arguments so that a "!" correction can send the same call again
    // (rebuilt, with the correction); m_redoWithCorrection makes onReplyFinished() do that for the aborted reply.
    QStringList m_userCorrections;
    struct AskArgs
    {
        QString userText;
        bool includeDocs = true;
        QString systemPrompt;
        QStringList docTopics;
        bool addToHistory = true;
        bool includeHistory = true;
        bool expectsCode = true;
        QString historyText;
    } m_lastAsk;
    bool m_redoWithCorrection = false;
    void redoLastAsk();
    bool m_correctionOutcomePending = false; // a correction was logged, the next script's outcome is not yet
    void logCorrection(const QString &text, bool now) const;
    void logCorrectionOutcome(const QString &result, const QString &detail);
    int m_httpRetryCount = 0;
    QTimer *m_retryTimer;
};

#endif
