/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Declares a handful of AiAssistant.cpp's internal free functions
             with external linkage, for exactly one reason: so
             tests/src/ai/ai_content_tests.cpp can link against and unit-test
             them directly. Not part of the public AiAssistant/
             AiContentProvider surface — nothing outside AiAssistant.cpp and
             the test binary should include this header.
 ******************************************************************************/

#ifndef AIASSISTANTINTERNAL_H
#define AIASSISTANTINTERNAL_H

#include "AiAssistant.h"

#include <QPair>
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

// Rewrites every part's bare, quoted reference to a compiled function so it matches the compile
// part's own canonical ("ai/"-prefixed) name — a plan can individually satisfy
// splitPartsMissingCreationName() in every part while still naming the SAME function two
// different ways across parts. See AiAssistant.cpp for the full rationale.
void reconcileCompiledFunctionNameAcrossParts(QList<AiAssistant::SplitPart> &plan);

// Removes every `plan` entry whose description is byte-identical to an earlier entry's, unioning
// the removed entry's DOCS topics into the one kept. `removedDescriptions`, if non-null, collects
// the description of each part removed this way. See AiAssistant.cpp for the full rationale.
void dedupeSplitPlanExactDuplicateParts(QList<AiAssistant::SplitPart> &plan, QStringList *removedDescriptions);

// Appends a concrete quoted name to every creation part that never stated one, mechanically —
// `assigned`, if non-null, collects the names given. See AiAssistant.cpp for the confirmed-live
// deadlock (an unnamed part can satisfy neither the naming check nor Direction 5) this closes.
void assignSplitPlanCreationNames(QList<AiAssistant::SplitPart> &plan, QStringList *assigned);

// Folds every part splitPlanUnnecessaryFragments() flags into its predecessor (descriptions joined,
// DOCS topics unioned, fragment removed); `mergedAway`, if non-null, collects each removed
// description. See AiAssistant.cpp for the confirmed-live non-convergence this replaces.
void mergeSplitPlanUnnecessaryFragments(QList<AiAssistant::SplitPart> &plan, QStringList *mergedAway);
// Folds each part in `parts` (ascending indices) into the part before it, the same way.
void mergeSplitPlanPartsBackward(QList<AiAssistant::SplitPart> &plan, const QList<int> &parts, QStringList *mergedAway);

// Generates a name that differs from `oldName` and doesn't already appear in `plan`'s descriptions
// (appends "_2", "_3", ...). See AiAssistant.cpp for the full rationale.
QString mechanicalAlternateSplitPlanName(const QString &oldName, const QList<AiAssistant::SplitPart> &plan);

// Rewrites every occurrence of each name in `collidingNames` across every `plan` entry's
// description with a mechanically-generated alternate, collecting "<old> -> <new>" into `renames`
// (if non-null). See AiAssistant.cpp for the full rationale.
void mechanicallyRenameSplitPlanCollisions(QList<AiAssistant::SplitPart> &plan, const QStringList &collidingNames,
                                           QStringList *renames);

// Returns a label for each distinct action group (compile/fit/compare/plot) whose keyword(s)
// appear in `originalRequestText` but no synonym from that same group appears anywhere across
// `plan`'s combined part descriptions. See AiAssistant.cpp for the full rationale.
QStringList splitPlanMissingRequestedActions(const QString &originalRequestText,
                                             const QList<AiAssistant::SplitPart> &plan);

// Returns the indices of `plan` entries whose action (compile/fit/compare/plot) is NEVER
// mentioned anywhere in `originalRequestText` — the mirror-image, over-coverage counterpart to
// splitPlanMissingRequestedActions() above. See AiAssistant.cpp for the full rationale.
QList<int> splitPlanUnrequestedActions(const QString &originalRequestText, const QList<AiAssistant::SplitPart> &plan);

// Returns the indices of `plan` entries that only fill/configure/finish what an earlier part (not
// necessarily the immediately preceding one — see the forward-chaining note in AiAssistant.cpp)
// already created, with no other distinct action of their own. See AiAssistant.cpp for the full
// rationale.
QList<int> splitPlanUnnecessaryFragments(const QList<AiAssistant::SplitPart> &plan);

// Returns the indices of `plan` entries whose resolved DOCS topics are identical to the
// immediately preceding part's — the mechanical backstop for rule 6 ("never leave two parts with
// an identical DOCS field"). See AiAssistant.cpp for the full rationale.
QList<int> splitPlanAdjacentSameDocs(const QList<AiAssistant::SplitPart> &plan);

