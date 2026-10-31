# QtiSAS DAN-SANS — SANS data reduction: raw detector runs of an instrument -> masks, sensitivity, the table of
#   configurations (empty beam, dark current, absolute calibration, beam center), a script table of sample runs ->
#   I[Q] tables (or files) per run -> merged curves per sample; global object Dan, one object per tab
# Dan is GLOBAL like Fittable/Jnse — NO import; its parts: Dan.Options (Dan.Options.Config), Dan.Rawdata, Dan.Mask,
#   Dan.Sensitivity, Dan.Processing (Dan.Processing.Options), Dan.Merge
# USE Dan for: SANS reduction, raw runs, masks, sensitivity, absolute calibration, beam center, transmission, radial
#   averaging I[Q], merging configurations (detector distances) or instruments — never own reading of raw files
# a reduction goes on over several requests: use the session, masks and tables the earlier ones made
# RUNS: run numbers as in the file names ("53216", "00082558"; ints too) or file paths; configurations 1..n
# FILES (mask, sensitivity, runs): a path ("~/..." or absolute) or a name in the input folder
# A CALL THAT MAKES NOTHING raises ValueError with DAN's reason; part.lastMessages(): DAN's warnings of the last
#   call even when it worked (e.g. "condition # 2:: plexi-file does not exist!" = no calibrant there: factor 1)
# THE RECIPE — <...> = the request's values (KWS-1 example: 3 configurations, Plexiglas calibration):
#   o, p = Dan.Options, Dan.Processing
#   o.newSession(); o.selectInstrument("<instrument>"); o.setInputPath("<raw folder>")
#     # 1. ONCE, when the request starts a session: newSession() clears masks, configurations and script tables —
#     #    the next steps of the same reduction continue in that session, never with newSession() again
#   Dan.Mask.newMask()                                     # 2. "mask": the instrument's edge + beam stop;
#     # ALL masks here, before the sensitivity (KWS-3: + newMask("mask-bs-…", beamstop=…), newMask("mask-tr-…", …))
#   Dan.Sensitivity.loadSensitivity("<file>.sens")         #    or newSensitivity("<run>", ...): "sens"
#   p.setConfigurations(<n>)                               # 3. one per detector distance / wavelength
#   p.setConfiguration(1, emptyBeam="<EB>", darkCurrent="<B4C>", calibrant="<plexi>", calibrantEmptyBeam="<EB>",
#                      calibrantDarkCurrent="<B4C>", center="<run>", transmission=<True: Tr measured here>)
#   p.calculate()                                          # 4. C, D, lambda, factor, centers (p.configuration(i))
#   s = p.newScriptTable("script", [<sample runs>])        # 5. the samples; set "Thickness" [cm] in it if needed
#   p.calculateTransmissions(s)
#   qi = p.process(s)                                      # 6. I[Q] tables "QI-SM-<run>-<sample>" (+ template)
#   merged = Dan.Merge.join(s + "-mergingTemplate")        # 7. one table per sample, the configurations joined
# REQUEST WORDS -> setConfiguration(i, ...) KEYWORD (i = the configuration's number; runs as the request gives them):
#   empty beam / EB <r>              -> emptyBeam="<r>"           dark current / B4C <r>        -> darkCurrent="<r>"
#   Plexiglas / water / calibrant <r> -> calibrant="<r>"          "with its empty beam <r>"     -> calibrantEmptyBeam="<r>"
#   direct-beam calibration with <r> -> calibrantEmptyBeam="<r>" only (no calibrant)
#   center from <r>                  -> center="<r>"
#   empty cell for Tr(EC -> EB) <r>  -> cellEmptyBeam="<r>"
#   transmissions measured in it     -> transmission=True
#   then ONE p.calculate(); read p.configuration(i)["factor"], ["xCenter"], ["yCenter"]
# AUTO MODE — a whole folder, no run numbers given ("reduce the data of <instrument> in <folder>"):
#   o.newSession(); o.selectInstrument("<instrument>"); o.setInputPath("<folder>")
#   s = p.autoConfig()                       # masks, sens, configurations, script table, Tr: ready (Scale,
#                                            #   HShift/VShift when their auto options scale, centerOffsets are on)
#   p.process(s)                             # then what the request asks, each its own line: kind "I-Q" (default),
#                                            #   "I-x-y", "I-Polar", ...; output "project" (default) or "files"
#   merged = Dan.Merge.join(s + "-mergingTemplate")   # merging only when asked; default join, not smart, nothing
#                                            #   removed; one configuration: nothing to merge (the I[Q] tables)
#   the roles of the runs come from the instrument's auto options (Dan.Options.Config.autoOptions());
#   autoConfig() step by step: Dan.Mask.autoMasks(); Dan.Sensitivity.autoSensitivity(); p.autoConfigurations();
#   p.calculate(); s = p.newScriptTable("script", Dan.Rawdata.sampleRuns()); p.calculateTransmissions(s);
#   p.autoScale(s) (option scale); p.addCenterOffsets(s, "mask", "sens") (option centerOffsets); its steps
#   ("DAN auto: ...") go to the terminal QtiSAS runs in, the configurations left out (no center run, skipped) to
#   p.lastMessages()
# CHOOSE THE CALL:
#   instrument names / modes             -> Dan.Options.instruments() / modes()
#   center or transmission method        -> Dan.Options.Config.setCenterMethod("hf"|"symmetry"|"header"),
#                                           setTransmissionMethod(<one of transmissionMethods()>)
#   the runs of the folder, a header table -> Dan.Rawdata.runs(), newHeaderTable(name, ["Sample", "D", ...])
#   add neighbouring runs (same sample + configuration) -> for g in Dan.Rawdata.neighbours(): addRuns(g["runs"])
#     (the request gives a number N of runs: neighbours(N))
#   add all runs of a sample + configuration, not one after the other -> for g in sampleGroups(): addRuns(g["runs"])
#   a run's file name / the files of runs (move, copy) -> Dan.Rawdata.files(runs)  (a run number is no file name)
#   change a value in runs' header files   -> Dan.Rawdata.setHeaderValue(Dan.Rawdata.runs(), key, value)
#     ("all samples"/"all files"/"the folder": runs(), EVERY run; sampleRuns() only when the request leaves the
#     standard runs out); keys: headerKeys(); check after it: no run left with the old value
#   a beam-stop / transmission mask       -> Dan.Mask.newMask(name, beamstop=(x1, x2, y1, y2), transmission=...)
#   a sector / rectangle / ellipse in a mask -> Dan.Mask.sector(...) / rectangle(...) / ellipse(...) on matrix
#   a transmission of one run (e.g. H2O)  -> Dan.Sensitivity.transmission(run, emptyBeam, mask) -> (Tr, dTr)
#   remove points at the curve ends       -> Dan.Processing.Options.setRemovePoints(first, last)   (I[Q])
#                                            Dan.Merge.join(..., removeFirst=, removeLast=)          (merged)
#   ASCII files instead of tables         -> p.process(s, output="files"), Dan.Merge.join(t, output="files")
#   join curves of two instruments        -> Dan.Merge.joinTables(name, [table1, table2]); agreement: overlap(t1, t2)
#   any instrument setting, e.g. points always removed -> Dan.Options.Config.setParameters({...});
#                                            keep it: Dan.Options.saveInstrument("<name>")

