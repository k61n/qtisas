/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: See QtiSasAiContent.h.
 ******************************************************************************/

#include "QtiSasAiContent.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

// Graph/curve styling — GraphWindow vs Graph methods, colour handling, axis titles, layer setup.
static QString graphChecklistRules()
{
    return "- Don't call showGrid() (or other layout overrides) unconditionally — they override "
           "QtiSAS Preferences (e.g. graphTitleOn(), graphMajTicksStyle()). Omit them unless the "
           "task explicitly requires overriding the user's configured defaults.\n"
           "- replot()/grid()/showGrid()/canvasGeometry()/setXTitle()/setYTitle()/setAxisTitle()/"
           "setScale() are Graph (layer) methods, never GraphWindow — call them on "
           "gw.layer(n)/gw.activeLayer() (also mechanically blocked pre-run if missed).\n"
           "- showAxis() does NOT exist on Graph (layer) or GraphWindow — calling it raises "
           "TypeError: no such method. Use enableAxis(axis, True/False) to show/hide an axis "
           "instead, e.g. layer.enableAxis(Graph.Axis.Right, True).\n"
           "- If the script calls gw.setNumLayers(n) to grow an EXISTING graph from 1 layer to "
           "more (e.g. adding a residuals panel to a graph created by an earlier "
           "plotResults()/plot() call), it MUST also call gw.setRows(...)/gw.setCols(...) BEFORE "
           "arrangeLayers() — setNumLayers() only adds the Graph object, it never updates the "
           "window's row/column count, which stays at the original graph's (1, 1). "
           "arrangeLayers(fit=False, ...) then lays out using that stale (1, 1): the new layer's "
           "row index gets clamped back onto row 0, landing it in the SAME position as the first "
           "layer, hidden behind it — arrangeLayers(fit=True, ...) does not fix this either, since "
           "for exactly 2 layers its auto-layout picks 1 row x 2 columns (side-by-side), not a "
           "vertical stack. This raises NO error and prints nothing wrong — the script completes "
           "normally and the added panel is simply invisible, so it will not be caught by any "
           "output the script itself prints. This does NOT apply to a graph created fresh via "
           "newGraph(name, 2, 2, 1) — that sets rows/cols at construction time and needs no fix. "
           "Confirmed live: a residuals-panel script called gw.setNumLayers(2) on a pre-existing "
           "single-layer 'FitResults' graph, then arrangeLayers(False, False) with no setRows()/"
           "setCols() call anywhere — the script ran with no error and printed a success message, "
           "but the residuals layer was not visible in the resulting graph.\n"
           "- If the script creates a NEW graph for data with a known physical quantity and unit, "
           "and doesn't already call setXTitle()/setYTitle() with that unit, ADD them — this is "
           "completing what the plot is supposed to show, not an unrequested embellishment (unlike "
           "tick/axis STYLING, which stays out unless asked for). The quantity/unit MUST be "
           "established by THIS request's own content — its explicit wording, its actual column "
           "names/descriptions, a domain it genuinely named (e.g. SANS, scattering) — never copied "
           "from an illustrative example elsewhere (including the SANS Q/Å⁻¹, I/cm⁻¹ example a few "
           "lines below, which is only an ILLUSTRATION of the general rule, not a default to fall "
           "back on for unrelated data). If THIS request's own data has no clearly established "
           "physical quantity or unit, leave the axis titles at their default (bare column name) — "
           "inventing a specific unit for data that was never described that way is fabrication, "
           "the same violation as inventing a numeric value, just in a title string instead of a "
           "number. Confirmed live: a plain, explicitly generic \"Gaussian peak model\" request — "
           "table columns named just \"x\"/\"y\", nothing about SANS, scattering, or Q anywhere in "
           "the request — still got labeled `setXTitle(\"Q (&Aring;<sup>-1</sup>)\")` / "
           "`setYTitle(\"I(Q) (cm<sup>-1</sup>)\")`, borrowing this exact checklist's own SANS "
           "example wholesale for data that was never Q/I at all.\n"
           "- When the quantity/unit genuinely IS established (e.g. SANS Q in Å⁻¹, I(Q) in cm⁻¹, a "
           "fitted radius in nm, from THIS request's own wording or column context — see the rule "
           "above), default axis labels just echo the bare column name (\"Q\", \"I\") with no unit "
           "and no indication of what the quantity physically is, so setting the real title is "
           "still required, not optional. Use HTML tags for units/exponents, not LaTeX: "
           "`g.setXTitle(\"Q (&Aring;<sup>-1</sup>)\")`, `g.setYTitle(\"I(Q) (cm<sup>-1</sup>)\")`. "
           "Confirmed live, repeatedly: many SANS-fit scripts in a row created a table with "
           "explicit Q/I(Q) columns and plotted them, and NONE ever set a real axis title — this "
           "never raises an error or looks obviously wrong in scriptPrint() output, so it keeps "
           "getting missed unless explicitly checked for here.\n"
           "- There is NO tick-COUNT setter on Graph under ANY name or arg count — not "
           "setMajTicks/setMinTicks, not setMajorTicks/setMinorTicks, not a combined "
           "setMajTicks(axis, majN, minN). All of these are hallucinated; none exist. Tick "
           "count is only settable via the majTicks/minTicks params of the FULL setScale(axis, "
           "start, end, step, majTicks, minTicks, ...) signature. If you don't already know the "
           "axis's current start/end, read them first with axisScale(axis) -> (start, end) and "
           "pass them straight back into setScale(...) alongside the new tick counts — don't just "
           "strip the tick-count request as unfulfillable, this IS achievable, just not via a "
           "standalone setter. This is NOT a one-off: confirmed live across THREE separate "
           "name/arg-count guesses in the same codebase, and one of them "
           "(`l.setMajTicks(Graph.Axis.Bottom, 5, 8)`) exhausted all 10 automatic error-retries "
           "and made the AI give up entirely — losing the legend and both scriptPrint() outputs "
           "the user had explicitly asked for, even though the table, compile, and fit had "
           "already completed successfully earlier in the same script.\n"
           "- In axis/plot titles, use <sup>/<sub> HTML tags for superscript/subscript (e.g. "
           "\"cm<sup>-1</sup>\", \"R<sub>g</sub>\") — never LaTeX-style ^{...}/_{...} notation, "
           "which renders as literal characters, not a real superscript/subscript.\n"
           "- A bare 0xRRGGBB int is safe as a QColor substitute for most colour parameters (e.g. "
           "setBackgroundColor, setFrameColor, setAxisColor), but NOT for setCurveLineColor()/"
           "setCurveSymbolColor()/setCurveSymbolBorderColor() specifically — these three each have "
           "BOTH an (index, int) overload, where the int is a small PALETTE INDEX, and an (index, "
           "QColor) overload. A bare Python int argument binds to the (index, int) overload, not "
           "the QColor one — so 0xFF0000 (16711680) is interpreted as colorIndex 16711680, wildly "
           "out of range for the palette (confirmed in source: Graph::setCurveLineColor(int,int) "
           "silently returns without changing anything when the index is out of range — no error, "
           "no warning, the curve keeps whatever colour it already had). A SMALL hex value (e.g. "
           "0x000003) is worse — it silently selects whatever unrelated colour sits at that small "
           "palette index instead of failing at all. For these three methods specifically, always "
           "use colorIndex(\"name\") or QColor(\"name\")/QColor(r,g,b) — never a bare int literal, "
           "regardless of how it looks like a valid RGB hex value.\n"
           "- \"colored symbols\"/\"colored markers\"/\"colored points\" (Scatter or LineSymbols "
           "curve style) means setCurveSymbolColor()/setCurveSymbolBorderColor() — NOT "
           "setCurveLineColor(), which only recolors the connecting LINE between points, never the "
           "symbol itself. Confirmed in source: a curve's initial symbol color is copied from its "
           "line color only ONCE, at curve creation (Graph::initCurveLayout()'s `cl.symCol = "
           "cl.lCol`); a later setCurveLineColor() call updates only the QwtPlotCurve's own pen, "
           "never touching the QwtSymbol already attached to it. A request for \"red star symbols\" "
           "satisfied only via setCurveLineColor(0, colorIndex(\"red\")) turns the connecting line "
           "red while the stars themselves keep whatever color they were auto-assigned at creation "
           "— call setCurveSymbolColor() (fill) and, for a filled look, setCurveSymbolBorderColor() "
           "(outline) too.\n"
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
           "- Same principle, third confirmed case: `addCurve()` has NO name-based dedup at all — "
           "it succeeds silently every time, even when a curve with the same title already exists "
           "on the layer. Confirmed live, worse than the plot()/compile() cases above: repeated "
           "retries styling one fit-results graph (each fixing an unrelated error a few lines "
           "later — a wrong color-argument type, a hallucinated legend method) kept re-running an "
           "unconditional `g.addCurve(t, \"I\", ...)` with no existence check, ending with the SAME "
           "data curve added FIVE separate times onto one layer alongside the real fit/residual "
           "curves — invisible in the script's own printed output, only found by inspecting the "
           "live graph afterward. Before calling addCurve() in a corrected/retried script, check "
           "for an existing curve with that title first: `if not any(g.curveTitle(i) == title for "
           "i in range(g.numCurves())): g.addCurve(...)`.\n";
}

// Compiler — setGroup() requirement, the redefinition/output-variable-assignment rule.
static QString compileChecklistRules()
{
    return "- A brand-new Compiler function (c.new() followed by setFunctionName()/setCode()/"
           "QtiSAS Preferences (e.g. graphTitleOn(), graphMajTicksStyle()). Omit them unless the "
           "task explicitly requires overriding the user's configured defaults.\n"
           "- replot()/grid()/showGrid()/canvasGeometry()/setXTitle()/setYTitle()/setAxisTitle()/"
           "setScale() are Graph (layer) methods, never GraphWindow — call them on "
           "gw.layer(n)/gw.activeLayer() (also mechanically blocked pre-run if missed).\n"
           "- showAxis() does NOT exist on Graph (layer) or GraphWindow — calling it raises "
           "TypeError: no such method. Use enableAxis(axis, True/False) to show/hide an axis "
           "instead, e.g. layer.enableAxis(Graph.Axis.Right, True).\n"
           "- If the script calls gw.setNumLayers(n) to grow an EXISTING graph from 1 layer to "
           "more (e.g. adding a residuals panel to a graph created by an earlier "
           "plotResults()/plot() call), it MUST also call gw.setRows(...)/gw.setCols(...) BEFORE "
           "arrangeLayers() — setNumLayers() only adds the Graph object, it never updates the "
           "window's row/column count, which stays at the original graph's (1, 1). "
           "arrangeLayers(fit=False, ...) then lays out using that stale (1, 1): the new layer's "
           "row index gets clamped back onto row 0, landing it in the SAME position as the first "
           "layer, hidden behind it — arrangeLayers(fit=True, ...) does not fix this either, since "
           "for exactly 2 layers its auto-layout picks 1 row x 2 columns (side-by-side), not a "
           "vertical stack. This raises NO error and prints nothing wrong — the script completes "
           "normally and the added panel is simply invisible, so it will not be caught by any "
           "output the script itself prints. This does NOT apply to a graph created fresh via "
           "newGraph(name, 2, 2, 1) — that sets rows/cols at construction time and needs no fix. "
           "Confirmed live: a residuals-panel script called gw.setNumLayers(2) on a pre-existing "
           "single-layer 'FitResults' graph, then arrangeLayers(False, False) with no setRows()/"
           "setCols() call anywhere — the script ran with no error and printed a success message, "
           "but the residuals layer was not visible in the resulting graph.\n"
           "- If the script creates a NEW graph for data with a known physical quantity and unit, "
           "and doesn't already call setXTitle()/setYTitle() with that unit, ADD them — this is "
           "completing what the plot is supposed to show, not an unrequested embellishment (unlike "
           "tick/axis STYLING, which stays out unless asked for). The quantity/unit MUST be "
           "established by THIS request's own content — its explicit wording, its actual column "
           "names/descriptions, a domain it genuinely named (e.g. SANS, scattering) — never copied "
           "from an illustrative example elsewhere (including the SANS Q/Å⁻¹, I/cm⁻¹ example a few "
           "lines below, which is only an ILLUSTRATION of the general rule, not a default to fall "
           "back on for unrelated data). If THIS request's own data has no clearly established "
           "physical quantity or unit, leave the axis titles at their default (bare column name) — "
           "inventing a specific unit for data that was never described that way is fabrication, "
           "the same violation as inventing a numeric value, just in a title string instead of a "
           "number. Confirmed live: a plain, explicitly generic \"Gaussian peak model\" request — "
           "table columns named just \"x\"/\"y\", nothing about SANS, scattering, or Q anywhere in "
           "the request — still got labeled `setXTitle(\"Q (&Aring;<sup>-1</sup>)\")` / "
           "`setYTitle(\"I(Q) (cm<sup>-1</sup>)\")`, borrowing this exact checklist's own SANS "
           "example wholesale for data that was never Q/I at all.\n"
           "- When the quantity/unit genuinely IS established (e.g. SANS Q in Å⁻¹, I(Q) in cm⁻¹, a "
           "fitted radius in nm, from THIS request's own wording or column context — see the rule "
           "above), default axis labels just echo the bare column name (\"Q\", \"I\") with no unit "
           "and no indication of what the quantity physically is, so setting the real title is "
           "still required, not optional. Use HTML tags for units/exponents, not LaTeX: "
           "`g.setXTitle(\"Q (&Aring;<sup>-1</sup>)\")`, `g.setYTitle(\"I(Q) (cm<sup>-1</sup>)\")`. "
           "Confirmed live, repeatedly: many SANS-fit scripts in a row created a table with "
           "explicit Q/I(Q) columns and plotted them, and NONE ever set a real axis title — this "
           "never raises an error or looks obviously wrong in scriptPrint() output, so it keeps "
           "getting missed unless explicitly checked for here.\n"
           "- There is NO tick-COUNT setter on Graph under ANY name or arg count — not "
           "setMajTicks/setMinTicks, not setMajorTicks/setMinorTicks, not a combined "
           "setMajTicks(axis, majN, minN). All of these are hallucinated; none exist. Tick "
           "count is only settable via the majTicks/minTicks params of the FULL setScale(axis, "
           "start, end, step, majTicks, minTicks, ...) signature. If you don't already know the "
           "axis's current start/end, read them first with axisScale(axis) -> (start, end) and "
           "pass them straight back into setScale(...) alongside the new tick counts — don't just "
           "strip the tick-count request as unfulfillable, this IS achievable, just not via a "
           "standalone setter. This is NOT a one-off: confirmed live across THREE separate "
           "name/arg-count guesses in the same codebase, and one of them "
           "(`l.setMajTicks(Graph.Axis.Bottom, 5, 8)`) exhausted all 10 automatic error-retries "
           "and made the AI give up entirely — losing the legend and both scriptPrint() outputs "
           "the user had explicitly asked for, even though the table, compile, and fit had "
           "already completed successfully earlier in the same script.\n"
           "- In axis/plot titles, use <sup>/<sub> HTML tags for superscript/subscript (e.g. "
           "\"cm<sup>-1</sup>\", \"R<sub>g</sub>\") — never LaTeX-style ^{...}/_{...} notation, "
           "which renders as literal characters, not a real superscript/subscript.\n"
           "- A bare 0xRRGGBB int is safe as a QColor substitute for most colour parameters (e.g. "
           "setBackgroundColor, setFrameColor, setAxisColor), but NOT for setCurveLineColor()/"
           "setCurveSymbolColor()/setCurveSymbolBorderColor() specifically — these three each have "
           "BOTH an (index, int) overload, where the int is a small PALETTE INDEX, and an (index, "
           "QColor) overload. A bare Python int argument binds to the (index, int) overload, not "
           "the QColor one — so 0xFF0000 (16711680) is interpreted as colorIndex 16711680, wildly "
           "out of range for the palette (confirmed in source: Graph::setCurveLineColor(int,int) "
           "silently returns without changing anything when the index is out of range — no error, "
           "no warning, the curve keeps whatever colour it already had). A SMALL hex value (e.g. "
           "0x000003) is worse — it silently selects whatever unrelated colour sits at that small "
           "palette index instead of failing at all. For these three methods specifically, always "
           "use colorIndex(\"name\") or QColor(\"name\")/QColor(r,g,b) — never a bare int literal, "
           "regardless of how it looks like a valid RGB hex value.\n"
           "- \"colored symbols\"/\"colored markers\"/\"colored points\" (Scatter or LineSymbols "
           "curve style) means setCurveSymbolColor()/setCurveSymbolBorderColor() — NOT "
           "setCurveLineColor(), which only recolors the connecting LINE between points, never the "
           "symbol itself. Confirmed in source: a curve's initial symbol color is copied from its "
           "line color only ONCE, at curve creation (Graph::initCurveLayout()'s `cl.symCol = "
           "cl.lCol`); a later setCurveLineColor() call updates only the QwtPlotCurve's own pen, "
           "never touching the QwtSymbol already attached to it. A request for \"red star symbols\" "
           "satisfied only via setCurveLineColor(0, colorIndex(\"red\")) turns the connecting line "
           "red while the stars themselves keep whatever color they were auto-assigned at creation "
           "— call setCurveSymbolColor() (fill) and, for a filled look, setCurveSymbolBorderColor() "
           "(outline) too.\n"
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
           "- Same principle, third confirmed case: `addCurve()` has NO name-based dedup at all — "
           "it succeeds silently every time, even when a curve with the same title already exists "
           "on the layer. Confirmed live, worse than the plot()/compile() cases above: repeated "
           "retries styling one fit-results graph (each fixing an unrelated error a few lines "
           "later — a wrong color-argument type, a hallucinated legend method) kept re-running an "
           "unconditional `g.addCurve(t, \"I\", ...)` with no existence check, ending with the SAME "
           "data curve added FIVE separate times onto one layer alongside the real fit/residual "
           "curves — invisible in the script's own printed output, only found by inspecting the "
           "live graph afterward. Before calling addCurve() in a corrected/retried script, check "
           "for an existing curve with that title first: `if not any(g.curveTitle(i) == title for "
           "i in range(g.numCurves())): g.addCurve(...)`.\n";
}

