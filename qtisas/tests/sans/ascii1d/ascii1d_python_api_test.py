"""ASCII.1D Python API (Ascii1d): the four directions with an explicit or a default name, and every error path.

Import (file -> table), export (table -> file), convert (file -> file, table -> table) on small synthetic data;
an invalid option or source raises ValueError naming the fix; a run that makes nothing raises; a skipped dataset is
printed, not shown as a dialog.
"""

import contextlib
import io
import os
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_c, set_app_locale

set_app_locale(locale_c())
a = Ascii1d
folder = tempfile.mkdtemp(prefix="qtisas-ascii1d-api-")


def raises(call, *words):
    try:
        call()
    except ValueError as e:
        assert all(w in str(e) for w in words), (str(e), words)
        return
    raise AssertionError("no ValueError: %r" % (words,))


try:
    for name, n in (("a.dat", 5), ("b.dat", 2)):
        with open(os.path.join(folder, name), "w", encoding="utf-8") as f:
            f.writelines("%g %g 0.1\n" % (0.01 * k, 2 * k) for k in range(1, n + 1))
    a.reset()
    a.setInputFolder(folder)
    a.setOutputFolder(os.path.join(folder, "out"))

    # file -> table: default name from the file, or the given one
    assert a.importFiles("a.dat") == ["a"], tableNames()
    assert a.importFiles("a.dat", "ApiGiven") == ["ApiGiven"]
    t = table("ApiGiven")
    assert (t.numRows(), t.cell(1, 1), sum(t.colData(2))) == (5, 0.01, 30), (t.numRows(), t.cell(1, 1))
    assert sorted(a.importFiles(os.path.join(folder, "*.dat"))) == ["a", "b"]
    assert a.importFiles("a.dat", ["ApiMixed"]) == ["ApiMixed"]  # one source as a string, its name in a list
    assert a.importFiles(("a.dat",), "ApiTuple") == ["ApiTuple"]

    # table -> file and table -> table, file -> file
    written = a.exportTables("ApiGiven", "given.RAD")
    assert written == [os.path.join(folder, "out", "given.RAD")] and os.path.isfile(written[0]), written
    written = a.exportTables("ApiGiven", "noext")  # a given name without an extension gets extension()
    assert written == [os.path.join(folder, "out", "noext.RAD")], written
    assert a.convertTables("ApiGiven", "ApiCopy") == ["ApiCopy"] and table("ApiCopy").numRows() == 5
    a.setExtension("cut")
    files = a.convertFiles(["a.dat", "b.dat"])
    assert [os.path.basename(f) for f in files] == ["a.cut", "b.cut"], files

    # options: removing points; a dataset left empty is skipped and printed, the others are made
    a.setRemovePoints(first=3)
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        made = a.importFiles(["a.dat", "b.dat"], ["ApiCutA", "ApiCutB"])
    assert made == ["ApiCutA"] and table("ApiCutA").numRows() == 2, made
    assert "b.dat" in out.getvalue() and "no data" in out.getvalue(), out.getvalue()
    assert any("b.dat" in m for m in a.lastMessages()), a.lastMessages()
    raises(lambda: a.importFiles("b.dat"), "no data")  # nothing made: raises
    a.reset()

    # the resolution sigma: the same value as resolutionSigma()
    a.setResolution(True)
    a.setResolutionGeometry(800, 400, 0.525, 10, 10, 30, 30, 6, 0.1)
    t = table(a.importFiles(os.path.join(folder, "a.dat"), "ApiSigma")[0])
    assert t.numCols() == 4 and abs(t.cell(4, 1) - a.resolutionSigma(0.01)) < 1e-12, (t.numCols(), t.cell(4, 1))
    a.reset()

    # what was asked but could not be done is printed, not skipped silently
    short = newTable("ApiShort", 2, 2)
    short.setColData(1, [1, 2])
    short.setColData(2, [1, 1])
    a.setCalculator("-", "ApiShort")
    made = a.convertTables("ApiGiven", "ApiCalc")
    assert table("ApiCalc").cell(2, 1) == 2 and any("not applied" in m for m in a.lastMessages()), a.lastMessages()
    a.reset()
    for w in windows():
        if w.metaObject().className() == "MultiLayer":
            closeWindow(w)
    a.setPlot(True)
    a.convertTables("ApiGiven", "ApiPlot")
    assert any("no active graph" in m for m in a.lastMessages()), a.lastMessages()
    a.reset()
    a.setHeader("3")
    a.setFormat("ascii+header")  # the same format: the header stays
    assert a.header() == "3", a.header()
    a.reset()

    # a saved format restores the options
    a.setFormat("nist")
    a.setColumns(3)
    assert a.saveFormat("QtisasTestAscii1dApi") and "QtisasTestAscii1dApi" in a.formats()
    a.reset()
    assert a.loadFormat("QtisasTestAscii1dApi") and (a.format(), a.columns()) == ("nist", 3), (a.format(), a.columns())
    os.remove(os.path.join(os.path.expanduser("~"), ".config", "qtisas", "ascii1dFormats",
                           "QtisasTestAscii1dApi.ASCII1D"))
    a.reset()

    # every invalid input raises ValueError and names the fix
    raises(lambda: a.setFormat("csv"), "ascii", "cansas", "nist")
    raises(lambda: a.setColumns(7), "2 to 4")
    raises(lambda: a.setHeader(""), "header")
    raises(lambda: a.setNameNumber(12), "0 (the file name)")
    raises(lambda: a.setRemovePoints(first=-1), "0 or more")
    raises(lambda: a.setPresentation("Kratki"), "Kratky")
    raises(lambda: a.setMath("^", 2), '"+"')
    raises(lambda: a.setCalculator("-", "NoSuchTable"), "Y column")
    raises(lambda: a.setBinning(0), "1 (off)")
    raises(lambda: a.setBinning(1, 3.0), "ratio")
    raises(lambda: a.setResolutionGeometry(0, 400), "> 0")
    raises(lambda: a.setResolutionGeometry(8, 4), "cm, not m", "800")  # metres given
    assert a.setResolutionGeometry(800, 400, pixelCm=0.525, lambdaA=6)  # optional ones by their unit-carrying names
    assert a.setHeader(5) and a.header() == "5"  # a number of lines as an int
    raises(lambda: a.importFiles("nope.dat"), "no such file", "nope.dat")
    raises(lambda: a.importFiles(folder), "a folder")
    raises(lambda: a.exportTables("NoSuch*"), "no table matches")
    raises(lambda: a.importFiles("ApiGiven"), "is a table of the project", 'convertTables("ApiGiven")')  # a table
    raises(lambda: a.convertTables("a.dat"), "is a file, not a table", 'importFiles("a.dat")')  # a file
    raises(lambda: a.importFiles(["a.dat"], ["x", "y"]), "at most one name per source")
    raises(lambda: a.loadFormat("NoSuchFormat"), "no format")
    with open(os.path.join(folder, "c.xml"), "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0"?>\n<SASroot><SASentry><SASdata>\n<Idata><Q>0.1</Q><I>5</I></Idata>\n'
                '</SASdata></SASentry></SASroot>\n')
    raises(lambda: a.importFiles("c.xml"), "canSAS XML", 'setFormat("cansas")')  # an XML file read as ASCII
    a.setFormat("cansas")
    assert a.importFiles("c.xml", "ApiXml") == ["ApiXml"] and table("ApiXml").numRows() == 1
    a.reset()
    raises(lambda: a.convertTables("ApiGiven"), "its source", "setIndexing(True)")  # would refill ApiGiven itself
    try:
        a.importFiles(5)
    except TypeError as e:
        assert "a string or a list of strings" in str(e), str(e)
    else:
        raise AssertionError("no TypeError for a number")
finally:
    shutil.rmtree(folder, ignore_errors=True)
    a.reset()
