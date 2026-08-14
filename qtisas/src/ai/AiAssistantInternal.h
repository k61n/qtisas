/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Declares a handful of AiAssistant.cpp's internal free functions
             with external linkage, for exactly one reason: so
             src/ai/tests/ai_content_tests.cpp can link against and unit-test
             them directly. Not part of the public AiAssistant/
             AiContentProvider surface — nothing outside AiAssistant.cpp and
             the test binary should include this header.
 ******************************************************************************/

#ifndef AIASSISTANTINTERNAL_H
#define AIASSISTANTINTERNAL_H

#include "AiAssistant.h"

#include <QString>
#include <QStringList>

// Splits docs already tagged with "\n\n=== filename ===\n" section headers, keeping only files
// whose name contains one of `topics` (case-insensitive substring). See the definition in
// AiAssistant.cpp for the full "lost in the middle" ordering rationale.
QString filterDocsByTopic(const QString &allDocs, const QStringList &topics);

// Parses one split-plan part's raw "DOCS: ..." field into a topic list, unioning in any topic
// keyword found in the part's own description text (including a "fittable" synonym regex — see
// definition). See AiAssistant.cpp for the full matching semantics.
QStringList splitPartTopicsFromAnswer(const QString &docsField, const QString &description,
                                      const QStringList &knownTopics);

// Returns the indices of `plan` entries whose description matches a creation verb but never
// states the created object's name in quotes. See AiAssistant.cpp for the full rationale.
QList<int> splitPartsMissingCreationName(const QList<AiAssistant::SplitPart> &plan);

// True if `code`, after stripping comments and blank lines, has no remaining lines at all — the
// script does absolutely nothing. See AiAssistant.cpp for the full rationale.
bool isEmptyActionScript(const QString &code);

// Returns the "# --- Part N: ... ---" numbers found in a merged script whose N exceeds
// `actualPartCount` — a mechanical fingerprint of the merge fabricating parts that never existed
// in the plan and never ran. See AiAssistant.cpp for the full rationale.
QList<int> inventedMergePartNumbers(const QString &code, int actualPartCount);

// Returns the module names of `code`'s import statements that never appear anywhere in
// `partScriptsJoined` — a mechanical fingerprint of the merge fabricating an import beyond what
// the literal part scripts actually used. See AiAssistant.cpp for the full rationale.
QStringList inventedMergeImports(const QString &code, const QString &partScriptsJoined);

// Extracts every quoted name in `requestText` explicitly described as already EXISTING (the word
// "existing" near a quoted string). See AiAssistant.cpp for the full rationale.
QStringList extractExistingEntityNames(const QString &requestText);

// Returns the subset of `checkedNames` for which `scriptOutput` contains a "MISSING: <name>"
// line. See AiAssistant.cpp for the full rationale.
QStringList missingEntityNames(const QString &scriptOutput, const QStringList &checkedNames);

#endif
