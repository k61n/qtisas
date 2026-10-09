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

#include <QList>
#include <QPair>
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
    // curated/hardcoded portion. docTopics/splitModeActive scope it to this call's own selected
    // topics (same list already used to filter attached doc content) plus split-mode-specific
    // rules only when actually in split mode — implementations should skip topic-specific
    // sections whose topic isn't present, but always include host-wide/general rules regardless.
    virtual QString selfReviewChecklist(const QStringList &docTopics, bool splitModeActive) const = 0;

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
    //
    // `docsIntegrated`: true when AiAssistant's DocsMode::Integrated is active — no per-topic doc
    // content will ever be attached to any part regardless of what DOCS list it's given. Passed
    // through as a plain bool rather than AiAssistant::DocsMode itself, to avoid this
    // host-independent interface depending back on AiAssistant's own type.
    virtual QString splitPlanPreflightPrompt(const QString &requestText, bool docsIntegrated) const = 0;

    // The one-shot pre-flight prompt asking whether `newRequestText` continues the ongoing
    // conversation or is a new, independent task, or is genuinely unclear — asked before
    // split-mode decomposition begins, only when there's existing conversation history/an existing
    // original request that could conflict. Deliberately NOT self-contained this time: unlike
    // splitPlanPreflightPrompt(), this call's own ask() site passes
    // includeHistory=true, so the real (settings-windowed) conversation history — including
    // whatever the prior turn actually PRODUCED (e.g. a created object's real name), not just what
    // was asked — arrives as genuine prior messages ahead of this text. Confirmed live: an earlier
    // version restated only the prior turn's raw request text ("new note") with no history at all,
    // so a follow-up referencing "the note"/"this note" by name couldn't be recognized as a
    // continuation — the model was never told what the prior turn actually created. Host-specific
    // only in the sense every prompt in this family lives behind this interface so no host holds
    // its own QString literals; the judgment itself is generic. Expected answer: exactly one word,
    // "CONTINUE", "NEW", or "UNCLEAR".
    virtual QString continuationCheckPreflightPrompt(const QString &newRequestText) const = 0;

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

    // A compact reminder of the EXPLORE convention (return a read-only "# EXPLORE"-first-line
    // script to check real state before committing to an action; QtiSAS runs it, feeds the
    // output back, then the real action script follows) — spliced into the base system prompt,
    // but ONLY for AiAssistant::DocsMode::Integrated (see buildSystemPrompt()'s call site).
    // Every other mode already gets the FULL teaching for this (the "Agentic Explore Mode"
    // section of this host's own doc corpus, attached wholesale), so injecting it there too
    // would be pure duplication. Integrated mode never attaches doc content locally at all —
    // whether the model ever learns EXPLORE exists then depends entirely on a server-side RAG
    // backend's retrieval happening to surface that doc section for a given call's query text,
    // which is neither guaranteed nor consistent across a conversation's many separate calls
    // (self-review, retries, ...). Confirmed live: a request needing exactly one EXPLORE round
    // to resolve a naming ambiguity instead spent 10 rounds guessing plausible window names by
    // trial and error — the model never once tried checking real state, apparently because
    // nothing in that conversation's later retrieval rounds happened to remind it EXPLORE was
    // an option. Host-specific because it's phrased around this host's own scripting API
    // (scriptPrint(), the specific marker convention), like every other hook in this interface.
    virtual QString exploreModeReminder() const = 0;

    // The opposite of exploreModeReminder(): text telling the model that probe rounds are NOT
    // available for this request, so it must not return a "# EXPLORE" script at all.
    //
    // Needed because dropping the reminder is not the same as forbidding the thing. Confirmed live:
    // with EXPLORE unavailable and the reminder correctly withheld, 6 of 12 LIST items still opened
    // with a probe — the API corpus carries its own "Agentic Explore Mode" rules and a MANDATORY
    // table, so the model was obeying the docs, not an invitation from here. Each of those cost a
    // refusal plus a re-prompt: ~111s of a 596s run. Silence left the corpus unopposed; this states
    // the contradiction explicitly, in the one place that knows the request's actual mode.
    virtual QString exploreUnavailableNote() const = 0;

    // States, as fact, what the host currently holds under each name the request calls existing —
    // `facts` pairs each name with the kind label(s) AiScriptHost::entityKinds() reported for it,
    // an empty list meaning nothing of any kind exists by that name. Returns empty when there is
    // nothing worth saying.
    //
    // This is the generate-only replacement for existingEntityVerificationPrompt()'s whole
    // round-trip, and strictly stronger than what it replaced: the same VERDICT facts, but read
    // from live state instead of from a script the model had to write, so it cannot be a fabricated
    // verdict (confirmed live: an entire check script was once a bare
    // scriptPrint("MISSING: Disk") — no check call anywhere, and wrong), costs no call, and
    // executes nothing in a mode where the user asked for no execution. It also carries what the
    // probe never could: the name's actual KIND, so "exists, as a matrix" can stop a request that
    // meant a table.
    virtual QString liveEntityFactsNote(const QList<QPair<QString, QStringList>> &facts) const = 0;

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
    // detected. `wholeRequest`: `partDescription` is a whole request, not one part of a split plan — a name it
    // leaves open is then the script's to choose, since no later part refers to it.
    virtual QString splitPartScopeViolation(const QString &code, const QString &partDescription,
                                            const QString &earlierPartsCode, bool wholeRequest = false) const = 0;

    // Names every DELIVERABLE `requestText` asks for that leaves a mechanical fingerprint in code,
    // and whose fingerprint appears nowhere in `code` — "you were asked for residuals and there is
    // no residuals column anywhere in this script". The under-delivery counterpart to
    // splitPartScopeViolation() above, which catches doing too MUCH; that one's own underreach
    // Direction only covers a missing fit call, and generalizing it per-Direction does not scale.
    //
    // Deliberately a presence test over the WHOLE script, never per-branch: a script that
    // legitimately guards ("if gw is None: report and stop") still has to CONTAIN the calls in the
    // branch that does the work, so a guard cannot trip this. Confirmed live, and the reason this
    // exists: asked to plot the data, fit curve and residuals into a graph, one item's script
    // contained no addCurve/insertCurve and no "residu" at all — its else-branch recoloured one
    // existing curve instead, i.e. it performed a LATER item's task and none of its own, and
    // nothing in the pipeline noticed.
    //
    // Host-specific for the same reason splitPartScopeViolation() is: the fingerprints are this
    // host's own API calls. Returns a human-readable explanation to feed back for a corrected
    // retry, or an empty string when every requested deliverable is accounted for.
    virtual QString missingRequestedDeliverables(const QString &code, const QString &requestText) const = 0;

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

    // Runs EVERY known, host-specific, STATELESS pre-execution mistake pattern against `code` (and,
    // for the patterns that need it, `originalRequestText` and/or `earlierPartsCode`) and collects
    // ALL matches — one dispatch point replacing what used to be 25 separate single-purpose pure
    // virtual methods here (compiled-function self-shadow bugs, fitCurve name-reconstruction
    // mismatches, 0-based Table/Matrix indexing, hallucinated Legend/GraphWindow methods,
    // nonexistent Qti3D-import forms, guessed candidate-name lists, ...). Each
    // individual pattern is still implemented as its own private helper in QtiSasAiContent.cpp, with
    // its own "confirmed live" incident writeup right next to its regex — only the PUBLIC interface
    // (and AiAssistant.cpp's call site) is consolidated. Deliberately excludes the self-review
    // BEFORE/AFTER regression checks below (selfReviewFunctionNameSubstitution and siblings) — those
    // compare a preReviewDraft against postReviewCode, a fundamentally different shape neither
    // reducible to nor combinable with a single-code-snapshot check. Host-specific because every
    // individual pattern depends on this host's own scripting API. Returns an empty list if nothing
    // matched.
    //
    // Deliberately collects every match instead of stopping at the first, as earlier versions of
    // this method did. Confirmed live: a single blocked script legitimately tripped MULTIPLE
    // independent patterns at once (e.g. a hardcoded chi2/dof alongside a missing PyQt import).
    // Stopping at the first meant only one issue was ever surfaced per retry round — the model fixed
    // that one, the corrected script tripped the second pattern on the very next round, and so on,
    // burning a retry per independent defect instead of fixing all of them together from one grounded
    // prompt. Every pattern here is a pure function of its inputs with no dependency on any other
    // pattern's result, so running all of them against the same code snapshot and combining the
    // matches is safe. Order is preserved (same fixed priority order as before) since it becomes the
    // presentation order in the combined retry prompt — see AiAssistant::runActionScript()'s call
    // site, which joins the list into one errContext.
    //
    // `earlierPartsCode` is the concatenated, actually-executed code of every earlier,
    // already-completed split-mode part (empty string outside split mode — see
    // AiAssistant::runActionScript()'s call site, which passes splitPartScripts.join("\n\n")) —
    // needed by patterns that check "was the requested action performed ANYWHERE in fulfilling the
    // whole request," not just within this one script. Confirmed live: without this, a check
    // requiring THIS part to contain a compile() call whenever the WHOLE original request mentions
    // "compile" would wrongly block a later part that legitimately just fits/plots a function an
    // EARLIER part already compiled — the intended division of labor in split mode, not a defect.
    //
    // `laterPartsDescriptions` is the symmetric case: descriptions (not code — they haven't run
    // yet) of every split-mode part AFTER this one. Confirmed live: a 2-part plan's Part 1 ("create
    // note X") was permanently blocked by requestedCreationNeverHappenedMistake() for not creating
    // a table that Part 2's own description already claimed — earlierPartsCode had no way to exempt
    // a deliverable that hasn't happened yet BECAUSE it's someone else's part, not a defect.
    //
    // exploreScript=true skips the "requested action never happened" checks: an EXPLORE script is
    // read-only by construction (explorePrefixBeforeFirstWriteCall() cuts every write call away),
    // so demanding newTable()/compile() in it is unsatisfiable. Confirmed live: that produced an
    // infinite loop — the check told the model to add newTable(), the addition was truncated, the
    // check fired again, byte-identical for 3+ rounds at 37-50s each.
    virtual QList<AiKnownMistakeMatch> detectKnownMistakes(const QString &code, const QString &originalRequestText,
                                                           const QString &earlierPartsCode,
                                                           const QString &laterPartsDescriptions = QString(),
                                                           bool exploreScript = false) const = 0;

    // An EXPLORE-labeled script is supposed to be strictly read-only reconnaissance — nothing
    // upstream of this call mechanically enforces that; it's purely a prompt-level convention the
    // model is expected to follow (see ai-python-api.md's "EXPLORE must be read-only" rule). This
    // scans `code` for this host's own write/create calls (newTable(), Compiler.compile(),
    // Fittable.fit(), newGraph(), addCurve(), ...) appearing at top-level (unindented) and, if any
    // are found, returns just the prefix of `code` strictly BEFORE the first such line — the
    // genuinely-read-only part actually worth running as EXPLORE. Returns `code` unchanged if no
    // write call is found at top level. Confirmed live: a script that started with a few harmless
    // `# EXPLORE` lines (categories(), functionNames() filtering) went on, in the SAME response, to
    // create a real table, compile two real fit functions, run two real fits, and build a real
    // graph — all executed for real under the EXPLORE label, since nothing distinguished "labeled
    // EXPLORE" from "safe to run as EXPLORE." Host-specific because the write/create vocabulary is
    // this host's own API surface, not something the orchestrator can name generically.
    virtual QString explorePrefixBeforeFirstWriteCall(const QString &code) const = 0;

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

    // Builds the "existing X" verification preflight prompt. `labels` per name narrows which
    // check(s) apply ({"window"}, {"compile","fittable"}, or empty for genuinely unknown — checks
    // everything). See AiAssistant::extractExistingEntityNames()/splitPlanCreationNames().
    // `includeDataProbe` asks the generated script to report a named table's row count and column
    // names alongside the bare exists/missing verdict — for a request that will FIT or PLOT that
    // table, existence alone is the wrong question. Confirmed live: a table created with 0 rows
    // reported EXISTS, the item proceeded, and fit() died ~90s later on "failed to read data for
    // the selected dataset(s)". False for a pure name-collision check, which has no dataset to
    // inspect and must stay a minimal read-only probe.
    virtual QString existingEntityVerificationPrompt(const QList<QPair<QString, QStringList>> &names,
                                                     bool includeDataProbe) const = 0;

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

    // Checks whether a pre-review draft had a COMPLETE, correct Compiler compile setup (all of
    // setFunctionName()/setGroup()/setParamCount()/setParamNamesAll()/setParamInitsAll()/
    // setParamLimitsAll()/setParamVariesAll(), for whichever ones it actually used, before
    // compile()), but self-review's own output still calls compile() while missing one or more of
    // those calls the draft had — self-review has no basis to drop required setup calls from an
    // already-complete sequence. Confirmed live: a correct, complete compile setup was replaced
    // wholesale with old-style raw-array C-function code (`double Fn(double* x, double* par)`)
    // missing setGroup()/parameter declarations entirely, raising ValueError: "compile: group is
    // empty" on the very next run — different in kind from selfReviewFunctionNameSubstitution/
    // selfReviewLookupToCreationRegression/selfReviewEnumConstantRegression above (a single wrong
    // name/lookup/value): this is a structural regression in a required multi-call sequence, not
    // one wrong value. Host-specific because it depends on this host's own Compiler setup-call
    // vocabulary. Returns a human-readable explanation, or an empty string if no such regression is
    // found (including when neither script calls compile() at all).
    virtual QString selfReviewCompileSequenceRegression(const QString &preReviewDraft,
                                                        const QString &postReviewCode) const = 0;

    // Checks whether a pre-review draft correctly imported a Qt class (QColor, QFont, QPen, QBrush,
    // QFrame, Qt) from ONE PyQt major version (PyQt5 or PyQt6), but self-review's own output
    // imports that SAME class from the OTHER major version instead — self-review has no basis to
    // switch between them, since it can't observe which one this build actually embeds. Confirmed
    // live: a correct `from PyQt6.QtGui import QColor, QFont` draft (this build's real binding) was
    // silently rewritten by self-review, while fixing an unrelated issue elsewhere in the same
    // script, to `from PyQt5.QtGui import QColor, QFont` — PyQt5 doesn't exist in this environment,
    // and the script reached actual execution and crashed with "ModuleNotFoundError: No module
    // named 'PyQt5'" a full round-trip later, a wasted retry the other self-review regression
    // backstops above don't cover (they check function names/lookups/enum constants/compile
    // sequences — none of them look at WHICH PyQt major version an already-correct import uses).
    // Self-review regenerates the whole script from scratch every pass rather than patching only
    // the lines it was actually asked to fix, so an unrelated-but-correct import line has no
    // protection from being silently "re-derived" from the model's own general training-data
    // priors (which skew heavily toward the far more common PyQt5) instead of copied verbatim from
    // the draft. Host-specific because it depends on this host's own PyQt-import calling
    // convention. Returns a human-readable explanation of the substitution, or an empty string if
    // no PyQt-version change is found (including when neither script imports any of these classes).
    virtual QString selfReviewPyQtVersionSubstitution(const QString &preReviewDraft,
                                                      const QString &postReviewCode) const = 0;

    // Checks whether a pre-review draft correctly reused an EXISTING compiled function by opening
    // it (Compiler.open(name)) and recompiling with NO setCode() call in between (relying on
    // whatever code is already loaded) — but self-review's own output adds a setCode() call for
    // that SAME function name where the draft had none, fabricating a replacement body from
    // nothing. Self-review has no docs or history telling it what the existing, already-working
    // code actually is, so any setCode() it invents here is a guess, not a fix. Confirmed live,
    // severe: EXPLORE had already printed the EXISTING, correct code for two functions
    // ("y = scale * exp(-pow((x - center) / width, 2.0) / 2.0) + background;"); the pre-review
    // draft correctly did nothing but `c.open(name); ok = c.compile()` for each — reusing that
    // known-good code. Self-review rewrote this into a version that adds `c.setFunctionName(name)`
    // (a no-op self-rename) and a brand-new `c.setCode(...)` with a fabricated, wrong-syntax full
    // C-function-signature body — neither function actually compiled (both returned False), and
    // because nothing checked the result (see the companion detectKnownMistakes() pattern for
    // that), the failure was silently swallowed and the whole request reported complete anyway.
    // Different in kind from selfReviewCompileSequenceRegression above: that one catches required
    // calls DROPPED from an already-complete sequence; this one catches a setCode() call ADDED
    // where the draft correctly had none — the same root cause (whole-script regeneration touching
    // something the requested fix never asked about) in the opposite direction. Host-specific
    // because it depends on this host's own Compiler.open()/setCode()/compile() calling
    // convention. Returns a human-readable explanation, or an empty string if no such fabrication
    // is found (including when neither script opens a function with no setCode() of its own).
    virtual QString selfReviewFabricatedSetCodeMistake(const QString &preReviewDraft,
                                                       const QString &postReviewCode) const = 0;

    // Checks whether a pre-review draft correctly named a newly-created object (newTable/newMatrix/
    // newGraph/newNote) using the exact quoted name from the request, but self-review's own output
    // still creates that SAME kind of object with a DIFFERENT name — self-review has no access to
    // the original request text, so it has no legitimate basis to alter a creation name the draft
    // already got right. Confirmed live: a request naming the table "SAXS_data" survived correctly
    // into the pre-review draft; self-review's rewrite silently dropped the underscore ("SAXSdata")
    // in BOTH the newTable() call and an unrelated scriptPrint() confirmation string in the same
    // script — the second occurrence rules out any C++-side name sanitization (which would never
    // touch a plain print string), confirming this is self-review's own text-level typo, not a
    // runtime effect. Host-specific because it depends on this host's own creation-call vocabulary.
    // Returns a human-readable explanation, or an empty string if no such substitution is found.
    virtual QString selfReviewCreationNameSubstitution(const QString &preReviewDraft,
                                                       const QString &postReviewCode) const = 0;

    // Checks whether `code` calls setGroup(name) with a value that matches none of the groups an
    // immediately-preceding EXPLORE round actually printed (a line containing "group"/"groups" plus
    // a bracketed, quoted list — e.g. a c.groups() listing) — a different shape from every check
    // above: those all compare a pre-review draft against self-review's own edit; this compares the
    // ORIGINAL candidate against real EXPLORE console output from earlier in the SAME part, so it
    // isn't reducible to the BEFORE/AFTER pattern either. setGroup() has no rule requiring an
    // existing group (compile-python-api.md only bans an empty name or one containing '/'), so this
    // can never be a hard error — a brand-new group is a legitimate answer — but real, just-printed
    // data sitting unused next to an unrelated invented value is worth surfacing once. Confirmed
    // live, twice: EXPLORE printed the real existing groups via c.groups(), and the very next
    // round's write action called setGroup("custom:sans") anyway, matching none of them — even in
    // the run where the printed list was complete and EXPLORE otherwise behaved well (found a real
    // reference function via the documented functionNames("ALL") keyword-search pattern). The
    // exploration ran, cost a full round-trip, and had zero effect on the output. Host-specific
    // because it depends on this host's own Compiler.groups()/setGroup() vocabulary. Returns a
    // human-readable explanation, or an empty string if there's no setGroup() call, no EXPLORE
    // output naming groups at all, or the chosen group does appear in what was printed.
    virtual QString exploreGroupsIgnoredMistake(const QString &code, const QString &exploreOutput) const = 0;

    // Returns `code` with setParamCount() corrected to the length every setParam*All() list agrees
    // on, or empty when there is nothing to repair or the lists disagree (then the intended count
    // is a judgment call, not a fact). Repaired rather than reported because the model proved
    // unable to act on it: qtisas's own ValueError names the exact mismatch and the retry came
    // back byte-identical twice, which then discarded the rest of a 12-item LIST.
    virtual QString compilerParamCountRepair(const QString &code) const = 0;

    // Returns `code` with setCode()'s lone declaration turned into an assignment to the yName, or
    // empty when there is nothing to repair or the body has several declarations (then which one
    // was meant to be the result is a guess). Same rationale as the count repair above: confirmed
    // live twice, "double I" -> clang error -> "double y" -> blocked cost two rounds every time.
    virtual QString compilerYNameAssignmentRepair(const QString &code) const = 0;

    // Returns `code` without its self-referential C++ declarations (`double A = A;`, which reads an uninitialized
    // local instead of the parameter), or empty when there are none. Repaired because the block alone did not help:
    // confirmed live, the local model resubmitted the same declarations five times in a row and gave the item up.
    virtual QString compilerSelfShadowRepair(const QString &code) const = 0;

    // The label the request asks its result line to start with ("R4:"), or empty when it names none.
    virtual QString resultLabel(const QString &requestText) const = 0;

    // The request's own sentences that ask for the result line ("Print ... The result label is \"R4:\"."), or empty.
    // A split plan gives them to its last part: confirmed live (LIST 5, item 4), no part's description carried the
    // result line, the part that printed one was told to compute values only a later part had, and no line was left.
    virtual QString resultLineDuty(const QString &requestText) const = 0;

    // Returns `code` with its fixed-text result lines (scriptPrint("R4: done")) turned into `pass` — with
    // `alsoComputed`, every one-line result-line print — or empty when it has none. Used for a split part before the
    // last one: that line is the last part's job, and blocking it there cost the item (the model sent it back
    // unchanged twice).
    virtual QString nonFinalResultLineRepair(const QString &code, const QString &requestText,
                                             bool alsoComputed) const = 0;

    // Non-empty when the script role-plays further turns ("USER:"/"ASSISTANT:" lines) with
    // invented prompts, scripts and tool output. Checked even for a script that failed to
    // compile — that is the usual way this surfaces, and "SyntaxError" names the wrong problem.
    virtual QString fabricatedConversationCheck(const QString &code) const = 0;
};

#endif
