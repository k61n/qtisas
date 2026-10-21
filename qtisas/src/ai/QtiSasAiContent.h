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

#include <QHash>

class QtiSasAiContent : public AiContentProvider
{
  public:
    QString selfReviewChecklist(const QStringList &docTopics, bool splitModeActive) const override;
    QString docTopicIndex() const override;
    QStringList knownDocTopics() const override;
    QStringList baselineDocTopics() const override;
    QStringList onDemandDocTopics() const override;
    QString loadDocs(bool unlimited, QString *statusOut) const override;
    QString splitPlanPreflightPrompt(const QString &requestText, bool docsIntegrated) const override;
    QString continuationCheckPreflightPrompt(const QString &newRequestText) const override;
    QString requestSuffix(bool autoRun) const override;
    QString apiCallingConventionHint() const override;
    QString assistantIdentityIntro() const override;
    QString existenceVerificationFunctionExamples() const override;
    QString noOpPrintFunctionName() const override;
    QString exploreModeReminder() const override;
    QString exploreUnavailableNote() const override;
    QString liveEntityFactsNote(const QList<QPair<QString, QStringList>> &facts) const override;
    QString missingRequestedDeliverables(const QString &code, const QString &requestText) const override;
    QString creationCallExamples() const override;
    QString lookupByNameExamples() const override;
    QString splitPartScopeViolation(const QString &code, const QString &partDescription,
                                    const QString &earlierPartsCode, bool wholeRequest = false) const override;
    bool isGiveUpFallbackScript(const QString &script) const override;
    QString retryHintForError(const QString &errContext) const override;
    bool isUnverifiedMissingClaim(const QString &script) const override;
    QList<AiKnownMistakeMatch> detectKnownMistakes(const QString &code, const QString &originalRequestText,
                                                   const QString &earlierPartsCode,
                                                   const QString &laterPartsDescriptions = QString(),
                                                   bool exploreScript = false) const override;
    QString explorePrefixBeforeFirstWriteCall(const QString &code) const override;
    QString selfReviewFunctionNameSubstitution(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString selfReviewFalseIncompletenessClaim(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString existingEntityVerificationPrompt(const QList<QPair<QString, QStringList>> &names,
                                             bool includeDataProbe) const override;
    QString selfReviewLookupToCreationRegression(const QString &preReviewDraft,
                                                 const QString &postReviewCode) const override;
    QString selfReviewEnumConstantRegression(const QString &preReviewDraft,
                                             const QString &postReviewCode) const override;
    QString selfReviewCompileSequenceRegression(const QString &preReviewDraft,
                                                const QString &postReviewCode) const override;
    QString selfReviewPyQtVersionSubstitution(const QString &preReviewDraft,
                                              const QString &postReviewCode) const override;
    QString selfReviewFabricatedSetCodeMistake(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString selfReviewCreationNameSubstitution(const QString &preReviewDraft,
                                               const QString &postReviewCode) const override;
    QString exploreGroupsIgnoredMistake(const QString &code, const QString &exploreOutput) const override;
    QString compilerParamCountRepair(const QString &code) const override;
    QString compilerYNameAssignmentRepair(const QString &code) const override;
    QString compilerSelfShadowRepair(const QString &code) const override;
    QString compilerParamIndexRepair(const QString &code) const override;
    QStringList compiledCodeBodies(const QString &code) const override;
    QString compilerParamVariesRepair(const QString &code, const QString &request) const override;
    QString fabricatedConversationCheck(const QString &code) const override;
    static QString repairedSetCodeBody(const QString &body, const QString &yName);
    static QString renamedResultAssignment(const QString &body, const QString &yName, const QStringList &reservedNames);
    QString resultLabel(const QString &requestText) const override;
    QString resultLineDuty(const QString &requestText) const override;
    QStringList resultLineValueList(const QString &requestText) const override
    {
        return requestedResultValues(requestText);
    }
    QString nonFinalResultLineRepair(const QString &code, const QString &requestText, bool alsoComputed) const override;
    QString repeatedResultLabelRepair(const QString &code, const QString &requestText) const override;
    QString labelOnlyResultLineRepair(const QString &code, const QString &requestText) const override;
    QString bareFolderNameRepair(const QString &code, const QString &requestText) const override;
    QString setCodeVariableRepair(const QString &code) const override;
    QString matrixTransposedCellRepair(const QString &code) const override;
    QString tableTransposedCellRepair(const QString &code) const override;
    QString muParserPowerRepair(const QString &code) const override;
    static QString existenceReadBeforeCreationMistake(const QString &code);
    static QString bareObjectMethodMistake(const QString &code);
    static QString requestDataNumberMistake(const QString &code, const QString &originalRequestText);
    QString bareCompilerDeleteRepair(const QString &code) const override;
    QString noteAutoexecReceiverRepair(const QString &code) const override;
    QString docTopicForKind(const QString &kind) const override;
    QString apiCallRepairs(const QString &code, const QString &requestText, bool resultLine,
                           QStringList *notes) const override;
    static QString recalculateExceptionNameMistake(const QString &code);
    QString tableFormulaCaretRepair(const QString &code, const QString &formulaLanguage) const override;
    QString compilerMissingGroupRepair(const QString &code) const override;
    static QString otherTabRunMistake(const QString &code, const QString &originalRequestText);
    static QString labeledResultLineMistake(const QString &code, const QString &originalRequestText);
    static QString staleResultValueMistake(const QString &code, const QString &originalRequestText);
    static QString inventedDeletionCallHint(const QString &errContext);
    static QString matrixIndexFormulaMistake(const QString &code, const QString &originalRequestText);
    static QString newNoteExtraTabMistake(const QString &code, const QString &originalRequestText);
    static QString earlierPartVariableMistake(const QString &code, const QString &earlierPartsCode);
    QString noteTabIndexRepair(const QString &code) const override;
    QString noteTabSelfRunRepair(const QString &code) const override;
    static QString scriptPrintKeywordMistake(const QString &code);
    static QString currentEditorAfterAddTabMistake(const QString &code, const QString &originalRequestText);
    // Python string literals blanked out, so a name inside one is not mistaken for a variable
    // reference — setParamNamesAll(["sigma", ...]) and setCode()'s C++ body are other namespaces.
    static QString codeWithoutStringLiterals(const QString &code);
    // The same, but each literal replaced by spaces of its own length (newlines kept): offsets match `code`.
    static QString codeWithLiteralsBlanked(const QString &code);
    static QString fabricatedConversationMistake(const QString &code);

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
    static QString compilerRedeclaredReservedNameMistake(const QString &code);
    static QString compilerYNameNeverAssignedMistake(const QString &code);
    static QString discoveredNameInsteadOfRequestedMistake(const QString &code, const QString &originalRequestText);
    static QString fitCurveNameReconstructionMismatch(const QString &code);
    static QString unrequestedAutoexecImmediateRun(const QString &code, const QString &originalRequestText);
    static QString zeroBasedTableIndexMistake(const QString &code);
    static QString changeFolderNoOpMistake(const QString &code);
    static QString tableCellRowColSwapMistake(const QString &code);
    static QString compilerSetCodeNestedFunctionMistake(const QString &code);
    static QString compilerFittableOnlyMethodMistake(const QString &code);
    static QString setParamNamesMissingAllMistake(const QString &code);
    static QString compilerNonexistentMethodMistake(const QString &code);
    static QString compilerSetCodeCppSyntaxMistake(const QString &code);
    static QString compilerMissingAiNamespaceMistake(const QString &code);
    static QString hallucinatedPlotDesignationCaseMistake(const QString &code);
    static QString staleFitStatisticMistake(const QString &code);
    static QString missingLegendCreationMistake(const QString &code);
    static QString unannouncedModelNameSubstitution(const QString &code, const QString &originalRequestText);
    static QString reuseInsteadOfCompileMistake(const QString &code, const QString &originalRequestText,
                                                const QString &earlierPartsCode);
    static QString columnFormulaInLoopMistake(const QString &code);
    static QString guardedCompileMistake(const QString &code, const QString &originalRequestText);
    static QString aiSandboxReopenMistake(const QString &code, const QString &originalRequestText,
                                          const QString &earlierPartsCode);
    static QString requestedCreationNeverHappenedMistake(const QString &code, const QString &originalRequestText,
                                                         const QString &earlierPartsCode,
                                                         const QString &laterPartsDescriptions);
    static QString hardcodedWinnerWithoutLiveComparisonMistake(const QString &code, const QString &originalRequestText,
                                                               const QString &earlierPartsCode);
    static QString hallucinatedLegendMethodMistake(const QString &code);
    static QString graphWindowLayerMethodMistake(const QString &code);
    static QString conditionalMaximizeWindowMistake(const QString &code);
    static QString curveExistenceGuardTitleMismatch(const QString &code);
    static QString windowNameMethodCallMistake(const QString &code);
    static QString creationChainedMethodCallMistake(const QString &code);
    static QString qti3DImportMistake(const QString &code);
    static QString setAutoexecOnScriptEditMistake(const QString &code);
    static QString plotStyleStringArgumentMistake(const QString &code);
    static QString matrixViewTypeIntMistake(const QString &code);
    static QString bareColumnNameInColumnsListMistake(const QString &code);
    static QString missingPyQtImportMistake(const QString &code);
    static QString missingRandomImportMistake(const QString &code);
    static QString unrequestedRandomDataMistake(const QString &code, const QString &originalRequestText);
    static QString unrequestedDanSessionMistake(const QString &code, const QString &originalRequestText);
    static QString columnListInsteadOfAggregateMistake(const QString &code, const QString &originalRequestText);
    static QString requestedResultLabel(const QString &originalRequestText);
    static QString resultLineDutyOf(const QString &requestText);
    static QStringList requestedResultValues(const QString &requestText);
    static QString resultLineTooFewValuesMistake(const QString &code, const QString &originalRequestText);
    static QString rowRangeEndsOneShortMistake(const QString &code);
    static QString hardcodedResultLineMistake(const QString &code, const QString &originalRequestText);
    static QString placeholderResultValueMistake(const QString &code, const QString &originalRequestText);
    static QString noWorkFailureResultMistake(const QString &code, const QString &originalRequestText);
    static QString resultsAfterConfigureMistake(const QString &code);
    QString unusedParameterMistake(const QString &code) const;
    static QString repeatedResultLabelMistake(const QString &code, const QString &originalRequestText);
    static QString pythonInputCallMistake(const QString &code);
    static QString guessedNameCandidateListMistake(const QString &code);
    static QString fragmentedStaticScriptPrintMistake(const QString &code);
    static QString doubleEscapedRawRegexMistake(const QString &code);
    static QString singletonCalledAsConstructorMistake(const QString &code);
    static QString pyQt6FrameStyleComboMistake(const QString &code);
    static QString fittableParametersDirectiveIgnoredMistake(const QString &code, const QString &originalRequestText);
    static QString redundantFittableReconfigureMistake(const QString &code, const QString &earlierPartsCode);
    static QString compilerScanGroupsAfterNameMistake(const QString &code);
    static QString fittableFitBeforeConfigureMistake(const QString &code);
    static QString scriptEditPrintDialogMistake(const QString &code);
    static QString duplicateWindowNameMistake(const QString &code);
    static QString textIntoNumericColumnMistake(const QString &code);
    static QString matrixIntegrateAsSumMistake(const QString &code, const QString &originalRequestText);
    static QString fittableSelectStageBeforeFunctionMistake(const QString &code);
    static QHash<QString, bool> openSessionsHaveSetCode(const QString &code);
};

#endif
