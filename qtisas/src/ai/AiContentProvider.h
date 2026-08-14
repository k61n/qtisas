/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Abstract interface supplying the AI orchestration logic (AiAssistant)
             with domain-specific KNOWLEDGE — the self-review checklist, doc-topic
             descriptions, and API documentation content — as opposed to
             AiScriptHost, which supplies script EXECUTION. Depends only on
             QtCore, matching AiScriptHost's own independence, so AiAssistant.cpp
             can stay free of QtiSAS-specific string literals: everything a
             future non-QtiSAS host would need to supply differently lives
             behind this interface instead of hardcoded inside AiAssistant.
 ******************************************************************************/

#ifndef AICONTENTPROVIDER_H
#define AICONTENTPROVIDER_H

#include <QString>
#include <QStringList>

// Wire format shared between AiContentProvider::loadDocs() implementations (which emit it) and
// AiAssistant.cpp's filterDocsByTopic() (which parses it back out) — a private contract between
// those two independently-compiled call sites, previously duplicated as an identical literal in
// each .cpp with no shared symbol tying them together. A section is written as
// `kDocSectionMarkerPrefix + filename + kDocSectionMarkerSuffix` immediately followed by that
// file's content.
inline const QString kDocSectionMarkerPrefix = QStringLiteral("\n\n=== ");
inline const QString kDocSectionMarkerSuffix = QStringLiteral(" ===\n");

// Result of AiContentProvider::detectKnownMistakes() — mirrors AiScriptResult's plain-struct style
// (see AiScriptHost.h). `header` is empty when no known pattern matched; otherwise it is a short,
// ready-to-print phrase identifying WHICH pattern fired (e.g. "nonexistent Qti3D import detected"),
// and `explanation` is the human-readable text to feed into the retry prompt.
struct AiKnownMistakeMatch
{
    QString header;
    QString explanation;
};

class AiContentProvider
{
  public:
    virtual ~AiContentProvider() = default;

    // The mandatory pre-execution self-review checklist — curated, silent-failure-prone gotchas
    // specific to this host's own scripting API. AiAssistant appends the user-editable rules list
    // (a generic mechanism, not host-specific) after this, so this method returns ONLY the
    // curated/hardcoded portion.
    virtual QString selfReviewChecklist() const = 0;

    // Topic name -> description text shown to the model during doc-topic selection and split-mode
    // decomposition (e.g. "table — Table/column creation and editing...").
    virtual QString docTopicIndex() const = 0;

    // The fixed set of topic names docTopicIndex() describes and that a raw "DOCS: ..." answer
    // gets substring-matched against (see AiAssistant's doc-selection and split-mode parsing).
    virtual QStringList knownDocTopics() const = 0;

    // Topics prepended to whatever the model actually selected — this host's own baseline
    // reference (e.g. assistant workflow rules, and the handful of functions used in nearly every
    // script) that's relevant regardless of what the request is otherwise about. Host-specific
    // because these are specific doc-file topic names, not a generic notion AiAssistant.cpp should
    // know about; kept separate from knownDocTopics() since these are never something the MODEL
    // selects itself, only something always added alongside its selection.
    virtual QStringList baselineDocTopics() const = 0;

    // Loads this host's full API documentation content, ready for AiAssistant's own topic
    // filtering (see filterDocsByTopic() in AiAssistant.cpp — that splitting logic is generic and
    // stays there; only the raw loading is host-specific). `unlimited`: true for the large/.md
    // docs, false for the compact/.pyi stubs. `statusOut`, if non-null, is set to a human-readable
    // status line describing what was loaded (or why nothing was) for display via
    // AiScriptHost::print().
    virtual QString loadDocs(bool unlimited, QString *statusOut) const = 0;

    // The one-shot pre-flight prompt asking the model whether/how to break `requestText` into
    // split-mode parts. Host-specific because its guidance (naming conventions, EXPLORE's
    // fresh-namespace rule, table()/graphWindow()/compiled-function lookup-by-name, ...) only
    // makes sense in terms of this host's own scripting API.
    virtual QString splitPlanPreflightPrompt(const QString &requestText) const = 0;

    // The one-shot pre-flight prompt asking the model which doc topics `requestText` needs.
    // Host-specific only insofar as its topic list/index comes from this same provider — the
    // wording itself is closer to generic mechanism, but kept alongside splitPlanPreflightPrompt()
    // for the same reason: ApplicationWindow.cpp (or any other host) should hold no QString
    // prompt literals of its own, only orchestration/wiring.
    virtual QString docSelectionPreflightPrompt(const QString &requestText) const = 0;

