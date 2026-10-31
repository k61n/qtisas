"""Dan.Rawdata.headerKeys(), headerValue() and setHeaderValue(): a header value read and written where the instrument's
header map reads it, on copies of runs:
- HDF5/NeXus (data/100001.nxs, in the repository: a SANS run of ILL-SAM-init's header map, its detector all ones, no
  names): Polarization written shorter and longer than its fixed-length string ("Out" -> "0" -> "down-down"), a number
  (Lambda); the other values as they were; numeric arrays (entry0/arrays: 1D, 2D, 3D, integer and float) read
  with [sum], [mean], [sum][sum], [last], [i,j] — a detector total as Sum;
- EDF (data/synthetic_0_00024.edf, in the repository: a KWS-X run of KWSX-init's Flexi header map, 16 x 16 ones, no
  names): the header block keeps its 4096 bytes, the image its bytes;
- XML (data/000001.xml, in the repository: a canSAS 1D file of ILL's example, three data points, no names; KWS1-2020
  with an XML header map set by setParameters()): only the elements' text changes (escaped; <SASnote/> filled), an
  element with elements in it refused;
the qtisas.com/dan-sans example runs (that part skipped without them):
- KWS-1 (ASCII: line + number, line + token, Flexi "{lambda=}"): only that line changes, with backup=True <file>.bak
  keeps the original (no backup by default), info() reads the new value (the headers DAN keeps are read again);
- KWS-3 (YAML: a scalar, an element of a device list, a text) and SANS-1 (Flexi "{SampleName=}", "{Time=}");
- refused: an unknown key (a misspelled one named with the nearest key), a run that is not there, the run number, a
  constant of the instrument, an empty list.
"""

import filecmp
import glob
import io
import os
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dan_reduction as dr

r = Dan.Rawdata


def raises(call, text):
    try:
        call()
    except ValueError as e:
        assert text in str(e), (text, str(e))
        return
    raise AssertionError("no ValueError: " + text)


def copy(folder, pattern, target):
    for f in glob.glob(os.path.join(folder, pattern)):
        shutil.copy(f, target)


def lines(path):
    with io.open(path, "rb") as f:
        return f.read().split(b"\n")


def same(a, b):
    return abs(float(a) - float(b)) <= 1e-9 * max(1.0, abs(float(b)))


