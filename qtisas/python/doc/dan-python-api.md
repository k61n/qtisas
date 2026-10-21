# QtiSAS DAN-SANS Python API

Python interface of the **DAN-SANS** tool (the "DAN" dock): reduction of small-angle neutron scattering data. The raw
detector runs of an instrument become, step by step as in the dock's tabs: masks and a sensitivity matrix, the
**table of configurations** (one column per detector distance / wavelength: empty beam, dark current, absolute
calibration, beam center), a **script table** of sample runs, **I[Q] tables** (or ASCII files) per run, and finally
**merged curves**, one per sample, the configurations joined.
The global object is `Dan` — already defined, like `Fittable` and `Jnse`: use it without any import. Its parts mirror
the tabs: `Dan.Options` (with `Dan.Options.Config`), `Dan.Rawdata`, `Dan.Mask`, `Dan.Sensitivity`, `Dan.Processing`
(with `Dan.Processing.Options`) and `Dan.Merge`.

> **Use `Dan` for every SANS-reduction task** — raw runs of a SANS instrument, masks, detector sensitivity,
> absolute calibration, beam center, transmissions, radial averaging I[Q], merging detector distances or instruments.
> Do not read raw detector files with your own Python code.

**One session, several requests:** `newSession()` clears masks, configurations and script tables. Call it once,
when a request starts a session; the next steps of the same reduction continue in that session — use the masks and
tables the earlier requests made, never `newSession()` again.

**Runs** are given as in the file names ("53216", "00082558"; ints too) or as file paths. **Configurations** are
numbered from 1, like the columns "cond.-#1" of the table of configurations. **Files** (mask, sensitivity, runs) are
paths (`~/...` or absolute) or names in the input folder.

**Errors:** a call that makes nothing raises `ValueError` with DAN's reason (an unknown instrument, a missing run,
mask or file, ...). `lastMessages()` of each part returns DAN's warnings of its last call even when it worked — e.g.
`"C-D-Lambda condition # 2:: plexi-file does not exist!"` after `calculate()` means: no calibrant in that
configuration, its factor stays 1. Nothing opens a dialog.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax.

---

## Quick Start — the recipe

First choose the call:

| The request asks for … | call |
|---|---|
| the instruments, the modes | `Dan.Options.instruments()`, `Dan.Options.modes()` |
| the center or transmission method | `Dan.Options.Config.setCenterMethod("hf" \| "symmetry" \| "header")`, `setTransmissionMethod(...)` |
| the runs of the folder, a table of header values | `Dan.Rawdata.runs()`, `Dan.Rawdata.newHeaderTable(name, ["Sample", "D", ...])` |
| a beam-stop or transmission mask | `Dan.Mask.newMask(name, beamstop=(x1, x2, y1, y2), transmission=...)` |
| the transmission of one run (e.g. H2O) | `Dan.Sensitivity.transmission(run, emptyBeam, mask)` → `(Tr, dTr)` |
| remove points at the ends of the curves | `Dan.Processing.Options.setRemovePoints(first, last)` (I[Q]); `Dan.Merge.run(..., removeFirst=, removeLast=)` (merged) |
| ASCII files instead of tables | `Dan.Processing.run(s, output="files")`, `Dan.Merge.run(t, output="files")` |
| join curves of two instruments | `Dan.Merge.join(name, [table1, table2])`; how well they agree: `Dan.Merge.overlap(t1, t2)` |

The request's words for `setConfiguration(i, ...)` (i = the configuration's number; the runs as the request gives
them), then ONE `calculate()` and `configuration(i)["factor"]`, `["xCenter"]`, `["yCenter"]`:

| the request says … | keyword |
|---|---|
| empty beam / EB `<r>` | `emptyBeam="<r>"` |
| dark current / B4C `<r>` | `darkCurrent="<r>"` |
| Plexiglas / water / calibrant `<r>` | `calibrant="<r>"` |
| "with its empty beam `<r>`" | `calibrantEmptyBeam="<r>"` |
| direct-beam calibration with `<r>` | `calibrantEmptyBeam="<r>"` only (no calibrant) |
| center from `<r>` | `center="<r>"` |
| empty cell for Tr(EC → EB) `<r>` | `cellEmptyBeam="<r>"` |
| transmissions measured in it | `transmission=True` |