# --- Dan.Options
newSession()   openSession(scriptTable)   # a saved session = its script table
instruments()->list   instrument()->str   selectInstrument(name)   modes()->list   selectMode(mode)
# PN: selectMode("(PN) Polarized Neutrons"), then selectPolarizationReductionMode("x421"), etc.; the PN reduction
#   itself needs the polarized script table (no Python call yet: from the interface)
# polarizationReductionModes()->list ("none", "x2", "x4", "x421".."x424"); polarizationReductionMode()->str
selectPolarizationReductionMode(mode)  # the PN correction combobox; ValueError unless PN mode is selected
# POL-ALIAS-UP/DOWN/UP-UP/UP-DOWN/DOWN-DOWN/DOWN-UP/OUT-UP/OUT-DOWN: instrument parameters via Config.setParameters().
# PN states (polarizer-analyzer; the alias name, as Polarization-Aliased shows it, and its number):
#   out 0, up 1, down 2, up-up 3, up-down 4, down-down 5, down-up 6, out-up -1, out-down -2
#   (out-up/out-down: polarizer out, analyzer in; reduced by "none" only; transmissions, calibrants)
# Aliases: whole-value wildcards (* ? [...]), case and spaces ignored, || alternatives.
# Rawdata.info()[i]["polarization"] stays raw; configuration grouping uses aliases in PN mode when polarization is a
# condition, raw values otherwise.
setInputPath(path)  inputPath()->str   setOutputPath(path)  outputPath()->str   # raw runs in, ASCII files out
#   setInputPath() sets the output folder to the input's parent (as the interface): setOutputPath() after it
#   (an output folder that does not exist yet is made)
saveInstrument(name)             # the current instrument with its changes, under a name of its own (then selected)
# Dan.Options.Config — the instrument's methods (selectInstrument() sets them; set after it):
centerMethods()->list ("hf", "symmetry", "header")  centerMethod()->str  setCenterMethod(method)
#   hf: moments around the beam stop's center (strong scattering near the beam stop); symmetry: X and Y
#   symmetrization (stable for weak scattering); header: the center written in the run's header
transmissionMethods()->list  transmissionMethod()->str  setTransmissionMethod(method)
parameters()->dict  setParameters({key: value, ...})   # the instrument's whole configuration as its .SANS file
#   holds it: "Options-1D-RemoveLast": "5", "Options-2D-CenterMethod": "SYM", ... (the keys: parameters());
#   set as selecting the instrument does; a value the interface does not take: ValueError
autoOptions()->dict  setAutoOptions({name: value, ...})   # auto mode's rules (Options > Auto-Mode), the instrument's
#   minDuration, minCounts, files, skipFiles, samples, skipSamples, nameField ("s_2": the 2nd part of the name split
#   at "_", also for grouping a sample's runs; "": whole name); sample-name wildcards ("||" between
#   alternatives): emptyBeam, darkCurrent, calibrant, calibrantEmptyBeam, calibrantDarkCurrent, cellEmptyBeam,
#   centerRuns, sensitivityRuns; singleDarkCurrent, singleCalibrantDarkCurrent, flexibleDistance (True/False);
#   center "maxflux"|"emptybeam"|"wildcard"; transmission, skip: configuration wildcards ("*C8D7*");
#   masks "none"|"configuration"|"wavelength"|"files"; transmissionMargin, beamSize, detectorX, detectorY;
#   sensitivity "run"|"files"|"flat"
#   scale True|False (off; on in the KWS-3 presets): autoConfig() adds the Scale column (autoScale());
#   centerOffsets True|False (off): autoConfig() adds HShift, VShift (addCenterOffsets(s, "mask", "sens"))

