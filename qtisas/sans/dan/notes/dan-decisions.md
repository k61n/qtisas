# DAN: decisions

One entry per decision, newest first. An entry is not rewritten when the decision changes: a new entry replaces
it and says so ("Replaces …"). Explanations and formulas are in the other notes of this folder:
[reduction and error propagation](dan-reduction-error-propagation.md),
[polarization reduction](dan-polarization-reduction.md),
[polarization absolute calibration](dan-polarization-absolute-calibration.md).

## 2026-10-29 — Polarization states out-up, out-down

- **Decision:** two states for polarizer out, analyzer up/down: `polarizedAlias()` −1 / −2 (`POL-ALIAS-OUT-UP`,
  `POL-ALIAS-OUT-DOWN`, matched before `down`/`up`); combo box positions 7/8; Pol-script table columns 11/12 (after
  the buffer columns; added to older tables on update); reduced only by "No Corrections" (`-OutUp`, `-OutDown`).
  A calibrant in these states: $T_{\rm pol}=1/\tau_A$ or $T_P/\tau_A$ with $\tau_A$ of its own run; sample rows
  without polarizer are state ≤ 0 in all $T_{\rm pol}$ cases.
- **Why:** transmissions (and calibrants) measured without polarizer but with the analyzer; negative numbers keep
  0..6 and every "> 0 = polarizer in" test unchanged.