work = tempfile.mkdtemp()
try:
    # --- HDF5/NeXus: the synthetic run
    folder = os.path.join(work, "nxs")
    os.mkdir(folder)
    copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data"), "100001.nxs", folder)
    dr.new_session("ILL-SAM-init", folder)
    keys = ("Sample-Title", "Duration", "D", "Lambda", "Sample-Thickness", "Comment1")
    before = {key: r.headerValue("100001", key) for key in keys}
    assert r.headerValue("100001", "Polarization") == "Out" and r.info(["100001"])[0]["polarization"] == "Out"
    (path,) = r.setHeaderValue("100001", "Polarization", "0")
    assert r.headerValue("100001", "Polarization") == "0" and r.info(["100001"])[0]["polarization"] == "0"
    assert not os.path.exists(path + ".bak")  # no backup by default
    r.setHeaderValue("100001", "Polarization", "down-down")  # longer than the dataset's 3 characters
    assert r.headerValue("100001", "Polarization") == "down-down"
    r.setHeaderValue("100001", "Lambda", 6)
    assert same(r.headerValue("100001", "Lambda"), 6) and same(r.info(["100001"])[0]["lambda"], 6)
    assert {key: r.headerValue("100001", key) for key in keys if key != "Lambda"} == \
           {key: value for key, value in before.items() if key != "Lambda"}
    # numeric arrays reduced by the action (before: a gate refused [sum] / [mean] on any array, -1; 2D and 3D arrays
    # were read into arrays of row pointers): a detector's total ([sum][sum]) as Sum
    arrays = {
        "Sum": ("counts2d;;;[sum][sum]", 49146), "Field-1": ("counts2d;;;[mean][mean]", 2.99963),  # 6 digits
        "Field-2": ("float2d;;;[sum][sum]", 16), "Field-3": ("counts1d;;;[sum]", 15),
        "Field-4": ("float1d;;;[mean]", 1.5),
        "Sample-Motor-1": ("counts1d;;;[last]", 5), "Sample-Motor-2": ("counts3d;;;[sum][sum]", 24),
        "Sample-Motor-3": ("counts3d;;;[sum]", 12), "Sample-Motor-4": ("counts2d;;;[1,2]", 130 % 7)}
    Dan.Options.Config.setParameters({key: "entry0/arrays/" + entry for key, (entry, _) in arrays.items()})
    Dan.Options.setInputPath(folder)
    for key, (entry, value) in arrays.items():
        assert same(r.headerValue("100001", key), value), (key, entry, r.headerValue("100001", key), value)
    assert same(r.info(["100001"])[0]["counts"], 49146), r.info(["100001"])[0]

    # a run file HDF5 cannot open (here no HDF5 at all; also a file locked while another process writes it): read as
    # a missing entry (before: HDF5's exception ended QtiSAS, in runs() reading the sample names)
    with io.open(os.path.join(folder, "100002.nxs"), "wb") as f:
        f.write(b"not an HDF5 file\n" * 64)
    assert "100002" in r.runs() and "100001" in r.runs(), r.runs()
    r.headerValue("100002", "Sample-Title")

    # --- EDF: the synthetic run
    folder = os.path.join(work, "edf")
    os.mkdir(folder)
    copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data"), "synthetic_0_00024.edf", folder)
    dr.new_session("KWSX-init", folder)
    (path,) = r.files(["00024"])
    with io.open(path, "rb") as f:
        original = f.read()
    r.setHeaderValue("00024", "Duration", 1234)
    r.setHeaderValue("00024", "Sample-Title", "Polymer-X_T30C")
    r.setHeaderValue("00024", "D", 2.5)
    assert r.headerValue("00024", "Duration") == "1234" and r.headerValue("00024", "D") == "2.5"
    info = r.info(["00024"])[0]
    assert info["sample"] == "Polymer-X_T30C" and same(info["duration"], 1234), info
    with io.open(path, "rb") as f:
        written = f.read()
    assert len(written) == len(original) and written[4096:] == original[4096:] and written[4094:4096] == b"}\n"
    assert b"EDF_HeaderSize = 4096 ;" in written[:4096] and b"Comment = Polymer-X_T30C ;" in written[:4096]

    # --- XML: the synthetic canSAS file, its header map set on a built-in instrument
    folder = os.path.join(work, "xml")
    os.mkdir(folder)
    copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "data"), "000001.xml", folder)
    dr.new_session("KWS1-2020", folder)
    Dan.Options.Config.setParameters({
        "DataFormat": "1", "Pattern": "#.xml", "Pattern-Select-Data": "*.xml", "2nd-Header-OK": "No",
        "XML-base": "SASroot:SASentry:", "Experiment-Title": "Title;;;", "Sample-Title": "SASsample:ID;;;",
        "Sample-Thickness": "SASsample:thickness;;;", "Lambda": "SASinstrument:SASsource:wavelength;;;",
        "D": "SASinstrument:SASdetector:SDD;;;", "Comment1": "SASnote;;;", "Comment2": "SASsample:position;;;"})
    Dan.Options.setInputPath(folder)
    (path,) = r.files(["000001"])
    original = lines(path)
    assert r.headerValue("000001", "Sample-Title") == "S1 2mm" and r.headerValue("000001", "Lambda") == "8.00"
    r.setHeaderValue("000001", "Sample-Title", "Polymer <B> & C")
    r.setHeaderValue("000001", "Lambda", 6)
    r.setHeaderValue("000001", "Comment1", "a note")  # <SASnote/>
    assert r.headerValue("000001", "Sample-Title") == "Polymer <B> & C" and r.headerValue("000001", "Lambda") == "6"
    assert r.headerValue("000001", "Comment1") == "a note" and same(r.info(["000001"])[0]["lambda"], 6)
    raises(lambda: r.setHeaderValue("000001", "Comment2", "x"), "not found or not writable")  # elements in it
    written = lines(path)
    changed = [(a, b) for a, b in zip(original, written) if a != b]
    assert len(written) == len(original) and changed == [
        (b"\t\t\t<ID>S1 2mm</ID>", b"\t\t\t<ID>Polymer &lt;B&gt; &amp; C</ID>"),
        (b'\t\t\t\t<wavelength unit="A">8.00</wavelength>', b'\t\t\t\t<wavelength unit="A">6</wavelength>'),
        (b"\t\t<SASnote/>", b"\t\t<SASnote>a note</SASnote>")], changed
finally:
    shutil.rmtree(work)

