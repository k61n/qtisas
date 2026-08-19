/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant implementation
 ******************************************************************************/

#include "AiAssistant.h"

#include "AiAssistantInternal.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimer>

// The config directory NAME userRulesFilePath()/requestTimingLogPath() build their path under —
// defaults to "qtisas" (this app's own identity) so nothing needs to call setConfigDirName() for
// QtiSAS itself; exists so a different host embedding this class isn't stuck writing into
// QtiSAS's own config directory. A plain static local (not a class member) since
// userRulesFilePath()/removeUserRule()/etc. are all static — callable, and called, with no
// AiAssistant instance in existence yet (see e.g. ApplicationWindow::addRule()).
static QString &configDirNameStorage()
{
    static QString name = QStringLiteral("qtisas");
    return name;
}

void AiAssistant::setConfigDirName(const QString &name)
{
    configDirNameStorage() = name;
}

// Shared by userRulesFilePath()/requestTimingLogPath() — this app's own per-OS base config
// directory, previously duplicated identically in both.
static QString aiConfigBasePath()
{
#ifdef Q_OS_WIN
    return QDir::homePath() + "/AppData/Local/" + configDirNameStorage() + "/";
#else
    return QDir::homePath() + "/.config/" + configDirNameStorage() + "/";
#endif
}

QString AiAssistant::userRulesFilePath()
{
    return aiConfigBasePath() + "ai/user-rules.md";
}

QString AiAssistant::requestTimingLogPath()
{
    const QString sasPath = aiConfigBasePath();
    return sasPath + "ai/request-timing.log";
}

void AiAssistant::logRequestTiming(QNetworkReply *reply) const
{
    if (!reply)
        return;

    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QStringList rateLimitParts;
    const auto &headerPairs = reply->rawHeaderPairs();
    for (const auto &pair : headerPairs)
    {
        const QString name = QString::fromLatin1(pair.first);
        if (name.contains("ratelimit", Qt::CaseInsensitive))
            rateLimitParts << name + "=" + QString::fromLatin1(pair.second);
    }

    const QString path = requestTimingLogPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&f);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << ","
        << QString::number(static_cast<double>(m_lastCallMs) / 1000.0, 'f', 2) << "," << m_lastPayload.size() << ","
        << httpCode << "," << (rateLimitParts.isEmpty() ? QString("-") : rateLimitParts.join(";")) << "\n";
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
QString AiAssistant::selfReviewChecklist() const
{
    QString checklist = m_content ? m_content->selfReviewChecklist() : QString();
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
// below — the line number shifts whenever surrounding script structure changes (e.g. self-review
// adding a guard), so two runs of the same underlying failure could otherwise produce
// byte-different error text and slip past the "identical error repeated" check.
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
    const QStringList knownTopics = m_content ? m_content->knownDocTopics() : QStringList();
    QStringList selected;
    if (!lower.contains("all"))
        for (const QString &topic : knownTopics)
            if (lower.contains(topic))
                selected << topic;
    if (m_host)
        m_host->print("▸▸▸ AI doc selection: " + (selected.isEmpty() ? QString("all") : selected.join(", ")));
    // The content provider's own baseline topics (assistant workflow rules, and the handful of
    // functions used in nearly every script) always ride along with whatever was actually
    // selected, PREPENDED so they land first in the prompt and the task-specific topic(s) end up
    // closest to the actual request (avoids "lost in the middle" — see filterDocsByTopic()).
    // Guarded on non-empty: an empty list is the "all"/unrecognized-answer fallback, and
    // filterDocsByTopic() treats an empty topic list as "send everything" — adding entries here
    // would wrongly turn that safe fallback into a narrow baseline-only filter instead.
    startRequest(selected.isEmpty() ? selected
                                    : ((m_content ? m_content->baselineDocTopics() : QStringList()) + selected));
}

