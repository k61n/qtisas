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
| select the PN correction mode (two or four analyzed states) | `Dan.Options.selectMode("(PN) Polarized Neutrons")`, then `Dan.Options.selectPolarizationReductionMode("x421")` (or `"x422"`, `"x423"`, `"x424"`, `"x4"`); the PN reduction itself needs the polarized script table, which has no Python call yet: run it from the interface |
| the center or transmission method | `Dan.Options.Config.setCenterMethod("hf" \| "symmetry" \| "header")`, `setTransmissionMethod(...)` |
| the runs of the folder, a table of header values | `Dan.Rawdata.runs()`, `Dan.Rawdata.newHeaderTable(name, ["Sample", "D", ...])` |
| add the neighbouring runs of the same sample and configuration (all measured one after the other) | `for g in Dan.Rawdata.neighbours(): Dan.Rawdata.addRuns(g["runs"])` |
| add N neighbouring runs (the request gives N) | `for g in Dan.Rawdata.neighbours(N): Dan.Rawdata.addRuns(g["runs"])` |
| add ALL runs of the same sample and configuration, also not measured one after the other | `for g in Dan.Rawdata.sampleGroups(): Dan.Rawdata.addRuns(g["runs"])`; to look at or edit them first: `t = Dan.Rawdata.newAddingTemplate()`, then `Dan.Rawdata.addRunsFromTable(t)` |
| change a value in runs' header files (e.g. a wrong polarization, thickness or sample name) | `Dan.Rawdata.setHeaderValue(Dan.Rawdata.runs(), "Polarization", "0")`; "all samples", "all files" or "the folder" is `Dan.Rawdata.runs()`, every run (`sampleRuns()` only when the request leaves the standard runs out); the keys: `Dan.Rawdata.headerKeys()`, a run's value: `Dan.Rawdata.headerValue(run, key)`; check after it that no run has the old value |
| a run's file name, the files of runs (to move or copy them) | `Dan.Rawdata.files(runs)`; the runs an added run was made of: `Dan.Rawdata.info([run])[0]["added"]` |
| a beam-stop or transmission mask | `Dan.Mask.newMask(name, beamstop=(x1, x2, y1, y2), transmission=...)` |
| a sector, rectangle or ellipse in a mask | `Dan.Mask.sector(...)`, `rectangle(...)`, `ellipse(...)` on the mask's matrix |
| the transmission of one run (e.g. H2O) | `Dan.Sensitivity.transmission(run, emptyBeam, mask)` → `(Tr, dTr)` |
| remove points at the ends of the curves | `Dan.Processing.Options.setRemovePoints(first, last)` (I[Q]); `Dan.Merge.join(..., removeFirst=, removeLast=)` (merged) |
| ASCII files instead of tables | `Dan.Processing.process(s, output="files")`, `Dan.Merge.join(t, output="files")` |
| the columns of the I[Q] curves (σ, dQ, anisotropy) | `Dan.Processing.Options.setCurveFormat(columns="dq-sigma", anisotropy=True)` |
| 2D data as Qx-Qy (SasView) or polar ASCII files | `Dan.Processing.Options.setMatrixFormat("qxqy-resolution", header=True, sasview=True)`, then `process(s, "I-x-y", "files")` |
| Guinier, Zimm, Porod, Kratky … curves | `Dan.Processing.Options.setPresentation("Guinier")` before `process(s)`; back: `setPresentation("QI")` |
| smoothing, wide-angle corrections, normalization, averaging method | `Dan.Processing.Options.setSmoothing(…)`, `setWideAngle(…)`, `setNormalization(…)`, `setRadialAveraging(…)` |
| join curves of two instruments | `Dan.Merge.joinTables(name, [table1, table2])`; how well they agree: `Dan.Merge.overlap(t1, t2)` |
| any instrument setting, e.g. points always removed | `Dan.Options.Config.setParameters({...})`; keep it: `Dan.Options.saveInstrument("<name>")` |
| the Q resolution: focusing, lenses or mirror, round apertures, detector resolution | `Dan.Options.Config.setParameters({"Resolusion-Focusing": "Yes", "Resolusion-Focusing-Type": "mirror", "Resolusion-CA-Round": "No", "Resolusion-SA-Round": "No", "Resolusion-Detector": "0.08"})` (the detector resolution in cm; the chromatic term of the σ only for `"lenses"`) |

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
p.calculateTransmissions(s)                           #    "Thickness" [cm]: from the headers (none or 0: 0.1)
qi = p.process(s)                                     # 6. "QI-SM-<run>-<sample>" tables + "script-mergingTemplate"
merged = Dan.Merge.join(s + "-mergingTemplate")       # 7. ["H-J", "H-L", "H-M"]: the configurations joined
```

## Auto mode — a whole folder without run numbers

For "reduce the data of `<instrument>` in `<folder>`", the runs' roles (empty beams, dark current, calibrant, centers,
sensitivity, samples) come from the instrument's **auto options** (Options > Auto-Mode, saved with the instrument):

```python
o, p = Dan.Options, Dan.Processing
o.newSession(); o.selectInstrument("KWS1-2020"); o.setInputPath("~/kws1")
s = p.autoConfig()                # masks, sensitivity, configurations, script table, Tr: ready (Scale,
                                  # HShift/VShift too when their auto options are on)
