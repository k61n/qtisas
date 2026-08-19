/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiSAS's own implementation of AiContentProvider — every string of
             QtiSAS-specific AI domain knowledge (the self-review checklist,
             doc-topic descriptions, and *-python-api doc loading) lives here,
             not in AiAssistant.cpp, so that class can stay free of any
             knowledge specific to this application (see AiContentProvider.h).
 ******************************************************************************/

#ifndef QTISASAICONTENT_H
#define QTISASAICONTENT_H

#include "AiContentProvider.h"

class QtiSasAiContent : public AiContentProvider
{
  public:
    QString selfReviewChecklist() const override;
    QString docTopicIndex() const override;
    QStringList knownDocTopics() const override;
    QStringList baselineDocTopics() const override;
    QString loadDocs(bool unlimited, QString *statusOut) const override;
    QString splitPlanPreflightPrompt(const QString &requestText) const override;
    QString docSelectionPreflightPrompt(const QString &requestText) const override;
    QString continuationCheckPreflightPrompt(const QString &newRequestText,
                                             const QString &priorRequestText) const override;
    QString requestSuffix(bool autoRun) const override;
    QString apiCallingConventionHint() const override;
    QString assistantIdentityIntro() const override;
    QString existenceVerificationFunctionExamples() const override;
    QString noOpPrintFunctionName() const override;
    QString creationCallExamples() const override;
    QString lookupByNameExamples() const override;
    QString splitPartScopeViolation(const QString &code, const QString &partDescription,
                                    const QString &earlierPartsCode) const override;
    bool isGiveUpFallbackScript(const QString &script) const override;
    QString retryHintForError(const QString &errContext) const override;
    bool isUnverifiedMissingClaim(const QString &script) const override;
    QList<AiKnownMistakeMatch> detectKnownMistakes(const QString &code,
                                                   const QString &originalRequestText) const override;
    QString selfReviewFunctionNameSubstitution(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString selfReviewFalseIncompletenessClaim(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString existingEntityVerificationPrompt(const QStringList &names) const override;
    QString selfReviewLookupToCreationRegression(const QString &preReviewDraft,
                                                 const QString &postReviewCode) const override;
    QString selfReviewEnumConstantRegression(const QString &preReviewDraft,
                                             const QString &postReviewCode) const override;

  private:
    // The individual pattern checks dispatched by detectKnownMistakes(), in the SAME priority
    // order that method calls them in — kept as private helpers (rather than folded inline into
    // detectKnownMistakes() itself) so each one keeps its own focused, independently-readable
    // implementation and doc comment (see QtiSasAiContent.cpp). Not part of AiContentProvider's
    // public interface: no other caller needs — or should need — a single pattern in isolation.
    // Exposed to AiContentTests only via friendship, so the existing per-pattern unit tests keep
    // calling these directly instead of going through detectKnownMistakes() and risking an earlier,
    // higher-priority pattern in a test's own code snippet masking the one actually under test.
    friend class AiContentTests;
    static QString compiledCodeSelfShadowBug(const QString &code);
    static QString fitCurveNameReconstructionMismatch(const QString &code);
    static QString unrequestedAutoexecImmediateRun(const QString &code, const QString &originalRequestText);
    static QString zeroBasedTableIndexMistake(const QString &code);
    static QString staleFitStatisticMistake(const QString &code);
    static QString missingLegendCreationMistake(const QString &code);
    static QString unannouncedModelNameSubstitution(const QString &code, const QString &originalRequestText);
    static QString hallucinatedLegendMethodMistake(const QString &code);
    static QString graphWindowLayerMethodMistake(const QString &code);
    static QString conditionalMaximizeWindowMistake(const QString &code);
    static QString windowNameMethodCallMistake(const QString &code);
    static QString nonexistentExistMatrixMistake(const QString &code);
    static QString qti3DImportMistake(const QString &code);
    static QString setAutoexecOnScriptEditMistake(const QString &code);
    static QString plotStyleStringArgumentMistake(const QString &code);
    static QString guessedNameCandidateListMistake(const QString &code);
};

#endif
