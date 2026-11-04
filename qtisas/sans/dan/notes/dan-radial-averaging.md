# Radial averaging in QtiSAS/DAN

The page "I[Q] :: Radial Averaging :: Method" of the Processing options sets how DAN turns a reduced 2D image into
I(Q): the averaging over rings of equal scattering angle, and the merging of neighbouring rings into one point.
The formulas describe the code as of 4 November 2026 (after the fixes of that day); the functions named are in
[dan18-process-dandan.cpp](../dan18-process-dandan.cpp). The errors of the rings:
[dan-reduction-error-propagation.md](dan-reduction-error-propagation.md) §5; decisions, dated:
[dan-decisions.md](dan-decisions.md).

## The options

| Widget | Instrument file | Session | Values |
|---|---|---|---|
| `radioButtonRadStd` "Standard (Shells)" / `radioButtonRadHF` "4-Pixel Interpolation (H.F.)" | `[Options-1D-RADmethod-HF]` | `Options::1D::I[Q]::Method` | Yes: H.F. (14 of the 21 built-in instruments), No: shells |
| `spinBoxAvlinear` "Linear Merging Factor" | `[Options-1D-RAD-LinearFactor]` | `Options::1D::I[Q]::LinearFactor` | 1..100 rings per point, 1: none |
| `doubleSpinBoxAvLog` "Progressive Merging Factor" | `[Options-1D-RAD-ProgressiveFactor]` | `Options::1D::I[Q]::ProgressiveFactor` | 1.000..1.200, 1: none |

Applied after the averaging, from other pages: "I[Q] :: Remove Points" (first/last points,
`Dan.Processing.Options.setRemovePoints()`), "Mask negative points" of I[Q] (`checkBoxMaskNegativeQ`,
`[Options-1D-RemoveNegativePoints]`), "I[Q] :: SAS Presentation" and the 4th column ("I[Q] :: Output Format").

## Geometry

Pixel $(i, j)$ (column, row, 0-based) with the beam center $(X_c, Y_c)$ of the row (0-based), the pixel width
$p$ (`detelem`: pixel size × binning, cm), the pixel asymmetry $a_y$ (height / width) and the sample-detector
distance $D$ [cm]: radius $r = p\sqrt{(i-X_c)^2 + a_y^2 (j-Y_c)^2}$, scattering angle $\Theta = \arctan(r/D)$,

$$
Q = \frac{4\pi}{\lambda}\sin\frac{\Theta}{2}.
$$

## "Standard (Shells)": `radAvStd`

Every unmasked pixel (mask > 0) goes into one ring: ring $n$ holds the pixels with
$D\tan\Theta = r_{\min} + n\,\Delta r$ (rounded), $\Delta r = \min(1, a_y)\,p$, $r_{\min}$ the radius of the
smallest angle on the detector (0 when the beam is on it). Its $Q$ is that of $r_{\min} + n\Delta r$. A rotated
detector (`[Detector-Angle-X/Y]` of the header or a constant, degrees) is handled: $\Theta$ is the angle between
the pixel's position and the rotated beam axis, the rings are rings of equal angle.

Per ring: $\Sigma I_p$, the number of pixels $n$, $\Sigma r_p I_p^2$ (the variances; of a smoothed image from the
pixels before smoothing) and $\Sigma I_p\cos 2\varphi_p$ (the anisotropy column, $\varphi$ the pixel's azimuth plus the
anisotropy offset `spinBoxAnisotropyOffset`).

## "4-Pixel Interpolation (H.F.)": `radAvHF`

Ring $n$ (radius $n$ pixel widths, $Q$ of $r = n\,p$) is sampled at $\max(\mathrm{round}(4\pi n), 1)$ points
$x = X_c + n\cos\phi$, $y = Y_c + (n/a_y)\sin\phi$ — two per pixel of arc. Each point is bilinear between its four
corner pixels (`floor` of the position), weights $(1-\Delta x)(1-\Delta y)$, … times the mask; pixels outside the
detector have no weight. A pixel hit by several points has its weights added first ($W_p$):

$$
I(Q) = \frac{\sum_p W_p I_p}{\sum_p W_p},\qquad dI = \frac{\sqrt{\sum_p W_p^2 r_p I_p^2}}{\sum_p W_p}.
$$

A rotated detector (the beam tilted as in Standard): ring $n$ is the cone of the angle $\arctan(n\,p/D)$ around
the beam, met by the detector plane — the points $u = \cos\theta\,b + \sin\theta(\cos\phi\,e_1 + \sin\phi\,e_2)$
($b$ the beam's direction, $e_1, e_2$ perpendicular to it, $e_1$ as close to the detector's x as it can be) taken
where they meet the plane $z = D$, with $\max(1, 4\pi n / b_z^2)$ points (the ring stretched by the tilt) and the
rings up to the largest angle at the corners. Without a rotation this is the circle above, unchanged.

## Merging rings into points: `radAvDataCorrectionOptions`

The rings' sums are added (intensity, variance, pixel count or weight, anisotropy), so a merged point's
$I = \Sigma I / \Sigma n$ is the pixel-weighted mean and $dI = \sqrt{\Sigma \mathrm{var}}/\Sigma n$.

