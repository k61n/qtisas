# Absolute calibration and run selection in QtiSAS/DAN

This note extends the [polarization-reduction note](dan-polarization-reduction.md). It distinguishes **current DAN x4 code** from the separate two-channel polarization-analysis modes `PNx421`–`PNx424` and their conditional physical interpretation. “Absolute intensity” means the thickness-corrected macroscopic differential cross section, usually in cm⁻¹.

Decisions behind it, dated: [dan-decisions.md](dan-decisions.md).

## How the present x4 path applies an absolute factor

For each analyzed run $j$, the script supplies `Factor` $F_j^{\rm tab}$, optional `Scale` $s_j$ (default 1), thickness, and analyzer transmission $\tau_j$. Let $R_j$ be its detector matrix after DAN's detector/background, mask, sensitivity, and optional detector-transmission/parallax processing. With $F_j=s_jF_j^{\rm tab}$ and mean script thickness $\bar d$, the implemented calculation is

$$
w_j=\frac{F_jR_j}{\bar d\,\tau_j},\qquad
\begin{pmatrix}T_-\\T_+\end{pmatrix}
=C\begin{pmatrix}t_{--}\\t_{++}\end{pmatrix},\qquad
\mathbf I=
\operatorname{diag}(T_-^{-1},T_-^{-1},T_+^{-1},T_+^{-1})K_4\mathbf w.
$$

The four-channel order is $(--,-+,+-,++)$. The two $t$ values on x4 rows **must represent analyzer-out** incident-down/up sample transmissions, measured as states 2/1 and copied to analyzed states 5/3. States 6/4 receive the corresponding incident-state values too. The current inverse uses the values on rows 5 and 3. The full $K_4$ coefficients are in [`matrixCorrectionPNx4()`](../dan-polarization/dan-polarization-corrections.cpp).

In the present x4 buffer branch, each buffer image is multiplied by its volume fraction and analyzer-transmission correction, but by the **sample** row's `Factor` and mean sample thickness rather than the buffer's loaded factor and thickness. The buffer correction also uses the sample polarizer/flipper parameters. A buffer measured with different calibration factors, thickness, or incident optics therefore needs an explicit consistency check before the subtracted output is interpreted as one absolute-scale cross section; the [implementation audit](dan-polarization-reduction.md) records the related variance issues.

`Factor` is calculated per DAN configuration. In simplified notation, $D_c,D_s$ are calibrant/configuration detector distances, $\mu$ the configured calibrant parameter, $T_{\rm EC}$ the configuration EC transmission, $T_{\rm pol}$ the code's optical-state adjustment, $\bar R_c$ the masked calibrant mean, $a$ direct-beam attenuator transmission, and $A_{\rm pix}$ pixel area.

| Method | Simplified factor or additional operation |
|---|---|
| Flat Scatter (FS) | $F=\mu(D_s/D_c)^2/(\bar R_cT_{\rm EC}T_{\rm pol})$ |
| Flat Scatter + Transmission | $F=\mu(1-T_c)(D_s/D_c)^2/(\bar R_cT_{\rm EC}T_{\rm pol})$ |
| Direct Beam (DB) | $F=aD_c^2/(I_{\rm DB}\mu A_{\rm pix}T_{\rm EC}T_{\rm pol})$ |
| Counts per Channel | FS factor; divide each prepared detector image by its positive monitor factor |
| DB from Monitor3 | DB expression with normalized Monitor3 count in place of $I_{\rm DB}$ |

The actual routines also handle calibrant backgrounds, geometry, sensitivity, and attenuators. $T_{\rm pol}$ is **not** the run's analyzer transmission $\tau_j$. DAN's $T_{\rm pol}$ branch handles states 0, 1/2, and some noncrossed analyzed states; crossed states 4/6 fall through to 1. A positive computed factor therefore does not verify transfer between arbitrary optical states. A calibrant in any state can set a common scale **in principle**, if its known nonzero response and every sample state's response are calibrated. A crossed-state direct beam approaches zero in ideal optics, making that choice poorly conditioned.

