/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant implementation
 ******************************************************************************/

#include "AiAssistant.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimer>

static QString loadSystemPrompt(QString *statusOut = nullptr, bool unlimited = false)
{
    QStringList docDirs = {
#ifdef Q_OS_MACOS
        QCoreApplication::applicationDirPath() + "/../Resources/python/doc"
#elif defined(Q_OS_WIN)
        QCoreApplication::applicationDirPath() + "/python/doc"
#else
        QCoreApplication::applicationDirPath() + "/../share/qtisas/python/doc",
        QCoreApplication::applicationDirPath() + "/../qtisas/python/doc",
        QCoreApplication::applicationDirPath() + "/python/doc",
#endif
    };

    QString docDir;
    for (const QString &d : docDirs)
        if (QDir(d).exists())
        {
            docDir = d;
            break;
        }

    if (docDir.isEmpty())
    {
        if (statusOut)
            *statusOut = "AI: API doc directory not found — sending without context.";
        return {};
    }

    // Compact mode: use .pyi stubs (~2500 tokens total, fits 8k context)
    // Unlimited mode: use full .md docs (~16500 tokens, for large-context models)
    const QString pattern = "*-python-api." + (unlimited ? QString("md") : QString("pyi"));
    QStringList files = QDir(docDir).entryList({pattern}, QDir::Files, QDir::Name);
    QString prompt;
    QStringList loaded, missing;

    for (const QString &fname : files)
    {
        QFile f(docDir + "/" + fname);
        if (f.open(QIODevice::ReadOnly))
        {
            prompt += "\n\n=== " + fname + " ===\n";
            prompt += QString::fromUtf8(f.readAll());
            loaded << fname;
        }
        else
            missing << fname;
    }

    if (statusOut)
    {
        if (loaded.isEmpty())
            *statusOut = "▸▸▸ AI: no API doc files found in " + docDir;
        else
            *statusOut = "▸▸▸ AI loaded API docs: " + loaded.join(", ") +
                         (missing.isEmpty() ? "" : " | missing: " + missing.join(", "));
    }
    return prompt;
}

QString AiAssistant::userRulesFilePath()
{
#ifdef Q_OS_WIN
    const QString sasPath = QDir::homePath() + "/AppData/Local/qtisas/";
#else
    const QString sasPath = QDir::homePath() + "/.config/qtisas/";
#endif
    return sasPath + "ai/user-rules.md";
}

QStringList AiAssistant::userRules()
{
    QFile f(userRulesFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QStringList rules;
    for (const QString &line : QString::fromUtf8(f.readAll()).split('\n'))
    {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith("- "))
            rules << trimmed.mid(2).trimmed();
    }
    return rules;
}

static bool saveUserRules(const QStringList &rules)
{
    const QString path = AiAssistant::userRulesFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;
    QTextStream out(&f);
    out << "# User Rules\n\n";
    for (const QString &r : rules)
        out << "- " << r << "\n";
    return true;
}

bool AiAssistant::addUserRule(const QString &text, int maxRules, QString *errorOut)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        if (errorOut)
            *errorOut = "addRule: rule text is empty.";
        return false;
    }
    QStringList rules = userRules();
    if (rules.size() >= maxRules)
    {
        if (errorOut)
            *errorOut = QString("addRule: rule limit reached (%1/%1). Options: remove an old rule with "
                                "removeRule(index) (see showRules() for indices), raise the limit in "
                                "Preferences → AI, or start over with removeAllRules().")
                            .arg(maxRules);
        return false;
    }
    rules << trimmed;
    return saveUserRules(rules);
}

bool AiAssistant::removeUserRule(int index, QString *errorOut)
{
    QStringList rules = userRules();
    if (index < 1 || index > rules.size())
    {
        if (errorOut)
            *errorOut = QString("removeRule: no rule at index %1 (have %2).").arg(index).arg(rules.size());
        return false;
    }
    rules.removeAt(index - 1);
    return saveUserRules(rules);
}

bool AiAssistant::clearUserRules()
{
    return saveUserRules({});
}

