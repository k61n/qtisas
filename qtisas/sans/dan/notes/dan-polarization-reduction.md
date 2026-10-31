# Polarization reduction in QtiSAS/DAN

This note summarizes polarization reduction in DAN, including the two-file modes `PNx421`–`PNx424`. The first spin label is incident, the second is scattered. [Absolute calibration and run selection](dan-polarization-absolute-calibration.md) are described separately.

Decisions behind it, dated: [dan-decisions.md](dan-decisions.md).

## Existing x2: incident up/down, no analyzer

Order the measured channels as (down, up). With polarizer polarization $P$, flipper efficiency $f$, and $P_f=2f-1$, define

$$
B=\frac12\begin{pmatrix}1+P&1-P\\1-PP_f&1+PP_f\end{pmatrix},
\qquad C=B^{-1}.
$$

DAN first corrects the measured transmission pair $\mathbf t$, then divides each measured scattering image $\mathbf s$ by its corrected transmission:

$$
\mathbf T=C\mathbf t,\qquad
\mathbf I_{x2}=C\,\operatorname{diag}(\mathbf T^{-1})\mathbf s.
$$

The inverse is defined only when $P\ne0$ and $P_f\ne-1$. **The order matters:** if the physical forward model mixes incident spins before their different sample transmissions, the required order is $\operatorname{diag}(\mathbf T)^{-1}C\mathbf s$. The two expressions agree when the transmissions are equal or the optics are ideal. The current uncertainty calculation also treats relative measured-transmission errors as though they were absolute. For independent measured transmissions $t_j$ with relative uncertainties $q_j$, the corrected relative uncertainty should be $q_i^{\rm corr}=\sqrt{\sum_j(C_{ij}t_jq_j)^2}/|T_i|$.

## Existing x4: four analyzed channels

Use channel order $(--,-+,+-,++)$. For each preprocessed detector image $R_j$, script factor $F_j$, mean sample thickness $\bar d$, and analyzer transmission $\tau_j$, the caller forms

$$
w_j=\frac{F_jR_j}{\bar d\,\tau_j}.
$$

It takes the two analyzer-out transmission values copied onto down-down and up-up rows, corrects them with $C$, and applies the four-channel inverse:

$$
\begin{pmatrix}T_-\\T_+\end{pmatrix}
=C\begin{pmatrix}t_{--}\\t_{++}\end{pmatrix},\qquad
\mathbf I_{x4}
=\operatorname{diag}(T_-^{-1},T_-^{-1},T_+^{-1},T_+^{-1})K_4\mathbf w.
$$

$K_4=N/D$ is the code's full polarizer/flipper/analyzer matrix; its exact coefficients are in [`matrixCorrectionPNx4()`](../dan-polarization/dan-polarization-corrections.cpp). DAN requires all four analyzed sample files. With ideal analyzers, $K_4=\tfrac12(C\otimes I_2)$. Whether that half is appropriate for absolute intensity depends on the definition of $\tau_j$; changing it without an instrument calibration would be unjustified.

## Two-channel polarization analysis: `PNx421`–`PNx424`

Let the unknown sample cross sections be, with both matrix axes ordered (up, down),

$$
S=\begin{pmatrix}\sigma_{++}&\sigma_{+-}\\\sigma_{-+}&\sigma_{--}\end{pmatrix}.
$$

| Mode | Analyzed files | Fixed setting | What two files can identify |
|---|---|---|---|
| `PNx421` | up-up (3), down-up (6) | Analyzer up | Two incident-corrected, analyzer-weighted outputs, if analyzer response is common |
| `PNx422` | up-up (3), up-down (4) | Incident up | Two analyzer-corrected, incident-weighted outputs |
| `PNx423` | up-down (4), down-down (5) | Analyzer down | Two incident-corrected, analyzer-weighted outputs, if analyzer response is common |
| `PNx424` | down-up (6), down-down (5) | Incident down | Two analyzer-corrected, incident-weighted outputs |

For each measured row $j$, the two-channel path forms $y_j=F_jR_j/(2d_j\tau_jt_j)$, using its factor, thickness, analyzer transmission, and analyzer-out sample transmission. For a **fixed analyzer** pair (`PNx421`/`PNx423`), let $b_{jk}$ be the prepared incident-spin fraction and $T_k$ the two pure-spin transmissions recovered from that pair. Only if both normalized analyzer responses are the same vector $h_s$ can the two-row inverse recover a common projection:

$$
E_{jk}=\frac{b_{jk}T_k}{t_j},\qquad
\mathbf y_s=E S h_s,\qquad
\mathbf z_s=E^{-1}\mathbf y_s=S h_s.
$$