# --- Dan.Rawdata
runs()->list                     # the run numbers of the input folder, sorted
runs(minDuration=-1, minCounts=-1, files="", skipFiles="", samples="", skipSamples="")->list  # filtered: > minimum
info(runs=None)->list of dict    # run, sample, duration, counts, C, D [m], lambda, beamSize, polarization, ...,
#   added (the runs it was added from; [] measured), files (full paths: detector + own header file — to move/copy a
#   run; a run number is no file name); a run that is not there: ValueError
neighbours(n=None, runs=None)->list of dict   # NEXT TO EACH OTHER in the folder, same sample + configuration;
#   n None: every WHOLE series (2+ runs, as many as measured); n=N only when the request gives N: groups of N;
#   label, condition, sample, runs; added runs left out
conditions(runs=None)->list of dict   # the runs grouped by configuration: label, C, D, lambda, beamSize, runs
sampleRuns(runs=None)->list      # the runs minus the standard runs (empty beams, B4C, calibrants, sensitivity) —
#   for the script table of auto mode; NOT "all samples" of a request: runs()
files(runs)->list                # the runs' files, full paths: detector file + own header file (KWS-3: .det,
#   .yaml); one run or a list; a run number is NO file name
headerItems()->list              # the names newHeaderTable() takes ("Sample", "C", "D", "lambda", "Duration", ...)
headerKeys()->list   headerValue(run, key)->str   # the header map's keys; a run's value as in its header
setHeaderValue(runs, key, value, backup=False)->list  # into the header files where the map reads key, read back; the
#   files; backup=True: <file>.bak first when there is none; ASCII/Flexi/EDF/XML/YAML/HDF5; not the run number or a
#   const; an empty list: ValueError "no runs: the list is empty" (a filter matched none: nothing to do)
newHeaderTable(name, items, runs="*")->str   # one row per run, a column per item; runs "*" or "<first>..<last>"
#   the header table: first column Runs (the run number), then one column per item
addRuns(runs, run=None)->list    # the runs added into ONE new run (matrices, duration, counts, monitors summed);
#   run None: <first run>0000<N>; the files written, in the OUTPUT folder (setInputPath() there to reduce it)
sampleGroups(runs=None)->list of dict   # same sample + configuration ANYWHERE in the folder (2+ runs; added runs
#   left out): label, condition, sample, runs
newAddingTemplate(runs=None, consecutive=None)->str   # sampleGroups() (consecutive=True: the neighbours() series)
#   as a table: a column each (first row the new run, then its runs)
addRunsFromTable(table, by="columns", skipSingle=None)->list   # "columns" | "rows": the first cell the new run,
#   the others added; skipSingle (default checked): a column/row of one run skipped, else copied
newInfoTable(name, runs, compact=None, matrices=None, detectorSum=None)->str   # all header values, a row per run
newInfoMatrix(name, runs, columns=0, mask=None, sens=None, norm=None, roi=None)->str   # images side by side
extractMatrices(runs, asDouble=None)->list    # <output folder>/raw-matrix/only_<run>_rawdata.matrix
#   info table/matrix of that name: added to; options None: as set; "_" in a matrix name becomes "-"

