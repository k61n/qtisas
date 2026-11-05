# Output formats of DAN: I[x,y], I[Q], SAS presentations

Three pages of the Processing options decide what a reduction writes: "I[x,y] :: Output Format of ASCII-files"
(2D data into files), "I[Q] :: Output Format" (the columns of the I[Q] curves) and "I[Q] :: SAS Presentation"
(Guinier, Zimm, … instead of I vs Q). Where the output goes — the project or files — is the Processing tab's
"Open in project" / files choice (Python: `process(s, kind, output="project" | "files")`). The formulas describe the
code as of 4 November 2026; the functions named are in `qtisas/sans/dan/`. The averaging that makes the curves:
[dan-radial-averaging.md](dan-radial-averaging.md); decisions, dated: [dan-decisions.md](dan-decisions.md).

## The options

| Widget | Instrument file | Session | Values |
|---|---|---|---|
| `comboBoxIxyFormat` | `[Options-2D-Output-Format]` | `Options::2D::OutputFormat` | 0 "I[x,y] >> Matrix", 1 "Table :: x-y-I-dI", 2 "Table :: Qx-Qy-I-dI", 3 "Table :: Qx-Qy-I-dI-Qz-σQ_para-σQ_perp-mask", 4 "Polar :: Q-φ-I-dI" |
| `checkBoxASCIIheaderIxy` "+header" | `[Options-2D-Header-Output-Format]` | `Options::2D::HeaderOutputFormat` | a line of column names |
| `checkBoxASCIIheaderSASVIEW` "+header: like SASVIEW(NIST)" | `[Options-2D-Header-SASVIEW]` | `Options::2D::HeaderSASVIEW` | the NIST lines `Data columns are …` and `ASCII data` before the data |
| `checkBoxASCIIignoreMask` "ignore mask" | `[Options-2D-Output-IgnoreMask]` (No in every built-in instrument) | `Options::2D::IgnoreMask` | masked pixels written too |
| `comboBox4thCol` | `[Options-1D-OutputFormat]` | `Options::1D::I[Q]::Format` | 0 Q-I-dI-σ, 1 Q-I-dI-dQ, 2 Q-I-dI, 3 Q-I-dI-dQ-σ |
| `checkBoxASCIIheader` "+header" | `[Options-1D-OutputFormat-PlusHeader]` | `Options::1D::I[Q]::PlusHeader` | a `###` line of the columns in I[Q] files, for every presentation |
| `checkBoxAnisotropy` "+anisotropy" | `[Options-1D-Anisotropy]` | `Options::1D::I[Q]::Anisotropy` | an anisotropy column |
| `spinBoxAnisotropyOffset` | `[Options-1D-AnisotropyAngle]` | `Options::1D::I[Q]::AnisotropyAngle` | 0..360° |
| `comboBoxSelectPresentation` | `[Options-1D-QI-Presentation]` | `Options::1D::SASpresentation` | QI, Guinier, Zimm, Porod, Porod2, logQ, logI, Debye, 1Moment, 2Moment, GuinierRod, GuinierPlate, Kratky |

## I[x,y]

**Into the project** an I-x-y is always a matrix, `I-<row>`; its axes are pixels, or with "I[x,y] :: Dimension"
in Q units $\frac{4\pi}{\lambda}\sin\left(\tfrac12\arctan\frac{(k - X_c)\,b\,p}{D}\right)$ at the first and last
column ($k$ 1-based, $X_c$ 0-based, $b$ binning, $p$ pixel size; rows the same with the pixel height). dI-x-y:
the matrix of absolute errors.

**Into files** (`ASCII-I/` with "sort output to folders"), `saveMatrixToFile` for format 0 (the image, MD lines of
MD values, no header) and `saveMatrixAsTableToFile` for the others: one line per unmasked pixel (all pixels with
"ignore mask"), rows of the image outer, columns inner. With pixel column $j$, row $i$ (0-based), center
$(X_c, Y_c)$ (0-based), pixel width $p_x = b\,p$ and height $p_y = a_y b\,p$, distance $D$:

| Format | Columns |
|---|---|
| 1 x-y | $j+1$, $i+1$, I, dI |
| 2 Qx-Qy | $Q_x = Q\,x/r$, $Q_y = Q\,y/r$ with $x = (j - X_c)p_x$, $y = (i - Y_c)p_y$, $r = \sqrt{x^2+y^2}$ and $Q$ as in the polar format: the components of the pixel's Q, $Q_x^2 + Q_y^2 = Q^2$ (as SasView's 2D models take it); I, dI |
| 3 Qx-Qy-…-mask | as 2, then $Q_z = 0$, σQ(para), σQ(perp), the beam-stop shadow factor (the mask value: 1 in the data, 0 masked) and SasView's mask (1 masked, 0 in the data): SasView's 2D columns |
| 4 polar | $Q = \frac{4\pi}{\lambda}\sin\left(\tfrac12\arctan\frac{r}{D}\right)$; φ in degrees: 0 toward increasing rows, 90 toward increasing columns; I, dI |

σQ(para) is the pixel's resolution σ (`sigmaGslMatrix`, as the I[Q] σ); σQ(perp) is
$\sqrt{\sigma^2 - \sigma_\lambda^2}$: σ without the wavelength spread's term, which widens Q along the radius only,
as `sigmaQmerged` has it, $\sigma_\lambda = Q\,\Delta\lambda/\sqrt6$ with and without focusing (the lenses'
chromatic term blurs both directions and stays). Masked pixels written with "ignore mask" have both σ = 0, the
shadow factor 0 and SasView's mask 1.

"+header" writes the column names (`x y I[x,y] dI[x,y]`, `Qx Qy …`, `Q phi …`). "+header: like SASVIEW" writes
the two lines SasView's 2D reader (`red2d_reader`, the NIST format) looks for:

```
Data columns are Qx - Qy - I(Qx,Qy) - err(I) - Qz - SigmaQ_parall - SigmaQ_perp - fSubS(beam stop shadow) - mask
ASCII data
```

(the first without the last five names for the four-column formats). The reader takes the 4th column as err(I) only
after a `Data columns` line naming `err(I)` — without it as Qz —, reads the data after `ASCII data`, and ignores
other header lines. Its 8th column is the beam-stop shadow factor, which SasView does not apply; the 9th is its mask
(1: left out), so pixels written with "ignore mask" stay masked in SasView.

## I[Q]

The table `QI-<row>` (project) or the file `QI-<row>-<instrument>.DAT` (`ASCII-QI/`) holds Q, I, dI and, by
"I[Q] :: Output Format", σ (`Sigma`, the resolution, `sigmaQmerged`), dQ (`dQ`) or both, and with "+anisotropy" a
column `Anisotropy` = $\Sigma I\cos 2(\varphi + \varphi_0) / \Sigma I$ of the point's pixels ($\varphi_0$ the
offset). σ and dQ: [dan-radial-averaging.md](dan-radial-averaging.md).

A file's header ("+header") names the columns of the presentation, I in 1/cm or in counts ("Counts per Channel"),
aligned with the numbers (`dan18::curveHeader`):

```
###     Q[1/A]          I[1/cm]         dI[1/cm]          dQ[1/A]       Sigma[1/A]    Anisotropy[1]
### Q^2[1/A^2]            ln(I)          d(ln I)    d(Q^2)[1/A^2]       Sigma[1/A]
```

(QI, and Guinier; dQ is that of the x column, σ always in Q). The radial averaging, the MS mode (both files) and
the slices write it.

The I[Q] slices (I-Qx, I-Qy) and the MS mode use the same columns.

## SAS presentations

`dan18::presentation()`: each point $(Q, I, dI, dQ)$ becomes $(x, y, dy, dx)$, $dy = |dy/dI|\,dI$,
$dx = |dx/dQ|\,dQ$; $I = 0$ gives $y = dy = 0$ where $y$ needs $1/I$ or $\log I$:

| Name | x | y | dy |
|---|---|---|---|
| QI | Q | I | dI |
| Guinier | Q² | ln\|I\| | dI/\|I\| |
| Zimm | Q² | 1/I | dI/I² |
| Porod | log₁₀ Q | log₁₀\|I\| | dI/(\|I\| ln 10) |
| Porod2 | Q² | I Q⁴ | dI Q⁴ |
| logQ | log₁₀ Q | I | dI |
| logI | Q | log₁₀\|I\| | dI/(\|I\| ln 10) |
| Debye | Q² | \|I\|^-1/2 | dI/(2\|I\|^3/2) |
| 1Moment | Q | I Q | dI Q |
| 2Moment, Kratky | Q | I Q² | dI Q² (one plot, two names: the Kratky plot is the second moment) |
| GuinierRod | Q² | ln\|I Q\| | dI/\|I\| |
| GuinierPlate | Q² | ln\|I Q²\| | dI/\|I\| |