// Fittable — weighting/yErr setup, results() unpacking, fitCurve table reading, Rule 1 (known name vs fitFunctions()
// discovery), hardcoded-metric fabrication.
static QString fittableChecklistRules()
{
    return "- Don't wrap setParamValue()/setParamVaries()/fit() in a try/except that swallows the "
           "error and continues (e.g. calling plotResults() afterward regardless) — let real "
           "errors propagate so they can be corrected properly.\n"
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
           "- The check above assumes the error column's yErr role was actually assigned in the "
           "FIRST place — verify that too. If the original request mentions weighted/error-bar "
           "fitting and a table-creation script (this part or an earlier one in the same "
           "conversation) makes a column clearly meant as the uncertainty (named dy/dI/error/sigma/"
           "unc, or explicitly described as one) but never calls t.setColumnRole(N, "
           "Table.PlotDesignation.yErr) on it, f.setWeightingDataset() will reject that column's "
           "fully-qualified name outright — no naming mistake needed, a CORRECTLY qualified "
           "\"table_col\" string still fails, because setWeightingDataset() only accepts columns "
           "that already carry the yErr role for that dataset slot. The resulting ValueError's own "
           "text (\"...use the fully-qualified 'tablename_colname' form...\") misleadingly points at "
           "a naming fix that was never the problem, and reassigning the role AFTER configure() has "
           "already run does not retroactively fix it either — configure() must be called again "
           "afterward. Confirmed live, severe: a missing setColumnRole(3, yErr) in an early part "
           "burned 12+ EXPLORE/retry rounds in a later part — dir(f), a hallucinated "
           "weightingDatasetNames() method, manually reassigning the role mid-script — before the "
           "real cause (the missing role, several parts back) was ever identified.\n"
           "- fittable-python-api.md Rule 1 already states this exception explicitly: \"if the "
           "script itself compiled the function via Compiler, the name is already known — no "
           "discovery needed.\" If an earlier part in this same conversation (visible in history) "
           "already compiled a custom function via Compiler.setFunctionName(...), a later part "
           "MUST use that exact confirmed name directly — never run a f.fitFunctions(pattern) "
           "discovery search for it instead, and never apply the SEPARATE, Rule-1-documented "
           "\"exclude ai/* candidates\" discovery pattern (meant only for finding a PRE-EXISTING, "
           "vetted library model when the exact name is genuinely unknown) to a just-compiled "
           "custom function — that pattern actively deprioritizes \"ai/\"-prefixed matches, so it "
           "can silently select a completely different, unrelated function of a similar name "
           "instead of the one actually intended, with no error: the fit still runs and reports a "
           "real chi2/dof, just for the wrong model, and the fitCurve-<name> table it creates is "
           "never the one a later part will correctly look for. Confirmed live: a part compiled "
           "\"ai/GaussPeak\" via Compiler, but the NEXT part searched "
           "f.fitFunctions(\"*GaussPeak*\") preferring non-\"ai/\" matches and configured whatever "
           "`hits[0]` returned instead of the literal \"ai/GaussPeak\" string it already had — a "
           "later part's fitCurve-ai/GaussPeak lookup then correctly, but confusingly, reported "
           "the table missing, since no fit had ever actually run against that function.\n"
           "- If the script calls f.configure(name, [\"table_col\", ...]) naming a table/column not "
           "yet confirmed to exist in THIS conversation, guard BOTH levels before the configure() "
           "call (see fittable-python-api.md Rule 2), nested as if/else so configure()/fit() only "
           "run in the innermost else. CRITICAL — the colNames() check uses the BARE column label "
           "ONLY, never the fully-qualified \"table_col\" string configure() itself takes — these "
           "are two different strings for two different APIs, confirmed against source: `colNames()` "
           "always returns bare labels (e.g. \"I\"), never table-prefixed, regardless of what "
           "configure()'s own dataset argument looks like. Concrete example, table \"Data\", "
           "column \"I\": `if not existTable(\"Data\"): scriptPrint(...) else: t = table(\"Data\"); "
           "if \"I\" not in t.colNames(): scriptPrint(...) else: f.configure(model, [\"Data_I\"])` — "
           "note \"I\" (bare) in the colNames() check vs. \"Data_I\" (qualified) in configure()'s "
           "own argument list a few lines later; these must NEVER be the same string. Confirmed "
           "live: self-review, applying this exact guard template, substituted the qualified "
           "dataset string (\"GaussPeak_I\") into the colNames() check instead of the bare label "
           "(\"I\") — the check then failed every time even though the column genuinely existed, "
           "silently blocking configure()/fit() in two separate parts with no error raised. "
           "existTable() ALONE is not enough — it only rules out \"table doesn't exist at all\"; a "
           "table can exist while the named column doesn't, and configure() still raises the exact "
           "same ValueError one level down. Don't let configure() itself discover either kind of "
           "missing. Confirmed live: once with no guard at all, once with an existTable()-only "
           "guard that let a missing-column case fall straight through into configure() anyway, and "
           "once with the bare-vs-qualified mixup above — each cost a full extra retry round-trip "
           "(or, in the mixup case, silently skipped the real work with no retry at all).\n"
           "- `Fittable.results()[\"paramName\"]` returns a `(value, error)` TUPLE for a "
           "single-dataset fit, never a bare scalar — indexing or unpacking is required before "
           "using it as a number (e.g. in an f-string with `:.2f`, or any arithmetic). Confirmed "
           "live TWICE in the same conversation: first `results()[\"R\"][0][0]` (one index too "
           "many — `results()[\"R\"][0]` is already the float) raised `TypeError: 'float' object "
           "is not subscriptable`; after that was corrected to the right `results()[\"R\"][0]`, a "
           "LATER, separately-generated script in the same conversation reverted to the bare, "
           "unindexed `results()[\"R\"]` passed straight into an f-string, raising `TypeError: "
           "unsupported format string passed to tuple.__format__` — the earlier, already-fixed "
           "mistake recurred because each new script is generated fresh, not diffed against the "
           "last corrected one. The reliable form is `value, error = f.results()[\"R\"]`, matching "
           "fittable-python-api.md's own documented example — never index blindly with `[0]` and "
           "never pass the dict entry directly into a format spec.\n"
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
           "- A hardcoded literal for a Fittable metric (chi2/dof, R^2, a fitted parameter value) is "
           "fabrication, even with a comment citing which earlier part it came from. Read it from "
           "the fitCurve-<model> table directly (read_characteristic()/read_param()) — never type "
           "in a literal number (also mechanically blocked pre-run if missed).\n"
           "- When a script adds both a data curve and a residuals curve, verify the residuals "
           "curve's TABLE argument is the fitCurve-<model> table (e.g. `best_fc`/`fc`), never the "
           "original data table — even when its column argument is spelled \"residues\"/\"residuals\" "
           "correctly, or the surrounding comment correctly says \"residuals\". The original data "
           "table has NO residuals column of its own; only the fitCurve table does. This is easy to "
           "miss because the original data table usually has a column literally named \"y\", which "
           "resembles a plausible curve source and raises no NameError/AttributeError. Confirmed "
           "live: a script correctly used `best_fc` for the fit line (`l1.insertCurve(best_fc, \"x\", "
           "\"y\", ...)`) two lines later, but the residuals line just above it was "
           "`l2.addCurve(t, \"y\", ...)` — `t` being the ORIGINAL data table, not `best_fc` — under "
           "a comment that correctly read \"# Add residuals\". The script ran with no error; the "
           "\"residuals panel\" silently plotted a second copy of the raw data instead of any actual "
           "residual. Survived three separate self-review passes (the original review plus two "
           "unrelated runtime-error retries) before reaching the merged script unfixed.\n"
           "- Don't add scientific/modeling complexity beyond what was explicitly requested — "
           "instrumental resolution smearing (setInstrumentalFit/setResoFunction/setResoDataset), "
           "polydispersity (setPolyFunction/setPolyEnabled/setPolyDataset), or any other Fittable "
           "refinement the request never mentioned is an unrequested embellishment, exactly like "
           "unrequested axis/tick styling elsewhere in this checklist — remove it. It also adds "
           "real failure surface for no benefit: confirmed live, a plain \"fit with weighted "
           "errors\" request grew an unrequested `setPolyDataset(1, \"\")` (an empty, meaningless "
           "sigma-column argument) and a full resolution-smearing setup, neither asked for, on top "
           "of the actual fit.\n";
}

// General script hygiene — missing imports, existTable()/colNames() guards, compile()'s return value, "don't silently
// rewrite unrelated lines".
static QString generalChecklistRules()
{
    return "- Unless the request itself explicitly describes real SANS/SAXS scattering data "
           "(mentions scattering, form factor, Guinier, radius of gyration, momentum transfer, "
           "Å⁻¹/nm⁻¹ units, or similar), do NOT default to this app's own scattering-domain "
           "notation — column names \"Q\"/\"I\"/\"dI\", Python variable names like `q`/`Itrue`/"
           "`Iobs`, or axis titles/units like \"I(Q)\"/\"Q (1/nm)\". When the request gives its "
           "own explicit names (e.g. \"x, y, dy columns\"), use exactly those, for the table "
           "columns AND the corresponding Python variables AND any axis titles built from them — "
           "not just the table. This is a strong, specific habit from this app's own domain (it's "
           "SANS/SAXS analysis software), not a neutral default — it will feel natural to reach "
           "for even though nothing in a generic request asked for it. Confirmed live, "
           "repeatedly, same request, same result each time: asked for a table named 'GaussPeak' "
           "with explicit \"x, y, dy columns\" containing a generic simulated Gaussian peak (no "
           "scattering context at all), the table was created with columns \"Q\"/\"I\"/\"dI\" "
           "instead — and the SAME run's variable names (`q`, `Itrue`, `Iobs`) and final axis "
           "titles (\"I(Q)\", \"Q (1/nm)\") followed the same pattern, showing it is one "
           "consistent bias spanning naming, code, and presentation, not an isolated slip in "
           "just one call.\n"
           "- Compiler, Fittable, and Dan (and any other top-level app widget accessed the same way) "
           "are pre-existing GLOBAL SINGLETON instances, accessed via their bare name — "
           "`c = Compiler`, `f = Fittable` — never called as a constructor. `Compiler()`/"
           "`Fittable()`/`Dan()` raises `TypeError: '<name>18' object is not callable` (the "
           "class name reveals the underlying C++ singleton — compile18/fittable18/dan18) "
           "since these are already-constructed instances exposed directly to Python, not "
           "classes you instantiate yourself. This is unlike Table/Matrix/Note/graph windows, "
           "which DO use factory functions (newTable()/newGraph()/...) to create a NEW "
           "instance each time. Confirmed live: a review pass changed an already-correct `c = "
           "Compiler` into `c = Compiler()`, and the resulting TypeError triggered a cascade "
           "of retries guessing ever-more-unrelated alternatives (`Dan.Compiler()`, "
           "`Fittable()`, `Dan()`) before exhausting the retry budget entirely — never once "
           "trying the simple fix of removing the parentheses.\n"
           "- Don't fabricate an unverified \"<X> already exists — skipping\" claim just to "
           "stop retrying after repeated unrelated failures (e.g. wrong Compiler/Fittable "
           "usage syntax) — that claim may be false, and a later part relying on it (e.g. "
           "fitting against a function that was never actually compiled) fails for a reason "
           "that looks unrelated to the real cause. If existence genuinely matters, check it "
           "properly (e.g. via fitFunctions()/functionNames()) before reporting it either way.\n"
           "- A concrete literal value shown in this checklist or the API docs (a color name, unit, "
           "symbol shape, group name, function name, ...) is an ILLUSTRATION of how to call "
           "the method, never a default to fall back on. If the request specifies its own "
           "explicit value for the same parameter, that value must be used instead, "
           "regardless of what any example shows. Confirmed live, twice, same shape: (1) "
           "axis titles — a script for a generic, unit-less \"Gaussian peak model\" request "
           "(columns just \"x\"/\"y\", nothing about SANS/scattering/Q anywhere in the "
           "request) got labeled with this exact checklist's own SANS Q/Å⁻¹, I/cm⁻¹ "
           "illustration verbatim; (2) symbol shape — a request explicitly asking for "
           "\"star\" symbols instead produced `symbolIndex(\"diamond\")` — "
           "graph-python-api.md's own worked example for setCurveSymbolStyle() uses "
           "\"diamond\" as its illustrative value, and that literal carried into the script "
           "unchanged instead of being replaced with the actually-requested \"star\".\n"
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
           "dataset name than the one the original request explicitly named (in quotes), just "
           "because that one happens to exist, is equally out of scope. An error's \"Available: "
           "X\" listing is context for a report, never implicit permission to configure/fit "
           "against X instead. Confirmed live: after correctly reporting that table \"AAA\" "
           "column \"BBB\" didn't exist, a later retry silently configured against a different "
           "table's column just because it was the only one available. This applies to a quoted "
           "Fittable function/model name too (also mechanically blocked pre-run if missed) — "
           "EXCEPTION granted only for Fittable's own required discovery workflow "
           "(fittable-python-api.md Rule 1), and the substitution MUST still be printed "
           "explicitly, never silently swapped in.\n"
           "- EXCEPTION to the rule above: it does NOT apply once the user's CURRENT message "
           "explicitly hands the choice back to you (\"select/choose it yourself\", \"use "
           "whatever's available\", \"pick any\", or similar) — that is authorized deviation, not "
           "silent substitution; honor it rather than reverting to the original name. Confirmed "
           "live: exactly this exception was missed once — after \"select column yourself\" "
           "correctly produced a script using a real column, self-review reverted it back to the "
           "original, already-rejected column name instead of accepting the authorized "
           "substitution.\n"
           "- That existTable()-guard template applies ONLY to a table/dataset the script did NOT "
           "itself create — never wrap it around a table this same script creates via newTable(...) "
           "a few lines earlier. Replacing that table's newTable(...) call and its data-fill loop "
           "with `if not existTable(\"name\"): scriptPrint(...)` guarantees the check fails, since "
           "nothing in the script ever created the table — the drafted script's creation step must "
           "stay. Confirmed live: a drafted script that created table \"SphereSANS2\" via newTable() "
           "and filled 150 rows was rewritten by self-review into a script that only checked "
           "existTable(\"SphereSANS2\") and printed \"does not exist\" — the newTable() call and the "
           "entire fill loop were dropped, so the table was never created and the check correctly, "
           "but uselessly, failed every time.\n"
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
           "which were ever the actual problem. (3) self-review silently changed a correct "
           "`leg.setText(fit_label + \"\\n\" + data_label)` into `... + \"\\\\n\" + ...` (single "
           "backslash-n into an escaped double-backslash-n) while fixing an unrelated compile "
           "error a few lines above — the legend then rendered a literal backslash-n between the "
           "two labels instead of an actual line break, silently, in every subsequent retry for "
           "the rest of the conversation.\n"
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
           "because self-review never checks against the API docs, only this checklist. EXCEPTION: "
           "a bare `0xRRGGBB` int (e.g. 0xFFFFFF for white) IS a genuinely correct, "
           "confirmed-live-working substitute for QColor(...) anywhere a QColor parameter is "
           "expected, needs no import at all, and should NOT be flagged or 'fixed' by adding an "
           "unnecessary QColor import — only a small colorIndex()-style int (0, 1, 2, ...), which "
           "is not a valid packed-RGB value, would be wrong here.\n"
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
           "redoing it.\n"
           "- Table.setCell/cell/cellData/setColumnRole and Matrix.setCell/cell are ALL 1-based — "
           "column/row index 0 never exists. Do NOT 'correct' an already-correct 1-based script "
           "to 0-based indexing (also mechanically blocked pre-run if missed).\n";
}