qi = p.process(s)                 # what the request asks: kind="I-Q" (default), "I-x-y", "I-Polar", ...;
                                  # output="project" (default) or "files"
merged = Dan.Merge.join(s + "-mergingTemplate")   # only when merging is asked: join, not smart, nothing removed
```

Processing and merging are lines of their own, with the options the request names; without them I[Q] into the
project, and the merge joins the curves as they are. With one configuration there is nothing to merge.
`autoConfig()` step by step: `Dan.Mask.autoMasks()`, `Dan.Sensitivity.autoSensitivity()`, `p.autoConfigurations()`,
`p.calculate()`, `s = p.newScriptTable("script", Dan.Rawdata.sampleRuns())`, `p.calculateTransmissions(s)`,
`p.autoScale(s)` (option `scale`), `p.addCenterOffsets(s, "mask", "sens")` (option `centerOffsets`). Its steps (`DAN auto: …`: masks, sensitivity, the runs of each configuration, factors, centers,
Scale) are printed to the terminal QtiSAS runs in; the configurations left out (no center run, skipped) are in
`p.lastMessages()`. Change a rule for this session with
`Dan.Options.Config.setAutoOptions({...})` before them.

---

## Dan.Options

| method | |
|---|---|
| `newSession()` | a new, empty session |
| `openSession(scriptTable)` | a saved session — its script table and the settings saved with it |
| `instruments()` → list, `instrument()` → str | the instrument definitions; the selected one |
| `selectInstrument(name)` | sets every option of the instrument (detector, header, methods, mask default, ...) |
| `modes()` → list, `selectMode(mode)` | e.g. "(SM) Standard Mode", "(BS) Buffer Subtraction [2D]" |
| `polarizationReductionModes()` → list, `polarizationReductionMode()` → str, `selectPolarizationReductionMode(mode)` | In PN mode, choose `"none"`, `"x2"`, `"x4"`, or `"x421"`–`"x424"`; the current selection is returned as one of these short names |
| `setInputPath(path)`, `inputPath()` | the folder of the raw runs (`~/...` or absolute); sets the output folder to its parent, as the interface does — `setOutputPath()` after it |
| `setOutputPath(path)`, `outputPath()` | where `output="files"` writes (`ASCII-QI/`, `ASCII-QI-MERGED/`); a folder that does not exist yet is made |
| `saveInstrument(name)` | the current instrument with its changes (e.g. `Config.setParameters()`) under a name of its own, then selected; a built-in name is refused |

For polarized-neutron reduction, select the overall PN mode first, then its correction mode. The eight polarization-header aliases are instrument parameters `POL-ALIAS-UP`, `POL-ALIAS-DOWN`, `POL-ALIAS-UP-UP`, `POL-ALIAS-UP-DOWN`, `POL-ALIAS-DOWN-DOWN`, `POL-ALIAS-DOWN-UP`, `POL-ALIAS-OUT-UP`, and `POL-ALIAS-OUT-DOWN`, configurable through `Dan.Options.Config.setParameters({...})`. Alias patterns are whole-value wildcards (`*`, `?`, `[...]`), case and spaces ignored; separate alternatives with `||`. The PN states, as *Polarization-Aliased* names them (first polarizer, then analyzer), with their numbers: `out` 0, `up` 1, `down` 2, `up-up` 3, `up-down` 4, `down-down` 5, `down-up` 6, `out-up` −1, `out-down` −2. `out-up`/`out-down` are runs with polarizer out and analyzer in: only the mode without corrections reduces them (as sample/(T·τ_A)); their transmission equals the sample's when the transmission does not depend on the spin, and a calibrant measured this way is valid for the absolute factor. `Dan.Rawdata.info()[i]["polarization"]` reports the raw header value, while DAN's *Polarization-Aliased* header item shows the alias. In PN mode, configuration grouping uses aliases when polarization is a condition; outside PN mode it keeps raw header values. The two-file modes return spin projections, not four corrected cross sections: x421/x423 analyzer-weighted, x422/x424 incident-weighted.

### Dan.Options.Config — the instrument's methods

`selectInstrument()` sets them; set them after it.

| method | |
|---|---|
| `centerMethods()` → `["hf", "symmetry", "header"]`, `centerMethod()`, `setCenterMethod(method)` | **hf**: moments around the beam stop's center — for a sample scattering strongly near the beam stop; **symmetry**: X and Y symmetrization — stable for weakly scattering data; **header**: the center written in the run's header |
| `transmissionMethods()`, `transmissionMethod()`, `setTransmissionMethod(method)` | the texts of DAN's list, e.g. "Direct Beam [dead-time +]" (counts through the transmission mask) |
| `parameters()` → dict, `setParameters({key: value, ...})` | the instrument's whole configuration as its `.SANS` file holds it: `"Options-1D-RemoveLast": "5"`, `"Options-2D-CenterMethod": "SYM"`, ... (the keys: `parameters()`); set as selecting the instrument does — a value the interface does not take raises `ValueError` |
| `autoOptions()` → dict, `setAutoOptions({name: value, ...})` | auto mode's rules, the instrument's: `minDuration`, `minCounts`, `files`, `skipFiles`, `samples`, `skipSamples`, `nameField` (`"s_2"`: only the 2nd part of the sample name split at `"_"` is used — for the wildcards, the filters and grouping one sample's runs; `""`: the whole name); sample-name wildcards (`"*eb*\|\|*ec*"`: alternatives) `emptyBeam`, `darkCurrent`, `calibrant`, `calibrantEmptyBeam`, `calibrantDarkCurrent`, `cellEmptyBeam`, `centerRuns`, `sensitivityRuns`; `singleDarkCurrent`, `singleCalibrantDarkCurrent`, `flexibleDistance` (True/False); `center`: `"maxflux"`, `"emptybeam"`, `"wildcard"`; configuration wildcards (`"*C8D7*"`) `transmission`, `skip`; `masks`: `"none"`, `"configuration"`, `"wavelength"`, `"files"`; `transmissionMargin`, `beamSize`, `detectorX`, `detectorY`; `sensitivity`: `"run"`, `"files"`, `"flat"`; `scale` (True/False; off by default, on in the KWS-3 presets): `autoConfig()` adds the `Scale` column (`autoScale()`); `centerOffsets` (True/False, off): `autoConfig()` adds `HShift`, `VShift` (`addCenterOffsets(s, "mask", "sens")`); `flexibleFrom` (m; 3.5): empty beams at this distance and more take the calibrant at the nearest other distance, `flexibleMin` (m; 1.5): that calibrant's distance must be above it (with `flexibleDistance`); `findBeamstop` (True): where no direct beam is measured, the beam-stop box (the instrument's size) centered on the beam stop's shadow in the empty beam; `openMask` (`"auto"`, `"yes"`, `"no"`): `"mask"` without the beam stop, so the direct beam's center and factor count the beam itself (`"auto"`: when the center is the empty beam, the masks per configuration and the sensitivity flat); `autoAdd` (False): `autoConfig()` first adds the runs of a sample measured several times in one configuration (`autoAdd()`); `infoTable` (False): an info table `"<script>-info"` of the selected runs (`autoInfoTable()`); `processIQ` (False): `autoConfig()` ends with I[Q] into the project (`process(s)`), `merge` (False, with `processIQ`): then the curves of each sample merged (`Dan.Merge.join(s + "-mergingTemplate")`, overlap 30) |

## Dan.Rawdata

| method | |
|---|---|
| `runs()` → list | the run numbers of the input folder, sorted |
| `runs(minDuration=-1, minCounts=-1, files="", skipFiles="", samples="", skipSamples="")` → list | only the runs above the minimums whose file and sample names match |
| `info(runs=None)` → list of dict | per run: `run`, `sample`, `duration`, `counts`, `C`, `D` [m], `lambda`, `beamSize`, `polarization`, `attenuator`, `detectorX`, `detectorY`, `added` (the runs it was added from, `[]` for a measured run), `files` (its files, full paths: the detector file and its own header file — to move or copy a run; a run number is no file name); a run that is not there is a `ValueError` |
| `conditions(runs=None)` → list of dict | the runs grouped by configuration (as the script table groups them): `label`, `C`, `D`, `lambda`, `beamSize`, `runs` |
| `neighbours(n=None, runs=None)` → list of dict | runs **next to each other in the folder**, of the same sample and the same configuration (DAN's own comparison, as `conditions()`). `n=None`: every **whole series**, 2 runs or more, as many as were measured (6 such runs: one group of 6); `n=N`: the series cut into groups of N (6 runs, N=3: 2 groups), the rest left over — only when the request gives the number. Per group `label`, `condition`, `sample`, `runs`. Runs made by adding are left out; `runs`: only these may be grouped (the others still part them) |
| `sampleRuns(runs=None)` → list | the runs minus every standard run of auto mode (empty beams, dark currents, calibrants, sensitivity), for the script table of auto mode; not "all samples" of a request: that is `runs()` |
| `files(runs)` → list | the runs' files, full paths in the order given: the detector file and its own header file (KWS-3: `.det` and `.yaml`); one run or a list. A run number is no file name: to move, copy or open a run's files use these |
| `headerItems()` → list | the names `newHeaderTable()` takes: "Sample", "C", "D", "lambda", "Duration", "Thickness", ... |
| `headerKeys()` → list, `headerValue(run, key)` → str | the instrument header map's keys ("Sample-Title", "Sample-Thickness", "Duration", "Lambda", "Polarization", ...) and one run's value as written in its header, before DAN converts it (`info()` gives the converted values) |
| `setHeaderValue(runs, key, value, backup=False)` → list | writes `value` into each run's header file where the header map reads `key`, then reads it back (the file is left as it was when that fails); the files written; an empty list: `ValueError` "no runs: the list is empty" (a filter that matched none: nothing to do). `backup=True`: `<file>.bak` first, only when there is none yet (it keeps the original from before the first change); ASCII, Flexi, EDF, XML, YAML and HDF5/NeXus headers (XML: only the element's text changes, an element with elements in it is not written; YAML: the file rewritten, comments not kept; HDF5: a text longer than the field is stored in a new field of the same name and attributes); not the run number or a constant of the instrument (`Config.setParameters()`) |
| `newHeaderTable(name, items, runs="*")` → str | a table, one row per run: the first column `Runs` (the run number), then a column per item; `runs`: `"*"` or `"<first>..<last>"` |
| `addRuns(runs, run=None)` → list | the runs **added into one new run**: the matrices summed, in the header the duration, counts and monitors; the new run's number `run`, by default the first run's + the count (`"53217"` + 2 runs: `"5321700002"`); returns the files written — in the **output folder**: to reduce the new run, `setInputPath()` there (or the output folder be the input folder) |
| `sampleGroups(runs=None)` → list of dict | the runs of the same sample and the same configuration **wherever they are in the folder** (not only one after the other), 2 runs or more; per group `label`, `condition` (its number in `conditions()`), `sample`, `runs`. Runs made by adding are left out |
| `newAddingTemplate(runs=None, consecutive=None)` → str | the "Generate Merge Table" (`consecutive=True`: "Only consecutive runs", the series of `neighbours()` instead; `None` as set): `sampleGroups()` as a table `adding-template-<n>`, a column `C<condition>-<sample>` each; first row the new run's number, below the runs to add — edit it if needed, then `addRunsFromTable()` |
| `addRunsFromTable(table, by="columns", skipSingle=None)` → list | the Add buttons: `"columns"` each column (first row the new run, below its runs), `"rows"` each row (first column the new run, right of it its runs); `skipSingle` ("Skip single runs", checked by default): a column (row) of one run is skipped, unchecked it is copied under the new number; the files written |
| `newInfoTable(name, runs, compact=None, matrices=None, detectorSum=None)` → str | "Header(s) - to - Info-Table": all header values, a row per run (added to the info table of that name); `matrices=True`: also each run's raw matrix `Matrix-<run>`; `detectorSum=True`: the counts in the active mask |
| `newInfoMatrix(name, runs, columns=0, mask=None, sens=None, norm=None, roi=None)` → str | "Image(s) - to - Info-Matrix": the runs' images side by side, `columns` per row (added to the info matrix of that name, with its columns); `mask`/`sens`: the active mask/sensitivity applied, `norm`: normalized, `roi`: only the mask's edge box. `"_"` in the name becomes `"-"` |
| `extractMatrices(runs, asDouble=None)` → list | the raw matrices as files `<output folder>/raw-matrix/only_<run>_rawdata.matrix` |

The options (`compact`, `matrices`, ..., `columns`, `asDouble`): `None` keeps what is set; a value given stays set.

## Dan.Mask

A box is `(x1, x2, y1, y2)` in detector pixels (1-based); `None` keeps what is set (after `selectInstrument()`: the
instrument's default), `0` means none.

**Stored and manual.** `newMask()` builds a mask from the mask parameters: the edge and beam-stop boxes and shapes,
dead rows and columns, and triangles. These parameters, with the edge and beam stop switched on or off, belong to the
instrument (`Dan.Options.saveInstrument()` keeps them) and to the session (its script's Settings table), and each
mask keeps them in its label (selecting it in the Mask tab brings them back), so the same mask can be built again.
The tools (`setValue`, `rectangle`, `ellipse`, `ellipseShell`, `sector`) and `addBeamstop()` are **manual** edits of
a matrix that already exists. None of what they change is stored anywhere, so a later `newMask()` of that name builds the mask without
them. To keep such a mask, save it with `saveMask()` and later bring it back with `loadMask()`.

| method | |
|---|---|
| `newMask(name="mask", edge=None, beamstop=None, shape=None, triangles=None, transmission=False, deadRows=None, deadCols=None, edgeShape=None)` → str | the detector edge and the beam stop masked (0); `shape` / `edgeShape`: `"rectangle"` or `"ellipse"` (the beam stop / the edge); `triangles`: triangles masked (exactly three points each), `"x1-y1,x2-y2,x3-y3;..."`; `deadRows`/`deadCols`: `"a-b;..."` whole rows/columns a..b, `"a-b-c-d"` rows a..b in columns c..d (for columns the other way round), `""` none; `transmission=True`: the inverse, only the beam-stop box counts — a **transmission mask**. A mask of that name is made anew |
| `addBeamstop(mask="mask", beamstop=None, shape=None)` → str | the beam-stop box masked in an existing mask, the rest of it kept (e.g. a second beam stop) |
| `loadMask(file, name="mask")` → str | a mask file (`.mask`) |
| `saveMask(mask="mask", file=None)` → str | the mask as a file, its path returned; `None`: `<input folder>/<mask>.mask` |
| `masks()` → list | the masks of the project for this detector size |
| `autoMasks()` → list | auto mode: `"mask"` and the masks the auto option `masks` asks for (`"mask-bs-9.2m"`, `"mask-tr-9.2m"`, ...); with `center="emptybeam"`, `masks="configuration"` and `sensitivity="flat"`, `"mask"` has no beam stop (the flat `"sens"` is made from it, the direct beam's center and factor count the beam; the samples get the configurations' beam-stop masks) |
| `loadMasks(pattern="*.mask")` → list | the mask files of the input folder (`"mask"`: the single one or `mask.mask`) |

The tools of "Adv Masking Options" work on **any** matrix, changing it in place, and return its name. `center` is
`(x, y)`, `size` `(x, y)` or one number, in pixels (1-based, 0.1 steps); the value is set inside the shape, with
`inside=False` outside it:

| method | |
|---|---|
| `setValue(matrix, value)` | every cell |
| `rectangle(matrix, center, size, inside=True, value=0)` | a rectangle |
| `ellipse(matrix, center, size, inside=True, value=0)` | an ellipse |
| `ellipseShell(matrix, center, size, shell, inside=True, value=0)` | the ring between size − shell and size + shell (all of the outer ellipse when shell ≥ size) |
| `sector(matrix, center, phi, inside=True, value=0)` | `phi=(from, to)` in degrees, from ≤ to: 0 toward larger y, 90 toward larger x; `(-30, 30)` goes through 0 |

Masks and sensitivities are matrices of the project: `matrix("mask").cell(row, col)` is 1 for a counted pixel and 0
for a masked one; `matrix("sens")` holds the sensitivity per pixel.

The values stay set: `newMask("mask-tr", transmission=True)` right after `newMask("mask-4m", beamstop=(57, 71, 58,
71))` takes the same box. The first Sensitivity or Processing call after the masks makes `"mask"` the active mask
again.

## Dan.Sensitivity

| method | |
|---|---|
| `newSensitivity(run, name="sens", emptyBeam="", darkCurrent="", transmission=1.0, mask="mask", errorRange=None, errorMatrix=None, maskedValue=None, center=None)` → str | from a flat scatterer (H2O, Plexiglas): its empty beam, dark current and transmission; from a provided pure-sensitivity run: only the run. `transmission`: a number, `"measured"` (the run against `emptyBeam` through the mask, the Transmission button) or `"formula"` (T(λ) = To − TA·exp(−λT/λ) of the instrument's calibrant); `errorRange=(min, max)`: only pixels whose relative error in % is in the range are used; `errorMatrix=True`: also `"<sens>Error"`; `maskedValue`: the sensitivity of masked pixels (0); `center=(x, y)`: the center of the parallax and θ-transmission corrections (1-based), None: the beam center of the run's configuration in the configuration table (the mask's beam stop when no configuration has the run's); the sensitivity's own run reduced with another transmission or center is not flat, and the reduction warns |
| `loadSensitivity(file, name="sens")` → str | a sensitivity file (`.sens`) |
| `saveSensitivity(name="sens", file=None)` → str | the sensitivity as a file, its path returned; `None`: `<input folder>/<sens>.sens` |
| `sensitivities()` → list | |
| `center(name="sens")` → tuple or None | the center the sensitivity was made with, `(x, y, "configuration"\|"mask"\|"manual")`; None for a sensitivity made before this version |
| `autoSensitivity()` → str | auto mode: `"sens"` from the sensitivity run, the `*.sens` file(s) of the input folder, or flat |
| `loadSensitivities(pattern="*.sens")` → list | the sensitivity files of the input folder |
| `transmission(run, emptyBeam, mask="mask")` → `(Tr, dTr)` | a transmission counted through a (transmission) mask — e.g. the H2O transmission for its sensitivity |

`errorRange`, `errorMatrix` and `maskedValue` are the Options page's values: `None` keeps what is set (after
`selectInstrument()`: the instrument's default), a value given stays set, and the instrument (`saveInstrument()`) and
the session keep them. Pixels outside `errorRange` are **also masked in the mask used** — the reduction then leaves
them out too, as after the interface's button; `lastMessages()` says how many (`"mask mask: 47 pixels with errors
outside 0..3 % masked"`); `Dan.Mask.newMask()` makes the mask anew without them.

