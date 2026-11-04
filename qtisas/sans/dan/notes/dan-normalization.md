# Data normalization in QtiSAS/DAN

The page "I[x,y] :: Data Normalization" of the Processing options sets how DAN puts runs of different length and
flux on one scale before they are subtracted from each other: per monitor count or per second. The formulas
describe the code as of 4 November 2026; the functions named are in `qtisas/sans/dan/`. The rest of the reduction:
[dan-reduction-error-propagation.md](dan-reduction-error-propagation.md); decisions, dated:
[dan-decisions.md](dan-decisions.md).

## The options

| Widget | Instrument file | Values |
|---|---|---|
| `comboBoxNorm` | `[Options-2D-Normalization-Type]` | 0 "Monitor2", 1 "Time", 2 "Monitor1" (the index; a session stores the text) |
| `spinBoxNorm` | `[Options-2D-Normalization-Factor]` | the constant $N$, an integer 1 … 10⁶; disabled in the GUI, set by the instrument file or a session (every built-in instrument: 1) |
| `checkBoxBCTimeNormalization` "Subtract #BC with TIME normalization" | `[Options-2D-Mask-Normalization-BC]` | Yes / No |

The values read from the run's header ([Header] map of the instrument):

| Header value | Code | Units |
|---|---|---|
| `[Duration]` | `Monitors::readDuration` | converted to seconds by "Units: Time" (`comboBoxUnitsTime`, `[Units-Time]`: 0 s, 1 s/10, 2 ms, 3 µs) |
| `[Monitor-1]`, `[Monitor-2]` | `readMonitor1`, `readMonitor2` | counts |
| `[Monitor-3\|Tr\|ROI]` | `readMonitor3` | counts (Monitor-3, or the transmission/ROI value of the header) |
| `[Sum]` | `readSum` | detector counts, for the detector's dead time |
| `[RT-Time-Factor]`, `[RT-Number-Repetitions]` | `readTimefactor`, `readNumberRepetitions` | real-time (RT) runs |

## The normalization factor

Code: [monitors.cpp](../dan-header-devices/monitors.cpp). A monitor's counts $M$ in a run of duration $t$ are
corrected for the monitor's dead time $\tau$ (`[Monitor1/2/3-Dead-Time]`, seconds), always with the model
$m = n/(1+n\tau)$ (true rate $n$, measured rate $m$; the non-paralysable model, see the dead time below):

$$
M_{\rm c} = \frac{M}{1 - \tau M / t}\qquad(\text{no correction when } t = 0).
$$

`Monitors::normalizationFactor` gives the factor $f$ every image is multiplied by:

| Type | $f$ |
|---|---|
| Monitor2 | $N / M_{2,\rm c}$ |
| Monitor1 | $N / M_{1,\rm c}$ |
| Time | $N / t$ |

A monitor or a duration that is not positive gives $f = 0$: the run's image becomes zero. The results log (and a
script's `lastMessages()`) says so once per run and processing: "run … : no Monitor2 in the header (not positive):
it cannot be normalized, its image is set to zero" (signal `Monitors::message`).

Real-time runs have a second factor, `normalizationFactorRT`:
$f_{\rm RT} = $ `[RT-Time-Factor]` / `[RT-Number-Repetitions]`, and 1 when the time factor is 1 (or missing).

## A corrected image

Every image the reduction uses is read by `dan18::readMatrixCor`
([dan18-process-matrix.cpp](../dan18-process-matrix.cpp)):

$$
I_p = n_p \cdot c_{\rm DT} \cdot f \cdot f_{\rm RT},
$$

$n_p$ the counts of pixel $p$, $c_{\rm DT}$ the detector's dead-time factor (below). So a sample, its empty cell,
buffer, blocked beam, the calibrant and its empty beam are all **counts per $N$ monitor counts** (or per $N$
seconds) before they are subtracted from each other: S − T·EC, S − f·Buffer, … are differences of the same
quantity, whatever the runs' lengths.

The absolute factor of a configuration comes from the calibrant normalized the same way, so $N$, the monitor's
efficiency and the time cancel in I [cm⁻¹]; the normalization type only has to be the same for the calibrant and
the samples, which it is (one option for everything).

