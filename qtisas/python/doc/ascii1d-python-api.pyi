# QtiSAS ASCII.SANS.1D — import/export of radially averaged 1D SAS data (Q, I, dI, sigma); global object Ascii1d
# Ascii1d is GLOBAL like Fittable/Compiler — NO import (from Ascii1d import ... raises ModuleNotFoundError)
# USE Ascii1d for EVERY import or export of 1D SAS data: a SANS/SAXS/SALS/SAS curve, a radially averaged dataset, a
#   1D/ascii/.dat/.RAD/canSAS/NIST/GRASP data file, or a request naming ASCII.SANS.1D/SANS.1D/ascii1d — never own
#   open()/split() reading or Table.importASCII(): header lines with numbers become wrong points, and the options
#   below (remove points, presentations, resolution sigma, binning) exist only here. ALSO for a file whose columns
#   are Q, I (dI, sigma), and for any task with an option only this tool has (remove points, a SAS presentation such
#   as Kratky/Guinier, the resolution sigma, binning).
# CHOOSE THE CALL by source and destination:
#   a TABLE in the project (QI-alpha, "QI-*")  -> table: convertTables(...)   -> file: exportTables(...)
#   a FILE on disk (sample.dat, "*.dat")         -> table: importFiles(...)     -> file: convertFiles(...)
#   a table is NEVER a file: importFiles("QI-alpha") is wrong for a table; exportTables() always writes files
# ONE CALL does the whole move with its options — never import then export (options stay set: a 2nd run applies
#   them again, e.g. removes the points twice). <...> = the request's values:
#   a.reset(); a.setInputFolder("~/<in>");  made = a.importFiles("<file>.dat")                 # file -> table
#   a.reset(); a.setOutputFolder("~/<out>"); a.setExtension("<ext>"); a.exportTables("<table>")  # table -> file
#   a.reset(); a.setOutputFolder("~/<out>"); a.setRemovePoints(first=3); a.convertFiles("~/<in>/*.dat")  # file->file
#   a.reset(); a.setResolution(True); a.setResolutionGeometry(...); a.convertTables("<table>", "<new>")  # tbl->tbl
# THE RECIPE — every task, in this order; <...> = the request's values:
#   a = Ascii1d
#   a.reset()                              # 1. ALWAYS first: options stay set between calls (also from the GUI)
#   a.setInputFolder("~/<folder>")         # 2. folders "~/..." or absolute, never relative
#   # 3. ONLY what the request says about the file — NOTHING for a plain file of numbers:
#   #    .xml/canSAS -> a.setFormat("cansas"); NIST -> a.setFormat("nist"); "5-line header" -> a.setHeader(5);
#   #    "data after the line X" -> a.setHeader("X")   (setHeader on a file WITHOUT a header drops data points)
#   # 4. ONLY the options the request asks for (setRemovePoints, setPresentation, setResolution, ...)
#   made = a.importFiles("<file>")         # 5. run — NO name unless the request names the result
#   scriptPrint("R1:", made[0])            #    use and print the name the run returned
# NAMES: names only when the request names the result: "header.dat as the table \"HeaderData\"" ->
#   importFiles("header.dat", "HeaderData"); otherwise t = table(made[0]) — never table("<file name>"), never an
#   invented name such as "Data"
# PATHS: a run returns ABSOLUTE paths — use them; Python's open() does NOT expand "~" (os.path.expanduser())
reset()