// Checklist for the mandatory pre-execution self-review pass (see askAi()'s responseReady
// handler): User Rules plus a short, hand-curated list of documented gotchas that are SILENT
// failures (no exception, so the error-retry mechanism can't catch them) rather than things
// already enforced by a real runtime error. Deliberately NOT the whole doc corpus — checking
// against hundreds of heterogeneous rules is the same large, fuzzy task generation already does
// (and already fails non-deterministically); a short, enumerable checklist is what a review
// pass can actually check reliably.
QString AiAssistant::selfReviewChecklist()
{
    QString checklist = "- Don't call showGrid() (or other layout overrides) unconditionally — they override "
                        "QtiSAS Preferences (e.g. graphTitleOn(), graphMajTicksStyle()). Omit them unless the "
                        "task explicitly requires overriding the user's configured defaults.\n"
                        "- Don't wrap setParamValue()/setParamVaries()/fit() in a try/except that swallows the "
                        "error and continues (e.g. calling plotResults() afterward regardless) — let real "
                        "errors propagate so they can be corrected properly.\n"
                        "- In axis/plot titles, use <sup>/<sub> HTML tags for superscript/subscript (e.g. "
                        "\"cm<sup>-1</sup>\", \"R<sub>g</sub>\") — never LaTeX-style ^{...}/_{...} notation, "
                        "which renders as literal characters, not a real superscript/subscript.\n"
                        "- If a table has a yErr/dI column intended for weighted fitting, assigning it "
                        "Table.PlotDesignation.yErr does NOT enable weighting by itself — you must also call "
                        "your Fittable instance's f.setWeightingEnabled(m, True) + f.setWeightingDataset(m, "
                        "colName) — methods on the Fittable object (e.g. `f`), never bare calls or methods on "
                        "the Table. `m` is a 1-based dataset INDEX (int), e.g. 1 for the first/only configured "
                        "dataset — NOT the \"tablename_colname\" dataset NAME string used almost everywhere "
                        "else in this API (configure()'s datasets=, f.datasets(), etc.). That's the one "
                        "exception, and it's easy to miss. Confirmed live, both wrong forms, repeatedly across "
                        "separate conversations: f.setWeightingEnabled(\"table_col\", True) raises TypeError: "
                        "setWeightingEnabled(self, m: int, enabled: bool): argument 1 has unexpected type "
                        "'str'; f.setWeightingEnabled(0, True) (0-based habit) raises ValueError: "
                        "setWeightingEnabled: dataset 0 out of range (1..N) — valid indices start at 1, not 0. "
                        "Without setWeightingEnabled/Dataset at all, chi2/dof silently reflects unweighted "
                        "residuals dominated by absolute data magnitude, not real fit quality (confirmed: "
                        "~1e14 unweighted vs ~1.8 weighted on the same fit) — no error, no warning. Confirmed "
                        "separately: writing this as a bare setWeightingEnabled(...) call (no object prefix) "
                        "raises NameError; as t.setDataSetWeightEnabled(...) on the Table raises AttributeError "
                        "— both are wrong, only f.setWeightingEnabled(...) works.\n"
                        "- When correcting a script after a runtime error, don't silently drop the "
                        "functionality that caused it — fix the call to do what was actually asked. If that's "
                        "genuinely not possible, say so in the printed output rather than quietly reverting to "
                        "default behavior. Confirmed: asked to set all fit parameters to an unbounded range, a "
                        "TypeError from passing Compiler's bracket-notation string (\"..\") to "
                        "Fittable.setParamLimits() (which takes separate float args, not a string — the two "
                        "classes have unrelated 'limit' APIs) was 'fixed' by deleting the limit-setting loop "
                        "entirely. The script then ran clean, but never set the requested limits at all. Same "
                        "pattern, different shape: asked for a GLOBAL fit across two named datasets, neither "
                        "existed — 'fixed' by dropping to a single-dataset fit against whatever ONE table was "
                        "available. That's not a corrected version of a global fit, it's a structurally "
                        "smaller, different task (a global/multi-dataset fit is impossible with fewer datasets "
                        "than requested). If a correction would reduce the number of datasets/parameters/"
                        "features below what the request asked for, don't run that reduced version — report "
                        "which specific data is missing via scriptPrint() and let the script end normally. Do "
                        "NOT raise an exception for this report — raising is treated as a bug and triggers "
                        "another automatic retry, undoing the correct report (confirmed live: this exact "
                        "mistake happened with `raise RuntimeError(\"...not available...\")` — the very next "
                        "retry abandoned the correct report and went back to attempting the smaller fit).\n"
                        "- Same violation, same-COUNT variant: swapping in a DIFFERENT specific table/column/"
                        "dataset name than the one the original request explicitly named, just because that "
                        "one happens to exist, is equally out of scope even though the count of items didn't "
                        "shrink. An error's \"Available: X\" listing is context for a report, never implicit "
                        "permission to configure/fit against X instead of what was actually named. Confirmed "
                        "live: after correctly reporting that table \"AAA\" column \"BBB\" didn't exist, a "
                        "later retry silently configured against a different table's column just because "
                        "it was the only one available — never re-raising that the original name was still "
                        "missing. EXCEPTION: this does NOT apply once the user's CURRENT message explicitly "
                        "hands the choice back to you (\"select/choose it yourself\", \"use whatever's "
                        "available\", \"pick any\", or similar) — that is authorized deviation, not silent "
                        "substitution; honor it rather than reverting to the original name. Confirmed live: "
                        "exactly this exception was missed once — after \"select column yourself\" correctly "
                        "produced a script using a real column, self-review reverted it back to the original, "
                        "already-rejected column name instead of accepting the authorized substitution.\n"
                        "- c.compile() returns a bool — always check it: `ok = c.compile(); if not ok: "
                        "scriptPrint(qti.app.resultsLog().toPlainText()[-500:]); raise RuntimeError(...)`. A bare "
                        "`c.compile()` with no check silently continues past a failed build.\n"
                        "- If the script calls f.configure(name, [\"table_col\", ...]) naming a table/column not "
                        "yet confirmed to exist in THIS conversation, guard BOTH levels before the configure() "
                        "call (see fittable-python-api.md Rule 2), nested as if/else so configure()/fit() only "
                        "run in the innermost else: `if not existTable(\"table\"): scriptPrint(...) else: t = "
                        "table(\"table\"); if \"col\" not in t.colNames(): scriptPrint(...) else: f.configure(...)`. "
                        "existTable() ALONE is not enough — it only rules out \"table doesn't exist at all\"; a "
                        "table can exist while the named column doesn't, and configure() still raises the exact "
                        "same ValueError one level down. Don't let configure() itself discover either kind of "
                        "missing. Confirmed live TWICE: once with no guard at all, once with an existTable()-only "
                        "guard that let a missing-column case fall straight through into configure() anyway — "
                        "both cost a full extra retry round-trip that the two-level check would have avoided.\n"
                        "- Never use exit()/quit()/sys.exit() for an early-exit guard (e.g. after an "
                        "existTable() check) — they all raise SystemExit, and the script runner treats "
                        "ANY uncaught exception, SystemExit included, as a script error and triggers an "
                        "automatic retry, exactly like a real crash. Use if/else nesting instead: put the "
                        "rest of the script in an else branch rather than exiting early from the if.\n"
                        "- If this script's only content is a scriptPrint() stating something 'does not "
                        "exist'/'not found', with no actual existTable()/colNames()/datasets()-style check "
                        "anywhere in it, that conclusion is being recycled from earlier in the conversation, "
                        "not verified now — replace it with a real check (see fittable-python-api.md Rule 2) "
                        "so the negative conclusion is only kept if it's still actually true. Confirmed live: "
                        "asked to retry after a phrasing implying the missing table now exists (\"new "
                        "table\"), the exact same 'does not exist' text from 3 turns earlier was resent "
                        "byte-for-byte with no verification call at all, twice in a row.\n"
                        "- \"Return it byte-for-byte unchanged if compliant\" applies to EVERY line of the "
                        "script — imports, assignments, any statement — not just names, and not just the "
                        "lines that seem related to whatever you're focused on. Silently dropping, reordering, "
                        "or altering a line that the checklist gives no reason to touch is itself a checklist "
                        "violation, even if the line looks unrelated to anything you're reviewing for. "
                        "Confirmed live TWICE, two different shapes: (1) self-review silently changed "
                        "newTable(\"SANS_sphere\", ...) to newTable(\"SANSsphere\", ...) with no checklist "
                        "violation requiring it — table/column/window/dataset names are matched as exact, "
                        "case-sensitive strings, so even dropping one underscore makes existTable()/table()/"
                        "graphWindow() silently fail to find the very object an earlier, successful step just "
                        "created; every later turn, unaware of the change, kept trying the original name "
                        "(returns None -> ValueError), then a third, different-again variant — three "
                        "spellings across one conversation, the real table never found again. (2) self-review "
                        "silently deleted two working, needed lines — \"from PyQt6.QtGui import QColor, QPen\" "
                        "and \"from PyQt6.QtCore import Qt\" — while leaving a QColor(...) call that depends "
                        "on them untouched a few lines later, turning a script that would have run clean into "
                        "one that raises NameError: name 'QColor' is not defined — which then triggered nine "
                        "further retry rounds guessing at an unrelated function's argument types, none of "
                        "which were ever the actual problem.\n"
                        "- Flag and remove any `import math` / `from math import ...` line, and rewrite "
                        "`math.sin(...)` / `math.cos(...)` / etc. calls to bare `sin(...)` / `cos(...)` — "
                        "sin, cos, exp, log, sqrt, pi, e, floor, ceil, abs are ALL already global in this "
                        "environment, and importing math is never correct here even though it's completely "
                        "normal, idiomatic Python elsewhere. This check does not depend on the API docs being "
                        "loaded for this request — apply it to every script regardless of topic. Confirmed "
                        "live: a script using `from math import pi, sin, cos` reached self-review and was "
                        "returned unchanged, twice in a row (self-review only checks against this checklist, "
                        "not the API docs, so a violation the docs describe elsewhere is invisible here unless "
                        "it's also listed as its own checklist item, which is why this one exists explicitly).\n"
                        "- If the script uses QColor, QFont, QPen, Qt, QFrame, or QBrush, it MUST start with "
                        "the matching import(s) — `from PyQt6.QtGui import QColor, QFont, QPen, QBrush`, "
                        "`from PyQt6.QtCore import Qt`, `from PyQt6.QtWidgets import QFrame` — add whichever "
                        "are missing; these are never in the default namespace and using them without the "
                        "import raises NameError. This check does not depend on the API docs being loaded for "
                        "this request — apply it to every script regardless of topic, same as the math-import "
                        "check above. Confirmed live: a script called QColor(\"red\") with no import, reached "
                        "self-review, and was returned unchanged; the resulting NameError was 'fixed' by "
                        "switching to Qt.red (also unimported, same NameError), then to a bare integer color "
                        "code — three rounds spent working around a missing import instead of just adding it, "
                        "because self-review never checks against the API docs, only this checklist.\n"
                        "- If the script calls setFrameStyle(...) on a LegendWidget/ImageWidget/RectangleWidget/"
                        "EllipseWidget (e.g. from newLegend()), and the argument is built from PyQt6's "
                        "QFrame.Shape/QFrame.Shadow (e.g. `QFrame.Shape.Box | QFrame.Shadow.Sunken`), that is "
                        "wrong regardless of whether the QFrame import is present — QtiSAS's own FrameWidget "
                        "base class takes a DIFFERENT, unrelated enum (Frame.FrameStyle.Line=1, .Shadow=2) and "
                        "does an exact-value match, not Qt's bit-flag decoding. A PyQt6 QFrame combo produces "
                        "some int that matches neither Line nor Shadow, so no NameError and no crash — the "
                        "frame border simply never renders. Fix by replacing the argument with "
                        "Frame.FrameStyle.Line.value or Frame.FrameStyle.Shadow.value — NOT the bare enum member "
                        "(Frame.FrameStyle.Shadow alone raises TypeError: argument has unexpected type "
                        "'FrameStyle'; confirmed live). FrameStyle has a member named None_, so unlike "
                        "Graph.Axis/Graph.Ticks/Graph.Scale/Graph.CurveType (no None member, bound as plain "
                        "ints, usable directly anywhere) it's a genuine Python enum.Enum, not an int — .value "
                        "is required wherever a plain int is expected. Also drop the now-unneeded "
                        "PyQt6.QtWidgets QFrame import if nothing else in the script needs it — do NOT "
                        "'fix' this by adding the QFrame import per the rule above; that only removes a "
                        "NameError this pattern doesn't even raise, while leaving the actual bug (wrong enum, "
                        "invisible frame) untouched. Confirmed live: a script styled a fit-result legend with "
                        "exactly this QFrame combo; it ran with no error and no visual sign anything was wrong, "
                        "the requested boxed/sunken legend frame just silently never appeared.\n"
                        "- After f.plotResults(), layer-1 curve index 0 is the DATA scatter and index 1 is "
                        "the FIT line (data is always inserted first) — never the other way round, and never "
                        "\"the only curve\". If the script styles curve 0 via Graph methods (e.g. "
                        "g.setCurveLineColor(0, ...)) with the intent of coloring/styling the FIT line "
                        "(e.g. after a request like \"set line color to X\"), that targets the wrong curve — "
                        "it recolors the raw data points instead, while the fit line stays unstyled. The "
                        "correct, purpose-built way to color the fit curve is f.setCurveColor(...), called "
                        "BEFORE fit()/plotResults() — prefer it over guessing at Graph curve indices "
                        "afterward; if both are present for the same color, the Graph-level call is "
                        "redundant at best and wrong-target at worst — remove it. Confirmed live: "
                        "f.setCurveColor(\"orange\") correctly styled the fit line, but the script also kept "
                        "an inherited g.setCurveLineColor(0, colorIndex(\"orange\")) line from an earlier turn, "
                        "silently recoloring the data scatter to match instead of leaving it alone.\n"
                        "- If the script calls `random.gauss()`, `random.random()`, `random.uniform()`, or any "
                        "other `random.*` function, `import random` must be present (as the first line, by "
                        "convention) — add it if missing. Same reasoning as the two import checks above: this "
                        "is a foundational rule that lives in the API docs, not in this checklist by default, "
                        "so self-review must check for it explicitly rather than assuming a missing-import "
                        "NameError can't happen here.\n"
                        "- If this is a corrected retry after a runtime error, and the script contains a "
                        "`plot(...)` call, check whether that same `plot(...)` call already ran successfully "
                        "in the FAILED attempt (i.e. the failure happened on a LATER line) — if so, the "
                        "corrected script blindly re-running it will create ANOTHER new graph window, since "
                        "`plot()` has no name-based reuse at all (unlike `newTable()`/`newGraph()`, which at "
                        "least resize an existing object instead of duplicating it). Fix by checking "
                        "`graphWindow(name)` first and skipping the re-plot if it already exists, or by moving "
                        "`plot()` after whatever was actually failing so it only runs once the rest works. This "
                        "check does not depend on the API docs being loaded — the error-retry prompt that "
                        "generates the corrected script deliberately omits docs to control context growth, so "
                        "self-review is the only place this can reliably be caught. Confirmed live: several "
                        "retries to fix an unrelated `addErrorBars()` argument mistake left six separate graph "
                        "windows from what was semantically one plotting request, one new window per retry. "
                        "The same principle applies to ANY expensive or side-effecting call earlier in the "
                        "script, not just `plot()` — `Compiler.compile()` is a second confirmed case: a script "
                        "that compiles a custom fit function, then fits/plots/styles it, needed several retries "
                        "to get an unrelated later call right, and `c.compile()` genuinely re-ran (a real "
                        "compile+link of a shared library, not a no-op) on every single one of those retries — "
                        "four full recompiles of the same function for what was semantically one request. "
                        "There's no name-based reuse check available for `compile()` the way there is for "
                        "`newTable()`/`newGraph()`, so the only mitigation is structuring the script so the "
                        "risky/uncertain calls come BEFORE the expensive ones wherever possible, or explicitly "
                        "checking whether the target of an expensive call already exists/succeeded before "
                        "redoing it.\n";
    for (const QString &r : AiAssistant::userRules())
        checklist += "- " + r + "\n";
    return checklist;
}

