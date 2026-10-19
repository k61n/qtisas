/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Unit tests for the pure, host-independent logic in src/ai —
             QtiSasAiContent's seven split-mode scope-violation "Directions"
             plus its self-review-fabrication checks, and AiAssistant.cpp's
             internal parsing/matching free functions (exposed for testing
             only via AiAssistantInternal.h). Each case transcribes a
             "Confirmed live" scenario already documented as a comment next
             to the checked function, so a future regex edit that silently
             breaks one of these incidents fails a test instead of waiting
             for that exact transcript shape to recur.
 ******************************************************************************/

#include "ai_content_tests.h"

#include "AiAssistant.h"
#include "AiAssistantInternal.h"
#include "QtiSasAiContent.h"

#include <memory>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest/QtTest>

// ---------------------------------------------------------------------------
// Direction 1 — graph overreach
// ---------------------------------------------------------------------------

void AiContentTests::selfShadowBug_namesEverySelfShadowedVariable()
{
    const QString code = "double gamma = gamma;\ndouble sigma = sigma;\n";
    const QString explanation = QtiSasAiContent::compiledCodeSelfShadowBug(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("gamma"));
    QVERIFY(explanation.contains("sigma"));
}

void AiContentTests::scopeViolation_reportsEveryDirectionAtOnce()
{
    // A compile part whose script both creates an unrelated graph (Direction 1) and never compiles
    // anything (Direction 4). Previously the first direction returned and the second surfaced a
    // round later; both now arrive in one numbered message.
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"FitResults\", 2, 2, 1)\n";
    const QString violation = content.splitPartScopeViolation(
        code, "compile custom Gaussian peak fit function named 'ai/GaussPeak'", QString());
    QVERIFY(!violation.isEmpty());
    QVERIFY(violation.contains("separate scope rules"));
    QVERIFY(violation.contains("1."));
    QVERIFY(violation.contains("2."));
}

void AiContentTests::direction1_blocksUnrelatedNewGraph()
{
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"FitResults\", 2, 2, 1)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "compare fit quality and select best model", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction1_allowsWhenDescriptionMentionsPlotting()
{
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"FitResults\", 2, 2, 1)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot data and best-fit curve into graph 'FitResults'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction1_catchesNewPlot3DOverreach()
{
    QtiSasAiContent content;
    // newPlot3D(...) must be recognized as graph-creation just like newGraph()/plot()/plotResults().
    const QString code = "g3 = newPlot3D(\"Surface\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "compare fit quality and select best model", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction1_allowsSurfaceDescriptionWithoutPlotWord()
{
    // Confirmed live false positive: a perfectly-scoped 3D-plotting part described as "create 3D
    // surface of sin(x)*cos(y) ..., name it 'Surface3D'" got blocked because its description said
    // "surface" rather than literally "plot"/"graph".
    QtiSasAiContent content;
    const QString code = "g3 = newPlot3D(\"Surface3D\", \"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 60, 60)\n"
                         "g3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    const QString violation = content.splitPartScopeViolation(
        code, "create 3D surface of sin(x)*cos(y) over -pi to pi with filled mesh, name it \"Surface3D\"", QString());
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 2 — fit underreach
// ---------------------------------------------------------------------------

void AiContentTests::windowName_camelCaseTracesBackToTheDescription()
{
    // The rule's own intent is that a name "trace back to the description, just reordered".
    // Confirmed live at the same item in three consecutive runs: "SinCosSurface" was one opaque
    // token before CamelCase counted as a boundary, so it matched nothing and cost a round-trip
    // until the model fell back to the bare word "surface".
    QtiSasAiContent content;
    const QString description = "Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi, "
                                "using a filled mesh style, then add a legend and axis labels.";
    for (const QString &name : {QStringLiteral("SinCosSurface"), QStringLiteral("SinCos3D"), QStringLiteral("surface")})
    {
        const QString code = "g3 = newPlot3D(\"" + name + "\", \"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 60, 60)\n";
        QVERIFY2(!content.splitPartScopeViolation(code, description, QString()).contains("never mentions the window"),
                 qPrintable("wrongly rejected: " + name));
    }
}

void AiContentTests::windowName_unrelatedNameStillRejected()
{
    // Every piece must still trace back — a name built from words the description never uses is
    // exactly what this rule exists to catch.
    QtiSasAiContent content;
    const QString description = "Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi.";
    const QString code = "g3 = newPlot3D(\"ScatterHistogram\", \"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 60, 60)\n";
    QVERIFY(content.splitPartScopeViolation(code, description, QString()).contains("never mentions the window"));
}

void AiContentTests::direction3_quotedTableAndGraphNamesDoNotCountAsFunctionNames()
{
    // Confirmed live: this exact item names a TABLE and a GRAPH in quotes while asking for two
    // UNNAMED functions. Both quoted names used to switch off the invented-name exemption, so the
    // names the model had to invent were flagged as another part's job — twice, killing the item.
    QtiSasAiContent content;
    const QString description =
        "Create a table \"GaussPeak\" with 200 rows of synthetic Gaussian-peak data (x, y, dy "
        "columns) with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit "
        "function. Plot the winning model into graph \"GaussPeakFit\" with two stacked panels.";
    const QString code = "c.setFunctionName(\"ai/GaussianPeak\")\nc.compile()\n"
                         "c.setFunctionName(\"ai/LorentzianPeak\")\nc.compile()\n";
    QVERIFY(!content.splitPartScopeViolation(code, description, QString()).contains("never mentions the"));
}

void AiContentTests::direction3_quotedFunctionNameStillEnforced()
{
    // The true positive must survive: when the description DOES name a function, a different name
    // is still a scope violation.
    QtiSasAiContent content;
    const QString description = "Compile a custom fit function named \"GaussPeakModel\".";
    const QString code = "c.setFunctionName(\"ai/SomethingElse\")\nc.compile()\n";
    QVERIFY(content.splitPartScopeViolation(code, description, QString()).contains("never mentions the"));
}

void AiContentTests::knownMistakes_scanGroupsAfterSetFunctionNameIsCaught()
{
    // Confirmed live: this exact order died on "compile: function name is empty" three lines below
    // a setFunctionName() call, because scanGroups() clears lineEditFunctionName on its way past.
    const QString broken = "c.new()\nc.setFunctionName(\"ai/PorodPowerLaw\")\nc.scanGroups()\n"
                           "c.setGroup(\"custom:porod\")\nc.compile()\n";
    QVERIFY(!QtiSasAiContent::compilerScanGroupsAfterNameMistake(broken).isEmpty());
    // The fix the model itself found: scan first, then name.
    const QString fixed = "c.scanGroups()\nc.new()\nc.setFunctionName(\"ai/PorodPowerLaw\")\n"
                          "c.setGroup(\"custom:porod\")\nc.compile()\n";
    QVERIFY(QtiSasAiContent::compilerScanGroupsAfterNameMistake(fixed).isEmpty());
    // A scanGroups() with no compile()/save() after it has nothing to break.
    QVERIFY(
        QtiSasAiContent::compilerScanGroupsAfterNameMistake("c.setFunctionName(\"x\")\nc.scanGroups()\n").isEmpty());
}

void AiContentTests::maximizeWindow_unconditionalInsideElseIsNotFlagged()
{
    // Confirmed live: this exact shape was blocked TWICE with the same explanation, the model
    // (correctly) changed other things, and the repeat-detector gave up — killing a whole LIST
    // item. maximizeWindow() sits at the `if`'s own indentation, i.e. unconditional; only the
    // enclosing `else:` made every line look "indented" to the old body regex.
    const QString code = "    if fc is None:\n"
                         "        scriptPrint(\"unavailable\")\n"
                         "    else:\n"
                         "        gw = graphWindow(\"FitResults\")\n"
                         "        if gw is None:\n"
                         "            gw = newGraph(\"FitResults\", 2, 2, 1)\n"
                         "        maximizeWindow(gw)\n"
                         "        gw.arrangeLayers(False, False)\n"
                         "        gw.layer(1).setCanvasGeometry(1, 2, 3, 4)\n";
    QVERIFY(QtiSasAiContent::conditionalMaximizeWindowMistake(code).isEmpty());
}

void AiContentTests::maximizeWindow_trulyGatedIsStillFlagged()
{
    // The real defect the check exists for must still fire: maximizeWindow() only in the
    // freshly-created branch, with the layout calls running unconditionally after it.
    const QString code = "gw = graphWindow(\"FitResults\")\n"
                         "if gw is None:\n"
                         "    gw = newGraph(\"FitResults\", 2, 2, 1)\n"
                         "    maximizeWindow(gw)\n"
                         "gw.arrangeLayers(False, False)\n"
                         "gw.layer(1).setCanvasGeometry(1, 2, 3, 4)\n";
    const QString explanation = QtiSasAiContent::conditionalMaximizeWindowMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("only inside the \"if gw is None:\" branch"));
}

void AiContentTests::deliverables_plotResultsCountsAsTheFitCurve()
{
    // fittable-python-api.md documents plotResults() as "plot data + fit curves in a named graph"
    // — the supported one-call path. Confirmed live as a false positive: a correct
    // f.plotResults("FitResults") was blocked for having no addCurve/insertCurve.
    QtiSasAiContent content;
    const QString request = "Plot the fit results into graph \"FitResults\", including the data and fit curve.";
    QVERIFY(content.missingRequestedDeliverables("f.plotResults(\"FitResults\")\n", request).isEmpty());
    // Residuals are NOT covered by plotResults(), so that half must still be demanded.
    const QString withResiduals = "Plot the data, fit curve, and residuals into graph \"FitResults\".";
    QVERIFY(
        content.missingRequestedDeliverables("f.plotResults(\"FitResults\")\n", withResiduals).contains("residuals"));
}

void AiContentTests::knownMistakes_fitBeforeConfigureIsCaught()
{
    // Confirmed live: f.fit() with only a table lookup ahead of it died on
    // "fit(): call setFunction() and selectDataset() first" — a wasted round-trip.
    QVERIFY(!QtiSasAiContent::fittableFitBeforeConfigureMistake("t = table(\"RawScan\")\nchi2 = f.fit()\n").isEmpty());
    QVERIFY(
        QtiSasAiContent::fittableFitBeforeConfigureMistake("f.configure(\"ai/M\", [\"RawScan_y\"])\nchi2 = f.fit()\n")
            .isEmpty());
}

void AiContentTests::knownMistakes_noteTabPrintIsCaught()
{
    // Confirmed live: n.tab("C").print() opened the native print dialog and hung the run.
    QVERIFY(!QtiSasAiContent::scriptEditPrintDialogMistake("s = n.tab(\"C\").print()\n").isEmpty());
    QVERIFY(!QtiSasAiContent::scriptEditPrintDialogMistake("n.editorAt(0).print()\n").isEmpty());
    QVERIFY(!QtiSasAiContent::scriptEditPrintDialogMistake("n.currentEditor() . print ()\n").isEmpty());
    QVERIFY(QtiSasAiContent::scriptEditPrintDialogMistake("print(n.tab(\"C\").toPlainText())\n").isEmpty());
}

void AiContentTests::knownMistakes_matrixIntegrateForSumIsCaught()
{
    // Confirmed live: "the sum of all cells" printed as m.integrate() (56.25 instead of 100).
    const QString request = R"(Create a 4x4 matrix "Prod" with i*j. Print the sum of all cells.)";
    QVERIFY(!QtiSasAiContent::matrixIntegrateAsSumMistake("print(m.integrate())\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::matrixIntegrateAsSumMistake("print(sum(m.cell(1, c) for c in r))\n", request).isEmpty());
    // an integral asked for, or no matrix at all: integrate() is right
    QVERIFY(QtiSasAiContent::matrixIntegrateAsSumMistake("print(m.integrate())\n",
                                                         "Print the sum and the integral of matrix \"M\".")
                .isEmpty());
    QVERIFY(
        QtiSasAiContent::matrixIntegrateAsSumMistake("print(m.integrate())\n", "Print the sum of column y.").isEmpty());
}

void AiContentTests::knownMistakes_duplicateWindowNameIsCaught()
{
    // Confirmed live: three newPlot3D("Spheres", ...) calls for "a 3D plot with 3 spheres".
    const QString call = "newPlot3D(\"Spheres\", \"x\", \"y\", \"z\", 0, 1, 0, 1, 10, 10, False, True)\n";
    QVERIFY(!QtiSasAiContent::duplicateWindowNameMistake(call + "g = " + call).isEmpty());
    QVERIFY(QtiSasAiContent::duplicateWindowNameMistake(call).isEmpty());
    // different names, or the same name in two branches
    QVERIFY(QtiSasAiContent::duplicateWindowNameMistake("newTable(\"A\")\nnewTable(\"B\")\n").isEmpty());
    QVERIFY(QtiSasAiContent::duplicateWindowNameMistake(
                "if c:\n    t = newTable(\"A\")\nelse:\n    t = newTable(\"A\", 5)\n")
                .isEmpty());
}

void AiContentTests::knownMistakes_textIntoNumericColumnIsCaught()
{
    // Confirmed live: "alpha" written into column 2 of a new table read back as an empty cell.
    const QString created = "t = newTable(\"Names\", 3, 2)\n";
    QVERIFY(!QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setText(2, 1, \"alpha\")\n").isEmpty());
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setColTextFormat(2)\nt.setText(2, 1, 'alpha')\n")
                .isEmpty());
    // the format set after the write is too late
    QVERIFY(
        !QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setText(2, 1, 'alpha')\nt.setColTextFormat(2)\n")
             .isEmpty());
    // a number, another column formatted, or a table the script did not create
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setText(1, 1, \"1.5\")\n").isEmpty());
    // confirmed live false alarm: a German number after setDecimalSeparators(2)
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setText(1, 2, \"2,5\")\n").isEmpty());
    QVERIFY(!QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setText(1, 2, \"---\")\n").isEmpty());
    QVERIFY(!QtiSasAiContent::textIntoNumericColumnMistake(created + "t.setColTextFormat(1)\nt.setText(2, 1, 'a')\n")
                 .isEmpty());
    QVERIFY(
        QtiSasAiContent::textIntoNumericColumnMistake("t = table(\"Names\")\nt.setText(2, 1, \"alpha\")\n").isEmpty());
}

void AiContentTests::unprintedOutputLine_flaggedOnlyWhenNotPrinted()
{
    // Confirmed live: the right values printed, but "R5: beta 3" was written as text after the code.
    const QString code = "t = table(\"Names\")\nscriptPrint(t.text(2, 2) + \" \" + str(t.cell(1, 3)))\n";
    QVERIFY(AiAssistant::unprintedOutputLineMistake(code, "R5: beta 3").contains("R5:"));
    QVERIFY(AiAssistant::unprintedOutputLineMistake(code + "scriptPrint(f\"R5: {x}\")\n", "R5: beta 3").isEmpty());
    QVERIFY(AiAssistant::unprintedOutputLineMistake(code + "print('R5:', x)\n", "R5: beta 3").isEmpty());
    QVERIFY(
        AiAssistant::unprintedOutputLineMistake("print(\"Number of tabs: \" + n)\n", "Number of tabs: 2").isEmpty());
    // the model's own label with the wrong number: the script's "R10:" line is what counts
    QVERIFY(AiAssistant::unprintedOutputLineMistake("print(f\"R10: {n}\")\n", "R1: 2 print(6*7)").isEmpty());
    // no label line: nothing to ask for
    QVERIFY(AiAssistant::unprintedOutputLineMistake(code, "beta 3").isEmpty());
}

void AiContentTests::labeledLastPrint_repairsOnlyALastTopLevelPrint()
{
    // Confirmed live: str([...]) printed "[]" right, the retry's rewrite printed "" instead.
    QCOMPARE(AiAssistant::labeledLastPrintRepair("x = []\nscriptPrint(str(x))\n", "R12: a b"),
             QString("x = []\nscriptPrint(\"R12: \" + str(str(x)))\n"));
    QCOMPARE(AiAssistant::labeledLastPrintRepair("print(a, b)", "R5: beta 3"), QString("print(\"R5:\", a, b)\n"));
    // already labeled, indented, or not a print: left to the retry
    QVERIFY(AiAssistant::labeledLastPrintRepair("print(f\"R5: {a}\")\n", "R5: beta 3").isEmpty());
    QVERIFY(AiAssistant::labeledLastPrintRepair("if a:\n    print(a)\n", "R5: beta 3").isEmpty());
    QVERIFY(AiAssistant::labeledLastPrintRepair("x = 1\n", "R5: beta 3").isEmpty());
    // a print with a label of its own: confirmed live, "R1: Text of Calc tab: print('A')"
    QVERIFY(AiAssistant::labeledLastPrintRepair("scriptPrint(\"Text of Calc tab: \" + t)\n", "R1: 2 x").isEmpty());
    QVERIFY(AiAssistant::labeledLastPrintRepair("print(f\"Tabs: {n}\")\n", "R1: 2 x").isEmpty());
}

void AiContentTests::requestSuffix_strippedInBothModes()
{
    // Confirmed live: the run-mode suffix's scriptPrint("...") counted as a quoted window name.
    QtiSasAiContent content;
    const QString request = "make a 3D plot with 3 spheres";
    QCOMPARE(withoutRequestSuffix(request + content.requestSuffix(true)), request);
    QCOMPARE(withoutRequestSuffix(request + content.requestSuffix(false)), request);
    QCOMPARE(withoutRequestSuffix(request), request);
}

void AiContentTests::requestOwnText_dropsListInstructionAndSuffix()
{
    QtiSasAiContent content;
    const QString item = R"(Create a table "Baseline" with 30 rows. The result label is "R1:".)";
    const QString listed =
        AiAssistant::itemsWithInstruction({item}, "Verify the underlying logic (formulas, fit setup).").value(0);
    QCOMPARE(requestOwnText(listed + content.requestSuffix(true)), item);
    QCOMPARE(requestOwnText(item + content.requestSuffix(false)), item);
    QCOMPARE(requestOwnText(item), item);
}

void AiContentTests::splitPlanChecks_ignoreListInstructionAndSuffix()
{
    // Confirmed live (LIST SPLIT): "fit setup" in the list instruction demanded a fit part, the run-mode suffix's
    // "write/run a script" demanded scripts in note tabs — on items asking for neither.
    QtiSasAiContent content;
    const QString tableItem = R"(Create a table "Baseline" with 30 rows. The result label is "R1:".)";
    const QString listed = AiAssistant::itemsWithInstruction(
                               {tableItem}, "Verify the underlying logic (formulas, fit setup, column roles).")
                               .value(0) +
                           content.requestSuffix(true);
    const QList<AiAssistant::SplitPart> tablePlan = {
        {R"(Create table "Baseline" with 30 rows and print "R1:".)", {"table"}}};
    QVERIFY(splitPlanMissingRequestedActions(listed, tablePlan).contains("fit"));
    QVERIFY(splitPlanMissingRequestedActions(requestOwnText(listed), tablePlan).isEmpty());

    const QString noteItem = R"(Create a note "Batch" with no content yet. Print its number of tabs.)";
    const QList<AiAssistant::SplitPart> notePlan = {{R"(Create note "Batch" and print its number of tabs.)", {"note"}}};
    const QString withSuffix = noteItem + content.requestSuffix(true);
    QVERIFY(splitPlanMissingRequestedActions(withSuffix, notePlan).contains("write the scripts into the note's tabs"));
    QVERIFY(splitPlanMissingRequestedActions(requestOwnText(withSuffix), notePlan).isEmpty());
}

void AiContentTests::pureTextAnswer_onlyLiteralPrints()
{
    // Confirmed live: self-review turned a right text answer into "Unable to determine ...".
    QVERIFY(AiAssistant::isPureTextAnswer("scriptPrint(\"No. That script creates cylinders.\")\n"));
    QVERIFY(AiAssistant::isPureTextAnswer("# answer\nprint('a')\nscriptPrint(\"b \\\"c\\\"\")\n"));
    QVERIFY(!AiAssistant::isPureTextAnswer("scriptPrint(f\"R = {r}\")\n"));
    QVERIFY(!AiAssistant::isPureTextAnswer("scriptPrint(\"n: \" + str(n))\n"));
    QVERIFY(!AiAssistant::isPureTextAnswer("x = 1\nscriptPrint(\"done\")\n"));
    QVERIFY(!AiAssistant::isPureTextAnswer(""));
}

void AiContentTests::shortQuestion_withoutActionVerb()
{
    // Confirmed live: "there are spheres?" went to the split planner.
    QVERIFY(AiAssistant::isShortQuestion("there are spheres?"));
    QVERIFY(AiAssistant::isShortQuestion("Why does it look like a cylinder? "));
    QVERIFY(!AiAssistant::isShortQuestion("can you plot the spheres again?"));
    QVERIFY(!AiAssistant::isShortQuestion("make a 3D plot with 3 spheres"));
}

void AiContentTests::trailingAnnotationOutput_strippedButRealAnnotationsKept()
{
    // Confirmed live: "R1: 385" after the script compiled (an annotated name) and the result line was never printed.
    QCOMPARE(AiAssistant::withoutTrailingAnnotationOutput("s = 385\nscriptPrint(str(s))\nR1: 385\n"),
             QString("s = 385\nscriptPrint(str(s))"));
    QCOMPARE(AiAssistant::withoutTrailingAnnotationOutput("x = []\nR12: []\n\n"), QString("x = []"));
    QVERIFY(AiAssistant::withoutTrailingAnnotationOutput("x: int = 5\n").isEmpty());
    QVERIFY(AiAssistant::withoutTrailingAnnotationOutput("d = {\n    \"a\": 1,\n}\n").isEmpty());
    QVERIFY(AiAssistant::withoutTrailingAnnotationOutput("R1: 385\n").isEmpty()); // nothing would be left
}

void AiContentTests::inputNamesInRequest_skipsNamesBeingCreated()
{
    const QString request = R"(Square the column "x" of table "Data" into a new table "Squares", then plot it.)";
    QCOMPARE(inputNamesInRequest(request), QStringList({"x", "Data"}));
    QVERIFY(inputNamesInRequest("Create a table \"T\" with 3 rows.").isEmpty());
    // LIST 5 item 3: what a created graph is drawn from is read, not created
    QCOMPARE(inputNamesInRequest(R"(Create a graph "Report5" with two stacked layers: the top one with the "Calib" y )"
                                 R"(data as points and the fitted curve from table "fitCurve-ai/Quad" as a line.)"),
             QStringList({"Calib", "fitCurve-ai/Quad"}));
    // benchmark ASCII1D item 2: the name a result is given after "as" is made, not read
    QVERIFY(inputNamesInRequest(R"(Import the ascii file header.dat as the table "HeaderData"; it has a header.)")
                .isEmpty());
    QVERIFY(inputNamesInRequest(R"(Save the dataset as "Out".)").isEmpty());
    QCOMPARE(inputNamesInRequest(R"(Export the SAS dataset "QI-alpha" as the file "alpha3.dat".)"),
             QStringList({"QI-alpha"}));
}

void AiContentTests::existingEntityNames_skipNameTheRequestCreatesElsewhere()
{
    // Confirmed live (benchmark LIST 5, item 5): "Auto" is created by the tab's script, then asked about — the item
    // was stopped as referring to a missing "Auto"
    const QString request = R"(Create a note "Pipeline" with two tabs named "make" and "check" (the first tab renamed )"
                            R"(too). The "make" tab holds a script that creates a table "Auto" with 5 rows. Run only )"
                            R"(the "make" tab. Print the number of tabs and whether table "Auto" exists.)";
    for (const QPair<QString, QString> &entry : extractExistingEntityNames(request))
        QVERIFY2(entry.first != "Auto", "a name the request creates is no existing input");
    QVERIFY(!inputNamesInRequest(request).contains("Auto"));
    // a name only read stays an input
    QCOMPARE(inputNamesInRequest(R"(Print whether table "Auto" exists.)"), QStringList({"Auto"}));
}

void AiContentTests::trailingOutputLines_strippedButCodeKept()
{
    // Confirmed live: the model wrote the expected output after the script, which then did not compile.
    QCOMPARE(AiAssistant::withoutTrailingOutputLines("n = 2\nprint(\"R10:\", n)\nR10: 2 print(6*7)\n\n"),
             QStringList({"n = 2\nprint(\"R10:\", n)", "n = 2"}));
    // the last line belongs to the code: indented, a statement, or a continued/unbalanced line
    QVERIFY(AiAssistant::withoutTrailingOutputLines("for i in r:\n    print(i)\n").isEmpty());
    QVERIFY(AiAssistant::withoutTrailingOutputLines("x = 1\nreturn x\n").isEmpty());
    QVERIFY(AiAssistant::withoutTrailingOutputLines("x = f(1,\n  2)\ny = g(\n").isEmpty());
    QVERIFY(AiAssistant::withoutTrailingOutputLines("one line only").isEmpty());
}

void AiContentTests::deliverables_flagsDeletionThatNeverHappens()
{
    // Confirmed live: asked to delete, the script only listed functions, and the item counted as done.
    QtiSasAiContent content;
    const QString request = R"(Delete the fit functions "ai/RefLine" and "ai/RefDecay" from the library.)";
    const QString listing = "c = Compiler\nscriptPrint(str(c.functionNames(\"ALL\")))\n";
    QVERIFY(content.missingRequestedDeliverables(listing, request).contains("deletion"));
    QVERIFY(content.missingRequestedDeliverables("c = Compiler\nc.delete(\"ai/RefLine\")\n", request).isEmpty());
    QVERIFY(content.missingRequestedDeliverables("closeWindow(table(\"T\"))\n", "Remove the table \"T\".").isEmpty());
    // no object noun after it: nothing to call
    QVERIFY(content.missingRequestedDeliverables("y = smooth(x)\n", "Remove the noise from column y.").isEmpty());
}

void AiContentTests::deliverables_chi2AsCriterionIsNoReport()
{
    // Confirmed live (benchmark LIST 5, item 2): "the function with the smaller chi2/dof" chooses by it; the correct
    // script was blocked twice for "never printing chi2"
    QtiSasAiContent content;
    const QString code = "best = min(results, key=lambda n: results[n][0])\nscriptPrint(\"R2:\", best, a, w)\n";
    QVERIFY(content
                .missingRequestedDeliverables(
                    code, "Fit both. Print the name of the function with the smaller chi2/dof, then its A and w.")
                .isEmpty());
    QVERIFY(content.missingRequestedDeliverables(code, "Fit it and report chi2/dof.").contains("chi2"));
    // a split part's own wording of the same task
    QVERIFY(content
                .missingRequestedDeliverables(
                    code, "Fit both to \"Peak5\"; compare chi2/dof, then print the name of the function with the "
                          "smaller chi2/dof.")
                .isEmpty());
}

void AiContentTests::splitPlanUnrequestedActions_smallerChi2IsAComparison()
{
    // Confirmed live (benchmark LIST 5, item 2): a part comparing chi2/dof was flagged as never requested
    const QString request = "Fit both to \"Peak5\". Print the name of the function with the smaller chi2/dof.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"fit both functions to table 'Peak5', compare chi2/dof and print the winner", {}};
    QVERIFY(splitPlanUnrequestedActions(request, plan).isEmpty());
    // a comparison the request never asked for is still flagged
    QCOMPARE(splitPlanUnrequestedActions("Fit \"ai/G\" to table \"Peak5\".", plan), QList<int>({0}));
}

void AiContentTests::splitPlanMissingActions_fitFunctionToTableIsAFit()
{
    // Confirmed live (benchmark LIST 5, item 1): "Fit function \"ai/Quad\" to table ..." was reported as no fit
    const QString request = R"(Compile a new fit function "ai/Quad" and fit it to "Calib".)";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Compile new fit function \"ai/Quad\" for y = a*x^2 + b*x + c", {}};
    plan << AiAssistant::SplitPart{R"(Fit function "ai/Quad" to table "Calib" weighted by dy)", {}};
    QVERIFY(!splitPlanMissingRequestedActions(request, plan).contains("fit"));
    // compiling a fit function alone is still no fit
    plan.removeLast();
    QVERIFY(splitPlanMissingRequestedActions(request, plan).contains("fit"));
}

void AiContentTests::deliverables_flagsMissingResidualsAndFitCurve()
{
    // The incident this check exists for: asked for data + fit curve + residuals, the script
    // recoloured one existing curve instead — a later item's task, and none of its own.
    QtiSasAiContent content;
    const QString request = "Plot the fit results from the previous step into graph \"FitResults\", "
                            "including the data, fit curve, and residuals.";
    const QString code = "gw = graphWindow(\"FitResults\")\n"
                         "if gw is None:\n"
                         "    scriptPrint(\"Graph 'FitResults' does not exist.\")\n"
                         "else:\n"
                         "    g = gw.activeLayer()\n"
                         "    g.setCurveLineColor(1, colorIndex(\"blue\"))\n";
    const QString explanation = content.missingRequestedDeliverables(code, request);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("residuals"));
    QVERIFY(explanation.contains("addCurve()"));
    QVERIFY(explanation.contains("omits 2 things"));
    QVERIFY(explanation.contains("only one that will do this task"));
}

void AiContentTests::deliverables_guardClauseDoesNotTrip()
{
    // A guarded script is legitimate (confirmed by review: creating the dataset was another item's
    // job) — what matters is that the working branch CONTAINS the calls, so a presence test over
    // the whole script must pass here.
    QtiSasAiContent content;
    const QString request = "Plot the fit curve and residuals into graph \"FitResults\".";
    const QString code = "gw = graphWindow(\"FitResults\")\n"
                         "if gw is None:\n"
                         "    scriptPrint(\"not there\")\n"
                         "else:\n"
                         "    l = gw.layer(1)\n"
                         "    l.insertCurve(fc, \"x\", \"y\", Graph.CurveType.Line)\n"
                         "    l.insertCurve(fc, \"x\", \"residues\", Graph.CurveType.Scatter)\n";
    QVERIFY(content.missingRequestedDeliverables(code, request).isEmpty());
}

void AiContentTests::deliverables_silentWhenEverythingDelivered()
{
    // Calibration case from the same run: this item passed every entry, and must stay passing.
    QtiSasAiContent content;
    const QString request = "Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi, "
                            "using a filled mesh style, then add a legend and axis labels.";
    const QString code = "g3 = newPlot3D(\"S\", \"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1.5, 1.5, 60, 60)\n"
                         "g3.setXAxisLabel(\"x\")\n"
                         "g3.showLegend(True)\n";
    QVERIFY(content.missingRequestedDeliverables(code, request).isEmpty());
}

void AiContentTests::deliverables_ignoresFitMentionedOnlyToExcludeIt()
{
    // Calibration false positive that removed the generic "fit" entry: this request names fitting
    // in order to FORBID it, and a fit() call here would be the defect, not the fix.
    QtiSasAiContent content;
    const QString request = "Compile a custom Porod power-law fit function with an adjustable exponent "
                            "parameter, then simulate a curve from it without fitting any real data.";
    const QString code = "c = Compiler\nc.new()\nok = c.compile()\n"
                         "f = Fittable\nf.configure(\"ai/PorodPowerLaw\", [])\n"
                         "chi2 = f.simulateCurve()\n";
    QVERIFY(content.missingRequestedDeliverables(code, request).isEmpty());
}

void AiContentTests::deliverables_separatesAddingATabFromActivatingOne()
{
    // Adding a tab and activating a window are different calls: a script that only activates the
    // window it already has has not added a script tab.
    QtiSasAiContent content;
    const QString request = "Add a script tab to note \"Batch\" that prints its row count via autoexec.";
    const QString onlyActivates = "n = note(\"Batch\")\nn.setAutoexec(True)\nn.setText(s)\n"
                                  "activateWindow(n)\nprint(t.numRows())\n";
    const QString explanation = content.missingRequestedDeliverables(onlyActivates, request);
    QVERIFY(explanation.contains("script TAB"));
    const QString withTab = "n = note(\"Batch\")\nn.setAutoexec(True)\nn.addTab()\n"
                            "n.editorAt(0).setText(s)\nprint(t.numRows())\n";
    QVERIFY(content.missingRequestedDeliverables(withTab, request).isEmpty());
}

void AiContentTests::deliverables_creatingANoteWithAScriptTabNeedsNoAddTab()
{
    // Confirmed live (item 4 of a 12-item LIST, every run): a new note already owns one tab —
    // note-python-api.md prints "3  (1 initial + 2 added)" — and setText() replaces the text of the
    // active editor tab, so writing the script straight into that tab IS the requested script tab.
    // Demanding addTab() here made the retry add a SECOND tab and write there, leaving the first
    // empty and the request's "keep it as the active tab" less true than in the rejected answer.
    QtiSasAiContent content;
    const QString request = "Create a note \"Setup\" with an auto-executing script tab that disables "
                            "automatic table-formula recalculation project-wide; keep it as the "
                            "active tab so it actually runs.";
    const QString code = "n = newNote(\"Setup\")\n"
                         "n.setText(\"setAutoUpdateTableValues(False)\")\n"
                         "n.setAutoexec(True)\n";
    QVERIFY(content.missingRequestedDeliverables(code, request).isEmpty());
    // A note with a script tab that holds no script at all is still incomplete.
    const QString empty = "n = newNote(\"Setup\")\nn.setAutoexec(True)\n";
    QVERIFY(content.missingRequestedDeliverables(empty, request).contains("script tab"));
}

void AiContentTests::extraction_enumerationCommaDoesNotHideCreationVerb()
{
    // Confirmed live (item 12 of a 12-item LIST): the bare-comma clause boundary cut "Plot" off
    // its own clause, leaving a lookback of "and residuals into ", so a graph the request was
    // plainly creating was extracted as an existing name.
    const QString request = "Plot the winning model's data, fit curve, and residuals into graph "
                            "\"GaussPeakFit\" with two stacked panels, a styled legend, and axis titles.";
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(request);
    for (const QPair<QString, QString> &entry : names)
        QVERIFY2(entry.first != "GaussPeakFit", "creation target extracted as an existing name");
}

void AiContentTests::extraction_commaThenStillStartsANewClause()
{
    // The one case the comma boundary was added for must keep working: a genuine INPUT named after
    // an unrelated creation in the same sentence.
    const QString request = R"(Create a table "Out" with 10 rows, then fit table "RawScan" with a line.)";
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(request);
    QStringList found;
    for (const QPair<QString, QString> &entry : names)
        found << entry.first;
    QVERIFY(found.contains("RawScan"));
    QVERIFY(!found.contains("Out"));
}

void AiContentTests::liveFacts_reportsMissingNameAsUnknownNotAbsent()
{
    // The absence is true of THIS moment and not of the moment the script runs, so it is reported
    // as UNKNOWN — see liveEntityFactsNote()'s own comment for the three confirmed-live failures
    // that came from stating it as a fact instead.
    QtiSasAiContent content;
    const QString note = content.liveEntityFactsNote({qMakePair(QString("SAXS_data"), QStringList())});
    QVERIFY(note.contains("\"SAXS_data\": UNKNOWN"));
    QVERIFY(!note.contains("does not exist"));
    QVERIFY(!note.contains("MISSING"));
    // What must survive: no probe round, resolve it in the script, and no invented stand-ins.
    QVERIFY(note.contains("Do not spend a separate round-trip"));
    QVERIFY(note.contains("if t is None: t = newTable(name, ...)"));
    QVERIFY(note.contains("never invent substitute data"));
}

void AiContentTests::liveFacts_reportsExistingKind()
{
    // The existing half stays a hard fact — existence is monotone across a session, and this is
    // where the saved probe and the kind-collision warning both come from.
    QtiSasAiContent content;
    const QString note = content.liveEntityFactsNote({qMakePair(QString("Data1"), QStringList{"matrix"})});
    QVERIFY(note.contains("\"Data1\": exists NOW, as: matrix"));
    QVERIFY(note.contains("that is a name collision"));
    // No unknown-name guidance when nothing is unknown — it would only invite defensive branching
    // around an object already known to be there.
    QVERIFY(!note.contains("UNKNOWN"));
}

void AiContentTests::liveFacts_mixesUnknownAndExistingIndependently()
{
    // The asymmetry is the whole design: one name resolved, one not, each getting its own half.
    QtiSasAiContent content;
    const QString note =
        content.liveEntityFactsNote({qMakePair(QString("sasviewmodels/sphere"), QStringList{"fittable"}),
                                     qMakePair(QString("SAXS_data"), QStringList())});
    QVERIFY(note.contains("\"sasviewmodels/sphere\": exists NOW, as: fit function"));
    QVERIFY(note.contains("\"SAXS_data\": UNKNOWN"));
    QVERIFY(note.contains("UNKNOWN is the real answer"));
    QVERIFY(note.contains("that is a name collision"));
}

void AiContentTests::liveFacts_translatesInternalKindLabels()
{
    // "compile"/"fittable" are the existence-check parser's vocabulary, not a user's — confirmed
    // live, a real note read `"sasviewmodels/sphere": EXISTS (compile, fittable)`.
    QtiSasAiContent content;
    const QString note =
        content.liveEntityFactsNote({qMakePair(QString("sasviewmodels/sphere"), QStringList{"compile", "fittable"})});
    QVERIFY(note.contains("exists NOW, as: compiled function and fit function"));
    QVERIFY(!note.contains("compile,"));
}

void AiContentTests::liveFacts_emptyWhenNothingToReport()
{
    QtiSasAiContent content;
    QVERIFY(content.liveEntityFactsNote({}).isEmpty());
}

void AiContentTests::direction2_blocksMissingFitCall()
{
    QtiSasAiContent content;
    const QString code = "fc = table(\"fitCurve-ai/Model\")\nscriptPrint(str(fc))\n";
    const QString violation = content.splitPartScopeViolation(
        code, "fit both Gaussian and Lorentzian models to the data with weighted errors", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction2_allowsRealFit()
{
    QtiSasAiContent content;
    const QString code = "f = Fittable\nf.configure(\"ai/Model\", [\"Data_y\"])\nchi2 = f.fit()\n";
    const QString violation =
        content.splitPartScopeViolation(code, "fit both models to the data with weighted errors", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction2_catchesRefitPrefix()
{
    // Regression for the Phase 1b word-boundary fix: "refit" has no \b immediately before "fit"
    // (contiguous letters), so the un-fixed regex would have missed this phrasing entirely.
    QtiSasAiContent content;
    const QString code = "scriptPrint(\"already fit earlier\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "refit both models with weighted errors", QString());
    QVERIFY(!violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 3 — compile overreach
// ---------------------------------------------------------------------------

void AiContentTests::direction3_allowsBareNameMatchingAiPrefixed()
{
    // Confirmed live: a description naming just "GaussPeakModel" (no "ai/" prefix — that
    // namespace is this app's own internal convention) must not block the correct, "ai/"-prefixed
    // compile that follows the established convention.
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakModel\")\nc.compile()\n";
    const QString violation =
        content.splitPartScopeViolation(code, "compile custom fit function named 'GaussPeakModel'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction3_blocksUnrelatedFunctionName()
{
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.setFunctionName(\"ai/LorentzianPeakModel\")\nc.compile()\n";
    const QString violation =
        content.splitPartScopeViolation(code, "compile custom fit function named 'GaussPeakModel'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction3_allowsInventedNameWhenDescriptionNamesNoFunction()
{
    // Confirmed live: the original request never named the function at all ("compile a custom
    // Porod power-law fit function with an adjustable exponent parameter"), so the model must
    // invent a name — no invented name can ever appear in a description that never specified
    // one. Blocking the first (and only) setFunctionName() call here would block every possible
    // name unconditionally and stall the whole split-mode run.
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.setFunctionName(\"ai/PorodPowerLaw\")\nc.compile()\n";
    const QString violation = content.splitPartScopeViolation(
        code, "compile a custom Porod power-law fit function with an adjustable exponent parameter", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction3_blocksSecondCompileCallEvenWhenDescriptionNamesNoFunction()
{
    // Mirrors the documented true-positive (self-review merging a second, unrelated compile job
    // into an otherwise correctly single-scoped part) but with an unnamed first function, to
    // confirm the "no name given" exemption only ever applies to the FIRST setFunctionName() call.
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.setFunctionName(\"ai/PorodPowerLaw\")\nc.compile()\n"
                         "c2 = Compiler\nc2.setFunctionName(\"ai/LorentzianPeakModel\")\nc2.compile()\n";
    const QString violation = content.splitPartScopeViolation(
        code, "compile a custom Porod power-law fit function with an adjustable exponent parameter", QString());
    QVERIFY(!violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 4 — compile underreach
// ---------------------------------------------------------------------------

void AiContentTests::direction4_blocksMissingCompileCall()
{
    QtiSasAiContent content;
    const QString code = "scriptPrint(str(dir(qti.app)))\n";
    const QString violation = content.splitPartScopeViolation(
        code, "compile custom Gaussian peak fit function named 'ai/GaussPeak'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction4_catchesRecompilePrefix()
{
    // Regression for the Phase 1b word-boundary fix: "recompile" has no \b immediately before
    // "compil" (contiguous letters).
    QtiSasAiContent content;
    const QString code = "scriptPrint(\"already compiled earlier\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "recompile the fit function named 'ai/GaussPeak'", QString());
    QVERIFY(!violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 5 — wrong target window
// ---------------------------------------------------------------------------

void AiContentTests::direction5_wholeRequestWithoutNameLeavesItToTheScript()
{
    // Confirmed live: a request naming no window blocked the plot "ThreeSpheres" ("three" is not "3").
    QtiSasAiContent content;
    const QString request = "make a 3D plot with 3 spheres connected along one line: radius ratio 5:1:3";
    const QString code = "g3 = plot3D(\"x\", \"y\", \"z\", 0, 1, 0, 1, 10, 10, False, True)\n"
                         "setWindowName(g3, \"ThreeSpheres\")\n";
    QVERIFY(!content.splitPartScopeViolation(code, request, QString(), true).contains("never mentions the window"));
    // a split part without a name, or a whole request that names a different window: still checked
    QVERIFY(content.splitPartScopeViolation(code, request, QString(), false).contains("never mentions the window"));
    QVERIFY(content.splitPartScopeViolation(code, request + " named \"Beads\"", QString(), true)
                .contains("never mentions the window"));
}

void AiContentTests::direction5_blocksMismatchedWindowName()
{
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"GaussianFit\", 2, 2, 1)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot data and best-fit curve into graph 'GaussPeakFit'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction5_allowsMatchingWindowName()
{
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"GaussPeakFit\", 2, 2, 1)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot data and best-fit curve into graph 'GaussPeakFit'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction5_blocksMismatchedNewPlot3DName()
{
    QtiSasAiContent content;
    const QString code = "g3 = newPlot3D(\"WrongName\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot surface into 3D graph 'RightName'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction5_allowsSanitizedFormulaNameMatch()
{
    // Confirmed live, severe: a part described only via its formula ("...plot 3D surface of
    // sin(x)*cos(y) for x,y...", no quoted name at all) deadlocked between Direction 5 (exact
    // substring) and setWindowName()'s own character rule (rejects the formula's punctuation) —
    // no name could satisfy both. Stripping punctuation from both sides before comparing resolves
    // it: "sinxcosy" (valid at runtime) now matches the description's "sin(x)*cos(y)".
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "setWindowName(g3, \"sinxcosy\")\n";
    const QString violation = content.splitPartScopeViolation(
        code, "Plot 3D surface of sin(x)*cos(y) for x,y from -pi to pi with filled mesh style", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction5_suggestsPlainWordFallbackOnDeadlock()
{
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "setWindowName(g3, \"CompletelyUnrelatedName\")\n";
    const QString violation = content.splitPartScopeViolation(
        code, "Plot 3D surface of sin(x)*cos(y) for x,y from -pi to pi with filled mesh style", QString());
    QVERIFY(!violation.isEmpty());
    QVERIFY(violation.contains("plain descriptive WORD"));
}

void AiContentTests::direction5_allowsReorderedWordTokenMatch()
{
    // Confirmed live: description says "3D surface" (3D-then-surface) but a perfectly sensible
    // candidate name is "Surface3D" (surface-then-3D) — "surface3d" is never a substring of
    // "...3dsurface...", so the plain substring check rejected this legitimately-scoped name
    // twice in a row, and the retry converged on the WORSE name "Surface" (dropping "3D"), which
    // then silently created a stray NEW window instead of updating the existing "Surface3D" one.
    QtiSasAiContent content;
    const QString code = "g3 = newPlot3D(\"Surface3D\", \"cos(x)*cos(x)\", -pi, pi, -pi, pi, -1, 1, 60, 60)\n";
    const QString violation = content.splitPartScopeViolation(
        code, "create the formula cos(x)*cos(x) over x,y from -pi to pi and plot it as a 3D surface", QString());
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 6 — cross-part function mismatch
// ---------------------------------------------------------------------------

void AiContentTests::direction6_blocksBareNameMatchingEarlierCompile()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakModel\")\nc.compile()\n";
    const QString code = "f = Fittable\nf.configure(\"GaussPeakModel\", [\"Data_y\"])\nchi2 = f.fit()\n";
    const QString violation =
        content.splitPartScopeViolation(code, "fit both models and compare chi2/dof", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction6_allowsAlreadyQualifiedName()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakModel\")\nc.compile()\n";
    const QString code = "f = Fittable\nf.configure(\"ai/GaussPeakModel\", [\"Data_y\"])\nchi2 = f.fit()\n";
    const QString violation =
        content.splitPartScopeViolation(code, "fit both models and compare chi2/dof", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 7 — hallucinated fitCurve name
// ---------------------------------------------------------------------------

void AiContentTests::direction7_blocksNameNotActuallyCompiled()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakFit\")\nc.compile()\n";
    const QString code = "fc = table(\"fitCurve-ai/GaussPeak\")\n"; // truncated/invented, not the real name
    const QString violation = content.splitPartScopeViolation(code, "read chi2/dof and compare", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction7_allowsNameThatWasCompiled()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakFit\")\nc.compile()\n";
    const QString code = "fc = table(\"fitCurve-ai/GaussPeakFit\")\n";
    const QString violation = content.splitPartScopeViolation(code, "read chi2/dof and compare", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction8_blocksPlot3DCreateWithoutSetWindowName()
{
    // Confirmed live: a part created a surface via plot3D(formula, ...) with no setWindowName()
    // call — a later part couldn't find it and rebuilt the whole surface from scratch instead.
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -3.14, 3.14, -3.14, 3.14, -1.5, 1.5)\n"
                         "g3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "create 3D surface plot of sin(x)*cos(y)", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction8_allowsPlot3DCreateWithSetWindowName()
{
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -3.14, 3.14, -3.14, 3.14, -1.5, 1.5)\n"
                         "setWindowName(g3, \"SurfacePlot\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "create 3D surface plot \"SurfacePlot\" of sin(x)*cos(y)", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction8_allowsPlot3DLookupForm()
{
    // plot3D("name") with a single argument is a LOOKUP of an existing window, not a creation —
    // must not be flagged for missing setWindowName().
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"SurfacePlot\")\ng3.showLegend(True)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "add legend and axis labels to the 3D plot", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction9_blocksMatrixLookupOnTableName()
{
    // Confirmed live: "MeshZ" was created via newTable(...) in an earlier part; this part's
    // matrix(name) lookup always returns None for a Table name, producing plot3D(None, ...).
    // setWindowName()'s target name is quoted in the description too, so Directions 5/8 don't
    // also fire, isolating Direction 9 here.
    QtiSasAiContent content;
    const QString earlierPartsCode = "t = newTable(\"MeshZ\", 50, 50)\n";
    const QString code = "g3 = plot3D(matrix(\"MeshZ\"), Qti3D.FILLEDMESH)\nsetWindowName(g3, \"MeshSurface\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot the mesh surface into graph 'MeshSurface'", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction9_blocksExistTableOnMatrixName()
{
    // Confirmed live: "SphereMap" was created via newMatrix(...); existTable() is strictly
    // type-specific and always returns False for it, silently no-opping the guarded block.
    QtiSasAiContent content;
    const QString earlierPartsCode = "m = newMatrix(\"SphereMap\", 50, 50)\n";
    const QString code = "if existTable(\"SphereMap\"):\n    g = plot3D(matrix(\"SphereMap\"), Qti3D.FILLEDMESH)\n"
                         "    setWindowName(g, \"SphereMapPlot\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot the sphere map into graph 'SphereMapPlot'", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction9_allowsConsistentTableUsage()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "t = newTable(\"MeshZ\", 50, 50)\n";
    const QString code = "if existTable(\"MeshZ\"):\n    t = table(\"MeshZ\")\n";
    const QString violation = content.splitPartScopeViolation(code, "read the mesh table", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction9_allowsConsistentMatrixUsage()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "m = newMatrix(\"SphereMap\", 50, 50)\n";
    const QString code = "g = plot3D(matrix(\"SphereMap\"), Qti3D.FILLEDMESH)\nsetWindowName(g, \"SphereMapPlot\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot the sphere map into graph 'SphereMapPlot'", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction10_blocksReusingSourceNameForNewWindow()
{
    // Confirmed live, severe: a part plotting existing matrix "SphereMap" reused that exact name
    // for the new 3D window via setWindowName — ValueError: name already exists. Burned 17 retries.
    QtiSasAiContent content;
    const QString code = "m = matrix(\"SphereMap\")\n"
                         "g3 = plot3D(m, 5)\n"
                         "setWindowName(g3, \"SphereMap\")\n";
    const QString violation = content.splitPartScopeViolation(
        code, "Plot a 3D surface of 'SphereMap' with filled mesh style, legend, and axis labels", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction10_allowsDistinctNewWindowName()
{
    QtiSasAiContent content;
    const QString code = "m = matrix(\"SphereMap\")\n"
                         "g3 = plot3D(m, 5)\n"
                         "setWindowName(g3, \"Surface\")\n";
    const QString violation = content.splitPartScopeViolation(
        code, "Plot a 3D surface of 'SphereMap' with filled mesh style, legend, and axis labels", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction10_allowsNoExistingLookupAtAll()
{
    QtiSasAiContent content;
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "setWindowName(g3, \"SinCos3D\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "create 3D surface plot named 'SinCos3D'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction11_blocksGraphWindowLookupOnGraph3DName()
{
    // Confirmed live, severe: setWindowName(g3, "Sincos3D") created a real Graph3D window; a
    // later part's graphWindow("Sincos3D") returned None, burning 8+ retries chasing dead ends.
    QtiSasAiContent content;
    const QString earlierPartsCode = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                                     "setWindowName(g3, \"Sincos3D\")\n";
    const QString code = "g3 = graphWindow(\"Sincos3D\")\ng3.setPlotStyle(\"FilledMesh\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "add a legend to graph 'Sincos3D'", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction11_blocksGraphWindowLookupOnNewPlot3DName()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "g3 = newPlot3D(\"Sincos3D\")\n";
    const QString code = "g3 = graphWindow(\"Sincos3D\")\ng3.showLegend(True)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "add a legend to graph 'Sincos3D'", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction11_blocksExistGraphOnNewPlot3DName()
{
    // benchmark LIST 5, item 4 (twice): existGraph("Saddle3D") right after newPlot3D("Saddle3D", m) printed False
    QtiSasAiContent content;
    const QString code =
        "g3 = newPlot3D(\"Saddle3D\", m, Qti3D.FILLEDMESH)\nscriptPrint(\"R4:\", existGraph(\"Saddle3D\"))\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot 'Saddle' as a 3D surface 'Saddle3D'", QString());
    QVERIFY(violation.contains("existPlot3D(\"Saddle3D\")"));
}

void AiContentTests::direction11_allowsPlot3DLookupOnGraph3DName()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                                     "setWindowName(g3, \"Sincos3D\")\n";
    const QString code = "g3 = plot3D(\"Sincos3D\")\ng3.showLegend(True)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "add a legend to graph 'Sincos3D'", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction11_allowsGraphWindowLookupOnUnrelatedName()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "gw = newGraph(\"FitResults\")\n";
    const QString code = "gw = graphWindow(\"FitResults\")\ngw.activeLayer().replot()\n";
    const QString violation = content.splitPartScopeViolation(code, "restyle graph 'FitResults'", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 12 — plot underreach
// ---------------------------------------------------------------------------

void AiContentTests::direction12_blocksMissingPlotCall()
{
    // Confirmed live: a part described as "plot winning model's data, fit curve, and residuals
    // into graph 'GaussPeakFit'..." only re-read chi2/dof from both fitCurve tables and printed
    // the winner (again — an earlier part already had) — zero graph API calls.
    QtiSasAiContent content;
    const QString code = "fc_g = table(\"fitCurve-ai/GaussianPeak\")\n"
                         "fc_l = table(\"fitCurve-ai/LorentzianPeak\")\n"
                         "scriptPrint(\"Winner: GaussianPeak\")\n";
    const QString violation = content.splitPartScopeViolation(
        code, "plot winning model's data, fit curve, and residuals into graph 'GaussPeakFit'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction12_allowsRealPlot()
{
    QtiSasAiContent content;
    const QString code = "gw = newGraph(\"GaussPeakFit\", 2, 1, 1)\n"
                         "gw.layer(1).insertCurve(table(\"GaussPeak\"), \"x\", \"y\", Graph.CurveType.Line)\n";
    const QString violation = content.splitPartScopeViolation(
        code, "plot winning model's data, fit curve, and residuals into graph 'GaussPeakFit'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction12_allowsStylingOnlyDescriptionWithoutDataWord()
{
    // A legitimately styling-only part mentions "graph" but not "data"/"curve"/"residual", and
    // correctly has no reason to call newGraph()/insertCurve()/etc. itself.
    QtiSasAiContent content;
    const QString code = "gw = graphWindow(\"GaussPeakFit\")\n"
                         "leg = gw.activeLayer().newLegend()\n"
                         "leg.setText(\"Gaussian fit\")\n";
    const QString violation =
        content.splitPartScopeViolation(code, "add a styled legend and axis titles to graph 'GaussPeakFit'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction12_allowsWhenDescriptionHasNoPlotVerb()
{
    QtiSasAiContent content;
    const QString code = "fc_g = table(\"fitCurve-ai/GaussianPeak\")\nscriptPrint(str(fc_g))\n";
    const QString violation =
        content.splitPartScopeViolation(code, "compare chi2/dof and report which model fits better", QString());
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Direction 14 — simulate underreach
// ---------------------------------------------------------------------------

void AiContentTests::direction14_blocksMissingSimulateCall()
{
    // Confirmed live, severe: a part described only as "simulate a curve from the compiled
    // Porod power-law function named 'ai/PorodFit'" instead opened an EXISTING graph window by
    // name and set its title/axis labels/log-log scale — 100% graph-styling code, zero Fittable
    // calls, not even a botched simulate attempt. The graph didn't even exist yet at this point
    // in the plan (a LATER part's own job was to create it), so the script's own "if gw is None"
    // branch caught the lookup failure and printed a message instead of erroring — still accepted
    // as "part done" with no simulation ever having happened.
    QtiSasAiContent content;
    const QString code = "gw = graphWindow(\"PorodSim\")\n"
                         "if gw is None:\n"
                         "    scriptPrint(\"No graph window 'PorodSim' found.\")\n"
                         "else:\n"
                         "    g = gw.activeLayer()\n"
                         "    g.setTitle(\"Porod Power-Law Fit\")\n"
                         "    g.setLinOrLogAxis(Graph.Axis.Bottom, True)\n";
    const QString violation = content.splitPartScopeViolation(
        code, "simulate a curve from the compiled Porod power-law function named 'ai/PorodFit'", QString());
    QVERIFY(!violation.isEmpty());
}

void AiContentTests::direction14_allowsRealSimulateCall()
{
    QtiSasAiContent content;
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/PorodFit\", [\"Q\"])\n"
                         "f.selectStage(\"simulate\")\n"
                         "f.simulateCurve()\n";
    const QString violation = content.splitPartScopeViolation(
        code, "simulate a curve from the compiled Porod power-law function named 'ai/PorodFit'", QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction14_allowsPastTenseAdjectiveDescriptionWithoutSimulateVerb()
{
    // "plot the SIMULATED Porod curve" describes an object an EARLIER part already produced —
    // past-tense adjective, not an instruction for THIS part to perform the simulation itself.
    // Must not be flagged just for containing "simulated".
    QtiSasAiContent content;
    const QString code =
        "g = newGraph(\"PorodSim\")\ng.insertCurve(table(\"simulatedCurve-ai/PorodFit\"), \"x\", \"y\")\n";
    const QString violation = content.splitPartScopeViolation(
        code,
        "plot the simulated Porod curve named 'ai/PorodFit' into graph 'PorodSim' with log-log scale "
        "and proper axis labels",
        QString());
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction15_blocksCrossPartVariableLeakage()
{
    // Confirmed live: part 3 computed best_fc/best_name; part 4 referenced both directly with
    // no local assignment of either, hitting NameError identically across multiple retries.
    QtiSasAiContent content;
    const QString earlierPartsCode = "chi2_g = f.fit()\nbest_fc = table(\"fitCurve-\" + best_name)\n"
                                     "best_name = \"GaussPeakModel\"\n";
    const QString code = "gw = newGraph(\"GaussPeakFit\", 2, 2, 1)\n"
                         "gw.layer(1).insertCurve(best_fc, \"x\", \"y\", Graph.CurveType.Line)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot results into graph 'GaussPeakFit'", earlierPartsCode);
    QVERIFY(!violation.isEmpty());
    QVERIFY(violation.contains("best_fc"));
}

void AiContentTests::direction15_allowsWhenNameIsReassignedLocally()
{
    QtiSasAiContent content;
    const QString earlierPartsCode = "best_fc = table(\"fitCurve-GaussPeakModel\")\n";
    const QString code = "best_fc = table(\"fitCurve-GaussPeakModel\")\n" // re-derived here, not assumed
                         "gw = newGraph(\"GaussPeakFit\")\n"
                         "gw.activeLayer().insertCurve(best_fc, \"x\", \"y\", Graph.CurveType.Line)\n";
    const QString violation =
        content.splitPartScopeViolation(code, "plot results into graph 'GaussPeakFit'", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

void AiContentTests::direction15_allowsCommonShortAliasesAcrossParts()
{
    // t/f/c/g/l1/l2 are routinely and CORRECTLY reassigned fresh in nearly every part —
    // flagging those would be pure noise, not a real instance of this mistake.
    QtiSasAiContent content;
    const QString earlierPartsCode = "t = newTable(\"GaussPeak\", 200, 3)\nf = Fittable\n";
    const QString code = "t = table(\"GaussPeak\")\nf = Fittable\nf.configure(\"GaussPeakModel\", [\"GaussPeak_y\"])\n";
    const QString violation = content.splitPartScopeViolation(code, "fit the model", earlierPartsCode);
    QVERIFY(violation.isEmpty());
}

// ---------------------------------------------------------------------------
// Self-review fabrication backstops
// ---------------------------------------------------------------------------

void AiContentTests::giveUpFallback_detectsSingleLinePrint()
{
    QtiSasAiContent content;
    QVERIFY(content.isGiveUpFallbackScript("scriptPrint(\"Table 'GaussPeak' does not exist\")"));
}

void AiContentTests::giveUpFallback_rejectsMultiLineScript()
{
    QtiSasAiContent content;
    const QString code = "t = table(\"GaussPeak\")\nscriptPrint(str(t.colNames()))\n";
    QVERIFY(!content.isGiveUpFallbackScript(code));
}

void AiContentTests::unverifiedMissingClaim_flagsBareAssertion()
{
    QtiSasAiContent content;
    QVERIFY(content.isUnverifiedMissingClaim("scriptPrint(\"Table 'GaussPeak' not found\")"));
}

void AiContentTests::unverifiedMissingClaim_allowsRealVerificationCall()
{
    QtiSasAiContent content;
    // Single significant line, but it IS the live check itself — a bare existTable() call has no
    // separate "not found" text of its own, so this should not be flagged as an unverified claim.
    QVERIFY(!content.isUnverifiedMissingClaim("scriptPrint(str(existTable(\"GaussPeak\")))"));
}

void AiContentTests::unverifiedMissingClaim_flagsNonexistentExistCurveCall()
{
    QtiSasAiContent content;
    // existCurve() doesn't exist (calling it raises NameError) — a leftover mention of it must NOT count
    // as proof a live check ran.
    const QString script = "# checked existCurve(\"Sphere\") -> False\n"
                           "scriptPrint(\"Graph 'Sphere' not found\")";
    QVERIFY(content.isUnverifiedMissingClaim(script));
}

void AiContentTests::unverifiedMissingClaim_allowsRealMatrixVerificationCall()
{
    QtiSasAiContent content;
    // matrix("name") is not None is the real, documented existence check for a Matrix.
    const QString script = "# checked matrix(\"Sphere\") is None\n"
                           "scriptPrint(\"Matrix 'Sphere' not found\")";
    QVERIFY(!content.isUnverifiedMissingClaim(script));
}

void AiContentTests::functionNameSubstitution_blocksUngroundedRename()
{
    // Confirmed live: self-review rewrote a correct, EXPLORE-verified "SphereSANS" into the
    // plausible-sounding but never-discovered "Sphere" — guaranteed "function not found" next run.
    QtiSasAiContent content;
    const QString preReviewDraft = "f = Fittable\nf.configure(\"SphereSANS\", [\"SAXS_data_I\"])\nf.fit()\n";
    const QString postReviewCode = "f = Fittable\nf.configure(\"Sphere\", [\"SAXS_data_I\"])\nf.fit()\n";
    QVERIFY(!content.selfReviewFunctionNameSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::functionNameSubstitution_allowsUnchangedName()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "f = Fittable\nf.configure(\"SphereSANS\", [\"SAXS_data_I\"])\nf.fit()\n";
    const QString postReviewCode = "f = Fittable\nf.configure(\"SphereSANS\", [\"SAXS_data_I\"])\nf.fit()\n";
    QVERIFY(content.selfReviewFunctionNameSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::functionNameSubstitution_ignoresCompilerSetFunctionName()
{
    // setFunctionName() (Compiler, naming a NEW function being compiled) must not be confused with
    // setFunction()/configure() (Fittable, selecting an EXISTING one) — a change here is expected
    // and legitimate (the whole point of compiling), not a hallucinated substitution.
    QtiSasAiContent content;
    const QString preReviewDraft = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakFit\")\nc.compile()\n";
    const QString postReviewCode = "c = Compiler\nc.setFunctionName(\"ai/GaussPeakModel\")\nc.compile()\n";
    QVERIFY(content.selfReviewFunctionNameSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::creationNameSubstitution_blocksDroppedUnderscore()
{
    // Confirmed live: a request naming the table "SAXS_data" survived correctly into the
    // pre-review draft; self-review's rewrite silently dropped the underscore ("SAXSdata") in both
    // the newTable() call and an unrelated scriptPrint() confirmation string in the same script.
    QtiSasAiContent content;
    const QString preReviewDraft = "t = newTable(\"SAXS_data\", 200, 3)\n"
                                   "scriptPrint(\"Table 'SAXS_data' created.\")\n";
    const QString postReviewCode = "t = newTable(\"SAXSdata\", 200, 3)\n"
                                   "scriptPrint(\"Table 'SAXSdata' created.\")\n";
    QVERIFY(!content.selfReviewCreationNameSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::creationNameSubstitution_allowsUnchangedName()
{
    QtiSasAiContent content;
    const QString code = "t = newTable(\"SAXS_data\", 200, 3)\nscriptPrint(\"done\")\n";
    QVERIFY(content.selfReviewCreationNameSubstitution(code, code).isEmpty());
}

void AiContentTests::creationNameSubstitution_ignoresDifferentObjectKinds()
{
    // A script creating a DIFFERENT kind of object between draft and review (e.g. self-review
    // legitimately adding a matrix nothing in the draft ever mentioned) must not false-trigger —
    // each creation kind is compared independently, never against a different kind's name.
    QtiSasAiContent content;
    const QString preReviewDraft = "t = newTable(\"SAXS_data\", 200, 3)\n";
    const QString postReviewCode = "t = newTable(\"SAXS_data\", 200, 3)\nm = newMatrix(\"Extra\", 10, 10)\n";
    QVERIFY(content.selfReviewCreationNameSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::exploreGroupsIgnoredMistake_blocksUnmatchedGroup()
{
    // Confirmed live, twice: EXPLORE printed the real existing groups via c.groups(), and the very
    // next round's write action called setGroup("custom:sans") anyway, matching none of them.
    QtiSasAiContent content;
    const QString exploreOutput =
        "Available groups: ['Custom', 'CustomModels', 'DLS Test', 'Form Factors', 'Lorentzian', "
        "'NIST-StRD', 'NSE', 'Polymer', 'SANS', 'Scatt. Laws']";
    const QString code = "c = Compiler\nc.new()\nc.setGroup(\"custom:sans\")\nc.compile()\n";
    const QString explanation = content.exploreGroupsIgnoredMistake(code, exploreOutput);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("Form Factors"));
    QVERIFY(explanation.contains("custom:sans"));
}

void AiContentTests::exploreGroupsIgnoredMistake_allowsReusedGroup()
{
    // A chosen group that DOES appear in what EXPLORE printed is a legitimate reuse — no false
    // positive.
    QtiSasAiContent content;
    const QString exploreOutput = "existing groups: ['Custom', 'Form Factors', 'SANS']";
    const QString code = "c = Compiler\nc.new()\nc.setGroup(\"Form Factors\")\nc.compile()\n";
    QVERIFY(content.exploreGroupsIgnoredMistake(code, exploreOutput).isEmpty());
}

void AiContentTests::exploreGroupsIgnoredMistake_allowsNoSetGroupCall()
{
    // Nothing to check when the script never calls setGroup() at all.
    QtiSasAiContent content;
    const QString exploreOutput = "existing groups: ['Custom', 'Form Factors', 'SANS']";
    const QString code = "c = Compiler\nc.open(\"ExistingFn\")\nc.compile()\n";
    QVERIFY(content.exploreGroupsIgnoredMistake(code, exploreOutput).isEmpty());
}

void AiContentTests::exploreGroupsIgnoredMistake_allowsExploreOutputWithNoGroupsListing()
{
    // An EXPLORE round that checked something unrelated (e.g. parameter names) never gets treated
    // as if it had covered groups — a brand-new group name is a legitimate answer here.
    QtiSasAiContent content;
    const QString exploreOutput = "sphere paramNames: ['scale', 'background', 'radius']";
    const QString code = "c = Compiler\nc.new()\nc.setGroup(\"custom:sans\")\nc.compile()\n";
    QVERIFY(content.exploreGroupsIgnoredMistake(code, exploreOutput).isEmpty());
}

void AiContentTests::falseIncompletenessClaim_blocksGiveUpAfterRealTableCreation()
{
    // Confirmed live: a part correctly created and filled table "MeshData"; self-review's give-up
    // claimed the table creation "does not perform the requested plotting job" — judging this
    // part against a LATER part's (plotting) job instead of its own creation job.
    QtiSasAiContent content;
    const QString preReviewDraft = "t = newTable(\"MeshData\", 2500, 3)\n"
                                   "t.setColNames([\"x\", \"y\", \"z\"])\n"
                                   "t.notifyChanges()\n";
    const QString postReviewCode = "scriptPrint(\"The table creation is incomplete and does not perform the requested "
                                   "plotting job. The task for this part is not performed.\")";
    QVERIFY(!content.selfReviewFalseIncompletenessClaim(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::falseIncompletenessClaim_allowsGenuineGiveUpWithNoRealWork()
{
    // No creation call anywhere in the pre-review draft — nothing contradicts the give-up claim,
    // so this must NOT be flagged (avoids blocking a genuine, correct give-up).
    QtiSasAiContent content;
    const QString preReviewDraft = "x = 1 + 1\nscriptPrint(str(x))\n";
    const QString postReviewCode = "scriptPrint(\"This part's job is not performed — no data was available.\")";
    QVERIFY(content.selfReviewFalseIncompletenessClaim(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::falseIncompletenessClaim_allowsNonGiveUpScript()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "t = newTable(\"MeshData\", 2500, 3)\n";
    const QString postReviewCode = "t = newTable(\"MeshData\", 2500, 3)\nt.notifyChanges()\n";
    QVERIFY(content.selfReviewFalseIncompletenessClaim(preReviewDraft, postReviewCode).isEmpty());
}

// ---------------------------------------------------------------------------
// Compile self-shadow / fitCurve name-reconstruction checks
// ---------------------------------------------------------------------------

void AiContentTests::selfShadowBug_detectsSelfReferentialDeclaration()
{
    const QString code = "double x0 = center;\ndouble gamma = gamma;\ndouble A = scale;\n";
    QVERIFY(!QtiSasAiContent::compiledCodeSelfShadowBug(code).isEmpty());
}

void AiContentTests::selfShadowBug_allowsNormalAliasing()
{
    const QString code = "double x0 = center;\ndouble sigma = width;\ndouble A = scale;\n";
    QVERIFY(QtiSasAiContent::compiledCodeSelfShadowBug(code).isEmpty());
}

void AiContentTests::explorePrefix_truncatesIndentedWriteCall()
{
    // Confirmed live: a complete configure+fit nested in else:/try: was never truncated, so the
    // whole fit ran under the # EXPLORE marker and the "already ran the action script" shortcut
    // then skipped self-review and every known-mistake check.
    QtiSasAiContent content;
    const QString code = "# EXPLORE: verify the table\n"
                         "if not existTable(\"SAXS_data\"):\n"
                         "    scriptPrint(\"missing\")\n"
                         "else:\n"
                         "    try:\n"
                         "        f.configure(\"sasviewmodels/sphere\", [\"SAXS_data_I\"])\n"
                         "        chi2 = f.fit()\n"
                         "    except Exception as e:\n"
                         "        scriptPrint(str(e))\n";
    const QString prefix = content.explorePrefixBeforeFirstWriteCall(code);
    QVERIFY(prefix.length() < code.length());
    QVERIFY(prefix.contains("scriptPrint(\"missing\")"));
    QVERIFY(!prefix.contains("f.configure"));
    QVERIFY(!prefix.contains("f.fit()"));
    // Cut back to the enclosing "else:", so what remains is still valid Python — it has to be,
    // because this prefix is executed.
    QVERIFY(!prefix.contains("else:"));
    QVERIFY(!prefix.contains("try:"));
}

void AiContentTests::explorePrefix_ignoresWriteCallNameInsideString()
{
    // The column-0 guard's real purpose — a quoted API name is not a call. Now handled by matching
    // against the string-literal-stripped code instead of by refusing indented lines.
    QtiSasAiContent content;
    const QString code = "# EXPLORE: read a note's script\n"
                         "n = note(\"Batch\")\n"
                         "scriptPrint(\"the tab calls newTable(...) on load\")\n";
    QCOMPARE(content.explorePrefixBeforeFirstWriteCall(code), code);
}

void AiContentTests::direction15_allowsParamNameMatchingEarlierPartVariable()
{
    // Confirmed live: part 1 used a Python `sigma = 1.0` for synthetic data; part 2 named a fit
    // parameter "sigma" in setParamNamesAll and used it as a C++ identifier inside setCode().
    // Neither is a Python variable reference, but the leakage check flagged it — unsatisfiable,
    // and it ended a 12-item run.
    QtiSasAiContent content;
    const QString earlierPartsCode = "sigma = 1.0\nmu = 10.0\nfor i in range(1, 201):\n"
                                     "    y = exp(-((x - mu) ** 2) / (2 * sigma ** 2))\n";
    const QString code = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/Gaussian\")\n"
                         "c.setParamNamesAll([\"scale\", \"center\", \"sigma\", \"background\"])\n"
                         "c.setCode(\"\"\"\n  I = scale * exp(-pow((x - center) / sigma, 2.0) / 2.0);\n\"\"\")\n"
                         "c.compile()\n";
    QVERIFY(content
                .splitPartScopeViolation(code, "compile custom Gaussian peak fit function named 'ai/Gaussian'",
                                         earlierPartsCode)
                .isEmpty());
}

void AiContentTests::yNameRepair_fixesEverySetCodeNotJustTheFirst()
{
    // Confirmed live: a part compiling two functions had its first body repaired and the second
    // left with "double I = ..." intact, because only the first setCode() was matched.
    QtiSasAiContent content;
    const QString code = "c.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  double I = scale * exp(-x);\n\"\"\")\n"
                         "c.compile()\n"
                         "c.new()\nc.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  double I = scale * gamma / (x * x);\n\"\"\")\n";
    const QString repaired = content.compilerYNameAssignmentRepair(code);
    QVERIFY(!repaired.isEmpty());
    QVERIFY(!repaired.contains("double I"));
    QVERIFY(repaired.contains("I = scale * exp(-x);"));
    QVERIFY(repaired.contains("I = scale * gamma / (x * x);"));
}

void AiContentTests::yNameNeverAssigned_flagsSecondBodyWhenFirstIsFine()
{
    const QString code = "c.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  I = scale * exp(-x);\n\"\"\")\n"
                         "c.new()\n"
                         "c.setCode(\"\"\"\n  double y = scale / (x * x);\n\"\"\")\n";
    QVERIFY(!QtiSasAiContent::compilerYNameNeverAssignedMistake(code).isEmpty());
}

void AiContentTests::yNameRepair_dropsTypeKeywordOnYNameDeclaration()
{
    // Round 1 of the confirmed-live loop: "double I = ..." with setYName("I") -> clang redefinition.
    QtiSasAiContent content;
    const QString code = "c.setYName(\"I\")\nc.setXName(\"q\")\n"
                         "c.setCode(\"\"\"\n  double I = scale * pow(q, -exponent) + background;\n\"\"\")\n";
    const QString repaired = content.compilerYNameAssignmentRepair(code);
    QVERIFY(!repaired.isEmpty());
    QVERIFY(repaired.contains("I = scale * pow(q, -exponent) + background;"));
    QVERIFY(!repaired.contains("double I"));
}

void AiContentTests::yNameRepair_retargetsRenamedDeclaration()
{
    // Round 2: renamed to "double y = ..." so nothing assigns I — compiles clean, returns garbage.
    QtiSasAiContent content;
    const QString code = "c.setYName(\"I\")\nc.setXName(\"q\")\n"
                         "c.setCode(\"\"\"\n  double y = scale * pow(q, -exponent) + background;\n\"\"\")\n";
    const QString repaired = content.compilerYNameAssignmentRepair(code);
    QVERIFY(!repaired.isEmpty());
    QVERIFY(repaired.contains("I = scale * pow(q, -exponent) + background;"));
    QVERIFY(!repaired.contains("double y"));
}

void AiContentTests::yNameRepair_refusesWithSeveralDeclarations()
{
    // Which local was meant to be the result is a guess here — leave it to the existing check.
    QtiSasAiContent content;
    const QString code = "c.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  double V = 2.0;\n  double y = scale * V;\n\"\"\")\n";
    QVERIFY(content.compilerYNameAssignmentRepair(code).isEmpty());
}

void AiContentTests::yNameRepair_leavesCorrectBodyAlone()
{
    QtiSasAiContent content;
    const QString code = "c.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  double V = 2.0;\n  I = scale * V;\n\"\"\")\n";
    QVERIFY(content.compilerYNameAssignmentRepair(code).isEmpty());
}

void AiContentTests::yNameRepair_renamesMisnamedAssignment()
{
    // LIST 5, items 1 and 2: setYName("y") with "I = ...;" (I is the default yName) — blocked, a round each time.
    QtiSasAiContent content;
    const QString head = "c.setYName(\"y\")\nc.setXName(\"x\")\nc.setParamNamesAll([\"A\", \"k\", \"c\"])\n";
    QString repaired = content.compilerYNameAssignmentRepair(head + "c.setCode(\"I = A*x*x + k*x + c;\")\n");
    QVERIFY(repaired.contains("setCode(\"y = A*x*x + k*x + c;\")"));
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(repaired).isEmpty());
    // with a helper local read twice: the declaration repair refuses, the rename does it
    repaired = content.compilerYNameAssignmentRepair(
        head + "c.setCode(\"\"\"\n  double e = exp(-k * x);\n  I = A * e + c * e;\n\"\"\")\n");
    QVERIFY(repaired.contains("  y = A * e + c * e;"));
    QVERIFY(repaired.contains("double e = exp(-k * x);"));
    // a parameter assigned is no misnamed result, and two candidates are a guess
    QVERIFY(content.compilerYNameAssignmentRepair(head + "c.setCode(\"A = 2.0;\")\n").isEmpty());
    QVERIFY(content.compilerYNameAssignmentRepair(head + "c.setCode(\"I = A; J = k;\")\n").isEmpty());
    // a name also read later is more than a misnamed result
    QVERIFY(content.compilerYNameAssignmentRepair(head + "c.setCode(\"t = A*x; t2 = t;\")\n").isEmpty());
}

void AiContentTests::paramCountRepair_correctsCountToAgreedListLength()
{
    // The confirmed-live loop: setParamCount(4) with five 3-entry lists, retried byte-identical
    // twice against an explicit ValueError, discarding items 8-12 of a LIST.
    QtiSasAiContent content;
    const QString code = "c.setParamCount(4)\n"
                         "c.setParamNamesAll([\"scale\", \"n\", \"background\"])\n"
                         "c.setParamInitsAll([1.0, 3.0, 0.0])\n"
                         "c.setParamLimitsAll([\"0..\", \"1..8\", \"..\"])\n"
                         "c.setParamVariesAll([True, True, True])\n"
                         "c.setParamDescrsAll([\"scale factor\", \"exponent n\", \"flat background\"])\n";
    const QString repaired = content.compilerParamCountRepair(code);
    QVERIFY(!repaired.isEmpty());
    QVERIFY(repaired.contains("setParamCount(3)"));
    QVERIFY(!repaired.contains("setParamCount(4)"));
    // Everything else must be untouched.
    QVERIFY(repaired.contains("c.setParamNamesAll([\"scale\", \"n\", \"background\"])"));
}

void AiContentTests::paramCountRepair_leavesConsistentCountAlone()
{
    QtiSasAiContent content;
    const QString code = "c.setParamCount(3)\n"
                         "c.setParamNamesAll([\"scale\", \"n\", \"background\"])\n"
                         "c.setParamInitsAll([1.0, 3.0, 0.0])\n";
    QVERIFY(content.compilerParamCountRepair(code).isEmpty());
}

void AiContentTests::paramCountRepair_refusesWhenListsDisagree()
{
    // Which list reflects the intent is a guess here — leave it to the runtime error.
    QtiSasAiContent content;
    const QString code = "c.setParamCount(4)\n"
                         "c.setParamNamesAll([\"scale\", \"n\", \"background\"])\n"
                         "c.setParamInitsAll([1.0, 3.0, 0.0, 0.5])\n";
    QVERIFY(content.compilerParamCountRepair(code).isEmpty());
}

void AiContentTests::paramCountRepair_ignoresScriptWithoutParamCount()
{
    QtiSasAiContent content;
    QVERIFY(content.compilerParamCountRepair("t = newTable(\"T\", 5, 2)\n").isEmpty());
}

void AiContentTests::discoveredName_detectsWildcardSearchInsteadOfRequestedName()
{
    // The confirmed-live case: request named sasviewmodels/sphere, script searched "*sphere*" and
    // configured hits[0] -> Core-Shell-Sphere-ShellPD-GH-5pt.
    const QString request = R"(Fit the existing "sasviewmodels/sphere" form-factor model against table "SAXS_data".)";
    const QString code = "f = Fittable\n"
                         "hits = [h for h in f.fitFunctions(\"*sphere*\") if not h.startswith(\"ai/\")]\n"
                         "model_name = hits[0]\n"
                         "f.configure(model_name, [\"SAXS_data_I\"])\n";
    const QString explanation = QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(code, request);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("sasviewmodels/sphere"));
}

void AiContentTests::discoveredName_allowsRequestedNameUsedVerbatim()
{
    const QString request = R"(Fit the existing "sasviewmodels/sphere" form-factor model against table "SAXS_data".)";
    const QString code = "f = Fittable\n"
                         "f.configure(\"sasviewmodels/sphere\", [\"SAXS_data_I\"])\n";
    QVERIFY(QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(code, request).isEmpty());
}

void AiContentTests::discoveredName_allowsExactCheckWithWildcardFallback()
{
    // The script that finally ran live and fitted the RIGHT model: exact-match first, wildcard only
    // as an untaken fallback. Names the requested model verbatim, so this must stay silent.
    const QString request = R"(Fit the existing "sasviewmodels/sphere" form-factor model against table "SAXS_data".)";
    const QString code = "f = Fittable\n"
                         "requested_model = \"sasviewmodels/sphere\"\n"
                         "if requested_model in f.fitFunctions():\n"
                         "    model_name = requested_model\n"
                         "else:\n"
                         "    hits = [h for h in f.fitFunctions(\"*sphere*\") if not h.startswith(\"ai/\")]\n"
                         "    model_name = hits[0]\n"
                         "f.configure(model_name, [\"SAXS_data_I\"])\n";
    QVERIFY(QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(code, request).isEmpty());
}

void AiContentTests::discoveredName_allowsWildcardSearchWithoutConfigure()
{
    // A pure listing/exploration search that never configures anything is legitimate.
    const QString request = R"(Fit the existing "sasviewmodels/sphere" form-factor model against table "SAXS_data".)";
    const QString code = "f = Fittable\nscriptPrint(str(f.fitFunctions(\"*sphere*\")))\n";
    QVERIFY(QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(code, request).isEmpty());
}

void AiContentTests::discoveredName_ignoresRequestNamingNoModel()
{
    const QString code = "f = Fittable\n"
                         "hits = f.fitFunctions(\"*sphere*\")\n"
                         "f.configure(hits[0], [\"SAXS_data_I\"])\n";
    QVERIFY(QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(code, "Fit something reasonable.").isEmpty());
}

void AiContentTests::yNameNeverAssigned_detectsRenamedVariable()
{
    // The confirmed-live silent break: clang's "redefinition of 'I'" was "fixed" by renaming to
    // double y, which compiles clean while I is never assigned.
    const QString code = "c.setYName(\"I\")\nc.setXName(\"x\")\n"
                         "c.setCode(\"\"\"\n  double y = scale * pow(x, -exponent) + background;\n\"\"\")\n";
    const QString explanation = QtiSasAiContent::compilerYNameNeverAssignedMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("UNINITIALIZED"));
}

void AiContentTests::yNameNeverAssigned_allowsPlainAssignment()
{
    const QString code = "c.setYName(\"Intensity\")\n"
                         "c.setCode(\"\"\"\n  double V = 2.0;\n  Intensity = scale * V;\n\"\"\")\n";
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(code).isEmpty());
}

void AiContentTests::yNameNeverAssigned_allowsCompoundAssignment()
{
    const QString code = "c.setYName(\"I\")\n"
                         "c.setCode(\"\"\"\n  I = 0.0;\n  I += scale * x;\n\"\"\")\n";
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(code).isEmpty());
}

void AiContentTests::yNameNeverAssigned_ignoresWhenYNameNotStated()
{
    // After c.load()/open() the real yName is unknown here — stay silent rather than false-block.
    const QString code = "c.open(\"SphereFit\")\nc.setCode(\"\"\"\n  I = scale * x;\n\"\"\")\n";
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(code).isEmpty());
}

void AiContentTests::redeclaredReservedName_ignoresYNameOnlyClash()
{
    // Deliberately NOT flagged: clang reports "error: redefinition of 'I'" naming the line, and the
    // normal error-retry fixes it. Blocking it here was confirmed live to dead-end a whole LIST.
    const QString code = "c.setYName(\"I\")\nc.setXName(\"x\")\n"
                         "c.setCode(\"\"\"\n  double I = scale * pow(x, -exponent) + background;\n\"\"\")\n";
    QVERIFY(QtiSasAiContent::compilerRedeclaredReservedNameMistake(code).isEmpty());
}

void AiContentTests::redeclaredReservedName_detectsSilentParameterShadowing()
{
    const QString code = "c.setYName(\"I\")\nc.setParamNamesAll([\"scale\", \"exponent\", \"background\"])\n"
                         "c.setCode(\"\"\"\n  double exponent = 4.0;\n  I = scale * pow(x, -exponent);\n\"\"\")\n";
    const QString explanation = QtiSasAiContent::compilerRedeclaredReservedNameMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("shadows"));
}

void AiContentTests::redeclaredReservedName_allowsOwnIntermediateLocals()
{
    // Documented as correct usage (compile-python-api.md): intermediates of the script's own
    // naming must never be flagged — only names the boilerplate already declares.
    const QString code = "c.setYName(\"I\")\nc.setXName(\"x\")\n"
                         "c.setParamNamesAll([\"scale\", \"radius\", \"background\"])\n"
                         "c.setCode(\"\"\"\n  double V = M_PI * radius * radius;\n"
                         "  double pq = V * V;\n  I = scale * pq + background;\n\"\"\")\n";
    QVERIFY(QtiSasAiContent::compilerRedeclaredReservedNameMistake(code).isEmpty());
}

void AiContentTests::fitCurveMismatch_detectsBareReconstructionAfterQualifiedCheck()
{
    const QString code = "if existTable(\"fitCurve-ai/GaussPeakModel\"):\n"
                         "    best_model = \"GaussPeakModel\"\n"
                         "fc = table(\"fitCurve-\" + best_model)\n";
    QVERIFY(!QtiSasAiContent::fitCurveNameReconstructionMismatch(code).isEmpty());
}

void AiContentTests::fitCurveMismatch_allowsConsistentQualifiedUsage()
{
    const QString code = "if existTable(\"fitCurve-ai/GaussPeakModel\"):\n"
                         "    fc = table(\"fitCurve-ai/GaussPeakModel\")\n";
    QVERIFY(QtiSasAiContent::fitCurveNameReconstructionMismatch(code).isEmpty());
}

void AiContentTests::autoexecImmediateRun_blocksUnrequestedExecute()
{
    // Confirmed live: a plain "make it auto-run when the project opens" request got setAutoexec()
    // AND an immediate n.tab(1).execute() the AI never asked for.
    const QString code = "n = newNote(\"Setup\")\n"
                         "n.setAutoexec(True)\n"
                         "n.setText(\"print('configured')\")\n"
                         "n.tab(1).execute()\n";
    const QString request = "Create a note that configures analysis defaults when the project file is opened.";
    QVERIFY(!QtiSasAiContent::unrequestedAutoexecImmediateRun(code, request).isEmpty());
}

void AiContentTests::autoexecImmediateRun_allowsWhenRequestAsksToRunNow()
{
    const QString code = "n = newNote(\"Setup\")\n"
                         "n.setAutoexec(True)\n"
                         "n.setText(\"print('configured')\")\n"
                         "n.tab(1).execute()\n";
    const QString request =
        "Create a note that auto-runs on project open, and also run it right now to confirm it works.";
    QVERIFY(QtiSasAiContent::unrequestedAutoexecImmediateRun(code, request).isEmpty());
    // benchmark LIST 1 item 4 (Codex): blocked twice although the request asked for exactly this run
    QVERIFY(QtiSasAiContent::unrequestedAutoexecImmediateRun(
                code, "Create a note \"Setup\" with an auto-executing script tab ... Run that tab once to test it, "
                      "then print whether automatic recalculation is switched on now.")
                .isEmpty());
    QVERIFY(QtiSasAiContent::unrequestedAutoexecImmediateRun(code, "Make it auto-run, and execute it once.").isEmpty());
}

void AiContentTests::autoexecImmediateRun_allowsAutoexecWithoutExecute()
{
    // Matches note-python-api.md's own documented "Auto-execute startup script" pattern exactly.
    const QString code = "n = newNote(\"Startup\")\n"
                         "n.setText(\"print('configured')\")\n"
                         "n.setAutoexec(True)\n";
    const QString request = "Create a note that configures analysis defaults when the project file is opened.";
    QVERIFY(QtiSasAiContent::unrequestedAutoexecImmediateRun(code, request).isEmpty());
}

void AiContentTests::zeroBasedIndex_blocksZeroColumnInSetColumnRole()
{
    // Confirmed live: self-review rewrote a correct 1-based setColumnRole(1,...)/(2,...)/(3,...)
    // to 0-based setColumnRole(0,...)/(1,...)/(2,...) — "no column 0 in table" ValueError.
    const QString code = "t = newTable(\"SAXS_data\", 200, 3)\n"
                         "t.setColumnRole(0, Table.PlotDesignation.X)\n";
    QVERIFY(!QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
}

void AiContentTests::zeroBasedIndex_blocksZeroColumnInSetCell()
{
    const QString code = "t.setCell(0, 1, 3.14)\n";
    QVERIFY(!QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
}

void AiContentTests::zeroBasedIndex_blocksZeroRowInSetCell()
{
    // Confirmed live: the same script's row argument was also converted to 0-based (`i - 1` where
    // the loop variable already started at 1), raising "no row 0 in table" on the very next fix.
    const QString code = "t.setCell(1, 0, 3.14)\n";
    QVERIFY(!QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
}

void AiContentTests::zeroBasedIndex_allowsCorrectOneBasedIndices()
{
    const QString code = "t = newTable(\"SAXS_data\", 200, 3)\n"
                         "t.setColumnRole(1, Table.PlotDesignation.X)\n"
                         "t.setColumnRole(2, Table.PlotDesignation.Y)\n"
                         "t.setColumnRole(3, Table.PlotDesignation.yErr)\n"
                         "t.setCell(1, 1, 3.14)\n";
    QVERIFY(QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
}

void AiContentTests::zeroBasedIndex_allowsMatrixOneBasedIndices()
{
    const QString code = "m = newMatrix(\"SphereMap\", 50, 50)\n"
                         "m.setCell(1, 1, 3.14)\n";
    QVERIFY(QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
}

void AiContentTests::changeFolderNoOp_blocksBareActiveFolderRoundTrip()
{
    // Confirmed live: generated as a fabricated attempt to "activate" a just-created Note's tab.
    const QString code = "changeFolder(activeFolder())\n";
    QVERIFY(!QtiSasAiContent::changeFolderNoOpMistake(code).isEmpty());
}

void AiContentTests::changeFolderNoOp_allowsForceTrueSecondArgument()
{
    // force=True makes this a real, meaningful refresh, not a no-op.
    const QString code = "changeFolder(activeFolder(), True)\n";
    QVERIFY(QtiSasAiContent::changeFolderNoOpMistake(code).isEmpty());
}

void AiContentTests::changeFolderNoOp_allowsChangeFolderToADifferentFolder()
{
    const QString code = "results = addFolder(\"Results\")\nchangeFolder(results)\n";
    QVERIFY(QtiSasAiContent::changeFolderNoOpMistake(code).isEmpty());
}

void AiContentTests::tableCellSwap_blocksLoopVarFirstLiteralSecond()
{
    // Confirmed live: t.setCell(i+1, 1, q[i]) inside "for i in range(100):" — the row-loop
    // variable landed in the (col, row) API's FIRST (column) position.
    const QString code = "t = table(\"SAXS_data\")\n"
                         "for i in range(100):\n"
                         "\tt.setCell(i+1, 1, q[i])\n";
    QVERIFY(!QtiSasAiContent::tableCellRowColSwapMistake(code).isEmpty());
}

void AiContentTests::tableCellSwap_blocksWithNewTableAssignment()
{
    const QString code = "t = newTable(\"SAXS_data\", 100, 3)\n"
                         "for i in range(100):\n"
                         "\tt.setCellData(i, 2, value)\n";
    QVERIFY(!QtiSasAiContent::tableCellRowColSwapMistake(code).isEmpty());
}

void AiContentTests::tableCellSwap_allowsCorrectColFirstOrder()
{
    const QString code = "t = table(\"SAXS_data\")\n"
                         "for i in range(100):\n"
                         "\tt.setCell(1, i+1, q[i])\n";
    QVERIFY(QtiSasAiContent::tableCellRowColSwapMistake(code).isEmpty());
}

void AiContentTests::tableCellSwap_allowsNonTableReceiver()
{
    // Same call shape, but the receiver was never assigned from table()/newTable()/currentTable()
    // — cell()/text() are also Matrix methods with the OPPOSITE (row, col) order, so a Matrix
    // instance correctly using a row-loop variable first must not be flagged.
    const QString code = "m = matrix(\"M\")\n"
                         "for i in range(100):\n"
                         "\tm.setCell(i, 1, value)\n";
    QVERIFY(QtiSasAiContent::tableCellRowColSwapMistake(code).isEmpty());
}

void AiContentTests::setCodeNestedFunction_blocksInlineDefinition()
{
    const QString code = "c.setCode(\"\"\"\n"
                         "double Iq(double q, double scale)\n"
                         "{\n"
                         "    return scale;\n"
                         "}\n"
                         "\"\"\")\n";
    QVERIFY(!QtiSasAiContent::compilerSetCodeNestedFunctionMistake(code).isEmpty());
}

void AiContentTests::setCodeNestedFunction_blocksViaVariableIndirection()
{
    const QString code = "code = \"\"\"\n"
                         "double Iq(double q, double scale)\n"
                         "{\n"
                         "    return scale;\n"
                         "}\n"
                         "\"\"\"\n"
                         "c.setCode(code)\n";
    QVERIFY(!QtiSasAiContent::compilerSetCodeNestedFunctionMistake(code).isEmpty());
}

void AiContentTests::setCodeNestedFunction_allowsDirectAssignment()
{
    const QString code = "c.setCode(\"y = scale * exp(-0.5 * pow((x - center) / sigma, 2.0)) + background;\")\n";
    QVERIFY(QtiSasAiContent::compilerSetCodeNestedFunctionMistake(code).isEmpty());
}

void AiContentTests::compilerFittableOnlyMethod_blocksSetParamLimitsOnCompiler()
{
    const QString code = "c = Compiler\nc.setParamLimits(\"scale\", 0.0, 1.0)\n";
    QVERIFY(!QtiSasAiContent::compilerFittableOnlyMethodMistake(code).isEmpty());
}

void AiContentTests::compilerFittableOnlyMethod_blocksSetParamSharedOnCompiler()
{
    const QString code = "c = Compiler\nc.setParamShared(\"scale\", True)\n";
    QVERIFY(!QtiSasAiContent::compilerFittableOnlyMethodMistake(code).isEmpty());
}

void AiContentTests::compilerFittableOnlyMethod_allowsOnFittableReceiver()
{
    const QString code = "f = Fittable\nf.setParamLimits(\"scale\", 0.0, 1.0)\n";
    QVERIFY(QtiSasAiContent::compilerFittableOnlyMethodMistake(code).isEmpty());
}

void AiContentTests::setParamNamesMissingAll_blocksBareCall()
{
    const QString code = "c.setParamNames([\"scale\", \"center\"])\n";
    QVERIFY(!QtiSasAiContent::setParamNamesMissingAllMistake(code).isEmpty());
}

void AiContentTests::setParamNamesMissingAll_allowsRealAllVariant()
{
    const QString code = "c.setParamNamesAll([\"scale\", \"center\"])\n";
    QVERIFY(QtiSasAiContent::setParamNamesMissingAllMistake(code).isEmpty());
}

void AiContentTests::compilerNonexistentMethod_blocksSetFunctionCode()
{
    const QString code = "c.setFunctionCode(gauss_code)\n";
    QVERIFY(!QtiSasAiContent::compilerNonexistentMethodMistake(code).isEmpty());
}

void AiContentTests::compilerNonexistentMethod_blocksCompileWithNameArgument()
{
    const QString code = "c.compile(\"GaussModel\")\n";
    QVERIFY(!QtiSasAiContent::compilerNonexistentMethodMistake(code).isEmpty());
}

void AiContentTests::compilerNonexistentMethod_allowsPlainCompileCall()
{
    const QString code = "c.setFunctionName(\"ai/GaussModel\")\nc.compile()\n";
    QVERIFY(QtiSasAiContent::compilerNonexistentMethodMistake(code).isEmpty());
}

void AiContentTests::compilerSetCodeCppSyntax_blocksDoubleStarInVariableIndirection()
{
    const QString code = "code = \"\"\"\n"
                         "double y = A * exp(-0.5 * ((x - x0) / sigma) ** 2) + bgd;\n"
                         "return y;\n"
                         "\"\"\"\n"
                         "c.setCode(code)\n";
    QVERIFY(!QtiSasAiContent::compilerSetCodeCppSyntaxMistake(code).isEmpty());
}

void AiContentTests::compilerSetCodeCppSyntax_blocksParaMIndexing()
{
    const QString code = "code = \"\"\"\n"
                         "double scale = ParaM[0];\n"
                         "y = scale;\n"
                         "\"\"\"\n"
                         "c.setCode(code)\n";
    QVERIFY(!QtiSasAiContent::compilerSetCodeCppSyntaxMistake(code).isEmpty());
}

void AiContentTests::compilerSetCodeCppSyntax_allowsPowAndNamedParameters()
{
    const QString code = "code = \"\"\"\n"
                         "y = scale * exp(-0.5 * pow((x - center) / sigma, 2.0)) + background;\n"
                         "\"\"\"\n"
                         "c.setCode(code)\n";
    QVERIFY(QtiSasAiContent::compilerSetCodeCppSyntaxMistake(code).isEmpty());
}

void AiContentTests::compilerMissingAiNamespace_blocksBareNameWithCompile()
{
    const QString code = "c.setFunctionName(\"GaussModel\")\nc.compile()\n";
    QVERIFY(!QtiSasAiContent::compilerMissingAiNamespaceMistake(code).isEmpty());
}

void AiContentTests::compilerMissingAiNamespace_allowsAiPrefixedName()
{
    const QString code = "c.setFunctionName(\"ai/GaussModel\")\nc.compile()\n";
    QVERIFY(QtiSasAiContent::compilerMissingAiNamespaceMistake(code).isEmpty());
}

void AiContentTests::compilerMissingAiNamespace_allowsWithoutCompileCall()
{
    const QString code = "c.setFunctionName(\"GaussModel\")\n";
    QVERIFY(QtiSasAiContent::compilerMissingAiNamespaceMistake(code).isEmpty());
}

void AiContentTests::plotDesignationCase_blocksLowercaseX()
{
    // Confirmed live: Table.PlotDesignation.x (lowercase) raised AttributeError — the real member
    // is Table.PlotDesignation.X (uppercase).
    const QString code = "t.setColumnRole(1, Table.PlotDesignation.x)\n";
    QVERIFY(!QtiSasAiContent::hallucinatedPlotDesignationCaseMistake(code).isEmpty());
}

void AiContentTests::plotDesignationCase_allowsRealMembers()
{
    const QString code = "t.setColumnRole(1, Table.PlotDesignation.X)\n"
                         "t.setColumnRole(2, Table.PlotDesignation.Y)\n"
                         "t.setColumnRole(3, Table.PlotDesignation.yErr)\n"
                         "t.setColumnRole(4, Table.PlotDesignation.Label)\n";
    QVERIFY(QtiSasAiContent::hallucinatedPlotDesignationCaseMistake(code).isEmpty());
}

void AiContentTests::staleFitStatistic_blocksHardcodedChi2WithNoFitOrFitCurve()
{
    // Confirmed live: a "compare chi2/dof from both fits" part pasted back the numbers an earlier
    // EXPLORE step in the SAME conversation had printed, instead of reading the fitCurve tables or
    // fitting again in this exact script.
    const QString code = "chi2_gauss = 1.24876841621\n"
                         "chi2_lorentz = 69.1182278334\n"
                         "if chi2_gauss <= chi2_lorentz:\n"
                         "    scriptPrint(\"Best model: ai/GaussPeakModel\")\n";
    QVERIFY(!QtiSasAiContent::staleFitStatisticMistake(code).isEmpty());
}

void AiContentTests::staleFitStatistic_allowsFreshFitCall()
{
    const QString code = "f = Fittable\nf.configure(\"ai/GaussPeakModel\", [\"GaussPeak_I\"])\n"
                         "chi2 = f.fit()\nscriptPrint(str(chi2))\n";
    QVERIFY(QtiSasAiContent::staleFitStatisticMistake(code).isEmpty());
}

void AiContentTests::staleFitStatistic_allowsFitCurveTableRead()
{
    const QString code = "fc = table(\"fitCurve-ai/GaussPeakModel\")\n"
                         "chi2_gauss = float(fc.text(7, 1))\n";
    QVERIFY(QtiSasAiContent::staleFitStatisticMistake(code).isEmpty());
}

void AiContentTests::staleFitStatistic_blocksHardcodedWinnerVarDespiteUnrelatedFitCall()
{
    // Confirmed live: a script that genuinely calls f.fit() for chi2_gauss/chi2_lorentz still
    // hardcoded a SEPARATE "winner" chi2 variable recalled from an earlier turn's printed output,
    // instead of deriving it from the fit results just computed in this same script.
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/GaussianPeakModel\", [\"GaussPeak_y\"])\n"
                         "chi2_gauss = f.fit()\n"
                         "f.configure(\"ai/LorentzianPeakModel\", [\"GaussPeak_y\"])\n"
                         "chi2_lorentz = f.fit()\n"
                         "chi2_winner = 1.001529\n"
                         "scriptPrint(\"Winner chi2/dof = %.6f\" % chi2_winner)\n";
    QVERIFY(!QtiSasAiContent::staleFitStatisticMistake(code).isEmpty());
}

void AiContentTests::staleFitStatistic_allowsHardcodedPlaceholderLaterReassignedLive()
{
    // A literal-looking placeholder init that's genuinely overwritten from a live source further
    // down is not the confirmed-live mistake — only a literal with no live reassignment is.
    const QString code = "chi2_best = 0.0\n"
                         "f = Fittable\n"
                         "f.configure(\"ai/GaussPeakModel\", [\"GaussPeak_I\"])\n"
                         "chi2_best = f.fit()\n";
    QVERIFY(QtiSasAiContent::staleFitStatisticMistake(code).isEmpty());
}

void AiContentTests::missingLegendCreation_blocksStyleOnlyIfNotNone()
{
    const QString code = "leg = l1.legend()\n"
                         "if leg is not None:\n"
                         "    leg.setFont(QFont(\"Arial\", 10))\n"
                         "    leg.setMargin(10)\n";
    QVERIFY(!QtiSasAiContent::missingLegendCreationMistake(code).isEmpty());
}

void AiContentTests::missingLegendCreation_allowsNewLegendFallback()
{
    const QString code = "leg = l1.legend()\n"
                         "if leg is None:\n"
                         "    leg = l1.newLegend()\n"
                         "leg.setFont(QFont(\"Arial\", 10))\n";
    QVERIFY(QtiSasAiContent::missingLegendCreationMistake(code).isEmpty());
}

void AiContentTests::missingLegendCreation_allowsPureReadNoStyling()
{
    // No styling calls on the legend variable at all — not the pattern this check targets.
    const QString code = "leg = l1.legend()\n"
                         "scriptPrint(\"has legend: \" + str(leg is not None))\n";
    QVERIFY(QtiSasAiContent::missingLegendCreationMistake(code).isEmpty());
}

void AiContentTests::modelNameSubstitution_blocksUnannouncedSwap()
{
    // Confirmed live: quoted "Sphere" silently swapped for "sasviewmodels/sphere", nothing printed.
    const QString request = "Fit the existing \"Sphere\" form-factor model from the Fittable "
                            "library against table \"SAXS_data\" columns I and dI.";
    const QString code = "f = Fittable\n"
                         "f.configure(\"sasviewmodels/sphere\", [\"SAXS_data_I\"])\n"
                         "chi2 = f.fit()\n"
                         "scriptPrint(f\"chi2/dof = {chi2:.4f}\")\n";
    QVERIFY(!QtiSasAiContent::unannouncedModelNameSubstitution(code, request).isEmpty());
}

void AiContentTests::modelNameSubstitution_allowsExactRequestedName()
{
    const QString request = R"(Fit the existing "GaussPeak" model against table "Data".)";
    const QString code = "f = Fittable\n"
                         "f.configure(\"GaussPeak\", [\"Data_I\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(QtiSasAiContent::unannouncedModelNameSubstitution(code, request).isEmpty());
}

void AiContentTests::modelNameSubstitution_allowsAnnouncedSwap()
{
    const QString request = R"(Fit the existing "Sphere" form-factor model against table "SAXS_data".)";
    const QString code = "f = Fittable\n"
                         "scriptPrint(\"Model 'Sphere' not found exactly — using closest match: "
                         "sasviewmodels/sphere\")\n"
                         "f.configure(\"sasviewmodels/sphere\", [\"SAXS_data_I\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(QtiSasAiContent::unannouncedModelNameSubstitution(code, request).isEmpty());
}

void AiContentTests::modelNameSubstitution_allowsNoQuotedModelInRequest()
{
    const QString request = "Fit a sphere form-factor model against table \"SAXS_data\".";
    const QString code = "f = Fittable\n"
                         "f.configure(\"sasviewmodels/sphere\", [\"SAXS_data_I\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(QtiSasAiContent::unannouncedModelNameSubstitution(code, request).isEmpty());
}

void AiContentTests::reuseInsteadOfCompile_blocksConfigureWithNoCompileCall()
{
    // Confirmed live TWICE independently: asked to "compile" a custom fit function, EXPLORE finds
    // a coincidentally-matching leftover name from an earlier session already registered in BOTH
    // Compiler.functionNames() and Fittable.fitFunctions(), and the action script configures/fits
    // it directly with no compile() call anywhere.
    const QString request = "Compile a custom Gaussian peak fit function and fit it with weighted errors.";
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/GaussPeakCustom\", [\"GaussPeak_y\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(!QtiSasAiContent::reuseInsteadOfCompileMistake(code, request, QString()).isEmpty());
}

void AiContentTests::reuseInsteadOfCompile_allowsRealCompileCall()
{
    const QString request = "Compile a custom Gaussian peak fit function and fit it with weighted errors.";
    const QString code = "c = Compiler\n"
                         "c.new()\n"
                         "c.setFunctionName(\"ai/GaussPeak\")\n"
                         "c.setGroup(\"custom:peak\")\n"
                         "c.setCode(\"y = scale * exp(-pow((x - center) / width, 2.0) / 2.0);\")\n"
                         "ok = c.compile()\n"
                         "f = Fittable\n"
                         "f.configure(\"ai/GaussPeak\", [\"GaussPeak_y\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(QtiSasAiContent::reuseInsteadOfCompileMistake(code, request, QString()).isEmpty());
}

void AiContentTests::reuseInsteadOfCompile_allowsFitOnlyRequestWithNoCompileWord()
{
    // A request that only asks to fit an existing/library function never mentions "compile" —
    // correctly exempt regardless of whether the script calls compile().
    const QString request = "Fit the Guinier model against table \"SAXS_data\".";
    const QString code = "f = Fittable\n"
                         "f.configure(\"Guinier\", [\"SAXS_data_I\"])\n"
                         "chi2 = f.fit()\n";
    QVERIFY(QtiSasAiContent::reuseInsteadOfCompileMistake(code, request, QString()).isEmpty());
}

void AiContentTests::aiSandboxReopen_blocksReopeningLeftoverForCompileRequest()
{
    // Benchmark LIST 3, item 8: the leftover ai/RefDecay was reopened and recompiled instead of defined as asked.
    const QString request = "Compile a new fit function \"ai/RefDecay\" for y = A*exp(-k*x) + c with all three "
                            "parameters adjustable.";
    const QString code = "c = Compiler\nc.open(\"ai/RefDecay\")\nok = c.compile()\n";
    QVERIFY(!QtiSasAiContent::aiSandboxReopenMistake(code, request, QString()).isEmpty());
}

void AiContentTests::aiSandboxReopen_allowsFreshDefinitionAndOwnEarlierPart()
{
    const QString request = "Compile a new fit function \"ai/RefDecay\" for y = A*exp(-k*x) + c.";
    const QString fresh = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/RefDecay\")\nc.setCode(\"y = A;\")\n"
                          "ok = c.compile()\n";
    QVERIFY(QtiSasAiContent::aiSandboxReopenMistake(fresh, request, QString()).isEmpty());
    // a later part reopening what an earlier part of this plan defined is this request's own work
    const QString reopen = "c = Compiler\nc.open(\"ai/RefDecay\")\nok = c.compile()\n";
    const QString earlier = "c.new()\nc.setFunctionName(\"ai/RefDecay\")\nok = c.compile()\n";
    QVERIFY(QtiSasAiContent::aiSandboxReopenMistake(reopen, request, earlier).isEmpty());
    // a library function outside ai/ may be reopened
    QVERIFY(
        QtiSasAiContent::aiSandboxReopenMistake("c.open(\"sasviewmodels/sphere\")\nc.compile()\n", request, QString())
            .isEmpty());
}

void AiContentTests::aiSandboxReopen_allowsRequestToChangeExistingFunction()
{
    const QString request = "Open the existing fit function \"ai/RefDecay\", change c to 0.5 and compile it again.";
    const QString code = "c = Compiler\nc.open(\"ai/RefDecay\")\nok = c.compile()\n";
    QVERIFY(QtiSasAiContent::aiSandboxReopenMistake(code, request, QString()).isEmpty());
}

void AiContentTests::guardedCompile_blocksCompileBehindExistenceCheck()
{
    // LIST 5, item 1 (twice): `if fn not in c.functionNames("ALL"):` around the compile — a failed earlier compile had
    // left the definition behind, so the compile was skipped and configure() reported "not found".
    const QString request = R"(Compile a new fit function "ai/Quad" for y = a*x^2 + b*x + c and fit it to "Calib".)";
    const QString guarded =
        "c = Compiler\nfn = \"ai/Quad\"\nif fn not in c.functionNames(\"ALL\"):\n    c.new()\n"
        "    c.setFunctionName(fn)\n\n    c.setCode(\"y = a*x*x + b*x + c;\")\n    ok = c.compile()\n"
        "f = Fittable\nf.configure(\"ai/Quad\", [\"Calib_y\"])\n";
    QVERIFY(!QtiSasAiContent::guardedCompileMistake(guarded, request).isEmpty());
    QString unconditional = guarded;
    unconditional.replace("if fn not in c.functionNames(\"ALL\"):\n", "");
    unconditional.replace("\n    ", "\n");
    QVERIFY(QtiSasAiContent::guardedCompileMistake(unconditional, request).isEmpty());
    // a guard around something else (a table) is fine; so is a request to change an existing function
    QVERIFY(QtiSasAiContent::guardedCompileMistake(
                "if not existTable(\"T\"):\n    t = newTable(\"T\", 3, 2)\nc.new()\nc.compile()\n", request)
                .isEmpty());
    QVERIFY(QtiSasAiContent::guardedCompileMistake(guarded, "Recompile the existing function \"ai/Quad\".").isEmpty());
}

void AiContentTests::paramIndexRepair_usesTheParameterNames()
{
    // LIST 5, item 1 (local model): every version used param[k]; clang's "did you mean 'ParaM'?" did not help
    QtiSasAiContent content;
    const QString head = "c.setParamNamesAll([\"a\", \"b\", \"c\"])\n";
    QString repaired = content.compilerParamIndexRepair(
        head + "c.setCode(\"\"\"\ny = param[0]*x*x + param[1]*x + param[2];\n\"\"\")\n");
    QVERIFY(repaired.contains("y = a*x*x + b*x + c;"));
    // the declaring variant becomes "double a = a;", which the self-shadow repair then removes
    repaired = content.compilerParamIndexRepair(
        head +
        "c.setCode(\"\"\"\ndouble a = param[0];\ndouble b = param[1];\ndouble c = param[2];\ny = a*x*x + b*x + c;\n"
        "\"\"\")\n");
    QVERIFY(repaired.contains("double a = a;"));
    repaired = content.compilerSelfShadowRepair(repaired);
    QVERIFY(!repaired.contains("double") && repaired.contains("y = a*x*x + b*x + c;"));
    // an array the body declares itself, an index past the names, and no literal names: left alone
    QVERIFY(content.compilerParamIndexRepair(head + "c.setCode(\"double p[2]; p[0] = x; y = p[0];\")\n").isEmpty());
    QVERIFY(content.compilerParamIndexRepair(head + "c.setCode(\"y = param[5];\")\n").isEmpty());
    QVERIFY(content.compilerParamIndexRepair("c.setParamNamesAll(names)\nc.setCode(\"y = param[0];\")\n").isEmpty());
    QCOMPARE(content.compiledCodeBodies("c.setCode(\"y = a;\")\nc.setCode(\"\"\"\ny = b;\n\"\"\")\n").size(), 2);
}

void AiContentTests::paramVariesRepair_makesEveryParameterAdjustable()
{
    // LIST 5, item 2 (local model): both functions compiled with no vary flags, so every parameter was fixed and
    // fit() failed "no adjustable parameters"; the retry set them, but behind an existence check that was skipped.
    const QtiSasAiContent content;
    const QString code = "c = Compiler\nc.new()\nc.setParamCount(3)\nc.setParamNamesAll([\"A\", \"w\", \"c\"])\n"
                         "c.compile()\nc.new()\nc.setParamCount(2)\nc.compile()\n";
    const QString request = "Compile two new fit functions, each with A, w and c adjustable.";
    QString repaired = content.compilerParamVariesRepair(code, request);
    QVERIFY(repaired.contains("c.setParamVariesAll([True, True, True])\nc.compile()\nc.new()"));
    QVERIFY(repaired.contains("c.setParamVariesAll([True, True])\nc.compile()\n"));
    // an indented compile() keeps its block
    repaired =
        content.compilerParamVariesRepair("if True:\n    c.new()\n    c.setParamCount(1)\n    c.compile()\n", request);
    QVERIFY(repaired.contains("\n    c.setParamVariesAll([True])\n    c.compile()"));
    // compile() inside a condition (ai-test-requests item 12): the flags go on the line before it
    repaired = content.compilerParamVariesRepair(
        "c.new()\nc.setParamNamesAll([\"A\", \"w\"])\nif not c.compile():\n    raise RuntimeError(\"x\")\n", request);
    QVERIFY2(repaired.contains("c.setParamVariesAll([True, True])\nif not c.compile():"), qPrintable(repaired));
    repaired = content.compilerParamVariesRepair("c.new()\nc.setParamCount(1)\nassert c.compile()\n", request);
    QVERIFY2(repaired.contains("c.setParamVariesAll([True])\nassert c.compile()"), qPrintable(repaired));
    // the script already decides, or the request names fixed parameters: unchanged
    QVERIFY(content
                .compilerParamVariesRepair("c.new()\nc.setParamCount(2)\nc.setParamVariesAll([True, False])\n"
                                           "c.compile()\n",
                                           request)
                .isEmpty());
    QVERIFY(content.compilerParamVariesRepair(code, "Compile it with c fixed at 0.1.").isEmpty());
}

void AiContentTests::columnFormulaInLoop_blocksSetCommandPerRow()
{
    // LIST 5, item 1 (NOSPLIT): a per-row formula left the last row's value in every row — a wrong fit, no error.
    const QString perRow = "for i in range(1, 22):\n    x = (i - 1) * 0.1\n    t.setCell(1, i, x)\n"
                           "    t.setCommand(2, f\"3*pow({x}, 2) + 1\")\nt.recalculate(2)\n";
    QVERIFY(QtiSasAiContent::columnFormulaInLoopMistake(perRow).contains("WHOLE column"));
    // once, after the loop: fine
    QVERIFY(QtiSasAiContent::columnFormulaInLoopMistake("for i in range(1, 22):\n    t.setCell(1, i, i)\n"
                                                        "t.setCommand(2, \"col(\\\"1\\\")**2\")\nt.recalculate(2)\n")
                .isEmpty());
}

void AiContentTests::guardedCreationRepair_keepsOnlyTheCreationInside()
{
    // LIST 5, item 1 (NOSPLIT): the table's whole set-up sat behind `if t is None:`, so on a leftover table none of
    // it ran. Only newTable() stays inside the check.
    const QString code = "t = table(\"Calib\")\nif t is None:\n    t = newTable(\"Calib\", 21, 3)\n"
                         "    t.setColNames([\"x\", \"y\", \"dy\"])\n    for i in range(1, 22):\n"
                         "        t.setCell(1, i, 0.1 * (i - 1))\n    t.recalculate(2)\nf = Fittable\n";
    QCOMPARE(AiAssistant::guardedCreationRepair(code, {}),
             QString("t = table(\"Calib\")\nif t is None:\n    t = newTable(\"Calib\", 21, 3)\n"
                     "t.setColNames([\"x\", \"y\", \"dy\"])\nfor i in range(1, 22):\n"
                     "    t.setCell(1, i, 0.1 * (i - 1))\nt.recalculate(2)\nf = Fittable\n"));
    // already in the right shape, a block that does not start with the creation, or one with an else: unchanged
    QVERIFY(AiAssistant::guardedCreationRepair("if t is None:\n    t = newTable(\"A\", 2, 2)\nt.setCell(1, 1, 2)\n", {})
                .isEmpty());
    QVERIFY(
        AiAssistant::guardedCreationRepair("if t is None:\n    scriptPrint(\"missing\")\n    t = newTable(\"A\")\n", {})
            .isEmpty());
    QVERIFY(AiAssistant::guardedCreationRepair(
                "if t is None:\n    t = newTable(\"A\", 2, 2)\n    t.setCell(1, 1, 2)\nelse:\n    pass\n", {})
                .isEmpty());
    // a table that existed before the request keeps its guarded set-up: it may be sample data for a missing one
    QVERIFY(AiAssistant::guardedCreationRepair(code, {"table:Calib"}).isEmpty());
}

void AiContentTests::skippedFailedStep_blocksFixBehindExistenceCheck()
{
    // LIST 5, item 1 (NOSPLIT): recalculate() failed on "^"; the retry moved the table creation, corrected formula
    // included, behind `if t is None:` — Calib existed from the failed attempt, so the fix never ran.
    const QString retry = "t = table(\"Calib\")\nif t is None:\n    t = newTable(\"Calib\", 21, 3)\n"
                          "    t.setCommand(2, \"3*pow(col(\\\"x\\\"),2)\")\n    t.recalculate(2)\n"
                          "f = Fittable\nf.fit()\n";
    const QStringList existed{"table:Calib"};
    QVERIFY(AiAssistant::skippedFailedStepMistake(retry, "t.recalculate(2)", existed).contains("\"Calib\" exists"));
    // the same block is fine when the object did not exist before, or when the failed step is outside it
    QVERIFY(AiAssistant::skippedFailedStepMistake(retry, "t.recalculate(2)", {"note:Other"}).isEmpty());
    QVERIFY(AiAssistant::skippedFailedStepMistake(retry, "chi2 = f.fit()", existed).isEmpty());
    // the existTable() form of the guard
    const QString byExist = "if not existTable(\"Calib\"):\n    t = newTable(\"Calib\", 21, 3)\n    t.recalculate(2)\n";
    QVERIFY(!AiAssistant::skippedFailedStepMistake(byExist, "t.recalculate(2)", existed).isEmpty());
    // repaired: the check goes, its block runs at the outer level; with an else branch it is left to the model
    const QString unguarded = AiAssistant::skippedFailedStepRepair(retry, "t.recalculate(2)", existed);
    QVERIFY(unguarded.startsWith("t = table(\"Calib\")\nt = newTable(\"Calib\", 21, 3)\nt.setCommand("));
    QVERIFY(unguarded.contains("\nt.recalculate(2)\nf = Fittable"));
    QVERIFY(AiAssistant::skippedFailedStepRepair(retry, "t.recalculate(2)", {"note:Other"}).isEmpty());
    QVERIFY(
        AiAssistant::skippedFailedStepRepair(byExist + "else:\n    t = table(\"Calib\")\n", "t.recalculate(2)", existed)
            .isEmpty());
}

void AiContentTests::unrequestedRandomData_blocksNoiseOnExactData()
{
    // Supervised LIST 3, item 7: exact data got random noise, the fit result was silently wrong.
    const QString request = "Create a table \"LineData\" with 20 rows: x = 1..20, y = 2x + 1, dy = 0.1.";
    const QString code = "import random\nfor i in range(1, 21):\n    t.setCell(2, i, 2*i + 1 + random.gauss(0, 0.1))\n";
    QVERIFY(!QtiSasAiContent::unrequestedRandomDataMistake(code, request).isEmpty());
    QVERIFY(QtiSasAiContent::unrequestedRandomDataMistake(code, request + " Add 5% Gaussian noise.").isEmpty());
    QVERIFY(QtiSasAiContent::unrequestedRandomDataMistake("import random\nt.setCell(2, 1, 3)\n", request).isEmpty());
}

void AiContentTests::columnListInsteadOfAggregate_blocksBareColData()
{
    // Supervised LIST 3, item 1: "Print the sum of column 2" printed the whole column.
    const QString request = "Print the sum of column 2. The result label is \"R1:\".";
    QVERIFY(!QtiSasAiContent::columnListInsteadOfAggregateMistake("scriptPrint(\"R1:\", t.colData(2))\n", request)
                 .isEmpty());
    QVERIFY(QtiSasAiContent::columnListInsteadOfAggregateMistake("scriptPrint(\"R1:\", sum(t.colData(2)))\n", request)
                .isEmpty());
    QVERIFY(
        QtiSasAiContent::columnListInsteadOfAggregateMistake("print('R1:', len(t.colData(2)))\n", request).isEmpty());
    // a request that asks for the values themselves
    QVERIFY(QtiSasAiContent::columnListInsteadOfAggregateMistake("scriptPrint(\"R1:\", t.colData(2))\n",
                                                                 "Print the values of column 2.")
                .isEmpty());
}

void AiContentTests::selfShadowRepair_removesSelfReferentialDeclarations()
{
    // LIST 3 item 8 / LIST 5 item 2: "double A = A; double w = w; double c = c;" resubmitted five times despite the
    // block.
    QtiSasAiContent content;
    const QString code =
        "c.setCode(\"\"\"\ndouble A = A; double w = w; double c = c;\ny = A*exp(-x*x/(2*w*w)) + c;\n\"\"\")\n";
    const QString repaired = content.compilerSelfShadowRepair(code);
    QVERIFY(!repaired.isEmpty());
    QVERIFY(!repaired.contains("double A = A"));
    QVERIFY(!repaired.contains("double c = c"));
    QVERIFY(repaired.contains("y = A*exp(-x*x/(2*w*w)) + c;"));
    QVERIFY(QtiSasAiContent::compiledCodeSelfShadowBug(repaired).isEmpty());
    QVERIFY(content.compilerSelfShadowRepair("c.setCode(\"double arg = x/w; y = A*arg;\")\n").isEmpty());
}

void AiContentTests::hardcodedResultLine_blocksFixedText()
{
    // LIST 3 item 3: scriptPrint("R3: 0 ValueError"), nothing computed.
    const QString request = "Print the number of empty cells. The result label is \"R3:\".";
    QVERIFY(!QtiSasAiContent::hardcodedResultLineMistake("scriptPrint(\"R3: 0 ValueError\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::hardcodedResultLineMistake("scriptPrint(\"R3:\", n, name)\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::hardcodedResultLineMistake("scriptPrint(f\"R3: {n} {name}\")\n", request).isEmpty());
    // a guard branch's failure message is no fabricated result
    QVERIFY(
        QtiSasAiContent::hardcodedResultLineMistake("    scriptPrint(\"R3: table not found\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::hardcodedResultLineMistake("scriptPrint(\"R3: 0 ValueError\")\n", "Print 3.").isEmpty());
}

void AiContentTests::repeatedResultLabel_blocksSeveralTopLevelPrints()
{
    // LIST 5 item 4: three top-level scriptPrint("R4:", ...) lines.
    const QString request = "Print a, b and c. The result label is \"R4:\".";
    QVERIFY(
        !QtiSasAiContent::repeatedResultLabelMistake("scriptPrint(\"R4:\", a)\nscriptPrint(\"R4:\", b, c)\n", request)
             .isEmpty());
    QVERIFY(QtiSasAiContent::repeatedResultLabelMistake("scriptPrint(\"R4:\", a, b, c)\n", request).isEmpty());
    // one print per branch is one line at run time
    QVERIFY(QtiSasAiContent::repeatedResultLabelMistake(
                "if t is None:\n    scriptPrint(\"R4: no table\")\nelse:\n    scriptPrint(\"R4:\", a)\n", request)
                .isEmpty());
}

void AiContentTests::resultLineDuty_extractsPrintThroughLabel()
{
    QtiSasAiContent content;
    const QString item =
        "Item 4: Create a 21x21 matrix \"Saddle\" spanning x and y from -1 to 1 and fill it with x*x - y*y. Plot it as "
        "a 3D surface in a window named \"Saddle3D\". Print the value at the centre of the matrix (row 11, column 11), "
        "the smallest and the largest value of all cells, and whether a 3D window \"Saddle3D\" exists. The result "
        "label is \"R4:\".\n\nInstruction for every item of this list: Use exactly the object names given.";
    const QString duty = content.resultLineDuty(item);
    QVERIFY(duty.startsWith("Print the value at the centre of the matrix (row 11, column 11)"));
    QVERIFY(duty.endsWith("The result label is \"R4:\"."));
    QCOMPARE(content.resultLabel(duty), QString("R4:"));
    // several "Print" sentences: the last one before the label
    QVERIFY(content.resultLineDuty("Print a. Then fit. Print b and c. The result label is \"R1:\".")
                .startsWith("Print b and c."));
    QVERIFY(content.resultLineDuty("Print a and b.").isEmpty());
    QVERIFY(content.resultLineDuty("Show a. The result label is \"R1:\".").isEmpty());
}

void AiContentTests::nonFinalResultLine_removesFixedTextOnly()
{
    // LIST 5, item 4: part 1 of 2 printed "R4: done". Item 3: a block of an earlier part's computed line was sent back
    // unchanged twice and the item given up — so the fixed-text line is removed, a computed one stays.
    QtiSasAiContent content;
    const QString request = "Make a matrix and plot it. Print its centre. The result label is \"R4:\".";
    const QString code = "m = newMatrix(\"Saddle\", 21, 21)\nif m:\n    scriptPrint(\"R4: done\")\n";
    QCOMPARE(content.nonFinalResultLineRepair(code, request, false),
             QString("m = newMatrix(\"Saddle\", 21, 21)\nif m:\n    pass\n"));
    QVERIFY(content.nonFinalResultLineRepair("scriptPrint(\"R4:\", m.cell(11, 11))\n", request, false).isEmpty());
    QVERIFY(content.nonFinalResultLineRepair("scriptPrint(f\"R4: {v}\")\n", request, false).isEmpty());
    QVERIFY(content.nonFinalResultLineRepair("scriptPrint(\"R4: done\")\n", "Print it.", false).isEmpty());
    // a self-review's computed line in an earlier part goes too (LIST 5, item 6), the rest of its script stays
    QCOMPARE(content.nonFinalResultLineRepair("closeWindow(t)\nscriptPrint(f\"R4: {a} {b}\")\n", request, true),
             QString("closeWindow(t)\npass\n"));
    // in the last part (no later parts) the fixed-text check still blocks
    QStringList headers;
    for (const AiKnownMistakeMatch &m : content.detectKnownMistakes("scriptPrint(\"R4: done\")\n", request, QString()))
        headers << m.header;
    QVERIFY(headers.contains("the result line is fixed text, not computed"));
    headers.clear();
    for (const AiKnownMistakeMatch &m :
         content.detectKnownMistakes("scriptPrint(\"R4: done\")\n", request, QString(), "plot it as 3D"))
        headers << m.header;
    QVERIFY(!headers.contains("the result line is fixed text, not computed"));
}

void AiContentTests::redundantReconfigure_allowsFullReSetup()
{
    // LIST 5, item 1: part 4 re-did configure, start values, weighting and fit — blocked twice, item given up.
    const QString earlier = "f = Fittable\nf.configure(\"ai/Quad\", [\"Calib_y\"])\nf.setParamValue(\"a\", 1)\n"
                            "f.setWeightingEnabled(1, True)\nf.setWeightingDataset(1, \"Calib_dy\")\nf.fit()\n";
    const QString full = "f.configure(\"ai/Quad\", [\"Calib_y\"])\nf.setParamValue(\"a\", 1)\n"
                         "f.setWeightingEnabled(1, True)\nf.setWeightingDataset(1, \"Calib_dy\")\nf.fit()\n";
    QVERIFY(QtiSasAiContent::redundantFittableReconfigureMistake(full, earlier).isEmpty());
    // the weighting dropped: still blocked
    QVERIFY(!QtiSasAiContent::redundantFittableReconfigureMistake(
                 "f.configure(\"ai/Quad\", [\"Calib_y\"])\nf.setParamValue(\"a\", 1)\nf.fit()\n", earlier)
                 .isEmpty());
}

void AiContentTests::noteTabIndex_repairsTabZero()
{
    // LIST 5, item 5: n.tab(0).execute() twice — tab() is 1-based, tab(0) is None.
    QtiSasAiContent content;
    QCOMPARE(content.noteTabIndexRepair("n.tab(0).execute()\nm.tab( 0 ).setText(s)\n"),
             QString("n.tab(1).execute()\nm.tab( 1 ).setText(s)\n"));
    QVERIFY(content.noteTabIndexRepair("n.tab(1).execute()\nn.editorAt(0).execute()\nn.tab(10)\n").isEmpty());
}

void AiContentTests::repeatedResultLabel_repairKeepsOnlyTheLastLine()
{
    // LIST 5 NOSPLIT, item 5: an early "R5:" line and the full one at the end — blocked, sent back unchanged, given up.
    QtiSasAiContent content;
    const QString request = "Run the tab. Print the number of tabs and the sum. The result label is \"R5:\".";
    const QString code = "n = newNote(\"P\")\nscriptPrint(\"R5:\", n.tabs())\nif n:\n    scriptPrint(\"R5:\", 1)\n"
                         "s = 15\nscriptPrint(\"R5:\", n.tabs(), True, s)\n";
    QCOMPARE(content.repeatedResultLabelRepair(code, request),
             QString("n = newNote(\"P\")\npass\nif n:\n    scriptPrint(\"R5:\", 1)\ns = 15\n"
                     "scriptPrint(\"R5:\", n.tabs(), True, s)\n"));
    QVERIFY(QtiSasAiContent::repeatedResultLabelMistake(content.repeatedResultLabelRepair(code, request), request)
                .isEmpty());
    // one top-level line, or no label in the request: nothing to repair
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R5:\", 1)\n", request).isEmpty());
    QVERIFY(content.repeatedResultLabelRepair(code, "Print it.").isEmpty());
    // an earlier print spread over several lines is left to the check
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R5:\",\n    1)\nscriptPrint(\"R5:\", 2)\n", request)
                .isEmpty());
    // the values split over the lines (local pyi re-runs, items 1 and 6): merged into one line, in their order — when
    // that line then has exactly the values the request asks for
    const QString threeValues = "Print a, b and n. The result label is \"R5:\".";
    QCOMPARE(content.repeatedResultLabelRepair(
                 "scriptPrint(f\"R5: {a:.4f} {b:.4f}\")\nx = 1\nscriptPrint(f\"R5: {n}\")\n", threeValues),
             QString("pass\nx = 1\nscriptPrint(\"R5:\", format(a, \".4f\"), format(b, \".4f\"), n)\n"));
    QCOMPARE(
        content.repeatedResultLabelRepair(
            "scriptPrint(f\"R5: {fns}\")\nscriptPrint(f\"R5: {'exists' if p else 'missing'}\", str(q))\n", threeValues),
        QString("pass\nscriptPrint(\"R5:\", fns, 'exists' if p else 'missing', str(q))\n"));
    // values overlapping in part are a guess: left to the check
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R5:\", n, e)\nscriptPrint(\"R5:\", n, t, s)\n", request)
                .isEmpty());
    // fixed text after the label: left to the check
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R5: 5\")\nscriptPrint(\"R5:\", n)\n", request).isEmpty());
    // a last line with as many values as the earlier one (f-string fields and arguments both count) is the answer
    QCOMPARE(
        content.repeatedResultLabelRepair("scriptPrint(f\"R5: {a} {b}\")\nscriptPrint(f\"R5: {a}\", b)\n", request),
        QString("pass\nscriptPrint(f\"R5: {a}\", b)\n"));
}

void AiContentTests::tableFormulaCaret_repairsToPythonPower()
{
    // LIST 5 NOSPLIT, item 1: "3*col(\"x\")^2 - ..." — ^ is XOR in Python: TypeError and a retry.
    QtiSasAiContent content;
    QCOMPARE(
        content.tableFormulaCaretRepair("t.setCommand(2, \"3*col(\\\"x\\\")^2 - 2*col(\\\"x\\\") + 1\")\n", "Python"),
        QString("t.setCommand(2, \"3*col(\\\"x\\\")**2 - 2*col(\\\"x\\\") + 1\")\n"));
    QCOMPARE(content.tableFormulaCaretRepair("t.setCommand(\"y\", 'col(\"x\")^3')\n", "Python"),
             QString("t.setCommand(\"y\", 'col(\"x\")**3')\n"));
    QVERIFY(content.tableFormulaCaretRepair("t.setCommand(2, \"pow(col(1), 2)\")\n", "Python").isEmpty());
    // the global scripting language muParser, or unknown: ^ is the power there, nothing to repair
    QVERIFY(content.tableFormulaCaretRepair("t.setCommand(2, \"col(1)^2\")\n", "muParser").isEmpty());
    QVERIFY(content.tableFormulaCaretRepair("t.setCommand(2, \"col(1)^2\")\n", QString()).isEmpty());
    // muParser asked for: ^ is its power
    QVERIFY(
        content
            .tableFormulaCaretRepair("t.setCommand(2, \"col(1)^2\")\nt.recalculate(2, forceMuParser=True)\n", "Python")
            .isEmpty());
    // forced positionally (series 5, Codex, LIST 3 item 2): kept; another column of the same script still repaired
    QVERIFY(content.tableFormulaCaretRepair("t.setCommand(3, \"col(1)^3\")\nt.recalculate(3, 1, -1, True)\n", "Python")
                .isEmpty());
    QCOMPARE(
        content.tableFormulaCaretRepair(
            "t.setCommand(3, \"col(1)^3\")\nt.recalculate(3, 1, -1, True)\nt.setCommand(2, \"col(1)^2\")\n", "Python"),
        QString("t.setCommand(3, \"col(1)^3\")\nt.recalculate(3, 1, -1, True)\nt.setCommand(2, \"col(1)**2\")\n"));
    // a matrix formula is muParser: untouched
    QVERIFY(content.tableFormulaCaretRepair("m.setFormula(\"x^2-y^2\")\n", "Python").isEmpty());
}

void AiContentTests::yNameRepair_usesDefaultAfterNew()
{
    // LIST 5 NOSPLIT, item 1: c.new() without setYName(), "I = a*x*x + b*x + c;" — clang: undeclared 'I'. After new()
    // the dependent variable is "f".
    QtiSasAiContent content;
    const QString code =
        "c = Compiler\nc.new()\nc.setFunctionName(\"ai/Quad\")\nc.setParamNamesAll([\"a\", \"b\", \"c\"])\n"
        "c.setCode(\"\"\"\nI = a*x*x + b*x + c;\n\"\"\")\nc.compile()\n";
    const QString repaired = content.compilerYNameAssignmentRepair(code);
    QVERIFY(repaired.contains("\nf = a*x*x + b*x + c;\n"));
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(repaired).isEmpty());
    // already right
    QVERIFY(content.compilerYNameAssignmentRepair(QString(code).replace("I = ", "f = ")).isEmpty());
    // an existing function opened, not a new one: the yName is unknown
    QVERIFY(content.compilerYNameAssignmentRepair(QString(code).replace("c.new()", "c.open(\"ai/Quad\")")).isEmpty());
}

void AiContentTests::repairsAndChecks_leaveEveryReferenceScriptAlone()
{
    // The reference scripts of the benchmark lists solve their items with the documented API (they score 100%), so a
    // repair that changes one of them fires on correct code — shaped to the case that exposed it rather than to a
    // general rule, and a check that blocks one is a false positive. Every repair runActionScript() applies, and every
    // known-mistake check, is run on every reference script with that item's own request.
    QtiSasAiContent content;
    const QString benchmarks = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath() + "/benchmarks";
    int checked = 0;
    for (const QString &list : {QStringLiteral("list3"), QStringLiteral("list4"), QStringLiteral("list5")})
    {
        QFile file(benchmarks + "/" + list + ".json");
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable("missing " + file.fileName()));
        const QJsonObject bench = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonArray items = bench.value("items").toArray();
        for (int i = 0; i < items.size(); ++i)
        {
            const QString name = QString("%1/item%2.py").arg(list).arg(i + 1, 2, 10, QLatin1Char('0'));
            QFile script(benchmarks + "/reference/" + name);
            if (!script.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            const QString code = QString::fromUtf8(script.readAll());
            const QString request =
                items.at(i).toObject().value("text").toString() + "\n\n" + bench.value("instruction").toString();
            const QList<QPair<QString, QString>> repairs = {
                {"compilerParamCountRepair", content.compilerParamCountRepair(code)},
                {"compilerParamVariesRepair", content.compilerParamVariesRepair(code, request)},
                {"compilerParamIndexRepair", content.compilerParamIndexRepair(code)},
                {"compilerSelfShadowRepair", content.compilerSelfShadowRepair(code)},
                {"compilerYNameAssignmentRepair", content.compilerYNameAssignmentRepair(code)},
                {"repeatedResultLabelRepair", content.repeatedResultLabelRepair(code, request)},
                {"labelOnlyResultLineRepair", content.labelOnlyResultLineRepair(code, request)},
                {"bareFolderNameRepair", content.bareFolderNameRepair(code, request)},
                {"setCodeVariableRepair", content.setCodeVariableRepair(code)},
                {"matrixTransposedCellRepair", content.matrixTransposedCellRepair(code)},
                {"tableTransposedCellRepair", content.tableTransposedCellRepair(code)},
                {"bareCompilerDeleteRepair", content.bareCompilerDeleteRepair(code)},
                {"noteAutoexecReceiverRepair", content.noteAutoexecReceiverRepair(code)},
                {"apiCallRepairs", content.apiCallRepairs(code, request, true, nullptr)},
                {"muParserPowerRepair", content.muParserPowerRepair(code)},
                {"tableFormulaCaretRepair", content.tableFormulaCaretRepair(code, "Python")},
                {"compilerMissingGroupRepair", content.compilerMissingGroupRepair(code)},
                {"noteTabIndexRepair", content.noteTabIndexRepair(code)},
                {"noteTabSelfRunRepair", content.noteTabSelfRunRepair(code)},
            };
            for (const QPair<QString, QString> &repair : repairs)
                QVERIFY2(repair.second.isEmpty(), qPrintable(repair.first + " changed the correct " + name));
            // and no check blocks it either
            QStringList blocked;
            for (const AiKnownMistakeMatch &m : content.detectKnownMistakes(code, request, QString()))
                blocked << m.header;
            QVERIFY2(blocked.isEmpty(), qPrintable(name + " blocked: " + blocked.join("; ")));
            ++checked;
        }
    }
    QVERIFY2(checked >= 20, qPrintable(QString("only %1 reference scripts found").arg(checked)));
}

void AiContentTests::missingGroup_repairsNewAiFunctionWithoutGroup()
{
    // LIST 5 local pyi, item 2: "save: group is empty" four times — no setGroup() anywhere.
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/Gauss5\")\nc.setCode(\"y = A;\")\nc.compile()\n"
                         "c.new()\n    c.setFunctionName('ai/Lor5')  # second\nc.save()\n";
    QCOMPARE(
        content.compilerMissingGroupRepair(code),
        QString(
            "c = Compiler\nc.new()\nc.setFunctionName(\"ai/Gauss5\")\nc.setGroup(\"ai\")\nc.setCode(\"y = A;\")\n"
            "c.compile()\nc.new()\n    c.setFunctionName('ai/Lor5')  # second\n    c.setGroup(\"ai\")\nc.save()\n"));
    // setGroup() before new() is lost (new() clears it): repeated after setFunctionName() (LIST 5 local pyi, item 1)
    QCOMPARE(content.compilerMissingGroupRepair("c.setGroup(match if match else \"custom:\" + domain)\nc.new()\n"
                                                "c.setFunctionName(\"ai/Quad\")\nc.compile()\n"),
             QString("c.setGroup(match if match else \"custom:\" + domain)\nc.new()\nc.setFunctionName(\"ai/Quad\")\n"
                     "c.setGroup(match if match else \"custom:\" + domain)\nc.compile()\n"));
    // a group set between new() and compile(), an opened (not new) function, or a non-ai name without any group: alone
    QVERIFY(
        content.compilerMissingGroupRepair("c.new()\nc.setFunctionName(\"ai/X\")\nc.setGroup(\"peaks\")\nc.compile()\n")
            .isEmpty());
    QVERIFY(
        content.compilerMissingGroupRepair("c.open(\"ai/X\")\nc.setFunctionName(\"ai/X\")\nc.compile()\n").isEmpty());
    QVERIFY(content.compilerMissingGroupRepair("c.new()\nc.setFunctionName(\"Mine\")\nc.compile()\n").isEmpty());
}

void AiContentTests::requestedResultValues_neverMoreThanTheAnswerHas()
{
    // The count is a lower bound by design (a check may only ask for more values, never fewer): over every benchmark
    // item, it must never exceed the number of values of the item's expected answer.
    const QString benchmarks = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath() + "/benchmarks";
    int counted = 0;
    for (const QString &list : {QStringLiteral("list3"), QStringLiteral("list4"), QStringLiteral("list5"),
                                QStringLiteral("ascii1d"), QStringLiteral("svd"), QStringLiteral("svd_holdout")})
    {
        QFile file(benchmarks + "/" + list + ".json");
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject bench = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonArray items = bench.value("items").toArray();
        for (int i = 0; i < items.size(); ++i)
        {
            const QJsonObject item = items.at(i).toObject();
            // as qtisas_benchmark.py sends it: the item text, then its result label
            const QString request = item.value("text").toString() +
                                    QString(" The result label is \"R%1:\".").arg(i + 1) + "\n\n" +
                                    bench.value("instruction").toString();
            const qsizetype asked = QtiSasAiContent::requestedResultValues(request).size();
            const qsizetype answer = item.value("expected").toString().split(' ', Qt::SkipEmptyParts).size();
            QVERIFY2(asked <= answer,
                     qPrintable(QString("%1 item %2: %3 values counted, the answer has %4: %5")
                                    .arg(list)
                                    .arg(i + 1)
                                    .arg(asked)
                                    .arg(answer)
                                    .arg(QtiSasAiContent::requestedResultValues(request).join(" | "))));
            counted += asked > 0;
        }
    }
    QVERIFY2(counted >= 15, qPrintable(QString("only %1 items gave a count").arg(counted)));
    // the LIST 5 items, exactly
    const QString label = " The result label is \"R1:\".";
    QCOMPARE(QtiSasAiContent::requestedResultValues(
                 "Print a, b and c with 4 decimals, and the number of rows where y is greater than 5." + label)
                 .size(),
             qsizetype(4));
    QCOMPARE(QtiSasAiContent::requestedResultValues("Print the name of the function with the smaller chi2/dof, then "
                                                    "its fitted A and w (as a positive number), each with 4 decimals." +
                                                    label)
                 .size(),
             qsizetype(3));
    // benchmark ASCII1D items 3 and 8: "the first and last Q" is two values of one noun
    QCOMPARE(QtiSasAiContent::requestedResultValues("Print the number of rows and the first and last Q." + label),
             QStringList({"the number of rows", "the first Q", "last Q"}));
    QCOMPARE(QtiSasAiContent::requestedResultValues("Print the value in row 3, column 4, and the sum of row 3." + label)
                 .size(),
             qsizetype(2));
}

void AiContentTests::resultLineTooFewValues_blocksShortLineOnly()
{
    // LIST 5 local pyi: R5 "15.0" for "the number of tabs, whether table "Auto" exists, and the sum of its column 1".
    const QString request = "Print the number of tabs, whether table \"Auto\" exists, and the sum of its column 1. The "
                            "result label is \"R5:\".";
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake("s = 15\nscriptPrint(f\"R5: {s}\")\n", request)
                .contains("asks for 3"));
    // all three, or more (the count is a lower bound), or a line that cannot be read off: no block
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake("scriptPrint(\"R5:\", n, e, s)\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake("scriptPrint(\"R5:\", n, e, s, x)\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake("scriptPrint(\"R5: \" + str(n))\n", request).isEmpty());
    // a guarded line counts (local pyi, item 3), a fixed-text fallback beside it does not
    QVERIFY(!QtiSasAiContent::resultLineTooFewValuesMistake(
                 "if t is None:\n    scriptPrint(\"R5: -1\")\nelse:\n    scriptPrint(f\"R5: {s}\")\n", request)
                 .isEmpty());
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake(
                "if t is None:\n    scriptPrint(\"R5: -1\")\nelse:\n    scriptPrint(\"R5:\", n, e, s)\n", request)
                .isEmpty());
    // an f-string argument of fields only is one value per field (benchmark SVD item 5, local pyi)
    QVERIFY(QtiSasAiContent::resultLineTooFewValuesMistake("scriptPrint(\"R5:\", n, f\"{e} {s:.4f}\")\n", request)
                .isEmpty());
    QVERIFY(
        !QtiSasAiContent::resultLineTooFewValuesMistake("scriptPrint(\"R5:\", n, f\"e={e}\")\n", request).isEmpty());
}

void AiContentTests::hardcodedResultLine_allowsFallbackBranch()
{
    // LIST 5 local pyi, item 3: "if t is None: scriptPrint(\"R3: -1\") else: ... scriptPrint(\"R3:\", n, ...)".
    const QString request = "Print the number of layers. The result label is \"R3:\".";
    QVERIFY(QtiSasAiContent::hardcodedResultLineMistake(
                "if t is None:\n    scriptPrint(\"R3: -1\")\nelse:\n    scriptPrint(\"R3:\", n)\n", request)
                .isEmpty());
    // a fixed line with no computed one, or at top level: still blocked
    QVERIFY(!QtiSasAiContent::hardcodedResultLineMistake("if t:\n    scriptPrint(\"R3: 2\")\n", request).isEmpty());
    QVERIFY(!QtiSasAiContent::hardcodedResultLineMistake("scriptPrint(\"R3: 2\")\nscriptPrint(\"R3:\", n)\n", request)
                 .isEmpty());
}

void AiContentTests::noteTabSelfRun_removedFromTabText()
{
    // LIST 5 local pyi, item 5: the "make" tab's text ended with "n.currentEditor().execute()".
    QtiSasAiContent content;
    const QString code =
        "n.tab(\"make\").setText('t = newTable(\"Auto\", 5, 1)\\nt.setCell(1, 1, 1)\\nn.currentEditor().execute()')\n"
        "n.tab(\"make\").execute()\n";
    QCOMPARE(content.noteTabSelfRunRepair(code), QString("n.tab(\"make\").setText('t = newTable(\"Auto\", 5, "
                                                         "1)\\nt.setCell(1, 1, 1)')\nn.tab(\"make\").execute()\n"));
    // the main script's own run of the tab stays; a tab text without such a line is left alone
    QVERIFY(content.noteTabSelfRunRepair("n.tab(\"make\").setText('x = 1')\nn.tab(\"make\").execute()\n").isEmpty());
    // the addTab()/currentEditor() check reads code, not a tab's text
    QVERIFY(QtiSasAiContent::currentEditorAfterAddTabMistake(
                "n.addTab()\nn.tab(1).setText('m.currentEditor().execute()')\nn.tab(\"make\").execute()\n",
                "Run only the \"make\" tab.")
                .isEmpty());
}

void AiContentTests::labelOnlyResultLine_labelMovesOntoValues()
{
    // LIST 5 NOSPLIT NOEXPLORE local pyi, item 3: the label printed alone first, the values without it at the end.
    QtiSasAiContent content;
    const QString request = "Create a graph. Print the number of layers, the number of curves in the top and in the "
                            "bottom layer, and the largest absolute residual with 4 decimals. The result label is "
                            "\"R3:\".";
    const QString code = "scriptPrint(\"R3:\")\nif t is None:\n    scriptPrint(\"Table Calib not found.\")\nelse:\n"
                         "    scriptPrint(n, a, b, f\"{r:.4f}\")\n";
    QCOMPARE(content.labelOnlyResultLineRepair(code, request),
             QString("pass\nif t is None:\n    scriptPrint(\"Table Calib not found.\")\nelse:\n"
                     "    scriptPrint(\"R3:\", n, a, b, f\"{r:.4f}\")\n"));
    // too few values, two candidate lines, the label printed with values too, or no label asked: left alone
    QVERIFY(content.labelOnlyResultLineRepair("scriptPrint(\"R3:\")\nscriptPrint(n, a)\n", request).isEmpty());
    QVERIFY(content.labelOnlyResultLineRepair("scriptPrint(\"R3:\")\nprint(n, a, b, r)\nprint(n, a, b, r)\n", request)
                .isEmpty());
    QVERIFY(content.labelOnlyResultLineRepair("scriptPrint(\"R3:\")\nscriptPrint(\"R3:\", n, a, b, r)\n", request)
                .isEmpty());
    QVERIFY(content.labelOnlyResultLineRepair(code, "Print it.").isEmpty());
    // a print before the label line, or one with fixed text, is not the values line
    QVERIFY(content.labelOnlyResultLineRepair("scriptPrint(n, a, b, r)\nscriptPrint(\"R3:\")\n", request).isEmpty());
    QVERIFY(content.labelOnlyResultLineRepair("scriptPrint(\"R3:\")\nscriptPrint(\"layers\", n, a, b)\n", request)
                .isEmpty());
    // series 5, LIST 3 item 3: a stray scriptPrint("R3:", end="") beside the full result line goes
    QCOMPARE(content.labelOnlyResultLineRepair(
                 "scriptPrint(\"R3:\", end=\"\")\nx = 1\nscriptPrint(\"R3:\", n, a, b, r)\n", request),
             QString("pass\nx = 1\nscriptPrint(\"R3:\", n, a, b, r)\n"));
}

void AiContentTests::bareFolderName_usedWhole()
{
    // LIST 5 NOSPLIT local pyi: best = "Lor5" for the compiled "ai/Lor5", then configure(best, ...) failed.
    QtiSasAiContent content;
    const QString request = "Compile \"ai/Gauss5\" and \"ai/Lor5\", fit both to \"Peak5\". Print the name of the "
                            "better one.";
    const QString code =
        "c.setFunctionName(\"ai/Gauss5\")\nc.setFunctionName(\"ai/Lor5\")\nc.setCode(\"\"\"Lor5\"\"\")\n"
        "best = 'Gauss5' if a < b else \"Lor5\"\nf.configure(best, [\"Peak5_y\"])\n"
        "t.setTitle(\"Lor5 fit\")\n";
    QCOMPARE(content.bareFolderNameRepair(code, request),
             QString("c.setFunctionName(\"ai/Gauss5\")\nc.setFunctionName(\"ai/Lor5\")\nc.setCode(\"\"\"Lor5\"\"\")\n"
                     "best = 'ai/Gauss5' if a < b else \"ai/Lor5\"\nf.configure(best, [\"Peak5_y\"])\n"
                     "t.setTitle(\"Lor5 fit\")\n"));
    // already whole, the short name quoted by the request itself, or no folder name in the request: left alone
    QVERIFY(content.bareFolderNameRepair("f.configure(\"ai/Lor5\")\n", request).isEmpty());
    QVERIFY(content.bareFolderNameRepair("t = table(\"Lor5\")\n", request + " Make a table \"Lor5\".").isEmpty());
    QVERIFY(content.bareFolderNameRepair("best = \"Lor5\"\n", "Fit \"Lor5\".").isEmpty());
    // two folders with the same short name: which one is a guess
    QVERIFY(content
                .bareFolderNameRepair("x = \"ai/Lor5\"\ny = \"peaks/Lor5\"\nbest = \"Lor5\"\n",
                                      "Use \"ai/Lor5\" and \"peaks/Lor5\".")
                .isEmpty());
    // a formula's "/" between quoted names must not pair the quotes wrongly (LIST 5, item 2's own wording)
    QCOMPARE(content.bareFolderNameRepair(
                 "c.setFunctionName(\"ai/Lor5\")\nbest = \"Lor5\"\n",
                 "Create a table \"Peak5\" with y = 2/(1 + (x/0.5)^2) + 0.1. Compile \"ai/Gauss5\" for "
                 "y = A*exp(-x^2/(2*w^2)) + c and \"ai/Lor5\" for y = A/(1 + (x/w)^2) + c."),
             QString("c.setFunctionName(\"ai/Lor5\")\nbest = \"ai/Lor5\"\n"));
    // a script that never writes the full name: the short one may mean something else
    QVERIFY(content.bareFolderNameRepair("best = \"Lor5\"\n", request).isEmpty());
}

void AiContentTests::setCodeVariable_inlinedSoTheBodyIsChecked()
{
    // Iteration 7, LIST 5 item 1: the body in a variable, "double I = ..." never seen by the yName repair.
    QtiSasAiContent content;
    const QString code = "c.new()\nc.setYName(\"y\")\nbody = \"\"\"\ndouble I = a*x*x + b*x + c;\n\"\"\"\n"
                         "c.setCode(body)\nc.compile()\n";
    const QString inlined = content.setCodeVariableRepair(code);
    QCOMPARE(inlined, QString("c.new()\nc.setYName(\"y\")\nbody = \"\"\"\ndouble I = a*x*x + b*x + c;\n\"\"\"\n"
                              "c.setCode(\"\"\"\ndouble I = a*x*x + b*x + c;\n\"\"\")\nc.compile()\n"));
    // and the body is now repaired like a literal one
    QVERIFY(content.compilerYNameAssignmentRepair(inlined).contains("y = a*x*x + b*x + c;"));
    // assigned twice, built by concatenation, or assigned after the call: left alone
    QVERIFY(content.setCodeVariableRepair("b = \"y = a;\"\nb = \"y = b;\"\nc.setCode(b)\n").isEmpty());
    QVERIFY(content.setCodeVariableRepair("b = \"y = \" + s\nc.setCode(b)\n").isEmpty());
    QVERIFY(content.setCodeVariableRepair("c.setCode(b)\nb = \"y = a;\"\n").isEmpty());
    QVERIFY(content.setCodeVariableRepair("c.setCode(\"y = a;\")\n").isEmpty());
}

void AiContentTests::matrixTransposedCell_swappedWhenOnlyThatFits()
{
    // LIST 4, item 3: a 3x4 matrix read transposed — m.cell(4, 3) and a "row 3" sum over column 3.
    QtiSasAiContent content;
    const QString code = "m = newMatrix(\"Grid\", 3, 4)\nm.setFormula(\"i + 10 * j\")\nm.calculate()\n"
                         "v = m.cell(4, 3)\ns = m.cell(1, 3) + m.cell(2, 3) + m.cell(3, 3) + m.cell(4, 3)\n";
    QCOMPARE(content.matrixTransposedCellRepair(code),
             QString("m = newMatrix(\"Grid\", 3, 4)\nm.setFormula(\"i + 10 * j\")\nm.calculate()\n"
                     "v = m.cell(3, 4)\ns = m.cell(3, 1) + m.cell(3, 2) + m.cell(3, 3) + m.cell(3, 4)\n"));
    // in range as written, square, a computed index, or out of range either way: left alone
    QVERIFY(content.matrixTransposedCellRepair("m = newMatrix(\"G\", 3, 4)\nv = m.cell(3, 4)\n").isEmpty());
    QVERIFY(content.matrixTransposedCellRepair("m = newMatrix(\"G\", 3, 3)\nv = m.cell(4, 3)\n").isEmpty());
    QVERIFY(content.matrixTransposedCellRepair("m = newMatrix(\"G\", 3, 4)\nv = m.cell(4, 3)\nw = m.cell(i, 1)\n")
                .isEmpty());
    QVERIFY(content.matrixTransposedCellRepair("m = newMatrix(\"G\", 3, 4)\nv = m.cell(5, 3)\n").isEmpty());
}

void AiContentTests::requestedResultValues_orderPhraseIsNoValue()
{
    // LIST 4, item 6: "table first" listed as a value of its own.
    QCOMPARE(QtiSasAiContent::requestedResultValues("Close and delete the table \"Names\" and the matrix \"Grid\". "
                                                    "Print whether each still exists afterwards, table first. "
                                                    "The result label is \"R6:\"."),
             QStringList({"whether each still exists afterwards"}));
}

void AiContentTests::requestedResultValues_formatPhraseIsNoValue()
{
    // benchmark SVD items 1 and 5: "rounded to 4 decimals" counted as a value blocked the right line
    const QString label = " The result label is \"R1:\".";
    QCOMPARE(QtiSasAiContent::requestedResultValues("Print the number of Q points and S12 at Q = 0.05, rounded to 4 "
                                                    "decimals." +
                                                    label),
             QStringList({"the number of Q points", "S12 at Q = 0.05"}));
    QCOMPARE(
        QtiSasAiContent::requestedResultValues("Print S22 and its error, with 3 significant digits." + label).size(),
        qsizetype(2));
    QCOMPARE(
        QtiSasAiContent::requestedResultValues("Print the radius, rounded to an integer, and chi2." + label).size(),
        qsizetype(2));
    // a value that only mentions decimals stays one
    QCOMPARE(QtiSasAiContent::requestedResultValues("Print the number of decimals and the sum." + label).size(),
             qsizetype(2));
}

void AiContentTests::requestCreatesName_onlyForACreation()
{
    // LIST 3, items 2 and 3 in split mode: "In \"Squares\", ..." was planned as a creation and stopped as a collision.
    QVERIFY(!AiAssistant::requestCreatesName("In \"Squares\", give column 3 a formula.", "Squares"));
    QVERIFY(AiAssistant::requestCreatesName("Create a table \"Squares\" with 10 rows.", "Squares"));
    QVERIFY(AiAssistant::requestCreatesName("Compile a new fit function \"ai/RefLine\" for y = a*x.", "ai/RefLine"));
    QVERIFY(AiAssistant::requestCreatesName("Plot it as a 3D surface in a window named \"Saddle3D\".", "Saddle3D"));
    QVERIFY(!AiAssistant::requestCreatesName("Create a graph \"DecayPlot\" with two stacked layers: the top one with "
                                             "the \"DecayData\" points.",
                                             "DecayData"));
}

void AiContentTests::textIntoNumericColumn_tableLookedUpAfterCreation()
{
    // LIST 4 item 5 in split mode: newTable("Names", 3, 2) on its own line, then t = table("Names") and t.setText(2,
    // ...).
    const QString code = "newTable(\"Names\", 3, 2)\nt = table(\"Names\")\nt.setText(2, 1, \"alpha\")\n";
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake(code).contains("setColTextFormat(2)"));
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake("newTable(\"Names\", 3, 2)\nt = table(\"Names\")\n"
                                                          "t.setColTextFormat(2)\nt.setText(2, 1, \"alpha\")\n")
                .isEmpty());
    // an existing table looked up, never created here: its column formats are unknown
    QVERIFY(
        QtiSasAiContent::textIntoNumericColumnMistake("t = table(\"Names\")\nt.setText(2, 1, \"alpha\")\n").isEmpty());
}

void AiContentTests::namesSomethingNew_onlyForAResolvedName()
{
    // A one-part plan's wording goes along only when it settled on a name the request left open.
    const QString request = R"(In "Squares", give column 3 a formula. Print the sum. The result label is "R2:".)";
    QVERIFY(!AiAssistant::namesSomethingNew("set column 3 of table 'Squares' to \"pow(column(1), 3)\", labeled \"R2:\"",
                                            request));
    QVERIFY(AiAssistant::namesSomethingNew("create a table named 'MyTable' with 5 rows", "Make a table with 5 rows."));
}

void AiContentTests::bareCompilerDelete_madeCompilerDelete()
{
    // Split-mode LIST 5, item 6: delete("ai/Quad") — NameError, repeated until the item was given up.
    QtiSasAiContent content;
    QCOMPARE(content.bareCompilerDeleteRepair("delete(\"ai/Quad\")\nx = c.delete(\"ai/Lor5\")\n"),
             QString("Compiler.delete(\"ai/Quad\")\nx = c.delete(\"ai/Lor5\")\n"));
    // a method call, one in a tab's text, or a delete the script defines itself: left alone
    QVERIFY(content.bareCompilerDeleteRepair("Compiler.delete(\"ai/Quad\")\n").isEmpty());
    // several literals before the calls (LIST 5 item 6: "deCompiler.lete(" from shifted offsets)
    QCOMPARE(content.bareCompilerDeleteRepair("c = Compiler\ndelete(\"ai/Quad\")\ndelete(\"ai/Gauss5\")\n"),
             QString("c = Compiler\nCompiler.delete(\"ai/Quad\")\nCompiler.delete(\"ai/Gauss5\")\n"));
    QVERIFY(content.bareCompilerDeleteRepair("n.tab(1).setText('delete(1)')\n").isEmpty());
    QVERIFY(content.bareCompilerDeleteRepair("def delete(n):\n    pass\ndelete(1)\n").isEmpty());
}

void AiContentTests::newTableNamesThatRun_skipsGuardedAndQuoted()
{
    // Series 4 (local): a guarded creation on an existing table and a newTable() in a note tab's text were both blocked
    // as "would wipe" and lost their items.
    QCOMPARE(newTableNamesThatRun("t = newTable(\"Peak\", 41, 2)\n"), QStringList{"Peak"});
    QVERIFY(newTableNamesThatRun("t = table(\"Peak\")\nif t is None:\n    t = newTable(\"Peak\", 41, 2)\n").isEmpty());
    QVERIFY(newTableNamesThatRun("t = table(\"Peak\")\nif t is None: t = newTable(\"Peak\", 41, 2)\n").isEmpty());
    QVERIFY(newTableNamesThatRun("if not existTable(\"Peak\"):\n    newTable(\"Peak\", 41, 2)\n").isEmpty());
    QVERIFY(newTableNamesThatRun("n.tab(\"make\").setText('t = newTable(\"Auto\", 5, 1)')\n").isEmpty());
    QVERIFY(newTableNamesThatRun("s = '''t = newTable(\"Auto\", 5, 1)\nt.setCell(1, 1, 1)'''\n").isEmpty());
    // a guard on another table, or no guard at all: the call runs
    QCOMPARE(newTableNamesThatRun("t = table(\"Other\")\nif t is None:\n    t = newTable(\"Peak\", 41, 2)\n"),
             QStringList{"Peak"});
    QCOMPARE(newTableNamesThatRun("for i in range(2):\n    t = newTable(\"Peak\", 41, 2)\n"), QStringList{"Peak"});
}

void AiContentTests::noWorkFailureResult_blocksMadeUpNotFound()
{
    // Series 4 (local, LIST 3 item 3, three runs): only scriptPrint("R3:", -1, "TableNotFound") — "Squares" existed.
    const QString request = R"(In "Squares", give column 4 the formula 1/(column 1 - 5). The result label is "R3:".)";
    QVERIFY(!QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R3:\", -1, \"TableNotFound\")\n", request)
                 .isEmpty());
    QVERIFY(!QtiSasAiContent::noWorkFailureResultMistake("# no table\nscriptPrint(\"R3:\", -1, \"Table not found\")\n",
                                                         request)
                 .isEmpty());
    // a script that looks the table up, a computed value, or no failure claim: left to the other checks
    QVERIFY(QtiSasAiContent::noWorkFailureResultMistake(
                "t = table(\"Squares\")\nif t is None:\n    scriptPrint(\"R3:\", -1, \"TableNotFound\")\n", request)
                .isEmpty());
    QVERIFY(
        QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R3:\", n, \"TableNotFound\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R3:\", 1, \"Squares\")\n", request)
                .isEmpty()); // values written in, none the request's (benchmark SVD, local pyi): blocked; a value the
                             // request gives: not
    const QString svd =
        "Print the number of unknowns and of the determined combinations. The result label is \"R10:\".";
    QVERIFY(!QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R10:\", 3, 45)\n", svd).isEmpty());
    QVERIFY(
        !QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R10:\", round(0.01 + 0.01, 3))\n", svd).isEmpty());
    QVERIFY(!QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R10:\", \"decomposition successful\")\n", svd)
                 .isEmpty());
    QVERIFY(QtiSasAiContent::noWorkFailureResultMistake("s = Svd\nscriptPrint(\"R10:\", 3, 45)\n", svd).isEmpty());
    QVERIFY(QtiSasAiContent::noWorkFailureResultMistake("scriptPrint(\"R10:\", 3, s.rank([0, 1]))\n", svd).isEmpty());
}

void AiContentTests::noteAutoexecReceiver_movedToTheNote()
{
    // Series 4 (local, LIST 3 item 10): n.tab(1).setAutoexec(False) twice, the model moved one, the item was lost.
    QtiSasAiContent content;
    QCOMPARE(content.noteAutoexecReceiverRepair("n.tab(1).setAutoexec(False)\nn.tab(2).setAutoexec(False)\n"),
             QString("n.setAutoexec(False)\nn.setAutoexec(False)\n"));
    QCOMPARE(content.noteAutoexecReceiverRepair("note(\"R\").currentEditor().setAutoexec(True)\n"),
             QString("note(\"R\").setAutoexec(True)\n"));
    QVERIFY(content.noteAutoexecReceiverRepair("n.setAutoexec(True)\n").isEmpty());
    QVERIFY(content.noteAutoexecReceiverRepair("n.tab(1).setText('m.tab(1).setAutoexec(True)')\n").isEmpty());
}

void AiContentTests::existenceReadBeforeDeletion_blocksVariablePrinted()
{
    // Series 4 (local, LIST 5 item 6): t = table("Peak5"); closeWindow(t); printed "t is not None" -> True.
    QVERIFY(!QtiSasAiContent::existenceReadBeforeCreationMistake(
                 "t = table(\"Peak5\")\nif t is not None:\n    closeWindow(t)\nscriptPrint(\"R6:\", t is not None)\n")
                 .isEmpty());
    QVERIFY(!QtiSasAiContent::existenceReadBeforeCreationMistake(
                 "e = existTable(\"Peak5\")\ncloseWindow(table(\"Peak5\"))\nscriptPrint(\"R6:\", e)\n")
                 .isEmpty());
    // read after the delete: fine
    QVERIFY(QtiSasAiContent::existenceReadBeforeCreationMistake(
                "t = table(\"Peak5\")\ncloseWindow(t)\nscriptPrint(\"R6:\", existTable(\"Peak5\"))\n")
                .isEmpty());
}

void AiContentTests::tableTransposedCell_swappedWhenOnlyThatFits()
{
    // LIST 3 item 5 (series 4 and 5, local): newTable("Format", 3, 1) written row-first, "no column 2" three times.
    QtiSasAiContent content;
    QCOMPARE(content.tableTransposedCellRepair("t = newTable(\"Format\", 3, 1)\nt.setCell(1, 1, 1234.5)\n"
                                               "t.setCell(2, 1, 0.25)\nt.setText(2, 1, \"2,5\")\nv = t.cell(2, 1)\n"),
             QString("t = newTable(\"Format\", 3, 1)\nt.setCell(1, 1, 1234.5)\nt.setCell(1, 2, 0.25)\n"
                     "t.setText(1, 2, \"2,5\")\nv = t.cell(1, 2)\n"));
    // already (col, row), or a call with a variable index: left alone
    QVERIFY(content.tableTransposedCellRepair("t = newTable(\"F\", 3, 1)\nt.setCell(1, 3, 1)\n").isEmpty());
    QVERIFY(content.tableTransposedCellRepair("t = newTable(\"F\", 3, 1)\nt.setCell(2, 1, 1)\nt.setCell(1, i, 2)\n")
                .isEmpty());
}

void AiContentTests::resultsAfterConfigure_blocksStartValuesRead()
{
    // LIST 5 item 2 (local, series 4 and 5): the winner configured again, then results() — the start values.
    QVERIFY(!QtiSasAiContent::resultsAfterConfigureMistake(
                 "f.configure(\"ai/G\", [\"P_y\"])\nc1 = f.fit()\nf.configure(\"ai/L\", [\"P_y\"])\nc2 = f.fit()\n"
                 "if c2 < c1:\n    f.configure(\"ai/L\", [\"P_y\"])\n    a, _ = f.results()[\"A\"]\n")
                 .isEmpty());
    // results() after its own fit(), or before any fit at all: fine
    QVERIFY(QtiSasAiContent::resultsAfterConfigureMistake(
                "f.configure(\"ai/G\", [\"P_y\"])\nc1 = f.fit()\na, _ = f.results()[\"A\"]\n")
                .isEmpty());
    QVERIFY(
        QtiSasAiContent::resultsAfterConfigureMistake("f.configure(\"ai/G\", [\"P_y\"])\np = f.results()\n").isEmpty());
}

void AiContentTests::apiCallRepairs_oneFormFixes()
{
    // Series 4–6: each confirmed live more than once.
    QtiSasAiContent content;
    QStringList notes;
    // bare functionExists() -> Compiler.functionExists()
    QCOMPARE(content.apiCallRepairs("x = [n for n in names if functionExists(n)]\n", "Delete them.", true, &notes),
             QString("x = [n for n in names if Compiler.functionExists(n)]\n"));
    QVERIFY(content.apiCallRepairs("c.functionExists(n)\n", "x", true, nullptr).isEmpty());
    QVERIFY(
        content.apiCallRepairs("def functionExists(n):\n    pass\nfunctionExists(1)\n", "x", true, nullptr).isEmpty());
    // setCellData(col, <list>) -> setColData
    QCOMPARE(content.apiCallRepairs("t.setCellData(3, [0.05] * 21)\n", "x", true, nullptr),
             QString("t.setColData(3, [0.05] * 21)\n"));
    QVERIFY(content.apiCallRepairs("t.setCellData(3, 1, 0.05)\n", "x", true, nullptr).isEmpty());
    // Note.setTabText / setTabScript -> the editor's setText
    QCOMPARE(content.apiCallRepairs("n.setTabText(0, '''t = 1\nprint(t)''')\n", "x", true, nullptr),
             QString("n.editorAt(0).setText('''t = 1\nprint(t)''')\n"));
    QCOMPARE(content.apiCallRepairs("n.setTabScript(\"make\", code)\n", "x", true, nullptr),
             QString("n.tab(\"make\").setText(code)\n"));
    // one addTab() too many: the first goes
    const QString tabs = R"(Create a note "Steps" with three tabs named "A", "B" and "C".)";
    QCOMPARE(content.apiCallRepairs("n = newNote(\"Steps\")\nn.addTab()\nn.renameTab(0, \"A\")\nn.addTab()\n"
                                    "n.renameTab(1, \"B\")\nn.addTab()\nn.renameTab(2, \"C\")\n",
                                    tabs, true, nullptr),
             QString("n = newNote(\"Steps\")\nn.renameTab(0, \"A\")\nn.addTab()\nn.renameTab(1, \"B\")\nn.addTab()\n"
                     "n.renameTab(2, \"C\")\n"));
    // the right count, or a numbered tab used before the second addTab(): left alone
    QVERIFY(content.apiCallRepairs("n = newNote(\"Steps\")\nn.addTab()\nn.addTab()\n", tabs, true, nullptr).isEmpty());
    QVERIFY(content
                .apiCallRepairs("n = newNote(\"S\")\nn.addTab()\nn.tab(2).setText('B')\nn.addTab()\nn.addTab()\n", tabs,
                                true, nullptr)
                .isEmpty());
    // int() around an existence value in the result line
    const QString whether = R"(Print whether "Peak5" still exists. The result label is "R6:".)";
    QCOMPARE(content.apiCallRepairs("e = existTable(\"Peak5\")\nscriptPrint(\"R6:\", int(e), int(existNote(\"P\")))\n",
                                    whether, true, nullptr),
             QString("e = existTable(\"Peak5\")\nscriptPrint(\"R6:\", e, existNote(\"P\"))\n"));
    QVERIFY(content.apiCallRepairs("scriptPrint(\"R6:\", int(n))\n", whether, true, nullptr).isEmpty());
    QVERIFY(content.apiCallRepairs("e = existTable(\"P\")\nscriptPrint(\"R6:\", int(e))\n", whether, false, nullptr)
                .isEmpty());
}

void AiContentTests::apiCallRepairs_splitResultLineMerged()
{
    // LIST 3 item 5 (local, series 4 and 6): "R5: 1234,5 0,25 -3" then "2.5" on a line of its own.
    QtiSasAiContent content;
    const QString request =
        "Read the three cells as text, then write \"2,5\" into row 2 and read the number. Print the "
        "three texts read first, then that number. The result label is \"R5:\".";
    const QString code = "a = t.text(1, 1)\nb = t.text(1, 2)\nc = t.text(1, 3)\nscriptPrint(\"R5:\", a, b, c)\n"
                         "t.setText(1, 2, \"2,5\")\nnumber = t.cell(1, 2)\nscriptPrint(number)\n";
    const QString repaired = content.apiCallRepairs(code, request, true, nullptr);
    QCOMPARE(repaired, QString("a = t.text(1, 1)\nb = t.text(1, 2)\nc = t.text(1, 3)\n_result_values = (a, b, c)\n"
                               "t.setText(1, 2, \"2,5\")\nnumber = t.cell(1, 2)\n"
                               "scriptPrint(\"R5:\", *_result_values, number)\n"));
    // complete already, an indented later print, or fixed text: left alone
    QVERIFY(
        content.apiCallRepairs("scriptPrint(\"R5:\", a, b, c, d)\nscriptPrint(x)\n", request, true, nullptr).isEmpty());
    QVERIFY(content.apiCallRepairs("scriptPrint(\"R5:\", a, b, c)\nif x:\n    scriptPrint(n)\n", request, true, nullptr)
                .isEmpty());
    QVERIFY(content.apiCallRepairs("scriptPrint(\"R5:\", a, b, c)\nscriptPrint(\"done\")\n", request, true, nullptr)
                .isEmpty());
    // not the item's last part: no result-line repair
    QVERIFY(content.apiCallRepairs(code, request, false, nullptr).isEmpty());
}

void AiContentTests::textIntoNumericColumn_listOfWords()
{
    // BlaBlaDor series 6, LIST 4 item 5: setText(2, i, ["alpha", "beta", "gamma"][i-1]) with no text format.
    QVERIFY(
        !QtiSasAiContent::textIntoNumericColumnMistake("t = newTable(\"Names\", 3, 2)\nfor i in range(1, 4):\n"
                                                       "    t.setText(2, i, [\"alpha\", \"beta\", \"gamma\"][i-1])\n")
             .isEmpty());
    QVERIFY(QtiSasAiContent::textIntoNumericColumnMistake(
                "t = newTable(\"Names\", 3, 2)\nt.setColTextFormat(2)\nfor i in range(1, 4):\n"
                "    t.setText(2, i, [\"alpha\", \"beta\", \"gamma\"][i-1])\n")
                .isEmpty());
}

void AiContentTests::docTopicForKind_mapsLiveKinds()
{
    QtiSasAiContent content;
    QCOMPARE(content.docTopicForKind("note"), QString("note"));
    QCOMPARE(content.docTopicForKind("3D plot"), QString("graph3d"));
    QVERIFY(content.docTopicForKind("window").isEmpty());
}

void AiContentTests::apiCallRepairs_series7()
{
    QtiSasAiContent content;
    // local LIST 1, item 12: the fit table's name built with the folder cut off; a fit table keeps the full name
    QCOMPARE(content.apiCallRepairs("fc = table(\"fitCurve-\" + better_model.split(\"/\")[1])\n", "x", true, nullptr),
             QString("fc = table(\"fitCurve-\" + better_model)\n"));
    QCOMPARE(content.apiCallRepairs("fc = table('fitCurve-' + m.replace('ai/', ''))\n", "x", true, nullptr),
             QString("fc = table('fitCurve-' + m)\n"));
    QVERIFY(content.apiCallRepairs("fc = table(\"fitCurve-\" + better_model)\n", "x", true, nullptr).isEmpty());
    // AlignPolicy on GraphWindow, never an int
    QCOMPARE(content.apiCallRepairs("g.setAlignPolicy(Graph.AlignPolicy.AlignCanvases)\n", "x", true, nullptr),
             QString("g.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)\n"));
    QCOMPARE(content.apiCallRepairs("g.setAlignPolicy(1)\n", "x", true, nullptr),
             QString("g.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)\n"));
    QVERIFY(content.apiCallRepairs("g.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)\n", "x", true, nullptr)
                .isEmpty());
    // setColTextFormat() before writing a number as text into a numeric column: removed
    QCOMPARE(content.apiCallRepairs("t.setCell(1, 1, 1234.5)\nt.setColTextFormat(1)\nt.setText(1, 2, \"2,5\")\n", "x",
                                    true, nullptr),
             QString("t.setCell(1, 1, 1234.5)\nt.setText(1, 2, \"2,5\")\n"));
    // a word written after it, or no numbers before: kept
    QVERIFY(content
                .apiCallRepairs("t.setCell(1, 1, 1)\nt.setColTextFormat(1)\nt.setText(1, 2, \"beta\")\n", "x", true,
                                nullptr)
                .isEmpty());
    QVERIFY(content.apiCallRepairs("t.setColTextFormat(2)\nt.setText(2, 1, \"2,5\")\n", "x", true, nullptr).isEmpty());
}

void AiContentTests::graph3DName_oldGraphClosedFirst()
{
    // Codex BIGMODEL, "Plot it as a 3D surface instead": the existing 2D graph closed, then newPlot3D on its name.
    QtiSasAiContent content;
    const QString closeFirst = "if existGraph(\"MetricsGraph\"):\n    closeWindow(graphWindow(\"MetricsGraph\"))\n"
                               "g3 = newPlot3D(\"MetricsGraph\", t, \"z\", 0)\n";
    QVERIFY(!content.splitPartScopeViolation(closeFirst, "plot it as a 3D surface", QString())
                 .contains("2D-GraphWindow-ONLY"));
    // the lookup AFTER this script's own newPlot3D() is the confusion the check is for
    const QString lookupAfter = "g3 = newPlot3D(\"S3D\", m)\nscriptPrint(existGraph(\"S3D\"))\n";
    QVERIFY(
        content.splitPartScopeViolation(lookupAfter, "plot a 3D surface", QString()).contains("2D-GraphWindow-ONLY"));
}

void AiContentTests::fitBeforeConfigure_ignoresText()
{
    // BIGMODEL, ai-test-requests item 10: "f.fit()" inside a printed message was taken for a call.
    QVERIFY(QtiSasAiContent::fittableFitBeforeConfigureMistake(
                "scriptPrint(\"A fit must be run first (e.g. f.fit() or f.plotResults()).\")\n")
                .isEmpty());
    QVERIFY(!QtiSasAiContent::fittableFitBeforeConfigureMistake("t = table(\"D\")\nf.fit()\n").isEmpty());
    QVERIFY(QtiSasAiContent::fittableSelectStageBeforeFunctionMistake("scriptPrint(\"f.selectStage('simulate')\")\n")
                .isEmpty());
}

void AiContentTests::unusedParameter_blocksDroppedTerm()
{
    // local series 6/7: "y = A / (1 + arg*arg);" for a function with parameters A, w, c.
    QtiSasAiContent content;
    QVERIFY(content
                .unusedParameterMistake("c.setParamNamesAll([\"A\", \"w\", \"c\"])\n"
                                        "c.setCode(\"double arg = x / w;\\ny = A / (1 + arg*arg);\")\n")
                .contains("c"));
    QVERIFY(content
                .unusedParameterMistake("c.setParamNamesAll([\"A\", \"w\", \"c\"])\n"
                                        "c.setCode(\"double arg = x / w;\\ny = A / (1 + arg*arg) + c;\")\n")
                .isEmpty());
}

void AiContentTests::missingHeaderRetryHint_namesTheInclude()
{
    // Series 4 (local, LIST 5 item 2): setHHeaders('#include "sasviewmodels/gaussian.h"') on a plain formula.
    QtiSasAiContent content;
    const QString hint = content.retryHintForError(
        "RuntimeError: compile: compilation of 'ai/Gauss5' failed: ai/Gauss5.cpp:18:10: fatal error: "
        "'sasviewmodels/gaussian.h' file not found");
    QVERIFY(hint.contains("sasviewmodels/gaussian.h"));
    QVERIFY(hint.contains("setHHeaders()"));
    // series 5 (BlaBlaDor, LIST 3 item 2): a bare column name in a muParser formula
    QVERIFY(content
                .retryHintForError("ValueError: recalculate: column 3 is read-only or its formula failed to evaluate.\n"
                                   "Unexpected token \"x\" found at position 0.")
                .contains("col(\"x\")"));
}

void AiContentTests::recalculateExceptionName_blocksWrappingValueError()
{
    // LIST 3 item 3, every configuration: type(e).__name__ printed ValueError for a ZeroDivisionError formula.
    const QString code = "try:\n    t.recalculate(4)\nexcept Exception as e:\n    name = type(e).__name__\n";
    QVERIFY(QtiSasAiContent::recalculateExceptionNameMistake(code).contains("str(e).splitlines()[1]"));
    QVERIFY(QtiSasAiContent::recalculateExceptionNameMistake("try:\n    t.recalculate(4)\nexcept Exception as e:\n"
                                                             "    name = str(e).splitlines()[1].split(':')[0]\n")
                .isEmpty());
    // series 5: the message's lines read, type().__name__ only the fallback when there is no second line
    QVERIFY(QtiSasAiContent::recalculateExceptionNameMistake(
                "try:\n    t.recalculate(4)\nexcept Exception as e:\n    lines = str(e).splitlines()\n"
                "    name = lines[1].split(':')[0] if len(lines) > 1 else type(e).__name__\n")
                .isEmpty());
    // an exception name read where no recalculate() is involved
    QVERIFY(QtiSasAiContent::recalculateExceptionNameMistake("try:\n    f()\nexcept Exception as e:\n"
                                                             "    n = type(e).__name__\n")
                .isEmpty());
}

void AiContentTests::yNameRepair_writtenNewlineIsAStatementBoundary()
{
    // LIST 5 item 1, split part 2: a one-line body "double x2 = x * x;\nI = a*x2 + b*x + c;" with no setYName().
    QtiSasAiContent content;
    const QString code = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/Quad\")\n"
                         "c.setCode(\"double x2 = x * x;\\nI = a * x2 + b * x + c;\")\nc.compile()\n";
    const QString repaired = content.compilerYNameAssignmentRepair(code);
    QVERIFY(repaired.contains("\\nf = a * x2 + b * x + c;"));
    // and the repaired body passes the never-assigned check
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake(repaired).isEmpty());
    // the block message names the c.new() default, not a setYName() call the script never made
    const QString message = QtiSasAiContent::compilerYNameNeverAssignedMistake(code);
    QVERIFY(message.contains("after c.new()") && !message.contains("set by setYName"));
    QVERIFY(QtiSasAiContent::compilerYNameNeverAssignedMistake("c = Compiler\nc.new()\nc.setYName(\"y\")\n"
                                                               "c.setCode(\"I = a;\")\n")
                .contains("set by setYName(\"y\")"));
}

void AiContentTests::seriesTwoGuards_onlyOnTheirCase()
{
    QtiSasAiContent content;
    // 26: a guard branch's failure report is no placeholder (LIST 3 item 3)
    const QString request = "Print the number of empty cells and the name of the error. The result label is \"R3:\".";
    QVERIFY(
        QtiSasAiContent::placeholderResultValueMistake(
            "if t is None:\n    scriptPrint(\"R3:\", -1, \"TableNotFound\")\nelse:\n    scriptPrint(\"R3:\", n, e)\n",
            request)
            .isEmpty());
    QVERIFY(
        !QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R3:\", n, \"not used\")\n", request).isEmpty());
    // 27: existence read before the creating call, printed after (LIST 5 item 4)
    QVERIFY(QtiSasAiContent::existenceReadBeforeCreationMistake(
                "g = existPlot3D(\"S3\")\nnewPlot3D(\"S3\", m, 5)\nscriptPrint(\"R4:\", g)\n")
                .contains("existPlot3D(\"S3\")"));
    QVERIFY(QtiSasAiContent::existenceReadBeforeCreationMistake(
                "newPlot3D(\"S3\", m, 5)\ng = existPlot3D(\"S3\")\nscriptPrint(\"R4:\", g)\n")
                .isEmpty());
    QVERIFY(QtiSasAiContent::existenceReadBeforeCreationMistake(
                "t = table(\"T\")\nif t is None:\n    t = newTable(\"T\", 3, 2)\nscriptPrint(\"R1:\", t.numRows())\n")
                .isEmpty());
    // 16b: a decimal of the request's data formula missing from the script (LIST 3 item 8)
    const QString data = "Create a table \"D\": x = 0, 0.1, ..., 10, y = 3*exp(-0.5*x) + 0.2, dy = 0.01.";
    QVERIFY(QtiSasAiContent::requestDataNumberMistake("t = newTable(\"D\", 101, 3)\ny = 3*exp(-0.5*x) + 0.02\n", data)
                .contains("0.2"));
    QVERIFY(QtiSasAiContent::requestDataNumberMistake("t = newTable(\"D\", 101, 3)\ny = 3*exp(-0.5*x) + 0.2\n", data)
                .isEmpty());
    // 24: ** in a formula recalculated with muParser forced (Codex, LIST 3 item 2)
    QCOMPARE(content.muParserPowerRepair("t.setCommand(3, \"col(1)**3\")\nt.recalculate(3, 1, -1, True)\n"),
             QString("t.setCommand(3, \"col(1)^3\")\nt.recalculate(3, 1, -1, True)\n"));
    QVERIFY(content.muParserPowerRepair("t.setCommand(3, \"col(1)**3\")\nt.recalculate(3)\n").isEmpty());
    // 28: the (value, error) tuple hint
    QVERIFY(content.retryHintForError("TypeError: bad operand type for abs(): 'tuple'").contains("value, error"));
    QVERIFY(content.retryHintForError("TypeError: unsupported format string passed to tuple.__format__")
                .contains("value, error"));
}

void AiContentTests::bareObjectMethod_blocksRecalculateWithoutTable()
{
    // LIST 3 item 3, all four local runs of series 3: a bare recalculate(4), its NameError caught and printed.
    QVERIFY(QtiSasAiContent::bareObjectMethodMistake("t.setCommand(4, \"1/(col(\\\"x\\\")-5)\")\nrecalculate(4)\n")
                .contains("t.recalculate"));
    QVERIFY(QtiSasAiContent::bareObjectMethodMistake("t.recalculate(4)\nn = t.numRows()\n").isEmpty());
    QVERIFY(QtiSasAiContent::bareObjectMethodMistake("def numRows(t):\n    return 3\nnumRows(t)\n").isEmpty());
    QVERIFY(QtiSasAiContent::bareObjectMethodMistake("n.tab(1).setText('recalculate(1)')\n").isEmpty());
}

void AiContentTests::placeholderResultValue_blocksFixedTextAmongValues()
{
    // LIST 5 NOSPLIT NOEXPLORE local pyi, item 5: the sum replaced by "not used".
    const QString request =
        "Create a note. Print the number of tabs, whether table \"Auto\" exists, and the sum of its "
        "column 1. The result label is \"R5:\".";
    QVERIFY(QtiSasAiContent::placeholderResultValueMistake(
                "scriptPrint(\"R5:\", n.tabs(), existTable(\"Auto\"), \"not used\")\n", request)
                .contains("\"not used\""));
    // all computed, text the request gives itself, a failure report, or all fixed text (another check's): fine
    QVERIFY(
        QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R5:\", n.tabs(), e, s)\n", request).isEmpty());
    QVERIFY(
        QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R5:\", n, \"Auto\", s)\n", request).isEmpty());
    QVERIFY(
        QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R5:\", n, \"not found\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R5:\", \"a\", \"b\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::placeholderResultValueMistake("scriptPrint(\"R5:\", f\"{n}\", s)\n", request).isEmpty());
}

void AiContentTests::creationNeverHappened_coversNamed3DWindow()
{
    // LIST 5 local pyi, item 4 (6 of 6 runs with EXPLORE on): the matrix filled, the 3D window only looked up.
    const QString request = "Create a 21x21 matrix \"Saddle\" and fill it. Plot it as a 3D surface in a window named "
                            "\"Saddle3D\". Print whether a 3D window \"Saddle3D\" exists.";
    const QString filled = "m = newMatrix(\"Saddle\", 21, 21)\nm.calculate()\n";
    QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(filled + "e = plot3D(\"Saddle3D\") is not None\n",
                                                                   request, QString(), QString())
                .contains("Saddle3D"));
    for (const QString &create : {QString("g = newPlot3D(\"Saddle3D\", m, Qti3D.FILLEDMESH)\n"),
                                  QString("g = plot3D(m, 5)\nsetWindowName(g, \"Saddle3D\")\n")})
        QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(filled + create, request, QString(), QString())
                    .isEmpty());
}

void AiContentTests::rowRangeOneShort_blocksLastRowSkipped()
{
    // LIST 5 local pyi, item 1: range(1, 21) over a 21-row table counted 4 rows instead of 5.
    const QString head = "t = newTable(\"Calib\", 21, 3)\n";
    QVERIFY(!QtiSasAiContent::rowRangeEndsOneShortMistake(head +
                                                          "n = 0\nfor i in range(1, 21):\n    if t.cell(2, i) > 5:\n"
                                                          "        n += 1\nscriptPrint(n)\n")
                 .isEmpty());
    QVERIFY(!QtiSasAiContent::rowRangeEndsOneShortMistake("for r in range(1, t.numRows()):\n    t.setCell(1, r, r)\n")
                 .isEmpty());
    // the whole range, a loop over something else, or an end that is not the row count: fine
    QVERIFY(
        QtiSasAiContent::rowRangeEndsOneShortMistake(head + "for i in range(1, 22):\n    t.cell(2, i)\n").isEmpty());
    QVERIFY(QtiSasAiContent::rowRangeEndsOneShortMistake(head + "for i in range(1, 21):\n    s += i\nt.cell(2, i)\n")
                .isEmpty());
    QVERIFY(
        QtiSasAiContent::rowRangeEndsOneShortMistake(head + "for i in range(1, 10):\n    t.cell(2, i)\n").isEmpty());
}

void AiContentTests::quotedNameWithFolder_findsTheFullName()
{
    // LIST 5 local pyi, item 2: "Lor5" configured where the request said "ai/Lor5".
    const QString request = R"(Compile "ai/Gauss5" and "ai/Lor5" and fit both to "Peak5".)";
    QCOMPARE(AiAssistant::quotedNameWithFolder(request, "Lor5"), QString("ai/Lor5"));
    QVERIFY(AiAssistant::quotedNameWithFolder(request, "Peak5").isEmpty());
    QVERIFY(AiAssistant::quotedNameWithFolder(request, "ai/Lor5").isEmpty());
    QVERIFY(AiAssistant::quotedNameWithFolder(request, "Lor").isEmpty());
}

void AiContentTests::repeatedResultLabel_followsTheRequestedCount()
{
    // local pyi, item 6: an early draft line and the real one merged into four values for a three-value item.
    QtiSasAiContent content;
    const QString request =
        "Print which names still exist, then whether \"A\" and \"B\" still exist. The result label is "
        "\"R6:\".";
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R6:\", draft)\nscriptPrint(\"R6:\", fns, a, b)\n", request)
                .startsWith("pass\n"));
    QVERIFY(content.repeatedResultLabelRepair("scriptPrint(\"R6:\", x, y)\nscriptPrint(\"R6:\", fns, a)\n", request)
                .isEmpty());
    QCOMPARE(content.repeatedResultLabelRepair("scriptPrint(\"R6:\", fns)\nscriptPrint(\"R6:\", a, b)\n", request),
             QString("pass\nscriptPrint(\"R6:\", fns, a, b)\n"));
}

void AiContentTests::scriptPrintKeyword_blocksEndOnly()
{
    // LIST 5, item 5: scriptPrint("R5:", n.tabs(), existTable("Auto"), end=" ") — TypeError.
    const QString split = "scriptPrint(\"R5:\", n.tabs(), existTable(\"Auto\"), end=\" \")\n";
    QVERIFY(!QtiSasAiContent::scriptPrintKeywordMistake(split).isEmpty());
    // sep= is supported (qtisasrc.py), only end= raises
    QVERIFY(QtiSasAiContent::scriptPrintKeywordMistake("scriptPrint(a, b, sep=\",\")\n").isEmpty());
    QVERIFY(QtiSasAiContent::scriptPrintKeywordMistake("scriptPrint(\"R5:\", n.tabs(), ok, total)\n").isEmpty());
    QVERIFY(QtiSasAiContent::scriptPrintKeywordMistake("print(a, end=\" \")\nscriptPrint(b)\n").isEmpty());
}

void AiContentTests::currentEditorAfterAddTab_blocksRunningNewTab()
{
    // LIST 5, item 5: "Run only the make tab" — addTab() then currentEditor().execute() ran the "check" tab.
    const QString code = "n = newNote(\"Pipeline\")\nn.renameTab(0, \"make\")\nn.addTab()\nn.renameTab(1, \"check\")\n"
                         "n.currentEditor().execute()\n";
    const QString request = "Create a note \"Pipeline\" with two tabs named \"make\" and \"check\". "
                            "Run only the \"make\" tab.";
    QVERIFY(!QtiSasAiContent::currentEditorAfterAddTabMistake(code, request).isEmpty());
    QVERIFY(
        QtiSasAiContent::currentEditorAfterAddTabMistake("n.addTab()\nn.tab(\"make\").execute()\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::currentEditorAfterAddTabMistake("n.currentEditor().execute()\nn.addTab()\n", request)
                .isEmpty());
    // benchmark LIST 1 item 4: the tab just added is the one to run — no tab named, no block
    QVERIFY(
        QtiSasAiContent::currentEditorAfterAddTabMistake(
            "n = newNote(\"Setup\")\nn.addTab()\nn.currentEditor().setText(\"x = 1\")\nn.currentEditor().execute()\n",
            "Create a note \"Setup\" with an auto-executing script tab. Run that tab once to test it.")
            .isEmpty());
}

void AiContentTests::matrixIndexFormula_blocksIndicesForXyFormula()
{
    // benchmark LIST 5, item 4: setFormula("i*i-j*j") for "fill it with x*x - y*y" — values -440 to 440
    const QString request =
        "Create a 21x21 matrix \"Saddle\" spanning x and y from -1 to 1 and fill it with x*x - y*y.";
    const QString code =
        "m = newMatrix(\"Saddle\", 21, 21)\nm.setCoordinates(-1, 1, -1, 1)\nm.setFormula(\"i*i-j*j\")\n";
    QVERIFY(!QtiSasAiContent::matrixIndexFormulaMistake(code, request).isEmpty());
    QString right = code;
    right.replace("i*i-j*j", "x*x - y*y");
    QVERIFY(QtiSasAiContent::matrixIndexFormulaMistake(right, request).isEmpty());
    // a request in indices is fine with indices
    QVERIFY(QtiSasAiContent::matrixIndexFormulaMistake(code, "Fill matrix M with the formula i+10*j.").isEmpty());
}

void AiContentTests::newNoteExtraTab_blocksOneTabTooMany()
{
    // benchmark LIST 5, item 5: "two tabs" got newNote() and addTab() twice — 3 tabs
    const QString request = R"(Create a note "Pipeline" with two tabs named "make" and "check".)";
    const QString twice = "n = newNote(\"Pipeline\")\nn.addTab()\nn.renameTab(0, \"make\")\nn.addTab()\n"
                          "n.renameTab(1, \"check\")\n";
    QVERIFY(QtiSasAiContent::newNoteExtraTabMistake(twice, request).contains("ends with 3"));
    const QString once = "n = newNote(\"Pipeline\")\nn.renameTab(0, \"make\")\nn.addTab()\nn.renameTab(1, \"check\")\n";
    QVERIFY(QtiSasAiContent::newNoteExtraTabMistake(once, request).isEmpty());
    // an existing note: its tab count is unknown here
    QVERIFY(
        QtiSasAiContent::newNoteExtraTabMistake("n = note(\"Pipeline\")\nn.addTab()\nn.addTab()\n", request).isEmpty());
}

void AiContentTests::staleResultValue_blocksValueReadBeforeTheWork()
{
    // local LIST 1, item 11 (twice): the legend was read, then created, and the old False printed
    const QString restyle = "Restyle the existing graph \"FitResults\" — set the legend font to bold 12pt. Print "
                            "whether that layer has a legend. The result label is \"R11:\".";
    const QString legend = "top_layer = g.layer(1)\nleg = top_layer.legend()\nhas_legend = leg is not None\n"
                           "if leg is None:\n    leg = top_layer.newLegend()\nscriptPrint(\"R11:\", has_legend)\n";
    QVERIFY(QtiSasAiContent::staleResultValueMistake(legend, restyle).contains("has_legend"));
    // local LIST 1, item 3: tabs and autoexec read before addTab() / setAutoexec(), f-string result line
    const QString addTab = "Add a script tab to note \"Batch\". Print the number of tabs of \"Batch\" and whether it "
                           "is set to autoexec. The result label is \"R3:\".";
    const QString tabs = "n = note(\"Batch\")\nnumTabs = n.tabs()\nauto = n.autoexec()\nn.addTab()\n"
                         "n.setAutoexec(True)\nscriptPrint(f\"R3: {numTabs} {auto}\")\n";
    QVERIFY(!QtiSasAiContent::staleResultValueMistake(tabs, addTab).isEmpty());
    // read after the work, a re-read, a difference: fine
    QVERIFY(QtiSasAiContent::staleResultValueMistake("n.addTab()\nnumTabs = n.tabs()\nscriptPrint(\"R3:\", numTabs)\n",
                                                     addTab)
                .isEmpty());
    QVERIFY(QtiSasAiContent::staleResultValueMistake(
                "numTabs = n.tabs()\nn.addTab()\nnumTabs = n.tabs()\nscriptPrint(\"R3:\", numTabs)\n", addTab)
                .isEmpty());
    QVERIFY(QtiSasAiContent::staleResultValueMistake(
                "before = n.tabs()\nn.addTab()\nscriptPrint(\"R3:\", n.tabs() - before)\n", addTab)
                .isEmpty());
    // LIST 1, item 4: the request asks for the state now, then a switch back afterwards
    QVERIFY(QtiSasAiContent::staleResultValueMistake(
                "numTabs = n.tabs()\nn.addTab()\nscriptPrint(\"R4:\", numTabs)\n",
                "Print whether it is on now, and switch it back on afterwards. The result label is \"R4:\".")
                .isEmpty());
}

void AiContentTests::labeledResultLine_blocksLabelsBetweenValues()
{
    // benchmark LIST 5, item 6: "R6: [] Peak5:missing Pipeline:missing" and "R6: [] Table 'Peak5' exists: False ..."
    const QString request = "Print which names still exist, then whether \"Peak5\" exists. The result label is "
                            "\"R6:\".";
    QVERIFY(!QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"R6: {left} Peak5:{p} Pipeline:{q}\")\n", request)
                 .isEmpty());
    QVERIFY(
        QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"R6: {left} Table 'Peak5' exists: {p}\")\n", request)
            .contains("exists:"));
    QVERIFY(
        !QtiSasAiContent::labeledResultLineMistake("scriptPrint(\"R6:\", left, \"Peak5=\", p)\n", request).isEmpty());
    // values only: fine, also with format specs and a literal value
    QVERIFY(QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"R6: {left} {p:.4f} {q}\")\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::labeledResultLineMistake("scriptPrint(\"R6:\", left, p, q)\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::labeledResultLineMistake("scriptPrint(\"R6:\", \"ai/Lor5\", a)\n", request).isEmpty());
    // other prints are not the result line
    QVERIFY(QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"Peak5 exists: {p}\")\n", request).isEmpty());
    // local LIST 1, item 7: a fallback line with no value has nothing to label ("Q=0.1" is prose)
    QVERIFY(
        QtiSasAiContent::labeledResultLineMistake("scriptPrint(\"R6:\", \"Error calculating I at Q=0.1\")\n", request)
            .isEmpty()); // benchmark SVD, local pyi, item 1: a failure report that names what is missing is no result
                         // line
    QVERIFY(QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"R6: Missing tables: {', '.join(m)}\")\n", request)
                .isEmpty());
    QVERIFY(
        !QtiSasAiContent::labeledResultLineMistake("scriptPrint(f\"R6: {left} Peak5:missing\")\n", request).isEmpty());
}

void AiContentTests::inventedDeletionCall_hintsTheRealCall()
{
    // benchmark LIST 5, item 6: deleteTable, deleteWindow, closeTable, deleteFunction, removeFitFunction
    QVERIFY(QtiSasAiContent::inventedDeletionCallHint("NameError: name 'deleteTable' is not defined")
                .contains("closeWindow(w)"));
    QVERIFY(QtiSasAiContent::inventedDeletionCallHint("NameError: name 'closeTable' is not defined")
                .contains("closeWindow(w)"));
    QVERIFY(QtiSasAiContent::inventedDeletionCallHint("NameError: name 'deleteFunction' is not defined")
                .contains("Compiler.delete("));
    QVERIFY(QtiSasAiContent::inventedDeletionCallHint(
                "AttributeError: 'fittable18' object has no attribute 'removeFitFunction'")
                .contains("Compiler.delete("));
    QVERIFY(QtiSasAiContent::inventedDeletionCallHint("NameError: name 'fitFunctions' is not defined").isEmpty());
    QtiSasAiContent content;
    QVERIFY(content.retryHintForError("NameError: name 'deleteWindow' is not defined").contains("closeWindow"));
}

void AiContentTests::noteTabScript_planMustWriteIt()
{
    // LIST 5, item 5: no part wrote a script into a tab — "create the note with two tabs" + "execute the make tab"
    const QString request = "Create a note \"Pipeline\" with two tabs named \"make\" and \"check\". The \"make\" tab "
                            "holds a script that creates a table \"Auto\". Run only the \"make\" tab.";
    AiAssistant::SplitPart notePart, runPart;
    notePart.description = R"(create a note named "Pipeline" with two tabs named "make" and "check")";
    runPart.description = R"(execute the script in the "make" tab of note "Pipeline" to create table "Auto")";
    QVERIFY(splitPlanMissingRequestedActions(request, {notePart, runPart})
                .contains("write the scripts into the note's tabs"));
    AiAssistant::SplitPart writePart;
    writePart.description = R"(add a script to the "make" tab that creates table "Auto", then run the make tab)";
    QVERIFY(!splitPlanMissingRequestedActions(request, {notePart, writePart})
                 .contains("write the scripts into the note's tabs"));
    // no tab in the request: the group does not apply
    QVERIFY(!splitPlanMissingRequestedActions("Write a script that prints 1.", {notePart})
                 .contains("write the scripts into the note's tabs"));
}

void AiContentTests::otherTabRun_blocksRunningAnotherTab()
{
    // LIST 5, item 5: "Run only the "make" tab" — the script also ran the "check" tab
    const QString request = R"(The "make" tab holds a script. Run only the "make" tab. Print the number of tabs.)";
    QVERIFY(QtiSasAiContent::otherTabRunMistake("n.tab(\"make\").execute()\nn.tab(\"check\").execute()\n", request)
                .contains("\"check\""));
    const QString byIndex = "n.renameTab(0, \"make\")\nn.addTab()\nn.renameTab(1, \"check\")\nn.tab(2).execute()\n";
    QVERIFY(!QtiSasAiContent::otherTabRunMistake(byIndex, request).isEmpty());
    QVERIFY(QtiSasAiContent::otherTabRunMistake("n.tab(\"make\").execute()\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::otherTabRunMistake("n.renameTab(0, \"make\")\nn.tab(1).execute()\n", request).isEmpty());
    QVERIFY(QtiSasAiContent::otherTabRunMistake("n.tab(\"check\").execute()\n", "Run the tabs.").isEmpty());
}

void AiContentTests::requestedCreation_tabScriptMessageNamesTheTab()
{
    // LIST 5, item 5: the plain "call newTable(...)" message sent the model toward creating Auto directly
    QtiSasAiContent content;
    const QString request = "Create a note \"Pipeline\" with two tabs named \"make\" and \"check\". The \"make\" tab "
                            "holds a script that creates a table \"Auto\" with 5 rows. Run only the \"make\" tab.";
    const QString code = "n = note(\"Pipeline\")\nn.tab(\"make\").execute()\n";
    QString explanation;
    for (const AiKnownMistakeMatch &m : content.detectKnownMistakes(code, request, "n = newNote(\"Pipeline\")\n"))
        if (m.header == "requested table/matrix/graph/note creation never happened")
            explanation = m.explanation;
    QVERIFY(explanation.contains("n.tab(\"make\").setText"));
    QVERIFY(explanation.contains("Do not create \"Auto\" directly"));
    // the table created inside the tab's script text counts as created
    QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(
                "n.tab(\"make\").setText('t = newTable(\"Auto\", 5, 1)')\n", request, "n = newNote(\"Pipeline\")\n",
                QString())
                .isEmpty());
}

void AiContentTests::earlierPartVariable_blocksUnassignedName()
{
    // LIST 5, item 4: part 2 used part 1's "m" — NameError.
    const QString earlier = "m = newMatrix(\"Saddle\", 21, 21)\nm.calculate()\nc = Compiler\n";
    const QString used = "scriptPrint(\"R4:\", m.cell(11, 11), m.min(), m.max())\n";
    const QString explanation = QtiSasAiContent::earlierPartVariableMistake(used, earlier);
    QVERIFY(explanation.contains("m = matrix(\"Saddle\")"));
    QVERIFY(QtiSasAiContent::earlierPartVariableMistake("m = matrix(\"Saddle\")\n" + used, earlier).isEmpty());
    QVERIFY(QtiSasAiContent::earlierPartVariableMistake("for m in ms:\n    m.calculate()\n", earlier).isEmpty());
    QVERIFY(QtiSasAiContent::earlierPartVariableMistake(used, QString()).isEmpty());
    // a plain alias is repeated as it was
    QVERIFY(QtiSasAiContent::earlierPartVariableMistake("c.delete(\"ai/Quad\")\n", earlier).contains("c = Compiler"));
    // only an attribute use counts; a name inside a string does not
    QVERIFY(QtiSasAiContent::earlierPartVariableMistake("scriptPrint(\"m.cell is fine\")\n", earlier).isEmpty());
}

void AiContentTests::mergeSplitPlanPartsBackward_mergesGivenParts()
{
    // LIST 5, item 4: "Part 2 needs no DOCS topic beyond Part 1 — merge them", unchanged for 2 rounds.
    AiAssistant::SplitPart make, print;
    make.description = "create a 21x21 matrix 'Saddle' and plot it as a 3D surface in window 'Saddle3D'";
    make.docTopics = QStringList{"matrix", "graph3d"};
    print.description = "Print center value, min and max, and whether 'Saddle3D' exists";
    print.docTopics = QStringList{"graph3d"};
    QList<AiAssistant::SplitPart> plan{make, print};
    QCOMPARE(splitPlanAdjacentSameDocs(plan), QList<int>{1});
    QStringList merged;
    mergeSplitPlanPartsBackward(plan, splitPlanAdjacentSameDocs(plan), &merged);
    QCOMPARE(plan.size(), 1);
    QVERIFY(plan[0].description.endsWith(", then print center value, min and max, and whether 'Saddle3D' exists"));
    QCOMPARE(plan[0].docTopics, (QStringList{"matrix", "graph3d"}));
}

void AiContentTests::userCorrection_parsesUrgentPrefix()
{
    bool now = true;
    QCOMPARE(AiAssistant::parseCorrection("  use table B  ", &now), QString("use table B"));
    QVERIFY(!now);
    QCOMPARE(AiAssistant::parseCorrection("! do not plot", &now), QString("do not plot"));
    QVERIFY(now);
    QCOMPARE(AiAssistant::parseCorrection("!", &now), QString()); // nothing to correct
}

void AiContentTests::userCorrection_blockListsEveryCorrection()
{
    QVERIFY(AiAssistant::correctionBlock({}).isEmpty()); // no correction: prompts unchanged
    const QString block = AiAssistant::correctionBlock({"use table B", "do not plot"});
    QVERIFY(block.contains("Correction from the user while this request runs"));
    QVERIFY(block.contains("the correction wins"));
    QVERIFY(block.contains("\n- use table B\n- do not plot"));
}

void AiContentTests::reuseInsteadOfCompile_allowsSplitModePartReusingEarlierPartsCompile()
{
    // Split-mode: an EARLIER part already compiled the function; THIS part correctly just fits it.
    // originalRequestText is the WHOLE multi-part request and still says "compile" regardless of
    // which part does it — must not block a later part for correctly not repeating that work.
    const QString request = "Compile a custom Gaussian peak fit function and fit it with weighted errors.";
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/GaussPeak\", [\"GaussPeak_y\"])\n"
                         "chi2 = f.fit()\n";
    const QString earlierPartsCode = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/GaussPeak\")\n"
                                     "c.setGroup(\"custom:peak\")\nc.setCode(\"...\")\nok = c.compile()\n";
    QVERIFY(QtiSasAiContent::reuseInsteadOfCompileMistake(code, request, earlierPartsCode).isEmpty());
}

void AiContentTests::requestedCreation_blocksTableNeverCreated()
{
    // Confirmed live: the request asked to create table "SAXS_data" and fill it with a sphere
    // form factor plus 3% Gaussian noise. The action script ran without error but only reopened
    // an existing compiled function and printed its parameter names — table "SAXS_data" was never
    // created, nothing was computed, and the imported random module was never called.
    const QString request = "Create a table \"SAXS_data\" with 3 columns Q, I, dI and fill it with "
                            "synthetic small-angle-scattering intensities for a sphere form factor "
                            "plus 3% Gaussian noise.";
    const QString code = "import random\n"
                         "c = Compiler\n"
                         "c.open(\"SimpleSphere\")\n"
                         "scriptPrint(str(c.paramNames()))\n";
    QVERIFY(!QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, QString(), QString()).isEmpty());
}

void AiContentTests::requestedCreation_allowsRealNewTableCall()
{
    const QString request = "Create a table \"SAXS_data\" with 3 columns Q, I, dI and fill it with "
                            "synthetic small-angle-scattering intensities for a sphere form factor "
                            "plus 3% Gaussian noise.";
    const QString code = "import random\n"
                         "t = newTable(\"SAXS_data\", 100, 3)\n"
                         "t.setColName(1, \"Q\"); t.setColName(2, \"I\"); t.setColName(3, \"dI\")\n";
    QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, QString(), QString()).isEmpty());
}

void AiContentTests::requestedCreation_allowsRequestWithNoCreateWord()
{
    // A request that only asks to fill an EXISTING table never says "create" near it — correctly
    // exempt regardless of what the script does.
    const QString request = "Fill the existing table \"SAXS_data\" with simulated intensities.";
    const QString code = "t = table(\"SAXS_data\")\nscriptPrint(str(t.numRows()))\n";
    QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, QString(), QString()).isEmpty());
}

void AiContentTests::requestedCreation_allowsSplitModePartReusingEarlierPartsCreation()
{
    // Split-mode: an EARLIER part already created the table; THIS part correctly just fills it.
    const QString request = "Create a table \"SAXS_data\" with 3 columns, then fill it with simulated "
                            "intensities.";
    const QString code = "t = table(\"SAXS_data\")\nt.setCell(1, 2, 1.23)\n";
    const QString earlierPartsCode = "t = newTable(\"SAXS_data\", 100, 3)\n";
    QVERIFY(
        QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, earlierPartsCode, QString()).isEmpty());
}

void AiContentTests::requestedCreation_allowsSplitModePartDeferringToLaterPartsCreation()
{
    // Confirmed live: a 2-part plan's Part 1 ("create note 'Log'") was permanently blocked for not
    // creating a table that Part 2's OWN description already claimed responsibility for — this
    // part's code has no newTable() and no earlier part does either, but a LATER part's description
    // names the exact table, so this must stay exempt just like the earlier-part case above.
    const QString request = "Create a note \"Log\", a table \"Metrics\" with 20 rows of x,y sample data, "
                            "and a styled graph of it with a legend — all in one request.";
    const QString code = "newNote(\"Log\")\n";
    const QString laterPartsDescriptions = "Create and fill table \"Metrics\" with 20 rows of x,y sample "
                                           "data, then create a styled graph of \"Metrics\" with a legend";
    QVERIFY(QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, QString(), laterPartsDescriptions)
                .isEmpty());
}

void AiContentTests::requestedCreation_blocksMatrixNeverCreated()
{
    const QString request = "Create a matrix \"HeightMap\" and fill it with sin(x)*cos(y) values.";
    const QString code = "m = matrix(\"HeightMap\")\nscriptPrint(str(m.numRows()))\n";
    QVERIFY(!QtiSasAiContent::requestedCreationNeverHappenedMistake(code, request, QString(), QString()).isEmpty());
}

void AiContentTests::hardcodedWinner_blocksHardcodedWinnerWithNoLiveComparison()
{
    // Confirmed live: EXPLORE (an earlier split-mode round) fit both a Gaussian and a Lorentzian
    // peak model and printed the comparison; part 4's action script hardcoded the winner's name
    // with no comparison logic anywhere in the script — correct only by coincidence with this run's
    // data.
    const QString request = "Fit both models and report which model fits better.";
    const QString earlierPartsCode = "f = Fittable\n"
                                     "f.configure(\"ai/GaussianPeak\", [\"GaussPeak_y\"])\n"
                                     "chi2_g = f.fit()\n"
                                     "f2 = Fittable\n"
                                     "f2.configure(\"ai/LorentzianPeak\", [\"GaussPeak_y\"])\n"
                                     "chi2_l = f2.fit()\n";
    const QString code = "fc = table(\"fitCurve-ai/GaussianPeak\")\n"
                         "leg.setText(\"Winner: ai/GaussianPeak\")\n";
    QVERIFY(!QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(code, request, earlierPartsCode).isEmpty());
}

void AiContentTests::hardcodedWinner_allowsNonSplitModeWithEmptyEarlierPartsCode()
{
    // Non-split mode always passes an empty earlierPartsCode — fittedNames.size() < 2 is always
    // true there, so this check can never fire outside split mode, by construction.
    const QString request = "Fit both models and report which model fits better.";
    const QString code = "fc = table(\"fitCurve-ai/GaussianPeak\")\n"
                         "leg.setText(\"Winner: ai/GaussianPeak\")\n";
    QVERIFY(QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(code, request, QString()).isEmpty());
}

void AiContentTests::hardcodedWinner_allowsRequestWithNoComparisonLanguage()
{
    // A split plan that fits two UNRELATED models across parts for reasons other than picking a
    // winner (no comparison ever requested) is correctly exempt.
    const QString request = "Fit a background model, then fit a peak model, then plot the peak model.";
    const QString earlierPartsCode = "f = Fittable\n"
                                     "f.configure(\"ai/Background\", [\"Data_y\"])\n"
                                     "chi2_b = f.fit()\n"
                                     "f2 = Fittable\n"
                                     "f2.configure(\"ai/PeakModel\", [\"Data_y\"])\n"
                                     "chi2_p = f2.fit()\n";
    const QString code = "fc = table(\"fitCurve-ai/PeakModel\")\n";
    QVERIFY(QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(code, request, earlierPartsCode).isEmpty());
}

void AiContentTests::hardcodedWinner_allowsScriptWithOwnLiveComparison()
{
    // The documented-correct pattern: THIS script itself configures both candidates and compares
    // their chi2/DoF live — must not be flagged just because it references both fitCurve tables.
    const QString request = "Fit both models and report which model fits better.";
    const QString earlierPartsCode = "f = Fittable\n"
                                     "f.configure(\"ai/GaussianPeak\", [\"GaussPeak_y\"])\n"
                                     "chi2_g = f.fit()\n"
                                     "f2 = Fittable\n"
                                     "f2.configure(\"ai/LorentzianPeak\", [\"GaussPeak_y\"])\n"
                                     "chi2_l = f2.fit()\n";
    const QString code =
        "f = Fittable\n"
        "f.configure(\"ai/GaussianPeak\", [\"GaussPeak_y\"])\n"
        "chi2_g = f.fit()\n"
        "f2 = Fittable\n"
        "f2.configure(\"ai/LorentzianPeak\", [\"GaussPeak_y\"])\n"
        "chi2_l = f2.fit()\n"
        "fc = table(\"fitCurve-ai/GaussianPeak\") if chi2_g <= chi2_l else table(\"fitCurve-ai/LorentzianPeak\")\n";
    QVERIFY(QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(code, request, earlierPartsCode).isEmpty());
}

void AiContentTests::hardcodedWinner_allowsWhenOnlyOneCandidateWasFitEarlier()
{
    // Only one candidate was genuinely fit anywhere in the plan — no evidence of a real comparison
    // to have skipped re-deriving.
    const QString request = "Fit both models and report which model fits better.";
    const QString earlierPartsCode = "f = Fittable\n"
                                     "f.configure(\"ai/GaussianPeak\", [\"GaussPeak_y\"])\n"
                                     "chi2_g = f.fit()\n";
    const QString code = "fc = table(\"fitCurve-ai/GaussianPeak\")\n";
    QVERIFY(QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(code, request, earlierPartsCode).isEmpty());
}

void AiContentTests::legendMethod_blocksSetPen()
{
    const QString code = "leg = l1.newLegend()\n"
                         "leg.setPen(QPen(QColor(\"black\"), 1))\n";
    QVERIFY(!QtiSasAiContent::hallucinatedLegendMethodMistake(code).isEmpty());
}

void AiContentTests::legendMethod_allowsRealMethods()
{
    const QString code = "leg = l1.newLegend()\n"
                         "leg.setText(\"GaussPeakCustom Fit\")\n"
                         "leg.setFont(QFont(\"Arial\", 10))\n"
                         "leg.setFramePen(QPen(QColor(\"black\"), 1))\n"
                         "leg.setFrameStyle(Frame.FrameStyle.Line.value)\n"
                         "leg.setOriginCoord(0.8, 0.8)\n";
    QVERIFY(QtiSasAiContent::hallucinatedLegendMethodMistake(code).isEmpty());
}

void AiContentTests::windowLayerMethod_blocksReplotOnWindow()
{
    const QString code = "gw = plot(\"simulatedCurve-ai/PorodPowerLaw\", 0.001, 0.5, Graph.CurveType.Line)\n"
                         "gw.replot()\n";
    QVERIFY(!QtiSasAiContent::graphWindowLayerMethodMistake(code).isEmpty());
}

void AiContentTests::windowLayerMethod_blocksSetXTitleOnWindow()
{
    const QString code = "gw = newGraph(\"Sim\")\n"
                         "gw.setXTitle(\"Q\")\n";
    QVERIFY(!QtiSasAiContent::graphWindowLayerMethodMistake(code).isEmpty());
}

void AiContentTests::windowLayerMethod_allowsMethodOnActiveLayer()
{
    const QString code = "gw = plot(t, (\"x\", \"y\"), Graph.CurveType.Line)\n"
                         "g = gw.activeLayer()\n"
                         "g.setXTitle(\"Q\")\n"
                         "g.replot()\n";
    QVERIFY(QtiSasAiContent::graphWindowLayerMethodMistake(code).isEmpty());
}

void AiContentTests::windowLayerMethod_allowsReassignedToLayer()
{
    const QString code = "gw = newGraph(\"Sim\")\n"
                         "gw = gw.activeLayer()\n"
                         "gw.setXTitle(\"Q\")\n"
                         "gw.replot()\n";
    QVERIFY(QtiSasAiContent::graphWindowLayerMethodMistake(code).isEmpty());
}

void AiContentTests::conditionalMaximize_blocksGatedBehindReuseCheck()
{
    // Confirmed live (latent): maximizeWindow() only runs on fresh creation, but arrangeLayers()/
    // setCanvasGeometry() run unconditionally right after — breaks on a re-run against an existing graph.
    const QString code = "gw = graphWindow(\"GaussPeakFit\")\n"
                         "if gw is None:\n"
                         "    gw = newGraph(\"GaussPeakFit\", 2, 2, 1)\n"
                         "    maximizeWindow(gw)\n"
                         "l1 = gw.layer(1)\n"
                         "l2 = gw.layer(2)\n"
                         "gw.setAlignPolicy(GraphWindow.AlignPolicy.AlignCanvases)\n"
                         "gw.arrangeLayers(False, False)\n"
                         "l1.setCanvasGeometry(0, 0, 100, 75)\n";
    QVERIFY(!QtiSasAiContent::conditionalMaximizeWindowMistake(code).isEmpty());
}

void AiContentTests::conditionalMaximize_allowsUnconditionalCall()
{
    const QString code = "gw = graphWindow(\"GaussPeakFit\")\n"
                         "if gw is None:\n"
                         "    gw = newGraph(\"GaussPeakFit\", 2, 2, 1)\n"
                         "maximizeWindow(gw)\n"
                         "l1 = gw.layer(1)\n"
                         "gw.arrangeLayers(False, False)\n"
                         "l1.setCanvasGeometry(0, 0, 100, 75)\n";
    QVERIFY(QtiSasAiContent::conditionalMaximizeWindowMistake(code).isEmpty());
}

void AiContentTests::conditionalMaximize_allowsNoStackedPanelRecipe()
{
    const QString code = "gw = graphWindow(\"Plot\")\n"
                         "if gw is None:\n"
                         "    gw = newGraph(\"Plot\")\n"
                         "    maximizeWindow(gw)\n"
                         "g = gw.activeLayer()\n"
                         "g.replot()\n";
    QVERIFY(QtiSasAiContent::conditionalMaximizeWindowMistake(code).isEmpty());
}

void AiContentTests::conditionalMaximize_allowsFreshGraphAlways()
{
    const QString code = "gw = newGraph(\"GaussPeakFit\", 2, 2, 1)\n"
                         "maximizeWindow(gw)\n"
                         "l1 = gw.layer(1)\n"
                         "gw.arrangeLayers(False, False)\n"
                         "l1.setCanvasGeometry(0, 0, 100, 75)\n";
    QVERIFY(QtiSasAiContent::conditionalMaximizeWindowMistake(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_blocksBareLiteralComparison()
{
    // Confirmed live: this exact guard shape ran unconditionally every time (curveTitle() never
    // equals a bare literal like "Data" — it's always auto-generated as "<table>_<column>"), and
    // the very next split-mode part's own EXPLORE round found 4 curves in a layer that should
    // have held 2, each duplicated exactly once.
    const QString code = "if not any(l1.curveTitle(i) == \"Data\" for i in range(l1.numCurves())):\n"
                         "    l1.insertCurve(t, \"x\", \"y\", Graph.CurveType.Scatter)\n";
    QVERIFY(!QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_blocksBareIdentifierComparison()
{
    // Same shape, comparing against a bare variable (e.g. winner == "ai/GaussPeak") instead of a
    // literal — equally unable to ever match the real "<table>_<column>" auto-generated title.
    const QString code = "if not any(l1.curveTitle(i) == winner for i in range(l1.numCurves())):\n"
                         "    l1.insertCurve(fc_win, \"x\", \"y\", Graph.CurveType.Line)\n";
    QVERIFY(!QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_allowsDynamicReconstruction()
{
    // A guard that genuinely reconstructs the real "<table>_<column>" format via concatenation is
    // not the guessing pattern this check targets.
    const QString code =
        "if not any(l1.curveTitle(i) == t.objectName() + \"_\" + \"y\" for i in range(l1.numCurves())):\n"
        "    l1.insertCurve(t, \"x\", \"y\", Graph.CurveType.Scatter)\n";
    QVERIFY(QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_allowsWhenNoInsertCurveFollows()
{
    // A pure read-only check with no insertCurve()/addCurve() call gated behind it has no
    // duplicate-accumulation risk.
    const QString code = "found = any(l1.curveTitle(i) == \"Data\" for i in range(l1.numCurves()))\n"
                         "scriptPrint(str(found))\n";
    QVERIFY(QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_allowsVariableAssignedViaReconstruction()
{
    // A bare identifier compared against curveTitle() might legitimately hold a properly
    // reconstructed "<table>_<column>" value, just factored into a variable first instead of
    // inlined — must not be punished for that one level of indirection.
    const QString code = "expected = t.objectName() + \"_y\"\n"
                         "if not any(l1.curveTitle(i) == expected for i in range(l1.numCurves())):\n"
                         "    l1.insertCurve(t, \"x\", \"y\", Graph.CurveType.Scatter)\n";
    QVERIFY(QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::curveExistenceGuard_blocksVariableAssignedViaHardcodedLiteral()
{
    // Confirmed live: `winner = "ai/GaussPeakFit"` is, one level of indirection removed, still
    // just a hardcoded literal — chasing the assignment must not accidentally exempt the exact
    // confirmed-live shape it was designed to keep catching.
    const QString code = "winner = \"ai/GaussPeakFit\"\n"
                         "if not any(l1.curveTitle(i) == winner for i in range(l1.numCurves())):\n"
                         "    l1.insertCurve(fc_win, \"x\", \"y\", Graph.CurveType.Line)\n";
    QVERIFY(!QtiSasAiContent::curveExistenceGuardTitleMismatch(code).isEmpty());
}

void AiContentTests::windowNameMethod_blocksMethodCallForm()
{
    // Confirmed live, history-triggered: g3.setWindowName("SphereMap") raised AttributeError.
    const QString code = "g3 = plot3D(m, style=5)\n"
                         "g3.setWindowName(\"SphereMap\")\n";
    const QString explanation = QtiSasAiContent::windowNameMethodCallMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("setWindowName(g3, \"SphereMap\")"));
}

void AiContentTests::windowNameMethod_allowsFreeFunctionForm()
{
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "setWindowName(g3, \"SinCos3D\")\n";
    QVERIFY(QtiSasAiContent::windowNameMethodCallMistake(code).isEmpty());
}

void AiContentTests::windowNameMethod_allowsNoSetWindowNameAtAll()
{
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "g3.setFilledMeshStyle()\n";
    QVERIFY(QtiSasAiContent::windowNameMethodCallMistake(code).isEmpty());
}

void AiContentTests::fittableSelectStageBeforeFunction_blocksSimulateBeforeConfigure()
{
    // Confirmed live: f.selectStage("simulate") called before f.configure(...) failed at runtime
    // with RuntimeError: selectStage(): call setFunction() first (currently in init stage).
    const QString code = "f = Fittable\n"
                         "f.selectStage(\"simulate\")\n"
                         "f.setSimUniformX(True)\n"
                         "f.configure(\"ai/PorodFit\", [])\n"
                         "chi2 = f.simulateCurve()\n";
    const QString explanation = QtiSasAiContent::fittableSelectStageBeforeFunctionMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("configure()/setFunction()"));
}

void AiContentTests::fittableSelectStageBeforeFunction_allowsConfigureFirst()
{
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/PorodFit\", [])\n"
                         "f.selectStage(\"simulate\")\n"
                         "chi2 = f.simulateCurve()\n";
    QVERIFY(QtiSasAiContent::fittableSelectStageBeforeFunctionMistake(code).isEmpty());
}

void AiContentTests::fittableSelectStageBeforeFunction_allowsSelectStageInitAlone()
{
    // selectStage("init") is valid from init stage itself -- never flagged regardless of order.
    const QString code = "f = Fittable\nf.selectStage(\"init\")\n";
    QVERIFY(QtiSasAiContent::fittableSelectStageBeforeFunctionMistake(code).isEmpty());
}

void AiContentTests::fittableSelectStageBeforeFunction_allowsSetFunctionFirst()
{
    const QString code = "f = Fittable\n"
                         "f.setFunction(\"ai/PorodFit\")\n"
                         "f.selectStage(\"simulate\")\n";
    QVERIFY(QtiSasAiContent::fittableSelectStageBeforeFunctionMistake(code).isEmpty());
}

void AiContentTests::creationChain_blocksChainedSetterCall()
{
    // Confirmed live: newMatrix("SphereMap", 50, 50).setFormula("sin(x)*cos(y)").calculate() —
    // AttributeError: 'NoneType' object has no attribute 'calculate'. Deliberately includes
    // parens INSIDE the quoted formula argument — the first version of this check's own regex
    // used [^()]* for argument content and silently never matched this exact motivating example
    // because of them; this test guards against reintroducing that.
    QtiSasAiContent content;
    const QString code = "newMatrix(\"SphereMap\", 50, 50).setFormula(\"sin(x)*cos(y)\").calculate()";
    const QString explanation = QtiSasAiContent::creationChainedMethodCallMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("separate statement"));
}

void AiContentTests::creationChain_allowsSingleChainedCall()
{
    // Only ONE method chained directly onto the constructor is fine — the break only happens
    // when a THIRD call chains onto a setter's None return, not the constructor's own real one.
    const QString code = "newMatrix(\"SphereMap\", 50, 50).setFormula(\"sin(x)*cos(y)\")";
    QVERIFY(QtiSasAiContent::creationChainedMethodCallMistake(code).isEmpty());
}

void AiContentTests::creationChain_allowsSeparateStatements()
{
    const QString code = "m = newMatrix(\"SphereMap\", 50, 50)\n"
                         "m.setFormula(\"sin(x)*cos(y)\")\n"
                         "m.calculate()\n";
    QVERIFY(QtiSasAiContent::creationChainedMethodCallMistake(code).isEmpty());
}

void AiContentTests::splitPlanFinal_extractsSectionAfterMarker()
{
    // Confirmed live: the draft/check/final two-stage format's draft section can legitimately
    // contain a part later corrected away in the final section — parsing the draft too would
    // resurrect a mistake the model itself already caught and fixed.
    const QString response = "PART: fit both models | DOCS: fittable\n"
                             "PART: compare chi2/dof | DOCS: fittable\n"
                             "---CHECK---\n"
                             "These should be merged under the same-DOCS rule.\n"
                             "---FINAL---\n"
                             "PART: fit both models and compare chi2/dof | DOCS: fittable\n";
    const QString section = splitPlanFinalSection(response);
    QVERIFY(!section.contains("---CHECK---"));
    QVERIFY(!section.contains("compare chi2/dof | DOCS: fittable\n---CHECK---"));
    QVERIFY(section.contains("fit both models and compare chi2/dof"));
    QCOMPARE(section.trimmed(), QString("PART: fit both models and compare chi2/dof | DOCS: fittable"));
}

void AiContentTests::splitPlanFinal_returnsWholeTextWhenNoMarker()
{
    // A model can still answer a two-stage-format prompt with a plain PART list and no marker at
    // all (mlx does this often enough) — that legitimately has no marker, and must be used as-is.
    const QString response = "PART: create table named 'X' | DOCS: table\n";
    QCOMPARE(splitPlanFinalSection(response), response);
}

void AiContentTests::splitPlanFinal_ignoresCheckSectionBeforeFinal()
{
    const QString response = "PART: draft one | DOCS: table\n"
                             "---CHECK---\n"
                             "No issues found.\n"
                             "---FINAL---\n"
                             "PART: draft one | DOCS: table\n";
    const QString section = splitPlanFinalSection(response);
    QVERIFY(!section.contains("No issues found"));
    QVERIFY(section.contains("PART: draft one"));
}

void AiContentTests::splitPlanFinal_toleratesDashCountVariation()
{
    // Confirmed by design, not yet a live incident — models don't always reproduce a literal
    // dash count exactly; the marker detection shouldn't be brittle to that.
    const QString response = "PART: draft | DOCS: none\n--FINAL--\nPART: final | DOCS: none\n";
    QCOMPARE(splitPlanFinalSection(response).trimmed(), QString("PART: final | DOCS: none"));
}

void AiContentTests::splitPlanFinal_handlesBracketedDoubleMarker()
{
    // Confirmed live, severe: the model bracketed its answer with "---FINAL---" on BOTH sides
    // (open AND close) instead of using it as a single opening delimiter the prompt asked for.
    // "everything after the LAST marker" landed after the CLOSING one and returned nothing,
    // discarding an otherwise fully correct, self-corrected 2-part plan and forcing an
    // unnecessary fallback to non-split mode for a real live request.
    const QString response = "PART: draft one | DOCS: graph3d\n"
                             "---CHECK---\n"
                             "reasoning about rules here\n"
                             "---FINAL---\n"
                             "PART: plot a 3D surface... | DOCS: graph3d\n"
                             "PART: add axis labels and legend to the graph named \"SurfacePlot\" | DOCS: graph3d\n"
                             "---FINAL---\n";
    const QString section = splitPlanFinalSection(response);
    QVERIFY(!section.contains("draft one"));
    QVERIFY(!section.contains("reasoning about rules"));
    QVERIFY(section.contains("plot a 3D surface"));
    QVERIFY(section.contains("SurfacePlot"));
}

void AiContentTests::splitPlanFinal_fallsBackToDraftWhenMarkerTrailsEmpty()
{
    // Confirmed live: a response can write "---FINAL---" and then simply stop (ran out of
    // budget, or otherwise never produced anything after it) — falling back to the draft that
    // preceded the marker is strictly better than treating the whole response as unparseable
    // when a perfectly usable draft is sitting right there.
    const QString response = "PART: draft that never got finalized | DOCS: table\n---FINAL---\n   \n";
    QCOMPARE(splitPlanFinalSection(response).trimmed(), QString("PART: draft that never got finalized | DOCS: table"));
}

void AiContentTests::unwrapScriptPrint_rewritesOneCallPerLine()
{
    // Confirmed live against a non-mlx backend: the model wrapped every line of an otherwise
    // correct draft/check/final answer in its own scriptPrint(...) call, mixing double- and
    // single-quoted string arguments (the single-quoted PART line contains unescaped literal
    // double quotes around the table name, valid Python since the enclosing quotes are single).
    const QString response = "scriptPrint(\"Stage 1 — DRAFT:\")\n"
                             "scriptPrint('PART: create table named \"SAXS_data\" with 3 columns | DOCS: table')\n"
                             "scriptPrint(\"\")\n"
                             "scriptPrint(\"---CHECK---\")\n"
                             "scriptPrint(\"No fixes needed.\")\n"
                             "scriptPrint(\"\")\n"
                             "scriptPrint(\"---FINAL---\")\n"
                             "scriptPrint('PART: create table named \"SAXS_data\" with 3 columns | DOCS: table')\n";
    const QString unwrapped = unwrapScriptPrintCalls(response);
    QVERIFY(!unwrapped.contains("scriptPrint"));
    QVERIFY(unwrapped.contains("---FINAL---"));
    QVERIFY(unwrapped.contains("PART: create table named \"SAXS_data\" with 3 columns | DOCS: table"));
}

void AiContentTests::unwrapScriptPrint_rewritesSingleCallWithEscapedNewlines()
{
    // Confirmed live: the same wrapping habit can also show up as ONE scriptPrint(...) call whose
    // string argument contains literal "\n" escape sequences rather than real newlines — the
    // entire draft/check/final answer arrives as a single physical line.
    const QString response = "scriptPrint(\"Stage 1 — DRAFT:\\nPART: create table named 'X' | DOCS: table\\n"
                             "---CHECK---\\nNo issues.\\n---FINAL---\\nPART: create table named 'X' | DOCS: table\")";
    const QString unwrapped = unwrapScriptPrintCalls(response);
    QVERIFY(!unwrapped.contains("scriptPrint"));
    const QStringList lines = unwrapped.split('\n');
    QVERIFY(lines.contains(QString("---FINAL---")));
    QCOMPARE(lines.last(), QString("PART: create table named 'X' | DOCS: table"));
}

void AiContentTests::unwrapScriptPrint_leavesPlainPartLinesUnchanged()
{
    // The common, already-working case (mlx, most of the time) — a plain PART/DOCS response with
    // no code wrapping at all must pass through byte-identical, never a false-positive rewrite.
    const QString response = "PART: fit both models | DOCS: fittable\n"
                             "PART: compare chi2/dof | DOCS: fittable\n";
    QCOMPARE(unwrapScriptPrintCalls(response), response);
}

void AiContentTests::unwrapScriptPrint_thenFinalSection_extractsRealPlanFromWrappedResponse()
{
    // Integration check matching handleSplitPlanResponse()'s actual call order: unwrap first, THEN
    // find the final section — a "---FINAL---" marker sitting inside a wrapped string's quotes
    // must be found exactly as a real one would be, with no leftover call syntax contaminating
    // what gets extracted afterward.
    const QString response = "scriptPrint(\"Stage 1 — DRAFT:\")\n"
                             "scriptPrint('PART: create table named \"SAXS_data\" with 3 columns | DOCS: table')\n"
                             "scriptPrint(\"---CHECK---\")\n"
                             "scriptPrint(\"No fixes needed.\")\n"
                             "scriptPrint(\"---FINAL---\")\n"
                             "scriptPrint('PART: create table named \"SAXS_data\" with 3 columns | DOCS: table')\n";
    const QString section = splitPlanFinalSection(unwrapScriptPrintCalls(response));
    QCOMPARE(section.trimmed(), QString("PART: create table named \"SAXS_data\" with 3 columns | DOCS: table"));
}

void AiContentTests::lookupRegression_blocksMatrixLookupReplacedByCreation()
{
    // Confirmed live: a correct `m = matrix("SphereMap")` lookup (grounded in real cross-turn
    // history) was rewritten by self-review into a from-scratch newMatrix() recreation with an
    // invented constructor signature.
    QtiSasAiContent content;
    const QString preReviewDraft = "m = matrix(\"SphereMap\")\ng3 = plot3D(m, 5)\n";
    const QString postReviewCode = "m = newMatrix(\"SphereMap\", -3.14159, 3.14159, 50, -3.14159, 3.14159, 50)\n"
                                   "m.setFormula(\"sin(x)*cos(y)\")\nm.calculate()\ng3 = plot3D(m, 5)\n";
    const QString explanation = content.selfReviewLookupToCreationRegression(preReviewDraft, postReviewCode);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("matrix(\"SphereMap\")"));
}

void AiContentTests::lookupRegression_allowsConsistentLookupInBoth()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "m = matrix(\"SphereMap\")\ng3 = plot3D(m, 5)\n";
    const QString postReviewCode = "m = matrix(\"SphereMap\")\nsetWindowName(g3, \"Surface\")\ng3 = plot3D(m, 5)\n";
    QVERIFY(content.selfReviewLookupToCreationRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::lookupRegression_allowsGenuineCreationInBoth()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "m = newMatrix(\"SphereMap\", 50, 50)\n";
    const QString postReviewCode = "m = newMatrix(\"SphereMap\", 50, 50)\nm.setFormula(\"sin(x)*cos(y)\")\n";
    QVERIFY(content.selfReviewLookupToCreationRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::lookupRegression_allowsLookupOrCreateIdiomUnchangedByReview()
{
    // Confirmed live (false positive, now fixed): the legitimate "look up or create" idiom has
    // BOTH note("Setup") and newNote("Setup") in the SAME, already-correct draft — self-review
    // making an unrelated edit elsewhere must not be flagged as a lookup-to-creation regression.
    QtiSasAiContent content;
    const QString preReviewDraft = "n = note(\"Setup\")\nif n is None:\n    n = newNote(\"Setup\")\n"
                                   "n.addTab()\n";
    const QString postReviewCode = "n = note(\"Setup\")\nif n is None:\n    n = newNote(\"Setup\")\n"
                                   "n.addTab()\nn.setAutoexec(True)\n";
    QVERIFY(content.selfReviewLookupToCreationRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::qti3DImport_blocksPyQt6ModulePath()
{
    // Confirmed live: from PyQt6.Qt3DExtras import Qti3D raised ImportError.
    const QString code = "from PyQt6.Qt3DExtras import Qti3D\ng3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(!QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::qti3DImport_blocksQtiModulePath()
{
    // Confirmed live, same mistake with a different guessed module path: raised ModuleNotFoundError.
    const QString code = "from qti.Qti3D import Qti3D\ng3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(!QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::qti3DImport_blocksBareImport()
{
    const QString code = "import Qti3D\ng3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(!QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::qti3DImport_blocksSubmoduleForm()
{
    // Qti3D lives directly in the "qti" module's namespace, not a "qti.Qti3D" submodule.
    const QString code = "import qti.Qti3D\ng3.setPlotStyle(qti.Qti3D.FILLEDMESH)\n";
    QVERIFY(!QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::qti3DImport_allowsRealQtiModuleImport()
{
    // Confirmed live: unlike every other guessed module path, "qti" is QtiSAS's real extension
    // module and Qti3D genuinely lives there — the script ran past this line without error.
    const QString code = "from qti import Qti3D, Graph3D\ng3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::qti3DImport_allowsGlobalUsageWithNoImport()
{
    const QString code = "g3 = plot3D(\"sin(x)*cos(y)\", -pi, pi, -pi, pi, -1, 1, 40, 40)\n"
                         "g3.setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(QtiSasAiContent::qti3DImportMistake(code).isEmpty());
}

void AiContentTests::setAutoexec_blocksOnCurrentEditor()
{
    // Confirmed live: n.currentEditor().setAutoexec(True) raised AttributeError.
    const QString code = "n = newNote(\"Setup\")\nn.addTab()\nn.currentEditor().setAutoexec(True)\n";
    const QString explanation = QtiSasAiContent::setAutoexecOnScriptEditMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("n.setAutoexec(True)"));
}

void AiContentTests::setAutoexec_blocksOnTabCall()
{
    const QString code = "n = newNote(\"Setup\")\nn.tab(1).setAutoexec(True)\n";
    QVERIFY(!QtiSasAiContent::setAutoexecOnScriptEditMistake(code).isEmpty());
}

void AiContentTests::setAutoexec_allowsOnNoteObject()
{
    const QString code = "n = newNote(\"Setup\")\nn.addTab()\nn.setAutoexec(True)\n";
    QVERIFY(QtiSasAiContent::setAutoexecOnScriptEditMistake(code).isEmpty());
}

void AiContentTests::plotStyleString_blocksSetPlotStyle()
{
    // Confirmed live: self-review turned setPlotStyle(Qti3D.FILLEDMESH) into
    // setPlotStyle("Filled Mesh"), raising TypeError.
    const QString code = "g3.curve(0).setPlotStyle(\"Filled Mesh\")\n";
    QVERIFY(!QtiSasAiContent::plotStyleStringArgumentMistake(code).isEmpty());
}

void AiContentTests::plotStyleString_blocksCustomPlotStyle()
{
    const QString code = "plot.customPlotStyle(\"Filled\")\n";
    QVERIFY(!QtiSasAiContent::plotStyleStringArgumentMistake(code).isEmpty());
}

void AiContentTests::plotStyleString_allowsQti3DConstant()
{
    const QString code = "g3.curve(0).setPlotStyle(Qti3D.FILLEDMESH)\n";
    QVERIFY(QtiSasAiContent::plotStyleStringArgumentMistake(code).isEmpty());
}

void AiContentTests::missingPyQtImport_blocksUnimportedQColor()
{
    // Confirmed live: a script called QColor(...) with no PyQt import, raising NameError at
    // runtime — the checklist already says to check for this, but self-review still misses it.
    const QString code = "leg.setBackgroundColor(QColor(255, 255, 255, 230))\n";
    QVERIFY(!QtiSasAiContent::missingPyQtImportMistake(code).isEmpty());
}

void AiContentTests::missingPyQtImport_blocksUnimportedQtNamespace()
{
    const QString code = "leg.setBackgroundColor(Qt.red)\n";
    QVERIFY(!QtiSasAiContent::missingPyQtImportMistake(code).isEmpty());
}

void AiContentTests::missingPyQtImport_allowsWithImport()
{
    const QString code = "from PyQt6.QtGui import QColor\n"
                         "leg.setBackgroundColor(QColor(255, 255, 255, 230))\n";
    QVERIFY(QtiSasAiContent::missingPyQtImportMistake(code).isEmpty());
}

void AiContentTests::missingPyQtImport_allowsQualifiedReference()
{
    // QtGui.QColor(...) is a fully-qualified reference needing no separate `from ... import` line.
    const QString code = "import PyQt6.QtGui as QtGui\n"
                         "leg.setBackgroundColor(QtGui.QColor(255, 255, 255, 230))\n";
    QVERIFY(QtiSasAiContent::missingPyQtImportMistake(code).isEmpty());
}

void AiContentTests::missingPyQtImport_allowsBareHexColor()
{
    const QString code = "leg.setBackgroundColor(0xFFFFFF)\n";
    QVERIFY(QtiSasAiContent::missingPyQtImportMistake(code).isEmpty());
}

void AiContentTests::missingRandomImport_blocksUnimportedUsage()
{
    // Confirmed live, repeatedly: a table-creation script adding synthetic noise via
    // random.gauss(...) with no `import random` line, raising NameError at runtime.
    const QString code = "y_obs = y_true + random.gauss(0, dy)\n";
    QVERIFY(!QtiSasAiContent::missingRandomImportMistake(code).isEmpty());
}

void AiContentTests::missingRandomImport_allowsWithImport()
{
    const QString code = "import random\n"
                         "y_obs = y_true + random.gauss(0, dy)\n";
    QVERIFY(QtiSasAiContent::missingRandomImportMistake(code).isEmpty());
}

void AiContentTests::missingRandomImport_allowsNumpyRandomReference()
{
    // np.random.gauss(...) needs `import numpy as np`, not `import random` — a different,
    // unrelated import this check doesn't attempt to verify.
    const QString code = "import numpy as np\n"
                         "y_obs = y_true + np.random.normal(0, dy)\n";
    QVERIFY(QtiSasAiContent::missingRandomImportMistake(code).isEmpty());
}

void AiContentTests::missingRandomImport_allowsVariableNamedRandomPrefix()
{
    // my_random.gauss(...) — "random" here is a suffix of a different identifier, not the module;
    // the lookbehind must not misflag it.
    const QString code = "my_random = SomeGenerator()\n"
                         "y_obs = y_true + my_random.gauss(0, dy)\n";
    QVERIFY(QtiSasAiContent::missingRandomImportMistake(code).isEmpty());
}

void AiContentTests::bareColumnNameInColumnsList_blocksBareLiteral()
{
    // Confirmed live, repeatedly: columnsList() returns QUALIFIED "TableName_column" labels, so
    // "z" in columnsList(Table.PlotDesignation.Z) is always False even when the role IS set.
    const QString code = "role_ok = \"z\" in columnsList(Table.PlotDesignation.Z)\n";
    const QString explanation = QtiSasAiContent::bareColumnNameInColumnsListMistake(code);
    QVERIFY(!explanation.isEmpty());
    QVERIFY(explanation.contains("\"z\""));
}

void AiContentTests::bareColumnNameInColumnsList_allowsQualifiedName()
{
    // "Baseline_z" already has the table-name prefix real columnsList() output uses — a
    // legitimate check, not the bug.
    const QString code = "role_ok = \"Baseline_z\" in columnsList(Table.PlotDesignation.Z)\n";
    QVERIFY(QtiSasAiContent::bareColumnNameInColumnsListMistake(code).isEmpty());
}

void AiContentTests::bareColumnNameInColumnsList_allowsUnrelatedColumnsListUse()
{
    // Printing the list, or comparing against something else entirely, is not this mistake.
    const QString code = "scriptPrint(str(columnsList(Table.PlotDesignation.X)))\n";
    QVERIFY(QtiSasAiContent::bareColumnNameInColumnsListMistake(code).isEmpty());
}

void AiContentTests::enumRegression_blocksQti3DConstantReplacedByString()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "g3.curve(0).setPlotStyle(Qti3D.FILLEDMESH)\n";
    const QString postReviewCode = "g3.curve(0).setPlotStyle(\"Filled Mesh\")\n";
    QVERIFY(!content.selfReviewEnumConstantRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::enumRegression_allowsConstantUnchanged()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "g3.curve(0).setPlotStyle(Qti3D.FILLEDMESH)\ng3.update()\n";
    const QString postReviewCode = "g3.curve(0).setPlotStyle(Qti3D.FILLEDMESH)\ng3.update()\n";
    QVERIFY(content.selfReviewEnumConstantRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::enumRegression_allowsUnrelatedEditWithCallRemoved()
{
    // The whole setPlotStyle(...) call was legitimately removed, not mangled — not this
    // regression's shape.
    QtiSasAiContent content;
    const QString preReviewDraft = "g3.curve(0).setPlotStyle(Qti3D.FILLEDMESH)\ng3.update()\n";
    const QString postReviewCode = "g3.update()\n";
    QVERIFY(content.selfReviewEnumConstantRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::compileSequenceRegression_blocksDroppedSetupCalls()
{
    // Confirmed live: a correct, complete compile setup was replaced wholesale with old-style
    // raw-array C-function code missing setGroup()/parameter declarations entirely, still ending
    // in compile() — raised ValueError: "compile: group is empty" on the very next run.
    QtiSasAiContent content;
    const QString preReviewDraft = "c = Compiler\n"
                                   "c.new()\n"
                                   "c.setFunctionName(\"ai/GaussPeak\")\n"
                                   "c.setGroup(\"custom:peaks\")\n"
                                   "c.setParamCount(4)\n"
                                   "c.setParamNamesAll([\"scale\", \"xc\", \"w\", \"background\"])\n"
                                   "c.setParamInitsAll([4.0, 0.6, 0.2, 0.0])\n"
                                   "c.setParamLimitsAll([\"0..\", \"..\", \"0..\", \"0..\"])\n"
                                   "c.setParamVariesAll([True, True, True, True])\n"
                                   "c.setCode(\"I = scale * exp(-pow((x - xc) / w, 2.0) / 2.0) + background;\")\n"
                                   "ok = c.compile()\n";
    const QString postReviewCode = "c = Compiler\n"
                                   "c.setFunctionName(\"ai/GaussPeak\")\n"
                                   "c.setCode(\"double GaussPeak(double* x, double* par) { return par[0]; }\")\n"
                                   "ok = c.compile()\n";
    QVERIFY(!content.selfReviewCompileSequenceRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::compileSequenceRegression_allowsCompleteSequenceUnchanged()
{
    QtiSasAiContent content;
    const QString code = "c = Compiler\n"
                         "c.new()\n"
                         "c.setFunctionName(\"ai/GaussPeak\")\n"
                         "c.setGroup(\"custom:peaks\")\n"
                         "c.setParamCount(4)\n"
                         "c.setParamNamesAll([\"scale\", \"xc\", \"w\", \"background\"])\n"
                         "c.setParamInitsAll([4.0, 0.6, 0.2, 0.0])\n"
                         "c.setParamLimitsAll([\"0..\", \"..\", \"0..\", \"0..\"])\n"
                         "c.setParamVariesAll([True, True, True, True])\n"
                         "ok = c.compile()\n";
    QVERIFY(content.selfReviewCompileSequenceRegression(code, code).isEmpty());
}

void AiContentTests::compileSequenceRegression_allowsWhenNeitherCompiles()
{
    // Legitimate decision to stop compiling entirely is a different, unrelated edit.
    QtiSasAiContent content;
    const QString preReviewDraft = "f = Fittable\nf.configure(\"Guinier\", [\"data_I\"])\nf.fit()\n";
    const QString postReviewCode = "f = Fittable\nf.configure(\"Guinier\", [\"data_I\"])\nchi2 = f.fit()\n";
    QVERIFY(content.selfReviewCompileSequenceRegression(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::pyQtVersionSubstitution_blocksPyQt6ChangedToPyQt5()
{
    // Confirmed live: a correct `from PyQt6.QtGui import QColor, QFont` draft (this build's real
    // binding) was silently rewritten by self-review, while fixing an unrelated issue elsewhere in
    // the same script, to `from PyQt5.QtGui import QColor, QFont` — PyQt5 doesn't exist in this
    // environment; the script reached actual execution and crashed with ModuleNotFoundError a full
    // round-trip later.
    QtiSasAiContent content;
    const QString preReviewDraft = "from PyQt6.QtGui import QColor, QFont\n"
                                   "leg.setTextColor(QColor(30, 30, 30))\n";
    const QString postReviewCode = "from PyQt5.QtGui import QColor, QFont\n"
                                   "leg.setTextColor(QColor(30, 30, 30))\n";
    QVERIFY(!content.selfReviewPyQtVersionSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::pyQtVersionSubstitution_blocksPyQt5ChangedToPyQt6()
{
    // Symmetric case: a genuinely PyQt5-only build's correct draft gets rewritten to PyQt6 — self-
    // review has no more basis to make this swap than the reverse.
    QtiSasAiContent content;
    const QString preReviewDraft = "from PyQt5.QtWidgets import QFrame\n"
                                   "leg.setFrameStyle(QFrame.Shadow.value)\n";
    const QString postReviewCode = "from PyQt6.QtWidgets import QFrame\n"
                                   "leg.setFrameStyle(QFrame.Shadow.value)\n";
    QVERIFY(!content.selfReviewPyQtVersionSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::pyQtVersionSubstitution_allowsSameVersionUnchanged()
{
    QtiSasAiContent content;
    const QString code = "from PyQt6.QtGui import QColor, QFont\n"
                         "leg.setTextColor(QColor(30, 30, 30))\n";
    QVERIFY(content.selfReviewPyQtVersionSubstitution(code, code).isEmpty());
}

void AiContentTests::pyQtVersionSubstitution_allowsWhenNeitherImportsQt()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "f = Fittable\nf.configure(\"Guinier\", [\"data_I\"])\nf.fit()\n";
    const QString postReviewCode = "f = Fittable\nf.configure(\"Guinier\", [\"data_I\"])\nchi2 = f.fit()\n";
    QVERIFY(content.selfReviewPyQtVersionSubstitution(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::fabricatedSetCode_blocksAddedSetCodeForReusedFunction()
{
    // Confirmed live, severe: EXPLORE had already printed the EXISTING, correct code for
    // "ai/GaussPeak"; the pre-review draft correctly did nothing but open+recompile it (no
    // setCode() of its own). Self-review rewrote this into a version that adds a fabricated
    // setCode() call — the function then failed to compile.
    QtiSasAiContent content;
    const QString preReviewDraft = "c = Compiler\nc.open(\"ai/GaussPeak\")\nok_g = c.compile()\n";
    const QString postReviewCode = "c = Compiler\n"
                                   "c.open(\"ai/GaussPeak\")\n"
                                   "c.setFunctionName(\"ai/GaussPeak\")\n"
                                   "c.setCode(\"double ai_GaussPeak(double x) { return x; }\")\n"
                                   "ok_g = c.compile()\n";
    QVERIFY(!content.selfReviewFabricatedSetCodeMistake(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::fabricatedSetCode_allowsWhenDraftAlreadyHasSetCode()
{
    // The draft already had its own setCode() for this function — self-review editing that
    // existing call is a different, legitimate edit this check has no opinion on.
    QtiSasAiContent content;
    const QString code = "c = Compiler\n"
                         "c.open(\"ai/GaussPeak\")\n"
                         "c.setCode(\"y = scale * exp(-pow((x - center) / width, 2.0) / 2.0) + background;\")\n"
                         "ok_g = c.compile()\n";
    QVERIFY(content.selfReviewFabricatedSetCodeMistake(code, code).isEmpty());
}

void AiContentTests::fabricatedSetCode_allowsWhenNeitherAddsSetCode()
{
    QtiSasAiContent content;
    const QString preReviewDraft = "c = Compiler\nc.open(\"ai/GaussPeak\")\nok_g = c.compile()\n";
    const QString postReviewCode = "c = Compiler\nc.open(\"ai/GaussPeak\")\nok_g = c.compile()\n"
                                   "scriptPrint(str(ok_g))\n";
    QVERIFY(content.selfReviewFabricatedSetCodeMistake(preReviewDraft, postReviewCode).isEmpty());
}

void AiContentTests::guessedNameList_blocksFirstMatchViaIndexZero()
{
    // Confirmed live: guessed ["Surface3D", "Graph3D_1", "Graph3D"] instead of using the concrete
    // name "Surface3D" already established earlier in the conversation, then kept only match [0].
    const QString code = "g3_list = []\n"
                         "for name in [\"Surface3D\", \"Graph3D_1\", \"Graph3D\"]:\n"
                         "    g = plot3D(name)\n"
                         "    if g is not None:\n"
                         "        g3_list.append((name, g))\n"
                         "if g3_list:\n"
                         "    _, g3_first = g3_list[0]\n";
    QVERIFY(!QtiSasAiContent::guessedNameCandidateListMistake(code).isEmpty());
}

void AiContentTests::guessedNameList_blocksColumnNameMembershipGuessing()
{
    // Confirmed live: the identical anti-pattern applied to a table COLUMN name instead of an
    // object name — guessed 4 spellings of the residuals column ("residuals", "Fit Residuals",
    // "residues", "Fit Residues") via membership test + break, when "residues" is the single
    // confirmed-correct name (used directly, no guessing, by the very next split-mode part in the
    // same conversation).
    const QString code = "cols = fc_win.colNames()\n"
                         "res_col = None\n"
                         "for c in [\"residuals\", \"Fit Residuals\", \"residues\", \"Fit Residues\"]:\n"
                         "    if c in cols:\n"
                         "        res_col = c\n"
                         "        break\n";
    QVERIFY(!QtiSasAiContent::guessedNameCandidateListMistake(code).isEmpty());
}

void AiContentTests::guessedNameList_allowsProcessingAllMatches()
{
    // Legitimately processing EVERY table in a list of real, known names — no first-match-only
    // signal (no break, no [0]/pop(0)/next() after the loop), so this is not the guessing pattern.
    const QString code = "for name in [\"SAXS_data\", \"SANS_data\"]:\n"
                         "    t = table(name)\n"
                         "    if t is not None:\n"
                         "        t.setColName(1, \"processed\")\n";
    QVERIFY(QtiSasAiContent::guessedNameCandidateListMistake(code).isEmpty());
}

void AiContentTests::guessedNameList_allowsSingleNameLoop()
{
    const QString code = "for name in [\"Surface3D\"]:\n"
                         "    g = plot3D(name)\n"
                         "    if g is not None:\n"
                         "        break\n";
    QVERIFY(QtiSasAiContent::guessedNameCandidateListMistake(code).isEmpty());
}

void AiContentTests::fragmentedPrint_blocksLongRunOfStaticLines()
{
    const QString code = "scriptPrint(\"QtiSAS AI Agent Information:\")\n"
                         "scriptPrint(\"==============================\")\n"
                         "scriptPrint(\"\")\n"
                         "scriptPrint(\"Purpose: Assist with QtiSAS data analysis operations\")\n"
                         "scriptPrint(\"\")\n";
    QVERIFY(!QtiSasAiContent::fragmentedStaticScriptPrintMistake(code).isEmpty());
}

void AiContentTests::fragmentedPrint_allowsShortRun()
{
    // A short header (title + underline + one blank line) is normal, not the fragmentation pattern.
    const QString code = "scriptPrint(\"Results:\")\n"
                         "scriptPrint(\"========\")\n"
                         "scriptPrint(\"\")\n"
                         "scriptPrint(\"chi2 = \" + str(chi2))\n";
    QVERIFY(QtiSasAiContent::fragmentedStaticScriptPrintMistake(code).isEmpty());
}

void AiContentTests::fragmentedPrint_allowsComputedValuesInLoop()
{
    // Values printed one at a time as they're computed inside a loop — the doc's explicit
    // carve-out — never matches the pure-literal-line shape.
    const QString code = "for name in tableNames():\n"
                         "    scriptPrint(\"Table: \" + name)\n"
                         "    scriptPrint(str(table(name).numRows()))\n";
    QVERIFY(QtiSasAiContent::fragmentedStaticScriptPrintMistake(code).isEmpty());
}

void AiContentTests::doubleEscapedRawRegex_blocksDoubledBackslashWordBoundary()
{
    // Confirmed live: r"\\b" is a raw string containing TWO literal backslash characters, not the
    // single-backslash word-boundary escape \b — the compiled pattern then requires an actual
    // backslash in c.code(), which never appears, so every parameter name is silently
    // misreported as "unused" regardless of whether it's genuinely used.
    const QString code = "import re\n"
                         "unused = [p for p in c.paramNames() if not re.search(r\"\\\\b\" + re.escape(p) + "
                         "r\"\\\\b\", c.code())]\n";
    QVERIFY(!QtiSasAiContent::doubleEscapedRawRegexMistake(code).isEmpty());
}

void AiContentTests::doubleEscapedRawRegex_allowsSingleBackslash()
{
    // The correct form — a single backslash inside the raw string is the genuine \b escape.
    const QString code = "import re\n"
                         "unused = [p for p in c.paramNames() if not re.search(r\"\\b\" + re.escape(p) + "
                         "r\"\\b\", c.code())]\n";
    QVERIFY(QtiSasAiContent::doubleEscapedRawRegexMistake(code).isEmpty());
}

void AiContentTests::doubleEscapedRawRegex_allowsWhenNoReCallPresent()
{
    // The doubled-backslash raw string alone, with no actual re.* call anywhere, isn't grounded as
    // a regex-escape mistake — could be a deliberate literal string elsewhere.
    const QString code = "label = r\"\\\\b\"\nscriptPrint(label)\n";
    QVERIFY(QtiSasAiContent::doubleEscapedRawRegexMistake(code).isEmpty());
}

void AiContentTests::doubleEscapedRawRegex_allowsWhenNoDoubledEscapePresent()
{
    const QString code = "import re\nm = re.search(\"gauss\", name.lower())\n";
    QVERIFY(QtiSasAiContent::doubleEscapedRawRegexMistake(code).isEmpty());
}

void AiContentTests::singletonConstructor_blocksCompilerCalledAsConstructor()
{
    // Confirmed live, repeatedly: self-review changed an already-correct `c = Compiler` into
    // `c = Compiler()`, crashing with TypeError: 'compile18' object is not callable.
    const QString code = "c = Compiler()\nc.new()\n";
    QVERIFY(!QtiSasAiContent::singletonCalledAsConstructorMistake(code).isEmpty());
}

void AiContentTests::singletonConstructor_blocksFittableCalledAsConstructor()
{
    const QString code = "f = Fittable()\nf.configure(\"Guinier\", [\"data_I\"])\n";
    QVERIFY(!QtiSasAiContent::singletonCalledAsConstructorMistake(code).isEmpty());
}

void AiContentTests::singletonConstructor_allowsBareSingletonUsage()
{
    const QString code = "c = Compiler\nc.new()\nf = Fittable\nf.configure(\"Guinier\", [\"data_I\"])\n";
    QVERIFY(QtiSasAiContent::singletonCalledAsConstructorMistake(code).isEmpty());
}

void AiContentTests::frameStyleCombo_blocksQFrameShapeShadowCombo()
{
    // Confirmed live, at least twice: setFrameStyle(QFrame.Box | QFrame.Sunken) ran with no error
    // and no visual sign anything was wrong — the requested frame simply never rendered, since
    // QtiSAS's own FrameStyle enum does an exact-value match, not bit-flag decoding.
    const QString code = "leg.setFrameStyle(QFrame.Box | QFrame.Sunken)\n";
    QVERIFY(!QtiSasAiContent::pyQt6FrameStyleComboMistake(code).isEmpty());
}

void AiContentTests::frameStyleCombo_blocksFullyQualifiedForm()
{
    const QString code = "leg.setFrameStyle(QFrame.Shape.Box | QFrame.Shadow.Sunken)\n";
    QVERIFY(!QtiSasAiContent::pyQt6FrameStyleComboMistake(code).isEmpty());
}

void AiContentTests::frameStyleCombo_allowsNativeFrameStyleValue()
{
    const QString code = "leg.setFrameStyle(Frame.FrameStyle.Shadow.value)\n";
    QVERIFY(QtiSasAiContent::pyQt6FrameStyleComboMistake(code).isEmpty());
}

void AiContentTests::frameStyleCombo_allowsWhenNoSetFrameStyleCall()
{
    const QString code = "leg.setBackgroundColor(QColor(255, 255, 255))\n";
    QVERIFY(QtiSasAiContent::pyQt6FrameStyleComboMistake(code).isEmpty());
}

void AiContentTests::explorePrefix_truncatesAtTopLevelWriteCall()
{
    // Confirmed live: an EXPLORE script's first few lines were genuine read-only reconnaissance,
    // but the SAME response went on to create a real table, compile real functions, fit, and plot
    // — all under the EXPLORE label. Only the prefix before the first top-level write call should
    // be considered safe to run.
    QtiSasAiContent content;
    const QString code = "c = Compiler\n"
                         "c.scanGroups()\n"
                         "scriptPrint(\"categories: \" + str(c.categories()))\n"
                         "\n"
                         "t = newTable(\"GaussPeak\", 200, 3)\n"
                         "t.setColName(1, \"x\")\n";
    const QString prefix = content.explorePrefixBeforeFirstWriteCall(code);
    QVERIFY(prefix.contains("scanGroups"));
    QVERIFY(prefix.contains("categories"));
    QVERIFY(!prefix.contains("newTable"));
    QVERIFY(!prefix.contains("setColName"));
}

void AiContentTests::explorePrefix_allowsPureReadOnlyScript()
{
    QtiSasAiContent content;
    const QString code = "t = table(\"GaussPeak\")\n"
                         "scriptPrint(\"exists: \" + str(t is not None))\n"
                         "if t is not None:\n"
                         "    scriptPrint(\"cols: \" + str(t.colNames()))\n";
    QCOMPARE(content.explorePrefixBeforeFirstWriteCall(code), code);
}

void AiContentTests::explorePrefix_ignoresWriteCallInsideIndentedBlock()
{
    // Cutting AT the indented line would leave "if …:" with an empty body — a SyntaxError, and
    // this prefix gets executed. So the cut moves back to the enclosing top-level statement, which
    // here is that same "if", leaving nothing to run. Letting the newTable() run instead (the old
    // behaviour) was the worse of the two: confirmed live, a whole configure+fit escaped that way.
    QtiSasAiContent content;
    const QString code = "if existTable(\"X\"):\n"
                         "    t = newTable(\"X\", 5, 2)\n"
                         "scriptPrint(\"done\")\n";
    const QString prefix = content.explorePrefixBeforeFirstWriteCall(code);
    QVERIFY(!prefix.contains("newTable"));
    QCOMPARE(prefix, QString());
}

void AiContentTests::explorePrefix_truncatesAtCompilerAuthoringCall()
{
    // Confirmed live: none of c.new()/setFunctionName()/setGroup()/setCode() (all genuine,
    // documented Compiler API calls, verified against compile-python-api.md) were in the write-call
    // list, so an EXPLORE script fully configured a new function in Compiler's live editing buffer
    // for real, cut short only at the final c.compile() line — leaving the buffer populated but
    // never saved. The cut must now happen at c.new() itself.
    QtiSasAiContent content;
    const QString code = "c = Compiler\n"
                         "c.scanGroups()\n"
                         "scriptPrint(\"categories: \" + str(c.categories()))\n"
                         "\n"
                         "c.new()\n"
                         "c.setFunctionName(\"ai/PORODTEST\")\n"
                         "c.setGroup(\"custom:sans\")\n"
                         "c.setCode(\"I = scale * pow(x, -exponent) + background;\")\n"
                         "ok = c.compile()\n";
    const QString prefix = content.explorePrefixBeforeFirstWriteCall(code);
    QVERIFY(prefix.contains("categories"));
    QVERIFY(!prefix.contains("c.new()"));
    QVERIFY(!prefix.contains("setFunctionName"));
    QVERIFY(!prefix.contains("setCode"));
    QVERIFY(!prefix.contains("c.compile()"));
}

void AiContentTests::explorePrefix_truncatesAtFittableConfigureCall()
{
    QtiSasAiContent content;
    const QString code = "f = Fittable\n"
                         "scriptPrint(\"models: \" + str(f.fitFunctions(\"*\")[:5]))\n"
                         "\n"
                         "f.configure(\"ai/PORODTEST\", [\"SAXS_data_I\"])\n"
                         "f.fit()\n";
    const QString prefix = content.explorePrefixBeforeFirstWriteCall(code);
    QVERIFY(prefix.contains("models"));
    QVERIFY(!prefix.contains("configure"));
    QVERIFY(!prefix.contains("f.fit()"));
}

// ---------------------------------------------------------------------------
// detectKnownMistakes() — public dispatch consolidating the 25 checks above, collecting ALL
// matches rather than stopping at the first
// ---------------------------------------------------------------------------

void AiContentTests::detectKnownMistakes_emptyWhenNothingMatches()
{
    QtiSasAiContent content;
    const QString code = "t = newTable(\"Results\", 10, 3)\nt.setCell(1, 1, 42.0)\n";
    const QList<AiKnownMistakeMatch> matches = content.detectKnownMistakes(code, QString(), QString());
    QVERIFY(matches.isEmpty());
}

void AiContentTests::detectKnownMistakes_reportsHeaderAndExplanationOnMatch()
{
    QtiSasAiContent content;
    const QString code = "import Qti3D\nplot3D(m, style=Qti3D.FILLEDMESH)\n";
    const QList<AiKnownMistakeMatch> matches = content.detectKnownMistakes(code, QString(), QString());
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().header, QString("nonexistent Qti3D import detected"));
    QVERIFY(!matches.first().explanation.isEmpty());
    QCOMPARE(matches.first().explanation, QtiSasAiContent::qti3DImportMistake(code));
}

void AiContentTests::detectKnownMistakes_collectsAllMatchesInPriorityOrder()
{
    // Deliberately trips BOTH compiledCodeSelfShadowBug (checked first) and
    // zeroBasedTableIndexMistake (checked later) in the same snippet — confirms
    // detectKnownMistakes() no longer stops at the first match: a single blocked script can
    // genuinely trip more than one independent pattern at once (confirmed live), and reporting
    // only the first meant the model burned a whole retry round per defect instead of fixing them
    // all from one grounded prompt. Both must appear, in the SAME fixed priority order
    // AiAssistant.cpp's former if/else-if chain used, instead of e.g. alphabetical or declaration
    // order.
    QtiSasAiContent content;
    const QString code = "double gamma = gamma;\nt.setCell(0, 1, 42.0)\n";
    QVERIFY(!QtiSasAiContent::compiledCodeSelfShadowBug(code).isEmpty());
    QVERIFY(!QtiSasAiContent::zeroBasedTableIndexMistake(code).isEmpty());
    const QList<AiKnownMistakeMatch> matches = content.detectKnownMistakes(code, QString(), QString());
    QCOMPARE(matches.size(), 2);
    QCOMPARE(matches.at(0).header, QString("compiled-function defect detected"));
    QCOMPARE(matches.at(1).header, QString("0-based table/matrix index detected"));
}

void AiContentTests::detectKnownMistakes_exploreScriptSkipsActionNeverHappenedChecks()
{
    // The confirmed-live infinite loop: an EXPLORE prefix has no newTable() (truncated away by
    // explorePrefixBeforeFirstWriteCall()), so the creation check fired every round and its
    // "add newTable(...)" advice could never be satisfied in that context.
    QtiSasAiContent content;
    const QString exploreCode = "t = table(\"SAXS_data\")\n"
                                "scriptPrint(\"Table exists: %s\" % str(t is not None))\n";
    const QString request = "Create a table \"SAXS_data\" with 3 columns Q, I, dI.";

    const QList<AiKnownMistakeMatch> asAction = content.detectKnownMistakes(exploreCode, request, QString());
    QVERIFY(!asAction.isEmpty());
    QCOMPARE(asAction.first().header, QString("requested table/matrix/graph/note creation never happened"));

    const QList<AiKnownMistakeMatch> asExplore =
        content.detectKnownMistakes(exploreCode, request, QString(), QString(), /*exploreScript=*/true);
    QVERIFY(asExplore.isEmpty());
}

void AiContentTests::detectKnownMistakes_exploreScriptStillReportsRealDefects()
{
    // Only the "never happened" family is skipped — a genuine defect in an EXPLORE script (here a
    // 0-based table index) must still be caught.
    QtiSasAiContent content;
    const QString exploreCode = "t = table(\"T\")\nscriptPrint(str(t.cell(0, 1)))\n";
    const QList<AiKnownMistakeMatch> matches =
        content.detectKnownMistakes(exploreCode, QString("Create a table \"T\"."), QString(), QString(),
                                    /*exploreScript=*/true);
    QVERIFY(!matches.isEmpty());
}

void AiContentTests::detectKnownMistakes_passesOriginalRequestTextThrough()
{
    // unrequestedAutoexecImmediateRun() is one of the two checks needing originalRequestText —
    // confirms detectKnownMistakes() actually threads it through rather than dropping it.
    QtiSasAiContent content;
    const QString code = "n = newNote(\"Setup\")\n"
                         "n.setAutoexec(True)\n"
                         "n.setText(\"print('configured')\")\n"
                         "n.tab(1).execute()\n";
    const QString requestWithoutRunNow =
        "Create a note that configures analysis defaults when the project file is opened.";
    const QList<AiKnownMistakeMatch> blocked = content.detectKnownMistakes(code, requestWithoutRunNow, QString());
    QCOMPARE(blocked.size(), 1);
    QCOMPARE(blocked.first().header, QString("unrequested immediate autoexec run detected"));

    const QString requestWithRunNow =
        "Create a note that auto-runs on project open, and also run it right now to confirm it works.";
    const QList<AiKnownMistakeMatch> allowed = content.detectKnownMistakes(code, requestWithRunNow, QString());
    QVERIFY(allowed.isEmpty());
}

void AiContentTests::detectKnownMistakes_passesEarlierPartsCodeThrough()
{
    // reuseInsteadOfCompileMistake() is the check needing earlierPartsCode (split-mode: an earlier
    // part may have already compiled what this part now just fits) — confirms detectKnownMistakes()
    // actually threads it through rather than dropping it.
    QtiSasAiContent content;
    const QString request = "Compile a custom Gaussian peak fit function and fit it with weighted errors.";
    const QString code = "f = Fittable\n"
                         "f.configure(\"ai/GaussPeak\", [\"GaussPeak_y\"])\n"
                         "chi2 = f.fit()\n";
    const QList<AiKnownMistakeMatch> blocked = content.detectKnownMistakes(code, request, QString());
    QCOMPARE(blocked.size(), 1);
    QCOMPARE(blocked.first().header, QString("requested compile never happened"));

    const QString earlierPartsCode = "c = Compiler\nc.new()\nc.setFunctionName(\"ai/GaussPeak\")\n"
                                     "c.setGroup(\"custom:peak\")\nc.setCode(\"...\")\nok = c.compile()\n";
    const QList<AiKnownMistakeMatch> allowed = content.detectKnownMistakes(code, request, earlierPartsCode);
    QVERIFY(allowed.isEmpty());
}

// ---------------------------------------------------------------------------
// retryHintForError()
// ---------------------------------------------------------------------------

void AiContentTests::retryHint_constructorMistake()
{
    QtiSasAiContent content;
    const QString err = "TypeError: 'fittable18' object is not callable\nat <ai>:1 — f = Fittable()";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("no parentheses"));
}

void AiContentTests::retryHint_fittableMethodGuessing()
{
    QtiSasAiContent content;
    const QString err = "AttributeError: 'fittable18' object has no attribute 'setModel'\nat <ai>:3";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("configure("));
}

void AiContentTests::retryHint_compilerFitFunctionsCrossSingleton()
{
    // Confirmed live, repeated twice: c = Compiler; c.fitFunctions(...) — that method belongs to
    // Fittable, not Compiler.
    QtiSasAiContent content;
    const QString err = "AttributeError: 'compile18' object has no attribute 'fitFunctions'\n"
                        "at <ai>:2 — if \"ai/Porod\" in c.fitFunctions():";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("functionNames("));
    QVERIFY(hint.contains("Fittable"));
}

void AiContentTests::retryHint_fitCurveHardcodedRow()
{
    QtiSasAiContent content;
    const QString err = "ValueError: could not convert string to float: 'ai/GaussPeakFit'\n"
                        "at <ai>:8 — chi2_gauss = float(fc_gauss.text(7, 1).replace(\"->\", \"\").strip())";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("read_characteristic"));
}

void AiContentTests::retryHint_missingQtImport()
{
    // Confirmed live: this NameError resolved two different ways in the same session — once by
    // correctly adding the import, once by silently deleting the styling call instead, dropping an
    // explicitly requested "styled legend" down to bare, unstyled text.
    QtiSasAiContent content;
    const QString err = "NameError: name 'QFont' is not defined\n"
                        "at <ai>:57 — leg.setFont(QFont(\"Arial\", 10))";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("from PyQt6.QtGui import QFont"));
    QVERIFY(hint.contains("do NOT remove"));
}

void AiContentTests::retryHint_setWindowNameCollisionSuggestsNewPlot3D()
{
    // Confirmed live: plot3D(...)+setWindowName(g3, name) raised this ValueError on a re-run, and
    // the retry that followed wrongly switched to graphWindow(name) (2D-only, AttributeError next).
    QtiSasAiContent content;
    const QString err = "ValueError: setWindowName: name 'Surface3D' already exists.\n"
                        "at <ai>:3 — setWindowName(g3, \"Surface3D\")";
    const QString hint = content.retryHintForError(err);
    QVERIFY(hint.contains("newPlot3D("));
    QVERIFY(hint.contains("graphWindow("));
    QVERIFY(hint.contains("Surface3D"));
}

void AiContentTests::retryHint_emptyForUnrelatedError()
{
    QtiSasAiContent content;
    const QString err = "NameError: name 'foo' is not defined";
    QVERIFY(content.retryHintForError(err).isEmpty());
}

// ---------------------------------------------------------------------------
// AiAssistant.cpp internal free functions (AiAssistantInternal.h)
// ---------------------------------------------------------------------------

void AiContentTests::filterDocs_selectsNamedTopicOnly()
{
    // Content markers deliberately share no substring with each other — "table"/"fittable" would
    // be a trap here, since "fittable" itself contains "table" as a substring.
    const QString allDocs = "\n\n=== table-python-api.md ===\nCONTENT_ONE\n"
                            "\n\n=== fittable-python-api.md ===\nCONTENT_TWO\n";
    const QString filtered = filterDocsByTopic(allDocs, {"fittable"});
    QVERIFY(filtered.contains("CONTENT_TWO"));
    QVERIFY(!filtered.contains("CONTENT_ONE"));
}

void AiContentTests::filterDocs_emptyTopicsReturnsEverything()
{
    const QString allDocs = "\n\n=== table-python-api.md ===\nCONTENT_ONE\n";
    QCOMPARE(filterDocsByTopic(allDocs, QStringList()), allDocs);
}

void AiContentTests::splitPartTopics_fittableSynonymOverridesNone()
{
    // "fittable" never appears verbatim in a natural description — the part talks about "fits"/
    // "chi2/dof" instead — so the synonym regex must still pull the topic in even though the raw
    // DOCS field said "none".
    const QStringList known = {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable"};
    const QStringList topics =
        splitPartTopicsFromAnswer("none", "compare chi2/dof from both fits and select best model", known);
    QVERIFY(topics.contains("fittable"));
}

void AiContentTests::splitPartTopics_graph3dSynonymOverridesNone()
{
    // Regression for the ai-test-requests.md "3D surface plot" finding: "graph3d" never appears
    // verbatim in a natural description either — the part says "3D surface"/"3D plot" instead —
    // so this synonym regex must still pull the topic in even though the raw DOCS field said "none".
    const QStringList known = {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable"};
    const QStringList topics = splitPartTopicsFromAnswer("none", "plot a 3D surface of sin(x)*cos(y)", known);
    QVERIFY(topics.contains("graph3d"));
}

void AiContentTests::splitPartTopics_ascii1dForSasDataInOrOut()
{
    // The routing rule: importing or exporting 1D SAS data — named by any of its usual words — needs the Ascii1d docs
    QtiSasAiContent content;
    const QStringList known = content.knownDocTopics();
    QVERIFY(known.contains("ascii1d"));
    for (const QString &request :
         {QStringLiteral("Import the SANS curve plain.dat into the project"),
          QStringLiteral("Import the radially averaged dataset grasp.dat"),
          QStringLiteral("export it as an ascii file"), QStringLiteral("read the canSAS file"),
          QStringLiteral("With ascii1d, cut the first 3 points"),
          QStringLiteral("convert it to the Kratky presentation"), QStringLiteral("save the SAXS curves as .RAD")})
        QVERIFY2(splitPartTopicsFromAnswer("none", request, known).contains("ascii1d"), qPrintable(request));
    // a name with SAS letters inside it is no such word
    QVERIFY(!splitPartTopicsFromAnswer("none", "fit the table SAXS_data with QtiSAS", known).contains("ascii1d"));
}

void AiContentTests::splitPartTopics_svdForContrastVariation()
{
    // contrast variation — named by any of its usual words — needs the Svd docs
    QtiSasAiContent content;
    const QStringList known = content.knownDocTopics();
    QVERIFY(known.contains("svd"));
    for (const QString &request :
         {QStringLiteral("Contrast variation: decompose mix0 and mix34"), QStringLiteral("make the SVD of the series"),
          QStringLiteral("print the partial structure factors at Q = 0.05"),
          QStringLiteral("At which D2O fraction is component a contrast matched?"),
          QStringLiteral("find the match point of the protein"), QStringLiteral("plan an H/D series"),
          QStringLiteral("singular value decomposition of the contrast matrix"),
          QStringLiteral("Repeat the decomposition of svdAB without mix34")})
        QVERIFY2(splitPartTopicsFromAnswer("none", request, known).contains("svd"), qPrintable(request));
    QVERIFY(!splitPartTopicsFromAnswer("none", "import the SANS curve plain.dat", known).contains("svd"));
}

void AiContentTests::splitPartTopics_fittableSynonymCatchesSimulate()
{
    // Confirmed live: "plot simulated 'ai/PorodPowerLaw' curve with log-log axes and proper
    // formatting" needs "fittable" docs to know simulateCurve()'s "simulatedCurve-<function>"
    // table-naming convention, but has no "fit"-rooted word, no "chi2", no "fitcurve" — the
    // synonym regex missed it entirely until "simulate"/"simulated" were added as their own
    // trigger, the actual reason this class of part needs these docs.
    const QStringList known = {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable"};
    const QStringList topics = splitPartTopicsFromAnswer(
        "none", "plot simulated 'ai/PorodPowerLaw' curve with log-log axes and proper formatting", known);
    QVERIFY(topics.contains("fittable"));
}

void AiContentTests::splitPartsMissingCreationName_catchesPlotWithoutName()
{
    // Regression for the Phase 1a fix: "plot" is a creation verb too, and this description never
    // states a quoted target name.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{
        "plot data (red star, size 6), best-fit curve (red line, thickness 3), and residuals "
        "(red star, size 6) in separate panels",
        {}};
    const QList<int> missing = splitPartsMissingCreationName(plan);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), 0);
}

void AiContentTests::splitPartsMissingCreationName_acceptsQuotedTableName()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create table named 'GaussPeak' with x, y, dy columns", {}};
    QVERIFY(splitPartsMissingCreationName(plan).isEmpty());
}

void AiContentTests::splitPartsMissingCreationName_catchesSurfaceWithoutName()
{
    // Regression for the graph3d Direction 8 finding: "surface" is a creation noun too, matching
    // real split-plan phrasing ("create 3D surface plot of sin(x)*cos(y)") with no quoted name.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create 3D surface plot of sin(x)*cos(y) with a filled mesh style", {}};
    const QList<int> missing = splitPartsMissingCreationName(plan);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), 0);
}

void AiContentTests::splitPartsMissingCreationName_acceptsQuotedSurfaceName()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create 3D surface plot \"SurfacePlot\" of sin(x)*cos(y)", {}};
    QVERIFY(splitPartsMissingCreationName(plan).isEmpty());
}

void AiContentTests::splitPlanCoverage_blocksMissingFitCompareplot()
{
    // Confirmed live, severe: a request combining "create a table... compile two fit
    // functions... fit both... compare chi2/dof... plot the winner..." was decomposed into a
    // SINGLE part covering only "create table + compile" — fit/compare/plot never appeared in
    // ANY part, yet the run completed after that one part and reported "all 1 part(s) completed".
    const QString request = "Create a table \"GaussPeak\" with 200 rows... Compile a custom Gaussian peak fit "
                            "function and a Lorentzian fit function. Fit both models with weighted errors. Compare "
                            "chi2/dof and report which model fits better. Plot the winning model's data, fit curve, "
                            "and residuals into graph \"GaussPeakFit\"...";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Create table 'GaussPeak' with 200 rows; compile custom Gaussian peak fit function "
                                   "'ai/GaussPeak' and Lorentzian peak fit function 'ai/LorentzianPeak'",
                                   {}};
    const QStringList missing = splitPlanMissingRequestedActions(request, plan);
    QVERIFY(missing.contains("fit"));
    QVERIFY(missing.contains("compare models / report a winner"));
    QVERIFY(missing.contains("plot/graph"));
    QVERIFY(!missing.contains("compile"));
}

void AiContentTests::splitPlanCoverage_allowsFullyCoveredPlan()
{
    const QString request = "Compile a custom Gaussian peak fit function. Fit it with weighted errors. Compare "
                            "chi2/dof against a Lorentzian and report which model fits better. Plot the winner.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Compile custom Gaussian peak fit function 'ai/GaussPeak'", {}}
         << AiAssistant::SplitPart{"Fit 'ai/GaussPeak' and compare chi2/dof to report the winner", {}}
         << AiAssistant::SplitPart{"Plot the winning model into graph 'GaussPeakFit'", {}};
    QVERIFY(splitPlanMissingRequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanCoverage_allowsWhenRequestNeverMentionsTheAction()
{
    // A request that never mentions "compare"/"plot" at all is untouched by those groups
    // regardless of how the plan is split — only groups whose keyword actually appears in the
    // request text are checked.
    const QString request = "Create a table 'Data' with 50 rows of x,y values.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Create table 'Data' with 50 rows of x,y values", {}};
    QVERIFY(splitPlanMissingRequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanCoverage_allowsExplicitlyNegatedFit()
{
    // Confirmed live, severe: "...then simulate a curve from it without fitting any real data"
    // matched the "fit" group on the word "fitting" — a NEGATION, not a requested action — so the
    // fixup retry demanded a part that performs fitting, directly contradicting what the request
    // actually asked for. Structurally unsatisfiable: burned the entire 3-round fixup budget every
    // time, always ending in "still has unresolved issue(s) — proceeding anyway."
    const QString request = "Compile a custom Porod power-law fit function with an adjustable "
                            "exponent parameter, then simulate a curve from it without fitting "
                            "any real data.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom Porod power-law fit function named 'ai/PorodFit' "
                                   "with adjustable exponent parameter",
                                   {}}
         << AiAssistant::SplitPart{"simulate a curve from the compiled Porod power-law function "
                                   "named 'ai/PorodFit'",
                                   {}}
         << AiAssistant::SplitPart{"plot the simulated Porod curve named 'ai/PorodFit' with log-log "
                                   "scale and proper axis labels",
                                   {}}
         << AiAssistant::SplitPart{"add a title and legend to the Porod curve plot named 'ai/PorodFit'", {}};
    QVERIFY(splitPlanMissingRequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanCoverage_ignoresModelSubstringInsideSasviewmodels()
{
    // Confirmed live, 3x: the loose "which...model" alternative matched "model" mid-word inside
    // "sasviewmodels", fabricating a "compare models / report a winner" requirement nobody asked for.
    const QString request = "Check which sasviewmodels fit functions are already compiled, then "
                            "simulate one of them without fitting any real data.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"check which sasviewmodels fit functions are already compiled, "
                                   "then select one and simulate it without fitting real data",
                                   {}};
    QVERIFY(splitPlanMissingRequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanCoverage_catchesDroppedSimulatePart()
{
    // Confirmed live, severe: the exact request above (same wording as
    // allowsExplicitlyNegatedFit()) had its simulate part silently dropped during a fixup round,
    // leaving only the compile part — nothing caught it, since "simulate" wasn't a tracked action
    // group at all before this fix.
    const QString request = "Compile a custom Porod power-law fit function with an adjustable "
                            "exponent parameter, then simulate a curve from it without fitting "
                            "any real data.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom Porod power-law fit function named 'ai/PorodFit' "
                                   "with adjustable exponent parameter",
                                   {}};
    const QStringList missing = splitPlanMissingRequestedActions(request, plan);
    QVERIFY(missing.contains("simulate"));
}

void AiContentTests::splitPlanOverreach_blocksInventedAnalysisPipelineForPlainTableRequest()
{
    // Confirmed live, severe: a request that only says "Create a table ... and fill it with
    // synthetic ... intensities ... plus 3% Gaussian noise" — zero mention of plotting, fitting,
    // comparing, or graph styling — was decomposed into a 9-part plan where parts 2-9 built an
    // entire unrequested SAXS analysis pipeline. Every other mechanical check passed cleanly on
    // this exact plan (well-named, no fill-only fragments), which is why this check exists.
    const QString request = "Create a table \"SAXS_data\" with 3 columns Q, I, dI and fill it with "
                            "synthetic small-angle-scattering intensities for a sphere form factor "
                            "plus 3% Gaussian noise.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create table \"SAXS_data\" with columns Q, I, dI and fill with "
                                   "synthetic SAXS data for sphere form factor plus 3% Gaussian noise",
                                   {}}
         << AiAssistant::SplitPart{"plot the SAXS data with Q vs I", {}}
         << AiAssistant::SplitPart{"fit the data with a sphere form factor model", {}}
         << AiAssistant::SplitPart{"plot fit results and overlay on original data", {}}
         << AiAssistant::SplitPart{"add legend and axis labels to the plot", {}}
         << AiAssistant::SplitPart{"set log-log scale for both axes", {}}
         << AiAssistant::SplitPart{"set major ticks for log-log axes", {}}
         << AiAssistant::SplitPart{"set minor ticks for log-log axes", {}}
         << AiAssistant::SplitPart{"set scientific notation for axis labels", {}};
    const QList<int> unrequested = splitPlanUnrequestedActions(request, plan);
    QCOMPARE(unrequested.size(), 4);
    QVERIFY(unrequested.contains(1));  // plot
    QVERIFY(unrequested.contains(2));  // fit
    QVERIFY(unrequested.contains(3));  // plot fit results
    QVERIFY(!unrequested.contains(0)); // table creation itself — not one of the four action groups
}

void AiContentTests::splitPlanOverreach_allowsPlanFullyWithinRequestedScope()
{
    const QString request = "Compile a custom Gaussian peak fit function. Fit it with weighted "
                            "errors. Compare chi2/dof against a Lorentzian and report which model "
                            "fits better. Plot the winner.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Compile custom Gaussian peak fit function 'ai/GaussPeak'", {}}
         << AiAssistant::SplitPart{"Fit 'ai/GaussPeak' and compare chi2/dof to report the winner", {}}
         << AiAssistant::SplitPart{"Plot the winning model into graph 'GaussPeakFit'", {}};
    QVERIFY(splitPlanUnrequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanOverreach_catchesUnrequestedSimulate()
{
    // Mirror of splitPlanCoverage_catchesDroppedSimulatePart() -- a part simulating something the
    // request never asked for must be flagged as over-scope, same as the other three groups.
    const QString request = "Create a table 'Data' with 50 rows of x,y values.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Create table 'Data' with 50 rows of x,y values", {}}
         << AiAssistant::SplitPart{"simulate an additional curve from the data", {}};
    const QList<int> unrequested = splitPlanUnrequestedActions(request, plan);
    QCOMPARE(unrequested.size(), 1);
    QVERIFY(unrequested.contains(1));
}

void AiContentTests::splitPlanOverreach_allowsWhenNeitherPlanNorRequestMentionTheAction()
{
    // A part doing something none of the four recognized action groups matches (e.g. plain table
    // creation) is never flagged regardless of the request text — same narrow-scope philosophy as
    // splitPlanMissingRequestedActions()'s sibling test.
    const QString request = "Create a table 'Data' with 50 rows of x,y values.";
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Create table 'Data' with 50 rows of x,y values", {}};
    QVERIFY(splitPlanUnrequestedActions(request, plan).isEmpty());
}

void AiContentTests::splitPlanFragments_blocksOriginalConfirmedFillIncident()
{
    // Confirmed live: a plan split "create a 50x50 matrix named 'X'" from "fill every cell of
    // 'X' using formula ... via setFormula()/calculate()" into two parts that should have been
    // one — filling has no independent value without the creation.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create a 50x50 matrix named 'X'", {}}
         << AiAssistant::SplitPart{"fill every cell of 'X' using formula sin(x)*cos(y) via setFormula()/calculate()",
                                   {}};
    const QList<int> unnecessary = splitPlanUnnecessaryFragments(plan);
    QCOMPARE(unnecessary.size(), 1);
    QCOMPARE(unnecessary.first(), 1);
}

void AiContentTests::splitPlanFragments_chainsThroughWholeStylingChainAfterPlotCreation()
{
    // Confirmed live, severe: a request that only asked to compile a Porod power-law function
    // and simulate a curve from it (no plotting/styling mentioned at all) was decomposed into 7
    // parts, 5 of which were pure styling continuations of the SAME plot-creation part — but only
    // the first link of that chain has a preceding part that itself looks like a creation step;
    // every later link only continues what an EARLIER part created, not the immediately preceding
    // filler. All 4 styling parts (indices 3-6) must be caught, not just the first one, or the
    // retry prompt only ever tells the model about one of the five parts that need merging.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile a custom Porod power-law fit function with an adjustable "
                                   "exponent parameter",
                                   {}}
         << AiAssistant::SplitPart{"simulate a curve from the compiled Porod function without fitting "
                                   "real data",
                                   {}}
         << AiAssistant::SplitPart{"plot the simulated Porod curve with log-log scale and proper axis "
                                   "formatting",
                                   {}}
         << AiAssistant::SplitPart{"add a title, axis labels, and legend to the Porod plot", {}}
         << AiAssistant::SplitPart{"set the graph window to use log-log scale for both axes", {}}
         << AiAssistant::SplitPart{"configure the major and minor tick settings for the log-log plot", {}}
         << AiAssistant::SplitPart{"finalize the graph styling including line style and color", {}};
    const QList<int> unnecessary = splitPlanUnnecessaryFragments(plan);
    QCOMPARE(unnecessary.size(), 4);
    QVERIFY(unnecessary.contains(3));
    QVERIFY(unnecessary.contains(4));
    QVERIFY(unnecessary.contains(5));
    QVERIFY(unnecessary.contains(6));
}

void AiContentTests::splitPlanFragments_allowsGenuineDistinctActionAfterCreation()
{
    // A styling-looking part that ALSO performs its own genuine distinct action (here, plotting
    // something new as a VERB, not merely referring back to the earlier plot as a trailing noun)
    // must not be flagged — it has independent value beyond finishing the preceding part.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"plot the simulated Porod curve with log-log scale", {}}
         << AiAssistant::SplitPart{"add a legend, and also plot the residuals separately", {}};
    QVERIFY(splitPlanUnnecessaryFragments(plan).isEmpty());
}

void AiContentTests::splitPlanFragments_chainsThroughNamedPlotSuffixVariant()
{
    // Confirmed live, second occurrence of the same incident: once the naming-consistency fixup
    // retry made every later part repeat the created object's exact name in quotes, "plot" stopped
    // being the trailing word ("...to the Porod curve plot named 'PorodPowerLaw'"), and one part
    // used the possessive "the plot's major and minor tick settings" — both noun-usage shapes the
    // original trailing-word-only exclusion missed, so this exact 3-part styling chain went
    // entirely unflagged the first time around.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodPowerLaw' with adjustable "
                                   "exponent parameter",
                                   {}}
         << AiAssistant::SplitPart{"simulate a curve from the compiled Porod power-law function named "
                                   "'PorodPowerLaw' without fitting real data",
                                   {}}
         << AiAssistant::SplitPart{"plot the simulated Porod curve named 'PorodPowerLaw' with log-log axes "
                                   "and proper formatting",
                                   {}}
         << AiAssistant::SplitPart{"add a title, axis labels, and legend to the Porod curve plot named "
                                   "'PorodPowerLaw'",
                                   {}}
         << AiAssistant::SplitPart{"configure the plot's major and minor tick settings for log-log scale "
                                   "for the plot named 'PorodPowerLaw'",
                                   {}}
         << AiAssistant::SplitPart{"set the plot's canvas color and axis label formats for the plot named "
                                   "'PorodPowerLaw'",
                                   {}};
    const QList<int> unnecessary = splitPlanUnnecessaryFragments(plan);
    QCOMPARE(unnecessary.size(), 3);
    QVERIFY(unnecessary.contains(3));
    QVERIFY(unnecessary.contains(4));
    QVERIFY(unnecessary.contains(5));
}

void AiContentTests::splitPlanFragments_catchesPlotAsDirectObjectOfLeadingVerb()
{
    // Confirmed live, a FOURTH noun-usage shape: "configure plot with log-log axes, axis labels,
    // and title" — "plot" is the direct object of THIS part's own leading filler verb
    // ("configure PLOT"), not trailing, not a naming clause, not possessive — none of the three
    // previously-fixed exclusions applied, so this real 5-part compile->simulate->plot->configure
    // ->legend chain went entirely unflagged; only "2 issue(s)" (unrelated to fragmentation) were
    // ever detected for it in a live run.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodFit' with adjustable "
                                   "exponent parameter",
                                   {}}
         << AiAssistant::SplitPart{"simulate curve from 'ai/PorodFit' without fitting real data", {}}
         << AiAssistant::SplitPart{"plot simulated 'ai/PorodFit' curve", {}}
         << AiAssistant::SplitPart{"configure plot with log-log axes, axis labels, and title", {}}
         << AiAssistant::SplitPart{"add legend to the plot", {}};
    const QList<int> unnecessary = splitPlanUnnecessaryFragments(plan);
    QCOMPARE(unnecessary.size(), 2);
    QVERIFY(unnecessary.contains(3));
    QVERIFY(unnecessary.contains(4));
}

void AiContentTests::reconcileCompiledName_rewritesBareReferencesToMatchCanonical()
{
    // Confirmed live: the compile part named 'ai/PorodPowerLaw' while every one of 5 later parts
    // referenced the same function as bare 'PorodPowerLaw' — each part individually states A
    // name, so nothing else catches the mismatch, and a later part's generated script could open/
    // configure the wrong (bare) name while the function actually compiled under the "ai/" one.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodPowerLaw' with adjustable "
                                   "exponent parameter",
                                   {}}
         << AiAssistant::SplitPart{"simulate a curve from the compiled Porod power-law function named "
                                   "'PorodPowerLaw' without fitting real data",
                                   {}}
         << AiAssistant::SplitPart{"plot the simulated Porod curve named 'PorodPowerLaw' with log-log axes", {}};
    reconcileCompiledFunctionNameAcrossParts(plan);
    QVERIFY(!plan[1].description.contains("'PorodPowerLaw'"));
    QVERIFY(plan[1].description.contains("'ai/PorodPowerLaw'"));
    QVERIFY(plan[2].description.contains("'ai/PorodPowerLaw'"));
    // The compile part's own description must stay exactly as-is.
    QCOMPARE(plan[0].description,
             QString("compile custom fit function named 'ai/PorodPowerLaw' with adjustable exponent parameter"));
}

void AiContentTests::reconcileCompiledName_leavesConsistentPlanUnchanged()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/GaussPeak'", {}}
         << AiAssistant::SplitPart{"fit 'ai/GaussPeak' with weighted errors", {}};
    const QString before1 = plan[1].description;
    reconcileCompiledFunctionNameAcrossParts(plan);
    QCOMPARE(plan[1].description, before1);
}

void AiContentTests::reconcileCompiledName_ignoresUnrelatedQuotedNames()
{
    // A DIFFERENT part's unrelated quoted name (a table, not this function) must not be touched
    // just because it happens to be quoted.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/GaussPeak'", {}}
         << AiAssistant::SplitPart{"create table 'GaussPeakData' with 100 rows", {}};
    const QString before1 = plan[1].description;
    reconcileCompiledFunctionNameAcrossParts(plan);
    QCOMPARE(plan[1].description, before1);
}

void AiContentTests::splitPartsMissingCreationName_catchesPlotCurveWithoutName()
{
    // Regression for the same Porod incident from the other direction: this part creates a new
    // graph window (plotting a curve) but never states a quoted name for it, which a later part
    // would have no reliable way to look up by name.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"plot the simulated Porod curve with log-log scale and proper axis "
                                   "formatting",
                                   {}};
    const QList<int> missing = splitPartsMissingCreationName(plan);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), 0);
}

void AiContentTests::isEmptyActionScript_detectsCommentsOnlyScript()
{
    // Confirmed live: self-review's entire response was a checklist-item-by-checklist-item
    // reasoning dump formatted as comments, with zero actual statements.
    const QString code = "# The checklist item \"X\" does not apply.\n"
                         "# The checklist item \"Y\" does not apply.\n"
                         "# Return the script unchanged.\n";
    QVERIFY(isEmptyActionScript(code));
}

void AiContentTests::isEmptyActionScript_detectsBlankScript()
{
    QVERIFY(isEmptyActionScript("\n\n   \n\t\n"));
    QVERIFY(isEmptyActionScript(""));
}

void AiContentTests::isEmptyActionScript_allowsRealCode()
{
    QVERIFY(!isEmptyActionScript("t = newTable(\"X\", 10, 2)\n"));
}

void AiContentTests::isEmptyActionScript_allowsCodeWithComments()
{
    const QString code = "# create the table\n"
                         "t = newTable(\"X\", 10, 2)\n"
                         "# done\n";
    QVERIFY(!isEmptyActionScript(code));
}

void AiContentTests::onlyPrintsResult_detectsAPrintOnlyScript()
{
    // benchmark LIST 1 item 10, local all-pyi BIGMODEL: the values read off the request, nothing plotted
    QVERIFY(onlyPrintsResult("scriptPrint(\"R10:\", 2, 2, 1)"));
    QVERIFY(onlyPrintsResult("from PyQt6.QtGui import QColor\n# the result\nscriptPrint(\"R10:\", -1, -1, -1)\n"));
    QVERIFY(onlyPrintsResult("print('R3:', note('Batch').tabs())"));
    // any other statement is work, or at least an attempt at it
    QVERIFY(!onlyPrintsResult("gw = newGraph(\"FitResults\", 2, 2, 1)\nscriptPrint(\"R10:\", gw.numLayers())"));
    QVERIFY(!onlyPrintsResult("if x:\n    scriptPrint('a')"));
    QVERIFY(!onlyPrintsResult("scriptPrint(\"R1:\",\n            1)"));
    QVERIFY(!onlyPrintsResult("# only a comment\n"));
}

void AiContentTests::blockReasonSummary_skipsTheQuotedRequest()
{
    // Confirmed live (all-pyi BIGMODEL, ai-test-requests item 12): the cut fell on the ". " after the item's first
    // sentence, inside the quote, and the rule that fired never showed.
    const QString item =
        "Create a table \"GaussPeak\" with 200 rows of synthetic Gaussian-peak data (x, y, dy columns) "
        "with 5% noise. Compile a custom Gaussian peak fit function and a Lorentzian fit function. "
        "Plot the winning model into graph \"GaussPeakFit\".";
    const QString explanation = "This part's own job (\"" + item +
                                "\") never mentions the window name \"GaussPeakFitResult\" this script creates. "
                                "Use the name the request gives.";
    const QString summary = blockReasonSummary(explanation);
    QVERIFY2(summary.contains("never mentions the window name \"GaussPeakFitResult\""), qPrintable(summary));
    QVERIFY2(summary.contains(" …"), qPrintable(summary));
    QVERIFY2(!summary.contains("Use the name"), qPrintable(summary));
    // A short quoted request stays whole.
    const QString shortOne = blockReasonSummary("This part's own job (\"Create a note. Then a table.\") does not "
                                                "mention plotting. Do not build a graph here.");
    QCOMPARE(shortOne, QString("This part's own job (\"Create a note. Then a table.\") does not mention plotting."));
}

void AiContentTests::blockReasonSummary_keepsListMarkersAndPlainSentences()
{
    QCOMPARE(blockReasonSummary("Random values the request never asked for. Remove them."),
             QString("Random values the request never asked for."));
    QCOMPARE(blockReasonSummary("Missing what the request asked for:\n1. a graph\n2. a legend. More text."),
             QString("Missing what the request asked for: 1. a graph 2. a legend."));
}

void AiContentTests::inventedMergePartNumbers_detectsPartsBeyondPlanSize()
{
    // Confirmed live: a 1-part plan (create table "SAXS_data") merged into a script carrying
    // "Part 2" through "Part 6" markers for an unrequested, never-executed graph+fit workflow.
    const QString code = "t = newTable(\"SAXS_data\", 200, 3)\n"
                         "# --- Part 2: create graph \"SAXS_graph\" ---\n"
                         "g = graphWindow(\"SAXS_graph\")\n"
                         "# --- Part 6: add fitted curve to the graph ---\n"
                         "g.addDataset(\"SAXS_fit_result_Q\", \"SAXS_fit_result_Yfit\")\n";
    const QList<int> invented = inventedMergePartNumbers(code, 1);
    QCOMPARE(invented.size(), 2);
    QVERIFY(invented.contains(2));
    QVERIFY(invented.contains(6));
}

void AiContentTests::inventedMergePartNumbers_allowsExactPartCount()
{
    const QString code = "# --- Part 1: create table \"X\" ---\n"
                         "t = newTable(\"X\", 10, 2)\n"
                         "# --- Part 2: fill column 1 ---\n"
                         "t.setCell(1, 1, 5)\n";
    QVERIFY(inventedMergePartNumbers(code, 2).isEmpty());
}

void AiContentTests::inventedMergePartNumbers_allowsScriptWithNoMarkers()
{
    QVERIFY(inventedMergePartNumbers("t = newTable(\"X\", 10, 2)\nt.setCell(1, 1, 5)\n", 1).isEmpty());
}

void AiContentTests::inventedMergeImports_detectsImportNeverUsedInParts()
{
    // Confirmed live: a 1-part "restyle graph" merge added imports for modules that don't exist
    // anywhere in the docs and were never in the actual executed script.
    const QString partScripts = "from PyQt6.QtGui import QColor, QFont\n"
                                "gw = graphWindow(\"FitResults\")\n"
                                "g = gw.activeLayer()\n";
    const QString mergedCode = "from PyQt6.QtGui import QColor, QFont\n"
                               "from qti.app import graphWindow, scriptPrint\n"
                               "from qti import Graph, Table, Dataset\n"
                               "gw = graphWindow(\"FitResults\")\n"
                               "g = gw.activeLayer()\n";
    const QStringList invented = inventedMergeImports(mergedCode, partScripts);
    QCOMPARE(invented.size(), 2);
    QVERIFY(invented.contains("qti.app"));
    QVERIFY(invented.contains("qti"));
}

void AiContentTests::inventedMergeImports_allowsImportsPresentInParts()
{
    const QString partScripts = "from math import pi, sin, cos\ng3 = plot3D(\"sin(x)*cos(y)\")\n";
    const QString mergedCode = "from math import pi, sin, cos\ng3 = plot3D(\"sin(x)*cos(y)\")\n";
    QVERIFY(inventedMergeImports(mergedCode, partScripts).isEmpty());
}

void AiContentTests::extractExistingEntityNames_findsExistingBeforeQuote()
{
    const QString request = "Fit the existing \"Sphere\" form-factor model from the Fittable library.";
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(request);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("Sphere"));
    // "form-factor model ... Fittable library" pins the kind even though no window noun precedes
    // the quote, so this narrows to compile+fittable instead of paying an all-three check whose
    // window half could only ever print MISSING. Was asserted as unknown before that detection
    // existed.
    QCOMPARE(names.first().second, QString("function"));
}

void AiContentTests::extractExistingEntityNames_findsExistingTablePhrasing()
{
    const QString request = "...against existing table \"SAXS_data\" columns I and dI, using weighted LM.";
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(request);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("SAXS_data"));
    QCOMPARE(names.first().second,
             QString("window")); // "table" sits right before the quote -- known kind, unified as "window"
}

void AiContentTests::extractExistingEntityNames_emptyWithoutExistingKeyword()
{
    const QString request = R"(Create a table "GaussPeak" and fit it with a "Sphere" model.)";
    QVERIFY(extractExistingEntityNames(request).isEmpty());
}

void AiContentTests::extractExistingEntityNames_dedupesRepeatedName()
{
    const QString request = R"(Using existing "SAXS_data" table, then again the existing "SAXS_data" table.)";
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(request);
    QCOMPARE(names.size(), 1);
}

void AiContentTests::extractExistingEntityNames_skipsConditionalCreation()
{
    // Confirmed live (LIST 3's instruction): the name was refused as missing although the request
    // itself says to create it when it is missing.
    QVERIFY(
        extractExistingEntityNames(
            R"(Also write that line into table "Results": create it if it does not exist (12 rows), then put it there.)")
            .isEmpty());
    QVERIFY(extractExistingEntityNames(R"(Append the value to table "Log", creating it if missing.)").isEmpty());
    // Without the condition it stays an input; a condition in a LATER clause does not count.
    QCOMPARE(extractExistingEntityNames(R"(Write the value into table "Results" and print it.)").size(), 1);
    QCOMPARE(extractExistingEntityNames(R"(Read table "Raw". Create table "Out" if it does not exist.)").size(), 1);
}

void AiContentTests::missingEntityNames_findsMissingMarker()
{
    const QString output = "VERDICT: EXISTS SAXS_data\nVERDICT: MISSING Sphere\n";
    const QList<QPair<QString, QString>> checked = {qMakePair(QString("SAXS_data"), QString()),
                                                    qMakePair(QString("Sphere"), QString())};
    const QStringList missing = missingEntityNames(output, checked);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), QString("Sphere"));
}

void AiContentTests::missingEntityNames_emptyWhenAllExist()
{
    const QString output = "VERDICT: EXISTS SAXS_data\nVERDICT: EXISTS Sphere\n";
    const QList<QPair<QString, QString>> checked = {qMakePair(QString("SAXS_data"), QString()),
                                                    qMakePair(QString("Sphere"), QString())};
    QVERIFY(missingEntityNames(output, checked).isEmpty());
}

void AiContentTests::missingEntityNames_labeledPerCheckMissingDoesNotOverrideVerdict()
{
    // Confirmed live: the window check correctly, but irrelevantly, reports MISSING for a
    // fit-function name, while the VERDICT line correctly says EXISTS.
    const QString output = "(window) MISSING: sasviewmodels/sphere\n"
                           "(compile) EXISTS: sasviewmodels/sphere\n"
                           "(fittable) EXISTS: sasviewmodels/sphere\n"
                           "VERDICT: EXISTS sasviewmodels/sphere\n";
    const QList<QPair<QString, QString>> checked = {qMakePair(QString("sasviewmodels/sphere"), QString())};
    QVERIFY(missingEntityNames(output, checked).isEmpty());
}

void AiContentTests::missingEntityNames_selfContradictingVerdictFailsOpen()
{
    // Confirmed live: (compile)/(fittable) correctly found the model, but the model's own final
    // VERDICT line still read MISSING — a self-contradictory conclusion, not a script bug. Trusting
    // the VERDICT line alone wrongly halted a legitimate fit request; the fail-open safety net
    // (any labeled EXISTS line overrides a contradictory MISSING verdict) must catch this.
    const QString output = "(compile) EXISTS: sasviewmodels/sphere\n"
                           "(fittable) EXISTS: sasviewmodels/sphere\n"
                           "(window) MISSING: sasviewmodels/sphere\n"
                           "VERDICT: MISSING sasviewmodels/sphere\n";
    const QList<QPair<QString, QString>> checked = {qMakePair(QString("sasviewmodels/sphere"), QString())};
    QVERIFY(missingEntityNames(output, checked).isEmpty());
}

void AiContentTests::missingEntityNames_knownKindTrustsOnlyItsOwnLabel()
{
    // A window-kind-known name must only ever be checked against "(window)" -- an unrelated
    // "(compile)"/"(fittable)" EXISTS for the identical string, or the combined VERDICT, is
    // irrelevant to whether THIS window reference is missing.
    const QString output = "(window) MISSING: SAXS_data\n"
                           "(compile) EXISTS: SAXS_data\n"
                           "(fittable) EXISTS: SAXS_data\n"
                           "VERDICT: EXISTS SAXS_data\n";
    const QList<QPair<QString, QString>> checked = {qMakePair(QString("SAXS_data"), QString("window"))};
    const QStringList missing = missingEntityNames(output, checked);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), QString("SAXS_data"));
}

void AiContentTests::splitPlanCreationNames_collectsCompileAndTableNames()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create table named 'GaussPeak' with 200 rows of x,y,dy data", {}}
         << AiAssistant::SplitPart{"compile custom Porod power-law fit function named 'ai/PorodFit'", {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 2);
    for (const auto &entry : names)
    {
        if (entry.first == "GaussPeak")
            QCOMPARE(entry.second, QStringList{"window"});
        else if (entry.first == "ai/PorodFit")
        {
            QVERIFY(entry.second.contains("compile"));
            QVERIFY(entry.second.contains("fittable"));
        }
        else
            QFAIL("unexpected creation name");
    }
}

void AiContentTests::splitPlanCreationNames_mergedCompileAndSimulateKeepsCompileLabels()
{
    // Confirmed live: a fixup round merged a compile part and a simulate part into one description
    // ("compile ... fit function ... named 'ai/PorodFit' and simulate curve from it without fitting
    // real data"). Greedy noun-matching previously backtracked to the LAST noun in the sentence
    // ("curve", from the simulate clause) instead of the one next to "compile" ("function"),
    // mapping the name to {"window"} instead of {"compile","fittable"} and silently letting a real
    // compile/fittable collision through unchecked.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom Porod power-law fit function with adjustable "
                                   "exponent parameter named 'ai/PorodFit' and simulate curve from "
                                   "it without fitting real data",
                                   {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("ai/PorodFit"));
    QVERIFY(names.first().second.contains("compile"));
    QVERIFY(names.first().second.contains("fittable"));
    QVERIFY(!names.first().second.contains("window"));
}

void AiContentTests::splitPlanCreationNames_mergedCreateTableAndPlotKeepsTableLabel()
{
    // A second, differently-worded merged-part case (create+table earlier, plot+graph later in
    // the same description) — same greedy-backtracking risk as the compile/simulate case above,
    // now covered with a different verb/noun pairing so the fix isn't only proven for one exact
    // wording.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create table named 'GaussPeak' with 200 rows of x,y,dy data and "
                                   "plot a preview panel of the raw data",
                                   {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("GaussPeak"));
    QCOMPARE(names.first().second, QStringList{"window"});
}

void AiContentTests::splitPlanCreationNames_ignoresReferenceOnlyParts()
{
    // "configure fittable to use function 'ai/Gaussian'" references an already-created object by
    // name — it must never be collected as if it were about to create that name fresh, or a
    // legitimate reuse of a real pre-existing object would wrongly halt the whole plan.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom Gaussian peak fit function named 'ai/Gaussian'", {}}
         << AiAssistant::SplitPart{"configure fittable to use function 'ai/Gaussian' and fit with "
                                   "weighted errors",
                                   {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("ai/Gaussian"));
}

void AiContentTests::splitPlanCreationNames_checksNoteCreationToo()
{
    // Confirmed live by code inspection: a note colliding with an EXISTING NOTE of the same name
    // is safe (returned untouched), but newNote()'s own SIP binding only checks for an existing
    // NOTE — a note name colliding with a DIFFERENT window type (table/matrix/graph) falls through
    // to ApplicationWindow::newNote(), which silently substitutes a completely different
    // auto-generated name with no error or warning. Since the mechanical check can't tell same-type
    // from cross-type apart, note is no longer exempt — it gets the same {"window"} label as the
    // others.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create note named 'Log' with a script printing the fit summary", {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("Log"));
    QCOMPARE(names.first().second, QStringList{"window"});
}

void AiContentTests::splitPlanCreationNames_skipsExistingReferenceFindsQualifiedCreation()
{
    // Confirmed live: "Look up the existing table named \"Metrics\" ... then create and configure
    // the named 3D graph \"MetricsSurface3D\"" captured "Metrics" (a read-only reference) as the
    // creation target and never captured "MetricsSurface3D" at all (the word right after
    // "named" was "3D", not the quote) — the plan then hard-stopped on a fake collision, dropping
    // the one name that actually needed checking.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"Look up the existing table named \"Metrics\", verify its surface "
                                   "mapping and column roles, then create and configure the named 3D "
                                   "graph \"MetricsSurface3D\" as a filled-mesh surface.",
                                   {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("MetricsSurface3D"));
}

void AiContentTests::dedupeSplitPlanExactDuplicateParts_removesNonAdjacentDuplicates()
{
    // Confirmed live: a fixup round left a 4-part plan with Part1==Part3 and Part2==Part4 —
    // non-adjacent, verbatim-identical descriptions surviving all fixup rounds untouched.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PORODTEST' with adjustable "
                                   "exponent parameter",
                                   {"ai", "app", "compile"}}
         << AiAssistant::SplitPart{"simulate curve from compiled Porod power-law function without "
                                   "fitting real data",
                                   {"ai", "app", "graph"}}
         << AiAssistant::SplitPart{"compile custom fit function named 'ai/PORODTEST' with adjustable "
                                   "exponent parameter",
                                   {"ai", "app", "compile", "fittable"}}
         << AiAssistant::SplitPart{"simulate curve from compiled Porod power-law function without "
                                   "fitting real data",
                                   {"ai", "app", "graph", "fittable"}};
    QStringList removed;
    dedupeSplitPlanExactDuplicateParts(plan, &removed);
    QCOMPARE(plan.size(), 2);
    QCOMPARE(removed.size(), 2);
    QVERIFY(plan[0].description.contains("compile"));
    QVERIFY(plan[1].description.contains("simulate"));
    QVERIFY(plan[0].docTopics.contains("fittable"));
    QVERIFY(plan[1].docTopics.contains("fittable"));
}

void AiContentTests::dedupeSplitPlanExactDuplicateParts_leavesDistinctPartsAlone()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"create table named 'GaussPeak' with 200 rows of data", {"table"}}
         << AiAssistant::SplitPart{"compile custom fit function named 'ai/Gaussian'", {"compile"}};
    QStringList removed;
    dedupeSplitPlanExactDuplicateParts(plan, &removed);
    QCOMPARE(plan.size(), 2);
    QVERIFY(removed.isEmpty());
}

void AiContentTests::mechanicalAlternateSplitPlanName_picksUnusedSuffix()
{
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodFit'", {}};
    QCOMPARE(mechanicalAlternateSplitPlanName("ai/PorodFit", plan), QString("ai/PorodFit_2"));
}

void AiContentTests::mechanicalAlternateSplitPlanName_skipsSuffixAlreadyInPlan()
{
    // Confirmed the candidate isn't picked blindly -- if "_2" already appears somewhere in the
    // plan (e.g. a previous mechanical rename), the next free suffix is used instead.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodFit_2'", {}};
    QCOMPARE(mechanicalAlternateSplitPlanName("ai/PorodFit", plan), QString("ai/PorodFit_3"));
}

void AiContentTests::mechanicallyRenameSplitPlanCollisions_updatesEveryPart()
{
    // Confirmed live, 3 times: the model just re-emits the identical colliding name unchanged when
    // asked to invent a different one. This mechanical rename must actually update EVERY part that
    // references the old name, not just the one that created it.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{"compile custom fit function named 'ai/PorodFit'", {}}
         << AiAssistant::SplitPart{"simulate curve from compiled function 'ai/PorodFit'", {}};
    QStringList renames;
    mechanicallyRenameSplitPlanCollisions(plan, {"ai/PorodFit"}, &renames);
    QCOMPARE(renames.size(), 1);
    QCOMPARE(renames.first(), QString("ai/PorodFit -> ai/PorodFit_2"));
    QVERIFY(plan[0].description.contains("ai/PorodFit_2"));
    QVERIFY(plan[1].description.contains("ai/PorodFit_2"));
    QVERIFY(!plan[0].description.contains("'ai/PorodFit'"));
}

void AiContentTests::splitPlanCreationCollisions_ignoresUnrelatedKindForSameName()
{
    // Confirmed live: a table and a fit function CAN legitimately share a name with no conflict —
    // compiling never touches window storage. A window-creation part's own name must only ever be
    // checked against the (window) label, never flagged just because (compile)/(fittable) EXISTS
    // for the identical string.
    const QString output = "(window) MISSING: ai/PorodFit\n"
                           "(compile) EXISTS: ai/PorodFit\n"
                           "(fittable) EXISTS: ai/PorodFit\n"
                           "VERDICT: EXISTS ai/PorodFit\n";
    const QList<QPair<QString, QStringList>> checked = {qMakePair(QString("ai/PorodFit"), QStringList{"window"})};
    QVERIFY(splitPlanCreationCollisions(output, checked).isEmpty());
}

void AiContentTests::splitPlanCreationCollisions_findsMatchingLabeledLine()
{
    // Confirmed live: a compile-creation part's chosen name genuinely already existed as a
    // compiled function — (compile)/(fittable) EXISTS must be flagged as a real collision.
    const QString output = "(window) MISSING: ai/PorodFit\n"
                           "(compile) EXISTS: ai/PorodFit\n"
                           "(fittable) EXISTS: ai/PorodFit\n"
                           "VERDICT: MISSING ai/PorodFit\n"; // even with the self-contradicting VERDICT confirmed live
    const QList<QPair<QString, QStringList>> checked = {
        qMakePair(QString("ai/PorodFit"), QStringList{"compile", "fittable"})};
    const QStringList collisions = splitPlanCreationCollisions(output, checked);
    QCOMPARE(collisions.size(), 1);
    QCOMPARE(collisions.first(), QString("ai/PorodFit"));
}

void AiContentTests::splitPlanCreationCollisions_flagsCollisionEvenWithContradictingMissingLine()
{
    // Confirmed live: a check-script can print BOTH "(window) EXISTS: X" and "(window) MISSING: X"
    // for the same X (a broken script). Confirmed live, separately: treating that as "no evidence,
    // proceed" is the wrong direction — it silently let newTable() resize-and-clear a name that, in
    // one observed case, genuinely already held a real table. EXISTS present at all must still
    // register as a collision, per this function's own fail-CLOSED design.
    const QString output = "(window) EXISTS: newTable\n"
                           "(window) MISSING: newTable\n";
    const QList<QPair<QString, QStringList>> checked = {qMakePair(QString("newTable"), QStringList{"window"})};
    const QStringList collisions = splitPlanCreationCollisions(output, checked);
    QCOMPARE(collisions.size(), 1);
    QCOMPARE(collisions.first(), QString("newTable"));
}

void AiContentTests::splitPlanCollisionsAfterSandboxExemption_exemptsAiPrefixedCompileCollision()
{
    // compile-python-api.md Rule 6: "ai/" is disposable scratch -- a compile/fittable collision
    // there must never block.
    const QList<QPair<QString, QStringList>> checked = {
        qMakePair(QString("ai/PorodFit"), QStringList{"compile", "fittable"})};
    QStringList sandboxExempt;
    const QStringList remaining = splitPlanCollisionsAfterSandboxExemption({"ai/PorodFit"}, checked, &sandboxExempt);
    QVERIFY(remaining.isEmpty());
    QCOMPARE(sandboxExempt.size(), 1);
    QCOMPARE(sandboxExempt.first(), QString("ai/PorodFit"));
}

void AiContentTests::splitPlanCollisionsAfterSandboxExemption_leavesNonAiCollisionBlocking()
{
    const QList<QPair<QString, QStringList>> checked = {
        qMakePair(QString("sasviewmodels/PorodFit"), QStringList{"compile", "fittable"})};
    QStringList sandboxExempt;
    const QStringList remaining =
        splitPlanCollisionsAfterSandboxExemption({"sasviewmodels/PorodFit"}, checked, &sandboxExempt);
    QCOMPARE(remaining.size(), 1);
    QVERIFY(sandboxExempt.isEmpty());
}

void AiContentTests::splitPlanCollisionsAfterSandboxExemption_leavesAiTableCollisionBlocking()
{
    // Rule 6 only covers the compile/fittable function namespace, not the unified window registry
    // (table/matrix/note/graph).
    const QList<QPair<QString, QStringList>> checked = {qMakePair(QString("ai/GaussPeak"), QStringList{"window"})};
    QStringList sandboxExempt;
    const QStringList remaining = splitPlanCollisionsAfterSandboxExemption({"ai/GaussPeak"}, checked, &sandboxExempt);
    QCOMPARE(remaining.size(), 1);
    QVERIFY(sandboxExempt.isEmpty());
}

void AiContentTests::splitPlanCreationNames_scriptTabNameIsNotAWindow()
{
    // Confirmed live: "tab" was not a recognized noun, so the capture ran on to the next one it did
    // recognize — "note" — and checked the TAB's name against the window registry, printing
    // "(window) MISSING: AutoExecScript" and burning a round-trip to prove a tab is not a window.
    AiAssistant::SplitPart part;
    part.description = "create a script tab named 'AutoExecScript' in the 'Batch' note";
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames({part});
    QStringList reported;
    for (const QPair<QString, QStringList> &e : names)
        reported << e.first + "=" + e.second.join("+");
    QVERIFY2(reported.isEmpty(), qPrintable("a tab name should need no collision check: " + reported.join(", ")));

    // The window case next to it must still work — same verb, a real window noun.
    AiAssistant::SplitPart window;
    window.description = "create a note named 'Batch' with no content yet";
    const QList<QPair<QString, QStringList>> windowNames = splitPlanCreationNames({window});
    QCOMPARE(windowNames.size(), 1);
    QCOMPARE(windowNames.first().first, QString("Batch"));
    QCOMPARE(windowNames.first().second, QStringList{"window"});
}

void AiContentTests::splitPlanCreationNames_ignoresIncidentalQuotedParameter()
{
    // Confirmed live: "... function named 'ai/PorodFit' in category 'test' ..." wrongly checked
    // "test" for existence too, inheriting the function's own compile/fittable labels, since a bare
    // quote scan collects EVERY quoted string in the description, not just the actual created name.
    QList<AiAssistant::SplitPart> plan;
    plan << AiAssistant::SplitPart{
        "compile a custom Porod power-law fit function named 'ai/PorodFit' in category 'test' with "
        "all parameters adjustable",
        {}};
    const QList<QPair<QString, QStringList>> names = splitPlanCreationNames(plan);
    QCOMPARE(names.size(), 1);
    QCOMPARE(names.first().first, QString("ai/PorodFit"));
}

void AiContentTests::splitPlanCreationNamesNeedingCheck_skipsSandboxExemptEntry()
{
    const QList<QPair<QString, QStringList>> all = {
        qMakePair(QString("ai/PorodFit"), QStringList{"compile", "fittable"}),
        qMakePair(QString("GaussPeak"), QStringList{"window"})};
    QStringList skipped;
    const QList<QPair<QString, QStringList>> needing = splitPlanCreationNamesNeedingCheck(all, &skipped);
    QCOMPARE(needing.size(), 1);
    QCOMPARE(needing.first().first, QString("GaussPeak"));
    QCOMPARE(skipped.size(), 1);
    QCOMPARE(skipped.first(), QString("ai/PorodFit"));
}

void AiContentTests::existingEntityVerificationPrompt_mentionsAllNames()
{
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"Sphere", {}}, {"SAXS_data", {}}}, false);
    QVERIFY(prompt.contains("\"Sphere\""));
    QVERIFY(prompt.contains("\"SAXS_data\""));
    QVERIFY(prompt.contains("EXISTS:"));
    QVERIFY(prompt.contains("MISSING:"));
}

void AiContentTests::existingEntityVerificationPrompt_windowOnlyOmitsFunctionChecks()
{
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"SAXS_data", {"window"}}}, false);
    QVERIFY(prompt.contains("\"SAXS_data\": window check ONLY."));
    QVERIFY(!prompt.contains("ALL THREE checks"));
}

void AiContentTests::existingEntityVerificationPrompt_functionOnlyOmitsWindowCheck()
{
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"ai/GaussPeak", {"compile", "fittable"}}}, false);
    QVERIFY(prompt.contains("\"ai/GaussPeak\": compile+fittable checks ONLY."));
    QVERIFY(!prompt.contains("ALL THREE checks"));
}

void AiContentTests::existingEntityVerificationPrompt_unknownKindAsksAllThree()
{
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"Disk", {}}}, false);
    QVERIFY(prompt.contains("\"Disk\": kind unknown — ALL THREE checks."));
}

void AiContentTests::existingEntityVerificationPrompt_dataProbeAsksForRowCount()
{
    // Existence is the wrong question for a table a request is about to fit: confirmed live, a
    // 0-row "SAXS_data" reported EXISTS and the fit died ~90s later inside fit().
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"SAXS_data", {"window"}}}, true);
    QVERIFY(prompt.contains("numRows()"));
    QVERIFY(prompt.contains("colNames()"));
    QVERIFY(prompt.contains("0 rows"));
    // The verdict line is still mandatory — the probe adds to it, never replaces it.
    QVERIFY(prompt.contains("VERDICT: EXISTS"));

    const QString without = content.existingEntityVerificationPrompt({{"SAXS_data", {"window"}}}, false);
    QVERIFY(!without.contains("numRows()"));
}

void AiContentTests::existingEntityVerificationPrompt_dataProbeOmittedForFunctionOnlyNames()
{
    // A compiled function has no rows to report, so even with the probe requested there is nothing
    // for it to inspect — asking anyway would invite a table() call on a function name.
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"ai/GaussPeak", {"compile", "fittable"}}}, true);
    QVERIFY(!prompt.contains("numRows()"));
}

void AiContentTests::extractExistingEntityNames_findsBareTableNounAnywhere()
{
    // Confirmed live: this exact request extracted only the model name. "SAXS_data" sits ~15 tokens
    // past "existing" and nowhere near the verb, so both distance-limited patterns missed the most
    // explicit signal present — and that was the name whose 0-row state actually broke the item.
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(
        "Fit the existing \"sasviewmodels/sphere\" form-factor model (scale, background, Rg are "
        "fitable) from the Fittable library against table \"SAXS_data\" columns I and dI, using "
        "weighted Levenberg-Marquardt, and report chi2/dof.");
    QStringList extracted;
    QString saxsKind;
    for (const QPair<QString, QString> &e : names)
    {
        extracted << e.first;
        if (e.first == "SAXS_data")
            saxsKind = e.second;
    }
    QVERIFY2(extracted.contains("SAXS_data"), qPrintable("extracted only: " + extracted.join(", ")));
    QCOMPARE(saxsKind, QString("window"));
    QVERIFY(extracted.contains("sasviewmodels/sphere"));
}

void AiContentTests::assignSplitPlanCreationNames_namesTheItem6Deadlock()
{
    // Confirmed live: this description survived FOUR fixup rounds demanding a quoted name, got
    // none, and the item then died at generation — Direction 5 rejects every possible name when the
    // governing description names no window, so no script could ever have passed.
    AiAssistant::SplitPart part;
    part.description = "Create 3D surface plot of sin(x)*cos(y) over x and y in range -pi to pi "
                       "using filled mesh style with axis labels and legend";
    QList<AiAssistant::SplitPart> plan{part};
    QVERIFY(!splitPartsMissingCreationName(plan).isEmpty()); // the deadlock, before

    QStringList assigned;
    assignSplitPlanCreationNames(plan, &assigned);

    QCOMPARE(assigned.size(), 1);
    QCOMPARE(assigned.first(), QString("Surface")); // meaningful, not "Filled"/"Using"
    QVERIFY(plan[0].description.contains("named \"Surface\""));
    QVERIFY(splitPartsMissingCreationName(plan).isEmpty()); // and after: nothing left to complain about
    // Must be plain alphanumeric — setWindowName() rejects anything else, which is exactly why
    // encoding the formula could never satisfy the check.
    static const QRegularExpression plainRe(R"(^[A-Za-z][A-Za-z0-9_]*$)");
    QVERIFY2(plainRe.match(assigned.first()).hasMatch(), qPrintable("not a valid window name: " + assigned.first()));
}

void AiContentTests::mergeSplitPlanUnnecessaryFragments_collapsesTheItem6Plan()
{
    // Confirmed live: the complaint was raised, the model resent the SAME two-part plan unchanged,
    // and the loop gave up and shipped the fragment. Both parts also carried identical DOCS.
    AiAssistant::SplitPart plot, configure;
    plot.description = "Plot a 3D surface of sin(x)*cos(y) over x and y in the range -pi to pi "
                       "using a filled mesh style named \"Surface\"";
    plot.docTopics = QStringList{"graph3d"};
    configure.description = "Add a legend and axis labels to the 3D surface plot";
    configure.docTopics = QStringList{"graph3d"};
    QList<AiAssistant::SplitPart> plan{plot, configure};
    QVERIFY(!splitPlanUnnecessaryFragments(plan).isEmpty()); // before

    QStringList merged;
    mergeSplitPlanUnnecessaryFragments(plan, &merged);

    QCOMPARE(plan.size(), 1); // collapses to the single-part shortcut
    QCOMPARE(merged.size(), 1);
    QVERIFY(plan[0].description.contains("named \"Surface\""));   // the assigned name survives
    QVERIFY(plan[0].description.contains(", then add a legend")); // leading capital lowered
    QCOMPARE(plan[0].docTopics, QStringList{"graph3d"});          // union, not duplicated
    QVERIFY(splitPlanUnnecessaryFragments(plan).isEmpty());       // after
}

void AiContentTests::mergeSplitPlanUnnecessaryFragments_collapsesAChain()
{
    // A chain of fragments (3 configures what 2 created, 2 configures what 1 created) must all
    // land in part 1 — which is why the merge walks indices in descending order.
    // Descriptions chosen against the detector's actual vocabulary: fillOnlyRe anchors at ^ (so the
    // part must OPEN with fill/add/set/style/...), and otherActionRe excludes anything naming a
    // real action — which is why "add the FIT curve" is correctly not a mere fragment.
    AiAssistant::SplitPart a, b, c;
    a.description = "create a graph named \"Fit\"";
    a.docTopics = QStringList{"graph"};
    b.description = "Style the graph's axes with minor tick marks";
    b.docTopics = QStringList{"graph"};
    c.description = "Set the legend font to bold 12pt";
    c.docTopics = QStringList{"graph"};
    QList<AiAssistant::SplitPart> plan{a, b, c};

    QStringList merged;
    mergeSplitPlanUnnecessaryFragments(plan, &merged);

    QCOMPARE(plan.size(), 1);
    QCOMPARE(merged.size(), 2);
    QVERIFY(plan[0].description.startsWith("create a graph named \"Fit\""));
    QVERIFY(plan[0].description.contains("style the graph's axes"));
    QVERIFY(plan[0].description.contains("set the legend font"));
}

void AiContentTests::assignSplitPlanCreationNames_leavesAlreadyNamedPartsAlone()
{
    AiAssistant::SplitPart named;
    named.description = "create a table named \"RawScan\" with 50 rows of x, y sample data";
    QList<AiAssistant::SplitPart> plan{named};
    const QString before = plan[0].description;

    QStringList assigned;
    assignSplitPlanCreationNames(plan, &assigned);

    QVERIFY2(assigned.isEmpty(), qPrintable("renamed an already-named part: " + assigned.join(", ")));
    QCOMPARE(plan[0].description, before);
}

void AiContentTests::extractExistingEntityNames_skipsDeletionTargets()
{
    // LIST 5 NOSPLIT, item 6: "Pipeline" was never created by item 5, and the existence check stopped the cleanup item
    // instead of letting it delete the rest and report that "Pipeline" does not exist.
    const QString request = R"(Delete the fit functions "ai/Quad", "ai/Gauss5" and "ai/Lor5" from the library, and )"
                            R"(close and delete the table "Peak5" and the note "Pipeline". Print whether "Peak5" and )"
                            R"("Pipeline" still exist. The result label is "R6:".)";
    QStringList names;
    for (const QPair<QString, QString> &e : extractExistingEntityNames(request))
        names << e.first;
    QVERIFY2(!names.contains("Peak5") && !names.contains("Pipeline"), qPrintable("checked: " + names.join(", ")));
    // what is deleted FROM a table: the table is still an input
    names.clear();
    for (const QPair<QString, QString> &e : extractExistingEntityNames(R"(Delete rows 2 to 5 from table "Data".)"))
        names << e.first;
    QVERIFY(names.contains("Data"));
    // "closest" is no deletion verb
    names.clear();
    for (const QPair<QString, QString> &e : extractExistingEntityNames(R"(Fit the closest table "Data" to ai/Quad.)"))
        names << e.first;
    QVERIFY(names.contains("Data"));
    // a deletion in an earlier clause does not make a later input optional
    names.clear();
    for (const QPair<QString, QString> &e :
         extractExistingEntityNames(R"(Close graph "G1". Then fit table "RawScan" with ai/Quad.)"))
        names << e.first;
    QVERIFY(names.contains("RawScan") && !names.contains("G1"));
}

void AiContentTests::extractExistingEntityNames_plotIntoGraphIsCreation()
{
    // Confirmed live, a regression of the bare-kind-noun pattern itself: "Plot ... into graph
    // \"FitResults\"" had FitResults treated as an EXISTING name, came back MISSING, and the whole
    // LIST item was refused ("reply with the correct name") for a graph the request was asking to
    // CREATE. plot()/plotResults() create a window, and the creation verb sits 48 characters back
    // — outside the fixed lookback window the guard originally used, hence the clause scoping.
    const QList<QPair<QString, QString>> names = extractExistingEntityNames(
        "Plot the fit results from the previous step into graph \"FitResults\", including the data, "
        "fit curve, and residuals.");
    QStringList extracted;
    for (const QPair<QString, QString> &e : names)
        extracted << e.first;
    QVERIFY2(!extracted.contains("FitResults"), qPrintable("wrongly treated as existing: " + extracted.join(", ")));

    // ...but the very next item DOES describe it as existing, and must still be checked.
    const QList<QPair<QString, QString>> restyle =
        extractExistingEntityNames("Restyle the existing graph \"FitResults\" — set the legend font to bold 12pt");
    QStringList restyleNames;
    for (const QPair<QString, QString> &e : restyle)
        restyleNames << e.first;
    QVERIFY2(restyleNames.contains("FitResults"), qPrintable("existing graph lost: " + restyleNames.join(", ")));
}

void AiContentTests::extractExistingEntityNames_skipsCreationTargets()
{
    // The guard that makes the pattern above safe. A creation target reported as an existing name
    // comes back MISSING, and the verification prompt answers MISSING with "stop and report to the
    // user" — which would kill this item before it created anything.
    const QList<QPair<QString, QString>> created =
        extractExistingEntityNames("Create a table \"GaussPeak\" with 200 rows of synthetic Gaussian-peak data");
    QStringList names;
    for (const QPair<QString, QString> &e : created)
        names << e.first;
    QVERIFY2(!names.contains("GaussPeak"), qPrintable("wrongly treated as existing: " + names.join(", ")));

    // ...but a creation verb in an EARLIER clause must not disqualify a genuine input later on.
    const QList<QPair<QString, QString>> mixed =
        extractExistingEntityNames(R"(Create a table "Out" with 10 rows, then fit table "RawScan" into it)");
    QStringList mixedNames;
    for (const QPair<QString, QString> &e : mixed)
        mixedNames << e.first;
    QVERIFY2(mixedNames.contains("RawScan"), qPrintable("input table lost: " + mixedNames.join(", ")));
    QVERIFY2(!mixedNames.contains("Out"), qPrintable("creation target kept: " + mixedNames.join(", ")));
}

void AiContentTests::extractExistingEntityNames_rangeDotsDoNotEndTheClause()
{
    // LIST 2 item 6: the dots of "1, 2, ..., 20" ended the clause, and the graph lost its "Create".
    const QString request = "Create a note \"Log\", a table \"Metrics\" with 20 rows of x, y, and z sample data "
                            "(x = 1, 2, ..., 20, y = 21 - x, z = x*y), and a styled graph \"MetricsPlot\" of it with "
                            "a legend.";
    QStringList names;
    for (const QPair<QString, QString> &e : extractExistingEntityNames(request))
        names << e.first;
    QVERIFY2(names.isEmpty(), qPrintable("wrongly treated as existing: " + names.join(", ")));
    // A real sentence end still ends the clause: the input after it is checked.
    names.clear();
    for (const QPair<QString, QString> &e : extractExistingEntityNames(R"(Create a table "Out". Fit table "RawScan".)"))
        names << e.first;
    QVERIFY2(names == QStringList{"RawScan"}, qPrintable(names.join(", ")));
}

void AiContentTests::extractExistingEntityNames_skipsPlaceholderNames()
{
    // LIST 2 item 3: a <placeholder> is filled in by the request's own work, it names nothing that could exist.
    const QString request = "Print whether its table \"simulatedCurve-<function name>\" exists.";
    QVERIFY(extractExistingEntityNames(request).isEmpty());
    QVERIFY(inputNamesInRequest(request).isEmpty());
}

void AiContentTests::existingEntityVerificationPrompt_windowOnlyOmitsFunctionRegistryMechanics()
{
    // A window-only batch shouldn't even be TOLD how to do the compile/fittable check — no name
    // here needs it, so describing it anyway just invites running it "since it's right there".
    QtiSasAiContent content;
    const QString prompt = content.existingEntityVerificationPrompt({{"SAXS_data", {"window"}}}, false);
    QVERIFY(!prompt.contains("COMPILED-FUNCTION"));
    QVERIFY(!prompt.contains("Compiler.functionNames(\"ALL\")"));
    QVERIFY(!prompt.contains("Two independent registries"));
}

// ---------------------------------------------------------------------------
// AiAssistant public statics
// ---------------------------------------------------------------------------

void AiContentTests::extractAvailableBlock_findsListedAlternatives()
{
    const QString err = "ValueError: dataset not found.\nAvailable datasets:\n  Table1_2\n  GaussPeak_y\n";
    const QString block = AiAssistant::extractAvailableBlock(err);
    QVERIFY(block.contains("GaussPeak_y"));
}

void AiContentTests::normalizeErrorForDedup_stripsLineNumbers()
{
    const QString a = "RuntimeError: fit failed\nat <ai>:4 — chi2 = f.fit()";
    const QString b = "RuntimeError: fit failed\nat <ai>:11 — chi2 = f.fit()";
    QCOMPARE(AiAssistant::normalizeErrorForDedup(a), AiAssistant::normalizeErrorForDedup(b));
}

void AiContentTests::normalizeErrorForDedup_stripsFitTimingNoise()
{
    // Confirmed live: errContext is the full captured result.output (preceding prints included,
    // not just the bare exception) — a script whose fit succeeds before later failing on an
    // unrelated line carries the fit's own "Fit done: N ms" progress log every retry, and that
    // timing varies by wall-clock jitter alone even when the code and the eventual error are
    // 100% identical. Without stripping it, the same underlying failure repeated 6+ times in a
    // row (33+ minutes) without ever triggering the "same diagnosis twice" stop, since the two
    // errContext strings were never byte-identical.
    const QString a = "Fit done: 7 ms - 2 iteration(s)  chi²/dof: 5.5E-24\n"
                      "AttributeError: 'NoneType' object has no attribute 'enableAxis'\n"
                      "at <ai>:85 — l2.enableAxis(Graph.Axis.Top, False)";
    const QString b = "Fit done: 582 ms - 2 iteration(s)  chi²/dof: 5.5E-24\n"
                      "AttributeError: 'NoneType' object has no attribute 'enableAxis'\n"
                      "at <ai>:85 — l2.enableAxis(Graph.Axis.Top, False)";
    QCOMPARE(AiAssistant::normalizeErrorForDedup(a), AiAssistant::normalizeErrorForDedup(b));
}

void AiContentTests::normalizeErrorForDedup_stripsQuotedSourceLineDifferences()
{
    // Confirmed live: split-mode part 4 assumed `best_fc` (a part 3 local variable) was still
    // bound — a "fresh execution per round" violation. Two consecutive attempts hit the identical
    // NameError, but reordered the surrounding insertCurve() calls, so the quoted source line
    // after "at <ai>:N —" differed each time even though the actual failure never changed.
    const QString a = "NameError: name 'best_fc' is not defined\n"
                      "at <ai>:25 — l2.insertCurve(best_fc, \"x\", \"residues\", Graph.CurveType.LineSymbols)";
    const QString b = "NameError: name 'best_fc' is not defined\n"
                      "at <ai>:25 — l1.insertCurve(best_fc, \"x\", \"y\", Graph.CurveType.Line)";
    QCOMPARE(AiAssistant::normalizeErrorForDedup(a), AiAssistant::normalizeErrorForDedup(b));
}

void AiContentTests::normalizeErrorForDedup_comparesTheExceptionNotRandomOutput()
{
    // all-pyi BIGMODEL, ai-test-requests item 12: every run fitted freshly generated random data before failing on the
    // same line, so the chi2/R² printed before the exception differed and the repeat was never seen.
    const QString a = "Fit done: 131 ms - 100 iteration(s)  chi²/dof: 7.92851916484E-01  R²: 9.99902790531E-01\n"
                      "Chi2/dof for gaussian peak: 0.0040\n"
                      "AttributeError: 'GraphWindow' object has no attribute 'newLegend'\n"
                      "at <ai>:227 — leg = g.newLegend()";
    const QString b = "Fit done: 182 ms - 100 iteration(s)  chi²/dof: 7.92001727830E-01  R²: 9.99938572700E-01\n"
                      "Chi2/dof for gaussian peak: 0.0040\n"
                      "AttributeError: 'GraphWindow' object has no attribute 'newLegend'\n"
                      "at <ai>:220 — leg = g.newLegend()";
    QCOMPARE(AiAssistant::normalizeErrorForDedup(a), AiAssistant::normalizeErrorForDedup(b));
    // a block explanation (no exception line) is still compared whole
    QVERIFY(AiAssistant::normalizeErrorForDedup("reason one.") != AiAssistant::normalizeErrorForDedup("reason two."));
}

void AiContentTests::normalizeErrorForDedup_stillDistinguishesRealDifferences()
{
    // The timing-noise fix must not become so broad it masks an actually-different failure —
    // a different exception type/message must still compare unequal.
    const QString a =
        "Fit done: 7 ms - 2 iteration(s)\nAttributeError: 'NoneType' object has no attribute 'enableAxis'";
    const QString b = "Fit done: 9 ms - 2 iteration(s)\nValueError: setParamValue: no such parameter 'radius'.";
    QVERIFY(AiAssistant::normalizeErrorForDedup(a) != AiAssistant::normalizeErrorForDedup(b));
}

void AiContentTests::functionNotFoundNameFromError_extractsName()
{
    // Confirmed live, both casings seen in the wild: Compiler's setFunction() capitalizes
    // "Function", Fittable's configure() lowercases it.
    QCOMPARE(AiAssistant::functionNotFoundNameFromError("setFunction: function 'gauss_peak' not found."),
             QString("gauss_peak"));
    QCOMPARE(AiAssistant::functionNotFoundNameFromError(
                 "ValueError: configure: 'ai/Gauss' failed. Function 'ai/Gauss' not found — use "
                 "fitFunctions() to list available names."),
             QString("ai/Gauss"));
}

void AiContentTests::functionNotFoundNameFromError_emptyForUnrelatedError()
{
    QVERIFY(AiAssistant::functionNotFoundNameFromError("ValueError: group is empty").isEmpty());
}

void AiContentTests::compiledFunctionNamesFromHistory_findsNamesAcrossTurns()
{
    // Confirmed live: two functions (GaussPeakModel, LorentzianPeakModel) genuinely compiled in
    // the FIRST turn of a long conversation, then later retries lost track of them entirely.
    const QList<QPair<QString, QString>> history = {
        {"turn 1 user", "c = Compiler\nc.setFunctionName(\"ai/GaussPeakModel\")\nc.compile()\n"},
        {"turn 2 user", "c.setFunctionName(\"ai/LorentzianPeakModel\")\nc.compile()\n"},
    };
    const QStringList names = AiAssistant::compiledFunctionNamesFromHistory(history);
    QCOMPARE(names.size(), 2);
    QVERIFY(names.contains("ai/GaussPeakModel"));
    QVERIFY(names.contains("ai/LorentzianPeakModel"));
}

void AiContentTests::compiledFunctionNamesFromHistory_dedupesRepeatedName()
{
    const QList<QPair<QString, QString>> history = {
        {"u1", "c.setFunctionName(\"gauss_peak\")\nc.compile()\n"},
        {"u2", "c.setFunctionName(\"gauss_peak\")\nc.compile()\n"},
    };
    QCOMPARE(AiAssistant::compiledFunctionNamesFromHistory(history).size(), 1);
}

void AiContentTests::compiledFunctionNamesFromHistory_emptyWhenHistoryHasNone()
{
    const QList<QPair<QString, QString>> history = {{"u1", "t = newTable(\"X\", 10, 2)\n"}};
    QVERIFY(AiAssistant::compiledFunctionNamesFromHistory(history).isEmpty());
}

void AiContentTests::parseSplitMarker_stripsExplicitMarker()
{
    QString effective;
    const bool wantsSplit = AiAssistant::parseSplitMarker("SPLIT: create a table and fit it", false, &effective);
    QVERIFY(wantsSplit);
    QCOMPARE(effective, QString("create a table and fit it"));
}

void AiContentTests::parseExploreMarker_noExploreWinsOverSubstring()
{
    // "NOEXPLORE" contains "EXPLORE", so testing the positive form first would match the tail of
    // the negative one and invert the caller's intent — the same ordering trap parseReviewMarker()
    // handles for NOREVIEW/REVIEW.
    QString effective;
    QVERIFY(!AiAssistant::parseExploreMarker("NOEXPLORE: create a table", true, &effective));
    QCOMPARE(effective, QString("create a table"));
    QVERIFY(!AiAssistant::parseExploreMarker("NO EXPLORE: create a table", true, &effective));
    QCOMPARE(effective, QString("create a table"));
}

void AiContentTests::parseExploreMarker_explicitExploreOverridesDefaultOff()
{
    QString effective;
    QVERIFY(AiAssistant::parseExploreMarker("EXPLORE: fit the existing model", false, &effective));
    QCOMPARE(effective, QString("fit the existing model"));
}

void AiContentTests::parseExploreMarker_untouchedTextKeepsDefault()
{
    QString effective;
    QVERIFY(AiAssistant::parseExploreMarker("create a table", true, &effective));
    QCOMPARE(effective, QString("create a table"));
    QVERIFY(!AiAssistant::parseExploreMarker("create a table", false, &effective));
    QCOMPARE(effective, QString("create a table"));
}

void AiContentTests::parseExploreMarker_combinesWithSplitAndReviewMarkers()
{
    // ApplicationWindow::askAi() chains the three parsers over the same shrinking string, so a
    // request can carry any combination in any order and each marker must strip only its own.
    QString effective;
    const bool wantsSplit = AiAssistant::parseSplitMarker("SPLIT NOREVIEW NOEXPLORE fit it", true, &effective);
    const bool wantsReview = AiAssistant::parseReviewMarker(effective, true, &effective);
    const bool wantsExplore = AiAssistant::parseExploreMarker(effective, true, &effective);
    QVERIFY(wantsSplit);
    QVERIFY(!wantsReview);
    QVERIFY(!wantsExplore);
    QCOMPARE(effective, QString("fit it"));
}

void AiContentTests::parseListRequest_stripsExploreMarkersFromRegion()
{
    const AiAssistant::ParsedListRequest parsed =
        AiAssistant::parseListRequest("LIST NOSPLIT NOREVIEW NOEXPLORE\n1. make a table\n2. plot it");
    QVERIFY(parsed.isList);
    QVERIFY(parsed.forceNoSplit);
    QVERIFY(parsed.forceNoReview);
    QVERIFY(parsed.forceNoExplore);
    QVERIFY(!parsed.forceExplore);
    // The real point of stripping: an unrecognized marker survives as leftover text, which makes
    // the region look like a genuine instruction_block — and "NOEXPLORE" would then be appended to
    // every item of the list as an instruction.
    QCOMPARE(parsed.instructionBlock, QString());
    QCOMPARE(parsed.items.size(), 2);
}

void AiContentTests::parseListRequest_markersOnlyRegionLeavesNoInstructionBlock()
{
    const AiAssistant::ParsedListRequest positive =
        AiAssistant::parseListRequest("LIST EXPLORE\n1. fit the sphere model\n2. plot the result");
    QVERIFY(positive.isList);
    QVERIFY(positive.forceExplore);
    QVERIFY(!positive.forceNoExplore);
    QCOMPARE(positive.instructionBlock, QString());

    // A real instruction alongside the markers must survive with the markers gone, not be eaten.
    const AiAssistant::ParsedListRequest mixed =
        AiAssistant::parseListRequest("LIST NOEXPLORE use log axes\n1. plot A\n2. plot B");
    QVERIFY(mixed.forceNoExplore);
    QCOMPARE(mixed.instructionBlock, QString("use log axes"));
}

void AiContentTests::itemsWithInstruction_appendsInstructionToEveryItem()
{
    // How every LIST instruction reaches its items now, with no model call. Confirmed live: dropped, the
    // instruction ("print one line R<n>: ...") reached no item; rewritten by a model, it became invented API calls.
    const AiAssistant::ParsedListRequest parsed =
        AiAssistant::parseListRequest("LIST NOEXPLORE\nprint one line per item\n1. plot A\n2. plot B");
    QCOMPARE(parsed.instructionBlock, QString("print one line per item"));
    const QStringList items = AiAssistant::itemsWithInstruction(parsed.items, parsed.instructionBlock);
    QCOMPARE(items.size(), 2);
    for (int i = 0; i < items.size(); ++i)
    {
        QVERIFY(items[i].startsWith(parsed.items[i]));
        QVERIFY(items[i].endsWith("print one line per item"));
    }
    QCOMPARE(AiAssistant::itemsWithInstruction(parsed.items, "  "), parsed.items);
}

void AiContentTests::looksLikeNewTaskReply_detectsNewKeyword()
{
    QVERIFY(AiAssistant::looksLikeNewTaskReply("this is a new task"));
}

void AiContentTests::looksLikeNewTaskReply_detectsUnrelatedKeyword()
{
    QVERIFY(AiAssistant::looksLikeNewTaskReply("no, it's unrelated to that"));
    QVERIFY(AiAssistant::looksLikeNewTaskReply("completely different topic"));
    QVERIFY(AiAssistant::looksLikeNewTaskReply("a separate thing entirely"));
}

void AiContentTests::looksLikeNewTaskReply_defaultsToContinueForContinuePhrasing()
{
    QVERIFY(!AiAssistant::looksLikeNewTaskReply("continue"));
    QVERIFY(!AiAssistant::looksLikeNewTaskReply("yes, same as before, keep going"));
}

void AiContentTests::looksLikeNewTaskReply_defaultsToContinueForAmbiguousReply()
{
    // Fail-open: an answer that doesn't clearly say "new" must default to continuing, matching
    // today's pre-existing (pre-check) behavior rather than risking an unwanted history wipe.
    QVERIFY(!AiAssistant::looksLikeNewTaskReply("what do you mean?"));
}

// ---------------------------------------------------------------------------
// Repetition-loop truncation — see truncateRepetitiveGarbage()'s doc comment in AiAssistant.cpp
// for both confirmed-live incidents these transcribe.
// ---------------------------------------------------------------------------

void AiContentTests::truncateRepetitiveGarbage_leavesHealthyCodeAlone()
{
    const QString code = "t = newTable(\"RawScan\", 50, 2)\n"
                         "t.setColNames([\"x\", \"y\"])\n"
                         "for i in range(1, 51):\n"
                         "    t.setCell(1, i, 0.1 * i)\n"
                         "t.notifyChanges()";
    QCOMPARE(truncateRepetitiveGarbage(code), code);
}

void AiContentTests::truncateRepetitiveGarbage_allowsLegitimateDoubledLine()
{
    // Confirmed live: a two-panel stacked-plot script legitimately re-applies the same canvas
    // geometry after adding its curves, so the identical long line appears exactly twice. Tier 1's
    // threshold must stay above that — a doubled line is normal code, not a decoding loop.
    const QString code = "l1.setCanvasGeometry(x, y, w, h1)\n"
                         "l2.setCanvasGeometry(x, y + h1, w, h2)\n"
                         "l1.insertCurve(fc, \"x\", \"y\", Graph.CurveType.Line)\n"
                         "l1.setCanvasGeometry(x, y, w, h1)\n"
                         "l2.setCanvasGeometry(x, y + h1, w, h2)\n"
                         "gw.keepAligned(True)";
    QCOMPARE(truncateRepetitiveGarbage(code), code);
}

void AiContentTests::truncateRepetitiveGarbage_keepsExactlyOnePassOfCyclingBlock()
{
    // Confirmed live (LIST NOSPLIT NOREVIEW run, item 3): an EXPLORE response cycled over
    // near-identical existence checks until it hit max_tokens. Cutting at the threshold occurrence
    // left THREE copies behind, all of which then executed — the same three lines printed three
    // times. Exactly one pass must survive.
    const QString pass = "scriptPrint(\"note Batch exists: %s\" % (n is not None))\n"
                         "t = table(\"RawScan\")\n"
                         "scriptPrint(\"table RawScan exists: %s\" % (t is not None))\n";
    const QString out = truncateRepetitiveGarbage(pass + pass + pass + pass);
    QCOMPARE(out.count("scriptPrint(\"note Batch exists"), 1);
    QCOMPARE(out.count("scriptPrint(\"table RawScan exists"), 1);
    QVERIFY(out.contains("repetition-loop garbage detected"));
}

void AiContentTests::truncateRepetitiveGarbage_cutsCycleAtItsOwnOpeningLines()
{
    // The cycle's opening lines are too short for kMinLineLen to have counted them, so a cut made
    // at the trigger line alone keeps a stray fragment of the second pass (its comment header and
    // setup call). Walking back over lines that are themselves repeats removes that fragment.
    const QString pass = "# EXPLORE: check whether note Batch exists\n"
                         "n = note(\"Batch\")\n"
                         "scriptPrint(\"note Batch exists: %s\" % (n is not None))\n";
    const QString out = truncateRepetitiveGarbage(pass + pass + pass + pass);
    QCOMPARE(out.count("# EXPLORE: check whether note Batch exists"), 1);
    QCOMPARE(out.count("n = note(\"Batch\")"), 1);
}

void AiContentTests::truncateRepetitiveGarbage_keepsLooseCutForStagedProtocol()
{
    // Confirmed live: a split-plan fixup restates an unchanged PART line verbatim across its
    // draft, self-check and final stages. Tier 2 must stay OFF here — cutting back to the second
    // occurrence would delete the real answer inside ---FINAL---, the exact regression the
    // stage-boundary reset was added to fix.
    const QString part = "PART 1: create a table named \"RawScan\" with 50 rows | DOCS: app, table\n";
    const QString response = part + part + "---CHECK---\n" + part + "---FINAL---\n" + part;
    const QString out = truncateRepetitiveGarbage(response);
    QCOMPARE(out, response);
    QVERIFY(splitPlanFinalSection(out).contains("RawScan"));
}

void AiContentTests::truncateRepetitiveGarbage_neverEndsOnADanglingBlockHeader()
{
    // The kept prefix is EXECUTED, and an EXPLORE script never passes through canCompile() — so a
    // cut landing right after a block header would hand the interpreter "if t is not None:" with
    // an empty body. The cut snaps back to the enclosing top-level statement instead.
    const QString pass = "t = table(\"RawScan\")\n"
                         "if t is not None:\n"
                         "    scriptPrint(\"RawScan has %d rows\" % t.numRows())\n";
    const QString out = truncateRepetitiveGarbage(pass + pass + pass + pass);
    QVERIFY(out != pass + pass + pass + pass);
    const QStringList kept = out.split('\n');
    int lastCode = -1;
    for (int i = 0; i < kept.size(); ++i)
        if (!kept[i].trimmed().isEmpty() && !kept[i].trimmed().startsWith('#'))
            lastCode = i;
    QVERIFY(lastCode >= 0);
    QVERIFY2(!kept[lastCode].trimmed().endsWith(':'),
             qPrintable("kept prefix ends on a dangling block header: " + kept[lastCode]));
}

void AiContentTests::truncateRepetitiveGarbage_leavesResponseAloneWhenNothingSalvageable()
{
    // Every line sits inside one top-level statement, so there is no safe boundary to cut at.
    // Replacing the response with an empty (comment-only) script would be strictly worse than
    // leaving a possibly-legitimate loop body alone — same reflex as the staged-protocol case.
    const QString code = "for i in range(1, 51):\n"
                         "    t.setCell(1, i, some_reasonably_long_expression)\n"
                         "    t.setCell(1, i, some_reasonably_long_expression)\n"
                         "    t.setCell(1, i, some_reasonably_long_expression)\n"
                         "    t.setCell(1, i, some_reasonably_long_expression)";
    QCOMPARE(truncateRepetitiveGarbage(code), code);
}

// ---------------------------------------------------------------------------
// Real-request regression fixture — see ai-test-requests.md's own header for what this file is
// and why it's deliberately NOT inside qtisas/python/doc (never loaded by the AI at runtime).
// ---------------------------------------------------------------------------

namespace
{
struct AiTestRequestEntry
{
    QString heading;
    QString request;
    QStringList expectTopics;
};

// Single source of truth for parsing ai-test-requests.md, shared by realRequests_data() (which
// turns each entry into one QTest data row) and realRequestsFixture_collectsAllEntriesCleanly()
// (which sanity-checks the parsed collection as a whole) — one parser, not two copies that could
// silently drift apart.
QList<AiTestRequestEntry> parseAiTestRequestsFixture()
{
    // Path relative to this .cpp's own compiled-in location — robust regardless of the test
    // binary's working directory or install layout.
    const QString fixturePath = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath() + "/ai-test-requests.md";
    QFile file(fixturePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');

    static const QRegularExpression headingRe(R"(^##\s+(.+?)\s*$)");
    static const QRegularExpression requestRe(R"(^>\s*(.+?)\s*$)");
    static const QRegularExpression topicsRe(R"(^\*\*Expected topics:\*\*\s*(.*?)\s*$)");

    QList<AiTestRequestEntry> entries;
    QString heading;
    QString request;
    for (const QString &line : lines)
    {
        const QRegularExpressionMatch headingMatch = headingRe.match(line);
        const QRegularExpressionMatch requestMatch = requestRe.match(line);
        const QRegularExpressionMatch topicsMatch = topicsRe.match(line);
        if (headingMatch.hasMatch())
            heading = headingMatch.captured(1);
        else if (requestMatch.hasMatch())
            request = requestMatch.captured(1);
        else if (topicsMatch.hasMatch())
        {
            QStringList topics;
            for (const QString &t : topicsMatch.captured(1).split(',', Qt::SkipEmptyParts))
                topics << t.trimmed();
            entries << AiTestRequestEntry{heading, request, topics};
        }
    }
    return entries;
}
} // namespace

void AiContentTests::realRequests_data()
{
    QTest::addColumn<QString>("request");
    QTest::addColumn<QStringList>("expectTopics");

    const QList<AiTestRequestEntry> entries = parseAiTestRequestsFixture();
    // Fails loudly if the fixture's own format ever stops matching this parser, instead of the
    // test silently exercising zero requests and reporting a hollow "pass".
    QVERIFY2(!entries.isEmpty(),
             "ai-test-requests.md parsed zero entries — check the blockquote/\"Expected topics:\" format");
    for (const AiTestRequestEntry &e : entries)
        QTest::newRow(e.heading.toUtf8().constData()) << e.request << e.expectTopics;
}

void AiContentTests::realRequests()
{
    QFETCH(QString, request);
    QFETCH(QStringList, expectTopics);

    QtiSasAiContent content;
    const QStringList detected = splitPartTopicsFromAnswer("none", request, content.knownDocTopics());
    for (const QString &topic : expectTopics)
        QVERIFY2(detected.contains(topic),
                 qPrintable(QString("expected topic \"%1\" not detected; got: %2").arg(topic, detected.join(", "))));
}

void AiContentTests::realRequestsFixture_collectsAllEntriesCleanly()
{
    // Distinct from realRequests() above: that tests BEHAVIOR per row; this tests the FIXTURE
    // ITSELF as a single collected list — right count, no entry silently empty or duplicated. A
    // malformed entry (e.g. a typo'd "**Expected topics:**" line) would otherwise just vanish from
    // the data table with every remaining row still reporting a clean "all passed".
    const QList<AiTestRequestEntry> entries = parseAiTestRequestsFixture();
    QCOMPARE(entries.size(), 10);

    QStringList seenHeadings, seenRequests;
    for (const AiTestRequestEntry &e : entries)
    {
        QVERIFY2(!e.heading.isEmpty(), "fixture entry with an empty heading");
        QVERIFY2(!e.request.isEmpty(), qPrintable("fixture entry \"" + e.heading + "\" has an empty request"));
        QVERIFY2(!e.expectTopics.isEmpty(), qPrintable("fixture entry \"" + e.heading + "\" has no expected topics"));
        QVERIFY2(!seenHeadings.contains(e.heading), qPrintable("duplicate fixture heading: \"" + e.heading + "\""));
        QVERIFY2(!seenRequests.contains(e.request),
                 qPrintable("duplicate fixture request text under \"" + e.heading + "\""));
        seenHeadings << e.heading;
        seenRequests << e.request;
    }
}

namespace
{
class ChatTestServer : public QTcpServer
{
  public:
    QList<QJsonObject> requests;
    QByteArray response;
    QList<int> statuses;
    bool hold = false;
    explicit ChatTestServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            auto *socket = nextPendingConnection();
            auto buffer = std::make_shared<QByteArray>();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
                *buffer += socket->readAll();
                const int end = static_cast<int>(buffer->indexOf("\r\n\r\n"));
                if (end < 0)
                    return;
                int length = 0;
                for (const QByteArray &line : buffer->left(end).split('\n'))
                    if (line.toLower().startsWith("content-length:"))
                        length = line.mid(line.indexOf(':') + 1).trimmed().toInt();
                if (buffer->size() < end + 4 + length)
                    return;
                requests << QJsonDocument::fromJson(buffer->mid(end + 4, length)).object();
                socket->disconnect(this);
                if (!hold)
                {
                    const int status = statuses.isEmpty() ? 200 : statuses.takeFirst();
                    socket->write("HTTP/1.1 " + QByteArray::number(status) +
                                  " Test\r\nContent-Type: application/json\r\nContent-Length: " +
                                  QByteArray::number(response.size()) + "\r\nConnection: close\r\n\r\n" + response);
                    socket->disconnectFromHost();
                }
            });
        });
    }
    void answer(const QString &text)
    {
        response = QJsonDocument(
                       QJsonObject{{"choices", QJsonArray{QJsonObject{{"message", QJsonObject{{"content", text}}}}}}})
                       .toJson(QJsonDocument::Compact);
    }
    QString url() const
    {
        return QString("http://127.0.0.1:%1").arg(serverPort());
    }
};
} // namespace

void AiContentTests::chat_isolatesPromptHistoryAndReplies()
{
    ChatTestServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    AiAssistant assistant;
    QtiSasAiContent content;
    assistant.setContentProvider(&content);
    assistant.configure({}, server.url(), "test", 1024, AiAssistant::DocsMode::Integrated);
    QSignalSpy scripts(&assistant, &AiAssistant::responseReady);
    QSignalSpy chats(&assistant, &AiAssistant::chatResponseReady);
    server.answer("print('old script')");
    assistant.ask("old script request", false);
    QTRY_COMPARE(scripts.count(), 1);
    const QString history = assistant.historyText();
    // Even unexpected executable-looking text must remain a chat reply, with its formatting intact.
    const QString reply = "```python\nprint('never execute')\n```";
    server.answer(reply);
    assistant.askChat("LIST EXPLORE run it: explain uncertainty");
    QTRY_COMPARE(chats.count(), 1);
    QCOMPARE(chats.at(0).at(0).toString(), reply);
    QCOMPARE(scripts.count(), 1);
    QCOMPARE(assistant.historyText(), history);
    QCOMPARE(assistant.historySize(), 1);
    const auto messages = server.requests.at(1).value("messages").toArray();
    QCOMPARE(messages.size(), 2);
    const QString system = messages.at(0).toObject().value("content").toString();
    QVERIFY(system.contains("natural language"));
    QVERIFY(!system.contains("return ONLY the raw executable Python"));
    QVERIFY(!system.contains("API documentation"));
    QVERIFY(messages.last().toObject().value("content").toString().contains("TOPICS: ai, app"));
    server.answer("A plain explanation.");
    assistant.askChat("Continue explaining");
    QTRY_COMPARE(chats.count(), 2);
    QCOMPARE(server.requests.at(2).value("messages").toArray().size(), 4);
    assistant.clearChatHistory();
    assistant.askChat("Fresh chat");
    QTRY_COMPARE(chats.count(), 3);
    QCOMPARE(server.requests.at(3).value("messages").toArray().size(), 2);
    QCOMPARE(assistant.historyText(), history);
    server.answer("print('next script')");
    assistant.ask("new script request", false);
    QTRY_COMPARE(scripts.count(), 2);
    QVERIFY(!QJsonDocument(server.requests.last()).toJson().contains("never execute"));
}

void AiContentTests::chat_documentationModesAndFollowups()
{
    class ChatDocs : public QtiSasAiContent
    {
      public:
        mutable int loads = 0;
        mutable bool full = false;
        QString loadDocs(bool unlimited, QString *) const override
        {
            ++loads;
            full = unlimited;
            return "\n\n=== app-python-api.md ===\nAPP_REFERENCE"
                   "\n\n=== graph-python-api.md ===\nGRAPH_REFERENCE"
                   "\n\n=== matrix-python-api.md ===\nMATRIX_REFERENCE";
        }
    };
    for (int variant = 0; variant < 4; ++variant)
    {
        ChatTestServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.answer("Explanation");
        ChatDocs content;
        AiAssistant assistant;
        assistant.setContentProvider(&content);
        assistant.configure({}, server.url(), "test", 1024,
                            variant == 3   ? AiAssistant::DocsMode::Integrated
                            : variant == 1 ? AiAssistant::DocsMode::Full
                                           : AiAssistant::DocsMode::Compact);
        assistant.setBigModel(variant == 2);
        QSignalSpy chats(&assistant, &AiAssistant::chatResponseReady);
        QSignalSpy scripts(&assistant, &AiAssistant::responseReady);
        auto payload = [&server]() { return QString::fromUtf8(QJsonDocument(server.requests.last()).toJson()); };
        assistant.askChat("Explain graph legends");
        QTRY_COMPARE(chats.count(), 1);
        QVERIFY(payload().contains("natural language"));
        QVERIFY(!payload().contains("return ONLY the raw executable Python"));
        if (variant == 3)
        {
            QCOMPARE(content.loads, 0);
            QVERIFY(!payload().contains("GRAPH_REFERENCE"));
            QVERIFY(payload().contains("TOPICS: ai, app, graph"));
        }
        else
        {
            QCOMPARE(content.loads, 1);
            QCOMPARE(content.full, variant == 1);
            QVERIFY(payload().contains("GRAPH_REFERENCE"));
            QCOMPARE(payload().contains("MATRIX_REFERENCE"), variant == 2);
        }
        assistant.askChat("And their titles?");
        QTRY_COMPARE(chats.count(), 2);
        QVERIFY(payload().contains(variant == 3 ? "TOPICS: ai, app, graph" : "GRAPH_REFERENCE"));
        assistant.clearChatHistory();
        assistant.askChat("Hello");
        QTRY_COMPARE(chats.count(), 3);
        if (variant == 3)
            QVERIFY(!payload().contains("app, graph"));
        else if (variant != 2)
            QVERIFY(!payload().contains("GRAPH_REFERENCE"));
        QCOMPARE(scripts.count(), 0);
        QCOMPARE(assistant.historySize(), 0);
        QCOMPARE(content.loads, variant == 3 ? 0 : 1);
    }
}

void AiContentTests::chat_preservesStreamedText()
{
    ChatTestServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    AiAssistant assistant;
    assistant.configure({}, server.url(), "test");
    QSignalSpy scripts(&assistant, &AiAssistant::responseReady);
    QSignalSpy chats(&assistant, &AiAssistant::chatResponseReady);
    server.response = "data: {\"choices\":[{\"delta\":{\"content\":\"**Hello**\"}}]}\n\n"
                      "data: [DONE]\n\n";
    assistant.askChat("hello");
    QTRY_COMPARE(chats.count(), 1);
    QCOMPARE(chats.at(0).at(0).toString(), QString("**Hello**"));
    QCOMPARE(scripts.count(), 0);
    QCOMPARE(assistant.historySize(), 0);
    server.response = "{\"message\":{\"content\":\"Second reply\"},\"done\":true}\n";
    assistant.askChat("hello again");
    QTRY_COMPARE(chats.count(), 2);
    QCOMPARE(chats.at(1).at(0).toString(), QString("Second reply"));
    QCOMPARE(scripts.count(), 0);
}

void AiContentTests::chat_busyAndCancellationStayIsolated()
{
    ChatTestServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    server.hold = true;
    AiAssistant assistant;
    assistant.configure({}, server.url(), "test");
    QSignalSpy scripts(&assistant, &AiAssistant::responseReady);
    QSignalSpy chats(&assistant, &AiAssistant::chatResponseReady);
    QSignalSpy errors(&assistant, &AiAssistant::errorOccurred);
    assistant.askChat("wait");
    QTRY_COMPARE(server.requests.size(), 1);
    assistant.ask("must not replace chat", false);
    QCOMPARE(errors.count(), 1);
    assistant.stop();
    QTRY_VERIFY(assistant.isIdle());
    QCOMPARE(scripts.count(), 0);
    QCOMPARE(chats.count(), 0);
    QCOMPARE(assistant.historySize(), 0);
    server.hold = false;
    server.answer("Recovered");
    assistant.askChat("new chat");
    QTRY_COMPARE(chats.count(), 1);
    QCOMPARE(server.requests.last().value("messages").toArray().size(), 2);
}

void AiContentTests::chat_retryKeepsChatRouting()
{
    ChatTestServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    server.statuses = {503, 200};
    server.answer("Retried chat reply");
    AiAssistant assistant;
    assistant.configure({}, server.url(), "test");
    QSignalSpy scripts(&assistant, &AiAssistant::responseReady);
    QSignalSpy chats(&assistant, &AiAssistant::chatResponseReady);
    assistant.askChat("Explain this");
    QTRY_COMPARE(chats.count(), 1);
    QCOMPARE(server.requests.size(), 2);
    QCOMPARE(server.requests.at(0), server.requests.at(1));
    QCOMPARE(chats.at(0).at(0).toString(), QString("Retried chat reply"));
    QCOMPARE(scripts.count(), 0);
    QCOMPARE(assistant.historySize(), 0);
}

QTEST_GUILESS_MAIN(AiContentTests)
