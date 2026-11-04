# Wide-angle corrections in QtiSAS/DAN

The page "I[x,y] :: Wide Angle Corrections" of the Processing options has three corrections. Each multiplies or
divides every pixel of a 2D image by a factor that depends only on the pixel's scattering angle $\Theta$; the
relative errors of the pixels therefore stay as they are. Decisions behind them, dated:
[dan-decisions.md](dan-decisions.md); the rest of the reduction:
[dan-reduction-error-propagation.md](dan-reduction-error-propagation.md). Literature: [References](#references).

## The scattering angle of a pixel

For the pixel in column $i$ and row $j$, with the center $(X_c, Y_c)$ (0-based pixels), the pixel size $p$ [cm],
the binning $b$, the pixel asymmetry $a_y$ (pixel height / width) and the sample-detector distance $D$ [cm]:

$$
r = b\,p\sqrt{(X_c-i)^2 + a_y^2\,(Y_c-j)^2},\qquad \Theta = \arctan\frac{r}{D}.
$$

$\Theta$ is the full scattering angle ($2\theta$ in the usual notation). Pixels with $\Theta \le 0.01$ rad are left
unchanged: every factor below differs from 1 there by less than $1.5\cdot10^{-4}$.

## 1. Parallax :: solid angle and detector response

Checkbox "Parallax ::" (`checkBoxParallax`, instrument `[Options-2D-HighQ]`), the type in `comboBoxParallax`
(`[Options-2D-HighQ-Parallax-Type]` 0 / 1). Code: `dan18::parallaxCorrection()` in
[dan18-process-matrix.cpp](../dan18-process-matrix.cpp). The pixel is **divided** by

| Type | Factor | Meaning |
|---|---|---|
| 0, "cos³(Θ) [Li]" | $\cos^3\Theta$ | the solid angle of a flat pixel at distance $D/\cos\Theta$, inclined by $\Theta$: $\Delta\Omega \propto \cos^3\Theta$; a thin flat detector (scintillator) whose efficiency does not depend on the angle |
| 1, "cos(α)⋅cos²(Θ) [He-tubes]" | $\cos\alpha\,\cos^2\Theta$ | for ³He tubes: the $\cos^2\Theta$ of the distance, and in place of the inclination $\cos\Theta$ the angle $\alpha$ in the vertical plane, $\sin\alpha = \dfrac{a_y\,b\,p\,(Y_c-j)}{D}\cos\Theta$ (vertical offset over the pixel's distance $D/\cos\Theta$) |

Type 0 is the standard solid-angle correction of a flat detector [1, 2]. Type 1 is the tube correction of He et
al. [3] (in Mantid: `SANSSolidAngleCorrection`, `DetectorTubes` [4]): a linear position-sensitive tube presents its
cylinder to the scattered beam, so the inclination in the plane across the tube does not reduce the solid angle,
and $\Delta\Omega \propto \cos\alpha\,\cos^2\Theta$, with $\alpha$ the angle between the sample-to-pixel vector and its
projection on the horizontal plane. SANS instruments mount their ³He tubes vertically, so $\alpha$ comes from the
vertical offset, as DAN takes it ($Y$: the rows of DAN's image, vertical once the instrument's read options —
transposition, mirroring — are applied). He et al. find type 0 overcorrecting tube data above about 10° and type 1
within 5 % of isotropic scatterers up to 35° [3].

**Fixed 2026-11-03:** $\sin\alpha$ was $a_y(Y_c-j)\cos\Theta/D$: a pixel offset divided by centimetres. With
$p \ne 1$ cm $\alpha$ was wrong (KWS-1, $p$ = 0.525 cm: about twice too large), and once $|Y_c-j|\cos\Theta > D$ (in
pixels) the arcsine of more than 1 gave NaN (KWS3 at 1.2 m: 17 of 132 I[Q] points). The argument is now in cm and
at most 1. Reductions with type 1 change by 0.02–0.4 % (KWS1-2020 example: the factors 53390 / 1765 / 117 became
53595 / 1772 / 118; the published `dan-example-kws1.qti` predates the fix).

## 2. Transmission :: Tr = f(cos Θ)

Checkbox "Transmission :: Tr = f ( cos Θ )" (`checkBoxParallaxTr`, `[Options-2D-HighQ-Tr]`). For a flat sample
perpendicular to the beam with transmission $T$, a neutron scattered at depth $z$ of the thickness $d$ travels
$z$ before and $(d-z)/\cos\Theta$ after the scattering. Averaged over $z$, the transmission of the scattered beam is

$$
T_\Theta = T\,\frac{T^{\,\kappa}-1}{\kappa\ln T},\qquad \kappa = \frac{1}{\cos\Theta}-1,
$$

which tends to $T$ for $\Theta\to0$ [1, 2] (in Mantid: `SANSWideAngleCorrection` [5]). DAN normalizes the sample
with $T$ (the absolute factor divided by $T$), so the remaining angular part
$f_\Theta = T_\Theta/T = (T^{\kappa}-1)/(\kappa\ln T)$ is:

- the **sample** divided by $f_\Theta$ (in `parallaxCorrection()`, with the sample's $T$; the buffer with its own),
- the **empty cell** multiplied by $f_\Theta$ before $T\cdot EC$ is subtracted (`transmissionThetaDependenceTrEC()`:
  the cell's scattering passes the sample at the same angle).

Only for $0 < T < 1$. In the buffer modes the buffer's EC term is $T_b f_{\Theta,b}$ with the buffer's own
transmission (fixed 2026-11-03: it was the sample's term rescaled by $T_b/T$, keeping the sample's $f_\Theta$).

## 3. Sensitivity :: transmission of the detector window

Checkbox "Sensitivity is function of transmission of Det. Window" (`checkBoxWaTrDet`), the window's transmission at
normal incidence $T_{\rm det}$ from the script table's column (outside $(0, 1]$: 1). A window of transmission
$T_{\rm det}$ passes $T_{\rm det}^{1/\cos\Theta}$ at the angle $\Theta$; the pixel is **multiplied** by

$$
T_{\rm det}^{\,1-1/\cos\Theta} \;(\ge 1),
$$

which undoes the extra absorption relative to normal incidence (the Beer–Lambert law of correction 2 for a layer the
neutron crosses entirely after the scattering; no reference of its own). Code: `dan18::gslMatrixWaTrDet()` in
[dan18-process-dandan.cpp](../dan18-process-dandan.cpp), applied to the sample, the empty cell and the buffer after
their dark current is subtracted (the dark current does not cross the window), around the row's beam center.

A sensitivity made of a run contains the window's absorption of its own measurement (its distance $D_s$, its center).
The data are therefore multiplied by the row's factor **over the sensitivity's**,
$w = T_{\rm det}^{\,1-1/\cos\Theta}\big/T_{\rm det}^{\,1-1/\cos\Theta_s}$, $\Theta_s$ the pixel's angle in the
sensitivity's geometry (`windowRelativeToSensitivity()`); for a flat sensitivity or one read from a file the row's
factor alone. At the sensitivity's own distance and center $w = 1$: the window is then in the sensitivity already.
Not included: the calibrant's measurement (no $T_{\rm det}$ outside the script table).

Since 2026-11-03 (before: $w$ multiplied $S$, $E$ before $B$ was subtracted, $wS - B = w(S-B) + (w-1)B$; and the
row's factor alone, the window counted twice when the sensitivity was measured at the same distance; the single-row
path set $T_{\rm det} = 0$ for negative values, an infinite factor).

## Where in the reduction: sample, empty cell, dark current

### What a pixel measures

Each run is normalized to its monitor (or time) when it is read. For a pixel at the angle $\Theta$, with the
flux $\Phi$, the pixel's efficiency $\varepsilon$, its solid angle $\Delta\Omega(\Theta)=\Delta\Omega_0\,P(\Theta)$
(correction 1), the detector window's angular absorption $T_{\rm det}^{1/\cos\Theta-1}=1/w(\Theta)$ (correction 3),
the sample's thickness $d$, transmission $T$ and angular part $f_\Theta$ (correction 2), and the cell's own
scattering $C(\Theta)$:

$$
\begin{aligned}
B &= \text{dark current (B4C / Cd run)},\\
E &= B + \frac{\varepsilon\,\Phi\,\Delta\Omega_0\,P(\Theta)}{w(\Theta)}\;C(\Theta)
   && \text{empty cell (EC)},\\
S &= B + \frac{\varepsilon\,\Phi\,\Delta\Omega_0\,P(\Theta)}{w(\Theta)}\;
   T f_\Theta\left[d\,\frac{d\Sigma}{d\Omega}(\Theta) + C(\Theta)\right] && \text{sample in its cell}.
\end{aligned}
$$

### Why each term is corrected

| Term | Why |
|---|---|
| $B$, dark current | electronic noise, cosmic and room background: in every run, whether the beam passes the sample or not, so it is subtracted from the sample and from the EC **unscaled** |
| $E$, empty cell | the cell's windows (and air) scatter too; in the sample run that scattering is attenuated by the sample, so the EC is subtracted **scaled by** $T f_\Theta$ (approximation: the cell's scattering is taken to cross the sample like the sample's own, $f_\Theta$; for the exit window it does not) |
| $T$ | the sample attenuates the beam: the measured intensity is $T$ times the unattenuated one at $\Theta=0$ |
| $f_\Theta$ | at the angle $\Theta$ the path after the scattering is longer, $(d-z)/\cos\Theta$, so more is absorbed than $T$ accounts for: $T_\Theta = T f_\Theta$ |
| $P(\Theta)$ | the pixel's solid angle shrinks with the angle ($\cos^3\Theta$ flat, $\cos\alpha\cos^2\Theta$ tubes): equal $d\Sigma/d\Omega$ gives fewer counts at the outer pixels |
| $w(\Theta)$ | the detector window is crossed obliquely: $T_{\rm det}^{1/\cos\Theta}$ instead of $T_{\rm det}$ |
| $\varepsilon$ | pixel efficiencies differ: the sensitivity matrix $\hat s \propto 1/\varepsilon$, measured with a flat scatterer whose own reduction gets corrections 1 and 2 |
| $\Phi\,\Delta\Omega_0$, $d$ | the absolute factor $F$ (calibrant or direct beam) and the thickness give cm⁻¹ |

### What DAN computes

With the mask $M$ and the sensitivity $\hat s$, the script-table reduction (`danDanMultiButton`) evaluates per row

$$
\frac{d\Sigma}{d\Omega}(\Theta) = \frac{F}{d\,T}\;\hat s\;M\;
\frac{w\,\big[S - B\big] - T f_\Theta\,w\,\big[E - B\big]}{P(\Theta)\,f_\Theta},
$$

which, with the model above, gives $d\Sigma/d\Omega$ exactly ($w$: the row's window factor over the sensitivity's).
The steps:

1. $S$, $E$ (and the buffer) read, normalized;
2. $B$ subtracted from each (with BC time normalization: $B$ rescaled to the run's monitor); then $w$ multiplied in
   (correction 3, if checked);
3. $E$ multiplied by $f_\Theta$ (correction 2, if checked) and by $T$, and subtracted from the sample;
4. the mask applied;
5. the sample divided by $P$ (correction 1) and by $f_\Theta$ (correction 2), the buffer likewise with its own $T$;
6. the buffer subtracted, the factor $F/(dT)$, the sensitivity $\hat s$, the constant background, the negative points,
   the smoothing and the outputs.

**Buffer modes.** The buffer run $S_b$ (thickness $d_b$, transmission $T_b$, factor $F_b$, volume fraction $\phi$)
is reduced like the sample, with its own $T_b$ and $f_{\Theta,b}$, and subtracted:

$$
\frac{d\Sigma}{d\Omega} = \frac{F}{d\,T}\,\hat s\,M\left[
\frac{w[S-B]-T f_\Theta\, w[E-B]}{P\,f_\Theta}
- \phi\,\frac{F_b\,d\,T}{F\,d_b\,T_b}\,\frac{w[S_b-B]-T_b f_{\Theta,b}\, w[E-B]}{P\,f_{\Theta,b}}\right],
$$

i.e. the sample reduced alone minus $\phi$ times the buffer reduced alone. Until 2026-11-03 the buffer's EC term had
$f_\Theta$ of the sample's $T$ (KWS3 at 1.2 m, $T\approx0.5$, $T_b=0.9$: $1.8\cdot10^{-4}$ of the curves; at
30°, $T=0.5$, $T_b=0.9$: the buffer's EC term 4.4 % too small).

The calibrant (absolute factor) and the sensitivity measurements get the corrections 1 and 2 too
([dan18-process.cpp](../dan18-process.cpp), [dan18-sens.cpp](../dan18-sens.cpp)), so the factor and the sensitivity
are consistent with the data. The calibrant's Tr(Θ) follows its own option (since 2026-11-03: it came with
"Parallax", its empty beam got $f_\Theta$ without the calibrant being divided by it, and with Tr(Θ) alone neither).

**The center.** Every wide-angle correction is centered on the beam of its measurement (since 2026-11-03):

| Measurement | Center $(X_c, Y_c)$ |
|---|---|
| a row of the script table (both reduction paths) | the row's X-center, Y-center: its configuration's beam center |
| the calibrant of a configuration | the configuration's beam center (the centers are calculated before the factors: `calculate()`, the table's "all" button); none yet: the mask's beam-stop box |
| the sensitivity | the beam center of the configuration its run belongs to; none: the mask's beam-stop box; the center fields when their group is checked ("manual"); kept in its label, `::Center::x\|y\|configuration` |

Before, corrections 1 and 2 of the rows and the calibrant used the center of the mask's beam-stop box: another point
when the box was not on the beam (a mask without beam stop: the instrument's box), and another one than the window
factor and the single-row path used. KWS1-2020: up to 0.2 % at the largest angles of 1.98 m (the box 1.3 pixel off
the beam); KWS3-2020: $2\cdot10^{-5}$. `checkRowAgainstSensitivity()` warns when a row's center differs from its
sensitivity's by more than half a pixel.

## Python

`Dan.Processing.Options.setWideAngle(parallax=None, transmission=None, window=None)`: parallax `"off"`, `"flat"`
(type 0), `"tubes"` (type 1); `transmission`, `window`: True / False; None: as it is.
`Dan.Processing.Options.wideAngle()` → `{"parallax": …, "transmission": …, "window": …}`. The window's
$T_{\rm det}$ is the script table's column `TrDet`.

## Tests

`tests/sans/dan/dan_wide_angle_test.py`: type 1 finite and within 0.5 % of type 0 at a small angle; with Tr(Θ) the
buffer subtraction equal to sample alone minus buffer alone;
`dan_wide_angle_options_test.py`: `setWideAngle()`/`wideAngle()`; a mask with its beam stop in a corner gives the
I[Q] of one with it on the beam; the 2D image with the window factor is the one without times the row's over the
sensitivity's factor, pixel by pixel (after the dark current);
`dan_sensitivity_center_test.py`: the sensitivity's center is its configuration's;
`dan_kws1_reduction_test.py`, `dan_auto_mode_test.py`: the KWS1-2020 reduction with type 1.

## References

1. A. Brûlet, D. Lairez, A. Lapp, J.-P. Cotton, "Improvement of data treatment in small-angle neutron scattering",
   *J. Appl. Cryst.* **40**, 165–177 (2007), doi:10.1107/S0021889806051442 — the solid angle at large angles and the
   angle-dependent transmission of the sample, $\kappa = 1/\cos\Theta - 1$.
2. B. R. Pauw, "Everything SAXS: small-angle scattering pattern collection and correction",
   *J. Phys.: Condens. Matter* **25**, 383201 (2013), doi:10.1088/0953-8984/25/38/383201 (open access,
   arXiv:1306.0637) — the corrections of a scattering pattern, solid angle and self-absorption among them.
3. L. He, C. Do, S. Qian, G. D. Wignall, W. T. Heller, K. C. Littrell, G. S. Smith, "Corrections for the geometric
   distortion of the tube detectors on SANS instruments at ORNL", *Nucl. Instrum. Methods Phys. Res. A* **775**,
   63–70 (2015), doi:10.1016/j.nima.2014.11.061 (arXiv:1405.2008) — $\cos\alpha\cos^2(2\theta)$ for tube detectors.
4. Mantid, `SANSSolidAngleCorrection` v1: https://mantidproject.github.io/docs-nightly/algorithms/SANSSolidAngleCorrection-v1.html
5. Mantid, `SANSWideAngleCorrection` v1: https://mantidproject.github.io/docs-nightly/algorithms/SANSWideAngleCorrection-v1.html