// Extracts an "Available <word>:\n  name1\n  name2..." block from a runtime error, e.g. the
// dataset-not-found errors QtiSAS's own API methods raise. Used by askAi()'s error-retry path to
// detect when successive retries keep failing against the same fixed set of alternatives —
// meaning the model is guessing different wrong names rather than converging on a real fix.
QString AiAssistant::extractAvailableBlock(const QString &err)
{
    static const QRegularExpression re(R"(Available \w+:\s*((?:\n[ \t]+\S.*)+))");
    const QRegularExpressionMatch m = re.match(err);
    return m.hasMatch() ? m.captured(0).trimmed() : QString();
}

// Strips "at <ai>:N" / "at <ai-explore>:N" location suffixes before the exact-repeat comparison
// below — the line number shifts whenever the surrounding script structure changes (e.g.
// self-review adding or removing a guard before the failing call), so two runs hitting the exact
// same underlying failure can otherwise produce byte-different error text and slip past the
// "identical error repeated" check, extending a loop that should have stopped. Confirmed live:
// the same fit() RuntimeError at line 4 (plain script) then line 11 (same script with an
// existTable()/colNames() guard prepended) was not recognized as a repeat.
QString AiAssistant::normalizeErrorForDedup(const QString &err)
{
    static const QRegularExpression re(R"(\bat <[^>]+>:\d+\b)");
    QString normalized = err;
    normalized.replace(re, "at <script>:N");
    return normalized;
}

// Doc-selection round-trip: the real request
// is held in pendingRequestText until this cheap, ai-doc-only pre-request comes back naming
// which topic docs are actually relevant, so the (much larger) real request(s) below only carry
// those. Substring match against a known topic list rather than strict comma-list parsing —
// robust to the model wrapping its answer in a sentence instead of a bare list. Note "graph3d"
// also contains "graph", so asking for graph3d harmlessly pulls in the graph doc too — acceptable
// over-inclusion, not worth the complexity to avoid. Empty/"all"/anything unrecognized means
// "load everything" — the safe default, never a hard failure.
// Sentinel meaning "genuinely zero topic docs" — distinct from an empty QStringList, which
// filterDocsByTopic() deliberately treats as "ambiguous, fail open to everything" (see its own
// comment). Without this distinction there is no way to express "this turn needs no API
// reference at all" — confirmed live: a long back-and-forth of plain discussion questions
// ("why was background fixed?", "are you sure?", ...) had nothing to match against the known
// topic list, so every single one fell through to "all" (~85K tokens of docs) on top of an
// already-large history, compounding turn over turn until the request exceeded the model's
// context window entirely (HTTP 400) and ended the conversation.
static const QString kNoDocsSentinel = QStringLiteral("__none__");