    // The one-shot pre-flight prompt asking whether `newRequestText` continues the task described
    // by `priorRequestText`, is a new independent task, or is genuinely unclear — asked before
    // split-mode decomposition begins, only when there's existing conversation history/an existing
    // original request that could conflict. Self-contained (restates both texts explicitly, needs
    // no external history) for the same reason splitPlanPreflightPrompt()/
    // docSelectionPreflightPrompt() are. Host-specific only in the sense every prompt in this
    // family lives behind this interface so no host holds its own QString literals; the judgment
    // itself is generic. Expected answer: exactly one word, "CONTINUE", "NEW", or "UNCLEAR".
    // Confirmed live: without this, an action-generation/error-retry call (which legitimately
    // needs SOME history — its own immediately-preceding attempt) could not tell "this task's own
    // prior turn" apart from "an earlier, unrelated request's turn" in the same flat history, and
    // latched onto the unrelated one instead.
    virtual QString continuationCheckPreflightPrompt(const QString &newRequestText,
                                                     const QString &priorRequestText) const = 0;

    // Suffix appended to every user request before sending it, steering the model's response
    // shape (raw runnable code vs. plain-text discussion). Host-specific because the autoRun
    // variant names this host's own script-output function (e.g. scriptPrint()) as the fallback
    // for a message that turns out to be a question rather than a coding task.
    virtual QString requestSuffix(bool autoRun) const = 0;

    // A short reminder of this host's own calling convention for its exposed scripting API — e.g.
    // "these functions are global, call them directly with no prefix/namespace/module qualifier"
    // — spliced into the generic base system prompt sent with every request. Host-specific because
    // it names concrete functions from this host's own API surface; kept behind this interface so
    // AiAssistant.cpp's own prompt scaffolding never has to name a single QtiSAS function.
    virtual QString apiCallingConventionHint() const = 0;

    // One short sentence introducing the assistant and this host application by name/domain (e.g.
    // "You are an assistant for QtiSAS, a scientific data analysis software. ") — opens the base
    // system prompt sent with every request. Host-specific because it names this host application
    // itself; kept behind this interface (rather than a literal in AiAssistant.cpp) for the exact
    // reason apiCallingConventionHint() is — a different host embedding this class must not inherit
    // QtiSAS's own name/identity.
    virtual QString assistantIdentityIntro() const = 0;

    // A short, comma-separated list of this host's own existence-verification function examples
    // (e.g. "existTable(), graphWindow(), fitFunctions(), colNames(), datasets()") — spliced into
    // prompts that need the model to back a "this doesn't exist"/"nothing found" conclusion with a
    // REAL live check rather than asserting it from memory/history. Host-specific because these are
    // concrete function names from this host's own API surface.
    virtual QString existenceVerificationFunctionExamples() const = 0;

    // The name of this host's own "print a message and do nothing else" function (e.g.
    // "scriptPrint()") — spliced into prompts warning against an unverified negative conclusion
    // expressed as a bare print call with no real check backing it. Host-specific because it names
    // a concrete function from this host's own API surface. See isGiveUpFallbackScript()'s doc
    // comment for the same underlying concept, checked mechanically rather than named in a prompt.
    virtual QString noOpPrintFunctionName() const = 0;

    // A short, comma-separated list of this host's own side-effecting creation-call examples (e.g.
    // "plot(), newTable(), newGraph(), Compiler.compile()") — spliced into the error-retry prompt's
    // warning against blindly rewriting a whole script from scratch when an earlier line in the
    // SAME script already ran successfully and took real effect before the error. Host-specific
    // because these are concrete function names from this host's own API surface.
    virtual QString creationCallExamples() const = 0;

    // A short description of how to look up (rather than recreate) something an EARLIER split-mode
    // part, or an earlier line in the SAME script, already created live — by NAME, never by
    // assuming a Python variable/object survives between separate script runs (e.g. "table(name),
    // graphWindow(name), re-opening a compiled function by name"). Host-specific because these are
    // concrete lookup functions from this host's own API surface.
    virtual QString lookupByNameExamples() const = 0;

    // Checks a split-mode part's generated `code` against its own declared `partDescription`,
    // in either direction: performing a DIFFERENT part's job (e.g. building a plot when this
    // part's job is something else), or NOT performing this part's own explicitly-described job
    // (e.g. describing a fit but never actually fitting). Host-specific because recognizing "this
    // looks like a plot" / "this looks like a fit" requires knowing this host's own API surface
    // (which calls create a plot, which perform a fit) — AiAssistant.cpp must stay free of those
    // specifics. `earlierPartsCode` is the concatenated, actually-executed code of every earlier,
    // already-completed part in this same split-mode plan — needed to cross-check a fit/configure
    // call in THIS part's code against a function an EARLIER part already compiled (see Direction
    // 6 in QtiSasAiContent's implementation). Returns a human-readable explanation of the violation
    // to feed back to the model for a corrected retry, or an empty string if no violation is
    // detected.
    virtual QString splitPartScopeViolation(const QString &code, const QString &partDescription,
                                            const QString &earlierPartsCode) const = 0;