Then the reduction, in this order (the KWS-1 example of qtisas.com/dan-sans: 3 configurations, Plexiglas as
calibrant, centers from a strongly scattering sample, transmissions measured in configuration 2):

```python
o, p = Dan.Options, Dan.Processing
o.newSession(); o.selectInstrument("KWS1-2020"); o.setInputPath("~/kws1")    # 1. once, when a session starts
Dan.Mask.newMask()                                    # 2. "mask": the instrument's edge and beam stop; all masks
                                                      #    here (KWS-3: the beam-stop and transmission masks too)
Dan.Sensitivity.loadSensitivity("~/kws1/sens-20200117.sens")   # "sens" (or newSensitivity(...))
p.setConfigurations(3)                                # 3. one per detector distance / wavelength
for i, (eb, plexi, plexiEB, center) in enumerate((("53216", "53229", "53230", "53218"),
                                                  ("53220", "53224", "53225", "53222"),
                                                  ("53225", "53224", "53225", "53227")), 1):
    p.setConfiguration(i, emptyBeam=eb, darkCurrent="48462", calibrant=plexi, calibrantEmptyBeam=plexiEB,
                       calibrantDarkCurrent="48462", center=center, cellEmptyBeam=eb, transmission=(i == 2))
p.calculate()                                         # 4. C, D, lambda, factors, centers: p.configuration(i)
s = p.newScriptTable("script", [53217, 53218, 53219, 53221, 53222, 53223, 53226, 53227, 53228])   # 5.
p.calculateTransmissions(s)                           #    "Thickness" [cm]: from the headers, set it if missing
qi = p.run(s)                                         # 6. "QI-SM-<run>-<sample>" tables + "script-mergingTemplate"
merged = Dan.Merge.run(s + "-mergingTemplate")        # 7. ["H-J", "H-L", "H-M"]: the configurations joined
```

---

## Dan.Options

| method | |
|---|---|
| `newSession()` | a new, empty session |
| `openSession(scriptTable)` | a saved session — its script table and the settings saved with it |
| `instruments()` → list, `instrument()` → str | the instrument definitions; the selected one |
| `selectInstrument(name)` | sets every option of the instrument (detector, header, methods, mask default, ...) |
| `modes()` → list, `selectMode(mode)` | e.g. "(SM) Standard Mode", "(BS) Buffer Subtraction [2D]" |
| `setInputPath(path)`, `inputPath()` | the folder of the raw runs (`~/...` or absolute) |
| `setOutputPath(path)`, `outputPath()` | where `output="files"` writes (`ASCII-QI/`, `ASCII-QI-MERGED/`); a folder that does not exist yet is made |

### Dan.Options.Config — the instrument's methods

`selectInstrument()` sets them; set them after it.

| method | |
|---|---|
| `centerMethods()` → `["hf", "symmetry", "header"]`, `centerMethod()`, `setCenterMethod(method)` | **hf**: moments around the beam stop's center — for a sample scattering strongly near the beam stop; **symmetry**: X and Y symmetrization — stable for weakly scattering data; **header**: the center written in the run's header |
| `transmissionMethods()`, `transmissionMethod()`, `setTransmissionMethod(method)` | the texts of DAN's list, e.g. "Direct Beam  [dead-time +]" (counts through the transmission mask) |

## Dan.Rawdata

| method | |
|---|---|
| `runs()` → list | the run numbers of the input folder, sorted |
| `headerItems()` → list | the names `newHeaderTable()` takes: "Sample", "C", "D", "lambda", "Duration", "Thickness", ... |
| `newHeaderTable(name, items, runs="*")` → str | a table, one row per run: the first column `Runs` (the run number), then a column per item; `runs`: `"*"` or `"<first>..<last>"` |

## Dan.Mask

