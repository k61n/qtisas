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
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

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
    return "- setGroup() is not optional for a brand-new Compiler function — c.new() leaves the "
           "group field empty, and save()/compile() both raise ValueError: compile: group is "
           "empty — call setGroup(\"name\") before save()/compile() if it was never called. "
           "The minimal create-and-compile flow is c.new() -> setFunctionName(...) -> "
           "setGroup(...) -> setCode(...) -> compile() — setGroup() also raises ValueError on its "
           "own if given an empty name or one containing '/'.\n"
           "- xName/yName locals (whatever setXName()/setYName() actually set them to — not "
           "always \"x\"/\"I\") are ALREADY DECLARED by the boilerplate before [code] runs — never "
           "re-declare either with `double <name> = ...;` inside setCode(), only assign to it. "
           "Re-declaring yName fails loudly (`error: redefinition of '<name>'`), but re-declaring "
           "xName is worse: it's legal C++ shadowing, compiles with no error at all, and silently "
           "discards the real parameter value, producing a wrong-but-successful fit.\n"
           "- When the function being compiled is a NAMED physical/mathematical model (a sphere "
           "form factor, Guinier law, a named peak shape, ...), the setCode() expression must be "
           "that model's real, well-known formula, carefully recalled — not a plausible-looking "
           "approximation — and must include every term the request explicitly calls out (\"with "
           "background\", \"plus a linear term\", ...), never silently dropped. This is the same "
           "lapse generalChecklistRules() documents for table/matrix formulas, just written in C++ "
           "here instead of Python — same failure mode, same fix: double-check the formula "
           "actually being written against what's actually known, before compiling it.\n";
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
           "of the actual fit.\n"
           "- \"(X, Y, Z - fittable parameters)\" names the model's ONLY varying parameters — every "
           "OTHER parameter must get an explicit setParamVaries(name, False) too, not be left at "
           "whatever Fittable's own default is. The inverse phrasing, \"(X, Y, Z - unfittable "
           "parameters)\", is the same rule flipped: X/Y/Z get False, every other parameter gets "
           "True. This is IMPOSSIBLE to satisfy without first knowing the model's FULL parameter "
           "list — if it isn't already confirmed earlier in this conversation, an EXPLORE round "
           "calling f.paramNames() is a REQUIRED prerequisite (fittable-python-api.md documents "
           "exactly this discovery step), never skip straight to setParamVaries() calls for only "
           "the named parameters and leave the rest untouched. Confirmed live: \"(scale, "
           "background, radius - fittable parameters)\" only set those three to "
           "setParamVaries(..., True), no EXPLORE round ever ran to discover what the model's "
           "other parameters even were, and they were left unspecified rather than explicitly "
           "fixed.\n";
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
           "- Same rule applies when the request gives NO value for a parameter at all, not just "
           "when it names a different one: use the genuinely minimal/empty creation form the docs "
           "already provide (e.g. newPlot3D(\"name\") with no formula) — never fall back to a "
           "doc's own illustrative example as if it were a sensible default. Confirmed live: even "
           "\"empty\" in the request wasn't enough on its own — \"new empty 3D plot 'PP'\" still "
           "produced newPlot3D(\"PP\", \"x\", -1, 1, -1, 1, -1, 1, 10, 10), a fabricated (if "
           "simpler) formula, after a bare \"new 3D plot\" request had already produced the doc's "
           "own sin(x)*cos(y) worked example verbatim; only explicitly saying \"without data\" "
           "produced the correct, empty newPlot3D(\"PP\"). Same habit for tables/matrices: an "
           "unqualified creation request should create the object and stop — no invented column "
           "names, fill values, or display/color-map styling beyond what was actually asked.\n"
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
           "to 0-based indexing (also mechanically blocked pre-run if missed).\n"
           "- When a request names a specific physical/mathematical model (a sphere form factor, "
           "Guinier law, a named peak shape, ...), actually recall its real, well-known formula "
           "carefully rather than approximating it with a plausible-looking but different "
           "expression — and represent EVERY term the request explicitly calls out (\"with "
           "background\", \"plus a linear term\", ...) concretely in the code, never silently "
           "dropped. Confirmed live: asked for \"a sphere form factor (with background) plus 4% "
           "Gaussian noise\", the generated intensity was a bare Gaussian decay in q "
           "(`exp(-q**2 * const)`) — not the real sphere form factor `3*(sin(qR) - "
           "qR*cos(qR))/(qR)**3` — with no background term added anywhere, even though the exact "
           "same model, asked the SAME formula directly and in isolation, immediately produced "
           "the correct expression — this is a real lapse under the full request/script-writing "
           "context, not a genuine knowledge gap, so slow down and double-check the formula "
           "actually being written against what's actually known, rather than pattern-matching to "
           "something merely similar-looking.\n";
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

// Scoped to docTopics/splitModeActive the same way attached doc content already is
// (filterDocsByTopic()) — generalChecklistRules() always applies (import/singleton/indexing rules
// aren't topic-specific), the other three only when their own topic is actually in play for this
// call, cutting a plain table/note task's payload from all five sections down to one.
QString QtiSasAiContent::selfReviewChecklist(const QStringList &docTopics, bool splitModeActive) const
{
    QString checklist = generalChecklistRules();
    if (docTopics.contains("graph", Qt::CaseInsensitive))
        checklist += graphChecklistRules();
    if (docTopics.contains("compile", Qt::CaseInsensitive))
        checklist += compileChecklistRules();
    if (docTopics.contains("fittable", Qt::CaseInsensitive))
        checklist += fittableChecklistRules();
    if (splitModeActive)
        checklist += splitModeChecklistRules();
    return checklist;
}


QString QtiSasAiContent::docTopicIndex() const
{
    return "ascii1d — import/export of 1D SAS data (Ascii1d: ASCII.SANS.1D): SANS/SAXS/SALS curves, radially "
           "averaged datasets, ascii/.dat/.RAD/canSAS/NIST/GRASP files; file to table, table to file, file to file, "
           "table to table; removing points, SAS presentations (Kratky, Guinier...), resolution sigma, binning.\n"
           "compile — create, edit, or modify fit functions, new OR existing (Compiler class: c = Compiler). FIF "
           "files, C++ code generation, parameters/limits, after-fit hooks.\n"
           "fittable — for EXISTING compiled functions ONLY, never compiles new ones: CONFIGURE first, then "
           "RUN the fit (Fittable class: f = Fittable). Parameters, weighting, resolution, polydispersity, "
           "global/batch fits.\n"
           "graph — 2D plots (GraphWindow/Graph/MultiLayer): curves, axes, layers, styling.\n"
           "graph3d — 3D surface/scatter plots (Graph3D/Qti3D). Fully self-contained: legend "
           "(showLegend()), title (setTitle()), and axis labels (setXAxisLabel()/setYAxisLabel()/"
           "setZAxisLabel()) are all native Graph3D methods, never the 2D graph topic's — a part "
           "that only creates/styles a Graph3D does not need the graph topic too. Worse than just "
           "unnecessary here: graph3d's own docs warn that graphWindow(\"name\") (the 2D lookup) "
           "returns None for a real Graph3D window (use plot3D(\"name\") instead) — a confirmed-"
           "live footgun that pulling in the graph topic risks reinforcing rather than avoiding.\n"
           "jnse — neutron spin echo data (Jnse: J-NSE): .dtr files into a header table (one row per q) and one "
           "dataset table per q (S(q,t)/S(q) vs Fourier time); multi-q fits of the datasets in Fittable "
           "(simpleDiffusion, shared D), relaxation rates.\n"
           "matrix — the Matrix class: 2D numeric grids/image-like data. Its (row, col) cell order "
           "is the OPPOSITE of Table's (col, row), but that distinction is documented within "
           "matrix's own docs — a part that only creates/fills/reads a Matrix does not need the "
           "table topic too just because of that cross-reference. Ambiguous \"color fill\"/"
           "\"colormap\"/\"grayscale\" language for matrix data has THREE genuinely different real "
           "meanings, not one: (1) the matrix's OWN in-place 2D view — "
           "m.setViewType(Matrix.ViewType.ImageView) + setRainbowColorMap()/setGrayScale(), same "
           "window, topic matrix only; (2) a separate 2D graph spectrogram — plot(matrix, style) "
           "/ plotSpectrogram(matrix, style) with Graph.CurveType.ColorMap/GrayScale/Contour/"
           "ImagePlot, a NEW GraphWindow, topic graph; (3) an actual 3D surface — "
           "plot3D(matrix, style) with a Qti3D.<CONSTANT>, a NEW Graph3D window, topic graph3d. "
           "Pick based on what the request actually asks for (in-place display vs. a new "
           "plot/window vs. an explicit 3D surface) — don't default to one without checking "
           "which is meant.\n"
           "note — the Note class: text/script notes and tabs.\n"
           "svd — contrast variation (Svd): partial structure factors S_ij of components from SANS curves in "
           "H2O/D2O solvent mixtures by singular value decomposition; match points, the rank of an H/D series, "
           "planning D2O fractions.\n"
           "table — the Table class: spreadsheet-like data tables, columns, import/export.\n";
}

QStringList QtiSasAiContent::knownDocTopics() const
{
    return {"table", "matrix", "graph", "graph3d", "note", "compile", "fittable", "ascii1d", "svd", "jnse"};
}

// the modules' docs (ASCII.SANS.1D, SVD, JNSE): sent when a request needs them — by its words or an object it names
QStringList QtiSasAiContent::onDemandDocTopics() const
{
    return {"ascii1d", "svd", "jnse"};
}

QStringList QtiSasAiContent::baselineDocTopics() const
{
    // "ai" — assistant workflow rules. "app" — newTable/newGraph/newMatrix/newNote/scriptPrint/
    // qti.app, used in nearly every script regardless of topic.
    return {"ai", "app"};
}

QString QtiSasAiContent::splitPlanPreflightPrompt(const QString &requestText, bool docsIntegrated) const
{
    // Rewritten as a numbered, priority-ordered rule list (was prose paragraphs) plus a
    // draft/check/final two-stage answer format — confirmed live via direct, isolated testing
    // against the raw model (bypassing this whole app), not just reasoned about:
    //
    // - The ORIGINAL prose-paragraph prompt reproducibly degenerated into repetition-loop
    //   garbage on this exact request — either identical lines repeated verbatim dozens of
    //   times, or a small set of ~9 phrasings cycled through repeatedly — confirmed to be a
    //   general model/decoding weakness (reproduced on a completely unrelated, non-QtiSAS prompt
    //   too: "list party-planning tasks" degenerated identically), NOT something specific to this
    //   prompt's own wording.
    // - Two DIFFERENT targeted reactive instructions ("merge neighboring same-DOCS lines",
    //   "stop immediately if about to repeat the previous line verbatim") were each tried alone
    //   and had ZERO effect on the loop — confirming this is a decoding-level termination
    //   failure, not an instruction-comprehension gap that a single added sentence could fix.
    // - This restructured version (numbered rules + a required self-check pass before the model
    //   commits to an answer) did NOT reproduce the loop across repeated tests, including on the
    //   GaussPeak compound multi-topic request (this project's own standing regression case) —
    //   where the check step visibly caught and fixed a real over-fragmentation (the draft
    //   separated "fit both models" from "compare chi2/dof" as two parts; the check step
    //   correctly merged them under rule 3 before finalizing).
    //
    // Rules are numbered programmatically below, not hardcoded, because rule 2 (the
    // Integrated-mode doc-footprint note) only exists in one of the two modes this function
    // serves — hardcoding "3.", "4.", ... after it would silently mislabel every rule whenever
    // docsIntegrated is false.
    QStringList rules;
    rules << "Split into multiple parts ONLY where doing so gives a genuine benefit: an earlier "
             "part performs a distinct, already-durable action (creating a table, compiling a "
             "function, running a fit, producing a graph, applying styling) that a LATER, "
             "unrelated part doesn't need to redo or risk re-running if it fails.";
    if (docsIntegrated)
        // Every OTHER reason above/below to split (durability, retry isolation) still applies
        // unchanged in Integrated mode — this rule only removes doc-footprint as an ADDITIONAL,
        // unstated pressure toward more parts. Confirmed by inspection (not a live incident
        // writeup): DocsMode::Integrated means no part's DOCS field ever attaches anything
        // regardless of what topics are listed (see AiAssistant::buildSystemPrompt()'s own
        // Integrated short-circuit), so a model defaulting to its usual "keep each part's doc
        // surface narrow" caution here is splitting defensively against a cost that was never
        // actually going to be paid.
        rules << "No per-topic documentation will be attached to ANY part regardless of its DOCS "
                 "field — this deployment already has full API knowledge built in. Doc footprint "
                 "is therefore NOT a reason to split here; decide purely on rule 1, and default "
                 "to the FEWEST parts that still respect it.";
    rules << "Do NOT fragment a request that is really one coherent step just to produce more "
             "parts — e.g. \"create a table and immediately fill it with computed values\" is "
             "ONE part, not two: filling has no independent value without the creation, and both "
             "would need to re-run together on any failure regardless. If the whole request is "
             "already a single coherent step, return exactly one part covering all of it.";
    rules << "Quick test: if the request already gives an explicit analytical formula (e.g. "
             "sin(x)*cos(y), x^2+y^2 — anything you could type directly as a plot3D(\"<here>\", "
             "...) formula string), that formula IS the complete data source — use exactly one "
             "part that plots straight from it, and do NOT create a table or matrix at all. Never "
             "precede it with a \"create a table/matrix with x,y,z values\" part: that table would "
             "be pure dead weight. Only propose a table/matrix-creation part when the request "
             "instead describes real/measured values, or explicitly names an existing table/"
             "matrix. Confirmed live, repeatedly, on the identical request: an unrequested table "
             "got built and manually populated via a loop instead of using the formula directly — "
             "wasted work every time, even though a version of this exact rule already warned "
             "against it.";
    rules << "A graph's (2D or 3D) creation and EVERY piece of its styling/configuration — "
             "mesh/line style, colors, rotation, scale, axis labels, axis numeric format, legend, "
             "title, layout — belong in the SAME single part as the creation, no matter how many "
             "distinct styling actions the request lists. Confirmed live: exactly this happened — "
             "a request to plot a surface with a specific mesh style, axis labels, and a legend "
             "was decomposed into 4 parts (create, set mesh style, add axis labels, add legend), "
             "and the eventual merge just concatenated 3 redundant graph-lookup-then-one-line-"
             "change scripts instead of the single coherent script this always should have been.";
    rules << "If two or more parts would need the exact same DOCS topic(s), merge them into ONE "
             "part instead of listing them separately — never leave two parts with an identical "
             "DOCS field when nothing else forces them apart under the rules above.";
    rules << "A single part's description may span MULTIPLE sentences or clauses — a longer, "
             "multi-sentence description for one part is always preferred over splitting into "
             "additional parts just to keep each description short.";
    rules << "Every qualifying detail in the ORIGINAL request — a parenthetical like \"(with "
             "background)\", a noise/error percentage, an explicit value, a named constraint — "
             "must survive into whichever part's description covers that action, worded however "
             "is natural but never dropped for brevity. A part's script is generated from ITS OWN "
             "description alone, not the original request text, so anything paraphrased away here "
             "is gone for good by the time code gets written. Confirmed live: \"...a sphere form "
             "factor (with background) plus 4% Gaussian noise\" produced a part description "
             "reading \"...for sphere form factor plus 4% Gaussian noise\" — \"(with background)\" "
             "silently disappeared already at THIS decomposition step, before any code was ever "
             "generated, and the resulting script predictably never added one.";
    rules << "A LATER part's script can assume every EARLIER part already ran successfully in "
             "this same conversation — whatever it created (a table, a compiled function, a "
             "graph, ...) already exists live — but must look it up BY NAME (table(name), "
             "graphWindow(name), re-opening a compiled function by name, etc.), never by assuming "
             "a variable from an earlier part's script is still bound; only printed output "
             "carries forward between separate script runs, never the Python objects themselves "
             "(the same rule that already governs EXPLORE rounds).";
    rules << "Decide and FIX every object name (table, graph, compiled function, ...) HERE, at "
             "this decomposition step — never leave one to be invented ad hoc when a part's "
             "script is actually generated later. Name it explicitly in the description of the "
             "part that CREATES it (e.g. \"create table named '<PlaceholderTable>' with...\", "
             "\"plot results into a graph named '<PlaceholderGraph>'\"), and repeat that EXACT "
             "same name in every LATER part's description that reads or references it (e.g. "
             "\"...add a legend to the '<PlaceholderGraph>' graph\", \"...read the last value "
             "from table '<PlaceholderTable>'\"). This applies to every kind of named object a "
             "later part must look up, not just a compiled function name — a name decided "
             "independently and separately by each part's own script is exactly how mismatches "
             "happen (one part creating a graph called \"FitResults\", a later part guessing a "
             "different name for it). The names in THIS rule and the example line below are "
             "placeholders illustrating the FORMAT only — every actual name you write must come "
             "from the REQUEST below (or be one you invent because the request left it unnamed), "
             "never copied from this instruction text or from any example in a loaded doc file. "
             "Confirmed live: asked for a table explicitly named 'GaussPeak', a decomposition "
             "instead invented an unrelated table named 'SphereSANS' — the exact placeholder-"
             "style name this instruction text used to use in this very spot before it was made "
             "an obvious placeholder — while also completely ignoring the required PART/DOCS "
             "format below in favor of freeform \"Part 1: ...\" prose.";
    rules << "A part's DOCS list must include every topic its own script actually calls into — "
             "not just the topic matching its primary/described action. A part whose job is "
             "graph styling but which must ALSO read back a value produced by a different "
             "topic's object (e.g. \"add a legend showing the fitted radius\" needs Fittable's "
             "f.results(), not just Graph's styling calls) still makes a real API call into that "
             "other topic, and needs that topic's docs too, or that call is pure guesswork. When "
             "in doubt, include the topic that PRODUCED a value a later part must read, not only "
             "the topic that CONSUMES or displays it.";
    rules << "\"Compile a custom fit function\" needs ONLY compile, never fittable — c.compile() "
             "calls into Compiler alone; fittable is for a part whose script calls Fittable itself "
             "(f.configure(), f.fit(), ...). Confirmed live: \"compile two custom fit functions\" "
             "still got DOCS: compile, fittable for a part whose script never touched Fittable, "
             "purely from \"fit\" appearing in the wording — that word describes the function's "
             "eventual USE, not this part's own API calls, and inflated that part's request enough "
             "to risk a timeout. Naming a function \"fit\"-something is not a Fittable call.";
    rules << "A part whose script only selects an ALREADY-compiled function BY NAME through "
             "Fittable (f.configure()/f.setFunction(), then f.fit() or f.simulateCurve()) needs "
             "ONLY fittable — never compile (selecting by name is not a Compiler call, even though "
             "an earlier part compiled that function) and never table (a result table fit()/"
             "simulateCurve() creates automatically is Fittable's own side effect, not this "
             "script's own Table-API call). Add graph only if the part's own description actually "
             "asks for a plot — producing or simulating a curve is not, by itself, a request to "
             "graph it.";
    rules << "A part whose script only creates/fills/reads a Matrix (setFormula(), calculate(), "
             "setViewType(), setRainbowColorMap()/setGrayScale(), ...) needs ONLY matrix — never "
             "table too, even though both are 2D numeric grids. Confirmed live: exactly this "
             "extra table topic accompanied a part whose script had zero Table-API calls, and the "
             "generated code called newTable() instead of newMatrix() for a request that explicitly "
             "asked for a matrix — caught only by a downstream mechanical check, not by this "
             "decomposition step. Add table only if the part's own description also names a real, "
             "separate Table action (creating/reading/plotting an actual table), never just because "
             "a matrix is table-shaped.";
    rules << QString("Default to the FEWEST parts that still satisfy rules 1-%1.").arg(rules.size());

    QStringList numberedRules;
    for (int i = 0; i < rules.size(); ++i)
        numberedRules << QString("%1. %2").arg(i + 1).arg(rules[i]);

    return "Split-mode pre-flight: decide whether the request below should be broken into an "
           "ORDERED list of smaller, independently-runnable parts, each generated, reviewed, and "
           "executed as its OWN separate script in sequence — NOT combined into one script — or "
           "whether it is already a single coherent step that shouldn't be fragmented.\n\n"
           "Follow these rules IN ORDER OF PRIORITY — an earlier rule always wins over a later "
           "one if they conflict:\n\n" +
           numberedRules.join("\n\n") +
           "\n\nAnswer in TWO stages.\n\n"
           "Stage 1 — DRAFT: write your draft PART/DOCS list, one PART per line, nothing else:\n"
           "PART: <short, self-contained, imperative description of exactly what this part must "
           "accomplish> | DOCS: <comma-separated topic names from the index below this part's "
           "script will need, or \"none\">\n\n"
           "Stage 2 — CHECK, then FINAL: on a new line write exactly \"---CHECK---\", then "
           "briefly verify your draft against the numbered rules above (does any line repeat an "
           "earlier line's action, violate the graph-styling or same-DOCS-merge rules, or "
           "fragment unnecessarily?). Then write exactly \"---FINAL---\" followed by the "
           "corrected list if anything needed fixing, or the same list unchanged if it was "
           "already correct. The FINAL list is the only part that will actually be used — "
           "everything before it is your own working draft, not the answer.\n\n"
           "Example demonstrating the format ONLY — invent your own description/topics from the "
           "ACTUAL request below, never reuse this scenario or name:\n"
           "PART: create table named '<PlaceholderTable>' with 50 rows of x,y data | DOCS: table\n\n"
           "Available topics:\n" +
           docTopicIndex() + "\nRequest:\n" + requestText;
}

QString QtiSasAiContent::continuationCheckPreflightPrompt(const QString &newRequestText) const
{
    return "The conversation above is what has actually happened so far. A new message just "
           "arrived:\n\n" +
           newRequestText +
           "\n\nDoes the new message CONTINUE the ongoing conversation above (references or builds "
           "on the same data/objects/goal, e.g. \"now add a legend to that\", \"also plot the "
           "residuals\", or refers to something the conversation above actually created), or is it "
           "a NEW, independent task unrelated to it (a different subject, different data, no "
           "reference back to anything above)? If you cannot tell confidently either way, say so "
           "honestly rather than guessing.\n\n"
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
    // Generate-only mode: the script will be SHOWN, never executed, and neither will anything this
    // session generated before it. So a resource the request names as existing may genuinely not
    // exist yet — typically because an earlier, equally unexecuted step was the thing that would
    // have created it. Saying so here replaces the existence-check round-trip that beginRequest()
    // now skips in this mode (see its own doc comment for why): the verdict's only useful effect was
    // to make the script defensive, and stating the situation gets that for free, without executing
    // anything or spending a call. Confirmed live: a generate-only item wrote
    // "if n is None: n = newNote(\"Batch\")" unprompted, which is exactly the wanted shape.
    return "\n\n(Answer in plain text if this is a question. "
           "Return raw Python code only if code was explicitly requested. Nothing in this session "
           "has been executed — not even scripts shown earlier in this same conversation — so any "
           "table/matrix/note/graph or compiled function this request names may not exist yet. "
           "Write code that CREATES it when absent and reuses it when present (e.g. "
           "\"t = table(name)\" then \"if t is None: t = newTable(name, ...)\"), and never stop or "
           "report failure merely because a name is missing.)";
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

QString QtiSasAiContent::exploreUnavailableNote() const
{
    // Deliberately contradicts the corpus head-on and by name: ai-python-api.md's "When EXPLORE is
    // mandatory" table and its "## For AI: Agentic Explore Mode" section are what the model is
    // following when it probes, so a vague "you may not need to explore" loses to them. Also gives
    // it the supported alternative in the same breath — reading live values at run time inside the
    // action script — because a prohibition with no replacement is what produced scripts that
    // simply guessed.
    return "\n\nPROBE ROUNDS ARE NOT AVAILABLE FOR THIS REQUEST. Do NOT return a \"# EXPLORE\" "
           "script: it will not be executed, no output will come back, and asking for one only "
           "wastes a round-trip. This overrides the API documentation's own \"Agentic Explore Mode\" "
           "section and its \"When EXPLORE is mandatory\" table for this request — those describe "
           "what happens when probing IS available, which it is not here.\n"
           "Answer directly instead. Where a probe would have told you a live value (whether a "
           "table/graph/note exists, a function's real parameter names, an existing group or "
           "template name), write the action script so it reads that value AT RUN TIME and branches "
           "on what it finds — that is the supported way to handle unknown project state, and it "
           "needs no separate round-trip. Do not invent a value you would have checked, and do not "
           "drop part of the task because you could not check it first.";
}

namespace
{
// One requested deliverable: what the REQUEST has to say for this to be expected, what the SCRIPT
// has to contain for it to count as delivered, and how to name it back to the model.
//
// Entries are chosen for a fingerprint that is unambiguous in BOTH directions. Two deliberate
// omissions, each a real false positive found while calibrating this against a 12-item run:
//   - a generic "fit" entry: "simulate a curve from it WITHOUT fitting any real data" mentions
//     fitting in order to exclude it, and would have demanded a fit() call that must not be there;
//   - "the data" as a plotting deliverable: too generic to distinguish "plot the data" from any
//     other sentence containing the word.
// Nothing here fires on 9 of that run's 12 items; the 3 it flags are all genuine.
struct RequestedDeliverable
{
    const char *requestPattern;
    const char *codePattern;
    const char *label;
};

const RequestedDeliverable kRequestedDeliverables[] = {
    {R"(\bresidual)", R"(residu)", "residuals (no residuals column is read or plotted anywhere)"},
    // plotResults() belongs here: fittable-python-api.md documents it as "plot data + fit curves in
    // a named graph", i.e. the supported ONE-call path. Confirmed live as a false positive without
    // it — a correct `f.plotResults("FitResults")` was blocked for having no addCurve/insertCurve,
    // demanding manual curve insertion the docs never require. Note it covers data + fit only, so
    // the separate "residuals" entry below still applies on its own.
    {R"(\bfit curve\b)", R"(\baddCurve\b|\binsertCurve\b|\bplotResults\b|\bplot\()",
     "the fit curve (no addCurve()/insertCurve()/plotResults()/plot() call anywhere)"},
    {R"(\blegend\b)", R"(\bnewLegend\b|\bshowLegend\b|\blegend\()", "the legend (nothing creates or shows one)"},
    {R"(\baxis (?:titles?|labels?)\b)", R"(\bset[XYZ]Title\b|\bsetAxisTitle\b|\bset[XYZ]AxisLabel\b)",
     "the axis titles/labels (none are set)"},
    {R"(\bminor tick)", R"(MinorTicks)", "the minor tick marks (no minor-tick call)"},
    // Only an ADD demands addTab(). A new note already owns one tab — note-python-api.md's own
    // example prints "3  (1 initial + 2 added)" — and setText() "replace[s] full text in the active
    // editor tab", so "create a note WITH a script tab" is satisfied by writing into the tab that
    // already exists. Confirmed live as a false positive: the three-line
    // newNote()/setText()/setAutoexec() answer was blocked in every run, and each retry spent a
    // round trip adding a SECOND tab to write into, leaving tab 0 empty for no gain.
    // There is deliberately NO "active tab" entry beside these two: note-python-api.md states that
    // addTab() already activates the tab it creates and that no Python call can activate a
    // different, already-existing tab (setCurrentIndex/setActiveTab/setCurrentTab are not on Note).
    // So "keep it as the active tab" is satisfied by doing nothing on a one-tab note, and by
    // addTab() itself otherwise — such an entry could only ever demand that spurious second tab.
    // activateWindow() would not have counted either way: it activates the WINDOW, not a tab.
    {R"(\badd(?:s|ing)?\b[^.]{0,40}?\bscript tab\b)", R"(\baddTab\b)",
     "the added script TAB (no addTab() — the request asks to ADD a tab to an existing note)"},
    {R"(\bscript tab\b)", R"(\baddTab\b|\beditorAt\b|\bcurrentEditor\b|\bsetText\b)",
     "the script tab's content (nothing writes a script into any tab)"},
    {R"(\bautoexec\b|\beach time the project loads\b|\bwhen the project (?:file )?is opened\b)", R"(\bsetAutoexec\b)",
     "auto-execution on project load (no setAutoexec() call)"},
    {R"(\bsimulate\b)", R"(\bsimulateCurve\b)", "the simulated curve (no simulateCurve() call)"},
    // A fit the request orders — "Fit ...", "... and fit ...", "from fits of ..." — not one it only mentions ("the fit
    // curve", "chi2 of the fit"): confirmed live (benchmark JNSE, local pyi, items 6 and 11), "Fit only the four
    // smallest q ... Print D" answered with scriptPrint("R6:", Fittable.paramValue("D")) — the D of an earlier fit
    {R"((?:^|[.;:!?]\s+|\b(?:and|then)\s+)(?:re)?fit\b(?!\s+(?:functions?|curves?|models?|results?)\b)|\bfits\s+of\b)",
     R"(\bfit\s*\()", "the fit (no fit() call — the request asks to fit, a value of an earlier fit is not it)"},
    {R"(\bcompile\b)", R"(\.compile\()", "the compile step (no compile() call)"},
    // Not as a criterion: "the function with the smaller chi2/dof" or "compare chi2/dof" asks to CHOOSE by it, not to
    // print it — confirmed live (benchmark LIST 5, item 2), a correct "R2: <name> <A> <w>" was blocked twice and the
    // item given up
    {R"((?<!smaller )(?<!larger )(?<!lower )(?<!higher )(?<!smallest )(?<!largest )(?<!lowest )(?<!highest )(?<!best ))"
     R"((?<!better )(?<!compare )(?<!comparing )(?<!compare the )(?<!compare their )(?<!comparing the ))"
     R"((?<!comparing their )(?:\bchi2\b|chi²))",
     R"(chi2|chi²)", "the chi2/dof report (chi2 is never printed)"},
    {R"(\bstacked panels?\b|\btwo panels?\b)", R"(\blayer\(2\)|\bnewGraph\([^)]*,\s*2)",
     "the second stacked panel (only one layer is ever created or used)"},
    {R"(\brow count\b)", R"(\bnumRows\b)", "the row count (numRows() is never read)"},
    // Confirmed live (benchmark LIST 3, item 12): asked to delete two fit functions, the script only listed the
    // library's functions and Compiler's methods, deleted nothing, and the item counted as done. Only "delete/remove"
    // whose OBJECT is such a thing (articles, a count or a quoted name in between), so "remove the noise from column
    // y" or "remove outliers" never asks for a call.
    {R"(\b(?:delete|remove)\s+(?:(?:the|these|those|this|that|all|both|two|three|\d+)\s+)*(?:"[^"]*"\s+)?(?:fit\s+)?)"
     R"((?:functions?|tables?|matri(?:x|ces)|notes?|graphs?|windows?|columns?|rows?|curves?|tabs?|folders?)\b)",
     R"(\.delete\(|\bdeleteFolder\b|\bcloseWindow\b|\.close\(\)|\bremove\w*\(|\bdelete\w*\()",
     "the deletion (no delete()/closeWindow()/remove...() call — listing what exists is not deleting it)"},
};
} // namespace

QString QtiSasAiContent::missingRequestedDeliverables(const QString &code, const QString &requestText) const
{
    if (code.isEmpty() || requestText.isEmpty())
        return {};
    QStringList missing;
    for (const RequestedDeliverable &d : kRequestedDeliverables)
    {
        const QRegularExpression requestRe(QString::fromUtf8(d.requestPattern),
                                           QRegularExpression::CaseInsensitiveOption);
        if (!requestRe.match(requestText).hasMatch())
            continue;
        const QRegularExpression codeRe(QString::fromUtf8(d.codePattern), QRegularExpression::CaseInsensitiveOption);
        if (codeRe.match(code).hasMatch())
            continue;
        missing << QString::fromUtf8(d.label);
    }
    if (missing.isEmpty())
        return {};
    // Phrased as an omission to be added, not as a defect to be explained: the failure this catches
    // is a script that quietly substituted a smaller task, and asking "why" invites a justification
    // round instead of the missing code.
    QStringList numbered;
    for (int i = 0; i < missing.size(); ++i)
        numbered << QString("%1. %2").arg(i + 1).arg(missing.at(i));
    return "This script omits " + QString::number(missing.size()) +
           (missing.size() == 1 ? " thing the request explicitly asked for:\n"
                                : " things the request explicitly asked "
                                  "for:\n") +
           numbered.join("\n") +
           "\n\nAdd it. If the script guards against a missing object (\"if x is None: report and stop\"), that guard "
           "is fine — but the branch that DOES the work still has to contain the calls listed above. Do not drop a "
           "requested deliverable, do not substitute a different action for it, and do not defer it to a later step: "
           "this script is the only one that will do this task.";
}

QString QtiSasAiContent::liveEntityFactsNote(const QList<QPair<QString, QStringList>> &facts) const
{
    if (facts.isEmpty())
        return {};
    QStringList lines;
    bool anyUnknown = false;
    bool anyExisting = false;
    for (const QPair<QString, QStringList> &entry : facts)
    {
        if (entry.second.isEmpty())
        {
            // Deliberately NOT reported as "does not exist", even though that is what the registries
            // say right now. This note only reaches a request whose script will be SHOWN and not
            // run, so "absent" is a claim about a moment that is not the one the script executes in:
            // the user may run an earlier item's script first, or none of them, or all of them
            // tomorrow. Stating the absence anyway produced three separate confirmed-live failures
            // in one run, all of them the model BELIEVING it:
            //   1. the fact copied into the script as a statement — an unconditional
            //      scriptPrint("Table 'SAXS_data' does not exist.") at the top of a script that
            //      would run after an earlier item had created it;
            //   2. the object recreated from scratch with worse data than the earlier item that
            //      really owns it produced (a Gaussian standing in for a sphere form factor);
            //   3. worst, asked only to RESTYLE a missing graph, the script invented an entire
            //      substitute dataset (a new "SampleData" table of exp(-x) noise) and renamed a
            //      graph of it, purely to have something to style.
            // Fighting a belief with prohibitions costs a paragraph and still leaves the belief;
            // not creating it costs nothing. The one thing the model genuinely needs here — resolve
            // this at run time, do not spend a round-trip asking — survives intact, and now agrees
            // with exploreUnavailableNote() instead of pulling against it.
            anyUnknown = true;
            lines << "- \"" + entry.first + "\": UNKNOWN — cannot be resolved ahead of time.";
        }
        else
        {
            // The existing half stays a hard fact, because existence is very nearly monotone: a
            // session's scripts create and overwrite, they do not delete or rename, so "exists now"
            // still holds when the script runs while "absent now" does not. This half is also where
            // the value is — confirmed live, it killed a probe AND the fitFunctions("*sphere*") +
            // hits[0] guessing for a name the request had already given exactly.
            //
            // Kinds are translated out of the internal check vocabulary ("compile"/"fittable") into
            // what a QtiSAS user would call them, since this text is read, not parsed.
            anyExisting = true;
            QStringList readable;
            for (const QString &kind : entry.second)
                readable << (kind == "compile"    ? QStringLiteral("compiled function")
                             : kind == "fittable" ? QStringLiteral("fit function")
                                                  : kind);
            lines << "- \"" + entry.first + "\": exists NOW, as: " + readable.join(" and ") + ".";
        }
    }
    QString note = "\n\nPROJECT STATE (read directly from QtiSAS, not guessed):\n" + lines.join("\n");
    // Each instruction is gated on ITS OWN case actually occurring: telling a model what to do about
    // an unknown name when nothing is unknown is an invitation to write pointless defensive
    // branching around an object already known to be there, and the mirror mistake spends context
    // on a case that cannot arise.
    if (anyUnknown)
        note += "\n\nUNKNOWN is the real answer, not a missing lookup: nothing generated in this session "
                "has been executed yet, so whether such a name exists depends on which earlier scripts "
                "get run and when. Do not assume it exists, do not assume it is absent, and do not state "
                "either as a fact in your script. Do not spend a separate round-trip asking — decide it "
                "AT RUN TIME, inside the script you are writing now: \"t = table(name)\" then \"if t is "
                "None: t = newTable(name, ...)\", so it works either way. If this request only MODIFIES a "
                "named object (restyle, add a curve, rename, configure), create just that object when it "
                "turns out to be absent and then do the requested work — never invent substitute data, a "
                "stand-in table, or extra objects the request never asked for.";
    if (anyExisting)
        note += "\n\nFor any name that exists above: use that object rather than creating another — when the "
                "request deletes, changes or reads it, your script still makes that call — and do not write a "
                "separate script to look it up again. If its kind is not the kind this request needs — "
                "an existing fit function where the request wants a graph of that name, say — that is a "
                "name collision, not the object you want: say so plainly, and do not overwrite it.";
    return note;
}

QString QtiSasAiContent::exploreModeReminder() const
{
    // The purpose line asks for an INTENDED USE, not just a description of what's being checked —
    // confirmed live (repeatedly, this session): the previous wording's one worked example
    // ("checking existing groups and previewing a reference model") recurred verbatim across
    // unrelated requests, and the exploration it labeled routinely had zero effect on the write
    // action that followed (e.g. EXPLORE prints the real c.groups() list, then setGroup() invents
    // an unrelated name anyway — twice, even when the printed list was complete). A bare label has
    // nothing for the next line of code to be held to; stating the use at least makes the model
    // commit to something concrete before it sees the answer.
    // No worked example: every one was copied as the probe itself. Confirmed live three times — "need an existing group
    // name to reuse in setGroup()" opened nearly every compile item (LIST 5 local pyi, 2026-10-10); "the column names
    // of the table the request names" became "the column names of the table \"Peak5\"" for the table the item creates;
    // "... of the existing table the request only reads, to pick its Y and error columns" was 19 of 36 probes word for
    // word (2026-10-11). A created object is ruled out in words instead.
    return "If you need to check real state (an existing table/graph/3D-plot/compiled function's "
           "actual name, its current parameters, or simply whether something already exists) "
           "before writing the real script, return a READ-ONLY script whose EXACT first line is "
           "\"# EXPLORE: <what you need to know, and what you'll do with the answer>\" — state the actual use, not "
           "just "
           "a label for what's being checked. An object this request itself creates has nothing to explore: it does "
           "not exist until your script makes it. QtiSAS runs "
           "it, captures every scriptPrint()/print() call's output, and sends it back to you as "
           "context — then return the real action script (no EXPLORE marker) using what you just "
           "confirmed, rather than guessing or repeating a value you assumed before seeing this. An "
           "EXPLORE script must never create or modify anything (no newTable()/newGraph()/"
           "newPlot3D()/plot()/addCurve()/compile()/setWindowName(), etc.) — split those calls into "
           "the action script instead. Not limited to one round: if the output raises a new "
           "question, return ANOTHER EXPLORE script rather than assuming the action script must "
           "come next.\n\n";
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
                                                 const QString &earlierPartsCode, bool wholeRequest) const
{
    // Every direction reports, instead of the first one returning immediately: they are
    // independent scope rules and a script can trip several at once, which previously cost
    // one retry round per direction. Same "collect all" reasoning detectKnownMistakes() and
    // runActionScript()'s block already apply; this was the last exclusive layer left.
    QStringList violations;

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
            violations << "This part's own job (\"" + partDescription +
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
            violations << "This part's own job (\"" + partDescription +
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
    //
    // The literal-name check below is only meaningful when a name was actually fixed somewhere in
    // text to check against. When the user's original request never names the function at all
    // (e.g. "compile a custom Porod power-law fit function with an adjustable exponent parameter")
    // the model MUST invent one, and by construction no invented name can appear in a description
    // that never specified one — the check would then block every possible name, unconditionally.
    // Confirmed live, severe: exactly this shape blocked a part twice (two different invented
    // names, "ai/PorodPowerLaw" then "PorodPowerLaw"), exhausted the "same explanation twice in a
    // row" retry guard, and killed the whole split-mode run.
    //
    // So the literal-name requirement is skipped for the FIRST setFunctionName() call only when
    // the part's own description never names any function at all (no quoted identifier anywhere)
    // — nothing exists to check an invented name against. Whenever the description DOES name a
    // specific function (quoted, e.g. "named 'GaussPeakModel'"), the first call is still checked
    // against it like any other — that's what catches the documented true-positive shape (a
    // SECOND, extra setFunctionName() call for an unrelated, separately-named function smuggled
    // into an otherwise correctly single-scoped part); a mismatch on the very first call against
    // an explicitly named target is just as real a violation.
    // A quoted name only counts as naming a FUNCTION when it isn't introduced as some other kind of
    // object. Confirmed live: a description reading `Create a table "GaussPeak" ... Plot ... into
    // graph "GaussPeakFit"` asked for two functions WITHOUT naming either, yet both quoted names
    // made descriptionNamesAFunction true — so the first-call exemption below switched off and the
    // invented "ai/GaussianPeak"/"ai/LorentzianPeak" were flagged as another part's job. That is
    // exactly the "blocks every possible name, unconditionally" failure the exemption exists to
    // prevent, reached through a side door: the item was blocked twice and the run died on it.
    static const QRegularExpression explicitNameInDescriptionRe(
        R"((?<!\btable )(?<!\bmatrix )(?<!\bnote )(?<!\bgraph )(?<!\bwindow )['"]([A-Za-z_][A-Za-z0-9_/]*)['"])",
        QRegularExpression::CaseInsensitiveOption);
    const bool descriptionNamesAFunction = explicitNameInDescriptionRe.match(partDescription).hasMatch();
    static const QRegularExpression setFunctionNameRe(R"(setFunctionName\(\s*["']([^"']+)["']\s*\))");
    // How many invented names the description actually licenses: one per "function" it asks for,
    // minimum one. Confirmed live in both directions — "compile a custom Porod power-law fit
    // FUNCTION" (one) must still flag a second, smuggled compile job, while "Compile a custom
    // Gaussian peak fit FUNCTION and a Lorentzian fit FUNCTION" (two) legitimately needs two
    // invented names and used to have its second one blocked no matter what it was called, twice,
    // killing the run. Counting the noun is what separates those two shapes.
    static const QRegularExpression functionNounRe(R"(\bfunctions?\b)", QRegularExpression::CaseInsensitiveOption);
    int inventedNamesAllowed = 0;
    QRegularExpressionMatchIterator nounIt = functionNounRe.globalMatch(partDescription);
    while (nounIt.hasNext())
    {
        nounIt.next();
        ++inventedNamesAllowed;
    }
    inventedNamesAllowed = qMax(inventedNamesAllowed, 1);

    QRegularExpressionMatchIterator it = setFunctionNameRe.globalMatch(code);
    int setFunctionNameIndex = 0;
    while (it.hasNext())
    {
        const QString name = it.next().captured(1);
        const QString bareName = name.startsWith("ai/", Qt::CaseInsensitive) ? name.mid(3) : name;
        const bool withinInventedAllowance = (setFunctionNameIndex++ < inventedNamesAllowed);
        if ((!withinInventedAllowance || descriptionNamesAFunction) &&
            !partDescription.contains(name, Qt::CaseInsensitive) &&
            !partDescription.contains(bareName, Qt::CaseInsensitive))
            violations << "This part's own job (\"" + partDescription +
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
            violations << "This part's own job (\"" + partDescription +
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
    // A whole request that quotes no name leaves the name to the script: the rule enforces a name "already decided
    // for it", and no later part refers to it. Confirmed live: "make a 3D plot with 3 spheres ..." (no name, not
    // split) blocked a correct plot named "ThreeSpheres" ("three" is not "3"), and the retries drifted into a broken
    // one. A split part still needs its name: later parts plot into it.
    static const QRegularExpression quotedNameRe(R"(["'][^"'\n]{1,80}["'])");
    QRegularExpressionMatchIterator windowIt = (wholeRequest && !quotedNameRe.match(partDescription).hasMatch())
                                                   ? QRegularExpressionMatchIterator()
                                                   : windowCreateRe.globalMatch(code);
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
        // CamelCase counts as a word boundary: the rule's own intent is that a name "trace back to
        // the description, just reordered", and "SinCosSurface" does — Sin, Cos and Surface are all
        // in "Plot a 3D surface of sin(x)*cos(y)". Without the split it was one opaque token
        // (`sincossurface`), matched nothing, and charged a round-trip at the same item in three
        // consecutive runs before the model fell back to the literal word "surface". Unrelated
        // names are still rejected: every piece must be present, so "ScatterPlot" against this
        // description still fails on `scatter`/`plot`... and on `plot` it would pass only because
        // the description really does say "Plot", which is the intended semantics.
        static const QRegularExpression tokenRe(R"([A-Z][a-z]+|[A-Za-z]+|[0-9]+)");
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
        violations << "This part's own job (\"" + partDescription +
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
                violations << "This part configures against \"" + configuredName +
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
                violations << "This part references \"fitCurve-" + referencedRaw +
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
            violations << "This part creates a 3D surface via plot3D(...) but never calls "
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
    // confirms a Table, existMatrix() a Matrix) — confirmed live, twice, complementary shapes:
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
                violations << "This part calls matrix(\"" + name + "\"), but \"" + name +
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
                violations << "This part calls existTable(\"" + name + "\")/table(\"" + name + "\"), but \"" + name +
                       "\" was created via newMatrix(...) — a Matrix, not a Table. "
                       "existTable() is strictly type-specific and always returns False for a Matrix "
                                  "name — any logic gated behind this check silently never runs, with no error. "
                                  "Use existMatrix(\"" +
                                  name + "\") to check a Matrix's existence instead.";
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
                violations << "This part looks up an EXISTING object via matrix()/table()/note()/"
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
        // Where this script itself first creates a Graph3D of that name: a 2D lookup BEFORE it is about the window that
        // is there now — a 2D graph of the same name, closed so the 3D one can take the name. Confirmed live (Codex
        // BIGMODEL, "Plot it as a 3D surface instead"): `if existGraph("MetricsGraph"): closeWindow(graphWindow(...))`
        // then newPlot3D("MetricsGraph", ...) was blocked four times — MetricsGraph was a 2D graph — and the item was
        // lost.
        QHash<QString, qsizetype> ownNewPlot3DAt;
        for (auto it = newPlot3DNameRe.globalMatch(code); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            if (!ownNewPlot3DAt.contains(m.captured(1)))
                ownNewPlot3DAt.insert(m.captured(1), m.capturedStart());
        }
        const bool namedEarlier = earlierPartsCode.contains("newPlot3D") || earlierPartsCode.contains("setWindowName");

        if (!graph3DNames.isEmpty())
        {
            // existGraph() too: confirmed live (benchmark LIST 5, item 4, twice), existGraph("Saddle3D") right after
            // newPlot3D("Saddle3D", ...) printed False — it only knows 2D graph windows.
            static const QRegularExpression graphWindowLookupRe(
                R"re(\b(graphWindow|existGraph)\s*\(\s*["']([^"']+)["'])re");
            QRegularExpressionMatchIterator gwIt = graphWindowLookupRe.globalMatch(code);
            while (gwIt.hasNext())
            {
                const QRegularExpressionMatch lookup = gwIt.next();
                const QString call = lookup.captured(1);
                const QString name = lookup.captured(2);
                if (!namedEarlier && ownNewPlot3DAt.contains(name) &&
                    lookup.capturedStart() < ownNewPlot3DAt.value(name))
                    continue;
                if (graph3DNames.contains(name))
                    violations << "This part calls " + call + "(\"" + name + "\"), but \"" + name +
                                      "\" is a Graph3D window (named via setWindowName() after plot3D(...), "
                                      "or newPlot3D(...)) — " +
                                      call +
                                      "() is 2D-GraphWindow-ONLY and always "
                                      "returns " +
                                      (call == "existGraph" ? "False" : "None") +
                                      " for a Graph3D name. "
                                      "Use plot3D(\"" +
                                      name + "\") to look up this window instead" +
                                      (call == "existGraph"
                                           ? " — or existPlot3D(\"" + name + "\") to test whether it exists."
                                           : ".");
            }
        }
    }

    // Direction 12 — UNDERREACH via plot (mirror image of Direction 1, same failure class as
    // Directions 2/4): a part whose own description explicitly requires PLOTTING data/a curve/
    // residuals into a graph instead only re-derives or re-verifies a fact from an earlier part
    // (e.g. re-reading fitCurve-<model> chi2/dof to confirm which model won) and stops there,
    // never calling any graph-creating/curve-adding function. Confirmed live: a part described as
    // "plot winning model's data, fit curve, and residuals into graph 'GaussPeakFit' with two
    // stacked panels, styled legend, and axis titles" ran a script that only re-read chi2/dof from
    // both fitCurve tables and printed the winner (again — part 3 already had) — zero graph API
    // calls despite "graph" being in this part's own DOCS. runActionScript()'s only success gate
    // is "no exception raised," so this counted as "part done," split-mode moved straight to
    // requesting the merged script, and the merge step then fabricated an entirely fictional
    // graph-plotting API (setStackedPanels(), .panel(), setDataCurve(), setLegendOn(), ...) out of
    // nothing to paper over a part that never actually plotted anything. Deliberately requires
    // BOTH a plot-verb AND a data/curve/residual noun in the description (not just Direction 1's
    // broader plot/graph/chart/surface keyword alone) — a legitimate styling-only part like "add a
    // styled legend and axis titles to graph 'X'" mentions "graph" but not "data"/"curve"/
    // "residual", and correctly has no reason to call any of these functions itself.
    static const QRegularExpression plotDataVerbRe(R"(\bplot\w*\b.*\b(data|curve|residual)\w*\b)",
                                                   QRegularExpression::CaseInsensitiveOption);
    if (plotDataVerbRe.match(partDescription).hasMatch())
    {
        static const QRegularExpression graphActionRe(
            R"(\b(newGraph|plotResults|newPlot3D|plot3D|insertCurve|addCurve)\s*\(|\bplot\s*\()");
        if (!graphActionRe.match(code).hasMatch())
            violations << "This part's own job (\"" + partDescription +
                              "\") explicitly requires PLOTTING data/a curve/residuals into a graph, but the "
                              "script never calls newGraph()/plot()/plotResults()/insertCurve()/addCurve() "
                              "anywhere — it only re-derives or re-prints a fact (e.g. re-reading chi2/dof to "
                              "confirm the winner) as if plotting were a different part's job or already done. "
                              "No part in this plan does that on this part's behalf. Actually build the graph "
                              "here: look up or create it by name, add the required curve(s), and apply the "
                              "requested styling — do not stop after just re-confirming which model won.";
    }

    // Direction 13 — UNDERREACH via 3D surface plot (mirror image of Direction 1, same failure
    // class as Direction 12 above — but Direction 12's own "data/curve/residual" noun requirement
    // deliberately excludes a pure formula-based 3D surface like "plot 3D surface of
    // sin(x)*cos(y)...", which has none of those nouns; that narrowness is correct for Direction
    // 12's own false-positive concern (a styling-only part mentions "graph" but not those nouns)
    // but leaves this shape uncovered). Confirmed live: a part described "plot 3D surface of
    // sin(x)*cos(y) ... using filled mesh style" was correctly blocked once for prematurely
    // adding legend/axis styling (Direction 1's own overreach check), and the retry that followed
    // didn't produce a corrected surface-plotting script — it abandoned plotting entirely and
    // called newNote("MyScript") instead, something with zero relation to graphing at all. That
    // ran without error and was accepted as "part done" — with nothing else in the pipeline
    // positioned to catch it, since none of Directions 1-12 match a script that does something
    // unrelated rather than a recognizable plotting mistake. The actual absence of any 3D surface
    // only surfaced later, when the NEXT part's plot3D(name) lookup returned None.
    static const QRegularExpression plotSurfaceVerbRe(R"(\bplot\w*\b.*\b(surface|mesh)\w*\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
    if (plotSurfaceVerbRe.match(partDescription).hasMatch())
    {
        static const QRegularExpression surfaceCreateRe(R"(\b(newPlot3D|plot3D)\s*\()");
        if (!surfaceCreateRe.match(code).hasMatch())
            violations << "This part's own job (\"" + partDescription +
                              "\") explicitly requires PLOTTING a 3D surface, but the script never calls "
                              "newPlot3D()/plot3D() anywhere — it does something unrelated instead. If an "
                              "earlier attempt at this part was blocked for a different reason, fix THAT "
                              "specific issue in the surface-plotting script itself; do not abandon plotting "
                              "and do something else.";
    }

    // Direction 14 — UNDERREACH via simulate (mirror image of Direction 2, same failure class):
    // a part whose own description explicitly requires PERFORMING a Fittable simulation (e.g.
    // "simulate a curve from the compiled ... function") instead does something ENTIRELY
    // unrelated — confirmed live, severe: a part described only as "simulate a curve from the
    // compiled Porod power-law function named 'ai/PorodFit'" (no plotting, no styling mentioned
    // anywhere in it) instead opened an EXISTING graph window by name and set its title/axis
    // labels/log-log scale — 100% graph-styling code, zero Fittable calls, not even a botched
    // attempt at simulating anything. That graph didn't even exist yet at this point in the plan
    // (a LATER part's own job was to create it), so the only reason this ran without immediately
    // erroring is that the script's own "if gw is None: scriptPrint(...)" branch caught the
    // lookup failure and printed a message instead of raising — still accepted as "part done"
    // with the actual simulation never having happened at all.
    //
    // "simulate" (present-tense imperative, this part's OWN job) must be distinguished from
    // "simulated" (past-tense adjective describing an object an EARLIER part already produced,
    // e.g. "plot the simulated Porod curve") — the fitVerbRe precedent above already established
    // this exact verb-vs-adjective distinction is necessary for "fit"/"fitted"; matches only
    // "simulate(s)" directly followed by its typical object, not the "-d" participle form.
    static const QRegularExpression simulateVerbRe(R"(\bsimulate[s]?\s+(?:a\s+|the\s+)?curves?\b)",
                                                   QRegularExpression::CaseInsensitiveOption);
    if (simulateVerbRe.match(partDescription).hasMatch())
    {
        static const QRegularExpression simulateCallRe(
            R"(\bsimulateCurve\s*\(|selectStage\s*\(\s*["']simulate["']|\.\s*simulate\s*\()");
        if (!simulateCallRe.match(code).hasMatch())
            violations << "This part's own job (\"" + partDescription +
                              "\") explicitly requires PERFORMING a simulation (Fittable.simulateCurve()), "
                              "but the script never calls selectStage(\"simulate\")/simulateCurve() "
                              "anywhere — it does something unrelated instead. If an earlier attempt at "
                              "this part was blocked for a different reason, fix THAT specific issue in the "
                              "simulate script itself; do not abandon simulating and do something else "
                              "(e.g. styling a graph another part is responsible for creating).";
    }

    // Direction 15 — cross-part variable leakage: a part references a name that was only ever
    // ASSIGNED in an EARLIER part's own script, never in this part's own — split-mode scripts run
    // as fully separate executions (ai-python-api.md's own "Fresh execution per round" rule), so
    // nothing a prior part bound to a local variable survives into this one. Confirmed live: part
    // 3 computed `best_fc`/`best_name` (which of two fits won); part 4 referenced both directly
    // with no local assignment of either, hitting `NameError: name 'best_fc' is not defined`
    // identically across multiple retries (the model reordered surrounding code each time without
    // ever actually defining the name) before the harness gave up. Deliberately scoped to
    // DISTINCTIVE names (5+ characters, past the common short aliases below) — `t`/`f`/`c`/`g`/
    // `l1`/`l2`/`m`/`i` are routinely and CORRECTLY reassigned fresh in nearly every part; flagging
    // those would be pure noise, not a real instance of this mistake.
    {
        static const QRegularExpression assignRe(R"((?:\A|\n)\s*(\w{5,})\s*=(?!=))");
        static const QSet<QString> commonAliasExclusions = {"table", "graph", "layer", "score", "count",
                                                            "value", "index", "names", "title"};
        QSet<QString> earlierNames;
        QRegularExpressionMatchIterator eIt = assignRe.globalMatch(earlierPartsCode);
        while (eIt.hasNext())
        {
            const QString name = eIt.next().captured(1);
            if (!commonAliasExclusions.contains(name))
                earlierNames << name;
        }
        // Matched against the code with string literals blanked out: a name inside a quoted string
        // is not a Python variable reference. Confirmed live: "sigma", a fit-parameter name in
        // setParamNamesAll([...]) and a boilerplate-declared C++ identifier inside setCode(), was
        // flagged as leaked from an earlier part's Python `sigma = 1.0` — unsatisfiable, since the
        // parameter is legitimately named sigma, so it blocked twice and ended the whole run.
        const QString pythonCode = codeWithoutStringLiterals(code);
        for (const QString &name : std::as_const(earlierNames))
        {
            const QRegularExpression usedRe(R"(\b)" + QRegularExpression::escape(name) + R"(\b)");
            if (!usedRe.match(pythonCode).hasMatch())
                continue;
            const QRegularExpression assignedHereRe(R"((?:\A|\n)\s*)" + QRegularExpression::escape(name) +
                                                    R"(\s*=(?!=))");
            if (assignedHereRe.match(pythonCode).hasMatch())
                continue;
            violations << "This part references \"" + name +
                              "\", but that name is only ever ASSIGNED in an EARLIER part's script, never in "
                              "this part's own. Split-mode parts run as fully separate executions — nothing "
                              "an earlier part bound to a local variable is still bound here, even though its "
                              "PRINTED output does carry forward. Re-derive \"" +
                              name +
                              "\" in this part's own code (e.g. look up the object it refers to by name, or "
                              "re-read a value this part's own script already printed) instead of assuming "
                              "it is still bound.";
        }
    }

    if (violations.isEmpty())
        return {};
    if (violations.size() == 1)
        return violations.first();
    QStringList numbered;
    for (int i = 0; i < violations.size(); ++i)
        numbered << QString("%1. %2").arg(i + 1).arg(violations.at(i));
    return "This part's script breaks " + QString::number(violations.size()) +
           " separate scope rules — fix ALL of them together in one corrected script:\n\n" + numbered.join("\n\n");
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
        R"(\b(existTable|existNote|existMatrix|existGraph|existWindow|graphWindow|matrix|fitFunctions|colNames|)"
        R"(datasets)\s*\()");
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

QString QtiSasAiContent::fabricatedConversationCheck(const QString &code) const
{
    return fabricatedConversationMistake(code);
}

QString QtiSasAiContent::fabricatedConversationMistake(const QString &code)
{
    // A generated script that role-plays the rest of the exchange: "USER:"/"ASSISTANT:" turns of
    // its own invention, complete with prompt boilerplate and made-up tool output. Confirmed live
    // three times, worst case one part's script followed by fabricated parts 3-5 — and then
    // fabricated parts 3-5 including error messages it lifted from an unrelated earlier fit
    // (sasviewmodels/sphere's parameter list, reused as a fake error for ai/Gaussian). Only ever
    // caught incidentally, as a syntax error, which told the model nothing about the real problem.
    static const QRegularExpression roleMarkerRe(R"(^[ \t]*(USER|ASSISTANT)[ \t]*:)",
                                                 QRegularExpression::MultilineOption);
    const QRegularExpressionMatch m = roleMarkerRe.match(codeWithoutStringLiterals(code));
    if (!m.hasMatch())
        return {};
    return "This script contains a \"" + m.captured(1) +
           ":\" line — you continued the conversation yourself instead of answering this one turn. "
           "Everything after that marker is invented: prompts you were never sent, scripts that "
           "never ran, and output/errors that were never produced (they are typically borrowed from "
           "an unrelated earlier step, so they look real but describe a different object). Return "
           "ONLY the Python script for the CURRENT request, ending at its last line — no "
           "\"USER:\"/\"ASSISTANT:\" turns, no simulated results. If you need real output before "
           "continuing, return a \"# EXPLORE\" script and stop; QtiSAS will run it and send back "
           "the actual output.";
}

QString QtiSasAiContent::codeWithLiteralsBlanked(const QString &code)
{
    // Same literals as codeWithoutStringLiterals(), each replaced by spaces of its own length: offsets stay valid.
    static const QRegularExpression literalRe(
        R"RX("""[\s\S]*?"""|'''[\s\S]*?'''|"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')RX");
    QString out = code;
    for (auto it = literalRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        for (qsizetype i = m.capturedStart(); i < m.capturedEnd(); ++i)
            if (out.at(i) != QLatin1Char('\n'))
                out[static_cast<int>(i)] = QLatin1Char(' ');
    }
    return out;
}

QString QtiSasAiContent::codeWithoutStringLiterals(const QString &code)
{
    // Triple-quoted first, so a setCode("""...""") body is removed whole rather than leaving its
    // interior exposed by the single-quote pass.
    static const QRegularExpression tripleRe(R"RX("""[\s\S]*?"""|'''[\s\S]*?''')RX");
    static const QRegularExpression singleRe(R"RX("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')RX");
    QString out = code;
    out.replace(tripleRe, " ");
    out.replace(singleRe, " ");
    return out;
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
    // Every self-shadowed name, not just the first — the aliasing pattern this comes from
    // (center->x0, width->sigma, scale->A) is applied per parameter, so a body that gets one wrong
    // can get several wrong the same way.
    QStringList names;
    QRegularExpressionMatchIterator shadowIt = selfShadowRe.globalMatch(code);
    while (shadowIt.hasNext())
    {
        const QString n = shadowIt.next().captured(1);
        if (!names.contains(n))
            names << n;
    }
    if (names.isEmpty())
        return {};
    const QString name = names.join("\", \"");
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

QStringList QtiSasAiContent::compiledCodeBodies(const QString &code) const
{
    static const QRegularExpression setCodeRe(
        R"RX(setCode\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)'))RX");
    QStringList bodies;
    for (auto it = setCodeRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        for (int g = 1; g <= 4; ++g)
            if (!m.captured(g).trimmed().isEmpty())
            {
                bodies << m.captured(g);
                break;
            }
    }
    return bodies;
}

QString QtiSasAiContent::compilerParamIndexRepair(const QString &code) const
{
    static const QRegularExpression namesRe(R"RX(setParamNamesAll\s*\(\s*\[([^\]]*)\])RX");
    const QRegularExpressionMatch names = namesRe.match(code);
    if (!names.hasMatch())
        return {};
    QStringList paramNames;
    static const QRegularExpression quotedRe(R"RX(["']([^"']+)["'])RX");
    for (auto it = quotedRe.globalMatch(names.captured(1)); it.hasNext();)
        paramNames << it.next().captured(1);
    if (paramNames.isEmpty())
        return {};
    static const QRegularExpression setCodeRe(
        R"RX(setCode\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)'))RX");
    static const QRegularExpression indexRe(R"(\b(param|params|par|pars|parameters|p)\s*\[\s*(\d+)\s*\])");
    static const QRegularExpression ownArrayRe(R"(\b(?:double|float|int|auto)\s+(\w+)\s*\[)");
    QString repaired = code;
    bool any = false;
    QList<QRegularExpressionMatch> calls;
    for (auto it = setCodeRe.globalMatch(code); it.hasNext();)
        calls << it.next();
    for (int c = static_cast<int>(calls.size()) - 1; c >= 0; --c) // back to front: offsets stay valid
    {
        int group = 0;
        for (int g = 1; g <= 4 && group == 0; ++g)
            if (!calls[c].captured(g).isEmpty())
                group = g;
        if (group == 0)
            continue;
        QString body = calls[c].captured(group);
        QStringList ownArrays; // an array the body declares itself is not a parameter list
        for (auto it = ownArrayRe.globalMatch(body); it.hasNext();)
            ownArrays << it.next().captured(1);
        QList<QRegularExpressionMatch> uses;
        for (auto it = indexRe.globalMatch(body); it.hasNext();)
            uses << it.next();
        bool changed = false;
        for (int u = static_cast<int>(uses.size()) - 1; u >= 0; --u)
        {
            const int k = uses[u].captured(2).toInt();
            if (ownArrays.contains(uses[u].captured(1)) || k < 0 || k >= paramNames.size())
                continue;
            body.replace(uses[u].capturedStart(), uses[u].capturedLength(), paramNames.at(k));
            changed = true;
        }
        if (!changed)
            continue;
        repaired.replace(calls[c].capturedStart(group), calls[c].capturedLength(group), body);
        any = true;
    }
    return any ? repaired : QString();
}

QString QtiSasAiContent::compilerParamVariesRepair(const QString &code, const QString &request) const
{
    static const QRegularExpression fixedRe(R"(\b(?:fix|fixed|fixing|hold|held|frozen)\b)",
                                            QRegularExpression::CaseInsensitiveOption);
    if (fixedRe.match(request).hasMatch()) // the request decides which parameters stay fixed
        return {};
    // A compile() inside a condition counts too: confirmed live (all-pyi BIGMODEL, ai-test-requests item 12), both
    // functions were compiled as "if not c.compile(): raise ...", the repair saw no compile(), and fit() failed with
    // "no adjustable parameters" — one more retry for a fix this repair exists to make.
    static const QRegularExpression compileRe(
        R"(^([ \t]*)(?:(?:if\s+not\s+|if\s+|assert\s+|\w+\s*=\s*))?(\w+)\.compile\s*\(\s*\))",
        QRegularExpression::MultilineOption);
    static const QRegularExpression startRe(R"(\.(?:new|compile)\s*\(\s*\))");
    static const QRegularExpression namesRe(R"RX(setParamNamesAll\s*\(\s*\[([^\]]*)\])RX");
    static const QRegularExpression quotedRe(R"RX(["']([^"']+)["'])RX");
    static const QRegularExpression countRe(R"(setParamCount\s*\(\s*(\d+)\s*\))");
    QList<QRegularExpressionMatch> compiles;
    for (auto it = compileRe.globalMatch(code); it.hasNext();)
        compiles << it.next();
    QString repaired = code;
    bool any = false;
    for (int c = static_cast<int>(compiles.size()) - 1; c >= 0; --c) // back to front: offsets stay valid
    {
        const qsizetype end = compiles[c].capturedStart();
        // this function's own definition: from its c.new() (or the previous compile()) to this compile()
        qsizetype begin = 0;
        for (auto it = startRe.globalMatch(code); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            if (m.capturedStart() >= end)
                break;
            begin = m.capturedEnd();
        }
        const QString definition = code.mid(begin, end - begin);
        if (definition.contains("setParamVaries"))
            continue;
        qsizetype count = 0;
        if (const QRegularExpressionMatch names = namesRe.match(definition); names.hasMatch())
            for (auto it = quotedRe.globalMatch(names.captured(1)); it.hasNext(); it.next())
                ++count;
        if (count == 0)
            if (const QRegularExpressionMatch n = countRe.match(definition); n.hasMatch())
                count = n.captured(1).toInt();
        if (count <= 0)
            continue;
        QStringList trues;
        for (qsizetype i = 0; i < count; ++i)
            trues << "True";
        repaired.insert(end, compiles[c].captured(1) + compiles[c].captured(2) + ".setParamVariesAll([" +
                                 trues.join(", ") + "])\n");
        any = true;
    }
    return any ? repaired : QString();
}

QString QtiSasAiContent::compilerSelfShadowRepair(const QString &code) const
{
    // Only C++ declares "double name = name;", so the whole script can be searched: in Python it does not parse.
    static const QRegularExpression selfShadowRe(R"([ \t]*\b(?:double|float|int)\s+(\w+)\s*=\s*\1\s*;[ \t]*\n?)");
    QString repaired = code;
    repaired.remove(selfShadowRe);
    return repaired == code ? QString() : repaired;
}

// The yName of the function `code` defines: its setYName() literal, or "f" when it starts a new function with c.new()
// and never sets one (after new() the variables are x = "x" and y = "f", compile-python-api.md RULE 7). Empty when
// unknown — after c.load()/open() of an existing function. Confirmed live (LIST 5 NOSPLIT, item 1): no setYName(),
// "I = a*x*x + b*x + c;", clang's "undeclared identifier 'I'" cost a round the repair could have saved.
static QString scriptYName(const QString &code)
{
    static const QRegularExpression yNameRe(R"(setYName\s*\(\s*["']([^"']+)["'])");
    if (const QRegularExpressionMatch m = yNameRe.match(code); m.hasMatch())
        return m.captured(1);
    static const QRegularExpression newFunctionRe(R"(\.new\s*\(\s*\))");
    return newFunctionRe.match(code).hasMatch() ? QStringLiteral("f") : QString();
}

QString QtiSasAiContent::compilerYNameAssignmentRepair(const QString &code) const
{
    // EVERY setCode() in the script, not just the first: confirmed live, a part compiling two
    // functions had its first body repaired and the second left with "double I = ..." intact.
    static const QRegularExpression setCodeRe(
        R"RX(setCode\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)'))RX");
    QList<QRegularExpressionMatch> codeCalls;
    QRegularExpressionMatchIterator codeIt = setCodeRe.globalMatch(code);
    while (codeIt.hasNext())
        codeCalls << codeIt.next();
    if (codeCalls.isEmpty())
        return {};

    // Only when this script states the yName itself, or starts a new function — after c.load()/open() it is unknown.
    const QString yName = scriptYName(code);
    if (yName.isEmpty())
        return {};
    // Names the generated function already provides: assigning one of them is not a misnamed result.
    QStringList reservedNames;
    static const QRegularExpression xNameRe(R"(setXName\s*\(\s*["']([^"']+)["'])");
    if (const QRegularExpressionMatch xMatch = xNameRe.match(code); xMatch.hasMatch())
        reservedNames << xMatch.captured(1);
    else
        reservedNames << QStringLiteral("x");
    static const QRegularExpression paramNamesRe(R"(setParamNamesAll\s*\(\s*\[([^\]]*)\])");
    static const QRegularExpression quotedRe(R"(["']([^"']+)["'])");
    for (auto it = paramNamesRe.globalMatch(code); it.hasNext();)
    {
        const QString list = it.next().captured(1);
        for (auto q = quotedRe.globalMatch(list); q.hasNext();)
            reservedNames << q.next().captured(1);
    }

    // Rebuilt back-to-front so each splice cannot shift the offsets of the ones not yet applied.
    QString repaired = code;
    bool anyRepair = false;
    for (int i = static_cast<int>(codeCalls.size() - 1); i >= 0; --i)
    {
        const QRegularExpressionMatch &codeCall = codeCalls[i];
        int bodyGroup = 0;
        for (int g = 1; g <= 4 && bodyGroup == 0; ++g)
            if (!codeCall.captured(g).isEmpty())
                bodyGroup = g;
        if (bodyGroup == 0)
            continue;
        const QString body = codeCall.captured(bodyGroup);
        QString fixedBody = repairedSetCodeBody(body, yName);
        if (fixedBody.isEmpty())
            fixedBody = renamedResultAssignment(body, yName, reservedNames);
        if (fixedBody.isEmpty())
            continue;
        repaired.replace(codeCall.capturedStart(bodyGroup), codeCall.capturedLength(bodyGroup), fixedBody);
        anyRepair = true;
    }
    return anyRepair ? repaired : QString();
}

// Returns `body` with its lone declaration turned into an assignment to yName, or empty when there
// is nothing to repair here — the per-body half of compilerYNameAssignmentRepair() above.
QString QtiSasAiContent::repairedSetCodeBody(const QString &body, const QString &yName)
{
    // Exactly one declaration, or the intended result variable is a guess. Its declared name may
    // be the yName itself ("double I = ..." — clang's redefinition) or anything else ("double y =
    // ..." — silently never assigns yName); both repair to the same assignment.
    static const QRegularExpression declRe(
        R"(\b(?:double|float|int|long|short|unsigned|signed|char|bool|auto)\s+([A-Za-z_]\w*)\s*=\s*([^;]+);)");
    QRegularExpressionMatchIterator it = declRe.globalMatch(body);
    if (!it.hasNext())
        return {};
    const QRegularExpressionMatch decl = it.next();
    if (it.hasNext())
        return {};
    const QString declaredName = decl.captured(1);

    // A declaration whose name is NOT the yName is only the result variable if the body never
    // assigns yName some other way, and if nothing else reads that local.
    const QRegularExpression assignRe(R"(\b)" + QRegularExpression::escape(yName) + R"(\s*(?:[+\-*/]?)=(?!=))");
    if (declaredName != yName && assignRe.match(body).hasMatch())
        return {};
    const QRegularExpression usesDeclaredRe(R"(\b)" + QRegularExpression::escape(declaredName) + R"(\b)");
    int uses = 0;
    QRegularExpressionMatchIterator useIt = usesDeclaredRe.globalMatch(body);
    while (useIt.hasNext())
    {
        useIt.next();
        ++uses;
    }
    if (uses != 1)
        return {};

    QString repairedBody = body;
    repairedBody.replace(decl.capturedStart(), decl.capturedLength(), yName + " = " + decl.captured(2).trimmed() + ";");
    return repairedBody;
}

// Returns `body` with its one assignment to an undeclared name renamed to yName, or empty when the body
// already assigns yName, or when which name holds the result is a guess. Confirmed live (LIST 5, items 1 and 2,
// local model): setYName("y") with "I = a*x*x + b*x + c;" (I is the default yName) was blocked as "never assigns
// y" and cost a full round each time.
// `body` with the line breaks a one-line Python string writes as "\n" / "\t" made real, for word-boundary matching:
// in "x;\nI = ..." the "n" before "I" is a word character, so \bI did not match.
static QString withWrittenBreaks(const QString &body)
{
    QString text = body;
    text.replace(QLatin1String("\\n"), QLatin1String("\n"));
    text.replace(QLatin1String("\\t"), QLatin1String("\t"));
    return text;
}

QString QtiSasAiContent::renamedResultAssignment(const QString &body, const QString &yName,
                                                 const QStringList &reservedNames)
{
    const QRegularExpression yAssignRe(R"(\b)" + QRegularExpression::escape(yName) + R"(\s*(?:[+\-*/]?)=(?!=))");
    if (yAssignRe.match(body).hasMatch())
        return {};
    // Statement-start assignments only: "double t = ..." starts with its type, "for (i = 0; ..." with "(".
    // A body written in a one-line Python string carries its line breaks as the two characters "\n" (or "\t"):
    // confirmed live, "double x2 = x * x;\nI = a*x2 + b*x + c;" was never repaired because "I" did not start a
    // statement.
    static const QRegularExpression assignRe(R"((?:^|[;{}\n]|\\[nt])\s*([A-Za-z_]\w*)\s*=(?!=))");
    static const QRegularExpression declRe(
        R"(\b(?:double|float|int|long|short|unsigned|signed|char|bool|auto)\s+([A-Za-z_]\w*))");
    QStringList declared;
    for (auto it = declRe.globalMatch(body); it.hasNext();)
        declared << it.next().captured(1);
    QList<QRegularExpressionMatch> candidates;
    for (auto it = assignRe.globalMatch(body); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1);
        if (!declared.contains(name) && !reservedNames.contains(name))
            candidates << m;
    }
    if (candidates.size() != 1)
        return {};
    // Assigned once and never read: anything else and the name is more than a misnamed result.
    const QString name = candidates.first().captured(1);
    const QRegularExpression usesRe(R"(\b)" + QRegularExpression::escape(name) + R"(\b)");
    int uses = 0;
    for (auto it = usesRe.globalMatch(withWrittenBreaks(body)); it.hasNext(); it.next())
        ++uses;
    if (uses != 1)
        return {};
    QString repaired = body;
    repaired.replace(candidates.first().capturedStart(1), candidates.first().capturedLength(1), yName);
    return repaired;
}

QString QtiSasAiContent::compilerParamCountRepair(const QString &code) const
{
    static const QRegularExpression countRe(R"(setParamCount\s*\(\s*(\d+)\s*\))");
    const QRegularExpressionMatch countMatch = countRe.match(code);
    if (!countMatch.hasMatch())
        return {};
    const int declaredCount = countMatch.captured(1).toInt();

    // Every setParam*All() list present must agree on a length before rewriting the count — if two
    // lists differ, which one reflects the intent is a guess, so leave it to the runtime error.
    static const QRegularExpression listRe(R"(setParam(?:Names|Inits|Limits|Varies|Descrs)All\s*\(\s*\[([^\]]*)\])");
    static const QRegularExpression itemRe(
        R"(["'][^"']*["']|\btrue\b|\bfalse\b|\bTrue\b|\bFalse\b|-?[\d.]+(?:[eE][-+]?\d+)?)");
    int agreedLength = -1;
    QRegularExpressionMatchIterator it = listRe.globalMatch(code);
    while (it.hasNext())
    {
        const QString inner = it.next().captured(1);
        int items = 0;
        QRegularExpressionMatchIterator itemIt = itemRe.globalMatch(inner);
        while (itemIt.hasNext())
        {
            itemIt.next();
            ++items;
        }
        if (items == 0)
            return {};
        if (agreedLength < 0)
            agreedLength = items;
        else if (agreedLength != items)
            return {};
    }
    if (agreedLength < 0 || agreedLength == declaredCount)
        return {};

    QString repaired = code;
    repaired.replace(countMatch.capturedStart(), countMatch.capturedLength(),
                     "setParamCount(" + QString::number(agreedLength) + ")");
    return repaired;
}

QString QtiSasAiContent::compilerYNameNeverAssignedMistake(const QString &code)
{
    // The boilerplate returns the yName local, so a body that never assigns it returns an
    // uninitialized value — and this compiles CLEANLY, so nothing downstream catches it.
    // Confirmed live: after clang rejected "double I = ..." with "redefinition of 'I'", the fix
    // attempt renamed the variable to "double y = ..." — no redefinition, no error, "compiled
    // successfully" printed, and I never assigned. Renaming escapes the redeclaration check, so
    // this one (assignment to yName present or not) is what actually pins the requirement.
    static const QRegularExpression setCodeRe(
        R"RX(setCode\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)'))RX");
    // Every setCode() body, not just the first (see compilerYNameAssignmentRepair()'s own note).
    QStringList bodies;
    QRegularExpressionMatchIterator codeIt = setCodeRe.globalMatch(code);
    while (codeIt.hasNext())
    {
        const QRegularExpressionMatch m = codeIt.next();
        for (int g = 1; g <= 4; ++g)
            if (!m.captured(g).trimmed().isEmpty())
            {
                bodies << m.captured(g);
                break;
            }
    }
    if (bodies.isEmpty())
        return {};

    // Only when this script states the yName itself or starts a new function — after c.load()/open() of an existing
    // function the real yName is unknown here, so staying silent beats a false block.
    const QString yName = scriptYName(code);
    if (yName.isEmpty())
        return {};

    // Each body separately: one function assigning yName must not excuse another that doesn't.
    // Confirmed live, a part compiling two functions had only its first body checked.
    const QRegularExpression assignRe(R"(\b)" + QRegularExpression::escape(yName) + R"(\s*(?:[+\-*/]?)=(?!=))");
    bool allAssign = true;
    for (const QString &b : bodies)
        if (!assignRe.match(withWrittenBreaks(b)).hasMatch())
            allAssign = false;
    if (allAssign)
        return {};

    // Without a setYName() call "f" is the c.new() default — the message must not claim a call that is not there.
    // Confirmed live: "set by setYName(\"f\")" for a script with no setYName() at all, whose body assigned "I".
    static const QRegularExpression setYNameRe(R"(\bsetYName\s*\()");
    const QString origin =
        setYNameRe.match(code).hasMatch()
            ? "set by setYName(\"" + yName + "\")"
            : "\"" + yName +
                  "\" after c.new(), since no setYName() sets another — or call "
                  "c.setYName(...) first to use your own name (\"y\" for a request written as y = ...)";
    return "The setCode() body never assigns to \"" + yName + "\", the dependent variable " + origin +
           ". The generated function ends with \"return " + yName +
           ";\", so as written it returns an UNINITIALIZED value on every evaluation — and this "
           "compiles cleanly, with no error to warn you. Assign the result to \"" +
           yName + "\" itself: write \"" + yName + " = ...;\". Do NOT declare it (no \"double " + yName +
           "\" — the boilerplate already declares it), and do NOT rename it to something else: a "
           "different name is just an unused local, which is exactly this bug.";
}

QString QtiSasAiContent::compilerRedeclaredReservedNameMistake(const QString &code)
{
    // A setCode() body that DECLARES a name the generated boilerplate already provides. Grounded
    // in the real generator (compile-explorer.cpp makeCPP()): yName is a local `double <yName>;`
    // inside functionSANS, so redeclaring it is a hard "error: redefinition" — confirmed live with
    // setYName("I") + `double I = scale * pow(x, -exponent) + background;`. xName, the parameter
    // names and initNow are file-scope `thread_local static`s and key/ParaM are function
    // parameters, so declaring those inside the body SILENTLY shadows the real value with no
    // compiler error at all — the worse variant, and the reason this can't be left to the compiler.
    static const QRegularExpression setCodeRe(
        R"RX(setCode\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)'))RX");
    // Every setCode() body, not just the first (see compilerYNameAssignmentRepair()'s own note).
    QStringList bodies;
    QRegularExpressionMatchIterator codeIt = setCodeRe.globalMatch(code);
    while (codeIt.hasNext())
    {
        const QRegularExpressionMatch m = codeIt.next();
        for (int g = 1; g <= 4; ++g)
            if (!m.captured(g).trimmed().isEmpty())
            {
                bodies << m.captured(g);
                break;
            }
    }
    if (bodies.isEmpty())
        return {};
    const QString body = bodies.join("\n");

    // Reserved names are read from THIS script's own setters, never hardcoded — yName is routinely
    // "I", "y", "Intensity", ... and the parameter names are arbitrary. An absent setter falls back
    // to the documented unset default; a name this script never states stays unknown, which only
    // costs a missed detection (fail-open) instead of risking a false block on a valid script.
    static const QRegularExpression yNameRe(R"(setYName\s*\(\s*["']([^"']+)["'])");
    static const QRegularExpression xNameRe(R"(setXName\s*\(\s*["']([^"']+)["'])");
    const QRegularExpressionMatch yMatch = yNameRe.match(code);
    const QRegularExpressionMatch xMatch = xNameRe.match(code);
    const QString yName = yMatch.hasMatch() ? yMatch.captured(1) : QString("f");
    const QString xName = xMatch.hasMatch() ? xMatch.captured(1) : QString("x");

    QStringList paramNames;
    static const QRegularExpression paramsAllRe(R"(setParamNamesAll\s*\(\s*\[([^\]]*)\])");
    static const QRegularExpression quotedItemRe(R"(["']([^"']+)["'])");
    const QRegularExpressionMatch paramsAll = paramsAllRe.match(code);
    if (paramsAll.hasMatch())
    {
        QRegularExpressionMatchIterator it = quotedItemRe.globalMatch(paramsAll.captured(1));
        while (it.hasNext())
            paramNames << it.next().captured(1);
    }
    static const QRegularExpression paramOneRe(R"(setParamName\s*\(\s*\d+\s*,\s*["']([^"']+)["'])");
    QRegularExpressionMatchIterator paramOne = paramOneRe.globalMatch(code);
    while (paramOne.hasNext())
        paramNames << paramOne.next().captured(1);

    // Declarations in the body: capture the declarator list between the type keyword and the first
    // "=" or ";" so both `double I = ...;` and `double a, b;` are covered.
    static const QRegularExpression declRe(
        R"(\b(?:double|float|int|long|short|unsigned|signed|char|bool|auto)\s+([^;{}()=]+?)\s*(?:=|;))");
    QStringList hardCollisions;
    QStringList silentCollisions;
    QRegularExpressionMatchIterator decl = declRe.globalMatch(body);
    while (decl.hasNext())
    {
        const QStringList declared = decl.next().captured(1).split(',');
        for (const QString &raw : declared)
        {
            const QString name = raw.trimmed().remove('*').remove('&').trimmed();
            if (name.isEmpty() || name.contains(' '))
                continue;
            if (name == yName)
            {
                if (!hardCollisions.contains(name))
                    hardCollisions << name;
            }
            else if (name == xName || name == "initNow" || name == "key" || name == "ParaM" ||
                     paramNames.contains(name))
            {
                if (!silentCollisions.contains(name))
                    silentCollisions << name;
            }
        }
    }
    // Silent collisions only — a yName clash is a loud clang "error: redefinition" that self-corrects.
    if (silentCollisions.isEmpty())
        return {};

    QString explanation = "The setCode() C++ body declares name(s) the generated function already "
                          "provides — assign to them, never declare them.\n";
    if (!hardCollisions.isEmpty())
        explanation += "- \"" + hardCollisions.join("\", \"") +
                       "\": this is the dependent variable (yName), "
                       "already declared as a local by the boilerplate. Declaring it again fails the compile "
                       "with \"error: redefinition\". Drop the type keyword: write \"" +
                       hardCollisions.first() + " = ...;\", not \"double " + hardCollisions.first() + " = ...;\".\n";
    if (!silentCollisions.isEmpty())
        explanation += "- \"" + silentCollisions.join("\", \"") +
                       "\": already in scope (the independent "
                       "variable, a fit parameter, or boilerplate state). Declaring these compiles CLEANLY but "
                       "silently shadows the real value, so the function computes with an uninitialized local "
                       "instead of the actual parameter. Use the name(s) directly, with no declaration.\n";
    explanation += "Intermediate locals of your OWN naming are fine — only these reserved names must not be "
                   "redeclared.";
    return explanation;
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
    // Also "run that tab once to test it": confirmed live (Codex, benchmark LIST 1 item 4), the request asked for the
    // test run in exactly those words, the script did it, and it was blocked twice until the item was given up.
    static const QRegularExpression immediateRunAuthorizedRe(
        R"(\b(run|execute|test|apply)\s+(it\s+)?(now|immediately|right away|right now)\b|)"
        R"(\b(run|execute)\s+(it|that|this|the)?\s*(tab|note|script)?\s*once\b)",
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

QString QtiSasAiContent::selfReviewCreationNameSubstitution(const QString &preReviewDraft,
                                                            const QString &postReviewCode) const
{
    // Same rationale and shape as selfReviewFunctionNameSubstitution() above, for a different call
    // family: self-review has no access to the original request text, so the pre-review draft's
    // name — generated straight from that request — is the only legitimate source of truth. Checks
    // each creation kind independently (not "the first creation call found overall") since a script
    // may create more than one kind of object and comparing across kinds would false-match.
    struct CreationKind
    {
        QString noun;
        QString creationCall;
    };
    static const QList<CreationKind> kinds = {
        {"table", "newTable"}, {"matrix", "newMatrix"}, {"graph", "newGraph"}, {"note", "newNote"}};
    for (const CreationKind &kind : kinds)
    {
        const QRegularExpression creationNameRe(R"(\b)" + kind.creationCall + R"(\s*\(\s*["']([^"']+)["'])");
        const QRegularExpressionMatch preMatch = creationNameRe.match(preReviewDraft);
        const QRegularExpressionMatch postMatch = creationNameRe.match(postReviewCode);
        if (!preMatch.hasMatch() || !postMatch.hasMatch())
            continue;
        const QString preName = preMatch.captured(1);
        const QString postName = postMatch.captured(1);
        if (preName == postName)
            continue;
        return "Self-review changed the " + kind.noun + " name passed to " + kind.creationCall + "() from \"" +
               preName + "\" to \"" + postName +
               "\". Self-review runs with no access to the original request text, so it has no "
               "legitimate basis to alter a creation name the draft already got right — unlike "
               "checklist items it IS grounded to check, any change here is more likely a "
               "typo/hallucinated substitution than a real fix.";
    }
    return {};
}

QString QtiSasAiContent::exploreGroupsIgnoredMistake(const QString &code, const QString &exploreOutput) const
{
    static const QRegularExpression setGroupRe(R"(\.setGroup\s*\(\s*["']([^"']+)["'])");
    const QRegularExpressionMatch groupCall = setGroupRe.match(code);
    if (!groupCall.hasMatch())
        return {};

    // Only a line that actually looks like a groups() listing counts — "group"/"groups" plus a
    // bracketed, quoted list on the same line — so an EXPLORE round that checked something
    // unrelated (params, column names, ...) never gets treated as if it had covered groups.
    static const QRegularExpression groupsListLineRe(R"((?im)^.*\bgroups?\b.*(\[[^\[\]]*\]).*$)");
    const QRegularExpressionMatch listLine = groupsListLineRe.match(exploreOutput);
    if (!listLine.hasMatch())
        return {};

    static const QRegularExpression quotedItemRe(R"(['"]([^'"]+)['"])");
    QStringList exploredGroups;
    QRegularExpressionMatchIterator it = quotedItemRe.globalMatch(listLine.captured(1));
    while (it.hasNext())
        exploredGroups << it.next().captured(1);

    const QString chosenGroup = groupCall.captured(1);
    if (exploredGroups.contains(chosenGroup))
        return {};

    return "The preceding EXPLORE step printed the real existing groups (" + exploredGroups.join(", ") +
           ") and this script calls setGroup(\"" + chosenGroup +
           "\"), which matches none of them. If you meant to reuse an existing group, use one of "
           "the ones listed above instead. If a new group is genuinely intentional, that's allowed "
           "— just confirm it, don't repeat a value you invented before you'd seen the real list.";
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

QString QtiSasAiContent::changeFolderNoOpMistake(const QString &code)
{
    // changeFolder(folder, force=False) bails out immediately when folder is already the active
    // one (ApplicationWindow::changeFolder()'s own first check) — activeFolder() always returns
    // exactly that, so changeFolder(activeFolder()) with no force=True is a guaranteed no-op by
    // construction, never a real action. The regex requires activeFolder()'s closing paren to be
    // immediately followed by changeFolder's own closing paren, so an explicit force=True second
    // argument (a genuinely meaningful call) does not match. Confirmed live: generated as an
    // attempt to "activate" a just-created Note's tab before relying on autoexec — folders and a
    // Note's tabs are unrelated, and the call was paired with a scriptPrint() falsely confirming
    // success for an action that did nothing at all.
    static const QRegularExpression noOpRe(R"(\bchangeFolder\s*\(\s*activeFolder\s*\(\s*\)\s*\))");
    QRegularExpressionMatch m = noOpRe.match(code);
    if (m.hasMatch())
        return "This script calls changeFolder(activeFolder()) — changeFolder() bails out "
               "immediately whenever the folder given is already the active one, and "
               "activeFolder() always returns exactly that folder, so this call is a guaranteed "
               "no-op that does nothing at all, every time. It is never a real way to \"activate\" "
               "anything — folders and a window's own state (e.g. a Note's active tab) are "
               "unrelated. Remove this line entirely; if a tab genuinely needs to be the active "
               "one, only addTab() makes a new tab active on creation — there is no way to "
               "reactivate an existing tab afterward.";
    return {};
}

QString QtiSasAiContent::tableCellRowColSwapMistake(const QString &code)
{
    // Table's setCell/setCellData/cell/text all take (col, row) — the opposite of Matrix's
    // (row, col) (see zeroBasedTableIndexMistake()'s doc comment on that same asymmetry) and of
    // the "loop over rows" intuition that carries the Matrix habit over. Confirmed live: a
    // table-fill script wrote t.setCell(i+1, 1, q[i]) / t.setCell(i+1, 2, ...) / t.setCell(i+1, 3,
    // ...) inside a "for i in range(100):" loop — i+1 (the ROW) landed in the COLUMN position and
    // a literal column index (1/2/3) landed in the ROW position. table-python-api.md's own
    // (col, row) rule was already present THREE separate times in this exact call's retrieved
    // context (a warning banner, an inline note, and a dedicated "Rule 1" mistake entry) and still
    // didn't prevent it — a documentation-only fix isn't reliable enough here.
    //
    // Heuristic, not a proof, so deliberately narrowed twice to keep false positives rare:
    // (1) the receiver must be a variable this same script assigned from table()/newTable()/
    // currentTable() — cell()/text() are also Matrix methods with the OPPOSITE (row, col) order,
    // so without this a genuinely correct Matrix call (row-loop variable first, on purpose) would
    // get mechanically "corrected" into a real bug instead of catching one.
    // (2) even restricted to Table, this still can't know which axis a loop variable represents —
    // "for col in range(1, ncols+1): t.setCell(col, 5, value)" (looping over columns with a fixed
    // row) matches the identical shape and is correct. Accepted anyway: QtiSAS tables are
    // row-heavy/column-sparse by construction (see table-python-api.md's own fill examples), so
    // looping over columns instead of rows is a rare pattern in practice, and the explanation
    // below tells the model to leave the call alone if that's genuinely what it's doing.
    static const QRegularExpression tableVarRe(R"(\b(\w+)\s*=\s*(?:table|newTable|currentTable)\s*\()");
    QSet<QString> tableVars;
    QRegularExpressionMatchIterator tableVarIt = tableVarRe.globalMatch(code);
    while (tableVarIt.hasNext())
        tableVars.insert(tableVarIt.next().captured(1));
    if (tableVars.isEmpty())
        return {};

    static const QRegularExpression loopVarRe(R"(\bfor\s+(\w+)\s+in\s+range\()");
    QSet<QString> loopVars;
    QRegularExpressionMatchIterator loopVarIt = loopVarRe.globalMatch(code);
    while (loopVarIt.hasNext())
        loopVars.insert(loopVarIt.next().captured(1));
    if (loopVars.isEmpty())
        return {};

    static const QRegularExpression callRe(
        R"(\b(\w+)\.(setCell|setCellData|cell|text)\s*\(\s*(\w+)(?:\s*[+\-]\s*\d+)?\s*,\s*(\d+)\s*[,)])");
    QRegularExpressionMatchIterator callIt = callRe.globalMatch(code);
    while (callIt.hasNext())
    {
        const QRegularExpressionMatch m = callIt.next();
        if (!tableVars.contains(m.captured(1)) || !loopVars.contains(m.captured(3)))
            continue;
        const QString method = m.captured(2);
        const QString loopVar = m.captured(3);
        const QString colLiteral = m.captured(4);
        return "This script calls " + m.captured(1) + "." + method + "(" + loopVar + ", " + colLiteral +
               ", ...) — a for-loop variable (\"" + loopVar +
               "\") as the FIRST argument and a bare "
               "integer literal as the SECOND. Table's cell methods take (col, row), so a row-loop "
               "variable belongs in the SECOND position, not the first. If this loop iterates rows "
               "(the common case), swap the arguments — the column should be the small literal, the "
               "loop variable the row: " +
               m.captured(1) + "." + method + "(" + colLiteral + ", " + loopVar + ", ...) — not " + m.captured(1) +
               "." + method + "(" + loopVar + ", " + colLiteral +
               ", ...). (If this loop genuinely iterates columns instead, this call is "
               "already correct and can be left as-is.)";
    }
    return {};
}

QString QtiSasAiContent::compilerSetCodeNestedFunctionMistake(const QString &code)
{
    // setCode(text) sets ONLY the body snippet inserted into an already-open function
    // (compile-explorer.cpp's saveAsCPP(): emits "extern \"C\" ... double functionSANS(...) { "
    // + setCode()'s text + " ... return <yName>; }" verbatim — confirmed directly against that
    // generator, not just the doc's own claim). A [code] snippet that itself contains a full
    // "TYPE NAME(args) { ... }" function definition is therefore always a C++ "function
    // definition is not allowed here" error — C++ has no nested named function definitions.
    // Confirmed live: setCode("double Iq(double q, ...) { return ...; }") instead of assigning
    // directly to the pre-declared dependent variable (e.g. "f = ...;"), apparently pattern-
    // matching a SasView-style standalone Iq(q, params...) function instead of this API's
    // "assign, don't declare" contract. Legitimate ONLY inside setIncludedFunctions() (a separate
    // insertion point outside functionSANS entirely) — never inside setCode().
    static const QRegularExpression nestedFunctionRe(
        R"(\b(?:double|float|int|void|bool)\s+\w+\s*\([^;{}]*\)\s*\n?\s*\{)");

    static const QRegularExpression inlineRe(R"re(\.setCode\s*\(\s*(?:"""(.*?)"""|'''(.*?)''')\))re",
                                             QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator inlineIt = inlineRe.globalMatch(code);
    while (inlineIt.hasNext())
    {
        const QRegularExpressionMatch m = inlineIt.next();
        const QString text = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (nestedFunctionRe.match(text).hasMatch())
            return "This script's setCode(...) argument contains a full C++ function definition "
                   "(\"TYPE NAME(args) { ... }\") — setCode() only sets the BODY inserted inside "
                   "the already-open functionSANS(), so a nested function definition there raises "
                   "\"function definition is not allowed here\". Assign directly to the dependent "
                   "variable instead (whatever setYName() was given, or the unset default \"f\") — "
                   "no signature, no braces, no return statement, e.g. "
                   "\"f = scale * exp(-0.5 * pow((x - center) / sigma, 2.0)) + background;\", not a "
                   "standalone \"double Iq(...) { return ...; }\" definition.";
    }

    QHash<QString, QString> stringVars;
    static const QRegularExpression varAssignRe(R"re(\b(\w+)\s*=\s*(?:"""(.*?)"""|'''(.*?)'''))re",
                                                QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator varIt = varAssignRe.globalMatch(code);
    while (varIt.hasNext())
    {
        const QRegularExpressionMatch m = varIt.next();
        stringVars.insert(m.captured(1), m.captured(2).isEmpty() ? m.captured(3) : m.captured(2));
    }
    static const QRegularExpression setCodeVarRe(R"(\.setCode\s*\(\s*(\w+)\s*\))");
    QRegularExpressionMatchIterator refIt = setCodeVarRe.globalMatch(code);
    while (refIt.hasNext())
    {
        const QString varName = refIt.next().captured(1);
        if (stringVars.contains(varName) && nestedFunctionRe.match(stringVars.value(varName)).hasMatch())
            return "The string assigned to \"" + varName + "\" and passed to setCode(" + varName +
                   ") contains a full C++ function definition (\"TYPE NAME(args) { ... }\") — "
                   "setCode() only sets the BODY inserted inside the already-open functionSANS(), "
                   "so a nested function definition there raises \"function definition is not "
                   "allowed here\". Assign directly to the dependent variable instead (whatever "
                   "setYName() was given, or the unset default \"f\") — no signature, no braces, "
                   "no return statement.";
    }
    return {};
}

QString QtiSasAiContent::compilerFittableOnlyMethodMistake(const QString &code)
{
    // setParamLimits(name, min, max) and setParamShared(name, shared)/paramShared(name) are real,
    // correctly-shaped methods — but only on Fittable (fittable18 in qtimod.sip), never on
    // Compiler (compile18) — confirmed against the SIP class boundary directly, not just the
    // docs. Compiler's own limits API is entirely different in both name AND shape:
    // setParamLimit(i, "min..max") (per-index, STRING range) or setParamLimitsAll([...]) (bulk,
    // STRING ranges) — Compiler has no "shared" concept at all. Confirmed live: a script
    // correctly using Fittable's float-argument setParamLimits()/setParamShared() syntax called
    // it on a Compiler object instead, and across 3 retries the model just deleted each call
    // rather than switching to Compiler's real, differently-shaped equivalent.
    static const QRegularExpression compilerVarRe(R"(\b(\w+)\s*=\s*Compiler\b)");
    QSet<QString> compilerVars;
    QRegularExpressionMatchIterator varIt = compilerVarRe.globalMatch(code);
    while (varIt.hasNext())
        compilerVars.insert(varIt.next().captured(1));
    if (compilerVars.isEmpty())
        return {};

    static const QRegularExpression callRe(R"(\b(\w+)\.(setParamShared|paramShared|setParamLimits)\s*\()");
    QRegularExpressionMatchIterator callIt = callRe.globalMatch(code);
    while (callIt.hasNext())
    {
        const QRegularExpressionMatch m = callIt.next();
        if (!compilerVars.contains(m.captured(1)))
            continue;
        const QString receiver = m.captured(1);
        const QString method = m.captured(2);
        if (method == "setParamLimits")
            return "This script calls " + receiver +
                   ".setParamLimits(...) — setParamLimits(name, "
                   "min, max) is real, but only on Fittable, never on Compiler. Compiler's own "
                   "limits API is a different shape entirely: setParamLimit(i, \"min..max\") "
                   "(per-index, a STRING range like \"0..100\" or \"0..\") or "
                   "setParamLimitsAll([...]) (bulk, a list of those same range strings) — raw "
                   "float arguments raise TypeError on Compiler's setParamLimitsAll, and "
                   "setParamLimits itself doesn't exist there at all.";
        return "This script calls " + receiver + "." + method + "(...) — " + method +
               "(name, "
               "...) is real, but only on Fittable, never on Compiler; Compiler has no \"shared "
               "parameter\" concept at all. If parameter sharing is genuinely needed, it belongs "
               "on the Fittable object used later for fitting, not on Compiler.";
    }
    return {};
}

QString QtiSasAiContent::setParamNamesMissingAllMistake(const QString &code)
{
    // setParamNames(...) (plural, no "All") doesn't exist on Compiler OR Fittable — the only real
    // bulk name-setter is setParamNamesAll(names) (Compiler; Fittable has no bulk name-setter
    // concept at all, since it reuses already-named library parameters). Confirmed live: dropped
    // entirely across a retry instead of renamed.
    static const QRegularExpression re(R"(\.setParamNames\s*\()");
    if (re.match(code).hasMatch())
        return "This script calls .setParamNames(...) — that method doesn't exist. The real bulk "
               "name-setter is setParamNamesAll(names) (same list-of-strings argument, matching "
               "setParamCount()) — never setParamNames() (no \"All\").";
    return {};
}

QString QtiSasAiContent::compilerNonexistentMethodMistake(const QString &code)
{
    // setFunctionCode() doesn't exist — the real body-setter is setCode(text) (see
    // compilerSetCodeNestedFunctionMistake()'s doc comment on what it actually does). compile()
    // takes no name argument at all — its only parameter is compileAll (a bool, defaulting to
    // False); the function name must already be set via setFunctionName() before calling it.
    // Confirmed live: both hallucinated together in the same script, with no setFunctionName()
    // call anywhere and no c.new()/setGroup() either.
    static const QRegularExpression setFunctionCodeRe(R"(\.setFunctionCode\s*\()");
    if (setFunctionCodeRe.match(code).hasMatch())
        return "This script calls .setFunctionCode(...) — that method doesn't exist. The real "
               "body-setter is setCode(text), and text must be a single assignment to the "
               "dependent variable (see setCode()'s own doc rules) — never a full function "
               "definition.";

    static const QRegularExpression compileWithNameRe(R"(\.compile\s*\(\s*["'])");
    if (compileWithNameRe.match(code).hasMatch())
        return "This script calls .compile(\"...\") with a function-name argument — compile() "
               "takes no such argument (its only parameter is compileAll, a bool). Set the "
               "function name beforehand with setFunctionName(\"...\"), then call compile() with "
               "no arguments.";

    return {};
}

QString QtiSasAiContent::compilerSetCodeCppSyntaxMistake(const QString &code)
{
    // Two confirmed-live C++-vs-Python mixups inside setCode()'s/setIncludedFunctions()'s text.
    // (1) "**" is not a C++ operator — "x ** 2" parses as "x * (*2)", dereferencing the integer
    // literal 2, which raises "indirection requires pointer operand" (compile-python-api.md's own
    // "no ** operator" rule). (2) ParaM is the raw void* the boilerplate passes to
    // readParameters()/saveParameters() around the snippet — it is NOT indexable; every declared
    // parameter is already available by NAME as a plain local, never via ParaM[i]. Confirmed
    // live: both together raised 5 separate clang errors in the same script (4x "subscript of
    // pointer to incomplete type 'void'", 1x "indirection requires pointer operand"), surviving 2
    // rounds unfixed until the retry budget ran out.
    QStringList candidateTexts;

    static const QRegularExpression inlineRe(
        R"re(\.set(?:Code|IncludedFunctions)\s*\(\s*(?:"""(.*?)"""|'''(.*?)''')\))re",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator inlineIt = inlineRe.globalMatch(code);
    while (inlineIt.hasNext())
    {
        const QRegularExpressionMatch m = inlineIt.next();
        candidateTexts << (m.captured(1).isEmpty() ? m.captured(2) : m.captured(1));
    }

    QHash<QString, QString> stringVars;
    static const QRegularExpression varAssignRe(R"re(\b(\w+)\s*=\s*(?:"""(.*?)"""|'''(.*?)'''))re",
                                                QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator varIt = varAssignRe.globalMatch(code);
    while (varIt.hasNext())
    {
        const QRegularExpressionMatch m = varIt.next();
        stringVars.insert(m.captured(1), m.captured(2).isEmpty() ? m.captured(3) : m.captured(2));
    }
    static const QRegularExpression setCodeVarRe(R"(\.set(?:Code|IncludedFunctions)\s*\(\s*(\w+)\s*\))");
    QRegularExpressionMatchIterator refIt = setCodeVarRe.globalMatch(code);
    while (refIt.hasNext())
    {
        const QString varName = refIt.next().captured(1);
        if (stringVars.contains(varName))
            candidateTexts << stringVars.value(varName);
    }

    static const QRegularExpression starStarRe(R"(\*\*)");
    static const QRegularExpression paraMIndexRe(R"(\bParaM\s*\[)");
    for (const QString &text : candidateTexts)
    {
        if (starStarRe.match(text).hasMatch())
            return "This script's setCode(...)/setIncludedFunctions(...) text contains \"**\" — "
                   "C++ has no exponentiation operator; \"x ** 2\" parses as \"x * (*2)\", "
                   "dereferencing the integer literal 2, which raises \"indirection requires "
                   "pointer operand\". Use pow(x, 2) or x * x instead.";
        if (paraMIndexRe.match(text).hasMatch())
            return "This script's setCode(...) text indexes ParaM directly (e.g. ParaM[0]) — "
                   "ParaM is the raw void* the boilerplate passes to readParameters()/"
                   "saveParameters() around the snippet; it is not indexable and raises "
                   "\"subscript of pointer to incomplete type 'void'\". Every declared parameter "
                   "is already available by its own NAME as a plain local — use the name "
                   "directly, never ParaM[i].";
    }
    return {};
}

QString QtiSasAiContent::compilerMissingAiNamespaceMistake(const QString &code)
{
    // Every function authored via .new() must be named "ai/<shortName>" (compile-python-api.md's
    // Rule 6) — never a bare name, and never an existing domain folder that a real library
    // function might already use or later collide with. Gated on .compile() rather than .new()
    // itself: confirmed live, every observed script that actually authors a new function calls
    // setFunctionName()/setCode()/compile() but skips .new() entirely — requiring .new() as the
    // gate would make this never fire in practice. .compile() together with a bare
    // setFunctionName() is still a reliable signal of authoring something new, since reusing an
    // existing library function for fitting never calls setFunctionName() with a fresh name at all.
    static const QRegularExpression compileCallRe(R"(\.compile\s*\()");
    if (!compileCallRe.match(code).hasMatch())
        return {};
    static const QRegularExpression setFunctionNameRe(R"(\.setFunctionName\s*\(\s*["']([^"']+)["'])");
    const QRegularExpressionMatch m = setFunctionNameRe.match(code);
    if (m.hasMatch() && !m.captured(1).startsWith("ai/"))
        return "This script calls .setFunctionName(\"" + m.captured(1) +
               "\") then .compile() — "
               "every function authored this way must be named under the reserved \"ai/\" folder "
               "(e.g. \"ai/" +
               m.captured(1) +
               "\"), never a bare name or an existing domain "
               "folder (sasviewmodels/, custom/, qtiplot/, ...) that a real library function "
               "might already use or later collide with.";
    return {};
}

QString QtiSasAiContent::hallucinatedPlotDesignationCaseMistake(const QString &code)
{
    // Confirmed live: Table.PlotDesignation.x (lowercase, matching the mixed-case xErr/yErr but
    // guessed for the plain X/Y/Z members too) raised AttributeError — the real members
    // (table-python-api.md) are an inconsistent-looking mix the model evidently didn't recall
    // exactly and guessed a "consistent" lowercase variant for instead.
    static const QStringList validMembers = {"None_", "X", "Y", "Z", "xErr", "yErr", "Label"};
    static const QRegularExpression usageRe(R"(\bTable\.PlotDesignation\.(\w+))");
    QRegularExpressionMatchIterator it = usageRe.globalMatch(code);
    while (it.hasNext())
    {
        const QString member = it.next().captured(1);
        if (!validMembers.contains(member))
            return "This script references Table.PlotDesignation." + member +
                   ", which does not exist and raises AttributeError — the real members are "
                   "exactly Table.PlotDesignation.None_/X/Y/Z/xErr/yErr/Label (X/Y/Z uppercase, "
                   "xErr/yErr lowercase-prefixed, Label capitalized) — don't guess a case variant "
                   "of a real member.";
    }
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
    // nothing about the underlying data changed between the read and the hardcoding. ALSO confirmed
    // live in a script that legitimately calls f.fit()/reads fitCurve-<name> for OTHER variables: a
    // "winner" variable was assigned a bare hardcoded literal recalled from an earlier turn's
    // printed output, with no live re-derivation of THAT SPECIFIC variable anywhere in the script —
    // a real fit() call elsewhere in the same script is not proof this one value is live, so the
    // check below is scoped per-variable rather than bailing out on any fit()/fitCurve in the file.
    static const QRegularExpression hardcodedChi2Re(R"(\b(chi2\w*)\s*=\s*-?\d+\.\d+\b)");
    QRegularExpressionMatchIterator it = hardcodedChi2Re.globalMatch(code);
    while (it.hasNext())
    {
        const QString varName = it.next().captured(1);
        const QRegularExpression liveAssignRe(
            "\\b" + QRegularExpression::escape(varName) +
            R"(\s*(?:,[^=\n]*)?=\s*[^=\n]*(?:\.fit\(|fitCurve|read_val\(|\.results\(\)))");
        if (liveAssignRe.match(code).hasMatch())
            continue; // this exact variable IS derived from a live fit round-trip somewhere
        return "This script assigns \"" + varName +
               "\" a bare hardcoded literal number, with no "
               "live .fit()/fitCurve-<name>/.results() assignment to THAT SAME variable anywhere in "
               "the script — a real fit() call elsewhere in the script for a DIFFERENT variable "
               "doesn't make this one live. fittable-python-api.md is explicit that the "
               "fitCurve-<name> table (or a fresh f.fit() call) is the ONLY reliable source for "
               "chi2/dof once you're past the part that originally ran the fit — never re-derive it "
               "from memory or an earlier turn's printed console text. Assign \"" +
               varName + "\" from a live read instead of hardcoding the recalled number.";
    }
    return {};
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

QString QtiSasAiContent::discoveredNameInsteadOfRequestedMistake(const QString &code,
                                                                 const QString &originalRequestText)
{
    // unannouncedModelNameSubstitution() below only matches a LITERAL configure("name") argument,
    // so a name DERIVED from a wildcard search and passed as a variable escapes it. Confirmed
    // live: the request named "sasviewmodels/sphere" (already verified EXISTS by the existence
    // check), and the script ran fitFunctions("*sphere*"), took hits[0], and fitted
    // "Core-Shell-Sphere-ShellPD-GH-5pt" instead — caught only three rounds later, once the wrong
    // name had been hardcoded. This blocks the derivation itself, before the wrong model is fitted.
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

    // Naming it anywhere at all is enough to stay silent — fail open rather than risk blocking a
    // script that legitimately mentions the requested name.
    if (code.contains("\"" + requestedName + "\"") || code.contains("'" + requestedName + "'"))
        return {};

    static const QRegularExpression wildcardSearchRe(
        R"(\.(?:fitFunctions|functionNames)\s*\(\s*["'][^"']*\*[^"']*["'])");
    if (!wildcardSearchRe.match(code).hasMatch())
        return {};

    static const QRegularExpression configureVarRe(R"(\.(?:configure|setFunction)\s*\(\s*(?!["'])[A-Za-z_])");
    if (!configureVarRe.match(code).hasMatch())
        return {};

    return "The request names \"" + requestedName +
           "\" explicitly, but this script never uses that name — it runs a wildcard "
           "fitFunctions()/functionNames() search and configures whatever the search happened to "
           "return. A pattern search returns UNRELATED models whose names merely share a word "
           "(e.g. \"*sphere*\" matches core-shell and polydisperse variants), so this silently "
           "fits a different model than the one asked for. Use \"" +
           requestedName +
           "\" verbatim in configure(). If you genuinely believe it does not exist, say so "
           "explicitly instead of substituting a search result for it.";
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

QString QtiSasAiContent::reuseInsteadOfCompileMistake(const QString &code, const QString &originalRequestText,
                                                      const QString &earlierPartsCode)
{
    // Confirmed live TWICE, independently, for two different physics targets (a sphere form
    // factor, and separately a Gaussian/Lorentzian peak pair): asked to "compile a custom fit
    // function," EXPLORE finds a coincidentally-matching name already sitting in the workspace
    // from an earlier session/attempt — Compiler.functionNames() AND Fittable.fitFunctions() BOTH
    // listing it is genuine evidence a working compiled function currently exists under that name —
    // and the action script configures/fits it directly, with no c.compile() call anywhere.
    // Existence in both interfaces proves a PRECONDITION ("a compiled function exists"), but
    // "compile X" is an imperative action that was actually requested, not a precondition to
    // verify — it should always produce a live compile() call in the executed script: c.open(name)
    // then compile() if the name already exists (re-affirms it's genuinely compiled right now,
    // rather than trusting a leftover that could be stale or, per the first incident, not actually
    // working despite the FIF existing), or the full c.new()/.../compile() sequence if it doesn't.
    // Request-text-gated: only fires when "compile" was actually asked for — a request that only
    // asks to FIT an existing/library function is correctly exempt.
    //
    // Confirmed live a THIRD time, downstream of this exact check: this explanation used to name
    // only setFunctionName()/setGroup()/setCode()/compile() for the "doesn't exist yet" branch,
    // omitting the parameter-setup calls entirely. Blocks like this one route through the SAME
    // no-fresh-docs retry path as a runtime error (AiAssistant::runActionScript() calls
    // ask(..., false) for every entry in errContext, including this one) — so this string is the
    // ONLY grounding the corrective round gets, not a substitute for docs it can fall back on. The
    // gap got filled with a plausible-but-nonexistent `c.addParameter(name, init, varies, min,
    // max)` — a single consolidated call other fitting libraries do have, QtiSAS does not. Spelling
    // out every required call closes exactly that gap; see compile-python-api.md's own minimal
    // create-and-compile flow for the authoritative version this mirrors.
    static const QRegularExpression requestedCompileRe(R"(\bcompile\b)", QRegularExpression::CaseInsensitiveOption);
    if (!requestedCompileRe.match(originalRequestText).hasMatch())
        return {};
    static const QRegularExpression compileCallRe(R"(\.compile\s*\()");
    if (compileCallRe.match(code).hasMatch())
        return {};
    // Split-mode: an EARLIER part in the SAME plan may have already compiled the function THIS
    // part now just fits/plots — that's the intended division of labor (e.g. "part 2: compile the
    // models", "part 3: fit them and compare"), not a violation. `originalRequestText` is the
    // WHOLE multi-part request and still says "compile" regardless of which part actually does it,
    // so without this guard this check would wrongly block every later part that correctly reuses
    // what an earlier part already built — forcing a redundant recompile, or worse, scolding a
    // legitimately correct later part into "fixing" something that was never broken. Deliberately
    // coarse (ANY compile() call in ANY earlier part exempts THIS part entirely, not just a
    // same-named one) — false negatives here are low-risk (a genuinely wrong later part with truly
    // no compile anywhere in the plan still gets caught), while false positives actively harm a
    // correct split-mode plan. Empty (and therefore never matching) outside split mode.
    if (compileCallRe.match(earlierPartsCode).hasMatch())
        return {};
    static const QRegularExpression configuresOrFitsRe(R"(\.(?:configure|setFunction)\s*\(\s*["'])");
    if (!configuresOrFitsRe.match(code).hasMatch())
        return {};
    return "The request explicitly asked to COMPILE a custom fit function, but this script never "
           "calls Compiler.compile() anywhere — it goes straight to Fittable.configure()/"
           "setFunction() against a name that happens to already exist in the workspace (likely a "
           "leftover from an earlier session/attempt). Finding the name in both "
           "Compiler.functionNames() and Fittable.fitFunctions() proves a working compiled function "
           "currently exists, but 'compile X' is an action that was requested, not a precondition "
           "to verify — call c.compile() in this script regardless. A name under \"ai/\" is the AI's own "
           "scratch area, so an existing one is a leftover that may hold other code: define it fresh anyway "
           "(the full sequence below; compile() replaces it). Only a library function outside \"ai/\" is "
           "reopened: c.open(name); c.compile(). The full sequence: "
           "c.new(); c.setFunctionName(name); c.setGroup(group); c.setYName(yname); "
           "c.setXName(xname); c.setParamCount(n); c.setParamNamesAll([...]); "
           "c.setParamInitsAll([...]); c.setParamLimitsAll([...]); c.setParamVariesAll([...]); "
           "c.setCode(...); ok = c.compile() — EVERY one of these calls is required, not just "
           "setCode()+compile(); there is no single consolidated 'add a parameter' call (no "
           "addParameter()/addParam() method exists) — parameter count, names, inits, limits, and "
           "vary-flags are five SEPARATE bulk-setter calls, each taking a same-length list.";
}

QString QtiSasAiContent::columnFormulaInLoopMistake(const QString &code)
{
    // Confirmed live (LIST 5, item 1, NOSPLIT, local model): setCommand(2, f"3*{x}^2 - 2*{x} + 1") once per row in a
    // for loop. Each call replaced the column's formula, the last one (x = 2.0) gave every row y = 9, and the fit to
    // that constant column "succeeded" with a = b = 0, c = 9 — a clean run with a wrong answer.
    static const QRegularExpression loopRe(R"(^([ \t]*)(?:for|while)\b.*:\s*(?:#.*)?$)");
    static const QRegularExpression setCommandRe(R"(^([ \t]*)\S.*\.setCommand\s*\()");
    QList<qsizetype> loopIndents;
    for (const QString &line : code.split('\n'))
    {
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#'))
            continue;
        qsizetype indent = 0;
        while (indent < line.size() && (line.at(indent) == ' ' || line.at(indent) == '\t'))
            ++indent;
        while (!loopIndents.isEmpty() && indent <= loopIndents.last())
            loopIndents.removeLast();
        if (!loopIndents.isEmpty() && setCommandRe.match(line).hasMatch())
            return "setCommand() is called inside a loop. It sets the formula of the WHOLE column, which "
                   "recalculate() then evaluates for every row, so each call replaces the one before and only the "
                   "last stays. Call it ONCE, outside any loop, with a formula that refers to other columns by name, "
                   "e.g. t.setCommand(2, \"3*col(\\\"x\\\")**2 + 1\"), then t.recalculate(2).";
        if (const QRegularExpressionMatch loop = loopRe.match(line); loop.hasMatch())
            loopIndents << indent;
    }
    return {};
}

QString QtiSasAiContent::guardedCompileMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 5, item 1, local model, twice): after a failed compile the retry wrapped the compile in
    // `if fn not in c.functionNames("ALL"):` — the failed compile had left its definition behind, so the check was
    // false, the compile never ran (no compile error to see) and configure() failed with "not found". A function a
    // request compiles is defined and compiled again every time; "ai/" is the AI's own sandbox.
    static const QRegularExpression requestedCompileRe(R"(\bcompile\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression existingTargetRe(R"(\b(?:existing|modify|change|edit|extend|update|recompile)\b)",
                                                     QRegularExpression::CaseInsensitiveOption);
    if (!requestedCompileRe.match(originalRequestText).hasMatch() ||
        existingTargetRe.match(originalRequestText).hasMatch())
        return {};
    static const QRegularExpression guardRe(
        R"(^([ \t]*)(?:if|elif)\b[^\n]*(?:\bfunctionNames\s*\(|\bfitFunctions\s*\(|\.open\s*\(|)"
        R"(\bfunctionName\s*\(\s*\)|\bexist\w*\s*\()[^\n]*:[ \t]*$)",
        QRegularExpression::MultilineOption);
    static const QRegularExpression compileRe(R"(\.\s*compile\s*\()");
    const QStringList lines = code.split('\n');
    for (auto it = guardRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch guard = it.next();
        const int indent = static_cast<int>(guard.captured(1).size());
        const int firstLine = static_cast<int>(code.left(guard.capturedEnd()).count('\n')) + 1;
        // the guarded block: the following lines indented deeper than the if (blank lines included)
        for (int i = firstLine; i < lines.size(); ++i)
        {
            const QString &line = lines.at(i);
            if (line.trimmed().isEmpty())
                continue;
            const int lineIndent =
                static_cast<int>(line.size() - QString(line).remove(QRegularExpression("^[ \t]+")).size());
            if (lineIndent <= indent)
                break;
            if (compileRe.match(line).hasMatch())
                return "This script compiles the function only if a check says it does not exist yet (" +
                       guard.captured(0).trimmed() +
                       "). A function the request asks to compile is defined and compiled again every time — "
                       "\"ai/\" is the AI's own sandbox: a failed earlier compile leaves its definition behind, "
                       "so this check skips the compile and configure() then reports \"not found\". Remove the "
                       "check: c.new(); c.setFunctionName(...); ... ; c.compile(), unconditionally. (Where a script "
                       "must know whether a fit function is usable, ask Fittable.fitFunctions() — the compiled "
                       "ones; Compiler.functionNames() lists definitions, also one whose compile failed.)";
        }
    }
    return {};
}

QString QtiSasAiContent::aiSandboxReopenMistake(const QString &code, const QString &originalRequestText,
                                                const QString &earlierPartsCode)
{
    // Confirmed live (benchmark LIST 3, item 8): "Compile a new fit function \"ai/RefDecay\" for y = A*exp(-k*x) + c"
    // with ai/RefDecay left over from an earlier run — the script opened the old one and recompiled it, so whatever
    // code the leftover held was fitted instead of the requested model. "ai/" is the AI's scratch area: a compile
    // request defines the function fresh there. Exempt: a request that asks to use or change an existing function, and
    // a name an earlier part of this plan defined (that one is this request's own work).
    static const QRegularExpression requestedCompileRe(R"(\bcompile\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression existingTargetRe(R"(\b(?:existing|modify|change|edit|extend|update|recompile)\b)",
                                                     QRegularExpression::CaseInsensitiveOption);
    if (!requestedCompileRe.match(originalRequestText).hasMatch() ||
        existingTargetRe.match(originalRequestText).hasMatch())
        return {};
    static const QRegularExpression newCallRe(R"(\.new\s*\(\s*\))");
    if (newCallRe.match(code).hasMatch())
        return {};
    static const QRegularExpression openAiRe(R"(\.open\s*\(\s*["'](ai/[^"']+)["'])");
    for (auto it = openAiRe.globalMatch(code); it.hasNext();)
    {
        const QString name = it.next().captured(1);
        const QRegularExpression definedEarlierRe(R"(setFunctionName\s*\(\s*["'])" + QRegularExpression::escape(name) +
                                                      R"(["'])",
                                                  QRegularExpression::CaseInsensitiveOption);
        if (definedEarlierRe.match(earlierPartsCode).hasMatch())
            continue;
        return "The request asks to compile a function, but this script reopens the existing \"" + name +
               "\" with c.open(). \"ai/\" is the AI's own scratch area: an existing name there is a leftover from an "
               "earlier attempt and may hold other code, so reopening it recompiles that old code, not the requested "
               "one. Define it fresh instead: c.new(); c.setFunctionName(\"" +
               name +
               "\"); c.setGroup(...); c.setYName(...); c.setXName(...); c.setParamCount(n); "
               "c.setParamNamesAll([...]); c.setParamInitsAll([...]); c.setParamLimitsAll([...]); "
               "c.setParamVariesAll([...]); c.setCode(...); ok = c.compile() — this replaces the old one.";
    }
    return {};
}

QString QtiSasAiContent::requestedCreationNeverHappenedMistake(const QString &code, const QString &originalRequestText,
                                                               const QString &earlierPartsCode,
                                                               const QString &laterPartsDescriptions)
{
    // Confirmed live: a request "Create a table \"SAXS_data\" with 3 columns Q, I, dI and fill it
    // with synthetic small-angle-scattering intensities for a sphere form factor plus 3% Gaussian
    // noise" ran as a SINGLE part (correctly — this genuinely is one coherent step). EXPLORE
    // correctly found the built-in "SimpleSphere" model, but the ACTION script that followed was
    // just "import random; c.open(\"SimpleSphere\"); scriptPrint(str(c.paramNames()))" — it ran
    // without error and was accepted as "part done," yet table "SAXS_data" was never created, no
    // intensities were ever computed, and the imported `random` module was never actually called.
    // None of the request's three actual requirements happened at all. Unlike
    // isGiveUpFallbackScript() (a single bare scriptPrint() line), this script has several real,
    // successfully-running API calls — it just does nothing relevant to what was asked, a distinct
    // "elaborate no-op" shape that check's narrow one-line heuristic can't catch.
    //
    // Mirrors reuseInsteadOfCompileMistake()'s split-mode safety immediately above: originalRequestText
    // is always the WHOLE (possibly multi-part) request, so a LATER part correctly reusing an
    // object an EARLIER part already created must stay exempt — checked via earlierPartsCode the
    // same way. Deliberately requires "create" and the object noun and its quoted name to appear
    // within a short span of each other (not just anywhere in the same request) — a request that
    // creates one object and separately just READS an existing, differently-purposed one quoted
    // elsewhere must not false-trigger on that unrelated quote.
    struct CreationKind
    {
        QString noun;
        QString creationCall;
    };
    static const QList<CreationKind> kinds = {
        {"table", "newTable"},
        {"matrix", "newMatrix"},
        {"graph", "newGraph"},
        {"note", "newNote"},
    };
    for (const CreationKind &kind : kinds)
    {
        const QRegularExpression requestedCreateRe(
            QString(R"(\bcreate\w*\b.{0,30}?\b%1s?\b.{0,20}?["']([^"']+)["'])").arg(kind.noun),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch m = requestedCreateRe.match(originalRequestText);
        if (!m.hasMatch())
            continue;
        const QRegularExpression creationCallRe(R"(\b)" + kind.creationCall + R"(\s*\()");
        if (creationCallRe.match(code).hasMatch() || creationCallRe.match(earlierPartsCode).hasMatch())
            continue;
        // A LATER part's own description already claims this exact name — that part, not this one,
        // is responsible for creating it (see this function's split-mode doc comment above).
        if (laterPartsDescriptions.contains(m.captured(1), Qt::CaseInsensitive))
            continue;
        // Created BY a script the request puts into a note's tab ("the "make" tab holds a script that creates a table
        // "Auto""): the remedy is that tab's script, not a direct call. Confirmed live (LIST 5, item 5): the plain
        // message sent the model toward newTable() in the main script — the item's known wrong answer.
        static const QRegularExpression tabScriptRe(R"(\btab\b[^.]{0,40}\bscript\b[^.]*$)",
                                                    QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression tabNameRe(R"RX("([^"]+)"\s+tab\b)RX");
        const QString before = originalRequestText.left(m.capturedStart());
        if (tabScriptRe.match(before).hasMatch())
        {
            const QRegularExpressionMatch tab = tabNameRe.match(before.mid(before.lastIndexOf('.') + 1));
            const QString tabName = tab.hasMatch() ? tab.captured(1) : QString("the tab");
            return "The request creates the " + kind.noun + " \"" + m.captured(1) +
                   "\" through a script in a note's tab, and nothing has written or run that script yet. Write it "
                   "into the tab (n.tab(\"" +
                   tabName + R"(").setText("..." with )" + kind.creationCall + "(\"" + m.captured(1) +
                   "\", ...)) and run that tab: n.tab(\"" + tabName + "\").execute(). Do not create \"" +
                   m.captured(1) + "\" directly in this script.";
        }
        return "The request explicitly asks to CREATE a " + kind.noun + " named \"" + m.captured(1) +
               "\", but this script never calls " + kind.creationCall +
               "(...) anywhere (and no earlier part in this plan created it either) — it only "
               "inspects/opens existing objects instead, with nothing that actually creates \"" +
               m.captured(1) + "\". Add the actual " + kind.creationCall + "(\"" + m.captured(1) +
               "\", ...) call, then perform the rest of the request against it.";
    }
    // A 3D window the request names ("Plot it as a 3D surface in a window named \"Saddle3D\""): confirmed live (LIST 5
    // local pyi, item 4, all 6 runs of a series with EXPLORE on), the script filled the matrix and only looked the
    // window up with plot3D("Saddle3D") — the item printed False for its own window. plot3D("name") with a string is a
    // lookup; creating is newPlot3D(), plot3D() of a matrix or formula, or setWindowName().
    static const QRegularExpression window3DRe(
        R"RX(\b3D\b[^."]{0,60}?\b(?:window|plot|surface)\b[^."]{0,30}?\b(?:named|called)\s+"([^"]+)")RX",
        QRegularExpression::CaseInsensitiveOption);
    if (const QRegularExpressionMatch w = window3DRe.match(originalRequestText);
        w.hasMatch() && !laterPartsDescriptions.contains(w.captured(1), Qt::CaseInsensitive))
    {
        static const QRegularExpression create3DRe(
            R"RX(\bnewPlot3D\s*\(|\bplot3D\s*\(\s*[^"'\s)]|\bsetWindowName\s*\()RX");
        if (!create3DRe.match(code).hasMatch() && !create3DRe.match(earlierPartsCode).hasMatch())
            return "The request asks for a 3D plot in a window named \"" + w.captured(1) +
                   "\", but this script never creates one — plot3D(\"" + w.captured(1) +
                   "\") with a name only looks an existing window up. Create it, e.g. newPlot3D(\"" + w.captured(1) +
                   "\", matrix, style), then do the rest of the request.";
    }
    return {};
}

QString QtiSasAiContent::hardcodedWinnerWithoutLiveComparisonMistake(const QString &code,
                                                                     const QString &originalRequestText,
                                                                     const QString &earlierPartsCode)
{
    // Confirmed live in split mode: EXPLORE (round 3) fit both a Gaussian and a Lorentzian peak
    // model and printed chi2_g=0.926, chi2_l=78.15, winner=ai/GaussianPeak — a COMPUTED CONCLUSION,
    // not a stable identity. Part 4's action script then hardcoded table("fitCurve-ai/GaussianPeak")
    // and the legend text "Winner: ai/GaussianPeak" with NO comparison logic anywhere in the script
    // — it never read chi2/DoF from either fitCurve table and never compared them. This happened to
    // be correct only because it matched what EXPLORE already found for THIS run's data; against
    // different noise/seed data where the Lorentzian actually won, the exact same script would
    // silently plot and label the WRONG model with no error at all. Root cause: each script
    // execution gets a FRESH Python namespace (EXPLORE's local variables never carry over), so a
    // computed conclusion is only available to a later part as printed TEXT in history — reusing it
    // as a literal instead of re-deriving it live is the mistake. Contrast with
    // reuseInsteadOfCompileMistake: compiling is a durable/idempotent action (once done, permanently
    // true, so a later part MAY safely skip redoing it), but "which model currently wins" is a
    // potentially non-durable, data-dependent fact that must be re-checked every time it's acted on
    // — so this check does the OPPOSITE of that one: it requires live re-derivation rather than
    // exempting reuse.
    //
    // Gate 1 — only fires when the request genuinely asked for a comparison/winner; a split plan
    // that fits two UNRELATED models across parts for reasons other than picking a winner (e.g.
    // "part 2: fit a background model, part 3: fit a peak model, part 4: plot the peak model") is
    // correctly exempt.
    static const QRegularExpression comparisonRequestedRe(
        R"(\b(?:compare|comparison|which\s+\w*\s*model|better\s+fit|winner|fits?\s+better)\b)",
        QRegularExpression::CaseInsensitiveOption);
    if (!comparisonRequestedRe.match(originalRequestText).hasMatch())
        return {};
    // Gate 2 — earlierPartsCode must show evidence 2+ DIFFERENT candidates were genuinely
    // configured/fit somewhere in this plan. Always empty in non-split mode, so this check can
    // NEVER fire outside split mode, by construction — no separate mode gate is needed.
    static const QRegularExpression configuredNameRe(R"(\.(?:configure|setFunction)\s*\(\s*["']([^"']+)["'])");
    QStringList fittedNames;
    QRegularExpressionMatchIterator cfgIt = configuredNameRe.globalMatch(earlierPartsCode);
    while (cfgIt.hasNext())
    {
        const QString name = cfgIt.next().captured(1);
        if (!fittedNames.contains(name))
            fittedNames << name;
    }
    if (fittedNames.size() < 2)
        return {};
    // Gate 3 — THIS script references exactly ONE of those candidates via a fitCurve-<name> lookup.
    // None (not this pattern at all) or 2+ (likely already comparing live itself) both fall through.
    static const QRegularExpression fitCurveNameRe(R"(fitCurve-([A-Za-z0-9_/\-]+))");
    QStringList namedInThisScript;
    QRegularExpressionMatchIterator fcIt = fitCurveNameRe.globalMatch(code);
    while (fcIt.hasNext())
    {
        const QString name = fcIt.next().captured(1);
        if (!namedInThisScript.contains(name))
            namedInThisScript << name;
    }
    if (namedInThisScript.size() != 1)
        return {};
    const QString hardcodedName = namedInThisScript.first();
    if (!fittedNames.contains(hardcodedName))
        return {}; // not one of the genuinely-fit candidates from earlier parts — different check's territory
    // Gate 4 — THIS script itself must not already be doing its own live comparison (the documented
    // and correct pattern: read both fitCurve-<name> tables' chi^2/DoF and pick dynamically).
    int configureCallsInThisScript = 0;
    QRegularExpressionMatchIterator selfCfgIt = configuredNameRe.globalMatch(code);
    while (selfCfgIt.hasNext())
    {
        selfCfgIt.next();
        ++configureCallsInThisScript;
    }
    if (configureCallsInThisScript >= 2)
        return {};
    return "This script references \"" + hardcodedName +
           "\" as the presumed winning model (via "
           "fitCurve-" +
           hardcodedName +
           ") with no live comparison anywhere in THIS script, but an "
           "earlier part fit multiple candidates (" +
           fittedNames.join(", ") +
           ") — evidence a real comparison exists somewhere in this plan. Hardcoding the name "
           "recalled from an earlier round's printed conclusion is fragile: correct only because it "
           "happens to match this run's actual result; re-run against different data where a "
           "different model wins, and this exact script would silently plot and label the WRONG "
           "model with no error. Re-derive the comparison live in this script instead: read BOTH "
           "candidates' fitCurve-<name> tables' chi^2/DoF and select dynamically (see "
           "fittable-python-api.md's worked example for comparing two already-fit models), then use "
           "the DYNAMICALLY selected table/name, not a hardcoded literal.";
}

QString QtiSasAiContent::hallucinatedLegendMethodMistake(const QString &code)
{
    // Confirmed live: a legend-styling script tried .setPen(), then .setShape(), then .setAlign()
    // on the SAME legend variable, one at a time, each caught only by a live AttributeError — none
    // of the three exist on Legend (SIP PyName for LegendWidget). Confirmed live again, a second,
    // unrelated incident: .setPosition() (a generic "typical plotting library" guess for
    // positioning), plus .setItemCount()/.setItemText() — inventing a per-item, multi-entry
    // legend-list API that doesn't exist at all; Legend's text is ONE combined string via
    // setText(str), never per-item.
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
        const QRegularExpression badMethodRe(
            QRegularExpression::escape(varName) +
            R"(\.(setPen|setShape|setAlign|setPosition|setItemCount|setItemText)\s*\()");
        const QRegularExpressionMatch m = badMethodRe.match(code);
        if (m.hasMatch())
        {
            const QString method = m.captured(1);
            QString realEquivalent;
            if (method == "setPen")
                realEquivalent = "setFramePen(QPen)/setFrameColor(QColor)/setFrameWidth(float)";
            else if (method == "setShape")
                realEquivalent = "setFrameStyle(Frame.FrameStyle.Line.value or .Shadow.value)";
            else if (method == "setAlign")
                realEquivalent = "setOriginCoord(x, y) — there is no \"align\" concept at all";
            else if (method == "setPosition")
                realEquivalent = "setOrigin(x, y) (pixel coordinates) or setOriginCoord(x, y) "
                                 "(axis data coordinates) — there is no named-position enum like "
                                 "Graph.Legend.Position.RightTop";
            else
                realEquivalent = "setText(str) with the FULL combined text as one string — there "
                                 "is no per-item list API (no item count, no indexed item text); "
                                 "build the whole legend's text yourself, e.g. "
                                 "setText(\"Data\\nFit\")";
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

    // The if-HEADER's own indentation is captured so the body can be delimited by DEPTH.
    // Confirmed live as a false positive otherwise: the previous pattern took the body as "any
    // following indented line", which inside an enclosing `else:` swallowed everything after the
    // branch — including an if-level, genuinely UNCONDITIONAL maximizeWindow() — and blocked a
    // correct script twice, until the repeat-detector gave up and killed the whole LIST item.
    static const QRegularExpression reuseBlockRe(
        R"(^([ \t]*)(\w+)[ \t]*=[ \t]*graphWindow\([^)]*\)[ \t]*\n\1if[ \t]+\2[ \t]+is[ \t]+None[ \t]*:[ \t]*\n)",
        QRegularExpression::MultilineOption);
    const QRegularExpressionMatch m = reuseBlockRe.match(code);
    if (!m.hasMatch())
        return {}; // no "reuse existing graph" pattern present

    const QString varName = m.captured(2);
    // The body is the run of lines indented STRICTLY deeper than the `if`. The first line back at
    // the `if`'s own level (or shallower) ends it — and a maximizeWindow() there is unconditional,
    // which is exactly what this check must not flag.
    const qsizetype baseDepth = m.captured(1).size();
    QString ifBlock;
    for (const QString &line : code.mid(m.capturedEnd(0)).split('\n'))
    {
        if (line.trimmed().isEmpty())
            continue; // blank lines belong to neither side
        const qsizetype depth = line.size() - static_cast<qsizetype>(QStringView(line).trimmed().size());
        if (depth <= baseDepth)
            break;
        ifBlock += line + '\n';
    }

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

QString QtiSasAiContent::curveExistenceGuardTitleMismatch(const QString &code)
{
    // Confirmed live: a guard like `if not any(l1.curveTitle(i) == "Data" for i in
    // range(l1.numCurves())): l1.insertCurve(...)` compares against a bare literal/plain
    // identifier — but curveTitle() has no setter anywhere in this API (no setCurveTitle()
    // exists), so a curve's title is ALWAYS auto-generated as "<table>_<column>" (confirmed live:
    // "GaussPeak_y", "fitCurve-ai/GaussPeak_y"). A bare label like "Data" or a bare model-name
    // variable (e.g. winner == "ai/GaussPeak") can never equal that auto-generated string, so
    // `not any(...)` is permanently True and insertCurve() runs UNCONDITIONALLY every time this
    // script executes — silently accumulating duplicate curves on every re-run with no error at
    // all. Confirmed live: the very next split-mode part's own EXPLORE round printed 4 curves in a
    // layer that should have held 2, each name duplicated exactly once, directly downstream of
    // exactly this guard shape.
    //
    // A bare identifier RHS is chased back to its LAST assignment before the guard (if any) rather
    // than judged as-is — a script that legitimately reconstructs the real title and factors it
    // into a variable first (`expected = t.objectName() + "_y"; ... curveTitle(i) == expected`)
    // must not be punished just for being one level of indirection removed from the confirmed-live
    // shape. Still flags the confirmed-live case itself (`winner = "ai/GaussPeakFit"` is, one level
    // removed, still just a hardcoded literal) and any bare name with no assignment found at all.
    static const QRegularExpression guardRe(
        R"((\w+)\.curveTitle\([^()]*\)\s*==\s*([^\n]+?)\s+for\s+\w+\s+in\s+range\(\1\.numCurves\(\)\)\)\s*:)");
    static const QRegularExpression bareLiteralRe(R"(^["'][^"']*["']$)");
    static const QRegularExpression bareIdentifierRe(R"(^\w+$)");
    QRegularExpressionMatchIterator it = guardRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString layerVar = m.captured(1);
        const QString rhs = m.captured(2).trimmed();
        // Allow anything that looks like a genuine dynamic reconstruction (concatenation, f-string,
        // a method call combining a table/model identifier with a column name) — only flag a bare
        // literal or a bare identifier reference, neither of which can ever match the real
        // "<table>_<column>" format.
        const bool isBareLiteral = bareLiteralRe.match(rhs).hasMatch();
        const bool isBareIdentifier = !isBareLiteral && bareIdentifierRe.match(rhs).hasMatch();
        if (!isBareLiteral && !isBareIdentifier)
            continue;
        if (isBareIdentifier)
        {
            // A bare identifier might legitimately hold a properly-reconstructed
            // "<table>_<column>" value assigned earlier in the script (e.g. `expected =
            // t.objectName() + "_y"; ... curveTitle(i) == expected`) — chase its LAST assignment
            // before this guard and judge THAT expression instead of the bare name itself, so a
            // genuine reconstruction factored into a variable isn't punished just for being one
            // level removed. Still flags the confirmed-live shape (`winner = "ai/GaussPeakFit"`,
            // itself just a hardcoded literal one level removed) and any name with no assignment
            // found at all (nothing grounds it as a real reconstruction either).
            const QString beforeGuard = code.left(m.capturedStart());
            const QRegularExpression assignRe(R"(\b)" + QRegularExpression::escape(rhs) + R"(\s*=\s*([^\n]+))");
            QString lastAssignedExpr;
            QRegularExpressionMatchIterator aIt = assignRe.globalMatch(beforeGuard);
            while (aIt.hasNext())
                lastAssignedExpr = aIt.next().captured(1).trimmed();
            if (!lastAssignedExpr.isEmpty() && !bareLiteralRe.match(lastAssignedExpr).hasMatch())
                continue; // assigned via something other than a bare literal — genuine reconstruction
        }
        const int guardEnd = static_cast<int>(m.capturedEnd());
        const QString trailing = code.mid(guardEnd, 200);
        const QRegularExpression insertRe(R"re(\b)re" + QRegularExpression::escape(layerVar) +
                                          R"re(\.(?:insertCurve|addCurve)\s*\()re");
        if (!insertRe.match(trailing).hasMatch())
            continue;
        return "This script guards `" + layerVar + ".insertCurve(...)` with `not any(" + layerVar +
               ".curveTitle(i) == " + rhs + " for i in range(" + layerVar +
               ".numCurves()))`, but curveTitle() has no setter anywhere in this API — every "
               "curve's title is ALWAYS auto-generated as \"<table>_<column>\" (confirmed live: "
               "\"GaussPeak_y\", \"fitCurve-ai/GaussPeak_y\"). A bare literal/identifier like " +
               rhs +
               " can never equal that auto-generated string, so this guard is permanently a "
               "no-op — insertCurve() runs unconditionally every time this script executes, "
               "silently accumulating duplicate curves on every re-run with no error at all. "
               "Either compare against the real auto-generated title (the table/curve identifier "
               "concatenated with the column name), or simply clear all curves unconditionally "
               "before rebuilding (for i in range(" +
               layerVar + ".numCurves()): " + layerVar +
               ".removeCurve(0)) instead of trying to detect which ones already exist.";
    }
    return {};
}

QString QtiSasAiContent::existingEntityVerificationPrompt(const QList<QPair<QString, QStringList>> &names,
                                                          bool includeDataProbe) const
{
    // Group by which check(s) actually apply — empty labels means genuinely unknown (all three).
    QStringList windowOnly, functionOnly, unknownKind, allQuoted;
    for (const QPair<QString, QStringList> &entry : names)
    {
        allQuoted << "\"" + entry.first + "\"";
        if (entry.second.isEmpty())
            unknownKind << entry.first;
        else if (entry.second.contains("window"))
            windowOnly << entry.first;
        else
            functionOnly << entry.first;
    }
    QStringList applicability;
    for (const QString &n : windowOnly)
        applicability << "\"" + n + "\": window check ONLY.";
    for (const QString &n : functionOnly)
        applicability << "\"" + n + "\": compile+fittable checks ONLY.";
    for (const QString &n : unknownKind)
        applicability << "\"" + n + "\": kind unknown — ALL THREE checks.";

    // Only describe a registry's check mechanics when some name actually needs it — otherwise an
    // unused-but-fully-explained check invites "it's right here, may as well run it too."
    const bool needsWindow = !windowOnly.isEmpty() || !unknownKind.isEmpty();
    const bool needsFunction = !functionOnly.isEmpty() || !unknownKind.isEmpty();
    // Both use an explicit if/else conditional expression, same shape as the VERDICT line below
    // (which has never shown this bug) — NOT the "A or B for name in LIST" phrasing this replaced,
    // which reads like a loop/generator, not a membership test, and is a plausible root cause of a
    // confirmed-live, 100%-reproducible bug: scripts printing BOTH the EXISTS and MISSING line for
    // the same name instead of exactly one.
    const QString windowText =
        "WINDOW — compute window_exists = name in [w.objectName() for w in windows()], then "
        "scriptPrint(\"(window) EXISTS: \" + name) if window_exists else scriptPrint(\"(window) "
        "MISSING: \" + name) — exactly ONE of the two, never both. Covers table/matrix/note/graph "
        "at once (one shared name registry; setWindowName() refuses a name any of them already "
        "uses). Never call existTable()/matrix()/note()/graphWindow() separately — same check, "
        "asked four times.";
    const QString functionText =
        "COMPILED-FUNCTION — compute compile_exists = name in Compiler.functionNames(\"ALL\") and "
        "fittable_exists = name in Fittable.fitFunctions(\"*\"), then scriptPrint(\"(compile) "
        "EXISTS: \" + name) if compile_exists else scriptPrint(\"(compile) MISSING: \" + name), "
        "and likewise for fittable_exists — exactly ONE EXISTS/MISSING line per label, never both "
        "for the same label";
    QString registryText;
    if (needsWindow && needsFunction)
        registryText = "Two independent registries: " + windowText + " And " + functionText +
                       " — a separate registry, never merged with the window check.";
    else if (needsWindow)
        registryText = windowText;
    else
        registryText = functionText + " — a separate, compiled-function-only registry.";

    // For a request that goes on to FIT or PLOT a named table, exists/missing is the wrong
    // question: a table can exist and still hold nothing. Confirmed live: "SAXS_data" was created
    // with 0 rows (named, roled, never filled), reported EXISTS here, and the fit then failed with
    // "fit(): failed to read data for the selected dataset(s)" ~90s later — a fact this script was
    // already standing in the right place to have reported for free. Only for window-kind names:
    // a compiled function has no rows, and only when the caller says this request will use the
    // data (a pure name-collision check has nothing to inspect).
    const QString dataProbeText =
        (includeDataProbe && needsWindow)
            ? QString("\n\nFor any name above that turns out to be an EXISTING TABLE, also print its "
                      "shape on its own extra line — scriptPrint(\"(rows) \" + name + \": \" + "
                      "str(t.numRows()) + \" rows, columns \" + str(t.colNames())) using t = "
                      "table(name) — because this request will fit or plot that table, and a table "
                      "that exists but holds 0 rows fails later inside fit()/addCurve() with an error "
                      "that never names the real cause. Still read-only, and it does not replace the "
                      "VERDICT line: print both.")
            : QString();

    // Anti-hallucination constraint first — the failure mode it guards against (a fabricated
    // VERDICT with no real check behind it) is the costliest one, so it comes before anything else.
    return "A bare scriptPrint(\"VERDICT: MISSING <name>\")/scriptPrint(\"VERDICT: EXISTS <name>\") "
           "with no real check call behind it is NOT valid — it's an unverified guess wearing the "
           "shape of a check, whether that's one check or several. Confirmed live: asked to verify "
           "\"Disk\" (a genuinely unknown-kind name, needing all three checks), the entire script "
           "was scriptPrint(\"MISSING: Disk\") — no windows()/functionNames()/fitFunctions() call "
           "anywhere, and wrong (\"Disk\" really was in Fittable.fitFunctions()). Every VERDICT "
           "below must come only after this script actually ran whichever check(s) apply to that "
           "name — not necessarily all three; see the per-name list below.\n\n"
           "Before doing anything else, verify whether each of the following name(s) — explicitly "
           "described in the request below as already EXISTING — actually exist: " +
           allQuoted.join(", ") +
           ".\n\nWrite ONE short, read-only Python script (no markdown fences, no explanation). " + registryText +
           "\n\nOnly run the check(s) that apply to EACH name below — never a check that doesn't "
           "apply, and never skip one that DOES apply just because another kind seems likelier "
           "from the name's own spelling/prefix:\n" +
           applicability.join("\n") +
           "\n\nStore each check you run in its own boolean, then compute that name's verdict as "
           "the OR of whichever booleans actually apply to it, and print exactly ONE final line "
           "per name: scriptPrint(\"VERDICT: EXISTS \" + name) if verdict_exists else "
           "scriptPrint(\"VERDICT: MISSING \" + name) — never a separate, freely-written "
           "conclusion; it must be computed from those same booleans, never contradicting them. Do "
           "not create, modify, delete, fit, or plot anything — a pure existence check. Do not "
           "attempt discovery/fuzzy/broadened matching (e.g. fitFunctions(\"*partial*\") guessing) "
           "— the EXACT name given, nothing else." +
           dataProbeText;
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

QString QtiSasAiContent::creationChainedMethodCallMistake(const QString &code)
{
    // Confirmed live: newMatrix("SphereMap", 50, 50).setFormula("sin(x)*cos(y)").calculate() —
    // AttributeError: 'NoneType' object has no attribute 'calculate'. newMatrix() itself returns
    // a real Matrix object, so the FIRST chained call (.setFormula(...)) works fine — the break
    // is one link later: setFormula() (like every setter in this API) returns None, not self, so
    // a THIRD link chained onto its result always fails. Every documented example for every
    // object type (Matrix, Table, Graph, ...) uses separate statements
    // (m = newX(...); m.setFormula(...); m.calculate()) for exactly this reason — this is a
    // general API-wide convention, not something specific to Matrix, so this checks any
    // new<Something>(...) constructor, not just newMatrix(). Repeated identically twice in the
    // same incident (retrying didn't change the model's assumption that chaining works), burning
    // the retry budget before this check existed.
    // Argument content is [^()"']* interleaved with whole "..."/'...' string literals, not a
    // blanket [^()]* — confirmed while writing this: the real incident's own argument,
    // "sin(x)*cos(y)", contains literal parens INSIDE the quoted string, which a naive [^()]*
    // excludes and would have made this check silently never match its own motivating example.
    static const QString arg = R"re((?:[^()"']|"[^"]*"|'[^']*')*)re";
    static const QRegularExpression chainRe(R"re(\bnew\w+\s*\()re" + arg + R"re(\)((?:\s*\.\s*\w+\s*\()re" + arg +
                                            R"re(\)){2,}))re");
    const QRegularExpressionMatch m = chainRe.match(code);
    if (!m.hasMatch())
        return {};
    return "This script chains multiple method calls directly onto a new<Something>(...) "
           "constructor call, e.g. \"" +
           m.captured(0) +
           "\". The constructor itself returns a real object, but every setter method in this "
           "API (setFormula(), setCoordinates(), setColNames(), ...) returns None, not self — "
           "chaining a THIRD call onto a setter's return value always raises \"AttributeError: "
           "'NoneType' object has no attribute '...'\". Assign the constructor's result to a "
           "variable and call each method as its own separate statement instead (e.g. "
           "m = newMatrix(...); m.setFormula(...); m.calculate()), matching every documented "
           "example for every object type in this API.";
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

QString QtiSasAiContent::matrixViewTypeIntMistake(const QString &code)
{
    // Confirmed live: m.setViewType(1) and m.setViewType(0) both raised TypeError: setViewType(
    // self, a0: Matrix.ViewType, renderImage: bool = True): argument 1 has unexpected type 'int'
    // — the OPPOSITE mistake shape from plotStyleStringArgumentMistake() above: unlike Qti3D's
    // plot-style constants (plain ints), Matrix.ViewType IS a genuine Python enum.Enum, and every
    // documented example uses the named member (matrix-python-api.md:
    // m.setViewType(Matrix.ViewType.ImageView)), never a bare int. Retried twice with different
    // bare ints before exhausting the retry budget and failing the whole request — the model had
    // used the correct enum form earlier in the SAME conversation, so this is inconsistent recall,
    // not a doc gap.
    static const QRegularExpression intViewTypeRe(R"(\bsetViewType\s*\(\s*\d+\s*[,)])");
    if (!intViewTypeRe.match(code).hasMatch())
        return {};
    return "This script passes a bare integer to setViewType() — it takes a Matrix.ViewType enum "
           "member only, never a plain int; this raises TypeError: argument 1 has unexpected type "
           "'int'. Use the named form instead, e.g. m.setViewType(Matrix.ViewType.ImageView) or "
           "m.setViewType(Matrix.ViewType.TableView) — never a bare 0/1.";
}

QString QtiSasAiContent::bareColumnNameInColumnsListMistake(const QString &code)
{
    // Confirmed live, repeatedly: columnsList() always returns QUALIFIED "TableName_column"
    // labels, never a bare column name, so "z" in columnsList(...) is unconditionally False
    // regardless of whether the role is actually set.
    static const QRegularExpression bareInColumnsListRe(
        R"((["'])([A-Za-z][A-Za-z0-9]*)\1\s+(?:not\s+)?in\s+columnsList\(\s*Table\.PlotDesignation\.\w+\s*\))");
    const QRegularExpressionMatch m = bareInColumnsListRe.match(code);
    if (!m.hasMatch())
        return {};
    const QString name = m.captured(2);
    return "This script checks \"" + name +
           "\" in columnsList(Table.PlotDesignation....) — that "
           "function returns QUALIFIED \"TableName_" +
           name +
           "\" labels, never the bare column name, "
           "so this comparison is always False even when the role IS set correctly. Check "
           "\"TableName_" +
           name +
           "\" in columnsList(...) instead (with the real table name), or "
           "any(n.endswith(\"_" +
           name + "\") for n in columnsList(...)).";
}

// Blanks triple-quoted string literals' interiors (kept same length/line count) so a usage scan
// below can't mistake text embedded in a note/script-content string for a live call in `code`.
static QString withTripleQuotedStringsBlanked(const QString &code)
{
    static const QRegularExpression tripleQuotedRe(R"("""[\s\S]*?"""|'''[\s\S]*?''')");
    QString result = code;
    QList<QRegularExpressionMatch> matches;
    QRegularExpressionMatchIterator it = tripleQuotedRe.globalMatch(code);
    while (it.hasNext())
        matches << it.next();
    for (auto matchIt = matches.crbegin(); matchIt != matches.crend(); ++matchIt)
    {
        QString blanked;
        blanked.reserve(matchIt->capturedLength());
        for (const QChar &ch : matchIt->captured())
            blanked += (ch == '\n') ? ch : QChar(' ');
        result.replace(matchIt->capturedStart(), matchIt->capturedLength(), blanked);
    }
    return result;
}

QString QtiSasAiContent::missingPyQtImportMistake(const QString &code)
{
    // Confirmed live, repeatedly — see selfReviewChecklist()'s own "Confirmed live" writeup a few
    // hundred lines up: the checklist already explicitly instructs self-review to check for exactly
    // this, but that's still prose an LLM pass can (and does) skip. A script calling QColor(...)/
    // QFont(...)/QPen(...)/QBrush(...)/QFrame(...), or referencing the Qt.* namespace, with no
    // matching PyQt import reaches execution unchanged and raises NameError, burning a retry round
    // guessing at an unrelated fix before anyone adds the missing import. Mirrors
    // fragmentedStaticScriptPrintMistake in spirit: a mechanical backstop for a checklist item prose
    // alone doesn't reliably enforce. Usage patterns require the class NOT be immediately preceded
    // by a dot or word character, so a qualified `QtGui.QColor(...)` (needing no separate `from ...
    // import` line) is never misflagged.
    const QString liveCode = withTripleQuotedStringsBlanked(code);
    QStringList missing;
    static const QStringList classCallNames = {"QColor", "QFont", "QPen", "QBrush", "QFrame"};
    for (const QString &className : classCallNames)
    {
        const QRegularExpression usageRe(R"((?<![.\w]))" + QRegularExpression::escape(className) + R"(\s*\()");
        if (!usageRe.match(liveCode).hasMatch())
            continue;
        const QRegularExpression importRe(R"(\bimport\b[^\n]*\b)" + QRegularExpression::escape(className) + R"(\b)");
        if (!importRe.match(liveCode).hasMatch())
            missing << className;
    }
    static const QRegularExpression qtNamespaceRe(R"((?<![.\w])Qt\.[A-Za-z_]\w*)");
    if (qtNamespaceRe.match(liveCode).hasMatch())
    {
        static const QRegularExpression qtImportRe(R"(\bimport\b[^\n]*\bQt\b)");
        if (!qtImportRe.match(liveCode).hasMatch())
            missing << "Qt";
    }
    if (missing.isEmpty())
        return {};
    return "This script uses " + missing.join(", ") +
           " with no matching PyQt import line — none of these are in the default namespace, so "
           "using them unimported raises NameError at runtime. Add `from PyQt6.QtGui import " +
           missing.join(", ") +
           "` (QFrame actually comes from PyQt6.QtWidgets, Qt from "
           "PyQt6.QtCore — split across the right import lines, don't put them all in one) instead "
           "of leaving this for a runtime retry to catch. EXCEPTION: a bare 0xRRGGBB int needs no "
           "import and is a valid substitute specifically for QColor(...), never for the others.";
}

QString QtiSasAiContent::unrequestedRandomDataMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (supervised LIST 3, item 7, local model): "y = 2x + 1, dy = 0.1" was filled with
    // y = 2*x + 1 + random.gauss(0, 0.1), so the fit printed a = 2.0013, b = 0.9962 instead of 2.0000 and 1.0000 — a
    // silent wrong result. Data the request gives exactly must not get noise the request never asked for.
    static const QRegularExpression randomCallRe(R"(\b(?:random|np\.random|numpy\.random)\.\w+\s*\()");
    static const QRegularExpression noiseAskedRe(R"(\b(?:nois[ey]\w*|random\w*|jitter\w*|perturb\w*|scatter\w*)\b)",
                                                 QRegularExpression::CaseInsensitiveOption);
    if (!randomCallRe.match(code).hasMatch() || noiseAskedRe.match(originalRequestText).hasMatch())
        return {};
    return "This script adds random values (random.<...>() / numpy.random) to the data, but the request never asks for "
           "noise or random numbers — it gives the values exactly. Fill the cells with exactly the requested values; "
           "noise changes every result computed from them.";
}

QString QtiSasAiContent::columnListInsteadOfAggregateMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (supervised LIST 3, item 1, local model): "Print the sum of column 2" was printed as
    // scriptPrint("R1:", t.colData(2)) — the whole column instead of one number, accepted as a successful run.
    static const QRegularExpression aggregateAskedRe(
        R"(\b(?:sum|total|mean|average|maximum|minimum|number of|count)\b)", QRegularExpression::CaseInsensitiveOption);
    if (!aggregateAskedRe.match(originalRequestText).hasMatch())
        return {};
    // a print whose argument is a bare colData(...) list (not inside sum(), len(), max(), min(), ...)
    static const QRegularExpression printLineRe(R"((?:scriptPrint|print)\s*\((.*)\))");
    static const QRegularExpression bareColDataRe(R"((^|[,(\s])\w+\.colData\s*\()");
    static const QRegularExpression wrappedRe(
        R"(\b(?:sum|len|max|min|mean|round|abs|sorted|statistics\.\w+)\s*\(\s*$)");
    for (const QString &line : code.split('\n'))
    {
        const QRegularExpressionMatch print = printLineRe.match(line);
        if (!print.hasMatch())
            continue;
        const QString args = print.captured(1);
        for (auto it = bareColDataRe.globalMatch(args); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            const QString before = args.left(m.capturedStart() + m.captured(1).size());
            if (!wrappedRe.match(before).hasMatch())
                return "The request asks for a single value (a sum, count, mean, maximum or minimum), but this script "
                       "prints a whole column — colData(...) is the list of all cell values. Compute the value first, "
                       "e.g. sum(t.colData(n)) or len(...), and print that number.";
        }
    }
    return {};
}

// The label the request asks the result line to start with ("The result label is \"R3:\"" -> "R3:"), or empty.
QString QtiSasAiContent::requestedResultLabel(const QString &originalRequestText)
{
    static const QRegularExpression labelRe(R"RX(\blabel\b[^"\n]{0,20}"([^"\s]{1,12}:)")RX",
                                            QRegularExpression::CaseInsensitiveOption);
    return labelRe.match(originalRequestText).captured(1);
}

QString QtiSasAiContent::resultLabel(const QString &requestText) const
{
    return requestedResultLabel(requestText);
}

QString QtiSasAiContent::resultLineDuty(const QString &requestText) const
{
    return resultLineDutyOf(requestText);
}

QString QtiSasAiContent::resultLineDutyOf(const QString &requestText)
{
    // From the last "Print" sentence before the label sentence through the label sentence: 'Print the value at the
    // centre ..., and whether a 3D window "Saddle3D" exists. The result label is "R4:".'
    const QString label = requestedResultLabel(requestText);
    if (label.isEmpty())
        return {};
    static const QRegularExpression labelSentenceRe(R"RX(The result label is "[^"\n]{1,13}"\.?)RX",
                                                    QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch labelMatch = labelSentenceRe.match(requestText);
    if (!labelMatch.hasMatch())
        return {};
    static const QRegularExpression printRe(R"((?:^|[.:!?]\s+|\n)(Print\b))");
    const QString beforeLabel = requestText.left(labelMatch.capturedStart());
    qsizetype printStart = -1;
    for (auto it = printRe.globalMatch(beforeLabel); it.hasNext();)
        printStart = it.next().capturedStart(1);
    if (printStart < 0)
        return {};
    return requestText.mid(printStart, labelMatch.capturedEnd() - printStart).simplified();
}

QStringList QtiSasAiContent::requestedResultValues(const QString &requestText)
{
    // The values the result line is asked for, one entry each, from the Print sentence of resultLineDutyOf(): "Print a,
    // b and c with 4 decimals, and the number of rows where y is greater than 5." gives four. A sentence lists its
    // values separated by commas, "and" and "then"; text in parentheses explains a value ("(row 11, column 11)", "(a
    // list, [] if none)"), "each with 4 decimals" formats them all, and "row 3, column 4" is one cell. Only a lower
    // bound: "the three texts" counts as one, so a check may only ever ask for more values, never fewer.
    QString sentence = resultLineDutyOf(requestText);
    if (sentence.isEmpty())
        return {};
    static const QRegularExpression printRe(R"(^Print\s+)", QRegularExpression::CaseInsensitiveOption);
    sentence.remove(printRe);
    // up to the end of the Print sentence: a period followed by a space and a capital letter, outside quotes
    static const QRegularExpression endRe(R"RX(\.\s+(?=[A-Z])|\.$)RX");
    QString cut;
    bool inQuote = false;
    for (qsizetype i = 0; i < sentence.size(); ++i)
    {
        const QChar ch = sentence.at(i);
        if (ch == QLatin1Char('"'))
            inQuote = !inQuote;
        if (!inQuote && ch == QLatin1Char('.') && endRe.match(sentence.mid(i, 3)).capturedStart() == 0)
            break;
        cut += ch;
    }
    static const QRegularExpression parenRe(R"(\([^()]*\))");
    cut.replace(parenRe, QStringLiteral(" "));
    static const QRegularExpression splitRe(R"(\s*,\s*(?:and\s+|then\s+)?|\s+and\s+|\s+then\s+)");
    static const QRegularExpression eachRe(R"(^each\b)", QRegularExpression::CaseInsensitiveOption);
    // an order for the values, no value of its own: "whether each still exists afterwards, table first" (LIST 4, item
    // 6) became the values "whether each still exists afterwards" and "table first"
    static const QRegularExpression orderRe(R"(^(?:the\s+)?\S+\s+first$|^(?:in\s+)?(?:that|this|the\s+same)\s+order$)",
                                            QRegularExpression::CaseInsensitiveOption);
    // a format for the values, no value of its own: "S22 and its error at Q = 0.05, rounded to 4 decimals" asks for
    // two values — confirmed live (benchmark SVD items 1 and 5, Codex): counted as a third, the right line was
    // blocked and the model added the unrounded value to fill it
    static const QRegularExpression formatRe(
        R"(^(?:(?:each|all|both)\s+)?(?:rounded\s+)?(?:to|with|in)\s+(?:\d+|an?|one|two|three|four|five|six)\s+)"
        R"((?:decimals?|decimal\s+places?|(?:significant\s+)?(?:digits?|figures?)|integers?)$|^rounded$)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression coordinateRe(R"(^(?:row|column|col)\s+\d)",
                                                 QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression hasCoordinateRe(R"(\b(?:row|column|col)\s+\d)",
                                                    QRegularExpression::CaseInsensitiveOption);
    // "the first and last Q" is two values of one noun: "the first" alone named none (and read as an order, "the
    // ... first"), so the model printed only one Q — confirmed live, benchmark ASCII1D items 3 and 8: "R3: 15 0.075"
    // for the rows, the first and the last Q
    static const QRegularExpression bareOrdinalRe(
        R"(^(?:the\s+)?(?:first|last|smallest|largest|lowest|highest|minimum|maximum|min|max)$)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression leadingOrdinalRe(
        R"(^(?:the\s+)?(?:first|last|smallest|largest|lowest|highest|minimum|maximum|min|max)\s+)",
        QRegularExpression::CaseInsensitiveOption);
    QStringList parts;
    for (QString part : cut.split(splitRe))
    {
        part = part.simplified();
        if (part.isEmpty() || eachRe.match(part).hasMatch() || formatRe.match(part).hasMatch() ||
            (orderRe.match(part).hasMatch() && !bareOrdinalRe.match(part).hasMatch()))
            continue;
        if (!parts.isEmpty() && coordinateRe.match(part).hasMatch() && hasCoordinateRe.match(parts.last()).hasMatch())
        {
            parts.last() += ", " + part;
            continue;
        }
        parts << part;
    }
    for (qsizetype i = parts.size() - 2; i >= 0; --i)
        if (bareOrdinalRe.match(parts.at(i)).hasMatch())
        {
            const QString noun = QString(parts.at(i + 1)).remove(leadingOrdinalRe);
            if (noun != parts.at(i + 1))
                parts[i] += " " + noun;
        }
    return parts;
}

QString QtiSasAiContent::earlierPartVariableMistake(const QString &code, const QString &earlierPartsCode)
{
    // Confirmed live (LIST 5, item 4): part 2 printed m.cell(11, 11) with the "m" of part 1's
    // m = newMatrix("Saddle", ...) — NameError, one round. Every part runs as a script of its own.
    if (earlierPartsCode.isEmpty())
        return {};
    const QString ownCode = codeWithoutStringLiterals(code);
    static const QRegularExpression earlierAssignRe(R"(^([A-Za-z_]\w*)\s*=(?!=)\s*([^\n]*))",
                                                    QRegularExpression::MultilineOption);
    // the lookup for what the earlier part created: newMatrix("Saddle", ...) -> matrix("Saddle")
    static const QRegularExpression creationRe(R"RX(^new(Table|Matrix|Note|Plot3D|Graph)\s*\(\s*("[^"]*"|'[^']*'))RX");
    for (auto it = earlierAssignRe.globalMatch(earlierPartsCode); it.hasNext();)
    {
        const QRegularExpressionMatch earlier = it.next();
        const QString name = earlier.captured(1);
        const QString n = QRegularExpression::escape(name);
        const QRegularExpression usedRe(R"((?<![.\w]))" + n + R"(\s*\.\s*\w+)");
        if (!usedRe.match(ownCode).hasMatch())
            continue;
        // bound in this script: assigned, a loop/with/except target, imported as, a def/lambda parameter, global
        const QRegularExpression boundRe(
            R"((?<![.\w]))" + n + R"(\s*(?:,[^=\n]*)?=(?!=)|\bfor\s+(?:(?!\bin\b)[^\n])*\b)" + n +
            R"(\b(?:(?!\bin\b)[^\n])*\bin\b|\bas\s+)" + n + R"(\b|\b(?:def|lambda|global)\b[^\n]*\b)" + n + R"(\b)");
        if (boundRe.match(ownCode).hasMatch())
            continue;
        const QString rhs = earlier.captured(2).trimmed();
        const QRegularExpressionMatch created = creationRe.match(rhs);
        QString lookup = created.hasMatch() ? created.captured(1) + "(" + created.captured(2) + ")" : rhs;
        if (created.hasMatch())
            lookup[0] = lookup[0].toLower();
        if (created.hasMatch() && created.captured(1) == "Graph")
            lookup = "graphWindow(" + created.captured(2) + ")";
        return "This script uses \"" + name + "\" (" + name + ".…), but never assigns it: \"" + name +
               "\" was assigned in an EARLIER part's script, and every part runs as a script of its own, so here it is "
               "undefined (NameError). Assign it again at the top of this script" +
               (lookup.isEmpty() || lookup.size() > 80 ? QString(".") : ": " + name + " = " + lookup);
    }
    return {};
}

QString QtiSasAiContent::matrixIndexFormulaMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (benchmark LIST 5, item 4): "spanning x and y from -1 to 1 and fill it with x*x - y*y" got
    // setCoordinates(-1, 1, -1, 1) and then setFormula("i*i-j*j") — the row/column indices, values -440 to 440.
    // Blocked, not repaired: i is the row (y) and j the column (x), so which one the model meant is a guess.
    static const QRegularExpression matrixRe(R"(\b(?:newMatrix|matrix)\s*\()");
    static const QRegularExpression requestXyRe(R"(\bx\s*[*^/+\-]|[*^/+\-]\s*x\b|\by\s*[*^/+\-]|[*^/+\-]\s*y\b)");
    static const QRegularExpression formulaRe(R"RX(\.\s*setFormula\s*\(\s*(?:"([^"]*)"|'([^']*)'))RX");
    static const QRegularExpression indexRe(R"(\b[ij]\b)");
    static const QRegularExpression coordinateRe(R"(\b[xy]\b)");
    if (!matrixRe.match(code).hasMatch() || !requestXyRe.match(originalRequestText).hasMatch())
        return {};
    for (auto it = formulaRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString formula = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (indexRe.match(formula).hasMatch() && !coordinateRe.match(formula).hasMatch())
            return "setFormula(\"" + formula +
                   "\") uses i and j, but in a matrix formula i and j are the row and column INDICES "
                   "(1, 2, 3, ...), not coordinates. The request gives the formula in x and y: those are the "
                   "coordinates set with setCoordinates(...). Write the formula with x and y, as the request does.";
    }
    return {};
}

QString QtiSasAiContent::newNoteExtraTabMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (benchmark LIST 5, item 5): "a note with two tabs" got newNote() and then addTab() twice — 3 tabs.
    // A new note already has one tab (note-python-api.md).
    static const QRegularExpression requestTabsRe(R"(\b(two|three|four|five|2|3|4|5)\s+(?:\w+\s+)?tabs\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression newNoteRe(R"(\bnewNote\s*\()");
    static const QRegularExpression addTabRe(R"(\.\s*addTab\s*\()");
    const QRegularExpressionMatch requested = requestTabsRe.match(originalRequestText);
    const QRegularExpressionMatch created = newNoteRe.match(code);
    if (!requested.hasMatch() || !created.hasMatch())
        return {};
    static const QStringList words{"two", "three", "four", "five"};
    const QString n = requested.captured(1).toLower();
    const int tabs = words.contains(n) ? static_cast<int>(words.indexOf(n)) + 2 : n.toInt();
    int added = 0;
    for (auto it = addTabRe.globalMatch(code, created.capturedEnd()); it.hasNext(); it.next())
        ++added;
    if (added < tabs)
        return {};
    return QString("The request asks for %1 tabs, but this script calls addTab() %2 times after newNote(): a new note "
                   "already HAS one tab, so it ends with %3. Call addTab() %4 time(s), and give the first tab its "
                   "name with renameTab(0, ...).")
        .arg(tabs)
        .arg(added)
        .arg(added + 1)
        .arg(tabs - 1);
}

// The arguments of a call, from the text right after its "(": split at top-level commas, up to its closing ")".
static QStringList topLevelArguments(const QString &afterParen)
{
    QStringList args;
    QString current;
    int depth = 0;
    QChar quote;
    for (const QChar ch : afterParen)
    {
        if (!quote.isNull())
        {
            current += ch;
            if (ch == quote)
                quote = QChar();
            continue;
        }
        if (ch == '"' || ch == '\'')
            quote = ch;
        else if (ch == '(' || ch == '[' || ch == '{')
            ++depth;
        else if (ch == ')' || ch == ']' || ch == '}')
        {
            if (depth == 0)
                break;
            --depth;
        }
        else if (ch == ',' && depth == 0)
        {
            args << current.trimmed();
            current.clear();
            continue;
        }
        current += ch;
    }
    if (!current.trimmed().isEmpty())
        args << current.trimmed();
    return args;
}

QString QtiSasAiContent::staleResultValueMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live three times (local LIST 1, items 3 and 11): has_legend = top_layer.legend() is not None, then
    // top_layer.newLegend(), then scriptPrint("R11:", ..., has_legend) — the object was right, the result line printed
    // the state before the work. The docs' read-after-work rule did not stop it. A dry run over every script that ran
    // (6413, all benchmark logs and reference solutions) flagged exactly these cases and nothing correct.
    // A value read and then changed by a later call on the SAME object; only object reads, never app-wide settings,
    // and never when the request itself asks for a value before a change ("... now, and switch it back afterwards").
    static const QList<QPair<QString, QStringList>> pairs = {
        {"legend", {"newLegend", "removeLegend"}},
        {"tabs", {"addTab", "removeTab"}},
        {"autoexec", {"setAutoexec"}},
        {"numCurves", {"insertCurve", "addCurve", "addCurves", "removeCurve", "deleteFitCurves", "plotResults"}},
        {"numLayers", {"addLayer", "setNumLayers", "deleteLayer", "removeLayer"}},
        {"numRows", {"setNumRows", "insertRows", "deleteRows", "addRows"}},
        {"numCols", {"setNumCols", "addColumn", "addColumns", "insertColumns", "removeColumn", "deleteColumn"}}};
    static const QRegularExpression beforeAskedRe(
        R"(\b(?:before|afterwards?|initially|originally|at first|restore|switch(?:es|ed)?(?: it)? back|old value|)"
        R"(previous value)\b)",
        QRegularExpression::CaseInsensitiveOption);
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty() || beforeAskedRe.match(originalRequestText).hasMatch())
        return {};
    QStringList readNames;
    for (const auto &pair : pairs)
        readNames << pair.first;
    static const QRegularExpression assignRe(R"(^\s*(\w+)\s*=(?!=)\s*(.+)$)");
    const QRegularExpression readRe(R"(\b(\w+)\s*\.\s*()" + readNames.join('|') + R"()\s*\()");
    static const QRegularExpression printRe(R"(^\s*scriptPrint\s*\((.*)$)");
    static const QRegularExpression fieldRe(R"(\{\s*(\w+)\s*(?:[:!][^}]*)?\})");
    static const QRegularExpression identRe(R"(^\w+$)");
    static const QRegularExpression wordRe(R"(\b(\w+)\b)");
    const QStringList lines = code.split('\n');
    // the read call in line `index`, or in the line that assigned one of its names (one step back)
    const auto readIn = [&](int index) -> QRegularExpressionMatch {
        const QString expr = assignRe.match(lines.at(index)).captured(2);
        QRegularExpressionMatch direct = readRe.match(expr);
        if (direct.hasMatch())
            return direct;
        for (auto w = wordRe.globalMatch(expr); w.hasNext();)
        {
            const QString ident = w.next().captured(1);
            for (int j = index - 1; j >= 0; --j)
            {
                const QRegularExpressionMatch assign = assignRe.match(lines.at(j));
                if (assign.hasMatch() && assign.captured(1) == ident)
                {
                    const QRegularExpressionMatch read = readRe.match(assign.captured(2));
                    if (read.hasMatch())
                        return read;
                    break;
                }
            }
        }
        return {};
    };
    for (int p = 0; p < lines.size(); ++p)
    {
        const QRegularExpressionMatch print = printRe.match(lines.at(p));
        if (!print.hasMatch())
            continue;
        const QStringList args = topLevelArguments(print.captured(1));
        if (args.isEmpty() || !args.first().contains(label))
            continue;
        QStringList printed;
        if (args.first().startsWith('f'))
            for (auto f = fieldRe.globalMatch(args.first()); f.hasNext();)
                printed << f.next().captured(1);
        for (int a = 1; a < args.size(); ++a)
            if (identRe.match(args.at(a)).hasMatch())
                printed << args.at(a);
        for (const QString &var : printed)
        {
            int assigned = -1;
            for (int i = p - 1; i >= 0 && assigned < 0; --i)
            {
                const QRegularExpressionMatch assign = assignRe.match(lines.at(i));
                if (assign.hasMatch() && assign.captured(1) == var)
                    assigned = i;
            }
            if (assigned < 0)
                continue;
            const QRegularExpressionMatch read = readIn(assigned);
            if (!read.hasMatch())
                continue;
            const QString object = read.captured(1);
            const QString method = read.captured(2);
            QStringList changes;
            for (const auto &pair : pairs)
                if (pair.first == method)
                    changes = pair.second;
            const QRegularExpression changeRe("\\b" + QRegularExpression::escape(object) + R"(\s*\.\s*()" +
                                              changes.join('|') + R"()\s*\()");
            for (int i = assigned + 1; i < p; ++i)
            {
                const QRegularExpressionMatch change = changeRe.match(lines.at(i));
                if (!change.hasMatch())
                    continue;
                return "The result line prints " + var + ", read with " + object + "." + method + "() before " +
                       object + "." + change.captured(1) +
                       "() changes it later in this script — so it prints the state BEFORE the work, not the result. "
                       "Read the value after the work, in the result line itself: replace " +
                       var + " in scriptPrint(\"" + label + "\", ...) by a fresh " + object + "." + method +
                       "() read (e.g. " + object + "." + method + "() is not None for whether it exists).";
            }
        }
    }
    return {};
}

QString QtiSasAiContent::labeledResultLineMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (benchmark LIST 5, item 6): "R6: [] Peak5:missing Pipeline:missing" and "R6: [] Table 'Peak5'
    // exists: False ..." — the request asks for the values only. The fixed text of the result line's print (string
    // literals, f-string text outside {...}) after the label must not hold a "name:" or "name=" label.
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty())
        return {};
    const QRegularExpression printRe(R"(^[ \t]*(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                         QRegularExpression::escape(label) + "[^\\n]*$",
                                     QRegularExpression::MultilineOption);
    static const QRegularExpression literalRe(R"RX(f?("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'))RX");
    static const QRegularExpression fieldRe(R"(\{[^{}]*\})");
    static const QRegularExpression innerLabelRe(R"((?:^|\s)([A-Za-z_][\w']*)\s*[:=](?!=))");
    // Only a line that prints values can label them. Confirmed live (local LIST 1, item 7): the unreachable fallback
    // branch scriptPrint("R7:", "Error calculating I at Q=0.1") was blocked for a label "Q:", the model kept the line
    // it could not see wrong, and the item stopped on the same diagnosis twice — its computed line was correct.
    static const QRegularExpression printCallRe(R"(^[ \t]*(?:scriptPrint|print)\s*\()");
    static const QRegularExpression valueRe(R"([A-Za-z0-9_])");
    for (auto it = printRe.globalMatch(code); it.hasNext();)
    {
        const QString line = it.next().captured(0);
        QString fixedText;
        bool printsValue = valueRe.match(QString(line).remove(printCallRe).remove(literalRe)).hasMatch();
        for (auto lit = literalRe.globalMatch(line); lit.hasNext();)
        {
            const QRegularExpressionMatch literal = lit.next();
            QString text = literal.captured(1);
            text = text.mid(1, text.size() - 2);
            if (literal.captured(0).startsWith('f') && fieldRe.match(text).hasMatch())
                printsValue = true;
            text.replace(fieldRe, " ");
            fixedText += " " + text;
        }
        if (!printsValue)
            continue;
        fixedText.replace(label, " ");
        // a failure report, its text starting with the failure, is no result line: confirmed live (benchmark SVD, local
        // pyi, item 1) — the guard branch scriptPrint(f"R1: Missing tables: {...}") was blocked for "tables:" until
        // the item was given up; the computed line beside it was right. "Peak5:missing" still labels a value
        static const QRegularExpression failureRe(
            R"(^\s*(?:error|missing|failed|cannot|could\s*not|unable|unavailable|)"
            R"(no\s+such|not\s+found|\w+\s+not\s+found))",
            QRegularExpression::CaseInsensitiveOption);
        if (failureRe.match(fixedText).hasMatch())
            continue;
        const QRegularExpressionMatch inner = innerLabelRe.match(fixedText);
        if (inner.hasMatch())
            return "The result line " + line.trimmed() + " labels its values (\"" + inner.captured(1) +
                   ":\"), but the request asks for the values only, in the order asked: print \"" + label +
                   "\" and then just the values, e.g. scriptPrint(\"" + label + "\", value1, value2).";
    }
    return {};
}

QString QtiSasAiContent::inventedDeletionCallHint(const QString &errContext)
{
    // Confirmed live (benchmark LIST 5, item 6): deleteFunction, removeFitFunction, deleteTable, deleteWindow,
    // closeTable — one invented name after the other, 705 s for a cleanup item. None exists, so the console's "a
    // method of ..." hint had nothing to point at.
    static const QRegularExpression windowRe(
        R"((?:NameError: name|has no attribute) )"
        R"('((?:delete|close|remove|kill)\w*(?:Table|Window|Note|Matrix|Graph|Plot)\w*)')",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression functionRe(
        R"((?:NameError: name|has no attribute) '((?:delete|remove|unload)\w*Function\w*)')",
        QRegularExpression::CaseInsensitiveOption);
    if (const QRegularExpressionMatch m = windowRe.match(errContext); m.hasMatch())
        return "REMINDER: \"" + m.captured(1) +
               "\" does not exist. closeWindow(w) closes a table, matrix, note or graph window AND removes it "
               "from the project — that is the whole \"close and delete\": closeWindow(table(\"T\")), "
               "closeWindow(note(\"N\")). Afterwards table(\"T\") is None.\n\n";
    if (const QRegularExpressionMatch m = functionRe.match(errContext); m.hasMatch())
        return "REMINDER: \"" + m.captured(1) +
               "\" does not exist. A compiled fit function is deleted with Compiler.delete(\"ai/Name\"); "
               "whether it still exists: \"ai/Name\" in Compiler.functionNames(\"ALL\").\n\n";
    return {};
}

QString QtiSasAiContent::otherTabRunMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 5, item 5): "Run only the "make" tab" — the plan and the script also ran the "check" tab.
    static const QRegularExpression onlyRe(R"RX(\brun\s+only\s+(?:the\s+)?"([^"]+)"\s+tab\b)RX",
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch only = onlyRe.match(originalRequestText);
    if (!only.hasMatch())
        return {};
    const QString allowed = only.captured(1);
    // tab names by 0-based index, from renameTab(i, "name") in this script; tab(int) is 1-based
    QMap<int, QString> names;
    static const QRegularExpression renameRe(R"RX(\brenameTab\s*\(\s*(\d+)\s*,\s*["']([^"']+)["'])RX");
    for (auto it = renameRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch r = it.next();
        names[r.captured(1).toInt()] = r.captured(2);
    }
    static const QRegularExpression runRe(
        R"RX(\b(?:tab\s*\(\s*(?:["']([^"']+)["']|(\d+))|editorAt\s*\(\s*(\d+))\s*\)\s*\.\s*execute\s*\()RX");
    for (auto it = runRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch r = it.next();
        QString ran = r.captured(1);
        if (ran.isEmpty() && !r.captured(2).isEmpty())
            ran = names.value(r.captured(2).toInt() - 1);
        if (ran.isEmpty() && !r.captured(3).isEmpty())
            ran = names.value(r.captured(3).toInt());
        if (!ran.isEmpty() && ran != allowed)
            return "The request says to run ONLY the \"" + allowed + "\" tab, but this script also runs the \"" + ran +
                   "\" tab. Remove that execute() call: the other tab's script is written into it, not run.";
    }
    return {};
}

QString QtiSasAiContent::noteAutoexecReceiverRepair(const QString &code) const
{
    // Confirmed live (series 4, LIST 3 item 10, local, NOSPLIT): n.tab(1).setAutoexec(False) and n.tab(2).setAutoexec(
    // False) — setAutoexecOnScriptEditMistake() blocked it, the model moved ONE call to n and kept the other, the same
    // block came back and the item was given up. setAutoexec() exists only on the Note, so the fix has one form: the
    // editor in between goes. Code only, not a tab's script text.
    static const QRegularExpression wrongObjectRe(
        R"re(\b[A-Za-z_]\w*(?:\s*\([^()\n]*\))?(\s*\.\s*(?:currentEditor\s*\(\s*\))re"
        R"re(|tab\s*\([^()\n]*\)|editorAt\s*\([^()\n]*\)))\s*\.\s*setAutoexec\s*\()re");
    const QString masked = codeWithLiteralsBlanked(code);
    QList<QPair<qsizetype, qsizetype>> spans; // [start, end) of each ".tab(...)" between the note and setAutoexec
    for (auto it = wrongObjectRe.globalMatch(masked); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        spans << qMakePair(m.capturedStart(1), m.capturedEnd(1));
    }
    if (spans.isEmpty())
        return {};
    QString repaired = code;
    for (int i = static_cast<int>(spans.size()) - 1; i >= 0; --i)
        repaired.remove(spans[i].first, spans[i].second - spans[i].first);
    return repaired;
}

QString QtiSasAiContent::bareCompilerDeleteRepair(const QString &code) const
{
    // Confirmed live (split-mode LIST 5, item 6, two runs in a row): delete("ai/Quad") — NameError, the error itself
    // naming Compiler.delete(...), the same line sent back until the item was given up. There is no global delete():
    // a call of that name, in a script that defines none, can only be Compiler's. Code only, not a tab's script text.
    static const QRegularExpression callRe(R"((?<![.\w])delete\s*\()");
    static const QRegularExpression definedRe(
        R"(\bdef\s+delete\b|(?<![.\w])delete\s*=(?!=)|\bimport\b[^\n]*\bdelete\b)");
    // A mask of the same length (every string literal's characters blanked): codeWithoutStringLiterals() shortens the
    // text, and its offsets applied to `code` put "Compiler." inside other words — confirmed live, "deCompiler.lete(".
    const QString stripped = codeWithLiteralsBlanked(code);
    if (!callRe.match(stripped).hasMatch() || definedRe.match(stripped).hasMatch())
        return {};
    // Replace only the calls outside string literals: walk the code with the masked text.
    QString repaired;
    qsizetype last = 0;
    for (auto it = callRe.globalMatch(stripped); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        repaired += code.mid(last, m.capturedStart() - last) + "Compiler.";
        last = m.capturedStart();
    }
    return repaired + code.mid(last);
}

QString QtiSasAiContent::recalculateExceptionNameMistake(const QString &code)
{
    // Confirmed live (LIST 3 item 3, every configuration, 2026-10-11): "the name of the Python error the formula
    // caused" printed as type(e).__name__ of the exception recalculate() raised — always ValueError, because
    // recalculate() wraps the formula's error; table-python-api says the formula's own error is the message's second
    // line, and the model read past it every time.
    static const QRegularExpression recalcRe(R"(\.\s*recalculate\s*\()");
    static const QRegularExpression nameRe(
        R"(\btype\s*\(\s*(\w+)\s*\)\s*\.\s*__name__|\b(\w+)\s*\.\s*__class__\s*\.\s*__name__)");
    const QString stripped = codeWithoutStringLiterals(code);
    if (!recalcRe.match(stripped).hasMatch())
        return {};
    const QRegularExpressionMatch m = nameRe.match(stripped);
    if (!m.hasMatch())
        return {};
    const QString e = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
    // A script that reads the message's lines already takes the formula's error; type().__name__ is then only its
    // fallback. Confirmed live (series 5, LIST 3 item 3): error_lines = str(e).splitlines(), type(e).__name__ only
    // when there is no second line — blocked twice anyway and the item given up.
    const QRegularExpression linesRe(R"(\bstr\s*\(\s*)" + QRegularExpression::escape(e) +
                                     R"(\s*\)\s*\.\s*(?:splitlines|split)\s*\()");
    if (linesRe.match(stripped).hasMatch())
        return {};
    return m.captured(0) +
           " of the exception recalculate() raised is always ValueError: recalculate() wraps the "
           "formula's own error. That error is the message's second line, and its name is str(" +
           e + ").splitlines()[1].split(\":\")[0] (e.g. ZeroDivisionError) — use that for the formula's error.";
}

QString QtiSasAiContent::bareObjectMethodMistake(const QString &code)
{
    // Confirmed live (LIST 3 item 3, 2026-10-12, all four local runs): t.setCommand(4, ...) then a BARE recalculate(4)
    // inside try/except — the NameError was caught and printed as the formula's error, no cell computed. These are
    // Table/Matrix methods with no global of the same name; a script that defines one itself is left alone.
    static const QRegularExpression callRe(
        R"((?<![.\w])(recalculate|setCommand|setCell|setColNames|setColumnRole|setColTextFormat)"
        R"(|setReadOnlyColumn|colNames|numRows|numCols|setFormula|calculate)\s*\()");
    const QString stripped = codeWithLiteralsBlanked(code);
    for (auto it = callRe.globalMatch(stripped); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1);
        const QRegularExpression definedRe(R"(\bdef\s+)" + name + R"(\b|(?<![.\w]))" + name +
                                           R"(\s*=(?!=)|\bimport\b[^\n]*\b)" + name + R"(\b)");
        if (definedRe.match(stripped).hasMatch())
            continue;
        return name + "() is called without its table/matrix — there is no global " + name +
               "(): it raises NameError (and an except around it hides that). Call it on the object: t." + name +
               "(...), e.g. t.recalculate(4).";
    }
    return {};
}

QString QtiSasAiContent::existenceReadBeforeCreationMistake(const QString &code)
{
    // Confirmed live twice (LIST 5 item 4, 2026-10-11, NOSPLIT; ai-python-api says it): g3_exists =
    // existPlot3D("Saddle3D") — or g3 = plot3D("Saddle3D") — read BEFORE newPlot3D("Saddle3D", ...), then printed:
    // "False" for a window the script did create. A value of name N read before the call that creates N, and printed
    // after it.
    static const QRegularExpression readRe(
        R"RX(^[ \t]*(\w+)\s*=\s*(?:not\s+)?(?:exist(?:Table|Matrix|Note|Graph|Plot3D)RX"
        R"RX(|Window)|table|matrix|note|graphWindow|plot3D)\s*\(\s*["']([^"'\n]+)["']\s*\))RX",
        QRegularExpression::MultilineOption);
    for (auto it = readRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch read = it.next();
        const QString var = QRegularExpression::escape(read.captured(1));
        const QString name = QRegularExpression::escape(read.captured(2));
        // The creating call, or the deleting one: closeWindow() of the name's object or of the variable itself.
        // Confirmed live (series 4, LIST 5 item 6, NOSPLIT): t = table("Peak5"); closeWindow(t); ... then
        // scriptPrint("R6:", ..., t is not None) — "True" for a table the script had just deleted.
        const QRegularExpression createRe(R"RX(\bnew(?:Table|Matrix|Note|Graph|Plot3D)\s*\(\s*["'])RX" + name +
                                          R"RX(["']|\bcloseWindow\s*\(\s*(?:\w+\s*\(\s*["'])RX" + name +
                                          R"RX(["']\s*\)|)RX" + var + R"RX()\s*\))RX");
        const QRegularExpressionMatch create = createRe.match(code, read.capturedEnd());
        if (!create.hasMatch())
            continue;
        const bool deleted = create.captured(0).startsWith("closeWindow");
        // assigned again after the read (t = newTable(...) in an "if t is None:" guard): read again, fine
        const QRegularExpression reassignRe("(?<![.\\w])" + var + R"(\s*=(?!=))");
        if (reassignRe.match(code, read.capturedEnd()).hasMatch())
            continue;
        const QRegularExpression printedRe(R"RX(\b(?:scriptPrint|print)\s*\([^\n]*(?<![.\w]))RX" + var +
                                           R"RX((?!\w))RX");
        if (!printedRe.match(code, create.capturedEnd()).hasMatch())
            continue;
        if (deleted)
            return read.captured(1) + " is read before " + create.captured(0) + " deletes \"" + read.captured(2) +
                   "\", and then printed — so it reports the state BEFORE the deletion (a variable keeps its old "
                   "object). Read every printed value after the work: call the existence check (e.g. existTable(\"" +
                   read.captured(2) + "\")) after closeWindow(), in the result line.";
        return read.captured(1) + " is read before " + create.captured(0) + "...) creates \"" + read.captured(2) +
               "\", and then printed — so it reports the state BEFORE the creation. Read every printed value after the "
               "work: call the existence check (e.g. existPlot3D(\"" +
               read.captured(2) + "\")) after the creating call, in the result line.";
    }
    return {};
}

QString QtiSasAiContent::unusedParameterMistake(const QString &code) const
{
    // Confirmed live twice (local, series 6 and 7): a formula copied without one of its terms — "y = A / (1 +
    // arg*arg);" for y = A/(1 + (x/w)^2) + c — compiled and fitted to wrong values with no error. A parameter the
    // script declares with setParamNamesAll() that its setCode() body never uses is that slip, or a dead parameter.
    static const QRegularExpression namesRe(R"RX(setParamNamesAll\s*\(\s*\[([^\]]*)\])RX");
    static const QRegularExpression quotedRe(R"RX(["']([A-Za-z_]\w*)["'])RX");
    const QRegularExpressionMatch names = namesRe.match(code);
    const QStringList bodies = compiledCodeBodies(code);
    if (!names.hasMatch() || bodies.size() != 1)
        return {};
    QStringList unused;
    for (auto it = quotedRe.globalMatch(names.captured(1)); it.hasNext();)
    {
        const QString name = it.next().captured(1);
        if (!QRegularExpression("\\b" + QRegularExpression::escape(name) + "\\b").match(bodies.first()).hasMatch())
            unused << name;
    }
    if (unused.isEmpty())
        return {};
    return "The parameter(s) " + unused.join(", ") +
           " are declared with setParamNamesAll() but never used in the setCode() body — the compiled function does "
           "not depend on them, so their fit values mean nothing. Write the formula exactly as the request gives it, "
           "every term included (e.g. \"... + " +
           unused.first() + "\").";
}

QString QtiSasAiContent::resultsAfterConfigureMistake(const QString &code)
{
    // Confirmed live twice (local, LIST 5 item 2: series 4 NOSPLIT, and series 5 NOSPLIT with fittable-python-api
    // saying it): both models fitted, then for the winner f.configure("ai/Lor5", ...) AGAIN and f.results() — the
    // function's start values, printed as "fitted" (1.0000 1.0000, then 0.0 0.0). A results() read whose nearest
    // preceding configure()/fit()/simulate() call is configure() reads no fit.
    static const QRegularExpression callRe(R"(\.\s*(configure|fit|simulate|results)\s*\()");
    const QString stripped = codeWithLiteralsBlanked(code);
    QString last;
    bool fitted = false;
    for (auto it = callRe.globalMatch(stripped); it.hasNext();)
    {
        const QString call = it.next().captured(1);
        if (call != "results")
        {
            fitted = fitted || call != "configure";
            last = call;
            continue;
        }
        if (last == "configure" && fitted)
            return "This script reads f.results() right after configure() — configure() resets every parameter to "
                   "the function's initial values, so results() returns the START values, not the fit's. Read each "
                   "model's results() right after its own fit(): chi_a = f.fit(); A_a, _ = f.results()[\"A\"]; then "
                   "configure and fit the next model, and compare the values you kept.";
    }
    return {};
}

QString QtiSasAiContent::requestDataNumberMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 3 item 8, 2026-10-11, NOSPLIT and in split plans): "y = 3*exp(-0.5*x) + 0.2" written as
    // "+ 0.02" — the fit then found c = 0.02. Every decimal number of a data formula the request gives ("y = ...", up
    // to its comma) must appear in a script that creates the data. Decimals only: an integer like 2 is everywhere.
    static const QRegularExpression newDataRe(R"(\bnew(?:Table|Matrix)\s*\()");
    if (!newDataRe.match(code).hasMatch())
        return {};
    static const QRegularExpression formulaRe(R"(\by\s*=\s*([^,;\n]+))");
    static const QRegularExpression decimalRe(R"((?<![\d.])(\d*\.\d+)(?![\d.]))");
    for (auto it = formulaRe.globalMatch(originalRequestText); it.hasNext();)
    {
        const QString formula = it.next().captured(1);
        if (!formula.contains(QLatin1Char('x')))
            continue;
        for (auto d = decimalRe.globalMatch(formula); d.hasNext();)
        {
            const QString number = d.next().captured(1);
            const QRegularExpression inCodeRe("(?<![\\d.])" + QRegularExpression::escape(number) + R"((?![\d]))");
            if (inCodeRe.match(code).hasMatch())
                continue;
            return "The request's data formula \"y = " + formula.trimmed() + "\" contains " + number +
                   ", but this script never uses " + number + " — copy the formula's numbers exactly.";
        }
    }
    return {};
}

QString QtiSasAiContent::scriptPrintKeywordMistake(const QString &code)
{
    // Confirmed live (LIST 5, item 5): scriptPrint("R5:", n.tabs(), ok, end=" ") then scriptPrint(s) — TypeError, and
    // the two-piece line could never have been ONE result line anyway.
    // sep= is supported (qtisasrc.py: scriptPrint(*args, sep=" ")), end= is not: one call is always one whole line.
    static const QRegularExpression keywordRe(R"(\bscriptPrint\s*\([^()\n]*(?:\([^()\n]*\)[^()\n]*)*\bend\s*=)");
    if (!keywordRe.match(code).hasMatch())
        return {};
    return "scriptPrint() takes no end= argument (TypeError): one call always prints one whole line. Compute every "
           "value first, then print the whole line in ONE call: scriptPrint(\"label\", value1, value2, ...).";
}

QString QtiSasAiContent::noteTabSelfRunRepair(const QString &code) const
{
    // Confirmed live (LIST 5 local pyi, item 5, runs 1 and 2 of iteration 2): the "make" tab's script text ended with
    // "n.currentEditor().execute()" — inside a tab's own script there is no "n", and a tab never runs its note; the
    // main script already ran it with n.tab("make").execute(). Blocked as "currentEditor() after addTab()", the model
    // dropped the whole note and printed made-up values twice. The line is removed from the tab's text.
    static const QRegularExpression setTextRe(
        R"RX(\.\s*setText\s*\(\s*(?:"""([\s\S]*?)"""|'''([\s\S]*?)'''|"((?:[^"\\\n]|\\.)*)"|'((?:[^'\\\n]|\\.)*)'))RX");
    // a run of a note's editor, as one line of the tab's text — written as "\n..." inside a one-line literal, or as a
    // real line of a triple-quoted one
    static const QRegularExpression selfRunRe(R"((?:\\n|\n)[ \t]*[A-Za-z_]\w*\s*\.\s*(?:currentEditor\s*\)"
                                              R"((\s*\)|tab\s*\([^()\n]*\))\s*\.\s*execute\s*\(\s*\)[ \t]*)");
    QList<QRegularExpressionMatch> texts;
    for (auto it = setTextRe.globalMatch(code); it.hasNext();)
        texts << it.next();
    QString repaired = code;
    bool anyRepair = false;
    for (int i = static_cast<int>(texts.size()) - 1; i >= 0; --i)
    {
        int group = 0;
        for (int g = 1; g <= 4 && group == 0; ++g)
            if (!texts[i].captured(g).isEmpty())
                group = g;
        if (group == 0)
            continue;
        QString text = texts[i].captured(group);
        const QString before = text;
        text.remove(selfRunRe);
        if (text == before)
            continue;
        repaired.replace(static_cast<int>(texts[i].capturedStart(group)),
                         static_cast<int>(texts[i].capturedLength(group)), text);
        anyRepair = true;
    }
    return anyRepair ? repaired : QString();
}

QString QtiSasAiContent::currentEditorAfterAddTabMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 5, item 5): "Run only the make tab" — the script renamed tab 0 to "make", called addTab()
    // (which activates the new tab), then n.currentEditor().execute(): it ran the empty-table "check" tab instead.
    // Real calls only: text inside a string — a tab's script — is not this script's code (see noteTabSelfRunRepair()).
    // Only when the request names a tab: "add a script tab … run that tab once" means the tab just added, which
    // currentEditor() is. Confirmed live (benchmark LIST 1 item 4, 2026-10-15): blocked in every lane, BlaBlaDor then
    // ran n.tab(1) — the empty first tab — and printed recalculation still on. A dry run over every logged script: the
    // narrower rule keeps all 70 LIST 5 item 5 blocks and drops exactly the three item 4 ones.
    static const QRegularExpression namedTabRe(R"RX("[^"]+"\s+tab\b|\btab\s+"[^"]+")RX",
                                               QRegularExpression::CaseInsensitiveOption);
    if (!namedTabRe.match(originalRequestText).hasMatch())
        return {};
    static const QRegularExpression addTabRe(R"(\baddTab\s*\()");
    static const QRegularExpression runCurrentRe(R"(\bcurrentEditor\s*\(\s*\)\s*\.\s*execute\s*\()");
    const QString stripped = codeWithoutStringLiterals(code);
    const QRegularExpressionMatch add = addTabRe.match(stripped);
    if (!add.hasMatch() || !runCurrentRe.match(stripped, add.capturedEnd()).hasMatch())
        return {};
    return "This script calls addTab() and later currentEditor().execute(). addTab() makes the NEW tab the current "
           "one, so this runs the tab added last, not the one the request names. Run the tab the request names by "
           "its name or its 1-based index (the first tab is 1): n.tab(\"make\").execute().";
}

QString QtiSasAiContent::noteTabIndexRepair(const QString &code) const
{
    // Confirmed live (LIST 5, item 5): n.tab(0).execute() — tab() is 1-based, so tab(0) is None. The runtime hint named
    // the None but not the index, and the same line came back twice until the item was given up.
    static const QRegularExpression tabZeroRe(R"(\.\s*tab\s*\(\s*(0)\s*\))");
    QString repaired = code;
    // Back to front, so a replacement cannot shift the offsets of the ones not yet applied.
    QList<qsizetype> zeros;
    for (auto it = tabZeroRe.globalMatch(code); it.hasNext();)
        zeros.prepend(it.next().capturedStart(1));
    for (const qsizetype at : zeros)
        repaired.replace(static_cast<int>(at), 1, QStringLiteral("1")); // int: Qt5's operator[] is ambiguous for it
    return zeros.isEmpty() ? QString() : repaired;
}

QString QtiSasAiContent::nonFinalResultLineRepair(const QString &code, const QString &requestText,
                                                  bool alsoComputed) const
{
    // Confirmed live (LIST 5, item 3): blocking a result line in an earlier part did not work either — the local model
    // sent the same line back twice and the item was given up. A fixed-text line there says nothing, so it goes.
    const QString label = requestedResultLabel(requestText);
    if (label.isEmpty())
        return {};
    const QRegularExpression lineRe(alsoComputed
                                        ? R"(^([ \t]*)(?:scriptPrint|print)\s*\(\s*f?(["']))" +
                                              QRegularExpression::escape(label) + R"([^\n]*\)[ \t]*$)"
                                        : R"(^([ \t]*)(?:scriptPrint|print)\s*\(\s*(["']))" +
                                              QRegularExpression::escape(label) + R"(\s*[^"'{}\n]*\2\s*\)[ \t]*$)",
                                    QRegularExpression::MultilineOption);
    QString repaired = code;
    repaired.replace(lineRe, "\\1pass");
    return repaired == code ? QString() : repaired;
}

// The values a one-line result print carries after its label, as Python expressions — scriptPrint(f"R1: {a:.4f} {b}",
// c) gives format(a, ".4f"), b, c — or false when the line is no such print: fixed text after the label ("R1: 5"), an
// unbalanced bracket, a nested field.
// The fields of an f-string's text, one value each ("{a:.4f}" -> format(a, ".4f")); the text without them in *rest.
// False for a field that cannot be read off (empty, or with a "!" conversion).
static bool fStringFields(QString text, QStringList *values, QString *rest)
{
    static const QRegularExpression fieldRe(R"(\{([^{}]*)\})");
    for (auto it = fieldRe.globalMatch(text); it.hasNext();)
    {
        QString field = it.next().captured(1).trimmed();
        // a format spec after the last ':' outside quotes and brackets
        int depth = 0;
        QChar quote;
        qsizetype specAt = -1;
        for (qsizetype i = 0; i < field.size(); ++i)
        {
            const QChar ch = field.at(i);
            if (!quote.isNull())
                quote = ch == quote ? QChar() : quote;
            else if (ch == QLatin1Char('"') || ch == QLatin1Char('\''))
                quote = ch;
            else if (ch == QLatin1Char('(') || ch == QLatin1Char('[') || ch == QLatin1Char('{'))
                ++depth;
            else if (ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}'))
                --depth;
            else if (ch == QLatin1Char(':') && depth == 0)
                specAt = i;
        }
        if (field.isEmpty() || field.contains(QLatin1Char('!')))
            return false;
        *values << (specAt < 0 ? field
                               : "format(" + field.left(specAt).trimmed() + ", \"" + field.mid(specAt + 1) + "\")");
    }
    *rest = text.replace(fieldRe, QStringLiteral(" "));
    return true;
}

static bool resultLineValues(const QString &line, const QString &label, QStringList *values)
{
    static const QRegularExpression literalRe(R"RX(\(\s*(f?)("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'))RX");
    const QRegularExpressionMatch literal = literalRe.match(line);
    if (!literal.hasMatch())
        return false;
    QString text = literal.captured(2).mid(1, literal.captured(2).size() - 2);
    const qsizetype labelAt = text.indexOf(label);
    if (labelAt < 0)
        return false;
    text = text.mid(labelAt + label.size());
    if (literal.captured(1) == QLatin1String("f") && !fStringFields(text, values, &text))
        return false;
    if (!text.trimmed().isEmpty())
        return false;
    // the arguments after the literal, split at the commas outside brackets and quotes — only arguments: a literal
    // continued by an operator ("R5: " + str(n)) is a line whose values cannot be read off
    const QString rest = line.mid(literal.capturedEnd(0));
    const QString restStart = rest.trimmed();
    if (!restStart.startsWith(QLatin1Char(',')) && !restStart.startsWith(QLatin1Char(')')))
        return false;
    int depth = 0;
    QChar quote;
    QString argument;
    bool closed = false;
    for (const QChar ch : rest)
    {
        if (!quote.isNull())
        {
            quote = ch == quote ? QChar() : quote;
            argument += ch;
            continue;
        }
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\''))
            quote = ch;
        else if (ch == QLatin1Char('(') || ch == QLatin1Char('[') || ch == QLatin1Char('{'))
            ++depth;
        else if (ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}'))
        {
            if (depth == 0)
            {
                closed = true;
                break;
            }
            --depth;
        }
        else if (ch == QLatin1Char(',') && depth == 0)
        {
            if (!argument.trimmed().isEmpty())
                *values << argument.trimmed();
            argument.clear();
            continue;
        }
        argument += ch;
    }
    if (!closed)
        return false;
    if (!argument.trimmed().isEmpty())
        *values << argument.trimmed();
    // an f-string argument of fields only prints each field as a value: f"{a:.4f} {b:.4f}" is two — confirmed live
    // (benchmark SVD item 5, local pyi): counted as one, the right line was blocked until the item was given up
    static const QRegularExpression fArgumentRe(R"RX(^f("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')$)RX");
    QStringList expanded;
    for (const QString &value : *values)
    {
        const QRegularExpressionMatch f = fArgumentRe.match(value);
        QStringList fields;
        QString rest;
        if (f.hasMatch() && fStringFields(f.captured(1).mid(1, f.captured(1).size() - 2), &fields, &rest) &&
            rest.trimmed().isEmpty() && !fields.isEmpty())
            expanded << fields;
        else
            expanded << value;
    }
    *values = expanded;
    // a keyword argument (sep=" ") is no value
    static const QRegularExpression keywordRe(R"(^[A-Za-z_]\w*\s*=(?!=))");
    values->erase(
        std::remove_if(values->begin(), values->end(), [](const QString &v) { return keywordRe.match(v).hasMatch(); }),
        values->end());
    return true;
}

QString QtiSasAiContent::repeatedResultLabelRepair(const QString &code, const QString &requestText) const
{
    // Confirmed live (LIST 5 NOSPLIT, item 5): an early scriptPrint("R5:", n.tabs(), exists_auto) and the full result
    // line at the end — blocked, the same script came back unchanged, the item was given up and item 6 failed with it.
    // Two shapes are repaired, anything else is left to repeatedResultLabelMistake():
    //   - the last line already carries every value of the earlier ones: they become `pass`;
    //   - the lines carry different values, the answer split over several lines — confirmed live twice (local pyi
    //     re-runs): "R1: a b c" then "R1: count", and "R6: {functions}" then "R6: peak pipeline"; keeping the last
    //     one alone printed "R1: 5" and "R6: missing missing". They become one line at the last one's place, the
    //     values in their order.
    // Values overlapping in part (n.tabs() in both lines above, with different others) are a guess, as is a print over
    // several lines or with fixed text after the label.
    const QString label = requestedResultLabel(requestText);
    if (label.isEmpty())
        return {};
    const QRegularExpression topLevelRe(R"(^(?:scriptPrint|print)\s*\(\s*f?["'])" + QRegularExpression::escape(label) +
                                            R"([^\n]*$)",
                                        QRegularExpression::MultilineOption);
    QList<QRegularExpressionMatch> lines;
    for (auto it = topLevelRe.globalMatch(code); it.hasNext();)
        lines << it.next();
    if (lines.size() < 2)
        return {};
    QList<QStringList> lineValues;
    for (const QRegularExpressionMatch &line : lines)
    {
        QStringList values;
        if (!resultLineValues(line.captured(0), label, &values) || values.isEmpty())
            return {};
        QStringList simplified;
        for (const QString &value : values)
            simplified << value.simplified();
        lineValues << simplified;
    }
    const QStringList &last = lineValues.last();
    bool lastHasAll = true;
    for (int i = 0; i + 1 < static_cast<int>(lineValues.size()) && lastHasAll; ++i)
        for (const QString &value : lineValues[i])
            lastHasAll = lastHasAll && last.contains(value);
    QStringList merged;
    if (!lastHasAll)
        for (const QStringList &values : lineValues)
            for (const QString &value : values)
            {
                if (merged.contains(value))
                    return {};
                merged << value;
            }
    // When the request says how many values it asks for, that decides: confirmed live (local pyi, item 6), an early
    // draft line and the real one carried different values and merged into four for a three-value item. The last line
    // complete: the others go; the merged line exactly complete: it stands; neither: left to the check.
    const int asked = static_cast<int>(requestedResultValues(requestText).size());
    // requestedResultValues() is a lower bound: a last line with at least that many values is complete.
    if (asked > 0)
    {
        if (static_cast<int>(last.size()) >= asked)
            lastHasAll = true;
        else if (lastHasAll || static_cast<int>(merged.size()) != asked)
            return {};
    }
    QString repaired = code;
    const QRegularExpressionMatch &lastLine = lines.last();
    if (!lastHasAll)
        repaired.replace(static_cast<int>(lastLine.capturedStart(0)), static_cast<int>(lastLine.capturedLength(0)),
                         "scriptPrint(\"" + label + "\", " + merged.join(", ") + ")");
    // Back to front, so a replacement cannot shift the offsets of the ones not yet applied.
    for (int i = static_cast<int>(lines.size()) - 2; i >= 0; --i)
        repaired.replace(static_cast<int>(lines[i].capturedStart(0)), static_cast<int>(lines[i].capturedLength(0)),
                         QStringLiteral("pass"));
    return repaired;
}

// The result-line print with the most readable values, at any indentation, and its values — a guarded line counts too
// (confirmed live, LIST 5 local pyi, item 3: "if ...: ... else: scriptPrint(f\"R3: {r:.4f}\")" was not seen by a
// top-level-only search). A fallback branch's fixed text cannot be read off and drops out. False when none can.
static bool fullestResultLine(const QString &code, const QString &label, QString *line, QStringList *values)
{
    const QRegularExpression printRe(R"(^[ \t]*(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                         QRegularExpression::escape(label) + R"([^\n]*$)",
                                     QRegularExpression::MultilineOption);
    bool found = false;
    for (auto it = printRe.globalMatch(code); it.hasNext();)
    {
        const QString candidate = it.next().captured(0).trimmed();
        QStringList candidateValues;
        if (!resultLineValues(candidate, label, &candidateValues) || candidateValues.isEmpty())
            continue;
        if (!found || candidateValues.size() > values->size())
        {
            *line = candidate;
            *values = candidateValues;
            found = true;
        }
    }
    return found;
}

QString QtiSasAiContent::resultLineTooFewValuesMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live, the most frequent local-pyi failure (LIST 5: R2 "2.0000 0.5000" without the function name, R3
    // "0.0000" without the layer and curve counts, R5 "15.0" without the tab count and the existence): the result line
    // carries fewer values than the request lists, the others printed on lines of their own or not at all. Fewer only —
    // requestedResultValues() is a lower bound — and only for a line whose values can be read off.
    const QString label = requestedResultLabel(originalRequestText);
    const QStringList asked = requestedResultValues(originalRequestText);
    if (label.isEmpty() || asked.isEmpty())
        return {};
    QString lastLine;
    QStringList values;
    if (!fullestResultLine(code, label, &lastLine, &values) || values.size() >= asked.size())
        return {};
    return QString("The result line %1 prints %2 value(s), but the request asks for %3, in this order: %4. Compute "
                   "every one of them and print them all on that ONE line: scriptPrint(\"%5\", value1, value2, ...) — "
                   "a value printed on another line does not count.")
        .arg(lastLine.trimmed())
        .arg(values.size())
        .arg(asked.size())
        .arg(asked.join(" | "))
        .arg(label);
}

QString QtiSasAiContent::rowRangeEndsOneShortMistake(const QString &code)
{
    // Confirmed live (LIST 5 local pyi, item 1): "for i in range(1, 21): if t.cell(2, i) > 5" on a 21-row table counted
    // 4 rows instead of 5 — rows are 1-based and range() excludes its end, so the last row was never read. Flagged when
    // the end is the row count itself — t.numRows() (m.numRows() too), or the literal row count of a newTable() /
    // newMatrix() of this script — and the loop body reads or writes cells by that index.
    const QString stripped = codeWithoutStringLiterals(code);
    static const QRegularExpression forRe(R"(^([ \t]*)for\s+([A-Za-z_]\w*)\s+in\s+range\s*\(\s*1\s*,\s*)"
                                          R"(([A-Za-z_]\w*\s*\.\s*numRows\s*\(\s*\)|\d+)\s*\)\s*:)",
                                          QRegularExpression::MultilineOption);
    // the literal row counts survive codeWithoutStringLiterals(): only the name argument is a string
    static const QRegularExpression newRe(R"(\bnew(?:Table|Matrix)\s*\([^,()]*,\s*(\d+))");
    QSet<QString> rowCounts;
    for (auto it = newRe.globalMatch(stripped); it.hasNext();)
        rowCounts.insert(it.next().captured(1));
    for (auto it = forRe.globalMatch(stripped); it.hasNext();)
    {
        const QRegularExpressionMatch loop = it.next();
        const QString end = loop.captured(3).simplified();
        if (!end.contains(QLatin1String("numRows")) && !rowCounts.contains(end))
            continue;
        // the loop body: the following non-empty lines indented deeper than the for line
        const qsizetype indent = loop.captured(1).size();
        QString body;
        const QStringList after = stripped.mid(loop.capturedEnd(0)).split('\n');
        for (int i = 1; i < after.size(); ++i)
        {
            const QString &line = after.at(i);
            if (line.trimmed().isEmpty())
                continue;
            qsizetype lead = 0;
            while (lead < line.size() && line.at(lead).isSpace())
                ++lead;
            if (lead <= indent)
                break;
            body += line + "\n";
        }
        const QRegularExpression cellRe(R"(\.\s*(?:cell|setCell|text|setText|cellData|setCellData)\s*\([^()\n]*\b)" +
                                        QRegularExpression::escape(loop.captured(2)) + R"(\b)");
        if (cellRe.match(body).hasMatch())
            return "This script loops for " + loop.captured(2) + " in range(1, " + end +
                   ") over cells, but rows are 1-based and range() excludes its end: the last row is never visited. "
                   "Use range(1, " +
                   end + " + 1).";
    }
    return {};
}

QString QtiSasAiContent::tableFormulaCaretRepair(const QString &code, const QString &formulaLanguage) const
{
    // Confirmed live (LIST 5 NOSPLIT, item 1): t.setCommand(2, "3*col(\"x\")^2 - 2*col(\"x\") + 1") — a column formula
    // is evaluated in the global scripting language, here Python, where ^ is XOR: "unsupported operand type(s) for ^"
    // and a retry. Only for Python, the language the host reports: in muParser — the global language there, or asked
    // for with recalculate(..., forceMuParser=True) — ^ is the power.
    // Confirmed live (series 5, Codex, LIST 3 item 2): "col(1)^3" recalculated with recalculate(3, 1, -1, True) —
    // muParser forced by the positional argument, which the old keyword-only test missed — became "col(1)**3", failed
    // in muParser three times and the item was given up. A column forced to muParser keeps its ^.
    if (formulaLanguage.compare(QLatin1String("Python"), Qt::CaseInsensitive) != 0)
        return {};
    static const QRegularExpression forcedRe(
        R"RX(\.\s*recalculate\s*\(\s*([^,()]+?)\s*,(?:[^,()]*,){2}\s*True\b)RX"
        R"RX(|\.\s*recalculate\s*\(\s*([^,()]+?)\s*,[^()]*forceMuParser\s*=\s*True)RX");
    QStringList forced;
    for (auto it = forcedRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        forced << (m.captured(1).isEmpty() ? m.captured(2) : m.captured(1)).trimmed();
    }
    static const QRegularExpression setCommandRe(
        R"RX(\bsetCommand\s*\(([^,()\n]+),\s*(?:"((?:[^"\\\n]|\\.)*)"|'((?:[^'\\\n]|\\.)*)'))RX");
    QList<QRegularExpressionMatch> formulas;
    for (auto it = setCommandRe.globalMatch(code); it.hasNext();)
        formulas << it.next();
    QString repaired = code;
    bool anyRepair = false;
    for (int i = static_cast<int>(formulas.size()) - 1; i >= 0; --i)
    {
        if (forced.contains(formulas[i].captured(1).trimmed()))
            continue;
        const int group = formulas[i].captured(2).isEmpty() ? 3 : 2;
        QString formula = formulas[i].captured(group);
        if (!formula.contains(QLatin1Char('^')))
            continue;
        formula.replace(QLatin1String("^"), QLatin1String("**"));
        repaired.replace(static_cast<int>(formulas[i].capturedStart(group)),
                         static_cast<int>(formulas[i].capturedLength(group)), formula);
        anyRepair = true;
    }
    return anyRepair ? repaired : QString();
}

QString QtiSasAiContent::compilerMissingGroupRepair(const QString &code) const
{
    // new() clears the editor, group included, and save()/compile() refuse an empty group. Each new() ...
    // compile()/save() stretch that sets no group gets one after its setFunctionName(): the last setGroup(...) written
    // before that new() — the script's own choice, lost by the order — or, with none and an "ai/" name, "ai" (RULE 12's
    // namespace). Both confirmed live (LIST 5 local pyi, items 1-2): no setGroup() at all, "group is empty" four times
    // in 22 minutes; and c.setGroup(match ...) moved above c.new() in all three runs of a series, the item given up
    // each time.
    static const QRegularExpression newRe(R"(\b([A-Za-z_]\w*)\s*\.\s*new\s*\(\s*\))");
    static const QRegularExpression groupRe(
        R"(\b[A-Za-z_]\w*\s*\.\s*setGroup\s*\(([^()\n]*(?:\([^()\n]*\)[^()\n]*)*)\))");
    static const QRegularExpression buildRe(R"(\.\s*(?:compile|save)\s*\()");
    static const QRegularExpression nameRe(
        R"RX(^([ \t]*)([A-Za-z_]\w*)\s*\.\s*setFunctionName\s*\(\s*["']([^"'\n]+)["']\s*\)[^\n]*$)RX",
        QRegularExpression::MultilineOption);
    QList<QRegularExpressionMatch> news;
    for (auto it = newRe.globalMatch(code); it.hasNext();)
        news << it.next();
    QList<QPair<int, QString>> inserts; // position, text
    for (int i = 0; i < static_cast<int>(news.size()); ++i)
    {
        const int start = static_cast<int>(news[i].capturedEnd(0));
        const int nextNew =
            i + 1 < news.size() ? static_cast<int>(news[i + 1].capturedStart(0)) : static_cast<int>(code.size());
        const QRegularExpressionMatch build = buildRe.match(code, start);
        if (!build.hasMatch() || build.capturedStart(0) > nextNew)
            continue;
        const int end = static_cast<int>(build.capturedStart(0));
        if (groupRe.match(code.mid(start, end - start)).hasMatch())
            continue;
        const QRegularExpressionMatch name = nameRe.match(code, start);
        if (!name.hasMatch() || name.capturedStart(0) > end)
            continue;
        QString group;
        for (auto it = groupRe.globalMatch(code.left(static_cast<int>(news[i].capturedStart(0)))); it.hasNext();)
            group = it.next().captured(1).trimmed();
        if (group.isEmpty() && name.captured(3).startsWith(QLatin1String("ai/")))
            group = QStringLiteral("\"ai\"");
        if (group.isEmpty())
            continue;
        inserts << qMakePair(static_cast<int>(name.capturedEnd(0)),
                             "\n" + name.captured(1) + name.captured(2) + ".setGroup(" + group + ")");
    }
    if (inserts.isEmpty())
        return {};
    QString repaired = code;
    for (int i = static_cast<int>(inserts.size()) - 1; i >= 0; --i)
        repaired.insert(inserts[i].first, inserts[i].second);
    return repaired;
}

// A value of a result line that is fixed text: a plain string literal, or an f-string without a field. `text` gets
// what it says.
static bool fixedTextValue(const QString &value, QString *text)
{
    static const QRegularExpression literalRe(R"RX(^(f?)("((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)')$)RX");
    const QRegularExpressionMatch m = literalRe.match(value.trimmed());
    if (!m.hasMatch())
        return false;
    *text = m.captured(3).isNull() ? m.captured(4) : m.captured(3);
    return m.captured(1).isEmpty() || !text->contains(QLatin1Char('{'));
}

QString QtiSasAiContent::docTopicForKind(const QString &kind) const
{
    // liveObjects()'s kinds (ApplicationWindow::liveObjects()) -> the python/doc file topics
    // (and entityKinds()'s module kinds of a table a module made)
    static const QHash<QString, QString> topics{{"table", "table"},
                                                {"matrix", "matrix"},
                                                {"note", "note"},
                                                {"graph", "graph"},
                                                {"3D plot", "graph3d"},
                                                {"function", "compile"},
                                                {"JNSE header table", "jnse"},
                                                {"JNSE dataset table", "jnse"},
                                                {"SVD table", "svd"}};
    return topics.value(kind);
}

// The argument spans of the call whose "(" is at `open` in `masked` (code with its string literals blanked, same
// length): top-level commas split, brackets and parentheses nest. False when the call is not closed.
static bool callArgumentSpans(const QString &masked, qsizetype open, QList<QPair<qsizetype, qsizetype>> *args,
                              qsizetype *close)
{
    int depth = 0;
    qsizetype start = open + 1;
    for (qsizetype i = open; i < masked.size(); ++i)
    {
        const QChar ch = masked.at(i);
        if (ch == QLatin1Char('(') || ch == QLatin1Char('[') || ch == QLatin1Char('{'))
            ++depth;
        else if (ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}'))
        {
            if (--depth == 0)
            {
                // a blanked literal is spaces in `masked`: an argument after a comma counts whatever it holds
                if (!args->isEmpty() || !masked.mid(start, i - start).trimmed().isEmpty())
                    *args << qMakePair(start, i);
                *close = i;
                return true;
            }
        }
        else if (ch == QLatin1Char(',') && depth == 1)
        {
            *args << qMakePair(start, i);
            start = i + 1;
        }
    }
    return false;
}

QString QtiSasAiContent::apiCallRepairs(const QString &code, const QString &requestText, bool resultLine,
                                        QStringList *notes) const
{
    // Calls whose fix has one form only, each confirmed live more than once (series 4–6). Applied back to front within
    // each rule, on the code with its string literals blanked, so a tab's script text is never touched.
    QString repaired = code;
    auto note = [notes](const QString &text) {
        if (notes)
            *notes << text;
    };

    // 1. A plain functionExists(...) — a method of Compiler/Fittable (6/6 local LIST 3 runs, item 12: NameError).
    {
        const QString masked = codeWithLiteralsBlanked(repaired);
        static const QRegularExpression bareRe(R"((?<![.\w])functionExists\s*\()");
        static const QRegularExpression definedRe(R"(\bdef\s+functionExists\b|(?<![.\w])functionExists\s*=(?!=))");
        if (!definedRe.match(masked).hasMatch())
        {
            QList<qsizetype> at;
            for (auto it = bareRe.globalMatch(masked); it.hasNext();)
                at.prepend(it.next().capturedStart());
            for (const qsizetype pos : at)
                repaired.insert(pos, QStringLiteral("Compiler."));
            if (!at.isEmpty())
                note("functionExists(...) became Compiler.functionExists(...) — it is a method, not a global.");
        }
    }

    // 2. setCellData(col, <list>) with two arguments — setCellData() writes ONE cell; a column from a list is
    //    setColData() (3 local runs: TypeError "argument 2 has unexpected type 'list'").
    {
        const QString masked = codeWithLiteralsBlanked(repaired);
        static const QRegularExpression callRe(R"(\.\s*(setCellData)\s*\()");
        QList<QPair<qsizetype, qsizetype>> names; // name spans to rename
        for (auto it = callRe.globalMatch(masked); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            QList<QPair<qsizetype, qsizetype>> args;
            qsizetype close = 0;
            if (!callArgumentSpans(masked, m.capturedEnd() - 1, &args, &close) || args.size() != 2)
                continue;
            const QString second = repaired.mid(args[1].first, args[1].second - args[1].first).trimmed();
            if (second.startsWith(QLatin1Char('[')) || second.startsWith(QLatin1String("list(")))
                names.prepend(qMakePair(m.capturedStart(1), m.capturedEnd(1)));
        }
        for (const auto &span : names)
            repaired.replace(static_cast<int>(span.first), static_cast<int>(span.second - span.first),
                             QStringLiteral("setColData"));
        if (!names.isEmpty())
            note("setCellData(col, <list>) became setColData(col, <list>) — setCellData() writes one cell.");
    }

    // 3. n.setTabText(i, code) / n.setTabScript(x, code) — Note has neither (16 AttributeErrors in the logs); a tab's
    //    script is its editor's text: n.editorAt(i).setText(code) (0-based, like the Qt index the model meant) /
    //    n.tab(x).setText(code).
    {
        const QString masked = codeWithLiteralsBlanked(repaired);
        static const QRegularExpression callRe(R"(\.\s*(setTabText|setTabScript)\s*\()");
        struct Edit
        {
            qsizetype from, to;
            QString text;
        };
        QList<Edit> edits;
        for (auto it = callRe.globalMatch(masked); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            QList<QPair<qsizetype, qsizetype>> args;
            qsizetype close = 0;
            if (!callArgumentSpans(masked, m.capturedEnd() - 1, &args, &close) || args.size() != 2)
                continue;
            const QString index = repaired.mid(args[0].first, args[0].second - args[0].first).trimmed();
            const QString text = repaired.mid(args[1].first, args[1].second - args[1].first).trimmed();
            const QString editor = m.captured(1) == QLatin1String("setTabText") ? "editorAt(" : "tab(";
            edits.prepend({m.capturedStart(1), close + 1, editor + index + ").setText(" + text + ")"});
        }
        for (const Edit &e : edits)
            repaired.replace(static_cast<int>(e.from), static_cast<int>(e.to - e.from), e.text);
        if (!edits.isEmpty())
            note("n.setTabText()/setTabScript() became n.editorAt(i)/tab(x).setText(...) — a tab's script is its "
                 "editor's text; Note has no setTabText().");
    }

    // 4. One addTab() too many after newNote() (3 local runs, LIST 4 item 1: blocked, sent back unchanged until the
    //    item was given up): the request asks for N tabs, the script calls addTab() exactly N times, each as a line of
    //    its own — the first goes, the new note's own tab is tab 1. Not when a numbered tab(k >= 2) / editorAt(k >= 1)
    //    is used before the second addTab(): that tab would no longer exist there.
    {
        static const QRegularExpression requestTabsRe(R"(\b(two|three|four|five|2|3|4|5)\s+(?:\w+\s+)?tabs\b)",
                                                      QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression newNoteRe(R"(\bnewNote\s*\()");
        static const QRegularExpression addTabLineRe(R"(^[ \t]*[A-Za-z_]\w*\s*\.\s*addTab\s*\(\s*\)[ \t]*\n?)",
                                                     QRegularExpression::MultilineOption);
        static const QRegularExpression anyAddTabRe(R"(\.\s*addTab\s*\()");
        static const QRegularExpression numberedRe(R"(\.\s*(tab|editorAt)\s*\(\s*(\d+)\s*\))");
        const QRegularExpressionMatch requested = requestTabsRe.match(requestText);
        const QString masked = codeWithLiteralsBlanked(repaired);
        const QRegularExpressionMatch created = newNoteRe.match(masked);
        if (requested.hasMatch() && created.hasMatch())
        {
            static const QStringList words{"two", "three", "four", "five"};
            const QString n = requested.captured(1).toLower();
            const int tabs = words.contains(n) ? static_cast<int>(words.indexOf(n)) + 2 : n.toInt();
            QList<QRegularExpressionMatch> lines;
            for (auto it = addTabLineRe.globalMatch(masked, created.capturedEnd()); it.hasNext();)
                lines << it.next();
            int all = 0;
            for (auto it = anyAddTabRe.globalMatch(masked, created.capturedEnd()); it.hasNext(); it.next())
                ++all;
            bool numberedBefore = false;
            if (lines.size() == tabs && all == tabs)
            {
                const qsizetype end = lines.size() > 1 ? lines[1].capturedStart() : masked.size();
                for (auto it = numberedRe.globalMatch(masked.left(end), lines[0].capturedEnd()); it.hasNext();)
                {
                    const QRegularExpressionMatch m = it.next();
                    numberedBefore =
                        numberedBefore || (m.captured(1) == QLatin1String("tab") ? m.captured(2).toInt() >= 2
                                                                                 : m.captured(2).toInt() >= 1);
                }
                if (!numberedBefore)
                {
                    repaired.remove(static_cast<int>(lines[0].capturedStart()),
                                    static_cast<int>(lines[0].capturedLength()));
                    note(QString("the first addTab() removed — the request asks for %1 tabs and a new note already has "
                                 "one.")
                             .arg(tabs));
                }
            }
        }
    }

    // 7. AlignPolicy is GraphWindow's enum, not Graph's (6 AttributeErrors in 3 logs, series 5–7; the docs list
    //    Graph.Axis/Scale/Ticks/CurveType together, the model extends the pattern) — and setAlignPolicy() takes no int
    //    (local series 7, LIST 5 item 3: the retry wrote setAlignPolicy(1) three times). 0 = AlignLayers, 1 =
    //    AlignCanvases (qtimod.sip).
    {
        const QString masked = codeWithLiteralsBlanked(repaired);
        static const QRegularExpression wrongEnumRe(R"((?<![.\w])Graph\s*\.\s*AlignPolicy\b)");
        static const QRegularExpression intArgRe(R"(\.\s*setAlignPolicy\s*\(\s*([01])\s*\))");
        QList<QPair<QRegularExpressionMatch, QString>> edits;
        for (auto it = wrongEnumRe.globalMatch(masked); it.hasNext();)
            edits.prepend(qMakePair(it.next(), QStringLiteral("GraphWindow.AlignPolicy")));
        for (auto it = intArgRe.globalMatch(masked); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            edits.prepend(
                qMakePair(m, QString(".setAlignPolicy(GraphWindow.AlignPolicy.%1)")
                                 .arg(m.captured(1) == QLatin1String("1") ? "AlignCanvases" : "AlignLayers")));
        }
        std::sort(edits.begin(), edits.end(),
                  [](const auto &a, const auto &b) { return a.first.capturedStart() > b.first.capturedStart(); });
        for (const auto &e : edits)
            repaired.replace(static_cast<int>(e.first.capturedStart()), static_cast<int>(e.first.capturedLength()),
                             e.second);
        if (!edits.isEmpty())
            note("AlignPolicy written as GraphWindow.AlignPolicy.<value> — it is GraphWindow's enum, and "
                 "setAlignPolicy() takes no int.");
    }

    // 8. setColTextFormat(c) before writing a NUMBER as text into a column this script filled with numbers (BlaBlaDor,
    //    series 6 and 7, LIST 3 item 5: setColTextFormat(1) then setText(1, 2, "2,5") — the cell then held text and
    //    cell() read 0.0; table-python-api says a number as text needs no text format). Only when every text written
    //    into that column after it is number-like and the column got setCell() numbers before.
    {
        const QString masked = codeWithLiteralsBlanked(repaired);
        static const QRegularExpression formatRe(
            R"(^[ \t]*([A-Za-z_]\w*)\s*\.\s*setColTextFormat\s*\(\s*(\d+)\s*\)[ \t]*\n?)",
            QRegularExpression::MultilineOption);
        QList<QRegularExpressionMatch> drops;
        for (auto it = formatRe.globalMatch(masked); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            const QString var = QRegularExpression::escape(m.captured(1));
            const QString col = m.captured(2);
            const QRegularExpression setCellRe("\\b" + var + R"(\s*\.\s*setCell\s*\(\s*)" + col + R"(\s*,)");
            const QRegularExpression setTextRe("\\b" + var + R"(\s*\.\s*setText\s*\(\s*)" + col +
                                               R"RX(\s*,[^,()]*,\s*(["'])(.*?)\1)RX");
            if (!setCellRe.match(repaired.left(m.capturedStart())).hasMatch())
                continue;
            static const QRegularExpression numberRe(R"(^\s*[-+]?\d[\d.,]*\s*$)");
            bool any = false;
            bool allNumbers = true;
            for (auto t = setTextRe.globalMatch(repaired, m.capturedEnd()); t.hasNext();)
            {
                any = true;
                allNumbers = allNumbers && numberRe.match(t.next().captured(2)).hasMatch();
            }
            if (any && allNumbers)
                drops.prepend(m);
        }
        for (const QRegularExpressionMatch &m : drops)
            repaired.remove(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()));
        if (!drops.isEmpty())
            note("setColTextFormat() removed — the column holds numbers, and a number written as text (\"2,5\") is "
                 "stored as that number in a Numeric column.");
    }

    // 9. A fit table name built from a function name with its folder cut off: "fitCurve-" + name.split("/")[1] or
    //    "fitCurve-" + name.replace("ai/", ""). A fit's table is "fitCurve-" + the FULL name, folder included (the
    //    docs say so); the lookup returns None, and a guarded script skips the plot silently. Confirmed live (local
    //    LIST 1, item 12, two runs): "R12: ai/GPeak 1 0" with no fit curve plotted, and a stop on the same diagnosis.
    {
        static const QRegularExpression cutRe(
            R"RX((["']fitCurve-["']\s*\+\s*)([A-Za-z_][\w.]*)\s*(?:\.split\(\s*["']/["']\s*\)\s*\[\s*-?1\s*\])RX"
            R"RX(|\.replace\(\s*["'][\w-]+/["']\s*,\s*["']["']\s*\)))RX");
        QList<QRegularExpressionMatch> cuts;
        for (auto it = cutRe.globalMatch(repaired); it.hasNext();)
            cuts.prepend(it.next());
        for (const QRegularExpressionMatch &m : cuts)
            repaired.replace(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()),
                             m.captured(1) + m.captured(2));
        if (!cuts.isEmpty())
            note("\"fitCurve-\" + the function name with its folder cut off became \"fitCurve-\" + the full name — a "
                 "fit's table is named after the full function name, folder included.");
    }

    const QString label = resultLine ? requestedResultLabel(requestText) : QString();

    // 5. int(<existence>) in the result line — a "whether" value is the boolean itself (BlaBlaDor, 2 runs: "0 0").
    if (!label.isEmpty())
    {
        const QRegularExpression lineRe(R"(^[ \t]*(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                            QRegularExpression::escape(label) + R"([^\n]*$)",
                                        QRegularExpression::MultilineOption);
        static const QRegularExpression intRe(
            R"(\bint\s*\(\s*((?:not\s+)?(?:exist\w*\s*\([^()\n]*\)|[A-Za-z_]\w*))\s*\))");
        static const QRegularExpression existAssignRe(R"(^[ \t]*([A-Za-z_]\w*)\s*=\s*(?:not\s+)?exist\w*\s*\()",
                                                      QRegularExpression::MultilineOption);
        QStringList existVars;
        for (auto it = existAssignRe.globalMatch(repaired); it.hasNext();)
            existVars << it.next().captured(1);
        bool changed = false;
        QList<QRegularExpressionMatch> resultLines;
        for (auto it = lineRe.globalMatch(repaired); it.hasNext();)
            resultLines.prepend(it.next());
        for (const QRegularExpressionMatch &l : resultLines)
        {
            QString line = l.captured(0);
            QList<QRegularExpressionMatch> ints;
            for (auto it = intRe.globalMatch(line); it.hasNext();)
                ints.prepend(it.next());
            for (const QRegularExpressionMatch &m : ints)
            {
                const QString inner = m.captured(1);
                if (!inner.contains(QLatin1String("exist")) && !existVars.contains(inner))
                    continue;
                line.replace(static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), inner);
                changed = true;
            }
            repaired.replace(static_cast<int>(l.capturedStart()), static_cast<int>(l.capturedLength()), line);
        }
        if (changed)
            note("int(...) removed around an existence value in the result line — a \"whether\" value is printed as "
                 "True/False.");
    }

    // 6. The result line printed before the last value, that value printed unlabelled after it (local LIST 3 item 5,
    //    series 4 and 6: "R5: 1234,5 0,25 -3" then "2.5"). Only when the labelled line is unindented and the only one,
    //    the later prints are unindented, unlabelled and the script's last prints, and together they give at least as
    //    many values as the request asks for while the labelled line alone gives fewer. The labelled line's values are
    //    kept where they were computed (a later step may change what they read) and printed with the rest at the end.
    if (!label.isEmpty())
    {
        // requestedResultValues() lists phrases: "the three texts read first" counts as three values
        qsizetype asked = 0;
        static const QRegularExpression countRe(R"(^(?:the\s+|its\s+)?(two|three|four|five|six|2|3|4|5|6)\s)",
                                                QRegularExpression::CaseInsensitiveOption);
        static const QStringList countWords{"two", "three", "four", "five", "six"};
        for (const QString &phrase : requestedResultValues(requestText))
        {
            const QRegularExpressionMatch m = countRe.match(phrase.trimmed());
            const QString n = m.captured(1).toLower();
            asked += !m.hasMatch() ? 1 : (countWords.contains(n) ? countWords.indexOf(n) + 2 : n.toInt());
        }
        const QRegularExpression labelledRe(R"(^(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                                QRegularExpression::escape(label) + R"([^\n]*$)",
                                            QRegularExpression::MultilineOption);
        const QRegularExpression anyLabelRe(R"((?:scriptPrint|print)\s*\(\s*f?["']\s*)" +
                                            QRegularExpression::escape(label));
        static const QRegularExpression printRe(R"(^(scriptPrint|print)\s*\(([^\n]*)\)[ \t]*$)",
                                                QRegularExpression::MultilineOption);
        static const QRegularExpression anyPrintRe(R"(^[ \t]*(?:scriptPrint|print)\s*\()",
                                                   QRegularExpression::MultilineOption);
        QList<QRegularExpressionMatch> labelled;
        for (auto it = labelledRe.globalMatch(repaired); it.hasNext();)
            labelled << it.next();
        int labelPrints = 0;
        for (auto it = anyLabelRe.globalMatch(repaired); it.hasNext(); it.next())
            ++labelPrints;
        QStringList first;
        if (asked > 0 && labelled.size() == 1 && labelPrints == 1 &&
            resultLineValues(labelled[0].captured(0), label, &first) && first.size() < asked)
        {
            QList<QRegularExpressionMatch> later;
            bool onlyTopLevel = true;
            for (auto it = anyPrintRe.globalMatch(repaired, labelled[0].capturedEnd()); it.hasNext();)
            {
                const QRegularExpressionMatch p = it.next();
                const QRegularExpressionMatch whole = printRe.match(repaired, p.capturedStart());
                if (!whole.hasMatch() || whole.capturedStart() != p.capturedStart())
                {
                    onlyTopLevel = false;
                    break;
                }
                later << whole;
            }
            QStringList rest;
            for (const QRegularExpressionMatch &p : later)
            {
                QStringList values;
                if (!resultLineValues("scriptPrint(\"" + label + "\", " + p.captured(2) + ")", label, &values))
                {
                    onlyTopLevel = false;
                    break;
                }
                rest << values;
            }
            QString text;
            const bool fixedText =
                std::any_of(rest.cbegin(), rest.cend(), [&text](const QString &v) { return fixedTextValue(v, &text); });
            if (onlyTopLevel && !later.isEmpty() && !fixedText && first.size() + rest.size() >= asked)
            {
                QString merged = repaired;
                // the later prints first (back to front), then the labelled line, so offsets stay valid
                for (int i = static_cast<int>(later.size()) - 1; i >= 0; --i)
                    merged.replace(static_cast<int>(later[i].capturedStart()),
                                   static_cast<int>(later[i].capturedLength()),
                                   i == later.size() - 1
                                       ? "scriptPrint(\"" + label + "\", *_result_values, " + rest.join(", ") + ")"
                                       : QStringLiteral("pass"));
                merged.replace(static_cast<int>(labelled[0].capturedStart()),
                               static_cast<int>(labelled[0].capturedLength()),
                               "_result_values = (" + first.join(", ") + (first.size() == 1 ? ",)" : ")"));
                repaired = merged;
                note("the result line was printed before its last value — it now prints every value at the end, "
                     "on one line.");
            }
        }
    }
    return repaired == code ? QString() : repaired;
}

QString QtiSasAiContent::muParserPowerRepair(const QString &code) const
{
    // Confirmed live (Codex, LIST 3 item 2, 2 of 4 runs): "a formula ... written in muParser syntax with ^" became
    // setCommand(3, "col(1)**3") with recalculate(3, 1, -1, True) — muParser has no **, the formula failed four times
    // and the item was given up. The mirror of tableFormulaCaretRepair(): ** becomes ^ in the formula of a column this
    // script recalculates with muParser forced (the 4th argument True, or forceMuParser=True).
    static const QRegularExpression forcedRe(
        R"RX(\.\s*recalculate\s*\(\s*([^,()]+?)\s*,(?:[^,()]*,){2}\s*True\b)RX"
        R"RX(|\.\s*recalculate\s*\(\s*([^,()]+?)\s*,[^()]*forceMuParser\s*=\s*True)RX");
    QStringList columns;
    for (auto it = forcedRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        columns << (m.captured(1).isEmpty() ? m.captured(2) : m.captured(1)).trimmed();
    }
    if (columns.isEmpty())
        return {};
    QString repaired = code;
    for (const QString &column : std::as_const(columns))
    {
        const QRegularExpression commandRe(R"RX(\.\s*setCommand\s*\(\s*)RX" + QRegularExpression::escape(column) +
                                           R"RX(\s*,\s*(?:"([^"\n]*)"|'([^'\n]*)'))RX");
        QList<QRegularExpressionMatch> commands;
        for (auto it = commandRe.globalMatch(repaired); it.hasNext();)
            commands << it.next();
        for (int i = static_cast<int>(commands.size()) - 1; i >= 0; --i)
        {
            const int group = commands[i].captured(1).isNull() ? 2 : 1;
            QString formula = commands[i].captured(group);
            if (!formula.contains(QLatin1String("**")))
                continue;
            formula.replace(QLatin1String("**"), QLatin1String("^"));
            repaired.replace(static_cast<int>(commands[i].capturedStart(group)),
                             static_cast<int>(commands[i].capturedLength(group)), formula);
        }
    }
    return repaired == code ? QString() : repaired;
}

// The cell calls of each object `creatorName` makes in `code` with literal sizes (name, rows, columns), swapped when
// they can only have been meant transposed: not square; every call of `methods` on it has literal indices; at least one
// is out of range; every one fits once its two indices are swapped. `rowFirst`: the calls take (row, col), else (col,
// row). Shared by matrixTransposedCellRepair() and tableTransposedCellRepair().
static QString swappedCellIndices(const QString &code, const QString &creatorName, const QString &methods,
                                  bool rowFirst)
{
    const QRegularExpression newRe(R"RX(^[ \t]*([A-Za-z_]\w*)\s*=\s*)RX" + creatorName +
                                       R"RX(\s*\(\s*(?:"[^"]*"|'[^']*')\s*,\s*(\d+)\s*,\s*(\d+)\s*\))RX",
                                   QRegularExpression::MultilineOption);
    QString repaired = code;
    for (auto it = newRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch created = it.next();
        const QString var = QRegularExpression::escape(created.captured(1));
        const int rows = created.captured(2).toInt();
        const int cols = created.captured(3).toInt();
        if (rows == cols)
            continue;
        const int first = rowFirst ? rows : cols;
        const int second = rowFirst ? cols : rows;
        const QRegularExpression anyCellRe(R"((?<![.\w]))" + var + R"(\s*\.\s*(?:)" + methods + R"()\s*\()");
        const QRegularExpression literalCellRe(R"((?<![.\w]))" + var + R"(\s*\.\s*(?:)" + methods +
                                               R"()\s*\(\s*(\d+)\s*,\s*(\d+)\s*([,)]))");
        int all = 0;
        for (auto c = anyCellRe.globalMatch(repaired); c.hasNext(); c.next())
            ++all;
        QList<QRegularExpressionMatch> calls;
        for (auto c = literalCellRe.globalMatch(repaired); c.hasNext();)
            calls << c.next();
        if (calls.isEmpty() || calls.size() != all)
            continue;
        bool outOfRange = false;
        bool allFitSwapped = true;
        for (const QRegularExpressionMatch &c : calls)
        {
            const int a = c.captured(1).toInt();
            const int b = c.captured(2).toInt();
            outOfRange = outOfRange || a < 1 || a > first || b < 1 || b > second;
            allFitSwapped = allFitSwapped && b >= 1 && b <= first && a >= 1 && a <= second;
        }
        if (!outOfRange || !allFitSwapped)
            continue;
        for (int i = static_cast<int>(calls.size()) - 1; i >= 0; --i)
        {
            const QRegularExpressionMatch &c = calls[i];
            repaired.replace(static_cast<int>(c.capturedStart(1)),
                             static_cast<int>(c.capturedEnd(2) - c.capturedStart(1)),
                             c.captured(2) + ", " + c.captured(1));
        }
    }
    return repaired == code ? QString() : repaired;
}

QString QtiSasAiContent::matrixTransposedCellRepair(const QString &code) const
{
    // Confirmed live (LIST 4 item 3, iteration 4 twice and iteration 7 run 1): "a matrix with 3 rows and 4 columns ...
    // the value in row 3, column 4, and the sum of row 3" got m.cell(4, 3) and m.cell(1, 3) + ... + m.cell(4, 3) —
    // the whole matrix read transposed. The runtime error ("no row 4 ... cell(row, col) takes the row first") came
    // back with the identical script. Matrix cells take (row, col).
    return swappedCellIndices(code, "newMatrix", "cell|setCell", true);
}

QString QtiSasAiContent::tableTransposedCellRepair(const QString &code) const
{
    // Confirmed live twice (LIST 3 item 5, series 4 NOSPLIT and series 5 LIST, local): newTable("Format", 3, 1) then
    // t.setCell(2, 1, 0.25), t.text(2, 1), ... — rows written as columns; "There's no column 2 in table Format! It
    // has 1 columns" came back with the same script three times and the item was given up. Table cells take
    // (col, row), table-python-api says so with an example.
    return swappedCellIndices(code, "newTable", "cell|setCell|text|setText|cellData|setCellData|cellOrNone", false);
}

QString QtiSasAiContent::setCodeVariableRepair(const QString &code) const
{
    // Every setCode() check and repair reads the body from the call's string literal. Confirmed live (iteration 7,
    // LIST 5 item 1): code_body = """double I = a*x*x + b*x + c;""" then c.setCode(code_body) — the yName repair never
    // saw the body, y was never assigned, and the fit printed its starting values 1 1 1; setCode(<variable>) was
    // written 119 times across the 2026-10-10 logs. A variable assigned a string literal once, at the top level, and
    // passed to setCode() is replaced there by that literal, so the body is checked like any other.
    static const QRegularExpression callRe(R"(\bsetCode\s*\(\s*([A-Za-z_]\w*)\s*\))");
    QString repaired = code;
    QList<QRegularExpressionMatch> calls;
    for (auto it = callRe.globalMatch(code); it.hasNext();)
        calls << it.next();
    for (int i = static_cast<int>(calls.size()) - 1; i >= 0; --i)
    {
        const QString name = calls[i].captured(1);
        const QString n = QRegularExpression::escape(name);
        const QRegularExpression assignRe(
            "^" + n + R"RX(\s*=\s*("""[\s\S]*?"""|'''[\s\S]*?'''|"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')[ \t]*$)RX",
            QRegularExpression::MultilineOption);
        const QRegularExpression anyAssignRe(R"((?<![.\w]))" + n + R"(\s*(?:[-+*/]?=)(?!=))");
        const QRegularExpressionMatch assign = assignRe.match(code);
        int assignments = 0;
        for (auto it = anyAssignRe.globalMatch(codeWithoutStringLiterals(code)); it.hasNext(); it.next())
            ++assignments;
        if (!assign.hasMatch() || assignments != 1 || assign.capturedStart(0) > calls[i].capturedStart(0))
            continue;
        repaired.replace(static_cast<int>(calls[i].capturedStart(1)), static_cast<int>(calls[i].capturedLength(1)),
                         assign.captured(1));
    }
    return repaired == code ? QString() : repaired;
}

QString QtiSasAiContent::bareFolderNameRepair(const QString &code, const QString &requestText) const
{
    // Confirmed live (LIST 5 NOSPLIT local pyi, all three runs of 2026-10-10 and 37 times across the earlier logs):
    // the function compiled as setFunctionName("ai/Lor5"), then best = "Lor5" to print "the name of the function", and
    // configure(best, ...) failed "not found" on every retry. A string literal that is exactly the part after the last
    // "/" of a name the request quotes with its folder becomes that full name — unless the request quotes the short
    // name too (then it may be another object), two quoted names share it, or the script never writes the full name
    // itself (it then shows no sign of meaning that object). Whole literals only: "Lor5 fit" stays.
    // The quoted names, paired quote by quote: a pattern for "a name with a slash" alone would pair the closing quote
    // of one name with the opening quote of the next across a formula's "/" — confirmed live, "2/(1 + (x/0.5)^2)"
    // between "Peak5" and "ai/Gauss5" hid both function names.
    static const QRegularExpression quotedRe(R"re("([^"\n]{1,80})")re");
    static const QRegularExpression folderNameRe(R"(^[^\s/]+(?:/[^\s/]+)+$)");
    QSet<QString> quoted;
    for (auto it = quotedRe.globalMatch(requestText); it.hasNext();)
        quoted.insert(it.next().captured(1));
    QHash<QString, QString> fullOf; // short name -> full name; empty when ambiguous
    for (const QString &full : std::as_const(quoted))
    {
        if (!folderNameRe.match(full).hasMatch())
            continue;
        const QString shortName = full.mid(full.lastIndexOf('/') + 1);
        if (shortName.trimmed().isEmpty() || quoted.contains(shortName) || !code.contains(full))
            continue;
        fullOf[shortName] = fullOf.contains(shortName) && fullOf.value(shortName) != full ? QString() : full;
    }
    if (fullOf.isEmpty())
        return {};
    // Triple-quoted literals first (a setCode() body is matched whole and left alone), then single-line ones.
    static const QRegularExpression literalRe(
        R"RX("""[\s\S]*?"""|'''[\s\S]*?'''|"((?:[^"\\\n]|\\.)*)"|'((?:[^'\\\n]|\\.)*)')RX");
    QString repaired;
    qsizetype last = 0;
    for (auto it = literalRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const bool doubleQuoted = !m.captured(1).isNull();
        const QString text = doubleQuoted ? m.captured(1) : m.captured(2);
        if (text.isNull() || fullOf.value(text).isEmpty())
            continue;
        const QChar q = doubleQuoted ? QLatin1Char('"') : QLatin1Char('\'');
        repaired += code.mid(last, m.capturedStart(0) - last) + q + fullOf.value(text) + q;
        last = m.capturedEnd(0);
    }
    if (last == 0)
        return {};
    return repaired + code.mid(last);
}

QString QtiSasAiContent::labelOnlyResultLineRepair(const QString &code, const QString &requestText) const
{
    // Confirmed live (LIST 5 NOSPLIT NOEXPLORE local pyi, item 3): scriptPrint("R3:") at the top, and at the end of the
    // work scriptPrint(num_layers, curves_top, curves_bottom, f"{r:.4f}") — every value right, on a line of its own,
    // so the result line was "R3:" alone. Repaired only when that is unambiguous: one print of the bare label, no other
    // print of the label, and exactly one later print without it whose values are computed (no fixed text) and at
    // least as many as the request asks for (requestedResultValues() is a lower bound). The label moves onto it.
    const QString label = requestedResultLabel(requestText);
    const qsizetype asked = requestedResultValues(requestText).size();
    if (label.isEmpty() || asked == 0)
        return {};
    const QString escaped = QRegularExpression::escape(label);
    const QRegularExpression labelOnlyRe(R"(^([ \t]*)(?:scriptPrint|print)\s*\(\s*(["'])\s*)" + escaped +
                                             R"(\s*\2\s*(,\s*end\s*=\s*(?:"[^"\n]*"|'[^'\n]*'))?\s*\)[ \t]*$)",
                                         QRegularExpression::MultilineOption);
    const QRegularExpression anyLabelRe(R"((?:scriptPrint|print)\s*\(\s*f?["']\s*)" + escaped);
    // Confirmed live (series 5, local, LIST 3 item 3, NOSPLIT): a first line scriptPrint("R3:", end="") — meant to
    // start the line the later print continues — kept through three retries after scriptPrintKeywordMistake() named
    // it, while the script also printed the full "R3:" line with its values; the item was given up. Such a line,
    // beside another print of the label, says nothing: it goes.
    {
        QList<QRegularExpressionMatch> endOnly;
        for (auto it = labelOnlyRe.globalMatch(code); it.hasNext();)
            if (const QRegularExpressionMatch m = it.next(); !m.captured(3).isEmpty())
                endOnly << m;
        qsizetype prints = 0;
        for (auto it = anyLabelRe.globalMatch(code); it.hasNext(); it.next())
            ++prints;
        if (!endOnly.isEmpty() && prints > endOnly.size())
        {
            QString repaired = code;
            for (int i = static_cast<int>(endOnly.size()) - 1; i >= 0; --i)
                repaired.replace(static_cast<int>(endOnly[i].capturedStart(0)),
                                 static_cast<int>(endOnly[i].capturedLength(0)), endOnly[i].captured(1) + "pass");
            return repaired;
        }
    }
    QList<QRegularExpressionMatch> labelOnly;
    for (auto it = labelOnlyRe.globalMatch(code); it.hasNext();)
        labelOnly << it.next();
    qsizetype labelPrints = 0;
    for (auto it = anyLabelRe.globalMatch(code); it.hasNext(); it.next())
        ++labelPrints;
    if (labelOnly.size() != 1 || labelPrints != 1)
        return {};
    static const QRegularExpression printRe(R"(^([ \t]*)(scriptPrint|print)\s*\(([^\n]*)$)",
                                            QRegularExpression::MultilineOption);
    QList<QRegularExpressionMatch> candidates;
    for (auto it = printRe.globalMatch(code, labelOnly.first().capturedEnd(0)); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        QStringList values;
        if (!resultLineValues("scriptPrint(\"" + label + "\", " + m.captured(3), label, &values) ||
            values.size() < asked)
            continue;
        QString text;
        if (std::any_of(values.cbegin(), values.cend(), [&text](const QString &v) { return fixedTextValue(v, &text); }))
            continue;
        candidates << m;
    }
    if (candidates.size() != 1)
        return {};
    const QRegularExpressionMatch &values = candidates.first();
    QString repaired = code;
    // the later line first, so the earlier offsets stay valid
    repaired.insert(static_cast<int>(values.capturedStart(3)), "\"" + label + "\", ");
    repaired.replace(static_cast<int>(labelOnly.first().capturedStart(0)),
                     static_cast<int>(labelOnly.first().capturedLength(0)), labelOnly.first().captured(1) + "pass");
    return repaired;
}

QString QtiSasAiContent::placeholderResultValueMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 5 NOSPLIT NOEXPLORE local pyi, item 5): scriptPrint("R5:", n.tabs(), existTable("Auto"),
    // "not used") — a requested value (the sum the tab's script computes) replaced by fixed text. Only a line that also
    // computes values (all fixed text is hardcodedResultLineMistake()'s), and only text the request does not give
    // itself ("[]" for "a list, [] if none") or that reports a failure.
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty())
        return {};
    // Every result line, but an indented one only when it is the script's only one: confirmed live (LIST 3 item 3,
    // 2026-10-11), the guard branch `if t is None: scriptPrint("R3:", -1, "TableNotFound")` was taken for the result
    // line (as many values as the real one, and first) and blocked until the item was given up. Failure words are
    // matched inside names too ("TableNotFound", "MissingData").
    const QRegularExpression printRe(R"(^([ \t]*)(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                         QRegularExpression::escape(label) + R"([^\n]*$)",
                                     QRegularExpression::MultilineOption);
    QList<QRegularExpressionMatch> lines;
    for (auto it = printRe.globalMatch(code); it.hasNext();)
        lines << it.next();
    static const QRegularExpression failureTextRe(R"((?:error|not\s*found|missing|failed|fails|does\s*not\s*exist)"
                                                  R"(|doesn'?t\s*exist|cannot|could\s*not|unavailable))",
                                                  QRegularExpression::CaseInsensitiveOption);
    for (const QRegularExpressionMatch &m : lines)
    {
        if (lines.size() > 1 && !m.captured(1).isEmpty())
            continue;
        const QString line = m.captured(0).trimmed();
        QStringList values;
        if (!resultLineValues(line, label, &values) || values.isEmpty())
            continue;
        QString placeholder;
        bool computed = false;
        for (const QString &value : values)
        {
            QString text;
            if (!fixedTextValue(value, &text))
                computed = true;
            else if (placeholder.isEmpty() && !text.trimmed().isEmpty() &&
                     !originalRequestText.contains(text.trimmed(), Qt::CaseInsensitive) &&
                     !failureTextRe.match(text).hasMatch())
                placeholder = value.trimmed();
        }
        if (!computed || placeholder.isEmpty())
            continue;
        return "The result line " + line + " prints the fixed text " + placeholder +
               " where the request asks for a value. Compute that value in this script and print it instead — if "
               "another script computes it (a note tab's script that is not run), compute it here the same way.";
    }
    return {};
}

QString QtiSasAiContent::noWorkFailureResultMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live three times (series 4, local, LIST 3 item 3 in three runs): after a correct block the model sent
    // only scriptPrint("R3:", -1, "TableNotFound") — "Squares" existed and the PROJECT STATE note said so. It ran
    // and the item was lost: every value is a literal, so the placeholder check exempts it as failure text and the
    // fixed-text check wants a single literal. A "not found" result that no lookup in the script produced is a guess.
    // Only a script that does nothing but print: any other statement may be the lookup that reports the failure.
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty())
        return {};
    static const QRegularExpression printLineRe(R"(^(?:scriptPrint|print)\s*\()");
    static const QRegularExpression skipLineRe(R"(^(?:#|import\b|from\s+\S+\s+import\b|pass$))");
    for (const QString &raw : code.split('\n'))
    {
        const QString line = raw.trimmed();
        if (!line.isEmpty() && !skipLineRe.match(line).hasMatch() && !printLineRe.match(line).hasMatch())
            return {};
    }
    const QRegularExpression resultRe(R"(^[ \t]*(?:scriptPrint|print)\s*\(\s*f?["'])" +
                                          QRegularExpression::escape(label) + R"([^\n]*$)",
                                      QRegularExpression::MultilineOption);
    static const QRegularExpression failureTextRe(R"((?:error|not\s*found|missing|failed|fails|does\s*not\s*exist)"
                                                  R"(|doesn'?t\s*exist|cannot|could\s*not|unavailable))",
                                                  QRegularExpression::CaseInsensitiveOption);
    for (auto it = resultRe.globalMatch(code); it.hasNext();)
    {
        const QString line = it.next().captured(0).trimmed();
        QStringList values;
        if (!resultLineValues(line, label, &values) || values.isEmpty())
            continue;
        bool allFixed = true;
        bool failure = false;
        bool fromRequest = false;
        static const QRegularExpression constantRe(R"(^(?:[-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+)?|True|False|None|\[\])$)");
        // arithmetic of number literals only, rounded or not: round(0.01 + 0.01, 3) computes nothing of the item
        static const QRegularExpression literalArithmeticRe(
            R"(^(?:round\s*\()?[-+*/%()\s\d.eE,]*\d[-+*/%()\s\d.eE,]*\)?$)");
        for (const QString &value : values)
        {
            QString text;
            const QString v = value.trimmed();
            if (fixedTextValue(value, &text))
            {
                failure = failure || failureTextRe.match(text).hasMatch();
                text = text.trimmed();
            }
            else if (constantRe.match(v).hasMatch() || literalArithmeticRe.match(v).hasMatch())
                text = v;
            else
                allFixed = false;
            // a value the request itself gives ("Squares") may be printed as it is
            if (!text.isEmpty() && QRegularExpression("(?<![\\w.])" + QRegularExpression::escape(text) + "(?![\\w.])",
                                                      QRegularExpression::CaseInsensitiveOption)
                                       .match(originalRequestText)
                                       .hasMatch())
                fromRequest = true;
        }
        // Confirmed live (benchmark SVD, local pyi, items 6, 9, 10 in every run): only scriptPrint("R10:", 3, 45),
        // scriptPrint("R9:", round(0.01 + 0.01, 3)), scriptPrint("R6:", "decomposition successful") — values written
        // in, none of them the request's, nothing computed
        if (allFixed && !failure && !fromRequest)
            return "This script does no work: it only prints " + line +
                   " — values written into the script, not computed, and none of them given by the request. Do the "
                   "item's work in this script with the documented calls (the recipe of its topic) and print the "
                   "variables that work produces.";
        if (allFixed && failure)
            return "This script does no work: it only prints " + line +
                   " — fixed values and a failure that no lookup in it found. Nothing reported the objects missing: "
                   "the ones this request uses are listed in the PROJECT STATE note when they exist. Do the item's "
                   "work in this script and print the values it produces; if an object really is missing, the script's "
                   "own lookup (table(\"name\") is None) must say so.";
    }
    return {};
}

QString QtiSasAiContent::hardcodedResultLineMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 3, item 3, local model): scriptPrint("R3: 0 ValueError") — the result written as fixed
    // text, nothing in the script computes it. A plain string literal (not an f-string, no further arguments) after
    // the label cannot be a result.
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty())
        return {};
    const QRegularExpression literalRe(R"((?:scriptPrint|print)\s*\(\s*(["']))" + QRegularExpression::escape(label) +
                                       R"(\s*[^"'{}\n]*\S[^"'{}\n]*\1\s*\))");
    // A literal reporting a failure ("R3: table not found") is a guard branch's message, not a fabricated result.
    static const QRegularExpression failureTextRe(
        R"(\b(?:error|not found|missing|failed|fails|does not exist|doesn't exist|cannot|could not|unavailable)\b)",
        QRegularExpression::CaseInsensitiveOption);
    // Nor is an indented one in a script that also prints a computed result line: a fallback branch. Confirmed live
    // (LIST 5 local pyi, item 3): "if t is None: scriptPrint(\"R3: -1\") else: ... scriptPrint(\"R3:\", n, ...)" —
    // blocked twice, the item given up.
    const QRegularExpression computedLineRe(R"((?:scriptPrint|print)\s*\(\s*(?:f["'])" +
                                            QRegularExpression::escape(label) + R"(|["'])" +
                                            QRegularExpression::escape(label) + R"(\s*["']\s*,))");
    const bool printsComputed = computedLineRe.match(code).hasMatch();
    QRegularExpressionMatch m;
    for (auto it = literalRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch candidate = it.next();
        const qsizetype lineStart = code.lastIndexOf('\n', candidate.capturedStart(0)) + 1;
        const bool indented = lineStart < code.size() && code.at(lineStart).isSpace();
        if (!failureTextRe.match(candidate.captured(0)).hasMatch() && !(indented && printsComputed))
        {
            m = candidate;
            break;
        }
    }
    if (!m.hasMatch())
        return {};
    return "The result line " + m.captured(0) + " is fixed text: nothing in this script computes the values after \"" +
           label + "\". Compute them in the script and print the variables, e.g. scriptPrint(\"" + label +
           "\", value1, value2).";
}

QString QtiSasAiContent::repeatedResultLabelMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (LIST 5, item 4, local model): three top-level scriptPrint("R4:", ...) lines, one per value, where
    // the item asks for exactly ONE result line. Only unindented prints count: one per if/else branch is fine.
    const QString label = requestedResultLabel(originalRequestText);
    if (label.isEmpty())
        return {};
    const QRegularExpression topLevelRe(R"(^(?:scriptPrint|print)\s*\(\s*f?["'])" + QRegularExpression::escape(label),
                                        QRegularExpression::MultilineOption);
    int count = 0;
    for (auto it = topLevelRe.globalMatch(code); it.hasNext(); it.next())
        ++count;
    if (count < 2)
        return {};
    return QString("This script prints the result label \"%1\" %2 times, but the request asks for exactly ONE result "
                   "line. Collect all requested values first and print them together in one line: "
                   "scriptPrint(\"%1\", value1, value2, ...).")
        .arg(label)
        .arg(count);
}

QString QtiSasAiContent::missingRandomImportMistake(const QString &code)
{
    // Confirmed live, repeatedly — a script generating synthetic data with added noise calls
    // random.gauss(...)/random.uniform(...)/etc. with no `import random` line, reaching execution
    // unchanged and raising NameError, burning a full retry round before anyone adds the missing
    // import. Same shape as missingPyQtImportMistake: a mechanical backstop for a mistake the model
    // makes reliably enough that a checklist reminder alone doesn't prevent it. The usage lookbehind
    // requires `random` not be immediately preceded by a dot or word character, so a qualified
    // reference (e.g. a variable named `my_random`, or `np.random.gauss(...)`) is never misflagged;
    // np.random specifically needs `import numpy as np`, a separate, unrelated import this check
    // doesn't attempt to verify.
    static const QRegularExpression usageRe(R"((?<![.\w])random\.\w+\s*\()");
    if (!usageRe.match(code).hasMatch())
        return {};
    static const QRegularExpression importRe(R"(\bimport\b[^\n]*\brandom\b)");
    if (importRe.match(code).hasMatch())
        return {};
    return "This script calls random.<method>(...) (e.g. random.gauss(), random.uniform()) with no "
           "`import random` line — `random` is not in the default namespace, so this raises "
           "NameError at runtime. Add `import random` near the top of the script instead of "
           "leaving this for a runtime retry to catch.";
}

QString QtiSasAiContent::pythonInputCallMistake(const QString &code)
{
    // Confirmed live: input() on the main GUI thread blocked on a real stdin read for 174+ seconds.
    static const QRegularExpression usageRe(R"((?<![.\w])input\s*\()");
    if (!usageRe.match(codeWithoutStringLiterals(code)).hasMatch())
        return {};
    return "This script calls input(...) — embedded scripts have no interactive console, so input() "
           "can never be answered and instead BLOCKS THE ENTIRE APPLICATION on a real stdin read. "
           "Never call input() here — get the value from a parameter, an existing table/object, or a "
           "hardcoded default instead.";
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

QString QtiSasAiContent::selfReviewCompileSequenceRegression(const QString &preReviewDraft,
                                                             const QString &postReviewCode) const
{
    // Confirmed live: a correct, complete compile setup — c.new()/setFunctionName()/setGroup()/
    // setParamCount()/setParamNamesAll()/setParamInitsAll()/setParamLimitsAll()/
    // setParamVariesAll()/setCode()/compile() — was replaced wholesale with old-style raw-array
    // C-function code (`double Fn(double* x, double* par) { double scale = par[0]; ... }`) with no
    // setGroup(), no parameter declarations at all, still ending in a compile() call — raising
    // ValueError: "compile: group is empty" on the very next run. Only applies when BOTH scripts
    // still attempt to compile something; a legitimate decision to stop compiling entirely is a
    // different, unrelated edit this check has no opinion on.
    static const QRegularExpression compileCallRe(R"(\.compile\s*\()");
    if (!compileCallRe.match(preReviewDraft).hasMatch() || !compileCallRe.match(postReviewCode).hasMatch())
        return {};
    static const QStringList requiredSetupCalls = {"setFunctionName",  "setGroup",         "setParamCount",
                                                   "setParamNamesAll", "setParamInitsAll", "setParamLimitsAll",
                                                   "setParamVariesAll"};
    QStringList dropped;
    for (const QString &callName : requiredSetupCalls)
    {
        const QRegularExpression callRe(R"(\.)" + callName + R"(\s*\()");
        if (callRe.match(preReviewDraft).hasMatch() && !callRe.match(postReviewCode).hasMatch())
            dropped << callName + "()";
    }
    if (dropped.isEmpty())
        return {};
    return "The script you were reviewing still had a complete Compiler setup, but your own edit "
           "still calls compile() while dropping " +
           dropped.join(", ") +
           " — present in the reviewed draft, missing now. compile-python-api.md's minimal "
           "create-and-compile flow requires ALL of setFunctionName()/setGroup()/setParamCount()/"
           "setParamNamesAll()/setParamInitsAll()/setParamLimitsAll()/setParamVariesAll() before "
           "compile() for a function using them — dropping any of these either raises a specific "
           "ValueError (e.g. \"group is empty\") or silently leaves parameters undeclared/unfixed. "
           "Restore the dropped call(s) exactly as they were in the reviewed draft.";
}

QHash<QString, bool> QtiSasAiContent::openSessionsHaveSetCode(const QString &code)
{
    // For each `c.open("<name>")` call, the "session" for that name runs from right after the
    // open() call up to the NEXT compile() call in the script (or the end of the script if none
    // follows) — mirrors how a Compiler script actually reads: open, optionally reconfigure, then
    // compile. Returns whether a setCode() call appears anywhere in that session, per name.
    QHash<QString, bool> result;
    static const QRegularExpression openRe(R"(\.open\s*\(\s*["']([^"']+)["']\s*\))");
    static const QRegularExpression compileRe(R"(\.compile\s*\()");
    static const QRegularExpression setCodeRe(R"(\.setCode\s*\()");
    QRegularExpressionMatchIterator it = openRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1);
        const int sessionStart = static_cast<int>(m.capturedEnd());
        const QRegularExpressionMatch compileMatch = compileRe.match(code, sessionStart);
        const int sessionEnd =
            compileMatch.hasMatch() ? static_cast<int>(compileMatch.capturedStart()) : static_cast<int>(code.length());
        const QString session = code.mid(sessionStart, sessionEnd - sessionStart);
        result[name] = setCodeRe.match(session).hasMatch();
    }
    return result;
}

QString QtiSasAiContent::selfReviewFabricatedSetCodeMistake(const QString &preReviewDraft,
                                                            const QString &postReviewCode) const
{
    // Confirmed live, severe: EXPLORE had already printed the EXISTING, correct code for two
    // functions ("y = scale * exp(-pow((x - center) / width, 2.0) / 2.0) + background;"); the
    // pre-review draft correctly did nothing but `c.open(name); ok = c.compile()` for each —
    // reusing that known-good code with no setCode() call at all. Self-review rewrote this into a
    // version that adds `c.setFunctionName(name)` (a no-op self-rename) and a brand-new
    // `c.setCode(...)` with a fabricated, wrong-syntax body — neither function actually compiled
    // (both returned False). Self-review has no docs or history telling it what the existing code
    // actually is, so inventing a replacement is a guess, never a legitimate fix, when the draft
    // correctly relied on reuse instead.
    const QHash<QString, bool> preSessions = openSessionsHaveSetCode(preReviewDraft);
    const QHash<QString, bool> postSessions = openSessionsHaveSetCode(postReviewCode);
    for (auto it = preSessions.constBegin(); it != preSessions.constEnd(); ++it)
    {
        if (it.value())
            continue; // draft already had its own setCode() for this name — not this pattern
        if (!postSessions.value(it.key(), false))
            continue; // self-review's edit still has no setCode() for this name either — fine
        return "The script you were reviewing correctly reused the EXISTING compiled function \"" + it.key() +
               "\" — opening it and recompiling with NO setCode() call, relying on whatever code "
               "is already loaded — but your own edit added a setCode() call for that SAME "
               "function with fabricated content. You have no way to know what the existing, "
               "already-working code actually is — inventing a replacement is a guess, not a fix. "
               "Restore the reviewed draft's behavior for \"" +
               it.key() + "\": open it and recompile with no setCode() call of your own.";
    }
    return {};
}

QString QtiSasAiContent::selfReviewPyQtVersionSubstitution(const QString &preReviewDraft,
                                                           const QString &postReviewCode) const
{
    // Confirmed live: a pre-review draft correctly imported `from PyQt6.QtGui import QColor, QFont`
    // (this build's real binding). Self-review's own output — while fixing an UNRELATED issue
    // elsewhere in the same script — silently rewrote that line to `from PyQt5.QtGui import QColor,
    // QFont`. PyQt5 doesn't exist in this environment; the script reached actual execution and
    // crashed with "ModuleNotFoundError: No module named 'PyQt5'" a full round-trip later. Self-
    // review regenerates the WHOLE script from scratch every pass rather than patching only the
    // lines it was actually asked to fix, so an unrelated-but-already-correct import has no
    // protection from being silently "re-derived" from the model's own general training-data priors
    // (which skew heavily toward the far more common PyQt5) instead of copied verbatim — the same
    // root cause as selfReviewFunctionNameSubstitution/selfReviewEnumConstantRegression above, just
    // never covered for PyQt major version before. Self-review has no way to observe which PyQt
    // version this build actually embeds (no docs, no history), so it has no legitimate basis to
    // switch between them either direction.
    static const QStringList qtClassNames = {"QColor", "QFont", "QPen", "QBrush", "QFrame", "Qt"};
    static const QRegularExpression importLineRe(R"(from\s+PyQt(5|6)\.\w+\s+import\s+([^\n]+))");
    auto versionsByClass = [](const QString &code) {
        QHash<QString, QString> versionOf;
        QRegularExpressionMatchIterator it = importLineRe.globalMatch(code);
        while (it.hasNext())
        {
            const QRegularExpressionMatch m = it.next();
            const QString version = m.captured(1);
            const QString namesPart = m.captured(2);
            for (const QString &className : qtClassNames)
            {
                const QRegularExpression classRe(R"(\b)" + QRegularExpression::escape(className) + R"(\b)");
                if (classRe.match(namesPart).hasMatch())
                    versionOf[className] = version;
            }
        }
        return versionOf;
    };
    const QHash<QString, QString> preVersions = versionsByClass(preReviewDraft);
    const QHash<QString, QString> postVersions = versionsByClass(postReviewCode);
    for (const QString &className : qtClassNames)
    {
        if (!preVersions.contains(className) || !postVersions.contains(className))
            continue;
        if (preVersions.value(className) == postVersions.value(className))
            continue;
        return "Your last response changed \"" + className + "\"'s import from PyQt" + preVersions.value(className) +
               " (in the reviewed draft) to PyQt" + postVersions.value(className) +
               " — self-review has no way to know which PyQt major version this build actually "
               "embeds, so switching between them is more likely a hallucinated regression than a "
               "real fix. Restore the import to PyQt" +
               preVersions.value(className) + " exactly as it was in the reviewed draft.";
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
    //
    // ALSO confirmed live in the identical shape applied to a table COLUMN name instead of an
    // object name: `for c in ["residuals", "Fit Residuals", "residues", "Fit Residues"]: if c in
    // cols: res_col = c; break` — guessing among 4 spellings of the residuals column, when
    // "residues" is the single confirmed-correct name (used directly, no guessing, by the very
    // next split-mode part in the SAME conversation). Same root cause, same fix: recognize `if var
    // in <container>:` membership testing as an equally strong "this is guessing candidates
    // against a real list" signal alongside the fixed lookupFns call list below.
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
        const QString trailing = code.mid(loopEnd);
        static const QRegularExpression firstMatchOnlyRe(R"(\bbreak\b|\[\s*0\s*\]|\.pop\s*\(\s*0\s*\)|\bnext\s*\()");
        if (!firstMatchOnlyRe.match(trailing).hasMatch())
            continue;
        for (const QString &fn : lookupFns)
        {
            const QRegularExpression lookupCallRe(R"re(\b)re" + QRegularExpression::escape(fn) + R"re(\s*\(\s*)re" +
                                                  QRegularExpression::escape(var) + R"re(\s*\))re");
            if (!lookupCallRe.match(code).hasMatch())
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
        const QRegularExpression membershipRe(R"re(\bif\s+)re" + QRegularExpression::escape(var) +
                                              R"re(\s+in\s+(\w+)\s*:)re");
        const QRegularExpressionMatch memberMatch = membershipRe.match(trailing);
        if (memberMatch.hasMatch())
        {
            return "This script iterates over a hardcoded list of guessed candidate spellings (" +
                   listContents.trimmed() + ") and checks `" + var + " in " + memberMatch.captured(1) +
                   "` to find whichever one already exists, keeping only the FIRST match — this "
                   "silently picks the WRONG column/name the moment more than one guessed spelling "
                   "happens to exist, with no error at all. Use the single confirmed-correct name "
                   "directly instead of guessing a list of plausible spellings — if genuinely "
                   "unknown, print " +
                   memberMatch.captured(1) +
                   " via scriptPrint() in an EXPLORE "
                   "round first and use the real name it shows.";
        }
    }
    return {};
}

QString QtiSasAiContent::fragmentedStaticScriptPrintMistake(const QString &code)
{
    // Confirmed live: a script whose only job was printing a multi-paragraph static summary issued
    // one scriptPrint("literal text") call per line (20+ calls in a row) instead of building ONE
    // multi-line string and calling scriptPrint() a single time — exactly the pattern the "Tips for
    // better results" doc section asks the model to avoid, but that section is advisory prose the
    // model doesn't reliably follow (and self-review runs with includeDocs=false, so it never even
    // sees it) — a mechanical check here is the only reliable enforcement. Narrow to RUNS of
    // consecutive lines that are ONLY `scriptPrint("literal")`/`scriptPrint('literal')` with no
    // concatenation, formatting, or variables — a script printing computed/looped values one at a
    // time (the doc's own carve-out) never matches this shape.
    static const QRegularExpression staticPrintLineRe(R"(^[ \t]*scriptPrint\(\s*(?:"[^"\\]*"|'[^'\\]*')\s*\)[ \t]*$)");
    const QStringList lines = code.split('\n');
    int run = 0;
    for (const QString &line : lines)
    {
        if (staticPrintLineRe.match(line).hasMatch())
        {
            ++run;
            if (run >= 4)
                return "This script prints a long run of purely static text via one scriptPrint() "
                       "call per line instead of one multi-line string + one call — build a single "
                       "multi-line string and call scriptPrint() once instead (this doesn't apply to "
                       "values printed one at a time as they're computed, e.g. progress inside a "
                       "loop, or a value only known after an earlier call on the same line).";
        }
        else
        {
            run = 0;
        }
    }
    return {};
}

QString QtiSasAiContent::doubleEscapedRawRegexMistake(const QString &code)
{
    // Confirmed live: `re.search(r"\\b" + re.escape(p) + r"\\b", c.code())` — a RAW string literal
    // r"\\b" is literally the THREE characters backslash-backslash-b, not the single-backslash
    // word-boundary escape \b. Python's re engine reads \\ as an escaped literal backslash, so the
    // compiled pattern requires an actual backslash character immediately before/after the
    // searched-for name — something that never appears in c.code() (or any typical compiled-
    // function source) — so the match can NEVER succeed for ANY name, silently reporting every
    // declared parameter as "unused" regardless of whether it's genuinely used. Harmless in
    // outcome here (an informational scriptPrint warning, not a raise), but a real, misleading
    // diagnostic printed as fact. Narrow, high-precision signal: a raw string starting with a
    // DOUBLED backslash immediately before one of the common single-letter regex escapes
    // (b/d/s/w, case-insensitive) is essentially always this exact mistake — a genuine intent to
    // match a literal backslash character followed by that letter is vanishingly rare — combined
    // with actual use of the re module's own matching functions elsewhere in the same script (the
    // context that makes "this is meant as a regex escape" the overwhelmingly likely reading).
    static const QRegularExpression reCallRe(
        R"(\bre\.(?:search|match|fullmatch|findall|finditer|sub|subn|compile)\s*\()");
    if (!reCallRe.match(code).hasMatch())
        return {};
    static const QRegularExpression doubledEscapeRe(R"(r["']\\\\[bdswBDSW])");
    const QRegularExpressionMatch m = doubledEscapeRe.match(code);
    if (!m.hasMatch())
        return {};
    return "This script uses a raw string literal (`" + m.captured(0) +
           "...`) starting with a DOUBLED backslash before a regex escape letter, alongside a "
           "real re.* call elsewhere in the script. In a raw string, \\\\ is TWO literal backslash "
           "characters, not the single-backslash escape (\\b/\\d/\\s/\\w) it looks like — the "
           "compiled pattern then requires an actual backslash character in the text being "
           "searched, which is never present in typical compiled-function source code, so the "
           "match silently fails every time with no error, misreporting things as absent that are "
           "actually present. Use a single backslash instead: r\"\\b\" (word boundary), never "
           "r\"\\\\b\".";
}

QString QtiSasAiContent::singletonCalledAsConstructorMistake(const QString &code)
{
    // Confirmed live, repeatedly and independently — the checklist already documents this exact
    // incident in detail (generalChecklistRules(): "a review pass changed an already-correct
    // `c = Compiler` into `c = Compiler()`, and the resulting TypeError triggered a cascade of
    // retries guessing ever-more-unrelated alternatives... before exhausting the retry budget
    // entirely") — and self-review still produces it. Compiler/Fittable/Dan are pre-existing
    // GLOBAL SINGLETON instances exposed directly to Python, accessed via their bare name, never
    // constructed. Calling one raises `TypeError: '<name>18' object is not callable` — the class
    // name (compile18/fittable18/dan18) reveals the underlying C++ singleton. A checklist item
    // alone doesn't reliably prevent this (same lesson as the math-import and QFrame-combo
    // checklist items below); this makes it a mechanical, pre-execution certainty instead.
    static const QHash<QString, QString> internalClassName = {
        {"Compiler", "compile18"}, {"Fittable", "fittable18"}, {"Dan", "dan18"}};
    for (auto it = internalClassName.constBegin(); it != internalClassName.constEnd(); ++it)
    {
        const QRegularExpression callRe(R"(\b)" + it.key() + R"(\s*\(\s*\))");
        if (!callRe.match(code).hasMatch())
            continue;
        return "This script calls " + it.key() + "() as if it were a constructor — " + it.key() +
               " is a pre-existing GLOBAL SINGLETON instance, exposed directly to Python and "
               "accessed via its bare name (e.g. `c = " +
               it.key() + "`), never called as a function. " + it.key() + "() raises `TypeError: '" + it.value() +
               "' object is not callable` (the class name reveals the underlying C++ singleton). "
               "Remove the parentheses.";
    }
    return {};
}

QString QtiSasAiContent::pyQt6FrameStyleComboMistake(const QString &code)
{
    // Confirmed live, at least twice independently — the checklist already documents this exact
    // incident in detail (graphChecklistRules()): "a script styled a fit-result legend with
    // exactly this QFrame combo; it ran with no error and no visual sign anything was wrong, the
    // requested boxed/sunken legend frame just silently never appeared." QtiSAS's own
    // Frame.FrameStyle enum (None_=0, Line=1, Shadow=2) is unrelated to PyQt6's
    // QFrame.Shape/QFrame.Shadow and does an EXACT-value match, not bit-flag decoding — a PyQt6
    // combo produces some int matching neither Line nor Shadow, so setFrameStyle() raises no
    // error at all; the frame border simply never renders. A checklist item alone doesn't
    // reliably prevent this — self-review has produced this exact combo again since that item was
    // written — so this makes it a mechanical, pre-execution certainty instead.
    static const QRegularExpression comboRe(
        R"(setFrameStyle\s*\([^()]*\bQFrame\.\w+(?:\.\w+)?[^()]*\|[^()]*\bQFrame\.\w+(?:\.\w+)?[^()]*\))");
    if (!comboRe.match(code).hasMatch())
        return {};
    return "This script calls setFrameStyle(QFrame.<Shape> | QFrame.<Shadow>) — a PyQt6 QFrame "
           "bit-flag combo. QtiSAS's own Frame.FrameStyle enum (None_=0, Line=1, Shadow=2) is a "
           "DIFFERENT, unrelated enum that does an EXACT-value match, not bit-flag decoding — the "
           "PyQt6 combo produces some int matching neither Line nor Shadow, so this raises no "
           "error but the frame border simply never renders, with no visual sign anything is "
           "wrong. Use Frame.FrameStyle.Line.value or Frame.FrameStyle.Shadow.value instead — "
           "never a PyQt6 QFrame.Shape/QFrame.Shadow value, combined or otherwise.";
}

QString QtiSasAiContent::fittableParametersDirectiveIgnoredMistake(const QString &code,
                                                                   const QString &originalRequestText)
{
    // The checklist rule for this (fittableChecklistRules()) has been confirmed live, twice, to
    // not reliably change output — self-review left the same gap in place both times. Mechanical
    // check instead, same rationale as pyQt6FrameStyleComboMistake() above.
    static const QRegularExpression directiveRe(R"re(\(([^()]+?)\s*-\s*(un)?fittable\s+parameters?\))re",
                                                QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch dm = directiveRe.match(originalRequestText);
    if (!dm.hasMatch())
        return {};

    static const QRegularExpression variesCallRe(R"re(setParamVaries\s*\(\s*"([^"]+)"\s*,)re");
    QSet<QString> touchedParams;
    QRegularExpressionMatchIterator it = variesCallRe.globalMatch(code);
    while (it.hasNext())
        touchedParams.insert(it.next().captured(1));
    if (touchedParams.isEmpty())
        return {}; // this part hasn't reached parameter configuration at all — not what this checks

    QStringList namedParams;
    for (const QString &p : dm.captured(1).split(',', Qt::SkipEmptyParts))
        namedParams << p.trimmed();

    // Confirmed live, repeatedly, on the identical request: every setParamVaries() call covers
    // ONLY the named parameters — the "every OTHER parameter gets the opposite value" half of the
    // directive is silently skipped, leaving their vary-state at Fittable's own default instead.
    for (const QString &p : touchedParams)
        if (!namedParams.contains(p, Qt::CaseInsensitive))
            return {}; // some other parameter WAS touched — directive apparently already handled
    const bool isUnfittableList = !dm.captured(2).isEmpty();
    return "The request says \"(" + dm.captured(1).trimmed() + " - " + (isUnfittableList ? "un" : "") +
           "fittable parameters)\", but this script's setParamVaries() calls cover ONLY those "
           "named parameters — every OTHER parameter of this model must get an explicit "
           "setParamVaries(name, " +
           QString(isUnfittableList ? "True" : "False") +
           ") call too, not be left at whatever Fittable's own default is. If the model's full "
           "parameter list isn't already confirmed earlier in this conversation, run an EXPLORE "
           "round calling f.paramNames() first to discover it (documented in "
           "fittable-python-api.md) — never skip straight to setParamVaries() calls for only the "
           "named parameters and leave the rest untouched.";
}

QString QtiSasAiContent::redundantFittableReconfigureMistake(const QString &code, const QString &earlierPartsCode)
{
    // Confirmed live, immediately downstream of fittableParametersDirectiveIgnoredMistake() above
    // fixing part 1's parameter setup: part 2 called configure() AGAIN for the same model, with a
    // wrong dataset list ([data_col, error_col] as if two separate datasets, instead of the error
    // column via setWeightingDataset()) — silently discarding every setParamValue()/
    // setParamLimits()/setParamVaries() call part 1 had just gotten right, since configure()
    // re-initializes the whole Fittable session for whatever it's given.
    if (earlierPartsCode.isEmpty())
        return {};
    static const QRegularExpression configureRe(R"re(\bconfigure\s*\(\s*"([^"]+)")re");
    QString earlierModel;
    QRegularExpressionMatchIterator earlierIt = configureRe.globalMatch(earlierPartsCode);
    while (earlierIt.hasNext())
        earlierModel = earlierIt.next().captured(1); // last one wins — the currently-configured model
    if (earlierModel.isEmpty())
        return {};
    const QRegularExpressionMatch thisMatch = configureRe.match(code);
    if (!thisMatch.hasMatch() || thisMatch.captured(1) != earlierModel)
        return {};
    // Nothing is lost when this script sets up again everything the earlier part set after its configure(): confirmed
    // live (LIST 5, item 1), a part that re-did configure, the start values, the weighting and the fit was blocked
    // twice, and the item given up.
    static const QRegularExpression setterRe(
        R"re(\b(setParam(?:Value|Limits|Varies)\s*\(\s*"[^"]+"|setWeighting(?:Enabled|Dataset)\b))re");
    const qsizetype lastEarlierConfigure = earlierPartsCode.lastIndexOf(configureRe);
    QStringList earlierSetters;
    for (auto it = setterRe.globalMatch(earlierPartsCode, lastEarlierConfigure); it.hasNext();)
        earlierSetters << it.next().captured(1).remove(QRegularExpression(R"(\s)"));
    QStringList ownSetters;
    for (auto it = setterRe.globalMatch(code, thisMatch.capturedEnd()); it.hasNext();)
        ownSetters << it.next().captured(1).remove(QRegularExpression(R"(\s)"));
    bool allRedone = true;
    for (const QString &setter : earlierSetters)
        if (!ownSetters.contains(setter))
            allRedone = false;
    if (allRedone)
        return {};
    return "This script calls configure(\"" + earlierModel +
           "\", ...) again, but an earlier part "
           "in this same conversation already configured Fittable for this exact model — "
           "configure() re-initializes the whole session, silently discarding every "
           "setParamValue()/setParamLimits()/setParamVaries() call that earlier part made. Remove "
           "this redundant configure() call entirely and continue directly from where the earlier "
           "part left off (f = Fittable is enough to rebind the alias); if this part also needs "
           "weighting enabled, call setWeightingEnabled()/setWeightingDataset() directly — never "
           "re-configure() to add it.";
}

QString QtiSasAiContent::compilerScanGroupsAfterNameMistake(const QString &code)
{
    // compile18::scanGroups() repopulates the explorer and CLEARS the editor fields as it goes —
    // compile-explorer.cpp does lineEditGroupName->setText("") and lineEditFunctionName->setText("")
    // — so a scanGroups() between setFunctionName() and compile() silently throws the name away.
    // Confirmed live: a script ran new() -> setFunctionName("ai/PorodPowerLaw") -> scanGroups() ->
    // ... -> compile() and died on "compile: function name is empty", which reads as a
    // contradiction of the setFunctionName() call plainly sitting three lines above. Calling
    // scanGroups() to look up a group name mid-sequence is the natural thing to do, which is why
    // this needs catching rather than documenting alone.
    static const QRegularExpression nameRe(R"(\bsetFunctionName\s*\()");
    static const QRegularExpression scanRe(R"(\bscanGroups\s*\(\s*\))");
    static const QRegularExpression compileRe(R"(\.\s*(?:compile|save)\s*\()");
    const QRegularExpressionMatch nameMatch = nameRe.match(code);
    if (!nameMatch.hasMatch())
        return {};
    const QRegularExpressionMatch compileMatch = compileRe.match(code, nameMatch.capturedEnd());
    if (!compileMatch.hasMatch())
        return {}; // name set but never compiled/saved here — nothing this check can say
    const QRegularExpressionMatch scanMatch = scanRe.match(code, nameMatch.capturedEnd());
    if (!scanMatch.hasMatch() || scanMatch.capturedStart() > compileMatch.capturedStart())
        return {}; // no scanGroups() in the window between naming and compiling
    return "This script calls scanGroups() AFTER setFunctionName() and before compile()/save(). "
           "scanGroups() rescans the function explorer and clears the editor fields while it does "
           "so — including the function name — so the name set above is gone by the time "
           "compile() runs, which then fails with \"compile: function name is empty\" even though "
           "setFunctionName() is plainly there. Call scanGroups()/groups() FIRST, before new(), "
           "pick the group, and only then call new()/setFunctionName()/setGroup()/.../compile().";
}

QString QtiSasAiContent::fittableFitBeforeConfigureMistake(const QString &code)
{
    // The sibling of fittableSelectStageBeforeFunctionMistake() below, for fit() rather than
    // selectStage(). fittable-python-api.md: "fit(), simulate(), plotResults() ... raise
    // RuntimeError if stage isn't \"fit\"", and configure()/setFunction() is what gets it there.
    // Confirmed live: a script called f.fit() with only a table lookup in front of it and died on
    // "fit(): call setFunction() and selectDataset() first (stage must be \"fit\")" — one wasted
    // round-trip for a precondition the docs state outright.
    static const QRegularExpression fitRe(R"((?<![.\w])\w+\s*\.\s*fit\s*\(\s*\))");
    // Jnse.initFit() configures Fittable too: confirmed live (benchmark JNSE holdout, Codex) — Jnse.initFit(...) then
    // Fittable.fit() was blocked, the model added Fittable.configure(), which reset the q of every dataset to the
    // function's default, and the fit printed D = 0.01 instead of 1.20
    static const QRegularExpression functionSelectRe(R"(\b(?:configure|setFunction|initFit)\s*\()");
    // Calls only, never text: confirmed live (BIGMODEL, ai-test-requests item 10) — scriptPrint("... run a fit first
    // (e.g. f.fit() ...)") was blocked as a fit() call without configure().
    const QString calls = codeWithLiteralsBlanked(code);
    const QRegularExpressionMatch fitMatch = fitRe.match(calls);
    if (!fitMatch.hasMatch())
        return {};
    const QRegularExpressionMatch functionMatch = functionSelectRe.match(calls);
    if (functionMatch.hasMatch() && functionMatch.capturedStart() < fitMatch.capturedStart())
        return {};
    return "This script calls fit() without configure()/setFunction() having selected a function "
           "and dataset first, so it raises RuntimeError (\"fit(): call setFunction() and "
           "selectDataset() first (stage must be \\\"fit\\\")\"). Reading the data table is not "
           "enough — Fittable's own stage has to be moved out of \"init\". Call "
           "configure(<function>, [<dataset>]) BEFORE fit(). Note that a PREVIOUS script's "
           "configure() does not carry over: each script starts from whatever stage the UI is "
           "actually in, which state()[\"stage\"] reports as \"init\"/\"ready\"/\"fitted\".";
}

QString QtiSasAiContent::scriptEditPrintDialogMistake(const QString &code)
{
    // Confirmed live (benchmark LIST 4, three runs in a row): to read a note tab's text, a retry called
    // n.tab("C").print() — ScriptEdit::print() opens the system's PRINT dialog (a native one on macOS, which nothing
    // can close from a script), so the run hung until it was killed. A note tab is reached by tab(...), editorAt(...)
    // or currentEditor(); a bare print(...) is Python's own and never matches.
    static const QRegularExpression printRe(R"(\b(?:tab|editorAt|currentEditor)\s*\([^()]*\)\s*\.\s*print\s*\()");
    if (!printRe.match(code).hasMatch())
        return {};
    return "This script calls print() on a note tab (a ScriptEdit). That does not return or show the tab's text: it "
           "opens the system's printer dialog, which blocks the run. To read a tab's text use toPlainText(), e.g. "
           "n.tab(\"C\").toPlainText(); to write it, setText(...).";
}

QString QtiSasAiContent::duplicateWindowNameMistake(const QString &code)
{
    // Confirmed live ("make a 3D plot with 3 spheres"): three top-level newPlot3D("Spheres", ...) calls — three
    // windows, the second and third renamed by QtiSAS, instead of the one plot asked for. Only unindented calls: the
    // same name in an if/else pair is fine.
    static const QRegularExpression createRe(
        R"(^(?:\w+\s*=\s*)?(newGraph|newPlot3D|newTable|newMatrix|newNote)\s*\(\s*["']([^"']+)["'])",
        QRegularExpression::MultilineOption);
    QHash<QString, int> count;
    QRegularExpressionMatchIterator it = createRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(2);
        if (++count[name] == 2)
            return "This script calls " + m.captured(1) + "(\"" + name +
                   "\", ...) more than once. Each call creates a NEW window — QtiSAS renames the later ones — so "
                   "this makes several windows where one was asked for. Create the window once and add the rest to "
                   "it; if that is not possible for this window type, say so instead of creating copies.";
    }
    return {};
}

QString QtiSasAiContent::textIntoNumericColumnMistake(const QString &code)
{
    // Confirmed live (benchmark LIST 4 item 5): t = newTable(...) then t.setText(2, 1, "alpha") without
    // setColTextFormat(2) — a new table's columns are Numeric, Table::cellEdited() evaluates "alpha" as an expression
    // and leaves the cell empty, so text(2, 2) read back "". Only tables this script creates: their columns are known
    // to be Numeric until a setCol...Format() call says otherwise.
    // The table's variable: t = newTable(...), or — the same table — newTable("Names", ...) on a line of its own and
    // t = table("Names") after it (confirmed live, LIST 4 item 5 in split mode: the check never saw the table).
    struct CreatedTable
    {
        QString var;
        qsizetype from = 0;
    };
    QList<CreatedTable> created;
    static const QRegularExpression newTableRe(R"(\b(\w+)\s*=\s*newTable\s*\()");
    for (auto it = newTableRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        created << CreatedTable{m.captured(1), m.capturedEnd()};
    }
    static const QRegularExpression bareNewTableRe(R"RX(^[ \t]*newTable\s*\(\s*(?:"([^"]+)"|'([^']+)'))RX",
                                                   QRegularExpression::MultilineOption);
    for (auto it = bareNewTableRe.globalMatch(code); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        const QString name = QRegularExpression::escape(m.captured(1).isEmpty() ? m.captured(2) : m.captured(1));
        const QRegularExpression lookupRe(R"RX(\b(\w+)\s*=\s*table\s*\(\s*["'])RX" + name + R"RX(["']\s*\))RX");
        if (const QRegularExpressionMatch lookup = lookupRe.match(code, m.capturedEnd()); lookup.hasMatch())
            created << CreatedTable{lookup.captured(1), lookup.capturedEnd()};
    }
    for (const CreatedTable &table : std::as_const(created))
    {
        const QString var = QRegularExpression::escape(table.var);
        // The text is a literal, or the first word of a list of them indexed per row — confirmed live (BlaBlaDor,
        // series 6, LIST 4 item 5): t.setText(2, i, ["alpha", "beta", "gamma"][i-1]), every cell left empty.
        const QRegularExpression setTextRe("\\b" + var +
                                           R"(\s*\.\s*setText\s*\(\s*(\d+)\s*,[^,()]*,\s*(?:\[\s*)?(["'])(.*?)\2)");
        QRegularExpressionMatchIterator it = setTextRe.globalMatch(code, table.from);
        while (it.hasNext())
        {
            const QRegularExpressionMatch m = it.next();
            // Anything with a digit may be a number in the table's own locale ("2,5" after German separators,
            // confirmed live as a false alarm when only C numbers were accepted) or an expression ("2*pi"): left alone.
            const QString text = m.captured(3).trimmed();
            static const QRegularExpression digitRe(R"(\d)");
            if (text.isEmpty() || digitRe.match(text).hasMatch())
                continue;
            const QRegularExpression formatRe("\\b" + var + R"(\s*\.\s*setCol\w*Format\s*\(\s*)" + m.captured(1) +
                                              R"(\s*[,)])");
            const QRegularExpressionMatch format = formatRe.match(code, table.from);
            if (format.hasMatch() && format.capturedStart() < m.capturedStart())
                continue;
            return "This script writes the text \"" + text + "\" with setText() into column " + m.captured(1) +
                   " of a table it created with newTable(), whose columns are Numeric. In a Numeric column, text that "
                   "is no number is evaluated as an expression, and \"" +
                   text + "\" is none, so the cell stays EMPTY. Call " + table.var + ".setColTextFormat(" +
                   m.captured(1) + ") before writing text into that column.";
        }
    }
    return {};
}

QString QtiSasAiContent::matrixIntegrateAsSumMistake(const QString &code, const QString &originalRequestText)
{
    // Confirmed live (benchmark LIST 3 item 6, local model, every run): "print the sum of all cells" answered with
    // m.integrate() — 56.25 instead of 100 for a 4x4 i*j matrix — although matrix-python-api.md says integrate() is
    // not the sum and the line was in the model's context. Only when the request asks for a sum of a matrix and
    // never for an integral.
    static const QRegularExpression sumRe(R"(\bsum\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression matrixRe(R"(\bmatri(?:x|ces)\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression integralRe(R"(\bintegra)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression integrateRe(R"(\.\s*integrate\s*\()");
    if (!integrateRe.match(code).hasMatch() || !sumRe.match(originalRequestText).hasMatch() ||
        !matrixRe.match(originalRequestText).hasMatch() || integralRe.match(originalRequestText).hasMatch())
        return {};
    return "This script uses Matrix.integrate() for the requested sum. integrate() is NOT the sum of the cells: it is "
           "the 2D trapezoidal integral over the matrix's x/y coordinates (edge cells weighted 1/2, corners 1/4, "
           "times dx*dy). Add the cells instead: sum(m.cell(r, c) for r in range(1, m.numRows() + 1) "
           "for c in range(1, m.numCols() + 1)).";
}

QString QtiSasAiContent::fittableSelectStageBeforeFunctionMistake(const QString &code)
{
    // fittable-python-api.md: selectStage(s) with s != "init" raises RuntimeError while Fittable
    // is still in "init" stage; configure()/setFunction() is what moves it out. Confirmed live:
    // selectStage("simulate") called before configure(), failing at runtime with exactly that
    // error, both documented worked examples show configure()/setFunction() called first.
    static const QRegularExpression selectStageRe(R"(\bselectStage\s*\(\s*(['"])([^'"]*)\1\s*\))");
    // Jnse.initFit() configures Fittable too: confirmed live (benchmark JNSE holdout, Codex) — Jnse.initFit(...) then
    // Fittable.fit() was blocked, the model added Fittable.configure(), which reset the q of every dataset to the
    // function's default, and the fit printed D = 0.01 instead of 1.20
    static const QRegularExpression functionSelectRe(R"(\b(?:configure|setFunction|initFit)\s*\()");
    QRegularExpressionMatch stageMatch;
    const QString calls = codeWithLiteralsBlanked(code); // a call inside a string literal is text, not a call
    QRegularExpressionMatchIterator it = selectStageRe.globalMatch(code);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        if (calls.at(m.capturedStart()).isSpace())
            continue;
        if (m.captured(2).compare("init", Qt::CaseInsensitive) != 0)
        {
            stageMatch = m;
            break;
        }
    }
    if (!stageMatch.hasMatch())
        return {};
    const QRegularExpressionMatch functionMatch = functionSelectRe.match(calls);
    if (functionMatch.hasMatch() && functionMatch.capturedStart() < stageMatch.capturedStart())
        return {};
    return "This script calls selectStage(\"" + stageMatch.captured(2) +
           "\") before "
           "configure()/setFunction() has selected a function — Fittable is still in its \"init\" "
           "stage at that point, and selectStage() raises RuntimeError (\"no function is set yet\") "
           "for any target besides \"init\" while there. Call configure(...) or "
           "setFunction(...) BEFORE selectStage(...).";
}

QList<AiKnownMistakeMatch> QtiSasAiContent::detectKnownMistakes(const QString &code, const QString &originalRequestText,
                                                                const QString &earlierPartsCode,
                                                                const QString &laterPartsDescriptions,
                                                                bool exploreScript) const
{
    // Dispatches EVERY pattern below unconditionally and collects ALL matches, in the SAME order
    // these were previously checked in as separate AiAssistant.cpp local variables/if-else branches
    // — one dispatch point replacing what used to be 25 separate single-purpose pure virtual
    // methods here. See AiContentProvider::detectKnownMistakes()'s doc comment for the full
    // "collect all, don't stop at first" rationale, and each private helper below for its own
    // "confirmed live" incident writeup.
    QList<AiKnownMistakeMatch> matches;
    QString explanation = compiledCodeSelfShadowBug(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"compiled-function defect detected", explanation};
    explanation = compilerRedeclaredReservedNameMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setCode() redeclares a pre-declared name", explanation};
    explanation = compilerYNameNeverAssignedMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setCode() never assigns the dependent variable", explanation};
    explanation = fitCurveNameReconstructionMismatch(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"fitCurve name mismatch detected", explanation};
    explanation = unrequestedAutoexecImmediateRun(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"unrequested immediate autoexec run detected", explanation};
    explanation = changeFolderNoOpMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"no-op changeFolder(activeFolder()) call detected", explanation};
    explanation = tableCellRowColSwapMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"Table cell row/col argument order likely swapped", explanation};
    explanation = compilerSetCodeNestedFunctionMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nested function definition inside setCode() detected", explanation};
    explanation = compilerFittableOnlyMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"Fittable-only method called on a Compiler object", explanation};
    explanation = setParamNamesMissingAllMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent setParamNames() detected", explanation};
    explanation = compilerNonexistentMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Compiler method detected", explanation};
    explanation = compilerSetCodeCppSyntaxMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"invalid C++ syntax inside setCode()/setIncludedFunctions() detected",
                                       explanation};
    explanation = compilerMissingAiNamespaceMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"compiled function missing the ai/ namespace prefix", explanation};
    explanation = zeroBasedTableIndexMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"0-based table/matrix index detected", explanation};
    explanation = hallucinatedPlotDesignationCaseMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Table.PlotDesignation member detected", explanation};
    explanation = staleFitStatisticMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"hardcoded chi2/dof detected", explanation};
    explanation = missingLegendCreationMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"legend created only conditionally", explanation};
    explanation = unannouncedModelNameSubstitution(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"unannounced model name substitution detected", explanation};
    explanation = discoveredNameInsteadOfRequestedMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"wildcard search used instead of the requested name", explanation};
    // Both of these assert "the requested action never happened" by requiring a WRITE call
    // (compile()/newTable()...) in `code` — unsatisfiable for an EXPLORE script, whose write calls
    // explorePrefixBeforeFirstWriteCall() has already cut away. See detectKnownMistakes()'s
    // exploreScript doc comment for the infinite loop this caused.
    if (!exploreScript)
    {
        explanation = reuseInsteadOfCompileMistake(code, originalRequestText, earlierPartsCode);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"requested compile never happened", explanation};
        explanation = columnFormulaInLoopMistake(code);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"a column formula set once per row", explanation};
        explanation = guardedCompileMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the compile is skipped when the function seems to exist", explanation};
        explanation = aiSandboxReopenMistake(code, originalRequestText, earlierPartsCode);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"a leftover ai/ function reopened instead of compiled fresh", explanation};
        explanation =
            requestedCreationNeverHappenedMistake(code, originalRequestText, earlierPartsCode, laterPartsDescriptions);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"requested table/matrix/graph/note creation never happened", explanation};
    }
    explanation = hardcodedWinnerWithoutLiveComparisonMistake(code, originalRequestText, earlierPartsCode);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"hardcoded winner model name with no live comparison", explanation};
    explanation = hallucinatedLegendMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Legend method detected", explanation};
    explanation = graphWindowLayerMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"GraphWindow/Graph layer method confusion detected", explanation};
    explanation = conditionalMaximizeWindowMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"maximizeWindow() conditionally gated behind graph-reuse check", explanation};
    explanation = curveExistenceGuardTitleMismatch(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"curve-existence guard can never match a real curve title", explanation};
    explanation = windowNameMethodCallMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setWindowName() called as a method", explanation};
    explanation = creationChainedMethodCallMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"method call chained onto a constructor's setter result", explanation};
    explanation = qti3DImportMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"nonexistent Qti3D import detected", explanation};
    explanation = setAutoexecOnScriptEditMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"setAutoexec() called on the wrong object", explanation};
    explanation = plotStyleStringArgumentMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"string passed to a plot-style setter", explanation};
    explanation = matrixViewTypeIntMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"bare int passed to Matrix.setViewType()", explanation};
    explanation = bareColumnNameInColumnsListMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"bare column name compared against columnsList()", explanation};
    explanation = missingPyQtImportMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"PyQt class used without a matching import", explanation};
    explanation = missingRandomImportMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"random module used without import", explanation};
    explanation = unrequestedRandomDataMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"random values the request never asked for", explanation};
    explanation = columnListInsteadOfAggregateMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a whole column printed instead of the requested value", explanation};
    // The result line belongs to the last part of a split plan: an earlier part cannot compute all of its values.
    // Confirmed live (LIST 5, item 4): part 1 of 2 printed "R4: done", the fixed-text check told it to compute the
    // values, one of which was part 2's 3D window, and the retry dropped the line altogether. An earlier part's
    // fixed-text line is removed by nonFinalResultLineRepair() instead; a computed one may stay (the last line counts).
    if (laterPartsDescriptions.trimmed().isEmpty())
    {
        explanation = hardcodedResultLineMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the result line is fixed text, not computed", explanation};
        explanation = repeatedResultLabelMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the result label printed more than once", explanation};
        explanation = labeledResultLineMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the result line labels its values", explanation};
        explanation = staleResultValueMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the result line prints a value read before the work", explanation};
        explanation = resultLineTooFewValuesMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"the result line carries fewer values than asked", explanation};
        explanation = placeholderResultValueMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"fixed text in place of a result value", explanation};
        explanation = noWorkFailureResultMistake(code, originalRequestText);
        if (!explanation.isEmpty())
            matches << AiKnownMistakeMatch{"only written-in values printed instead of the work", explanation};
    }
    explanation = rowRangeEndsOneShortMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a row loop that stops one row short", explanation};
    explanation = unusedParameterMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a declared parameter the code never uses", explanation};
    explanation = resultsAfterConfigureMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"results() read after configure() without a fit", explanation};
    explanation = earlierPartVariableMistake(code, earlierPartsCode);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a variable of an earlier part's script used", explanation};
    explanation = matrixIndexFormulaMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"matrix formula over indices i, j instead of x, y", explanation};
    explanation = newNoteExtraTabMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"one tab too many: a new note already has one", explanation};
    explanation = otherTabRunMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a tab run that the request does not run", explanation};
    explanation = bareObjectMethodMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a table/matrix method called without its object", explanation};
    explanation = existenceReadBeforeCreationMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a value read before the call that creates it", explanation};
    explanation = requestDataNumberMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"a number of the request's data formula missing", explanation};
    explanation = recalculateExceptionNameMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"the wrapping ValueError named instead of the formula's error", explanation};
    explanation = scriptPrintKeywordMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"scriptPrint() given end=", explanation};
    explanation = currentEditorAfterAddTabMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"currentEditor() after addTab() is the new tab", explanation};
    explanation = pythonInputCallMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"input() call would hang the application", explanation};
    explanation = guessedNameCandidateListMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"guessed a list of candidate names instead of using the known one", explanation};
    explanation = fragmentedStaticScriptPrintMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"static text split across one scriptPrint() call per line", explanation};
    explanation = doubleEscapedRawRegexMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"doubled backslash in raw regex string", explanation};
    explanation = singletonCalledAsConstructorMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"Compiler/Fittable/Dan called as a constructor", explanation};
    explanation = pyQt6FrameStyleComboMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"PyQt6 QFrame combo passed to setFrameStyle()", explanation};
    explanation = fittableParametersDirectiveIgnoredMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"fittable/unfittable parameters directive only partially applied", explanation};
    explanation = redundantFittableReconfigureMistake(code, earlierPartsCode);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"redundant Fittable.configure() discards earlier part's setup", explanation};
    explanation = fittableSelectStageBeforeFunctionMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"selectStage() called before configure()/setFunction()", explanation};
    explanation = fittableFitBeforeConfigureMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"fit() called before configure()/setFunction()", explanation};
    explanation = compilerScanGroupsAfterNameMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"scanGroups() clears the function name set before it", explanation};
    explanation = duplicateWindowNameMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"the same window created more than once", explanation};
    explanation = textIntoNumericColumnMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"text written into a Numeric column", explanation};
    explanation = matrixIntegrateAsSumMistake(code, originalRequestText);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"Matrix.integrate() used for a sum of cells", explanation};
    explanation = scriptEditPrintDialogMistake(code);
    if (!explanation.isEmpty())
        matches << AiKnownMistakeMatch{"print() on a note tab opens the printer dialog", explanation};
    return matches;
}

QString QtiSasAiContent::explorePrefixBeforeFirstWriteCall(const QString &code) const
{
    // Confirmed live: an EXPLORE-labeled script opened with a few harmless read-only lines
    // (Compiler.categories()/functionNames() filtering) then, in the SAME response, went on to
    // create a real table, compile two real fit functions, run two real fits, and build a real
    // graph — all executed for real, since nothing upstream distinguishes "labeled EXPLORE" from
    // "safe to run as EXPLORE." Deliberately narrow to TOP-LEVEL (unindented) matches only — cutting
    // inside an indented if/for block would leave a syntactically broken fragment behind; every
    // confirmed incident (and every EXPLORE example in ai-python-api.md) has these calls as flat
    // top-level statements, so this is not a real loss of coverage for the case that matters.
    //
    // Confirmed live, second incident, worse than the first: an EXPLORE script for a compile task
    // ran c.new(), setFunctionName(), setGroup(), setYName(), setXName(), setInfo(),
    // setParamCount(), setParamNamesAll(), setParamInitsAll(), setParamLimitsAll(),
    // setParamVariesAll(), setParamDescrsAll(), and setCode() — every one a genuine, documented
    // Compiler API call (verified against compile-python-api.md, not a hallucination) — and NONE of
    // them were in this list, so all of it executed for real, fully configuring a new function in
    // Compiler's live editing buffer, cut short only at the very last line, ok = c.compile(). The
    // buffer was left populated but never saved, and (compounding with the caller's own
    // action-script-marker shortcut — see handleExploreResponse()'s matching fix) the whole part
    // was then wrongly declared done without ever actually compiling anything. Added the
    // Compiler-authoring calls (and Fittable's configure(), the same kind of gap for the fit-setup
    // side) so the cut happens at c.new()/f.configure() itself, long before any real state gets
    // mutated.
    static const QRegularExpression writeCallRe(
        R"(^[ \t]*(?:\w+\s*=\s*)?(?:\w+\.)?(?:newTable|newMatrix|newNote|newGraph|newLegend|addCurve|)"
        R"(insertCurve|addErrorBars|maximizeWindow|closeGraph|setCell|setCellData|setText|)"
        R"(setColData|setRowData|appendRowData|setColName|setColNames|setColumnRole|deleteRows|)"
        R"(removeCol|insertColumns|recalculate|setCommand|normalize|setRandomValues|)"
        R"(setNormalRandomValues|setNumRows|setNumCols|importASCII|exportASCII|notifyChanges|)"
        R"(replot|compile|save|delete|fit|simulate|plotResults|simulateCurve|plot|new|)"
        R"(setFunctionName|setGroup|setYName|setXName|setInfo|setParamCount|setParamNamesAll|)"
        R"(setParamInitsAll|setParamLimitsAll|setParamVariesAll|setParamDescrsAll|setCode|)"
        R"(configure)\s*\()");
    // Indentation allowed. The old form required the call at column 0 (an explicit
    // !line.at(0).isSpace() guard), so any write call nested in if/else/for/try was invisible —
    // confirmed live: an EXPLORE bundled a complete configure+fit inside "else:"/"try:", nothing
    // truncated, and the whole fit ran under the # EXPLORE marker. Worse than the read-only
    // invariant alone: the "EXPLORE already ran the action script — done" shortcut then declared
    // the item finished, so self-review and all of detectKnownMistakes() were skipped entirely.
    //
    // Matched line-by-line against the string-literal-stripped code (that guard's real purpose),
    // so a quoted setText("... newTable ...") still cannot trip it. Offsets come from the ORIGINAL
    // lines, since the return value slices `code`; codeWithoutStringLiterals() replaces literals
    // in place without changing line count or line lengths.
    // An INDENTED match cuts back to the start of its enclosing top-level statement, not to the
    // matching line: cutting mid-block would leave "else:"/"try:" with an empty body, a
    // SyntaxError, and this prefix is EXECUTED. That was the original column-0 guard's real
    // concern — but refusing indented lines outright let the write run instead, which is worse.
    const QStringList lines = code.split('\n');
    const QStringList strippedLines = codeWithoutStringLiterals(code).split('\n');
    int offset = 0;
    int topLevelOffset = 0;
    for (int i = 0; i < lines.size(); ++i)
    {
        const QString &probe = i < strippedLines.size() ? strippedLines.at(i) : lines.at(i);
        const bool atTopLevel = !probe.isEmpty() && !probe.at(0).isSpace();
        if (!probe.trimmed().isEmpty() && writeCallRe.match(probe).hasMatch())
            return code.left(atTopLevel ? offset : topLevelOffset);
        if (atTopLevel)
            topLevelOffset = offset;
        offset += static_cast<int>(lines.at(i).length() + 1); // +1 for the '\n' split() consumed
    }
    return code;
}

QString QtiSasAiContent::retryHintForError(const QString &errContext) const
{
    // Confirmed live (LIST 5 item 2, 2026-10-11): best_A = results["A"] then abs(best_A) — "bad operand type for abs():
    // 'tuple'", the same script twice and the item given up, although fittable-python-api shows exactly this as WRONG.
    // A tuple where a number belongs is, in a fit script, results()["name"]: the (value, error) pair.
    // Also "unsupported format string passed to tuple.__format__" — the type named without quotes; confirmed live
    // (series 6, local, LIST 3 item 7: "{:.4f}".format(r_a) with r_a = f.results()["a"]): the hint never fired, the
    // same script came back and the item was given up.
    static const QRegularExpression tupleErrorRe(R"(TypeError:[^\n]*(?:'tuple'|\btuple\.__format__))");
    if (tupleErrorRe.match(errContext).hasMatch())
        return "REMINDER (this error): f.results()[\"name\"] is the (value, error) TUPLE, never a number — unpack it "
               "first: value, error = f.results()[\"name\"], then use value (abs(value), f\"{value:.4f}\").\n\n";

    if (const QString deletion = inventedDeletionCallHint(errContext); !deletion.isEmpty())
        return deletion;

    // Confirmed live (series 5, BlaBlaDor, LIST 3 item 2, two runs): setCommand(3, "x^3") recalculated with muParser —
    // "Unexpected token "x" found at position 0." on the error's second line — sent back unchanged until the item
    // was given up. table-python-api says it ("Bare names fail -- always col("x") or col(idx)"); the error does not.
    static const QRegularExpression bareColumnRe(R"RX(recalculate:[^\n]*\n\s*Unexpected token "([A-Za-z_]\w*)")RX");
    if (const QRegularExpressionMatch bare = bareColumnRe.match(errContext); bare.hasMatch())
        return "REMINDER (this error): a column formula refers to a column as col(\"" + bare.captured(1) +
               "\") or col(index), never by the bare name \"" + bare.captured(1) + "\" — e.g. \"col(1)^3\".\n\n";

    // Confirmed live (series 4, LIST 5 item 2, local): a plain formula compiled with setHHeaders('#include
    // "sasviewmodels/gaussian.h"') — a header that does not exist; the retries changed the body and kept the include.
    static const QRegularExpression missingHeaderRe(R"(fatal error: '([^']+)' file not found)");
    if (const QRegularExpressionMatch header = missingHeaderRe.match(errContext); header.hasMatch())
        return "REMINDER (this error): the compile failed at the #include of \"" + header.captured(1) +
               "\" — that header does not exist; the setCode() body is not the cause. A formula written with "
               "exp/sin/pow/sqrt needs NO setHHeaders() at all: remove the setHHeaders() call. Include a header only "
               "when the code calls a library function whose header exists.\n\n";

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
