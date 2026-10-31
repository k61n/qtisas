# DAN: data reduction and error propagation

How DAN (QtiSAS's SANS reduction) turns raw detector runs into I(Q), and how the statistical error travels
through each step. The formulas describe the code as of 29 October 2026; the functions named are in
`qtisas/sans/dan/`. The polarized modes (PN, x2, x4, x421–x424) have their own notes in
[dan-polarization-reduction.md](dan-polarization-reduction.md).

Decisions behind it, dated: [dan-decisions.md](dan-decisions.md).

## 1. Conventions

| symbol | meaning |
|---|---|
| S, EC, BC, Buf | sample, empty cell (or empty beam), blocked beam (B4C, dark current), buffer: corrected detector images |
| N_x | raw counts of image x in one pixel (after region of interest and binning) |
| T, T_b | transmission of sample, buffer (relative to the EC) |
| F | absolute factor of the configuration (script column `Factor`), times the optional `Scale` |
| d | sample thickness [cm] |
| f | buffer volume fraction |
| sens | sensitivity matrix (1 on average over the mask) |
| r_x | relative variance of x in one pixel, (σ_x / x)² |

**Representation of errors.** Per pixel DAN carries the **relative variance** r = (σ/I)². It becomes an absolute
error only at the output: `dI = sqrt(r) · |I|` (2D outputs) or `ΣdII += r_p · I_p²` (radial averaging).
Scaling an image by a constant (monitor, time, dead time, F, d, T, sens, parallax) leaves r unchanged;
**sums and differences must be done in absolute variances** and converted back.

## 2. Reading the images

| step | function | intensity | error |
|---|---|---|---|
| raw image, DD×DD, cut to the region of interest and binned to MD×MD | `readMatrix(Number, …)`, `readMatrixByNameROI` | counts N | – |
| relative variance from the raw counts | `readErrorMatrix` | – | r = 1/N (0 where N = 0) |
| corrected image | `readMatrixCor` | N · deadTime · normalization · RT-normalization | unchanged: the factors are constants |
| BC with time normalization | `readMatrixCorTimeNormalizationOnly` + `NormSample` | rescaled to the sample's monitor/time | unchanged |
| shift (HShift, VShift) | `gslMatrixShift` | interpolated | r interpolated the same way |

`readMatrix` and `readErrorMatrix` read the run at its **detector size DD** and cut the region of interest exactly
as `readMatrixCor` does, so intensity and error belong to the same pixels. (Until 2026-10-29 the error matrix
was read as MD×MD and `readErrorMatrix` returned N/|N| = 1 instead of 1/N: since the refactor of 2025-12-05
every counted pixel had r = 1.)

## 3. Absolute factor and transmission

The factor F of a configuration comes from the calibrant (Flat Scatter, Flat Scatter + Transmission, Direct
Beam, Direct Beam from Monitor-3; `dan18-process.cpp`). Its **statistical error** is calculated from the
calibrant's counts:

```
Flat Scatter:  F ∝ 1/Σ_p sens_p P'_p,   P' = P − (1−T_c)·BC − T_c·EB  (T_c weight 0 without EB)
Direct Beam:   F ∝ 1/Σ_p sens_p (EB − BC)_p
σ_F / F = sqrt( Σ_p sens_p² var(P'_p) ) / Σ_p sens_p P'_p ,   var from the raw counts (`addCountingVariance`)
```

The sensitivity's own error is left out: it is in every sample pixel's error (§4.2), and with the sensitivity
made from the same calibrant run the calibrant's counts cancel in I ∝ S/P' anyway. (Before 2026-10-29 Flat
Scatter logged the standard error of the pixel spread, which also holds real structure and vanishes with a
sensitivity of the same run; Direct Beam had none.) Monitor-3 has no error.

It is shown as `F±σ` in the configuration table (like the centers) and copied into the script's `Factor` column
as `F [ ±σ ]` (like the transmissions; the column is Text). Reading takes `F [ ±σ ]`, `F±σ` and the old plain `F`
(`factorValue`, `factorError`); Python: `configuration(i)["factor"]`, `["factorError"]`. σ_F/F is a common error
like the transmission's: it goes into ε, once per point of a 1D output (§7). The calibrant's transmission T_c, T_EC,
the attenuation (Direct Beam) and μ are taken as given (error 0).

The sample transmission T and its σ_T are read from the script's `Transmission-Sample` column, `"T [ ± σ_T ]"`;
`singleDanFactorTransThickness` returns σ_T / T (relative).

The scale applied to the sample is

```
abs = Scale · F / (d · T)            (abs0 = Scale · F / d;  abs = abs0 / T)
```

## 4. One script row (standard mode SM, `danDanMultiButton`)

### 4.1 Intensity

```
Sample  = S − BC − T·(EC − BC)                           (with an EC)
Sample  = S − BC                                         (without an EC)
Buffer  = Buf − BC − T_b·(EC − BC)                       (with a buffer; without an EC: Buf − BC)
Sample −= s_B · Buffer,     s_B = f · T / T_b            (s_B = fractionBuffer · absBuffer / abs)
I       = abs · Sample · sens                            (or · 1/normalization for the Monitor-3 calibration)
I      −= c                                              (optional constant background: script column `Background`)
```

Then: negative pixels masked (optional), matrix smoothing `matrixConvolusion` (optional), parallax and
detector-transmission corrections (constant factors per pixel).

### 4.2 Error of one pixel

The error loop computes the relative variance of the bracket in absolute variances, `err2` being the signal:

```
signal   = S − T·EC − w_BC·BC − s_B·(Buf − T_b·EC − w'_BC·BC)       w_BC = 1−T (with EC) or 1; w'_BC likewise
variance = S²·r_S
         + c_EC²·EC²·r_EC + c_BC²·BC²·r_BC                           one coefficient each (their noise cancels in part):
                                                                     c_EC = −T + s_B·T_b,  c_BC = −w_BC + s_B·w'_BC
         + s_B²·Buf²·r_Buf
         + (S − BC)²·(σ_T/T)² + s_B²·(Buf − BC)²·(σ_Tb/T_b)²         transmission terms: 2D outputs only (§7)
r_pixel  = variance / signal²  +  r_sens
```

Without a buffer the EC and BC terms are `T²·EC²·r_EC + w_BC²·BC²·r_BC`.

**Why the transmission terms carry (S − BC), not the whole signal.** With abs = abs0/T,

```
I = abs0 · [ (S − BC)/T − (EC − BC) − (f/T_b)·(Buf − BC − T_b·(EC − BC)) ]
```

T divides only the sample part; the EC part does not depend on T (the 1/T of abs and the T of T·EC cancel), and
the buffer part depends only on T_b. Hence

```
∂I/∂T   = −abs0 (S − BC) / T²        →  (σ_I/I)²_T  = (S − BC)² (σ_T/T)² / signal²
∂I/∂T_b =  abs0 f (Buf − BC) / T_b²  →  (σ_I/I)²_Tb = s_B² (Buf − BC)² (σ_Tb/T_b)² / signal²
```

with I = abs0 · signal / T: exactly the two transmission terms of the loop. (Where `transmissionThetaDependenceTrEC`
makes the EC's T angle dependent, the error still uses the plain T: a small, angle-dependent mismatch.) For 1D
outputs the same derivatives give the row's transmission error, once per point (§7).

`r_sens` is the sensitivity's relative variance, 1/N of the run the sensitivity was made from
(`getSensitivityNumber` + `readErrorMatrix`); 0 for a sensitivity loaded from a file.

After the loop the image is only scaled (abs, sens), so `SampleErr = r_pixel` stays valid, with two exceptions
that change r:

| step | r afterwards |
|---|---|
| constant background I → I − c (`subtractConstantBackground`) | r · I² / (I − c)²: the absolute error stays |
| buffer as sensitivity (`Use-Buffer-as-Sensitivity` = yes): R = N/B, N = Sample, B = scaled Buffer | r_N + r_B + 2·r_B·B/N (N and B correlated), without r_sens |
| smoothing (`matrixConvolusion`) | Σ_k r_k I_k² / (Σ_k I_k)² over the kernel |

Masked pixels have r = 0 and are left out of every average.

### 4.3 The sensitivity and the rows reduced with it

The sensitivity is made from a flat scatterer with the same steps (P − BC − T_s·(EB − BC), wide-angle corrections),
so for that run as a sample I = abs · mean · c_row/c_sens: flat only when both are made alike.

| parameter | sensitivity | row | when not the same |
|---|---|---|---|
| transmission | `Tr` of the sensitivity tab (typed, measured, or the formula T(λ)) | `Transmission-Sample` | the EB part differs by ΔT·EB; EB is not flat, so I(Q) drifts (H2O 119992: Tr 0.5191 vs T 0.5161, 0.1 % from low to high Q) |
| center of the parallax and θ-transmission corrections | the mask's beam stop, or the center fields (checked group) | the row mask's beam stop (`readCenterfromMaskName`) | cos³θ around two centers: I(Q) curved |

The label keeps both, `…::Tr::0.5191::Center::63.498|64.784|mask` (1-based; `manual` for the fields' values;
`::Center::No` without a calibrant; older labels have no center). `checkRowAgainstSensitivity` warns per row
(log, `lastMessages()`): the sensitivity's own run with another T (> 0.5·10⁻⁴), or, with parallax or θ-transmission
on, a center more than 0.5 pixel away. The radial averaging uses the row's X-, Y-center, not the mask's.

## 5. Radial averaging and other outputs

### 5.1 Standard averaging (`radAvStd`)

Each pixel goes into one Q bin; for the n pixels p of a bin

```
I(Q)  = (1/n) Σ I_p
dI(Q) = sqrt( Σ r_p I_p² / n²  +  (ε I(Q))² )      ε: the row's transmission error, once per point (§7)
```

The same ε term is added in HF averaging, MS mode and the slices.

### 5.2 HF averaging (`radAvHF`)

Each ring is sampled at 4πr points, each point bilinear between its four corner pixels (`floor` of the
position), with weights w. A pixel is hit by several points, so its weights are added first:

```
W_p   = Σ_points w_p                          (per ring; map pixel → W_p)
I(Q)  = Σ W_p I_p / Σ W_p
dI(Q) = sqrt( Σ W_p² r_p I_p² ) / Σ W_p
```

(Before 2026-10-29: Σ w² per sample, about 1.75× too small; corner pixels of points at x ∈ (−1, 0) were taken as
pixel 0 with weights 1.5 / −0.5.)

### 5.3 Merging of bins and skipped points (`radAvDataCorrectionOptions`)

Log and linear merging add I, Σ r I² and the weights (as `double`: HF weights are fractional) of the merged bins,
then I = ΣI/Σn, dI = sqrt(Σ r I²)/Σn. Skipped first/last points and negative points are flagged, not mixed in.

### 5.4 MS mode (`radUniStandartMSmode`)

For each ring a straight-line fit of I against sin²φ: I = c₀ + c₁ sin²φ. The errors are those of the fit,
var(c₀) = χ²/dof · cov₀₀, var(c₁) = χ²/dof · cov₁₁ (the pixels' own errors are not used: weights 1); merged bins
add variances, dI = sqrt(Σvar)/n.

### 5.5 Presentations (`presentation()`)

The table columns are (x, y, dy, dx) of the selected presentation, dy = |dy/dI|·dI, dx = |dx/dQ|·dQ:

| presentation | y | dy |
|---|---|---|
| QI, logQ | I | dI |
| Guinier | ln\|I\| | dI/\|I\| |
| GuinierRod, GuinierPlate | ln\|I Q\|, ln\|I Q²\| | dI/\|I\| (Q exact) |
| Zimm | 1/I | dI/I² |
| Porod, logI | log₁₀\|I\| | dI/(\|I\| ln 10) |
| Debye | \|I\|^−½ | ½ dI \|I\|^−3/2 |
| Porod2 | I Q⁴ | dI Q⁴ |
| 1Moment | Q I | Q dI |
| 2Moment, Kratky | Q² I | Q² dI |

I = 0 gives y = dy = 0 where y needs 1/I or a logarithm.

### 5.6 2D outputs and slices

`I-x-y` writes I per pixel; `dI-x-y` writes sqrt(r)·|I|. Horizontal and vertical slices average rows or columns
like §5.1.

## 6. Merging configurations (`dan18-merge1d.cpp`)

- **Join** cuts the overlapping curves at the middle of their overlap; points are not averaged.
- **Smart join** scales each curve to its neighbour by the ratio of the mean levels Σ I Q^k / n in the overlap
  (k: the Q exponent). The factor multiplies I and, with "scale error-bars too" (on by default, always on from
  Python), dI: the relative error of a curve stays. The factor's own uncertainty is not propagated.
- **Rebin** averages the points of a new Q bin without weights: I = ΣI/n, dI = sqrt(ΣdI²)/n; the resolution
  columns σ and dQ are the points' rms (plus half the bin width for σ).

## 7. Errors common to all pixels: transmission, factor, thickness

T, F and d are **one number per run**: every pixel of the run is multiplied by the same value. Their errors are
statistical in origin (σ_T from counting the transmission measurement) but **fully correlated** between pixels
and between Q points.

For I(Q) = (T/n) Σ_p (S_p − B_p) (one T, n pixels):

```
σ_I² = (T²/n²) Σ_p (σ_S,p² + σ_B,p²)      independent per pixel: ∝ 1/n
     + I(Q)² · (σ_T/T)²                    common: not reduced by averaging
cov(I(Q_i), I(Q_j)) = I(Q_i) I(Q_j) (σ_T/T)²
```

A common error added to every pixel's r would shrink as 1/√n in an average, as if every pixel had its own T
(σ_T/T = 0.41 %, n = 300 pixels in a ring: 0.024 % instead of 0.41 %). DAN therefore keeps it out of the pixel
variance where pixels are summed:

| output | error matrix (per pixel) | transmission error |
|---|---|---|
| 1D: `I-Q`, `I-Qx`, `I-Qy` (radial averaging, MS mode, slices) | statistical only | once per point: dI² = dI_stat² + (ε·I)² |
| 2D: `I-x-y`, `dI-x-y`, ... | statistical + the transmission terms of §4.2 | in each pixel's dI (correct for one pixel) |

`isOneDimensionalOutput(button)` decides; the output functions (`radUni`, `radUniStandartMSmode`,
`horizontalSlice`, `verticalSlice`) get ε through `singleDanMultiButton(…, transmissionError)`.

**ε: one value per script row** (per output channel in the PN modes), the relative common error of I (transmissions
and absolute factor; σ_F/F from the row's `Factor`, `F [ ±σ ]`):

```
ε² = (κ · σ_T/T)² + (κ_b · σ_Tb/T_b)² + (σ_F/F)²   sample and buffer transmissions and the factor: independent
κ   = Σ_p (S − BC)_p / Σ_p signal_p          the sample's share before the EC subtraction (§4.2)
κ_b = Σ_p s_B (Buf − BC)_p / Σ_p signal_p    the buffer's share
```

(sums over the mask, in the units of the error loop). κ = 1 without EC and buffer; for a weak sample with a strong
EC (or a buffer close to the sample) κ, κ_b ≫ 1: the transmission error is then the larger part of dI.

| mode | ε from |
|---|---|
| SM, BS | the sums above, in the error loop of `danDanMultiButton` |
| BS-SENS (R = N/B) | per-pixel means: κ = ⟨(S − BC)/N⟩, κ_b = ⟨(Buf − BC)(s_B/N + 1/B_sub)⟩, B_sub the buffer after EC and BC |
| PN | Σ sample part / Σ result with σ_T/T of the row, and Σ v·buffer / Σ result with σ_Tb/T_b |
| x2, x4 | per channel: Σ sample part (after the correction) with σ_rel(T₋) or σ_rel(T₊), Σ v·buffer part with the buffer's σ_rel(T₋/T₊) |
| x421–x424 | per output: each measured channel's share (`TwoChannelPair::channelShares`) with that channel's σ_T/T, sample and v·buffer |

Limits:
- κ, κ_b are detector means: exact when the sample share has the same Q shape as I (no or weak EC), approximate
  for weak samples, where I_S/I varies with Q (exact: the sample-part image averaged like I, not implemented);
- PN x2/x4: the mixing of T₋ and T₊ through the polarizer inverse (both from T_down, T_up) is ignored;
- a constant background (§4.2) changes I but not the T-dependent part; ε·I then differs by c/I from the exact
  term;
- rebinning a 1D curve later (§6) combines dI as independent: the transmission part shrinks again;
- the raw-data views (`danDanMultiButtonSingleLine`) have no transmission error.

The thickness d is a common factor of the same kind, taken as given (error 0). The two-file modes add σ_F/F once when
both rows share the factor (one configuration), else with the channels' shares.

## 8. Not modelled (known, by choice)

| item | effect |
|---|---|
| N = 0 gives r = 0 | sparse pixels at high Q count as exact (Poisson: about one count of variance) |
| dead time scales I only | σ of strongly dead-time-corrected runs slightly low |
| sensitivity error from the calibrant's counts only | EB and BC of the sensitivity run ignored |
| sample run = sensitivity run | the pixel noise cancels in I, but r_S + r_sens is reported |
| smoothing, shifts, HF interpolation | neighbouring pixels/bins become correlated; averages treat them as independent |
| linear merging | remainder points (n mod L) dropped |
| polar output (`I-Polar`) | intensity only, no error |

## 9. Where to look

| topic | code |
|---|---|
| raw images, error matrices | `dan18-process-matrix.cpp`: `readMatrix`, `readMatrixCor`, `readErrorMatrix`, `readErrorMatrixRel` |
| one row, all modes but PN | `dan18-process-dandan.cpp`: `danDanMultiButton`, `danDanMultiButtonSingleLine` (raw-data views) |
| averaging and presentations | `dan18-process-dandan.cpp`: `radUni`, `radAvStd`, `radAvHF`, `radAvDataCorrectionOptions`, `presentation`, `radUniStandartMSmode` |
| output of one row | `dan18-process-single-line-in-script.cpp`: `singleDanMultiButton`, `singleDanSimplified` (PN) |
| sensitivity | `dan18-sens.cpp`: `createSensFull`, `getSensitivityNumber`, `sensitivityCenter`, `checkRowAgainstSensitivity` |
| merging | `dan18-merge1d.cpp` |
| polarized modes | `dan-polarization/dan-polarization-corrections.cpp`, `dan18-process-polarization.cpp`, [notes](dan-polarization-reduction.md) |
| tests | `qtisas/tests/sans/dan/dan_error_propagation_test.py`, `dan_sensitivity_center_test.py`, `dan_polarization_corrections_tests.cpp` |