// Split-mode specific — EXPLORE side effects from an earlier round, task abdication ("happens in a later part").
static QString splitModeChecklistRules()
{
    return "- The script you are reviewing may not be the first code to run in this request — an "
           "earlier `# EXPLORE` round in the SAME conversation executes through the identical "
           "script runner as the action script you're now reviewing, with real, permanent side "
           "effects and no rollback. 'EXPLORE stays read-only' is a prompt-level convention the "
           "model is expected to follow, NOT a runtime guarantee — nothing prevents (and nothing "
           "here has reviewed) an EXPLORE-labeled script that itself called addCurve()/setCell()/"
           "Compiler.compile()/etc. Confirmed live: the addCurve()-quintupled case above happened "
           "specifically inside a block the model itself labeled `# EXPLORE`, which never reached "
           "this checklist at all (self-review only ever runs on action scripts). When reviewing "
           "an action script that follows an EXPLORE round this same request, treat anything that "
           "EXPLORE round's output shows as already true, live state — e.g. a graph/curve/table it "
           "printed as already existing — never something to recreate from scratch.\n"
           "- A script whose entire content is a scriptPrint() claiming the actual work \"happens in "
           "a later part\"/\"in a separate step\"/\"will be done next\", with no real check "
           "(existTable(), colNames(), etc.) backing that claim, is not a completion — it is the "
           "task being skipped while still LOOKING like a valid, error-free result. There is no "
           "other script that will additionally do this one's job; if this is a split-mode part, "
           "no OTHER part covers it either (see the part's own description restated above, if "
           "present). Confirmed live: a part whose sole assigned job was to create a table came "
           "back as ONLY `scriptPrint(\"Table not created — this script only creates the table in "
           "a later part of the full plan.\")` — false, no later part did, and the table was never "
           "created for the rest of the conversation. If the script genuinely cannot proceed, the "
           "scriptPrint() must name a SPECIFIC missing prerequisite confirmed by a real check run "
           "in THIS script (not recalled from memory) — never a vague deferral to other, unnamed "
           "work.\n";
}

QString QtiSasAiContent::selfReviewChecklist() const
{
    return graphChecklistRules() + compileChecklistRules() + fittableChecklistRules() + generalChecklistRules() +
           splitModeChecklistRules();
}


QString QtiSasAiContent::docTopicIndex() const
{
    return "compile — create, edit, and compile NEW fit functions (Compiler class: c = Compiler). FIF "
           "files, C++ code generation, parameters/limits, after-fit hooks.\n"
           "fittable — configure and RUN fits using EXISTING compiled functions (Fittable class: "
           "f = Fittable). Parameters, weighting, resolution, polydispersity, global/batch fits.\n"
           "graph — 2D plots (GraphWindow/Graph/MultiLayer): curves, axes, layers, styling.\n"
           "graph3d — 3D surface/scatter plots (Graph3D/Qti3D).\n"
           "matrix — the Matrix class: 2D numeric grids/image-like data.\n"
           "note — the Note class: text/script notes and tabs.\n"
           "table — the Table class: spreadsheet-like data tables, columns, import/export.\n";
}

