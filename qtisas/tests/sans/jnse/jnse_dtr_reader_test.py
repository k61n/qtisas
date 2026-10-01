"""JNSE .dtr reader: datasets are read whatever the blank lines and markers between them, and format errors are
reported without a crash, a hang or a half-filled header row.

Files written to a temporary folder (each read with "Read NSE data", answering its dialogs):
  good.dtr  - 3 datasets: leading blank lines, "#nxt" with and without a blank line before it, a column-title
              line and a cut data line (fewer than 7 values) among the data, and the last data line as the last
              line of the file, without a newline
  bad.dtr   - 1 good dataset, then one with a broken header line: only the first is read, the error names the
              file and the line, and the header table gets no row for the broken one
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtCore import QMetaObject
    from PyQt6.QtWidgets import QApplication
except ImportError:
    from PyQt5.QtCore import QMetaObject
    from PyQt5.QtWidgets import QApplication

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import accept_file, accept_text, answering, locale_c, set_app_locale, use_qt_dialogs

use_qt_dialogs()
set_app_locale(locale_c())

KEYS = ["q", "q_var", "q_unit_SI", "t_unit_SI", "temp", "numor1", "ref1", "bgr1", "withbgr", "tfac_bgr", "volfrac",
        "qbins", "tbins", "oor_cnt", "upd_cnt"]


def dataset(name, run, q, rows, broken=False):
    lines = ['%s "info of %s"' % (name, name), "%s fqt vs t/ns %d" % (name, run), "", "parameter"]
    for key in KEYS:
        lines.append("%-10s %r" % ("qq" if broken and key == "temp" else key, q if key == "q" else 1.0))
    lines += ["", "values", "  tau/ns  S(q,t)/S(q)  err  var  var(tau)  nupdates  Qeff"]
    lines += ["  " + "  ".join(repr(v) for v in row) for row in rows]
    return lines


row = lambda tau: [tau, 0.9, 0.01, 0.2, 0.001, 96.0, 0.05]
good = ["", ""] + dataset("A", 1, 0.05, [row(0.1), row(0.2)])
good += ["", "#nxt"] + dataset("B", 2, 0.06, [row(0.3), [0.4, 0.8], row(0.5)])  # a cut line among the data
good += ["#nxt"] + dataset("C", 3, 0.07, [row(0.6), row(0.7)])  # no blank line before "#nxt"
bad = dataset("D", 4, 0.08, [row(0.8)]) + ["", "#nxt"] + dataset("E", 5, 0.09, [row(0.9)], broken=True) + ["#eod"]

folder = tempfile.mkdtemp()
files = {"good.dtr": "\n".join(good), "bad.dtr": "\n".join(bad) + "\n"}  # good.dtr: no newline at the end
for name, text in files.items():
    with open(os.path.join(folder, name), "w") as f:
        f.write(text)

jnse = [w for w in QApplication.allWidgets() if w.metaObject().className() == "jnse18"][0]
messages = []
for file_name in ("good.dtr", "bad.dtr"):
    with answering(accept_text("JNSEreader"), accept_file(folder, file_name)) as shown:
        QMetaObject.invokeMethod(jnse, "readNseData")
    messages += shown

header = table("JNSEreader")
samples = [header.text(1, r).split()[0] for r in range(1, header.numRows() + 1)]
assert samples == ["A", "B", "C", "D"], samples  # E is broken: no row for it
runs = [header.text(4, r) for r in range(1, 5)]
assert runs == ["1", "2", "3", "4"], runs
q = [header.cellOrNone(5, r) for r in range(1, 5)]
assert q == [0.05, 0.06, 0.07, 0.08], q
assert header.text(2, 1) == "info of A", header.text(2, 1)

taus = []
for r in range(1, 5):
    data = table(header.text(20, r))
    taus.append([data.cellOrNone(1, k) for k in range(1, data.numRows() + 1)])
assert taus == [[0.1, 0.2], [0.3, 0.5], [0.6, 0.7], [0.8]], taus  # the cut line skipped, the last line read

assert len(messages) == 1 and "bad.dtr" in messages[0], messages

shutil.rmtree(folder)
