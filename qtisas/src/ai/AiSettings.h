/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plain, persistable AI configuration knobs — extracted out of
             ApplicationWindow so that class doesn't directly hold AI-specific
             settings state. Held as a single member on ApplicationWindow.
 ******************************************************************************/

#ifndef AISETTINGS_H
#define AISETTINGS_H

#include <QSettings>

struct AiSettings
{
    // Caps addRule() — independent of the AI preset/model selected in AiConfig.
    int maxUserRules = 20;

    // Shared cap on automatic EXPLORE-chaining and error-retry rounds per request.
    int maxAutoContinue = 4;

    // When true, every ApplicationWindow::askAi() request is split-mode by default (see
    // AiAssistant.h's splitModeActive doc comment) — a leading "SPLIT:" marker is then redundant,
    // and a leading "NOSPLIT:" marker forces that one request to skip split mode instead. When
    // false, split mode stays fully opt-in via "SPLIT:".
    bool splitByDefault = true;

    // Caps how many parts a single split-mode decomposition is allowed to produce.
    int maxSplitParts = 8;

    // When true (default), split mode ends by requesting a final, NEVER-EXECUTED merged reference
    // script collapsing all parts into one (see AiAssistant::requestSplitMerge()'s doc comment).
    // When false, split mode skips that step entirely — each part already ran independently, so
    // this only affects whether a standalone reference script gets printed at the end.
    bool splitRequestMerge = true;

    // When true (default), every candidate action script is sent back to the AI once for a
    // mandatory pre-execution self-review pass (checklist + the 9 mechanical regression backstops)
    // before it ever runs — see AiAssistant::maybeDispatchSelfReview()'s doc comment. When false,
    // that whole round-trip is skipped and a compilable candidate goes straight to
    // runActionScript() — trading the checklist/backstop safety net for one fewer HTTP round-trip
    // per script. detectKnownMistakes() and the other pre-execution mechanical checks in
    // runActionScript() itself are unaffected either way; only the SEPARATE review-and-correct pass
    // is what this toggles.
    bool selfReviewEnabled = true;

    // When true (default), the two-phase agentic EXPLORE loop is available: the model may answer
    // with a "# EXPLORE" read-only probe script, QtiSAS runs it and feeds the real output back
    // before asking for the action script — see AiAssistant::handleExploreResponse(). When false,
    // exploreModeReminder() is dropped from the system prompt, AND a "# EXPLORE" script that
    // arrives anyway is answered with a single "EXPLORE is disabled" re-prompt instead of being
    // run: the API docs carry their own EXPLORE rules, so the model can still ask for a round on
    // the corpus's authority alone and dropping the reminder cannot prevent that by itself.
    // Deliberately does NOT touch the existence-check or split-plan collision-check round-trips —
    // those also send read-only scripts, but they're deterministic machinery QtiSAS drives itself,
    // not model-initiated exploration, and disabling them would silently remove collision guards.
    bool exploreEnabled = true;

    // HTTP timeout of one AI call, for every backend; 0 = automatic: 600 s for a loopback base URL, 120 s for a cloud
    // one (60 s before the docs are cached). Confirmed live: slower BlaBlaDor models (Kimi, GLM, DeepSeek on JUWELS)
    // need more than 120 s for a ~55K-token docs prompt and failed every item with "Operation timed out".
    int requestTimeoutSec = 0;
};

// Keeps the "/AI" group name and its key strings colocated with the struct they belong to,
// instead of duplicated as literals in ApplicationWindow::readSettings()/saveSettings(). Existing
// values in `s` are used as the fallback default for a not-yet-present key, same as every other
// setting in this app's config file.
inline void AiReadSettings(QSettings *settings, AiSettings &s)
{
    settings->beginGroup("/AI");
    s.maxUserRules = settings->value("/MaxUserRules", s.maxUserRules).toInt();
    s.maxAutoContinue = settings->value("/MaxAutoContinue", s.maxAutoContinue).toInt();
    s.splitByDefault = settings->value("/SplitByDefault", s.splitByDefault).toBool();
    s.maxSplitParts = settings->value("/MaxSplitParts", s.maxSplitParts).toInt();
    s.splitRequestMerge = settings->value("/SplitRequestMerge", s.splitRequestMerge).toBool();
    s.selfReviewEnabled = settings->value("/SelfReviewEnabled", s.selfReviewEnabled).toBool();
    s.exploreEnabled = settings->value("/ExploreEnabled", s.exploreEnabled).toBool();
    // the former loopback-only "/LocalTimeoutSec": its old default 600 means automatic, any other value is kept
    const int localTimeoutSec = settings->value("/LocalTimeoutSec", 600).toInt();
    s.requestTimeoutSec = settings->value("/RequestTimeoutSec", localTimeoutSec == 600 ? 0 : localTimeoutSec).toInt();
    settings->endGroup();
}

inline void AiWriteSettings(QSettings *settings, const AiSettings &s)
{
    settings->beginGroup("/AI");
    settings->setValue("/MaxUserRules", s.maxUserRules);
    settings->setValue("/MaxAutoContinue", s.maxAutoContinue);
    settings->setValue("/SplitByDefault", s.splitByDefault);
    settings->setValue("/MaxSplitParts", s.maxSplitParts);
    settings->setValue("/SplitRequestMerge", s.splitRequestMerge);
    settings->setValue("/SelfReviewEnabled", s.selfReviewEnabled);
    settings->setValue("/ExploreEnabled", s.exploreEnabled);
    settings->setValue("/RequestTimeoutSec", s.requestTimeoutSec);
    settings->remove("/LocalTimeoutSec");
    settings->endGroup();
}

#endif