# --- Dan.Mask  (box = (x1, x2, y1, y2) in pixels, 1-based; None: as set = the instrument's default; 0: none)
#   STORED: newMask()'s parameters (edge, beamstop, shape, edgeShape, deadRows, deadCols, triangles; edge/beam stop
#   on or off) are the instrument's (saveInstrument), the session's and in the mask's label; MANUAL: the tools and
#   addBeamstop() change the matrix only — newMask() of that name drops them; keep it as a file: saveMask() /
#   loadMask()
newMask(name="mask", edge=None, beamstop=None, shape=None, triangles=None, transmission=False, deadRows=None,
        deadCols=None, edgeShape=None)->str
#   shape / edgeShape: "rectangle" | "ellipse" (beam stop / edge); triangles: "x1-y1,x2-y2,x3-y3;..." (3 points each);
#   deadRows/deadCols: "a-b;..." whole rows/columns a..b, "a-b-c-d" rows a..b in columns c..d (columns: the other
#   way round); "": none;
#   transmission=True: the inverse — only the beam-stop box counts (a transmission mask);
#   a mask of that name is made anew
#   the values stay set: a later newMask() without edge/beamstop takes the last ones; the first Sensitivity or
#   Processing call after the masks activates "mask" again (the sensitivity's mask, its values shown)
loadMask(file, name="mask")->str   masks()->list
addBeamstop(mask="mask", beamstop=None, shape=None)->str   # the box masked in an existing mask, the rest kept
saveMask(mask="mask", file=None)->str   # the path; None: <input folder>/<mask>.mask
# the "Adv Masking Options" tools on ANY matrix, changed in place; center=(x, y), size=(x, y) or one number, in
#   pixels (1-based, 0.1 steps); inside=False: the value set outside the shape; returns the matrix name
setValue(matrix, value)          # every cell
rectangle(matrix, center, size, inside=True, value=0)   ellipse(matrix, center, size, inside=True, value=0)
ellipseShell(matrix, center, size, shell, inside=True, value=0)   # the ring between size - shell and size + shell
sector(matrix, center, phi, inside=True, value=0)   # phi=(from, to) degrees, from <= to: 0 toward larger y, 90
#   toward larger x; (-30, 30) goes through 0
autoMasks()->list                # auto mode: "mask" + the auto option masks' (e.g. "mask-bs-9.2m", "mask-tr-9.2m");
#   center "emptybeam" + masks "configuration" + sensitivity "flat": "mask" WITHOUT beam stop (flat sens = mask)
loadMasks(pattern="*.mask")->list   # the mask files of the input folder
# masks and sensitivities are matrices of the project: matrix("mask").cell(row, col) is 1 for a counted pixel, 0 for a
#   masked one; matrix("sens") holds the sensitivity per pixel

# --- Dan.Sensitivity
newSensitivity(run, name="sens", emptyBeam="", darkCurrent="", transmission=1.0, mask="mask", errorRange=None,
               errorMatrix=None, maskedValue=None, center=None)->str
#   from a flat scatterer (H2O, Plexiglas) or a provided pure-sensitivity run (then only the run);
#   transmission: a number | "measured" (run vs emptyBeam through the mask) | "formula" (the calibrant's T(lambda));
#   errorRange=(min, max): only pixels with a relative error in % inside are used — the others are ALSO masked in
#   the mask (lastMessages() says how many; newMask() makes it anew); errorMatrix=True: also "<sens>Error";
#   maskedValue: the sensitivity of masked pixels; these three: None as set, stored with instrument and session
#   center=(x, y): the center of the wide-angle corrections (1-based); None: the mask's beam stop
center(name="sens")->tuple|None   # (x, y, "mask"|"manual"): the center it was made with; None: older label
loadSensitivity(file, name="sens")->str   sensitivities()->list
saveSensitivity(name="sens", file=None)->str   # the path; None: <input folder>/<sens>.sens
autoSensitivity()->str           # auto mode: "sens" from the sensitivity run, the *.sens file(s) or flat
loadSensitivities(pattern="*.sens")->list
transmission(run, emptyBeam, mask="mask")->tuple (Tr, dTr)   # counted through the mask