QStringList QtiSasAiContent::knownDocTopics() const
{
    return {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable"};
}

QStringList QtiSasAiContent::baselineDocTopics() const
{
    // "ai" — assistant workflow rules. "app" — newTable/newGraph/newMatrix/newNote/scriptPrint/
    // qti.app, used in nearly every script regardless of topic.
    return {"ai", "app"};
}

QString QtiSasAiContent::splitPlanPreflightPrompt(const QString &requestText) const
{
    return "Split-mode pre-flight: decide whether the request below should be broken into an "
           "ORDERED list of smaller, independently-runnable parts, each generated, reviewed, and "
           "executed as its OWN separate script in sequence — NOT combined into one script — or "
           "whether it is already a single coherent step that shouldn't be fragmented.\n\n"
           "Split into multiple parts ONLY where doing so gives a genuine benefit: an earlier part "
           "performs a distinct, already-durable action (creating a table, compiling a function, "
           "running a fit, producing a graph, applying styling) that a LATER, unrelated part "
           "doesn't need to redo or risk re-running if it fails. Do NOT fragment a request that is "
           "really one coherent step just to produce more parts — e.g. \"create a table and "
           "immediately fill it with computed values\" is ONE part, not two: filling has no "
           "independent value without the creation, and both would need to re-run together on any "
           "failure regardless. If the whole request is already a single coherent step, say so by "
           "returning exactly one part covering all of it.\n\n"
           "A LATER part's script can assume every EARLIER part already ran successfully in this "
           "same conversation — whatever it created (a table, a compiled function, a graph, ...) "
           "already exists live — but must look it up BY NAME (table(name), graphWindow(name), "
           "re-opening a compiled function by name, etc.), never by assuming a variable from an "
           "earlier part's script is still bound; only printed output carries forward between "
           "separate script runs, never the Python objects themselves (the same rule that already "
           "governs EXPLORE rounds).\n\n"
           "Decide and FIX every object name (table, graph, compiled function, ...) HERE, at this "
           "decomposition step — never leave one to be invented ad hoc when a part's script is "
           "actually generated later. Name it explicitly in the description of the part that "
           "CREATES it (e.g. \"create table named '<PlaceholderTable>' with...\", \"plot results "
           "into a graph named '<PlaceholderGraph>'\"), and repeat that EXACT same name in every "
           "LATER part's description that reads or references it (e.g. \"...add a legend to the "
           "'<PlaceholderGraph>' graph\", \"...read the last value from table "
           "'<PlaceholderTable>'\"). This applies to every kind of named object a later part must "
           "look up, not just a compiled function name — a name decided independently and "
           "separately by each part's own script is exactly how mismatches happen (one part "
           "creating a graph called \"FitResults\", a later part guessing a different name for "
           "it). The names in THIS paragraph and the example line below are placeholders "
           "illustrating the FORMAT only — every actual name you write must come from the "
           "REQUEST below (or be one you invent because the request left it unnamed), never "
           "copied from this instruction text or from any example in a loaded doc file. Confirmed "
           "live: asked for a table explicitly named 'GaussPeak', a decomposition instead invented "
           "an unrelated table named 'SphereSANS' — the exact placeholder-style name this "
           "instruction text used to use in this very spot before it was made an obvious "
           "placeholder — while also completely ignoring the required PART/DOCS format below in "
           "favor of freeform \"Part 1: ...\" prose.\n\n"
           "A part's DOCS list must include every topic its own script actually calls into — not "
           "just the topic matching its primary/described action. A part whose job is graph "
           "styling but which must ALSO read back a value produced by a different topic's object "
           "(e.g. \"add a legend showing the fitted radius\" needs Fittable's f.results(), not just "
           "Graph's styling calls) still makes a real API call into that other topic, and needs "
           "that topic's docs too, or that call is pure guesswork. When in doubt, include the "
           "topic that PRODUCED a value a later part must read, not only the topic that CONSUMES "
           "or displays it.\n\n"
           "Answer in EXACTLY this format, one PART per line, nothing else — no numbering prose, "
           "no markdown, no explanation outside this format:\n"
           "PART: <short, self-contained, imperative description of exactly what this part must "
           "accomplish> | DOCS: <comma-separated topic names from the index below this part's "
           "script will need, or \"none\">\n\n"
           "Example line demonstrating the format ONLY — invent your own description/topics from "
           "the ACTUAL request below, never reuse this scenario or name:\n"
           "PART: create table named '<PlaceholderTable>' with 50 rows of x,y data | DOCS: table\n\n"
           "Available topics:\n" +
           docTopicIndex() + "\nRequest:\n" + requestText;
}

QString QtiSasAiContent::docSelectionPreflightPrompt(const QString &requestText) const
{
    return "Which documentation topics do you need to fulfill the request below? Answer with "
           "ONLY a comma-separated list of topic names from the index below (say \"all\" if "
           "genuinely unsure which apply, or \"none\" if this is a question/discussion about "
           "something already done rather than a new coding/analysis task — none of the API "
           "reference topics apply to a plain question) — no code, no explanation, nothing "
           "else.\n\n"
           "Available topics:\n" +
           docTopicIndex() + "\nRequest:\n" + requestText;
}

QString QtiSasAiContent::continuationCheckPreflightPrompt(const QString &newRequestText,
                                                          const QString &priorRequestText) const
{
    return "The assistant has been working on this task in the current conversation:\n\n" + priorRequestText +
           "\n\nA new message just arrived:\n\n" + newRequestText +
           "\n\nDoes the new message CONTINUE the task above (references or builds on the same "
           "data/objects/goal, e.g. \"now add a legend to that\", \"also plot the residuals\"), or "
           "is it a NEW, independent task unrelated to it (a different subject, different data, "
           "no reference back to the task above)? If you cannot tell confidently either way, say "
           "so honestly rather than guessing.\n\n"
           "Answer with EXACTLY one word: CONTINUE, NEW, or UNCLEAR. No explanation, no "
           "punctuation, nothing else.";
}

QString QtiSasAiContent::requestSuffix(bool autoRun) const
{
    if (autoRun)
        // Confirmed live: autoRun stays on across a whole conversation (it's per-call from the
        // shift-key state, not reset by message content), so a plain question asked mid-conversation
        // still gets this suffix — with no way to answer in free text, a model can fall back to a
        // degenerate but syntactically-valid no-op (e.g. a bare "None" statement) that compiles,
        // passes self-review (nothing there flags a no-op as non-compliant), and runs silently with
        // no output at all. Explicitly redirecting to scriptPrint() gives it a compliant way to
        // actually answer instead.
        return "\n\n(Return ONLY executable Python code — no explanations, no markdown "
               "fences, no free text outside code. If this message is a question or "
               "discussion rather than a request to write/run a script, answer it via "
               "one or more scriptPrint(\"...\") calls instead of returning nothing "
               "meaningful.)";
    return "\n\n(Answer in plain text if this is a question. "
           "Return raw Python code only if code was explicitly requested.)";
}

QString QtiSasAiContent::apiCallingConventionHint() const
{
    return "All QtiSAS API functions (newTable, plot, note, Dan, Fittable, etc.) are global — "
           "call them directly without any prefix, namespace, or module qualifier — EXCEPT the "
           "qti.app.graph*() preference getters (qti.app.graphTitleOn(), "
           "qti.app.graphMajTicksStyle(), qti.app.graphCanvasColor(), etc.), which require the "
           "qti.app. prefix even though nearly everything else doesn't; calling one bare raises "
           "NameError. ";
}

QString QtiSasAiContent::assistantIdentityIntro() const
{
    return "You are an assistant for QtiSAS, a scientific data analysis software. ";
}

QString QtiSasAiContent::existenceVerificationFunctionExamples() const
{
    return "existTable()/graphWindow()/fitFunctions()/colNames()/datasets()";
}

QString QtiSasAiContent::noOpPrintFunctionName() const
{
    return "scriptPrint()";
}

QString QtiSasAiContent::creationCallExamples() const
{
    return "plot(), newTable(), newGraph(), Compiler.compile()";
}

QString QtiSasAiContent::lookupByNameExamples() const
{
    return "table(name), graphWindow(name), re-opening a compiled function by name";
}

// plot3D(...)'s creation forms (formula/table/matrix/parametric surface) always pass more than one
// top-level argument; the single-argument plot3D("name") form is a LOOKUP, not a creation. A plain
// regex can't reliably tell these apart: a formula argument itself contains matched parens (e.g.
// "sin(x)*cos(y)"), which trips up any pattern that stops scanning at the first ')' it sees. This
// walks each plot3D(...) call's own argument list character-by-character — skipping the contents
// of quoted strings entirely, so a formula's internal parens/commas never affect the count — to
// find a genuine top-level comma.
static bool plot3DCallHasMultipleArgs(const QString &code)
{
    static const QRegularExpression plot3dStartRe(R"(\bplot3D\s*\()");
    QRegularExpressionMatchIterator it = plot3dStartRe.globalMatch(code);
    while (it.hasNext())
    {
        int i = static_cast<int>(it.next().capturedEnd()); // just past the opening '('
        int depth = 1;
        bool inQuote = false;
        QChar quoteChar;
        for (; i < code.size() && depth > 0; ++i)
        {
            const QChar c = code.at(i);
            if (inQuote)
            {
                if (c == quoteChar)
                    inQuote = false;
            }
            else if (c == '\'' || c == '"')
            {
                inQuote = true;
                quoteChar = c;
            }
            else if (c == '(')
                ++depth;
            else if (c == ')')
                --depth;
            else if (c == ',' && depth == 1)
                return true;
        }
    }
    return false;
}

QString QtiSasAiContent::splitPartScopeViolation(const QString &code, const QString &partDescription,
                                                 const QString &earlierPartsCode) const
{
    // Direction 1 — OVERREACH: a part whose own description names a different job entirely (e.g.
    // "compare fit quality and select the best model") instead builds the ENTIRE plot — a later,
    // separate part's explicit responsibility. Self-review is already told about this exact
    // failure shape (see AiAssistant's splitScopePrompt "Equally a violation" paragraph, which
    // inlines this very example) but doesn't reliably catch it — it recurred live even with that
    // matching example already in the prompt. A part that never actually needs to create a graph
    // has no legitimate reason to call newGraph()/plot()/plotResults() at all, so this is checked
    // mechanically instead of trusting the model's own judgment a second time. Deliberately narrow
    // (graph-creation only, not every possible cross-topic action) to avoid false positives on a
    // part that legitimately needs to READ another topic's data — already covered by the separate
    // "DOCS list must include every topic it calls into" rule, which is about reading, not
    // creating. Covers all three of this host's graph-creating entry points: the low-level
    // newGraph(), the global plot() convenience function, and Fittable's own plotResults() —
    // confirmed live: a "fit ... and save results" part (no plotting in its own description)
    // called plotResults() anyway, creating an extra, unstyled graph window alongside the one a
    // later, dedicated plotting part correctly built to the request's actual styling spec.
    // Confirmed live, a false positive on the OTHER side: a legitimately-scoped part described as
    // "create 3D surface of sin(x)*cos(y) ..., name it 'Surface3D'" — a perfectly clear 3D-plotting
    // job — got blocked anyway because its description said "surface" rather than literally "plot"
    // or "graph". Widened to also recognize "surface"/"3d"/"mesh" as plotting-intent keywords.
    static const QRegularExpression newGraphRe(R"(\b(newGraph|plotResults|newPlot3D|plot3D|plot)\s*\()");
    if (newGraphRe.match(code).hasMatch())
    {
        static const QRegularExpression plotKeywordRe(R"(\b(plot|graph|chart|visuali[sz]e|surface|3d|mesh)\w*\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
        if (!plotKeywordRe.match(partDescription).hasMatch())
            return "This part's own job (\"" + partDescription +
                   "\") does not mention plotting/graphing, but the script calls "
                   "newGraph()/plot()/plotResults() — that is a DIFFERENT, separate part's "
                   "explicit responsibility (see the full plan restated below). Do not build or "
                   "show a graph here; perform ONLY this part's own described action, and trust a "
                   "later part to handle plotting.";
    }

    // Direction 2 — UNDERREACH (mirror image): a part whose own description explicitly requires
    // PERFORMING a fit (e.g. "fit both models... with weighted errors... report best model name")
    // instead only READS pre-existing fitCurve-<model> results, as if some earlier part had
    // already fit them — even when no earlier part in the plan ever calls configure()/fit() at
    // all. Confirmed live: the part printed an honest "fit-curve table is missing — did both fits
    // actually run?" and exited without error, which counted as a successful "part done"
    // (runActionScript()'s only success gate is "no exception raised"). The NEXT part then had to
    // notice via its own EXPLORE round that nothing had actually been fit and perform both fits
    // itself before it could do its own, unrelated job — the final result was only correct
    // because that part happened to self-heal; a part making the same wrong assumption would have
    // silently produced a broken result while every part still reported "done." The "fit
    // both/all/each/the/to <...>" shape targets the VERB usage ("fit the data") without matching
    // a plain NOUN mention ("compare fit quality") — a later, legitimate comparison-only part
    // describing itself that way must not be flagged just for mentioning the word "fit".
    // The leading \b requires a boundary immediately before "fit" — "refit both models" has none
    // (contiguous letters), so an explicit optional "re" prefix is matched instead of dropping the
    // boundary outright (which would risk false positives on unrelated words ending the same way).
    static const QRegularExpression fitVerbRe(R"(\b(re)?fit(ting)?\s+(both|all|each|the|to)\b)",
                                              QRegularExpression::CaseInsensitiveOption);
    if (fitVerbRe.match(partDescription).hasMatch())
    {
        static const QRegularExpression fitCallRe(R"(\.\s*(configure|fit)\s*\()");
        if (!fitCallRe.match(code).hasMatch())
            return "This part's own job (\"" + partDescription +
                   "\") explicitly requires PERFORMING a fit, but the script never calls "
                   "configure()/fit() anywhere — it only reads pre-existing fitCurve-<model> "
                   "results, as if an earlier part had already fit them. No part in this plan "
                   "does that on this part's behalf. If those results don't already exist live, "
                   "actually call configure() and fit() here for every model this part is "
                   "responsible for — do not report them missing and stop.";
    }

    // Direction 3 — OVERREACH via compile: a part compiles a function whose name isn't mentioned
    // anywhere in its own description — almost always a LATER part's compile job, pulled forward
    // and merged into this one. Confirmed live: self-review took a correctly single-scoped
    // "compile gaussian-peak" draft and ADDED a second setFunctionName(...)+compile() block for
    // "lorentzian-peak" — a separate part's own explicitly distinct job — even though it ran
    // without error and produced a real, correctly-compiled second function. Unlike the
    // graph-overreach check above, this needs no keyword allowlist: setFunctionName() is
    // unambiguously compile-only, so any name it's called with that the part's own description
    // never mentions is a real mismatch.
    // Matches against EITHER the full captured name or its bare form with a leading "ai/"
    // stripped — that prefix is this app's own internal namespacing convention for AI-compiled
    // functions (see fittable-python-api.md Rule 1's "exclude ai/* candidates" discovery pattern),
    // never something a part's own description is obligated to spell out. Confirmed live, severe:
    // a part described only "compile custom fit function named 'GaussPeakModel'" (no "ai/" prefix
    // in the description at all) correctly called setFunctionName("ai/GaussPeakModel"), following
    // the same convention every other successful run in this session used — and got wrongly
    // blocked here for it, forcing the retry to drop the "ai/" prefix and compile under a bare,
    // unnamespaced name instead. That inconsistency (later parts sometimes correctly using the
    // bare name, self-review sometimes "correcting" it back to a stale "ai/"-prefixed leftover
    // from an earlier session) directly caused a real runtime ValueError several parts later, and
    // burned 6+ EXPLORE rounds tracing it back to this false positive.
    static const QRegularExpression setFunctionNameRe(R"(setFunctionName\(\s*["']([^"']+)["']\s*\))");
    QRegularExpressionMatchIterator it = setFunctionNameRe.globalMatch(code);
    while (it.hasNext())
    {
        const QString name = it.next().captured(1);
        const QString bareName = name.startsWith("ai/", Qt::CaseInsensitive) ? name.mid(3) : name;
        if (!partDescription.contains(name, Qt::CaseInsensitive) &&
            !partDescription.contains(bareName, Qt::CaseInsensitive))
            return "This part's own job (\"" + partDescription +
                   "\") never mentions the "
                   "function name \"" +
                   name + "\", but the script calls setFunctionName(\"" + name +
                   "\") — that looks like a DIFFERENT, separate part's compile job pulled into "
                   "this one. Perform ONLY this part's own described action; leave other "
                   "functions for their own part.";
    }

    // Direction 4 — UNDERREACH via compile (mirror image of Direction 2, same failure class): a
    // part whose own description explicitly requires COMPILING a function gives up after repeated
    // wrong-guess retries and settles for a diagnostic probe (e.g. dir(qti.app), fitFunctions())
    // that runs without error and reports nothing about compiling — meaning "part done" without
    // the function ever actually being created. Confirmed live, severe: a part described as
    // "compile custom Gaussian peak fit function named 'ai/GaussPeak'" spiraled through 8
    // wrong-guess retries for the Compiler singleton (Compiler(), Dan.Compiler(), Fittable(),
    // newFittable(), ...), then gave up and printed a dir(qti.app) listing instead — which ran
    // cleanly and was accepted as "part done." The LATER fit part then configured "ai/GaussPeak"
    // successfully anyway — but only because a function of that exact name already existed as an
    // unrelated LEFTOVER from an earlier, different test session; the fit ran against completely
    // the wrong model and both fits it "compared" converged to garbage (chi2/dof ~69,
    // non-converged, identical for two structurally different models) with no error anywhere to
    // signal any of this. The "compile"/"compiling" shape (not "compiled", past tense) targets
    // the VERB usage requiring action now, without matching a part that legitimately only READS
    // an already-compiled function from an earlier part. Same boundary caveat as fitVerbRe above:
    // "recompile the function" has no \b immediately before "compil" (contiguous letters), so an
    // explicit optional "re" prefix is matched rather than dropping the leading boundary outright.
    static const QRegularExpression compileVerbRe(R"(\b(re)?compil(e|ing)\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    if (compileVerbRe.match(partDescription).hasMatch())
    {
        static const QRegularExpression compileCallRe(R"(\.\s*compile\s*\()");
        if (!compileCallRe.match(code).hasMatch())
            return "This part's own job (\"" + partDescription +
                   "\") explicitly requires "
                   "COMPILING a function, but the script never calls compile() anywhere. If "
                   "repeated attempts at the Compiler singleton syntax keep failing, the fix is "
                   "to get that syntax right (c = Compiler, bare, no parentheses — see the "
                   "checklist), not to give up and print diagnostic information instead — that "
                   "would leave the function never created, silently reusing whatever unrelated "
                   "function of the same name may already exist from an earlier session.";
    }

    // Direction 5 — mismatched/wrong target name: mirrors Direction 3, but for window-creating
    // calls instead of setFunctionName(). splitPlanPreflightPrompt() already mandates that any
    // part which creates/plots into a graph states that graph's exact name in quotes in its OWN
    // description — the same mandate Direction 3 already relies on for compiled function names —
    // so a part whose code creates a window under a name never mentioned in its own description is
    // either targeting the wrong (differently-named) window or inventing a name nobody asked for.
    // Confirmed live: a part described "...plot data + best curve + residuals into graph
    // 'GaussPeakFit'" instead called f.plotResults("GaussianFit", "") and
    // f.plotResults("LorentzianFit", "") — two generic, unstyled, wrongly-named windows,
    // "GaussPeakFit" itself never touched — so the compound request's actual final,
    // specifically-styled deliverable was never created, with no error anywhere to signal it.
    // newPlot3D("name") follows the same quoted-name-as-first-arg convention as newGraph()/
    // plotResults() and is safe to add here. plot3D(...) is deliberately excluded: its first
    // quoted argument is either an existing window's name to LOOK UP (not create) or a formula
    // string ("sin(x)*cos(y)") — including it would misread a formula as an invented window name.
    static const QRegularExpression windowCreateRe(
        R"(\b(?:plotResults|newGraph|newPlot3D|setWindowName)\s*\([^)]*?["']([^"']+)["'])");
    QRegularExpressionMatchIterator windowIt = windowCreateRe.globalMatch(code);
    while (windowIt.hasNext())
    {
        const QString name = windowIt.next().captured(1);
        if (name.isEmpty())
            continue;
        if (partDescription.contains(name, Qt::CaseInsensitive))
            continue;
        // Normalize both sides by stripping non-alphanumeric characters before comparing — a
        // sensible candidate name is often a SANITIZED version of a formula/phrase already in the
        // description (setWindowName() itself only allows letters/digits/underscore/hyphen, so a
        // name containing a raw formula's punctuation is invalid there too), and a literal,
        // unnormalized substring match can never succeed for both constraints at once. Confirmed
        // live, severe: a part described only via its formula ("...plot 3D surface of
        // sin(x)*cos(y) for x,y...", no quoted name at all — the naming-retry ALSO failed to add
        // one) could satisfy the runtime's character rule (e.g. "sinxcosy") or this
        // description-match rule (e.g. "sin(x)*cos(y)" verbatim) but never both — a genuine
        // deadlock that killed the whole request.
        static const QRegularExpression nonAlnumRe(R"([^A-Za-z0-9])");
        const QString normalizedName = QString(name).remove(nonAlnumRe);
        const QString normalizedDescription = QString(partDescription).remove(nonAlnumRe);
        if (!normalizedName.isEmpty() && normalizedDescription.contains(normalizedName, Qt::CaseInsensitive))
            continue; // matches after stripping punctuation — treat as satisfying the naming rule
        // Word/digit-run token match, ORDER-INDEPENDENT — the substring check above requires the
        // name's pieces to appear in the exact same sequence as the description, but natural
        // language often reorders them (e.g. description says "3D surface", a perfectly sensible
        // name is "Surface3D" — "surface3d" is never a substring of "...3dsurface..."). Confirmed
        // live: this exact reversal got a legitimately-scoped "Surface3D" rejected twice in a row,
        // and the retry that followed converged on the WORSE name "Surface" (dropping "3D"
        // entirely) — which then silently created a stray NEW window instead of continuing to
        // update the "Surface3D" window the rest of the conversation had been building on.
        static const QRegularExpression tokenRe(R"([A-Za-z]+|[0-9]+)");
        QSet<QString> nameTokens;
        QRegularExpressionMatchIterator nameTokIt = tokenRe.globalMatch(name);
        while (nameTokIt.hasNext())
            nameTokens.insert(nameTokIt.next().captured(0).toLower());
        QSet<QString> descTokens;
        QRegularExpressionMatchIterator descTokIt = tokenRe.globalMatch(partDescription);
        while (descTokIt.hasNext())
            descTokens.insert(descTokIt.next().captured(0).toLower());
        bool allTokensPresent = !nameTokens.isEmpty();
        for (const QString &token : nameTokens)
        {
            if (!descTokens.contains(token))
            {
                allTokensPresent = false;
                break;
            }
        }
        if (allTokensPresent)
            continue; // every piece of the name traces back to the description, just reordered
        return "This part's own job (\"" + partDescription +
               "\") never mentions the window "
               "name \"" +
               name +
               "\" this script creates/targets (even after stripping punctuation from both "
               "sides) — every part that creates or plots into a graph must use the EXACT name "
               "already decided for it in this part's own description (see the split-plan's own "
               "naming rule), never a different or invented one. If the description itself never "
               "gives a clean, alphanumeric name (e.g. it only embeds a formula like "
               "\"sin(x)*cos(y)\", which setWindowName() will reject as invalid characters "
               "regardless), fall back to a plain descriptive WORD already present in the "
               "description instead (e.g. \"surface\"/\"filled\"/\"mesh\"/\"legend\"/\"axis\"/"
               "\"labels\" — whichever ones actually appear) rather than trying to encode the "
               "formula into the name.";
    }

    // Direction 6 — cross-part function-name mismatch: a part configures a fit against a bare
    // (non-"ai/"-prefixed) name that matches — after stripping "ai/" — a function an EARLIER part
    // in THIS SAME conversation already compiled via Compiler.setFunctionName("ai/<name>"). Per
    // fittable-python-api.md Rule 1's own stated exception ("if the script itself compiled the
    // function via Compiler, the name is already known — no discovery needed"), this part should
    // have used that exact confirmed name directly; instead its own fitFunctions() discovery (the
    // documented "exclude ai/* candidates" pattern for finding a pre-existing LIBRARY model when
    // the true name is genuinely unknown — inapplicable here, since the name was never unknown)
    // found a same-named-but-unrelated, unprefixed function instead. Confirmed live: a part
    // configured against "GaussPeakModel" moments after an earlier part in the SAME conversation
    // had compiled "ai/GaussPeakModel" — the fit ran, reported a real, plausible-looking chi2/dof,
    // with no error anywhere to signal it was fit against a stale leftover from an unrelated,
    // earlier test session instead of the function this conversation actually just built.
    static const QRegularExpression configureNameRe(R"(\.\s*configure\s*\(\s*["']([^"']+)["'])");
    QRegularExpressionMatchIterator configureIt = configureNameRe.globalMatch(code);
    while (configureIt.hasNext())
    {
        const QString configuredName = configureIt.next().captured(1);
        if (configuredName.startsWith("ai/", Qt::CaseInsensitive))
            continue;
        static const QRegularExpression compiledNameRe(R"(setFunctionName\(\s*["'](ai/[^"']+)["']\s*\))");
        QRegularExpressionMatchIterator compiledIt = compiledNameRe.globalMatch(earlierPartsCode);
        while (compiledIt.hasNext())
        {
            const QString compiledName = compiledIt.next().captured(1); // e.g. "ai/GaussPeakModel"
            if (configuredName.compare(compiledName.mid(3), Qt::CaseInsensitive) == 0)
                return "This part configures against \"" + configuredName +
                       "\", but an earlier "
                       "part in THIS conversation already compiled \"" +
                       compiledName +
                       "\" via Compiler — that exact name is already known (fittable-python-api.md "
                       "Rule 1's own stated exception: no fitFunctions() discovery is needed, or "
                       "correct, once this script itself compiled the function). Use \"" +
                       compiledName + "\" directly instead of \"" + configuredName +
                       "\" — the discovery search found a same-named-but-unrelated, unprefixed "
                       "function instead, likely a stale leftover from an earlier, different "
                       "session.";
        }
    }

    // Direction 7 — hallucinated fitCurve/function name: a part references a "fitCurve-<name>"
    // table (existTable()/table()) whose <name> — after stripping a leading "ai/" the same way
    // Direction 6 does — does not match ANY function this conversation actually compiled via
    // Compiler.setFunctionName() earlier. Confirmed live: after a ValueError from misreading a
    // fitCurve table by hardcoded row index (see retryHintForError()'s matching hint), an
    // error-retry — which doesn't resend docs — invented "fit-ai/GaussPeakFit" (wrong prefix) on
    // one attempt, then a LATER part invented "fitCurve-ai/GaussPeak" and
    // "fitCurve-ai/LorentzianPeak" (both altered from the real compiled names
    // "ai/GaussPeakFit"/"ai/LorentzPeakFit") — three distinct hallucinated variants across one
    // conversation, none of which raised any error (existTable() just silently returned False), so
    // the final plot's fit curve and residuals were never actually added.
    static const QRegularExpression anyCompiledNameRe(R"(setFunctionName\(\s*["']([^"']+)["']\s*\))");
    QStringList compiledNames;
    QRegularExpressionMatchIterator anyCompiledIt = anyCompiledNameRe.globalMatch(earlierPartsCode);
    while (anyCompiledIt.hasNext())
    {
        QString n = anyCompiledIt.next().captured(1);
        if (n.startsWith("ai/", Qt::CaseInsensitive))
            n = n.mid(3);
        compiledNames << n;
    }
    if (!compiledNames.isEmpty())
    {
        static const QRegularExpression fitCurveRefRe(R"((?:table|existTable)\s*\(\s*["']fitCurve-([^"']+)["']\s*\))");
        QRegularExpressionMatchIterator fitCurveIt = fitCurveRefRe.globalMatch(code);
        while (fitCurveIt.hasNext())
        {
            const QString referencedRaw = fitCurveIt.next().captured(1);
            QString referenced = referencedRaw;
            if (referenced.startsWith("ai/", Qt::CaseInsensitive))
                referenced = referenced.mid(3);
            bool matchesAny = false;
            for (const QString &cn : compiledNames)
                if (cn.compare(referenced, Qt::CaseInsensitive) == 0)
                {
                    matchesAny = true;
                    break;
                }
            if (!matchesAny)
                return "This part references \"fitCurve-" + referencedRaw +
                       "\", but no earlier "
                       "part in THIS conversation compiled a function by that name (checked "
                       "against: " +
                       compiledNames.join(", ") +
                       ") — this looks like an invented or misremembered variant of the real "
                       "compiled name. Use the EXACT name a Compiler.setFunctionName(...) call "
                       "earlier in this conversation actually used, never a "
                       "recalled-from-memory approximation.";
        }
    }

    // Direction 8 — unnamed 3D surface, unfindable by a later part: unlike newGraph()/
    // plotResults()/Compiler.setFunctionName(), the creating overloads of plot3D() (formula/
    // table/matrix/parametric surface — recognized via plot3DCallHasMultipleArgs() above,
    // distinguishing them from the single-argument plot3D("name") LOOKUP form) take NO name
    // argument at all. graph3d-python-api.md's own docs name setWindowName(g3, name) as the only
    // way to make such a window findable again via plot3D("name") later. Confirmed live: a part
    // created a surface via plot3D(formula, ...) and never called setWindowName() — a LATER part,
    // needing to add a legend/axis labels to that exact surface, tried plot3D("Plot") (a guessed
    // name), got None, and rebuilt the ENTIRE surface from scratch instead — leaving TWO windows
    // behind (the original, unstyled, orphaned one, plus a full duplicate that got the requested
    // styling).
    if (plot3DCallHasMultipleArgs(code))
    {
        static const QRegularExpression setWindowNameRe(R"(\bsetWindowName\s*\()");
        if (!setWindowNameRe.match(code).hasMatch())
            return "This part creates a 3D surface via plot3D(...) but never calls "
                   "setWindowName(g3, \"name\") afterward — plot3D()'s creating overloads take no "
                   "name argument, so without setWindowName() this window is permanently unfindable "
                   "by name (plot3D(\"name\") lookup) for any later part that needs to reference it, "
                   "e.g. to add a legend or styling. Prefer newPlot3D(\"<exact name from this "
                   "part's own description>\", ...) instead of plot3D(...) — same trailing "
                   "arguments, but it names the window atomically and updates it in place on any "
                   "later re-run, so there is no separate setWindowName() step that can raise "
                   "ValueError: name already exists. If you keep plot3D(...), add "
                   "setWindowName(g3, \"<name>\") right after creating it.";
    }

    // Direction 9 — cross-type name confusion: a name created as a Table via newTable(name,...)
    // is later looked up as a Matrix via matrix(name), or a name created as a Matrix via
    // newMatrix(name,...) is later checked/looked up as a Table via existTable(name)/table(name).
    // QtiSAS's exist*()/lookup functions are strictly type-specific (existTable() only ever
    // confirms a Table — there is no existMatrix()) — confirmed live, twice, complementary shapes:
    // (1) a name created via newTable(...) was looked up via matrix(name), which always returns
    // None for a non-Matrix name, so plot3D(None, ...) produced an unusable window whose
    // setWindowName() then rejected every name tried as "invalid characters" — a misleading error
    // that sent 8 retries chasing the wrong cause before giving up entirely; (2) a name created
    // via newMatrix(...) was later gated behind `if existTable(name):` — always False for a Matrix
    // name — silently skipping the ENTIRE rest of that part's real work (an entire 3D plot/styling
    // block) with no error at all, reported as a successfully completed part.
    static const QRegularExpression newTableRe(R"(\bnewTable\s*\(\s*["']([^"']+)["'])");
    static const QRegularExpression newMatrixRe(R"(\bnewMatrix\s*\(\s*["']([^"']+)["'])");
    QHash<QString, QString> nameType; // name -> "table" | "matrix"
    const QString allEarlierAndCurrent = earlierPartsCode + "\n" + code;
    QRegularExpressionMatchIterator tIt = newTableRe.globalMatch(allEarlierAndCurrent);
    while (tIt.hasNext())
        nameType[tIt.next().captured(1)] = "table";
    QRegularExpressionMatchIterator mIt = newMatrixRe.globalMatch(allEarlierAndCurrent);
    while (mIt.hasNext())
        nameType[mIt.next().captured(1)] = "matrix";

    if (!nameType.isEmpty())
    {
        static const QRegularExpression matrixLookupRe(R"(\bmatrix\s*\(\s*["']([^"']+)["'])");
        QRegularExpressionMatchIterator mlIt = matrixLookupRe.globalMatch(code);
        while (mlIt.hasNext())
        {
            const QString name = mlIt.next().captured(1);
            if (nameType.value(name) == "table")
                return "This part calls matrix(\"" + name + "\"), but \"" + name +
                       "\" was created via newTable(...) — a Table, not a Matrix. matrix(\"" + name +
                       "\") always returns None for a non-Matrix name, silently producing broken "
                       "downstream behavior (e.g. plot3D(None, ...) creates an unusable window). "
                       "Use table(\"" +
                       name + "\") to look up this object instead.";
        }
        static const QRegularExpression tableLookupRe(R"(\b(?:existTable|table)\s*\(\s*["']([^"']+)["'])");
        QRegularExpressionMatchIterator tlIt = tableLookupRe.globalMatch(code);
        while (tlIt.hasNext())
        {
            const QString name = tlIt.next().captured(1);
            if (nameType.value(name) == "matrix")
                return "This part calls existTable(\"" + name + "\")/table(\"" + name + "\"), but \"" + name +
                       "\" was created via newMatrix(...) — a Matrix, not a Table. "
                       "existTable() is strictly type-specific and always returns False for a Matrix "
                       "name (there is no existMatrix()) — any logic gated behind this check silently "
                       "never runs, with no error. Use matrix(\"" +
                       name + "\") is not None to check a Matrix's existence instead.";
        }
    }

    // Direction 10 — setWindowName() reusing an existing object's own name for a NEW window: a
    // part visualizing/plotting an existing table/matrix/note/graph (looked up via matrix(name)/
    // table(name)/note(name)/graphWindow(name)) then calls setWindowName(newVar, name) with that
    // SAME name for the newly created window. Window names are unique across EVERY window type in
    // this host — matrix, graph, Graph3D, table, and note all share ONE namespace — so this always
    // raises ValueError: "name already exists" (the source object is still occupying it). Confirmed
    // live, severe: a part described "...plot a 3D surface of 'SphereMap'..." (the split-plan's own
    // wording reused the SOURCE matrix's name, never giving the new 3D window a distinct one) burned
    // 17 retries cycling between this collision, names with spaces (also invalid), and
    // Direction 5 rejecting every distinct alternative (SphereMap_3D, SphereMap3D, Sphere3D) for not
    // literally appearing in the part's own description — only succeeding by luck on a generic,
    // disconnected name ("Surface") that happened to share a substring with the description's own
    // wording. The fix must not suggest a fabricated suffix (Direction 5 would just reject it too)
    // — steer toward a word or short phrase ALREADY present in partDescription instead.
    static const QRegularExpression existingLookupRe(
        R"re(\b(?:matrix|table|note|graphWindow)\s*\(\s*["']([^"']+)["']\s*\))re");
    QStringList existingNames;
    QRegularExpressionMatchIterator lookupIt = existingLookupRe.globalMatch(code);
    while (lookupIt.hasNext())
    {
        const QString name = lookupIt.next().captured(1);
        if (!existingNames.contains(name))
            existingNames << name;
    }
    if (!existingNames.isEmpty())
    {
        static const QRegularExpression setWindowNameArgRe(R"re(\bsetWindowName\s*\(\s*\w+\s*,\s*["']([^"']+)["'])re");
        QRegularExpressionMatchIterator setIt = setWindowNameArgRe.globalMatch(code);
        while (setIt.hasNext())
        {
            const QString newName = setIt.next().captured(1);
            if (existingNames.contains(newName))
                return "This part looks up an EXISTING object via matrix()/table()/note()/"
                       "graphWindow() named \"" +
                       newName + "\", then calls setWindowName(..., \"" + newName +
                       "\") — reusing that SAME name for the NEW window it creates. Window names "
                       "are unique across EVERY window type (matrix, graph, Graph3D, table, note "
                       "all share one namespace), so this raises ValueError: \"name already "
                       "exists\" — the source object is still using it. Pick a genuinely "
                       "different name for the new window — one that is ALSO a word or short "
                       "phrase already present in this part's own description (\"" +
                       partDescription + "\"), not a suffixed variant of \"" + newName +
                       "\" (e.g. \"_3D\"/\"3D\" appended) — this part's own scope check requires "
                       "the created window's name to literally appear in that description text, "
                       "and an invented suffix won't.";
        }
    }

    // Direction 11 — GraphWindow-vs-Graph3D cross-lookup confusion: a name given to a Graph3D
    // window via setWindowName(g3, name) (g3 assigned from any plot3D(...) call, lookup or
    // creation) is later looked up via graphWindow(name) — which is 2D-GraphWindow-ONLY and always
    // returns None for a Graph3D name (there is no graphWindow()-equivalent for Graph3D; the
    // correct lookup is plot3D(name)). Confirmed live, severe: after setWindowName(g3, "Sincos3D")
    // created a real Graph3D window, a later part's graphWindow("Sincos3D") returned None — read
    // as "the window doesn't exist" instead of "wrong lookup function for this window type" — and
    // burned 8+ retries chasing dead ends (currentGraph3D(), graphNames(), graphs(), a
    // from-scratch numpy/matrix rebuild) before giving up on reuse entirely.
    {
        QHash<QString, bool> isGraph3DVar;
        static const QRegularExpression plot3DAssignRe(R"re(\b(\w+)\s*=\s*plot3D\s*\()re");
        const QString allEarlierAndCurrentG3 = earlierPartsCode + "\n" + code;
        QRegularExpressionMatchIterator pIt = plot3DAssignRe.globalMatch(allEarlierAndCurrentG3);
        while (pIt.hasNext())
            isGraph3DVar[pIt.next().captured(1)] = true;

        QStringList graph3DNames;
        static const QRegularExpression setWindowNameGraph3DRe(
            R"re(\bsetWindowName\s*\(\s*(\w+)\s*,\s*["']([^"']+)["'])re");
        QRegularExpressionMatchIterator swIt = setWindowNameGraph3DRe.globalMatch(allEarlierAndCurrentG3);
        while (swIt.hasNext())
        {
            const QRegularExpressionMatch m = swIt.next();
            if (isGraph3DVar.value(m.captured(1)))
            {
                const QString name = m.captured(2);
                if (!graph3DNames.contains(name))
                    graph3DNames << name;
            }
        }
        static const QRegularExpression newPlot3DNameRe(R"re(\bnewPlot3D\s*\(\s*["']([^"']+)["'])re");
        QRegularExpressionMatchIterator npIt = newPlot3DNameRe.globalMatch(allEarlierAndCurrentG3);
        while (npIt.hasNext())
        {
            const QString name = npIt.next().captured(1);
            if (!graph3DNames.contains(name))
                graph3DNames << name;
        }

        if (!graph3DNames.isEmpty())
        {
            static const QRegularExpression graphWindowLookupRe(R"re(\bgraphWindow\s*\(\s*["']([^"']+)["'])re");
            QRegularExpressionMatchIterator gwIt = graphWindowLookupRe.globalMatch(code);
            while (gwIt.hasNext())
            {
                const QString name = gwIt.next().captured(1);
                if (graph3DNames.contains(name))
                    return "This part calls graphWindow(\"" + name + "\"), but \"" + name +
                           "\" is a Graph3D window (named via setWindowName() after plot3D(...), "
                           "or newPlot3D(...)) — graphWindow() is 2D-GraphWindow-ONLY and always "
                           "returns None for a Graph3D name (there is no graphWindow()-equivalent "
                           "for Graph3D). Use plot3D(\"" +
                           name +
                           "\") to look up this window "
                           "instead.";
            }
        }
    }

    return {};
}