void AiAssistant::handleDocSelectionResponse(const QString &code)
{
    awaitingDocSelection = false;
    const QString lower = code.toLower();
    if (lower.contains("none"))
    {
        if (m_host)
            m_host->print("▸▸▸ AI doc selection: none");
        startRequest({kNoDocsSentinel});
        return;
    }
    // Order here becomes doc order (after the "ai"/"app" baseline — see below): data
    // "container"/presentation topics first, then the "verb" topics (compile,
    // fittable) last so the one actually driving the final action sits closest to
    // the request itself (same "lost in the middle" rationale as the baseline
    // prepend below). "graph3d" must be checked after "graph" is fine here since both
    // are matched independently against the model's answer text below.
    static const QStringList kKnownTopics = {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable"};
    QStringList selected;
    if (!lower.contains("all"))
        for (const QString &topic : kKnownTopics)
            if (lower.contains(topic))
                selected << topic;
    if (m_host)
        m_host->print("▸▸▸ AI doc selection: " + (selected.isEmpty() ? QString("all") : selected.join(", ")));
    // "ai" (assistant workflow rules) and "app" (newTable/newGraph/newMatrix/newNote/
    // scriptPrint/qti.app — used in nearly every script) always ride along with
    // whatever was actually selected, PREPENDED so they land first in the prompt and
    // the task-specific topic(s) end up closest to the actual request (avoids
    // "lost in the middle" — see filterDocsByTopic()). Guarded on non-empty: an empty
    // list is the "all"/unrecognized-answer fallback, and filterDocsByTopic() treats
    // an empty topic list as "send everything" — adding entries here would wrongly
    // turn that safe fallback into a narrow ai+app-only filter instead.
    startRequest(selected.isEmpty() ? selected : (QStringList{"ai", "app"} + selected));
}

void AiAssistant::startRequest(const QStringList &docTopics)
{
    selectedDocTopics = docTopics;
    // Restate the ORIGINAL request explicitly whenever this turn's own text differs
    // from it (i.e. every short follow-up like "try again"/"noch mal") — see
    // originalRequestText's declaration. Without this, the model tends to
    // continue from its own last SCRIPT in history (e.g. a substitute table already
    // identified as wrong) rather than re-deriving the goal from the real request,
    // which appears only once, much earlier in a growing, code-dominated history.
    const QString requestForModel = (pendingRequestText == originalRequestText)
                                        ? pendingRequestText
                                        : ("Original request for this conversation — the actual goal; do not drift "
                                           "from its explicitly named resources just because this message is short:\n" +
                                           originalRequestText + "\n\nCurrent message:\n" + pendingRequestText);
    ask(requestForModel, true, QString(), docTopics);
}

QString AiAssistant::originalRequestReminder() const
{
    if (pendingRequestText == originalRequestText)
        return {};
    return "Original request for this conversation — the actual goal, don't lose sight of it "
           "even as you explore/retry:\n" +
           originalRequestText + "\n\n";
}

// Two-phase agentic loop: EXPLORE script — run, capture output, feed back to AI.
void AiAssistant::handleExploreResponse(const QString &code)
{
    // Reset unconditionally, even though this flag is normally cleared where the
    // self-review response is consumed below: if self-review itself ever returns
    // an # EXPLORE script instead of a corrected action script, execution returns
    // early from THIS branch and never reaches that reset. Left stale, the next
    // real candidate script (produced after this EXPLORE round via "Now generate
    // the action script.") would be wrongly treated as an already-reviewed
    // self-review result and skip the mandatory review entirely.
    awaitingSelfReview = false;
    if (m_host)
        m_host->print("▸▸▸ AI code (exploring):\n\n" + code);
    if (++autoContinueCount > maxAutoContinue())
    {
        if (m_host)
            m_host->print("▸▸▸ AI: stopped after " + QString::number(maxAutoContinue()) +
                          " automatic EXPLORE/retry rounds — no action script produced yet.");
        return;
    }
    if (!m_host)
        return;
    // Display already happened live during runScript() itself — do NOT print exploreResult.output
    // here too, see that method's doc comment on why that would double every scriptPrint() line.
    const AiScriptResult exploreResult = m_host->runScript(code, "<ai-explore>");
    const QString out = exploreResult.output.isEmpty() ? "(no output)" : exploreResult.output;

    // The model sometimes bundles a complete action script into the same response
    // as EXPLORE (marked by a dedicated "# Action script" comment LINE) instead of
    // keeping the two-phase split intact — that combined script already ran above.
    // Asking for "the action script" again would redundantly re-run the whole
    // thing: confirmed live, it recompiled the same fit function, regenerated the
    // same table, and refit, all from scratch. Skip the follow-up when that marker
    // is present AND this run raised no error — the task is already done. If it
    // DID error, fall through to the normal path below so the failure still gets
    // surfaced.
    // MUST be a dedicated marker line, not a bare substring search — a plain
    // .contains("action script") also matches an EXPLORE script's own unrelated
    // comment merely mentioning the words in passing (confirmed live: "# -- after
    // seeing real output, write the action script:" is a note-to-self inside a
    // pure read-only EXPLORE script, not a bundled action script), which wrongly
    // ended the task right there — twice in a row, including once when the user's
    // new instruction ("select yourself column") was never even attempted.
    static const QRegularExpression actionScriptMarkerRe(R"(^[ \t]*#+[ \t\-]*action script\b)",
                                                         QRegularExpression::CaseInsensitiveOption |
                                                             QRegularExpression::MultilineOption);
    if (exploreResult.error.isEmpty() && actionScriptMarkerRe.match(code).hasMatch())
    {
        m_host->print("▸▸▸ AI: EXPLORE response already included and ran the action script — done.");
        return;
    }

    ask(originalRequestReminder() + "Exploration output:\n```\n" + out + "\n```\nNow generate the action script.", true,
        QString(), selectedDocTopics);
}

// See AiScriptHost::canCompile()'s comment on why the syntax check itself stays host-side rather
// than becoming inline Python-C-API calls here.
bool AiAssistant::maybeDispatchSelfReview(const QString &code, bool &compilable)
{
    compilable = m_host && m_host->canCompile(code);
    const bool isSelfReviewResult = awaitingSelfReview;
    awaitingSelfReview = false;

    if (!compilable || isSelfReviewResult)
        return false;

    // Mandatory pre-execution self-review: send the candidate script back to the
    // AI once, against a short checklist (User Rules + curated silent-failure
    // gotchas), before it ever runs. Runs exactly once per candidate script —
    // the review's own response arrives with isSelfReviewResult == true and goes
    // straight to execution below, so this can't loop.
    if (m_host)
        m_host->print("▸▸▸ AI code (self-review pending):\n\n" + code);
    awaitingSelfReview = true;
    // Scope-reduction check is gated to error-retry corrections ONLY (see
    // isErrorRetryCorrection's declaration) — confirmed live, applying it
    // unconditionally made self-review block a fresh, never-yet-run first attempt
    // by pattern-matching its own stale "this doesn't exist" conclusion from
    // several turns earlier in a long conversation, even after the user had since
    // created the missing table. A first attempt must always be allowed to
    // actually run and produce fresh evidence, not be pre-judged from history.
    const QString scopeCheckPrompt = isErrorRetryCorrection
                                         ? QString("IMPORTANT: compare the script's SCOPE against the original request "
                                                   "restated below — don't rely on recalling it from earlier in the "
                                                   "conversation. Two distinct violations to check for: (1) the script "
                                                   "requests FEWER datasets/parameters/features than the original "
                                                   "request specified (e.g. a global fit across N named datasets "
                                                   "narrowed to fewer because some weren't found); (2) the script "
                                                   "requests the SAME COUNT but a DIFFERENT specific name than what "
                                                   "was explicitly asked for (e.g. the request named table \"AAA\" "
                                                   "column \"BBB\", but the script configures a dataset from a "
                                                   "DIFFERENT table/column just because that one happens to exist — "
                                                   "an \"Available: X\" hint in an error is context for your report, "
                                                   "never implicit permission to substitute X for what was actually "
                                                   "named). Either one is a checklist violation (see the 'silently "
                                                   "drop functionality' item) — REPLACE the entire script with ONLY a "
                                                   "scriptPrint() call naming exactly what's missing and why it's "
                                                   "insufficient. DELETE every other line — configure()/fit()/"
                                                   "setParamValue()/etc. must not remain, not even after the "
                                                   "scriptPrint() call. Confirmed live: a script that kept "
                                                   "scriptPrint() as a warning but left the narrowed configure()/"
                                                   "fit() calls running right after it still hit the underlying "
                                                   "failure, just one step later — adding the print is not enough, "
                                                   "the narrowed attempt itself must be gone. Do NOT raise an "
                                                   "exception either: raising is treated as a bug and triggers "
                                                   "another automatic retry, which would try to 'fix' what is "
                                                   "actually already the correct, final answer (confirmed live: "
                                                   "raising RuntimeError here caused exactly that). The corrected "
                                                   "script must be a single scriptPrint(...) line and nothing else "
                                                   "— that is what ends the script cleanly with nothing further to "
                                                   "retry. EXCEPTION: skip all of the above if the CURRENT message "
                                                   "(restated further below, separately from the original request) "
                                                   "explicitly hands the choice back to you (\"select/choose it "
                                                   "yourself\", \"use whatever's available\", \"pick any\", or "
                                                   "similar) — that authorizes exactly this kind of substitution, so "
                                                   "leave the script as the authorized substitution, don't revert it "
                                                   "to the original name.\n\n")
                                         : QString();
    isErrorRetryCorrection = false;
    // includeDocs=false: this call re-fires once per candidate script, i.e. once
    // more per error-retry — same compounding-context risk as the error-retry
    // call below. Safe to skip here specifically because self-review checks
    // against an explicit checklist already inlined in this prompt, not against
    // the API docs — it isn't generating code from API knowledge.
    // addToHistory=false: this exchange (the full checklist text + candidate
    // script) is pure plumbing whose result is consumed immediately below — it has
    // no narrative value for later turns, and letting it accumulate in m_history
    // (once per candidate script, i.e. potentially every EXPLORE/error-retry round)
    // was confirmed live to eventually make the model latch onto an OLDER script
    // buried in that repeated checklist boilerplate instead of the current one
    // actually placed in front of it a few turns later.
    ask("Before this runs, check it against the checklist below. If it "
        "violates anything, return ONLY the corrected script. If not, "
        "return the SAME script unchanged. Return ONLY executable Python "
        "code — no explanation, no markdown fences.\n\n"
        "IMPORTANT: checklist items (including User Rules) are DEFAULTS, "
        "not absolute overrides. If the ORIGINAL request (earlier in this "
        "conversation) explicitly specified something that contradicts a "
        "checklist item for this exact aspect (e.g. it named a specific "
        "color while a rule sets a different default color), the explicit "
        "request wins — do not force the rule's value over it. Only apply "
        "a checklist item where the original request left that aspect "
        "unspecified.\n\n"
        "IMPORTANT: precedence when the two disagree is CURRENT MESSAGE > "
        "checklist > ORIGINAL REQUEST, not the other way around. The original "
        "request (restated below) is the BASELINE goal, not an immovable "
        "constraint the user can never revise — if the CURRENT message (also "
        "restated below) explicitly authorizes deviating from something the "
        "original request named (e.g. \"select/choose it yourself\", \"use "
        "whatever's available\", \"pick any column\", or similar explicit "
        "hand-off of a specific choice back to you), that authorization wins, "
        "including over the 'don't substitute a different specific name' "
        "checklist item — that item exists to block UNAUTHORIZED silent "
        "substitution, not to override a substitution the user just asked "
        "for in their own words. Confirmed live: after correctly reporting "
        "that column \"ZZ\" didn't exist, the user's very next message said "
        "\"select column yourself\"; the following draft correctly picked a "
        "real column and configured against it, and self-review WRONGLY "
        "reverted that back to the original, already-rejected \"ZZ\" — "
        "undoing the user's explicit authorization by treating the stale "
        "original wording as if it still governed.\n\n"
        "This is a compliance check, not a chance to improve the script. "
        "Only remove or correct what's needed to comply with the checklist "
        "— do NOT add new functionality, styling, or embellishments beyond "
        "what the checklist or the original request actually calls for, "
        "even if it seems like a nice addition (e.g. extra tick/axis "
        "styling for a 'nice' or 'presentation' plot that wasn't otherwise "
        "requested). Any addition you make is itself unreviewed code and "
        "carries the same risk of being wrong as the original draft.\n\n"
        "IMPORTANT: do not re-fix runtime errors here, even if you can see one "
        "earlier in this conversation. This script is the already-corrected "
        "attempt for that error — trust it. If it still contains the mistake, "
        "that will surface again as a new runtime error and go through the "
        "normal error-retry step, not this one. Confirmed live: this step has "
        "returned a DIFFERENT fix attempt than the script it was given (e.g. "
        "toggling a weighting call's dataset argument between an index and a "
        "name across successive reviews, or dropping the feature it couldn't "
        "get right) — that is out of scope here regardless of how it turns "
        "out. If the script below violates none of the checklist items, "
        "return it byte-for-byte unchanged, mistake and all.\n\n" +
            scopeCheckPrompt +
            // Both restated explicitly: the persistent per-conversation anchor
            // (originalRequestText — NOT just pendingRequestText, which
            // is only this turn's literal text, possibly a bare "try again"/
            // "noch mal") AND the current turn's own text, so self-review can
            // tell when the current message explicitly overrides the original —
            // see the precedence note above and originalRequestText's
            // declaration.
            "Original request (baseline goal, not an immovable constraint):\n" + originalRequestText +
            "\n\n"
            "Current message (may explicitly authorize deviating from the "
            "original — see precedence note above):\n" +
            pendingRequestText +
            "\n\n"
            "Script:\n```python\n" +
            code + "\n```\n\nChecklist:\n" + selfReviewChecklist(),
        false, QString(), QStringList(), false);
    return true;
}

// Call after maybeDispatchSelfReview() returns false, passing the `compilable` it computed.
void AiAssistant::runActionScript(const QString &code, bool compilable)
{
    if (!compilable)
    {
        if (m_host)
            m_host->print("▸▸▸ AI code (not runnable):\n\n" + code);
        return;
    }
    if (!m_host)
        return;

    m_host->print("▸▸▸ AI code (running):\n\n" + code);
    // Runs via runScript() (AiScriptHost) — see AiScriptResult's doc comment. The upfront
    // "compilable" check (done by the caller via maybeDispatchSelfReview) is what decides
    // whether to even get here at all — runScript() itself has no such pre-check. Display
    // already happened live during runScript() itself — do NOT print result.output here too,
    // see that method's doc comment on why that would double every scriptPrint() line
    // (confirmed live: exactly this happened).
    const AiScriptResult result = m_host->runScript(code, "<ai>");
    // result.error alone is still the GATE for "did this run actually fail" — deliberately not
    // result.output, since normal fit/print output (e.g. fit progress, Chi2/DoF) also lands
    // there via ApplicationWindow::scriptPrint(), which would make every successful run look
    // like a failure.
    const QString err = result.error;
    if (err.isEmpty())
        return;

    // result.output holds everything printed during this run, in order, INCLUDING the
    // exception text itself — used below instead of the bare exception for retry context and
    // repeat-detection, since the useful diagnostic detail is often in a PRECEDING print, not
    // the exception text. Confirmed live: a compile() failure printed the actual compiler error
    // ("undeclared identifier 'y'") via scriptPrint() before raising a generic "RuntimeError:
    // Compilation failed" — exactly the documented, recommended pattern — and the retry, seeing
    // only the generic exception text, corrected something unrelated instead of the real
    // mistake.
    const QString errContext = result.output.isEmpty() ? err : result.output;
    // Broader, complementary check to the one below: if the error is byte-for-byte identical to
    // the previous retry's, the correction attempt changed nothing at all — confirmed live, an
    // empty-table fit raised the exact same RuntimeError for all 10 retries with the exact same
    // code resubmitted unchanged each time. That error names no specific alternative (unlike
    // the dataset-not-found case below), so the "Available X:" check can't catch it — an exact
    // repeat is its own signal regardless of the error's shape.
    if (normalizeErrorForDedup(errContext) == normalizeErrorForDedup(lastRetryError))
    {
        m_host->print("▸▸▸ AI: stopping — identical error repeated with no change:\n" + errContext +
                      "\n\nFurther retries are unlikely to help without new "
                      "information. Please advise and try again.");
        lastRetryError.clear();
        lastAvailableBlock.clear();
        return;
    }
    lastRetryError = errContext;

    // If the request named a specific dataset/function/column and it genuinely doesn't exist,
    // that IS the correct result of the request — not something to keep guessing past.
    // Detected by the error offering the same fixed set of alternatives twice in a row:
    // confirmed live, the model kept inventing different wrong names (never the one actually
    // offered) for all 10 retries. Stop and hand this back as information rather than
    // continuing to guess, or — worse — ever silently substituting an unrelated available name
    // the user never asked for, which would produce a confident-looking but meaningless result.
    const QString availableBlock = extractAvailableBlock(errContext);
    if (!availableBlock.isEmpty() && availableBlock == lastAvailableBlock)
    {
        m_host->print("▸▸▸ AI: stopping — repeated error against the same fixed "
                      "alternatives:\n" +
                      availableBlock +
                      "\n\nThe requested name doesn't seem to exist. Please confirm "
                      "the correct name (or load/create the data) and try again.");
        lastAvailableBlock.clear();
        return;
    }
    if (!availableBlock.isEmpty())
        lastAvailableBlock = availableBlock;

    if (++autoContinueCount > maxAutoContinue())
    {
        m_host->print("▸▸▸ AI: script failed again after " + QString::number(maxAutoContinue()) +
                      " automatic retries — giving up.\nLast error:\n" + errContext);
        return;
    }

    // includeDocs=false: this call can repeat up to maxAutoContinue() times, and resending the
    // full docs on every retry compounds with the growing history until it blows the model's
    // context window (see ask()'s comment). The model already has a doc-informed script
    // attempt in its own history to correct from.
    // Confirmed live: without explicit direction to use hints the error itself provides,
    // retries have cycled through unrelated syntax variations (or repeated the identical
    // failing line verbatim) while ignoring a specific named value the error already gave —
    // e.g. an error listing "Available datasets: Table1_2" was never tried, even across 10
    // retries in a row.
    // Marks the candidate this produces as an error-retry correction, so the self-review pass
    // below knows to actually apply the scope-reduction check (see self-review's IMPORTANT
    // block) — that check must never fire on a fresh first attempt that hasn't run yet.
    isErrorRetryCorrection = true;
    ask(originalRequestReminder() + "The action script raised an error:\n```\n" + errContext +
            "\n```\nProvide a corrected script. IMPORTANT: if this "
            "error names a specific available option — an exact "
            "dataset/function/column name, a valid range, or "
            "similar (e.g. \"Available datasets: X\") — the fix is "
            "almost always to use that exact value. Do not guess a "
            "different name, vary unrelated call syntax instead, or "
            "repeat the same failing line unchanged.",
        false);
}

static const QString kBasePrompt =
    "You are an assistant for QtiSAS, a scientific data analysis software. "
    "When asked for code, return ONLY the raw executable Python code — no markdown fences, no explanation. "
    "All QtiSAS API functions (newTable, plot, note, Dan, Fittable, etc.) are global — "
    "call them directly without any prefix, namespace, or module qualifier. "
    "Format code with one statement per line and standard 4-space indentation for blocks. "
    "When asked a question, answer concisely in plain text. "
    "You have NO filesystem access — do not pretend to read, write, or list files. "
    "NEVER import QApplication from PyQt5/PyQt6 — it will crash the application.\n\n"
    "If the current message says or implies that something reported missing earlier in this "
    "conversation now exists (e.g. \"try again\", \"it exists now\", \"I created the table\", "
    "\"new table\", or any other phrasing implying the state changed), actually attempt the "
    "operation fresh — call the same API and see what happens. Do not just repeat an earlier "
    "conclusion from conversation history without trying again; trust the user's current "
    "statement about the current state over your own prior turn's conclusion, which may be "
    "stale. Confirmed live: told three times in a row that a table now existed, the reply was "
    "the same canned \"does not exist\" report every time, without a single fresh attempt at "
    "the actual call.\n\n"
    "STRUCTURAL RULE, not dependent on recognizing any particular phrase above: NEVER state that "
    "something doesn't exist / wasn't found by returning a bare scriptPrint() with no actual "
    "verification call in that same script — a \"doesn't exist\" claim is only trustworthy if "
    "THIS response's own code just checked it (existTable(), colNames(), datasets(), etc.), not "
    "because an earlier turn concluded it. If you're about to write a negative-conclusion "
    "scriptPrint() and the script has no real check backing it, add the check instead of trusting "
    "memory — this holds regardless of how the user's message is worded, including short or "
    "ambiguous follow-ups you don't have a specific rule for. Confirmed live: asked to retry after "
    "\"new table\" — a phrasing not covered by the examples above — the reply was the exact same "
    "\"does not exist\" text from three turns earlier, byte-for-byte, generated with no script at "
    "all (not even an existTable() call), twice in a row.\n\n"
    "Before reusing wording from an earlier \"not found\"/\"missing\" report in this conversation, "
    "re-check it against the SPECIFIC error or result in front of you right now — do not paste "
    "that earlier phrasing onto a new situation just because it looks similar. Confirmed live: "
    "after a dataset name was confirmed correct and configure() succeeded, a later fit() failure "
    "for a completely different reason (empty data, not a missing name) was reported using the "
    "exact same \"table not found\" wording from earlier turns — factually wrong for what had "
    "actually just happened.\n\n"
    "When two turns in this conversation give conflicting facts about the SAME thing (e.g. two "
    "different dataset names, because the underlying data changed between them), the MORE RECENT "
    "one wins — never fall back to an older one just because it appeared in an earlier, seemingly "
    "more \"official\" attempt. Confirmed live: immediately after running f.datasets() and getting "
    "back a specific current name, the very next attempt used a DIFFERENT, older name from several "
    "turns earlier instead — predictably failing with the exact list it had just been shown "
    "moments before. If a value was directly confirmed by code output earlier in THIS response "
    "chain, use that value, not one from further back.\n\n";

static const QString kDocsIntro = "QtiSAS Python API documentation is provided below. "
                                  "Use it to answer questions about available classes and functions.\n\n";

// One line per *-python-api doc file, used only for the doc-selection pre-request (see
// AiAssistant::docTopicIndex()) — never sent alongside the full docs themselves. "ai" and "app"
// are deliberately absent: ai-python-api.md is what this very question is sent from (see
// docTopics={"ai"} at the call site), and app-python-api.md (global functions used in nearly
// every script — newTable/newGraph/newMatrix/newNote/scriptPrint/qti.app) is unconditionally
// added back onto whatever the model picks from this list — see ApplicationWindow::askAi().
static const QString kDocTopicIndex =
    "compile — create, edit, and compile NEW fit functions (Compiler class: c = Compiler). FIF "
    "files, C++ code generation, parameters/limits, after-fit hooks.\n"
    "fittable — configure and RUN fits using EXISTING compiled functions (Fittable class: "
    "f = Fittable). Parameters, weighting, resolution, polydispersity, global/batch fits.\n"
    "graph — 2D plots (GraphWindow/Graph/MultiLayer): curves, axes, layers, styling.\n"
    "graph3d — 3D surface/scatter plots (Graph3D/Qti3D).\n"
    "matrix — the Matrix class: 2D numeric grids/image-like data.\n"
    "note — the Note class: text/script notes and tabs.\n"
    "table — the Table class: spreadsheet-like data tables, columns, import/export.\n";

// Splits docs already tagged by loadSystemPrompt() with "\n\n=== filename ===\n" section
// headers, keeping only files whose name contains one of `topics` (case-insensitive substring,
// e.g. "compile" matches "compile-python-api.md"). Empty `topics`, or a filter that matches
// nothing, returns `allDocs` unchanged — this is a cost optimization, never a hard gate, so a
// bad/empty topic list must fail open to the full doc set rather than starving the model.
static QString filterDocsByTopic(const QString &allDocs, const QStringList &topics)
{
    // Explicit "zero docs" request — see kNoDocsSentinel's declaration for why this must be
    // distinguished from the empty-list fail-open case right below it, not folded into it.
    if (topics.size() == 1 && topics.first() == kNoDocsSentinel)
        return {};
    if (topics.isEmpty())
        return allDocs;

    // Parse into (filename, full section text) sections once, in original file order.
    struct Section
    {
        QString fname;
        QString text;
    };
    QList<Section> sections;
    const QString marker = "\n\n=== ";
    qsizetype pos = allDocs.indexOf(marker);
    while (pos >= 0)
    {
        const qsizetype headerEnd = allDocs.indexOf(" ===\n", pos + marker.length());
        if (headerEnd < 0)
            break;
        const QString fname = allDocs.mid(pos + marker.length(), headerEnd - (pos + marker.length()));
        const qsizetype nextPos = allDocs.indexOf(marker, headerEnd);
        const qsizetype sectionEnd = nextPos >= 0 ? nextPos : allDocs.length();
        sections.append({fname, allDocs.mid(pos, sectionEnd - pos)});
        pos = nextPos;
    }

    // Emit matched sections in the ORDER `topics` is given, not disk/alphabetical order. LLMs
    // attend to the start/end of a long prompt more reliably than the middle ("lost in the
    // middle") — the caller lists baseline topics ("ai"/"app") first and the model's own
    // selected, task-specific topics last (see selectedDocTopics' construction in
    // handleDocSelectionResponse()), so the doc content most relevant to THIS request ends up closest to the
    // actual request text that follows the system prompt, instead of wherever it happens to
    // sort alphabetically among unrelated topics.
    QString result;
    QSet<QString> added;
    for (const QString &topic : topics)
        for (const Section &s : sections)
            if (!added.contains(s.fname) && s.fname.contains(topic, Qt::CaseInsensitive))
            {
                result += s.text;
                added.insert(s.fname);
            }
    return result.isEmpty() ? allDocs : result;
}

// Remove <think>...</think> reasoning blocks emitted by DeepSeek-R1/QwQ-style models.
static QString stripThinkBlocks(const QString &s)
{
    QString r = s;
    while (true)
    {
        const auto open = r.indexOf("<think>", 0, Qt::CaseInsensitive);
        if (open < 0)
            break;
        const auto close = r.indexOf("</think>", open, Qt::CaseInsensitive);
        if (close < 0)
        {
            r = r.left(open);
            break;
        }
        r.remove(open, close - open + 8); // 8 = len("</think>")
    }
    return r;
}

// Strip all markdown code fence lines (``` or ```python etc.) from AI responses.
// The text browser already applies Python highlighting, so fences are pure noise.
static QString stripCodeFences(const QString &s)
{
    QStringList out;
    for (const QString &line : s.split('\n'))
        if (!line.trimmed().startsWith("```"))
            out << line;
    return out.join('\n').trimmed();
}

static QString cleanResponse(const QString &s)
{
    return stripCodeFences(stripThinkBlocks(s)).trimmed();
}

AiAssistant::AiAssistant(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)), m_retryTimer(new QTimer(this))
{
    connect(m_nam, &QNetworkAccessManager::finished, this, &AiAssistant::onReplyFinished);
    m_retryTimer->setSingleShot(true);
    // Resends the exact request that hit a transient error — see emitHttpError() in
    // onReplyFinished() for when this fires and why m_lastRequest/m_lastPayload (not a rebuild)
    // are what get resent.
    connect(m_retryTimer, &QTimer::timeout, this, [this]() {
        m_requestTimer.start();
        m_currentReply = m_nam->post(m_lastRequest, m_lastPayload);
    });
}