### The blocked beam (dark current) with time normalization

The blocked beam (B4C, `#BC`) measures what the detector counts without the beam: electronic noise and room
background, which grow with the **time**, not with the monitor. With "Subtract #BC with TIME normalization" the BC
run is read per second (`readMatrixCorTimeNormalizationOnly`: $f = N/t_{\rm BC}$) and then rescaled to the run X it
is subtracted from:

$$
\mathrm{BC}_X = \frac{n_{\rm BC}\,c_{\rm DT}}{t_{\rm BC}}\; t_X\; f_X ,
$$

the dark-current rate times X's duration, normalized as X is (`NormSample` = $f_X/(N/t_X)$ in the code). With the
type "Time" this is the same as the plain reading. Without the option the BC run is normalized by its own monitor,
which is right only for background that follows the beam.

X is the sample, the empty cell and the buffer of the row (`danDanMultiButton`, `danDanMultiButtonSingleLine`),
the plexiglass and empty beam of the calibrant (`dan18-process.cpp`), the flat scatterer and empty beam of the
sensitivity (`dan18-sens.cpp`), and the sample/EC of the centers and factors calculated in the script table.

### Counts per Channel

The calibration "Counts per Channel" (`comboBoxACmethod` 3) divides the reduced image by the sample's $f$ again:
the output is counts per pixel of the sample's own run (dead-time corrected, EC and BC subtracted at the sample's
normalization).

## The detector's dead time