- **With it:** `Dan.Processing.calculate()` presses the polarization row too (as the interface's corner button);
  `updateMaskNamesInScript` skips a row whose condition the configuration table no longer has (it crashed).
- **Not yet:** the transmission of these runs into x42 rows, the ³He decay in `readTransmission`.
- **Test:** `dan_polarization_alias_test.py`.

## 2026-10-29 — Header values: no backup by default

- **Decision:** `setHeaderValue(runs, key, value, backup=False)`: `<file>.bak` only when asked for (`backup=True`).
  An empty list of runs is an error that says so ("no runs: the list is empty, nothing to do"), in every
  `Dan.Rawdata`/`Dan.Processing` call taking runs. Replaces the default `backup=True` of 2026-10-28.
- **Why:** an AI request changed 349 runs and wrote 349 copies of the files unasked; the next one filtered the
  already changed runs, got an empty list and read "no runs" as a wrong call, twice.
- **Docs:** "all samples", "all files", "the folder" are `runs()`; `sampleRuns()` (without the standard runs) is
  for the script table of auto mode.

## 2026-10-29 — The sensitivity keeps its transmission and center; rows are checked against it

- **Decision:** the sensitivity's label ends with `::Center::x|y|mask` (the mask's beam stop, shown in the center
  fields) or `…|manual` (the checked center group's values); the reduction warns when a row is the sensitivity's
  own run with another transmission, or when its wide-angle corrections use another center (> 0.5 pixel). Only a
  warning: the reduction goes on.
- **Why:** H2O as sensitivity and as sample was not flat (0.1 %): the sensitivity was made with Tr 0.5191 (the
  calibrant formula T(λ)), the row had T 0.5161; with the same T it is flat to 2·10⁻⁹. A different center curves it
  the same way.
- **Kept:** the row's parallax and θ-transmission corrections use its mask's beam stop, as before (the radial
  averaging the row's center); the absolute calibration keeps the mask's center (a minor effect there).
- **Code:** `sensitivityLabelField`, `sensitivityCenter`, `checkRowAgainstSensitivity`, `danWarning`;
  [§4.3](dan-reduction-error-propagation.md#43-the-sensitivity-and-the-rows-reduced-with-it).
- **Test:** `dan_sensitivity_center_test.py`.

## 2026-10-29 — Absolute factor with its statistical error

- **Decision:** the absolute factor carries its statistical error from the calibrant's counts (not the
  sensitivity's), shown as `F±σ` in the configuration table and as `F [ ±σ ]` in the script's `Factor` column,
  which is Text; reading accepts the old plain `F`.
- **Why:** the factor's error was only logged (Flat Scatter, from the pixel spread) or missing (Direct Beam).
- **With it:** σ_F/F is added to the common term ε of 1D outputs, ε² = ε_T² + (σ_F/F)²; the factor's other
  inputs (T_c, T_EC, attenuation, μ) and the thickness are taken as given (error 0).
- **Code:** `addCountingVariance`, `relativeErrorOfSum`, `factorText`, `factorScriptText`, `factorValue`,
  `factorError`; [§3](dan-reduction-error-propagation.md#3-absolute-factor-and-transmission).

## 2026-10-29 — Notes: one folder, one naming pattern

- **Decision:** all DAN notes are in `qtisas/sans/dan/notes/`, named `dan-<area>-<topic>.md`; decisions are kept
  in this file.
- **Why:** one place to look; module subfolders (`dan-polarization/`) hold code only.

## 2026-10-29 — Transmission error: once per point in 1D outputs, per pixel in 2D outputs

- **Decision:** the error matrix carries the independent per-pixel errors (counting statistics, sensitivity).
  The transmission errors σ_T, σ_Tb, common to all pixels of a run, are added once per point of a 1D output
  (`I-Q`, `I-Qx`, `I-Qy`): dI² = dI_stat² + (ε·I)², with one ε per script row (per output channel in the PN
  modes). 2D outputs keep them in each pixel's error.
- **Why:** added per pixel, a common error shrinks as 1/√n in every average (0.41 % → 0.024 % for 300 pixels);
  each pixel's own error stays complete in 2D.
- **Approximation:** ε = κ·σ_T/T (+ κ_b·σ_Tb/T_b in quadrature), κ a detector mean of the sample's share.
- **Code:** `isOneDimensionalOutput`, `transmissionError`, `singleDanMultiButton(…, transmissionError)`;
  [§7](dan-reduction-error-propagation.md#7-errors-common-to-all-pixels-transmission-factor-thickness).
- **Test:** `dan_error_propagation_test.py`.

## 2026-10-29 — Error propagation fixes

- **Decision:** the per-pixel error is the relative variance 1/N of the raw counts, read at the detector size DD
  with the run's region of interest and binning (`readErrorMatrix`, `readMatrix`).
- **Why:** since the refactor of 2025-12-05 `readErrorMatrix` returned N/|N| = 1 and `readMatrix(Number, data)`
  read MD×MD: every counted pixel had r = 1, and on MD ≠ DD instruments the error matrix belonged to other pixels.
- **With it (same day):** constant background keeps the absolute error; BC weight 1 without EC; EC and BC with one
  coefficient each when a buffer is subtracted; presentations' dI from one function (`presentation()`); MS-mode
  dI a standard deviation; HF averaging adds a pixel's weights before squaring and takes corner pixels with
  `floor()`; PN x4 EC and buffer variances; PN buffer rows only from the buffer sample.

## 2026-10-29 — Smart Join scales error bars

- **Decision:** "scale error-bars too" is on by default and always on from Python (`Merge.join(smart=True)`).
- **Why:** scaling I without dI changes a curve's relative errors.

## 2026-10-28 — PN corrections in their own folder

- **Decision:** the PN corrections (x2, x4, x421–x424) are in `dan-polarization/dan-polarization-corrections.*`,
  namespace `DanPolarization`, depending on GSL only.
- **Why:** the formulas can be tested without DAN (`dan_polarization_corrections_tests`).

## 2026-10-28 — Header values written from Python

- **Decision:** `Dan.Rawdata.setHeaderValue(runs, key, value, backup=True)` writes a value where the header map
  reads it, in every header format (ASCII, Flexi, EDF, XML, YAML, HDF5), reads it back and restores the file when
  that fails; `<file>.bak` keeps the original.
- **Why:** corrections of raw headers (e.g. a wrong polarization state) without leaving DAN.

## 2026-10-27 — Polarization aliases are wildcards

- **Decision:** the `POL-ALIAS-*` fields match the whole header value as wildcards (`*`, `?`, `[...]`), case and
  spaces ignored, `||` between alternatives (the same matcher as auto mode); in PN mode configurations are grouped
  by the aliased value.
- **Why:** header values with run-specific parts (e.g. `…_pol_in_…RFflipper_on…`) map to one state.