void AiAssistant::clearHistory()
{
    m_history.clear();
    m_pendingUserText.clear();
    m_cachedDocs.clear();
}

QString AiAssistant::historyText() const
{
    if (m_history.isEmpty())
        return "(no history)";
    QStringList lines;
    for (int i = 0; i < m_history.size(); ++i)
    {
        lines << QString("Q%1: %2").arg(i + 1).arg(m_history[i].first.split('\n').first());
        lines << QString("A%1: %2").arg(i + 1).arg(m_history[i].second.left(120).replace('\n', ' '));
    }
    return lines.join('\n');
}

void AiAssistant::configure(const QString &apiKey, const QString &baseUrl, const QString &model, int maxTokens,
                            DocsMode docsMode)
{
    m_apiKey = apiKey;
    m_baseUrl = baseUrl;
    m_model = model;
    m_maxTokens = maxTokens;
    m_docsMode = docsMode;
    m_cachedDocs.clear(); // reload docs if provider/docs mode changes
}

QString AiAssistant::docTopicIndex()
{
    return kDocTopicIndex;
}

AiAssistant::DocsMode AiAssistant::docsModeFromSettings(const QSettings &settings)
{
    const int raw = settings.value("AI/DocsMode", -1).toInt();
    if (raw == static_cast<int>(DocsMode::Compact) || raw == static_cast<int>(DocsMode::Full) ||
        raw == static_cast<int>(DocsMode::Integrated))
        return static_cast<DocsMode>(raw);
    // No AI/DocsMode key yet — this preset predates the setting; migrate from the AI/Unlimited
    // bool it was actually saved with.
    return settings.value("AI/Unlimited", false).toBool() ? DocsMode::Full : DocsMode::Compact;
}

