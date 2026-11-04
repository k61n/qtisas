"""Auto mode adds the runs of a sample measured several times in one configuration (skipped without the qtisas.com
example files), on a copy of the SANS-1 runs (two Ebeam runs measured twice each, not one after the other):

- Dan.Processing.autoAdd(): the new runs in the input folder (the first run's number + the count), their single runs
  moved to the sub-folder "added-sources", which is no run folder (also with "Include Sub-Folders"); a sample
  measured once stays; again: nothing to add;
- "Only consecutive runs": only runs one after the other are added (here none);
- the auto option autoAdd (off by default): autoConfig() adds first, the configurations then use the added runs.
"""

import os
import shutil
import sys
import tempfile

try:
    from PyQt6.QtWidgets import QCheckBox
except ImportError:
    from PyQt5.QtWidgets import QCheckBox

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

p = Dan.Processing
sans1 = dr.data_folder("sans1", "D*.001")


def copy():
    work = tempfile.mkdtemp()
    for f in os.listdir(sans1):
        if os.path.isfile(os.path.join(sans1, f)):
            shutil.copy(os.path.join(sans1, f), work)
    return work


GROUPS = [["497587", "497879"], ["497588", "497860"]]
work = copy()
try:
    dr.new_session("SANS1", work)
    Dan.findChild(QCheckBox, "checkBoxDirsIndir").setChecked(True)  # sub-folders are run folders
    assert [g["runs"] for g in Dan.Rawdata.sampleGroups()] == GROUPS
    runs = Dan.Rawdata.runs()
    files = Dan.Rawdata.files([r for g in GROUPS for r in g])
    assert p.autoAdd() == ["49758700002", "49758800002"]
    sources = os.path.join(work, "added-sources")
    assert sorted(os.listdir(sources)) == sorted(os.path.basename(f) for f in files)
    assert not any(os.path.exists(f) for f in files)
    single = {r for g in GROUPS for r in g}
    after = Dan.Rawdata.runs()
    assert set(after) == set(runs) - single | {"49758700002", "49758800002"}, after  # none of added-sources
    added = Dan.Rawdata.info(["49758700002"])[0]
    assert added["sample"] == "Ebeam" and added["added"] == GROUPS[0], added
    assert p.autoAdd() == []  # nothing left to add
finally:
    shutil.rmtree(work)

# "Only consecutive runs": the Ebeam runs are not one after the other
work = copy()
try:
    dr.new_session("SANS1", work)
    Dan.findChild(QCheckBox, "checkBoxAddConsecutive").setChecked(True)
    assert p.autoAdd() == [] and not os.path.exists(os.path.join(work, "added-sources"))
    Dan.findChild(QCheckBox, "checkBoxAddConsecutive").setChecked(False)
finally:
    shutil.rmtree(work)

# the auto option: autoConfig() adds first
work = copy()
try:
    dr.new_session("SANS1", work)
    assert Dan.Options.Config.autoOptions()["autoAdd"] is False
    Dan.Options.Config.setAutoOptions({"autoAdd": True})
    assert p.autoConfig("auto-added") == "auto-added", p.lastError()
    assert len(os.listdir(os.path.join(work, "added-sources"))) == 4
    runs = set(Dan.Rawdata.runs())
    assert {"49758700002", "49758800002"} <= runs and not {r for g in GROUPS for r in g} & runs, runs
finally:
    shutil.rmtree(work)