# --- Dan.Processing  (configuration i = column i of the table of configurations)
# script tables, templates and curves are passed by NAME (the string newScriptTable() returned,
#   "script-mergingTemplate", "H-J"), never as Table objects
setConfigurations(n)   configurations()->int
setConfiguration(i, emptyBeam=, darkCurrent=, calibrant=, calibrantEmptyBeam=, calibrantDarkCurrent=, center=,
                 mask=, sens=, transmissionMask=, cellEmptyBeam=, transmission=None)   # only the given fields
#   calibrant: the flat scatterer of the absolute calibration (none: direct beam -> calibrantEmptyBeam); center: a
#   run scattering near the beam stop; transmission=True: sample transmissions are measured in this configuration
#   (copied to the rows of the same sample in the other configurations); cellEmptyBeam: the EB for Tr(EC -> EB)
#   calibrantDarkCurrent: the dark current of the calibration runs — default: the configuration's darkCurrent
calculate()                       # reads C, D, lambda, beam size; calculates mu, Tr, factor, centers, Tr(EC -> EB)
#   with each configuration's mask and sens — as the table's headers in the interface; a direct-beam calibration
#   with the instrument's "Direct Beam:: Use Current Mask and Sensitivity Matrices" (KWS-3) takes the factor and the
#   center through the active "mask" and "sens" (a beam-stop mask hides the beam); set the masks before calculate()
configuration(i)->dict            # C, D, lambda, beamSize, factor, factorError, xCenter, yCenter, xCenterError, mask,
#   sens, ... (factorError: the factor's statistical error, from the calibrant's counts)
newScriptTable(name, runs)->str   # one row per run, its configuration found by C, D, lambda, beam size, ...
#   it MAKES the script table — never newTable() for it
#   columns: Run-info (sample name), #-Run, #-Condition, ..., Thickness [cm], Transmission-Sample, Factor, ...;
#   add a column "Scale" (a factor per row) by the table API when a configuration must be scaled
#   its cells are TEXT: t.text(col, row), columns by name: col = list(t.colNames()).index("Transmission-Sample") + 1;
#   Transmission-Sample is "0.8981 [ ±0.0012 ]" -> float(t.text(col, row).split("[")[0]) (t.cell() gives 0.0 for it);
#   #-Run is the run as text ("53222"), #-Condition the configuration number
calculateTransmissions(scriptTable)
process(scriptTable, kind="I-Q", output="project")->list   # the tables (or files) made; kind: "I-Q", "I-x-y",
#   "dI-x-y", "I-Qx", "I-Qy", "Sigma-x-y", "Q-x-y", "dQ-x-y", "I-Polar"; I-Q also makes "<script>-mergingTemplate"
autoConfigurations()->list of dict   # auto mode: the table of configurations from the runs; configuration(i) of each
runCenter(run, mask="mask", sens="sens")->tuple (x, y)   # a run's beam center by the center method (1-based)
centerOffset(run, xCenter, yCenter, mask="mask", sens="sens")->tuple (HShift, VShift)   # moves it onto (x, y)
addCenterOffsets(scriptTable, mask=None, sens=None)->int   # HShift, VShift columns: each sample's center onto its
#   empty beam's (the row's X-, Y-center); None: as the configuration's center was found; processing shifts by them
autoScale(scriptTable)->int      # a wavelength whose configurations all measure transmissions: a "Scale" column,
#   every row normalized to the same sample at the longest distance; the rows scaled
autoConfig(scriptTable="script")->str   # auto mode up to the script table, ready for process() (and Dan.Merge.join())
# Dan.Processing.Options:  setRemovePoints(first, last)   removePoints()->tuple

# --- Dan.Merge  (tables of the project)
join(templateTable, overlap=30, smart=False, removeFirst=0, removeLast=0, output="project")->list
#   the Join page: one curve per template row (its name: the sample), every point kept with its own sigma;
#   smart=True scales the curves to each other in their overlap (absolute data: never unless asked);
#   output "project" = the Merge button, "files" = Merge ASCII; a template of one configuration: nothing to merge
rebin(templateTable, points=None, qMin=None, qMax=None, logStep=None, removeFirst=0, removeLast=0, output="project")
#   ->list   the Rebin page: one Q grid of points (None: the page's value), qMin..qMax (None: the data's range),
#   logarithmic steps (None: as set)
joinTables(name, tables, overlap=30)->str    # any tables joined, e.g. a merged KWS-1 curve with a merged KWS-3 one
overlap(first, second)->tuple (qMin, qMax, points, ratio)   # ratio = mean I(second) / I(first) in the overlap
