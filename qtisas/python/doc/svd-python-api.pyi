# QtiSAS SVD — contrast variation: partial structure factors S_ij of N components from SANS curves measured in
#   H2O/D2O solvent mixtures (same samples, only the solvent's D2O fraction changes); global object Svd
# Svd is GLOBAL like Fittable/Ascii1d — NO import (from Svd import ... raises ModuleNotFoundError)
# USE Svd for: SVD / singular value decomposition of SANS data, contrast variation, partial structure factors,
#   an H/D (D2O) series, match points, planning which D2O fractions to measure — never own numpy/least squares
# UNITS: SLDs in cm^-2 (order 1e10, e.g. D2O ~6.4e10); D2O fractions 0..1 (34% is 0.34) — other units are refused
# Svd's input is set only by these calls (or readTable) — the SVD dock's fields are not read
# THE RECIPE — <...> = the request's values:
#   s = Svd
#   s.reset()                                                # 1. ALWAYS first: the input stays set between calls
#   s.setSolvent(<sld H2O>, <sld D2O>)                       # 2. the solvent
#   s.setComponents([("<component>", <sld in H2O>, <sld in D2O>), ...])   # 3. one tuple per component, cm^-2
#   s.setContrasts([("<table>", <D2O fraction>), ...])       # 4. one tuple per measured solvent
#   name = s.decompose("<result name>")                      # 5. a name only when the request names the result,
#                                                            #    else s.decompose(); use the name it returns
#   S12 = s.value(name, "S12", <Q>)                          # 6. read ANY value of an SVD table at a Q this way:
#                                                            #    column label + Q (column 1 is Comment, not Q)
#   len(s.qValues(name))  = the number of Q points;  len(s.contrasts()) = the number of contrasts used
#   plot S_ij vs Q with error bars: t = table(name); g = newGraph("<graph>").activeLayer()
#     g.insertCurve(t, "Q1", "S11", Graph.CurveType.LineSymbols)
#     g.addErrorBars(name + "_S11", t, "dS11", 1, 1, 8, QColor("gray"), False, True, True)  # QColor: PyQt6.QtGui
# CHOOSE THE CALL:
#   decompose S_ij from measured curves           -> setContrasts(...) + decompose()
#   the D2O fraction where a component is matched -> matchPoint(sldH2O, sldD2O)        (needs only setSolvent)
#   how many combinations a set of fractions fixes -> rank(fractions)       (needs setSolvent + setComponents)
#   the errors a set of fractions would give       -> plan(fractions, partials, relError, background)
#   the same decomposition again / without some contrasts -> s.readTable("<svd table>");
#     s.setContrasts([c for c in s.contrasts() if c[0] != "<table>"]); s.decompose("<result name>")
#   another set of contrasts is ALWAYS a new decompose(): an SVD table holds only its own contrasts —
#     never read a value of another decomposition
reset()

setSolvent(sldH2Ocm2, sldD2Ocm2)
setComponents([(name, sldH2Ocm2, sldD2Ocm2), ...])   # a component's SLD in H2O and in D2O (H/D exchange; equal
#   values: no exchange); its SLD in a mixture is h + phi (d - h)
setContrasts([(table, d2oFraction), ...])            # tables whose columns 1, 2, 3 are Q, I, dI (by position; the
#   names do not matter; no dI column: errors 0), the same Q points in all; Ascii1d.importFiles() tables fit as they are
#   (table, d2oFraction, solventSldCm2): a measured solvent SLD instead of H2O + phi (D2O - H2O)

decompose(name="", qPoints=0)->str   # the SVD table it made — use the returned name ("svd" by default; a taken
#   name gets a number: svdAB -> svdAB1); qPoints 0 = the rows of the shortest data table
#   makes nothing -> ValueError with the reason (fewer than N(N+1)/2 contrasts, a table missing or too short);
#   warnings are printed (Q points that differ; only r of n combinations of S_ij determined)
value(svdTable, column, q)->float   # the value of column ("S12", "dS11", "Icon3", "I3") at the Q point q
#   (relative 1e-6) of an SVD table — no row or column numbers; ValueError names the nearest Q or the columns;
#   only the columns of Q points (Qk, Ik, dIk, Iconk, Sij, dSij) — the input columns are refused
qValues(svdTable)->list   # the Q points of an SVD table (its Q1); len() = the number of Q points
# THE SVD TABLE, one row per Q point (row k = the k-th Q of the data tables):
#   Q1, I1, dI1, ..., QM, IM, dIM   the data of contrast 1..M (in setContrasts() order)
#   contrast k (setContrasts() order) -> columns Qk, Ik, dIk and Iconk; the reconstructed / measured intensity
#     of the k-th contrast = Iconk / Ik in the same row — read it, never rebuild it from the SLD columns
#   k of a table = its position in setContrasts() = the row of its name in TableName (mix46 given third ->
#     s.value("<svd table>", "Icon3", q) / s.value("<svd table>", "I3", q)); never by row or column number
#   ROWS MEAN TWO THINGS: in Q.., I.., dI.., S.., dS.., Icon.. a row is a Q point; in TableName, HDcomposition,
#     SLDsolvent, SLD<name> a row is a contrast — a value at Q is never read in the row of a table's name
#   S11, dS11, S12, dS12, ..., S22, ...  the partial structure factors (i <= j, row by row) and their errors
#                                   (propagated from dI)
#   then the solvent as component N+1: S1(N+1) ... S(N+1)(N+1) with errors — 2 components: S13, S23, S33
#     (S_i3 = -sum_j S_ij, S33 = S11 + S22 + 2 S12); 3 components: S14, S24, S34, S44
#   Icon1..IconM                    the intensity of each contrast rebuilt from the S_ij
#   the input, by rows: Comment (row 1 "Solvent SLD", rows 2..N+1 the component names), hSLDcomponent and
#     dSLDcomponent (row 1 the H2O and D2O SLDs, rows 2..N+1 the components' SLD in H2O and in D2O)
#   per contrast (rows 1..M): TableName, HDcomposition, SLDsolvent, SLD<name> (SLD<number> for an empty or
#     repeated name), the contrast matrix a.. and U (u..)
#   per combination (rows 1..N(N+1)/2): V.. and the singular values S
readTable(svdTable)   # the solvent, components and contrasts of an SVD table made before
contrasts()->list     # [(table, d2oFraction, solventSldCm2), ...] as set — setContrasts() takes it back as it is
components()->list    # the component names as set

matchPoint(sldH2Ocm2, sldD2Ocm2)->float   # D2O fraction of zero contrast; outside 0..1 printed: never matched
rank(d2oFractions)->int   # combinations of the N(N+1)/2 S_ij the fractions determine; one H/D series: at most 3
#   (every intensity is a parabola in the D2O fraction), so N >= 3 is never fully determined; a repeated
#   fraction adds no combination
plan(d2oFractions, partials, relError=0.01, background=0)->list   # expected dS/S of S11, S12, ..., S1N, S22, ...
#   (i <= j, the order of the SVD table) for intensity errors dI = relError*I + background*I(D2O); partials = ALL
#   N(N+1)/2 expected S_ij at one Q, in that order — 2 components:
#   [s.value("<svd table>", "S11", q), s.value("<svd table>", "S12", q), s.value("<svd table>", "S22", q)];
#   inf = not determined (or the given S_ij is 0); the smaller, the better the plan
lastMessages()->list
