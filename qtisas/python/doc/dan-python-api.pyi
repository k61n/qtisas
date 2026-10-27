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
#   qi = p.run(s)                                          # 6. I[Q] tables "QI-SM-<run>-<sample>" (+ template)
#   merged = Dan.Merge.run(s + "-mergingTemplate")         # 7. one table per sample, the configurations joined
# REQUEST WORDS -> setConfiguration(i, ...) KEYWORD (i = the configuration's number; runs as the request gives them):
#   empty beam / EB <r>              -> emptyBeam="<r>"           dark current / B4C <r>        -> darkCurrent="<r>"
#   Plexiglas / water / calibrant <r> -> calibrant="<r>"          "with its empty beam <r>"     -> calibrantEmptyBeam="<r>"
#   direct-beam calibration with <r> -> calibrantEmptyBeam="<r>" only (no calibrant)
#   center from <r>                  -> center="<r>"
#   empty cell for Tr(EC -> EB) <r>  -> cellEmptyBeam="<r>"
#   transmissions measured in it     -> transmission=True
#   then ONE p.calculate(); read p.configuration(i)["factor"], ["xCenter"], ["yCenter"]
# CHOOSE THE CALL:
#   instrument names / modes             -> Dan.Options.instruments() / modes()
#   center or transmission method        -> Dan.Options.Config.setCenterMethod("hf"|"symmetry"|"header"),
#                                           setTransmissionMethod(<one of transmissionMethods()>)
#   the runs of the folder, a header table -> Dan.Rawdata.runs(), newHeaderTable(name, ["Sample", "D", ...])
#   a beam-stop / transmission mask       -> Dan.Mask.newMask(name, beamstop=(x1, x2, y1, y2), transmission=...)
#   a transmission of one run (e.g. H2O)  -> Dan.Sensitivity.transmission(run, emptyBeam, mask) -> (Tr, dTr)
#   remove points at the curve ends       -> Dan.Processing.Options.setRemovePoints(first, last)   (I[Q])
#                                            Dan.Merge.run(..., removeFirst=, removeLast=)          (merged)
#   ASCII files instead of tables         -> p.run(s, output="files"), Dan.Merge.run(t, output="files")
#   join curves of two instruments        -> Dan.Merge.join(name, [table1, table2]); agreement: overlap(t1, t2)
#   any instrument setting, e.g. points always removed -> Dan.Options.Config.setParameters({...});
#                                            keep it: Dan.Options.saveInstrument("<name>")

# --- Dan.Options
newSession()   openSession(scriptTable)   # a saved session = its script table
instruments()->list   instrument()->str   selectInstrument(name)   modes()->list   selectMode(mode)
setInputPath(path)  inputPath()->str   setOutputPath(path)  outputPath()->str   # raw runs in, ASCII files out
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

# --- Dan.Rawdata
runs()->list                     # the run numbers of the input folder, sorted
headerItems()->list              # the names newHeaderTable() takes ("Sample", "C", "D", "lambda", "Duration", ...)
newHeaderTable(name, items, runs="*")->str   # one row per run, a column per item; runs "*" or "<first>..<last>"
#   the header table: first column Runs (the run number), then one column per item

# --- Dan.Mask  (box = (x1, x2, y1, y2) in pixels, 1-based; None: as set = the instrument's default; 0: none)
newMask(name="mask", edge=None, beamstop=None, shape=None, polygons=None, transmission=False)->str
#   shape: "rectangle" | "ellipse" (beam stop); polygons: "x1-y1,x2-y2,x3-y3;..." triangles masked;
#   transmission=True: the inverse — only the beam-stop box counts (a transmission mask);
#   a mask of that name is made anew
#   the values stay set: a later newMask() without edge/beamstop takes the last ones; the first Sensitivity or
#   Processing call after the masks activates "mask" again (the sensitivity's mask, its values shown)
loadMask(file, name="mask")->str   masks()->list
# masks and sensitivities are matrices of the project: matrix("mask").cell(row, col) is 1 for a counted pixel, 0 for a
#   masked one; matrix("sens") holds the sensitivity per pixel

# --- Dan.Sensitivity
newSensitivity(run, name="sens", emptyBeam="", darkCurrent="", transmission=1.0, mask="mask")->str
#   from a flat scatterer (H2O, Plexiglas) or a provided pure-sensitivity run (then only the run)
loadSensitivity(file, name="sens")->str   sensitivities()->list
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
#   with each configuration's mask and sens; without calibrant (direct beam) the factor and the center through the
#   full mask "mask" (a beam-stop mask hides the beam); set the configurations' masks before calculate()
configuration(i)->dict            # C, D, lambda, beamSize, factor, xCenter, yCenter, xCenterError, mask, sens, ...
newScriptTable(name, runs)->str   # one row per run, its configuration found by C, D, lambda, beam size, ...
#   it MAKES the script table — never newTable() for it
#   columns: Run-info (sample name), #-Run, #-Condition, ..., Thickness [cm], Transmission-Sample, Factor, ...;
#   add a column "Scale" (a factor per row) by the table API when a configuration must be scaled
#   its cells are TEXT: t.text(col, row), columns by name: col = list(t.colNames()).index("Transmission-Sample") + 1;
#   Transmission-Sample is "0.8981 [ ±0.0012 ]" -> float(t.text(col, row).split("[")[0]) (t.cell() gives 0.0 for it);
#   #-Run is the run as text ("53222"), #-Condition the configuration number
calculateTransmissions(scriptTable)
run(scriptTable, kind="I-Q", output="project")->list   # the tables (or files) made; kind: "I-Q", "I-x-y",
#   "dI-x-y", "I-Qx", "I-Qy", "Sigma-x-y", "Q-x-y", "dQ-x-y", "I-Polar"; I-Q also makes "<script>-mergingTemplate"
# Dan.Processing.Options:  setRemovePoints(first, last)   removePoints()->tuple

# --- Dan.Merge  (tables of the project)
run(templateTable, method="join", overlap=30, removeFirst=0, removeLast=0, output="project")->list
#   one curve per template row (its name: the sample); join keeps every point with its own sigma, rebin rebins
join(name, tables, overlap=30)->str          # e.g. a merged KWS-1 curve with a merged KWS-3 curve, no scaling
overlap(first, second)->tuple (qMin, qMax, points, ratio)   # ratio = mean I(second) / I(first) in the overlap
