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
    settings->endGroup();
}

#endif
