# QtiSAS SVD Python API

Python interface of the **SVD** tool (the "SVD" dock): contrast variation. The same samples of N components are
measured as SANS curves in several H2O/D2O solvent mixtures — only the solvent's D2O fraction changes — and the
singular value decomposition separates the intensities into the **partial structure factors** S_ij of the
components (and of the solvent as component N+1).
The global object is `Svd` — already defined, like `Fittable` and `Ascii1d`: use it without any import
(`from Svd import Svd` raises ModuleNotFoundError).

> **Use `Svd` for every contrast-variation task** — whenever a request names SVD or a singular value
> decomposition of SANS data, contrast variation, partial structure factors, an H/D (D2O) series, match points, or
> planning which D2O fractions to measure. Do not solve the contrast equations with your own numpy or least squares:
> `Svd` builds the contrast matrix, drops the combinations the data cannot determine, propagates the errors and
> writes the result table.

**Units:** SLDs in **cm^-2** (of the order 1e10, e.g. D2O ≈ 6.4e10); D2O fractions from **0 to 1** (34% is 0.34). A
value in Å^-2 (6.4e-6) or a percentage is refused with the reason, never used.

`Svd`'s input is set only by its calls (or `readTable()`): the fields of the SVD dock are not read.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax;
> required parameters are always positional.

---

## Quick Start — the recipe

First choose the call:

| The request asks for … | call | needs |
|---|---|---|
| the S_ij of measured curves (a decomposition) | `setContrasts(...)` + `decompose()` | solvent, components, contrasts |
| the D2O fraction where a component is matched | `matchPoint(sldH2O, sldD2O)` | solvent |
| how many combinations a set of fractions determines | `rank(fractions)` | solvent, components |
| the errors a set of fractions would give, the better plan | `plan(fractions, partials, relError, background)` | solvent, components |
| the same decomposition again, or without some contrasts | `readTable(svdTable)` + `setContrasts(...)` + `decompose()` | an SVD table |

Another set of contrasts is **always a new `decompose()`**: an SVD table holds only its own contrasts — never read a
value of another decomposition. To repeat one without some contrasts:

```python
s = Svd
s.readTable("<svd table>")                                 # its solvent, components and contrasts
s.setContrasts([c for c in s.contrasts() if c[0] != "<table>"])
name = s.decompose("<result name>")
value = s.value(name, "<column>", <Q>)
```

Then every task, in this order:

```python
s = Svd
s.reset()                                                  # 1. always first: the input stays set between calls
s.setSolvent(<sld of H2O>, <sld of D2O>)                   # 2. the solvent, cm^-2
s.setComponents([("<component>", <sld in H2O>, <sld in D2O>),   # 3. one tuple per component, cm^-2
                 ("<component>", <sld in H2O>, <sld in D2O>)])
s.setContrasts([("<table>", <D2O fraction>),               # 4. one tuple per measured solvent
                ("<table>", <D2O fraction>)])
name = s.decompose("<result name>")                        # 5. a name only when the request names it, else
                                                           #    s.decompose(); use the name it returns
S12 = s.value(name, "S12", <Q>)                            # 6. read any value of an SVD table this way: label + Q
```

The values in `< >` are the request's. Read the SVD table with `value(svdTable, column, q)` — by column label and
Q, never by row or column number (column 1 is `Comment`, not Q). The number of Q points is `len(s.qValues(name))`,
the number of contrasts used `len(s.contrasts())`.

To plot partial structure factors against Q with their error bars (X is `Q1`, a curve is named `<table>_<column>`;
`QColor` comes from `PyQt6.QtGui`):

```python
t = table(name)
g = newGraph("<graph>").activeLayer()
g.insertCurve(t, "Q1", "S11", Graph.CurveType.LineSymbols)
g.addErrorBars(name + "_S11", t, "dS11", 1, 1, 8, QColor("gray"), False, True, True)
```

---

## Input

| Method | Description |
|---|---|
| `reset()` | clear the solvent, components and contrasts |
| `setSolvent(sldH2Ocm2, sldD2Ocm2)` | the SLDs of H2O and D2O |
| `setComponents([(name, sldH2Ocm2, sldD2Ocm2), ...])` | one tuple per component: its SLD in H2O and in D2O (labile H exchanging with the solvent; equal values: no exchange). Its SLD in a mixture is h + φ(d − h) |
| `setContrasts([(table, d2oFraction), ...])` | one tuple per measured solvent: a table whose columns 1, 2, 3 are Q, I, dI — by position, the column names do not matter; without a dI column the errors are 0 — with the same Q points in all tables, and its D2O fraction. Tables of `Ascii1d.importFiles()` fit as they are |
| `setContrasts([(table, d2oFraction, solventSldCm2), ...])` | the same with a measured solvent SLD instead of H2O + φ(D2O − H2O) |
| `readTable(svdTable)` | the solvent, components and contrasts of an SVD table made before |
| `contrasts()` | `[(table, d2oFraction, solventSldCm2), ...]` as set — `setContrasts()` takes it back as it is, the measured solvent SLDs included |
| `components()` | the component names as set |

