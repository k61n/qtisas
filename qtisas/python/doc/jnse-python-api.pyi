# QtiSAS JNSE — neutron spin echo (J-NSE) data: .dtr files -> a header table (one row per q) + one dataset table
#   per q (S(q,t)/S(q) vs Fourier time tau), and the multi-q fit of the datasets in Fittable; global object Jnse
# Jnse is GLOBAL like Fittable/Ascii1d — NO import (from Jnse import ... raises ModuleNotFoundError)
# USE Jnse for: NSE / neutron spin echo / J-NSE data, .dtr files, S(q,t)/S(q), the intermediate scattering function,
#   Fourier times, relaxation/diffusion fits over several q — never own open()/split() reading of a .dtr file
# UNITS of the files: as the header's q-unit / t-unit say (1e10: q in 1/A, 1e-09: tau in ns); then
#   simpleDiffusion's D is in A^2/ns and a rate D q^2 in 1/ns
# THE RECIPE — <...> = the request's values:
#   j = Jnse
#   j.reset()                                              # 1. ALWAYS first: the input folder stays set
#   j.setInputFolder("~/<folder>")                         # 2. "~/..." or absolute
#   info = j.importFiles("<file>.dtr")                   # 3. or a list / "*.dtr"; a name only when the request
#                                                          #    names the header table: importFiles(files, "<name>")
#   q = j.header(info, "q")                              # 4. a header column, row by row (numbers or texts)
#   sets = j.datasets(info)                              #    the dataset table of each row, in the same order
#   chi2 = j.fit(info, "<function>", <first row>, <last row>, ["<shared>", ...], {"<p>": <start>}, {"<p>": <fixed>})
#   D = Fittable.paramValue("D")                           # 5. results: Fittable (shared: one value; else per row)
# CHOOSE THE CALL:
#   read .dtr files                     -> importFiles(files, headerTable="")   (the header table's name returned)
#   a value of every q (q, temp, ...)    -> header(headerTable, column)
#   the data of one q                    -> table(datasets(headerTable)[row - 1]) — columns below, by label
#   fit some/all q together, in one call -> fit(...)        (its results also go to the results log)
#   set up the fit, then tune Fittable   -> initFit(...), Fittable.setParam...(), Fittable.fit()
#     NEVER Fittable.configure() after initFit(): it sets Fittable up anew and resets each row's fixed q
#   the fit function is missing          -> compile it (Compiler) — e.g. simpleDiffusion of qtisas.com/jnse
reset()
setInputFolder(folder)  inputFolder()->str

importFiles(files, headerTable="")->str   # files: a name, a list, or wildcards ("~/d/*.dtr"), relative to
#   inputFolder(); the header table it made — default "<first file read>-info" (report19971_1.2.dtr ->
#   report19971_1-2-info; files that held nothing do not count); a header table of that name is APPENDED to; a
#   non-header table of that name is refused;
#   nothing read -> ValueError with the reason; a missing file is printed and skipped, a file that does not fit
#   the format is read up to that line (printed)
# THE HEADER TABLE, one row per dataset (= per q, in file order; a second file appends rows):
#   sample-code, sample-info, mode, run-number (texts), q (1/A), dq, q-unit, t-unit, temp, numor, ref, bgr,
#   with-bgr, tfac-bgr, volfrac, qbins, tbins, oor-cnt, upd-cnt (numbers), table-name (its dataset table)
# A DATASET TABLE (hidden, one per row, named "<file>-<run>-v-<n>"; every import makes new ones, n counting up —
#   so ALWAYS take the names from datasets(headerTable), never build or reuse them), one row per Fourier time:
#   tau-ns [X], Sqt-To-Sq [Y] = S(q,t)/S(q), d-Sqt-To-Sq [yErr], var-Sqt-To-Sq, var-tau-ns, nupdates, Qeff
#   its Fittable dataset is "<dataset table>_Sqt-To-Sq"
header(headerTable, column)->list   # a column, row by row: numbers for a number column, texts otherwise
datasets(headerTable)->list         # the dataset tables, row by row (row k -> datasets(...)[k - 1])
# the rows of one series (a temperature, a file, a sample) from a header column, then fit them:
#   rows = [k + 1 for k, T in enumerate(j.header(info, "temp")) if T == <temperature>]
#   j.fit(info, "<function>", rows[0], rows[-1], ...)       # the rows of one import are consecutive
# a q the request gives is ROUNDED ("q = 0.0705"): take the NEAREST row, never test equality:
#   q = j.header(info, "q"); row = 1 + min(range(len(q)), key=lambda k: abs(q[k] - <q>))
# Fittable holds only the LAST fit: read each fit's parameters right after it (Fittable.paramValue) — to compare
#   two series, fit one, read, fit the other, read; never parse fitCurve tables for that
# plot S(q,t)/S(q) vs tau with error bars: t = table(name); g = newGraph("<graph>").activeLayer()
#   g.insertCurve(t, "tau-ns", "Sqt-To-Sq", Graph.CurveType.LineSymbols)
#   g.addErrorBars(name + "_Sqt-To-Sq", t, "d-Sqt-To-Sq", 1, 1, 8, QColor("gray"), False, True, True)  # PyQt6.QtGui

initFit(headerTable, function, fromRow=1, toRow=0)->list   # Fittable set up for a multi-q fit of the rows
#   fromRow..toRow (1-based; toRow 0 = the last): one dataset per row, the function's parameter q FIXED at each row's
#   q; returns the Fittable datasets. ValueError: no such function (compile it), no parameter q, rows outside
fit(headerTable, function, fromRow=1, toRow=0, shared=[], start={}, fixed={})->float   # initFit + the fit:
#   shared: parameters fitted in common for all rows (e.g. ["A", "D"]); start: start values {name: value};
#   fixed: parameters held at a value {name: value}; returns chi2/dof (already reduced), and writes the fitted
#   parameters to the results log. Read them with Fittable.paramValue(name) (shared) or paramValue(name, k) (row k
#   of the fit, 1-based within fromRow..toRow)
lastMessages()->list