bool QtiSasAiContent::isGiveUpFallbackScript(const QString &script) const
{
    QStringList lines;
    for (const QString &line : script.split('\n'))
    {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#'))
            lines << trimmed;
    }
    if (lines.size() != 1)
        return false;
    static const QRegularExpression scriptPrintRe(R"(^scriptPrint\s*\()");
    return scriptPrintRe.match(lines.first()).hasMatch();
}

bool QtiSasAiContent::isUnverifiedMissingClaim(const QString &script) const
{
    if (!isGiveUpFallbackScript(script))
        return false;
    static const QRegularExpression missingClaimRe(R"(\b(not found|does\s*n[o']?t\s+exist|no longer exist|missing)\b)",
                                                   QRegularExpression::CaseInsensitiveOption);
    if (!missingClaimRe.match(script).hasMatch())
        return false;
    // Any of this host's own live existence-checking calls appearing anywhere in the give-up line
    // means the claim WAS actually verified in this exact script — not exhaustive of every such
    // call in the API, but covers the common ones across every topic (table/matrix/note/graph
    // existence, compiled-function/dataset lookup, column presence).
    static const QRegularExpression verificationCallRe(
        R"(\b(existTable|existNote|graphWindow|matrix|fitFunctions|colNames|datasets)\s*\()");
    return !verificationCallRe.match(script).hasMatch();
}

QString QtiSasAiContent::selfReviewFalseIncompletenessClaim(const QString &preReviewDraft,
                                                            const QString &postReviewCode) const
{
    if (!isGiveUpFallbackScript(postReviewCode))
        return {};
    // Distinct claim shape from isUnverifiedMissingClaim() — that one covers "X doesn't exist";
    // this covers "this part's own job wasn't done/is incomplete", a scope-judgment claim rather
    // than an existence claim.
    static const QRegularExpression incompleteClaimRe(
        R"(\b(is incomplete|does\s*n[o']?t\s+perform|is\s*n[o']?t\s+performed|doesn['’]?t\s+perform|fails?\s+to\s+(perform|fulfill)|not\s+fulfilled|not\s+performed)\b)",
        QRegularExpression::CaseInsensitiveOption);
    if (!incompleteClaimRe.match(postReviewCode).hasMatch())
        return {};
    // Any of these appearing in the PRE-review draft is strong evidence real, substantive work was
    // actually present — not exhaustive of every creation call in the API, but covers the common
    // ones across every topic.
    static const QRegularExpression creationCallRe(
        R"(\b(newTable|newMatrix|newGraph|newNote|newPlot3D|plot3D)\s*\(|\.compile\s*\(|\.fit\s*\()");
    if (!creationCallRe.match(preReviewDraft).hasMatch())
        return {};
    return "Self-review's response claims this part's job is incomplete/not performed, but the "
           "script it was asked to review already contains a real creation/action call — strong "
           "evidence the work WAS actually done. If this claim is about a DIFFERENT part's job "
           "(e.g. plotting, when this part's own described job is only to create something), that "
           "is a scope-judgment error — self-review must judge this part SOLELY against its own "
           "described action, never against a later or different part's responsibility.";
}