The outputs are named with the presentation (`QI-Guinier-<row>`); the columns keep their names Q, I, dI, dQ,
Sigma. **dQ follows the Q column** ($dx$ above: Guinier $2Q\,dQ$, Porod $dQ/(Q\ln 10)$); **σ does not**: it stays
the resolution in Q, independent of the presentation. The page's label shows the axes ("ln[I] vs. Q²").

## The resolution σ: pinhole and focusing

The σ column of I[Q] and σQ(para) of the Qx-Qy files come from `dan18::sigmaQmerged` (per pixel `sigmaGslMatrix`,
the same with one ring), [dan18-process-dandan.cpp](../dan18-process-dandan.cpp). σ is a Gaussian standard deviation
[Å⁻¹], the square root of the sum of the variances of independent contributions, each from its own shape: a box of
width $w$ has $w^2/12$, a uniformly lit disc of radius $r$ $r^2/4$ in one direction, a triangle of FWHM $\Delta$
$\Delta^2/6$ ($0.4247 = 1/(2\sqrt{2\ln 2})$ below turns a Gaussian's FWHM into its σ).

### The options

"Resolution:: Options" of the instrument page; each is written into the instrument file and the session and read back:

| Option | Widget | Instrument file | Session |
|---|---|---|---|
| "Focusing Geometry (Lenses, Mirror)" | `checkBoxResoFocus` | `[Resolusion-Focusing]` Yes / No | `Options::Resolusion::Focusing` |
| the focusing optic, "lenses" or "mirror" (beside it, shown only when it is checked) | `comboBoxResoFocusType` | `[Resolusion-Focusing-Type]` | `Options::Resolusion::Focusing::Type` (none in older sessions: lenses) |
| "Entrance Aperture [CA] :: Round (Diameter)" | `checkBoxResoCAround` | `[Resolusion-CA-Round]` | `Options::Resolusion::CA::Round` |
| "Sample Aperture [SA] :: Round (Diameter)" | `checkBoxResoSAround` | `[Resolusion-SA-Round]` | `Options::Resolusion::SA::Round` |
| "Detector Space Resolusion" [cm] | `lineEditDetReso` | `[Resolusion-Detector]` | `Options::Resolusion::Detector` |

Focusing is on in 11 of the 21 built-in instruments: the KWS-3 ones with a mirror, KWS2-HRD with lenses. From Python:
`Dan.Options.Config.setParameters({"Resolusion-Focusing-Type": "mirror", ...})` (any of the keys above).

### Parameters

| Symbol | Meaning | Source |
|---|---|---|
| $\lambda$, $k = 2\pi/\lambda$ | wavelength [Å] | the row (from the run's header or the selector) |
| $\Delta\lambda$ | relative wavelength spread (FWHM / λ) | the header's `[Delta-Lambda]` |
| $C$ | collimation length [cm] | the row (from the header) |
| $D$ | sample-detector distance [cm] | the row (from the header) |
| $r_1$ | radius of the source (collimation) aperture [cm]: of a round aperture that spreads the angle alike | `[CA-X]` × `[CA-Y]`: round: $\sqrt{xy}/2$, the radius of a diameter; a rectangle: $\sqrt{(x^2+y^2)/6}$, its variance $x^2/12$ along x and $y^2/12$ along y, their mean $(x^2+y^2)/24 = r^2/4$ |
| $r_2$ | radius of the sample aperture [cm] | `[SA-X]` × `[SA-Y]`, the same with "SA Round" |
| $w$ | the pixel width [cm] | $b\,\frac{1+a_y}{2}\,p$: binning $b$, pixel size $p$ (`lineEditResoPixelSize`), asymmetry $a_y$ |
| $w_{\rm AV}$ | the width of the averaging ring [cm] | $m\,w$, $m$ the rings merged into the point |
| $\Delta_{\rm D1}$ | the detector's spatial resolution, its real pixel: positions closer than it give one signal [cm] | "Detector Space Resolusion"; not set (0): $w$ |

### Pinhole

Focusing off. Each contribution enters with its variance (Mildner & Carpenter 1984; the NIST reduction), the angle
factors are those of Pedersen, Posselt & Mortensen (1990). With $\theta_0 = \arcsin(Q/2k)$ (half the scattering
angle) and $c = \cos^2 2\theta_0$:

$$
\sigma^2 = \frac{Q^2\Delta\lambda^2}{6}
+ k^2\cos^2\theta_0\,\frac14\left[\frac{r_1^2}{C^2} + r_2^2\left(\frac1C + \frac cD\right)^2\right]
+ \left(\frac{k\cos\theta_0\,c}{D}\right)^2\frac{\Delta_{\rm D1}^2 + w_{\rm AV}^2}{12}
+ \frac{k^2}{2D^2}\,\bigl(2A\lambda^2(C + D)D\bigr)^2\frac{\Delta\lambda^2}{6}:
$$

- "Sigma-W", the wavelength: the velocity selector's triangular distribution of FWHM $\Delta\lambda$ has the
  variance $\Delta\lambda^2/6$;
- "Sigma-C1", the collimation: a uniformly lit circle of radius $r$ spreads the angle with the variance $r^2/4$
  in one direction; the source aperture seen from the sample at $C$, the sample aperture from the detector at
  $1/(1/C + c/D)$;
- "Sigma-D1", the detector's spatial resolution, its real pixel: a box of width $\Delta_{\rm D1}$ (as a Gaussian
  of that FWHM, $(0.4247\,\Delta_{\rm D1})^2$, the variance would be 2.2 times larger; a two-point resolution is
  0.85–1.2 FWHM of a Gaussian point-spread function);
- "Sigma-AV", the averaging ring: all radii across its width $w_{\rm AV}$ count alike, a box of variance
  $w_{\rm AV}^2/12$;
- gravity: a neutron falls by $A\lambda^2z^2$ over the flight $z$ ($A = g\,m_n^2/2h^2 = 3.133\cdot10^{-9}$
  cm⁻¹Å⁻²). The two apertures fix its parabola at the source aperture and the sample: behind the sample it falls by
  $A\lambda^2(C + D)D$ more than the straight line, the beam center taking the nominal wavelength's. A wavelength
  $\lambda(1+\varepsilon)$ falls $2\varepsilon$ times more: a vertical spread of variance
  $(2A\lambda^2(C+D)D)^2\Delta\lambda^2/6$, half of it in the radial average. Negligible at short distances and
  wavelengths (8 m / 5 Å: 0.1 mm of fall); at 20 m / 10 Å a spread of 2 mm, as wide as a pixel's; at 40 m / 20 Å
  (a fall of 40 cm) 3.3 cm, two to three times all the other terms at small Q.

**Why variances.** Variances of independent contributions add whatever their shapes (a triangle, a box, the
trapezoid of two apertures' overlap); widths add in quadrature only for Gaussians. And a smooth I(Q) smeared by any
resolution function is, to the leading order, $I + \tfrac12\sigma^2 I''$: only the variance enters, so the Gaussian
that a fit applies (Fittable, SasView/sasmodels: σ is the standard deviation) needs the true variance. Pedersen et
al. (1990) give each term as a Gaussian of the same FWHM; for the aperture overlap that is 10–25 % more than its
standard deviation (they note it is poor when the two apertures differ by more than a factor of two).

**Checked** against a Monte Carlo simulation of the setup (square apertures 3 × 3 and 1 × 1 cm, a flat detector,
a triangular wavelength distribution, the detector's resolution and the ring as boxes): the formula is within 1 %
of the simulated standard deviation for $C, D$ = 8/8, 8/1.5 and 20/20 m (3–11 % above it only next to the beam,
$Q$ = 0.002–0.003 Å⁻¹ at 20 and 8 m), and within 0.1 % up to $2\theta = 57°$ with the angle factors (1 % without
them). NIST's `SANSResolution.java` has the same variances, the detector resolution as a Gaussian FWHM; SasView's
calculator has the pixel only, no detector resolution.

### Focusing

A symmetric optic (lenses or a mirror) images the source onto the detector 1:1: source and detector both at $2f$
from it, $2f = C$ (the collimation length); the sample sits at $D$ (the sample-detector distance, offset included)
before the detector. On KWS-3 the detector is always at $2f = C$ (11 m in the built-in KWS-3 instruments) and $D$
goes from 10 m down to a few centimetres; KWS2-HRD (lenses) has $2f = C = 19$ m.

$$
\sigma^2 = k^2\left[\frac{r_1^2}{4D^2} + \chi\left(\frac{2}{3}\,\frac{\Delta\lambda^2\,r_2^2\,C^2}{D^4}
+ \frac43\,\frac{A^2\lambda^4C^4\Delta\lambda^2}{D^2}\right)
+ \frac{\Delta_{\rm D1}^2 + w_{\rm AV}^2}{12\,D^2}\right] + \frac{Q^2\Delta\lambda^2}{6}:
$$

- the source's image (1:1), a uniformly lit disc of radius $r_1$, seen from the sample: variance $r_1^2/4$ over
  $D^2$;
- the chromatic blur, lenses only ($\chi = 1$; a mirror is achromatic, $\chi = 0$): the focal length of a lens goes
  with $1/\lambda^2$, so a wavelength $\lambda(1+\varepsilon)$ moves the image plane by $-4\varepsilon C$ and a ray
  that crossed the optic at the height $h$ lands $4\varepsilon h$ off the image. The beam at the optic has the
  radius $r_2C/D$ (it converges onto the detector through the sample aperture $r_2$): variance
  $16\cdot\frac{\Delta\lambda^2}{6}\cdot\frac{(r_2C/D)^2}{4}$, seen from the sample over $D^2$. Valid while the
  shifted focus stays well behind the sample, $p = 4\Delta\lambda(C-D)/D \lesssim 0.3$ (within 1–3 % of the
  simulation; 6 % below it at $p$ = 0.36, 30 % at 0.75). With lenses the sample is always next to them, $D \approx C$,
  so $p \approx 0$;
- gravity, lenses only (also $\chi$; a mirror compensates it): a neutron falls by $A\lambda^2z^2$ over the flight
  $z$, $A = g\,m_n^2/(2h^2) = 3.133\cdot10^{-9}$ cm⁻¹Å⁻²; through the lenses from source to detector
  $2A\lambda^2C^2$, whatever the ray (they image the fall too). Its wavelength spread, $4A\lambda^2C^2\varepsilon$,
  is vertical: in the radial average half its variance, $\frac12(4A\lambda^2C^2)^2\frac{\Delta\lambda^2}{6}$, over
  $D^2$. At $2f$ = 19 m and Δλ 0.1: σ 3 % larger at 7 Å, up to 1.6 times at 15 Å (the smallest Q);
- the detector as without focusing (below);
- the wavelength, the selector's triangle, $Q^2\Delta\lambda^2/6$, whatever the optic.

A mirror's σ depends on $D$ only; the sample aperture enters only through the lenses' chromatic term.
Without focusing gravity enters as in the pinhole formula above (the fall through the two apertures).

**Checked** against a Monte Carlo simulation of ideal 1:1 imaging (rectangular apertures, the image plane moved by
the wavelength for lenses, the scattering at the sample, the detector's resolution and the ring as boxes): within
1 % for KWS-3 (mirror, 12.8 Å, Δλ 0.15, 9.2 m and 1.2 m; 2–3 % above at the smallest Q) and for lenses at
$2f$ = 19 m ($D$ = 19 m; 10 m with Δλ 0.05); with gravity (the fall along every flight, the beam center at the
nominal wavelength's spot) within 1–4 % at 7 and 15 Å, $D$ = 19 and 12 m. The former terms were up to 40 % below
it, the most at large Q.

**The detector term.** A blur on the detector is seen from the sample at $D$ whatever the optic, so it enters as
without focusing. Inside the optic's bracket it was $0.4247^2\,k^2R_{\rm det}^2/(3D^2)$ with
$R_{\rm det} = \max(w_{\rm AV}, \Delta_{\rm D1})/\sqrt\pi$: the width taken at once as a Gaussian's FWHM
($0.4247^2$), as a box's half-width ($1/3$) and as the radius of a circle of the pixel's area ($1/\pi$); the source
term of the bracket cancels its $1/3$ ($3r_1^2$), the detector's did not. The variance is now 5 times the former on
KWS-3 ($\Delta_{\rm D1}$ = 0.08 cm, $w$ = 0.034 cm). This change alone, on the KWS-3 example runs (mirror, 9.2 m
and 1.2 m), makes σ 7 % larger at the smallest Q, 2 % at 10 times that Q and nothing at 30 times; the detector's
share of σ² at the smallest Q is 16 % (it was 3 %).

### Directions and where σ is used

Only the wavelength term $Q\Delta\lambda$ widens Q along the radius alone; the others are geometric and blur both
directions. So σQ(para) = σ and σQ(perp) = $\sqrt{\sigma^2 - \sigma_\lambda^2}$ in the model's own $\sigma_\lambda$
($Q\Delta\lambda/\sqrt6$, with and without focusing). Gravity (pinhole, focusing lenses) is vertical: σ holds half its variance $g$
(the radial average); a pixel at the azimuth φ from the horizontal gets $g\sin^2\varphi$ in σQ(para) and
$g\cos^2\varphi$ in σQ(perp) instead.

- I[Q]: `sigmaQmerged` with the point's merged rings $m$ (MS mode the same).
- The slices I-Qx, I-Qy: σ at the slice's own Q, $|Q_x|$ or $|Q_y|$ (the wavelength term along x is
  $Q_x\Delta\lambda$), $m = 1$. They write σ only with "Q-I-dI-σ"; with the other choices Q, I, dI.
- Per pixel: `sigmaGslMatrix` (σQ(para) of the Qx-Qy files), the matrix Sigma-x-y (`sigmaMatrix`); the pixel's
  radius with the asymmetry, $r = p_x\sqrt{(j - X_c)^2 + a_y^2 (i - Y_c)^2}$.

### dQ

A separate, simpler Q uncertainty, the same for an I[Q] point and a pixel (`dQ`; the matrix dQ-x-y):

$$
dQ = Q\sqrt{\Delta\lambda^2 + \delta_D^2 + \left(\frac{w_{\rm point}}{2r}\right)^2},
$$

$\Delta\lambda$ the run's relative wavelength spread (0.01 when the header has none), $\delta_D$ = 1, 2 or 5 cm over
$D$ (D < 50 cm, < 400 cm, more), $w_{\rm point}$ the width of the point (its rings: $m$ ring widths; a pixel: its
width) and $r$ its radius on the detector ($0$ at $Q = 0$: no radius term).

## Python

`Dan.Processing.Options`: `setMatrixFormat(format, header, sasview, ignoreMask)` / `matrixFormat()` (`"matrix"`,
`"xy"`, `"qxqy"`, `"qxqy-resolution"`, `"polar"`), `setCurveFormat(columns, header, anisotropy, anisotropyOffset)`
/ `curveFormat()` (`"sigma"`, `"dq"`, `"none"`, `"dq-sigma"`), `setPresentation(name)` / `presentation()`; None
keeps an option. Docs: `qtisas/python/doc/dan-python-api.md`.

## Tests

[dan_export_options_test.py](../../../tests/sans/dan/dan_export_options_test.py): the Python options and their
errors, the label of the presentation set by a script, the I[Q] table's columns for each choice (with the
anisotropy column), the five I-x-y file formats (header line, number of columns, 1-based pixels, the `ASCII data`
line), |(Qx, Qy)| = the polar file's Q pixel by pixel, σQ(perp) ≤ σQ(para) with σQ(para)² − σQ(perp)² ∝ Q², the
9th column 1 − the 8th, with "ignore mask" every pixel, the masked ones with 1,
dQ = 2Q·dQ and σ unchanged under Guinier in SM and MS mode.
[dan_resolution_test.py](../../../tests/sans/dan/dan_resolution_test.py): the lenses / mirror option (shown only
with focusing, the instruments' values, `setParameters()`, the session), the chromatic term for lenses only (σ²
of lenses minus a mirror's the same at every Q), the slices' σ at their own Q, dQ of the matrix and of I[Q] alike
(no NaN), the pinhole detector terms (a resolution of one pixel = none; twice that adds $3w^2/12$ of detector
width), and
2f = C: a mirror's σ independent of C, the lenses' chromatic term ∝ C²; the focusing terms as variances: a
mirror's σ² linear in Q² with the source's and the detector's variance (apertures from the run's beam size) at
Q = 0, the lenses' chromatic term $\frac23\Delta\lambda^2 r_2^2C^2/D^4$ and gravity with the $\Delta\lambda$ of that
slope, and their sum growing as $4\cdot$chromatic $+ 16\cdot$gravity with twice $C$.
[ascii1d_resolution_test.py](../../../tests/sans/ascii1d/ascii1d_resolution_test.py): ASCII.1D's σ (the same
model, the pixel as both detector terms) against the formula above for three geometries and five Q, and with
"Focused?" with lenses and with a mirror against the focusing formula ($2f = C$, the sample at $D$), the optic
and "Colimation = 2f:" shown only when focused, a wrong optic a ValueError, and a format file keeping every
resolution option (an older `{Reso-Lens-Yes-No} Yes` read as focused lenses).

## Fixed (audit 2026-11-04)

| Point | Before |
|---|---|
| The presentation's label follows any change (instrument, session, Python) | only a change with the mouse |
| `setMatrixFormat()`, `setCurveFormat()`, `setPresentation()` and their getters | no Python access |
| MS mode: dQ follows the Q column's presentation, as in the other modes (σ stays in Q, as there) | dQ written as in QI |
| Qx, Qy: the components of the pixel's Q, $Q\,x/r$, $Q\,y/r$ | $\frac{4\pi}{\lambda}\sin(\tfrac12\arcsin\frac{x}{D})$ per axis: \|(Qx, Qy)\| 0.07 % above Q at the edge (KWS-3, 1.2 m), 2 % on the axis at $x/D = 0.2$ |
| The raw-data view (`danDanMultiButtonSingleLine`) writes the table formats 1–4 into files as the script table's processing does | the call was commented out (an old signature): no file, no message |
| The raw-data view's matrix in Q units: the pixel size times the binning from their own fields | `lineEditPS`, which follows only a binning chosen in the interface |
| Pinhole σ: "Sigma-D1" from the detector resolution (none: one pixel), "Sigma-AV" from the ring's pixels | both took the larger of the merged rings' pixel size and the detector resolution |
| Focusing: the chromatic term for lenses only; the optic chosen beside "Focusing Geometry" (lenses / mirror), in the instrument (`[Resolusion-Focusing-Type]`, the built-in instruments set) and the session | always added: a mirror (KWS-3) got it too |
| The slices' σ at the slice's own Q | the mean σ of the pixels' full Q |
| Focusing: $2f = C$ (the collimation), the sample at $D$: the source image $r_1/D$, the lenses' chromatic blur $4r_2C\Delta\lambda/D^2$ | $C = D$, $f = D/2$ fixed; the row's $C$ unused |
| One dQ for I[Q] and dQ-x-y, from the run's wavelength spread | two estimates: I[Q] $1/\sqrt{\text{selector rpm}}$ (else 0.01), 1/2/5 cm over D; the matrix 0.01, 1 cm over D, pixel over radius |
| dQ-x-y at $Q = 0$: no radius term | infinite: NaN under the mask |
| The per-pixel σ and dQ with the pixel asymmetry | without it (all built-in instruments have $a_y = 1$) |
| "ignore mask" in the instrument (`[Options-2D-Output-IgnoreMask]`, No in all 21) and the session | in neither: lost with them |
| The I[Q] file header for every presentation (its columns, I in 1/cm or counts) | QI only, always `I[1/cm]` |
| "+header: like SASVIEW" writes NIST's `Data columns … err(I) …` line before `ASCII data` | without it SasView took dI as Qz (wrong Q) and shifted the σ columns |
| Pinhole σ as the sum of variances: the wavelength $Q^2\Delta\lambda^2/6$, the apertures $r^2/4$, the detector resolution (the real pixel) and the ring as boxes, $(\Delta_{\rm D1}^2 + w_{\rm AV}^2)/12$; focusing: the same detector terms; ASCII.1D (and Fittable's "ASCII.1D.SANS") the same | Pedersen's Gaussians of equal FWHM ($0.4247\,Q\Delta\lambda$, $0.4247\,\Delta\beta_1$), the ring and the detector resolution as $w/\sqrt\pi$: 4–30 % above the simulated σ (KWS-1-like, 8/8 m: +16 % at 0.01 Å⁻¹); focusing: $\max(R_{\rm AV}, R_{\rm D1})$ in the optic's bracket, reduced by $0.4247^2/(3\pi)$: a fifth of the variance on KWS-3, σ up to 7 % smaller |
| Focusing σ as variances: the source's image $r_1^2/4$, the wavelength $Q^2\Delta\lambda^2/6$ (as without focusing), the lenses' blur $\frac23\Delta\lambda^2r_2^2C^2/D^4$ and their gravity $\frac43A^2\lambda^4C^4\Delta\lambda^2/D^2$ (not with a mirror); ASCII.1D's lenses the same | $0.4247^2\,k^2r_1^2$ (the radius as a FWHM), $0.4247^2Q^2\Delta\lambda^2/3$, $0.4247^2\cdot\frac83 k^2\Delta\lambda^2r_2^2C^2/D^4$: KWS-3 σ 10–40 % below the simulation, the most at large Q |
| ASCII.1D: "Focused?" (it was "Lenses?") as DAN's focusing, $2f$ = the collimation distance ("Colimation = 2f:"), the sample at the detector distance; the optic lenses / mirror beside it; `setResolutionFocusing(on, optic)`; a format file keeps focusing, the optic and the units of Q and λ | lenses only, with a focus field of their own and the source seen over $C$; the units were not saved |
| Gravity without focusing: the fall through the two apertures, $A\lambda^2(C+D)D$, spread by the wavelength (Monte Carlo: within 1–3 % at 40 m / 20 Å and 20 m / 10 Å) | left out: at 40 m / 20 Å σ was half the simulated one |
| Gravity in the Qx-Qy files (pinhole and focusing lenses): its vertical variance to σQ(para) by sin² of the pixel's azimuth, to σQ(perp) by cos² (`gravityVariance()`) | half of it in both, in every direction |
| Rectangular apertures by their variance, $r^2 = (x^2+y^2)/6$ (also ASCII.1D) | circles of the same area, $r^2 = xy/\pi$: 4.5 % less variance for a square |
| "qxqy-resolution": a 9th column, SasView's mask (1: masked), named in the `Data columns` line | with "ignore mask" the masked pixels counted in SasView (it ignores the 8th column, the shadow factor) |
| Focusing: σQ(perp) = σ without the wavelength term of the focusing model | σQ(perp) was the (non-focusing) wavelength term itself: 1.1·10⁻³ > σQ(para) 7.1·10⁻⁴ (KWS-3, 1.2 m) |

## Open points

None at the moment.

## References

- J. S. Pedersen, D. Posselt, K. Mortensen, *Analytical treatment of the resolution function for small-angle
  scattering*, J. Appl. Cryst. **23**, 321–333 (1990),
  [doi:10.1107/S0021889890003946](https://doi.org/10.1107/S0021889890003946) — the contributions W (wavelength), C1
  (collimation), D1 (detector resolution) and AV (averaging) of the pinhole σ.
- D. F. R. Mildner, J. M. Carpenter, *Optimization of the experimental resolution for small-angle scattering*,
  J. Appl. Cryst. **17**, 249–256 (1984),
  [doi:10.1107/S0021889884011468](https://doi.org/10.1107/S0021889884011468) — the collimation term, the variances
  of the resolution.
- D. F. R. Mildner, *Resolution of small-angle neutron scattering with a reflective focusing optic*, J. Appl. Cryst.
  **47** (2014), [NIST](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=915797) — the resolution of a
  focusing mirror (audit point 4).
- J. G. Barker, J. S. Pedersen, *Instrumental smearing effects in radially symmetric small-angle neutron scattering
  by numerical and analytical methods*, J. Appl. Cryst. **28**, 105–114 (1995),
  [doi:10.1107/S0021889894010095](https://doi.org/10.1107/S0021889894010095) — the Gaussian resolution function
  checked against the exact smearing.
- G. Goerigk, Z. Varga, *Comprehensive upgrade of the high-resolution small-angle neutron scattering instrument
  KWS-3 at FRM II*, J. Appl. Cryst. **44**, 337–342 (2011),
  [doi:10.1107/S0021889811000628](https://doi.org/10.1107/S0021889811000628) — KWS-3, the focusing-mirror instrument.
- SasView's Q resolution calculator,
  [resolution_calculator.py](https://github.com/SasView/sasview/blob/main/src/sas/sascalc/calculator/resolution_calculator.py)
  — the same variances (apertures, pixel $w^2/12$, wavelength $\Delta\lambda^2/6$), no detector resolution.
- Implementations for comparison: jscatter `resFunct` (Pedersen et al. 1990, eq. 33,
  [source](https://jscatter.readthedocs.io/en/latest/_modules/jscatter/smallanglescattering.html)); NIST
  [SANSResolution.java](https://webster.ncnr.nist.gov/resources/sansmodels/SANSResolution.java) (the variances,
  detector FWHM/2.3548 and pixel $w^2/12$; DAN takes the detector resolution as a box); Mantid
  [ReactorSANSResolution](https://mantidproject.github.io/docs-nightly/algorithms/ReactorSANSResolution-v1.html)
  (Mildner–Carpenter).