A calibrant measured with the analyzer only (out-up −1, out-down −2; since 2026-10-29) is a valid choice: its beam is unpolarized, so the analyzer passes the same fraction $\tau_A$ of every spin and $T_{\rm pol}=1/\tau_A$ (sample rows without polarizer: out, out-up, out-down) or $T_P/\tau_A$ (sample rows with the polarizer), with $\tau_A$ of the calibrant's own run — the ³He cell decays between runs (about $O P\tanh(O P)\,\Delta t/T_1$, ≈ 2 %/h for $OP≈2$, $T_1=100$ h). Rows without polarizer now count as state ≤ 0 in all $T_{\rm pol}$ cases (before only 0). Caveat: $\tau_A$ is one number per run; the cell's transmission varies with scattering angle (path length), which a flat calibrant averages over the detector.

## Incident flux, transmission, and the visible half

For a direct-beam calibration, let $B_c=a\epsilon\Phi_0g_c$ be the attenuated detector count in calibrant state $c$, and let $t_r$ be upstream throughput for target sample state $r$. The incident intensity **at the sample** is

$$
\Phi_r=\frac{B_c}{a\epsilon}\frac{t_r}{g_c}.
$$

An analyzer lies downstream of the sample: its response contributes to $g_c$ if the direct beam passes through it, but not to the target incident flux $t_r$. For measured transmissions, DAN uses $T_{s/e}=J_s/J_e$ and $T_{e/b}=J_e/J_b$. In a simplified DB path,

$$
F=\frac{K}{B_cT_{e/b}T_{\rm pol}}
\quad\Longrightarrow\quad
\frac{F}{T_{s/e}}=\frac{KJ_b}{B_cJ_sT_{\rm pol}}.
$$

The EC reading $J_e$ cancels, leaving the **empty-beam** reference $J_b$ as the effective optical reference. Check that its polarization state, the calibrant state, and DAN's $T_{\rm pol}$ branch agree. For example, if calibrant and empty beam are state 0 but EC is state 1, the branch uses polarizer throughput $t_P$ where the complete measured-transmission chain needs 1; the scale is then high by $1/t_P$ under the stated flux model. The script's run numbers alone do not establish where a transmission monitor sits or which optical state its exposure used.

In ideal optics, current $K_4=\tfrac12 I_4$, so $I_j=F_jR_j/(2\bar d\,\tau_jT_{\operatorname{incident}(j)})$. This **does not by itself prove a factor-of-two bug**: an analyzer transmission defined for unpolarized input and one defined for selected-spin input differ by that factor in the ideal limit. Verify the instrument convention against a known standard before altering $F$ or $K_4$.

## Which files and table fields matter

The configuration's `#-EC [EB]` selects the EC run; its `#-EB` is the reference for `Tr [EC-to-EB]`. Its `#-BC` is a blocked-beam run. The separate `Abs.Cal. [#-FS]`, `Abs.Cal. [#-EB]`, and `Abs.Cal. [#-BC]` fields select the absolute standard/direct beam and calibration backgrounds. In particular, the two `#-EB` fields serve different paths. The main script's `Transmission-Sample` measures sample relative to its assigned EC; a force-copy step fills missing matching scattering-row values but does not overwrite positive ones or check reference optics.

| `scriptPN` state | Run field | Role |
|---|---|---|
| 0 | `#-out` | Optional x4 EC reference row |
| 1 / 2 | `#-up` / `#-down` | x2 sample; x4 analyzer-out transmission sources and EC fallback |
| 3 / 4 | `#-up-up` / `#-up-down` | Required x4 analyzed sample channels |
| 5 / 6 | `#-down-down` / `#-down-up` | Required x4 analyzed sample channels |
| −1 / −2 | `#-out-up` / `#-out-down` (columns 11/12, after the buffer columns) | Polarizer out, analyzer up/down: "No Corrections" only (sample/(T·τ_A)); transmissions and calibrants measured through the analyzer |

`PNx2` requires states 1/2; `PNx4` requires all states 3–6. For x4 empty-cell subtraction, DAN tries an assigned EC from state 0, then 2, then 1 and projects that **one** EC image into four results. The NIST analyzed-SANS procedure instead corrects analyzed EC scattering separately before subtraction; reducing to two analyzed EC channels requires its stated nonmagnetic symmetry and perfect-flipper assumptions. The NIST procedure also calls for an analyzer-out open beam and sample transmission, plus separate optical-calibration measurements. Current DAN accepts calibrated optical parameters from constants, tables, or headers; it does not perform that measurement sequence automatically. See the [NIST protocol](https://www.nist.gov/document/sans4-reduction-protocol) and [Pol-Corr paper](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=909074).