QString QtiSasAiContent::compiledCodeSelfShadowBug(const QString &code)
{
    // A Compiler setCode() C++ body redeclares a name by initializing it FROM ITSELF —
    // `double name = name;` — always a copy-paste/aliasing mistake, never legitimate C++: at the
    // point of a local declaration, the name it introduces is already in scope for its own
    // initializer, so the right-hand side reads the NEW, not-yet-initialized local, not whatever
    // that name meant just before this line (typically an existing Fittable-declared parameter of
    // the same name). This compiles cleanly with no error or warning surfaced to the script
    // runner, then produces undefined/garbage behavior at runtime. Confirmed live: a Lorentzian
    // peak model's setCode() had `double gamma = gamma;` (meant to alias the "gamma" parameter
    // under a descriptive local name, following the same pattern used for the other three
    // parameters — center->x0, width->sigma, scale->A — but the chosen alias for this one
    // happened to collide with the parameter's own name), silently disconnecting the compiled
    // function's actual output from its real "gamma" value; the resulting fit produced NaN
    // chi²/dof every single time, and over a dozen retries chased a wrong dataset-naming
    // hypothesis instead of ever suspecting the compiled function itself, since the compile step
    // itself had reported success.
    static const QRegularExpression selfShadowRe(R"(\bdouble\s+(\w+)\s*=\s*\1\s*;)");
    const QRegularExpressionMatch m = selfShadowRe.match(code);
    if (!m.hasMatch())
        return {};
    const QString name = m.captured(1);
    return "The compiled function's C++ code declares `double " + name + " = " + name +
           ";` — a self-referential declaration that reads its own not-yet-initialized value, not "
           "whatever \"" +
           name +
           "\" meant just before this line (e.g. an existing parameter of "
           "the same name). This compiles with no error and produces garbage/undefined behavior at "
           "runtime, completely disconnected from the real parameter. If this was meant to alias a "
           "DIFFERENT-named parameter under this name, fix the right-hand side to that parameter's "
           "actual name; if it was meant to just use the parameter directly, delete this line "
           "entirely and use \"" +
           name +
           "\" as-is everywhere below — it is already a pre-declared "
           "double in scope.";
}

QString QtiSasAiContent::fitCurveNameReconstructionMismatch(const QString &code)
{
    // Confirmed live: a script's existTable("fitCurve-ai/GaussPeakModel") check correctly found
    // the real table and set best_model = "GaussPeakModel" (the BARE name, dropping "ai/"), then
    // reconstructed fc = table("fitCurve-" + best_model) -> "fitCurve-GaussPeakModel" — missing
    // "ai/" — which does not exist; `fc is None` silently ended the entire plotting job with "Fit
    // curve table not found", even though the real fit curve table had already been confirmed to
    // exist moments earlier in the very same script.
    static const QRegularExpression qualifiedCheckRe(R"(fitCurve-ai/)");
    if (!qualifiedCheckRe.match(code).hasMatch())
        return {};
    static const QRegularExpression bareReconstructRe(R"(fitCurve-"\s*\+\s*\w+\s*\))");
    if (!bareReconstructRe.match(code).hasMatch())
        return {};
    return "This script checks/reads a fully-qualified \"fitCurve-ai/<name>\" table somewhere, but "
           "also reconstructs a fitCurve table name via `\"fitCurve-\" + <variable>` elsewhere — "
           "that variable was very likely assigned the BARE name (without the \"ai/\" prefix) from "
           "the earlier qualified check, so the reconstructed lookup will not find the real table "
           "and silently returns None with no exception, ending the script's real work under a "
           "misleading \"not found\" message. Keep the full \"ai/<name>\" string (including the "
           "prefix) in whatever variable is used to rebuild a \"fitCurve-...\" name later, or "
           "reconstruct it explicitly as `\"fitCurve-ai/\" + <bare name>`.";
}

QString QtiSasAiContent::unrequestedAutoexecImmediateRun(const QString &code, const QString &originalRequestText)
{
    static const QRegularExpression autoexecRe(R"(\bsetAutoexec\s*\(\s*(?:True)?\s*\))");
    if (!autoexecRe.match(code).hasMatch())
        return {};
    static const QRegularExpression executeRe(R"(\.execute\s*\(\s*\))");
    if (!executeRe.match(code).hasMatch())
        return {};
    // The request itself may explicitly want both — e.g. "set it up to auto-run AND run it now to
    // confirm it works" — in which case calling execute() here is authorized, not a violation.
    static const QRegularExpression immediateRunAuthorizedRe(
        R"(\b(run|execute|test|apply)\s+(it\s+)?(now|immediately|right away|right now)\b)",
        QRegularExpression::CaseInsensitiveOption);
    if (immediateRunAuthorizedRe.match(originalRequestText).hasMatch())
        return {};
    return "This script calls setAutoexec(True) (configuring the note to run automatically the "
           "NEXT time the project file is opened) AND ALSO calls .execute() to run it immediately, "
           "right now, in this script. The request never asked for it to run now, only to auto-run "
           "later — the documented pattern for this exact task (note-python-api.md's own "
           "\"Auto-execute startup script\" example) ends at setAutoexec(True), with no execute() "
           "call. With a real (non-print-stub) payload, running it now in addition would apply "
           "whatever it does twice: once unrequested, right now, and once for real at the next "
           "project open. Remove the .execute() call unless the current message explicitly asks "
           "for it to also run right now.";
}

QString QtiSasAiContent::selfReviewFunctionNameSubstitution(const QString &preReviewDraft,
                                                            const QString &postReviewCode) const
{
    // Deliberately excludes Compiler.setFunctionName() (a DIFFERENT call, naming a function being
    // compiled, not selecting one) — "setFunction(" requires "(" immediately after, which
    // "setFunctionName(" never has.
    static const QRegularExpression functionNameRe(R"(\b(?:configure|setFunction)\s*\(\s*["']([^"']+)["'])");
    const QRegularExpressionMatch preMatch = functionNameRe.match(preReviewDraft);
    const QRegularExpressionMatch postMatch = functionNameRe.match(postReviewCode);
    if (!preMatch.hasMatch() || !postMatch.hasMatch())
        return {};
    const QString preName = preMatch.captured(1);
    const QString postName = postMatch.captured(1);
    if (preName == postName)
        return {};
    return "Self-review changed the function/model name passed to configure()/setFunction() from "
           "\"" +
           preName + "\" to \"" + postName +
           "\". Self-review runs with no access to this conversation's history (including any "
           "EXPLORE round's fitFunctions() output) and no API docs, so it has no legitimate basis "
           "to judge which function name is correct — unlike checklist items it IS grounded to "
           "check, any change here is more likely a hallucinated substitution than a real fix.";
}

QString QtiSasAiContent::zeroBasedTableIndexMistake(const QString &code)
{
    // First-argument (column) position: Table.setCell/cell/cellData/setColumnRole(col, ...) and
    // Matrix.setCell/cell(row, ...) — column/row index 0 never exists for either class.
    static const QRegularExpression firstArgZeroRe(R"(\.(?:setCell|cell|cellData|setColumnRole)\s*\(\s*0\s*[,)])");
    QRegularExpressionMatch m = firstArgZeroRe.match(code);
    if (m.hasMatch())
        return "This script calls ." + m.captured(0).mid(1, m.captured(0).indexOf('(') - 1) +
               "(0, ...) — column/row index 0 never exists for Table or Matrix; both are strictly "
               "1-based (index 1 is the first column/row). This raises ValueError: \"There's no "
               "column/row 0 in table/matrix <name>!\". Do not convert an already-correct 1-based "
               "index to 0-based — shift every index in this call chain by +1 instead.";
    // Second-argument (row, for Table's col-then-row methods only — setColumnRole's 2nd arg is a
    // role enum, not an index, so it's excluded here) position: Table.setCell/cell/cellData(col, 0, ...).
    static const QRegularExpression secondArgZeroRe(R"(\.(?:setCell|cell|cellData)\s*\([^,()]+,\s*0\s*[,)])");
    m = secondArgZeroRe.match(code);
    if (m.hasMatch())
        return "This script calls ." + m.captured(0).mid(1, m.captured(0).indexOf('(') - 1) +
               "(..., 0, ...) — row index 0 never exists for Table; rows are strictly 1-based (row "
               "1 is the first row). This raises ValueError: \"There's no row 0 in table <name>!\". "
               "Do not convert an already-correct 1-based row index to 0-based (e.g. via `i - 1` "
               "inside a loop that already starts at 1) — use the loop variable directly.";
    return {};
}

QString QtiSasAiContent::staleFitStatisticMistake(const QString &code)
{
    // Confirmed live: a "compare chi2/dof from both fits" part had no memory of an earlier part's
    // fit() calls (each split-mode part runs as an independent script) and had, one step earlier
    // in the SAME conversation, already correctly read both values live via a
    // read_characteristic(fc, "chi^2/DoF") search against the real fitCurve-<name> tables — the
    // documented, correct pattern — but the actual action script threw that away and pasted the
    // printed numbers back in as bare literals instead. Got the right answer this time only because
    // nothing about the underlying data changed between the read and the hardcoding.
    static const QRegularExpression hardcodedChi2Re(R"(\bchi2\w*\s*=\s*-?\d+\.\d+\b)");
    if (!hardcodedChi2Re.match(code).hasMatch())
        return {};
    if (code.contains(QStringLiteral(".fit(")) || code.contains(QStringLiteral("fitCurve")))
        return {};
    return "This script assigns a chi2/dof-named variable a bare hardcoded literal number, with no "
           ".fit() call or fitCurve-<name> table read anywhere in this script to back it up. "
           "fittable-python-api.md is explicit that the fitCurve-<name> table is the ONLY reliable "
           "source for chi2/dof once you're past the part that originally ran the fit — never "
           "re-derive it from memory or an earlier turn's printed console text. Read it via "
           "table(\"fitCurve-<name>\") and search for the \"chi^2/DoF\" label by text (or call "
           "f.fit() again in this exact script) instead of hardcoding the recalled number.";
}

QString QtiSasAiContent::missingLegendCreationMistake(const QString &code)
{
    static const QRegularExpression legendGetterRe(R"(\b(\w+)\s*=\s*\w+\.legend\(\))");
    const QRegularExpressionMatch m = legendGetterRe.match(code);
    if (!m.hasMatch())
        return {};
    // A newLegend() call anywhere means the missing-legend case IS handled somewhere in this
    // script, regardless of the exact control-flow shape used to reach it.
    if (code.contains(QStringLiteral("newLegend")))
        return {};
    const QString varName = m.captured(1);
    const QRegularExpression stylingRe(
        QRegularExpression::escape(varName) +
        R"(\.(setFont|setBackgroundColor|setBackgroundBrush|setPen|setMargin|setPadding|setShape|setText)\s*\()");
    if (!stylingRe.match(code).hasMatch())
        return {};
    return "This script reads an existing legend via `" + varName +
           " = <layer>.legend()` and only styles it if one already exists — legend() is a pure "
           "getter, it never creates one (see graph-python-api.md/fittable-python-api.md's "
           "documented \"if leg is None: leg = g.newLegend()\" pattern). If the layer didn't "
           "already have a legend, this silently produces NO legend at all, with no error, even "
           "when one was explicitly requested. Add the newLegend() fallback before styling " +
           varName + ".";
}

QString QtiSasAiContent::unannouncedModelNameSubstitution(const QString &code, const QString &originalRequestText)
{
    // Confirmed live, twice: a request quoting a model name ("Sphere") near "model"/"function" had
    // that exact name silently dropped in favor of a different fitFunctions() candidate, with
    // nothing printed to reveal the requested name was never used — once producing a catastrophic
    // fit (R² = -190), once quietly landing on "sasviewmodels/sphere" and printing only the
    // resulting chi2/dof. fittable-python-api.md Rule 1 permits the broadened search but requires
    // the substitution to be announced.
    static const QRegularExpression quotedThenModelRe(
        R"re("([A-Za-z][\w/-]*)"\s+(?:[a-zA-Z-]+\s+){0,3}(?:model|function)\b)re");
    static const QRegularExpression modelThenQuotedRe(
        R"re(\b(?:model|function)\s+(?:named\s+|called\s+)?"([A-Za-z][\w/-]*)")re");
    QRegularExpressionMatch m = quotedThenModelRe.match(originalRequestText);
    if (!m.hasMatch())
        m = modelThenQuotedRe.match(originalRequestText);
    if (!m.hasMatch())
        return {};
    const QString requestedName = m.captured(1);

    static const QRegularExpression configureRe(R"(\.(?:configure|setFunction)\s*\(\s*["']([^"']+)["'])");
    QRegularExpressionMatchIterator it = configureRe.globalMatch(code);
    bool sawConfigureCall = false;
    while (it.hasNext())
    {
        sawConfigureCall = true;
        if (it.next().captured(1).compare(requestedName, Qt::CaseInsensitive) == 0)
            return {}; // requested name used verbatim — no substitution happened
    }
    if (!sawConfigureCall)
        return {}; // nothing to check — this script never configures a Fittable model

    static const QRegularExpression announcementRe(
        R"(not found|no exact match|closest match|does not exist exactly|instead of|substitut)",
        QRegularExpression::CaseInsensitiveOption);
    if (announcementRe.match(code).hasMatch())
        return {}; // substitution was explicitly announced somewhere in the script

    return "The request quotes a model/function name (\"" + requestedName +
           "\") but this script's configure()/setFunction() call uses a different name, with no "
           "explicit announcement anywhere in the script that \"" +
           requestedName +
           "\" wasn't matched exactly. fittable-python-api.md Rule 1 permits broadening an unmatched "
           "quoted name via fitFunctions() discovery, but the substitution MUST be printed "
           "explicitly (e.g. \"Model '" +
           requestedName +
           "' not found exactly — using closest match: <name>\") — never silently swapped in. Add "
           "that announcement, or use \"" +
           requestedName + "\" verbatim if it does exist.";
}

QString QtiSasAiContent::hallucinatedLegendMethodMistake(const QString &code)
{
    // Confirmed live: a legend-styling script tried .setPen(), then .setShape(), then .setAlign()
    // on the SAME legend variable, one at a time, each caught only by a live AttributeError — none
    // of the three exist on Legend (SIP PyName for LegendWidget).
    static const QRegularExpression legendVarRe(R"(\b(\w+)\s*=\s*\w+\.(?:newLegend|legend)\s*\()");
    QRegularExpressionMatchIterator varIt = legendVarRe.globalMatch(code);
    QStringList legendVars;
    while (varIt.hasNext())
    {
        const QString name = varIt.next().captured(1);
        if (!legendVars.contains(name))
            legendVars << name;
    }
    for (const QString &varName : legendVars)
    {
        const QRegularExpression badMethodRe(QRegularExpression::escape(varName) +
                                             R"(\.(setPen|setShape|setAlign)\s*\()");
        const QRegularExpressionMatch m = badMethodRe.match(code);
        if (m.hasMatch())
        {
            const QString method = m.captured(1);
            QString realEquivalent;
            if (method == "setPen")
                realEquivalent = "setFramePen(QPen)/setFrameColor(QColor)/setFrameWidth(float)";
            else if (method == "setShape")
                realEquivalent = "setFrameStyle(Frame.FrameStyle.Line.value or .Shadow.value)";
            else
                realEquivalent = "setOriginCoord(x, y) — there is no \"align\" concept at all";
            return "This script calls ." + method +
                   "() on a Legend (LegendWidget) object — this "
                   "method does not exist. graph-python-api.md documents Legend's real API "
                   "(FrameWidget base methods): use " +
                   realEquivalent + " instead.";
        }
    }
    return {};
}

