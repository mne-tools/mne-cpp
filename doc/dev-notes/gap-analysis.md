# MNE-CPP Gap Analysis — Algorithms & Features Not Yet Ported

Internal developer reference. This analysis was removed from the public website docs
to avoid advertising missing features. Use it to guide future porting work.

Last updated: 9 April 2026

---

## DSP Library (DSPLIB)

### High Priority

| Algorithm | MNE-Python Module | Description |
|---|---|---|
| Multitaper PSD | `mne.time_frequency.psd_array_multitaper` | Thomson's multitaper spectral estimation — higher spectral resolution than Welch |
| Multitaper TFR | `mne.time_frequency.tfr_array_multitaper` | DPSS-windowed time-frequency representation |
| Stockwell TFR | `mne.time_frequency._stockwell` | S-transform for adaptive time-frequency analysis |
| Extended Infomax ICA | `mne.preprocessing.infomax_` | Extended Infomax — handles super- and sub-Gaussian sources |
| Picard ICA | `mne.preprocessing.ICA` (method='picard') | Faster, more robust ICA via preconditioned gradient descent |
| eLORETA (standalone) | `mne.minimum_norm._eloreta` | Already available via `InvMinimumNorm`; standalone function wrapper pending |
| Mixed-Norm (MxNE) | `mne.inverse_sparse.mxne_inverse` | L1/L2 sparse inverse for focal source estimation |
| Gamma-MAP | `mne.inverse_sparse._gamma_map` | Hierarchical Bayesian sparse inverse |

### Medium Priority

| Algorithm | MNE-Python Module | Description |
|---|---|---|
| Cross-Spectral Density | `mne.time_frequency.csd` | CSD via Fourier, multitaper, or Morlet — prerequisite for DICS beamformer improvements |
| Current Source Density | `mne.preprocessing._csd` | Surface Laplacian for EEG — improves spatial resolution |
| CSP (Common Spatial Patterns) | `mne.decoding.csp` | Spatial filter maximizing class-discriminative variance for BCI |
| Cluster Permutation Statistics | `mne.stats.cluster_level` | Non-parametric cluster-level tests for spatio-temporal data |
| OTP (Oversampled Temporal Projection) | `mne.preprocessing.otp` | MEG denoising via oversampled temporal projection |

### From MNE-C Only (Not in MNE-Python)

| Tool | MNE-C | Description |
|---|---|---|
| `mne_label_ssp` | `mne_label_ssp` | Create SSP projectors that suppress activity from a specific brain region |
| `mne_map_data` | `mne_map_data` | Map data between arbitrary surface meshes |
| `mne_average_estimates` | `mne_average_estimates` | Weighted averaging of source estimate (.stc) files |
| `mne_process_stc` | `mne_process_stc` | Manipulate and post-process .stc files |
| `mne_make_uniform_stc` | `mne_make_uniform_stc` | Create uniform source estimates for testing |

---

## Inverse Library (INVLIB)

| Algorithm | MNE-Python | Description |
|---|---|---|
| Mixed-Norm (MxNE) | `mne.inverse_sparse.mxne_inverse` | L1/L2 sparse inverse for focal sources |
| TF-MxNE | `mne.inverse_sparse.mxne_inverse` | Time-frequency mixed-norm sparse inverse |
| Gamma-MAP | `mne.inverse_sparse._gamma_map` | Hierarchical Bayesian sparse inverse |
| SAM (Synthetic Aperture Magnetometry) | — | Beamformer variant for MEG (enum defined, implementation pending) |
| TRAP MUSIC | `mne.beamformer._rap_music` | Truncated RAP-MUSIC variant |

---

## Fiff Library (FIFFLIB)

| Feature | MNE-Python | Description |
|---|---|---|
| Annotations | `mne.Annotations` | Time-stamped labels for raw data segments (e.g., BAD, EDGE) |
| BDF/EDF reader | `mne.io.read_raw_edf()` | Native FIFF wrapping of BDF/EDF+ format |
| BrainVision reader | `mne.io.read_raw_brainvision()` | BrainVision .vhdr/.vmrk/.eeg reader |
| Fieldtrip reader | `mne.io.read_raw_fieldtrip()` | FieldTrip .mat file reader |
| Montage | `mne.channels.DigMontage` | Standard montage and custom electrode placement |

Note: MNE-CPP handles non-FIFF formats via CLI converter tools
(mne_edf2fiff, mne_brain_vision2fiff, mne_ctf2fiff).

---

## Forward Library (FWDLIB)

| Feature | MNE-Python | Description |
|---|---|---|
| OpenMEEG BEM | `mne.make_forward_solution(solver='openmeeg')` | BEM via OpenMEEG library — more accurate for complex geometries |
| Finite Element Forward | External (SimNIBS) | FEM-based forward modelling for realistic tissue boundaries |
| Dipole Patch Statistics | `mne.forward.compute_depth_prior()` | Depth-weighting priors for regularisation of deep sources |
| Sensitivity Map (full) | `mne.sensitivity_map()` | Column-norm map of lead-field matrix for source-space coverage analysis |

---

## Connectivity Library (CONNLIB)

| Algorithm | MNE-Python (`mne-connectivity`) | Description |
|---|---|---|
| Directed Transfer Function (DTF) | `spectral_connectivity_epochs(method='dtf')` | Granger-causality-based directed connectivity in frequency domain |
| Partial Directed Coherence (PDC) | `spectral_connectivity_epochs(method='pdc')` | Normalised directed coherence based on MVAR models |
| Granger Causality (GC) | `spectral_connectivity_epochs(method='gc')` | Time-domain Granger causality |
| Amplitude Envelope Correlation (AEC) | `mne_connectivity.envelope_correlation()` | Power envelope correlation with optional orthogonalisation |
| Coherence (time-resolved) | `spectral_connectivity_time()` | Sliding-window or Hilbert-based time-resolved connectivity |
| Power Envelope Correlation | `mne_connectivity.envelope_correlation()` | Broadband power envelope correlation for resting-state analysis |
| Multivariate Connectivity (MIC/MIM) | `spectral_connectivity_epochs(method='mic')` | Multivariate interaction measure for multi-channel connectivity |

---

## Disp3D Library (DISP3DLIB)

| Feature | MNE-Python | Description |
|---|---|---|
| Volume Rendering | `mne.viz.plot_volume_source_estimates()` | Ray-marched or sliced volumetric source estimate display |
| Streamlines / Tractography | — | DTI tractography overlay (available in other tools like MRIcroGL) |
| Glass Brain | `nilearn.plotting.plot_glass_brain()` | Semi-transparent maximum-intensity projection |
| Time-Series Overlay | `mne.viz.plot_source_estimates('time_viewer')` | Integrated time-series and 3D surface in single view |
| Movie Export | `Brain.save_movie()` | Automated animation export to video file |
