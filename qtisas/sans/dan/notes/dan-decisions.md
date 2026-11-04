# DAN: decisions

One entry per decision, newest first. An entry is not rewritten when the decision changes: a new entry replaces
it and says so ("Replaces …"). Explanations and formulas are in the other notes of this folder:
[reduction and error propagation](dan-reduction-error-propagation.md),
[polarization reduction](dan-polarization-reduction.md),
[polarization absolute calibration](dan-polarization-absolute-calibration.md),
[wide-angle corrections](dan-wide-angle-corrections.md).

## 2026-11-04 — Resolution σ: gravity of focusing lenses

- **Decision:** with focusing lenses σ has a gravity term, with the chromatic term's switch χ (a mirror: 0, it
  compensates gravity): a neutron falls by $A\lambda^2z^2$ ($A = g\,m_n^2/2h^2 = 3.133\cdot10^{-9}$ cm⁻¹Å⁻²),
  through the lenses $2A\lambda^2C^2$ from source to detector whatever the ray; its wavelength spread
  $4A\lambda^2C^2\varepsilon$ is vertical, half its variance in the radial average:
  $\frac43k^2A^2\lambda^4C^4\Delta\lambda^2/D^2$. DAN and ASCII.1D. Pinhole σ stays without gravity (below a pixel
  on KWS-1/2).
- **Why:** at $2f$ = 19 m, Δλ 0.1, lenses' σ at small Q was 3 % (7 Å) to 37 % (15 Å) below a Monte Carlo
  simulation with gravity; with the term within 1–4 %.