QString QtiSasAiContent::graphWindowLayerMethodMistake(const QString &code)
{
    // Confirmed live: .setXTitle() then .replot() called directly on plot()'s own GraphWindow
    // return value, hitting both AttributeErrors in sequence across two separate retries — despite
    // graph-python-api.md's own dedicated "GraphWindow vs Graph" mistake table covering this trap.
    static const QRegularExpression windowVarRe(R"(\b(\w+)\s*=\s*(?:plot|newGraph)\s*\()");
    QRegularExpressionMatchIterator varIt = windowVarRe.globalMatch(code);
    QStringList windowVars;
    while (varIt.hasNext())
    {
        const QString name = varIt.next().captured(1);
        if (!windowVars.contains(name))
            windowVars << name;
    }
    // showAxis is deliberately excluded: it doesn't exist on Graph (layer) either (see
    // graphChecklistRules()'s own "showAxis() does NOT exist" bullet) — it's not a GraphWindow-vs-
    // Graph split at all, and suggesting `layer.showAxis()` as the fix would just be a different
    // wrong-method guess.
    static const QStringList layerOnlyMethods = {"replot",    "grid",      "showGrid",     "canvasGeometry",
                                                 "setXTitle", "setYTitle", "setAxisTitle", "setScale"};
    for (const QString &varName : windowVars)
    {
        // Reassigned to its own active layer/a specific layer somewhere — treat as resolved, this
        // variable no longer refers to the GraphWindow from that point on.
        const QRegularExpression reassignedRe(QRegularExpression::escape(varName) + R"(\s*=\s*)" +
                                              QRegularExpression::escape(varName) + R"(\.(?:activeLayer|layer)\s*\()");
        if (reassignedRe.match(code).hasMatch())
            continue;
        for (const QString &method : layerOnlyMethods)
        {
            const QRegularExpression badMethodRe(QRegularExpression::escape(varName) + R"(\.)" + method + R"(\s*\()");
            if (badMethodRe.match(code).hasMatch())
                return "This script calls ." + method + "() directly on \"" + varName +
                       "\" — the GraphWindow returned by plot()/newGraph(), not a Graph (layer). " + method +
                       "() belongs to Graph only and raises AttributeError on GraphWindow "
                       "(see graph-python-api.md's \"GraphWindow vs Graph\" mistake table). Call "
                       "`layer = " +
                       varName + ".activeLayer()` (or `.layer(n)`) first, then call " + method + "() on layer.";
        }
    }
    return {};
}

QString QtiSasAiContent::conditionalMaximizeWindowMistake(const QString &code)
{
    // Confirmed live (latent, not yet manifested): a "reuse or create" stacked-panel setup called
    // maximizeWindow() only inside the "if gw is None:" branch, while arrangeLayers()/
    // setCanvasGeometry() ran unconditionally right after — fine on first creation (this run), but
    // on a re-run against an already-existing graph of that name, maximizeWindow() never fires while
    // the layout calls still do, violating graph-python-api.md's documented step-0-before-
    // arrangeLayers ordering.
    if (!code.contains(QStringLiteral("arrangeLayers(")) && !code.contains(QStringLiteral("setCanvasGeometry(")))
        return {}; // not a stacked-panel recipe script — nothing to check

    static const QRegularExpression reuseBlockRe(
        R"((\w+)\s*=\s*graphWindow\([^)]*\)\s*\n\s*if\s+\1\s+is\s+None\s*:\s*\n((?:[ \t]+\S.*\n?)+))");
    const QRegularExpressionMatch m = reuseBlockRe.match(code);
    if (!m.hasMatch())
        return {}; // no "reuse existing graph" pattern present

    const QString varName = m.captured(1);
    const QString ifBlock = m.captured(2);
    const int totalCount = static_cast<int>(code.count(QStringLiteral("maximizeWindow(")));
    const int inBlockCount = static_cast<int>(ifBlock.count(QStringLiteral("maximizeWindow(")));
    if (inBlockCount == 0 || totalCount > inBlockCount)
        return {}; // not gated behind this branch, or also called unconditionally elsewhere

    return "This script calls maximizeWindow(" + varName + ") only inside the \"if " + varName +
           " is None:\" branch (i.e., only when the graph is freshly created), but also calls "
           "arrangeLayers()/setCanvasGeometry() later, unconditionally. graph-python-api.md's "
           "stacked-panel recipe requires maximizeWindow() to run BEFORE arrangeLayers()/"
           "setCanvasGeometry() every time, not just on first creation — \"canvas sizes wrong — "
           "geometry computed before resize\" if skipped. On a re-run against an already-existing \"" +
           varName +
           "\"-named graph, maximizeWindow() would never fire while the layout calls "
           "still do. Call maximizeWindow(" +
           varName + ") unconditionally, right after the if/else that gets or creates it.";
}

QString QtiSasAiContent::existingEntityVerificationPrompt(const QStringList &names) const
{
    QStringList quoted;
    for (const QString &n : names)
        quoted << "\"" + n + "\"";
    return "Before doing anything else, verify whether each of the following name(s) — explicitly "
           "described in the request below as already EXISTING — actually exist: " +
           quoted.join(", ") +
           ".\n\nWrite ONE short, read-only Python script (no markdown fences, no explanation) "
           "that checks each name against every plausible kind it could be — existTable(name) is "
           "not None, existMatrix(name) is not None, note(name) is not None, graphWindow(name) is "
           "not None, name in Compiler.functionNames(\"ALL\"), name in Fittable.fitFunctions(\"*\") "
           "— and for each name prints EXACTLY one line: scriptPrint(\"EXISTS: \" + name) if it "
           "matches ANY of those, or scriptPrint(\"MISSING: \" + name) if it matches NONE. Do not "
           "create, modify, delete, fit, or plot anything — this is a pure existence check, "
           "nothing else. Do not attempt any discovery/fuzzy search/broadened matching (e.g. "
           "fitFunctions(\"*partial*\") guessing) — check the EXACT name given, nothing else.\n\n";
}

QString QtiSasAiContent::windowNameMethodCallMistake(const QString &code)
{
    // Confirmed live, history-triggered: g3.setWindowName("SphereMap") — setWindowName() is
    // ALWAYS a global free function, never a method on any window object, regardless of type.
    static const QRegularExpression methodCallRe(R"re(\b(\w+)\.setWindowName\s*\(\s*(["'][^"']*["'])\s*\))re");
    const QRegularExpressionMatch m = methodCallRe.match(code);
    if (!m.hasMatch())
        return {};
    const QString varName = m.captured(1);
    const QString nameArg = m.captured(2);
    return "This script calls " + varName + ".setWindowName(" + nameArg +
           ") — setWindowName() is ALWAYS a global free function, never a method on any window "
           "object (graph, matrix, Graph3D, ...); this raises AttributeError regardless of the "
           "object's actual type. Replace it with setWindowName(" +
           varName + ", " + nameArg + ") instead — same two arguments, called as a plain function, not a method.";
}

QString QtiSasAiContent::nonexistentExistMatrixMistake(const QString &code)
{
    // Confirmed live, severe: a part guessed existMatrix("SphereMap") by analogy with
    // existTable() — Matrix has no exist*() helper at all. Raised NameError, exhausting the rest
    // of a 15-retry budget before the whole request gave up.
    static const QRegularExpression existMatrixRe(R"(\bexistMatrix\s*\()");
    if (!existMatrixRe.match(code).hasMatch())
        return {};
    return "This script calls existMatrix(...) — this function does NOT exist. Matrix has no "
           "exist*() helper at all, unlike Table's existTable(). Use matrix(name) is not None to "
           "check a Matrix's existence instead.";
}

QString QtiSasAiContent::selfReviewLookupToCreationRegression(const QString &preReviewDraft,
                                                              const QString &postReviewCode) const
{
    // Confirmed live: a correct `m = matrix("SphereMap")` lookup (grounded in real cross-turn
    // history the draft-generation call could see but self-review cannot) was rewritten into
    // `m = newMatrix("SphereMap", -3.14159, 3.14159, 50, ...)` — an invented, nonexistent
    // constructor signature — raising TypeError on the very next run.
    static const QHash<QString, QString> creationForLookup = {
        {"matrix", "newMatrix"}, {"table", "newTable"}, {"note", "newNote"}, {"graphWindow", "newGraph"}};
    static const QRegularExpression lookupRe(
        R"re(\b\w+\s*=\s*(matrix|table|note|graphWindow)\s*\(\s*["']([^"']+)["']\s*\))re");
    QRegularExpressionMatchIterator it = lookupRe.globalMatch(preReviewDraft);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString lookupFn = m.captured(1);
        const QString name = m.captured(2);
        const QString creationFn = creationForLookup.value(lookupFn);
        const QRegularExpression creationRe(QRegularExpression::escape(creationFn) + R"re(\s*\(\s*["'])re" +
                                            QRegularExpression::escape(name) + R"re(["'])re");
        // Require the creation call to be genuinely NEW in postReviewCode — absent from
        // preReviewDraft too — not just present alongside the lookup. Confirmed live (false
        // positive): the legitimate "look up or create" idiom (`n = note(name); if n is None:
        // n = newNote(name)`) has BOTH forms in the SAME, already-correct draft; without this
        // guard, this check fired on ANY self-review edit to a script using that idiom at all,
        // even a genuinely unrelated, correct improvement — reverting it for no real reason.
        if (creationRe.match(postReviewCode).hasMatch() && !creationRe.match(preReviewDraft).hasMatch())
            return "The script you were reviewing correctly looked up an EXISTING object via " + lookupFn + "(\"" +
                   name + "\") — your own edit replaced that with " + creationFn + "(\"" + name +
                   "\", ...), recreating it from scratch instead. You have no docs or history to "
                   "judge whether recreating is correct, or what constructor arguments/signature "
                   "to use — confirmed live, this guessed a signature that doesn't exist and "
                   "raised TypeError. If the object already exists (this part didn't create it — "
                   "an earlier turn/part did), keep the original lookup form (" +
                   lookupFn + "(\"" + name + "\")) instead of recreating it.";
    }
    return {};
}

QString QtiSasAiContent::qti3DImportMistake(const QString &code)
{
    // Confirmed live, repeated with a DIFFERENT wrong module path each time (PyQt6.Qt3DExtras,
    // then qtidas/qtiplot3d) — Qti3D is ALWAYS global, no import needed. `from qti import Qti3D`
    // is the one exception: "qti" is QtiSAS's real extension module (see PyInit_qti in
    // PythonScripting.cpp) and Qti3D genuinely lives there, so that specific form actually works —
    // confirmed live when a script that survived this check unblocked ran past that line and only
    // failed later, on an unrelated setWindowName collision. `import Qti3D`/`import qti.Qti3D`
    // (Qti3D is not a submodule) remain wrong in every form, as does `from qti import Qti3D`
    // guessed with any OTHER module path.
    static const QRegularExpression fromQtiImportRe(R"(^[ \t]*from\s+qti\s+import\b[^\n]*\bQti3D\b)",
                                                    QRegularExpression::MultilineOption);
    static const QRegularExpression badFromImportRe(R"(^[ \t]*from\s+(?!qti\s+import\b)[^\n]*\bQti3D\b)",
                                                    QRegularExpression::MultilineOption);
    static const QRegularExpression bareImportRe(R"(^[ \t]*import\b[^\n]*\bQti3D\b)",
                                                 QRegularExpression::MultilineOption);
    if (fromQtiImportRe.match(code).hasMatch())
        return {};
    if (!badFromImportRe.match(code).hasMatch() && !bareImportRe.match(code).hasMatch())
        return {};
    return "This script imports Qti3D from the wrong place — Qti3D is ALWAYS global (like "
           "Graph.CurveType/Table.PlotDesignation elsewhere in this API), no import needed at "
           "all. The only import that would even work is `from qti import Qti3D` (QtiSAS's real "
           "extension module) — `import Qti3D`, `import qti.Qti3D` (not a submodule), and every "
           "other guessed module path raise ImportError/ModuleNotFoundError. Simplest: delete the "
           "import line entirely and use Qti3D.<CONSTANT> directly — do not also remove whatever "
           "line(s) use it.";
}

QString QtiSasAiContent::setAutoexecOnScriptEditMistake(const QString &code)
{
    // Confirmed live: n.currentEditor().setAutoexec(True) raised AttributeError — setAutoexec()
    // is a Note-level method, ScriptEdit has no autoexec-related methods at all. The next retry
    // guessed a second, entirely nonexistent method (setAutoRun) on the same wrong object instead
    // of moving setAutoexec() to the Note itself.
    static const QRegularExpression wrongObjectRe(
        R"re(\.(?:currentEditor\s*\(\s*\)|tab\s*\([^()]*\)|editorAt\s*\([^()]*\))\s*\.\s*setAutoexec\s*\()re");
    if (!wrongObjectRe.match(code).hasMatch())
        return {};
    return "This script calls .setAutoexec(...) on a ScriptEdit object (the result of "
           ".currentEditor()/.tab(...)/.editorAt(...)) — setAutoexec() is a Note-level method, "
           "ScriptEdit has no autoexec-related methods at all; this raises AttributeError. Call "
           "setAutoexec() directly on the Note object instead (e.g. `n.setAutoexec(True)`), not on "
           "any editor/tab object.";
}

QString QtiSasAiContent::plotStyleStringArgumentMistake(const QString &code)
{
    // Confirmed live: self-review turned a correct setPlotStyle(Qti3D.FILLEDMESH) into
    // setPlotStyle("Filled Mesh") — these setters take an int/Qti3D constant only, never a string,
    // and raise TypeError: setPlotStyle(self, style: int): argument 1 has unexpected type 'str'.
    static const QRegularExpression stringStyleRe(R"re(\b(?:setPlotStyle|customPlotStyle)\s*\(\s*["'])re");
    if (!stringStyleRe.match(code).hasMatch())
        return {};
    return "This script passes a quoted string to setPlotStyle()/customPlotStyle() — these always "
           "take an int/Qti3D constant (e.g. Qti3D.FILLEDMESH), never a string name; this raises "
           "TypeError: argument 1 has unexpected type 'str'. Replace the string with the matching "
           "Qti3D constant (NOPLOT=0, WIREFRAME=1, HIDDENLINE=2, FILLED=3, FILLEDMESH=4, POINTS=5, "
           "USER=6) — do not guess a bare int, use the named Qti3D.<CONSTANT> form.";
}

QString QtiSasAiContent::selfReviewEnumConstantRegression(const QString &preReviewDraft,
                                                          const QString &postReviewCode) const
{
    // Confirmed live: self-review replaced a correct setPlotStyle(Qti3D.FILLEDMESH) with
    // setPlotStyle("Filled Mesh") (a TypeError), and the runtime-error retry that followed "fixed"
    // the crash by guessing a bare int (2, i.e. HIDDENLINE) instead of recalling the correct
    // constant (FILLEDMESH == 4) — the script then ran with no further error but silently
    // rendered the WRONG style, contradicting the request's explicit "filled mesh style". Self-
    // review has no basis to replace an already-correct enum-namespace constant with anything
    // else in the same call.
    static const QStringList enumNamespaces = {"Qti3D", "Graph.CurveType", "Table.PlotDesignation", "Graph3D.PlotType",
                                               "Graph3D.AxisNumericFormat"};
    for (const QString &ns : enumNamespaces)
    {
        const QRegularExpression enumCallRe(R"re(\b(\w+)\s*\(\s*)re" + QRegularExpression::escape(ns) +
                                            R"re(\.[A-Za-z_][A-Za-z0-9_]*\s*[,)])re");
        QRegularExpressionMatchIterator preIt = enumCallRe.globalMatch(preReviewDraft);
        while (preIt.hasNext())
        {
            const QString methodName = preIt.next().captured(1);
            const QRegularExpression sameConstCallRe(R"re(\b)re" + QRegularExpression::escape(methodName) +
                                                     R"re(\s*\()re" + QRegularExpression::escape(ns) + R"re(\.)re");
            if (sameConstCallRe.match(postReviewCode).hasMatch())
                continue; // still correct
            const QRegularExpression anyCallRe(R"re(\b)re" + QRegularExpression::escape(methodName) + R"re(\s*\()re");
            if (!anyCallRe.match(postReviewCode).hasMatch())
                continue; // call removed entirely — a different, unrelated edit
            return "The script you were reviewing correctly called " + methodName + "(" + ns +
                   ".<CONSTANT>) — your own edit replaced that constant with something else in the "
                   "same call. " +
                   ns +
                   " constants are ALWAYS the correct argument here (never a "
                   "string, never a hand-picked int) — restore the original " +
                   ns +
                   ".<CONSTANT> reference exactly as it was, do not substitute a guessed literal "
                   "value.";
        }
    }
    return {};
}

QString QtiSasAiContent::guessedNameCandidateListMistake(const QString &code)
{
    // Confirmed live: instead of using the concrete window name explicitly established earlier in
    // this exact conversation ("into a graph named 'Surface3D'"), a part guessed
    // `for name in ["Surface3D", "Graph3D_1", "Graph3D"]:` and used whichever one happened to
    // already exist — it worked only because the real name was listed first and no other
    // candidate matched; the moment more than one guessed name matches a real object (e.g. an
    // orphaned window left over from an earlier failed attempt), this silently targets the WRONG
    // object with no error at all. Deliberately narrow to the "keep only the first match" shape
    // (a `break` right after the found-check, or indexing/popping element 0 of an accumulated
    // list after the loop) to avoid flagging a script that legitimately processes EVERY item in a
    // list of names it was genuinely given, which has no such first-match-only signal.
    static const QRegularExpression forLoopRe(R"re(\bfor\s+(\w+)\s+in\s+\[([^\[\]]+)\]\s*:)re");
    static const QRegularExpression stringLiteralRe(R"(["'][^"']*["'])");
    static const QStringList lookupFns = {"plot3D", "table", "matrix", "graphWindow", "note"};
    QRegularExpressionMatchIterator it = forLoopRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString var = m.captured(1);
        const QString listContents = m.captured(2);
        const int loopEnd = static_cast<int>(m.capturedEnd());
        int stringCount = 0;
        QRegularExpressionMatchIterator sIt = stringLiteralRe.globalMatch(listContents);
        while (sIt.hasNext())
        {
            sIt.next();
            ++stringCount;
        }
        if (stringCount < 2)
            continue;
        for (const QString &fn : lookupFns)
        {
            const QRegularExpression lookupCallRe(R"re(\b)re" + QRegularExpression::escape(fn) + R"re(\s*\(\s*)re" +
                                                  QRegularExpression::escape(var) + R"re(\s*\))re");
            if (!lookupCallRe.match(code).hasMatch())
                continue;
            const QString trailing = code.mid(loopEnd);
            static const QRegularExpression firstMatchOnlyRe(
                R"(\bbreak\b|\[\s*0\s*\]|\.pop\s*\(\s*0\s*\)|\bnext\s*\()");
            if (!firstMatchOnlyRe.match(trailing).hasMatch())
                continue;
            return "This script iterates over a hardcoded list of guessed candidate names (" + listContents.trimmed() +
                   ") and calls " + fn + "(" + var +
                   ") to find whichever one already exists, keeping only the FIRST match — this "
                   "silently targets the WRONG object the moment more than one candidate matches "
                   "a real object (e.g. an orphaned window left over from an earlier failed "
                   "attempt), with no error at all. Use the EXACT name already established "
                   "earlier in this conversation/request instead of guessing a list of plausible "
                   "ones — if genuinely unknown, enumerate real objects (windows(), tableNames(), "
                   "matrixNames(), ...) instead of guessing.";
        }
    }
    return {};
}

