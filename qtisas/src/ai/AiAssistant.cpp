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
#include <QMetaEnum>
#include <QNetworkRequest>
#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimer>

#include <algorithm>

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

QString AiAssistant::correctionLogPath()
{
    return aiConfigBasePath() + "ai/corrections.log";
}

static void appendJsonLine(const QString &path, const QJsonObject &entry)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::Append | QIODevice::Text))
        f.write(QJsonDocument(entry).toJson(QJsonDocument::Compact) + "\n");
}

void AiAssistant::logCorrection(const QString &text, bool now) const
{
    const QString step = awaitingSelfReview                           ? "self-review"
                         : awaitingSplitPlan                          ? "split plan"
                         : awaitingSplitMerge                         ? "split merge"
                         : awaitingExistenceCheckScript               ? "existence check"
                         : m_busy                                     ? "generation"
                         : (m_retryTimer && m_retryTimer->isActive()) ? "HTTP retry"
                                                                      : "between steps";
    QJsonObject entry{{"time", QDateTime::currentDateTime().toString(Qt::ISODate)},
                      {"event", "correction"},
                      {"text", text},
                      {"now", now},
                      {"model", m_model.isEmpty() ? m_baseUrl : m_model},
                      {"request", withoutRequestSuffix(originalRequestText).left(600)},
                      {"step", step},
                      {"retries", autoContinueCount},
                      {"last_script", m_history.isEmpty() ? QString() : m_history.last().second.left(3000)},
                      {"last_error", lastRetryError.left(1500)}};
    if (listModeActive)
        entry["list_item"] = listItemIndex + 1;
    if (splitModeActive)
        entry["split_part"] = splitPartIndex + 1;
    appendJsonLine(correctionLogPath(), entry);
}

void AiAssistant::logCorrectionOutcome(const QString &result, const QString &detail)
{
    if (!m_correctionOutcomePending)
        return;
    m_correctionOutcomePending = false;
    appendJsonLine(correctionLogPath(), QJsonObject{{"time", QDateTime::currentDateTime().toString(Qt::ISODate)},
                                                    {"event", "outcome"},
                                                    {"result", result},
                                                    {"detail", detail.left(1500)}});
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
    QString checklist = m_content ? m_content->selfReviewChecklist(selectedDocTopics, splitModeActive) : QString();
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
// byte-different error text and slip past the "same diagnosis repeated" check.
QString AiAssistant::normalizeErrorForDedup(const QString &err)
{
    // Strips the WHOLE "at <ai>:N — <quoted source line>" suffix, not just the line number — the
    // quoted source text legitimately differs between two attempts sharing the exact same root
    // cause (e.g. `best_fc` still undefined) whenever the model reorders/rewrites surrounding
    // code while making the identical underlying mistake. Confirmed live: a NameError for
    // `best_fc` (a "fresh execution per round" violation — split-mode part 4 assumed part 3's
    // local variable was still bound) failed twice in a row, quoting `l2.insertCurve(best_fc,
    // ...)` the first time and `l1.insertCurve(best_fc, ...)` the second (the two insertCurve
    // calls got reordered) — same exception, same message, different quoted line, so the old
    // line-number-only strip still left these comparing unequal. `[^\n]*` (not `.*` with a
    // multiline flag) deliberately stops at the next newline rather than needing a pattern
    // option, matching this file's other single-line-safe regexes.
    static const QRegularExpression lineRe(R"(\bat <[^>]+>:\d+[^\n]*)");
    // errContext is the full captured result.output when non-empty, not just the bare exception
    // (see its assignment above — preceding prints often carry the useful diagnostic detail) —
    // but that means genuinely-identical retries (same code, same error) still carry real
    // wall-clock noise from anything printed before the failure, e.g. a fit's own progress log
    // ("Fit done: 7 ms - 2 iteration(s)..."). Confirmed live: the same script hit the same
    // AttributeError 6+ times in a row (33+ minutes of retries) because each attempt's "X ms"
    // timing never matched the last byte-for-byte, so this dedup check never once fired despite
    // the actual failure being 100% identical every time. Iteration counts and chi2/R²/Q values
    // are left alone — those are deterministic for identical inputs and SHOULD still contribute
    // to detecting a genuine difference; only the timing figure is inherently non-reproducible
    // noise on an otherwise-identical run.
    static const QRegularExpression timingRe(R"(\b\d+\s*ms\b)");
    QString normalized = err;
    normalized.replace(lineRe, "at <script>:N");
    normalized.replace(timingRe, "N ms");
    return normalized;
}

QString AiAssistant::functionNotFoundNameFromError(const QString &errContext)
{
    static const QRegularExpression re(R"([Ff]unction\s+'([^']+)'\s+not found)");
    const QRegularExpressionMatch m = re.match(errContext);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString AiAssistant::createdByEarlierPartsNote() const
{
    // What the earlier parts created, by name, from the host's live state — not left to the model to infer from the
    // plan. Confirmed live (LIST 5 item 5, split mode): part 2's tab script created "Auto", part 3 called
    // newTable("Auto") again, was blocked ("would wipe an existing table") and sent the same script until the item
    // was given up.
    if (!m_host)
        return {};
    QStringList created;
    for (const QString &object : m_host->liveObjects())
        if (!m_objectsAtRequestStart.contains(object))
            created << object;
    if (created.isEmpty())
        return {};
    m_host->print("▸▸▸ AI split-mode: created by earlier parts — " + created.join(", "));
    // A created table with its rows, columns and roles: what a later part needs to read it (dataset "Calib_y", the
    // error column), not only its name — confirmed live, a fit part re-guessed the column names part 1 had chosen.
    QStringList described;
    for (const QString &object : created)
    {
        const QString tablePrefix = QStringLiteral("table:");
        const QString shape =
            object.startsWith(tablePrefix) ? m_host->tableShape(object.mid(tablePrefix.size())) : QString();
        described << (shape.isEmpty() ? object : object + " (" + shape + ")");
    }
    return " They are, exactly: " + described.join(", ") +
           " — they exist now: use them as they are, never create any of them again.";
}

QString AiAssistant::projectFactsNote() const
{
    // What a refused probe usually asks for — which objects exist, their kinds, a table's columns — answered from the
    // host's own state, with nothing executed. Confirmed live (BIGMODEL, newest build): a refused probe asked for
    // tableNames() and Fittable's current function; the refusal told the model nothing. Objects created during this
    // LIST or request come first, with table shapes; the rest by name, capped.
    if (!m_host)
        return {};
    const QStringList live = m_host->liveObjects();
    if (live.isEmpty())
        return {};
    const QStringList &baseline = listModeActive ? m_listObjectsAtStart : m_objectsAtRequestStart;
    QStringList recent;
    QStringList older;
    for (const QString &object : live)
        (baseline.contains(object) ? older : recent) << object;
    QStringList described;
    for (const QString &object : recent)
    {
        const QString name = object.section(':', 1);
        QString line = object.section(':', 0, 0) + " \"" + name + "\"";
        if (object.startsWith(QLatin1String("table:")))
            if (const QString shape = m_host->tableShape(name); !shape.isEmpty())
                line += " (" + shape + ")";
        described << line;
    }
    constexpr int kMaxOlder = 40;
    for (int i = 0; i < older.size() && i < kMaxOlder; ++i)
        described << older[i].section(':', 0, 0) + " \"" + older[i].section(':', 1) + "\"";
    if (older.size() > kMaxOlder)
        described << QString("... and %1 more").arg(older.size() - kMaxOlder);
    return "\n\nWhat QtiSAS knows about the project right now (read from its own state, nothing executed): " +
           described.join("; ") + ". A fit's results are in table \"fitCurve-<function>\".";
}

void AiAssistant::recordFinishedListItem()
{
    // What the item that just ended made: new objects (the host's live state against the item's start) and the
    // functions its successful scripts compiled — a recompiled function keeps its name, so the object list alone
    // misses it. Confirmed live (Codex BIGMODEL): item 4 deleted and recompiled "ai/ExpDecay", item 5 "Fit the model
    // against the data" then fitted item 3's simulated sasviewmodels model — the note had listed only new tables.
    if (!listModeActive || listItemIndex < 0 || listItemIndex >= listItems.size() || !m_host)
        return;
    EarlierItem item;
    item.number = listItemIndex + 1;
    for (const QString &object : m_host->liveObjects())
        if (!m_itemObjectsAtStart.contains(object))
            item.objects << object;
    static const QRegularExpression nameRe(R"RX(\bsetFunctionName\s*\(\s*["']([^"']+)["'])RX");
    static const QRegularExpression compileRe(R"(\.\s*compile\s*\()");
    for (const QString &script : std::as_const(m_itemRanScripts))
        if (compileRe.match(script).hasMatch())
            for (auto it = nameRe.globalMatch(script); it.hasNext();)
            {
                const QString object = "function:" + it.next().captured(1);
                if (!item.objects.contains(object) && !item.compiled.contains(object))
                    item.compiled << object;
            }
    if (!item.objects.isEmpty() || !item.compiled.isEmpty())
        m_earlierItems << item;
}

QString AiAssistant::createdByEarlierItemsNote() const
{
    // What the earlier items of this LIST made, item by item, from the host's live state — a LIST item gets no
    // history, so "the previous step" / "the model" / "it" pointed at nothing. Confirmed live (BIGMODEL,
    // ai-test-requests item 10): the model fitted a leftover library function to the wrong data; on the newest build it
    // sent an EXPLORE probe asking for Fittable's current function and the table names — refused with EXPLORE off.
    if (!listModeActive || listItemIndex <= 0 || !m_host || m_earlierItems.isEmpty())
        return {};
    const QStringList live = m_host->liveObjects();
    QStringList perItem;
    QStringList consoleParts;
    QStringList previousDescribed; // what the item right before this one made, as described below
    QString previousTable;
    for (const EarlierItem &item : m_earlierItems)
    {
        QStringList described;
        for (const QString &object : item.objects + item.compiled)
        {
            const bool compiledAgain = item.compiled.contains(object);
            if (!compiledAgain && !live.contains(object))
                continue; // deleted since
            const QString kind = object.section(':', 0, 0);
            const QString name = object.section(':', 1);
            QString line = kind + " \"" + name + "\"" + (compiledAgain ? " (compiled)" : QString());
            if (kind == "table")
                if (const QString shape = m_host->tableShape(name); !shape.isEmpty())
                    line += " (" + shape + ")";
            static const QString fitPrefix = QStringLiteral("fitCurve-");
            if (kind == "table" && name.startsWith(fitPrefix) && !name.endsWith("-session"))
                line += " — the result table of a fit of " + name.mid(fitPrefix.size());
            described << line;
        }
        if (described.isEmpty())
            continue;
        perItem << QString("item %1: ").arg(item.number) + described.join("; ");
        consoleParts << QString("item %1: ").arg(item.number) + (item.objects + item.compiled).join(", ");
        if (item.number == listItemIndex)
        {
            previousDescribed = described;
            for (const QString &object : item.objects)
                if (object.startsWith("table:") && !object.endsWith("-session") && live.contains(object))
                {
                    previousTable = object.section(':', 1);
                    break;
                }
        }
    }
    if (perItem.isEmpty())
        return {};
    // "The previous step" pinned to the item it names. Confirmed live (local all-pyi LIST 1, item 10, five runs of
    // 2026-10-14): "Plot the fit results from the previous step" with item 9's "fitCurve-sasviewmodels/sphere" listed
    // above; every run made up data and a fit of its own instead, or looked up an invented fit table.
    static const QRegularExpression previousRe(R"(\b(?:previous|preceding|last|earlier)\s+(?:step|item|task|part)\b)",
                                               QRegularExpression::CaseInsensitiveOption);
    QString previousNote;
    if (!previousDescribed.isEmpty() && listItemIndex < listItems.size() &&
        previousRe.match(listItems[listItemIndex].text).hasMatch())
        previousNote = QString("\n\"The previous step\" in this item is item %1, and it made: ").arg(listItemIndex) +
                       previousDescribed.join("; ") + ". Work on exactly these objects, by name" +
                       (previousTable.isEmpty() ? QString() : " — e.g. table(\"" + previousTable + "\")") +
                       "; nothing else is meant.";
    m_host->print("▸▸▸ AI LIST: made by earlier items — " + consoleParts.join(" | "));
    if (!previousNote.isEmpty())
        m_host->print(QString("▸▸▸ AI LIST: \"the previous step\" = item %1").arg(listItemIndex));
    return "\n\nMADE BY EARLIER ITEMS of this list (read from QtiSAS — they exist now), item by item:\n- " +
           perItem.join("\n- ") +
           "\nWhen this item refers to an earlier result without naming it (\"the previous step\", \"the model\", "
           "\"the data\", \"it\"), it means these objects — the most recent item's first; use them by name." +
           previousNote;
}

bool AiAssistant::namesSomethingNew(const QString &description, const QString &requestText)
{
    // name-like only: a planner also quotes formulas ("pow(column(1), 3)") and labels, which name no object
    static const QRegularExpression quotedRe(R"RX("([A-Za-z_][\w./:-]{0,79})"|'([A-Za-z_][\w./:-]{0,79})')RX");
    for (auto it = quotedRe.globalMatch(description); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (!requestText.contains(name, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool AiAssistant::requestCreatesName(const QString &requestText, const QString &name)
{
    const QRegularExpression createRe(
        R"RX((?:\b(?:create|make|build|compile|new|add)\b\W+(?:\w+\W+){0,4}|\bnamed\s+)")RX" +
            QRegularExpression::escape(name) + "\"",
        QRegularExpression::CaseInsensitiveOption);
    return createRe.match(requestText).hasMatch();
}

QString AiAssistant::quotedNameWithFolder(const QString &requestText, const QString &shortName)
{
    if (shortName.isEmpty() || shortName.contains('/'))
        return {};
    static const QRegularExpression quotedRe(R"re("([^"\n]{1,80})")re");
    for (auto it = quotedRe.globalMatch(requestText); it.hasNext();)
    {
        const QString name = it.next().captured(1);
        if (name.endsWith("/" + shortName))
            return name;
    }
    return {};
}

QStringList AiAssistant::compiledFunctionNamesFromHistory(const QList<QPair<QString, QString>> &history)
{
    QStringList names;
    static const QRegularExpression re(R"(setFunctionName\s*\(\s*["']([^"']+)["']\s*\))");
    for (const auto &turn : history)
    {
        QRegularExpressionMatchIterator it = re.globalMatch(turn.second);
        while (it.hasNext())
        {
            const QString name = it.next().captured(1);
            if (!names.contains(name))
                names << name;
        }
    }
    return names;
}

// Word-boundary match of `topic` against `text` — used everywhere a topic name is looked for in
// freeform model text (doc-selection answers, split-part descriptions/DOCS fields) instead of
// plain QString::contains(), which would let "graph3d" (containing the literal characters
// "graph") silently satisfy a check meant for the standalone topic "graph" too, regardless of
// intent. Confirmed live this isn't harmless overlap the way it looks: graph-python-api.md's
// "legend" (newLegend()/setLegend(), a 2D text-box widget) and graph3d-python-api.md's "legend"
// (showLegend(), a colorscale toggle) are same-named but unrelated APIs — auto-including "graph"
// for every graph3d-only task injects a differently-behaving, same-named method into context
// rather than just padding it with redundant information.
static bool textNamesTopic(const QString &text, const QString &topic)
{
    static QHash<QString, QRegularExpression> cache;
    auto it = cache.find(topic);
    if (it == cache.end())
        it = cache.insert(topic, QRegularExpression(R"(\b)" + QRegularExpression::escape(topic) + R"(\b)",
                                                    QRegularExpression::CaseInsensitiveOption));
    return it.value().match(text).hasMatch();
}

// Doc-selection round-trip: the real request
// is held in pendingRequestText until this cheap, ai-doc-only pre-request comes back naming
// which topic docs are actually relevant, so the (much larger) real request(s) below only carry
// those. textNamesTopic() rather than strict comma-list parsing — robust to the model wrapping
// its answer in a sentence instead of a bare list, without the "graph3d" substring false-positive
// plain contains() would have (see that function's doc comment). Empty/"all"/anything
// unrecognized means "load everything" — the safe default, never a hard failure.
// Sentinel meaning "genuinely zero topic docs" — distinct from an empty QStringList, which
// filterDocsByTopic() deliberately treats as "ambiguous, fail open to everything" (see its own
// comment). Without this distinction there is no way to express "this turn needs no API
// reference at all" — confirmed live: a long back-and-forth of plain discussion questions
// ("why was background fixed?", "are you sure?", ...) had nothing to match against the known
// topic list, so every single one fell through to "all" (~85K tokens of docs) on top of an
// already-large history, compounding turn over turn until the request exceeded the model's
// context window entirely (HTTP 400) and ended the conversation.
static const QString kNoDocsSentinel = QStringLiteral("__none__");

// Human-readable label for a split-plan part's doc topics, in the 3 places that print/reconstruct
// "DOCS: X" — must not conflate the sentinel with genuinely-empty (see kNoDocsSentinel's comment
// above): an empty list's "all" fallback is correct there, but would be actively misleading here
// (announcing "docs: all" for a part that was deliberately given zero docs, e.g. under
// DocsMode::Integrated — see handleSplitPlanResponse()'s parse loop).
static QString docTopicsDisplayLabel(const QStringList &topics)
{
    if (topics == QStringList{kNoDocsSentinel})
        return "none";
    return topics.isEmpty() ? "all" : topics.join(", ");
}

// Shared by docTopicsFromRequestText() below and splitPartTopicsFromAnswer() further down —
// dedicated synonym regexes for topic names that rarely appear verbatim in natural prose:
// "fittable" (the class name itself) and "graph3d" (users say "3D surface"/"3D plot", never the
// literal topic name) — confirmed live via ai-test-requests.md's own "3D surface plot" fixture
// entry, which detected zero topics at all before this regex was added.
static const QHash<QString, QRegularExpression> topicSynonymRe = {
    // "simulate"/"simulated" added — confirmed live: "plot simulated 'ai/PorodPowerLaw' curve with
    // log-log axes and proper formatting" needs to look up the table simulateCurve() created
    // (named "simulatedCurve-<function>", a naming convention documented ONLY in
    // fittable-python-api.md — see its "simulateCurve()'s output table" section, including the
    // warning against reaching for plot("simulatedCurve-<function>", ...) as a formula string
    // instead), but got no "fittable" topic at all: it has no "fit"-rooted word, no "chi2", no
    // "fitcurve". The SIBLING part ("simulate curve from compiled ... function without fitting
    // real data") only happened to get "fittable" by accident — it contains the word "fitting"
    // inside "without fitting real data", a NEGATION, not an actual fit action — not because
    // simulateCurve() itself (the real reason either part needs these docs) was ever matched.
    // (?!\s*(?:function|model)s?\b) added — confirmed live: "compile two custom fit functions"
    // matched bare \bfit\b and force-added fittable to a part whose script only ever called
    // Compiler, regardless of what DOCS the model itself had already correctly said (compile
    // only) — this mechanical backstop ran unconditionally on the model's answer either way.
    // "fit function"/"fit model" name what's being AUTHORED, not an action calling into Fittable.
    {"fittable", QRegularExpression(R"(\bfit(s|ted|ting)?\b(?!\s*(?:function|model)s?\b)|chi2|chi²|fitcurve|)"
                                    R"(\bsimulat(e|ed|ing|ion)\b)",
                                    QRegularExpression::CaseInsensitiveOption)},
    {"graph3d",
     QRegularExpression(R"(\b3d\b|\bsurface\s*plot\b|\bplot3d\b)", QRegularExpression::CaseInsensitiveOption)},
    // table/matrix/graph/note/compile: docTopicsFromRequestText() has no model-generated DOCS field
    // to fall back on the way splitPartTopicsFromAnswer() does — these five need to carry the full
    // keyword-matching weight on their own, unlike fittable/graph3d above (which were only ever
    // meant as a backstop for cases the model's own answer already mostly covered).
    {"table",
     QRegularExpression(R"(\btables?\b|\bcolumns?\b|\bspreadsheet\b)", QRegularExpression::CaseInsensitiveOption)},
    {"matrix", QRegularExpression(R"(\bmatri(?:x|ces)\b)", QRegularExpression::CaseInsensitiveOption)},
    {"graph", QRegularExpression(R"(\bplot(?:s|ted|ting)?\b|\bgraphs?\b|\bcharts?\b|\blegend\b)",
                                 QRegularExpression::CaseInsensitiveOption)},
    {"note", QRegularExpression(R"(\bnotes?\b)", QRegularExpression::CaseInsensitiveOption)},
    // "fit function(s)" added — confirmed live (benchmark LIST 3, item 12): "Delete the fit functions ... from the
    // library" matched no topic at all; managing the library (delete, open, list) is the Compiler's, and fittable's
    // own regex deliberately excludes "fit function" (see above).
    {"compile", QRegularExpression(R"(\bcompil(?:e|ed|ing|er)\b|\bfit\s+functions?\b)",
                                   QRegularExpression::CaseInsensitiveOption)},
};

// The user's request without requestSuffix()'s transport guidance (either mode's), for everything that inspects what
// the user asked: topic matching, the scope checks, the input names. Confirmed live: the run-mode suffix quotes
// scriptPrint("..."), and the window-name rule took that "..." for a name the request had decided.
QString withoutRequestSuffix(const QString &text)
{
    for (const QString &marker : {QStringLiteral("\n\n(Answer in plain text if this is a question."),
                                  QStringLiteral("\n\n(Return ONLY executable Python code")})
        if (const qsizetype pos = text.indexOf(marker); pos >= 0)
            return text.left(pos);
    return text;
}

static QString topicAnalysisText(const QString &text)
{
    return withoutRequestSuffix(text);
}

// True if topic's own keyword regex matches text — shared by splitPartTopicsFromAnswer() (widens
// a DOCS-field selection) and docTopicsFromRequestText().
static bool topicSynonymMatch(const QString &topic, const QString &text)
{
    const auto it = topicSynonymRe.constFind(topic);
    if (it == topicSynonymRe.constEnd() || !it.value().match(text).hasMatch())
        return false;
    if (topic == "graph" && topicSynonymRe.value("graph3d").match(text).hasMatch())
        return false;
    return true;
}

// Every known topic whose OWN keyword regex matches the request text — the same predicate the
// removed doc-selection pre-flight used only as a narrowing filter, applied additively instead.
//
// Confirmed live: "Compile a custom Porod power-law fit function ... then simulate a curve from
// it" selected `compile` alone, with no `fittable` — yet simulateCurve()/the Fittable stage API
// live only in the fittable docs, and topicSynonymRe's own "fittable" entry already matches
// "simulat(e|ed|ing|ion)". The model simply omitted it, and nothing on the non-split path could
// add a topic back: only split mode widens (see splitPartTopicsFromAnswer()), so a NOSPLIT run
// had no backstop at all. Under-selection is the expensive failure here — the part then guesses
// the API and burns its whole retry budget — while over-selection costs prompt tokens only.
//
// Note this makes the model's own doc-selection answer redundant BY CONSTRUCTION: narrowing kept
// {model-selected AND keyword-matched}, so adding widening leaves {keyword-matched} regardless of
// what the model said. That is why beginNonSplitRequest() no longer asks it at all — the answer
// could not change the outcome, and asking cost a full round-trip (13-18s measured) per request.
QStringList AiAssistant::docTopicsFromRequestText(const QString &text) const
{
    const QString requestText = topicAnalysisText(text);
    QStringList topics;
    for (const QString &topic : (m_content ? m_content->knownDocTopics() : QStringList()))
        if (topicSynonymMatch(topic, requestText))
            topics << topic;
    return topics;
}

// Shared tail of both topic-selection paths: report, prepend the baseline, dispatch.
void AiAssistant::startRequestWithSelectedTopics(const QStringList &selected, const QString &label)
{
    if (m_host)
        m_host->print("▸▸▸ " + label + ": " + (selected.isEmpty() ? QString("all") : selected.join(", ")));

    // Baseline topics are PREPENDED so task-specific ones land closest to the request
    // (avoids "lost in the middle" — see filterDocsByTopic()). Guarded on non-empty: an
    // empty list means "send everything", and prepending here would wrongly narrow it.
    startRequest(selected.isEmpty() ? selected
                                    : ((m_content ? m_content->baselineDocTopics() : QStringList()) + selected));
}

// Where a clause of a request ends: a sentence end, a semicolon, a line end or ", then ...". A full stop only before
// whitespace or the end, and never one of "...": confirmed live (LIST 2 item 6), "Create a note \"Log\", a table
// \"Metrics\" ... (x = 1, 2, ..., 20, ...), and a styled graph \"MetricsPlot\"" ended a clause at the dots of the
// range, the graph lost its "Create", was taken as an existing input and the item was refused.
static const QString kClauseBoundaryPattern =
    QStringLiteral(R"((?<!\.)\.(?!\.)(?=\s|$)|[;\n]|,\s*(?:then|next|afterwards?|finally|after that)\b)");

// A quoted name with a <placeholder> in it ("simulatedCurve-<function name>") is a pattern the request fills in, not a
// name that could exist: confirmed live (LIST 2 item 3), "whether its table \"simulatedCurve-<function name>\" exists"
// was checked as an existing input, came back missing and the item was refused.
static bool isPlaceholderName(const QString &name)
{
    static const QRegularExpression placeholderRe(R"(<[^<>]+>)");
    return placeholderRe.match(name).hasMatch();
}

// True if the name spanning [nameStart, nameEnd) in `text` is something the request CREATES, not an input: a creation
// verb earlier in its own clause, or a conditional creation ("create it if it does not exist") later in it. Shared by
// extractExistingEntityNames() and inputNamesInRequest().
static bool inCreationClause(const QString &text, qsizetype nameStart, qsizetype nameEnd)
{
    // "plot\w*" belongs here: plot()/plotResults() CREATE a window (app-python-api.md says so
    // outright), and splitPlanCreationNames() already counts "plot" as a creation verb for the
    // collision check. Confirmed live, and this pattern's own regression: "Plot the fit results
    // from the previous step into graph \"FitResults\"" had FitResults extracted as an EXISTING
    // name, came back MISSING, and the whole item was refused with "reply with the correct name" —
    // for a graph the request was plainly asking to CREATE.
    static const QRegularExpression creationVerbRe(
        R"(\b(?:creat\w*|mak\w*|build\w*|generat\w*|synthesi\w*|plot\w*|draw\w*|add|adds|adding|new)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // Clause-scoped lookback, NOT a fixed character window. A fixed 40 chars was wrong in both
    // directions: too short to see the "Plot" 48 characters before `graph "FitResults"`, and
    // widening it would have reached into an earlier, unrelated clause and disqualified a genuine
    // input ("Create a table \"Out\" ..., then fit table \"RawScan\"" must still check RawScan).
    // Scoping to the current clause — back to the nearest clause boundary — gets both right,
    // because a creation verb governs the clause it appears in, not the whole request.
    //
    // A BARE comma is deliberately NOT a boundary. It was, and an enumeration inside the very
    // clause that holds the creation verb then cut the verb off: confirmed live, "Plot the winning
    // model's data, fit curve, and residuals into graph \"GaussPeakFit\"" left a lookback of only
    // "and residuals into ", so a graph the request was plainly creating was reported as an
    // existing name — and it really did match a stale GaussPeakFit.fif left by an earlier run, so
    // the fact fed back was true, irrelevant, and about the wrong kind of object entirely. Only a
    // comma that actually introduces a new predicate (", then fit ...") still counts, which is the
    // one case the comma was added for. Widening this errs toward skipping a check rather than
    // claiming a creation target, which is the safe direction: a skipped check costs nothing here,
    // while a wrong claim feeds a false fact and, in auto-run mode, refuses the item outright.
    static const QRegularExpression clauseBoundaryRe(kClauseBoundaryPattern, QRegularExpression::CaseInsensitiveOption);
    // "if it does not exist", "if missing", "if needed": the creation is meant only when the name is absent
    static const QRegularExpression conditionalRe(
        R"(\bif\b(?:\s+\S+){0,4}?\s+(?:not\s+exist\w*|n't\s+exist\w*|missing|absent|needed|necessary)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // A name the creation READS FROM is its input, not its target: "Create a graph \"Report5\" ... with the \"Calib\" y
    // data ... the fitted curve from table \"fitCurve-ai/Quad\"". Confirmed live (LIST 5 item 3, 2026-10-11): both were
    // taken as created, so no project state was given for them, and the model printed "-1 -1 -1" with the comment
    // "cannot execute without proper data" while both tables existed. A name right after from/of (with at most "the"
    // and a kind word between), or right before data/points/values/column(s)/residuals, is read.
    static const QRegularExpression sourceBeforeRe(R"(\b(?:from|of)\s+(?:the\s+)?(?:\w+\s+)?"?$)",
                                                   QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression sourceAfterRe(R"(^"?\s+(?:\w+\s+)?(?:data|points|values|columns?|residuals)\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    if (sourceBeforeRe.match(text.left(nameStart)).hasMatch() || sourceAfterRe.match(text.mid(nameEnd)).hasMatch())
        return false;
    const QString before = text.left(nameStart);
    // Last boundary's END, not its start: a ", then" boundary is several characters long, and
    // keeping its own text in the lookback would leave nothing wrong but reads as sloppy.
    qsizetype boundaryEnd = 0;
    QRegularExpressionMatchIterator boundaryIt = clauseBoundaryRe.globalMatch(before);
    while (boundaryIt.hasNext())
        boundaryEnd = boundaryIt.next().capturedEnd(0);
    const QString lookback = before.mid(boundaryEnd); // whole clause when there is no boundary
    if (creationVerbRe.match(lookback).hasMatch())
        return true;
    // A conditional creation AFTER the name, in the same clause, makes it no input either.
    // Confirmed live: `write that line into table "Results": create it if it does not exist`
    // claimed Results as existing, it came back MISSING, and the item was refused outright —
    // although the request said in so many words what to do when it is missing.
    const QString after = text.mid(nameEnd);
    const QRegularExpressionMatch nextBoundary = clauseBoundaryRe.match(after);
    const QString lookahead = nextBoundary.hasMatch() ? after.left(nextBoundary.capturedStart(0)) : after;
    return creationVerbRe.match(lookahead).hasMatch() && conditionalRe.match(lookahead).hasMatch();
}

// True if the name starting at `nameStart` in `text` is what its clause deletes or closes ("close and delete the table
// \"Peak5\" and the note \"Pipeline\""). Such a name is no input that must exist: missing is already the end state the
// request asks for, and the item's own script reports it. Confirmed live (LIST 5 NOSPLIT, item 6): an earlier item had
// failed to create "Pipeline", and the existence check stopped the cleanup item outright instead of letting it delete
// the rest and print that "Pipeline" does not exist. Not when the name only holds what is deleted ("delete rows 2-5
// from table \"Data\"", "remove column \"y\" of table \"T\""): a from/in/of between verb and name keeps it an input.
static bool inDeletionClause(const QString &text, qsizetype nameStart)
{
    // Word forms of the verbs, not prefixes: "clos\w*" also took "closest" and "closer".
    static const QRegularExpression deletionVerbRe(R"(\b(?:delet(?:e|es|ed|ing)|remov(?:e|es|ed|ing)|clos(?:e)"
                                                   R"(|es|ed|ing)|discard(?:s|ed|ing)?|drop(?:s|ped|ping)?)\b)",
                                                   QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression containerRe(R"(\b(?:from|in|of|inside)\b)",
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression clauseBoundaryRe(kClauseBoundaryPattern, QRegularExpression::CaseInsensitiveOption);
    const QString before = text.left(nameStart);
    qsizetype boundaryEnd = 0;
    for (auto it = clauseBoundaryRe.globalMatch(before); it.hasNext();)
        boundaryEnd = it.next().capturedEnd(0);
    const QString lookback = before.mid(boundaryEnd);
    qsizetype verbEnd = -1;
    for (auto it = deletionVerbRe.globalMatch(lookback); it.hasNext();)
        verbEnd = it.next().capturedEnd(0);
    return verbEnd >= 0 && !containerRe.match(lookback.mid(verbEnd)).hasMatch();
}

// Every quoted name the request creates somewhere — in any of its clauses. Such a name is never an input, wherever
// else it appears: confirmed live (benchmark LIST 5, item 5), "the \"make\" tab holds a script that creates a table
// \"Auto\" ... Print ... whether table \"Auto\" exists" stopped the item as referring to a missing "Auto".
static QStringList namesCreatedInRequest(const QString &requestText)
{
    static const QRegularExpression quotedRe(R"re("([^"\n]{1,80})")re");
    QStringList created;
    QRegularExpressionMatchIterator it = quotedRe.globalMatch(requestText);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1).trimmed();
        if (!created.contains(name) && inCreationClause(requestText, m.capturedStart(0), m.capturedEnd(0)))
            created << name;
    }
    return created;
}

QStringList inputNamesInRequest(const QString &requestText)
{
    // Every quoted name the request does not create — whatever its wording ("In \"Squares\", ..."
    // names no kind and says nothing like "existing", so extractExistingEntityNames() never sees it).
    // Answered by the host's own state (see AiAssistant::existingInputNamesNote()), never by a script.
    static const QRegularExpression quotedRe(R"re("([^"\n]{1,80})")re");
    const QStringList created = namesCreatedInRequest(requestText);
    QStringList names;
    QRegularExpressionMatchIterator it = quotedRe.globalMatch(requestText);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1).trimmed();
        if (name.isEmpty() || names.contains(name) || created.contains(name) || isPlaceholderName(name))
            continue;
        names << name;
    }
    return names;
}

// Extracts every quoted name in `requestText` implied to already exist, paired with the KNOWN
// kind ("window" — see existingEntityVerificationPrompt()'s doc comment on why table/matrix/note/
// graph collapse to this one label) when the word directly before the quote names one of them, or
// an empty string when genuinely unknown — via either of two patterns:
//   1. The word "existing" followed by up to 3 other words then a quoted string, e.g. `the
//      existing "Sphere" form-factor model` (no noun before the quote — unknown kind) or
//      `existing table "SAXS_data"` (known: window).
//   2. A READ-oriented verb (fit/plot/read/analyze) followed by up to 2 other words then an
//      object-type noun (table/matrix/note/graph) then a quoted string, e.g. `Fit table
//      "SAXS_data"` — the same implied-existence reading ai-python-api.md's own EXPLORE rule
//      already gives this phrasing.
// Pattern 2 is deliberately narrower than a bare verb+quote match — it requires the object-type
// noun directly before the quote, and skips any match with "new" between the verb and the noun
// (e.g. "plot a new graph 'Y'") — to keep this a CREATE-vs-READ distinction, not a blanket trigger
// on every verb+quote combination. Host-independent: pure text pattern matching, no
// QtiSAS-specific vocabulary involved.
//
// The known kind is derived the SAME way for both patterns — whichever one matched, check whether
// the word immediately before the quote is table/matrix/note/graph — rather than baking it only
// into pattern 2's own capture, so "existing table \"X\"" (pattern 1) is recognized as a known
// window reference exactly like "fit table \"X\"" (pattern 2) is. A name matched twice keeps
// whichever match found a known kind. Never a "function"/"compile" kind here: the noun list is
// deliberately table/matrix/note/graph only, so a function/model reference (e.g. "the existing
// 'sasviewmodels/sphere' form-factor model") stays unknown — there is no dedicated function-noun
// detection.
QList<QPair<QString, QString>> extractExistingEntityNames(const QString &requestText)
{
    QList<QPair<QString, QString>> names;
    const QStringList created = namesCreatedInRequest(requestText); // see namesCreatedInRequest()
    static const QRegularExpression existingRe(R"re(\bexisting\b(?:\s+\S+){0,3}?\s*"([^"]+)")re",
                                               QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression verbImpliedRe(
        R"re(\b(?:fit|plot|read|analyze)\b(?:\s+\S+){0,2}?\s+(?:table|matrix|note|graph)\s*"([^"]+)")re",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression trailingNounRe(R"(\b(?:table|matrix|note|graph)\s*$)",
                                                   QRegularExpression::CaseInsensitiveOption);
    // Function-kind nouns, the counterpart to trailingNounRe's window nouns. Checked on BOTH sides
    // of the quoted name, because the natural phrasing puts the noun AFTER it: real requests say
    // `the existing "sasviewmodels/sphere" form-factor model`, where nothing before the quote
    // names a kind at all. A before-quote form ("the fit function \"x\"") is covered too.
    // Two-token window after the name reaches across a qualifier like "form-factor".
    static const QRegularExpression functionNounBeforeRe(R"(\b(?:model|models|function|functions|library)\s*$)",
                                                         QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression functionNounAfterRe(
        R"(^(?:\s+\S+){0,2}?\s+(?:model|models|function|functions|library)\b)",
        QRegularExpression::CaseInsensitiveOption);
    for (const QRegularExpression &re : {existingRe, verbImpliedRe})
    {
        QRegularExpressionMatchIterator it = re.globalMatch(requestText);
        while (it.hasNext())
        {
            const QRegularExpressionMatch m = it.next();
            const QString between = requestText.mid(m.capturedStart(0), m.capturedStart(1) - m.capturedStart(0));
            if (between.contains("new", Qt::CaseInsensitive))
                continue;
            const QString name = m.captured(1);
            if (created.contains(name.trimmed()) || isPlaceholderName(name) ||
                inDeletionClause(requestText, m.capturedStart(1)))
                continue;
            // `between` always ends with the pattern's own literal opening quote character (both
            // patterns end with \s*"(...)"), which would otherwise defeat trailingNounRe's \s*$
            // anchor — strip it before checking what word precedes the quote.
            const QString beforeQuote = between.chopped(1);
            // A function noun pins the kind just as well as a window noun, and the caller widens an
            // unknown kind to ALL THREE checks — so without this, `the existing
            // "sasviewmodels/sphere" form-factor model` paid a window check that could only ever
            // print MISSING (confirmed live). Window noun wins when both appear, e.g. "the existing
            // model graph \"x\"": a graph really is a window, whatever else the phrase calls it.
            const bool hasWindowNoun = trailingNounRe.match(beforeQuote).hasMatch();
            const bool hasFunctionNoun =
                !hasWindowNoun && (functionNounBeforeRe.match(beforeQuote).hasMatch() ||
                                   functionNounAfterRe.match(requestText.mid(m.capturedEnd(0))).hasMatch());
            const QString knownKind = hasWindowNoun     ? QStringLiteral("window")
                                      : hasFunctionNoun ? QStringLiteral("function")
                                                        : QString();
            bool merged = false;
            for (QPair<QString, QString> &entry : names)
            {
                if (entry.first != name)
                    continue;
                if (entry.second.isEmpty() && !knownKind.isEmpty())
                    entry.second = knownKind;
                merged = true;
                break;
            }
            if (!merged)
                names << qMakePair(name, knownKind);
        }
    }

    // Third pattern, on its own because it needs a guard the other two don't: a bare kind noun
    // immediately followed by a quoted name, ANYWHERE in the request. Both patterns above are
    // distance-limited from their anchor word, and confirmed live that loses the most explicit
    // signal a request can carry — `Fit the existing "sasviewmodels/sphere" ... against table
    // "SAXS_data" columns I and dI` names the kind AND the name outright, yet "SAXS_data" sits ~15
    // tokens past "existing" and the verb "Fit" is nowhere near "table", so neither pattern saw it.
    // That was the one name whose state actually mattered: it had been created with 0 rows, and the
    // fit died on "failed to read data for the selected dataset(s)" a minute and a half later.
    //
    // The guard is not optional. This shape also matches a CREATION target ("Create a table
    // \"GaussPeak\" with 200 rows"), and a creation target reported as an existing name comes back
    // MISSING — which the verification prompt answers with "stop and report to the user", killing
    // an item before it does anything. Creation of a name is a different mechanism entirely (see
    // splitPlanCreationNames()); this one must only ever claim names the request treats as INPUT.
    static const QRegularExpression kindNounRe(R"re(\b(table|matrix|note|graph)\s*"([^"]+)")re",
                                               QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator kindIt = kindNounRe.globalMatch(requestText);
    while (kindIt.hasNext())
    {
        const QRegularExpressionMatch m = kindIt.next();
        const QString name = m.captured(2);
        if (created.contains(name.trimmed()) || isPlaceholderName(name) ||
            inCreationClause(requestText, m.capturedStart(0), m.capturedEnd(0)) ||
            inDeletionClause(requestText, m.capturedStart(2)))
            continue;
        bool merged = false;
        for (QPair<QString, QString> &entry : names)
        {
            if (entry.first != name)
                continue;
            if (entry.second.isEmpty())
                entry.second = QStringLiteral("window");
            merged = true;
            break;
        }
        if (!merged)
            names << qMakePair(name, QStringLiteral("window"));
    }
    return names;
}

// Returns the subset of `checkedNames` (name, knownKind-or-empty pairs from
// extractExistingEntityNames()) found missing.
//
// A KNOWN-kind name trusts ONLY its own labeled line ("(window) MISSING: <name>") — sidesteps the
// VERDICT self-contradiction risk below entirely. An UNKNOWN-kind name (still gets all three
// checks — window/compile/fittable) falls back to VERDICT+fail-open: only "missing" if the
// verdict says so AND no "EXISTS: <name>" substring appears anywhere (confirmed live: the model's
// own labeled lines correctly found "(compile) EXISTS: ..."/"(fittable) EXISTS: ..." while its own
// VERDICT line still said "MISSING" — self-contradictory, not a script bug).
QStringList missingEntityNames(const QString &scriptOutput, const QList<QPair<QString, QString>> &checkedNames)
{
    QStringList missing;
    for (const QPair<QString, QString> &entry : checkedNames)
    {
        const QString &name = entry.first;
        const QString &knownKind = entry.second;
        if (!knownKind.isEmpty())
        {
            // EXISTS must win over a contradicting MISSING for the same label — mirrors the
            // unknown-kind branch below (and splitPlanCreationCollisions()'s own fail-closed
            // rule), since a check-script can print both lines for the same name.
            if (scriptOutput.contains("(" + knownKind + ") MISSING: " + name) &&
                !scriptOutput.contains("(" + knownKind + ") EXISTS: " + name))
                missing << name;
            continue;
        }
        if (scriptOutput.contains("VERDICT: MISSING " + name) && !scriptOutput.contains("EXISTS: " + name))
            missing << name;
    }
    return missing;
}

void AiAssistant::resetPendingRoundTrips()
{
    awaitingSelfReview = false;
    awaitingSelfReviewSkepticismRetry = false;
    awaitingSelfReviewScopeRetry = false;
    awaitingSelfReviewFunctionNameRetry = false;
    awaitingSelfReviewIncompletenessRetry = false;
    awaitingSelfReviewLookupRegressionRetry = false;
    awaitingSelfReviewEnumRegressionRetry = false;
    awaitingSelfReviewCompileSequenceRetry = false;
    awaitingSelfReviewPyQtVersionRetry = false;
    awaitingSelfReviewFabricatedSetCodeRetry = false;
    awaitingSelfReviewCreationNameRetry = false;
    awaitingSelfReviewExploreGroupRetry = false;
    awaitingExistenceCheckScript = false;
    awaitingSplitPlanCreationCheckScript = false;
    awaitingContinuationCheck = false;
    awaitingSplitPlan = false;
    awaitingSplitMerge = false;
    awaitingSplitMergeRetry = false;
}

QString AiAssistant::failedRunObjectsNote() const
{
    if (m_failedRunObjects.isEmpty() && m_closedFailedObjects.isEmpty())
        return {};
    QString closedNote;
    if (!m_closedFailedObjects.isEmpty())
    {
        QStringList named;
        for (const QString &object : m_closedFailedObjects)
            named << object.section(':', 0, 0) + " \"" + object.section(':', 1) + "\"";
        closedNote = "The failed attempt's " + named.join(", ") +
                     " was closed — it no longer exists: create it again exactly as the request says. ";
    }
    // Confirmed live (LIST 5, item 1, NOSPLIT): "look each up and reuse it" for a table became `if t is None:` around
    // its whole creation — the corrected formula inside it never ran. newTable() on the same name clears the table,
    // so a table or matrix is simply created again; only what would be added twice is reused.
    QStringList recreate;
    QStringList reuse;
    for (const QString &object : m_failedRunObjects)
    {
        const QString kind = object.section(':', 0, 0);
        const QString named = kind + " \"" + object.section(':', 1) + "\"";
        if (kind == "table" || kind == "matrix")
            recreate << named;
        else if (kind != "function") // a requested function is compiled again: see the sentence above
            reuse << named;
    }
    QString note;
    if (!recreate.isEmpty())
        note += "The failed attempt(s) left " + recreate.join(", ") +
                ": create and fill it again exactly as the request says, with no existence check around it — "
                "the new...() call on its name clears it. ";
    if (!reuse.isEmpty())
        note += "The failed attempt(s) already created these, and they still exist: " + reuse.join(", ") +
                ". Look each up by name and reuse it; do not add its tabs, layers or curves a second time. ";
    return closedNote + note;
}

void AiAssistant::beginRequest(const QString &aiText, bool autoRunValue, bool wantsSplit)
{
    // Checked here too, not just inside ask() (which still keeps its own copy of this guard as a
    // safety net for any internal chained call) — confirmed live: a request submitted while
    // another was still in flight got THIS FAR before the busy-guard buried inside some nested
    // ask() call finally rejected it, but by then every reset below had already run and mutated
    // shared state (pendingRequestText, originalRequestText, splitModeActive, m_lastScriptUnrun,
    // ...) that the ALREADY-in-flight request's own still-pending async response would read once
    // it returned — a real, if narrow, race between a request that never actually got submitted
    // and one genuinely in progress. Rejecting before any reset runs makes an overlapping request
    // a true no-op: no state change, no history entry, nothing.
    if (m_busy)
    {
        emit errorOccurred("▸▸▸ AI: previous request still in progress.");
        return;
    }
    // Per-request state reset — moved here from ApplicationWindow::askAi() so that function stops
    // reaching into AiAssistant's internal fields directly (see this method's header doc comment).
    autoRun = autoRunValue;
    autoContinueCount = 0;
    m_failedRunObjects.clear();
    m_closedFailedObjects.clear();
    m_failedRunScripts.clear();
    m_objectsAtRequestStart = m_host ? m_host->liveObjects() : QStringList();
    consecutiveExploreCount = 0;
    lastExploreOutput.clear();
    lastExploreCode.clear();
    // Every pending round-trip, not the three that used to be listed here — see
    // resetPendingRoundTrips()'s doc comment for the crash a hand-picked subset caused.
    resetPendingRoundTrips();
    // A script left over from a request the user has since moved on from is no longer safely "the
    // thing 'run it' refers to" — see hasUnrunLastScript()'s doc comment. Reached only for a
    // request ApplicationWindow's "run it" fast path did NOT consume (that path returns before
    // ever calling beginRequest()), so any request landing here supersedes it unconditionally.
    m_lastScriptUnrun = false;
    selectedDocTopics.clear();
    resetErrorRetryState();
    splitModeActive = wantsSplit;
    splitPlanFixupRoundsUsed = 0;
    lastSplitPlanFixupComplaints.clear();
    splitPlanCreationNameRetryRoundsUsed = 0;
    splitPlanSandboxExemptNames.clear();
    splitPlan.clear();
    splitPartIndex = -1;
    splitPartScripts.clear();
    pendingRequestText = aiText;
    pendingLiveFactsNote.clear();
    // Set only on the FIRST request of a conversation — see originalRequestText's declaration for
    // why this must NOT be overwritten by every subsequent short follow-up. Also refreshed
    // whenever m_history is empty, even if originalRequestText already holds text from an earlier
    // request — confirmed live: a request that gets blocked before ever producing a real
    // history-adding exchange (e.g. this file's own split-plan collision halt) leaves
    // originalRequestText set to ITS text with m_history still empty; the ONLY other place that
    // refreshes this anchor is handleContinuationCheckResponse(), but dispatchAfterExistenceCheck()
    // skips that whole check whenever m_history.isEmpty() ("nothing in history could conflict with
    // this request"). The next, entirely unrelated request then satisfies neither refresh
    // condition, so every fixup-retry prompt for it keeps resending the PRIOR request's text as
    // "Original request" — confirmed live: a request mentioning "an adjustable exponent parameter"
    // leaked that exact phrase into a later, unrelated request's generated plan this way, the
    // phrase never having appeared in the later request's own text at all. An empty history means
    // (by dispatchAfterExistenceCheck()'s own stated reasoning) nothing could meaningfully be a
    // continuation of anything, so whatever's currently anchored is safe to overwrite.
    if (originalRequestText.isEmpty() || m_history.isEmpty())
        originalRequestText = aiText;

    // When scripts run: what the request's input names really are now — see existingInputNamesNote(). (Generate-only
    // mode gets the fuller liveEntityFactsFor() note below instead.)
    m_existingInputTables.clear();
    m_unprintedOutputRetried = false;
    m_singlePartDescription.clear();
    m_resultLineTask.clear();
    m_rejectedSetCodes.clear();
    m_lastFailedStatement.clear();
    if (autoRun)
    {
        QString factsSummary;
        pendingLiveFactsNote =
            existingInputNamesNote(withoutRequestSuffix(aiText), &m_existingInputTables, &factsSummary);
        if (!factsSummary.isEmpty() && m_host)
            m_host->print("▸▸▸ AI: " + factsSummary);
        m_earlierItemsNote = createdByEarlierItemsNote();
        pendingLiveFactsNote += m_earlierItemsNote;
    }
    else
        m_earlierItemsNote.clear();

    // Generate-only mode (autoRun == false) skips existence verification altogether.
    //
    // Nothing this session generated has been executed, so a name the request treats as existing is
    // very often something an EARLIER, equally unexecuted item was going to create — "missing" is
    // the normal state there, not a problem. Confirmed live in a generate-only LIST: item 7's
    // table-creation script was shown and never run, so item 9's check reported
    // "VERDICT: MISSING SAXS_data" for a table the run itself had just declined to create.
    //
    // Three things wrong with checking anyway: it costs a full round-trip (measured at 24.2s on
    // that item) to learn something already implied by "nothing has run"; it executes a script and
    // prints to the console in a mode where the user asked for neither, which is the same contract
    // exploreDisposition() now respects; and its MISSING verdict feeds a refusal that would abort
    // the item outright. The verdict's one useful effect — making the model write defensively — is
    // obtained for free by telling it the situation instead (see existenceUncheckedNote()), and the
    // model already does it unprompted: a generate-only item produced
    // "if n is None: n = newNote(\"Batch\")" with no check at all.
    //
    // The check keeps its full force when scripts WILL run, which is when overwriting or reading a
    // live object can actually destroy something.
    if (m_content && autoRun)
    {
        const QList<QPair<QString, QString>> existingNames = extractExistingEntityNames(aiText);
        // The host answers itself (entityKinds(), its own live state), as the split-plan check does since series 4:
        // no model-written script, no round-trip. Confirmed live (BIGMODEL, ai-test-requests item 9): the check cost a
        // whole ~114K-token call to print "(window) EXISTS: SAXS_data". The model-written script stays the fallback
        // for a host that cannot tell.
        if (!existingNames.isEmpty() && m_host)
        {
            QStringList plainNames;
            for (const QPair<QString, QString> &entry : existingNames)
                plainNames << entry.first;
            const QList<QStringList> kinds = m_host->entityKinds(plainNames);
            if (kinds.size() == plainNames.size())
            {
                static const QStringList windowKinds = {"window", "table", "matrix", "note", "graph", "3D plot"};
                QStringList missing;
                for (int i = 0; i < plainNames.size(); ++i)
                {
                    bool exists = false;
                    // a "function" is compiled or fittable — entityKinds() names those two
                    const QString label = existingNames[i].second;
                    for (const QString &kind : kinds[i])
                        exists = exists || label.isEmpty() ||
                                 (label == "window"     ? windowKinds.contains(kind)
                                  : label == "function" ? (kind == "compile" || kind == "fittable")
                                                        : kind == label);
                    if (!exists)
                        missing << plainNames[i];
                }
                finishExistenceCheck(aiText, missing);
                return;
            }
        }
        if (!existingNames.isEmpty())
        {
            QList<QPair<QString, QStringList>> names;
            for (const QPair<QString, QString> &entry : existingNames)
                names << qMakePair(entry.first, entry.second.isEmpty() ? QStringList() : QStringList{entry.second});
            pendingExistenceCheckRequestText = aiText;
            awaitingExistenceCheckScript = true;
            // expectsCode=false: without it, Integrated mode attaches exploreModeReminder() here too,
            // and confirmed live, the model returned a bare "EXPLORE" instead of the real check script.
            //
            // Topics hint appended explicitly (unlike every other call here, this one had none at
            // all) — under Integrated mode that left this call with zero RAG scoping, falling back
            // to generic unscoped retrieval. baselineDocTopics() ("ai"/"app") only, same as every
            // other preflight-style call in this file — not the full entity-kind list, which would
            // balloon this cheap check to whole-corpus size the way an unrelated single-topic part
            // did (see splitPlanPreflightPrompt()'s "table alongside matrix" rule).
            // includeDataProbe: only worth the extra line when this request will actually USE the
            // data behind a named table — see the virtual's own doc comment for the 0-row incident.
            static const QRegularExpression dataUseRe(R"(\bfit\w*\b|\bplot\w*\b|\bgraph\w*\b|\bchi2\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
            const bool wantsDataProbe = dataUseRe.match(aiText).hasMatch();
            ask(m_content->existingEntityVerificationPrompt(names, wantsDataProbe) + topicsHintLine(QStringList()),
                true, QString(), QStringList(), false, false, false);
            return;
        }
    }
    else
    {
        // Generate-only mode: the facts the skipped round-trip would have produced, read straight
        // out of the host instead — same VERDICT content, no call, nothing executed. See
        // AiContentProvider::liveEntityFactsNote()'s doc comment for why this is stronger than the
        // script it replaces, not merely cheaper.
        QString factsSummary;
        pendingLiveFactsNote = liveEntityFactsFor(aiText, &factsSummary);
        if (!factsSummary.isEmpty() && m_host)
            m_host->print("▸▸▸ AI: " + factsSummary);
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
    const QList<QPair<QString, QString>> checkedNames = extractExistingEntityNames(aiText);
    finishExistenceCheck(aiText, missingEntityNames(result.output, checkedNames));
}

void AiAssistant::finishExistenceCheck(const QString &aiText, const QStringList &missing)
{
    if (!m_host)
        return;
    // Gated on autoRun even though beginRequest() no longer starts a check in generate-only mode: a
    // reply for a check dispatched while auto-run was still on can land here after the flag changed,
    // and refusing the request then would be the one outcome generate-only mode must never produce
    // — "Nothing was created or run" is trivially true there, and the missing name is usually what
    // an earlier unexecuted item would have created.
    if (!missing.isEmpty() && autoRun)
    {
        QStringList quoted;
        for (const QString &n : missing)
            quoted << "\"" + n + "\"";
        m_host->print("▸▸▸ AI: stopping — this request refers to " + quoted.join(", ") + " as already existing, but " +
                      (missing.size() == 1 ? "no match was found for it" : "no match was found for them") +
                      ". Nothing was created or run. Reply with the correct name, or say \"create a new "
                      "one\"/\"pick any\" to explicitly authorize that instead." +
                      // in a LIST: skip this item and go on — confirmed live, without it the list stalled for
                      // good (a missing name that an earlier, failed item should have created)
                      splitModeGiveUpNote());
        return;
    }
    dispatchAfterExistenceCheck(aiText);
}

void AiAssistant::dispatchAfterExistenceCheck(const QString &aiText)
{
    // The continuation-check gate below now runs regardless of splitModeActive — confirmed live:
    // it used to live only in the splitModeActive branch (effectively inside what's now
    // dispatchByMode()), so any non-auto-run request skipped it entirely and, with it, the
    // originalRequestText refresh handleContinuationCheckResponse() performs. A conversation that
    // did one auto-run split request ("new Table") followed by a plain, non-auto-run one ("new
    // Matrix and Graph") left originalRequestText frozen on "new Table" for the second request,
    // which then got prefixed with a misleading "your actual goal is: new Table" reminder and
    // duly regenerated a table nobody asked for that turn. Gating on splitModeActive here instead
    // of inside dispatchByMode() fixes that for both modes at once.
    if (!m_content)
    {
        // No content provider to phrase any preflight with (shouldn't happen in practice — see
        // setContentProvider()'s call sites) — fall back to a plain request instead of firing
        // ask() with an empty prompt and leaving a flag stuck waiting on nothing.
        startRequest(QStringList());
        return;
    }
    // listModeActive: startNextListItem() already advanced m_contextHistoryStart deterministically
    // for this item, so asking the model to judge "new or continuation?" would both be redundant
    // and reintroduce the judgment call that leak came from.
    if (m_history.isEmpty() || listModeActive)
    {
        // Fresh conversation — nothing in history could conflict with this request, so the
        // continuation-check would just be a wasted round-trip. Unchanged from before this check
        // existed.
        dispatchByMode(aiText);
        return;
    }
    // History exists — could belong to this same ongoing task, or to an earlier, unrelated
    // request (see awaitingContinuationCheck's doc comment for the confirmed-live failure this
    // guards against). Defer dispatch until handleContinuationCheckResponse() decides which.
    pendingDispatchRequestText = aiText;
    awaitingContinuationCheck = true;
    // includeHistory=true (unlike every other preflight call in this file): the real, settings-
    // windowed conversation history is exactly the context this judgment needs — not just what was
    // ASKED in the prior turn, but what it actually PRODUCED (e.g. a created object's real name).
    // Confirmed live: an earlier version restated only m_history.last().first (the prior turn's raw
    // request text, e.g. "new note") with no history at all, so a follow-up referencing "the
    // note"/"this note" by name couldn't be recognized as a continuation. Real history also makes
    // originalRequestText moot as an anchor here (it's frozen on the FIRST message of the whole
    // conversation and never updated — the wrong anchor for this judgment regardless).
    // docTopics=QStringList() (not {"ai"}): the actual prompt text carries noDocsHintLine()'s own
    // unconditional "TOPICS: none", so an empty docTopics here keeps the "topics hint: ..."
    // diagnostic (buildSendingDiagnostic()) honest about what's genuinely sent — {"ai"} used to
    // print "topics hint: ai" for a call whose real wire text always says "TOPICS: none".
    ask(m_content->continuationCheckPreflightPrompt(aiText) + noDocsHintLine(), true, QString(), QStringList(), false,
        true, false);
}

void AiAssistant::dispatchByMode(const QString &aiText)
{
    // Confirmed live: "there are spheres?" went to the split planner, whose answer was a script, not a plan — a
    // wasted call, then all docs attached. A question has nothing to decompose.
    if (splitModeActive && isShortQuestion(withoutRequestSuffix(aiText)))
    {
        if (m_host)
            m_host->print("▸▸▸ AI: a question — answering it directly, without a split plan.");
        splitModeActive = false;
    }
    if (splitModeActive)
        beginSplitPlanDecomposition(aiText);
    else
        beginNonSplitRequest(aiText);
}

void AiAssistant::beginNonSplitRequest(const QString &aiText)
{
    // Doc topics are selected mechanically from request keywords; an LLM pre-flight was removed
    // because its answer became irrelevant after keyword matching was added. Zero matches fall
    // through to startRequestWithSelectedTopics()'s empty-list "send everything" fallback.
    startRequestWithSelectedTopics(docTopicsFromRequestText(aiText), QStringLiteral("AI doc topics"));
}

void AiAssistant::beginSplitPlanDecomposition(const QString &aiText)
{
    // The continuation-check gate (history empty? existing conversation?) now lives one level up,
    // in dispatchAfterExistenceCheck() — this only dispatches the decomposition preflight itself,
    // for a request already confirmed (or fresh enough) to decompose right now.
    awaitingSplitPlan = true;
    // docTopics=QStringList() (not {"ai"}): see continuationCheckPreflightPrompt()'s call site
    // just above for why — noDocsHintLine() already puts the real "TOPICS: none" in the prompt
    // text itself, so this only needs to keep the "topics hint: ..." diagnostic honest.
    ask(m_content->splitPlanPreflightPrompt(aiText, docsMode() == DocsMode::Integrated) + noDocsHintLine(), true,
        QString(), QStringList(), false, false, false);
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
        // NOT clearHistory() — the stored transcript is kept in full regardless of this verdict
        // (see m_contextHistoryStart's doc comment); only future calls' CONTEXT excludes turns
        // before this point, not "show history"/the record itself. An earlier version called
        // clearHistory() here, so a later message merely judged "unrelated" permanently erased
        // real earlier turns (e.g. "Hi") a user might still want to recall.
        m_contextHistoryStart = m_history.size();
        if (m_host)
            m_host->print("▸▸▸ AI: this looks like a new, unrelated task — earlier conversation "
                          "won't be resent as context from here on (still available via "
                          "\"history\").");
    }
    // Re-anchor to THIS request as the new baseline goal — unconditionally, regardless of the
    // CONTINUE/NEW verdict above. pendingDispatchRequestText is, by construction, the request
    // dispatchAfterExistenceCheck() deferred (its only writer, and this is the only reader), so
    // it's the correct "actual goal" anchor for whatever happens next even when the check says
    // CONTINUE. Gating this on NEW left originalRequestText stuck on a stale, unrelated earlier
    // task whenever the check misjudged a genuinely new request as a continuation — confirmed
    // live: a "make a new matrix" anchor kept leaking into every retry prompt for an unrelated
    // later 3D-surface-plot request, because CONTINUE was judged and this line never ran.
    originalRequestText = pendingDispatchRequestText;
    // CONTINUE, or anything unrecognized — fail-open (same philosophy as every other
    // verify-and-retry in this class): proceed exactly as the fresh-conversation path would,
    // history intact. dispatchByMode() (not beginSplitPlanDecomposition() directly) since this
    // request may or may not want split mode — see dispatchAfterExistenceCheck()'s doc comment.
    if (!m_content)
    {
        startRequest(QStringList());
        return;
    }
    dispatchByMode(pendingDispatchRequestText);
}

QString AiAssistant::topicsHintLine(const QStringList &docTopics) const
{
    // Always includes baselineDocTopics(), same reasoning as startSplitPart()'s own inline
    // equivalent — this is pure signal for an optional server-side retrieval step, not the
    // "none" LOCAL-attachment choice docTopics itself protects at its own call sites, so it's
    // never gated by that choice. kNoDocsSentinel filtered out — it's a LOCAL attachment
    // placeholder, not a real topic name a retrieval proxy could ever match a file on.
    const QStringList baselineTopics = m_content ? m_content->baselineDocTopics() : QStringList();
    QStringList hintTopics = baselineTopics;
    for (const QString &topic : docTopics)
        if (topic != kNoDocsSentinel && !hintTopics.contains(topic))
            hintTopics << topic;
    if (docsMode() == DocsMode::Integrated && !hintTopics.isEmpty())
        return "\n\nTOPICS: " + hintTopics.join(", ");
    return {};
}

QString AiAssistant::noDocsHintLine() const
{
    return docsMode() == DocsMode::Integrated ? "\n\nTOPICS: none" : QString();
}

void AiAssistant::startRequest(const QStringList &docTopics, const QString &resolvedTaskDescription)
{
    selectedDocTopics = docTopics;
    QString requestForModel;
    m_singlePartDescription = resolvedTaskDescription;
    // The one-part plan's paraphrase goes along only when it names an object the request does not — the case it exists
    // for (a request with no name, the plan settled on one). Otherwise it adds nothing the request does not say and can
    // only contradict it. Confirmed live (split-mode LISTs, 2026-10-11): 10 of 30 one-part items failed against 2 of
    // 18 multi-part ones — "In \"Squares\", give column 3 ..." planned as "create table named 'Squares' with three
    // columns and five rows", "column 2, in text format" planned without "in text format" (the text cell came back
    // empty).
    if (!resolvedTaskDescription.isEmpty() && !namesSomethingNew(resolvedTaskDescription, originalRequestText))
    {
        if (m_host)
            m_host->print("▸▸▸ AI split-plan: the plan's wording names nothing the request does not — the request "
                          "itself is sent.");
        m_singlePartDescription.clear();
    }
    if (!m_singlePartDescription.isEmpty())
    {
        // Called from handleSplitPlanResponse()'s single-part shortcut: a plan that settled on
        // exactly one part is collapsed into a direct (non-split) call to skip split-mode's
        // per-part/merge overhead — but that part's OWN description already went through
        // splitPartsMissingCreationName()'s check, which requires it to name any created object
        // concretely (see that function's doc comment). Confirmed live: discarding this and
        // falling back to the bare original request instead produced a shown split-plan
        // ("create a table named 'MyTable'...") that then contradicted itself when the actual
        // generation call, given only the bare ambiguous original text with no name at all,
        // asked the user to specify a name the plan had already settled on one paragraph earlier.
        // Using the part's resolved description keeps what's shown and what's actually generated
        // consistent. The request itself goes along too: the description is the planner's paraphrase, and whatever it
        // left out was lost — confirmed live, a LIST item's "print exactly ONE line: the label, then only the values"
        // came out as "R6: sum=100.000000, m[2,3]=6.000000, ...", and a requested tab count was dropped
        requestForModel =
            (pendingRequestText == originalRequestText)
                ? ("Request:\n" + originalRequestText +
                   "\n\nGenerate a script for exactly this task — the request above as planned: " +
                   resolvedTaskDescription)
                : ("Original request for this conversation — the actual goal; do not drift "
                   "from its explicitly named resources just because this message is short:\n" +
                   originalRequestText + "\n\nGenerate a script for exactly this task: " + resolvedTaskDescription);
    }
    else
    {
        // Restate the ORIGINAL request explicitly whenever this turn's own text differs
        // from it (i.e. every short follow-up like "try again"/"noch mal") — see
        // originalRequestText's declaration. Without this, the model tends to
        // continue from its own last SCRIPT in history (e.g. a substitute table already
        // identified as wrong) rather than re-deriving the goal from the real request,
        // which appears only once, much earlier in a growing, code-dominated history.
        requestForModel = (pendingRequestText == originalRequestText)
                              ? pendingRequestText
                              : ("Original request for this conversation — the actual goal; do not drift "
                                 "from its explicitly named resources just because this message is short:\n" +
                                 originalRequestText + "\n\nCurrent message:\n" + pendingRequestText);
    }
    // historyText: always the user's own literal current message, never requestForModel — neither
    // the TOPICS: hint appended below nor the "Original request for this conversation..."
    // reminder wrapper built above belong in stored history/"show history". Confirmed live (the
    // TOPICS: case): appending the hint straight onto requestForModel and letting THAT get stored
    // verbatim leaked "TOPICS: ai, app" into m_history, which then kept resurfacing in every later
    // reference to this turn. Confirmed live again (the reminder-wrapper case, previously missed
    // by this same fix): a short follow-up that triggered the "Original request..." wrapper stored
    // that whole multi-line wrapper as the turn's "question" instead of the actual short message
    // typed, corrupting "show history" and every future consumer of m_history's raw text.
    const QString textForHistory = pendingRequestText;

    // Confirmed live: every request that resolves to a single part (handleSplitPlanResponse()'s
    // single-part shortcut calls startRequest(), never startSplitPart()) was reaching the server
    // with files=None — completely unscoped retrieval — despite the client's own "topics hint:
    // ..." diagnostic implying otherwise (that diagnostic describes selectedDocTopics, for LOCAL
    // doc attachment, a different thing entirely from the TOPICS: line topicsHintLine() builds,
    // which is the only thing qtisas-rag's files_from_topics() actually parses).
    requestForModel += topicsHintLine(docTopics);
    requestForModel += pendingLiveFactsNote;

    ask(requestForModel, true, QString(), docTopics, true, true, true, textForHistory);
}

QString AiAssistant::originalRequestReminder() const
{
    if (pendingRequestText == originalRequestText)
        return {};
    return "Original request for this conversation — the actual goal, don't lose sight of it "
           "even as you explore/retry:\n" +
           originalRequestText + "\n\n";
}

// listModeActive stays true only while another item follows, and the next item is dispatched
// immediately below — so there is no idle window where an unrelated manual request's success could
// be misread as the abandoned item finishing (confirmed live before this was handled at all).
QString AiAssistant::splitModeGiveUpNote()
{
    QString note;
    if (splitModeActive)
    {
        note = QString("\n\n(split-mode part %1/%2 — later parts were not attempted; merge was "
                       "never requested.)")
                   .arg(splitPartIndex + 1)
                   .arg(splitPlan.size());
        splitModeActive = false;
    }
    if (listModeActive)
    {
        // Skip the failed item, don't abandon the list: confirmed live four times, one stuck item
        // discarded every remaining item plus all the work already completed (e.g. items 8-12 and
        // ~8min lost to a setParamCount mismatch confined to item 8). Items are independent
        // enough that a later one usually still runs; continued on the next event-loop turn so
        // this failure message is emitted before the next item's own output.
        const bool moreItems = listItemIndex + 1 < listItems.size();
        note += moreItems ? QString("\n\n(LIST item %1/%2 failed — skipping it and continuing with "
                                    "the next item.)")
                                .arg(listItemIndex + 1)
                                .arg(listItems.size())
                          : QString("\n\n(LIST item %1/%2 failed — it was the last item.)")
                                .arg(listItemIndex + 1)
                                .arg(listItems.size());
        if (moreItems)
        {
            QTimer::singleShot(0, this, [this]() {
                if (listModeActive)
                    startNextListItem();
            });
        }
        else
        {
            listModeActive = false;
            listItems.clear();
            listItemIndex = -1;
        }
    }
    return note;
}

void AiAssistant::recordGiveUpInHistory(const QString &reason, const QString &lastError)
{
    m_history.append({"[[internal: automatic retry loop ended]]",
                      "[[internal: this task did NOT complete successfully — " + reason +
                          (lastError.isEmpty() ? QString() : (".\nLast error:\n" + lastError)) +
                          "\nIf asked about this task's status, report that it FAILED or did not finish, "
                          "quoting the actual error above — never claim success.]]"});
}

QStringList splitPartTopicsFromAnswer(const QString &docsField, const QString &description,
                                      const QStringList &knownTopics)
{
    const QString lower = docsField.toLower();

    const QString descLower = description.toLower();
    QStringList keywordMatches;
    for (const QString &topic : knownTopics)
    {
        if (textNamesTopic(descLower, topic))
        {
            keywordMatches << topic;
            continue;
        }
        if (!topicSynonymMatch(topic, description))
            continue;
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
        if (textNamesTopic(lower, topic))
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
    //
    // "3d"/"mesh" added to the noun group — confirmed live, a THIRD wording slipped past even the
    // "surface" fix above: "create 3D plot of sin(x)*cos(y) ... using filled mesh style" has
    // neither "graph" nor "surface", only "plot" as a bare verb with no qualifying noun following
    // it at all. Left unnamed, the resulting window got named after the FORMULA itself
    // ("sin(x)*cos(y)"), and the next part spent 10 rounds guessing plausible-sounding names
    // before one happened to satisfy Direction 5 by coincidence (a word already present in that
    // LATER part's own description) — the eventual "success" still silently created a second,
    // orphaned window rather than ever finding the first one.
    // "curve" added to the noun group, mirroring the same fix in splitPlanUnnecessaryFragments()
    // below — confirmed live: "plot the simulated Porod curve with log-log scale and proper axis
    // formatting" creates a new graph window (via plot()/insertCurve()) exactly like any other
    // plot-creating description, but wasn't recognized as a creation step at all because "curve"
    // wasn't in this noun list, so its missing quoted name went unchecked.
    // "function" made plural-tolerant (function(s)?) — same gap as splitPlanUnnecessaryFragments()'s
    // own copy of this pattern: "compile two custom fit functions" (plural) matched neither
    // singular "function" nor "curves?"'s own explicit plural handling, so a multi-function
    // compile part's missing name went unchecked entirely rather than flagged.
    static const QRegularExpression creationVerbRe(
        R"(\b(compile|create|make|build|plot)\b.*)"
        R"(\b(functions?|table|graph|matrix|note|panels?|surface|3d|mesh|curves?)\b)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression quotedNameRe(R"(['"][^'"]+['"])");
    QList<int> missing;
    for (int i = 0; i < plan.size(); ++i)
        if (creationVerbRe.match(plan[i].description).hasMatch() && !quotedNameRe.match(plan[i].description).hasMatch())
            missing << i;
    return missing;
}

// Maps a matched creation noun (creationVerbRe's captured object-type word) to the labeled
// existence-check line(s) — see existingEntityVerificationPrompt() — whose EXISTS actually means a
// collision is destructive for THAT kind of creation.
//   - a function name colliding with Compiler.functionNames()/Fittable.fitFunctions() overwrites
//     the .fif/.cpp/.so/.dylib files in place, no backup, no confirmation.
//   - table/matrix/note/graph all share ONE underlying namespace — app-python-api.md's own
//     windows() ("all open windows regardless of type") and setWindowName()'s cross-type
//     ValueError confirm QtiSAS never lets two of these coexist under the same name — so there is
//     exactly one real thing to check ("window"), not four independent ones.
//   - table/graph collisions are destructive (Table::clear() wipes every cell; a reused graph
//     clears its curves). Matrix collisions only resize (lower severity). Note collisions look
//     safe ONLY for a same-type match (the existing note is returned untouched) — but confirmed
//     live by code inspection: newNote()'s own SIP binding only checks for an EXISTING NOTE by
//     that name; if a DIFFERENT window type already holds it, that check misses it entirely, and
//     ApplicationWindow::newNote() then silently substitutes a completely different
//     auto-generated name via alreadyUsedName() — no error, no warning, and the script's own note
//     ends up under a name it never asked for. Since the mechanical check can only ask "does ANY
//     window have this name" (not "is the SAME-TYPE window the one holding it"), note gets the
//     same {"window"} label as the others rather than an exemption — an occasional unnecessary
//     halt on a genuinely-safe note reuse is a smaller cost than silently missing this.
static QStringList creationCollisionLabelsForNoun(const QString &noun)
{
    const QString lower = noun.toLower();
    if (lower.startsWith("function"))
        return {"compile", "fittable"};
    // A note's script TAB is in no registry at all — not the shared window-name registry
    // (setWindowName() knows nothing about tabs; they are addressed by renameTab()/tab() index or
    // label), and obviously not the compiled-function one. So there is nothing a tab name can
    // collide WITH, and an empty label list makes the caller skip it entirely.
    //
    // Confirmed live: "create a script tab named 'AutoExecScript' in the 'Batch' note" had no
    // recognized noun at "tab", so the capture ran on to the next one it did recognize — "note" —
    // and checked the TAB's name against the window registry. That printed
    // "(window) MISSING: AutoExecScript" and spent a whole round-trip proving a tab is not a
    // window. Worse, a MISSING verdict is what the existence gate refuses requests on, so a
    // differently-worded part could have had the item aborted outright over a name that was never
    // meant to be a window in the first place.
    // Exact match, NOT startsWith("tab") — that also swallows "table", which stripped the window
    // label off every table creation and silently disabled the overwrite guard for tables. Caught
    // by splitPlanCreationNames_collectsCompileAndTableNames() within a minute of writing it.
    if (lower == "tab" || lower == "tabs")
        return {};
    return {"window"}; // table, matrix, note, graph, panel(s), surface, 3d, mesh, curve(s)
}

// Collects the quoted name of every part the plan is actually about to CREATE, paired with which
// labeled existence-check line(s) a collision on that name would actually need to match to be
// destructive (see creationCollisionLabelsForNoun() above) — checking a name against every one of
// the three kinds regardless of what's being created would both under- and over-block: a compile
// part would wrongly halt on an unrelated pre-existing TABLE sharing the name (compiling never
// touches tables at all), while a note-creation part would need never block at all. Reuses
// creationVerbRe (same as splitPartsMissingCreationName() above) to decide "this part creates
// something", paired with a CAPTURING quoted-name match instead of a presence check. Deliberately
// reuses creationVerbRe rather than a broader "any quoted name in any part" scan — a part that only
// REFERENCES an already-created object by name (e.g. "configure fittable to use function
// 'ai/Gaussian'") must never be treated as if it were about to create that name fresh, or a
// legitimate reuse of a genuinely pre-existing object would wrongly halt the whole plan.
// A part naming more than one distinct object of DIFFERENT kinds in its own description is a rare
// enough shape (the split-plan prompt otherwise pushes toward one creation per part) that every
// quoted name in such a part is approximated as needing the SAME label set, derived from the
// part's own single creationVerbRe match — not worth a more precise per-name noun association.
QList<QPair<QString, QStringList>> splitPlanCreationNames(const QList<AiAssistant::SplitPart> &plan)
{
    // Confirmed live: a split-plan fixup round merged a compile part and a simulate part into ONE
    // ("compile custom Porod power-law fit function ... named 'ai/PorodFit' and simulate curve
    // from it without fitting real data") — greedy .* here matched as much as possible, so
    // creationVerbRe's own noun capture backtracked all the way to the LAST noun in the
    // description ("curve", from the simulate clause) instead of the one actually tied to the
    // compile verb ("function"). That mapped 'ai/PorodFit' to {"graph"} instead of
    // {"compile","fittable"}, so splitPlanCreationCollisions() checked the wrong labeled line,
    // found no "(graph) EXISTS", and silently let a genuine, confirmed compile/fittable collision
    // through uncaught — exactly the destructive case this whole check exists to stop. Lazy .*?
    // instead finds the CLOSEST noun after the verb, which is the one actually describing what
    // that verb creates, regardless of what unrelated creation-shaped words appear later in the
    // same merged description.
    static const QRegularExpression creationVerbRe(
        R"(\b(compile|create|make|build|plot)\b.*?)"
        R"(\b(functions?|tabs?|table|graph|matrix|note|panels?|surface|3d|mesh|curves?)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // Confirmed live: "... function named 'ai/PorodFit' in category 'test' ..." has TWO quoted
    // strings; a bare quote scan wrongly checked "test" for existence too. Restricted to the name
    // following "named"/"called", matching namedQuotedRe's own convention elsewhere in this file.
    // Up to 3 intervening words tolerated (e.g. "the named 3D graph \"X\"") — without it, a quote
    // separated from named/called by a noun/qualifier was never captured at all.
    static const QRegularExpression quotedNameCaptureRe(R"(\b(?:named|called)\b(?:\s+\S+){0,3}?\s+(['"])([^'"]+)\1)",
                                                        QRegularExpression::CaseInsensitiveOption);
    // Confirmed live: "the existing table named \"Metrics\"" captured "Metrics" as a creation
    // target — a read-only reference, not what the part is about to create.
    static const QRegularExpression existingWordRe(R"(\bexisting\b)", QRegularExpression::CaseInsensitiveOption);
    QList<QPair<QString, QStringList>> result;
    for (const AiAssistant::SplitPart &part : plan)
    {
        const QRegularExpressionMatch verbMatch = creationVerbRe.match(part.description);
        if (!verbMatch.hasMatch())
            continue;
        const QStringList labels = creationCollisionLabelsForNoun(verbMatch.captured(2));
        if (labels.isEmpty())
            continue; // no recognized noun matched -- nothing to check
        QRegularExpressionMatchIterator it = quotedNameCaptureRe.globalMatch(part.description);
        while (it.hasNext())
        {
            const QRegularExpressionMatch nameMatch = it.next();
            const int lookbackStart = static_cast<int>(qMax(0, nameMatch.capturedStart(0) - 40));
            const QString lookback = part.description.mid(lookbackStart, nameMatch.capturedStart(0) - lookbackStart);
            if (existingWordRe.match(lookback).hasMatch())
                continue;
            const QString name = nameMatch.captured(2);
            bool merged = false;
            for (QPair<QString, QStringList> &entry : result)
            {
                if (entry.first != name)
                    continue;
                for (const QString &label : labels)
                    if (!entry.second.contains(label))
                        entry.second << label;
                merged = true;
                break;
            }
            if (!merged)
                result << qMakePair(name, labels);
        }
    }
    return result;
}

// Checks each (name, relevantLabels) entry from splitPlanCreationNames() against ONLY the labeled
// per-check line(s) relevant to how that name is actually being created — e.g. a table-creation
// name is a real collision only if the script's own "(window) EXISTS: <name>" line says so; an
// unrelated "(compile) EXISTS: <name>" line for the SAME string is irrelevant, since compiling and
// creating a table touch completely separate storage (confirmed live: a table and a fit function
// CAN legitimately share a name with no conflict at all). Never consults the combined VERDICT
// line — that line answers "does this exist as ANY kind", which is the wrong question here and
// would make this exactly as over-broad as checking all three.
//
// Fails CLOSED, same as missingEntityNames() fails OPEN in the opposite direction, and for the
// mirrored reason: a single matching labeled line is enough to flag a collision, because the
// downside of a false positive here (an unnecessary halt) is far cheaper than the downside of a
// false negative (a silent, unbackuped overwrite of a real file — see
// creationCollisionLabelsForNoun()'s doc comment for which kinds are actually destructive).
//
// Confirmed live: a check-script can print BOTH "(window) EXISTS: X" and "(window) MISSING: X"
// for the same X (a broken/self-contradictory script). A prior version of this function treated
// that as "no evidence, skip" — confirmed live to be the wrong direction: it silently proceeded to
// newTable()'s resize-and-clear path against a name that, in one observed case, genuinely already
// held a real table. EXISTS being present at all is treated as a collision regardless of whatever
// else the same broken script also printed — reverted back to that, deliberately, per the fail-
// CLOSED reasoning above: an unnecessary halt is always the cheaper mistake here.
QStringList splitPlanCreationCollisions(const QString &scriptOutput,
                                        const QList<QPair<QString, QStringList>> &checkedNames)
{
    QStringList collisions;
    for (const QPair<QString, QStringList> &entry : checkedNames)
    {
        for (const QString &label : entry.second)
        {
            if (!scriptOutput.contains("(" + label + ") EXISTS: " + entry.first))
                continue;
            collisions << entry.first;
            break;
        }
    }
    return collisions;
}

// compile-python-api.md Rule 6: "ai/" is reserved, disposable scratch for AI-authored one-offs —
// a collision there is expected reuse, never a destructive surprise, so it must never block.
// Table/matrix/graph names are untouched — Rule 6 says nothing about those.
bool isRule6SandboxExemptCreationName(const QString &name, const QStringList &labels)
{
    return name.startsWith("ai/", Qt::CaseInsensitive) && (labels.contains("compile") || labels.contains("fittable"));
}

// Removes any collision covered by isRule6SandboxExemptCreationName() above, appending each
// removed name to `sandboxExempt` (if non-null).
QStringList splitPlanCollisionsAfterSandboxExemption(const QStringList &collisions,
                                                     const QList<QPair<QString, QStringList>> &checkedNames,
                                                     QStringList *sandboxExempt)
{
    QStringList remaining = collisions;
    for (int i = static_cast<int>(remaining.size()) - 1; i >= 0; --i)
    {
        const QString &name = remaining[i];
        QStringList labels;
        for (const QPair<QString, QStringList> &entry : checkedNames)
            if (entry.first == name)
                labels = entry.second;
        if (!isRule6SandboxExemptCreationName(name, labels))
            continue;
        if (sandboxExempt)
            *sandboxExempt << name;
        remaining.removeAt(i);
    }
    return remaining;
}

// Skips the verification round-trip entirely for a name covered by
// isRule6SandboxExemptCreationName() above — its collision-check answer can never change the
// outcome (Rule 6 lets it through either way), so asking is pure waste. Returns `checkedNames`
// with those entries removed, appending each to `skipped` (if non-null).
QList<QPair<QString, QStringList>> splitPlanCreationNamesNeedingCheck(
    const QList<QPair<QString, QStringList>> &checkedNames, QStringList *skipped)
{
    QList<QPair<QString, QStringList>> remaining;
    for (const QPair<QString, QStringList> &entry : checkedNames)
    {
        if (isRule6SandboxExemptCreationName(entry.first, entry.second))
        {
            if (skipped)
                *skipped << entry.first;
            continue;
        }
        remaining << entry;
    }
    return remaining;
}

// Mechanically applies compile-python-api.md's Rule 6 ("ai/" namespace prefix for any newly
// compiled function) to a compile part's own description, once, right when the split-plan is
// parsed — rather than leaving each generation/retry round of that part to apply the rule
// correctly from doc guidance alone. Confirmed live: every observed compile part used the bare
// name regardless of doc content being present. Only rewrites the name following "named" in a
// "compile ... function" description — this has no equivalent for non-split requests (see
// compilerMissingAiNamespaceMistake()'s doc comment for the mechanical post-hoc check covering
// that case instead).
static void applyAiNamespaceToCompileParts(QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression compileFunctionRe(R"(\bcompile\b.*\bfunction\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression namedQuotedRe(R"(\bnamed\s+(['"])([^'"]+)\1)",
                                                  QRegularExpression::CaseInsensitiveOption);
    for (AiAssistant::SplitPart &part : plan)
    {
        if (!compileFunctionRe.match(part.description).hasMatch())
            continue;
        const QRegularExpressionMatch m = namedQuotedRe.match(part.description);
        if (!m.hasMatch() || m.captured(2).startsWith("ai/"))
            continue;
        part.description.replace(m.capturedStart(2), m.capturedLength(2), "ai/" + m.captured(2));
    }
}

// applyAiNamespaceToCompileParts() above only corrects the COMPILE part's OWN description — it
// does nothing about OTHER parts in the same plan that reference the SAME function by its bare,
// unprefixed name. Confirmed live: a compile part named 'ai/PorodPowerLaw' (the model's own
// choice this time, not even needing the rewrite above) while every one of 5 LATER parts
// referenced the same function as bare 'PorodPowerLaw' — each part individually satisfies
// splitPartsMissingCreationName() (it DOES state a quoted name), so nothing else catches this;
// it's a genuine name MISMATCH across parts, not a missing name. This is exactly the failure
// shape splitPartScopeViolation()'s Direction 3 doc comment already describes causing "a real
// runtime ValueError several parts later" when a later part's generated script ends up opening/
// configuring the WRONG (bare) name while the compiled function actually lives under the
// "ai/"-prefixed one. Rewrites every later part's bare, quoted reference to match the compile
// part's own canonical (correctly "ai/"-prefixed) name, so every part's description is
// internally consistent before any part's script is ever generated.
void reconcileCompiledFunctionNameAcrossParts(QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression compileFunctionRe(R"(\bcompile\b.*\bfunction\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression namedQuotedRe(R"(\bnamed\s+(['"])([^'"]+)\1)",
                                                  QRegularExpression::CaseInsensitiveOption);
    for (const AiAssistant::SplitPart &compilePart : plan)
    {
        if (!compileFunctionRe.match(compilePart.description).hasMatch())
            continue;
        const QRegularExpressionMatch m = namedQuotedRe.match(compilePart.description);
        if (!m.hasMatch() || !m.captured(2).startsWith("ai/", Qt::CaseInsensitive))
            continue;
        const QString canonicalName = m.captured(2);
        const QString bareName = canonicalName.mid(3);
        if (bareName.isEmpty())
            continue;
        const QRegularExpression bareQuotedRe("(['\"])" + QRegularExpression::escape(bareName) + "\\1",
                                              QRegularExpression::CaseInsensitiveOption);
        for (AiAssistant::SplitPart &part : plan)
        {
            if (&part == &compilePart)
                continue;
            part.description.replace(bareQuotedRe, "\\1" + canonicalName + "\\1");
        }
    }
}

// Confirmed live: a fixup round left two non-adjacent parts with byte-identical descriptions
// (would have compiled/simulated the same thing twice) — no other check catches this. Fixed
// mechanically, no model round-trip: nothing to negotiate about two identical strings. Scans
// backward so removal never invalidates an index still to be checked; DOCS are unioned in.
void dedupeSplitPlanExactDuplicateParts(QList<AiAssistant::SplitPart> &plan, QStringList *removedDescriptions)
{
    for (int i = static_cast<int>(plan.size() - 1); i > 0; --i)
    {
        for (int j = 0; j < i; ++j)
        {
            if (plan[i].description != plan[j].description)
                continue;
            for (const QString &topic : plan[i].docTopics)
                if (!plan[j].docTopics.contains(topic))
                    plan[j].docTopics << topic;
            if (removedDescriptions)
                *removedDescriptions << plan[i].description;
            plan.removeAt(i);
            break;
        }
    }
}

// Confirmed live, 3 separate times: asked to invent a different placeholder name for a colliding
// one, the model just re-emitted the identical name unchanged. Once the model-based retry budget
// Folds every part splitPlanUnnecessaryFragments() flags back into the part it belongs to, instead
// of asking the model to do it.
//
// Confirmed live, item 6 of the fixture: "Plot a 3D surface ... then add a legend and axis labels"
// decomposed into "plot the surface" + "add a legend and axis labels to the 3D surface plot", the
// second being pure configuration of what the first just created. The complaint was raised, the
// model resent the SAME plan unchanged, and the loop gave up ("not converging, proceeding anyway")
// — shipping the very fragment it had objected to. Both parts also carried an identical DOCS field,
// which splitPlanAdjacentSameDocs() independently detected and which is suppressed at the complaint
// site precisely because it is the same verdict: these are one action, not two.
//
// The parent is always i-1: splitPlanUnnecessaryFragments() only ever flags a part whose immediate
// predecessor is itself a creation part (or an already-flagged part, chaining backwards). Merging in
// DESCENDING index order therefore keeps every earlier index valid and collapses a chain correctly —
// 3 into 2, then 2 into 1.
void mergeSplitPlanUnnecessaryFragments(QList<AiAssistant::SplitPart> &plan, QStringList *mergedAway)
{
    mergeSplitPlanPartsBackward(plan, splitPlanUnnecessaryFragments(plan), mergedAway);
}

void mergeSplitPlanPartsBackward(QList<AiAssistant::SplitPart> &plan, const QList<int> &parts, QStringList *mergedAway)
{
    const QList<int> &fragments = parts;
    for (int k = static_cast<int>(fragments.size()) - 1; k >= 0; --k)
    {
        const int i = fragments[k];
        if (i <= 0 || i >= plan.size())
            continue;
        const AiAssistant::SplitPart fragment = plan[i];
        AiAssistant::SplitPart &parent = plan[i - 1];
        // ", then <fragment>" keeps the original request's own sequencing readable in the merged
        // description. Lower-casing a leading capital avoids "..., then Add a legend"; guarded on
        // the second character so "3D"/"NaN"-style starts are left intact.
        QString tail = fragment.description;
        if (tail.size() > 1 && tail[0].isUpper() && tail[1].isLower())
            tail[0] = tail[0].toLower();
        parent.description += ", then " + tail;
        // Union the topics, dropping the "no docs" sentinel — it means "this part needs none",
        // which cannot survive a merge with a part that does need some.
        QStringList topics;
        for (const QStringList &src : {parent.docTopics, fragment.docTopics})
            for (const QString &t : src)
                if (t != kNoDocsSentinel && !topics.contains(t))
                    topics << t;
        parent.docTopics = topics.isEmpty() ? QStringList{kNoDocsSentinel} : topics;
        plan.removeAt(i);
        if (mergedAway)
            *mergedAway << fragment.description;
    }
}

// is exhausted, generating the alternate name in C++ instead is guaranteed to actually differ and
// terminates immediately — nothing left to negotiate with the model. Only used for a name the
// fixup process itself invented (never one the user typed), so there is nothing surprising about
// the rename to explain back.
QString mechanicalAlternateSplitPlanName(const QString &oldName, const QList<AiAssistant::SplitPart> &plan)
{
    for (int suffix = 2; suffix < 100; ++suffix)
    {
        const QString candidate = oldName + "_" + QString::number(suffix);
        bool used = false;
        for (const AiAssistant::SplitPart &part : plan)
            if (part.description.contains(candidate))
            {
                used = true;
                break;
            }
        if (!used)
            return candidate;
    }
    return oldName + "_" + QString::number(QDateTime::currentMSecsSinceEpoch());
}

// Rewrites every occurrence of each colliding name across every part's description with a
// Gives every creation part that never named its object a concrete, quoted name — mechanically,
// instead of spending a fixup round asking the model for one.
//
// Confirmed live, item 6 of the test fixture ("Plot a 3D surface of sin(x)*cos(y) ... then add a
// legend and axis labels"): four fixup rounds demanded a quoted name, none produced one, and the
// item then DIED at generation. The deadlock is structural, not a model failure — the request
// names no window, so splitPartScopeViolation()'s Direction 5 (the created window's name must
// appear in the governing description) rejects EVERY possible name, including the two the model
// tried. Asking harder cannot fix that; supplying the name can, and it closes both halves at once:
// no complaint to raise, and a description Direction 5 can match against.
//
// It also makes the part visible to the creation-collision check: splitPlanCreationNames() keys off
// quoted names, so an unnamed creation part was silently exempt from the overwrite guard entirely.
//
// The name must be plain alphanumeric — setWindowName() rejects anything else, which is exactly why
// encoding the formula ("sin(x)*cos(y)") can never work. Preferred nouns first so the name means
// something ("Surface" for a 3D surface), then any ordinary word from the description, then a
// generic fallback; uniqueness across the plan reuses mechanicalAlternateSplitPlanName().
void assignSplitPlanCreationNames(QList<AiAssistant::SplitPart> &plan, QStringList *assigned)
{
    static const QStringList kPreferredNouns = {"surface", "mesh", "map",   "curve",  "peak", "scan", "results",
                                                "graph",   "plot", "table", "matrix", "note", "fit",  "data"};

    // Deliberately excludes the words that describe the ACTION or its styling rather than the
    // object — a window called "Filled" or "Using" would be worse than useless to a later part.
    static const QStringList kStopWords = {"create", "creates", "make", "build",  "generate", "plot",
                                           "plots",  "add",     "adds", "using",  "with",     "from",
                                           "over",   "into",    "and",  "the",    "for",      "style",
                                           "filled", "range",   "axis", "labels", "legend",   "new"};
    static const QRegularExpression wordRe(R"(\b([A-Za-z][A-Za-z0-9]{2,})\b)");
    for (int idx : splitPartsMissingCreationName(plan))
    {
        const QString desc = plan[idx].description;
        QString base;
        for (const QString &noun : kPreferredNouns)
        {
            static const QRegularExpression::PatternOptions ci = QRegularExpression::CaseInsensitiveOption;
            if (QRegularExpression("\\b" + noun + "\\b", ci).match(desc).hasMatch())
            {
                base = noun;
                break;
            }
        }
        if (base.isEmpty())
        {
            QRegularExpressionMatchIterator it = wordRe.globalMatch(desc);
            while (it.hasNext())
            {
                const QString word = it.next().captured(1);
                if (!kStopWords.contains(word.toLower()))
                {
                    base = word;
                    break;
                }
            }
        }
        if (base.isEmpty())
            base = QStringLiteral("Result");
        base[0] = base[0].toUpper();
        QString name = base;
        bool taken = false;
        for (const AiAssistant::SplitPart &part : plan)
            if (part.description.contains("\"" + name + "\""))
            {
                taken = true;
                break;
            }
        if (taken)
            name = mechanicalAlternateSplitPlanName(base, plan);
        plan[idx].description += QString(" named \"%1\"").arg(name);
        if (assigned)
            *assigned << name;
    }
}

// mechanically-generated alternate (see mechanicalAlternateSplitPlanName() above), collecting
// "<old> -> <new>" into `renames` (if non-null) for the print line callers show the user.
void mechanicallyRenameSplitPlanCollisions(QList<AiAssistant::SplitPart> &plan, const QStringList &collidingNames,
                                           QStringList *renames)
{
    for (const QString &oldName : collidingNames)
    {
        const QString newName = mechanicalAlternateSplitPlanName(oldName, plan);
        for (AiAssistant::SplitPart &part : plan)
            part.description.replace(oldName, newName);
        if (renames)
            *renames << oldName + " -> " + newName;
    }
}

// splitPlanPreflightPrompt() extensively instructs naming consistency and when NOT to
// over-fragment, but never requires the parts to collectively cover every distinct action the
// original request actually asks for — nothing downstream checked this either. Confirmed live,
// severe: a request combining "create a table... compile two fit functions... fit both...
// compare chi2/dof... plot the winner..." was decomposed into a SINGLE part covering only "create
// table + compile" — the fit/compare/plot clauses never appeared in ANY part, yet the run
// completed after that one part and reported "all 1 part(s) completed" as if the whole request
// had been fulfilled. Deliberately narrow, high-precision action-verb groups (rather than a
// general clause-coverage NLP heuristic) to keep false positives rare: each group only fires if
// its keyword(s) appear in the ORIGINAL REQUEST TEXT but no synonym from that SAME group appears
// anywhere across the combined part descriptions — a request that never mentions "compare" at all
// is untouched by that group regardless of how the plan is split. Generic/host-independent (plain
// English action words, no QtiSAS-specific API knowledge needed), same reasoning as
// splitPartsMissingCreationName() above for living here rather than behind AiContentProvider.
QStringList splitPlanMissingRequestedActions(const QString &originalRequestText,
                                             const QList<AiAssistant::SplitPart> &plan)
{
    struct ActionGroup
    {
        QString label;
        QRegularExpression re;
        QRegularExpression requestAlso; // the request must match this too (default: anything)
    };
    static const QList<ActionGroup> groups = {
        {"compile", QRegularExpression(R"(\bcompile\b)", QRegularExpression::CaseInsensitiveOption)},
        // Negative lookahead excludes "fit function"/"fitting function" — naming WHAT was
        // compiled (a noun phrase), not describing the fitting ACTION itself. Confirmed live: a
        // part's description said "compile custom Gaussian peak fit function..." (compiling only)
        // with no part anywhere actually performing a fit — without this exclusion, that same
        // phrase would have falsely satisfied the "fit" group and hidden a genuine coverage gap.
        //
        // Negative lookbehind excludes "without/never/not fitting" — confirmed live, severe: a
        // request explicitly saying "...simulate a curve from it without fitting any real data"
        // still matched this group (on "fitting"), so the retry loop demanded a part that
        // performs fitting — a requirement directly CONTRADICTING what the request actually
        // asked for. Structurally unsatisfiable: no plan can simultaneously honor "without
        // fitting" and add a fitting part, so this false positive burned the entire fixup-round
        // budget every single time this phrasing occurred, every round ending in "still has
        // unresolved issue(s) — proceeding anyway" rather than ever converging.
        // "fit function X TO table Y" still fits — confirmed live (benchmark LIST 5, item 1), a part "Fit function
        // \"ai/Quad\" to table \"Calib\" ..." was reported as never covering the requested fit
        {"fit", QRegularExpression(R"((?<!\b(?:without|never|not)\s)\bfit(?:ting)?\b)"
                                   R"((?!\s+functions?\b(?![^.\n]{0,80}?\bto\b)))",
                                   QRegularExpression::CaseInsensitiveOption)},
        // Confirmed live: no trailing \b + \w* let this match "model" mid-word inside
        // "sasviewmodels" — requiring "model(s)" as its own word fixes that false positive.
        // "the function with the smaller chi2/dof" compares too — confirmed live (benchmark LIST 5, item 2), a part
        // "fit both ..., compare chi2/dof" was flagged as never requested, for a request that chose by the smaller one
        {"compare models / report a winner",
         QRegularExpression(R"(\bcompar(?:e|ison|ing)\b|\bwhich\s+models?\b|\bfits?\s+better\b|\bwinner\b|)"
                            R"(\b(?:smaller|larger|lower|higher|smallest|largest|lowest|highest|best|better)\s+)"
                            R"((?:chi2|chi²|chi-?squared?|r2|r²)\b)",
                            QRegularExpression::CaseInsensitiveOption)},
        // "plot3D"/"graph3D" (no space) confirmed live to NOT match \bplot\b/\bgraph\b — "t"/"h"
        // immediately followed by a digit is not a \b transition (digits count as \w too), so a
        // request literally saying "new plot3D" was wrongly flagged as never having asked for a
        // plot/graph action at all. "3d"/"3D" as an optional suffix on both words closes this.
        {"plot/graph", QRegularExpression(R"(\bplot(?:ting|3d)?\b|\bgraph(?:3d)?\b|\bchart\b)",
                                          QRegularExpression::CaseInsensitiveOption)},
        // Confirmed live, severe: "...simulate a curve from it without fitting any real data" —
        // the simulate part was silently dropped during a fixup round, leaving only the compile
        // part, and nothing caught it since "simulate" wasn't a tracked action group at all.
        {"simulate", QRegularExpression(R"(\bsimulat(?:e|ion|ing)\b)", QRegularExpression::CaseInsensitiveOption)},
        // Confirmed live (LIST 5, item 5): "the "make" tab holds a script that creates ..." became "create the note
        // with two tabs" + "execute the make tab" — no part wrote any script into a tab, so there was nothing to run.
        // Only a WRITING verb before "script" counts: "execute the script in the make tab" is no such part.
        {"write the scripts into the note's tabs",
         QRegularExpression(R"(\b(?:holds?|contains?|writes?|writing|sets?|puts?|adds?|adding|with)\b[^.\n]{0,40})"
                            R"(\bscripts?\b)",
                            QRegularExpression::CaseInsensitiveOption),
         QRegularExpression(R"(\btabs?\b)", QRegularExpression::CaseInsensitiveOption)},
    };
    QString combinedDescriptions;
    for (const AiAssistant::SplitPart &p : plan)
        combinedDescriptions += p.description + "\n";
    QStringList missing;
    for (const ActionGroup &g : groups)
    {
        if (!g.re.match(originalRequestText).hasMatch() || !g.requestAlso.match(originalRequestText).hasMatch())
            continue;
        if (!g.re.match(combinedDescriptions).hasMatch())
            missing << g.label;
    }
    return missing;
}

// Detects a "(with X)"-style qualifying detail in the ORIGINAL request whose key word never
// appears anywhere across the combined part descriptions — confirmed live: "...sphere form factor
// (with background) plus 4% Gaussian noise" produced a part description reading "...for sphere
// form factor plus 4% Gaussian noise" — "(with background)" silently disappeared already at this
// decomposition step, before any code was ever generated (see splitPlanPreflightPrompt()'s own
// rule on this). Checks only the FIRST significant word of the detail (e.g. "background" out of
// "background subtraction") rather than the full phrase — a full-phrase match would be too strict
// against reasonable paraphrasing.
QStringList splitPlanDroppedParentheticalDetail(const QString &originalRequestText,
                                                const QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression parentheticalRe(R"(\(with\s+([^()]+)\))",
                                                    QRegularExpression::CaseInsensitiveOption);
    QString combinedDescriptions;
    for (const AiAssistant::SplitPart &p : plan)
        combinedDescriptions += p.description + "\n";
    QStringList missing;
    QRegularExpressionMatchIterator it = parentheticalRe.globalMatch(originalRequestText);
    while (it.hasNext())
    {
        const QString detail = it.next().captured(1).trimmed();
        const QString keyWord = detail.split(' ', Qt::SkipEmptyParts).value(0);
        if (keyWord.isEmpty())
            continue;
        const QRegularExpression keyWordRe(QRegularExpression::escape(keyWord),
                                           QRegularExpression::CaseInsensitiveOption);
        if (!keyWordRe.match(combinedDescriptions).hasMatch())
            missing << ("\"(with " + detail + ")\"");
    }
    return missing;
}

// The mirror image of splitPlanMissingRequestedActions() above: that check catches a requested
// action MISSING from the plan (under-coverage); this one catches a PART whose action was never
// requested at all (over-coverage). Confirmed live, severe: a request that only said "Create a
// table ... and fill it with synthetic ... intensities ... plus 3% Gaussian noise" — zero mention
// of plotting, fitting, comparing, or graph styling — was decomposed into a 9-part plan where
// parts 2-9 built an entire unrequested SAXS analysis pipeline (plot, fit, overlay, legend,
// log-log scale, ticks, scientific notation). Every OTHER mechanical check available at the time
// passed cleanly on that plan (each part was well-named, none was a pure fill-only fragment of
// another) precisely because none of them ever compares a part's action against the ORIGINAL
// REQUEST rather than against the rest of the plan. Same deliberately narrow, high-precision
// action-verb groups as splitPlanMissingRequestedActions() (duplicated rather than shared, same
// reasoning as requestedCreationNeverHappenedMistake()/selfReviewCreationNameSubstitution()'s
// separate CreationKind lists in QtiSasAiContent.cpp) to keep false positives rare — a part doing
// something none of these four groups recognizes (e.g. "create a table") is never flagged here
// regardless of whether it was requested, exactly like its sibling.
QList<int> splitPlanUnrequestedActions(const QString &originalRequestText, const QList<AiAssistant::SplitPart> &plan)
{
    struct ActionGroup
    {
        QString label;
        QRegularExpression re;
    };
    static const QList<ActionGroup> groups = {
        {"compile", QRegularExpression(R"(\bcompile\b)", QRegularExpression::CaseInsensitiveOption)},
        // "fit function X TO table Y" still fits — confirmed live (benchmark LIST 5, item 1), a part "Fit function
        // \"ai/Quad\" to table \"Calib\" ..." was reported as never covering the requested fit
        {"fit", QRegularExpression(R"((?<!\b(?:without|never|not)\s)\bfit(?:ting)?\b)"
                                   R"((?!\s+functions?\b(?![^.\n]{0,80}?\bto\b)))",
                                   QRegularExpression::CaseInsensitiveOption)},
        // "the function with the smaller chi2/dof" compares too — confirmed live (benchmark LIST 5, item 2), a part
        // "fit both ..., compare chi2/dof" was flagged as never requested, for a request that chose by the smaller one
        {"compare models / report a winner",
         QRegularExpression(R"(\bcompar(?:e|ison|ing)\b|\bwhich\s+models?\b|\bfits?\s+better\b|\bwinner\b|)"
                            R"(\b(?:smaller|larger|lower|higher|smallest|largest|lowest|highest|best|better)\s+)"
                            R"((?:chi2|chi²|chi-?squared?|r2|r²)\b)",
                            QRegularExpression::CaseInsensitiveOption)},
        // "plot3D"/"graph3D" (no space) confirmed live to NOT match \bplot\b/\bgraph\b — "t"/"h"
        // immediately followed by a digit is not a \b transition (digits count as \w too), so a
        // request literally saying "new plot3D" was wrongly flagged as never having asked for a
        // plot/graph action at all. "3d"/"3D" as an optional suffix on both words closes this.
        {"plot/graph", QRegularExpression(R"(\bplot(?:ting|3d)?\b|\bgraph(?:3d)?\b|\bchart\b)",
                                          QRegularExpression::CaseInsensitiveOption)},
        // Kept in sync with splitPlanMissingRequestedActions()'s own "simulate" group above.
        {"simulate", QRegularExpression(R"(\bsimulat(?:e|ion|ing)\b)", QRegularExpression::CaseInsensitiveOption)},
    };
    QList<int> unrequested;
    for (int i = 0; i < plan.size(); ++i)
        for (const ActionGroup &g : groups)
            if (g.re.match(plan[i].description).hasMatch() && !g.re.match(originalRequestText).hasMatch())
            {
                unrequested << i;
                break;
            }
    return unrequested;
}

// A part whose ENTIRE job is filling/configuring/finishing what the IMMEDIATELY PRECEDING part
// just created, with no other distinct action of its own, should have been ONE part per
// splitPlanPreflightPrompt()'s own worked example ("create a table and immediately fill it with
// computed values" is ONE part, not two: filling has no independent value without the creation,
// and both would need to re-run together on any failure regardless). Confirmed live: a plan split
// "create a 50x50 matrix named 'X'" from "fill every cell of 'X' using formula ... via
// setFormula()/calculate()" into two parts — part 1's own generated script redundantly finished
// part 2's job too (nothing else for a bare "create" part to legitimately do), and the eventual
// merge step then folded the two back together WRONG (a duplicate newMatrix() call instead of one
// clean creation) rather than the single coherent part this always should have been.
//
// Deliberately narrow — matches ONLY a part whose description STARTS with a fill/populate/
// calculate/configure verb and contains no other action word, immediately following a part that
// created an object of the matching kind — to keep false positives rare: a part that also fits,
// compiles, or plots (a genuinely distinct, independently-durable action) is a legitimate separate
// part even if it touches the same object, and is deliberately not flagged.
//
// fillOnlyRe originally only covered table/matrix-filling wording (fill/populate/calculate/
// compute) — confirmed live: a plan split "plot a 3D surface ... using filled mesh style" from
// "add axis labels and a legend to that same graph" into two parts, and this check never even
// looked closer at the second part, since "add" wasn't in the verb list — styling/configuring a
// just-created graph has exactly the same "no independent value without the creation" property as
// filling a just-created table, just with different verbs. Added add/set/label/style/configure to
// cover it. otherActionRe's own word list dropped bare "graph" for the same incident: descriptions
// overwhelmingly use "graph" as a NOUN referring to the thing just created ("that same graph",
// "the 3D surface graph"), not as a verb — matching it here wrongly treated every such reference
// as "doing something else distinct" and excluded the flag. "plot" already covers the genuine
// verb sense ("plot the results too").
//
// creationRe originally recognized only compile/create/make/build — confirmed live, still missed
// the SAME incident above even after the fillOnlyRe/otherActionRe fixes: "plot the 'SurfaceData'
// table as a 3D surface using filled mesh style" uses "plot", not any of those four, so the part
// BEFORE the styling one was never recognized as a creation step at all, and the loop's
// `!creationRe.match(plan[i-1]...)` guard skipped the pair before ever reaching fillOnlyRe/
// otherActionRe. splitPartsMissingCreationName() (below) already learned this exact lesson for a
// different purpose — its creationVerbRe includes "plot" plus panels?/surface/3d/mesh in the noun
// group for the same reason ("create 3D plot of sin(x)*cos(y) ... using filled mesh style" has
// neither "graph" nor "table"). Mirrored here instead of inventing a narrower variant.
QList<int> splitPlanUnnecessaryFragments(const QList<AiAssistant::SplitPart> &plan)
{
    // "curve" added to the noun group — confirmed live: "plot the simulated Porod curve with
    // log-log scale and proper axis formatting" was never recognized as a creation step at all
    // (no "graph"/"table"/etc, only "curve", which wasn't in this list), so the styling-only parts
    // that followed it never even got their PRECEDING part checked as a creation step in the first
    // place — the whole chain below it went uninspected.
    // "function" made plural-tolerant (function(s)?) — "compile two custom fit functions" (plural,
    // the natural phrasing for a multi-function compile part) matched neither "function" nor
    // "curves?"'s own explicit plural handling, so this creation step went unrecognized the same
    // way the singular-only "curve" gap once did, silently skipping the fill-only check for
    // whatever part follows it.
    static const QRegularExpression creationRe(
        R"(\b(compile|create|make|build|plot)\b.*)"
        R"(\b(functions?|table|graph|matrix|note|panels?|surface|3d|mesh|curves?)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // "finalize" added — confirmed live: "finalize the graph styling including line style and
    // color" is exactly the same fill-only shape as "add"/"style"/"configure", just a different
    // verb never covered before.
    static const QRegularExpression fillOnlyRe(
        R"(^\s*(fill|populate|calculate|compute|add|set|label|style|configure|finalize)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // "plot" here is meant to catch a filler part that ALSO does its own genuine plotting
    // (verb usage, e.g. "plot the residuals too") — not a filler part that merely REFERS to the
    // graph a preceding part already created (noun usage, e.g. "add a legend to the Porod plot",
    // "tick settings for the log-log plot"). Confirmed live: bare \bplot\b matched both, so every
    // styling part in a real plot→title→scale→ticks→style chain that happened to end its
    // description with "... plot" was wrongly treated as if it performed its own distinct
    // plotting action, and none of them were ever flagged. The negative lookahead excludes "plot"
    // when it is the description's trailing content word (optionally followed by one punctuation
    // mark) — the confirmed noun-usage shape — while still matching genuine mid-sentence verb
    // usage ("plot the results too") unchanged.
    //
    // Confirmed live, a SECOND noun-usage shape the above alone still missed: once split-plan
    // descriptions started repeating the created object's exact name in every later part (per the
    // naming-consistency instructions elsewhere in this same prompt), "plot" stopped being
    // sentence-trailing at all — "...to the Porod curve plot named 'PorodPowerLaw'" and "...for
    // the plot named 'PorodPowerLaw'" both have real content AFTER "plot", so the original
    // trailing-word check no longer excluded them, and a 3-part styling chain went entirely
    // unflagged again. Extended to also exclude "plot" immediately followed by an optional
    // "named '<X>'"/"called '<X>'" clause before the end. A THIRD shape in the SAME incident:
    // "the plot's major and minor tick settings" — the possessive "plot's" is unambiguously a
    // noun reference (nothing conjugates as "plot's" as a verb), so it's excluded outright,
    // regardless of position in the sentence.
    //
    // Confirmed live, a FOURTH noun-usage shape the above three still missed: "configure plot
    // with log-log axes, axis labels, and title" — here "plot" is the direct OBJECT of this
    // part's own leading filler verb ("configure PLOT"), not trailing, not followed by a naming
    // clause, not possessive — none of the three existing exclusions applied, so this exact
    // description (part of a real compile→simulate→plot→configure→legend chain) went unflagged
    // and the whole styling tail was never merged. Excludes "plot" whenever it immediately
    // follows one of fillOnlyRe's own verbs (the same list, reused here) — "configure/set/
    // style/... PLOT" is reliably a noun-object in that position, regardless of what comes after
    // it, unlike "plot the residuals too" where "plot" itself is the leading verb.
    //
    // Fifth shape, confirmed live: "...3D surface plot 'surface_data'" — a trailing quoted name
    // with no "named"/"called" at all. Made that keyword optional in the trailing-name clause.
    //
    // Sixth shape, confirmed live: bare \bfit\b never matches the natural plural "fits" ("configure
    // and run FITS using Gaussian and Lorentzian models..." — no word boundary between "fit" and
    // the trailing "s"). A part whose own description used the plural got wrongly treated as having
    // no distinct action of its own and force-merged into a PRECEDING "compile a custom fit
    // function" part, even though running an actual fit against data is a genuinely distinct,
    // independently valuable action, not a mere continuation of compiling the function definition
    // — exactly the "also fits, compiles, or plots" exemption this whole regex exists to grant, just
    // missed by the singular-only pattern. Made fit(s?) plural-tolerant, same as compile/compare;
    // kept the "not followed by 'function(s)'" exclusion so "compile a custom ... fit function"
    // itself still doesn't count as a genuine fitting action here.
    static const QRegularExpression otherActionRe(
        R"(\bfits?\b(?!\s+functions?)|\b(compiles?|compares?)\b|)"
        R"((?<!\b(?:fill|populate|calculate|compute|add|set|label|style|configure|finalize)\s)\bplots?\b)"
        R"((?!'s)(?!(?:\s+(?:(?:named|called)\s+)?['"][^'"]*['"])?\s*[.,]?\s*$))",
        QRegularExpression::CaseInsensitiveOption);

    // Chains forward past the first filler: a creation step is often followed by SEVERAL
    // fill-only parts in a row (create plot → title/legend → axis scale → ticks → line style),
    // and only the first one has a PRECEDING part that itself matches creationRe — the rest only
    // continue what an EARLIER part created, not what the immediately preceding filler did.
    // Confirmed live: without this, a 5-part styling chain after one plot-creation part had only
    // its first link flagged, leaving 3 more unnecessary fragments unreported to the retry prompt.
    QList<int> unnecessary;
    for (int i = 1; i < plan.size(); ++i)
    {
        const bool precedingIsCreationOrChainedFiller =
            creationRe.match(plan[i - 1].description).hasMatch() || unnecessary.contains(i - 1);
        if (!precedingIsCreationOrChainedFiller)
            continue;
        if (!fillOnlyRe.match(plan[i].description).hasMatch())
            continue;
        if (otherActionRe.match(plan[i].description).hasMatch())
            continue;
        unnecessary << i;
    }
    return unnecessary;
}

// Mechanical backstop for rule 6 ("never leave two parts where the LATER one needs no NEW DOCS
// ground beyond the EARLIER one") — originally a pure equality check on adjacent parts' topic
// lists. Confirmed live: a round-1 exact-equality match ([ai, app, fittable] on both a "run the
// fits" and a "compare chi2/dof" part) got the model to restructure the plan, but the later part's
// topics ended up [ai, app, fittable] against the earlier part's [ai, app, fittable, table] — a
// subset, not an exact match — so this check went quiet from round 2 onward even though the two
// parts still did the identical comparison twice, by two different methods, in the final merged
// script. Widened to also catch the LATER part's topics being a (non-strict) subset of the
// EARLIER's — the later part bringing no new documentation ground is the actual redundancy signal.
//
// Deliberately ONE-DIRECTIONAL, not "subset in either direction" — confirmed live this matters: a
// "configure fittable and run both fits" part ([fittable]) followed by a "compare chi2/dof and plot
// into a graph" part ([fittable, graph]) got flagged here for 3 fixup rounds straight (the model
// correctly never complied, since there was nothing wrong to fix) purely because the EARLIER part's
// topics happened to be a subset of the LATER's. That's the opposite signal from the confirmed
// incident above: the later part needs something ADDITIONAL (graph) the earlier one didn't, which
// means it's doing genuinely more work, not re-deriving the same thing — comparing/plotting fit
// results is legitimately separate work from producing them. Only "later ⊆ earlier" indicates
// redundancy; "earlier ⊆ later" indicates the opposite. "none"/ambiguous parts are excluded either
// side.
QList<int> splitPlanAdjacentSameDocs(const QList<AiAssistant::SplitPart> &plan)
{
    QList<int> same;
    for (int i = 1; i < plan.size(); ++i)
    {
        const QStringList &prev = plan[i - 1].docTopics;
        const QStringList &cur = plan[i].docTopics;
        const bool prevNoneOrAmbiguous = prev.isEmpty() || prev == QStringList{kNoDocsSentinel};
        const bool curNoneOrAmbiguous = cur.isEmpty() || cur == QStringList{kNoDocsSentinel};
        if (prevNoneOrAmbiguous || curNoneOrAmbiguous)
            continue;
        const QSet<QString> prevSet(prev.cbegin(), prev.cend());
        const QSet<QString> curSet(cur.cbegin(), cur.cend());
        if ((curSet - prevSet).isEmpty())
            same << i;
    }
    return same;
}

// Mechanical backstop for the (previously prose-only, always-on, never actually pinpointing a
// part) rule that a part whose DOCS list includes fittable must ACTUALLY call into Fittable itself
// (configure/run/simulate a fit) — merely compiling a new function's definition via Compiler never
// needs it. Confirmed live: "compile custom fit functions 'ai/Gaussian' and 'ai/Lorentzian'" (a
// pure Compiler action, no Fittable call anywhere in it) kept DOCS: [..., fittable, compile] for 3
// fixup rounds straight despite the always-on prose rule saying exactly this. The downstream
// damage wasn't cosmetic either: splitPlanAdjacentSameDocs() then saw the NEXT part's real fittable
// work as needing no topic beyond this compile-only part's (inflated) topic set, and kept
// generating a "merge them" complaint that was actually WRONG advice — merging would have thrown
// away the genuine, correct separation between compiling and fitting. A specific, numbered
// complaint naming the exact violating part (matching every OTHER rule in this fixup loop, all of
// which converge far more reliably than bare always-on prose alone) is the fix, same lesson as
// every other checker in this file.
QList<int> splitPlanCompileOnlyHasFittable(const QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression compileActionRe(R"(\bcompil\w*\b)", QRegularExpression::CaseInsensitiveOption);
    // Genuine Fittable usage — configuring/running/simulating a fit via Fittable itself, as opposed
    // to merely compiling a function's definition via Compiler. "fit(s)" excludes a trailing
    // "function(s)" the same way otherActionRe's own fit alternative does, for the same reason:
    // "compile a custom ... fit function" contains the substring "fit" but performs no fitting
    // action at all.
    static const QRegularExpression genuineFittableRe(
        R"(\bconfigur\w*\b|\brun\b|\bperform\w*\b|\bsimulat\w*\b|\bfits?\b(?!\s+functions?))",
        QRegularExpression::CaseInsensitiveOption);

    QList<int> flagged;
    for (int i = 0; i < plan.size(); ++i)
    {
        if (!plan[i].docTopics.contains(QStringLiteral("fittable"), Qt::CaseInsensitive))
            continue;
        if (!compileActionRe.match(plan[i].description).hasMatch())
            continue;
        if (genuineFittableRe.match(plan[i].description).hasMatch())
            continue;
        flagged << i;
    }
    return flagged;
}

// A part whose ENTIRE job is comparing/deciding/reporting a winner, with no NEW named/durable
// object of its own, has no independent value in isolation — the same "no independent value on its
// own" principle splitPlanUnnecessaryFragments() already applies to fill-only styling parts (merge
// into whatever CREATED the thing being styled), just pointed the other way for a decision step:
// a pure comparison exists ONLY to feed a later decision (which curve to plot, which result to
// report), so it belongs with whatever CONSUMES that decision, never with whatever merely produced
// the values being compared. Reasoned, not yet confirmed live: split-execution's "later parts can't
// share Python variables, only printed output" rule means that if a "compare X vs Y, pick a winner"
// part is left standing alone between the part that fitted X/Y and the part that plots the winner,
// the SAME comparison gets redone twice — once here to report it, once again inside the plotting
// part's own forced re-derivation from the fitCurve/result tables (it has no other way to know
// which one to plot). Merging compare INTO its consumer computes the decision exactly once and
// reuses it for both the report and the plot-curve selection. Reuses the "compare models / report a
// winner" wording already defined inline in unrequestedActionGroups() above (duplicated rather than
// shared, matching this file's existing pattern of small regex duplication across independent
// checkers — see splitPlanUnnecessaryFragments()'s own comment on mirroring creationRe rather than
// inventing a narrower variant).
QList<int> splitPlanCompareOnlyMergesForward(const QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression compareOnlyRe(
        R"(\bcompar(?:e|ison|ing)\b|\bwhich\s+models?\b|\bfits?\s+better\b|\bwinner\b)",
        QRegularExpression::CaseInsensitiveOption);
    // If the compare-only part ALSO creates its own named object (e.g. "compare X and Y, then plot
    // the winner into graph 'Z'" as one already-merged description), it's not a bare decision step
    // anymore and this check doesn't apply — creationRe mirrored from splitPlanUnnecessaryFragments.
    static const QRegularExpression createsOwnObjectRe(
        R"(\b(compile|create|make|build|plot)\b.*)"
        R"(\b(functions?|table|graph|matrix|note|panels?|surface|3d|mesh|curves?)\b)",
        QRegularExpression::CaseInsensitiveOption);

    QList<int> forwardMergeIndices; // index i: part i should merge FORWARD into part i+1
    for (int i = 0; i + 1 < plan.size(); ++i)
    {
        if (!compareOnlyRe.match(plan[i].description).hasMatch())
            continue;
        if (createsOwnObjectRe.match(plan[i].description).hasMatch())
            continue;
        forwardMergeIndices << i;
    }
    return forwardMergeIndices;
}

// Mechanical backstop for rule 4 ("never invent a table/matrix when the request already gives an
// analytical formula") — rule 4 only ever reaches the model on the FIRST preflight call;
// buildSplitPlanFixupRetryPrompt() has its own separate, fixed rule list that never repeats it,
// so a table/matrix invented in round 1's own FINAL answer had no way to be caught in rounds 2-3.
// Confirmed live: rule 4's own reworded text still didn't stop it happening in the real app even
// though isolated single-call tests converged clean — this checker gives the fixup loop a
// complaint to act on, which rounds 2-3 otherwise never had for this failure shape at all.
// The consuming part doesn't reliably repeat the table/matrix's own quoted name (rule 9's naming-
// consistency instruction is often ignored — confirmed live elsewhere in this file) — e.g. "plot
// 3D surface named 'X' using the table data" never re-mentions the table's own name at all. Keyed
// on plot+table/matrix co-occurrence instead of exact-name matching for that reason.
QList<int> splitPlanUnnecessaryDataTable(const QString &originalRequestText, const QList<AiAssistant::SplitPart> &plan)
{
    static const QRegularExpression formulaRe(R"(\b(?:sin|cos|tan|exp|log|sqrt)\s*\(\s*[xy]\s*\))",
                                              QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tableCreationRe(
        R"(\b(?:create|make|build|generate|construct|produce|set\s*up)\b.*\b(?:table|matrix)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // No longer requires "table"/"matrix" to appear in the CONSUMING part's own description —
    // confirmed live: "plot 3D surface of sin(x)*cos(y)... named 'surface_plot'" never says
    // "table" anywhere, so the old plotUsingDataRe (which required that word) never matched, and a
    // real 40,000-row table got built and then silently never used — the final newPlot3D() call
    // plotted straight from the formula string regardless. The outer originalRequestText guards
    // below already establish the table was never actually requested, so any later plotting part
    // is enough on its own; requiring it to also name-check "table" added a false-negative risk
    // for no real safety benefit.
    static const QRegularExpression plotUsingDataRe(R"(\b(?:plot|graph|chart|surface)\b)",
                                                    QRegularExpression::CaseInsensitiveOption);

    QList<int> unnecessary;
    if (!formulaRe.match(originalRequestText).hasMatch())
        return unnecessary;
    // If the ORIGINAL request itself (not just a part's own description) explicitly asks to
    // create a table/matrix, that object was deliberately requested — never something to remove
    // just because a formula also happens to be present. Confirmed live: "Create a 150x50 matrix
    // 'SphereMap11' and fill every cell using the formula sin(x)*cos(y) ... via setFormula() and
    // calculate(). Plot than matrix" — "Create" is the request's own first word, and it names the
    // real Matrix API methods explicitly — still got its matrix-creation part removed across 3
    // fixup rounds, discarding a deliberately-requested persistent object for an ungrounded
    // "plot from formula" rewrite that may never create anything actually named "SphereMap11".
    if (tableCreationRe.match(originalRequestText).hasMatch())
        return unnecessary;
    for (int i = 0; i < plan.size(); ++i)
    {
        if (!tableCreationRe.match(plan[i].description).hasMatch())
            continue;
        for (int j = i + 1; j < plan.size(); ++j)
        {
            if (plotUsingDataRe.match(plan[j].description).hasMatch())
            {
                unnecessary << i;
                break;
            }
        }
    }
    return unnecessary;
}

// The first sentence of each explanation is enough to recognize the rule. It ends at the first ". " that is not a list
// marker: confirmed live, a cut at "1. " showed only "...asked for:\n1." and hid what was missing. A quoted request
// at the start ("This part's own job (\"...\")") is skipped and, when long, shortened: confirmed live (all-pyi
// BIGMODEL, a 4-sentence LIST item), the cut fell on the ". " after the item's first sentence, inside the quote, and
// the line ended "...with 5% noise." — the rule that blocked the script never showed.
// True when every statement of `code` is a print()/scriptPrint() call (imports and comments aside): the script reads
// and changes nothing. Lines only, deliberately: a print spread over several lines or with a call inside it that does
// work is not counted, so this never hides a real reason.
bool onlyPrintsResult(const QString &code)
{
    static const QRegularExpression printRe(R"(^(?:scriptPrint|print)\s*\(.*\)\s*;?\s*$)");
    static const QRegularExpression skipRe(R"(^(?:#|import\s|from\s+\S+\s+import\s))");
    int prints = 0;
    for (const QString &raw : code.split('\n'))
    {
        const QString line = raw.trimmed();
        if (line.isEmpty() || skipRe.match(line).hasMatch())
            continue;
        if (!printRe.match(line).hasMatch())
            return false;
        ++prints;
    }
    return prints > 0;
}

QString blockReasonSummary(const QString &explanation)
{
    QString text = explanation;
    qsizetype searchFrom = 0;
    if (const qsizetype open = text.indexOf("(\""); open >= 0 && open < 60)
    {
        if (const qsizetype close = text.indexOf("\")", open + 2); close > open)
        {
            const qsizetype quoteStart = open + 2;
            if (close - quoteStart > 80)
            {
                text = text.left(quoteStart) + text.mid(quoteStart, 60).trimmed() + " …" + text.mid(close);
                searchFrom = quoteStart + text.mid(quoteStart).indexOf("\")") + 2;
            }
            else
                searchFrom = close + 2;
        }
    }
    qsizetype end = text.indexOf(". ", searchFrom);
    while (end > 0)
    {
        const qsizetype lineStart = text.lastIndexOf('\n', end - 1) + 1;
        bool listMarker = end > lineStart;
        for (qsizetype k = lineStart; k < end && listMarker; ++k)
            listMarker = text.at(k).isDigit();
        if (!listMarker)
            break;
        end = text.indexOf(". ", end + 1);
    }
    const QString first = end > 0 && end < 400 ? text.left(end + 1) : text.left(400);
    return first.simplified();
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

// splitPlanPreflightPrompt() (QtiSasAiContent.cpp) and buildSplitPlanFixupRetryPrompt() below both
// ask for a two-stage DRAFT / ---CHECK--- / ---FINAL--- answer — everything before the marker is
// the model's own working draft and self-review, not the answer; parsing it too would double-count
// parts that appear in both the draft and the (possibly corrected) final list. Falls back to the
// whole response unchanged when no marker is present — a model can still answer either prompt with
// a plain PART list and no marker at all, and that response must be used as-is.
//
// Confirmed live: the model doesn't always use the marker as a single opening delimiter the way
// the prompt asks — one real response bracketed its answer with "---FINAL---" on BOTH sides (open
// and close), so "everything after the LAST marker" landed after the CLOSING one and returned
// nothing, discarding an otherwise fully correct, self-corrected plan and forcing an unnecessary
// fallback to non-split mode. Tries multiple candidate sections in order and uses the first
// non-empty one: between the first two markers (handles the open/close-bracket shape); after the
// last marker (the originally intended shape, still handles it correctly since with only one
// marker "first two" doesn't apply); before the first marker (recovers the draft itself if the
// model wrote a marker but never actually produced anything after or between).
// Confirmed live against a non-mlx backend: a model that otherwise obeys "when asked a question,
// answer in plain text" (kBasePromptTemplate) can still default to wrapping its whole split-plan
// answer in scriptPrint(...)/print(...) calls — either one call per line, or a single call whose
// string argument itself contains escaped "\n" sequences instead of real newlines. Either shape
// defeats partRe, since a genuine plan line then reads scriptPrint('PART: ... | DOCS: ...'), not
// PART: ... | DOCS: .... Runs BEFORE splitPlanFinalSection(): unwrapping first means a
// "---FINAL---" marker sitting inside a wrapped string's quotes is found the same way a real one
// would be, with no leftover call syntax (a trailing "))") contaminating what gets extracted
// afterward. Fails open — a line that isn't a bare scriptPrint(...)/print(...) call is kept
// unchanged, and if nothing at all unwrapped, `code` is returned byte-identical.
QString unwrapScriptPrintCalls(const QString &code)
{
    static const QRegularExpression dq(R"re(^\s*(?:scriptPrint|print)\s*\(\s*"((?:[^"\\]|\\.)*)"\s*\)\s*$)re");
    static const QRegularExpression sq(R"(^\s*(?:scriptPrint|print)\s*\(\s*'((?:[^'\\]|\\.)*)'\s*\)\s*$)");
    auto unescape = [](QString s) {
        s.replace("\\n", "\n").replace("\\t", "\t");
        s.replace("\\\"", "\"").replace("\\'", "'");
        s.replace("\\\\", "\\");
        return s;
    };

    QStringList outLines;
    bool anyUnwrapped = false;
    for (const QString &line : code.split('\n'))
    {
        QRegularExpressionMatch m = dq.match(line);
        if (!m.hasMatch())
            m = sq.match(line);
        if (m.hasMatch())
        {
            anyUnwrapped = true;
            outLines << unescape(m.captured(1)).split('\n');
        }
        else
        {
            outLines << line;
        }
    }
    return anyUnwrapped ? outLines.join('\n') : code;
}

QString splitPlanFinalSection(const QString &code)
{
    static const QRegularExpression finalMarkerRe(R"(-{2,}\s*FINAL\s*-{2,})",
                                                  QRegularExpression::CaseInsensitiveOption);
    QList<QRegularExpressionMatch> markers;
    QRegularExpressionMatchIterator it = finalMarkerRe.globalMatch(code);
    while (it.hasNext())
        markers << it.next();
    if (markers.isEmpty())
        return code;

    QStringList candidates;
    if (markers.size() >= 2)
        candidates << code.mid(markers[0].capturedEnd(), markers[1].capturedStart() - markers[0].capturedEnd());
    candidates << code.mid(markers.last().capturedEnd());
    candidates << code.left(markers.first().capturedStart());
    for (const QString &candidate : candidates)
        if (!candidate.trimmed().isEmpty())
            return candidate;
    return code;
}

// Rewritten as a numbered rule list plus a draft/check/final two-stage answer format — same
// restructuring already applied to splitPlanPreflightPrompt() (see that function's own doc
// comment for the general rationale), for the same reason: the ORIGINAL prose-paragraph version
// of THIS prompt reproducibly caused two distinct failures live, confirmed via direct, isolated
// testing against the raw model (bypassing this whole app): a repetition-loop (60-184s of
// looped/garbage output) on a graph3d styling-fragmentation complaint, and outright format
// abandonment — a bare pseudocode function-call list instead of PART/DOCS lines, ALSO inventing
// unrequested scope (plotting/fitting/residuals never asked for) — on the SAXS_data "187-parts"
// complaint, twice independently.
//
// Validated the same way splitPlanPreflightPrompt() was: re-sent both confirmed-broken complaints
// verbatim against the raw model with this restructured prompt. The graph3d 2-part fragmentation
// case now merges cleanly into one part, fast, well-formed, no loop. The SAXS_data 3-part CHAIN
// fragmentation case still doesn't fully converge on the merge in one round even with an explicit
// "merge the entire chain" rule (5) — a real, unresolved model-reasoning limit for a 3-deep
// dependency chain specifically — but critically, the response itself stayed clean, fast, and
// well-formed every time this was tested: valid PART/DOCS lines, no repetition, no hallucinated
// scope. That's a strictly safer failure than either original failure mode: the existing
// kMaxSplitPlanFixupRounds retry loop (with its own stuck-detection) already tolerates "still not
// perfect after N rounds" by proceeding anyway, so a well-formed-but-imperfect plan costs nothing
// extra, where a repetition-loop cost 60-184s and format abandonment cost the whole split-mode
// attempt.
//
// Rule 6 (topic vocabulary) exists because an early draft of this rewrite let the model invent
// new DOCS phrases ("table creation, table structure", "noise addition, data noise") instead of
// reusing a real topic name — confirmed live in the same isolated testing above. Deliberately
// constrained to topics already present in the previous plan (not the full docTopicIndex(), which
// is never sent in this fixup-retry call at all) since this call is correcting an EXISTING plan's
// mistakes, not choosing topics from scratch.
static QString buildSplitPlanFixupRetryPrompt(const QString &originalRequestText, const QString &previousPlanText,
                                              const QString &complaintsText)
{
    QStringList rules;
    rules << "Fix every problem listed above, all at once — do not leave any of them unresolved, "
             "and do not introduce a new one.";
    rules << "Every part that creates a table, graph, or compiled function MUST state its exact "
             "name in quotes in its own description — invent a short descriptive placeholder name "
             "yourself right now if the original request left it unnamed (e.g. \"compile custom "
             "fit function named '<PlaceholderFunction>'\" — a placeholder illustrating the FORMAT "
             "only, never a name to actually use).";
    rules << "Every distinct action the original request asks for must be covered by at least one "
             "part.";
    rules << "A flagged qualifying detail from the original request (e.g. \"(with background)\") "
             "missing from every part's description must be added back into whichever part's "
             "description covers that action — worded however is natural, not necessarily verbatim "
             "— never left out.";
    rules << "A part whose action was never requested anywhere in the ORIGINAL request text at all "
             "must be REMOVED from the plan entirely — do not keep it, and do not merge it into "
             "another part (there is nothing to merge; it should simply not exist).";
    rules << "A part that only fills/configures what an earlier part already created, with no "
             "other distinct action of its own, must be merged into the SAME part as whatever it "
             "depends on. If this applies to a CHAIN of consecutive parts (each one only "
             "filling/configuring what the part before it created), merge the ENTIRE CHAIN into "
             "ONE single part, not just the adjacent pair closest to each other — DOCS list is the "
             "union of every part merged this way, and the final plan must never contain two parts "
             "left in the same dependency chain. This applies with NO exception to a graph's "
             "(2D or 3D) creation and every piece of its OWN styling/configuration — mesh/line "
             "style, colors, rotation, scale, axis labels/titles, axis numeric format, legend, "
             "title, layout: all of it belongs in the SAME part as the creation, no matter how many "
             "distinct styling actions the request lists, and no matter how natural a separate "
             "\"now style it\" part may look. Confirmed live, repeatedly, in this exact spot: a "
             "\"create/plot a graph\" part followed by an \"add legend\" (or axis-labels, or "
             "mesh-style) part gets flagged here every fixup round in a row without ever actually "
             "being merged — this is the single most persistent failure this fixup loop sees, so "
             "treat any part whose job is graph styling as needing to fold into its graph's "
             "creation part FIRST, before any other restructuring this round.";
    rules << "Two adjacent parts flagged because the LATER one needs no DOCS topic beyond what the "
             "EARLIER one already needed (identical topic sets, or the later a subset of the "
             "earlier) must be merged into ONE part covering both actions, DOCS field the union of "
             "both, not left as two separate parts.";
    rules << "A flagged table/matrix-creation part must be REMOVED entirely, and the later part "
             "that plotted it rewritten to plot straight from the original request's own formula "
             "instead (plot3D(\"<formula>\", ...) or the equivalent) — never left referencing the "
             "now-removed table/matrix by name.";
    rules << "A part whose DOCS list includes fittable but whose own description only compiles a "
             "new fit function (never configuring or running an existing one) must have fittable "
             "removed from that DOCS field — compiling alone never needs it; only a part whose "
             "script actually calls Fittable itself (f.configure(), f.fit(), ...) does.";
    rules << "A part whose ENTIRE job is comparing/deciding/reporting a winner (e.g. \"compare "
             "chi2/dof and determine the better model\") and that creates no new named object of "
             "its own must be merged FORWARD into the part right after it, never backward into "
             "whatever part produced the values it compares — a bare comparison has no independent "
             "value; it only exists to feed the decision the NEXT part needs (e.g. which curve to "
             "plot), and split-execution can't carry that decision forward as a live variable, only "
             "as printed text, so leaving it standing alone forces the same comparison to be "
             "silently redone inside the part that actually needs the answer.";
    rules << "Do not otherwise change the plan, and do not invent a different scenario — every "
             "part/DOCS field not implicated by a problem above must stay exactly as it was.";
    rules << "Every DOCS field must use ONLY topic names already appearing in the ORIGINAL plan "
             "above (e.g. \"table\", \"graph3d\", \"fittable\", ...) — never invent a new topic "
             "label or description phrase in its place.";

    QStringList numberedRules;
    for (int i = 0; i < rules.size(); ++i)
        numberedRules << QString("%1. %2").arg(i + 1).arg(rules[i]);

    return "Original request:\n" + originalRequestText + "\n\nYour previous split plan:\n" + previousPlanText +
           "\n\nThat plan has the following problem(s):\n" + complaintsText +
           "\n\nFollow these rules IN ORDER OF PRIORITY:\n\n" + numberedRules.join("\n\n") +
           "\n\nAnswer in TWO stages.\n\n"
           "Stage 1 — DRAFT: write your corrected PART/DOCS list, one PART per line, nothing "
           "else:\nPART: <description> | DOCS: <topics, or \"none\">\n\n"
           "Stage 2 — CHECK, then FINAL: on a new line write exactly \"---CHECK---\", then "
           "briefly verify your draft actually fixes every problem listed above and introduces no "
           "new one. Then write exactly \"---FINAL---\" followed by the corrected list.";
}

// Confirmed live: a colliding name flagged by splitPlanCreationCollisions() is often one the
// ORIGINAL request never mentioned at all — it was invented by this same fixup process's own
// "invent a placeholder name" rule (e.g. the request just said "compile a custom Porod power-law
// fit function", never naming it; the plan invented 'ai/PorodFit', which happened to already
// exist). Halting and asking the USER to supply a different name is the wrong remediation for a
// name the user never chose in the first place — there is nothing for them to correct. This
// prompt instead asks the model to invent a DIFFERENT placeholder for exactly the same part(s),
// updating every other part that references the old name so the plan stays internally consistent
// (mirrors reconcileCompiledFunctionNameAcrossParts()'s own concern), otherwise unchanged. Reuses
// the same PART/DOCS answer format handleSplitPlanResponse() already parses, so the response goes
// back through the exact same pipeline (naming/fragment/action checks and the collision check
// itself all run again naturally) rather than a bespoke parser.
// allFunctionKind: whether EVERY colliding name is a compile/fittable creation (never mixed with
// table/matrix/note/graph in the same round) — confirmed live: this prompt's own "ai/" example was
// the ONLY naming illustration given, and got applied to a TABLE rename too even though "ai/" is
// Rule 6's compile/fittable-only sandbox convention. Only offered as the example when it's
// actually true for every name being renamed this round.
static QString buildSplitPlanCreationCollisionRenamePrompt(const QString &originalRequestText,
                                                           const QString &previousPlanText,
                                                           const QStringList &collidingNames, bool allFunctionKind)
{
    QStringList quoted;
    for (const QString &n : collidingNames)
        quoted << "'" + n + "'";
    const QString namingExample = allFunctionKind
                                      ? "e.g. the \"ai/\" prefix for a compiled function"
                                      : "e.g. a distinguishing suffix like \"_2\" — the \"ai/\" prefix is ONLY "
                                        "for a compiled/fittable function, never for a table/matrix/note/graph";
    return "Original request:\n" + originalRequestText + "\n\nYour split plan:\n" + previousPlanText +
           "\n\nThe placeholder name(s) " + quoted.join(", ") +
           " you invented already exist as something else — the original request never asked for "
           "that specific name, so invent a DIFFERENT placeholder for the part(s) that create " +
           (collidingNames.size() == 1 ? "it" : "them") + " instead (same naming convention as before, " +
           namingExample +
           "). Update every OTHER part that references the old name so it uses the new one "
           "instead — the plan must stay internally consistent. Do not otherwise change the plan. "
           "Answer with the corrected PART/DOCS list, one PART per line, nothing else:\nPART: "
           "<description> | DOCS: <topics, or \"none\">";
}

void AiAssistant::handleSplitPlanResponse(const QString &code)
{
    awaitingSplitPlan = false;
    splitPlan.clear();
    splitPartScripts.clear();
    splitPartIndex = -1;
    const QStringList knownTopics = m_content ? m_content->knownDocTopics() : QStringList();
    static const QRegularExpression partRe(R"(^\s*PART\s*:\s*(.+?)\s*\|\s*DOCS\s*:\s*(.+?)\s*$)",
                                           QRegularExpression::CaseInsensitiveOption);
    const QStringList lines = splitPlanFinalSection(unwrapScriptPrintCalls(code)).split('\n');
    for (const QString &line : lines)
    {
        const QRegularExpressionMatch m = partRe.match(line);
        if (!m.hasMatch())
            continue;
        const QString description = m.captured(1).trimmed();
        // No LOCAL doc content is ever attached in Integrated mode regardless of topic (same
        // fact beginNonSplitRequest() already short-circuits on) — but unlike that path, nothing
        // here skips ASKING for per-part topics, since splitting and doc-topic assignment come
        // out of the same preflight call. These topics are still kept (not blanked) even in
        // Integrated mode: startSplitPart() forwards them as a "TOPICS: ..." hint line in the
        // per-part prompt text, for a server-side RAG backend to key its own retrieval on — see
        // that call site's doc comment. A deployment not using such a backend just sees a few
        // extra, harmless words of prompt text.
        splitPlan << SplitPart{description, splitPartTopicsFromAnswer(m.captured(2), description, knownTopics)};
    }
    applyAiNamespaceToCompileParts(splitPlan);
    reconcileCompiledFunctionNameAcrossParts(splitPlan);
    QStringList removedDuplicateParts;
    dedupeSplitPlanExactDuplicateParts(splitPlan, &removedDuplicateParts);
    if (!removedDuplicateParts.isEmpty() && m_host)
        m_host->print(QString("▸▸▸ AI split-plan: removed %1 duplicate part(s) (identical to an "
                              "earlier part).")
                          .arg(removedDuplicateParts.size()));

    if (splitPlan.isEmpty())
    {
        // Unparseable plan — a malformed decomposition answer must never block the request
        // outright, but falling back to a single SplitPart with docTopics=QStringList() fails
        // open to the ENTIRE doc corpus (an empty topic list means "attach every doc topic", see
        // filterDocsByTopic()), risking blowing past the model's context window. Abandon split
        // mode entirely instead and reuse the same "which doc topics do you need" pre-flight a
        // plain (non-split) request already uses (ApplicationWindow::askAi()'s docsMode==Selective).
        if (m_host)
            m_host->print("▸▸▸ AI split-plan: unparseable — abandoning split mode, selecting doc "
                          "topics from the request text instead of attaching all of them.\n"
                          "▸▸▸ AI split-plan raw response (for diagnosing the parse failure):\n" +
                          code);
        splitModeActive = false;
        if (m_content)
        {
            // Same mechanical selection beginNonSplitRequest() uses — see docTopicsFromRequestText()
            // for why the doc-selection LLM pre-flight this used to fire could not affect the
            // outcome. Falling back from a failed split plan is exactly when NOT to spend another
            // round-trip.
            startRequestWithSelectedTopics(docTopicsFromRequestText(originalRequestText),
                                           QStringLiteral("AI doc topics"));
        }
        else
        {
            // No content provider (shouldn't happen in practice — see setContentProvider()'s call
            // sites) — fall back to a plain request instead of firing ask() with an empty prompt
            // and leaving a flag stuck waiting on nothing.
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

    // Combined, capped verify-and-retry on the decomposition step itself. These four checks
    // (naming completeness, action coverage, unnecessary fragmentation, unrequested actions) used
    // to be independent one-shot blocks, each individually documented as "never retries a second
    // time... so a plan that still fails proceeds anyway rather than retrying forever" — but that
    // guarantee only held per individual check, not for the combined process. Confirmed live,
    // severe: a real plan ping-ponged between "missing explicit name(s)" and "unnecessary
    // fragmentation" THREE FULL CYCLES (6 wasted round-trips, one taking 111s) before converging —
    // each retry prompt showed the model only ONE class of complaint, so fixing it (e.g. adding a
    // name by re-splitting a merged part back apart) had no visibility into the OTHER constraint
    // it was simultaneously breaking, and nothing bounded the TOTAL round-trips across the three
    // independently-one-shot gates. Checking all four together every round and folding every
    // currently-detected complaint into ONE retry prompt gives the model the complete picture each
    // time instead of whack-a-mole; splitPlanFixupRoundsUsed guarantees termination (fail-open,
    // accept the plan as-is) even if the model still can't satisfy everything.
    static const int kMaxSplitPlanFixupRounds = 3;
    // Name the unnamed BEFORE any check runs — see assignSplitPlanCreationNames() for the live
    // deadlock this replaces. missingNames below is then empty by construction in the normal case,
    // so no fixup round is ever spent on a complaint the model demonstrably cannot satisfy.
    QStringList mechanicallyNamed;
    assignSplitPlanCreationNames(splitPlan, &mechanicallyNamed);
    if (!mechanicallyNamed.isEmpty() && m_host)
        m_host->print("▸▸▸ AI split-plan: named unnamed creation part(s) mechanically — " +
                      mechanicallyNamed.join(", ") +
                      " (the request named none, and every check "
                      "downstream needs a concrete name to match against).");
    // Then fold back any part that is pure configuration of the one before it — see
    // mergeSplitPlanUnnecessaryFragments() for the live non-convergence this replaces. Runs AFTER
    // naming so the merged description keeps the name that was just assigned.
    QStringList mergedFragments;
    mergeSplitPlanUnnecessaryFragments(splitPlan, &mergedFragments);
    if (!mergedFragments.isEmpty() && m_host)
        m_host->print("▸▸▸ AI split-plan: merged back " + QString::number(mergedFragments.size()) +
                      " part(s) that only configured what an earlier part created — \"" +
                      mergedFragments.join("\", \"") + "\" (one action, not two).");
    // Removed here, not asked for: confirmed live (LIST 5, item 2), the model resent the plan with the same "fittable"
    // in a compile-only part's DOCS field three rounds in a row, and the plan went ahead with it anyway.
    QStringList trimmedDocsParts;
    for (int i : splitPlanCompileOnlyHasFittable(splitPlan))
    {
        QStringList &topics = splitPlan[i].docTopics;
        for (qsizetype t = topics.size() - 1; t >= 0; --t)
            if (topics.at(t).compare(QStringLiteral("fittable"), Qt::CaseInsensitive) == 0)
                topics.removeAt(t);
        trimmedDocsParts << QString::number(i + 1);
    }
    if (!trimmedDocsParts.isEmpty() && m_host)
        m_host->print("▸▸▸ AI split-plan: removed the fittable docs from compile-only part(s) " +
                      trimmedDocsParts.join(", ") + " — compiling alone never needs them.");
    // Merged here, not asked for: confirmed live (LIST 5, item 4), "Part 2 needs no DOCS topic beyond Part 1 — merge
    // them" came back unchanged for 2 rounds and the plan went ahead unmerged. A compare-only part merges FORWARD (its
    // own complaint below), so it is left to that one.
    {
        QList<int> sameDocs;
        const QList<int> forward = splitPlanCompareOnlyMergesForward(splitPlan);
        for (int i : splitPlanAdjacentSameDocs(splitPlan))
            if (!forward.contains(i))
                sameDocs << i;
        QStringList mergedSameDocs;
        mergeSplitPlanPartsBackward(splitPlan, sameDocs, &mergedSameDocs);
        if (!mergedSameDocs.isEmpty() && m_host)
            m_host->print("▸▸▸ AI split-plan: merged " + QString::number(mergedSameDocs.size()) +
                          " part(s) needing no docs beyond the part before into that part — \"" +
                          mergedSameDocs.join("\", \"") + "\".");
    }
    const QList<int> missingNames = splitPartsMissingCreationName(splitPlan);
    const QStringList missingActions = splitPlanMissingRequestedActions(originalRequestText, splitPlan);
    const QList<int> unnecessaryFragments = splitPlanUnnecessaryFragments(splitPlan);
    const QList<int> unrequestedActions = splitPlanUnrequestedActions(originalRequestText, splitPlan);
    const QList<int> adjacentSameDocs = splitPlanAdjacentSameDocs(splitPlan);
    const QList<int> unnecessaryDataTables = splitPlanUnnecessaryDataTable(originalRequestText, splitPlan);
    const QStringList droppedDetails = splitPlanDroppedParentheticalDetail(originalRequestText, splitPlan);
    const QList<int> compileOnlyFittable = splitPlanCompileOnlyHasFittable(splitPlan);
    const QList<int> compareOnlyForward = splitPlanCompareOnlyMergesForward(splitPlan);
    const bool planHasFixableIssues =
        !missingNames.isEmpty() || !missingActions.isEmpty() || !unnecessaryFragments.isEmpty() ||
        !unrequestedActions.isEmpty() || !adjacentSameDocs.isEmpty() || !unnecessaryDataTables.isEmpty() ||
        !droppedDetails.isEmpty() || !compileOnlyFittable.isEmpty() || !compareOnlyForward.isEmpty();
    QStringList complaints;
    if (planHasFixableIssues)
    {
        // Removal always wins over a "keep and fix" complaint about the SAME part — a part flagged
        // as unrequested won't exist afterward, so naming/merging/replacing it is moot, and asking
        // for both at once gives the model two contradictory instructions it can't jointly satisfy.
        // Confirmed live: a plot part never asked for in the original request kept BOTH "state its
        // name" and "must be REMOVED entirely... there is nothing to merge" every round for 3
        // rounds straight, never converging on either. unnecessaryFragments/unnecessaryDataTables
        // carry the same "shouldn't exist separately" verdict, just from a different checker — a
        // part they flag must suppress missingNames/adjacentSameDocs too, not just unrequestedActions'
        // own. toRemove (the full union) handles that; the two narrower sets below give each
        // removal-type checker priority over the ones after it so a part gets exactly one removal
        // complaint instead of stacking near-duplicates from multiple checkers agreeing it's dead.
        const QSet<int> unrequestedSet(unrequestedActions.cbegin(), unrequestedActions.cend());
        const QSet<int> unnecessaryFragmentsSet(unnecessaryFragments.cbegin(), unnecessaryFragments.cend());
        const QSet<int> toRemove = unrequestedSet + unnecessaryFragmentsSet +
                                   QSet<int>(unnecessaryDataTables.cbegin(), unnecessaryDataTables.cend());
        const QSet<int> compileOnlyFittableSet(compileOnlyFittable.cbegin(), compileOnlyFittable.cend());
        const QSet<int> compareOnlyForwardSet(compareOnlyForward.cbegin(), compareOnlyForward.cend());
        for (int i : missingNames)
            if (!toRemove.contains(i))
                complaints << QString("Part %1 (\"%2\") creates something but never states its name "
                                      "in quotes.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description);
        if (!missingActions.isEmpty())
            complaints << ("That plan never covers the following action(s) explicitly requested "
                           "above, in ANY part's description: " +
                           missingActions.join(", ") + ".");
        if (!droppedDetails.isEmpty())
            complaints << ("The original request's qualifying detail(s) " + droppedDetails.join(", ") +
                           " never appear in ANY part's description — add them back to whichever "
                           "part's description covers that action, worded however is natural.");
        for (int i : unnecessaryFragments)
            if (!unrequestedSet.contains(i))
                complaints << QString("Part %1 (\"%2\") only fills/configures what Part %3 (\"%4\") "
                                      "just created, with no other distinct action of its own.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description)
                                  .arg(i)
                                  .arg(splitPlan[i - 1].description);
        for (int i : unrequestedActions)
            complaints << QString("Part %1 (\"%2\") performs an action the original request never "
                                  "asked for at all.")
                              .arg(i + 1)
                              .arg(splitPlan[i].description);
        for (int i : adjacentSameDocs)
            // A part's inflated (wrongly-including-fittable) DOCS field can make the FOLLOWING
            // part look topically redundant with it when it isn't — the fix there is trimming the
            // preceding part's DOCS field, not merging two genuinely distinct actions. Suppressed
            // here in favor of the compileOnlyFittable complaint below, which names the real fix.
            // Also suppressed when Part i itself is a pure compare-only step the OTHER checker
            // below wants merged FORWARD into Part i+1 instead — the topically-driven backward
            // merge this loop would otherwise suggest is the wrong direction for that part.
            if (!toRemove.contains(i) && !compileOnlyFittableSet.contains(i - 1) && !compareOnlyForwardSet.contains(i))
                complaints << QString("Part %1 (\"%2\") needs no DOCS topic beyond what Part %3 "
                                      "(\"%4\") already needed — merge them into one part.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description)
                                  .arg(i)
                                  .arg(splitPlan[i - 1].description);
        for (int i : compileOnlyFittable)
            complaints << QString("Part %1 (\"%2\")'s DOCS list includes fittable, but its own "
                                  "description only compiles a function's definition — never "
                                  "configures or runs one. Remove fittable from that DOCS field; "
                                  "compiling alone never needs it.")
                              .arg(i + 1)
                              .arg(splitPlan[i].description);
        for (int i : compareOnlyForward)
            if (!toRemove.contains(i + 1))
                complaints << QString("Part %1 (\"%2\") has no durable object of its own — it only "
                                      "decides something Part %3 (\"%4\") needs, and split-execution "
                                      "can't share that decision as a live variable across parts, "
                                      "only as printed text. Merge Part %1 FORWARD into Part %3, not "
                                      "backward into whatever produced the values it compares.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description)
                                  .arg(i + 2)
                                  .arg(splitPlan[i + 1].description);
        for (int i : unnecessaryDataTables)
            if (!unrequestedSet.contains(i) && !unnecessaryFragmentsSet.contains(i))
                complaints << QString("Part %1 (\"%2\") creates a table/matrix that a later part "
                                      "only plots — but the original request already gives the data "
                                      "as a formula. Remove this part and plot straight from the "
                                      "formula instead.")
                                  .arg(i + 1)
                                  .arg(splitPlan[i].description);
    }
    // Confirmed live, twice, independently: the resent plan can carry the EXACT same unresolved
    // complaint round after round (the model doesn't merge what it's asked to merge) — see
    // lastSplitPlanFixupComplaints' doc comment. Detecting that up front, before spending another
    // round-trip, is strictly better than discovering it only after asking again: same outcome,
    // fewer wasted calls.
    const bool stuck = planHasFixableIssues && complaints == lastSplitPlanFixupComplaints;
    if (planHasFixableIssues && !stuck && splitPlanFixupRoundsUsed < kMaxSplitPlanFixupRounds)
    {
        // Reconstructs the plan actually produced, in the same PART/DOCS format it was parsed
        // from, so the retry can show it back to the model instead of merely describing it.
        QStringList planLines;
        for (const SplitPart &p : splitPlan)
            planLines << QString("PART: %1 | DOCS: %2").arg(p.description).arg(docTopicsDisplayLabel(p.docTopics));
        ++splitPlanFixupRoundsUsed;
        lastSplitPlanFixupComplaints = complaints;
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-plan: %1 issue(s) detected — asking the model to fix "
                                  "and resend the plan (round %2/%3).\n\n  - ")
                              .arg(complaints.size())
                              .arg(splitPlanFixupRoundsUsed)
                              .arg(kMaxSplitPlanFixupRounds) +
                          complaints.join("\n  - "));
        awaitingSplitPlan = true;
        // This ask() call is a fresh, standalone request — addToHistory=false on the initial
        // decomposition call above means neither the original request nor the plan it produced
        // is otherwise visible here, so both are restated explicitly below rather than relying
        // on history the model can't actually see. includeHistory=false enforces that same
        // intent programmatically.
        // docTopics=QStringList() (not {"ai"}): see beginSplitPlanDecomposition()'s call site for
        // why — noDocsHintLine() already puts the real "TOPICS: none" in the prompt text itself.
        ask(buildSplitPlanFixupRetryPrompt(originalRequestText, planLines.join("\n"), complaints.join("\n")) +
                noDocsHintLine(),
            true, QString(), QStringList(), false, false, false);
        return;
    }
    if (planHasFixableIssues && m_host)
    {
        if (stuck)
            m_host->print(QString("▸▸▸ AI split-plan: same issue(s) came back unchanged after %1 "
                                  "fixup round(s) — not converging, proceeding anyway instead of "
                                  "spending the remaining round(s).\n\n  - ")
                              .arg(splitPlanFixupRoundsUsed) +
                          complaints.join("\n  - "));
        else
            m_host->print(QString("▸▸▸ AI split-plan: still has unresolved issue(s) after %1 fixup "
                                  "round(s) — proceeding anyway rather than retrying indefinitely."
                                  "\n\n  - ")
                              .arg(kMaxSplitPlanFixupRounds) +
                          complaints.join("\n  - "));
    }

    // The plan is now fully finalized (fixup converged or gave up) — show it now, before anything
    // else, regardless of whether the collision check below ends up halting the request: a halt
    // must never leave the user unable to see what the plan actually finalized to (confirmed
    // live: with the summary print previously living inside finalizeSplitPlanAndLaunch(), a
    // collision halt skipped it entirely, hiding the very plan structure needed to tell whether an
    // earlier fixup round's complaint — e.g. a duplicated part — actually got resolved or not).
    if (m_host)
    {
        // Show the topics actually scoped per part (baseline "ai"/"app" + the part's own), not
        // just what the model named — startSplitPart() prepends baseline before every real
        // request, so the announcement should match instead of silently omitting it. This must
        // hold for a none/ambiguous part too: startSplitPart()'s own hintTopics
        // (baselineTopics + (noneOrAmbiguous ? QStringList() : part.docTopics)) still includes
        // baseline unconditionally even then — confirmed live: this line used to fall back to the
        // bare partTopics for exactly that case, printing "[docs: none]"/"[docs: all]" for a part
        // whose real TOPICS hint was actually "ai, app" once it ran.
        const QStringList baselineTopics = m_content ? m_content->baselineDocTopics() : QStringList();
        QStringList summary;
        for (int i = 0; i < splitPlan.size(); ++i)
        {
            const QStringList &partTopics = splitPlan[i].docTopics;
            const bool noneOrAmbiguous = partTopics.isEmpty() || partTopics == QStringList{kNoDocsSentinel};
            const QStringList effectiveTopics = baselineTopics + (noneOrAmbiguous ? QStringList() : partTopics);
            summary << QString("  %1. %2 [docs: %3]")
                           .arg(i + 1)
                           .arg(splitPlan[i].description)
                           .arg(docTopicsDisplayLabel(effectiveTopics));
        }
        m_host->print("▸▸▸ AI split-plan (" + QString::number(splitPlan.size()) + " part(s)):\n\n" +
                      summary.join("\n"));
    }

    // One last check before anything actually runs: does any part's own to-be-created name
    // (splitPlanCreationNames()) already exist? See splitPlanCreationCollisions()'s doc comment
    // for why a silent recompile/table-clear/graph-clear/note-misnaming over an existing object is
    // a real risk. Skipped entirely (no extra round-trip) when the plan creates nothing
    // check-worthy, matching this file's general "don't spend a call on a check with nothing to
    // check" practice.
    // A name Rule 6 already exempts unconditionally (see isRule6SandboxExemptCreationName())
    // never needs asking about — its collision-check answer can't change the outcome either way,
    // so skip the round-trip for it entirely rather than checking and discarding the answer.
    QStringList skippedAsExempt;
    const QList<QPair<QString, QStringList>> creationNames =
        splitPlanCreationNamesNeedingCheck(splitPlanCreationNames(splitPlan), &skippedAsExempt);
    if (!skippedAsExempt.isEmpty())
    {
        splitPlanSandboxExemptNames << skippedAsExempt;
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-plan: skipping existence-check for %1 — an \"ai/\" "
                                  "compile/fittable collision is always safe to overwrite "
                                  "(compile-python-api.md Rule 6).")
                              .arg(skippedAsExempt.join(", ")));
    }
    // The host answers itself when it can: the same lookup the PROJECT STATE note uses, no model-written script.
    // Confirmed live (LIST 5 item 5, split mode, 2026-10-11): on a fresh project the model's check script printed
    // "(window) MISSING: Pipeline", then "(window) EXISTS: Pipeline" and "VERDICT: EXISTS", the item was stopped as a
    // collision, and a stray note "Notes1" was left behind. The answer is written in that script's own line format,
    // so the decision below stays the same.
    if (!creationNames.isEmpty() && m_content && m_host)
    {
        QStringList names;
        for (const QPair<QString, QStringList> &entry : creationNames)
            names << entry.first;
        const QList<QStringList> kinds = m_host->entityKinds(names);
        if (kinds.size() == names.size())
        {
            static const QStringList windowKinds = {"window", "table", "matrix", "note", "graph", "3D plot"};
            QStringList lines;
            for (int i = 0; i < names.size(); ++i)
                for (const QString &label : creationNames[i].second)
                {
                    bool exists = false;
                    for (const QString &kind : kinds[i])
                        exists = exists || (label == "window" ? windowKinds.contains(kind) : kind == label);
                    lines << "(" + label + ") " + (exists ? "EXISTS" : "MISSING") + ": " + names[i];
                }
            finishSplitPlanCreationCheck(lines.join("\n"));
            return;
        }
    }
    if (!creationNames.isEmpty() && m_content)
    {
        awaitingSplitPlanCreationCheckScript = true;
        // expectsCode=false/includeHistory=false: same reasoning as beginRequest()'s "existing X"
        // verification call this mirrors — a bare check script, not a continuation of history.
        // includeDataProbe=false: these names are about to be CREATED, so there is no existing
        // dataset to report a shape for — this stays the minimal collision probe.
        ask(m_content->existingEntityVerificationPrompt(creationNames, false) + topicsHintLine(QStringList()), true,
            QString(), QStringList(), false, false, false);
        return;
    }
    finalizeSplitPlanAndLaunch();
}

void AiAssistant::finalizeSplitPlanAndLaunch()
{
    // The result line is the last part's job: only it can compute every value. Confirmed live (LIST 5, items 4 and 6):
    // no part's description carried it, so either no part printed it or two parts each printed half of it.
    if (m_content && !splitPlan.isEmpty())
    {
        const QString duty = m_content->resultLineDuty(withoutRequestSuffix(originalRequestText));
        const QString label = m_content->resultLabel(duty);
        // The duty itself, not just its label: confirmed live (benchmark LIST 5, item 6, 4 of 5 runs), the plan's last
        // part read "check which functions still exist and print the result with label R6:" — the label was there,
        // the existence of Peak5 and Pipeline was not, and the line came out as "R6: []".
        if (!duty.isEmpty() && !splitPlan.last().description.contains(duty))
        {
            splitPlan.last().description += " Then: " + duty;
            if (m_host && splitPlan.size() > 1)
                m_host->print(QString("▸▸▸ AI split-plan: the result line (\"%1\") is part %2's job.")
                                  .arg(label)
                                  .arg(splitPlan.size()));
        }
    }
    // A plan that settled on exactly one part is, by definition, not actually being split — run
    // it as a plain request instead of paying for split-mode's overhead (the "Split-mode part 1 of
    // 1" framing in startSplitPart()'s prompt, and — the real cost — requestSplitMerge()'s entire
    // extra round-trip afterward asking the model to "merge" a single script with nothing to merge
    // it against). This is deliberately checked here, AFTER the three retries above, not as an
    // early return before them: a one-part plan can still be missing a name, missing coverage, or
    // (moot for a single part) fragmented, and those checks apply exactly the same regardless of
    // how many parts the plan settled on.
    if (splitPlan.size() == 1)
    {
        if (m_host)
            m_host->print("▸▸▸ AI split-plan: single part — running directly without split-mode "
                          "overhead.");
        splitModeActive = false;
        const bool noneOrAmbiguous =
            splitPlan[0].docTopics.isEmpty() || splitPlan[0].docTopics == QStringList{kNoDocsSentinel};
        startRequest(noneOrAmbiguous
                         ? splitPlan[0].docTopics
                         : ((m_content ? m_content->baselineDocTopics() : QStringList()) + splitPlan[0].docTopics),
                     splitPlan[0].description);
        return;
    }
    startSplitPart(0);
}

void AiAssistant::handleSplitPlanCreationCheckScriptResponse(const QString &code)
{
    awaitingSplitPlanCreationCheckScript = false;
    if (!m_host)
        return;
    QString syntaxError;
    if (!m_host->canCompile(code, &syntaxError))
    {
        // Fail open — same philosophy as handleExistenceCheckScriptResponse(): a broken
        // verification script must never become a new way to get permanently stuck.
        m_host->print("▸▸▸ AI: creation-name collision check script wasn't valid Python — skipping "
                      "the check, proceeding.");
        finalizeSplitPlanAndLaunch();
        return;
    }
    const AiScriptResult result = m_host->runScript(code, "<ai-existence-check>");
    if (!result.error.isEmpty())
    {
        m_host->print("▸▸▸ AI: creation-name collision check script errored — skipping the check, "
                      "proceeding.");
        finalizeSplitPlanAndLaunch();
        return;
    }
    finishSplitPlanCreationCheck(result.output);
}

void AiAssistant::finishSplitPlanCreationCheck(const QString &checkOutput)
{
    const QList<QPair<QString, QStringList>> checkedNames = splitPlanCreationNames(splitPlan);
    QStringList collisions = splitPlanCreationCollisions(checkOutput, checkedNames);
    QStringList sandboxExempt;
    collisions = splitPlanCollisionsAfterSandboxExemption(collisions, checkedNames, &sandboxExempt);
    if (!sandboxExempt.isEmpty())
    {
        splitPlanSandboxExemptNames << sandboxExempt;
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-plan: %1 already exist(s) under \"ai/\" — treated as "
                                  "disposable scratch (compile-python-api.md Rule 6), proceeding to "
                                  "recompile over it.")
                              .arg(sandboxExempt.join(", ")));
    }

    // A name the request only works ON was made a creation by the plan. Confirmed live (LIST 3 items 2 and 3, split
    // mode, 4 of 4): "In \"Squares\", give column 3 a formula ..." was planned as "create table named 'Squares' with
    // three columns ...", and the collision stop below gave the item up without running anything. The plan's part
    // is told to use the existing object instead; a script that would create it again is still blocked
    // (existingTableRecreatedMistake()).
    QStringList workedOn;
    for (const QString &n : collisions)
        if (originalRequestText.contains("\"" + n + "\"") && !requestCreatesName(originalRequestText, n))
            workedOn << n;
    if (!workedOn.isEmpty())
    {
        for (SplitPart &part : splitPlan)
            for (const QString &n : workedOn)
                if (part.description.contains(n))
                    part.description += " (\"" + n + "\" already exists: use it, do not create it again.)";
        for (const QString &n : workedOn)
            collisions.removeAll(n);
        if (m_host)
            m_host->print("▸▸▸ AI split-plan: " + workedOn.join(", ") +
                          " already exist(s) and the request works on it, not creates it — the plan uses the "
                          "existing one.");
    }

    if (!collisions.isEmpty())
    {
        // A collision on a name the user never actually asked for (it was invented by this same
        // fixup process's own placeholder rule) has nothing for the user to correct — see
        // buildSplitPlanCreationCollisionRenamePrompt()'s doc comment. Only a name that genuinely
        // appears in the user's own original wording is a real ambiguity worth halting for.
        QStringList userSpecified, aiInvented;
        for (const QString &n : collisions)
            (originalRequestText.contains(n, Qt::CaseInsensitive) ? userSpecified : aiInvented) << n;

        static const int kMaxSplitPlanCreationNameRetryRounds = 2;
        if (userSpecified.isEmpty() && !aiInvented.isEmpty() &&
            splitPlanCreationNameRetryRoundsUsed < kMaxSplitPlanCreationNameRetryRounds)
        {
            ++splitPlanCreationNameRetryRoundsUsed;
            QStringList planLines;
            for (const SplitPart &p : splitPlan)
                planLines << QString("PART: %1 | DOCS: %2").arg(p.description).arg(docTopicsDisplayLabel(p.docTopics));
            if (m_host)
                m_host->print(QString("▸▸▸ AI split-plan: invented name(s) %1 already exist — asking "
                                      "the model to invent different one(s) (round %2/%3).")
                                  .arg(aiInvented.join(", "))
                                  .arg(splitPlanCreationNameRetryRoundsUsed)
                                  .arg(kMaxSplitPlanCreationNameRetryRounds));
            awaitingSplitPlan = true;
            bool allFunctionKind = true;
            for (const QString &n : aiInvented)
            {
                QStringList labels;
                for (const QPair<QString, QStringList> &entry : checkedNames)
                    if (entry.first == n)
                        labels = entry.second;
                if (!labels.contains("compile") && !labels.contains("fittable"))
                {
                    allFunctionKind = false;
                    break;
                }
            }
            // docTopics=QStringList() (not {"ai"}): see beginSplitPlanDecomposition()'s call site
            // for why — noDocsHintLine() already puts the real "TOPICS: none" in the prompt text.
            ask(buildSplitPlanCreationCollisionRenamePrompt(originalRequestText, planLines.join("\n"), aiInvented,
                                                            allFunctionKind) +
                    noDocsHintLine(),
                true, QString(), QStringList(), false, false, false);
            return;
        }

        // The model-based retry above has been given every chance it gets — confirmed live, 3
        // separate times, it just re-emits the same colliding name unchanged. A name the user
        // never asked for has nothing to negotiate, so generate the alternate mechanically instead
        // of a hard halt: guaranteed to actually differ, and terminates immediately.
        if (userSpecified.isEmpty() && !aiInvented.isEmpty())
        {
            QStringList renames;
            mechanicallyRenameSplitPlanCollisions(splitPlan, aiInvented, &renames);
            if (m_host)
                m_host->print("▸▸▸ AI split-plan: the model didn't invent a different name after " +
                              QString::number(kMaxSplitPlanCreationNameRetryRounds) +
                              " round(s) — renamed mechanically instead: " + renames.join(", "));
            finalizeSplitPlanAndLaunch();
            return;
        }

        QStringList quoted;
        for (const QString &n : collisions)
            quoted << "\"" + n + "\"";
        splitModeActive = false; // before the note below: no part ran, so only a LIST's skip note applies
        m_host->print("▸▸▸ AI: stopping — " + quoted.join(", ") +
                      (collisions.size() == 1 ? " already exists" : " already exist") +
                      ", and compiling/creating over " + (collisions.size() == 1 ? "it" : "them") +
                      " silently overwrites the existing one, with no backup and no confirmation. "
                      "Nothing was created or run. Reply with a different name, or say \"overwrite "
                      "it\"/\"reuse it\" to explicitly authorize that instead." +
                      splitModeGiveUpNote());
        splitPlan.clear();
        splitPartScripts.clear();
        splitPartIndex = -1;
        return;
    }
    finalizeSplitPlanAndLaunch();
}

void AiAssistant::startSplitPart(int index)
{
    // Belt and braces behind resetPendingRoundTrips(): splitPlan[index] is an UNCHECKED QList
    // access, so a stale caller reaching here after the plan was cleared read out of bounds and
    // faulted on the garbage SplitPart's docTopics (confirmed live, SIGSEGV in growAppend()).
    // Any future path that loses track of the plan should get a message, never a crash.
    if (index < 0 || index >= splitPlan.size())
    {
        if (m_host)
            m_host->print(QString("▸▸▸ AI: internal — split part %1 requested but the plan holds %2 "
                                  "part(s); abandoning split mode for this request.")
                              .arg(index)
                              .arg(splitPlan.size()));
        splitModeActive = false;
        splitPartIndex = -1;
        return;
    }
    splitPartIndex = index;
    // Each part gets its own fresh per-request state — same reset ApplicationWindow::askAi()
    // does for a brand-new whole request — so a part's retries are judged against the full
    // m_maxAutoContinue budget, not whatever was left over from an earlier part.
    autoContinueCount = 0;
    consecutiveExploreCount = 0;
    lastExploreOutput.clear();
    lastExploreCode.clear();
    consecutiveDocRetryCount = 0;
    awaitingSelfReview = false;
    lastAvailableBlock.clear();
    lastRetryError.clear();
    isErrorRetryCorrection = false;
    m_freshRetryUsed = false;

    SplitPart part = splitPlan[index];
    // "none"/ambiguous-"all" stay exactly as returned (never get the baseline topics prepended) —
    // same guard as startRequestWithSelectedTopics()'s own baseline-topic prepend.
    const bool noneOrAmbiguous = part.docTopics.isEmpty() || part.docTopics == QStringList{kNoDocsSentinel};
    // The doc of every kind an earlier part created goes along: a later part works on those objects whatever its
    // description stresses. Confirmed live (series 6, local, LIST 5 item 5): part 2 wrote the scripts into the note's
    // "tabs" with topics ai, app, table — no note doc — invented setTabText() and the item was given up.
    if (index > 0 && !noneOrAmbiguous && m_host && m_content)
        for (const QString &object : m_host->liveObjects())
            if (!m_objectsAtRequestStart.contains(object))
                if (const QString topic = m_content->docTopicForKind(object.section(':', 0, 0));
                    !topic.isEmpty() && !part.docTopics.contains(topic))
                    part.docTopics << topic;
    selectedDocTopics = noneOrAmbiguous
                            ? part.docTopics
                            : ((m_content ? m_content->baselineDocTopics() : QStringList()) + part.docTopics);

    // No LOCAL doc content is ever attached in Integrated mode (buildSystemPrompt()'s own
    // short-circuit), but a server-side RAG proxy sitting in front of the actual model can use
    // this line to scope ITS OWN retrieval to just these topics instead of searching its whole
    // corpus — e.g. qtisas-rag's core.py already supports per-file retrieval scoping but had no
    // per-request signal to drive it with until this line existed. On its own line, after
    // part.description ends, so nothing parsing that description out of the prompt (client- or
    // server-side) can ever capture this text as part of it. A deployment not using such a proxy
    // just sees a few extra, harmless words of prompt text.
    //
    // Always includes baselineDocTopics() ("ai"/"app" — assistant workflow rules plus the handful
    // of app-level calls used in nearly every script), same as selectedDocTopics above, even when
    // this part named no specific topics of its own (noneOrAmbiguous) — deliberately NOT gated by
    // that guard the way selectedDocTopics is: that guard exists to respect an EXPLICIT "none"
    // answer for LOCAL doc attachment (a real zero-docs choice worth honoring there), but this
    // hint has no such tradeoff to protect — it's pure signal for an optional server-side
    // retrieval step, and there's no reason a part with no NAMED topics should get LESS scoping
    // than falling back to baseline, rather than none at all.
    const QStringList baselineTopics = m_content ? m_content->baselineDocTopics() : QStringList();
    const QStringList hintTopics = baselineTopics + (noneOrAmbiguous ? QStringList() : part.docTopics);
    const QString topicsHint = (docsMode() == DocsMode::Integrated && !hintTopics.isEmpty())
                                   ? "\n\nTOPICS: " + hintTopics.join(", ")
                                   : QString();

    QStringList allParts;
    for (int i = 0; i < splitPlan.size(); ++i)
        allParts << QString("%1. %2%3")
                        .arg(i + 1)
                        .arg(splitPlan[i].description)
                        .arg(i == index ? "   <-- THIS PART — generate ONLY this one now"
                                        : (i < index ? "  (already ran successfully)" : ""));

    // Isolated, single-line block repeated as the VERY LAST thing in the prompt — separate from
    // the "Full plan for context" listing above, where this part's own (often short) description
    // sits immediately next to OTHER parts' (sometimes much longer/denser) descriptions, with only
    // a small arrow marker distinguishing which one is actually current. Confirmed live: given
    // exactly that shape (a short "compile a function" line sitting right above a long, multi-
    // clause "configure, compare, plot, style" line), a part generated the LATER, longer part's
    // content instead of its own — the model latched onto the more substantial-looking neighbor
    // despite the arrow. Position alone wasn't the gap (the existing "Generate a script for ONLY
    // this part: ..." line already sits at the very end of the prompt) — this block has nothing
    // else in it to compete with instead: no neighboring part descriptions, no plan-listing
    // formatting, just this part's own task, so there's nothing for attention to drift to.
    const QString currentRequestBlock = "\n\n=== CURRENT REQUEST — the ONLY thing to generate now ===\n" +
                                        QString::number(index + 1) + ". " + part.description +
                                        "\n=== END CURRENT REQUEST ===";

    // Confirmed live: even though the split-plan level already skipped/exempted an existence
    // check for one of these names (Rule 6 sandbox), the model still re-derived "does this exist?"
    // itself inside its own EXPLORE step, wasting a round on a question the system already
    // settled. Telling the part outright, right next to its isolated task block, stops that.
    QStringList exemptNamesInThisPart;
    for (const QString &name : splitPlanSandboxExemptNames)
        if (part.description.contains(name) && !exemptNamesInThisPart.contains(name))
            exemptNamesInThisPart << name;
    const QString sandboxExemptNote =
        exemptNamesInThisPart.isEmpty()
            ? QString()
            : ("\n\nNote: " + exemptNamesInThisPart.join(", ") +
               " already confirmed safe to compile/overwrite without checking existence first "
               "(reserved \"ai/\" namespace, compile-python-api.md Rule 6) — skip any existence "
               "check for it and proceed directly.");

    const QString prompt =
        "Your actual task is stated in the \"=== CURRENT REQUEST ===\" section at the very end of "
        "this message — read that section first if it helps. Everything between here and there "
        "(the full plan, the original request, prior turns) is reference material to consult while "
        "doing that task, not a list of additional things to do.\n\n"
        "Split-mode part " +
        QString::number(index + 1) + " of " + QString::number(splitPlan.size()) +
        ".\n\nFull plan for context — do not redo an earlier part, do not jump ahead to a later one:\n" +
        allParts.join("\n") +
        "\n\nOriginal request — its names, formulas, numbers and roles are authoritative; the part description "
        "below only says WHICH of its steps this script does (where they differ, the request is right):\n" +
        originalRequestText + "\n\nGenerate a script for ONLY this part: " + part.description + topicsHint +
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
                         "themselves (the same rule that already governs EXPLORE rounds)." +
                         createdByEarlierPartsNote()
                   : QString()) +
        m_earlierItemsNote + currentRequestBlock + sandboxExemptNote + m_partMissingNote;
    m_partMissingNote.clear();
    // historyText: a short "Part N of M: <description>" instead of `prompt` itself — the ONE call
    // in this file not already following startRequest()'s own rule (raw scaffolding must never be
    // what gets stored in m_history). Confirmed live: the full plan re-listing was being saved
    // verbatim every part, compounding into the "N older turn(s) omitted" budget pressure seen in
    // every multi-part trace, and duplicating stale "THIS PART" markers across history.
    const QString historyText = QString("Part %1 of %2: %3").arg(index + 1).arg(splitPlan.size()).arg(part.description);
    ask(prompt, true, QString(), selectedDocTopics, true, true, true, historyText);
}

void AiAssistant::advanceSplitPart(const QString &code)
{
    // The names this part's description says it creates must exist now — a part counted as done when its script ran
    // without an error, never whether it delivered (confirmed live, LIST 5 2026-10-11: later parts recreated or missed
    // what an earlier part should have made). Missing: the part runs once more with the reason; still missing, the
    // plan goes on and the later parts see the real state (createdByEarlierPartsNote()). A part whose description
    // writes a script into a tab creates its names only when that tab runs — not checked.
    if (m_host && !m_partVerifyRetried &&
        !splitPlan[splitPartIndex].description.contains("script", Qt::CaseInsensitive))
    {
        const QString description = splitPlan[splitPartIndex].description;
        const QList<QPair<QString, QStringList>> planned = splitPlanCreationNames({splitPlan[splitPartIndex]});
        // Only names of objects: a name the description gives to a tab, column, layer or label is part of an object,
        // never one itself. Confirmed live (series 7, local, LIST 5 item 5): "a note named 'Pipeline' with two tabs
        // named 'make' and 'check'" — "make" was reported missing, the part ran again and added a third tab.
        static const QRegularExpression nounRe(R"(\b(tabs?|columns?|layers?|labels?|parameters?|table|graph)"
                                               R"(|matrix|note|functions?|panels?|surface|plot|window)\b)",
                                               QRegularExpression::CaseInsensitiveOption);
        static const QStringList partNouns{"tab",    "tabs",  "column", "columns",   "layer",
                                           "layers", "label", "labels", "parameter", "parameters"};
        QStringList names;
        for (const QPair<QString, QStringList> &entry : planned)
        {
            const qsizetype at = description.indexOf(entry.first);
            QString noun;
            for (auto it = nounRe.globalMatch(description.left(qMax<qsizetype>(at, 0))); it.hasNext();)
                noun = it.next().captured(1).toLower();
            if (!partNouns.contains(noun))
                names << entry.first;
        }
        const QList<QStringList> kinds = names.isEmpty() ? QList<QStringList>() : m_host->entityKinds(names);
        QStringList missing;
        if (kinds.size() == names.size())
            for (int i = 0; i < names.size(); ++i)
                if (kinds[i].isEmpty())
                    missing << "\"" + names[i] + "\"";

        // A created table must also have the columns the description names. Confirmed live (series 7, local, LIST 5
        // item 1): part 1 ran only newTable("Calib", 21, 3) — taken from an EXPLORE reply — so "Calib" existed but had
        // no "x", "y", "dy"; the part counted as done and the next two failed. The table is the one the description
        // creates ("create table "Calib" ... columns "x", ..."), whether or not it is phrased with "named".
        static const QRegularExpression createdTableRe(
            R"RX(\b(?:create|make|build)\b[^.;]*?\btable\s+(?:named\s+|called\s+)?["']([^"'\n]+)["'])RX",
            QRegularExpression::CaseInsensitiveOption);
        // the quoted list right after "columns" ("columns "x", "y" and "dy"") — not quoted values further on
        // ("column 2 holds "alpha", "beta"")
        static const QRegularExpression columnsClauseRe(
            R"RX(\bcolumns?\s*(?:named\s+|called\s+|:\s*)?((?:["'][A-Za-z_]\w{0,30}["']\s*(?:,|\band\b)?\s*)+))RX",
            QRegularExpression::CaseInsensitiveOption);
        // identifier-like only: a quoted formula or a result label ("R1:") is no column name
        static const QRegularExpression quotedRe(R"RX(["']([A-Za-z_]\w{0,30})["'])RX");
        if (const QRegularExpressionMatch created = createdTableRe.match(description); created.hasMatch())
        {
            const QString table = created.captured(1);
            const QString shape = m_host->tableShape(table);
            for (auto c = columnsClauseRe.globalMatch(description, created.capturedEnd()); c.hasNext();)
                for (auto q = quotedRe.globalMatch(c.next().captured(1)); q.hasNext();)
                {
                    const QString column = q.next().captured(1);
                    const QString entry = "column \"" + column + "\" of table \"" + table + "\"";
                    if (!shape.isEmpty() && column != table && !shape.contains("\"" + column + "\"") &&
                        !missing.contains(entry))
                        missing << entry;
                }
        }
        if (!missing.isEmpty())
        {
            m_partVerifyRetried = true;
            m_partMissingNote = "\n\nThis part ran once already, but " + missing.join(", ") +
                                " does not exist afterwards — this part's script must create it.";
            m_host->print(QString("▸▸▸ AI split-mode: part %1/%2 ran, but %3 does not exist — running the part "
                                  "once more.")
                              .arg(splitPartIndex + 1)
                              .arg(splitPlan.size())
                              .arg(missing.join(", ")));
            startSplitPart(splitPartIndex);
            return;
        }
    }
    m_partVerifyRetried = false;
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
    // listModeActive also skips the merge — it's a NEVER-EXECUTED reference script (see
    // handleSplitMergeResponse()'s own comment), and firing it here would block the next list
    // item behind an extra, pointless round-trip: this AiAssistant only ever has one request in
    // flight (see m_busy), so a pending merge call would reject startNextListItem()'s own ask().
    if (!m_splitMergeEnabled || listModeActive)
    {
        if (m_host)
            m_host->print(QString("▸▸▸ AI split-mode: all %1 part(s) completed — merge step skipped (%2).")
                              .arg(splitPlan.size())
                              .arg(listModeActive ? "LIST mode" : "disabled"));
        splitModeActive = false; // split-mode conversation state ends here, same as after a merge
        if (listModeActive)
            advanceListItem();
        return;
    }
    if (m_host)
        m_host->print(
            QString("▸▸▸ AI split-mode: all %1 part(s) completed — requesting merged script.").arg(splitPlan.size()));
    requestSplitMerge();
}

void AiAssistant::beginListRequest(const ParsedListRequest &parsed, bool splitByDefault, bool reviewByDefault,
                                   bool exploreByDefault, bool autoRunValue)
{
    // The instruction is appended to every item as it is, no model call. A rewrite call used to merge it into each
    // item without the API docs, and confirmed live it turned "write that line into table Results" into invented
    // calls (table("Results").setItem(...)) that every item then copied, each costing a retry; it also cost 20-30 s.
    const QStringList items = itemsWithInstruction(parsed.items, parsed.instructionBlock);
    printParsedListItems(items, !parsed.instructionBlock.trimmed().isEmpty());
    startListItems(items, parsed.forceSplit, parsed.forceNoSplit, parsed.forceReview, parsed.forceNoReview,
                   parsed.forceExplore, parsed.forceNoExplore, splitByDefault, reviewByDefault, exploreByDefault,
                   autoRunValue);
}

void AiAssistant::printParsedListItems(const QStringList &items, bool wasTransformed)
{
    if (!m_host)
        return;
    QStringList summary;
    for (int i = 0; i < items.size(); ++i)
        summary << QString("  %1. %2").arg(i + 1).arg(items[i]);
    const QString header =
        wasTransformed ? "instruction applied to all " + QString::number(items.size()) : QString::number(items.size());
    m_host->print("▸▸▸ AI: list " + header + " item(s):\n\n" + summary.join("\n"));
}

QStringList AiAssistant::itemsWithInstruction(const QStringList &items, const QString &instruction)
{
    if (instruction.trimmed().isEmpty())
        return items;
    QStringList result;
    for (const QString &item : items)
        result << item + "\n\nInstruction for every item of this list: " + instruction.trimmed();
    return result;
}

void AiAssistant::startListItems(const QStringList &items, bool forceSplit, bool forceNoSplit, bool forceReview,
                                 bool forceNoReview, bool forceExplore, bool forceNoExplore, bool splitByDefault,
                                 bool reviewByDefault, bool exploreByDefault, bool autoRunValue)
{
    listModeActive = true;
    m_listTotalTimer.start();
    listItems.clear();
    for (const QString &text : items)
    {
        ListItem item;
        item.text = text;
        listItems << item;
    }
    listItemIndex = -1;
    m_listObjectsAtStart = m_host ? m_host->liveObjects() : QStringList();
    m_earlierItems.clear();
    listItemForceSplit = forceSplit;
    listItemForceNoSplit = forceNoSplit;
    listItemForceReview = forceReview;
    listItemForceNoReview = forceNoReview;
    listItemForceExplore = forceExplore;
    listItemForceNoExplore = forceNoExplore;
    listSplitByDefault = splitByDefault;
    listReviewByDefault = reviewByDefault;
    listExploreByDefault = exploreByDefault;
    autoRun = autoRunValue;
    // Mirrors m_contextHistoryStart's exact mechanism — see m_listHistoryStart's own doc comment.
    m_listHistoryStart = m_history.size();
    startNextListItem();
}

// Elapsed time for the item that JUST finished, formatted like m_lastCallMs's own "Xsec" — printed
// once, at the point the NEXT item starts (or the list completes), never retroactively edited into
// an already-printed line. Empty before any item has actually finished (listItemIndex < 0).
QString AiAssistant::listItemElapsedNote() const
{
    if (listItemIndex < 0 || !m_listItemTimer.isValid())
        return {};
    const QString item = QString("took %1sec").arg(static_cast<double>(m_listItemTimer.elapsed()) / 1000.0, 0, 'f', 1);
    if (!m_listTotalTimer.isValid())
        return " (" + item + ")";
    // Running total since the LIST started, so a long list's accumulated cost is visible without
    // adding up every per-item figure by hand. Minutes once past 60s — a bare "1574.3sec" on a
    // 12-item list is accurate but not readable at a glance.
    const qint64 totalMs = m_listTotalTimer.elapsed();
    const QString total = totalMs < 60000
                              ? QString("%1sec total").arg(static_cast<double>(totalMs) / 1000.0, 0, 'f', 1)
                              : QString("%1min %2sec total").arg(totalMs / 60000).arg((totalMs % 60000) / 1000);
    return " (" + item + ", " + total + ")";
}

void AiAssistant::startNextListItem()
{
    const QString finishedNote = listItemElapsedNote();
    recordFinishedListItem();
    ++listItemIndex;
    if (listItemIndex >= listItems.size())
    {
        listModeActive = false;
        if (m_host)
            m_host->print(QString("▸▸  AI LIST: all %1 item(s) completed%2.").arg(listItems.size()).arg(finishedNote));
        return;
    }
    const ListItem &item = listItems[listItemIndex];
    if (m_host)
        m_host->print(QString("▸▸  AI LIST: item %1/%2%3 — %4")
                          .arg(listItemIndex + 1)
                          .arg(listItems.size())
                          .arg(finishedNote)
                          .arg(item.text));
    m_listItemTimer.start();
    m_itemObjectsAtStart = m_host ? m_host->liveObjects() : QStringList();
    m_itemRanScripts.clear();
    const bool wantsSplit = listItemForceSplit ? true : listItemForceNoSplit ? false : listSplitByDefault;
    setSelfReviewEnabled(listItemForceReview ? true : listItemForceNoReview ? false : listReviewByDefault);
    // Re-applied per item, exactly like self-review above: every list item runs through the normal
    // single-request pipeline, and nothing else restores these between items.
    setExploreEnabled(listItemForceExplore ? true : listItemForceNoExplore ? false : listExploreByDefault);
    // Each item is a genuinely fresh, independent request — not a follow-up to the previous item
    // — so originalRequestText must be cleared first; beginRequest() only sets it when empty (see
    // its own doc comment), which is exactly the "new top-level request" behavior every item needs.
    originalRequestText.clear();
    // Closed at THIS item's boundary. Briefly widened to the whole LIST so a back-reference ("plot
    // the fit results from the previous step") had something behind it; reverted after that made
    // later items replay earlier items' SCRIPTS prepended to their own — confirmed live, one run
    // where item 4 re-created item 2's table and re-did item 3's work while never creating the note
    // it was asked for, and items 9-11 each re-ran the whole create+fit+plot pipeline. A dangling
    // reference costs one EXPLORE round; script-latching silently produces the wrong work.
    m_contextHistoryStart = m_history.size();
    // A correction belongs to the item it was given for. Confirmed live (supervised LIST 3 run): a correction with code
    // for item 6's matrix stayed in the prompts of items 7 and 9, and the local model wrote that matrix code there.
    clearUserCorrections();
    m_itemOutput.clear();
    m_resultLineRetried = false;
    m_lastBlockedScript.clear();
    m_freshRetryUsed = false;
    m_blockedRounds = 0;
    // The same transport suffix askAi() appends to a single request: an item is one, and in generate-only mode the
    // suffix is what says nothing in this session has run.
    // The result line's values as a numbered list, from the request's own Print sentence: the most frequent local-pyi
    // failure was a result line short of values (the others on lines of their own), checked only after the script
    // was written; said up front, it costs a line. Only for two or more values, where an order can be lost.
    QString itemText = item.text;
    if (m_content)
    {
        const QStringList values = m_content->resultLineValueList(item.text);
        if (values.size() >= 2)
        {
            QStringList numbered;
            for (int i = 0; i < static_cast<int>(values.size()); ++i)
                numbered << QString("%1) %2").arg(i + 1).arg(values.at(i));
            itemText += "\nIts result line holds these values, in this order: " + numbered.join("; ") + ".";
        }
    }
    beginRequest(itemText + (m_content ? m_content->requestSuffix(autoRun) : QString()), autoRun, wantsSplit);
}

void AiAssistant::advanceListItem()
{
    if (requestMissingResultLine())
        return;
    startNextListItem();
}

bool AiAssistant::requestMissingResultLine()
{
    if (!m_content || !m_host || m_resultLineRetried || listItemIndex < 0 || listItemIndex >= listItems.size())
        return false;
    const QString itemText = withoutRequestSuffix(listItems[listItemIndex].text);
    const QString label = m_content->resultLabel(itemText);
    if (label.isEmpty())
        return false;
    const QRegularExpression printedRe("^\\s*" + QRegularExpression::escape(label),
                                       QRegularExpression::MultilineOption);
    if (printedRe.match(m_itemOutput).hasMatch())
        return false;
    // Confirmed live (LIST 5, item 4): a block removed the line from part 1, part 2 never had it, and the item ended
    // silently without a result. Once per item; everything the item made already exists, so this is a print-only task.
    m_resultLineRetried = true;
    const QString duty = m_content->resultLineDuty(itemText);
    m_resultLineTask = duty.isEmpty() ? "Print the result line \"" + label + "\" the request asks for." : duty;
    m_host->print("▸▸▸ AI: this item finished, but no script printed its \"" + label +
                  "\" result line — asking for "
                  "it once.");
    autoContinueCount = 0;
    consecutiveExploreCount = 0;
    consecutiveDocRetryCount = 0;
    m_failedRunObjects.clear();
    m_closedFailedObjects.clear();
    m_failedRunScripts.clear();
    resetErrorRetryState();
    const QString prompt =
        "Every step of this request has run, but no script printed its result line. The request asks:\n" +
        m_resultLineTask +
        "\n\nWrite a script that only computes and prints that line: look up what this request created by name (" +
        m_content->lookupByNameExamples() +
        ", ...), compute every value from the project as it is now, and print exactly ONE line starting with \"" +
        label + "\". Do not create, change or delete anything.";
    ask(prompt + topicsHintLine(selectedDocTopics), true, QString(), selectedDocTopics, true, true, true, prompt);
    return true;
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
    // expectsCode=false: merge combines already-verified code, it never explores — the
    // EXPLORE-mechanism reminder buildSystemPrompt() would otherwise add (expectsCode defaults
    // true) is pure noise here, never applicable to what this call actually does.
    ask(mergePrompt + noDocsHintLine(), false, QString(), QStringList(), false, false, false);
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
            // expectsCode=false: same reasoning as requestSplitMerge()'s own call — a merge retry
            // never explores either.
            // noDocsHintLine() to match the first merge call's own — without it this retry reached
            // the server with no TOPICS: line and took the unscoped whole-corpus fallback.
            ask(retryPrompt + noDocsHintLine(), false, QString(), QStringList(), false, false, false);
            return;
        }
    }

    splitModeActive = false; // split-mode conversation state ends here regardless of outcome
    const bool compilable = m_host->canCompile(code);
    m_host->print(QString("▸▸▸ AI split-mode merged script (%1 — NOT executed, verify before reuse):\n\n")
                      .arg(compilable ? "syntax-checked OK" : "NOT syntactically valid") +
                  code);
    // Lets a follow-up "run it" execute this exact merged script — see hasUnrunLastScript()'s doc
    // comment. Every ask() in the split-mode pipeline has addToHistory=false by design (keeps
    // per-part scaffolding out of history), so without this, the merged script was never in
    // m_history at all: "run it" here would either fall through to the model (risking the same
    // reinterpretation hallucination this fast path exists to avoid) or, worse, run some
    // unrelated stale entry. Gated on compilable, same as the plain non-split "AI code (not run)"
    // path's own isRealActionScript check.
    if (compilable)
    {
        m_history.append({originalRequestText, code});
        markLastScriptUnrun();
    }
}

// The dedicated "# Action script" marker LINE the model uses when it bundles a complete action
// script into the same response as its EXPLORE probe instead of keeping the two phases apart.
// Hoisted to file scope because handleExploreResponse() now has two users: the disabled-EXPLORE
// refusal below (which hands the bundled half back as a draft to revise) and the "already ran the
// action script" shortcut further down. MUST be a marker LINE, never a bare substring search —
// see that shortcut's own call site for the confirmed-live incident where .contains("action
// script") matched an EXPLORE script's unrelated note-to-self and wrongly ended the task.
static const QRegularExpression kActionScriptMarkerRe(R"(^[ \t]*#+[ \t\-]*action script\b)",
                                                      QRegularExpression::CaseInsensitiveOption |
                                                          QRegularExpression::MultilineOption);

// Two-phase agentic loop: EXPLORE script — run, capture output, feed back to AI.
// EXPLORE handling is split four ways: this policy function decides WHETHER a probe may run, and
// the three functions below carry out the chosen outcome. It used to be one 247-line function
// mixing policy, presentation, execution and prompt-building, which is why the disabled-EXPLORE
// path ended up duplicating the round-budget check — every new rule had to be threaded into a
// branch that also did four other things. Every "should this probe run" rule now lives here.
AiAssistant::ExploreDisposition AiAssistant::exploreDisposition() const
{
    // Off in Preferences, or NOEXPLORE on this request.
    if (!m_exploreEnabled)
        return ExploreDisposition::Refuse;
    // Generate-only mode (bare Enter/Click, autoRun == false): the user asked to SEE a script, not
    // to have anything executed against their project. A probe is model-authored code, and the
    // read-only guarantee is a blocklist of recognized write calls, not a sandbox —
    // explorePrefixBeforeFirstWriteCall()'s own doc comment records two confirmed-live leaks, one
    // of which ran an entire Compiler authoring sequence (c.new() through setCode()) for real
    // because none of those calls were in the list. Running that when the user declined execution
    // is a contract violation with a demonstrated, non-zero chance of real mutation.
    if (!autoRun)
        return ExploreDisposition::Refuse;
    return ExploreDisposition::Run;
}

// Charges this round against the shared EXPLORE/error-retry budget, reporting and recording the
// give-up when it is spent. `refusing` only selects the wording — a model that keeps sending probes
// at a refusing host and one that keeps exploring without ever producing a script are the same
// non-convergence, and the same budget has to stop both.
bool AiAssistant::exploreRoundBudgetExhausted(bool refusing)
{
    if (++autoContinueCount <= maxAutoContinue())
        return false;
    recordGiveUpInHistory(refusing ? QString("stopped after %1 rounds — kept returning # EXPLORE scripts while "
                                             "EXPLORE was not available")
                                         .arg(maxAutoContinue())
                                   : QString("stopped after %1 automatic EXPLORE/retry rounds").arg(maxAutoContinue()),
                          QString());
    if (m_host)
    {
        QString msg = refusing ? "▸▸▸ AI: stopped after " + QString::number(maxAutoContinue()) +
                                     " rounds — the AI kept returning # EXPLORE scripts, but probe rounds are "
                                     "not available for this request."
                               : "▸▸▸ AI: stopped after " + QString::number(maxAutoContinue()) +
                                     " automatic EXPLORE/retry rounds — no action script produced yet.";
        msg += splitModeGiveUpNote();
        m_host->print(msg);
    }
    return true;
}

// exploreModeReminder() asks for "# EXPLORE: <purpose>" on the first line — surfaced here so a long
// trace is scannable without reading the whole script. Falls back to the plain header when no
// purpose is given (an older/bare "# EXPLORE" still matches this file's own detection regex, which
// only requires a word boundary after EXPLORE).
void AiAssistant::printExploreScript(const QString &code)
{
    if (!m_host)
        return;
    static const QRegularExpression explorePurposeRe(R"(^#\s*EXPLORE\s*:?\s*(.*)$)",
                                                     QRegularExpression::CaseInsensitiveOption);
    const int firstLineEnd = static_cast<int>(code.indexOf('\n'));
    const QString firstLine = firstLineEnd < 0 ? code : code.left(firstLineEnd);
    const QRegularExpressionMatch purposeMatch = explorePurposeRe.match(firstLine);
    const QString purpose = purposeMatch.hasMatch() ? purposeMatch.captured(1).trimmed() : QString();
    m_host->print(purpose.isEmpty() ? "▸▸▸ AI code (exploring):\n\n" + code
                                    : "▸▸▸ AI code (exploring: " + purpose + "):\n\n" + code);
}

// Shows the probe without running it and asks for the action script instead.
//
// buildSystemPrompt() already stops asking for probe rounds when EXPLORE is off, but that alone
// cannot prevent one: outside Integrated docs mode the API corpus carries its own "Agentic Explore
// Mode" rules, so the model can still open with "# EXPLORE" on the docs' authority. Refuse it once
// — never run it (the setting would mean nothing), and never dead-end the request either, which is
// the failure this whole path exists to avoid.
void AiAssistant::refuseExploreRound(const QString &code)
{
    if (exploreRoundBudgetExhausted(/*refusing=*/true))
        return;
    // Split the reply at the "# ACTION SCRIPT" marker, when there is one: the probe half is what
    // gets shown as "not run", and the action half is either handed back for revision (below) or —
    // in generate-only mode — simply shown as the answer.
    const QRegularExpressionMatch bundledMatch = kActionScriptMarkerRe.match(code);
    const QString bundledDraft = bundledMatch.hasMatch() ? code.mid(bundledMatch.capturedStart()).trimmed() : QString();
    const QString probeOnly = bundledMatch.hasMatch() ? code.left(bundledMatch.capturedStart()).trimmed() : code;
    if (m_host)
        m_host->print(m_exploreEnabled ? "▸▸▸ AI code (EXPLORE not run — generate-only mode):\n\n" + probeOnly
                                       : "▸▸▸ AI code (EXPLORE disabled — not run):\n\n" + probeOnly);
    // Generate-only mode with the action script ALREADY in the reply: show it and stop. Asking again
    // would spend a whole round-trip reproducing code that is on screen, and nothing is going to run
    // it, so a script written before the model knew no probe output was coming can at worst be
    // suboptimal — never harmful. Deliberately NOT done when auto-run is on and only the EXPLORE
    // setting is off: there the script WOULD execute, so a draft resting on unverified assumptions
    // has to be rewritten knowing it must read those values at run time.
    //
    // Routed back through responseReady() rather than printed directly, so the action half takes the
    // exact same path any other answer does — host-side display, markLastScriptUnrun() for a later
    // "run it", and (the reason this matters) the LIST advance that lives in that handler. Printing
    // it here instead would stall a generate-only LIST on this item forever. Deferred by one event
    // loop turn, matching how list continuation is scheduled elsewhere in this file, so this item's
    // own output is fully emitted before the next one starts.
    if (!autoRun && !bundledDraft.isEmpty())
    {
        if (m_host)
            m_host->print("▸▸▸ AI: the reply also carried a complete action script — showing that "
                          "instead of asking for it again, since nothing runs in generate-only mode.");
        QTimer::singleShot(0, this, [this, bundledDraft]() { emit responseReady(bundledDraft); });
        return;
    }
    // A response that bundles "# EXPLORE" AND a complete "# ACTION SCRIPT" into one reply gets its
    // action half handed back instead of thrown away. Discarding it outright is exactly the
    // condition the run path's own discardedDraftBlock exists to avoid — see its doc comment: a
    // draft dropped without feedback had invented a nonexistent group ("custom:sans"), and the very
    // next round, now holding the real list, wrote the identical invented name again because
    // nothing tied the rewrite to what had just been rejected. Confirmed live here too: this path's
    // first real firing discarded a bundled draft carrying that same "custom:sans".
    const QString bundledDraftBlock =
        bundledDraft.isEmpty()
            ? QString()
            : "\n\nYou bundled this action script into the same response, written BEFORE you knew no "
              "exploration output was coming (discarded, never executed):\n```\n" +
                  bundledDraft +
                  "\n```\nRevise THAT script rather than starting over — but treat every value in it "
                  "that only a probe could have confirmed (a group name, a function name, a column "
                  "label, a template name) as an unverified guess: either replace it with something "
                  "the documentation states outright, or have the script read it at run time. Do not "
                  "carry such a guess through unchanged just because you already wrote it once.";
    const QString followUp =
        originalRequestReminder() +
        "The script above was a \"# EXPLORE\" probe, but probe rounds are NOT AVAILABLE for this "
        "request — it was NOT executed, and nothing was read from the project. You will not receive "
        "any exploration output, now or later, so do not wait for it and do not ask again.\n\n"
        "Write and return the ACTION script right now (no # EXPLORE marker), working only from the API "
        "documentation and this conversation. Where a probe would have told you a live value, write "
        "code that reads it AT RUN TIME inside the action script itself and branches on what it finds "
        "— that is the supported way to handle unknown project state here, and it needs no separate "
        "round-trip. Do not invent a value you would have checked, and do not drop part of the task "
        "because you could not check it first." +
        projectFactsNote() + bundledDraftBlock;
    // Same call shape as requestActionScriptAfterExplore()'s own ask() — see its
    // topicsHintLine()/historyText notes for why the hinted text is not what gets stored.
    ask(followUp + topicsHintLine(selectedDocTopics), true, QString(), selectedDocTopics, true, true, true, followUp);
}

// Runs the probe's read-only prefix and returns its output. False means "stop here" — either there
// is no host, or the response bundled an action script that has now already run.
bool AiAssistant::runExploreProbe(const QString &code, QString *output, QString *discardedDraft)
{
    if (!m_host)
        return false;
    // EXPLORE is supposed to be strictly read-only (ai-python-api.md's own rule) — nothing before
    // this point mechanically enforces that, so a response that bundles real write/create calls in
    // with its EXPLORE lines would otherwise execute all of it for real. Run only the safe,
    // read-only PREFIX; a host-side-only note (never sent back to the model) flags that a suffix was
    // skipped, purely for local transparency. See
    // AiContentProvider::explorePrefixBeforeFirstWriteCall()'s doc comment for the confirmed-live
    // incident this closes — and for the two it did NOT, which is why exploreDisposition() refuses
    // to run any probe at all in generate-only mode.
    const QString safeCode = m_content->explorePrefixBeforeFirstWriteCall(code);
    // Everything cut away above (empty when nothing was truncated) — kept only long enough to seed
    // the follow-up prompt, never executed, never stored past this call. Confirmed live: asked to
    // compile "ai/PorodFit", the discarded draft here invented a nonexistent group ("custom:sans",
    // matching none of the real c.groups()) before it could have seen the real list; simply
    // discarding it meant the very next round, now holding that real list in its own prompt, wrote
    // the identical invented group again — nothing tied the regeneration to what had just become
    // visible. Feeding this draft back alongside the real exploration output gives the model
    // something concrete to revise instead of a blank slate with no reason to differ.
    *discardedDraft = code.mid(safeCode.length());
    if (safeCode.length() != code.length())
        m_host->print("▸▸▸ AI: this EXPLORE script contained write/action calls past the point shown "
                      "above — only the read-only prefix was executed; the rest was skipped.");
    // Same mechanical scan runActionScript() already runs before executing a candidate script —
    // EXPLORE previously skipped it entirely and could crash at runtime (e.g. scriptPrint()'s
    // multi-arg mistake) on something already cataloged and normally caught for free beforehand.
    const QList<AiKnownMistakeMatch> knownMistakes =
        m_content ? m_content->detectKnownMistakes(safeCode, mistakeCheckRequestText(), splitPartScripts.join("\n\n"),
                                                   laterSplitPartDescriptions(), /*exploreScript=*/true)
                  : QList<AiKnownMistakeMatch>();
    if (!knownMistakes.isEmpty())
    {
        QStringList headers, explanations;
        for (const AiKnownMistakeMatch &m : knownMistakes)
        {
            headers << m.header;
            explanations << m.explanation;
        }
        m_host->print("▸▸▸ AI: this EXPLORE script was blocked before running — " + headers.join("; ") + ".");
        *output = explanations.join("\n\n");
        return true;
    }
    // Display already happened live during runScript() itself — do NOT print exploreResult.output
    // here too, see that method's doc comment on why that would double every scriptPrint() line.
    // A probe in a part before the last does not print the result line either: confirmed live (benchmark LIST 5, item
    // 6), part 1's probe printed "R6: [...]" before anything was deleted — the item's answer if a later part fails.
    QString probeCode = safeCode;
    if (m_content && !laterSplitPartDescriptions().isEmpty())
        if (const QString stripped = m_content->nonFinalResultLineRepair(safeCode, originalRequestText, true);
            !stripped.isEmpty())
            probeCode = stripped;
    const AiScriptResult exploreResult = m_host->runScript(probeCode, "<ai-explore>");
    *output = exploreResult.output.isEmpty() ? "(no output)" : exploreResult.output;
    m_itemOutput += exploreResult.output + "\n";

    // The model sometimes bundles a complete action script into the same response as EXPLORE
    // (marked by a dedicated "# Action script" comment LINE) instead of keeping the two-phase split
    // intact — that combined script already ran above. Asking for "the action script" again would
    // redundantly re-run the whole thing: confirmed live, it recompiled the same fit function,
    // regenerated the same table, and refit, all from scratch. Skip the follow-up when that marker
    // is present AND this run raised no error — the task is already done. If it DID error, fall
    // through to the normal path so the failure still gets surfaced.
    // MUST be a dedicated marker line, not a bare substring search — a plain
    // .contains("action script") also matches an EXPLORE script's own unrelated comment merely
    // mentioning the words in passing (confirmed live: "# -- after seeing real output, write the
    // action script:" is a note-to-self inside a pure read-only EXPLORE script, not a bundled action
    // script), which wrongly ended the task right there — twice in a row, including once when the
    // user's new instruction ("select yourself column") was never even attempted.
    // Checked against `safeCode` (what actually ran), not the original `code` — if the marker sits
    // past explorePrefixBeforeFirstWriteCall()'s cut point, that bundled action script was skipped,
    // not executed, so "already done" would be wrong.
    //
    // Confirmed live: that reasoning alone isn't enough — explorePrefixBeforeFirstWriteCall() only
    // cuts starting at a matched WRITE-CALL line, so a marker COMMENT sitting anywhere before that
    // call survives truncation intact even when everything the marker was introducing (a real
    // c.compile() call, in the confirmed incident) got cut away right after it. `safeCode`
    // containing the marker text is therefore not proof the action script actually ran in full —
    // only `safeCode.length() == code.length()` (nothing was truncated at all) is. Requiring that
    // explicitly closes the gap regardless of how complete that write-call list happens to be.
    //
    // Nor is a marker proof the action script did its job: confirmed live (LIST 5, item 6), a reply cut short as
    // repetition-loop garbage kept only "c = Compiler" after the marker, deleted nothing, and the part counted as done.
    // A bundled action script never went through the deliverable check, so it is checked here, after the fact: when
    // it misses something its task asked for, the usual follow-up asks for the action script with this output.
    const bool bundledAction = safeCode.length() == code.length() && exploreResult.error.isEmpty() &&
                               kActionScriptMarkerRe.match(safeCode).hasMatch();
    const QString bundledMissing =
        bundledAction && m_content
            ? m_content->missingRequestedDeliverables(
                  safeCode.mid(kActionScriptMarkerRe.match(safeCode).capturedStart()), currentScopeDescription())
            : QString();
    if (bundledAction && !bundledMissing.isEmpty())
        m_host->print("▸▸▸ AI: the action script bundled with this EXPLORE did not do the whole task — asking for "
                      "the action script.");
    if (bundledAction && bundledMissing.isEmpty())
    {
        m_host->print("▸▸▸ AI: EXPLORE response already included and ran the action script — done.");
        // The same success path as a script run by runActionScript(): without it a split part or LIST item that ended
        // here never moved on — confirmed live (LIST 5 with EXPLORE on), the list waited 40 minutes on item 3.
        consecutiveDocRetryCount = 0;
        m_failedRunObjects.clear();
        m_closedFailedObjects.clear();
        m_failedRunScripts.clear();
        m_itemRanScripts << safeCode;
        if (splitModeActive)
            advanceSplitPart(safeCode);
        else if (listModeActive)
            advanceListItem();
        return false;
    }
    return true;
}

// Feeds the exploration output back and asks for the action script, escalating when the model keeps
// probing instead of producing one.
void AiAssistant::requestActionScriptAfterExplore(const QString &code, const QString &output,
                                                  const QString &discardedDraft)
{
    // Kept for maybeDispatchSelfReview()'s exploreGroupsIgnoredMistake() check — see
    // lastExploreOutput's own doc comment (AiAssistant.h) for why this needs to survive past this
    // function's return.
    lastExploreOutput = output;

    // Confirmed live: with the full request history intact (no truncation — the transcript's own
    // "N history turn(s)" headers showed no omission) and every fact already printed earlier in this
    // SAME conversation, six straight EXPLORE rounds re-verified already-known state (table columns,
    // curve counts, a table's existence) instead of ever comparing it against the exact string that
    // had actually failed — burning the entire remaining autoContinueCount budget on zero net
    // progress before the request died at the round cap. The soft "Now generate the action script"
    // phrasing below is already sent every round and was plainly not enough on its own to break that
    // loop. Past kMaxConsecutiveExplore, escalate to a much more direct instruction instead of
    // repeating the same phrasing a fourth/fifth/sixth time.
    // A byte-identical repeat means this round produced nothing new to converge on — waiting for the
    // round cap just spends another 40-50s reproducing it. Confirmed live: the same EXPLORE script
    // came back 3 rounds running, unchanged.
    // The read-only probes compared, not the whole replies: confirmed live (LIST 5, item 3), the same probe came back
    // with a bundled action script that differed by one import line, and ran again for nothing.
    const bool repeatedIdenticalExplore =
        !lastExploreCode.isEmpty() &&
        (code == lastExploreCode || (m_content && m_content->explorePrefixBeforeFirstWriteCall(code) ==
                                                      m_content->explorePrefixBeforeFirstWriteCall(lastExploreCode)));
    lastExploreCode = code;
    if (repeatedIdenticalExplore && m_host)
        m_host->print("▸▸▸ AI: this EXPLORE probe is identical to the previous round's — exploring "
                      "again can't add anything, asking for the action script instead.");

    ++consecutiveExploreCount;
    static const int kMaxConsecutiveExplore = 3;
    // Empty whenever nothing was truncated (the common case) — both followUp branches below are then
    // byte-for-byte identical to before this block existed.
    const QString discardedDraftBlock =
        discardedDraft.trimmed().isEmpty()
            ? QString()
            : "\n\nBefore seeing the exploration output below, you had already started writing this "
              "(discarded, never executed):\n```\n" +
                  discardedDraft.trimmed() +
                  "\n```\nRevise it "
                  "using the real values below instead of the ones you guessed — don't repeat a value "
                  "you invented before you could see this data. NOTE: this is a FRAGMENT, cut at the "
                  "first write call, so its own setup lines (\"f = Fittable\", \"c = Compiler\", "
                  "\"import random\", PyQt imports, ...) are very likely missing — they were on the "
                  "other side of the cut. Do not return it as-is: every script runs in a fresh "
                  "namespace, so rewrite the setup lines it needs at the top. Confirmed live: this "
                  "fragment began at \"f.configure(...)\" and was returned verbatim, dying on "
                  "\"NameError: name 'f' is not defined\".";
    const QString followUp =
        (consecutiveExploreCount >= kMaxConsecutiveExplore || repeatedIdenticalExplore)
            ? originalRequestReminder() + "Exploration output:\n```\n" + output + "\n```" + discardedDraftBlock +
                  "\nThis is EXPLORE round " + QString::number(consecutiveExploreCount) +
                  " in a row with no action-script attempt in between. STOP EXPLORING NOW — everything "
                  "needed is already in this conversation's own history, including your own prior "
                  "EXPLORE output. Do NOT return another # EXPLORE script. Write and return the "
                  "corrected ACTION script right now (no # EXPLORE marker), using the facts you have "
                  "already confirmed — do not re-verify anything you already checked earlier in this "
                  "exchange."
            : originalRequestReminder() + "Exploration output:\n```\n" + output + "\n```" + discardedDraftBlock +
                  "\nNow generate the action script.";
    // TOPICS: hint for an optional server-side RAG proxy — confirmed live missing here too (see
    // startRequest()'s doc comment on the same gap and topicsHintLine()'s doc comment): this call
    // generates the real action script right after an EXPLORE round, exactly the kind of call that
    // benefits from topic-scoped retrieval, and previously had no hint line at all.
    // historyText=followUp (pre-hint): see startRequest()'s doc comment on why the hinted text must
    // never be what actually gets stored in m_history.
    ask(followUp + topicsHintLine(selectedDocTopics), true, QString(), selectedDocTopics, true, true, true, followUp);
}

// True when `code` holds a line that is neither blank nor a comment.
static bool hasExecutableLine(const QString &code)
{
    return std::ranges::any_of(code.split('\n'), [](const QString &line) {
        const QString trimmed = line.trimmed();
        return !trimmed.isEmpty() && !trimmed.startsWith('#');
    });
}

// Two-phase agentic loop orchestration: decide, then carry out. Each step's own doc comment above
// carries the confirmed-live incidents that shaped it.
void AiAssistant::handleExploreResponse(const QString &code)
{
    // Reset unconditionally, even though this flag is normally cleared where the self-review
    // response is consumed: if self-review itself ever returns an # EXPLORE script instead of a
    // corrected action script, execution returns early from here and never reaches that reset. Left
    // stale, the next real candidate script (produced after this EXPLORE round via "Now generate the
    // action script.") would be wrongly treated as an already-reviewed self-review result and skip
    // the mandatory review entirely.
    awaitingSelfReview = false;
    if (exploreDisposition() == ExploreDisposition::Refuse)
    {
        refuseExploreRound(code);
        return;
    }
    // Display BEFORE the budget check, so the round that exhausts the budget still shows what was
    // asked for rather than only the give-up line.
    printExploreScript(code);
    if (exploreRoundBudgetExhausted(/*refusing=*/false))
        return;
    // The same probe as the round before, with nothing run in between (lastExploreCode is cleared by every action
    // attempt): its output is already in hand, so it is not run again.
    if (m_content && !lastExploreCode.isEmpty() && !lastExploreOutput.isEmpty())
    {
        const QString probe = m_content->explorePrefixBeforeFirstWriteCall(code);
        if (probe == m_content->explorePrefixBeforeFirstWriteCall(lastExploreCode))
        {
            if (m_host)
                m_host->print("▸▸▸ AI: the same EXPLORE probe as the previous round — not run again; its output is "
                              "already in hand.");
            // The reply carries an action script written after that output came back: take it. Confirmed live (LIST 5
            // local pyi, item 2): the identical probe with the same bundled action script four rounds in a row, each
            // round answered with "write the action script".
            if (kActionScriptMarkerRe.match(code).hasMatch() && takeBundledActionScript(code))
                return;
            requestActionScriptAfterExplore(code, lastExploreOutput, code.mid(probe.length()));
            return;
        }
    }
    // A probe was answered already, and this reply carries an action script written after that answer came back: it is
    // the answer, not another question. Confirmed live (LIST 5 NOSPLIT local pyi, item 2): after "Table 'Peak5' not
    // found." the same check came back twice more, each with its action script, differing by one import line, so the
    // identical-probe rule above did not see the repeat.
    if (!lastExploreOutput.isEmpty() && kActionScriptMarkerRe.match(code).hasMatch() && takeBundledActionScript(code))
        return;
    // A probe with nothing to run: the reply is an action script under an EXPLORE header. Confirmed live (LIST 5
    // NOSPLIT, item 4): "# EXPLORE" over m = newMatrix(...) ... — the read-only prefix was the header alone, nothing
    // ran, and the next round only asked for the script that was already there.
    const QString probe = m_content ? m_content->explorePrefixBeforeFirstWriteCall(code) : code;
    if (!hasExecutableLine(probe) && takeBundledActionScript(code))
        return;
    QString output;
    QString discardedDraft;
    if (!runExploreProbe(code, &output, &discardedDraft))
        return;
    // A probe that printed nothing taught nothing: the next round would write the bundled action script again
    // knowing exactly what it knew when it wrote it. Only with an "# ACTION SCRIPT" marker: confirmed live (LIST 5
    // NOSPLIT re-run, item 5), a probe cut at newNote() before its first print had no action script behind it at all.
    if (!discardedDraft.trimmed().isEmpty() && output == QLatin1String("(no output)") &&
        kActionScriptMarkerRe.match(code).hasMatch() && takeBundledActionScript(code))
        return;
    requestActionScriptAfterExplore(code, output, discardedDraft);
}

// Hands the reply minus its "# EXPLORE" header line to the action path, the same way refuseExploreRound() hands back a
// bundled draft — through responseReady(), so self-review, the repairs, the known-mistake checks and the LIST advance
// all apply. A read-only probe half left in it runs again, which is harmless. False when nothing executable is left.
bool AiAssistant::takeBundledActionScript(const QString &code)
{
    // From the "# ACTION SCRIPT" marker when every write statement above it is repeated below it (a read-only probe
    // half has none) — otherwise the probe half is part of the work and the whole reply goes. Both confirmed live (LIST
    // 5 NOSPLIT re-runs, item 4): newMatrix() above the marker and an action half that only read "Saddle" (the action
    // half alone dropped the matrix); and the same newMatrix() lines in both halves, where the whole reply came back
    // from self-review as the probe half alone, without the action half's plot3D(). A write statement is one line that
    // explorePrefixBeforeFirstWriteCall() cuts at, compared whitespace-simplified.
    static const QRegularExpression headerRe(R"(^#\s*EXPLORE\b[^\n]*\n?)", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch marker = kActionScriptMarkerRe.match(code);
    bool actionHalfAlone = false;
    if (marker.hasMatch() && m_content)
    {
        const QString probeHalf = code.left(marker.capturedStart());
        QSet<QString> actionLines;
        for (const QString &line : code.mid(marker.capturedStart()).split('\n'))
            actionLines.insert(line.simplified());
        actionHalfAlone = true;
        for (const QString &line : probeHalf.split('\n'))
        {
            const QString statement = line.trimmed();
            const bool isWrite = !statement.isEmpty() && !statement.startsWith('#') &&
                                 m_content->explorePrefixBeforeFirstWriteCall(statement) != statement;
            if (isWrite && !actionLines.contains(line.simplified()))
            {
                actionHalfAlone = false;
                break;
            }
        }
    }
    QString script = actionHalfAlone ? code.mid(marker.capturedStart()) : code;
    script.remove(headerRe);
    if (!hasExecutableLine(script))
        return false;
    if (m_host)
        m_host->print("▸▸▸ AI: taking the action script this EXPLORE reply carries instead of asking for it again — "
                      "another probe round has nothing to tell.");
    consecutiveExploreCount = 0;
    QTimer::singleShot(0, this, [this, script]() { emit responseReady(script.trimmed()); });
    return true;
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

// Backstop 7's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewCompileSequenceRegression()): self-review dropped one or more
// required Compiler setup calls (setFunctionName/setGroup/setParamCount/setParamNamesAll/
// setParamInitsAll/setParamLimitsAll/setParamVariesAll) while still calling compile(), with no
// basis to do so.
static QString buildCompileSequenceRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response still calls Compiler.compile(), but dropped one or more required "
           "setup calls that the reviewed script had:\n\n" +
           regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which had the complete, "
           "correct setup:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — do "
           "not drop any of its Compiler setup calls again. Return ONLY executable Python code — no "
           "explanation, no markdown fences.";
}

// Backstop 8's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewPyQtVersionSubstitution()): self-review changed an already-correct
// PyQt import's major version (PyQt5<->PyQt6) for the same Qt class, with no basis to do so.
static QString buildPyQtVersionRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response changed a PyQt import's major version with no basis to do so:\n\n" +
           regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which used the correct PyQt "
           "version:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — do "
           "not change its PyQt import version again. Return ONLY executable Python code — no "
           "explanation, no markdown fences.";
}

// Backstop 9's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewFabricatedSetCodeMistake()): self-review added a fabricated
// Compiler.setCode() call for a function the draft correctly reused via open()+compile() with no
// setCode() of its own.
static QString buildFabricatedSetCodeRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response added a fabricated Compiler.setCode() call with no basis to do "
           "so:\n\n" +
           regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which correctly reused the "
           "existing compiled function:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — do "
           "not add a setCode() call for a function it correctly reused without one. Return ONLY "
           "executable Python code — no explanation, no markdown fences.";
}

// Backstop 10's one-shot corrective prompt (see maybeDispatchSelfReview()'s use of
// AiContentProvider::selfReviewCreationNameSubstitution()): self-review changed the name passed to
// a creation call (newTable/newMatrix/newGraph/newNote) that the draft already had correct.
static QString buildCreationNameRetryPrompt(const QString &regressionExplanation, const QString &preReviewDraft)
{
    return "Your last response changed a creation call's name with no basis to do so:\n\n" + regressionExplanation +
           "\n\nHere is the script you were originally asked to review, which used the correct "
           "name:\n```python\n" +
           preReviewDraft +
           "\n```\nReturn THAT script, corrected only for genuine checklist violations if any — do "
           "not change that name again. Return ONLY executable Python code — no explanation, no "
           "markdown fences.";
}

// The one-shot corrective prompt for AiContentProvider::exploreGroupsIgnoredMistake() (see
// maybeDispatchSelfReview()'s use of it): unlike the backstop prompts above, this checks the
// ORIGINAL candidate itself, not a self-review regression — so, unlike those, it doesn't demand a
// revert; setGroup() with a brand-new name is a legitimate answer, this just asks the model to
// confirm that's actually what it meant rather than an unnoticed leftover guess.
static QString buildExploreGroupsIgnoredRetryPrompt(const QString &mismatchExplanation, const QString &candidate)
{
    return "Before this runs:\n\n" + mismatchExplanation + "\n\nHere is the script:\n```python\n" + candidate +
           "\n```\nIf you meant to reuse an existing group, change setGroup(...) to one of the ones "
           "listed above. If a new group is genuinely intentional, return the script unchanged. "
           "Return ONLY executable Python code — no explanation, no markdown fences.";
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
    // expectsCode=false: self-review reviews an ALREADY-generated candidate and returns it
    // corrected-or-unchanged — it never decides whether to explore first (that decision belongs
    // to the initial generation this candidate already came from), so the EXPLORE-mechanism
    // reminder buildSystemPrompt() would otherwise add (expectsCode defaults true) is pure noise
    // here, sent on every self-review round of every part of every retry.
    ask(prompt + noDocsHintLine(), false, QString(), QStringList(), false, false, false);
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
    // A weak model's "expected output" written after the code (confirmed live: "R1: 2 print(6*7)" as the last line)
    // makes the whole script not compile, and every retry repeated it. Dropped when the rest compiles — and said.
    m_removedTrailingOutput.clear();
    if (!compilable && m_host)
    {
        for (const QString &trimmed : withoutTrailingOutputLines(code))
            if (m_host->canCompile(trimmed))
            {
                m_host->print("▸▸▸ AI: removed text after the script's last line (not Python, e.g. an expected "
                              "output) — running the rest. Removed:\n" +
                              code.mid(trimmed.size()).trimmed());
                m_removedTrailingOutput = code.mid(trimmed.size()).trimmed();
                code = trimmed;
                compilable = true;
                syntaxError.clear();
                break;
            }
    }
    // Confirmed live: "R1: 385" written after the script is valid Python (an annotated name), so the script compiled
    // with it, printed only "385", and nothing above ever looked at it.
    if (compilable && m_host)
        if (const QString trimmed = withoutTrailingAnnotationOutput(code);
            !trimmed.isEmpty() && m_host->canCompile(trimmed))
        {
            m_removedTrailingOutput = code.mid(trimmed.size()).trimmed();
            m_host->print("▸▸▸ AI: removed text after the script's last line (not Python, e.g. an expected "
                          "output) — running the rest. Removed:\n" +
                          m_removedTrailingOutput);
            code = trimmed;
        }
    const bool isSelfReviewResult = awaitingSelfReview;
    awaitingSelfReview = false;
    // Confirmed live: self-review answered "The script is already compliant with all checklist items." — prose, no
    // script — which then counted as a script that does not compile, and the retry lost the reviewed (runnable)
    // draft altogether. A review with nothing runnable in it changes nothing: the draft it reviewed stays. An empty
    // answer compiles but is no review either.
    if (isSelfReviewResult && (!compilable || code.trimmed().isEmpty()) && m_host && !m_selfReviewDraftCode.isEmpty())
    {
        m_host->print("▸▸▸ AI: self-review returned no runnable script — keeping the reviewed one.");
        code = m_selfReviewDraftCode;
        compilable = true;
        syntaxError.clear();
    }
    // In a part before the last, a review that adds the result line has done another part's job. Confirmed live
    // (LIST 5, item 6): asked to review "close Peak5 and Pipeline" (3 lines), the review returned the whole item —
    // deleting the functions with invented calls and printing "R6:" — three times, and the item was given up. Only the
    // line goes: going back to the draft instead kept a bad draft three times (once an unrelated sphere compile) and
    // threw away what the review had fixed.
    if (isSelfReviewResult && compilable && m_content && m_host && !m_selfReviewDraftCode.isEmpty() &&
        !laterSplitPartDescriptions().isEmpty())
        if (const QString stripped = m_content->nonFinalResultLineRepair(code, originalRequestText, true);
            !stripped.isEmpty())
        {
            m_host->print("▸▸▸ AI: self-review printed the result line, which is the last part's job — that line "
                          "removed.");
            code = stripped;
        }
    // A review that brings in a known mistake its draft did not have undid a fix — the review has no docs to check
    // against. Confirmed live (series 5, BlaBlaDor, LIST 3 item 3): the retry draft read the formula's error with
    // str(e).splitlines()[1]; the review rewrote it to type(e).__name__, which was blocked again, and the item was
    // given up. Then the draft runs.
    if (isSelfReviewResult && compilable && m_content && m_host && !m_selfReviewDraftCode.isEmpty() &&
        code.trimmed() != m_selfReviewDraftCode.trimmed())
    {
        auto headersOf = [this](const QString &script) {
            QStringList headers;
            for (const AiKnownMistakeMatch &m : m_content->detectKnownMistakes(
                     script, mistakeCheckRequestText(), splitPartScripts.join("\n\n"), laterSplitPartDescriptions()))
                headers << m.header;
            return headers;
        };
        const QStringList reviewed = headersOf(code);
        const QStringList draft = headersOf(m_selfReviewDraftCode);
        QStringList introduced;
        for (const QString &h : reviewed)
            if (!draft.contains(h))
                introduced << h;
        bool draftOnlyFewer = true;
        for (const QString &h : draft)
            draftOnlyFewer = draftOnlyFewer && reviewed.contains(h);
        if (!introduced.isEmpty() && draftOnlyFewer)
        {
            m_host->print("▸▸▸ AI: self-review brought in a mistake its draft did not have (" + introduced.join("; ") +
                          ") — running the draft instead.");
            code = m_selfReviewDraftCode;
        }
    }
    if (isSelfReviewResult)
        m_selfReviewDraftCode.clear();

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
    if (isSelfReviewResult && !isScopeRetryResult && splitModeActive && m_content && m_userCorrections.isEmpty())
    {
        const QString earlierPartsCode = splitPartScripts.join("\n\n");
        const QString postViolation =
            m_content->splitPartScopeViolation(code, splitPlan[splitPartIndex].description, earlierPartsCode);
        if (!postViolation.isEmpty() &&
            m_content->splitPartScopeViolation(preReviewDraft, splitPlan[splitPartIndex].description, earlierPartsCode)
                .isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: added out-of-scope content to the reviewed draft — asking it "
                              "to revert once.");
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

    // A seventh, distinct one-shot mechanical backstop: self-review has no basis to drop required
    // Compiler setup calls (setFunctionName/setGroup/setParamCount/setParamNamesAll/
    // setParamInitsAll/setParamLimitsAll/setParamVariesAll) from an already-complete compile()
    // sequence — confirmed live: a correct, complete setup was replaced wholesale with old-style
    // raw-array C-function code missing setGroup()/parameter declarations entirely, raising
    // ValueError: "compile: group is empty" on the very next run. Different in kind from backstops
    // 3/5/6 above (a name/lookup/enum swap) — this is a structural regression in a required
    // multi-call sequence, not a single wrong value.
    const bool isCompileSequenceRetryResult = awaitingSelfReviewCompileSequenceRetry;
    awaitingSelfReviewCompileSequenceRetry = false;
    if (isSelfReviewResult && !isCompileSequenceRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewCompileSequenceRegression(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: dropped required Compiler setup calls while "
                              "still compiling — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewCompileSequenceRetry = true;
            askSelfReviewRound(buildCompileSequenceRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // An eighth, distinct one-shot mechanical backstop: self-review has no way to observe which
    // PyQt major version this build actually embeds (no docs, no history), so it has no legitimate
    // basis to switch an already-correct import between PyQt5 and PyQt6 for the same Qt class —
    // confirmed live: a correct `from PyQt6.QtGui import QColor, QFont` draft was silently rewritten
    // to `from PyQt5.QtGui import QColor, QFont` while self-review fixed an UNRELATED issue
    // elsewhere in the same script; PyQt5 doesn't exist in this environment, and the script reached
    // actual execution and crashed with ModuleNotFoundError a full round-trip later. Different in
    // kind from backstop 3 (a name substitution) — this is an unrelated-line regression introduced
    // as a side effect of self-review regenerating the whole script rather than patching only what
    // it was asked to fix.
    const bool isPyQtVersionRetryResult = awaitingSelfReviewPyQtVersionRetry;
    awaitingSelfReviewPyQtVersionRetry = false;
    if (isSelfReviewResult && !isPyQtVersionRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewPyQtVersionSubstitution(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: changed a PyQt import's major version with no "
                              "basis to do so — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewPyQtVersionRetry = true;
            askSelfReviewRound(buildPyQtVersionRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // A ninth, distinct one-shot mechanical backstop: self-review has no docs or history telling
    // it what an EXISTING compiled function's code actually is, so it has no legitimate basis to
    // invent a setCode() call for a function the draft correctly reused via open()+compile() with
    // no setCode() of its own — confirmed live, severe: EXPLORE had already printed the existing,
    // correct code for two functions; the draft correctly did nothing but reopen and recompile
    // them; self-review rewrote this into a version with a brand-new, fabricated setCode() call
    // for each — neither function actually compiled. Different in kind from backstop 7 above
    // (dropped required calls from an already-complete sequence) — this is a call ADDED where the
    // draft correctly had none, the same root cause in the opposite direction.
    const bool isFabricatedSetCodeRetryResult = awaitingSelfReviewFabricatedSetCodeRetry;
    awaitingSelfReviewFabricatedSetCodeRetry = false;
    if (isSelfReviewResult && !isFabricatedSetCodeRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewFabricatedSetCodeMistake(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: fabricated a setCode() call for a function that "
                              "was correctly being reused — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewFabricatedSetCodeRetry = true;
            askSelfReviewRound(buildFabricatedSetCodeRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // A tenth, distinct one-shot mechanical backstop: self-review has no access to the original
    // request text, so it has no legitimate basis to alter a creation call's (newTable/newMatrix/
    // newGraph/newNote) name away from what the pre-review draft already got right — confirmed
    // live: a request naming the table "SAXS_data" survived correctly into the pre-review draft;
    // self-review's rewrite silently dropped the underscore ("SAXSdata") in BOTH the newTable()
    // call and an unrelated scriptPrint() confirmation string in the same script — the second
    // occurrence rules out any C++-side name sanitization (which would never touch a plain print
    // string), confirming this is self-review's own text-level typo, not a runtime effect.
    const bool isCreationNameRetryResult = awaitingSelfReviewCreationNameRetry;
    awaitingSelfReviewCreationNameRetry = false;
    if (isSelfReviewResult && !isCreationNameRetryResult && m_content)
    {
        const QString regression = m_content->selfReviewCreationNameSubstitution(preReviewDraft, code);
        if (!regression.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI self-review: changed a creation call's name with no basis to "
                              "do so — asking it to revert once.");
            awaitingSelfReview = true;
            awaitingSelfReviewCreationNameRetry = true;
            askSelfReviewRound(buildCreationNameRetryPrompt(regression, preReviewDraft));
            return true;
        }
    }

    // DIFFERENT IN KIND from the ten backstops above (all gated on isSelfReviewResult, since they
    // check self-review's OWN edits against preReviewDraft): this checks the ORIGINAL candidate
    // itself against real EXPLORE console output from earlier in this SAME part, so it's gated the
    // opposite way — on the FIRST pass, before self-review has even run once. consecutiveExploreCount
    // > 0 is the signal that this candidate genuinely followed an unbroken EXPLORE streak (it and
    // lastExploreOutput share the exact same reset lifecycle — see lastExploreOutput's doc comment).
    // compilable/m_selfReviewEnabled mirror the mandatory-dispatch gate just below: no point asking
    // a self-review-family round to confirm a setGroup() choice in a script that won't even compile,
    // or when the user has turned self-review off entirely.
    const bool isExploreGroupRetryResult = awaitingSelfReviewExploreGroupRetry;
    awaitingSelfReviewExploreGroupRetry = false;
    if (!isSelfReviewResult && !isExploreGroupRetryResult && compilable && m_selfReviewEnabled &&
        consecutiveExploreCount > 0 && m_content)
    {
        const QString mismatch = m_content->exploreGroupsIgnoredMistake(code, lastExploreOutput);
        if (!mismatch.isEmpty())
        {
            if (m_host)
                m_host->print("▸▸▸ AI: setGroup() ignores the groups EXPLORE just printed — asking "
                              "it to confirm or fix that once.");
            awaitingSelfReview = true;
            awaitingSelfReviewExploreGroupRetry = true;
            askSelfReviewRound(buildExploreGroupsIgnoredRetryPrompt(mismatch, code));
            return true;
        }
    }

    // m_selfReviewEnabled off (a user setting — see AiSettings::selfReviewEnabled's doc comment):
    // skip the mandatory review pass entirely, straight to runActionScript(). Checked here rather
    // than at the top of the function since every backstop block above only ever fires on
    // isSelfReviewResult == true — reachable only once a review round was actually dispatched — so
    // this is the one place a bypass is needed; nothing above becomes reachable-but-wrong when
    // self-review is disabled, it simply never gets a chance to run.
    // A text answer has no code to check, and self-review (no history) can only lose what it says: confirmed live,
    // Codex's right "No. That script creates connected cylindrical/conical surfaces, not three spheres." became
    // "Unable to determine whether spheres exist from the provided script."
    if (!compilable || isSelfReviewResult || !m_selfReviewEnabled || isPureTextAnswer(code))
        return false;

    // Mandatory pre-execution self-review: send the candidate script back to the
    // AI once, against a short checklist (User Rules + curated silent-failure
    // gotchas), before it ever runs. Runs exactly once per candidate script —
    // the review's own response arrives with isSelfReviewResult == true and goes
    // straight to execution below, so this can't loop.
    if (m_host)
        m_host->print("▸▸▸ AI code (self-review pending):\n\n" + code);
    awaitingSelfReview = true;
    m_selfReviewDraftCode = code;
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
void AiAssistant::runActionScript(const QString &rawCode, bool compilable, const QString &syntaxError)
{
    if (!m_host)
        return;

    QString code = rawCode;
    // Repaired, not reported: see AiContentProvider::compilerParamCountRepair()'s doc comment for
    // the confirmed-live retry loop that made asking pointless. Printed so the edit is never silent.
    if (compilable && m_content)
    {
        // first: every setCode() repair and check below reads the body from the call's literal
        if (const QString inlined = m_content->setCodeVariableRepair(code); !inlined.isEmpty())
        {
            m_host->print("▸▸▸ AI: setCode(<variable>) written with the variable's text, so its body is checked "
                          "like any other.");
            code = inlined;
        }
        const QString countRepaired = m_content->compilerParamCountRepair(code);
        if (!countRepaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected setParamCount() to match the parameter lists this "
                          "script actually provides — running the corrected version below.");
            code = countRepaired;
        }
        if (const QString varied = m_content->compilerParamVariesRepair(code, originalRequestText); !varied.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: setParamVariesAll([True, ...]) added before compile() — the "
                          "request fixes no parameter, so every parameter of the new function is adjustable.");
            code = varied;
        }
        // before the self-shadow repair: "double a = param[0];" becomes "double a = a;", which that one removes
        if (const QString indexed = m_content->compilerParamIndexRepair(code); !indexed.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected setCode(): param[k] replaced by the parameter names from "
                          "setParamNamesAll() — inside setCode() a parameter is used by its name.");
            code = indexed;
        }
        const QString shadowRepaired = m_content->compilerSelfShadowRepair(code);
        if (!shadowRepaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected setCode(): removed self-referential declarations such as "
                          "\"double A = A;\" (the parameter is used directly) — running the corrected version below.");
            code = shadowRepaired;
        }
        if (!laterSplitPartDescriptions().isEmpty())
            if (const QString repaired = m_content->nonFinalResultLineRepair(code, originalRequestText, false);
                !repaired.isEmpty())
            {
                m_host->print("▸▸▸ AI: removed a fixed-text result line — the last part of the plan prints it.");
                code = repaired;
            }
        // The last (or only) part: an earlier result line goes, the last one is the answer. Confirmed live (LIST 5
        // NOSPLIT, item 5): blocked, the same script came back unchanged, and the item and the next one were lost.
        if (laterSplitPartDescriptions().isEmpty())
            if (const QString repaired = m_content->repeatedResultLabelRepair(code, originalRequestText);
                !repaired.isEmpty())
            {
                m_host->print("▸▸▸ AI: auto-corrected: only the last result line stays — the request asks for "
                              "exactly ONE.");
                code = repaired;
            }
        if (laterSplitPartDescriptions().isEmpty())
            if (const QString repaired = m_content->labelOnlyResultLineRepair(code, originalRequestText);
                !repaired.isEmpty())
            {
                m_host->print("▸▸▸ AI: auto-corrected: the result label, printed alone, moved onto the line that "
                              "prints the values.");
                code = repaired;
            }
        {
            QStringList notes;
            if (const QString repaired = m_content->apiCallRepairs(code, originalRequestText,
                                                                   laterSplitPartDescriptions().isEmpty(), &notes);
                !repaired.isEmpty())
            {
                for (const QString &n : notes)
                    m_host->print("▸▸▸ AI: auto-corrected: " + n);
                code = repaired;
            }
        }
        if (const QString repaired = m_content->noteAutoexecReceiverRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: setAutoexec() moved from the tab/editor to the note — only a "
                          "note has setAutoexec().");
            code = repaired;
        }
        if (const QString repaired = m_content->bareCompilerDeleteRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: delete(...) became Compiler.delete(...) — there is no global "
                          "delete().");
            code = repaired;
        }
        if (const QString repaired = m_content->tableTransposedCellRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: the table's cell calls wrote rows as columns (out of range as "
                          "written, all in range swapped) — a table cell is (col, row).");
            code = repaired;
        }
        if (const QString repaired = m_content->matrixTransposedCellRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: the matrix's cell(row, col) calls read it transposed (out of range "
                          "as written, all in range swapped) — row and column swapped.");
            code = repaired;
        }
        if (const QString repaired = m_content->bareFolderNameRepair(code, originalRequestText); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: a name the request gives with its folder was written without it — "
                          "it is used whole (\"ai/\" is part of the name).");
            code = repaired;
        }
        if (const QString repaired = m_content->muParserPowerRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected a muParser formula: ** became ^ — muParser has no ** (its power is "
                          "^).");
            code = repaired;
        }
        if (const QString repaired = m_content->tableFormulaCaretRepair(code, m_host->formulaLanguage());
            !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected a column formula: ^ became ** — a column formula is Python, where ^ "
                          "is XOR.");
            code = repaired;
        }
        if (const QString repaired = m_content->compilerMissingGroupRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: setGroup() added after setFunctionName() — new() clears the group, "
                          "and save()/compile() need one.");
            code = repaired;
        }
        if (const QString repaired = m_content->noteTabSelfRunRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: a tab's own script no longer runs the note (removed its "
                          "currentEditor()/tab().execute() line) — the main script runs the tab.");
            code = repaired;
        }
        if (const QString repaired = m_content->noteTabIndexRepair(code); !repaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected tab(0) to tab(1) — tab() is 1-based, tab(0) is always None.");
            code = repaired;
        }
        const QString yNameRepaired = m_content->compilerYNameAssignmentRepair(code);
        if (!yNameRepaired.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected setCode() to assign the dependent variable set by setYName() "
                          "(it declared a local or assigned another name instead) — running the corrected version "
                          "below.");
            code = yNameRepaired;
        }
    }
    // Recognizing "this looks like a plot" / "this looks like a fit" is host-API-specific
    // knowledge, not generic orchestration — lives behind AiContentProvider, not hardcoded here.
    // See AiContentProvider.h's splitPartScopeViolation() doc comment.
    // Also for a NON-split request, whose scope is simply the whole request text. Previously gated
    // on splitModeActive, so anything "running directly without split-mode overhead" got no scope
    // guard at all — confirmed live in one LIST run: "create a table with 50 rows" also plotted it,
    // renamed and maximized the window; two other items added unrequested styling.
    // A one-part plan's description joins the request: confirmed live, the plan named the unnamed 3D plot "Surface",
    // the script used that name, and was blocked because the bare request never mentions it.
    const QString scopeDescription = currentScopeDescription();
    // A correction typed during the run may contradict the request these two read: then they are skipped.
    const bool checkScope = compilable && m_content && !scopeDescription.isEmpty() && m_userCorrections.isEmpty();
    const QString scopeViolation =
        checkScope ? m_content->splitPartScopeViolation(code, scopeDescription, splitPartScripts.join("\n\n"),
                                                        !splitModeActive)
                   : QString();
    // The under-delivery counterpart, checked against the same scopeDescription and reported in the
    // same round as the overreach one — see missingRequestedDeliverables()'s doc comment.
    const QString missingDeliverables =
        checkScope ? m_content->missingRequestedDeliverables(code, scopeDescription) : QString();
    // Unconditional (not gated on splitModeActive) — every pattern detectKnownMistakes() checks is
    // a genuine defect regardless of split mode (e.g. a Compiler setCode() self-shadow bug compiles
    // with no error, so nothing else in the pipeline would ever catch it before it silently
    // corrupts a fit) — see AiContentProvider::detectKnownMistakes()'s doc comment for the full
    // family and why this is one call instead of 16. splitPartScripts is empty outside split mode,
    // so this naturally passes an empty earlierPartsCode there — no separate gating needed, same
    // reasoning as splitPartScopeViolation's own earlierPartsCode argument above. Collects ALL
    // matching patterns, not just the first — see that doc comment's "collect all, don't stop at
    // first" rationale.
    QList<AiKnownMistakeMatch> knownMistakes =
        (compilable && m_content)
            ? m_content->detectKnownMistakes(code, mistakeCheckRequestText(), splitPartScripts.join("\n\n"),
                                             laterSplitPartDescriptions())
            : QList<AiKnownMistakeMatch>();
    // What this request's own failed attempts already did counts as its own work: a check that no longer fires once
    // their scripts count as earlier code (a retry reusing the function its failed attempt compiled) is dropped. Only
    // dropped, never added: a check that fires only WITH them (e.g. "configure() again discards the earlier setup")
    // would wrongly treat the failed attempt as a finished earlier part. See m_failedRunScripts.
    if (!knownMistakes.isEmpty() && !m_failedRunScripts.isEmpty())
    {
        QStringList stillFiring;
        for (const AiKnownMistakeMatch &m : m_content->detectKnownMistakes(
                 code, mistakeCheckRequestText(), (splitPartScripts + m_failedRunScripts).join("\n\n"),
                 laterSplitPartDescriptions()))
            stillFiring << m.header;
        knownMistakes.erase(
            std::remove_if(knownMistakes.begin(), knownMistakes.end(),
                           [&](const AiKnownMistakeMatch &m) { return !stillFiring.contains(m.header); }),
            knownMistakes.end());
    }
    // A setCode() body this request already had rejected is not tried again. Confirmed live (LIST 5, item 1, local
    // model): the retries switched between a body blocked for redeclared parameters and one that failed to compile.
    if (compilable && m_content)
        for (const QString &body : m_content->compiledCodeBodies(code))
            for (const QPair<QString, QString> &rejected : m_rejectedSetCodes)
                if (body.simplified() == rejected.first)
                {
                    knownMistakes << AiKnownMistakeMatch{
                        "a setCode() this request already rejected",
                        "This setCode() body was already tried in this request and rejected: " + rejected.second +
                            ". Do not send it again — write a different body that fixes exactly that."};
                    break;
                }
    if (compilable)
        if (const QString moved = guardedCreationRepair(code, m_objectsAtRequestStart); !moved.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: only newTable()/newMatrix() stays inside \"if ... is None:\" — "
                          "the set-up after it now runs on the table whether it was just created or existed.");
            code = moved;
        }
    if (compilable && !m_lastFailedStatement.isEmpty())
        if (const QString unguarded = skippedFailedStepRepair(code, m_lastFailedStatement, m_failedRunObjects);
            !unguarded.isEmpty())
        {
            m_host->print("▸▸▸ AI: auto-corrected: the step that failed (" + m_lastFailedStatement.trimmed() +
                          ") was inside an existence check that the failed attempt's own object makes skip — the "
                          "check is removed, so the corrected step runs.");
            code = unguarded;
        }
    if (compilable && !m_lastFailedStatement.isEmpty())
        if (const QString skipped = skippedFailedStepMistake(code, m_lastFailedStatement, m_failedRunObjects);
            !skipped.isEmpty())
            knownMistakes << AiKnownMistakeMatch{"the failed step is skipped behind an existence check", skipped};
    if (compilable)
        if (const QString unknownColumn = formulaColumnNameMistake(code); !unknownColumn.isEmpty())
            knownMistakes << AiKnownMistakeMatch{"a formula column name the table does not have", unknownColumn};
    if (compilable)
        if (const QString recreated = existingTableRecreatedMistake(code); !recreated.isEmpty())
            knownMistakes << AiKnownMistakeMatch{"newTable() would wipe an existing table", recreated};
    if (compilable && !m_removedTrailingOutput.isEmpty())
        if (const QString labeled = labeledLastPrintRepair(code, m_removedTrailingOutput); !labeled.isEmpty())
        {
            m_host->print("▸▸▸ AI: the script's result line was written as text after it — its last print now "
                          "carries that label.");
            code = labeled;
        }
    // Once per request: confirmed live (LIST 3 item 12, LIST 4 item 5) the right values printed without the requested
    // "R5:" line, which the model had written as text after the code. A second time the script just runs.
    if (compilable && !m_unprintedOutputRetried && !m_removedTrailingOutput.isEmpty())
        if (const QString unprinted = unprintedOutputLineMistake(code, m_removedTrailingOutput); !unprinted.isEmpty())
        {
            knownMistakes << AiKnownMistakeMatch{"output line written as text, not printed", unprinted};
            m_unprintedOutputRetried = true;
        }
    // Unconditional and host-independent — see isEmptyActionScript()'s doc comment. Confirmed
    // live: self-review's "ONLY executable code" instruction broke down entirely, returning a
    // checklist-reasoning dump formatted as comments instead of any actual script.
    const bool emptyScript = compilable && isEmptyActionScript(code);

    // Checked before the syntax branch: a fabricated conversation almost always fails to compile,
    // so "SyntaxError" would be all the model ever heard about it — naming the wrong problem
    // entirely. Confirmed live three times.
    const QString fabricated = m_content ? m_content->fabricatedConversationCheck(code) : QString();

    QString errContext;
    // The script a block stopped, for the retry prompt: its history entry is replaced by a placeholder (so nothing
    // treats it as having run), and confirmed live (LIST 5, item 4) the retry, told "keep every other line", could not
    // see them and wrote a different script from scratch, dropping the result line.
    QString blockedScript;
    bool blockedUnchanged = false;   // the script blocked just before, sent back as it was
    bool failedRunUnchanged = false; // the script that failed just before, sent back as it was
    if (!fabricated.isEmpty())
    {
        m_host->print("▸▸▸ AI code (blocked — script invents further conversation turns):\n\n" + code);
        if (!m_history.isEmpty())
            m_history.last().second = "(blocked before running — fabricated conversation turns; corrected below)";
        errContext = fabricated;
    }
    else if (!compilable)
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
    else if (!scopeViolation.isEmpty() || !missingDeliverables.isEmpty() || !knownMistakes.isEmpty())
    {
        // Blocked BEFORE running — unlike a runtime error, these are knowable up front from the
        // code (and this part's own description) alone, and running it first would mean e.g. an
        // out-of-scope newGraph() call already creating a real, duplicate window before the retry
        // below even fires. See AiContentProvider::detectKnownMistakes()'s doc comment for the
        // pattern family, each with its own "confirmed live" writeup in QtiSasAiContent.cpp.
        //
        // Scope violation and known mistakes report TOGETHER, not as exclusive branches: they are
        // independent analyses of the same script, and keeping them separate meant only one defect
        // per round was ever named. Confirmed live: a simulate part's script both plotted (out of
        // scope) AND called selectStage() before setFunction() — two rounds at ~90s each to hear
        // about them one at a time. This is the same "collect all, don't stop at the first"
        // reasoning detectKnownMistakes() already applies internally, extended one level up.
        QStringList headers;
        QStringList explanations;
        // A script that only prints — every reason below is then about work it never attempts, and the long list of
        // them did not get through: confirmed live (benchmark LIST 1 item 10, local all-pyi BIGMODEL), "plot into
        // FitResults … print the number of layers and curves" answered with scriptPrint("R10:", 2, 2, 1), values read
        // off the request, blocked with two reasons, the same line back five times — through a fresh retry as well.
        // One plain reason instead.
        if (onlyPrintsResult(code) && (!scopeViolation.isEmpty() || !missingDeliverables.isEmpty()))
        {
            headers << "only the result line, none of the work";
            explanations << "This script does none of the work the request asks for: it only prints the result line, "
                            "with values taken from the request text — nothing the request creates or changes exists "
                            "yet. Write the script that does the task first (create, compute, fit, plot — whatever the "
                            "request says), then print the result line at its end, with the values read back from "
                            "what the script made.";
        }
        else if (!scopeViolation.isEmpty())
        {
            headers << "out of scope for this part";
            explanations << scopeViolation;
        }
        if (!missingDeliverables.isEmpty() && !headers.contains("only the result line, none of the work"))
        {
            headers << "requested deliverable missing";
            explanations << missingDeliverables;
        }
        for (const AiKnownMistakeMatch &m : knownMistakes)
        {
            headers << m.header;
            explanations << m.explanation;
        }
        m_host->print("▸▸▸ AI code (blocked — " + headers.join("; ") + "):\n\n" + code);
        logCorrectionOutcome("blocked", headers.join("; "));
        blockedUnchanged = code.trimmed() == m_lastBlockedScript.trimmed();
        m_lastBlockedScript = code;
        // The reason, not only its header: confirmed live, a user saw "out of scope for this part" with no way to
        // tell which rule fired or why. See blockReasonSummary().
        QStringList reasons;
        for (const QString &explanation : explanations)
            reasons << "  • " + blockReasonSummary(explanation);
        m_host->print("▸▸▸ AI: why it was blocked:\n" + reasons.join("\n"));
        // Blocked never ran, so it must not stay in history as this part's answer — same patch
        // (and same guaranteed-last-entry invariant) as the self-review/invalid-response ones above.
        if (!m_history.isEmpty())
            m_history.last().second = "(blocked before running — " + headers.join("; ") + "; corrected below)";
        blockedScript = code;
        // only a block about the setCode() body itself: one for an unrelated line says nothing about the body
        QStringList bodyReasons;
        for (const QString &header : headers)
            if (header.contains("setCode", Qt::CaseInsensitive))
                bodyReasons << header;
        if (m_content && !bodyReasons.isEmpty())
            for (const QString &body : m_content->compiledCodeBodies(code))
                m_rejectedSetCodes << qMakePair(body.simplified(), "blocked (" + bodyReasons.join("; ") + ")");
        if (explanations.size() == 1)
        {
            errContext = explanations.first();
        }
        else
        {
            QStringList numbered;
            for (int i = 0; i < explanations.size(); ++i)
                numbered << QString("%1. %2").arg(i + 1).arg(explanations.at(i));
            errContext = "This script has " + QString::number(explanations.size()) +
                         " separate known issues — fix ALL of them together in one corrected "
                         "script:\n\n" +
                         numbered.join("\n\n");
        }
        // Confirmed live: after a run that did everything but failed in its last line, the retry sent only the
        // fixed last lines, was blocked here for the missing compile()/newTable(), and the item was given up.
        if (!missingDeliverables.isEmpty() && !m_failedRunObjects.isEmpty())
            errContext += "\n\nSend the WHOLE script again, not only the part after the earlier error: every script "
                          "is checked and run as a whole. " +
                          failedRunObjectsNote();
    }
    else if (emptyScript)
    {
        // Blocked BEFORE running — an all-comments script is a silent no-op, not a completed
        // action; running it would report "success" (no exception) for a part that did nothing.
        m_host->print("▸▸▸ AI code (blocked — no executable code, comments only):\n\n" + code);
        // Never ran, and comment-only prose in history is exactly what the invalid-response
        // placeholder above exists to keep the model from building on.
        if (!m_history.isEmpty())
            m_history.last().second = "(blocked before running — no executable code; corrected below)";
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
        // Trailing blank line (regardless of whether `code` itself already ends in one) so the
        // script's own live scriptPrint() output — appended immediately after by runScript()
        // below — is visually separated from the echoed code instead of running straight into
        // its last line.
        // A real attempt at the actual fix is being made — breaks any streak of pure-diagnostic
        // EXPLORE rounds tracked by consecutiveExploreCount (see its doc comment in AiAssistant.h).
        // lastExploreOutput shares that exact lifecycle — cleared here too.
        consecutiveExploreCount = 0;
        lastExploreOutput.clear();
        lastExploreCode.clear();
        m_lastBlockedScript.clear();
        // What runs is what the conversation keeps. Every auto-repair above changes only `code`; the reply in m_history
        // still held the draft before them, and a retry rebuilt from it brought the repaired lines back. Confirmed live
        // (LIST 5 NOSPLIT local pyi, item 2): param[k] in setCode() was repaired, the script failed later for another
        // reason, and every retry wrote param[k] again. The draft is replaced where the reply holds it — the whole
        // reply, or the action half of an EXPLORE reply. Here, at the one place a script runs, so a repair added later
        // is covered too.
        if (code != rawCode && !m_history.isEmpty() && !rawCode.trimmed().isEmpty() &&
            m_history.last().second.contains(rawCode.trimmed()))
            m_history.last().second.replace(rawCode.trimmed(), code.trimmed());
        m_host->print("▸▸▸ AI code (running):\n\n" + code.trimmed() + "\n\n");
        // Runs via runScript() (AiScriptHost) — see AiScriptResult's doc comment. Display already
        // happened live during runScript() itself — do NOT print result.output here too, see
        // that method's doc comment on why that would double every scriptPrint() line (confirmed
        // live: exactly this happened).
        const QStringList objectsBefore = m_host->liveObjects();
        const AiScriptResult result = m_host->runScript(code, "<ai>");
        m_itemOutput += result.output + "\n";
        // result.error alone is still the GATE for "did this run actually fail" — deliberately
        // not result.output, since normal fit/print output (e.g. fit progress, Chi2/DoF) also
        // lands there via ApplicationWindow::scriptPrint(), which would make every successful
        // run look like a failure.
        logCorrectionOutcome(result.error.isEmpty() ? "ran" : "failed", result.error);
        if (result.error.isEmpty())
        {
            // A genuine success breaks any chain of doc-retries that led here — see
            // consecutiveDocRetryCount's doc comment in AiAssistant.h for why this resets ONLY on
            // success, not on every attempt like consecutiveExploreCount does.
            consecutiveDocRetryCount = 0;
            m_failedRunObjects.clear();
            m_closedFailedObjects.clear();
            m_failedRunScripts.clear();
            m_itemRanScripts << code;
            if (splitModeActive)
                advanceSplitPart(code);
            else if (listModeActive)
                advanceListItem();
            return;
        }
        // result.output holds everything printed during the run, INCLUDING the exception text —
        // used below instead of the bare exception for retry context and repeat-detection, since
        // the useful diagnostic detail is often in a PRECEDING print, not the exception text itself.
        errContext = result.output.isEmpty() ? result.error : result.output;
        for (const QString &object : m_host->liveObjects())
            if (!objectsBefore.contains(object) && !m_failedRunObjects.contains(object))
                m_failedRunObjects << object;
        // A clean slate for the retry: confirmed live three times (series 5–6, local and BlaBlaDor) — a retry found the
        // failed attempt's note or graph by name (newNote()/graphWindow() reuse it) and added its tabs or curves a
        // second time: 3 tabs, 5 tabs, 4 curves, each printed as a result. The windows that attempt created are closed;
        // functions stay (a retry compiles them again).
        if (const QStringList closed = m_host->closeObjects(m_failedRunObjects); !closed.isEmpty())
        {
            for (const QString &object : closed)
            {
                m_failedRunObjects.removeAll(object);
                if (!m_closedFailedObjects.contains(object))
                    m_closedFailedObjects << object;
            }
            m_host->print("▸▸▸ AI: closed what the failed attempt created (" + closed.join(", ") +
                          ") — the retry starts from the state the first attempt saw.");
        }
        failedRunUnchanged = !m_failedRunScripts.isEmpty() && m_failedRunScripts.last().trimmed() == code.trimmed();
        m_failedRunScripts << code;
        static const QRegularExpression failedLineRe(R"(at <ai>:\d+ — ([^\n]+))");
        QString failedStatement;
        for (auto it = failedLineRe.globalMatch(result.error + "\n" + result.output); it.hasNext();)
            failedStatement = it.next().captured(1);
        m_lastFailedStatement = failedStatement;
        // clang rejected the body — unless it stopped at an #include that names no file: then the header line is the
        // fault, not the body. Confirmed live (series 4, LIST 5 item 2): setHHeaders('#include "sasviewmodels/
        // gaussian.h"') on a plain formula; every retry's body was blocked as "already rejected", the model rewrote
        // the correct body five times, kept the include and the item was lost.
        static const QRegularExpression missingHeaderRe(R"(fatal error: '[^']+' file not found)");
        if (m_content && result.error.contains("compilation of", Qt::CaseInsensitive) &&
            !missingHeaderRe.match(result.error + "\n" + result.output).hasMatch())
            for (const QString &body : m_content->compiledCodeBodies(code))
                m_rejectedSetCodes << qMakePair(body.simplified(),
                                                "it failed to compile (" + result.error.section('\n', 0, 1) + ")");
    }
    // Broader, complementary check to the one below: if errContext is byte-for-byte identical to
    // the previous retry's, the SAME diagnosis was handed back twice and wasn't acted on either
    // time — confirmed live two different ways: (1) an empty-table fit raised the exact same
    // RuntimeError for all 10 retries with the exact same code resubmitted unchanged each time;
    // (2) a mechanical detectKnownMistakes() block (qti3DImportMistake) fired on TWO VISIBLY
    // DIFFERENT wrong import lines in a row (`from qti.api import Qti3D`, then `import Qti3D`) —
    // the CODE changed between attempts, but the explanation text is deliberately identical
    // regardless of which specific wrong form was guessed (see that check's own doc comment), so
    // errContext still matched and this correctly stopped rather than waiting for a third,
    // equally-wrong guess. errContext (not the code) is what's compared — that error names no
    // specific alternative (unlike the dataset-not-found case below), so the "Available X:" check
    // can't catch it — an exact repeat of the DIAGNOSIS is its own signal regardless of whether the
    // code attempts themselves differed.
    // At most 2 x maxAutoContinue() blocked attempts — the same budget runtime errors have, doubled because a block
    // costs no run.
    if (!blockedScript.isEmpty() && ++m_blockedRounds > 2 * maxAutoContinue())
    {
        recordGiveUpInHistory(QString("stopping — %1 attempts blocked").arg(m_blockedRounds - 1), errContext);
        QString msg = QString("▸▸▸ AI: stopping — %1 attempts in a row were blocked before running; the last reason:\n")
                          .arg(m_blockedRounds - 1) +
                      errContext;
        msg += splitModeGiveUpNote();
        m_host->print(msg);
        lastRetryError.clear();
        return;
    }
    // The same block for the same script: the retry prompt quotes the blocked script and the conversation holds it, and
    // the model copied it. Once, it is asked from the request itself, without either; a second repeat stops below.
    // The same for a script that failed at run time and came back unchanged with the same error: confirmed live (LIST
    // 4 item 3, iteration 7), m.cell(4, 3) on a 3x4 matrix, "no row 4 ... takes the row first", the identical script.
    if ((blockedUnchanged || failedRunUnchanged) && !m_freshRetryUsed &&
        normalizeErrorForDedup(errContext) == normalizeErrorForDedup(lastRetryError))
    {
        m_freshRetryUsed = true;
        m_host->print(QString("▸▸▸ AI: the %1 script came back unchanged — asking once more from the request "
                              "itself, without the conversation.")
                          .arg(failedRunUnchanged ? "failed" : "blocked"));
        const QString freshPrompt =
            (splitModeActive ? "Generate a script for ONLY this part: " + splitPlan[splitPartIndex].description + "\n\n"
                             : QString()) +
            "Request:\n" + mistakeCheckRequestText() +
            (failedRunUnchanged
                 ? "\n\nA script written for it failed twice, unchanged, with this error:\n```\n"
                 : "\n\nA script written for it was rejected before running, twice, for this reason:\n```\n") +
            errContext +
            "\n```\nWrite the script anew from the request itself, so that it does everything the request asks for "
            "and avoids that reason. " +
            failedRunObjectsNote() +
            // The request's own facts go with it: without the conversation, a LIST item lost the note on what the
            // earlier items made. Confirmed live (local LIST 1, item 10): the fresh retry wrote "not guaranteed
            // without knowing what was actually fit" and looked up the docs' example table instead of item 9's fit.
            m_earlierItemsNote;
        ask(freshPrompt + topicsHintLine(selectedDocTopics), true, QString(), selectedDocTopics, true, false, true,
            freshPrompt);
        return;
    }
    if (normalizeErrorForDedup(errContext) == normalizeErrorForDedup(lastRetryError))
    {
        recordGiveUpInHistory("stopping — same diagnosis repeated, not corrected", errContext);
        QString msg = "▸▸▸ AI: stopping — the same corrective explanation was given twice in a row "
                      "and wasn't applied either time:\n" +
                      errContext +
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
        recordGiveUpInHistory("stopping — same list of alternatives offered twice, not used", errContext);
        QString msg = "▸▸▸ AI: stopping — the same list of available alternatives was offered "
                      "twice in a row, and the requested name still wasn't corrected to one of "
                      "them:\n" +
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
        recordGiveUpInHistory(
            QString("script failed again after %1 automatic retries — giving up").arg(maxAutoContinue()), errContext);
        QString msg = "▸▸▸ AI: script failed again after " + QString::number(maxAutoContinue()) +
                      " automatic retries — giving up.\nLast error:\n" + errContext;
        msg += splitModeGiveUpNote();
        m_host->print(msg);
        return;
    }

    // Marks the candidate as an error-retry correction so the self-review scope-reduction check
    // fires (see self-review's IMPORTANT block) — gated on the error actually naming a concrete
    // alternative ("Available: X"), never on a plain coding mistake unrelated to name substitution.
    isErrorRetryCorrection = errContext.contains("available", Qt::CaseInsensitive);
    // Re-grounds a retry against a KNOWN, recurring failure signature — see
    // AiContentProvider::retryHintForError()'s doc comment. Empty (the common case) when
    // errContext matches nothing known.
    QString retryHint = m_content ? m_content->retryHintForError(errContext) : QString();
    // A name the request quotes in full, used without its folder: confirmed live (LIST 5 local pyi, item 2), "ai/Lor5"
    // configured as "Lor5" — not found, the same error twice, the item given up. Any folder, any kind of name.
    if (const QString quoted = quotedNameWithFolder(originalRequestText, functionNotFoundNameFromError(errContext));
        !quoted.isEmpty())
        retryHint += "REMINDER: \"" + functionNotFoundNameFromError(errContext) +
                     "\" was not found, and the request names \"" + quoted +
                     "\" — use a name exactly as the request quotes it, its folder included.\n\n";
    // Re-grounds against THIS conversation's own prior compiles, not just static docs — see
    // functionNotFoundNameFromError()/compiledFunctionNamesFromHistory()'s doc comments.
    if (!functionNotFoundNameFromError(errContext).isEmpty())
    {
        const QStringList compiledNames = compiledFunctionNamesFromHistory(m_history);
        if (!compiledNames.isEmpty())
            retryHint += "REMINDER: earlier in this exact conversation you already attempted to "
                         "compile: \"" +
                         compiledNames.join("\", \"") +
                         "\". Verify with f.fitFunctions() (a METHOD call on the Fittable "
                         "instance, never a bare global) which of these actually succeeded "
                         "before inventing yet another new function name.\n\n";
    }
    const QString creationCallExamples = m_content ? m_content->creationCallExamples() : QString();
    // Re-attaches the SAME doc topics already selected for this request (selectedDocTopics is set
    // once during doc selection and persists for the request's lifetime — see its declaration in
    // AiAssistant.h) rather than leaving this round doc-free. Confirmed live, repeatedly, in one
    // long correction chain: with no docs, successive corrective rounds drifted further from the
    // documented API with each attempt instead of converging — a hallucinated Compiler.
    // addParameter() (no such method; the real API is 5 separate setParamXAll() bulk setters), a
    // 2-item Fittable dataset list built from one table's own X/Y columns (fittable-python-api.md's
    // own documented "WRONG" example, reproduced fresh), and Compiler()/Fittable() called as
    // constructors (the exact incident retryHintForError() already exists to re-ground, but only
    // reactively, after a matching error appears) — none of these were the specific defect the
    // PRECEDING round was blocked for; they were introduced fresh while regenerating unrelated
    // code with nothing but memory to fall back on. A correction that rewrites substantial code
    // needs the same grounding real generation does; self-review's own checklist-only pass
    // (askSelfReviewRound()) is deliberately left doc-free, since that pass isn't generating code
    // from API knowledge in the first place — this path is different: it's regeneration, not review.
    //
    // Capped at kMaxConsecutiveDocRetries: past that many consecutive doc-attached corrections with
    // no success in between, fall back to no docs for this round — see consecutiveDocRetryCount's
    // doc comment in AiAssistant.h for the context-window-overflow risk this bounds.
    ++consecutiveDocRetryCount;
    static const int kMaxConsecutiveDocRetries = 2;
    const bool retryIncludeDocs = consecutiveDocRetryCount <= kMaxConsecutiveDocRetries;
    // Split-mode part anchor, same "Generate a script for ONLY this part: ..." phrasing
    // startSplitPart() itself uses (see its prompt at this file's other reference) — qtisas-rag's
    // _PART_DESCRIPTION_RE recognizes this literal phrase and narrows its retrieval query to just
    // the captured description. Without it, this retry's own text carries no task-identifying
    // content whenever originalRequestReminder() is empty (the common case: pendingRequestText
    // already equals originalRequestText), and qtisas-rag's history-based fallback anchors on
    // messages[0] instead — confirmed live, that's whatever the FIRST turn of the whole
    // conversation was, which is only "this part's task" when nothing preceded it. A prior,
    // unrelated task left in history (continuation-check judged CONTINUE, so it was never
    // cleared) poisoned the retry's retrieval query with that stale first turn instead.
    const QString splitPartAnchor =
        splitModeActive ? ("Generate a script for ONLY this part: " + splitPlan[splitPartIndex].description + "\n\n")
                        : QString();
    const QString blockedScriptBlock =
        blockedScript.isEmpty()
            ? QString()
            : "Your script, blocked before running (it did NOT run):\n```\n" + blockedScript.trimmed() +
                  "\n```\nCorrect THIS script: change only what the reason below names.\n\n";
    const QString retryPromptBase =
        splitPartAnchor + retryHint + originalRequestReminder() + blockedScriptBlock +
        (blockedScript.isEmpty() ? "The action script raised an error:\n```\n"
                                 : "It was blocked for this reason:\n```\n") +
        errContext +
        "\n```\nProvide a corrected script. IMPORTANT: if this "
        "error names a specific available option — an exact "
        "dataset/function/column name, a valid range, or "
        "similar (e.g. \"Available datasets: X\") — the fix is "
        "almost always to use that exact value. Do not guess a "
        "different name, vary unrelated call syntax instead, or "
        "repeat the same failing line unchanged. IMPORTANT: if the output above shows this "
        "script already executed some lines successfully before the error, send the WHOLE "
        "script again (every script is checked and run as a whole), but make it safe to run "
        "a second time — an earlier successful side-effecting call (" +
        creationCallExamples +
        ", ...) already took effect live. Never add to an object a second time: a note's tabs, a graph's layers or "
        "curves — reuse such a note or graph by name. A table or matrix needs no existence check at all: newTable() / "
        "newMatrix() on its name clears it, so create and fill it again exactly as before; if you keep such a "
        "check, only the newTable()/newMatrix() call goes inside it, every other step on the table after it, "
        "outside. But the step that FAILED "
        "must run again: never put it, or its correction, into "
        "a branch that is skipped because the object now exists — fix it on the existing object, outside such a "
        "check, or create the object again. Not so for a function the request asks to COMPILE: define and compile it "
        "again, "
        "every time, never behind a check whether it already exists — a failed compile leaves its definition behind, "
        "so such a check skips the compile and the next call fails with \"not found\". " +
        failedRunObjectsNote() +
        "IMPORTANT: change only what "
        "this error actually implicates — confirmed live, a retry that correctly fixed one line "
        "also silently dropped an unrelated, still-needed import several lines earlier that had "
        "nothing to do with the error, breaking a part of the script that was already correct. "
        "Keep every other line exactly as it was, even when rewriting the whole script.";
    // TOPICS: hint for an optional server-side RAG proxy — confirmed live missing here too (see
    // startRequest()'s doc comment on the same gap and topicsHintLine()'s doc comment). This is
    // the main error-retry call, fired after EVERY runtime error — the single most common
    // code-regeneration path in the whole system — and had no hint line at all until now.
    // Deliberately NOT gated by retryIncludeDocs: that cap exists to bound LOCAL doc content
    // (real token weight) after repeated consecutive retries, but this hint is a few words with
    // no such cost — no reason to drop it just because local re-attachment paused. historyText=
    // retryPromptBase (pre-hint): see startRequest()'s doc comment on why the hinted text must
    // never be what actually gets stored in m_history.
    ask(retryPromptBase + topicsHintLine(selectedDocTopics), retryIncludeDocs, QString(),
        retryIncludeDocs ? selectedDocTopics : QStringList(), true, true, true, retryPromptBase);
}

// "%1" is filled in by AiContentProvider::apiCallingConventionHint() at request time (see ask())
// — the one piece of concrete API knowledge this otherwise fully generic prompt needs, kept out of
// this literal so AiAssistant.cpp never has to name a single QtiSAS function itself. "%2"/"%3"/"%4"
// are filled the same way, by assistantIdentityIntro()/existenceVerificationFunctionExamples()/
// noOpPrintFunctionName() respectively — see ask()'s construction of sysPrompt below.
// Host-independent (PyQt6/QColor etc. are the same fact for any host, not a QtiSAS-specific API
// name) — unlike apiCallingConventionHint() and friends below, so this lives here as a plain
// constant rather than a new AiContentProvider virtual. Added to the ALWAYS-SENT base prompt,
// closest to generation now that docs sit before the system prompt (see build_augmented_prompt()'s
// doc comment on the RAG server side) — this exact rule already existed four other places (a doc
// RULE #1, a PRE-FLIGHT checklist item, the self-review checklist, a worked example) and still
// missed the first draft repeatedly in every one of them; this is the one place that's always
// present and never depends on self-review or a topic doc happening to be attached.
static const QString kPyQtImportBaseRule =
    "If a script uses QColor, QFont, QPen, Qt, QFrame, or QBrush, it MUST start with the matching "
    "import (from PyQt6.QtGui/.QtCore/.QtWidgets) — none of these are in the default namespace.";

// Extracted out of kBasePromptTemplate below — four narrow, single-incident rules (all about
// existence-checking/stale-conclusion handling) that were being sent unconditionally on every
// call, even for parts with nothing to do with any of that (e.g. "compile a function"). Only
// spliced in when buildSystemPrompt()'s own relevance check finds existence/missing/not-found
// language in what's actually being asked — see that function's own doc comment.
static const QString kExistenceHandlingRules =
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
    "something doesn't exist / wasn't found by returning a bare %2 with no actual "
    "verification call in that same script — a \"doesn't exist\" claim is only trustworthy if "
    "THIS response's own code just checked it (%1, etc.), not "
    "because an earlier turn concluded it. If you're about to write a negative-conclusion "
    "%2 and the script has no real check backing it, add the check instead of trusting "
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

static const QString kBasePromptTemplate =
    "%2"
    "When asked for code, return ONLY the raw executable Python code — no markdown fences, no explanation. "
    "%1"
    "Format code with one statement per line and standard 4-space indentation for blocks. "
    "When asked a question, answer concisely in plain text. "
    "You have NO filesystem access — do not pretend to read, write, or list files. "
    "NEVER import QApplication from PyQt6 — it will crash the application. "
    "%3\n\n"
    "%4"
    "%5";

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
    // selected, task-specific topics last (see startRequestWithSelectedTopics()), so the doc
    // content most relevant to THIS request ends up closest to the
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

// Detects and truncates decoding-level repetition-loop garbage. Confirmed live: an EXPLORE
// response hit max_tokens cycling near-identical "check if we need to set X" blocks, and the whole
// multi-KB reply landed in m_history, inflating the NEXT call's prefill by 90+ seconds. Only lines
// at least kMinLineLen long count — "else:"/"return" recur in legitimately varied code.
//
// Repeat counts reset at each stage-boundary marker. Counting across the WHOLE response broke the
// DRAFT/---CHECK---/---FINAL--- fixup protocol, which legitimately restates an unchanged PART line
// once per stage: the 4th, wanted occurrence inside ---FINAL--- was truncated away as garbage,
// deleting the real answer and surfacing as "unparseable — abandoning split mode".
//
// Two tiers, because cutting AT the threshold occurrence left kRepeatThreshold-1 = three copies —
// and all three then executed. cleanResponse() is the single network-response chokepoint and hands
// its result on as an ordinary valid response, so nothing downstream can tell truncation happened
// (the marker appended below is a bare `#` comment no caller reads). Harmless for a read-only
// EXPLORE; the same path carries ACTION scripts, where three passes of newNote()/addTab()/setCell()
// would run for real. So tier 1 (kRepeatThreshold, the field-proven gate) only decides THAT a
// response is degenerate, and tier 2 cuts back to the pattern's second occurrence so exactly one
// pass survives. Tier 2 cannot misfire on healthy output: it is only reached for a response tier 1
// already condemned, and it stays OFF for the staged protocol above.
static const QRegularExpression kRepetitionStageBoundaryRe(R"(-{2,}\s*(?:CHECK|FINAL)\s*-{2,})",
                                                           QRegularExpression::CaseInsensitiveOption);

// Walks a truncation point back to the start of the top-level statement enclosing it. The kept
// prefix is EXECUTED, and an EXPLORE script is routed on its first-line marker alone without ever
// passing through canCompile() (see the exploreMarkerRe branch in ApplicationWindow's responseReady
// handler), so a cut landing inside an indented block can leave "if x:"/"for ...:" with an empty
// body and turn a recoverable repetition loop into a SyntaxError. Same hazard and same remedy as
// QtiSasAiContent::explorePrefixBeforeFirstWriteCall(), which snaps its own cut for this reason.
// Returns -1 when no safe boundary exists above the cut (everything sits inside one top-level
// statement); the caller then leaves the response untouched rather than emit an empty script.
static int snapRepetitionCutToTopLevel(const QStringList &lines, int cutBefore)
{
    if (cutBefore <= 0 || cutBefore >= lines.size())
        return cutBefore;
    // A cut before a blank or an unindented line already ends on a complete statement.
    const QString &cutLine = lines.at(cutBefore);
    if (cutLine.trimmed().isEmpty() || !cutLine.at(0).isSpace())
        return cutBefore;
    for (int i = cutBefore - 1; i >= 0; --i)
    {
        const QString &line = lines.at(i);
        if (line.trimmed().isEmpty() || line.at(0).isSpace())
            continue;
        return i > 0 ? i : -1;
    }
    return -1;
}

QString truncateRepetitiveGarbage(const QString &s)
{
    constexpr int kMinLineLen = 20;
    constexpr int kRepeatThreshold = 4; // same line this many times in one response = degenerate
    const QStringList lines = s.split('\n');
    // Tier 2 off for the staged protocol — see this function's doc comment.
    const bool stagedProtocol = kRepetitionStageBoundaryRe.match(s).hasMatch();
    QHash<QString, int> seenCount;
    for (int i = 0; i < lines.size(); ++i)
    {
        const QString trimmed = lines[i].trimmed();
        if (kRepetitionStageBoundaryRe.match(trimmed).hasMatch())
        {
            seenCount.clear();
            continue;
        }
        if (trimmed.length() < kMinLineLen)
            continue;
        if (++seenCount[trimmed] < kRepeatThreshold)
            continue;
        int cutBefore = i;
        if (!stagedProtocol)
        {
            // Tier 2: cut back to this line's SECOND occurrence, then keep walking back over the
            // lines immediately before it that are themselves repeats of earlier ones — the
            // cycle's own opening lines (its comment header, a short setup call), which the
            // kMinLineLen filter never counted. Without that walk the cut lands mid-cycle and
            // keeps a stray fragment of a second pass. Only exact duplicates of something further
            // up are walked over, so nothing unique to the surviving pass is ever lost: every line
            // of the FIRST pass is its own first occurrence, which is where the walk stops.
            QHash<QString, int> firstIndex;
            for (int j = 0; j < lines.size(); ++j)
            {
                const QString probe = lines[j].trimmed();
                if (!probe.isEmpty() && !firstIndex.contains(probe))
                    firstIndex.insert(probe, j);
            }
            int seen = 0;
            for (int j = 0; j < i; ++j)
            {
                if (lines[j].trimmed() != trimmed || ++seen < 2)
                    continue;
                cutBefore = j;
                break;
            }
            while (cutBefore > 0)
            {
                const QString prev = lines[cutBefore - 1].trimmed();
                if (!prev.isEmpty() && firstIndex.value(prev, cutBefore - 1) >= cutBefore - 1)
                    break;
                --cutBefore;
            }
            cutBefore = snapRepetitionCutToTopLevel(lines, cutBefore);
            if (cutBefore < 0)
                return s;
        }
        QStringList kept = lines.mid(0, cutBefore);
        while (!kept.isEmpty() && kept.last().trimmed().isEmpty())
            kept.removeLast();
        // Nothing salvageable — leave the response alone rather than replace it with a
        // comment-only script, the same "don't delete the real answer" reflex as the staged-
        // protocol regression above.
        if (kept.isEmpty())
            return s;
        return kept.join('\n') + "\n\n# [truncated — repetition-loop garbage detected and removed]";
    }
    return s;
}

static QString cleanResponse(const QString &s)
{
    return truncateRepetitiveGarbage(stripCodeFences(stripThinkBlocks(s)).trimmed());
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
    m_contextHistoryStart = 0;
    m_lastScriptUnrun = false;
    m_pendingUserText.clear();
    m_cachedDocs.clear();
}

void AiAssistant::resetErrorRetryState()
{
    m_blockedRounds = 0;
    m_partVerifyRetried = false;
    m_partMissingNote.clear();
    m_lastBlockedScript.clear();
    m_freshRetryUsed = false;
    lastAvailableBlock.clear();
    lastRetryError.clear();
    isErrorRetryCorrection = false;
    consecutiveDocRetryCount = 0;
}

bool AiAssistant::hasUnrunLastScript() const
{
    return m_lastScriptUnrun && !m_history.isEmpty();
}

void AiAssistant::markLastScriptUnrun()
{
    m_lastScriptUnrun = true;
}

void AiAssistant::runLastScript(const QString &confirmationMessage)
{
    m_lastScriptUnrun = false;
    resetErrorRetryState();
    if (m_history.isEmpty())
        return;
    // stripCodeFences(): defense-in-depth, not the fix itself (see onReplyFinished()'s own
    // comment for that) — cheap and idempotent, and guards against any OTHER path that might
    // someday append unstripped text into m_history, given runActionScript() below is called
    // with compilable=true, skipping the canCompile() gate that would otherwise catch a stray
    // fence before it reaches the interpreter.
    const QString code = stripCodeFences(m_history.last().second);
    // Printed here — only once it's known whether the mechanical detectKnownMistakes() check
    // below (the SAME one runActionScript() itself runs) will block this script — rather than by
    // the caller before this even starts. Confirmed live: printing "no re-generation needed"
    // unconditionally beforehand was wrong exactly when a correction cycle fired right after it.
    const bool blocked =
        m_content && !m_content
                          ->detectKnownMistakes(code, mistakeCheckRequestText(), splitPartScripts.join("\n\n"),
                                                laterSplitPartDescriptions())
                          .isEmpty();
    if (m_host && !blocked)
        m_host->print(confirmationMessage);
    runActionScript(code, true, QString());
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

// Finds `wordPattern` (a raw regex fragment, e.g. "SPLIT" or "NO\\s*SPLIT") anywhere in `text` as
// a standalone ALL-CAPS word — \b on both sides rejects "SPLITa"/"aSPLIT" while still matching
// "SPLIT a"/"a SPLIT". An optional trailing colon is consumed too. On a match, *textOut (if given)
// gets `text` with the marker removed and surrounding whitespace collapsed via QString::simplified().
static bool findAndStripCapitalMarker(const QString &text, const QString &wordPattern, QString *textOut)
{
    const QRegularExpression re(R"(\b)" + wordPattern + R"(\b:?)");
    const QRegularExpressionMatch m = re.match(text);
    if (!m.hasMatch())
        return false;
    if (textOut)
        *textOut = (text.left(m.capturedStart()) + " " + text.mid(m.capturedEnd())).simplified();
    return true;
}

bool AiAssistant::parseSplitMarker(const QString &text, bool splitByDefault, QString *effectiveTextOut)
{
    // "NOSPLIT" is the mirror-image escape hatch for when splitByDefault is ON: forces this ONE
    // request to skip split mode regardless of the default. Checked first since a match makes
    // SPLIT irrelevant either way. Case-sensitive, capital-only, like parseListRequest()'s marker.
    QString afterNoSplit;
    if (findAndStripCapitalMarker(text, "NO\\s*SPLIT", &afterNoSplit))
    {
        if (effectiveTextOut)
            *effectiveTextOut = afterNoSplit;
        return false;
    }
    QString afterSplit;
    const bool splitMatch = findAndStripCapitalMarker(text, "SPLIT", &afterSplit);
    if (effectiveTextOut)
        *effectiveTextOut = splitMatch ? afterSplit : text;
    return splitByDefault || splitMatch;
}

bool AiAssistant::parseReviewMarker(const QString &text, bool reviewByDefault, QString *effectiveTextOut)
{
    QString afterNoReview;
    if (findAndStripCapitalMarker(text, "NO\\s*REVIEW", &afterNoReview))
    {
        if (effectiveTextOut)
            *effectiveTextOut = afterNoReview;
        return false;
    }
    QString afterReview;
    const bool reviewMatch = findAndStripCapitalMarker(text, "REVIEW", &afterReview);
    if (effectiveTextOut)
        *effectiveTextOut = reviewMatch ? afterReview : text;
    return reviewByDefault || reviewMatch;
}

bool AiAssistant::parseExploreMarker(const QString &text, bool exploreByDefault, QString *effectiveTextOut)
{
    // "NOEXPLORE" checked first, exactly as parseReviewMarker() checks "NOREVIEW" before "REVIEW":
    // the negative form contains the positive one as a substring, so testing the positive first
    // would match the tail of the negative and invert the caller's intent.
    QString afterNoExplore;
    if (findAndStripCapitalMarker(text, "NO\\s*EXPLORE", &afterNoExplore))
    {
        if (effectiveTextOut)
            *effectiveTextOut = afterNoExplore;
        return false;
    }
    QString afterExplore;
    const bool exploreMatch = findAndStripCapitalMarker(text, "EXPLORE", &afterExplore);
    if (effectiveTextOut)
        *effectiveTextOut = exploreMatch ? afterExplore : text;
    return exploreByDefault || exploreMatch;
}

AiAssistant::ParsedListRequest AiAssistant::parseListRequest(const QString &text)
{
    // Case-sensitive, capital-only, unlike parseSplitMarker()'s CaseInsensitiveOption above — see
    // this function's own header doc comment for why: this bypasses the whole split-plan LLM
    // decomposition, a much larger effect than that marker's simple on/off toggle, so it's worth
    // requiring a deliberate, unambiguous marker rather than matching casually.
    static const QRegularExpression markerRe(R"(^\s*LIST\b)");
    ParsedListRequest result;
    const QRegularExpressionMatch markerMatch = markerRe.match(text);
    if (!markerMatch.hasMatch())
        return result;

    const QString afterMarker = text.mid(markerMatch.capturedLength());
    result.items = parseNumberedListItems(afterMarker);
    if (result.items.size() < 2)
        return {}; // fail open — not enough numbered items to call this a list (isList stays false)

    result.isList = true;
    // Everything before the first numbered item is the instruction_block region — re-locate it
    // with the same item regex parseNumberedListItems() uses internally, since that function only
    // returns the captured text, not the match positions this needs.
    static const QRegularExpression itemRe(R"(^\s*\d+[.)]\s+.+$)", QRegularExpression::MultilineOption);
    const QRegularExpressionMatch firstItemMatch = itemRe.match(afterMarker);
    QString region = afterMarker.left(firstItemMatch.capturedStart());

    // SPLIT/NOSPLIT/REVIEW/NOREVIEW/EXPLORE/NOEXPLORE are checked and stripped from this region
    // FIRST — whatever text remains afterward becomes the real instruction_block, appended to every
    // item verbatim. Markers alone (nothing left after stripping) never become one. A marker NOT
    // stripped here doesn't just fail to take effect, it actively misfires: it survives as leftover
    // text, making the region look like a real instruction_block, and "NOEXPLORE" then gets
    // appended to every item of the list as an instruction.
    result.forceNoSplit = findAndStripCapitalMarker(region, "NO\\s*SPLIT", &region);
    if (!result.forceNoSplit)
        result.forceSplit = findAndStripCapitalMarker(region, "SPLIT", &region);
    result.forceNoReview = findAndStripCapitalMarker(region, "NO\\s*REVIEW", &region);
    if (!result.forceNoReview)
        result.forceReview = findAndStripCapitalMarker(region, "REVIEW", &region);
    result.forceNoExplore = findAndStripCapitalMarker(region, "NO\\s*EXPLORE", &region);
    if (!result.forceNoExplore)
        result.forceExplore = findAndStripCapitalMarker(region, "EXPLORE", &region);

    result.instructionBlock = region.trimmed();
    return result;
}

QStringList AiAssistant::parseNumberedListItems(const QString &text)
{
    // "N. text"/"N) text" pattern, one match per line — 2+ required by parseListRequest() (a
    // single numbered mention isn't a list), but this function itself just returns whatever it
    // finds, empty or not.
    static const QRegularExpression itemRe(R"(^\s*\d+[.)]\s+(.+)$)", QRegularExpression::MultilineOption);
    QStringList items;
    QRegularExpressionMatchIterator it = itemRe.globalMatch(text);
    while (it.hasNext())
        items << it.next().captured(1).trimmed();
    return items;
}

bool AiAssistant::looksLikeNewTaskReply(const QString &reply)
{
    static const QRegularExpression newRe(R"(\b(new|different|unrelated|separate)\b)",
                                          QRegularExpression::CaseInsensitiveOption);
    return newRe.match(reply).hasMatch();
}

bool AiAssistant::looksLikeRunItRequest(const QString &text)
{
    // Whole-message match (^...$, not just \b...\b) — deliberately narrower than
    // looksLikeNewTaskReply() above: a longer message that happens to contain "run it" as part of
    // something bigger ("run it and then plot the results") is a real new request, not a bare
    // re-execution ask, and must fall through to normal dispatch instead of short-circuiting here.
    static const QRegularExpression runItRe(R"(^\s*(?:please\s+)?(?:run|execute)\s+(?:it|that|this)\s*[.!]?\s*$)",
                                            QRegularExpression::CaseInsensitiveOption);
    return runItRe.match(text).hasMatch();
}

bool AiAssistant::looksLikeRunItAgainRequest(const QString &text)
{
    static const QRegularExpression runAgainRe(
        R"(^\s*(?:please\s+)?(?:run|execute|do)\s+(?:it|that|this)\s+again\s*[.!]?\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    return runAgainRe.match(text).hasMatch();
}

bool AiAssistant::looksLikeRunHistoryIndexRequest(const QString &text, int *indexOut)
{
    static const QRegularExpression re(R"(^\s*(?:please\s+)?(?:run|execute)\s+[QA](\d+)\s*[.!]?\s*$)",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(text);
    if (!m.hasMatch())
        return false;
    if (indexOut)
        *indexOut = m.captured(1).toInt();
    return true;
}

bool AiAssistant::runHistoryScript(int oneBasedIndex)
{
    m_lastScriptUnrun = false;
    if (oneBasedIndex < 1 || oneBasedIndex > m_history.size())
        return false;
    resetErrorRetryState();
    // stripCodeFences(): see runLastScript()'s own comment for why this is defense-in-depth.
    const QString code = stripCodeFences(m_history[oneBasedIndex - 1].second);
    // Same reasoning as runLastScript(): printed only once it's known this won't be blocked by
    // the mechanical detectKnownMistakes() check runActionScript() itself is about to run.
    const bool blocked =
        m_content && !m_content
                          ->detectKnownMistakes(code, mistakeCheckRequestText(), splitPartScripts.join("\n\n"),
                                                laterSplitPartDescriptions())
                          .isEmpty();
    if (m_host && !blocked)
        m_host->print(
            QString("▸▸▸ AI: running turn #%1's script directly (no re-generation needed).").arg(oneBasedIndex));
    runActionScript(code, true, QString());
    return true;
}

// The EXPLORE slot of the base prompt, which is now three-valued rather than two: invite a probe,
// forbid one, or say nothing.
//
// Forbidding matters independently of not inviting — see exploreUnavailableNote()'s doc comment for
// the 6-of-12 measurement. Note the invitation stays Integrated-mode-only (every other mode already
// receives the full "Agentic Explore Mode" doc section, so repeating it here would duplicate), while
// the PROHIBITION is sent in every mode: it exists precisely to contradict that doc section, so
// withholding it exactly where the section is present would defeat the point.
QString AiAssistant::liveEntityFactsFor(const QString &requestText, QString *consoleLine) const
{
    if (!m_content || !m_host)
        return {};
    const QList<QPair<QString, QString>> existingNames = extractExistingEntityNames(requestText);
    if (existingNames.isEmpty())
        return {};
    QStringList names;
    for (const QPair<QString, QString> &entry : existingNames)
        names << entry.first;
    // The host is asked, not consulted-and-second-guessed: the extracted KIND hint (entry.second —
    // what the request's own wording claimed the name was) is deliberately NOT used to filter the
    // answer. That hint is the request's assumption, and the whole point here is to report what is
    // really there, including the case where the two disagree.
    const QList<QStringList> kinds = m_host->entityKinds(names);
    // A host that can't answer returns nothing (see AiScriptHost.h) — say nothing rather than
    // reporting every name as missing, which would be a confident lie in the costly direction.
    if (kinds.size() != names.size())
        return {};
    QList<QPair<QString, QStringList>> facts;
    QStringList summary;
    for (int i = 0; i < names.size(); ++i)
    {
        facts << qMakePair(names[i], kinds[i]);
        summary << "\"" + names[i] + "\": " + (kinds[i].isEmpty() ? QString("not found") : kinds[i].join("+"));
    }
    if (consoleLine)
        *consoleLine = "project state — " + summary.join("; ");
    return m_content->liveEntityFactsNote(facts);
}

QString AiAssistant::existingInputNamesNote(const QString &requestText, QStringList *existingTables,
                                            QString *consoleLine) const
{
    if (!m_content || !m_host)
        return {};
    const QStringList names = inputNamesInRequest(requestText);
    if (names.isEmpty())
        return {};
    const QList<QStringList> kinds = m_host->entityKinds(names);
    if (kinds.size() != names.size())
        return {};
    // Only what exists: when scripts run, "not found" is no fact worth stating (an earlier item may create it
    // first), while "exists NOW, as a table" is exactly what a weak model needs to use table() instead of newTable().
    QList<QPair<QString, QStringList>> facts;
    QStringList summary;
    for (int i = 0; i < names.size(); ++i)
    {
        if (kinds[i].isEmpty())
            continue;
        // A table's columns, roles and rows go with its existence: the column names of a table the request names were
        // the most frequent EXPLORE probe (about 30 of 36 in the 2026-10-11 runs), 19 of them the prompt's own example
        // copied word for word. Facts the host holds are given, not asked for.
        QStringList described = kinds[i];
        if (const qsizetype t = described.indexOf("table"); t >= 0)
            if (const QString shape = m_host->tableShape(names[i]); !shape.isEmpty())
                described[t] = "table (" + shape + ")";
        facts << qMakePair(names[i], described);
        summary << "\"" + names[i] + "\": " + described.join("+");
        if (existingTables && kinds[i].contains("table"))
            *existingTables << names[i];
    }
    if (facts.isEmpty())
        return {};
    if (consoleLine)
        *consoleLine = "project state — " + summary.join("; ");
    return m_content->liveEntityFactsNote(facts);
}

QString AiAssistant::formulaColumnNameMistake(const QString &code) const
{
    // Confirmed live (LIST 3 item 3, 2026-10-11, NOSPLIT): the PROJECT STATE note gave "Squares" with columns "x",
    // "y", "dy", "d" and table-python-api says col("1") is the column NAMED "1"; the retry still wrote
    // setCommand(4, "1/(col(\"1\")-5)"), every row failed, and the item printed 10 empty cells. Only a table this
    // script looks up and does not rename (setColNames()), checked against its columns as the host has them now.
    if (!m_host)
        return {};
    static const QRegularExpression lookupRe(R"RX(\b(\w+)\s*=\s*table\s*\(\s*["']([^"'\n]+)["']\s*\))RX");
    static const QRegularExpression shapeColumnRe(R"RX(\d+ "([^"]*)")RX");
    static const QRegularExpression colRe(R"RX(\bcol\s*\(\s*\\?["']([^"'\\]+)\\?["']\s*\))RX");
    for (auto it = lookupRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch lookup = it.next();
        const QString var = QRegularExpression::escape(lookup.captured(1));
        if (QRegularExpression("\\b" + var + R"(\s*\.\s*setColNames\s*\()").match(code).hasMatch())
            continue;
        const QString shape = m_host->tableShape(lookup.captured(2));
        if (shape.isEmpty())
            continue;
        QStringList columns;
        for (auto c = shapeColumnRe.globalMatch(shape); c.hasNext();)
            columns << c.next().captured(1);
        const QRegularExpression commandRe("\\b" + var + R"RX(\s*\.\s*setCommand\s*\([^\n]*)RX");
        for (auto c = commandRe.globalMatch(code, lookup.capturedEnd()); c.hasNext();)
        {
            const QString line = c.next().captured(0);
            for (auto r = colRe.globalMatch(line); r.hasNext();)
            {
                const QString name = r.next().captured(1);
                if (columns.contains(name))
                    continue;
                QStringList listed;
                for (int i = 0; i < columns.size(); ++i)
                    listed << QString("%1 \"%2\"").arg(i + 1).arg(columns[i]);
                return "A column formula of table \"" + lookup.captured(2) + "\" uses col(\"" + name +
                       "\"), but that table has no column named \"" + name + "\" — its columns are " +
                       listed.join(", ") + ". col(\"name\") takes a column NAME; for a column number write col(" +
                       "index), e.g. col(1).";
            }
        }
    }
    return {};
}

QStringList newTableNamesThatRun(const QString &code)
{
    // Confirmed live twice (series 4, local): a newTable() blocked as "would wipe" could not run —
    // (1) LIST 4 item 2: `t = table("Peak")` / `if t is None: t = newTable("Peak", ...)` on an existing "Peak";
    // (2) LIST 5 item 5: newTable("Auto", ...) inside the text of a note tab (a string literal), never executed.
    // Each item was lost to the repeated block. Returned: the names of newTable() calls outside string literals and
    // not inside an "if" that runs only when that table is missing.
    static const QRegularExpression literalRe(
        R"RX("""[\s\S]*?"""|'''[\s\S]*?'''|"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')RX");
    QString blanked = code; // each literal's characters blanked, newlines kept: offsets stay valid
    for (auto it = literalRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        // The name argument itself stays readable: only literals that are not newTable()'s first argument go.
        static const QRegularExpression callBeforeRe(R"(\bnewTable\s*\(\s*$)");
        if (callBeforeRe.match(blanked.left(m.capturedStart())).hasMatch())
            continue;
        for (qsizetype i = m.capturedStart(); i < m.capturedEnd(); ++i)
            if (blanked.at(i) != QLatin1Char('\n'))
                blanked[static_cast<int>(i)] = QLatin1Char(' ');
    }
    auto indentOf = [](const QString &line) {
        qsizetype n = 0;
        while (n < line.size() && (line.at(n) == ' ' || line.at(n) == '\t'))
            ++n;
        return n;
    };
    const QStringList lines = blanked.split('\n');
    const QStringList original = code.split('\n'); // the same lines with their literals: the checks read names
    static const QRegularExpression newTableRe(R"RX(\bnewTable\s*\(\s*["']([^"']+)["'])RX");
    // Line `at` opens a block that runs only when table `name` is missing: "if not existTable("<name>"):", or
    // "if <v> is None:" / "if not <v>:" with <v> = table("<name>") assigned above it.
    auto opensMissingBlock = [&original](int at, const QString &name) {
        const QString esc = QRegularExpression::escape(name);
        const QRegularExpression existRe(R"RX(^\s*if\s+not\s+existTable\s*\(\s*["'])RX" + esc + R"RX(["']\s*\)\s*:)RX");
        static const QRegularExpression noneRe(R"(^\s*if\s+(?:(\w+)\s+is\s+None|not\s+(\w+))\s*:)");
        if (existRe.match(original[at]).hasMatch())
            return true;
        const QRegularExpressionMatch none = noneRe.match(original[at]);
        if (!none.hasMatch())
            return false;
        const QString var = none.captured(1).isEmpty() ? none.captured(2) : none.captured(1);
        const QRegularExpression lookupRe("\\b" + QRegularExpression::escape(var) + R"RX(\s*=\s*table\s*\(\s*["'])RX" +
                                          esc + R"RX(["']\s*\))RX");
        return lookupRe.match(original.mid(0, at).join('\n')).hasMatch();
    };
    QStringList names;
    for (int li = 0; li < lines.size(); ++li)
        for (auto it = newTableRe.globalMatch(lines[li]); it.hasNext();)
        {
            const QString name = it.next().captured(1);
            // The same line ("if t is None: t = newTable(...)") or the nearest enclosing, less indented line above.
            bool guarded = opensMissingBlock(li, name);
            for (int up = li - 1; up >= 0 && !guarded && indentOf(lines[li]) > 0; --up)
                if (!original[up].trimmed().isEmpty() && indentOf(original[up]) < indentOf(lines[li]))
                {
                    guarded = opensMissingBlock(up, name);
                    break; // only the nearest enclosing block decides
                }
            if (!guarded && !names.contains(name))
                names << name;
        }
    return names;
}

QString AiAssistant::existingTableRecreatedMistake(const QString &code) const
{
    for (const QString &name : newTableNamesThatRun(code))
    {
        if (m_existingInputTables.contains(name))
            return "This script calls newTable(\"" + name + "\"), but table \"" + name +
                   "\" already exists and this request only uses it. newTable() on an existing name WIPES it "
                   "(resized, all data cleared) — use t = table(\"" +
                   name + "\") and work on the table as it is.";
        // Confirmed live (LIST 5, item 2): part 2 of 3 ("compile the functions") also re-ran part 1's
        // newTable("Peak5", ...) — harmless with the same data, a wiped table with any other. "Exists now" is the
        // host's live answer, not the earlier script's text (series 4: a newTable() in a note tab's text was read
        // as a created table that never existed).
        if (!splitModeActive || !m_host)
            continue;
        bool createdEarlier = false;
        for (const QString &earlier : splitPartScripts)
            createdEarlier = createdEarlier || newTableNamesThatRun(earlier).contains(name);
        if (!createdEarlier)
            continue;
        const QList<QStringList> kinds = m_host->entityKinds({name});
        if (kinds.size() == 1 && kinds.first().contains("table"))
            return "This script calls newTable(\"" + name +
                   "\"), but an earlier part of this plan already created table \"" + name +
                   "\" and it exists now. newTable() on an existing name WIPES it — this part must not create it "
                   "again. Remove that table-creating code; if this part needs the table, use t = table(\"" +
                   name + "\").";
    }
    return {};
}

QStringList AiAssistant::withoutTrailingOutputLines(const QString &code)
{
    // Up to 3 lines, each unindented, with balanced brackets and quotes, not a Python statement start and not a line
    // that continues: e.g. "R1: 2 print(6*7)" after the code. Real code passes these tests too, so the caller takes
    // the SMALLEST cut that compiles (returned first). A truncated statement is never taken for output.
    static const QRegularExpression keywordRe(
        R"(^(?:def|class|if|elif|else|for|while|try|except|finally|with|return|import|from|raise|del|pass|lambda|)"
        R"(global|assert|yield|async|await)\b)");
    QStringList lines = code.split('\n');
    auto dropBlankTail = [&lines]() {
        while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
            lines.removeLast();
    };
    dropBlankTail();
    QStringList candidates;
    while (!lines.isEmpty() && candidates.size() < 3)
    {
        const QString line = lines.last();
        const QString t = line.trimmed();
        if (line.startsWith(' ') || line.startsWith('\t') || keywordRe.match(t).hasMatch() || t.endsWith(':') ||
            t.endsWith(',') || t.endsWith('\\') || t.count('(') != t.count(')') || t.count('[') != t.count(']') ||
            t.count('{') != t.count('}') || t.count('"') % 2 || t.count('\'') % 2)
            break;
        lines.removeLast();
        dropBlankTail();
        if (lines.isEmpty())
            break;
        candidates << lines.join('\n');
    }
    return candidates;
}

bool AiAssistant::isPureTextAnswer(const QString &code)
{
    // Plain literals only: an f-string or a concatenation reads values, and those are worth reviewing.
    static const QRegularExpression literalPrintRe(
        R"re(^(?:scriptPrint|print)\(\s*[rR]?("|')(?:(?!\1)[^\\]|\\.)*\1\s*\)$)re");
    bool any = false;
    for (const QString &line : code.split('\n'))
    {
        const QString t = line.trimmed();
        if (t.isEmpty() || t.startsWith('#'))
            continue;
        if (!literalPrintRe.match(t).hasMatch())
            return false;
        any = true;
    }
    return any;
}

bool AiAssistant::isShortQuestion(const QString &requestText)
{
    static const QRegularExpression actionVerbRe(
        R"(\b(?:create|make|plot|draw|build|generate|fit|compile|add|delete|remove|rename|set|change|calculate|)"
        R"(compute|load|save|export|import|open|close|fix|run|fill|sort|copy|move|replace|convert|write)\b)",
        QRegularExpression::CaseInsensitiveOption);
    const QString t = requestText.trimmed();
    return t.endsWith('?') && t.size() <= 300 && !actionVerbRe.match(t).hasMatch();
}

QString AiAssistant::labeledLastPrintRepair(const QString &code, const QString &removedText)
{
    // Repaired, not asked for: confirmed live, a retry asked to only add "R12: " in front of a print that was right
    // (str([...]) -> "[]") rewrote it as " ".join(...) -> "" instead, in run after run.
    if (unprintedOutputLineMistake(code, removedText).isEmpty())
        return {};
    static const QRegularExpression labelLineRe(R"(^\s*([A-Za-z][\w ]{0,30}?)\s*:\s*\S)");
    QString label;
    for (const QString &line : removedText.split('\n'))
        if (const QRegularExpressionMatch m = labelLineRe.match(line); m.hasMatch())
        {
            label = m.captured(1);
            break;
        }
    QStringList lines = code.split('\n');
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
        lines.removeLast();
    if (label.isEmpty() || lines.isEmpty())
        return {};
    static const QRegularExpression printRe(R"(^(scriptPrint|print)\((.*)\)\s*$)");
    const QRegularExpressionMatch m = printRe.match(lines.last());
    // A print with a label of its own ("Text of Calc tab: " + ...) is one of several described lines, not the bare
    // values: confirmed live, labeling it gave "R1: Text of Calc tab: print('A')". Left to the retry.
    static const QRegularExpression ownLabelRe(R"(^\s*[fFrR]?["'][^"']*:\s*["'{])");
    if (!m.hasMatch() || m.captured(2).trimmed().isEmpty() || ownLabelRe.match(m.captured(2)).hasMatch())
        return {};
    lines.last() = m.captured(1) == "print" ? "print(\"" + label + ":\", " + m.captured(2) + ")"
                                            : "scriptPrint(\"" + label + ": \" + str(" + m.captured(2) + "))";
    return lines.join('\n') + '\n';
}

QString AiAssistant::withoutTrailingAnnotationOutput(const QString &code)
{
    // "name: <number|quote|bracket>" at the top level: an annotation that is never a type, i.e. an output line.
    static const QRegularExpression outputLineRe(R"(^[A-Za-z_]\w*\s*:\s*(?:[-+]?\d|["'\[(]))");
    QStringList lines = code.split('\n');
    auto dropBlankTail = [&lines]() {
        while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
            lines.removeLast();
    };
    dropBlankTail();
    int removed = 0;
    while (!lines.isEmpty() && removed < 3 && outputLineRe.match(lines.last()).hasMatch())
    {
        lines.removeLast();
        dropBlankTail();
        ++removed;
    }
    return removed > 0 && !lines.isEmpty() ? lines.join('\n') : QString();
}

namespace
{
// The `if X is None:` / `if not exist...("X"):` block that skips the failed statement — see
// AiAssistant::skippedFailedStepMistake(). guardLine is -1 when there is none.
struct SkippedFailedStep
{
    int guardLine = -1;
    int blockEnd = 0; // the first line after the guarded block
    QString guardText;
    QString object;
};

SkippedFailedStep findSkippedFailedStep(const QString &code, const QString &failedStatement,
                                        const QStringList &failedRunObjects)
{
    static const QRegularExpression callRe(R"((?:^|[=\s.])([A-Za-z_]\w*)\s*\()");
    const QRegularExpressionMatch failedCall = callRe.match(failedStatement.trimmed());
    if (!failedCall.hasMatch() || failedRunObjects.isEmpty())
        return {};
    const QRegularExpression callInLineRe(R"(\b)" + QRegularExpression::escape(failedCall.captured(1)) + R"(\s*\()");
    // var = table("Calib") / matrix(...) / note(...) ...: the guarded variable's object name
    static const QRegularExpression lookupRe(R"RX(^\s*(\w+)\s*=\s*\w+\s*\(\s*["']([^"']+)["']\s*\))RX",
                                             QRegularExpression::MultilineOption);
    QHash<QString, QString> objectOf;
    for (auto it = lookupRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        objectOf[m.captured(1)] = m.captured(2);
    }
    static const QRegularExpression guardRe(
        R"RX(^([ \t]*)if\s+(?:(\w+)\s+is\s+None|not\s+\w*[Ee]xist\w*\s*\(\s*["']([^"']+)["']\s*\))\s*:[ \t]*$)RX",
        QRegularExpression::MultilineOption);
    auto indentOf = [](const QString &line) {
        qsizetype n = 0;
        while (n < line.size() && (line.at(n) == ' ' || line.at(n) == '\t'))
            ++n;
        return n;
    };
    const QStringList lines = code.split('\n');
    for (auto it = guardRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch guard = it.next();
        const QString object = guard.captured(2).isEmpty() ? guard.captured(3) : objectOf.value(guard.captured(2));
        bool existed = false;
        for (const QString &failed : failedRunObjects)
            if (!object.isEmpty() && failed.endsWith(":" + object))
                existed = true;
        if (!existed)
            continue;
        const qsizetype indent = guard.captured(1).size();
        const int guardLine = static_cast<int>(code.left(guard.capturedStart(0)).count('\n'));
        bool containsFailed = false;
        int i = guardLine + 1;
        for (; i < lines.size(); ++i)
        {
            const QString &line = lines.at(i);
            if (line.trimmed().isEmpty())
                continue;
            if (indentOf(line) <= indent) // the guarded block ends here
                break;
            if (callInLineRe.match(line).hasMatch())
                containsFailed = true;
        }
        if (containsFailed)
            return {guardLine, i, guard.captured(0).trimmed(), object};
    }
    return {};
}
} // namespace

QString AiAssistant::guardedCreationRepair(const QString &code, const QStringList &objectsBefore)
{
    // Confirmed live (LIST 5, item 1, NOSPLIT): `t = table("Calib")` / `if t is None:` held the table's whole set-up,
    // corrected formula included; the table existed from the failed attempt, so none of it ran. Only the creating
    // call belongs behind such a check: the set-up after it runs on the table either way. Not for a table or matrix
    // that existed before this request: there the set-up may be sample data meant only for a missing one.
    static const QRegularExpression guardRe(R"(^([ \t]*)if\s+(\w+)\s+is\s+None\s*:[ \t]*$)");
    static const QRegularExpression elseRe(R"(^\s*(?:else|elif)\b)");
    auto indentOf = [](const QString &line) {
        qsizetype n = 0;
        while (n < line.size() && (line.at(n) == ' ' || line.at(n) == '\t'))
            ++n;
        return n;
    };
    static const QRegularExpression lookupRe(R"RX(^\s*(\w+)\s*=\s*(table|matrix)\s*\(\s*["']([^"']+)["']\s*\))RX",
                                             QRegularExpression::MultilineOption);
    QHash<QString, QString> objectOf;
    for (auto it = lookupRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        objectOf[m.captured(1)] = m.captured(2) + ":" + m.captured(3);
    }
    QStringList lines = code.split('\n');
    bool any = false;
    for (int g = 0; g < lines.size(); ++g)
    {
        const QRegularExpressionMatch guard = guardRe.match(lines.at(g));
        if (!guard.hasMatch() || objectsBefore.contains(objectOf.value(guard.captured(2))))
            continue;
        const qsizetype indent = guard.captured(1).size();
        const QRegularExpression createRe(R"(^\s*)" + QRegularExpression::escape(guard.captured(2)) +
                                          R"(\s*=\s*new(?:Table|Matrix)\s*\()");
        int first = g + 1; // the block's first statement must be the creating call
        while (first < lines.size() && lines.at(first).trimmed().isEmpty())
            ++first;
        if (first >= lines.size() || indentOf(lines.at(first)) <= indent || !createRe.match(lines.at(first)).hasMatch())
            continue;
        const qsizetype blockIndent = indentOf(lines.at(first));
        int end = first + 1;
        for (; end < lines.size(); ++end)
            if (!lines.at(end).trimmed().isEmpty() && indentOf(lines.at(end)) <= indent)
                break;
        if (end < lines.size() && elseRe.match(lines.at(end)).hasMatch())
            continue; // moving the set-up out would split the if from its else
        int moved = 0;
        for (int i = first + 1; i < end; ++i)
            if (!lines.at(i).trimmed().isEmpty())
            {
                if (indentOf(lines.at(i)) < blockIndent) // a continuation line: leave the block alone
                {
                    moved = 0;
                    break;
                }
                ++moved;
            }
        if (moved == 0)
            continue;
        for (int i = first + 1; i < end; ++i)
            if (!lines.at(i).trimmed().isEmpty())
                lines[i] = guard.captured(1) + lines.at(i).mid(blockIndent);
        any = true;
        g = first;
    }
    return any ? lines.join('\n') : QString();
}

QString AiAssistant::skippedFailedStepMistake(const QString &code, const QString &failedStatement,
                                              const QStringList &failedRunObjects)
{
    // Confirmed live (LIST 5, item 1, NOSPLIT, local model): recalculate() failed on "^" in the column formula; the
    // retry put the whole table creation, corrected formula included, behind `t = table("Calib")` / `if t is None:`.
    // Calib existed from the failed attempt, so the block was skipped, the column stayed empty, and fit() failed twice.
    const SkippedFailedStep step = findSkippedFailedStep(code, failedStatement, failedRunObjects);
    if (step.guardLine < 0)
        return {};
    return "The step that failed (" + failedStatement.trimmed() + ") is now inside \"" + step.guardText +
           "\" — but \"" + step.object +
           "\" exists since the failed attempt, so this block is skipped and the corrected step never "
           "runs. Run the corrected step outside the check, on the existing \"" +
           step.object + "\" (or create it again: the new... call on its name clears it).";
}

QString AiAssistant::skippedFailedStepRepair(const QString &code, const QString &failedStatement,
                                             const QStringList &failedRunObjects)
{
    // Confirmed live (LIST 5, item 1, NOSPLIT): told so twice, the local model only added the comment "Recalculate
    // outside the existence check" and left the block as it was. The guard goes, its block runs every time: the
    // object came from this request's own failed attempt, so creating it again is what the request asked for.
    const SkippedFailedStep step = findSkippedFailedStep(code, failedStatement, failedRunObjects);
    if (step.guardLine < 0)
        return {};
    QStringList lines = code.split('\n');
    static const QRegularExpression elseRe(R"(^\s*(?:else|elif)\b)");
    if (step.blockEnd < lines.size() && elseRe.match(lines.at(step.blockEnd)).hasMatch())
        return {}; // an else branch would run too: left to the model
    const QString guardIndent = lines.at(step.guardLine).left(lines.at(step.guardLine).indexOf("if"));
    qsizetype blockIndent = -1;
    for (int i = step.guardLine + 1; i < step.blockEnd; ++i)
    {
        const QString &line = lines.at(i);
        if (line.trimmed().isEmpty())
            continue;
        qsizetype n = 0;
        while (n < line.size() && (line.at(n) == ' ' || line.at(n) == '\t'))
            ++n;
        if (blockIndent < 0 || n < blockIndent)
            blockIndent = n;
    }
    if (blockIndent <= guardIndent.size())
        return {};
    for (int i = step.guardLine + 1; i < step.blockEnd; ++i)
        if (!lines.at(i).trimmed().isEmpty())
            lines[i] = guardIndent + lines.at(i).mid(blockIndent);
    lines.removeAt(step.guardLine);
    return lines.join('\n');
}

QString AiAssistant::unprintedOutputLineMistake(const QString &code, const QString &removedText)
{
    // "R5: beta 3" or "Number of tabs: 2": a short label, a colon, a value. The label counts as printed when any
    // string literal in the script starts with it followed by a colon (print("R5:", x), f"R5: {x}") — its trailing
    // digits may differ: confirmed live, "R1: 2 print(6*7)" written after a script for item 10.
    static const QRegularExpression labelLineRe(R"(^\s*([A-Za-z][\w ]{0,30}?)\s*:\s*\S)");
    for (const QString &line : removedText.split('\n'))
    {
        const QRegularExpressionMatch m = labelLineRe.match(line);
        if (!m.hasMatch())
            continue;
        const QString label = m.captured(1);
        QString stem = label;
        while (!stem.isEmpty() && stem.back().isDigit())
            stem.chop(1);
        const QRegularExpression printedRe(R"([fFrR]?["'])" + QRegularExpression::escape(stem) + R"(\d*\s*:)");
        if (printedRe.match(code).hasMatch())
            continue;
        return "Your answer ended with this text after the script: \"" + line.trimmed() +
               "\". It is not Python, so it was removed — and the script itself never prints a \"" + label +
               ":\" line, so that requested output is missing. If the script already prints those values, keep that "
               "print "
               "exactly as it is and only put \"" +
               label + ": \" in front of it, e.g. scriptPrint(\"" + label +
               ": \" + str(...)). Otherwise print the line FROM the script, with the values the script computes in "
               "the form the request asks for — never write the values into the string yourself.";
    }
    return {};
}

QString AiAssistant::currentScopeDescription() const
{
    if (splitModeActive && splitPartIndex >= 0 && splitPartIndex < splitPlan.size())
        return splitPlan[splitPartIndex].description;
    if (!m_resultLineTask.isEmpty())
        return m_resultLineTask;
    return m_singlePartDescription.isEmpty()
               ? withoutRequestSuffix(originalRequestText)
               : withoutRequestSuffix(originalRequestText) + "\n" + m_singlePartDescription;
}

// The request a candidate script is checked against: the item's, or — while its missing result line is asked for —
// that print-only task. Confirmed live (LIST 5 local pyi, item 5): the item's first script made the note, ran its tab
// and created "Auto"; the result-line follow-up only printed, and the creation check, reading the item's "Create a
// note ...", blocked it twice as "creation never happened" until the item was given up.
QString AiAssistant::mistakeCheckRequestText() const
{
    return m_resultLineTask.isEmpty() ? originalRequestText : m_resultLineTask;
}

QString AiAssistant::laterSplitPartDescriptions() const
{
    if (!splitModeActive)
        return {};
    QStringList later;
    for (int i = splitPartIndex + 1; i < splitPlan.size(); ++i)
        later << splitPlan[i].description;
    return later.join("\n");
}

QString AiAssistant::generatedScriptWarning(const QString &code) const
{
    if (!m_content || code.isEmpty())
        return {};
    // Same scope description runActionScript() uses: this part's own job in split mode, otherwise
    // this turn's request. pendingRequestText, not originalRequestText — a LIST item sets the
    // former per item, while the latter stays anchored to the first request of the conversation.
    const QString scopeDescription =
        (splitModeActive && splitPartIndex >= 0 && splitPartIndex < splitPlan.size())
            ? splitPlan[splitPartIndex].description
            : (m_singlePartDescription.isEmpty()
                   ? withoutRequestSuffix(pendingRequestText)
                   : withoutRequestSuffix(pendingRequestText) + "\n" + m_singlePartDescription);
    if (scopeDescription.isEmpty())
        return {};

    // All three of runActionScript()'s pre-run ANALYSES, not just the deliverable one. They were
    // reachable ONLY from the auto-run path, so a generate-only script had never in this project's
    // history been looked at by any of them — confirmed live by what that let through unremarked
    // across three consecutive 12-item runs: QFont(...) with no "from PyQt6.QtGui import QFont"
    // anywhere (missingPyQtImportMistake) and random.gauss() with no "import random"
    // (missingRandomImportMistake). Nothing was wrong with either check; neither was ever called
    // here.
    //
    // runActionScript()'s two REPAIRS (compilerParamCountRepair(), compilerYNameAssignmentRepair())
    // are deliberately NOT included. They rewrite the script rather than describe it, and on this
    // path the script is both what gets printed and — via markLastScriptUnrun()/runLastScript(),
    // which re-reads m_history.last() — what a later "run it" would execute, so applying one here
    // means patching history too. That is a behaviour change, not a missing call.
    //
    // Collect-all rather than first-match, same reasoning as the auto-run branch's own: the user is
    // reading one printed script and gets exactly one chance to be told everything wrong with it.
    // splitPartScripts is empty outside split mode, so earlierPartsCode passes through naturally.
    QStringList headers;
    QStringList explanations;
    const QString missingDeliverables =
        m_userCorrections.isEmpty() ? m_content->missingRequestedDeliverables(code, scopeDescription) : QString();
    if (!missingDeliverables.isEmpty())
    {
        headers << "requested deliverable missing";
        explanations << missingDeliverables;
    }
    const QString scopeViolation = m_userCorrections.isEmpty()
                                       ? m_content->splitPartScopeViolation(
                                             code, scopeDescription, splitPartScripts.join("\n\n"), !splitModeActive)
                                       : QString();
    if (!scopeViolation.isEmpty())
    {
        headers << "out of scope";
        explanations << scopeViolation;
    }
    for (const AiKnownMistakeMatch &m : m_content->detectKnownMistakes(
             code, mistakeCheckRequestText(), splitPartScripts.join("\n\n"), laterSplitPartDescriptions()))
    {
        headers << m.header;
        explanations << m.explanation;
    }
    if (explanations.isEmpty())
        return {};
    if (explanations.size() == 1)
        return headers.first() + " — " + explanations.first();
    QStringList numbered;
    for (int i = 0; i < explanations.size(); ++i)
        numbered << QString("%1. %2").arg(i + 1).arg(explanations.at(i));
    return headers.join("; ") + " — " + QString::number(explanations.size()) + " separate issues in this script:\n\n" +
           numbered.join("\n\n");
}

QString AiAssistant::exploreSlotText(bool expectsCode) const
{
    if (!m_content)
        return {};
    // A preflight whose answer is plain text (a PART/DOCS plan, a doc topic list, ...) gets neither
    // — see buildSystemPrompt()'s own note on why mentioning scripts there skews the answer.
    if (!expectsCode)
        return {};
    if (!m_exploreEnabled || !autoRun)
        return m_content->exploreUnavailableNote();
    // Invited only when the request refers to an object that neither exists nor is created by it. Confirmed live
    // (2026-10-11): with the project state already giving existence and each table's columns, the model still probed
    // 11 times in one LIST 3 run, asking what that state said ("check if table Squares exists and get its column
    // names"); it probes because it is invited, and NOEXPLORE scored equal or better in every comparison.
    if (!requestHasUnknownObject())
        return {};
    return docsMode() == DocsMode::Integrated ? m_content->exploreModeReminder() : QString();
}

bool AiAssistant::requestHasUnknownObject() const
{
    if (!m_host)
        return true;
    if (m_unknownObjectCheckedFor == originalRequestText)
        return m_unknownObject;
    m_unknownObjectCheckedFor = originalRequestText;
    static const QRegularExpression objectRe(R"RX(\b(?:table|matrix|graph|note|window|plot|functions?|in|from)\s+)RX"
                                             R"RX((?:the\s+)?"([^"\n]{1,80})"|"([^"\n/]{1,80}/[^"\n]{1,80})")RX",
                                             QRegularExpression::CaseInsensitiveOption);
    QStringList names;
    for (auto it = objectRe.globalMatch(originalRequestText); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (!names.contains(name) && !requestCreatesName(originalRequestText, name))
            names << name;
    }
    m_unknownObject = false;
    if (!names.isEmpty())
    {
        const QList<QStringList> kinds = m_host->entityKinds(names);
        if (kinds.size() != names.size())
            m_unknownObject = true; // the host cannot tell: a probe may help
        for (const QStringList &k : kinds)
            m_unknownObject = m_unknownObject || k.isEmpty();
    }
    return m_unknownObject;
}

QString AiAssistant::buildSystemPrompt(const QString &systemPrompt, bool includeDocs, const QStringList &docTopics,
                                       bool expectsCode, const QString &userText)
{
    // exploreModeReminder() only for Integrated mode (every other mode already gets the FULL
    // "Agentic Explore Mode" doc section attached wholesale, so repeating it here would be pure
    // duplication — see that hook's own doc comment in AiContentProvider.h) AND only when THIS
    // call actually expects a script back — never for a preflight whose answer is plain text (a
    // PART/DOCS plan, a doc topic list, ...). Confirmed live: reminding the model it can return a
    // script right before a call that explicitly wants a plain-text plan measurably increased how
    // often it returned one instead of the plan — see ask()'s expectsCode doc comment.
    //
    // kExistenceHandlingRules is only spliced in (via %4) when `userText` actually looks like it
    // involves existence-checking — see kExistenceHandlingRules' own doc comment on why this used
    // to be unconditional. Empty `userText` (no caller-provided text to check) fails OPEN —
    // included by default — so a future call site that doesn't pass it doesn't silently lose this
    // rule; ask() always passes its own userText, so this only ever applies to some other caller.
    static const QRegularExpression existenceRelevanceRe(R"(\bexist\w*\b|\bmissing\b|\bnot\s+found\b)",
                                                         QRegularExpression::CaseInsensitiveOption);
    const bool needsExistenceRules = userText.isEmpty() || existenceRelevanceRe.match(userText).hasMatch();
    const QString existenceRulesText =
        needsExistenceRules
            ? kExistenceHandlingRules.arg(m_content ? m_content->existenceVerificationFunctionExamples() : QString())
                  .arg(m_content ? m_content->noOpPrintFunctionName() : QString())
            : QString();
    QString sysPrompt = kBasePromptTemplate.arg(m_content ? m_content->apiCallingConventionHint() : QString())
                            .arg(m_content ? m_content->assistantIdentityIntro() : QString())
                            .arg(kPyQtImportBaseRule)
                            .arg(existenceRulesText)
                            .arg(exploreSlotText(expectsCode));

    if (!systemPrompt.isEmpty())
        return sysPrompt + systemPrompt;

    // The API docs are tens of thousands of tokens, and every ask() call is a stateless HTTP
    // request that must resend the full system prompt. `askSelfReviewRound()` (the checklist-
    // compliance pass and its regression backstops) is the one call site that still deliberately
    // passes includeDocs=false — it isn't generating code from API knowledge, so full doc content
    // adds cost with no benefit there (see maybeDispatchSelfReview()'s doc comment). Every other
    // call site — initial generation, the EXPLORE-output-to-action-script follow-up, and
    // runActionScript()'s error/block-retry — passes includeDocs=true with selectedDocTopics
    // (narrowed to just the relevant files — see filterDocsByTopic()), since all three actually
    // regenerate substantial code and were confirmed live to drift into hallucinated APIs without
    // it (see runActionScript()'s doc comment on the specific incidents this closed).
    //
    // RESIDUAL RISK, not yet hit again but worth watching: this docs-on-every-retry design was
    // ORIGINALLY includeDocs=false specifically for retries, precisely because resending docs on
    // every one of up to m_maxAutoContinue attempts, compounding with growing history, was seen
    // live to blow the context window ("maximum context length" after 2-3 retries on one
    // request). windowedHistoryStart()'s ~80,000-char cap bounds the HISTORY portion of the
    // payload regardless of includeDocs, but nothing caps the DOCS portion itself — a request
    // whose selectedDocTopics span several large files, retried close to m_maxAutoContinue times
    // (configurable well above the documented default of 4), could still reach this same failure
    // mode again on a smaller-context-window model. No incident yet; flagging so a future report
    // of "maximum context length" isn't mistaken for a new bug.
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
        {
            // Empty docTopics ("all") would otherwise fail open to the entire corpus — baseline
            // (ai/app) is a cheaper, still-safe fallback for an unrecognized/topic-less request.
            const QStringList effectiveTopics =
                docTopics.isEmpty() && m_content ? m_content->baselineDocTopics() : docTopics;
            sysPrompt += kDocsIntro + filterDocsByTopic(m_cachedDocs, effectiveTopics);
        }
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

qsizetype AiAssistant::historyLowerBound() const
{
    return qMax(m_contextHistoryStart, m_listHistoryStart);
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
    // i never goes below m_contextHistoryStart, OR m_listHistoryStart while a LIST is in
    // progress (see that field's own doc comment) — turns before either boundary belong to a
    // task/list the continuation-check or the current LIST call has already moved past;
    // excluding them from what gets sent is independent of, and stacks with, the token-budget
    // cutoff below. m_listHistoryStart stays 0 outside LIST mode, so qMax is a no-op then.
    const qsizetype lowerBound = historyLowerBound();
    for (qsizetype i = m_history.size() - 1; i >= lowerBound; --i)
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

QString AiAssistant::buildSendingDiagnostic(qint64 payloadSize, qsizetype historyStart, qsizetype contextBoundary,
                                            const QStringList &docTopics, bool includeHistory) const
{
    // Rough estimate only (~4 bytes/token is a common heuristic for English/code text) — there's
    // no real tokenizer here. Purpose is to give visibility into request size *before* a "maximum
    // context length" HTTP error, not to be exact (the error itself already reports a precise
    // count, but only after rejecting the request).
    //
    // includeHistory==false makes windowedHistoryStart() return m_history.size() (i.e. historyStart
    // > 0, same as an actually-trimmed-for-budget case) unconditionally, regardless of how small the
    // real payload is — that's a deliberate per-call-type exclusion (self-review, split-plan
    // preflight, continuation-check, ...), not budget-driven trimming, and needs its own wording so
    // it doesn't misleadingly claim turns were "omitted to stay under budget" when none were ever
    // considered for inclusion in the first place.
    // Two independent exclusions reach this as one historyStart, and conflating them was
    // confirmed live to send a reader hunting a phantom budget problem: a new-task call printed
    // "0/3 (3 older omitted to stay under budget)" when the char budget was never consulted at
    // all — all 3 turns sat before m_contextHistoryStart. Attributed separately, and stacked when
    // both genuinely applied.
    const qsizetype boundaryExcluded = qBound(qsizetype(0), contextBoundary, historyStart);
    const qsizetype budgetExcluded = historyStart - boundaryExcluded;
    QStringList reasons;
    if (boundaryExcluded > 0)
        reasons << QString("%1 from an earlier task").arg(boundaryExcluded);
    if (budgetExcluded > 0)
        reasons << QString("%1 older omitted to stay under budget").arg(budgetExcluded);

    const QString historyPart =
        !includeHistory
            ? (m_history.isEmpty()
                   ? QString("history not sent for this call")
                   : QString("history not sent for this call (%1 turn(s) in conversation)").arg(m_history.size()))
        : historyStart > 0 ? QString("%1/%2 history turn(s) (%3)")
                                 .arg(m_history.size() - historyStart)
                                 .arg(m_history.size())
                                 .arg(reasons.join(", "))
                           : QString("%1 history turn(s)").arg(m_history.size());
    // kNoDocsSentinel gets special-cased rather than reusing docTopicsDisplayLabel() — that
    // helper's "empty means all" convention is right for the split-plan announcement it serves,
    // but wrong here: an empty docTopics reaching this diagnostic means "not applicable", so it
    // stays suppressed entirely, same as before this sentinel case was added.
    //
    // Integrated mode gets its own wording ("topics hint", never "docs") — buildSystemPrompt()'s
    // own short-circuit means NO doc content is EVER attached here regardless of what docTopics
    // holds; printing "docs: table" for a call that attaches nothing is the exact misleading
    // diagnostic the kNoDocsSentinel blanking (removed from handleSplitPlanResponse() when the
    // TOPICS: hint line was added to startSplitPart()'s prompt) used to exist to prevent.
    // Confirmed live: reintroduced exactly this way, real topic names now printing as "docs: ..."
    // for every self-review/retry round of an Integrated-mode split part, not just the one call
    // that actually carries the TOPICS: line.
    const bool integrated = docsMode() == DocsMode::Integrated;
    const QString docsLabel = integrated ? "topics hint" : "docs";
    const QString docsPart = docTopics.isEmpty() ? QString()
                             : docTopics == QStringList{kNoDocsSentinel}
                                 ? QString(", %1: none").arg(docsLabel)
                                 : QString(", %1: %2").arg(docsLabel, docTopics.join(", "));
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
                      const QStringList &docTopics, bool addToHistory, bool includeHistory, bool expectsCode,
                      const QString &historyText)
{
    if (m_busy)
    {
        emit errorOccurred("▸▸▸ AI: previous request still in progress.");
        return;
    }
    m_busy = true;
    m_lastAsk = {userText,     includeDocs,    systemPrompt, docTopics,
                 addToHistory, includeHistory, expectsCode,  historyText};

    // Safety net, not the primary mechanism: callers append their own hint line (topicsHintLine()/
    // noDocsHintLine()) because only they know the right scope. Four sites were found tonight that
    // forgot, and each silently got the server's unscoped whole-corpus fallback instead — one of
    // them 46,332 tokens for a text rewrite. Idempotent: skipped when a TOPICS: line is present.
    static const QRegularExpression topicsLinePresentRe(R"(^TOPICS:\s)", QRegularExpression::MultilineOption);
    const QString corrections = correctionBlock(m_userCorrections);
    const QString text = (topicsLinePresentRe.match(userText).hasMatch()
                              ? userText
                              : userText + (includeDocs ? topicsHintLine(docTopics) : noDocsHintLine())) +
                         corrections;

    // Auto-detect API format from URL: Anthropic Messages API vs OpenAI-compatible
    const bool isAnthropic = m_baseUrl.contains("anthropic.com", Qt::CaseInsensitive);

    const QString sysPrompt = buildSystemPrompt(systemPrompt, includeDocs, docTopics, expectsCode, text);

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false; // always disable streaming — we read the complete response at once
    // OpenAI's reasoning-model family (o1/o3/o4/gpt-5...) rejects "max_tokens" and requires
    // "max_completion_tokens" instead; Anthropic and OpenAI-compatible chat models keep "max_tokens".
    static const QRegularExpression reasoningModelRe(R"(^(o[0-9]|gpt-5))", QRegularExpression::CaseInsensitiveOption);
    if (!isAnthropic && reasoningModelRe.match(m_model).hasMatch())
        body["max_completion_tokens"] = m_maxTokens;
    else
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
        messages.append(QJsonObject{{"role", "user"}, {"content", text}});
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
        messages.append(QJsonValue(QJsonObject{{"role", "user"}, {"content", text}}));
        body["messages"] = messages;
    }

    // Store pending user text to save in history after response — historyText, when given,
    // overrides what gets saved (see ask()'s doc comment); userText above is unaffected, so the
    // model always sees the real, fully-constructed prompt regardless of this override.
    m_pendingUserText = (historyText.isEmpty() ? userText : historyText) + corrections;
    m_pendingAddToHistory = addToHistory;

    const QString endpoint = resolveEndpoint(m_baseUrl, isAnthropic);

    QNetworkRequest req{QUrl(endpoint)};
    // Loopback backends are latency-bound by local generation, not payload size — confirmed live: 60-150+s on a
    // non-trivial request (no cached docs, 60000 branch), past a cloud-sized budget. Give loopback one generous fixed
    // timeout, unrelated to the cached-docs heuristic. User-configurable rather than fixed: confirmed live, a compile
    // call under larger num_ctx hit "Operation canceled" (Qt's transfer-timeout error) at 300s, then again at 600s —
    // generation need trades directly against num_ctx/prompt size, so this can't be one fixed constant.
    const bool isLocalBackend = m_baseUrl.contains("127.0.0.1") || m_baseUrl.contains("localhost");
    // First request carries the full API docs in the system prompt — allow extra time. A set request timeout wins for
    // every backend: confirmed live, slower cloud models need more than 120 s for a full-docs prompt as well
    const int automaticMs = isLocalBackend ? 600000 : (m_cachedDocs.isEmpty() ? 60000 : 120000);
    req.setTransferTimeout(m_requestTimeoutSec > 0 ? m_requestTimeoutSec * 1000 : automaticMs);
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
    emit infoMessage(
        buildSendingDiagnostic(payload.size(), omittedTurns, historyLowerBound(), docTopics, includeHistory));
    // Stored for a possible transient-error retry (see onReplyFinished()'s emitHttpError) — a
    // fresh, user-initiated call always starts with a clean retry budget, even if a previous,
    // unrelated call had exhausted its own.
    m_lastRequest = req;
    m_lastPayload = payload;
    m_httpRetryCount = 0;
    m_requestTimer.start();
    m_currentReply = m_nam->post(req, payload);
}

bool AiAssistant::addUserCorrection(const QString &text, bool now)
{
    m_userCorrections << text;
    logCorrection(text, now);
    m_correctionOutcomePending = true;
    if (!now)
        return false;
    if (m_currentReply)
    {
        m_redoWithCorrection = true;
        m_currentReply->abort(); // finished() -> onReplyFinished() sends the call again
        return true;
    }
    if (m_retryTimer && m_retryTimer->isActive()) // waiting to resend after an HTTP error: resend now, corrected
    {
        m_retryTimer->stop();
        m_httpRetryCount = 0;
        m_busy = false;
        redoLastAsk();
        return true;
    }
    return false; // between calls: the next one carries it anyway
}

void AiAssistant::redoLastAsk()
{
    const AskArgs args = m_lastAsk;
    ask(args.userText, args.includeDocs, args.systemPrompt, args.docTopics, args.addToHistory, args.includeHistory,
        args.expectsCode, args.historyText);
}

QString AiAssistant::correctionBlock(const QStringList &corrections)
{
    if (corrections.isEmpty())
        return {};
    return "\n\nCorrection from the user while this request runs — follow it; where it conflicts with the request "
           "above, the correction wins:\n- " +
           corrections.join("\n- ");
}

QString AiAssistant::parseCorrection(const QString &text, bool *now)
{
    QString correction = text.trimmed();
    const bool urgent = correction.startsWith('!');
    if (urgent)
        correction = correction.mid(1).trimmed();
    if (now)
        *now = urgent;
    return correction;
}

bool AiAssistant::isIdle() const
{
    return !m_busy && !listModeActive && !(m_retryTimer && m_retryTimer->isActive());
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
    // Confirmed live: without this, the next request — even an obviously unrelated one typed
    // right after Stop — still relied on the continuation-check's own model judgment to decide
    // whether to advance this boundary, and that guess can be wrong, leaving the cancelled
    // request's text as the retrieval anchor for everything after it.
    m_contextHistoryStart = m_history.size();
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
    if (m_redoWithCorrection) // aborted by a "!" correction: the same call again, now carrying it
    {
        m_redoWithCorrection = false;
        m_pendingUserText.clear();
        redoLastAsk();
        return;
    }

    // Single point for all HTTP/network error reporting — used by both the
    // Qt-error path (network failure) and the post-streaming HTTP-code path.
    constexpr int kMaxHttpRetries = 5;
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
            // 1.5x backoff per attempt: 2s, 3s, 4.5s, 6.75s, 10.125s.
            double delaySec = 2.0;
            for (int i = 1; i < m_httpRetryCount; ++i)
                delaySec *= 1.5;
            int delayMs = static_cast<int>(delaySec * 1000);
            if (code == 429)
            {
                bool ok = false;
                const int raSecs = reply->rawHeader("Retry-After").trimmed().toInt(&ok);
                if (ok && raSecs > 0)
                    delayMs = raSecs * 1000;
            }
            // Cap the wait regardless of source — some providers (confirmed live: Cerebras) send an
            // implausibly large Retry-After (observed: ~1084234s, ~12.5 days) that would otherwise
            // stall the app for days instead of failing fast; 2 minutes is well past any legitimate
            // short-term rate-limit backoff.
            constexpr int kMaxRetryDelayMs = 120 * 1000;
            delayMs = qMin(delayMs, kMaxRetryDelayMs);
            emit httpRetryStatus(QString("AI: HTTP %1 — retrying in %2s (attempt %3/%4)...")
                                     .arg(code)
                                     .arg(delayMs / 1000.0, 0, 'f', 1)
                                     .arg(m_httpRetryCount)
                                     .arg(kMaxHttpRetries),
                                 m_httpRetryCount == 1);
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
        else if (code > 0)
        {
            const QString detail =
                body.left(300).trimmed().isEmpty() ? reply->errorString() : QString::fromUtf8(body.left(300).trimmed());
            emit errorOccurred(QString("AI HTTP %1: %2").arg(code).arg(detail) + splitModeGiveUpNote());
        }
        else
        {
            // code == 0 means no HTTP status was ever received — a connection-level failure (TCP reset, backend
            // crash) before any response. Name the actual NetworkError enum value too, and flag a local backend
            // as the likely process-crash suspect.
            const QString errName =
                QString::fromLatin1(QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error()));
            const bool isLocalBackend = m_baseUrl.contains("127.0.0.1") || m_baseUrl.contains("localhost");
            QString msg = QString("AI: connection error (%1): %2").arg(errName, reply->errorString());
            if (isLocalBackend)
                msg += " — the local backend process may have crashed or closed the connection; check its own logs.";
            emit errorOccurred(msg + splitModeGiveUpNote());
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
        // Confirmed live: m_history used to store `response` (raw, markdown fences intact) while
        // this signal emitted cleanResponse(response) — the first display/run always used the
        // clean copy and worked, but runLastScript()/runHistoryScript() read straight from
        // m_history with `compilable` hardcoded true (skipping the canCompile() gate that would
        // otherwise have caught this), so "run it"/"run it again" fed the raw fenced text
        // straight to the interpreter and failed with "SyntaxError: invalid syntax at <ai>:1" —
        // line 1 being the literal "```python" fence. Cleaning once and storing/emitting the same
        // string keeps every later reader of m_history[...].second consistent with what was
        // actually shown to the user.
        const QString cleaned = cleanResponse(response);
        if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
            m_history.append({m_pendingUserText, cleaned});
        m_pendingUserText.clear();
        emit responseReady(cleaned);
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

    // Confirmed live (benchmark item 12, part 3): the self-review call came back empty and ended the request, although
    // the reviewed draft was complete. An empty review changes nothing — the draft runs, as for a prose-only review.
    if (text.isEmpty() && awaitingSelfReview && !m_selfReviewDraftCode.isEmpty())
    {
        m_pendingUserText.clear();
        emit responseReady(QString());
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
        // See the identical fix in the streamed-response branch above for why this stores the
        // cleaned string, not the raw one.
        const QString cleaned = cleanResponse(response);
        if (!m_pendingUserText.isEmpty() && m_pendingAddToHistory)
            m_history.append({m_pendingUserText, cleaned});
        m_pendingUserText.clear();
        emit responseReady(cleaned);
    }
}