There is a different, **model-dependent** two-file inverse for a spin-symmetric scatterer. Assume $\sigma_{++}=\sigma_{--}=N$ and $\sigma_{+-}=\sigma_{-+}=F$ at each pixel. Let $P_j^{\rm eff}=E_{j,+}-E_{j,-}$ be the incident polarization after the transmission weighting above, and let $a_j=+A_j$ for an up analyzer or $a_j=-A_j$ for a down analyzer, where $A_j$ is that run's analyzer efficiency. Setting $c_j=P_j^{\rm eff}a_j$ gives

$$
\begin{pmatrix}y_1\\y_2\end{pmatrix}
=\frac12\begin{pmatrix}1+c_1&1-c_1\\1+c_2&1-c_2\end{pmatrix}
\begin{pmatrix}N\\F\end{pmatrix}.
$$

This matrix is invertible when $c_1\ne c_2$. In particular, with $D=(y_1-y_2)/(c_1-c_2)$ and $M=(c_1y_2-c_2y_1)/(c_1-c_2)$, the corrected values are $N=M+D$ and $F=M-D$. A small $|c_1-c_2|$ amplifies counting and calibration errors. This inversion handles run-dependent analyzer efficiencies but requires **both** equalities above; equality of the two spin-flip channels alone is insufficient. The current `PNx421`/`PNx423` code applies $E^{-1}$ and does not perform this symmetry-constrained inversion. For a general magnetic sample, two files still cannot determine four cross sections; measure both analyzer orientations and use `PNx4` instead.

For a **fixed incident** pair (`PNx422`/`PNx424`), let the rows of $H_r$ contain the two calibrated analyzer responses. If $b_{jk}$ is the prepared fraction of incident spin $k$ for measured row $j$, its effective fraction *after that row's sample-transmission normalization* is

$$
w_{jk}=\frac{b_{jk}T_k}{t_j},\qquad t_j=\sum_k b_{jk}T_k.
$$

Here $T_k$ is the pure-spin sample transmission under the stated forward model. The implemented analyzer inverse yields a common outgoing-spin projection only when both rows have the same effective weights $w_{1k}=w_{2k}=w_k$ and $H_r$ is invertible:

$$
\mathbf y_r=H_r(S^{\mathsf T}w),\qquad
\mathbf z_r=H_r^{-1}\mathbf y_r=S^{\mathsf T}w.
$$

Writing the *effective* incident weights as $w_{\uparrow}=(p,1-p)^{\mathsf T}$ and $w_{\downarrow}=(q,1-q)^{\mathsf T}$, the outputs under that common-weight condition are explicitly

$$
\mathbf z_{422}=\begin{pmatrix}p\sigma_{++}+(1-p)\sigma_{-+}\\p\sigma_{+-}+(1-p)\sigma_{--}\end{pmatrix},\qquad
\mathbf z_{424}=\begin{pmatrix}q\sigma_{++}+(1-q)\sigma_{-+}\\q\sigma_{+-}+(1-q)\sigma_{--}\end{pmatrix}.
$$

Thus `PNx422` and `PNx424` are useful when only one incident setting and both analyzer orientations were measured: they separate the *outgoing* spin states for that transmitted incident mixture. They are not, by themselves, four fully corrected spin cross sections. If the two rows have different effective weights, the implemented $H_r^{-1}\mathbf y_r$ is still a calibrated linear combination, but it is not one common $S^{\mathsf T}w$ projection. The code warns about differing incident parameters but still produces output; it does not verify equality of the transmission-weighted fractions. Even for a nonmagnetic background with $\sigma_{++}=\sigma_{--}$ and $\sigma_{+-}=\sigma_{-+}$, these outputs remain incident-weighted unless the known incident leakage is corrected in a further symmetry-constrained inversion. The current two-file code does not perform that extra inversion. [NIST Pol-Corr](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=909074) describes two-file background reduction under this symmetry assumption with its flipper parameter set to unity, using the full polarization-correction framework; it does not establish that arbitrary magnetic samples can be fully corrected from two files.

Each pair has two measurements for four unknown cross sections. It cannot generally recover all four. Pure NSF/SF results need ideal relevant optics, extra independent measurements, or a justified sample symmetry with its corresponding leakage correction. When all four analyzed sample states are available, `PNx4` is the appropriate mode for four-channel correction; the two-file modes remain useful for partial acquisitions and diagnostics. Analyzer transmission and efficiency can vary by run in measured x4 data, so the shared-$h_s$ condition for fixed-analyzer pairs must be checked. If it fails, the implemented inverse gives calibrated linear combinations with differing analyzer response, not a common $S h_s$ projection.

## Sample, buffer, and empty cell

The two-channel modes select the *same two analyzed states* for sample, buffer, and empty cell, correct each pair separately, then form

$$
\mathbf J=\mathcal K_S\mathbf m_S
-v\mathcal K_B\mathbf m_B
-\beta\mathbf e,\qquad
\beta=\begin{cases}1-v,&\text{complete valid buffer pair},\\1,&\text{no buffer requested}.\end{cases}
$$