- **With it:** lenses' σ larger at long wavelengths, up to 1.6 times at 15 Å at the smallest Q; a mirror unchanged.
  The chromatic term's validity restated, $p = 4\Delta\lambda(C-D)/D \lesssim 0.3$; with lenses always
  $D \approx C$, $p \approx 0$: no warning needed. The page:
  [dan-output-formats.md](dan-output-formats.md#focusing).
- **Code:** `dan18::sigmaQmerged`, `ascii1d18::sigma`.
- **Test:** `dan_resolution_test.py`, `ascii1d_resolution_test.py`.

## 2026-11-04 — Detector resolution: the real pixel; ASCII.1D lenses as DAN's focusing

- **Decision:** "Detector Space Resolusion" is the detector's real pixel, the smallest distance that still gives two
  signals; Sigma-D1 is a box of that width, variance $\Delta_{\rm D1}^2/12$, like the averaging ring (not set: one
  pixel, as before). Replaces the Gaussian FWHM of the entries below. ASCII.1D's lenses follow DAN's focusing:
  source and detector at $2f$ = the collimation distance, the sample at the detector distance; their focus field
  is gone (`{Reso-Focus}` no longer saved, ignored when read). "Lenses?" became "Focused?" with the optic beside
  it (lenses / mirror, as DAN), "Colimation = 2f:" when focused; `setResolutionFocusing(on, optic)` replaces
  `setResolutionLenses(on, focus)`; a format file keeps focusing, the optic and the units of Q and λ (not saved
  before), and reads an older `{Reso-Lens-Yes-No} Yes` as focused lenses.
- **Why:** the field's meaning (the user: like a "real" pixel size) and the default of one pixel are a box. As a
  Gaussian of that FWHM the term had 2.2 times the variance. ASCII.1D and DAN gave different σ for the same lenses.
- **With it:** σ at most 7–8 % smaller than with the FWHM reading (KWS-3, smallest Q; under 1 % at large Q). The
  Monte Carlo with box detectors: pinhole within 1 % (3–11 % above next to the beam), focusing within 1 % (2–3 %
  at the smallest Q). The page: [dan-output-formats.md](dan-output-formats.md#the-resolution-σ-pinhole-and-focusing).
- **Code:** `dan18::sigmaQmerged`, `ascii1d18::sigma`, `ascii1d18::setResolutionFocusing`, `focusedChanged`, `readCurrentASCII1D`, `saveCurrentASCII1D`,
  `ascii1d.ui`.
- **Test:** `dan_resolution_test.py`, `ascii1d_resolution_test.py`, `ascii1d_locale_test.py`.

## 2026-11-04 — Resolution σ with focusing as variances; rectangular apertures; SasView's mask

- **Decision:** the focusing σ, too, is the square root of a sum of variances: the source's image (1:1), a uniformly
  lit disc, $k^2r_1^2/4D^2$; the wavelength $Q^2\Delta\lambda^2/6$, as without focusing (σQ(perp) of the Qx-Qy files
  without it, the same in both modes); lenses only, the chromatic blur
  $k^2\cdot16\frac{\Delta\lambda^2}{6}\frac{(r_2C/D)^2}{4}/D^2$ (the image plane moves by $-4\varepsilon C$, a ray at
  the height $h$ on the optic lands $4\varepsilon h$ off). Rectangular apertures enter by their variance,
  $r^2 = (x^2+y^2)/6$, round ones as before. ASCII.1D's lenses the same (their own $f$ and $C$). "qxqy-resolution"
  writes a 9th column, SasView's mask (1: masked), named in the `Data columns` line. Gravity stays out: compensated
  on KWS-3, below a pixel on the pinhole instruments.
- **Why:** the reasons of the entry below (σ as a standard deviation; variances add for any shape). The former terms
  took the radius $r_1$ and the shift as Gaussian FWHMs and the wavelength with $0.4247^2/3$ instead of $1/6$:
  KWS-3 σ was 10–40 % below a Monte Carlo simulation of the setup, the most at large Q, where the wavelength
  dominates; the new σ is within 1 % of it (2–4 % above at the smallest Q). The circle of the same area gave a
  square 4.5 % less variance. SasView ignores the 8th column (the shadow factor): with "ignore mask" masked pixels
  counted there.
- **With it:** KWS-3 σ larger, up to 1.6 times at large Q; the lenses' chromatic term 39 % more variance (valid while
  $4C\Delta\lambda \ll D$: 6 % low at $4C\Delta\lambda/D = 0.76$). The "R1/R2 [cm]" check of the raw-data page shows
  the new radii. The page: [dan-output-formats.md](dan-output-formats.md#focusing).
- **Code:** `dan18::sigmaQmerged`, `saveMatrixAsTableToFile`, `Collimation::readR1`, `readR2`, `ascii1d18::sigma`.
- **Test:** `dan_resolution_test.py`, `dan_export_options_test.py`, `ascii1d_resolution_test.py`.

## 2026-11-04 — Resolution σ: the sum of variances

- **Decision:** the pinhole σ is the square root of the sum of the contributions' variances (Mildner & Carpenter
  1984, as NIST's reduction): the wavelength $Q^2\Delta\lambda^2/6$ (the selector's triangle of FWHM
  $\Delta\lambda$), each aperture $r^2/4$ (a uniformly lit circle; the sample aperture at $1/(1/C + c/D)$), the
  detector resolution as a Gaussian FWHM ($0.4247\,\Delta_{\rm D1}$; not set: one pixel), the averaging ring as a
  box ($w_{\rm AV}^2/12$); the angle factors of Pedersen et al. (1990). With focusing the same two detector terms,
  outside the optic's bracket. ASCII.1D's σ (also Fittable's "ASCII.1D.SANS") the same, the pixel as both detector
  terms.
- **Why:** σ is used as a Gaussian's standard deviation (Fittable, SasView/sasmodels), and a smooth curve's smearing
  depends on the variance only ($I + \tfrac12\sigma^2 I''$); variances add for any shape, FWHMs only for Gaussians.
  Pedersen's equal-FWHM Gaussians gave 4–30 % more than a Monte Carlo simulation of the setup (KWS-1-like, C/D =
  8/8, 8/1.5, 20/20 m), the new σ is within 1 % of it (NIST's formula too; SasView's calculator lacks the detector
  resolution, up to 20 % lower). The detector width $w/\sqrt\pi$ (the radius of a circle of the pixel's area) had no
  basis in the paper; only dropping it (D1 and AV as FWHMs) would have made σ larger still (7–36 %).
- **Focusing, the detector term:** it was $(2\cdot0.4247)^2\frac{\pi^2}{3\lambda^2}\frac{R_{\rm det}^2}{D^2} =
  0.4247^2\,\frac{k^2 R_{\rm det}^2}{3D^2}$ with $R_{\rm det} = \Delta/\sqrt\pi$: the width reduced three times — as a
  Gaussian's FWHM ($0.4247^2$), as a box's half-width ($1/3$, variance $R^2/3$) and as the radius of a circle of the
  pixel's area ($1/\pi$). Only one reading can hold, and the source term of the same bracket ($3r_1^2$) cancels its
  $1/3$; the detector's did not. Now $\frac{k^2}{D^2}[(0.4247\,\Delta_{\rm D1})^2 + w_{\rm AV}^2/12]$, as without
  focusing: the blur on the detector is seen from the sample at $D$ whatever the optic. In variance $3\pi \approx
  9.4$ times the former D1 term, with the ring added 10 times the former term on KWS-3 ($\Delta_{\rm D1}$ = 0.08 cm,
  $w$ = 0.034 cm), 14 times for $\Delta_{\rm D1} = w$. On σ (KWS-3 example runs, mirror, 9.2 m and 1.2 m): +21 %
  at the smallest Q, +9 % at 12 × the first Q, +2 % at 35 ×; the detector's share of σ² at the smallest Q 35 %
  (it was 5 %). Assumes "Detector Space Resolusion" is a FWHM (to be confirmed for KWS-3).
- **With it:** pinhole σ 4–16 % smaller (KWS-1-like, 8/8 m: 15 % at 0.01 Å⁻¹); focusing σ up to 21 % larger (above);
  σQ(perp) of the Qx-Qy files without $Q\Delta\lambda/\sqrt6$. The focusing source and wavelength terms are not
  variances yet (open point). The page:
  [dan-output-formats.md](dan-output-formats.md#the-resolution-σ-pinhole-and-focusing).
- **Code:** `dan18::sigmaQmerged`, `saveMatrixAsTableToFile`, `ascii1d18::sigma`.
- **Test:** `dan_resolution_test.py`, `ascii1d_resolution_test.py`.

## 2026-11-04 — Radial averaging: merged points

- **Decision:** a merged point's Q is the mean Q of its pixels; progressive merging makes each point the factor
  times wider than the one before (by point, not by ring index); MS mode drops the merged-away rings; linear merging
  keeps a last, shorter group; dQ takes the point's width over its radius; H.F. warns on a rotated detector;
  `Dan.Processing.Options.setRadialAveraging()`.
- **Why:** empty rings (beam stop, detector edge) pulled Q out of the data (linear 7, KWS-3: the first point at
  5.4·10⁻⁵ below the first pixel at 9.1·10⁻⁵); widths $f^{\,i}$ by ring index left 19 points at 1.05 and 5 at 1.2
  of 137 rings; MS mode wrote 118 points of I = 0; linear merging read past its arrays; dQ shrank with merging.
- **With it:** unmerged I(Q) (both factors 1, the built-in default) unchanged except the dQ column; merged curves
  change. The page: [dan-radial-averaging.md](dan-radial-averaging.md).

## 2026-11-04 — Dead time: the exp model solved, names as in the literature

- **Decision:** the dead-time model N = n exp(−nτ) solves its root (Newton until the relative step is below
  10⁻⁵); the GUI calls it "Paralysable:" and N = n/(1+nτ) "Non-Paralysable:", the monitors' model
  "Non-Paralysable".
- **Why:** the loop waited for `GSL_CONTINUE`, which `gsl_root_fdfsolver_iterate()` never returns: one Newton
  step, the correction too small at high rates (−1.3 % at mτ = 0.2, −12 % at mτ = 0.33); the names were swapped
  against the usual nomenclature (Knoll).
- **With it:** KWS3-2020 (τ 2.7·10⁻⁶ s, direct beam 8·10⁻⁶ s): the 1.2 m H-M transmission 0.8088 → 0.8087,
  its I[Q] +3.5·10⁻⁴, the other rows below 2·10⁻⁶; the test references updated. Instrument files unchanged
  (`[Options-2D-DeadTimeModel-NonPara] Yes` = the exp model); sessions write the formula, old ones are read as
  before.

## 2026-11-04 — Normalization: the buffer's dark current, Monitor-3 dead time

- **Decision:** with "Subtract #BC with TIME normalization" the buffer's BC is rescaled to the buffer's own
  duration and monitor in every row, also without an empty cell; Monitor-3 takes its own dead time; the
  transmission error adds the monitors' $1/M$; a run that cannot be normalized is reported in the results log;
  the error terms weigh the EC's and the buffer's dark current at their own scale (and the centers in the script
  table subtract the EC's own); `Dan.Processing.Options.setNormalization()`.
- **Why:** without an EC the buffer got the dark current at the sample's time/monitor; Monitor-3 used Monitor-2's
  dead-time field; the transmission error added $f = N/M$ ($\sqrt N$ too large for a constant $N \ne 1$); a run
  without monitor counts became zero with a console line only.
- **With it:** no change for the built-in instruments' results (N = 1, equal M2/M3 dead times, buffers with an
  EC). The page and the open points: [dan-normalization.md](dan-normalization.md).

## 2026-11-03 — He-tubes parallax: sin α in centimetres

- **Decision:** the parallax type "cos(α)⋅cos²(Θ) [He-tubes]" takes $\sin\alpha = a_y\,b\,p\,(Y_c-j)\cos\Theta/D$,
  the vertical offset in cm (pixel size $p$, binning $b$) over the distance in cm, at most 1. It divided the
  offset in pixels by the distance in cm.
- **Why:** $\alpha$ was wrong for every pixel size other than 1 cm (KWS-1: about twice too large), and the arcsine
  of more than 1 gave NaN at the outer rows (KWS3 at 1.2 m: 17 of 132 I[Q] points).
- **With it:** KWS1-2020 reductions (type 1, data and Plexiglas calibrant) change by 0.02–0.4 %; the tests'
  reference values updated, the published `dan-example-kws1.qti` predates the fix.
- **Also:** in the buffer modes the buffer's empty-cell term with Tr(Θ) is $T_b f_\Theta(T_b)(E-B)$, built from
  $E-B$; it was the sample's $T f_\Theta(T)(E-B)$ rescaled by $T_b/T$ (both reduction paths).
- **Also:** every wide-angle correction centered on its measurement's beam: a row's X/Y-center (both reduction
  paths), a calibrant's configuration center (centers calculated before factors), a sensitivity's configuration
  center (label source `configuration`); the calibrant's Tr(Θ) by its own option. The window factor after the dark
  current, relative to the sensitivity's own (`windowRelativeToSensitivity()`). Python:
  `Dan.Processing.Options.setWideAngle()`/`wideAngle()`. KWS1-2020 references moved again (up to 0.2 %), KWS3 2e-5.
- **Not yet:** the window factor in the calibrant's measurement (no $T_{\rm det}$ outside the script table).
- **Test:** `dan_wide_angle_test.py`, `dan_wide_angle_options_test.py`, `dan_sensitivity_center_test.py`.

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