    // True if `script` is just a single call to this host's own "print a message and do nothing
    // else" function (e.g. scriptPrint(...)) — the shape a part takes when it gives up on its
    // real job and reports that instead. Host-specific because it needs to recognize this host's
    // own print-function name. Used by AiAssistant to verify the split-mode merge step didn't
    // silently substitute a different, more complete-looking script for a part that actually just
    // gave up — confirmed live: the merge step's own prompt already explicitly forbids exactly
    // this ("never substitute an earlier draft... even if it looks more complete or better
    // styled") and still did it.
    virtual bool isGiveUpFallbackScript(const QString &script) const = 0;

    // A short, host-specific grounding reminder to prepend to an error-retry prompt when
    // `errContext` matches a KNOWN, recurring failure signature (e.g. this host's own singleton
    // objects being called as constructors) — or an empty string if it matches nothing known.
    // Error-retry prompts don't resend API docs (see AiAssistant::runActionScript()'s doc comment
    // on why), so once a retry drifts off the correct syntax it has nothing but its own prior,
    // possibly-also-wrong guesses in history to build on. Confirmed live, severe: a single
    // Compiler-singleton mistake escalated across 8 retries into wildly different wrong guesses
    // (Dan.Compiler(), Fittable(), a fabricated newFittable(), ...) before the part gave up
    // entirely — this exists to re-ground exactly that recurring failure shape cheaply, without
    // resending the full doc corpus on every retry.
    virtual QString retryHintForError(const QString &errContext) const = 0;

    // True if `script` is a give-up (see isGiveUpFallbackScript()) whose message asserts something
    // is missing/not found/doesn't exist, AND the script contains no call to any of this host's own
    // live existence-verification functions (existTable(), graphWindow(), fitFunctions(),
    // colNames(), datasets(), ...) anywhere in it — i.e. the claim is asserted from text judgment
    // alone, never actually checked in this exact script. Used by AiAssistant to catch self-review
    // fabricating exactly this shape of claim when it turns a normal, working candidate into a
    // give-up — confirmed live, severe, DESPITE this exact failure already being explicitly,
    // repeatedly warned against in both the base system prompt and this host's own checklist:
    // prompt wording alone does not reliably prevent it, so this exists as a mechanical backstop.
    virtual bool isUnverifiedMissingClaim(const QString &script) const = 0;

    // Runs every known, host-specific, STATELESS pre-execution mistake pattern against `code` (and,
    // for the two patterns that need it, `originalRequestText`) in a fixed priority order, stopping
    // at the first match — one dispatch point replacing what used to be 16 separate single-purpose
    // pure virtual methods here (compiled-function self-shadow bugs, fitCurve name-reconstruction
    // mismatches, 0-based Table/Matrix indexing, hallucinated Legend/GraphWindow methods,
    // nonexistent existMatrix()/Qti3D-import forms, guessed candidate-name lists, ...). Each
    // individual pattern is still implemented as its own private helper in QtiSasAiContent.cpp, with
    // its own "confirmed live" incident writeup right next to its regex — only the PUBLIC interface
    // (and AiAssistant.cpp's call site) is consolidated. Deliberately excludes the self-review
    // BEFORE/AFTER regression checks below (selfReviewFunctionNameSubstitution and siblings) — those
    // compare a preReviewDraft against postReviewCode, a fundamentally different shape neither
    // reducible to nor combinable with a single-code-snapshot check. Host-specific because every
    // individual pattern depends on this host's own scripting API. Returns an AiKnownMistakeMatch
    // with an empty header/explanation if nothing matched.
    virtual AiKnownMistakeMatch detectKnownMistakes(const QString &code, const QString &originalRequestText) const = 0;

    // Checks whether self-review's own output changed the model-name argument to configure()/
    // setFunction() compared to `preReviewDraft` (the script it was actually asked to review).
    // Self-review runs with includeHistory=false, so it never sees an earlier EXPLORE round's
    // printed fitFunctions() candidates in the SAME conversation — unlike checklist items it IS
    // grounded to judge, it has no legitimate basis to decide which function name is correct, so
    // ANY change here is more likely a hallucinated substitution than a real fix. Host-specific
    // because it depends on this host's own configure()/setFunction() calling convention. Returns
    // a human-readable explanation of the substitution, or an empty string if the name is
    // unchanged (or neither script calls configure()/setFunction() at all).
    virtual QString selfReviewFunctionNameSubstitution(const QString &preReviewDraft,
                                                       const QString &postReviewCode) const = 0;