# --- the four directions: each returns what it made, in source order — table NAMES, or absolute file PATHS
#   files: absolute, "~/...", or relative to inputFolder(); wildcards in the file part ("~/d/run_*.dat")
#   tables: names or wildcard patterns ("QI-*"); sources and names: a string or a list each, any combination;
#   names: one per source ("" = default)
#   makes nothing -> ValueError with the reason; a skipped dataset is printed ("... contains no data, and is
#   skipped") and listed in lastMessages(), the others are made
importFiles(files, names=[])->list    # file -> table: a.importFiles("s.dat"); ("s.dat", "S") if the request names S
convertFiles(files, names=[])->list   # file -> file (into outputFolder())
exportTables(tables, names=[])->list  # table -> file (into outputFolder()): a.exportTables("QI-a", "a3.dat")
convertTables(tables, names=[])->list # table -> table — REFUSES a default name equal to its source ("QI-a" with no
#   prefix): give names, setAutoPrefix(True) + setPresentation(), or setIndexing(True)
lastMessages()->list
# a GIVEN name wins (table name, file name in outputFolder(), or a path); an existing table of that name is refilled;
#   a file name without extension gets extension(): exportTables("QI-a", "a3") writes a3.RAD
# DEFAULT names: the source base name, non-alphanumerics -> "-" (run_102.dat -> table "run-102", file "run-102.RAD");
#   setAutoPrefix(True): presentation prefix "QI-run-102"/"Kratky-run-102"; setNameNumber(k): k-th number in the
#   name, always prefixed ("QI-102"); setIndexing(True): unique names instead of replacing (tables "-v-1",
#   "-v-2"; files "-v-0.RAD", "-v-1.RAD")
#   WITHOUT a name or indexing, importing the same file again REPLACES the earlier table
# tables made: columns Q [X], I [Y], dI [yErr], Sigma [xErr] (4th only with sigma)

# --- reading
setFormat(format)  format()->str      # "ascii", "ascii+header" (reset() default, 0 lines), "cansas" (canSAS 1D
#   XML), "nist"; changing the format resets the header — setHeader() AFTER setFormat()
setHeader(linesOrMarker)  header()->str  # 5 or "5": skip 5 header lines ("starts with a 5-line text header");
#   other text: skip up to the line containing it
#   GRASP: setHeader("***** Data *****"). Every line with >= 2 numbers is a point (its first numbers = Q, I, dI,
#   sigma): "Cx = 64.4493 Cy = 64.1428" or "# date: 2026-10-19" in a header become wrong points if not skipped —
#   a file WITH A HEADER ("5-line header", "data after the line ...") ALWAYS needs setHeader(N | "that line")
setColumns(n)  columns()->int        # 2..4 columns written (Q, I, dI, sigma), default 4
setInputFolder(folder)  inputFolder()->str

# --- writing
setOutputFolder(folder)  outputFolder()->str   # created if missing
setExtension(ext)  extension()->str            # without the dot, default "RAD"
setAutoPrefix(on)  setNameNumber(k)  setIndexing(on)

# --- processing (in this order, every direction)
setRemovePoints(first=0, last=0, from=0, to=0, negative=False)  # ends, a 1-based range, I <= 0
setPresentation(to, from="QI")  presentations()->list  # Guinier (ln I vs Q²), Zimm, Porod, Porod2, logQ, logI,
#   Debye, 1Moment, 2Moment, GuinierRod, GuinierPlate, Kratky (I·Q² vs Q); to == from = off
setResolution(on)                       # adds sigma(Q) as column 4
setResolutionGeometry(collimationCm, detectorCm, pixelCm=0.525, sampleApertureWmm=10, sampleApertureHmm=10,
                      collimationApertureWmm=30, collimationApertureHmm=30, lambdaA=6, deltaLambda=0.1)
#   CM and MM, NOT METRES: 8 m collimation, 4 m detector = setResolutionGeometry(800, 400, ...); a distance below 50
#   raises ValueError (given in metres)
setResolutionLenses(on, focus=0)        # focus in cm
resolutionSigma(q)->float               # sigma at one Q, current geometry
setMath(op, factor=1, op2="", factor2=1)  # "+","-","*","/"; "" = off: scale 2 then -0.01 = setMath("*",2,"-",0.01)
setCalculator(op, column)               # I op another curve point by point: Y column "buffer_I" or a table; same
#   number of points; op "" = off
setBinning(every=1, ratio=1)            # average every n points (last incomplete group dropped); ratio 1.0-1.995
#   > 1: progressive binning

# --- plot / saved settings
setPlot(on, xError=False, yError=True)  # tables made go into the ACTIVE graph
saveFormat(name)  loadFormat(name)  formats()->list   # the GUI's Session Tools formats
