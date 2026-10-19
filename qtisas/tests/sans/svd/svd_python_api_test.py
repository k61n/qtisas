"""The SVD module's Python interface (Svd): input checks that name the fix, the decomposition of the qtisas.com/svd
series, match points, the rank of a series and the errors a contrast plan would give.

Svd is global, set by calls (not by the interface's widgets). SLDs are in cm^-2, D2O fractions from 0 to 1; a wrong
unit or a percentage is refused, not used. decompose() returns the table it made.
"""

import math


def raises(call, error, text):
    try:
        call()
    except error as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no %s: %s" % (error.__name__, text))


H, D = -5.61e9, 6.335e10
components = [("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)]
fractions = [0.0, 0.34, 0.46, 0.8, 1.0]
q = [0.005 * k for k in range(1, 41)]
S11 = [1 / (1 + (50 * v) ** 2) for v in q]
S12 = [0.3 * math.exp(-((20 * v) ** 2)) for v in q]
S22 = [math.exp(-((30 * v) ** 2)) for v in q]
for x in fractions:
    solvent = H + x * (D - H)
    a, b = (h + x * (d - h) - solvent for _, h, d in components)
    intensity = [a * a * s11 + 2 * a * b * s12 + b * b * s22 for s11, s12, s22 in zip(S11, S12, S22)]
    t = newTable("apiMix%d" % round(100 * x), len(q), 3)
    t.setColData(1, q)
    t.setColData(2, intensity)
    t.setColData(3, [0.01 * i for i in intensity])
tables = ["apiMix%d" % round(100 * x) for x in fractions]

# nothing set: the error names the calls
Svd.reset()
raises(lambda: Svd.decompose("apiNone"), ValueError, "setSolvent(sldH2Ocm2, sldD2Ocm2)")
assert not existTable("apiNone")

# SLDs in cm^-2: A^-2 (or its 1e-6 number) refused
raises(lambda: Svd.setSolvent(-0.561, 6.335), ValueError, "1e-6 A^-2 = 1e10 cm^-2")
raises(lambda: Svd.setSolvent(-5.61e-7, 6.335e-6), ValueError, "cm^-2")
Svd.setSolvent(H, D)
raises(lambda: Svd.setComponents([("a", 1.48, 2.374)]), ValueError, "a in H2O")
raises(lambda: Svd.setComponents([("a", 1.48e10)]), TypeError, "(name, sldH2Ocm2, sldD2Ocm2)")
Svd.setComponents(components)

# fractions from 0 to 1, tables that exist
raises(lambda: Svd.setContrasts([("apiMix34", 34)]), ValueError, "34% is 0.34")
raises(lambda: Svd.setContrasts([("apiMissing", 0.5)]), ValueError, "no table apiMissing")
raises(lambda: Svd.setContrasts(["apiMix0"]), TypeError, "(table, d2oFraction)")

# too few contrasts for two components: no table, the reason
Svd.setContrasts([("apiMix0", 0.0), ("apiMix100", 1.0)])
raises(lambda: Svd.decompose("apiTwo"), ValueError, "at least 3 H/D contrasts")
assert not existTable("apiTwo")

# the series: the table returned, all shared Q points, S_ij of the data
Svd.setContrasts(list(zip(tables, fractions)))
name = Svd.decompose("apiSVD")
assert name == "apiSVD", name
t = table(name)
col = lambda label: t.colNames().index(label) + 1
assert t.numRows() == 40 and t.cellOrNone(col("Q1"), 40) is not None
for label, expected in (("S11", S11), ("S12", S12), ("S22", S22)):
    for row in (1, 10, 40):
        got = t.cell(col(label), row)
        assert abs(got - expected[row - 1]) <= 1e-6 * abs(expected[row - 1]) + 1e-12, (label, row, got)
assert abs(t.cell(col("SLDsolvent"), 2) - (H + 0.34 * (D - H))) < 1, t.cell(col("SLDsolvent"), 2)
assert Svd.decompose("apiSVD") == "apiSVD1"  # a taken name gets a number of its own, returned
assert Svd.decompose("apiSVD10", 10) == "apiSVD10" and table("apiSVD10").numRows() == 10

# a measured solvent SLD per contrast is used instead of the fraction's
Svd.setContrasts([(n, x, H + x * (D - H)) for n, x in zip(tables, fractions)])
assert abs(table(Svd.decompose("apiSVDsld")).cell(col("S12"), 10) - S12[9]) < 1e-6

# match points: the fraction, a note outside 0..1
assert abs(Svd.matchPoint(1.48e10, 2.374e10) - 0.340053) < 1e-6
assert Svd.matchPoint(8e10, 8e10) > 1
raises(lambda: Svd.matchPoint(H, D), ValueError, "no match point")

# rank: three combinations from any H/D series
assert Svd.rank([0, 0.5, 1]) == 3
assert Svd.rank([0, 0.5, 0.5]) == 2
assert Svd.rank(0.5) == 1
Svd.setComponents(components + [("c", 4.0e10, 4.0e10)])
assert Svd.rank([m / 8 for m in range(9)]) == 3
raises(lambda: Svd.rank([0, 50]), ValueError, "34% is 0.34")
Svd.setComponents(components)

# plan: dS/S for the partial structure factors at one Q, intensity errors relError I + background I(D2O)
partials = [S11[9], S12[9], S22[9]]
errors = Svd.plan([0, 0.5, 1], partials, 0.01, 0.01)
assert len(errors) == 3 and abs(errors[1] - 1.109) < 1e-3, errors
with_background = [Svd.plan(p, partials, 0.01, 0.01)[1] for p in
                   ([0, 0.25, 0.5, 0.75, 1], fractions, [0.2, 0.3, 0.4, 0.5, 0.6])]
relative_only = [Svd.plan(p, partials)[1] for p in ([0, 0.25, 0.5, 0.75, 1], fractions, [0.2, 0.3, 0.4, 0.5, 0.6])]
assert min(range(3), key=lambda i: with_background[i]) == 1, with_background
assert min(range(3), key=lambda i: relative_only[i]) == 2, relative_only
assert Svd.plan([0, 0.5, 0.5], partials)[1] == math.inf  # S12 not determined by two fractions
raises(lambda: Svd.plan([0, 0.5, 1], [1.0, 2.0]), ValueError, "the 3 partial structure factors")

# an SVD table read back: its solvent, components and contrasts, ready for a decomposition of a subset
Svd.reset()
raises(lambda: Svd.decompose("apiNone"), ValueError, "not set: the solvent")
raises(lambda: Svd.readTable("apiMix0"), ValueError, "not an SVD table")
Svd.readTable("apiSVD")
assert Svd.components() == ["a", "b"], Svd.components()
assert [c[:2] for c in Svd.contrasts()] == list(zip(tables, fractions)), Svd.contrasts()
assert all(abs(c[2] - (H + c[1] * (D - H))) < 1 for c in Svd.contrasts()), Svd.contrasts()  # the solvent SLD
Svd.setContrasts([c for c in Svd.contrasts() if c[0] != "apiMix34"])
t4 = table(Svd.decompose("apiSVD4"))
assert abs(t4.cell(t4.colNames().index("S12") + 1, 10) - S12[9]) < 1e-6
assert t4.colNames().count("Icon4") == 1 and "Icon5" not in t4.colNames()
Svd.reset()
Svd.setSolvent(H, D)
raises(lambda: Svd.decompose("apiNone"), ValueError, "not set: the components")

# value(): by column label and Q, an error naming the nearest Q or the columns
assert abs(Svd.value("apiSVD", "S12", 0.05) - S12[9]) < 1e-6
assert abs(Svd.value("apiSVD", "Icon3", 0.05) / table("apiMix46").cell(2, 10) - 1) < 1e-9
raises(lambda: Svd.value("apiSVD", "S12", 0.0512), ValueError, "the nearest is 0.05 (row 10)")
raises(lambda: Svd.value("apiSVD", "S99", 0.05), ValueError, "has no column S99")
raises(lambda: Svd.value("apiMix0", "S12", 0.05), ValueError, "not an SVD table")

# an SLD with a negative exponent: the value it was meant to be
raises(lambda: Svd.setSolvent(-5.61e-9, 6.335e10), ValueError, "in cm^-2 it is -5.61e+09")
raises(lambda: Svd.setSolvent(-5.61e9, 6.335e-10), ValueError, "in cm^-2 it is 6.335e+10")

# value(): only the columns of Q points; qValues(): the Q points
raises(lambda: Svd.value("apiSVD", "SLDsolvent", 0.05), ValueError, "not a column of Q points")
raises(lambda: Svd.value("apiSVD", "S", 0.05), ValueError, "not a column of Q points")
assert Svd.qValues("apiSVD") == table("apiSVD").colData(table("apiSVD").colNames().index("Q1") + 1)[:40]
assert len(Svd.qValues("apiSVD")) == 40 and len(Svd.qValues("apiSVD10")) == 10
raises(lambda: Svd.qValues("apiMix0"), ValueError, "not an SVD table")

# a measured solvent SLD survives readTable() + contrasts() + setContrasts()
Svd.reset()
Svd.setSolvent(H, D)
Svd.setComponents(components)
Svd.setContrasts([(n, x, H + x * (D - H) + 1e8) for n, x in zip(tables, fractions)])
measured = table(Svd.decompose("apiMeasured"))
Svd.reset()
Svd.readTable("apiMeasured")
Svd.setContrasts(Svd.contrasts())
again = table(Svd.decompose("apiMeasuredAgain"))
col = lambda t, label: t.colNames().index(label) + 1
assert abs(again.cell(col(again, "SLDsolvent"), 2) - (H + 0.34 * (D - H) + 1e8)) < 1
assert abs(again.cell(col(again, "S12"), 10) - measured.cell(col(measured, "S12"), 10)) < 1e-9