## The decomposition

| Method | Returns | Description |
|---|---|---|
| `decompose(name="", qPoints=0)` | `str` | the SVD table it made — use the returned name. Default name "svd"; a taken name gets a number (`svdAB` → `svdAB1`). `qPoints` 0: the rows of the shortest data table |
| `value(svdTable, column, q)` | `float` | the value of a column (`"S12"`, `"dS11"`, `"Icon3"`, `"I3"`) at the Q point q (relative 1e-6) of an SVD table; only the columns of Q points (`Qk`, `Ik`, `dIk`, `Iconk`, `Sij`, `dSij`) — the input columns are refused; `ValueError` names the nearest Q or the columns |
| `qValues(svdTable)` | `list` | the Q points of an SVD table (its `Q1`); `len()` is the number of Q points |

`decompose()` makes nothing and raises `ValueError` with the reason when the input cannot be decomposed: fewer than
N(N+1)/2 contrasts (2 components: 3), a missing table, a table with fewer Q points than asked. Warnings are printed
and listed in `lastMessages()`: Q points that differ between the tables, and a series that determines only some
combinations of the S_ij.

**The SVD table**, one row per Q point. Contrast k (in `setContrasts()` order) has the columns `Qk`, `Ik`, `dIk`
and `Iconk`: the reconstructed / measured intensity of the k-th contrast is `Iconk / Ik` in the same row — read it,
never rebuild it from the SLD columns. The k of a table is its position in `setContrasts()`, which is the row
of its name in the `TableName` column: mix46 given third →
`value("<svd table>", "Icon3", q) / value("<svd table>", "I3", q)`, never by row or column number.

**Rows mean two things:** in the `Q…`, `I…`, `dI…`, `S…`, `dS…` and `Icon…` columns a row is a Q point; in
`TableName`, `HDcomposition`, `SLDsolvent` and `SLD<name>` a row is a contrast. A value at a Q is never read in the
row of a table's name.

| Columns | Content |
|---|---|
| `Q1, I1, dI1, ..., QM, IM, dIM` | the data of contrast 1..M, in `setContrasts()` order |
| `S11, dS11, S12, dS12, ..., S22, ...` | the partial structure factors of the components (i ≤ j, row by row) and their errors (propagated from dI) |
| `S1(N+1), ..., S(N+1)(N+1)` with errors | the solvent as component N+1 — 2 components: `S13, S23, S33` (S13 = −S11 − S12, S23 = −S22 − S12, S33 = S11 + S22 + 2 S12); 3 components: `S14, S24, S34, S44` |
| `Icon1 ... IconM` | the intensity of each contrast rebuilt from the S_ij |
| `Comment` | row 1 "Solvent SLD", rows 2..N+1 the component names |
| `hSLDcomponent, dSLDcomponent` | row 1 the SLDs of H2O and D2O, rows 2..N+1 each component's SLD in H2O and in D2O |
| `TableName, HDcomposition, SLDsolvent, SLD<name>` | per contrast (rows 1..M): the data table, the D2O fraction, the solvent SLD, each component's SLD (`SLD<number>` for an empty or repeated name) |
| `a11, a12, ...`, `u11, ...` | per contrast (rows 1..M): the contrast matrix A and the U of its SVD |
| `V11, ...`, `S` | per combination (rows 1..N(N+1)/2): the V of the SVD and the singular values |

## Planning a series

| Method | Returns | Description |
|---|---|---|
| `matchPoint(sldH2Ocm2, sldD2Ocm2)` | `float` | the D2O fraction of zero contrast of a component; outside 0..1 printed: no mixture matches it |
| `rank(d2oFractions)` | `int` | how many combinations of the N(N+1)/2 S_ij the fractions determine |
| `plan(d2oFractions, partials, relError=0.01, background=0)` | `list` | the expected relative errors dS/S of S11, S12, ..., S1N, S22, ... (i ≤ j, the order of the SVD table) for intensity errors dI = relError·I + background·I(D2O); `partials` = **all** N(N+1)/2 expected S_ij at one Q in that order — 2 components: `[s.value("<svd table>", "S11", q), s.value("<svd table>", "S12", q), s.value("<svd table>", "S22", q)]`; `inf` = not determined (or the given S_ij is 0). The smaller, the better the plan |
| `lastMessages()` | `list` | the warnings of the last call |

One H/D series determines **at most 3** combinations, whatever the number of fractions: every intensity is a
parabola in the D2O fraction. So 2 components (3 unknowns) need at least 3 different fractions, and 3 or more
components are never fully determined by an H/D series alone; a repeated fraction adds no combination. More
fractions than needed reduce the errors.

Whether a plan is good depends on the intensity errors: with relative errors only (background 0), fractions near the
match points look best; a background (incoherent scattering) makes them the worst — give `plan()` the errors the
request assumes.