**Progressive** (factor $f > 1$): point $k = 0, 1, \ldots$ holds $\max(1, \mathrm{round}(f^{\,k}))$ rings — each
point $f$ times wider than the one before (`progressiveWidth`). Example (KWS-3, 137 rings at 9.2 m): $f = 1.05$
gives 38 points, $f = 1.2$ gives 15.

**Linear** (factor $L > 1$): rings $Lk \ldots Lk+L-1$ form point $k$, the last group may hold fewer rings; after a
progressive merging the remaining points are merged the same way.

**Q of a merged point:** the mean $Q$ of its pixels, $\Sigma n_j Q_j / \Sigma n_j$ over its rings (H.F.: the
weights); a ring without pixels (under the beam stop, beyond the detector edge) does not move it.

Then empty points are dropped, the first/last points removed ("Remove Points"), and with "Mask negative points"
the points with $\Sigma I \le 0$.

## The 4th column: resolution σ or dQ

- σ (`sigmaQmerged`): the instrumental resolution, pinhole or focusing (wavelength spread, apertures, collimation,
  detector term growing with the merged rings $m$): the formulas in
  [dan-output-formats.md](dan-output-formats.md#the-resolution-σ-pinhole-and-focusing).
- dQ (`dQ`): $Q\sqrt{\Delta\lambda^2 + \delta_D^2 + \delta_d^2}$ with the run's relative wavelength spread
  $\Delta\lambda$ (0.01 when the header has none), $\delta_D$ = 1, 2 or 5 cm over $D$ (D < 50, < 400, more), and
  the point's width: $\delta_d = 0.5\,m\,w/d$, $m$ rings of width $w$ (shells $\min(1, a_y)\,p$, H.F. $p$) at the
  radius $d = D\tan(2\arcsin(Q\lambda/4\pi))$; the matrix dQ-x-y the same per pixel
  ([dan-output-formats.md](dan-output-formats.md#dq)).

## MS mode (`radUniStandartMSmode`)

The magnetic-sample mode fits each ring (radius $n$ pixel widths, its pixels $I(\varphi) = c_0 + c_1\sin^2\varphi$;
I[Q] NC = $c_0$, MC = $c_1$) and merges rings progressively as above: a point is the mean of its fitted rings, its
$Q$ their mean $Q$; no linear merging. dQ as above, with $w = p$.

## Python

`Dan.Processing.Options.setRadialAveraging(method=None, linear=None, progressive=None)`: `method` "shells" or
"hf", `linear` 1..100, `progressive` 1.0..1.2; None keeps the option. `radialAveraging()` →
`{"method": …, "linear": …, "progressive": …}`. "Remove Points": `setRemovePoints()`. The instrument and the
session set them as in the GUI.

## Tests

The KWS-1 and KWS-3 reductions ([dan_kws1_reduction_test.py](../../../tests/sans/dan/dan_kws1_reduction_test.py),
[dan_kws3_reduction_test.py](../../../tests/sans/dan/dan_kws3_reduction_test.py)) check I(Q) without merging against
reference values (KWS1-2020: shells, KWS3-2020: H.F.);
[dan_error_propagation_test.py](../../../tests/sans/dan/dan_error_propagation_test.py) compares the errors of both
methods; [dan_rotated_detector_test.py](../../../tests/sans/dan/dan_rotated_detector_test.py): with a rotation
of 1° H.F. and Standard give the same I[Q] (median ratio within 2 %), the rotation itself changes it.
[dan_radial_averaging_test.py](../../../tests/sans/dan/dan_radial_averaging_test.py): the Python options,
merged points (linear 3, 7, progressive 1.05, 1.2, both methods) with $Q$ inside the unmerged curve's range and
increasing, the number of progressive points, MS mode without points of $I = 0$, the method kept by a session. The
dQ column was checked by hand: dQ/Q grows with the merged rings (KWS-3, 9.2 m, linear 7: 0.026 at the last point,
unmerged 0.0066).

## Fixed (audit and fixes 2026-11-04)

| Point | Before |
|---|---|
| MS mode, progressive merging: merged-away rings are no points | they kept their count with I = 0: 118 of 137 points at 1.05 (MC and NC) |
| $Q$ of a merged point: the mean of its pixels | the plain mean of its rings, also of empty ones: linear 7 put the first point at 5.4·10⁻⁵, the first pixels are at 9.1·10⁻⁵ |
| Progressive widths $f^{\,k}$ by point | $f^{\,i}$ by ring index: 19 points at 1.05, 5 at 1.2 |
| Linear merging: the bounds check before the read; the last, shorter group kept | read past the arrays when $N \bmod L \ge L/2$ |
| dQ: the width of the point's rings over its radius | the pixel term shrank with merging; in shells mode the ring index was taken as radius |
| H.F. on a rotated detector: rings of equal angle around the tilted beam (as Standard) | circles on the detector, the rotation ignored |
| A session with "Standard (Shells)" selects it | the button of the session before |
| `[Options-1D-RAD-LinearFactor]` written once; "Standard"; `radAvHF`'s dead variables gone | twice; "Standart" |
| `setRadialAveraging()` / `radialAveraging()` | no Python access |

## Open points

None.
