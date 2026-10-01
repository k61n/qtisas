"""NIST StRD nonlinear regression check for FitTable: one CTest entry per function (see tests/CMakeLists.txt).

The function is given by the NIST_FUNCTION environment variable (e.g. "Bennett5" for NIST/Bennett5). What is tested:
  1. The data (x, y) and the certified results are read from the Info page of the function's .fif file in the
     FitTable library. NIST's two starting points are Start 1 = [initial values] and Start 2 from the Info page.
  2. From each start a Levenberg-Marquardt fit, first in C and then in German number format (FitTable's localized
     widgets), must reproduce every certified parameter, every certified standard deviation and the certified
     residual sum of squares, each to a relative TOLERANCE.
All four fits run even if one fails; the test then fails listing every mismatch. The fit's own console output is
silenced; a short report of each check goes to stderr (ctest -V). A function not in the library is skipped.
"""

import html
import os
import re
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import locale_c, locale_german, quiet, report, set_app_locale, skip

TOLERANCE = 1e-6  # relative, for parameters, standard deviations and the residual sum of squares
# Lanczos1 is generated data exact to 13 digits (RSS 1.4e-25): its certified RSS and standard deviations depend on
# residuals at the limit of double precision, so they are only reproduced to about 2e-3 (the parameters to 1e-6)
STATISTICS_TOLERANCE = {"NIST/Lanczos1": 1e-2}
# The Info pages of these two older functions do not list NIST's Start 2
START2 = {
    "NIST/Misra1a": [250.0, 5.0e-4],
    "NIST/Kirby2": [1.5, -0.15, 0.0025, -0.0015, 2.0e-5],
}


def read_fif(function):
    """x, y, certified {name: (value, sd)}, rss and the two starts from <library>/<function>.fif."""
    path = os.path.join(Fittable.Path, function + ".fif")
    if not os.path.isfile(path):
        skip("%s not found" % path)
    with open(path, encoding="utf-8") as f:
        text = f.read()
    info = text[text.index("[description]"):text.index("[x]")]
    lines = [html.unescape(re.sub(r"<[^>]+>", "", line)).strip() for line in info.splitlines()]
    lines = [line for line in lines if line]

    number = lambda s: float(s.replace(" ", ""))  # the Misra1a Info has "2. 3894212918E+02"
    start = lines.index("Data:")
    x_at, y_at, cert_at = lines.index("x", start), lines.index("y", start), lines.index("Certified Values:")
    x = [number(v) for v in lines[x_at + 1:y_at]]
    y = [number(v) for v in lines[y_at + 1:cert_at]]

    certified = {}
    for line in lines[cert_at + 1:]:
        match = re.match(r"β(\d+)\s+(.+?)\s+(\S+)$", line)
        if match:
            certified["beta" + match.group(1)] = (number(match.group(2)), number(match.group(3)))
    rss = number(re.search(r"Residual Sum of Squares:\s*(\S+)", "\n".join(lines)).group(1))

    start1 = [float(v.split("[")[0]) for v in text.split("[initial values]\n")[1].split("\n")[0].split(",")]
    match = re.search(r"Start 2:\s*(.*)", "\n".join(lines))
    start2 = [float(v) for v in re.findall(r"β\d+ = (\S+?)(?:,|$)", match.group(1))] if match else START2[function]
    return x, y, certified, rss, [start1, start2]


def relative(value, expected):
    return abs(value - expected) / abs(expected)


def fit(function, start, certified, rss, dof, label):
    """Mismatches of one fit; dof is N - p (NIST's Rat43 file certifies 9 for 15 points and 4 parameters)."""
    f = Fittable
    for i, value in enumerate(start):
        f.setParamValue("beta%d" % (i + 1), value)
    with quiet():
        chi2_dof = f.fit()
    results = f.results()

    problems, worst = [], 0.0
    statistics_tolerance = STATISTICS_TOLERANCE.get(function, TOLERANCE)
    checks = []
    for name, (value, sd) in certified.items():
        got, err = results[name]
        checks += [(name, got, value, TOLERANCE), ("sd(%s)" % name, err, sd, statistics_tolerance)]
    checks.append(("RSS", chi2_dof * dof, rss, statistics_tolerance))
    for what, got, expected, tolerance in checks:
        if got is None:
            problems.append("%s: %s is not available" % (label, what))
            continue
        diff = relative(got, expected)
        if diff > tolerance:
            problems.append("%s: %s = %r, certified %r" % (label, what, got, expected))
        elif tolerance == TOLERANCE:
            worst = max(worst, diff)
    report("  %-15s %s (largest relative difference %.1e <= %.0e%s)"
           % (label, "FAILED" if problems else "OK", worst, TOLERANCE,
              "; sd and RSS within %.0e" % statistics_tolerance if statistics_tolerance != TOLERANCE else ""))
    return problems


def run(function):
    f = Fittable
    if function not in f.fitFunctions(function):
        skip("FitTable function %r is not available" % function)
    x, y, certified, rss, starts = read_fif(function)
    assert len(x) == len(y) and all(len(s) == len(certified) for s in starts), (len(x), len(y), starts)
    dof = len(x) - len(certified)
    report("%s: %d points, %d parameters, certified RSS %g; LM fits from both NIST starts, in C and German"
           % (function, len(x), len(certified), rss), first=True)

    name = "NIST_" + function.split("/")[-1] + "_data"
    t = newTable(name, len(x), 2)
    t.setColData(1, x)
    t.setColData(2, y)

    f.configure(function, [name + "_2"])
    f.setFitMethod("LM")
    for p in certified:
        f.setParamLimits(p, float("-inf"), float("inf"))

    problems = []
    for number, start in enumerate(starts, 1):
        for label, locale in (("C", locale_c()), ("German", locale_german())):
            set_app_locale(locale)
            problems += fit(function, start, certified, rss, dof, "Start %d %s" % (number, label))
    assert not problems, function + ":\n  " + "\n  ".join(problems)


run("NIST/" + os.environ.get("NIST_FUNCTION", ""))
