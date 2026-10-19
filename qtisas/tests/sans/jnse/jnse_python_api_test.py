"""The JNSE module's Python interface (Jnse): .dtr import into a header table and dataset tables, reading them, and
the multi-fit set-up with its refusals.

Synthetic files like a real J-NSE report (CR line ends, six q, S(q,t)/S(q) = A exp(-D q^2 t)); the fit itself runs
when the library has simpleDiffusion (the NSE functions of qtisas.com/jnse), else that part is left out.
"""

import math
import os
import shutil
import tempfile


def raises(call, error, text):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


def write_dtr(path, sample, info, run, temp, D, A=0.95, qs=(0.03893043, 0.05502866, 0.07045429, 0.08793830,
              0.1034360, 0.1196200), taus=(0.5005917, 2.003166, 4.006066, 6.008912, 10.01473, 20.02899, 30.04368,
              40.05837, 50.07301, 60.08769, 70.10233, 80.11702)):
    """A J-NSE .dtr file like report19971_1.2.dtr (CR line ends, one dataset per q): S(q,t)/S(q) = A exp(-D q^2 t),
    err = 0.01 + 0.0002 t, q in 1/A, t in ns, D in A^2/ns."""
    def e(v):  # Fortran E15.7: 0.3893043E-01
        if v == 0:
            return "0.0000000E+00".rjust(15)
        m, x = ("%.6E" % abs(v)).split("E")
        digits = m.replace(".", "")
        return (("-" if v < 0 else "") + "0." + digits + "E%+03d" % (int(x) + 1)).rjust(15)
    lines = []
    for k, q in enumerate(qs):
        lines += ['%s "%s"' % (sample, info), "%s W  fqt   vs   t/ns  %d%02d" % (sample, run, k + 1), "", "parameter"]
        header = [("q", e(q)), ("q_var", e(0.0002)), ("q_unit_SI", e(1e10)), ("t_unit_SI", e(1e-9)),
                  ("temp", e(temp)), ("numor1", "%d" % run), ("ref1", "%d" % (run - 1)), ("bgr1", "%d" % (run + 5)),
                  ("withbgr", "1"), ("tfac_bgr", e(0.9)), ("volfrac", e(1.0)), ("qbins", "%d" % len(qs)),
                  ("tbins", "%d" % len(taus)), ("oor_cnt", "226"), ("upd_cnt", "3760")]
        lines += ["%-10s %s" % (key, value.strip() if "E" in value else value.rjust(8)) for key, value in header]
        lines += ["", "values",
                  "        tau/ns    S(q,t)/S(q)    err(sqt/sq)    var(sqt/sq)    var(tau)/ns   nupdates   Qeff/A**-1  "]
        for t in taus:
            s = A * math.exp(-D * q * q * t)
            lines.append("  %s%s%s%s%s%10d%s" % (e(t), e(s), e(0.01 + 0.0002 * t), e(0.1), e(0.0), 36, e(q)))
        lines += ["", "#nxt"]
    with open(os.path.expanduser(path), "w", newline="") as f:
        f.write("\r".join(lines) + "\r")


folder = tempfile.mkdtemp()
write_dtr(os.path.join(folder, "report90001_1.2.dtr"), "PEP100", "PEP 100k in d decane 4mm", 90001, 300.0, 3.0)
write_dtr(os.path.join(folder, "report90002_1.2.dtr"), "PEP100", "PEP 100k in d decane 4mm", 90002, 350.0, 4.5)
open(os.path.join(folder, "broken.dtr"), "w").write("PEP100 \"x\"\nnot a dtr file\n")
before = qti.app.activeFolder().name()