// Extracts every quoted name in `requestText` explicitly described as already EXISTING — the
// word "existing" followed by up to 3 other words then a quoted string, e.g. `the existing
// "Sphere" form-factor model` or `existing table "SAXS_data"`. Deliberately keys off this one
// literal word rather than trying to infer "referenced vs. created" from creation-verb heuristics
// (fragile, and this codebase's own confirmed-live incidents all used exactly this phrasing) — a
// request that doesn't use the word "existing" for a name gets no check at all (fail-open by
// construction, favoring zero false positives over full coverage). Host-independent: pure text
// pattern matching, no QtiSAS-specific vocabulary involved.
QStringList extractExistingEntityNames(const QString &requestText)
{
    QStringList names;
    static const QRegularExpression re(R"re(\bexisting\b(?:\s+\S+){0,3}?\s*"([^"]+)")re",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = re.globalMatch(requestText);
    while (it.hasNext())
    {
        const QString name = it.next().captured(1);
        if (!names.contains(name))
            names << name;
    }
    return names;
}

// Returns the subset of `checkedNames` for which `scriptOutput` contains a "MISSING: <name>" line
// — deterministic substring matching against the verification script's own printed output, never
// a semantic interpretation of it. Host-independent: the "EXISTS: "/"MISSING: " prefixes are
// defined by AiContentProvider::existingEntityVerificationPrompt()'s own prompt text, but checking
// for them is plain string work.
QStringList missingEntityNames(const QString &scriptOutput, const QStringList &checkedNames)
{
    QStringList missing;
    for (const QString &name : checkedNames)
        if (scriptOutput.contains("MISSING: " + name))
            missing << name;
    return missing;
}

void AiAssistant::beginRequest(const QString &aiText, bool autoRunValue, bool wantsSplit)
{
    // Per-request state reset — moved here from ApplicationWindow::askAi() so that function stops
    // reaching into AiAssistant's internal fields directly (see this method's header doc comment).
    autoRun = autoRunValue;
    autoContinueCount = 0;
    awaitingSelfReview = false;
    selectedDocTopics.clear();
    lastAvailableBlock.clear();
    lastRetryError.clear();
    isErrorRetryCorrection = false;
    splitModeActive = wantsSplit;
    awaitingSplitPlan = false;
    awaitingSplitMerge = false;
    splitPlan.clear();
    splitPartIndex = -1;
    splitPartScripts.clear();
    pendingRequestText = aiText;
    // Set only on the FIRST request of a conversation — see originalRequestText's declaration for
    // why this must NOT be overwritten by every subsequent short follow-up.
    if (originalRequestText.isEmpty())
        originalRequestText = aiText;

    if (m_content)
    {
        const QStringList existingNames = extractExistingEntityNames(aiText);
        if (!existingNames.isEmpty())
        {
            pendingExistenceCheckRequestText = aiText;
            awaitingExistenceCheckScript = true;
            ask(m_content->existingEntityVerificationPrompt(existingNames), true, QString(), QStringList(), false,
                false);
            return;
        }
    }
    dispatchAfterExistenceCheck(aiText);
}

void AiAssistant::handleExistenceCheckScriptResponse(const QString &code)
{
    awaitingExistenceCheckScript = false;
    const QString aiText = pendingExistenceCheckRequestText;
    if (!m_host)
        return;
    QString syntaxError;
    if (!m_host->canCompile(code, &syntaxError))
    {
        // Fail open — a broken verification script must not become a new way to get permanently
        // stuck. Skip verification entirely and proceed as if nothing needed checking.
        m_host->print("▸▸▸ AI: existence-check script wasn't valid Python — skipping verification, proceeding.");
        dispatchAfterExistenceCheck(aiText);
        return;
    }
    const AiScriptResult result = m_host->runScript(code, "<ai-existence-check>");
    if (!result.error.isEmpty())
    {
        m_host->print("▸▸▸ AI: existence-check script errored — skipping verification, proceeding.");
        dispatchAfterExistenceCheck(aiText);
        return;
    }
    const QStringList checkedNames = extractExistingEntityNames(aiText);
    const QStringList missing = missingEntityNames(result.output, checkedNames);
    if (!missing.isEmpty())
    {
        QStringList quoted;
        for (const QString &n : missing)
            quoted << "\"" + n + "\"";
        m_host->print("▸▸▸ AI: stopping — this request refers to " + quoted.join(", ") + " as already existing, but " +
                      (missing.size() == 1 ? "no match was found for it" : "no match was found for them") +
                      ". Nothing was created or run. Reply with the correct name, or say \"create a new "
                      "one\"/\"pick any\" to explicitly authorize that instead.");
        return;
    }
    dispatchAfterExistenceCheck(aiText);
}

void AiAssistant::dispatchAfterExistenceCheck(const QString &aiText)
{
    if (splitModeActive)
        beginSplitPlanDecomposition(aiText);
    else
        beginNonSplitRequest(aiText);
}

void AiAssistant::beginNonSplitRequest(const QString &aiText)
{
    if (!m_content || docsMode() == DocsMode::Integrated)
    {
        // No doc content is ever attached in Integrated mode regardless of topic, so asking
        // "which topics do you need" would just be a wasted extra round-trip — go straight to the
        // real request. Same fallback as beginSplitPlanDecomposition() when m_content is null.
        awaitingDocSelection = false;
        startRequest(QStringList());
        return;
    }
    awaitingDocSelection = true;
    // includeHistory=false: this prompt already inlines aiText itself and asks nothing about the
    // rest of the conversation — same reasoning as the split-plan preflight's own includeHistory
    // argument.
    ask(m_content->docSelectionPreflightPrompt(aiText), true, QString(), {"ai"}, false, false);
}

void AiAssistant::beginSplitPlanDecomposition(const QString &aiText)
{
    awaitingDocSelection = false;
    if (!m_content)
    {
        // No content provider to phrase either preflight with (shouldn't happen in practice —
        // see setContentProvider()'s call sites) — fall back to a plain request instead of firing
        // ask() with an empty prompt and leaving a flag stuck waiting on nothing.
        startRequest(QStringList());
        return;
    }
    if (m_history.isEmpty())
    {
        // Fresh conversation — nothing in history could conflict with this request, so the
        // continuation-check would just be a wasted round-trip. Unchanged from before this check
        // existed.
        awaitingSplitPlan = true;
        ask(m_content->splitPlanPreflightPrompt(aiText), true, QString(), {"ai"}, false, false);
        return;
    }
    // History exists — could belong to this same ongoing task, or to an earlier, unrelated
    // request (see awaitingContinuationCheck's doc comment for the confirmed-live failure this
    // guards against). Defer the split-plan preflight until handleContinuationCheckResponse()
    // decides which.
    pendingSplitRequestText = aiText;
    awaitingContinuationCheck = true;
    ask(m_content->continuationCheckPreflightPrompt(aiText, originalRequestText), true, QString(), {"ai"}, false,
        false);
}

void AiAssistant::handleContinuationCheckResponse(const QString &code)
{
    awaitingContinuationCheck = false;
    const QString answer = code.trimmed().toUpper();
    if (answer.contains("UNCLEAR"))
    {
        if (m_host)
            m_host->print("▸▸▸ AI: not sure whether this continues the current task or starts a "
                          "new one — please clarify (e.g. \"new task\" or \"continue\").");
        awaitingContinuationClarification = true;
        return;
    }
    if (answer.contains("NEW"))
    {
        clearHistory();
        // Re-anchor to THIS request as the new baseline goal — mirrors what a genuinely fresh
        // askAi() call does (originalRequestText "set once, the first call after history is
        // cleared"); leaving it empty here would lose the anchor for every later self-review/
        // retry prompt that restates it.
        originalRequestText = pendingSplitRequestText;
        if (m_host)
            m_host->print("▸▸▸ AI: this looks like a new, unrelated task — starting fresh "
                          "(previous conversation context cleared).");
    }
    // CONTINUE, or anything unrecognized — fail-open (same philosophy as every other
    // verify-and-retry in this class): proceed exactly as the fresh-conversation path would,
    // history intact.
    if (!m_content)
    {
        startRequest(QStringList());
        return;
    }
    awaitingSplitPlan = true;
    ask(m_content->splitPlanPreflightPrompt(pendingSplitRequestText), true, QString(), {"ai"}, false, false);
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

QString AiAssistant::splitModeGiveUpNote()
{
    if (!splitModeActive)
        return {};
    const QString note = QString("\n\n(split-mode part %1/%2 — later parts were not attempted; "
                                 "merge was never requested.)")
                             .arg(splitPartIndex + 1)
                             .arg(splitPlan.size());
    splitModeActive = false;
    return note;
}

// Parses one "DOCS: ..." field from a split-plan line using the EXACT SAME topic-matching
// semantics as handleDocSelectionResponse() above — kept as a separate helper so split mode
// carries zero risk to that already-tuned path. Also unions in any topic keyword found in the
// part's own description (mechanical backstop against an incomplete/wrong DOCS field), with
// dedicated synonym regexes for topic names that rarely appear verbatim in natural prose:
// "fittable" (the class name itself) and "graph3d" (users say "3D surface"/"3D plot", never the
// literal topic name) — confirmed live via ai-test-requests.md's own "3D surface plot" fixture
// entry, which detected zero topics at all before this regex was added.
static const QHash<QString, QRegularExpression> topicSynonymRe = {
    {"fittable",
     QRegularExpression(R"(\bfit(s|ted|ting)?\b|chi2|chi²|fitcurve)", QRegularExpression::CaseInsensitiveOption)},
    {"graph3d",
     QRegularExpression(R"(\b3d\b|\bsurface\s*plot\b|\bplot3d\b)", QRegularExpression::CaseInsensitiveOption)},
};

QStringList splitPartTopicsFromAnswer(const QString &docsField, const QString &description,
                                      const QStringList &knownTopics)
{
    const QString lower = docsField.toLower();

    const QString descLower = description.toLower();
    QStringList keywordMatches;
    for (const QString &topic : knownTopics)
    {
        if (descLower.contains(topic))
        {
            keywordMatches << topic;
            continue;
        }
        const auto it = topicSynonymRe.constFind(topic);
        if (it != topicSynonymRe.constEnd() && it.value().match(description).hasMatch())
            keywordMatches << topic;
    }

    // "all" means fail-open to the entire doc corpus (see filterDocsByTopic()'s comment: an EMPTY
    // topic list, not a populated one, is what triggers that) — keyword matches would only ever
    // narrow this, never broaden it, so leave it untouched.
    if (lower.contains("all"))
        return {};

    if (lower.contains("none"))
        return keywordMatches.isEmpty() ? QStringList{kNoDocsSentinel} : keywordMatches;

    QStringList selected;
    for (const QString &topic : knownTopics)
        if (lower.contains(topic))
            selected << topic;
    for (const QString &topic : keywordMatches)
        if (!selected.contains(topic))
            selected << topic;
    return selected;
}

// The decomposition prompt already instructs naming every created object explicitly in the
// description of the part that creates it, but that instruction alone has a confirmed miss rate.
// Detects a part whose own description describes creating a named object but never states a
// quoted name for it — a mechanical, cross-checkable signal independent of model self-compliance.
QList<int> splitPartsMissingCreationName(const QList<AiAssistant::SplitPart> &plan)
{
    // "plot" is as much a graph-creation action as newGraph() itself — a part described only as
    // "plot ... in separate panels" (no "create"/"build" verb at all) previously slipped past this
    // check unnamed, then had no way to ever satisfy splitPartScopeViolation()'s Direction 5 (which
    // requires the created window's name to appear in the part's own description): the description
    // never named anything, so no retry could succeed, and the whole part died after two identical,
    // deduped block attempts. "surface" covers graph3d descriptions ("create 3D surface plot of
    // ...") the same way — confirmed live: an unnamed one left a later part with no real name to
    // look up via plot3D("name"), so it just rebuilt the whole surface from scratch instead (see
    // Direction 8, which requires setWindowName() on the code side of this same problem).
    static const QRegularExpression creationVerbRe(
        R"(\b(compile|create|make|build|plot)\b.*\b(function|table|graph|matrix|note|panels?|surface)\b)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression quotedNameRe(R"(['"][^'"]+['"])");
    QList<int> missing;
    for (int i = 0; i < plan.size(); ++i)
        if (creationVerbRe.match(plan[i].description).hasMatch() && !quotedNameRe.match(plan[i].description).hasMatch())
            missing << i;
    return missing;
}

// True if `code`, after stripping comments and blank lines the same way
// AiContentProvider::isGiveUpFallbackScript() does, has NO remaining lines at all — the script
// does absolutely nothing. Confirmed live, severe: self-review's mandatory "ONLY executable
// Python code — no explanation" instruction broke down entirely on one turn — its entire response
// was a verbose checklist-item-by-checklist-item reasoning dump, formatted as "#" comments, with
// zero actual statements. A comments-only file is syntactically valid Python, so it compiled and
// ran as a silent no-op (no exception, nothing happened), and the pipeline treated it as a
// successfully completed part — after which the split-mode merge step, told to use "ONLY the
// literal code given... never substitute... the code below IS what actually ran", had nothing
// real to reproduce and fabricated entirely different, never-executed code from imagination
// instead. Generic/host-independent (no QtiSAS-specific knowledge needed), unlike the other
// mechanical checks — lives here rather than behind AiContentProvider for that reason.
bool isEmptyActionScript(const QString &code)
{
    const auto lines = code.split('\n');

    return std::all_of(lines.cbegin(), lines.cend(), [](const QString &line) {
        const QString trimmed = line.trimmed();
        return trimmed.isEmpty() || trimmed.startsWith('#');
    });
}

void AiAssistant::handleSplitPlanResponse(const QString &code)
{
    awaitingSplitPlan = false;
    splitPlan.clear();
    splitPartScripts.clear();
    splitPartIndex = -1;
    // Set only when THIS response is itself the naming-retry's answer — read once, right before
    // the flag that would otherwise get overwritten by the completeness check further down.
    const bool isNamingRetryResponse = awaitingSplitPlanNamingRetry;
    awaitingSplitPlanNamingRetry = false;

    const QStringList knownTopics = m_content ? m_content->knownDocTopics() : QStringList();
    static const QRegularExpression partRe(R"(^\s*PART\s*:\s*(.+?)\s*\|\s*DOCS\s*:\s*(.+?)\s*$)",
                                           QRegularExpression::CaseInsensitiveOption);
    const QStringList lines = code.split('\n');
    for (const QString &line : lines)
    {
        const QRegularExpressionMatch m = partRe.match(line);
        if (!m.hasMatch())
            continue;
        const QString description = m.captured(1).trimmed();
        splitPlan << SplitPart{description, splitPartTopicsFromAnswer(m.captured(2), description, knownTopics)};
    }

    if (splitPlan.isEmpty())
    {
        // Unparseable plan — a malformed decomposition answer must never block the request
        // outright, but falling back to a single SplitPart with docTopics=QStringList() fails
        // open to the ENTIRE doc corpus (an empty topic list means "attach every doc topic", see
        // filterDocsByTopic()), risking blowing past the model's context window. Abandon split
        // mode entirely instead and reuse the same "which doc topics do you need" pre-flight a
        // plain (non-split) request already uses (ApplicationWindow::askAi()'s docsMode==Selective).
        if (m_host)
            m_host->print("▸▸▸ AI split-plan: unparseable — abandoning split mode, asking which "
                          "doc topics this request needs instead of attaching all of them.\n"
                          "▸▸▸ AI split-plan raw response (for diagnosing the parse failure):\n" +
                          code);
        splitModeActive = false;
        if (m_content)
        {
            awaitingDocSelection = true;
            // Same prompt wording as ApplicationWindow::askAi()'s own doc-selection pre-flight —
            // both now source it from AiContentProvider so AiAssistant.cpp stays free of
            // QtiSAS-specific string literals (see AiContentProvider.h). includeHistory=false:
            // this prompt is a narrow, self-contained "which topics does the request below need"
            // question — it already inlines originalRequestText and asks nothing about the rest
            // of the conversation; same history-bleeding risk and fix as the split-plan preflight
            // it's falling back from.
            ask(m_content->docSelectionPreflightPrompt(originalRequestText), true, QString(), {"ai"}, false, false);
        }
        else
        {
            // No content provider to phrase the doc-selection question with (shouldn't happen
            // in practice — see setContentProvider()'s call sites) — fall back to the pre-fix
            // behavior (single part, full doc corpus) instead of firing ask() with an empty
            // prompt and leaving awaitingDocSelection stuck waiting on nothing.
            startRequest(QStringList());
        }
        return;
    }
    else if (splitPlan.size() > maxSplitParts())
    {
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-plan: %1 parts requested — capping at %2.")
                              .arg(splitPlan.size())
                              .arg(maxSplitParts()));
        while (splitPlan.size() > maxSplitParts())
            splitPlan.removeLast();
    }

    // One-shot verify-and-retry on the decomposition step itself: the prompt already explicitly
    // instructs fixing every created object's name in the part that creates it, but that
    // instruction alone has a confirmed non-trivial miss rate (see
    // splitPartsMissingCreationName()'s comment). Never retries a second time — isNamingRetryResponse
    // guards this whole block so the retry's own answer is accepted regardless of outcome, same
    // fail-open philosophy as every other split-mode parsing fallback here.
    if (!isNamingRetryResponse)
    {
        const QList<int> missing = splitPartsMissingCreationName(splitPlan);
        if (!missing.isEmpty())
        {
            QStringList complaints;
            for (int i : missing)
                complaints << QString("Part %1 (\"%2\") creates something but never states its "
                                      "name in quotes.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description);
            // Reconstructs the plan actually produced, in the same PART/DOCS format it was parsed
            // from, so the retry can show it back to the model instead of merely describing it.
            QStringList planLines;
            for (const SplitPart &p : splitPlan)
                planLines << QString("PART: %1 | DOCS: %2")
                                 .arg(p.description)
                                 .arg(p.docTopics.isEmpty() ? "all" : p.docTopics.join(", "));
            if (m_host)
                m_host->print("▸▸▸ AI split-plan: missing explicit name(s) — asking the model to "
                              "fix and resend the plan once.");
            awaitingSplitPlan = true;
            awaitingSplitPlanNamingRetry = true;
            // This ask() call is a fresh, standalone request — addToHistory=false on the initial
            // decomposition call above means neither the original request nor the plan it produced
            // is otherwise visible here, so both are restated explicitly below rather than relying
            // on history the model can't actually see. includeHistory=false enforces that same
            // intent programmatically — this was previously left at its default (true), an
            // oversight matching the same bug fixed in the initial decomposition call and
            // requestSplitMerge(): restating everything explicitly in the prompt only prevents
            // history-bleeding if history is actually excluded, not just narrated around.
            ask("Original request:\n" + originalRequestText + "\n\nYour previous split plan:\n" + planLines.join("\n") +
                    "\n\nThat plan is missing required explicit names:\n" + complaints.join("\n") +
                    "\n\nEvery part that creates a table, graph, or compiled function MUST state "
                    "its exact name in quotes in its own description (e.g. \"compile custom fit "
                    "function named '<PlaceholderFunction>'\" — a placeholder illustrating the "
                    "FORMAT only, never a name to actually use). Add ONLY the missing name(s) to "
                    "the plan above — do not otherwise change it, and do not invent a different "
                    "scenario. Resend the COMPLETE corrected plan, same PART/DOCS format as "
                    "before, one PART per line, nothing else.",
                true, QString(), {"ai"}, false, false);
            return;
        }
    }

    if (m_host)
    {
        QStringList summary;
        for (int i = 0; i < splitPlan.size(); ++i)
            summary << QString("  %1. %2 [docs: %3]")
                           .arg(i + 1)
                           .arg(splitPlan[i].description)
                           .arg(splitPlan[i].docTopics.isEmpty() ? "all" : splitPlan[i].docTopics.join(", "));
        m_host->print("▸▸▸ AI split-plan (" + QString::number(splitPlan.size()) + " part(s)):\n" + summary.join("\n"));
    }
    startSplitPart(0);
}

void AiAssistant::startSplitPart(int index)
{
    splitPartIndex = index;
    // Each part gets its own fresh per-request state — same reset ApplicationWindow::askAi()
    // does for a brand-new whole request — so a part's retries are judged against the full
    // m_maxAutoContinue budget, not whatever was left over from an earlier part.
    autoContinueCount = 0;
    awaitingSelfReview = false;
    lastAvailableBlock.clear();
    lastRetryError.clear();
    isErrorRetryCorrection = false;

    const SplitPart &part = splitPlan[index];
    // "none"/ambiguous-"all" stay exactly as returned (never get the baseline topics prepended) —
    // same guard as handleDocSelectionResponse()'s own baseline-topic prepend above.
    const bool noneOrAmbiguous = part.docTopics.isEmpty() || part.docTopics == QStringList{kNoDocsSentinel};
    selectedDocTopics = noneOrAmbiguous
                            ? part.docTopics
                            : ((m_content ? m_content->baselineDocTopics() : QStringList()) + part.docTopics);

    QStringList allParts;
    for (int i = 0; i < splitPlan.size(); ++i)
        allParts << QString("%1. %2%3")
                        .arg(i + 1)
                        .arg(splitPlan[i].description)
                        .arg(i == index ? "   <-- THIS PART — generate ONLY this one now"
                                        : (i < index ? "  (already ran successfully)" : ""));

    const QString prompt =
        "Split-mode part " + QString::number(index + 1) + " of " + QString::number(splitPlan.size()) +
        ".\n\nFull plan for context — do not redo an earlier part, do not jump ahead to a later one:\n" +
        allParts.join("\n") + "\n\nOriginal request (baseline goal for the whole plan):\n" + originalRequestText +
        "\n\nGenerate a script for ONLY this part: " + part.description +
        "\n\nThis part's description above is the COMPLETE task for this script — perform it now. "
        "The other parts listed above are shown for context only (what already ran, what comes "
        "next); they are separate, already-decided tasks, never a backup that will additionally "
        "cover THIS part's own job. Do not defer this part's action to \"a later part\" or \"a "
        "separate step\" — no other script will do it. Do not explore or act on a DIFFERENT part's "
        "topic just because it appears in the plan above; stay inside this part's own action." +
        (index > 0 ? "\n\nEarlier part(s) already ran successfully as SEPARATE scripts in this same "
                     "conversation — whatever they created (a table, a compiled function, a graph, "
                     "...) already exists live. Look it up BY NAME (" +
                         (m_content ? m_content->lookupByNameExamples() : QString()) +
                         ", etc.) — do NOT assume any variable from "
                         "an earlier part's script is still bound; only the conversation's PRINTED output "
                         "carries forward between separate script runs, never the Python objects "
                         "themselves (the same rule that already governs EXPLORE rounds)."
                   : QString());
    // includeDocs=true, addToHistory defaults true: this exchange (part description + resulting
    // script) is exactly the kind of turn later parts and the final merge call need to see in
    // history — same as startRequest()'s own initial-request call.
    ask(prompt, true, QString(), selectedDocTopics);
}

void AiAssistant::advanceSplitPart(const QString &code)
{
    splitPartScripts << code;
    const int next = splitPartIndex + 1;
    if (next < splitPlan.size())
    {
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-mode: part %1/%2 done — starting part %3.")
                              .arg(splitPartIndex + 1)
                              .arg(splitPlan.size())
                              .arg(next + 1));
        startSplitPart(next);
        return;
    }
    if (!m_splitMergeEnabled)
    {
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-mode: all %1 part(s) completed — merge step skipped (disabled).")
                              .arg(splitPlan.size()));
        splitModeActive = false; // split-mode conversation state ends here, same as after a merge
        return;
    }
    if (m_host)
        m_host->print(
            QString("▸▸▸ AI split-mode: all %1 part(s) completed — requesting merged script.").arg(splitPlan.size()));
    requestSplitMerge();
}

void AiAssistant::requestSplitMerge()
{
    awaitingSplitMerge = true;
    QStringList numbered;
    for (int i = 0; i < splitPartScripts.size(); ++i)
        numbered
            << QString("# --- Part %1: %2 ---\n%3").arg(i + 1).arg(splitPlan[i].description).arg(splitPartScripts[i]);

    const QString mergePrompt =
        "All parts of this split-mode request have now run successfully as SEPARATE scripts, in "
        "order. Merge them into ONE standalone script that would reproduce the exact same end "
        "result if run alone, from scratch, in a brand-new session with nothing yet created.\n\n"
        "Specifically: collapse every 'look up what an earlier part already created' call "
        "(" +
        (m_content ? m_content->lookupByNameExamples() : QString()) +
        ", ...) back into "
        "DIRECT creation inline — in one merged script there is no 'earlier part' anymore, "
        "everything is created fresh, in sequence, within this single script. Preserve the ORDER "
        "the parts ran in. This is a mechanical merge, not a chance to rewrite behavior — do not "
        "add, remove, or improve any functionality beyond what the parts below actually did. Use "
        "ONLY the literal code given for each part below — never substitute an earlier draft of "
        "that part you may recall from earlier in this conversation, even if it looks more "
        "complete or better styled. The code below IS what actually ran and produced the live "
        "result; anything else, however plausible, did not.\n\n"
        "Return ONLY the merged, executable Python script — no markdown fences, no explanation, "
        "no comments about the merge process itself.\n\n" +
        numbered.join("\n\n");
    // includeDocs=false: every part already used the docs correctly earlier in this
    // conversation; this is a mechanical rewrite of already-correct code, not a task needing
    // fresh API reference. addToHistory=false: one-shot, terminal request — its answer is
    // checked for compilability and shown to the user, never resent or built upon.
    // includeHistory=false: this prompt already inlines every part's own literal code above (the
    // ONLY thing it should reproduce, per its own "never substitute an earlier draft you may
    // recall" instruction) — omitting m_history removes the one thing it could latch onto instead,
    // the same reasoning already applied to every self-review call (see askSelfReviewRound()).
    // Confirmed live, missed here: without this, sending a second, unrelated split-mode request
    // later in the SAME conversation (without an explicit "clear history" in between) let its merge
    // step pull in a PRIOR, different request's own parts instead of only the ones just listed
    // above — "only clear history helps" was the reported symptom.
    ask(mergePrompt, false, QString(), QStringList(), false, false);
}

// Extracts the one significant (non-blank, non-comment) line from a script — used to check
// whether a part's literal give-up fallback (see AiContentProvider::isGiveUpFallbackScript())
// survived into the merged output. Generic string handling, not host-specific — unlike
// RECOGNIZING that a line is a "give up" call in the first place, which lives behind
// AiContentProvider since it depends on this host's own print-function name.
static QString significantLine(const QString &script)
{
    for (const QString &line : script.split('\n'))
    {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#'))
            return trimmed;
    }
    return {};
}

// Detects the merge fabricating entirely new "parts" beyond what actually ran. Confirmed live:
// asked to create a single table (a 1-part plan), the merge instead returned a script carrying
// "# --- Part 2 ... Part 6 ---" markers for an unrequested, never-executed graph+fit workflow —
// despite the merge prompt's own "no comments about the merge process itself" instruction.
// Purely mechanical: requestSplitMerge() defines this exact "# --- Part N: ... ---" marker format
// for the real parts, so any N beyond the real part count is self-incriminating.
QList<int> inventedMergePartNumbers(const QString &code, int actualPartCount)
{
    QList<int> invented;
    static const QRegularExpression partMarkerRe(R"(#\s*-{2,}\s*Part\s+(\d+)\s*:)");
    QRegularExpressionMatchIterator it = partMarkerRe.globalMatch(code);
    while (it.hasNext())
    {
        const int n = it.next().captured(1).toInt();
        if (n > actualPartCount && !invented.contains(n))
            invented << n;
    }
    return invented;
}

// Detects the merge adding an import for a module never referenced anywhere in the literal part
// scripts — a narrower manifestation of the same "merge invents content beyond what actually ran"
// failure as inventedMergePartNumbers() above, just without a numbered-part marker to catch it (a
// single-part merge can still fabricate an import). Confirmed live: a 1-part "restyle graph"
// merge added `from qti.app import graphWindow, scriptPrint` and `from qti import Graph, Table,
// Dataset` — neither module exists anywhere in this host's docs; graphWindow/scriptPrint/Graph/
// Table are global built-ins, never imported. Checked by module name (not the full import line) so
// a legitimate import merely reformatted (e.g. combined onto one line) isn't flagged.
QStringList inventedMergeImports(const QString &code, const QString &partScriptsJoined)
{
    QStringList invented;
    static const QRegularExpression importRe(R"(^[ \t]*(?:import\s+([\w.]+)|from\s+([\w.]+)\s+import))",
                                             QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = importRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString module = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (!module.isEmpty() && !partScriptsJoined.contains(module) && !invented.contains(module))
            invented << module;
    }
    return invented;
}

void AiAssistant::handleSplitMergeResponse(const QString &code)
{
    const bool isMergeRetryResponse = awaitingSplitMergeRetry;
    awaitingSplitMerge = false;
    awaitingSplitMergeRetry = false;
    if (!m_host)
        return;

    // One-shot verify-and-retry, same shape as handleSplitPlanResponse()'s naming-completeness
    // check: confirmed live, the merge step substituted a fabricated, more-complete-looking
    // script for a part whose actual, literal script was just a give-up scriptPrint(...) message
    // — despite the merge prompt already explicitly forbidding exactly this. isMergeRetryResponse
    // guards this block so the retry's own answer is accepted regardless of outcome, same
    // fail-open philosophy as every other split-mode verify-and-retry.
    if (!isMergeRetryResponse && m_content)
    {
        QStringList missingParts;
        for (int i = 0; i < splitPartScripts.size(); ++i)
        {
            if (m_content->isGiveUpFallbackScript(splitPartScripts[i]) &&
                !code.contains(significantLine(splitPartScripts[i])))
                missingParts << QString("Part %1 (\"%2\") actually ran as just:\n%3")
                                    .arg(i + 1)
                                    .arg(splitPlan[i].description)
                                    .arg(significantLine(splitPartScripts[i]));
        }
        const QList<int> inventedParts = inventedMergePartNumbers(code, static_cast<int>(splitPartScripts.size()));
        const QStringList inventedImports = inventedMergeImports(code, splitPartScripts.join("\n\n"));
        if (!missingParts.isEmpty() || !inventedParts.isEmpty() || !inventedImports.isEmpty())
        {
            if (!missingParts.isEmpty())
                m_host->print("▸▸▸ AI split-mode merge: a part's literal give-up script didn't "
                              "survive into the merge — asking the model to redo it once.");
            if (!inventedParts.isEmpty())
            {
                QStringList numbers;
                for (int n : inventedParts)
                    numbers << QString::number(n);
                m_host->print(QString("▸▸▸ AI split-mode merge: the merge invented %1 part(s) "
                                      "(numbered %2) that never existed in the %3-part plan and "
                                      "never ran — asking the model to redo it once.")
                                  .arg(inventedParts.size())
                                  .arg(numbers.join(", "))
                                  .arg(splitPartScripts.size()));
            }
            if (!inventedImports.isEmpty())
                m_host->print(QString("▸▸▸ AI split-mode merge: the merge added import(s) for %1 "
                                      "never used in any part's literal script — asking the model "
                                      "to redo it once.")
                                  .arg(inventedImports.join(", ")));
            QStringList numbered;
            for (int i = 0; i < splitPartScripts.size(); ++i)
                numbered << QString("# --- Part %1: %2 ---\n%3")
                                .arg(i + 1)
                                .arg(splitPlan[i].description)
                                .arg(splitPartScripts[i]);
            QString retryPrompt = "Your merged script broke the merge instructions:\n\n";
            if (!missingParts.isEmpty())
                retryPrompt += "It silently substituted different code for at least one part, "
                               "instead of using the literal script that actually ran:\n" +
                               missingParts.join("\n\n") + "\n\n";
            if (!inventedParts.isEmpty())
            {
                QStringList numbers;
                for (int n : inventedParts)
                    numbers << QString::number(n);
                retryPrompt += QString("It invented %1 additional part(s) (numbered %2) beyond "
                                       "the %3 part(s) listed below. Only those %3 part(s) ever "
                                       "existed in the plan and actually ran — nothing else was "
                                       "requested or executed. Drop the invented part(s) entirely.\n\n")
                                   .arg(inventedParts.size())
                                   .arg(numbers.join(", "))
                                   .arg(splitPartScripts.size());
            }
            if (!inventedImports.isEmpty())
                retryPrompt += "It added import(s) for " + inventedImports.join(", ") +
                               " — none of these modules are used anywhere in the literal part "
                               "scripts below. Drop these imports entirely; do not add anything "
                               "the parts didn't already use.\n\n";
            retryPrompt += "Redo the merge. Use ONLY the literal code given for each part below, "
                           "byte-for-byte — including any part whose real script was just a "
                           "scriptPrint(...) give-up message, even though that looks incomplete. Do "
                           "not invent a more complete version of what a part 'should' have done, "
                           "and do not add any part beyond the ones listed below. Return ONLY the "
                           "merged, executable Python script — no markdown fences, no "
                           "explanation.\n\n" +
                           numbered.join("\n\n");
            awaitingSplitMerge = true;
            awaitingSplitMergeRetry = true;
            ask(retryPrompt, false, QString(), QStringList(), false, false);
            return;
        }
    }

    splitModeActive = false; // split-mode conversation state ends here regardless of outcome
    const bool compilable = m_host->canCompile(code);
    m_host->print(QString("▸▸▸ AI split-mode merged script (%1 — NOT executed, verify before reuse):\n\n")
                      .arg(compilable ? "syntax-checked OK" : "NOT syntactically valid") +
                  code);
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
        {
            QString msg = "▸▸▸ AI: stopped after " + QString::number(maxAutoContinue()) +
                          " automatic EXPLORE/retry rounds — no action script produced yet.";
            msg += splitModeGiveUpNote();
            m_host->print(msg);
        }
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

// Despite every generation/retry prompt instructing "return ONLY executable Python code," a
// response can still open with a natural-language paragraph before the actual code — confirmed
// live: five consecutive "not runnable" retries in a row each began with commentary like "The
// error indicates..."/"Here is the corrected script:" followed by a blank line then valid code,
// with no markdown fence around either part for cleanResponse()'s stripCodeFences() to key off.
// canCompile() then chokes on the PROSE (Python's tokenizer rejecting whatever punctuation it
// hits first — an em-dash, in that transcript), and the model — never told its own commentary is
// what's being checked — spent every subsequent retry inventing unrelated theories ("a Unicode
// arrow character", "a quoting issue", "an en-dash from copy-paste") instead of ever identifying
// the real problem, until the retry budget for that exact repeated error ran out.
// Strips a leading paragraph ONLY if it independently fails to compile on its own (confirmed
// prose, not code — even a lone leading import or comment block DOES compile alone and is left
// untouched) AND the remainder after it compiles cleanly on its own. Both conditions must hold
// before anything is discarded, so a genuine syntax error in real code is never misdiagnosed as
// "just commentary" and silently dropped.
static QString stripLeadingCommentaryParagraphs(const QString &code, AiScriptHost *host)
{
    if (!host)
        return code;
    static const QRegularExpression paragraphBreakRe(R"(\n\s*\n)");
    const QStringList paragraphs = code.split(paragraphBreakRe);
    for (int i = 0; i < paragraphs.size() - 1; ++i)
    {
        if (host->canCompile(paragraphs[i].trimmed()))
            break; // this paragraph is itself valid code (or a comment/import) — stop, keep it
        const QString remainder = paragraphs.mid(i + 1).join("\n\n").trimmed();
        if (!remainder.isEmpty() && host->canCompile(remainder))
            return remainder;
    }
    return code;
}

// See AiScriptHost::canCompile()'s comment on why the syntax check itself stays host-side rather
// than becoming inline Python-C-API calls here.
// Backstop 1's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::isUnverifiedMissingClaim()): self-review fabricated an unverified "missing"
// claim, discarding an already-correct `preReviewDraft`. Sends both scripts back and asks for
// either a real verification call or the original, unchanged.
static QString buildSkepticismRetryPrompt(const QString &code, const QString &preReviewDraft,
                                          const QString &existenceVerificationFunctionExamples)
{
    return "Your last response replaced a working script with this:\n```python\n" + code +
           "\n```\nThat claims something is missing/not found, but nothing in this exact "
           "response actually checked that live (no " +
           existenceVerificationFunctionExamples +
           " call, or similar, appears anywhere in it) — an unverified "
           "negative conclusion like this is not a valid outcome. Here is the script you were "
           "originally asked to review:\n```python\n" +
           preReviewDraft +
           "\n```\nIf, after actually reconsidering, you still believe something referenced in "
           "it is genuinely unavailable, add a REAL verification call and branch on its result "
           "— do not assert it from memory. Otherwise, return the original script above, "
           "corrected only for genuine checklist violations if any. Return ONLY executable "
           "Python code — no explanation, no markdown fences.";
}

// Backstop 2's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::splitPartScopeViolation() against self-review's own output): self-review
// re-introduced a scope violation (`postViolation`'s message) into an already-correct
// `preReviewDraft`. Sends both back and asks for the original, unchanged.
static QString buildScopeRetryPrompt(const QString &postViolation, const QString &preReviewDraft)
{
    return "Your last response added functionality beyond this part's own job:\n\n" + postViolation +
           "\n\nHere is the script you were originally asked to review, which did NOT have "
           "this problem:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if "
           "any — do not add the extra functionality back, and do not perform any other "
           "part's job. Return ONLY executable Python code — no explanation, no markdown "
           "fences.";
}

// Backstop 3's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewFunctionNameSubstitution()): self-review changed the
// configure()/setFunction() model name from what `preReviewDraft` had, with no grounding (no
// EXPLORE output, no docs) to judge whether that change is actually correct.
static QString buildFunctionNameRetryPrompt(const QString &substitutionExplanation, const QString &preReviewDraft)
{
    return "Your last response changed a function/model name with no basis to do so:\n\n" + substitutionExplanation +
           "\n\nHere is the script you were originally asked to review, which used the "
           "correct name:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if "
           "any — do not change the function/model name again unless the current message "
           "explicitly authorizes a different one. Return ONLY executable Python code — no "
           "explanation, no markdown fences.";
}

// Backstop 4's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewFalseIncompletenessClaim()): self-review falsely claimed this
// part's job is incomplete/not performed, while `preReviewDraft` demonstrably contains real
// creation/action work.
static QString buildIncompletenessRetryPrompt(const QString &falseClaimExplanation, const QString &preReviewDraft)
{
    return "Your last response falsely claimed this part's job wasn't done:\n\n" + falseClaimExplanation +
           "\n\nHere is the script you were originally asked to review, which already performs "
           "this part's own described action:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — "
           "judge it SOLELY against this part's own described action, never against a different "
           "or later part's job. Return ONLY executable Python code — no explanation, no markdown "
           "fences.";
}

// Backstop 5's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewLookupToCreationRegression()): self-review replaced a correct
// object lookup with a from-scratch recreation, with no docs/history to judge whether that's
// correct or what constructor signature to use.
static QString buildLookupRegressionRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response replaced a correct object lookup with a from-scratch recreation, "
           "with no basis to do so:\n\n" +
           regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which used the correct "
           "lookup:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — "
           "do not replace the lookup with a recreation again unless the current message "
           "explicitly says the object needs to be created fresh. Return ONLY executable Python "
           "code — no explanation, no markdown fences.";
}

// Backstop 6's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewEnumConstantRegression()): self-review replaced a correct enum-
// namespace constant argument (Qti3D.*, etc.) with something else in the same call, with no basis
// to do so.
static QString buildEnumRegressionRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response replaced a correct enum-namespace constant with something else, "
           "with no basis to do so:\n\n" +
           regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which used the correct "
           "constant:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — "
           "do not replace the constant with a string or a different value again. Return ONLY "
           "executable Python code — no explanation, no markdown fences.";
}

// The scope-reduction-check paragraph prepended to the mandatory self-review prompt — empty
// unless `isErrorRetryCorrection` (see that field's own doc comment on AiAssistant.h). Gated
// rather than unconditional, since applying it to a fresh first attempt risks pattern-matching a
// stale "this doesn't exist" conclusion from earlier in the conversation.
static QString buildScopeCheckPrompt(bool isErrorRetryCorrection)
{
    return isErrorRetryCorrection ? QString("IMPORTANT: compare the script's SCOPE against the original request "
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
}

// The split-mode-specific context paragraph(s) prepended to the self-review prompt — restates
// which part this is, which earlier parts already succeeded live (so "I can't see it was
// created" is never grounds to declare something missing — see the skepticism backstop above,
// which exists because this alone didn't prevent that failure), and the two confirmed-live
// violation shapes to watch for (deferring this part's own job, or performing a different part's
// job). Empty when not in split mode.
static QString buildSplitScopePrompt(bool splitModeActive, int splitPartIndex,
                                     const QList<AiAssistant::SplitPart> &splitPlan)
{
    if (!splitModeActive)
        return {};

    QString earlierPartsNote;
    if (splitPartIndex > 0)
    {
        QStringList earlierDescriptions;
        for (int i = 0; i < splitPartIndex; ++i)
            earlierDescriptions << QString("part %1 (\"%2\")").arg(i + 1).arg(splitPlan[i].description);
        earlierPartsNote = QString("Part(s) 1-%1 of this plan already ran SUCCESSFULLY as separate scripts earlier "
                                   "in this same conversation: %2. Whatever each one created (a table, a compiled "
                                   "function, a graph, ...) already exists live right now — this review has no "
                                   "access to that history (by design), so \"I can't see it was created\" has NO "
                                   "evidential value here, never grounds to declare something missing. If you must "
                                   "doubt something's existence, the ONLY valid basis is a live check THIS exact "
                                   "script just ran and that failed — never the absence of evidence in what you "
                                   "happen to be able to see right now.\n\n")
                               .arg(splitPartIndex)
                               .arg(earlierDescriptions.join("; "));
    }
    return earlierPartsNote +
           QString("IMPORTANT — split-mode: this script is for PART %1 of %2 in a larger plan ONLY: "
                   "\"%3\". Judge it SOLELY against that one action — the 'original request' restated "
                   "further below is background for the WHOLE multi-part plan, not license to defer "
                   "this part's own job to some other part or to a general sense that \"it'll happen "
                   "later\". If this script does not itself perform the action described above — e.g. "
                   "it only inspects/reads something unrelated, or states the real work happens \"in a "
                   "later part\"/\"in a separate step\" — that is a checklist violation regardless of "
                   "whether it runs without error: no other part will additionally do this one's job. "
                   "Confirmed live: exactly this happened for a table-creation part; the table was "
                   "never created for the rest of the conversation as a result.\n\n"
                   "Equally a violation: performing a DIFFERENT part's job — one not yet reached, or "
                   "already completed by an earlier part — in addition to or instead of this part's "
                   "own action, even if it runs without error and produces real, correct-looking "
                   "output. Confirmed live: a part described as \"Compare fit qualities and select "
                   "best model\" instead built the entire final plot — a later, separate part's "
                   "explicit job — and hardcoded the winning model's name rather than using the value "
                   "it had just computed. The next part then generated its own independent version of "
                   "the same plot into the same reused graph window; since addCurve()/insertCurve() "
                   "have no name-based dedup, every curve ended up duplicated.\n\n")
               .arg(splitPartIndex + 1)
               .arg(splitPlan.size())
               .arg(splitPlan[splitPartIndex].description);
}

// Assembles the full mandatory self-review request: the fixed compliance-check instructions
// (byte-for-byte preservation, precedence rules, "don't re-fix runtime errors here") plus the
// two context paragraphs built above, the conversation anchors, the candidate script, and the
// checklist. See maybeDispatchSelfReview()'s doc comment for why this pass never sees the API
// docs, only this checklist.
static QString buildSelfReviewRequestPrompt(const QString &code, const QString &scopeCheckPrompt,
                                            const QString &splitScopePrompt, const QString &originalRequestText,
                                            const QString &pendingRequestText, const QString &checklist)
{
    return "Before this runs, check it against the checklist below. If it "
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
           "IMPORTANT: when you DO find a violation, edit the SMALLEST possible span to fix it — "
           "copy every other line of the script byte-for-byte from the input below, do not "
           "regenerate the whole script from memory. Confirmed live, twice: fixing a missing-Qt-"
           "import violation silently dropped an unrelated 'import random' line that was already "
           "correct and needed a few lines later (NameError one retry later, on a line the "
           "violation being fixed never touched); on a separate turn, fixing an unrelated import "
           "violation silently changed an already-correct, already-tested setWeightingDataset(...) "
           "argument from its fully-qualified table_col name back to a bare column name, "
           "reproducing a ValueError that had already been fixed earlier in the same conversation. "
           "Neither line had anything to do with the violation actually being corrected — touching "
           "them was never authorized by the checklist. Add/remove only what the specific checklist "
           "item you're applying requires; leave every other line exactly as given, whitespace and "
           "all.\n\n"
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
           splitScopePrompt + "Original request (baseline goal, not an immovable constraint):\n" + originalRequestText +
           "\n\n"
           "Current message (may explicitly authorize deviating from the "
           "original — see precedence note above):\n" +
           pendingRequestText +
           "\n\n"
           "Script:\n```python\n" +
           code + "\n```\n\nChecklist:\n" + checklist;
}

void AiAssistant::askSelfReviewRound(const QString &prompt)
{
    ask(prompt, false, QString(), QStringList(), false, false);
}

bool AiAssistant::maybeDispatchSelfReview(QString &code, bool &compilable, QString &syntaxError)
{
    compilable = m_host && m_host->canCompile(code, &syntaxError);
    if (!compilable && m_host)
    {
        const QString trimmed = stripLeadingCommentaryParagraphs(code, m_host);
        if (trimmed != code)
        {
            code = trimmed;
            compilable = m_host->canCompile(code, &syntaxError);
        }
    }
    const bool isSelfReviewResult = awaitingSelfReview;
    awaitingSelfReview = false;

    // The draft this reviews was appended to m_history the instant its HTTP response arrived,
    // before self-review ever ran. If self-review changes the script, that correction must be
    // patched back into history or later ask() calls (including split-mode merge) keep reading
    // the stale pre-review draft. m_history.last() is guaranteed to still be this exact draft's
    // entry here, so it's always safe to overwrite unconditionally.
    // Captured BEFORE the history patch below overwrites it — the skepticism-retry check right
    // after needs to compare self-review's OWN output against what it was actually asked to
    // review, i.e. m_history.last().second still holding the PRE-review draft at this exact point.
    const QString preReviewDraft = (isSelfReviewResult && !m_history.isEmpty()) ? m_history.last().second : QString();

    if (isSelfReviewResult && !m_history.isEmpty())
        m_history.last().second = code;

    // One-shot mechanical backstop: self-review has already been explicitly, repeatedly told
    // never to assert something is missing/not found without a live check in THIS script backing
    // it (see kBasePromptTemplate's "STRUCTURAL RULE" and QtiSasAiContent's splitModeChecklistRules(), both
    // sent on every self-review call) — and confirmed live, still does it anyway: a correct,
    // working fit-comparison script was discarded in favor of a fabricated "table not found"
    // give-up for a table an earlier part had already created and used successfully two turns
    // earlier in the SAME conversation. Prompt wording alone isn't reliable enough here (both of
    // those rules directly describe this exact shape and were violated regardless), so this catches
    // it mechanically instead: self-review turned a normal (non-give-up) draft into a give-up
    // asserting something is missing, with zero verification call anywhere in its own one-line
    // output to back that claim.
    const bool isSkepticismRetryResult = awaitingSelfReviewSkepticismRetry;
    awaitingSelfReviewSkepticismRetry = false;
    if (isSelfReviewResult && !isSkepticismRetryResult && m_content && m_content->isUnverifiedMissingClaim(code) &&
        !m_content->isGiveUpFallbackScript(preReviewDraft))
    {
        if (m_host)
            m_host->print("▸▸▸ AI self-review: claimed something is missing with no live check "
                          "backing it — asking it to reconsider once.");
        awaitingSelfReview = true;
        awaitingSelfReviewSkepticismRetry = true;
        askSelfReviewRound(buildSkepticismRetryPrompt(
            code, preReviewDraft, m_content ? m_content->existenceVerificationFunctionExamples() : QString()));
        return true;
    }

    // A second, distinct one-shot mechanical backstop: self-review's own splitScopePrompt already
    // explicitly warns "Equally a violation: performing a DIFFERENT part's job... in addition to
    // or instead of this part's own action" — and confirmed live, self-review still does exactly
    // that TO ITSELF: given an already-correct, plotting-free fit-comparison draft (already
    // corrected once via runActionScript()'s own scope-violation retry), self-review rewrote it
    // into a much bigger script that ALSO built an entire graph — content the reviewed draft never
    // had. runActionScript()'s splitPartScopeViolation() check DOES catch this (same violation
    // message both times, since it's a pure function of the part's description), but by the time
    // it does, the error-retry/dedup budget treats it as an ordinary stuck-in-a-loop failure and
    // gives up after just 2 identical-shaped attempts — even though the ACTUAL problem is self-
    // review re-introducing the same violation into an already-fixed draft, not the generation step
    // failing to fix it. Catching it here, one step earlier, against the PRE-review draft (which
    // must NOT already have had the same violation, or this would wrongly block legitimate content
    // a retry is still working through) avoids burning that shared budget on self-review's own
    // mistake.
    const bool isScopeRetryResult = awaitingSelfReviewScopeRetry;
    awaitingSelfReviewScopeRetry = false;
    if (isSelfReviewResult && !isScopeRetryResult && splitModeActive && m_content)
    {
        const QString earlierPartsCode = splitPartScripts.join("\n\n");
        const QString postViolation =
            m_content->splitPartScopeViolation(code, splitPlan[splitPartIndex].description, earlierPartsCode);
        if (!postViolation.isEmpty() &&
            m_content->splitPartScopeViolation(preReviewDraft, splitPlan[splitPartIndex].description, earlierPartsCode)
                .isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: added out-of-scope content to an already-correct "
                              "script — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewScopeRetry = true;
            askSelfReviewRound(buildScopeRetryPrompt(postViolation, preReviewDraft));
            return true;
        }
    }

    // A third, distinct one-shot mechanical backstop: self-review has no grounds to judge a
    // configure()/setFunction() model name (see AiContentProvider::selfReviewFunctionNameSubstitution()'s
    // doc comment on why — no history, no docs), yet confirmed live it silently rewrote a correct,
    // EXPLORE-verified name into a plausible-sounding but never-discovered one anyway, guaranteeing
    // a "function not found" error on the very next run.
    const bool isFunctionNameRetryResult = awaitingSelfReviewFunctionNameRetry;
    awaitingSelfReviewFunctionNameRetry = false;
    if (isSelfReviewResult && !isFunctionNameRetryResult && m_content)
    {
        const QString substitution = m_content->selfReviewFunctionNameSubstitution(preReviewDraft, code);
        if (!substitution.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: changed a function/model name with no basis to "
                              "do so — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewFunctionNameRetry = true;
            askSelfReviewRound(buildFunctionNameRetryPrompt(substitution, preReviewDraft));
            return true;
        }
    }

    // A fourth, distinct one-shot mechanical backstop: self-review falsely declared this part's
    // job incomplete/not performed while the pre-review draft demonstrably already did real,
    // substantive work — confirmed live, severe: a part correctly created and filled a table
    // matching its own description; self-review's response, self-contradictorily saying both
    // "this script creates a table" AND "no table creation occurs" in the same message, discarded
    // it by judging it against a LATER part's job (plotting) instead of its own. That false
    // give-up ran clean (no exception) and was accepted as a completed part, so the table was
    // never actually created.
    const bool isIncompletenessRetryResult = awaitingSelfReviewIncompletenessRetry;
    awaitingSelfReviewIncompletenessRetry = false;
    if (isSelfReviewResult && !isIncompletenessRetryResult && m_content)
    {
        const QString falseClaim = m_content->selfReviewFalseIncompletenessClaim(preReviewDraft, code);
        if (!falseClaim.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: falsely claimed this part's job wasn't done — "
                              "asking it to reconsider once.");
            awaitingSelfReview = true;
            awaitingSelfReviewIncompletenessRetry = true;
            askSelfReviewRound(buildIncompletenessRetryPrompt(falseClaim, preReviewDraft));
            return true;
        }
    }

    // A fifth, distinct one-shot mechanical backstop: self-review has no docs or history to judge
    // whether recreating an object it finds looked up via matrix(name)/table(name)/note(name)/
    // graphWindow(name) is correct, or what constructor signature to use — confirmed live, it
    // replaced a correct `m = matrix("SphereMap")` lookup with a from-scratch
    // `m = newMatrix("SphereMap", -3.14159, 3.14159, 50, ...)` recreation using an invented,
    // nonexistent constructor signature, raising TypeError on the very next run.
    const bool isLookupRegressionRetryResult = awaitingSelfReviewLookupRegressionRetry;
    awaitingSelfReviewLookupRegressionRetry = false;
    if (isSelfReviewResult && !isLookupRegressionRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewLookupToCreationRegression(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: replaced a correct object lookup with a "
                              "from-scratch recreation — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewLookupRegressionRetry = true;
            askSelfReviewRound(buildLookupRegressionRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // A sixth, distinct one-shot mechanical backstop: self-review has no basis to replace an
    // already-correct enum-namespace constant (Qti3D.*, etc.) with anything else in the same
    // call — confirmed live, it replaced setPlotStyle(Qti3D.FILLEDMESH) with
    // setPlotStyle("Filled Mesh") (TypeError), and the runtime-error retry that followed "fixed"
    // the crash with a guessed bare int instead of the requested constant — silently wrong
    // output, no error.
    const bool isEnumRegressionRetryResult = awaitingSelfReviewEnumRegressionRetry;
    awaitingSelfReviewEnumRegressionRetry = false;
    if (isSelfReviewResult && !isEnumRegressionRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewEnumConstantRegression(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: replaced a correct enum constant with "
                              "something else — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewEnumRegressionRetry = true;
            askSelfReviewRound(buildEnumRegressionRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // m_selfReviewEnabled off (a user setting — see AiSettings::selfReviewEnabled's doc comment):
    // skip the mandatory review pass entirely, straight to runActionScript(). Checked here rather
    // than at the top of the function since every backstop block above only ever fires on
    // isSelfReviewResult == true — reachable only once a review round was actually dispatched — so
    // this is the one place a bypass is needed; nothing above becomes reachable-but-wrong when
    // self-review is disabled, it simply never gets a chance to run.
    if (!compilable || isSelfReviewResult || !m_selfReviewEnabled)
        return false;

    // Mandatory pre-execution self-review: send the candidate script back to the
    // AI once, against a short checklist (User Rules + curated silent-failure
    // gotchas), before it ever runs. Runs exactly once per candidate script —
    // the review's own response arrives with isSelfReviewResult == true and goes
    // straight to execution below, so this can't loop.
    if (m_host)
        m_host->print("▸▸▸ AI code (self-review pending):\n\n" + code);
    awaitingSelfReview = true;
    const QString scopeCheckPrompt = buildScopeCheckPrompt(isErrorRetryCorrection);
    isErrorRetryCorrection = false;
    const QString splitScopePrompt = buildSplitScopePrompt(splitModeActive, splitPartIndex, splitPlan);
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
    askSelfReviewRound(buildSelfReviewRequestPrompt(code, scopeCheckPrompt, splitScopePrompt, originalRequestText,
                                                    pendingRequestText, selfReviewChecklist()));
    return true;
}

// Call after maybeDispatchSelfReview() returns false, passing the `compilable` it computed.
void AiAssistant::runActionScript(const QString &code, bool compilable, const QString &syntaxError)
{
    if (!m_host)
        return;

    // Recognizing "this looks like a plot" / "this looks like a fit" is host-API-specific
    // knowledge, not generic orchestration — lives behind AiContentProvider, not hardcoded here.
    // See AiContentProvider.h's splitPartScopeViolation() doc comment.
    const QString scopeViolation = (compilable && splitModeActive && m_content)
                                       ? m_content->splitPartScopeViolation(code, splitPlan[splitPartIndex].description,
                                                                            splitPartScripts.join("\n\n"))
                                       : QString();
    // Unconditional (not gated on splitModeActive) — every pattern detectKnownMistakes() checks is
    // a genuine defect regardless of split mode (e.g. a Compiler setCode() self-shadow bug compiles
    // with no error, so nothing else in the pipeline would ever catch it before it silently
    // corrupts a fit) — see AiContentProvider::detectKnownMistakes()'s doc comment for the full
    // family and why this is one call instead of 16.
    const AiKnownMistakeMatch knownMistake =
        (compilable && m_content) ? m_content->detectKnownMistakes(code, originalRequestText) : AiKnownMistakeMatch();
    // Unconditional and host-independent — see isEmptyActionScript()'s doc comment. Confirmed
    // live: self-review's "ONLY executable code" instruction broke down entirely, returning a
    // checklist-reasoning dump formatted as comments instead of any actual script.
    const bool emptyScript = compilable && isEmptyActionScript(code);

    QString errContext;
    if (!compilable)
    {
        // Used to just print "not runnable" and stop here — a permanent, silent dead end with no
        // retry and no give-up message, confirmed live: a stray leftover markdown-fence remnant
        // (e.g. a lone "``" the extractor's fence-stripping didn't recognize as a fence) left in
        // the extracted code produced exactly this, and the conversation never advanced again —
        // not even a status query got anything but the stale "waiting on part N" state. Feeding
        // the real SyntaxError text (from canCompile()'s errorOut) through the SAME retry path a
        // runtime error already uses below gives it a genuine chance at an automatic fix instead.
        m_host->print("▸▸▸ AI code (not runnable):\n\n" + code);
        errContext = "SyntaxError: " + (syntaxError.isEmpty()
                                            ? QString("the script did not compile as valid Python — check for stray "
                                                      "text (e.g. a leftover markdown code-fence marker), an "
                                                      "incomplete statement, or mismatched brackets/quotes.")
                                            : syntaxError);
        // This candidate is garbage from history's perspective — not valid Python, sometimes an
        // extended rambling non-code response instead of a small mistake. Keeping it verbatim in
        // history risks the next retry building on top of its own confused prior "turn" instead
        // of producing clean code — confirmed live: a "not runnable" response full of hedging,
        // self-contradicting prose (hallucinating a nonexistent API, then chasing a phantom
        // Unicode-character bug that was actually in its OWN commentary) led directly into an
        // even more confused follow-up attempt with the identical fabricated error, because the
        // rambling text was still sitting in history for the model to keep building on. Replacing
        // it with a short placeholder mirrors maybeDispatchSelfReview()'s analogous history-patch
        // for a stale pre-review draft (same file, above) — m_history.last() is guaranteed to
        // still be this exact candidate's entry, since nothing else appends between the HTTP
        // response arriving and this point.
        if (!m_history.isEmpty())
            m_history.last().second = "(invalid response — not valid Python, omitted)";
    }
    else if (!scopeViolation.isEmpty())
    {
        // Blocked BEFORE running — unlike a runtime error, the violation is knowable up front
        // from the code and this part's own description alone, and running it first would mean
        // e.g. an out-of-scope newGraph() call already creating a real, duplicate window before
        // the retry below even fires.
        m_host->print("▸▸▸ AI code (blocked — out of scope for this part):\n\n" + code);
        errContext = scopeViolation;
    }
    else if (!knownMistake.explanation.isEmpty())
    {
        // Blocked BEFORE running, same reasoning as the scope-violation branch above — see
        // AiContentProvider::detectKnownMistakes()'s doc comment for the full family of patterns
        // checked here, each with its own "confirmed live" incident writeup in QtiSasAiContent.cpp.
        m_host->print("▸▸▸ AI code (blocked — " + knownMistake.header + "):\n\n" + code);
        errContext = knownMistake.explanation;
    }
    else if (emptyScript)
    {
        // Blocked BEFORE running — an all-comments script is a silent no-op, not a completed
        // action; running it would report "success" (no exception) for a part that did nothing.
        m_host->print("▸▸▸ AI code (blocked — no executable code, comments only):\n\n" + code);
        errContext =
            "This script contains no executable code at all — only comments (possibly an "
            "explanation of your own reasoning instead of an actual answer). Return ONLY "
            "executable Python code that actually performs" +
            (splitModeActive ? " this part's own described action: \"" + splitPlan[splitPartIndex].description + "\""
                             : QString(" what was requested")) +
            " — no explanation, no reasoning, no markdown fences, and never just comments.";
    }
    else
    {
        m_host->print("▸▸▸ AI code (running):\n\n" + code);
        // Runs via runScript() (AiScriptHost) — see AiScriptResult's doc comment. Display already
        // happened live during runScript() itself — do NOT print result.output here too, see
        // that method's doc comment on why that would double every scriptPrint() line (confirmed
        // live: exactly this happened).
        const AiScriptResult result = m_host->runScript(code, "<ai>");
        // result.error alone is still the GATE for "did this run actually fail" — deliberately
        // not result.output, since normal fit/print output (e.g. fit progress, Chi2/DoF) also
        // lands there via ApplicationWindow::scriptPrint(), which would make every successful
        // run look like a failure.
        if (result.error.isEmpty())
        {
            if (splitModeActive)
                advanceSplitPart(code);
            return;
        }
        // result.output holds everything printed during the run, INCLUDING the exception text —
        // used below instead of the bare exception for retry context and repeat-detection, since
        // the useful diagnostic detail is often in a PRECEDING print, not the exception text itself.
        errContext = result.output.isEmpty() ? result.error : result.output;
    }
    // Broader, complementary check to the one below: if the error is byte-for-byte identical to
    // the previous retry's, the correction attempt changed nothing at all — confirmed live, an
    // empty-table fit raised the exact same RuntimeError for all 10 retries with the exact same
    // code resubmitted unchanged each time. That error names no specific alternative (unlike
    // the dataset-not-found case below), so the "Available X:" check can't catch it — an exact
    // repeat is its own signal regardless of the error's shape.
    if (normalizeErrorForDedup(errContext) == normalizeErrorForDedup(lastRetryError))
    {
        QString msg = "▸▸▸ AI: stopping — identical error repeated with no change:\n" + errContext +
                      "\n\nFurther retries are unlikely to help without new "
                      "information. Please advise and try again.";
        msg += splitModeGiveUpNote();
        m_host->print(msg);
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
        QString msg = "▸▸▸ AI: stopping — repeated error against the same fixed "
                      "alternatives:\n" +
                      availableBlock +
                      "\n\nThe requested name doesn't seem to exist. Please confirm "
                      "the correct name (or load/create the data) and try again.";
        msg += splitModeGiveUpNote();
        m_host->print(msg);
        lastAvailableBlock.clear();
        return;
    }
    if (!availableBlock.isEmpty())
        lastAvailableBlock = availableBlock;

    if (++autoContinueCount > maxAutoContinue())
    {
        QString msg = "▸▸▸ AI: script failed again after " + QString::number(maxAutoContinue()) +
                      " automatic retries — giving up.\nLast error:\n" + errContext;
        msg += splitModeGiveUpNote();
        m_host->print(msg);
        return;
    }

    // includeDocs=false: this call can repeat up to maxAutoContinue() times, and resending full
    // docs on every retry would compound with growing history until it blows the context window.
    // The model already has a doc-informed script attempt in its own history to correct from.

    // Marks the candidate as an error-retry correction so the self-review scope-reduction check
    // fires (see self-review's IMPORTANT block) — gated on the error actually naming a concrete
    // alternative ("Available: X"), never on a plain coding mistake unrelated to name substitution.
    isErrorRetryCorrection = errContext.contains("available", Qt::CaseInsensitive);
    // Re-grounds a retry against a KNOWN, recurring failure signature — see
    // AiContentProvider::retryHintForError()'s doc comment. Empty (the common case) when
    // errContext matches nothing known.
    const QString retryHint = m_content ? m_content->retryHintForError(errContext) : QString();
    const QString creationCallExamples = m_content ? m_content->creationCallExamples() : QString();
    const QString lookupExample = m_content ? m_content->lookupByNameExamples().section(',', 0, 0) : QString();
    ask(retryHint + originalRequestReminder() + "The action script raised an error:\n```\n" + errContext +
            "\n```\nProvide a corrected script. IMPORTANT: if this "
            "error names a specific available option — an exact "
            "dataset/function/column name, a valid range, or "
            "similar (e.g. \"Available datasets: X\") — the fix is "
            "almost always to use that exact value. Do not guess a "
            "different name, vary unrelated call syntax instead, or "
            "repeat the same failing line unchanged. IMPORTANT: if the output above shows this "
            "script already executed some lines successfully before the error, do not blindly "
            "rewrite the WHOLE script from scratch — an earlier successful side-effecting call "
            "(" +
            creationCallExamples +
            ", ...) already took effect live "
            "and does not need to run again; check whether its target already exists (e.g. " +
            lookupExample +
            " is not None) before repeating it, or restructure so the fix comes "
            "after what already succeeded instead of duplicating it.",
        false);
}

// "%1" is filled in by AiContentProvider::apiCallingConventionHint() at request time (see ask())
// — the one piece of concrete API knowledge this otherwise fully generic prompt needs, kept out of
// this literal so AiAssistant.cpp never has to name a single QtiSAS function itself. "%2"/"%3"/"%4"
// are filled the same way, by assistantIdentityIntro()/existenceVerificationFunctionExamples()/
// noOpPrintFunctionName() respectively — see ask()'s construction of sysPrompt below.
static const QString kBasePromptTemplate =
    "%2"
    "When asked for code, return ONLY the raw executable Python code — no markdown fences, no explanation. "
    "%1"
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
    "something doesn't exist / wasn't found by returning a bare %4 with no actual "
    "verification call in that same script — a \"doesn't exist\" claim is only trustworthy if "
    "THIS response's own code just checked it (%3, etc.), not "
    "because an earlier turn concluded it. If you're about to write a negative-conclusion "
    "%4 and the script has no real check backing it, add the check instead of trusting "
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

// Splits docs already tagged by AiContentProvider::loadDocs() with "\n\n=== filename ===\n" section
// headers, keeping only files whose name contains one of `topics` (case-insensitive substring,
// e.g. "compile" matches "compile-python-api.md"). Empty `topics`, or a filter that matches
// nothing, returns `allDocs` unchanged — this is a cost optimization, never a hard gate, so a
// bad/empty topic list must fail open to the full doc set rather than starving the model.
QString filterDocsByTopic(const QString &allDocs, const QStringList &topics)
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
    const QString &marker = kDocSectionMarkerPrefix;
    qsizetype pos = allDocs.indexOf(marker);
    while (pos >= 0)
    {
        const qsizetype headerEnd = allDocs.indexOf(kDocSectionMarkerSuffix, pos + marker.length());
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

QString AiAssistant::docTopicIndex() const
{
    return m_content ? m_content->docTopicIndex() : QString();
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

bool AiAssistant::parseSplitMarker(const QString &text, bool splitByDefault, QString *effectiveTextOut)
{
    // "NOSPLIT:" is the mirror-image escape hatch for when splitByDefault is ON: forces this ONE
    // request to skip split mode (e.g. a plain question that shouldn't pay for the split-plan
    // pre-flight or have autoRun forced on) regardless of the default. Checked first since its
    // match would otherwise also need excluding from splitMarkerRe (its own leading "split:"
    // substring isn't at position 0, so no actual regex clash, but checking NOSPLIT first keeps
    // the precedence obvious).
    static const QRegularExpression noSplitMarkerRe(R"(^\s*no\s*split\s*:\s*)",
                                                    QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression splitMarkerRe(R"(^\s*split\s*:\s*)", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch noSplitMatch = noSplitMarkerRe.match(text);
    const QRegularExpressionMatch splitMatch =
        noSplitMatch.hasMatch() ? QRegularExpressionMatch() : splitMarkerRe.match(text);
    if (effectiveTextOut)
        *effectiveTextOut = noSplitMatch.hasMatch() ? text.mid(noSplitMatch.capturedLength())
                            : splitMatch.hasMatch() ? text.mid(splitMatch.capturedLength())
                                                    : text;
    return noSplitMatch.hasMatch() ? false : (splitByDefault || splitMatch.hasMatch());
}

bool AiAssistant::looksLikeNewTaskReply(const QString &reply)
{
    static const QRegularExpression newRe(R"(\b(new|different|unrelated|separate)\b)",
                                          QRegularExpression::CaseInsensitiveOption);
    return newRe.match(reply).hasMatch();
}

QString AiAssistant::buildSystemPrompt(const QString &systemPrompt, bool includeDocs, const QStringList &docTopics)
{
    QString sysPrompt = kBasePromptTemplate.arg(m_content ? m_content->apiCallingConventionHint() : QString())
                            .arg(m_content ? m_content->assistantIdentityIntro() : QString())
                            .arg(m_content ? m_content->existenceVerificationFunctionExamples() : QString())
                            .arg(m_content ? m_content->noOpPrintFunctionName() : QString());

    if (!systemPrompt.isEmpty())
        return sysPrompt + systemPrompt;

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
        if (m_cachedDocs.isEmpty() && m_content)
        {
            QString status;
            m_cachedDocs = m_content->loadDocs(m_docsMode == DocsMode::Full, &status);
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
    return sysPrompt;
}

qsizetype AiAssistant::windowedHistoryStart(bool includeHistory) const
{
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
    if (!includeHistory)
        // start stays at m_history.size() — no prior turns resent, see includeHistory's doc
        // comment on ask() for why self-review specifically opts out of this.
        return start;

    qint64 historyChars = 0;
    for (qsizetype i = m_history.size() - 1; i >= 0; --i)
    {
        const qint64 entryChars = m_history[i].first.size() + m_history[i].second.size();
        if (historyChars > 0 && historyChars + entryChars > kMaxHistoryCharsForRequest)
            break;
        historyChars += entryChars;
        start = i;
    }
    return start;
}

// Not every OpenAI-compatible provider follows OpenAI/Groq's own "base + /v1/chat/completions"
// convention — confirmed live: Google's Gemini OpenAI-compat layer serves at
// ".../v1beta/openai/chat/completions" with no extra "/v1/" segment (the "v1beta" already IS
// its version marker), so blindly appending "/v1/chat/completions" there produces a wrong,
// 404-ing URL even with an otherwise-clean base URL. If the base URL already ends with the
// expected suffix, use it as-is instead of appending again; only fall back to the standard
// append (after stripping a redundant trailing /v1, which some users still add out of habit)
// when it doesn't.
static QString resolveEndpoint(QString baseUrl, bool isAnthropic)
{
    while (baseUrl.endsWith('/'))
        baseUrl.chop(1);
    const bool alreadyComplete =
        (isAnthropic && baseUrl.endsWith("/v1/messages")) || (!isAnthropic && baseUrl.endsWith("/chat/completions"));
    if (alreadyComplete)
        return baseUrl;
    while (baseUrl.endsWith("/v1"))
        baseUrl.chop(3);
    return isAnthropic ? baseUrl + "/v1/messages" : baseUrl + "/v1/chat/completions";
}

QString AiAssistant::buildSendingDiagnostic(qint64 payloadSize, qsizetype historyStart,
                                            const QStringList &docTopics) const
{
    // Rough estimate only (~4 bytes/token is a common heuristic for English/code text) — there's
    // no real tokenizer here. Purpose is to give visibility into request size *before* a "maximum
    // context length" HTTP error, not to be exact (the error itself already reports a precise
    // count, but only after rejecting the request).
    const QString historyPart = historyStart > 0
                                    ? QString("%1/%2 history turn(s) (%3 older omitted to stay under budget)")
                                          .arg(m_history.size() - historyStart)
                                          .arg(m_history.size())
                                          .arg(historyStart)
                                    : QString("%1 history turn(s)").arg(m_history.size());
    const QString docsPart = docTopics.isEmpty() ? QString() : QString(", docs: %1").arg(docTopics.join(", "));
    // m_lastCallMs is the PREVIOUS call's round-trip time (set in onReplyFinished, just before
    // this one started) — reported here rather than for this call, since this one hasn't
    // happened yet. -1 until the first call in the session completes.
    const double lastCallSec = static_cast<double>(m_lastCallMs) / 1000.0;
    const QString lastCallPart =
        m_lastCallMs >= 0 ? QString(", last call %1sec").arg(lastCallSec, 0, 'f', 1) : QString();
    return QString("AI: sending ~%1KB (~%2K tokens estimated), %3%4%5")
        .arg(qRound(static_cast<double>(payloadSize) / 1024.0))
        .arg(qRound(static_cast<double>(payloadSize) / 4.0 / 1000.0))
        .arg(historyPart, lastCallPart, docsPart);
}

void AiAssistant::ask(const QString &userText, bool includeDocs, const QString &systemPrompt,
                      const QStringList &docTopics, bool addToHistory, bool includeHistory)
{
    if (m_busy)
    {
        emit errorOccurred("▸▸▸ AI: previous request still in progress.");
        return;
    }
    m_busy = true;

    // Auto-detect API format from URL: Anthropic Messages API vs OpenAI-compatible
    const bool isAnthropic = m_baseUrl.contains("anthropic.com", Qt::CaseInsensitive);

    const QString sysPrompt = buildSystemPrompt(systemPrompt, includeDocs, docTopics);

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false; // always disable streaming — we read the complete response at once
    body["max_tokens"] = m_maxTokens;

    // Build messages array including recent conversation history
    QJsonArray messages;
    const qsizetype start = windowedHistoryStart(includeHistory);
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

    const QString endpoint = resolveEndpoint(m_baseUrl, isAnthropic);

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
    emit infoMessage(buildSendingDiagnostic(payload.size(), omittedTurns, docTopics));
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

// Ollama NDJSON streaming: each line is a JSON object with message.content + done flag.
// Accumulate content from all lines even when stream:false is ignored. Returns the accumulated
// text, or an empty string if `raw` doesn't look like this shape.
static QString parseOllamaNdjson(const QByteArray &raw)
{
    if (!raw.contains('\n') || !raw.startsWith('{'))
        return {};
    const QList<QByteArray> lines = raw.split('\n');
    if (lines.size() <= 1)
        return {};
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
    return accumulated;
}

// If server ignored stream:false and sent SSE, accumulate all delta content. Returns the
// accumulated text, or an empty string if `raw` doesn't look like an SSE stream.
static QString parseSse(const QByteArray &raw)
{
    if (!raw.startsWith("data:"))
        return {};
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
    return accumulated;
}

// Anthropic format: { "content": [{"type":"text","text":"..."}] }. Caller checks
// obj.contains("content") first — this only extracts, never decides the branch.
static QString parseAnthropicResponse(const QJsonObject &obj)
{
    QString text;
    const QJsonArray arr = obj["content"].toArray();
    for (const auto &v : arr)
        if (v.toObject()["type"].toString() == "text")
            text += v.toObject()["text"].toString();
    return text;
}

// OpenAI-compatible format: { "choices": [{"message":{"content":"..."}}] }. Caller checks
// obj.contains("choices") first — this only extracts, never decides the branch.
static QString parseOpenAiResponse(const QJsonObject &obj)
{
    const QJsonArray arr = obj["choices"].toArray();
    return arr.isEmpty() ? QString() : arr[0].toObject()["message"].toObject()["content"].toString();
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
    logRequestTiming(reply); // one line per reply, including transient failures about to be retried

    // Single point for all HTTP/network error reporting — used by both the
    // Qt-error path (network failure) and the post-streaming HTTP-code path.
    constexpr int kMaxHttpRetries = 3;
    // Built from the content provider's own topic list rather than hardcoded, so this generic
    // HTTP-remediation text never has to name QtiSAS doc topics itself. Capped to a handful for a
    // readable inline example — the existing "..." after it already signals "not exhaustive".
    const QString multiTopicExample = (m_content && !m_content->knownDocTopics().isEmpty())
                                          ? m_content->knownDocTopics().mid(0, 4).join(" + ")
                                          : QStringLiteral("several unrelated topics");
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
                               " automatic retries." + wait + splitModeGiveUpNote());
        }
        else if (code == 413)
        {
            // Not always "too large" in absolute terms — a provider/key's own per-request token
            // limit can be low enough that even the smallest possible request exceeds it, so the
            // message below also names that possibility rather than only the size-reduction fixes.
            emit errorOccurred("AI: request too large (HTTP 413). "
                               "Try disabling \"Send full API docs\" in Preferences → AI, "
                               "clearing history, or — if this is a compound request touching "
                               "several topics (" +
                               multiTopicExample +
                               ", ...) — "
                               "prefixing it with \"SPLIT:\" so each part only loads the docs it "
                               "actually needs instead of all of them at once. If it still fails, "
                               "this provider/key's own per-request token limit may be too low for "
                               "this app's minimum request size — check its rate-limit/quota settings." +
                               splitModeGiveUpNote());
        }
        else if (code == 400 && QString::fromUtf8(body).contains("context length", Qt::CaseInsensitive))
        {
            // Unlike 413, this isn't caught by any preflight check — docs are re-embedded in full
            // on every turn and only m_history is size-bounded, so a long conversation's total
            // payload can silently grow past the context window with no warning until rejected here.
            emit errorOccurred("AI: prompt exceeds this model's context window (HTTP 400). The "
                               "conversation has grown too large — try disabling \"Send full API docs\" "
                               "in Preferences -> AI (switches to the much smaller compact reference), "
                               "clearing history / starting a new conversation, or — if this request "
                               "touches several topics at once (" +
                               multiTopicExample +
                               ", "
                               "...) — prefixing it with \"SPLIT:\" so each part only loads the docs "
                               "it actually needs instead of attaching all of them together in one shot." +
                               splitModeGiveUpNote());
        }
        else
        {
            const QString detail =
                body.left(300).trimmed().isEmpty() ? reply->errorString() : QString::fromUtf8(body.left(300).trimmed());
            emit errorOccurred((code > 0 ? QString("AI HTTP %1: %2").arg(code).arg(detail) : detail) +
                               splitModeGiveUpNote());
        }
    };

    if (reply->error() != QNetworkReply::NoError)
    {
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emitHttpError(httpCode, reply->readAll());
        return;
    }

    QByteArray raw = reply->readAll();

    // Some providers ignore stream:false — accumulate their own streaming shapes if present.
    QString streamedAccumulated = parseOllamaNdjson(raw);
    if (streamedAccumulated.isEmpty())
        streamedAccumulated = parseSse(raw);
    if (!streamedAccumulated.isEmpty())
    {
        const QString response = streamedAccumulated.trimmed();
        if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
            m_history.append({m_pendingUserText, response});
        m_pendingUserText.clear();
        emit responseReady(cleanResponse(response));
        return;
    }

    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpCode != 200)
    {
        emitHttpError(httpCode, raw);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    const QJsonObject obj = doc.object();

    QString text;

    if (obj.contains("content"))
        text = parseAnthropicResponse(obj);
    else if (obj.contains("choices"))
        text = parseOpenAiResponse(obj);
    else if (obj.contains("error"))
    {
        m_pendingUserText.clear();
        emit errorOccurred("AI error: " + obj["error"].toObject()["message"].toString() + splitModeGiveUpNote());
        return;
    }

    if (text.isEmpty())
    {
        m_pendingUserText.clear();
        // A malformed body degrades to this same "empty response" gate either way (obj is a
        // default-constructed, key-less QJsonObject when parsing fails) — surface the actual parse
        // error when there is one, instead of a generic message with no diagnosable cause.
        const QString detail = parseError.error != QJsonParseError::NoError
                                   ? QString(" (response body wasn't valid JSON: %1)").arg(parseError.errorString())
                                   : QString();
        emit errorOccurred("AI: empty response." + detail + splitModeGiveUpNote());
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