A box is `(x1, x2, y1, y2)` in detector pixels (1-based); `None` keeps what is set (after `selectInstrument()`: the
instrument's default), `0` means none.

| method | |
|---|---|
| `newMask(name="mask", edge=None, beamstop=None, shape=None, polygons=None, transmission=False)` → str | the detector edge and the beam stop masked (0); `shape`: `"rectangle"` or `"ellipse"` (the beam stop); `polygons`: triangles masked, `"x1-y1,x2-y2,x3-y3;..."`; `transmission=True`: the inverse, only the beam-stop box counts — a **transmission mask**. A mask of that name is made anew |
| `loadMask(file, name="mask")` → str | a mask file (`.mask`) |
| `masks()` → list | the masks of the project for this detector size |

Masks and sensitivities are matrices of the project: `matrix("mask").cell(row, col)` is 1 for a counted pixel and 0
for a masked one; `matrix("sens")` holds the sensitivity per pixel.

The values stay set: `newMask("mask-tr", transmission=True)` right after `newMask("mask-4m", beamstop=(57, 71, 58,
71))` takes the same box. The first Sensitivity or Processing call after the masks makes `"mask"` the active mask
again.

## Dan.Sensitivity

| method | |
|---|---|
| `newSensitivity(run, name="sens", emptyBeam="", darkCurrent="", transmission=1.0, mask="mask")` → str | from a flat scatterer (H2O, Plexiglas): its empty beam, dark current and transmission; from a provided pure-sensitivity run: only the run |
| `loadSensitivity(file, name="sens")` → str | a sensitivity file (`.sens`) |
| `sensitivities()` → list | |
| `transmission(run, emptyBeam, mask="mask")` → `(Tr, dTr)` | a transmission counted through a (transmission) mask — e.g. the H2O transmission for its sensitivity |

## Dan.Processing

Script tables, templates and curves are passed by **name** (the string `newScriptTable()` returned,
`"script-mergingTemplate"`, `"H-J"`), never as Table objects.

| method | |
|---|---|
| `setConfigurations(n)`, `configurations()` | the number of columns of the table of configurations |
| `setConfiguration(i, emptyBeam=, darkCurrent=, calibrant=, calibrantEmptyBeam=, calibrantDarkCurrent=, center=, mask=, sens=, transmissionMask=, cellEmptyBeam=, transmission=None)` | only the given fields change. `calibrant`: the flat scatterer of the absolute calibration (none: the direct beam, `calibrantEmptyBeam` and `calibrantDarkCurrent` only); `center`: a run scattering near the beam stop (or the empty beam); `transmission=True`: sample transmissions are measured in this configuration (copied to the rows of the same sample in the other configurations); `cellEmptyBeam`: the empty beam of Tr(EC → EB); `calibrantDarkCurrent`: the dark current of the calibration runs — default: the configuration's `darkCurrent` |
| `calculate()` | reads C, D, λ and the beam size from the headers; calculates μ, the calibrant's Tr, the absolute factor, the beam centers and Tr(EC → EB) — with each configuration's mask and sensitivity; without a calibrant (direct beam), the factor and the center are calculated through the full mask `"mask"`, since a beam-stop mask hides the beam; I[Q] and the transmissions use each configuration's own masks. Set the configurations' masks before `calculate()` |
| `configuration(i)` → dict | `emptyBeam`, `darkCurrent`, `C`, `D`, `lambda`, `beamSize`, `calibrant…`, `mu`, `factor`, `center`, `xCenter`, `yCenter`, `xCenterError`, `yCenterError`, `mask`, `sens`, `transmission`, `transmissionMask`, `cellEmptyBeam`, `cellTransmission` |
| `newScriptTable(name, runs)` → str | one row per run; DAN finds its configuration by C, D, λ, beam size, ... (`#-Condition`). It **makes** the script table — never `newTable()` for it |
| `calculateTransmissions(scriptTable)` | the `Transmission-Sample` column |
| `run(scriptTable, kind="I-Q", output="project")` → list | the tables or files made. `kind`: `"I-Q"` (radial average), `"I-x-y"`, `"dI-x-y"`, `"I-Qx"`, `"I-Qy"`, `"Sigma-x-y"`, `"Q-x-y"`, `"dQ-x-y"`, `"I-Polar"`. I-Q in the project also makes `"<script>-mergingTemplate"` (one row per sample) |

**The script table** — columns used by `run()`: `Run-info` (the sample name; rows of the same name are one sample),
`#-Run`, `#-Condition`, `#-BC`, `#-EC [EB]`, `Thickness` [cm], `Transmission-Sample`, `Factor`, `X-center`,
`Y-center`, `Mask`, `Sens`. Change them with the table API (`table(s).setText(col, row, ...)`). A column **`Scale`**
(added by the table API) multiplies a row's I[Q]: e.g. KWS-3 scales its 1.2 m data to the 9.2 m transmission of the
same sample, `Scale = Tr(1.2 m) / Tr(9.2 m)`.

Its cells are **text**: read them with `t.text(col, row)` and find columns by name
(`col = list(t.colNames()).index("Transmission-Sample") + 1`). `Transmission-Sample` is `"0.8981 [ ±0.0012 ]"`, so
the value is `float(t.text(col, row).split("[")[0])` — `t.cell()` gives 0.0 for such text. `#-Run` is the run number
as text ("53222"), `#-Condition` the configuration number.

### Dan.Processing.Options

| method | |
|---|---|
| `setRemovePoints(first, last)`, `removePoints()` → `(first, last)` | the I[Q] points removed at the beginning and the end of every curve |

## Dan.Merge

Works on tables of the project (the I[Q] tables of `Dan.Processing.run(s)`).

| method | |
|---|---|
| `run(templateTable, method="join", overlap=30, removeFirst=0, removeLast=0, output="project")` → list | one merged curve per template row, named after the sample. **join** keeps every point with its own σ (the overlap: the % of the overlap range where both are kept); **rebin** rebins onto one Q grid |
| `join(name, tables, overlap=30)` → str | any tables joined — e.g. a merged KWS-1 curve with a merged KWS-3 curve; no scaling (absolute units) |
| `overlap(first, second)` → `(qMin, qMax, points, ratio)` | where two curves overlap and the mean ratio I(second) / I(first) there — 1 for curves in the same absolute units |

---

## Examples

**KWS-3** — direct-beam calibration, a provided pure sensitivity, transmission masks, the 1.2 m data scaled:

```python
o, p = Dan.Options, Dan.Processing
o.newSession(); o.selectInstrument("KWS3-2020"); o.setInputPath("~/kws3")
Dan.Mask.newMask()                                    # the edge only
Dan.Sensitivity.newSensitivity("00000001")            # a pure sensitivity: no dark current subtracted
p.setConfigurations(2)
for i, eb in enumerate(("00082558", "00082645"), 1):  # direct beam: EB and B4C only, centers from the EB
    p.setConfiguration(i, emptyBeam=eb, darkCurrent="00000002", calibrantEmptyBeam=eb,
                       calibrantDarkCurrent="00000002", center=eb, cellEmptyBeam=eb)
p.calculate()
for i, (tag, bs, tr) in enumerate((("9m", (114, 126, 134, 147), (117, 123, 137, 144)),
                                   ("1m", (110, 131, 119, 142), (117, 123, 127, 135))), 1):
    Dan.Mask.newMask("mask-bs-" + tag, beamstop=bs)
    Dan.Mask.newMask("mask-tr-" + tag, beamstop=tr, transmission=True)
    p.setConfiguration(i, mask="mask-bs-" + tag, transmissionMask="mask-tr-" + tag, transmission=True)
s = p.newScriptTable("script", ["00082557", "00082559", "00082561", "00082646", "00082647", "00082648"])
p.calculateTransmissions(s)
# column "Scale": 1 for 9.2 m, Tr(1.2 m) / Tr(9.2 m) of the same sample for 1.2 m (table API)
p.Options.setRemovePoints(0, 5)
p.run(s)
Dan.Merge.run(s + "-mergingTemplate")
```

**Two instruments** (absolute units, no scaling):

```python
Dan.Merge.overlap("KWS3-H-J", "KWS1-H-J")             # (qMin, qMax, points, ratio)
Dan.Merge.join("H-J-all", ["KWS3-H-J", "KWS1-H-J"])
```