$c_{\rm DT} = n/m$, the true count rate over the measured one, $m$ = `[Sum]`/$t$ of the run (`deadTimeFactorDetector`;
the direct beam's own dead time `lineEditDBdeadtime` in `deadTimeFactorDetectorDB`), $\tau$ = `lineEditDeadTime`
(`[Detector-Dead-Time]`). Two models, by the radio buttons (`[Options-2D-DeadTimeModel-NonPara]` Yes / No):

| GUI | Measured rate | $c_{\rm DT}$ | Code (`Monitors::deadTimeFactor`) |
|---|---|---|---|
| "Paralysable:" (`radioButtonDeadTimeCh`, default) | $m = n\,e^{-n\tau}$ | $n/m$ from the Newton root of $n e^{-n\tau} = m$ (relative step < 10⁻⁵), the lower root ($n < 1/\tau$); $e$ above the maximum $m > 1/(e\tau)$ | `deadTimeRoot.cpp` |
| "Non-Paralysable:" (`radioButtonDeadTimeDet`) | $m = n/(1+n\tau)$ | $1/(1-m\tau)$; 100 above $m > 0.99/\tau$ | closed form |

**Names** as in the usual nomenclature (Knoll, *Radiation Detection and Measurement*). Until 2026-11-04 the two
labels (and "Monitors Dead-Time [sec, … Model]") were the other way round; the formulas next to the buttons, and
the instrument key, were and are the same: `[Options-2D-DeadTimeModel-NonPara] Yes` is the exp model (every built-in
instrument). A session writes the model with its formula (`Options::2D::DeadTimeModel`); an older session's
"Non-Paralysable" is read as the exp model.

**Per photomultiplier** (KWS-1/KWS-2, `comboBoxDTtype`, only on an unbinned 128×128 detector, `deadtimeMatrix`):
"Detector" corrects the whole image with one $c_{\rm DT}$; "1x PhotoM" splits it into 8×8 photomultiplier blocks
(13 or 17 pixels a side) and corrects each with its own rate and $64\tau$; "9x PhotoM" each block with the rate of
it and its neighbours (3×3 blocks) and $4\tau$. The same model (exp or 1/(1+nτ)) as for the whole detector.

## Transmission from the header

`dan18::readTransmission` ([dan18-options-header.cpp](../dan18-options-header.cpp)) by `comboBoxTransmMethod`:

| Method | Transmission | Normalization |
|---|---|---|
| 0 "Monitor-3" | $\dfrac{M_{3,\rm c}^S\,f^S}{M_{3,\rm c}^{EC}\,f^{EC}}$ (`readMonitor3Normalized`) | each run by its own $f$ |
| 1 "Direct Beam" | the counts in the transmission mask, detector dead time replaced by the direct-beam one, times $f$ | each run by its own $f$ |
| 2 "Tr in Header" | $M_3^S / M_3^{EC}$ as written | none: the header holds the transmission |
| 3 "ROI in Header" | as 0, times the direct-beam dead-time factor of each run | each run by its own $f$ |
| 4 "λ<9.5 Å: ROI in Header; λ≥9.5 Å: Direct Beam" | 3 or 1 by the sample's wavelength | each run by its own $f$ |

The statistical error adds $1/M_3$ (or $1/n$ in the mask) of both runs and, for the monitor types, the monitors'
$1/M = f/N$.

## Errors

$f$, $f_{\rm RT}$ and $c_{\rm DT}$ are constants per run: they scale the intensity and leave the relative variance
of each pixel unchanged ([dan-reduction-error-propagation.md](dan-reduction-error-propagation.md) §2). The
monitor's own counting error ($1/M$ relative variance) is not carried into the pixel errors; it enters only the
transmission's error.

## Python

`Dan.Processing.Options.setNormalization(by=None, factor=None, darkCurrentByTime=None)`: `by` "monitor2", "time"
or "monitor1", `factor` 1..10⁶, `darkCurrentByTime` the BC option; None keeps the option. `normalization()` →
`{"by": …, "factor": …, "darkCurrentByTime": …}`. The instrument (`Dan.Options.selectInstrument()`) and the
session set them as in the GUI.

## Tests

[dan_normalization_test.py](../../../tests/sans/dan/dan_normalization_test.py), on a copy of the KWS-3 runs with
Monitor2 normalization: the buffer's dark current in a row without an empty cell (a buffer with the sample's counts
and monitor, measured twice as long, leaves the sample's dark current), the transmission's error for N = 1 and
1000, the message of a run without monitor counts, and the error bars of a row with half the buffer: its BC
cancels (−BC + BC/2·2), so I and dI are those of the same row without any BC; `setNormalization()` and its
ValueErrors. [dan_dead_time_test.py](../../../tests/sans/dan/dan_dead_time_test.py): both dead-time models, a
row's image with three dead times over the one without — the rate $m$ that each correction implies is the same
for all three ($\ln c/(c\tau)$, $(1-1/c)/\tau$).

## Fixed (audit 2026-11-03, fixes 2026-11-04)

| Point | Before |
|---|---|
| Monitor-3's dead time from `[Monitor3-Dead-Time]` (`lineEditDeadTimeM3`) | Monitor-2's field was passed for both |
| The buffer's BC rescaled to the buffer also in a row without an EC (both `danDanMultiButton*`) | `&& ECexist`: buffer − BC used the BC scaled to the sample's time/monitor |
| The transmission error adds the monitors' $1/M = f/N$ (`readTransmission`, `readTransmissionMaskDB`) | added $f = N/M$: $\sqrt N$ times too large for $N \ne 1$ |
| $f = 0$ in the results log and the script messages | console only |
| The BC's error terms weigh the EC's and the buffer's BC as subtracted: $-\mathrm{BC} + T\,r_{EC}\mathrm{BC}$ and $s_B(r_B - T_b r_{EC})\mathrm{BC}$, $r_X = (t f)_X / (t f)_S$ (`bcScaleRelativeToSample`) | as the sample's BC: $(1-T)$, $(1-T_b)$ |
| `calculateCentersInScript` subtracts the EC's own BC from the EC | the sample's |
| `setNormalization()` / `normalization()` | no Python access |
| The exp model's root solved (`Monitors::deadTimeFactor` loops while `iterate()` returns `GSL_SUCCESS`; `deadTimeRoot_fdf` returns the function) | one Newton step from $n = m$: $c_{\rm DT}$ too small at high rates, for $\tau = 6.5\cdot10^{-7}$ s $-0.03\,\%$ at $10^5$ s⁻¹, $-1.3\,\%$ at $3\cdot10^5$, $-12\,\%$ at $5\cdot10^5$ |
| The model names as in the literature | swapped |

## Open points

| # | Point | Effect |
|---|---|---|
| 1 | `normalizationFactorRT` ignores the repetitions when the time factor is 1 | to check in the audit of the RT mode |