QList<AiKnownMistakeMatch> QtiSasAiContent::detectKnownMistakes(const QString &code,
                                                                const QString &originalRequestText) const
{
    // Dispatches EVERY pattern below unconditionally and collects ALL matches, in the SAME order
    // these were previously checked in as separate AiAssistant.cpp local variables/if-else branches
    // — one dispatch point replacing what used to be 16 separate single-purpose pure virtual
    // methods here. See AiContentProvider::detectKnownMistakes()'s doc comment for the full
    // "collect all, don't stop at first" rationale, and each private helper below for its own
    // "confirmed live" incident writeup.
    QList<AiKnownMistakeMatch> matches;
    QString explanation = compiledCodeSelfShadowBug(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"compiled-function defect detected", explanation};
    explanation = fitCurveNameReconstructionMismatch(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"fitCurve name mismatch detected", explanation};
    explanation = unrequestedAutoexecImmediateRun(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"unrequested immediate autoexec run detected", explanation};
    explanation = zeroBasedTableIndexMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"0-based table/matrix index detected", explanation};
    explanation = staleFitStatisticMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"hardcoded chi2/dof detected", explanation};
    explanation = missingLegendCreationMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"legend created only conditionally", explanation};
    explanation = unannouncedModelNameSubstitution(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"unannounced model name substitution detected", explanation};
    explanation = hallucinatedLegendMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Legend method detected", explanation};
    explanation = graphWindowLayerMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"GraphWindow/Graph layer method confusion detected", explanation};
    explanation = conditionalMaximizeWindowMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"maximizeWindow() conditionally gated behind graph-reuse check", explanation};
    explanation = windowNameMethodCallMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setWindowName() called as a method", explanation};
    explanation = nonexistentExistMatrixMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent existMatrix() detected", explanation};
    explanation = qti3DImportMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Qti3D import detected", explanation};
    explanation = setAutoexecOnScriptEditMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setAutoexec() called on the wrong object", explanation};
    explanation = plotStyleStringArgumentMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"string passed to a plot-style setter", explanation};
    explanation = guessedNameCandidateListMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"guessed a list of candidate names instead of using the known one", explanation};
    return matches;
}

QString QtiSasAiContent::retryHintForError(const QString &errContext) const
{
    // A ValueError converting fitCurve table text to float, where the failing line calls
    // .text(N, M) with LITERAL row/column indices, is the confirmed-live "hardcoded row instead
    // of searching by label" mistake fittable-python-api.md/.pyi explicitly warns against: a
    // fitCurve-<model> table's Characteristics/Parameters row order SHIFTS depending on which
    // options (resolution, polydispersity, ...) are enabled, so a fixed row/col index can land on
    // an unrelated row (e.g. a function-name header) instead of "chi^2/DoF". Retries don't resend
    // docs, so once this happens the model has nothing to fall back on but guessing — confirmed
    // live: the very next retry invented an entirely different, wrong table-naming convention
    // ("fit-<model>" instead of "fitCurve-<model>") rather than re-deriving the already-correct
    // name it had used moments earlier in the same script.
    static const QRegularExpression fixedRowValueErrorRe(
        R"(could not convert string to float[\s\S]*\.text\s*\(\s*\d+\s*,\s*\d+\s*\))");
    if (fixedRowValueErrorRe.match(errContext).hasMatch())
        return "REMINDER (this specific error keeps recurring): a fitCurve-<model> table's "
               "Characteristics/Parameters row order is NOT fixed — it shifts depending on which "
               "options are enabled, so reading a hardcoded row/column index is exactly what "
               "raised this error. Never hardcode a row number — search for the row by its LABEL "
               "instead: `def read_characteristic(fc, label):\n    for row in range(1, "
               "fc.numRows()+1):\n        if fc.text(6, row).strip() == label: return "
               "fc.text(7, row).replace(\"->\", \"\").strip()` then `float(read_characteristic(fc, "
               "\"chi^2/DoF\"))`. The table's own NAME is also not something to guess or "
               "reinvent — it is exactly \"fitCurve-\" + the model name already used in "
               "configure()/fit() earlier in this exact script or conversation, never a "
               "different prefix like \"fit-\".\n\n";

    // A NameError on a bare PyQt class (QFont/QColor/QPen/QBrush/Qt) almost always means the
    // `from PyQt6.QtGui import ...` line was never added — confirmed live to resolve two different
    // ways in the SAME session: once correctly (the retry added the missing import), once by
    // silently DELETING the styling call that needed it instead (losing an explicitly requested
    // "styled legend" down to a bare, unstyled leg.setText() with no font/color at all). Steer
    // toward the correct fix explicitly, since the wrong one still "succeeds" with no exception.
    static const QRegularExpression missingQtImportRe(
        R"(NameError: name '(QFont|QColor|QPen|QBrush|Qt)' is not defined)");
    const QRegularExpressionMatch qtImportMatch = missingQtImportRe.match(errContext);
    if (qtImportMatch.hasMatch())
        return "REMINDER (this specific error keeps recurring): \"" + qtImportMatch.captured(1) +
               "\" is a real, correctly-used class — the only problem is the missing import. Add "
               "`from PyQt6.QtGui import " +
               qtImportMatch.captured(1) +
               "` (or extend an existing `from PyQt6.QtGui import ...` line already in this "
               "script) — do NOT remove the line(s) that use it. Deleting a styling call to dodge "
               "this error silently drops whatever was explicitly requested (e.g. a \"styled "
               "legend\" ends up with no styling at all, just bare text) with no error to reveal "
               "it.\n\n";

    // A ValueError from setWindowName() colliding with an existing name almost always means this
    // script re-ran a plot3D(...)+setWindowName(g3, name) creation that already succeeded on an
    // earlier turn — confirmed live: the retry that followed guessed graphWindow(name) instead (a
    // 2D-GraphWindow-ONLY lookup that always returns None for a Graph3D name), then chased that
    // AttributeError with an unrelated, unnecessary Qti3D import. Worse, plot3D(...)'s own
    // creation call ALREADY SUCCEEDED before setWindowName() raised — so every failed retry through
    // this same wrong pattern leaves behind an orphaned, auto-named Graph3D window; confirmed live,
    // 5 accumulated after one such cascade. newPlot3D(name, ...) checks for the existing window
    // BEFORE creating anything, so it can never hit this error or leak a window.
    static const QRegularExpression windowNameCollisionRe(R"(setWindowName:\s*name\s*'([^']+)'\s*already exists)");
    const QRegularExpressionMatch collisionMatch = windowNameCollisionRe.match(errContext);
    if (collisionMatch.hasMatch())
        return "REMINDER (this specific error keeps recurring): \"" + collisionMatch.captured(1) +
               "\" already exists — this script's plot3D(...)+setWindowName(g3, \"" + collisionMatch.captured(1) +
               "\") pattern already created it on an earlier turn, and plot3D(...) just created "
               "ANOTHER orphaned window before setWindowName() raised. Do NOT switch to "
               "graphWindow(\"" +
               collisionMatch.captured(1) +
               "\") — that is 2D-GraphWindow-ONLY and always returns None for a Graph3D name "
               "(raising AttributeError on the very next line). The correct fix is "
               "newPlot3D(\"" +
               collisionMatch.captured(1) +
               "\", <same source args you already have>) in place of plot3D(...)+setWindowName(...) "
               "— it updates the EXISTING window in place if one exists, or creates it fresh "
               "otherwise, and can never raise this error.\n\n";

    // The internal C++/SIP class names (compile18/fittable18/dan18) are exactly what these three
    // singletons' TypeError/AttributeError text names them as.
    static const QRegularExpression singletonErrorRe(R"(\b(compile18|fittable18|dan18)\b)");
    if (!singletonErrorRe.match(errContext).hasMatch())
        return {};

    static const QString constructorHint =
        "REMINDER (this specific error keeps recurring): Compiler, Fittable, and Dan are "
        "pre-existing GLOBAL SINGLETON instances, accessed via their bare name — `c = "
        "Compiler`, `f = Fittable` — never called as a constructor, and never through some "
        "OTHER class/function (Dan.Compiler(), Fittable(), newFittable(), and similar are "
        "ALL wrong). The fix is always the same: use the bare name directly, no "
        "parentheses, no alternate class or function.\n\n";

    // "... object is not callable" is unambiguously the constructor-call mistake
    // (Compiler()/Fittable()/Dan()) — confirmed live, repeatedly.
    static const QRegularExpression notCallableRe(R"(\bnot callable\b)", QRegularExpression::CaseInsensitiveOption);
    if (notCallableRe.match(errContext).hasMatch())
        return constructorHint;

    // "... object has no attribute '...'" on fittable18 SPECIFICALLY is a DIFFERENT, confirmed-live
    // failure, not the constructor mistake: the singleton was already correctly obtained bare
    // (f = Fittable), but the model then guessed a plausible-sounding, nonexistent setter surface
    // instead of the real configure()/setWeighting*/fit() shape — cascaded across more than a dozen
    // retries in one conversation (setModel, setData, setYCol/setXCol/setDYCol, setWeighted, .new(),
    // setModelName/setDataName/setYColumn, function(), fit() with positional arguments — none of
    // these exist), each guess a fresh, equally-wrong new name, since neither this reminder's old
    // text nor the checklist (which only covers weighting-index/discovery nuances ON TOP of correct
    // usage) ever states the actual method list. compile18/dan18 don't get this branch: no live
    // evidence yet of the same guessing failure there — Compiler's own setter-heavy API happens to
    // match what a model naturally guesses, unlike Fittable's configure()-centric one.
    static const QRegularExpression noAttributeRe(R"(\bno attribute\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression fittableNameRe(R"(\bfittable18\b)");
    if (noAttributeRe.match(errContext).hasMatch() && fittableNameRe.match(errContext).hasMatch())
        return "REMINDER (this specific error keeps recurring): Fittable (`f = Fittable`, no "
               "parentheses — that part is already correct) has NO setModel()/setData()/"
               "setYCol()/setXCol()/setDYCol()/setWeighted()/setModelName()/setDataName()/"
               "setYColumn()/function()/new() methods — none of these exist, no matter how "
               "plausible they sound. The ONLY correct setup sequence is: "
               "f.configure(functionName, [\"table_col\", ...]) to select the model AND "
               "dataset(s) in ONE call (each dataset is a single \"tablename_colname\" string, "
               "never separate table/column arguments); then, only if weighted fitting is "
               "needed, f.setWeightingMethod(0), f.setWeightingEnabled(m, True), "
               "f.setWeightingDataset(m, \"table_dyCol\") (m is a 1-based dataset INDEX, not a "
               "name); then f.fit() with NO arguments at all — it takes none and returns "
               "chi2/dof directly. Do not invent a different setter name or pass fit() any "
               "arguments.\n\n";

    // "... object has no attribute '...'" on compile18 SPECIFICALLY, where the missing attribute is
    // actually a real FITTABLE method borrowed onto the wrong singleton — confirmed live, repeated
    // twice in the same conversation even after the first AttributeError: `c = Compiler` then
    // `c.fitFunctions(...)` (fitFunctions() belongs to Fittable, not Compiler — Compiler's own
    // equivalent is functionNames()). Without this branch, the fallback below (the constructor
    // hint, about parentheses) is irrelevant to this error and gave the retry no useful signal at
    // all, which is exactly why it repeated verbatim instead of converging.
    static const QRegularExpression compileNameRe(R"(\bcompile18\b)");
    static const QRegularExpression missingAttrRe(R"(no attribute '(\w+)')", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch missingAttrMatch = missingAttrRe.match(errContext);
    if (noAttributeRe.match(errContext).hasMatch() && compileNameRe.match(errContext).hasMatch() &&
        missingAttrMatch.hasMatch())
    {
        static const QStringList fittableOnlyMethods = {
            "fitFunctions",        "configure", "fit",           "setWeightingMethod", "setWeightingEnabled",
            "setWeightingDataset", "results",   "setParamValue", "simulateCurve",      "selectStage"};
        const QString missingAttr = missingAttrMatch.captured(1);
        if (fittableOnlyMethods.contains(missingAttr))
            return "REMINDER (this specific error keeps recurring): Compiler (`c = Compiler`) does "
                   "NOT have " +
                   missingAttr +
                   "() — that belongs to Fittable (`f = Fittable`), a DIFFERENT singleton. "
                   "Compiler's own equivalent for listing/checking existing compiled function "
                   "names is functionNames(\"ALL\") (filter it yourself, e.g. `[h for h in "
                   "c.functionNames(\"ALL\") if h.lower() == name.lower()]`) — never fitFunctions(), "
                   "that name only exists on Fittable. Do not borrow a method name from one "
                   "singleton onto the other.\n\n";
    }

    // Every other compile18/dan18/fittable18 "has no attribute" shape not covered above: fall
    // back to the constructor-mistake reminder, the still-most-common cause for those two.
    return constructorHint;
}

QString QtiSasAiContent::loadDocs(bool unlimited, QString *statusOut) const
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
            prompt += kDocSectionMarkerPrefix + fname + kDocSectionMarkerSuffix;
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