// Returns the indices of `plan` entries that create a table/matrix a later part only plots, when
// `originalRequestText` already gives the data as an analytical formula (sin/cos/tan/exp/log/sqrt
// of x or y) — the mechanical backstop for rule 4. See AiAssistant.cpp for the full rationale.
QList<int> splitPlanUnnecessaryDataTable(const QString &originalRequestText, const QList<AiAssistant::SplitPart> &plan);

// Returns everything after the LAST "---FINAL---"-shaped marker in `code`, or `code` unchanged if
// no such marker is present. See AiAssistant.cpp for the full rationale (the draft/check/final
// two-stage split-plan answer format).
QString splitPlanFinalSection(const QString &code);

// Rewrites a response wrapped in scriptPrint(...)/print(...) calls (one call per line, or a
// single call with escaped "\n" sequences) back into plain multi-line text, so a model that
// insists on "answering with code" doesn't defeat split-plan parsing. Returns `code` unchanged if
// no such wrapping is found. See AiAssistant.cpp for the full rationale.
QString unwrapScriptPrintCalls(const QString &code);

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

// Extracts every quoted name in `requestText` explicitly described as already EXISTING, paired
// with the KNOWN kind (table/matrix/note/graph) when the surrounding text names one, or an empty
// string when genuinely unknown. See AiAssistant.cpp for the full rationale.
QList<QPair<QString, QString>> extractExistingEntityNames(const QString &requestText);
// Every quoted name the request does not CREATE (a creation verb in its clause, or "create it if missing") — the
// names it treats as inputs, whatever its wording. See AiAssistant::existingInputNamesNote().
QStringList inputNamesInRequest(const QString &requestText);
QString withoutRequestSuffix(const QString &text);

// Returns the subset of `checkedNames` (name, knownKind-or-empty pairs) found missing — trusting
// only the relevant labeled line when the kind is known, falling back to the VERDICT+safety-net
// logic when it isn't. See AiAssistant.cpp for the full rationale.
QStringList missingEntityNames(const QString &scriptOutput, const QList<QPair<QString, QString>> &checkedNames);

// Collects the quoted name of every `plan` entry that matches a creation verb (compile/create/
// make/build/plot), paired with which labeled existence-check line(s) — "compile"/"fittable" for a
// function, "table", "matrix", "graph" — a collision on that name would need to match to actually
// be destructive for that kind of creation (a "note" match yields no entry at all: reusing an
// existing note is never destructive). See AiAssistant.cpp for the full rationale.
QList<QPair<QString, QStringList>> splitPlanCreationNames(const QList<AiAssistant::SplitPart> &plan);

// The mirror-image check, for names a plan is about to CREATE rather than ones it claims already
// exist: returns the subset of `checkedNames` (from splitPlanCreationNames() above) for which
// `scriptOutput` contains a labeled "(<label>) EXISTS: <name>" line for one of that entry's OWN
// relevant labels — never the combined VERDICT line, and never a label irrelevant to how that name
// is being created. See AiAssistant.cpp for the full rationale.
QStringList splitPlanCreationCollisions(const QString &scriptOutput,
                                        const QList<QPair<QString, QStringList>> &checkedNames);

// compile-python-api.md Rule 6: "ai/" is reserved, disposable scratch — true for a compile/fittable
// name under "ai/", whose collision is always safe to overwrite. See AiAssistant.cpp for the full
// rationale.
bool isRule6SandboxExemptCreationName(const QString &name, const QStringList &labels);

// Removes any collision covered by isRule6SandboxExemptCreationName() from `collisions`, appending
// each removed name to `sandboxExempt` (if non-null). See AiAssistant.cpp for the full rationale.
QStringList splitPlanCollisionsAfterSandboxExemption(const QStringList &collisions,
                                                     const QList<QPair<QString, QStringList>> &checkedNames,
                                                     QStringList *sandboxExempt);

// Truncates decoding-level repetition-loop garbage, keeping exactly one pass of the cycling
// pattern (the staged DRAFT/---CHECK---/---FINAL--- protocol keeps the older, looser cut instead).
// Returns `s` unchanged when nothing repeats often enough, or when no salvageable prefix exists.
// See AiAssistant.cpp for the full rationale.
QString truncateRepetitiveGarbage(const QString &s);

// Skips the verification round-trip for any name covered by isRule6SandboxExemptCreationName() —
// its answer can never change the outcome. Returns `checkedNames` with those entries removed,
// appending each to `skipped` (if non-null). See AiAssistant.cpp for the full rationale.
QList<QPair<QString, QStringList>> splitPlanCreationNamesNeedingCheck(
    const QList<QPair<QString, QStringList>> &checkedNames, QStringList *skipped);

#endif