## Dan.Processing

Script tables, templates and curves are passed by **name** (the string `newScriptTable()` returned,
`"script-mergingTemplate"`, `"H-J"`), never as Table objects.

| method | |
|---|---|
| `setConfigurations(n)`, `configurations()` | the number of columns of the table of configurations |
| `setConfiguration(i, emptyBeam=, darkCurrent=, calibrant=, calibrantEmptyBeam=, calibrantDarkCurrent=, center=, mask=, sens=, transmissionMask=, cellEmptyBeam=, transmission=None)` | only the given fields change. `calibrant`: the flat scatterer of the absolute calibration (none: the direct beam, `calibrantEmptyBeam` and `calibrantDarkCurrent` only); `center`: a run scattering near the beam stop (or the empty beam); `transmission=True`: sample transmissions are measured in this configuration (copied to the rows of the same sample in the other configurations); `cellEmptyBeam`: the empty beam of Tr(EC → EB); `calibrantDarkCurrent`: the dark current of the calibration runs — default: the configuration's `darkCurrent` |
| `calculate()` | reads C, D, λ and the beam size from the headers; calculates μ, the calibrant's Tr, the absolute factor, the beam centers and Tr(EC → EB) — with each configuration's mask and sensitivity, as the table's headers in the interface. A direct-beam calibration on an instrument with "Direct Beam:: Use Current Mask and Sensitivity Matrices" (KWS-3) takes the factor and the center through the active `"mask"` and `"sens"`, since a beam-stop mask hides the beam; I[Q] and the transmissions use each configuration's own masks. Set the configurations' masks before `calculate()` |
| `configuration(i)` → dict | `emptyBeam`, `darkCurrent`, `C`, `D`, `lambda`, `beamSize`, `calibrant…`, `mu`, `factor`, `factorError` (the factor's statistical error, from the calibrant's counts), `center`, `xCenter`, `yCenter`, `xCenterError`, `yCenterError`, `mask`, `sens`, `transmission`, `transmissionMask`, `cellEmptyBeam`, `cellTransmission` |
| `newScriptTable(name, runs)` → str | one row per run; DAN finds its configuration by C, D, λ, beam size, ... (`#-Condition`). It **makes** the script table — never `newTable()` for it |
| `calculateTransmissions(scriptTable)` | the `Transmission-Sample` column |
| `autoConfigurations()` → list of dict | auto mode: the table of configurations from the runs; `configuration(i)` of each; conditions left out are in `lastMessages()` |
| `runCenter(run, mask="mask", sens="sens")` → `(x, y)` | a run's beam center (1-based pixels) through a mask and sensitivity, by the center method of Options (H.F. starting at the mask's beam-stop center, symmetry, or the header) |
| `centerOffset(run, xCenter, yCenter, mask="mask", sens="sens")` → `(HShift, VShift)` | the shift that moves the run's center onto `(xCenter, yCenter)`: `xCenter - x`, `yCenter - y` |
| `addCenterOffsets(scriptTable, mask=None, sens=None)` → int | the script table's `HShift`, `VShift` columns (made, or written again): each sample run's center moved onto its empty beam's (the row's `X-center`, `Y-center`); `mask`/`sens` `None`: as the configuration's center was found (the row's, or the active ones with "Use Current Mask and Sensitivity"); processing shifts each sample matrix by them; the rows written |
| `autoScale(scriptTable)` → int | auto mode: for a wavelength whose configurations all measure their own transmissions, a `Scale` column normalizing each row to the same sample at the longest distance |
| `autoConfig(scriptTable="script")` → str | auto mode up to the script table, ready for processing: (`autoAdd()` first with the auto option `autoAdd`), masks, sensitivity, configurations, `calculate()`, the script table of `sampleRuns()`, transmissions, the info table (option `infoTable`), `autoScale()` (option `scale`); its name. Then `process()` and, when asked, `Dan.Merge.join()` — or both by the options `processIQ`, `merge` |
| `autoAdd()` → list | auto mode: the runs of a sample measured several times in one configuration (the adding template's groups; "Only consecutive runs" as set) added into new runs in the input folder; their single runs moved to the sub-folder `added-sources` there (no run folder any more, also with "Include Sub-Folders"); a sample measured once stays. The added runs (`[]`: nothing to add); ValueError when adding failed |
| `autoInfoTable(name="info")` → str | auto mode's runs only: an info table (a row per run, its header values) of all runs the run filters (Options > Auto-Mode > Runs) select; a new table — an existing name is a ValueError; its name |
| `process(scriptTable, kind="I-Q", output="project")` → list | the tables, matrices (2D kinds into the project) or files made. `kind`: `"I-Q"` (radial average), `"I-x-y"`, `"dI-x-y"`, `"I-Qx"`, `"I-Qy"`, `"Sigma-x-y"`, `"Q-x-y"`, `"dQ-x-y"`, `"I-Polar"`. I-Q in the project also makes `"<script>-mergingTemplate"` (one row per sample) |

**The script table** — columns used by `process()`: `Run-info` (the sample name; rows of the same name are one sample),
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
| `setSmoothing(pixels)`, `smoothing()` → int | "I[x,y] :: Matrix Convolution" before every output: the pixels of the kernel, 1 (none), 5 (the neighbours), 9 (3 × 3), 25 (5 × 5); the error bars stay those of the data |
| `setWideAngle(parallax=None, transmission=None, window=None)`, `wideAngle()` → dict | "I[x,y] :: Wide Angle Corrections": `parallax` `"off"`, `"flat"` (cos³Θ) or `"tubes"` (cos α · cos²Θ, ³He tubes); `transmission` the sample's angle-dependent transmission; `window` the detector window's (its transmission: the script table's column `TrDet`); None keeps the option |
| `setNormalization(by=None, factor=None, darkCurrentByTime=None)`, `normalization()` → dict | "I[x,y] :: Data Normalization": `by` `"monitor2"`, `"time"` or `"monitor1"` (images per `factor` monitor counts or seconds), `factor` 1..10⁶, `darkCurrentByTime` the dark current (#BC) per second, rescaled to each run's duration and normalization; None keeps the option |
| `setRadialAveraging(method=None, linear=None, progressive=None)`, `radialAveraging()` → dict | "I[Q] :: Radial Averaging :: Method": `method` `"shells"` (Standard: each pixel in its ring) or `"hf"` (4-Pixel Interpolation; on a rotated detector along rings of equal angle, as `"shells"`), `linear` 1..100 rings per point, `progressive` 1.0..1.2 (each point that many times wider than the one before; 1: none); None keeps the option |
| `setMatrixFormat(format=None, header=None, sasview=None, ignoreMask=None)`, `matrixFormat()` → dict | "I[x,y] :: Output Format of ASCII-files", for `process(s, "I-x-y", "files")`: `format` `"matrix"` (the image), `"xy"` (x, y [1-based pixels], I, dI), `"qxqy"` (Qx, Qy, I, dI; Qx² + Qy² = Q²), `"qxqy-resolution"` (+ Qz, σQ parallel, σQ perpendicular, the beam-stop shadow factor (1: in the data) and SasView's mask (1: masked, with `ignoreMask`): SasView's 2D columns), `"polar"` (Q, φ [°], I, dI); `header` a line of column names (not for `"matrix"`), `sasview` the NIST lines `Data columns are Qx - Qy - I(Qx,Qy) - err(I) …` and `ASCII data` before the data (SasView's 2D reader needs both), `ignoreMask` masked pixels too (off in every instrument, kept by the session); None keeps the option. Into the project an I-x-y is always a matrix |
| `setCurveFormat(columns=None, header=None, anisotropy=None, anisotropyOffset=None)`, `curveFormat()` → dict | "I[Q] :: Output Format": the columns after Q, I, dI: `"sigma"` (the resolution σ), `"dq"`, `"none"` or `"dq-sigma"`; `header` a `###` line naming the columns of the presentation in the files (I in 1/cm, or counts for "Counts per Channel"); `anisotropy` a column Σ I cos 2φ / Σ I, φ the azimuth plus `anisotropyOffset` (0..360°); None keeps the option |
| `setPresentation(name)`, `presentation()` → str | "I[Q] :: SAS Presentation" of every I[Q] output: `QI`, `Guinier` (ln I vs Q²), `Zimm` (1/I vs Q²), `Porod` (log I vs log Q), `Porod2` (IQ⁴ vs Q²), `logQ`, `logI`, `Debye` (I^-1/2 vs Q²), `1Moment` (IQ vs Q), `2Moment`, `Kratky` (IQ² vs Q), `GuinierRod` (ln IQ vs Q²), `GuinierPlate` (ln IQ² vs Q²); case ignored. Tables and files of another presentation than QI are named with it (`QI-Guinier-…`) and hold x, y, dy and dx (dQ follows the Q column) of it; σ stays the resolution in Q |

## Dan.Merge

Works on tables of the project (the I[Q] tables of `Dan.Processing.process(s)`).

| method | |
|---|---|
| `join(templateTable, overlap=30, smart=False, removeFirst=0, removeLast=0, output="project")` → list | the **Join** page: one merged curve per template row, named after the sample; every point kept with its own σ (the overlap: the % of the overlap range where both are kept). `smart=True` scales the curves to each other in their overlap — absolute data never unless asked. `output="project"` is the Merge button, `"files"` Merge ASCII; the template is the project table to merge from; a template of one configuration has nothing to merge |
| `rebin(templateTable, points=None, qMin=None, qMax=None, logStep=None, removeFirst=0, removeLast=0, output="project")` → list | the **Rebin** page: the curves rebinned onto one Q grid of `points` (None: the page's value) from `qMin` to `qMax` (None: the data's range), logarithmic steps or not (`logStep`, None: as set) |
| `joinTables(name, tables, overlap=30)` → str | any tables joined, through the Merge tab's own table (one row) — e.g. a merged KWS-1 curve with a merged KWS-3 curve; no scaling (absolute units) |
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
p.process(s)
Dan.Merge.join(s + "-mergingTemplate")
```

**Two instruments** (absolute units, no scaling):

```python
Dan.Merge.overlap("KWS3-H-J", "KWS1-H-J")             # (qMin, qMax, points, ratio)
Dan.Merge.joinTables("H-J-all", ["KWS3-H-J", "KWS1-H-J"])
```