void AiAssistant::ask(const QString &userText, bool includeDocs, const QString &systemPrompt,
                      const QStringList &docTopics, bool addToHistory)
{
    if (m_busy)
    {
        emit errorOccurred("▸▸▸ AI: previous request still in progress.");
        return;
    }
    m_busy = true;

    // Auto-detect API format from URL: Anthropic Messages API vs OpenAI-compatible
    const bool isAnthropic = m_baseUrl.contains("anthropic.com", Qt::CaseInsensitive);

    QString sysPrompt = kBasePrompt;

    if (systemPrompt.isEmpty())
    {
        // The API docs are tens of thousands of tokens, and every ask() call is a stateless HTTP
        // request that must resend the full system prompt. Callers pass includeDocs=false for the
        // error-retry loop specifically (ApplicationWindow::askAi()) — that's the one call site
        // that can repeat up to m_maxAutoContinue times, and re-attaching the docs on every one of
        // those retries compounds with the growing message history until it blows the model's
        // context window (seen live: "maximum context length" after 2-3 retries on one request).
        // The initial request and the EXPLORE-output-to-action-script call still get full docs —
        // each happens once per request, and the action-script call in particular needs accurate
        // API reference or the model falls back to hallucinated, non-existent methods. docTopics
        // (set by ApplicationWindow::askAi() from a preceding doc-selection round-trip) narrows
        // this to just the relevant files — see filterDocsByTopic().
        //
        // DocsMode::Integrated skips this block entirely — no doc content is ever attached in
        // that mode, on the assumption the model/deployment already has QtiSAS's API knowledge
        // baked in (fine-tuning, an uploaded knowledge base, or a custom server-side system
        // prompt). See DocsMode's declaration and startRequest().
        if (includeDocs && m_docsMode != DocsMode::Integrated)
        {
            if (m_cachedDocs.isEmpty())
            {
                QString status;
                m_cachedDocs = loadSystemPrompt(&status, m_docsMode == DocsMode::Full);
                if (!status.isEmpty())
                    emit infoMessage(status); // shown only on first load
            }
            if (!m_cachedDocs.isEmpty())
                sysPrompt += kDocsIntro + filterDocsByTopic(m_cachedDocs, docTopics);
        }

        sysPrompt += QString("\n\nCurrent auto-continue limit: %1 total automatic EXPLORE/error-retry "
                             "continuations per request (see the rules above).\n")
                         .arg(m_maxAutoContinue);

        const QStringList rules = userRules();
        if (!rules.isEmpty())
        {
            sysPrompt += QString("\n\n## User Rules (%1/%2)\n\n").arg(rules.size()).arg(m_maxUserRules);
            for (const QString &r : rules)
                sysPrompt += "- " + r + "\n";
        }
    }
    else
        sysPrompt += systemPrompt;

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false; // always disable streaming — we read the complete response at once
    body["max_tokens"] = m_maxTokens;

    // Build messages array including recent conversation history
    QJsonArray messages;
    // Cap how much of the growing conversation gets resent — every ask() call is a stateless
    // HTTP request, so without a bound, a long-running conversation (many EXPLORE/self-review/
    // retry rounds) eventually blows the model's context window regardless of includeDocs, once
    // accumulated history alone is large enough. Seen live: an EXPLORE re-entry deep into a
    // conversation, combined with already-large history from prior rounds, hit "maximum context
    // length" even though the small includeDocs=false calls right before it were fine. Same
    // budget regardless of includeDocs — one predictable number beats two behaviors to reason
    // about. m_history itself is never trimmed — only what's sliced into this request; the full
    // record stays available for historyText() ("show history").
    constexpr qint64 kMaxHistoryCharsForRequest = 80000; // ~20k tokens at the ~4 chars/token estimate
    qsizetype start = m_history.size();
    qint64 historyChars = 0;
    for (qsizetype i = m_history.size() - 1; i >= 0; --i)
    {
        const qint64 entryChars = m_history[i].first.size() + m_history[i].second.size();
        if (historyChars > 0 && historyChars + entryChars > kMaxHistoryCharsForRequest)
            break;
        historyChars += entryChars;
        start = i;
    }
    const qsizetype omittedTurns = start;

    if (isAnthropic)
    {
        // Anthropic Messages API: system is a top-level field; wrap in array for prompt caching
        body["system"] = QJsonArray{
            QJsonObject{{"type", "text"}, {"text", sysPrompt}, {"cache_control", QJsonObject{{"type", "ephemeral"}}}}};
        for (qsizetype i = start; i < m_history.size(); ++i)
        {
            messages.append(QJsonObject{{"role", "user"}, {"content", m_history[i].first}});
            messages.append(QJsonObject{{"role", "assistant"}, {"content", m_history[i].second}});
        }
        messages.append(QJsonObject{{"role", "user"}, {"content", userText}});
        body["messages"] = messages;
    }
    else
    {
        // OpenAI-compatible: system is the first entry in the messages array
        messages.append(QJsonValue(QJsonObject{{"role", "system"}, {"content", sysPrompt}}));
        for (qsizetype i = start; i < m_history.size(); ++i)
        {
            messages.append(QJsonObject{{"role", "user"}, {"content", m_history[i].first}});
            messages.append(QJsonObject{{"role", "assistant"}, {"content", m_history[i].second}});
        }
        messages.append(QJsonValue(QJsonObject{{"role", "user"}, {"content", userText}}));
        body["messages"] = messages;
    }

    // Store pending user text to save in history after response
    m_pendingUserText = userText;
    m_pendingAddToHistory = addToHistory;

    QString baseUrl = m_baseUrl;
    while (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    // Not every OpenAI-compatible provider follows OpenAI/Groq's own "base + /v1/chat/completions"
    // convention — confirmed live: Google's Gemini OpenAI-compat layer serves at
    // ".../v1beta/openai/chat/completions" with no extra "/v1/" segment (the "v1beta" already IS
    // its version marker), so blindly appending "/v1/chat/completions" there produces a wrong,
    // 404-ing URL even with an otherwise-clean base URL. If the base URL already ends with the
    // expected suffix, use it as-is instead of appending again; only fall back to the standard
    // append (after stripping a redundant trailing /v1, which some users still add out of habit)
    // when it doesn't.
    QString endpoint;
    const bool alreadyComplete =
        (isAnthropic && baseUrl.endsWith("/v1/messages")) || (!isAnthropic && baseUrl.endsWith("/chat/completions"));
    if (alreadyComplete)
        endpoint = baseUrl;
    else
    {
        while (baseUrl.endsWith("/v1"))
            baseUrl.chop(3);
        endpoint = isAnthropic ? baseUrl + "/v1/messages" : baseUrl + "/v1/chat/completions";
    }

    QNetworkRequest req{QUrl(endpoint)};
    // First request carries the full API docs in the system prompt — allow extra time
    req.setTransferTimeout(m_cachedDocs.isEmpty() ? 60000 : 120000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Force HTTP/1.1 — seen live: Qt's HTTP/2 client mid-stream reset with a bare "HTTP/2
    // protocol error" on these long, single-request-at-a-time AI calls, aborting the whole
    // turn with no retry. This class of requests gains nothing from h2 multiplexing, so just
    // avoid the flaky code path.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    if (isAnthropic)
    {
        req.setRawHeader("x-api-key", m_apiKey.toUtf8());
        req.setRawHeader("anthropic-version", "2023-06-01");
        req.setRawHeader("anthropic-beta", "prompt-caching-2024-07-31");
    }
    else if (!m_apiKey.isEmpty())
    {
        req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    // Rough estimate only (~4 bytes/token is a common heuristic for English/code text) — there's
    // no real tokenizer here. Purpose is to give visibility into request size *before* a "maximum
    // context length" HTTP error, not to be exact (the error itself already reports a precise
    // count, but only after rejecting the request).
    const QString historyPart = omittedTurns > 0
                                    ? QString("%1/%2 history turn(s) (%3 older omitted to stay under budget)")
                                          .arg(m_history.size() - start)
                                          .arg(m_history.size())
                                          .arg(omittedTurns)
                                    : QString("%1 history turn(s)").arg(m_history.size());
    const QString docsPart = docTopics.isEmpty() ? QString() : QString(", docs: %1").arg(docTopics.join(", "));
    // m_lastCallMs is the PREVIOUS call's round-trip time (set in onReplyFinished, just before
    // this one started) — reported here rather than for this call, since this one hasn't
    // happened yet. -1 until the first call in the session completes.
    const double lastCallSec = static_cast<double>(m_lastCallMs) / 1000.0;
    const QString lastCallPart =
        m_lastCallMs >= 0 ? QString(", last call %1sec").arg(lastCallSec, 0, 'f', 1) : QString();
    emit infoMessage(QString("AI: sending ~%1KB (~%2K tokens estimated), %3%4%5")
                         .arg(qRound(static_cast<double>(payload.size()) / 1024.0))
                         .arg(qRound(static_cast<double>(payload.size()) / 4.0 / 1000.0))
                         .arg(historyPart, lastCallPart, docsPart));
    // Stored for a possible transient-error retry (see onReplyFinished()'s emitHttpError) — a
    // fresh, user-initiated call always starts with a clean retry budget, even if a previous,
    // unrelated call had exhausted its own.
    m_lastRequest = req;
    m_lastPayload = payload;
    m_httpRetryCount = 0;
    m_requestTimer.start();
    m_currentReply = m_nam->post(req, payload);
}

void AiAssistant::stop()
{
    // abort() still triggers finished() (with QNetworkReply::OperationCanceledError), so this
    // routes through the normal onReplyFinished()/emitHttpError() path below with no special
    // casing — m_busy resets and the user gets an ordinary "operation canceled" style message.
    if (m_currentReply)
        m_currentReply->abort();
    // During a scheduled retry's backoff delay there IS no QNetworkReply yet (the request hasn't
    // been resent), so the abort() above has nothing to cancel — stop the pending retry directly
    // and reset state manually, since onReplyFinished() (which normally does this) won't fire for
    // a retry that never actually got sent.
    if (m_retryTimer->isActive())
    {
        m_retryTimer->stop();
        m_httpRetryCount = 0;
        m_busy = false;
        m_pendingUserText.clear();
    }
}

void AiAssistant::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply == m_currentReply)
        m_currentReply = nullptr;

    m_busy = false; // reset on any reply (error or success)
    // Recorded regardless of success/failure — latency is meaningful either way, and this is
    // read back as "last call" on the NEXT request's "sending" line (see ask()).
    if (m_requestTimer.isValid())
        m_lastCallMs = m_requestTimer.elapsed();

    // Single point for all HTTP/network error reporting — used by both the
    // Qt-error path (network failure) and the post-streaming HTTP-code path.
    constexpr int kMaxHttpRetries = 3;
    const auto emitHttpError = [&](int code, const QByteArray &body) {
        // 503/502/504 ("server unavailable"/gateway) and 429 (rate limit) are transient —
        // confirmed live against Gemini's free-tier flash model returning 503 "high demand"
        // twice in a row, requiring the user to notice and manually resend the exact same
        // request both times. Auto-retry a bounded number of times with backoff instead; any
        // other code (400, 401, 404, ...) is a real, non-transient problem — fail immediately,
        // same as before.
        const bool retryable = (code == 503 || code == 502 || code == 504 || code == 429);
        if (retryable && m_httpRetryCount < kMaxHttpRetries)
        {
            ++m_httpRetryCount;
            int delayMs = 2000 << (m_httpRetryCount - 1); // 2s, 4s, 8s
            if (code == 429)
            {
                bool ok = false;
                const int raSecs = reply->rawHeader("Retry-After").trimmed().toInt(&ok);
                if (ok && raSecs > 0)
                    delayMs = raSecs * 1000;
            }
            emit infoMessage(QString("AI: HTTP %1 — retrying in %2s (attempt %3/%4)...")
                                 .arg(code)
                                 .arg(delayMs / 1000.0, 0, 'f', 1)
                                 .arg(m_httpRetryCount)
                                 .arg(kMaxHttpRetries));
            m_busy = true;
            m_retryTimer->start(delayMs);
            return;
        }
        m_httpRetryCount = 0;

        m_pendingUserText.clear();
        if (code == 429)
        {
            const QByteArray ra = reply->rawHeader("Retry-After");
            const QString wait = ra.isEmpty() ? QString() : QString(" Retry after %1 s.").arg(ra.trimmed().constData());
            emit errorOccurred("AI: rate limit reached (HTTP 429) — gave up after " + QString::number(kMaxHttpRetries) +
                               " automatic retries." + wait);
        }
        else if (code == 413)
        {
            // Confirmed live: this isn't always "the request is too large" in absolute terms —
            // seen with a Groq key capped at 6000 tokens/minute, where even the smallest possible
            // request (compact docs, no history) already exceeded the account's own per-request
            // token limit and got 413 instead of the 429 a quota-exhaustion case would return.
            // Disabling full docs / clearing history genuinely helps when the request really is
            // large, but can't fix a ceiling that low — worth naming that possibility too.
            emit errorOccurred("AI: request too large (HTTP 413). "
                               "Try disabling \"Send full API docs\" in Preferences → AI or "
                               "clearing history. If it still fails, this provider/key's own "
                               "per-request token limit may be too low for this app's minimum "
                               "request size — check its rate-limit/quota settings.");
        }
        else
        {
            const QString detail =
                body.left(300).trimmed().isEmpty() ? reply->errorString() : QString::fromUtf8(body.left(300).trimmed());
            emit errorOccurred(code > 0 ? QString("AI HTTP %1: %2").arg(code).arg(detail) : detail);
        }
    };

    if (reply->error() != QNetworkReply::NoError)
    {
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emitHttpError(httpCode, reply->readAll());
        return;
    }

    QByteArray raw = reply->readAll();

    // Ollama NDJSON streaming: each line is a JSON object with message.content + done flag
    // Accumulate content from all lines even when stream:false is ignored
    if (raw.contains('\n') && raw.startsWith('{'))
    {
        const QList<QByteArray> lines = raw.split('\n');
        if (lines.size() > 1)
        {
            QString accumulated;
            for (const QByteArray &line : lines)
            {
                if (line.trimmed().isEmpty())
                    continue;
                const QJsonObject obj = QJsonDocument::fromJson(line.trimmed()).object();
                if (obj.contains("message"))
                    accumulated += obj["message"].toObject()["content"].toString();
                else if (obj.contains("choices"))
                    accumulated += obj["choices"].toArray().at(0).toObject()["delta"].toObject()["content"].toString();
            }
            if (!accumulated.isEmpty())
            {
                const QString response = accumulated.trimmed();
                if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
                    m_history.append({m_pendingUserText, response});
                m_pendingUserText.clear();
                emit responseReady(cleanResponse(response));
                return;
            }
        }
    }

    // If server ignored stream:false and sent SSE, accumulate all delta content
    if (raw.startsWith("data:"))
    {
        QString accumulated;
        for (const QByteArray &line : raw.split('\n'))
        {
            const QByteArray trimmed = line.trimmed();
            if (!trimmed.startsWith("data:") || trimmed == "data: [DONE]")
                continue;
            const QJsonObject chunk = QJsonDocument::fromJson(trimmed.mid(5).trimmed()).object();
            if (chunk.contains("choices"))
                accumulated += chunk["choices"].toArray().at(0).toObject()["delta"].toObject()["content"].toString();
            else if (chunk.contains("message"))
                accumulated += chunk["message"].toObject()["content"].toString();
        }
        if (!accumulated.isEmpty())
        {
            const QString response = accumulated.trimmed();
            if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
                m_history.append({m_pendingUserText, response});
            m_pendingUserText.clear();
            emit responseReady(cleanResponse(response));
            return;
        }
    }
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpCode != 200)
    {
        emitHttpError(httpCode, raw);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    const QJsonObject obj = doc.object();

    QString text;

    // Anthropic format: { "content": [{"type":"text","text":"..."}] }
    if (obj.contains("content"))
    {
        const QJsonArray arr = obj["content"].toArray();
        for (const auto &v : arr)
            if (v.toObject()["type"].toString() == "text")
                text += v.toObject()["text"].toString();
    }
    // OpenAI-compatible: { "choices": [{"message":{"content":"..."}}] }
    else if (obj.contains("choices"))
    {
        const QJsonArray arr = obj["choices"].toArray();
        if (!arr.isEmpty())
            text = arr[0].toObject()["message"].toObject()["content"].toString();
    }
    else if (obj.contains("error"))
    {
        m_pendingUserText.clear();
        emit errorOccurred("AI error: " + obj["error"].toObject()["message"].toString());
        return;
    }

    if (text.isEmpty())
    {
        m_pendingUserText.clear();
        emit errorOccurred("AI: empty response.");
    }
    else
    {
        const QString response = text.trimmed();
        if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
            m_history.append({m_pendingUserText, response});
        m_pendingUserText.clear();
        emit responseReady(cleanResponse(response));
    }
}