    // Checks whether self-review's own output is a give-up (see isGiveUpFallbackScript()) claiming
    // this part's job is "incomplete"/"not performed"/doesn't fulfill the request, while
    // `preReviewDraft` demonstrably contains a real creation/action call (newTable, newMatrix,
    // newGraph, newNote, Compiler.compile(), f.fit(), plot3D/newPlot3D) — strong evidence real
    // work was actually done, making a blanket "this doesn't do anything" dismissal very likely
    // wrong. Confirmed live, severe: a part correctly created and filled a table matching its own
    // description; self-review's response — self-contradictorily saying both "this script creates
    // a table" AND "no table creation occurs in this script's current form" in the same message —
    // discarded it, judging it against a LATER part's job (plotting) instead of its own, directly
    // contradicting self-review's own "judge it SOLELY against that one action" instruction. That
    // false give-up ran clean (no exception) and was accepted as a completed part, so the table
    // was never actually created — the next part's own EXPLORE then correctly found it missing,
    // but by then the real cause (self-review's own mistake) was already several turns back.
    // Host-specific because it depends on this host's own creation-call vocabulary. Returns a
    // human-readable explanation, or an empty string if the pre-review draft has no such evidence.
    virtual QString selfReviewFalseIncompletenessClaim(const QString &preReviewDraft,
                                                       const QString &postReviewCode) const = 0;

    // Builds the "existing X" verification preflight prompt: asks the model to write ONE short,
    // read-only Python script that checks each name in `names` against every plausible kind
    // (table, matrix, note, graph, compiled/Fittable function) and prints exactly "EXISTS: <name>"
    // or "MISSING: <name>" for each — no discovery/fuzzy search, the EXACT name only. Host-specific
    // because it depends on this host's own existTable()/existMatrix()/note()/graphWindow()/
    // Compiler.functionNames()/Fittable.fitFunctions() vocabulary. See
    // AiAssistant::extractExistingEntityNames() for how `names` is derived from the request text,
    // and AiAssistant::beginRequest()'s doc comment for the overall rationale.
    virtual QString existingEntityVerificationPrompt(const QStringList &names) const = 0;

    // Checks whether a pre-review draft correctly looked up an EXISTING object via
    // matrix(name)/table(name)/note(name)/graphWindow(name), but self-review's own output replaced
    // that lookup with a from-scratch newMatrix(name, ...)/newTable(name, ...)/newNote(name, ...)/
    // newGraph(name, ...) recreation for the SAME name — self-review has no docs or history to
    // judge whether recreating is correct, or what constructor signature to use. Confirmed live:
    // a correct `m = matrix("SphereMap")` (grounded in real cross-turn history the draft-
    // generation call could see but self-review cannot) was rewritten into
    // `m = newMatrix("SphereMap", -3.14159, 3.14159, 50, ...)` — an invented, nonexistent
    // constructor signature — raising TypeError on the very next run. Host-specific because it
    // depends on this host's own lookup/creation function-name pairs. Returns a human-readable
    // explanation, or an empty string if no such regression is found.
    virtual QString selfReviewLookupToCreationRegression(const QString &preReviewDraft,
                                                         const QString &postReviewCode) const = 0;

    // Checks whether a pre-review draft correctly passed an enum-namespace constant (Qti3D.*,
    // Graph.CurveType.*, Table.PlotDesignation.*, Graph3D.PlotType.*,
    // Graph3D.AxisNumericFormat.*) as an argument to some call, but self-review's own output
    // still makes that SAME call while no longer passing a value from that namespace — self-
    // review has no basis to replace an already-correct constant reference. Confirmed live:
    // self-review replaced setPlotStyle(Qti3D.FILLEDMESH) with setPlotStyle("Filled Mesh") (a
    // TypeError), and the runtime-error retry that followed "fixed" the crash by guessing a
    // bare int (2, i.e. HIDDENLINE) instead of recalling the correct constant (FILLEDMESH == 4)
    // — the script then ran with no further error but silently rendered the WRONG style,
    // contradicting the request's explicit "filled mesh style". Host-specific because it
    // depends on this host's own enum-namespace names. Returns a human-readable explanation, or
    // an empty string if no such regression is found.
    virtual QString selfReviewEnumConstantRegression(const QString &preReviewDraft,
                                                     const QString &postReviewCode) const = 0;
};

#endif