kws1, kws3, sans1 = dr.data_folder("kws1", "*.DAT"), dr.data_folder("kws3", "*.yaml"), dr.data_folder("sans1", "D*.001")
work = tempfile.mkdtemp()
try:
    # --- KWS-1: ASCII
    folder = os.path.join(work, "kws1")
    os.mkdir(folder)
    copy(kws1, "*5321[78]_*.DAT", folder)
    dr.new_session("KWS1-2020", folder)
    assert {"Sample-Title", "Duration", "Lambda", "Attenuator"} <= set(r.headerKeys()), r.headerKeys()
    (path,) = r.files(["53217"])
    before = lines(path)
    duration = r.info(["53217"])[0]["duration"]
    assert r.headerValue("53217", "Duration") == "1000"

    assert r.setHeaderValue("53217", "Duration", 1500, backup=True) == [path]
    assert r.headerValue("53217", "[Duration]") == "1500"
    assert filecmp.cmp(path + ".bak", os.path.join(kws1, os.path.basename(path)), shallow=False)
    after = lines(path)
    changed = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert len(after) == len(before) and len(changed) == 1, changed
    assert after[changed[0]] == before[changed[0]].replace(b"1000", b"1500", 1), (before[changed[0]], after[changed[0]])
    assert same(r.info(["53217"])[0]["duration"], 1.5 * duration), (r.info(["53217"])[0]["duration"], duration)

    r.setHeaderValue("53217", "lambda", "5.5")  # Flexi "{lambda=}", case ignored in the key
    assert same(r.headerValue("53217", "Lambda"), 5.5)
    r.setHeaderValue("53217", "Sample-Title", "Polymer-A")  # a token of its line
    assert r.headerValue("53217", "Sample-Title").strip() == "Polymer-A"
    assert r.info(["53217"])[0]["sample"].startswith("Polymer-A"), r.info(["53217"])[0]["sample"]

    files = r.setHeaderValue(["53217", "53218"], "Sample-Thickness", 0.2)
    assert len(files) == 2 and not os.path.exists(files[1] + ".bak"), files
    assert all(same(r.headerValue(run, "Sample-Thickness"), 0.2) for run in ("53217", "53218"))

    raises(lambda: r.setHeaderValue("53217", "Nope", 1), "no header key Nope")
    # a misspelled key: named with the nearest key, never taken for it
    raises(lambda: r.setHeaderValue("53217", "Sample-tickness", 0.1), "no header key Sample-tickness (did you mean "
           "Sample-Thickness?)")
    raises(lambda: r.headerValue("53217", "Sample-tickness"), "(did you mean Sample-Thickness?)")
    raises(lambda: r.newHeaderTable("h", ["Thicknes"]), "no header item Thicknes (did you mean Thickness?)")
    # an empty list (a filter that matched none): said so, not just "no runs"
    raises(lambda: r.setHeaderValue([], "Duration", 1), "no runs: the list is empty, nothing to do")
    raises(lambda: r.setHeaderValue(["53217", "99999"], "Duration", 1), "no run 99999")
    raises(lambda: r.setHeaderValue("53217", "Sample-Run-Number", 1), "the run number")
    raises(lambda: r.setHeaderValue("53217", "Attenuator", "x10"), "a constant of the instrument")
    raises(lambda: r.headerValue("53217", "Nope"), "no header key Nope")
    assert r.headerValue("53217", "Duration") == "1500"  # the refused calls changed nothing

    # --- KWS-3: YAML
    folder = os.path.join(work, "kws3")
    os.mkdir(folder)
    copy(kws3, "00082557_*", folder)
    dr.new_session("KWS3-2020", folder)
    run = "00082557"
    r.setHeaderValue(run, "Sample-Thickness", 0.15)
    r.setHeaderValue(run, "Lambda", 6.0)  # an element of the device list
    r.setHeaderValue(run, "Sample-Title", "Polymer-B")
    assert same(r.headerValue(run, "Sample-Thickness"), 0.15) and same(r.headerValue(run, "Lambda"), 6.0)
    assert r.headerValue(run, "Sample-Title") == "Polymer-B" and same(r.headerValue(run, "Duration"), 2000)
    assert same(r.info([run])[0]["lambda"], 6.0), r.info([run])[0]

    # --- SANS-1: Flexi
    folder = os.path.join(work, "sans1")
    os.mkdir(folder)
    copy(sans1, "D0497589.001", folder)
    dr.new_session("SANS1", folder)
    r.setHeaderValue("497589", "Sample-Title", "Polymer-C")
    r.setHeaderValue("497589", "Duration", 150)
    assert r.headerValue("497589", "Sample-Title") == "Polymer-C" and r.headerValue("497589", "Duration") == "150"
finally:
    shutil.rmtree(work)