j = Jnse
j.reset()
raises(lambda: j.setInputFolder(os.path.join(folder, "missing")), ValueError, "no folder")
j.setInputFolder(folder)
raises(lambda: j.importFiles("missing.dtr"), ValueError, "looked for")
raises(lambda: j.importFiles("missing.dtr"), ValueError, "The .dtr files in")
raises(lambda: j.importFiles("broken.dtr"), ValueError, "no .dtr dataset read")
assert not existTable("broken-info")
newTable("plainTable", 3, 2)
raises(lambda: j.importFiles("report90001_1.2.dtr", "plainTable"), ValueError, "not a JNSE header table")

# one file: the header table (default name), six datasets, the script's folder kept
header = j.importFiles("report90001_1.2.dtr")
assert header == "report90001_1-2-info", header
assert qti.app.activeFolder().name() == before, qti.app.activeFolder().name()
qs = j.header(header, "q")
assert [round(q, 5) for q in qs] == [0.03893, 0.05503, 0.07045, 0.08794, 0.10344, 0.11962], qs
assert j.header(header, "temp") == [300.0] * 6 and j.header(header, "run-number")[0] == "9000101"
sets = j.datasets(header)
assert len(sets) == 6 and all(existTable(s) for s in sets), sets
t = table(sets[2])
assert list(t.colNames()) == ["tau-ns", "Sqt-To-Sq", "d-Sqt-To-Sq", "var-Sqt-To-Sq", "var-tau-ns", "nupdates", "Qeff"], t.colNames()
assert t.numRows() == 12
tau, s = t.cell(1, 5), t.cell(2, 5)
assert abs(s - 0.95 * math.exp(-3.0 * qs[2] ** 2 * tau)) < 1e-6, (tau, s)
raises(lambda: j.header(header, "Q"), ValueError, "has no column Q")
raises(lambda: j.datasets("plainTable"), ValueError, "not a JNSE header table")

# a header table named as a file: use it, no import
raises(lambda: j.importFiles(header + ".dtr"), ValueError, "is a JNSE header table of the project, not a file")

# a second file appended to the same header table; a wildcard
assert j.importFiles("report90002_*.dtr", header) == header
assert j.header(header, "temp") == [300.0] * 6 + [350.0] * 6

# the multi-fit set-up: refusals that name the fix
raises(lambda: j.initFit(header, "noSuchNseFunction"), ValueError, "compile it first")
raises(lambda: j.initFit(header, "noSuchNseFunction", 5, 3), ValueError, "")
raises(lambda: j.initFit("plainTable", "simpleDiffusion"), ValueError, "not a JNSE header table")

if "simpleDiffusion" in Fittable.fitFunctions():
    raises(lambda: j.initFit(header, "simpleDiffusion", 0, 3), ValueError, "are not rows of")
    assert j.initFit(header, "simpleDiffusion", 1, 6) == [s + "_Sqt-To-Sq" for s in sets]
    assert [round(Fittable.paramValue("q", k), 5) for k in range(1, 7)] == [round(q, 5) for q in qs]
    raises(lambda: j.fit(header, "simpleDiffusion", 1, 6, ["X"]), ValueError, "no parameter X to share")
    chi2 = j.fit(header, "simpleDiffusion", 1, 6, ["A", "D"], {"A": 1.0, "D": 1.0}, {"w": 0.0})
    assert chi2 < 1e-3, chi2
    assert abs(Fittable.paramValue("D") - 3.0) < 1e-3 and abs(Fittable.paramValue("A") - 0.95) < 1e-3
    assert "JNSE fit: simpleDiffusion of %s, rows 1..6" % header in resultsLog().toPlainText()

shutil.rmtree(folder)

# no name: the first file READ names the header table, not a file that held nothing
folder = tempfile.mkdtemp()
write_dtr(os.path.join(folder, "report90003_1.2.dtr"), "PEP100", "x", 90003, 300.0, 3.0)
open(os.path.join(folder, "a-broken.dtr"), "w").write("PEP100 \"x\"\nnot a dtr file\n")
j.setInputFolder(folder)
assert j.importFiles("*.dtr") == "report90003_1-2-info"
assert not existTable("a-broken-info")
shutil.rmtree(folder)