$v$ is buffer volume fraction. The implementation requires a complete buffer pair when buffer is requested. It reads each analyzed EC run from the `#-EC` field of the corresponding sample script row and looks up its polarizer/flipper settings by that EC run; factor, thickness reference, and analyzer values remain those on the corresponding sample row. If either selected row assigns an EC run and $\beta>0$, both analyzed EC files must be present; an incomplete pair skips that reduction. With no assigned EC, it omits EC subtraction but still subtracts a requested buffer. A single analyzer-out EC image cannot generally determine two analyzer-resolved backgrounds. If the optical responses of sample, buffer, and EC differ, the resulting subtracted components are distinct linear combinations rather than one common cross section; a scalar gain correction alone cannot change an analyzer mixture. The code applies $v$ once to intensity and $v^2$ once to its per-pixel variance.

For scalar, consistently corrected measurements before transmission normalization, the background algebra is $(S-t_S EC)-v(B-t_B EC)=S-vB-(t_S-vt_B)EC$. After separately dividing the sample and buffer terms by $t_S$ and $t_B$, it becomes $S/t_S-vB/t_B-(1-v)EC$, the coefficient used above. This identity handles transmission weighting; it cannot supply the missing spin information or make two different analyzer-weighted projections identical.

If a buffer or EC is independently justified as spin-symmetric, its two fixed-analyzer measurements can instead be inverted for its own $(N,F)$ using the matrix above and its own optical parameters. After matching the absolute-scale and transmission convention, those components can be forward-projected through the **sample's** incident and analyzer response before subtraction. This can correct a symmetric background despite different analyzer efficiencies, but leaves a general two-file magnetic sample as a projection. The current two-channel subtraction does not perform that background inversion and reprojection; it directly subtracts the separately corrected two-file outputs.

## Polarizer out, analyzer in: out-up, out-down

States −1 (`out-up`) and −2 (`out-down`), set by the instrument parameters `POL-ALIAS-OUT-UP` / `POL-ALIAS-OUT-DOWN` and matched before `down`/`up`, are runs with an unpolarized incident beam and the analyzer up/down. An `out-down` run measures $\tfrac{1-A}{2}(\sigma_{++}+\sigma_{-+})+\tfrac{1+A}{2}(\sigma_{+-}+\sigma_{--})$ summed over incident spins, so no incident correction applies: x2, x4 and x42 skip them; "No Corrections" reduces them like every state (sample$/(T\tau_A)$, EC$/\tau_A$), outputs `-OutUp`/`-OutDown`. Their main use is the sample transmission: with unpolarized incident neutrons $t=[\tfrac{1-A}{2}T_++\tfrac{1+A}{2}T_-]$, i.e. $t=T$ for a spin-independent transmission, provided the empty beam was measured through the same analyzer and the ³He decay between the two runs is corrected or negligible. Such a $T$ in both rows of an x42 pair is consistent: the incident inversion gives $T_+=T_-=T$.

## Implementation and interpretation

`PNx2` still uses unanalyzed states 1/2; `PNx4` still requires analyzed states 3–6. The four `PNx42*` choices reuse the existing seven-state polarization table and select only their specified two analyzed rows. `scriptPN` pairs the runs; their DAN `#-Condition` values normally differ by spin state and need not match. They use per-run factors, transmission, thickness and analyzer parameters, combine masks, and reject singular two-row corrections. The inverse is implemented in [dan-polarization-corrections.cpp](../dan-polarization/dan-polarization-corrections.cpp) (`correctTwoChannelPair()`) and exercised by [its test](../../../tests/sans/dan/dan_polarization_corrections_tests.cpp). Pixel error images contain propagated per-pixel variance; shared factor, transmission, and optical-calibration uncertainty remains separate. When the two analyzer responses differ in a fixed-analyzer pair, the mode logs a warning and returns calibrated linear combinations, not a common $S h_s$ projection. The analogous limitation applies to a fixed-incident pair whose preparations differ.

The audited [x4 path](../dan18-process-polarization.cpp) also has separate issues that should not be copied into a new mode: corrected buffer variance receives an extra $v^2$ on addition (making its contribution scale as $v^4$); a Boolean test of a zero-based buffer row index skips row 0; buffer image scaling uses the **sample** row's absolute factor and mean sample thickness rather than the buffer's loaded values, and buffer polarization correction uses the sample's polarizer and flipper parameters; EC scaling can use an unresolved buffer index; and the EC variance calculation squares a relative variance and mismatches two channel coefficients. These buffer choices give the intended scale only when the sample and buffer calibration and thickness conventions coincide. The [absolute-calibration note](dan-polarization-absolute-calibration.md) describes the reference convention.
