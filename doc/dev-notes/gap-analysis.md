# MNE-CPP Gap Analysis — Comprehensive Feature Comparison

Internal developer reference. Compares mne-cpp against MNE-C (SVN) and MNE-Python
to identify all features and algorithms not yet ported.

Last updated: 28 April 2026 (v2.3.0 planning sweep — gaps prioritised against
v2.3.0 release story; see [v2.3.0-requirements.md](v2.3.0-requirements.md) for
the seven-pillar plan: Pillar 0 = MNE Inspect multimodal viz (sEEG/ECoG/MRI/
source overlay) — **HIGHEST PRIORITY**, Pillar A = MNE Scan production
hardening, Pillar B = mne_analyze cortical workflow, Pillar C = mne_analyze_studio,
Pillar D = MNE-Python algorithm parity push, Pillar E = mne-mna Python pkg,
Pillar F = NeuralSet interop)

---

## Legend

- **Source**: which reference codebase has the feature (C = MNE-C SVN, Py = MNE-Python, Both)
- **Priority**: High / Medium / Low — based on user demand and scientific utility
- ✅ = mne-cpp already has this    ❌ = missing from mne-cpp

---

## 1. Inverse & Source Estimation (INVLIB)

### What mne-cpp already has

| Algorithm | Status | Notes |
|---|---|---|
| MNE (L2 minimum norm) | ✅ | Full implementation |
| dSPM (noise-normalised) | ✅ | Full implementation |
| sLORETA | ✅ | Full implementation |
| eLORETA | ✅ | Iterative weight refinement, configurable convergence |
| LCMV beamformer | ✅ | Evoked + covariance modes, weight normalisation |
| DICS beamformer | ✅ | Frequency-domain, CSD-based |
| RAP-MUSIC | ✅ | Subspace-based multi-dipole localisation |
| PwlRapMusic | ✅ | Piecewise-linear RAP-MUSIC variant |
| Dipole fitting (ECD) | ✅ | Full pipeline with CLI tool `mne_dipole_fit` |
| HPI fitting | ✅ | Real-time head position estimation |
| CLI inverse operator | ✅ | `mne_inverse_operator` in `src/tools/inverse/` |
| CLI source estimation | ✅ | `mne_compute_mne` in `src/tools/inverse/` |
| CLI continuous inverse | ✅ | `mne_compute_raw_inverse` in `src/tools/inverse/` |
| Sensitivity map | ✅ | `mne_sensitivity_map` in `src/tools/inverse/` |
| Mixed-Norm (MxNE) | ✅ | L1/L2 sparse inverse for focal source estimation (added v2.2.0) |
| Gamma-MAP | ✅ | Hierarchical Bayesian sparse inverse with ARD (added v2.2.0) |
| CMNE (LSTM-based inverse) | ✅ | Context-based MNE via ONNX Runtime + PyTorch training (added v2.2.0) |

### Gaps

| Algorithm | Source | Priority | Description |
|---|---|---|---|
| TF-MxNE | Py | High | Time-frequency mixed-norm sparse inverse |
| TRAP-MUSIC | Py | Medium | Truncated RAP-MUSIC variant |
| SAM (Synthetic Aperture Magnetometry) | — | Low | Beamformer variant for MEG (enum defined in mne-cpp, no implementation) |
| Resolution metrics | Py | Medium | Point-spread function, cross-talk function analysis for inverse operators |

---

## 2. Signal Processing (DSPLIB)

### What mne-cpp already has

| Algorithm | Status | Notes |
|---|---|---|
| FIR filter (Parks-McClellan, cosine) | ✅ | Overlap-add FFT convolution, zero-phase |
| IIR filter (Butterworth) | ✅ | Bilinear transform, biquad cascade (SOS) |
| Welch PSD | ✅ | Configurable windowing and overlap |
| Morlet TFR | ✅ | Complex wavelet convolution |
| Spectrogram (STFT) | ✅ | Gaussian windowing |
| Multitaper PSD (DPSS) | ✅ | Thomson's multitaper with DPSS tapers (added v2.2.0) |
| Cross-Spectral Density (CSD) | ✅ | CSD via Fourier, multitaper, or Morlet (added v2.2.0) |
| FastICA | ✅ | Deflationary, logcosh nonlinearity |
| Extended Infomax ICA | ✅ | Handles sub- and super-Gaussian sources (added v2.2.0) |
| SSS / tSSS | ✅ | Spherical harmonic basis, temporal extension |
| xDAWN | ✅ | Event-related response enhancement spatial filter |
| ECG / EOG detection | ✅ | Bandpass + adaptive threshold |
| Bad channel detection | ✅ | Flat / high-variance / low-correlation |
| SPHARA | ✅ | VectorView gradiometer projector |
| Polyphase resampling | ✅ | Hamming-windowed sinc kernel |
| Epoch extraction | ✅ | Event-locked, baseline correction, rejection |
| Cosine filter | ✅ | Ramp windowing |

### Gaps

| Algorithm | Source | Priority | Description |
|---|---|---|---|
| Multitaper TFR | Py | High | DPSS-windowed time-frequency representation |
| Stockwell TFR (S-transform) | Py | Medium | Adaptive time-frequency analysis |
| Picard ICA | Py | Medium | Faster, more robust ICA via preconditioned gradient descent |
| Current Source Density (surface Laplacian) | Py | Medium | Spatial derivative for EEG — improves topographic resolution |
| OTP (Oversampled Temporal Projection) | Py | Low | MEG denoising via oversampled temporal projection |
| Maxwell filter CLI tool | C | Medium | Standalone Maxwell filtering command-line interface (library exists, no CLI) |
| IIR filter types (Chebyshev I/II) | Py | Low | Additional IIR designs beyond Butterworth |
| AR model PSD | Py | Low | Autoregressive spectral estimation |

---

## 3. Forward Modelling (FWDLIB)

### What mne-cpp already has

| Algorithm | Status | Notes |
|---|---|---|
| BEM (constant collocation) | ✅ | Multi-layer volume conductivity |
| BEM (linear collocation) | ✅ | Triangular element basis |
| Multi-layer EEG sphere model | ✅ | Berg-Scherg parameterisation |
| Coil models (MEG + EEG) | ✅ | Multi-point integration |
| Lead field assembly | ✅ | Multi-threaded computation |
| Volume source space | ✅ | Grid with exclusion radius |
| Surface source space decimation | ✅ | ICO/OCT downsampling |
| CLI forward solution | ✅ | `mne_forward_solution` in `src/tools/forward/` |
| CLI BEM preparation | ✅ | `mne_prepare_bem_model` in `src/tools/forward/` |
| CLI source space creation | ✅ | `mne_make_source_space` in `src/tools/forward/` |
| CLI spherical BEM | ✅ | `mne_make_sphere_bem` in `src/tools/forward/` |
| Surface to BEM | ✅ | `mne_surf2bem` in `src/tools/forward/` |
| Surface check | ✅ | `mne_check_surface` in `src/tools/forward/` |
| Forward solution averaging | ✅ | `mne_average_forward_solutions` in `src/tools/forward/` |
| Forward model setup | ✅ | `mne_setup_forward_model` in `src/tools/forward/` |
| MRI setup | ✅ | `mne_setup_mri` in `src/tools/forward/` |
| Flash BEM | ✅ | `mne_flash_bem` in `src/tools/forward/` |
| Watershed BEM | ✅ | `mne_watershed_bem` in `src/tools/forward/` |
| Sensitivity map | ✅ | `mne_sensitivity_map` in `src/tools/inverse/` |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| OpenMEEG BEM | Py | Medium | External BEM solver — more accurate for complex geometries |
| Finite Element Forward (FEM) | External | Low | FEM-based forward modelling (SimNIBS integration) |
| Depth-weighting priors | Py | Medium | `compute_depth_prior()` for regularisation of deep sources |

---

## 4. FIFF & I/O (FIFFLIB + BIDS)

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| FIFF binary read/write | ✅ | Full format support |
| SSP projector management | ✅ | Read, write, apply |
| CTF compensation | ✅ | Synthetic gradiometer support |
| Coordinate transforms | ✅ | Head / device / MRI frames |
| FreeSurfer surfaces (.surf) | ✅ | Via FS library |
| FreeSurfer annotations (.annot) | ✅ | Parcellation labels |
| FreeSurfer labels (.label) | ✅ | ROI I/O |
| MGH/MGZ MRI volumes | ✅ | Via MRI library |
| EDF/EDF+ reader | ✅ | Via BIDS reader + `mne_edf2fiff` CLI tool |
| BrainVision reader | ✅ | Via BIDS reader + `mne_brain_vision2fiff` CLI tool |
| CTF converter | ✅ | `mne_ctf2fiff`, `mne_ctf_dig2fiff` in `src/tools/conversion/` |
| KIT converter | ✅ | `mne_kit2fiff` in `src/tools/conversion/` |
| Tufts converter | ✅ | `mne_tufts2fiff` in `src/tools/conversion/` |
| Eximia converter | ✅ | `mne_eximia2fiff` in `src/tools/conversion/` |
| Surface format converter | ✅ | `mne_convert_surface` in `src/tools/conversion/` |
| Digitiser data converter | ✅ | `mne_convert_dig_data` in `src/tools/conversion/` |
| Raw MATLAB export | ✅ | `mne_raw2mat` in `src/tools/conversion/` |
| COR set creation | ✅ | `mne_make_cor_set` in `src/tools/conversion/` |
| BIDS dataset I/O | ✅ | Native BIDS support with sidecars |
| STC source estimates | ✅ | Read/write |
| Covariance matrices | ✅ | FIFF format read/write |
| Evoked data | ✅ | Read/write |
| Digitiser points (FIFF + HPTS) | ✅ | Read/write |
| LSL streaming | ✅ | Dedicated LSL library |
| FiffAnnotations | ✅ | Time-stamped labels for raw data (BAD, EDGE, etc.) with FIFF I/O (added v2.2.0) |
| MNA / MNX project format | ✅ | Portable project container: JSON (.mna) + CBOR binary (.mnx), lossless round-trip, extras preservation (added v2.2.0) |
| Channel derivations | ✅ | Mathematical channel combinations / bipolar montages via `MNEDeriv` (added v2.2.0) |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Standard montages (10-20, 10-10, 10-05) | Py | High | Built-in electrode position databases for standard EEG systems |
| FieldTrip reader | Py | Low | `.mat` file reading for FieldTrip data |
| EEGLAB reader | Py | Low | `.set` / `.fdt` file reading |
| EGI-MFF reader | Py | Low | Geodesic EGI format |
| SNIRF reader (fNIRS) | Py | Low | Functional near-infrared spectroscopy format |
| Curry reader | Py | Low | Curry EEG format |
| MEF reader | Py | Low | Mayo Electrophysiology Format |
| GDF reader | Py | Low | General Data Format for biosignal data |
| Epochs MATLAB export | C | Low | `mne_epochs2mat` — epoch export to MATLAB |
| DICOM reader | C | Low | MRI DICOM import (libmshDicom in MNE-C) |
| W-data format | C | Low | `.w` file I/O for weight matrices |


---

## 5. Connectivity (CONNECTIVITYLIB)

### What mne-cpp already has

| Metric | Status | Notes |
|---|---|---|
| Coherence (magnitude-squared) | ✅ | Frequency domain |
| Coherency (complex-valued) | ✅ | Frequency domain |
| Imaginary coherence | ✅ | Volume-conduction robust |
| Phase-lag index (PLI) | ✅ | Volume-conduction robust |
| Weighted PLI | ✅ | Weighted variant |
| Unbiased squared PLI | ✅ | Debiased estimator |
| Debiased squared weighted PLI | ✅ | Debiased WPLI² |
| Phase-locking value (PLV) | ✅ | Phase synchronisation |
| Correlation (Pearson) | ✅ | Time domain |
| Cross-correlation | ✅ | Lagged time domain |
| Network graph (nodes + edges) | ✅ | Graph representation |
| MVAR model fitting | ✅ | Levinson–Durbin recursion (`MvarModel`) |
| Granger Causality (GC) | ✅ | Spectral GC via MVAR + spectral factorisation |
| Directed Transfer Function (DTF) | ✅ | From MVAR transfer matrix |
| Partial Directed Coherence (PDC) | ✅ | From MVAR coefficients |

### Gaps

| Algorithm | Source | Priority | Description |
|---|---|---|---|
| Amplitude Envelope Correlation (AEC) | Py (mne-connectivity) | Medium | Power envelope correlation with optional orthogonalisation |
| Power Envelope Correlation | Py (mne-connectivity) | Medium | Broadband power envelope correlation for resting-state |
| Time-resolved coherence | Py (mne-connectivity) | Low | Sliding-window or Hilbert-based time-resolved connectivity |
| Multivariate connectivity (MIC/MIM) | Py (mne-connectivity) | Low | Multivariate interaction measure for multi-channel analysis |
| Spectral connectivity from CSD | Py (mne-connectivity) | Medium | Connectivity computed from cross-spectral density matrices |

---

## 6. Preprocessing & Channel Management

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| SSP projector application | ✅ | Read, create, apply |
| CTF compensation | ✅ | Synthetic gradiometers |
| SSS / tSSS (Maxwell filtering) | ✅ | Full spherical harmonic decomposition |
| ICA (FastICA) | ✅ | Within DSP library |
| ECG / EOG artifact detection | ✅ | Bandpass + threshold |
| Bad channel detection | ✅ | Flat / variance / correlation |
| xDAWN spatial filter | ✅ | For ERP enhancement |
| Resampling | ✅ | Polyphase anti-aliased |
| Baseline correction | ✅ | In epoch extraction |
| Anonymisation | ✅ | `mne_anonymize` in `src/tools/preprocessing/` |
| Bad channel marking | ✅ | `mne_mark_bad_channels` in `src/tools/preprocessing/` |
| Channel renaming | ✅ | `mne_rename_channels` in `src/tools/preprocessing/` |
| Covariance to SSP | ✅ | `mne_cov2proj` in `src/tools/preprocessing/` |
| CTF compensation data | ✅ | `mne_compensate_data`, `mne_create_comp_data` |
| 4D compensation insert | ✅ | `mne_insert_4D_comp` in `src/tools/preprocessing/` |
| Coil type fix | ✅ | `mne_fix_mag_coil_types` in `src/tools/preprocessing/` |
| Raw processing | ✅ | `mne_process_raw` in `src/tools/preprocessing/` |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|

| ICA: Picard | Py | Medium | Faster, more robust ICA |
| ICA: automatic component classification | Py | Medium | Auto-detect ECG/EOG/muscle components by correlation |
| SSP: label-constrained | C | Low | `mne_label_ssp` — suppress activity from specific brain region |
| Maxwell filter: movement compensation | Py | High | Head movement correction during tSSS |
| Maxwell filter: auto bad channel detection | Py | Medium | Automatic bad channel detection via Maxwell basis |
| Current Source Density (surface Laplacian) | Py | Medium | Spatial filtering for EEG |
| OTP (Oversampled Temporal Projection) | Py | Low | MEG noise reduction |
| fNIRS preprocessing | Py | Low | Optical density, Beer-Lambert, TDDR |
| Stimulus artifact repair | Py | Low | Fix electrical stimulation artifacts |

---

## 7. Covariance & Whitening

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| Empirical noise covariance | ✅ | From raw/epochs data |
| Tikhonov regularisation | ✅ | Per-channel-type diagonal scaling |
| Eigenvalue decomposition | ✅ | Rank estimation |
| Cholesky decomposition | ✅ | For whitening |
| Whitening / dewhitening | ✅ | Applied in inverse |
| Real-time covariance (streaming) | ✅ | `RtCov` for online estimation |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Ledoit-Wolf shrinkage | Py | High | Optimal shrinkage estimator — better for rank-deficient data |
| Diagonal regularisation method | Py | Low | Simple diagonal covariance model |
| Empirical Bayes regularisation | Py | Low | Cross-validated regularisation |
| Shrunk covariance | Py | Low | Oracle Approximating Shrinkage (OAS) |
| Cross-validated method selection | Py | Medium | Automatically pick best regularisation method |

---

## 8. Statistics & Decoding

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| Paired t-test permutation | ✅ | `StatsCluster::permutationTest()` (sts library) |
| One-sample permutation | ✅ | `StatsCluster::oneSamplePermutationTest()` (sign-flip) |
| F-test cluster permutation | ✅ | `StatsCluster::fTestPermutationTest()` |
| Spatio-temporal clustering | ✅ | BFS over adjacency; `StatsAdjacency::fromSourceSpaceTemporal()` |
| TFCE (Threshold-Free Cluster Enhancement) | ✅ | `StatsCluster::tfce()` (Smith & Nichols 2009) |
| Ledoit–Wolf / OAS shrinkage | ✅ | sts library |
| Cortical-mesh adjacency | ✅ | `StatsAdjacency::fromSourceSpace()` |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| FDR / Bonferroni correction | Py | Medium | Multiple comparison correction |
| Bootstrap confidence intervals | Py | Low | Non-parametric confidence intervals |
| Linear regression (channel-wise) | Py | Low | Regression on raw/epochs data |
| CSP (Common Spatial Patterns) | Py | Medium | Spatial filter for BCI classification |
| SPoC (Source Power Comodulation) | Py | Low | Covariance-based feature extraction |
| SSD (Spatio-Spectral Decomposition) | Py | Low | Frequency-selective spatial components |
| EMS (Eigenvector Methods) | Py | Low | Supervised spatial extraction |
| SlidingEstimator / GeneralizingEstimator | Py | Low | Time-resolved decoding wrappers |
| Cross-validation utilities | Py | Low | Scikit-learn integration for M/EEG |

---

## 9. Source Space & Morphing

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| Surface source space (ICO/OCT) | ✅ | Decimation and downsampling |
| Volume source space | ✅ | `make_volume_source_space()` |
| Surface morphing (vertex maps) | ✅ | `MNEMorphMap` — sparse interpolation |
| Patch statistics | ✅ | Cortical patch identification |
| ICP alignment | ✅ | Point cloud registration |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| SourceMorph (full) | Py | High | Unified morphing with volume-to-volume, sparse smoothing, and fsaverage mapping |

Note: The following are already ported in `src/tools/`:
- `mne_smooth` — geodesic surface smoothing
- `mne_morph_labels` — morph labels between subjects
- `mne_annot2labels` — annotation to label extraction
- `mne_volume_data2mri` — map volumetric source data to MRI grid
- `mne_make_morph_maps` — morph map computation
- `mne_add_patch_info` — cortical patch definitions
- `mne_volume_source_space` — volumetric source space creation

---

## 10. Visualisation (DISP3DLIB)

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| 3D brain surface rendering | ✅ | QRhi/OpenGL-based |
| Source estimate overlays | ✅ | Activity-dependent colour mapping |
| Dipole visualisation | ✅ | Arrow rendering |
| Network / connectivity graphs | ✅ | 3D graph visualisation |
| Sensor / digitiser display | ✅ | Points and surfaces |
| BEM surface rendering | ✅ | Multi-layer display |
| Real-time data streaming | ✅ | Threaded workers for sensor + source updates |
| Field interpolation | ✅ | Smooth topographic mapping |
| 2D plots (line, bar, TF, topo) | ✅ | Via DISP library |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Volume source estimate rendering | Py | Medium | Ray-marched or sliced volumetric display |
| Glass brain projection | Py (nilearn) | Low | Semi-transparent maximum-intensity projection |
| Time-series + 3D linked view | Py | Medium | Integrated time viewer with 3D surface |
| Movie / animation export | — | — | `mne_make_movie` ported in `src/tools/inverse/` |
| Topomap sequences | Py | Medium | Time-stepped topographic map arrays |
| ICA component property plots | Py | Low | Source / topography / variance plots for ICA components |
| Covariance matrix heatmap | Py | Low | Visualise noise/data covariance structure |

---

## 11. Simulation

### What mne-cpp already has

| Feature | Status | Notes |
|---|---|---|
| FIFF simulator (MNE Scan plugin) | ✅ | Replays FIFF files for real-time testing |
| `mne_simu` CLI tool | ✅ | Ported from MNE-C in `src/tools/simulation/mne_simu` |

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Source activity simulation | Py | Medium | `simulate_stc()`, `simulate_sparse_stc()` — generate brain activity |
| Raw data simulation | Py | Medium | `simulate_raw()` — synthetic continuous data from forward model |
| Evoked data simulation | Py | Medium | `simulate_evoked()` — synthetic averaged responses with noise |
| Artifact injection | Py | Low | `add_noise()`, `add_ecg()`, `add_eog()`, `add_chpi()` |

---

## 12. MNE-C Command-Line Tools

MNE-C has ~78 CLI tools. **mne-cpp has achieved complete parity and beyond** with 82 CLI
tools in `src/tools/`, plus 6 GUI applications. All remaining MNE-C legacy tools
have been ported. The extra 4 tools are MNA-specific utilities that have no MNE-C
counterpart.

### Tools in `src/tools/` (82 total)

**Conversion** (`src/tools/conversion/`, 16 tools):
`mne_brain_vision2fiff`, `mne_convert_ctf_markers`, `mne_convert_dig_data`,
`mne_convert_lspcov`, `mne_convert_ncov`, `mne_convert_surface`,
`mne_ctf2fiff`, `mne_ctf_dig2fiff`, `mne_edf2fiff`,
`mne_epochs2mat`, `mne_eximia2fiff`, `mne_kit2fiff`, `mne_make_cor_set`,
`mne_mna_bids_converter`, `mne_raw2mat`, `mne_tufts2fiff`

**Forward** (`src/tools/forward/`, 14 tools):
`mne_average_forward_solutions`, `mne_check_surface`,
`mne_fit_sphere_to_surf`, `mne_flash_bem`, `mne_forward_solution`,
`mne_make_scalp_surfaces`, `mne_make_source_space`, `mne_make_sphere_bem`,
`mne_prepare_bem_model`, `mne_setup_forward_model`, `mne_setup_mri`,
`mne_surf2bem`, `mne_transform_points`, `mne_watershed_bem`

**Inverse** (`src/tools/inverse/`, 15 tools):
`mne_average_estimates`, `mne_compute_cmne`, `mne_compute_mne`,
`mne_compute_raw_inverse`, `mne_dipole_fit`, `mne_inverse_operator`,
`mne_inverse_pipeline`, `mne_label_ssp`, `mne_make_movie`,
`mne_make_uniform_stc`, `mne_map_data`, `mne_process_stc`,
`mne_sensitivity_map`, `mne_smooth`, `mne_volume_data2mri`

**Preprocessing** (`src/tools/preprocessing/`, 19 tools):
`mne_add_to_meas_info`, `mne_add_triggers`, `mne_anonymize`,
`mne_change_baselines`, `mne_change_nave`, `mne_compensate_data`,
`mne_copy_processing_history`, `mne_cov2proj`, `mne_create_comp_data`,
`mne_dacq_annotator`, `mne_fix_mag_coil_types`, `mne_fix_stim14`,
`mne_insert_4D_comp`, `mne_make_derivations`, `mne_mark_bad_channels`,
`mne_process_raw`, `mne_rename_channels`, `mne_toggle_skips`

**Info** (`src/tools/info/`, 11 tools):
`mne_check_eeg_locations`, `mne_collect_transforms`, `mne_compare_fif_files`,
`mne_evoked_data_summary`, `mne_list_bem`, `mne_list_coil_def`,
`mne_list_proj`, `mne_list_source_space`, `mne_sensor_locations`,
`mne_show_fiff`, `mne_show_mna`

**Surface** (`src/tools/surface/`, 6 tools):
`mne_add_patch_info`, `mne_annot2labels`, `mne_make_eeg_layout`,
`mne_make_morph_maps`, `mne_morph_labels`, `mne_volume_source_space`

**Simulation** (`src/tools/simulation/`, 1 tool): `mne_simu`

**Server** (`src/tools/server/`, 1 tool): `mne_rt_server`

### GUI Applications (`src/applications/`, 6 total)

`mne_analyze`, `mne_analyze_studio`, `mne_browse`, `mne_dipole_fit`,
`mne_inspect`, `mne_scan`

### MNE-C tools — all ported

All MNE-C CLI tools have been ported to mne-cpp. No remaining gaps.

---

## 13. Real-Time Processing (mne-cpp Advantage)

This section documents features where mne-cpp **exceeds** both MNE-C and MNE-Python.
Neither reference codebase has these real-time capabilities.

| Feature | Status | Notes |
|---|---|---|
| MNE Scan (real-time platform) | ✅ | Plugin-based real-time acquisition and processing |
| Real-time averaging | ✅ | Stimulus-locked online averaging |
| Real-time covariance estimation | ✅ | Streaming covariance updates |
| Real-time filtering | ✅ | Online FIR/IIR application |
| Real-time inverse (rt-MNE) | ✅ | Online source estimation |
| Real-time forward computation | ✅ | Online forward model updates |
| Real-time HPI | ✅ | Continuous head tracking |
| Real-time connectivity | ✅ | Streaming connectivity metrics |
| Real-time noise monitoring | ✅ | Online noise statistics |
| Hardware plugins (BabyMEG, BrainAmp, gusbamp, eegosports, tmsi, natus) | ✅ | Direct hardware integration |
| LSL streaming | ✅ | Lab Streaming Layer I/O |
| FieldTrip buffer | ✅ | FieldTrip real-time integration |
| BIDS native support | ✅ | Structured dataset I/O |
| WebAssembly deployment | ✅ | Browser-based applications |

---

## 14. GUI Application: `mne_analyze` vs. MNE-C SVN `mne_analyze`

MNE-C SVN `mne_analyze` (v2.55) is a comprehensive Motif/X11/OpenGL application
with ~70 source files, ~40 menu items, and ~20 dialogs. The mne-cpp `mne_analyze`
is a modern plugin-based Qt application with QRhi rendering and WebAssembly support.

### What mne-cpp `mne_analyze` already has

| Feature | Status | Notes |
|---|---|---|
| **Data loading** | | |
| Raw FIFF file loading | ✅ | Via `DataLoader` plugin with file dialog |
| Event file loading (`.eve`) | ✅ | `EventModel` integration |
| BEM surface loading | ✅ | `BemDataModel` |
| Average file loading (`*-ave.fif`) | ✅ | `AveragingDataModel` |
| Covariance file loading (`*-cov.fif`) | ✅ | `CovarianceModel` |
| MRI coordinate transform loading | ✅ | `MriCoordModel` |
| Multiple data sets simultaneously | ✅ | BIDS-style tree view |
| WebAssembly file loading | ✅ | Via `QFileDialog::getOpenFileContent` |
| Command-line file input (`-f`/`--file`) | ✅ | `AnalyzeCore::parseCmdLineInputs` |
| **Signal browsing** | | |
| Raw time-series display | ✅ | `FiffRawView` with custom delegate |
| Channel scaling (per type) | ✅ | `SCALING_MAP_CHANGED` events |
| Signal/background color customisation | ✅ | Via settings plugin |
| Zoom (vertical amplitude) | ✅ | Continuous zoom control |
| Time window size adjustment | ✅ | Configurable window width |
| Channel selection (show subset) | ✅ | Interactive 2D layout picker |
| Event marker overlay | ✅ | Toggle event display on raw data |
| Scroll to event | ✅ | Jump to event sample position |
| Real-time streaming display | ✅ | MNE Scan session integration |
| **Filtering** | | |
| FIR/IIR filter design | ✅ | Full `FilterDesignView` |
| Real-time filter application | ✅ | On-the-fly filtering of displayed data |
| Per-channel-type filtering | ✅ | MEG grad/mag, EEG separate |
| **Averaging** | | |
| Epoch-based averaging | ✅ | `MNEEpochDataList::computeAverage` |
| Filtered averaging | ✅ | With active filter kernel |
| Butterfly view | ✅ | `ButterflyView` |
| 2D layout view (topographic) | ✅ | `AverageLayoutView` |
| Baseline correction | ✅ | Configurable window |
| Artifact rejection (EOG) | ✅ | Threshold-based |
| Modality selection | ✅ | `ModalitySelectionView` |
| **Channel selection** | | |
| 2D channel layout visualisation | ✅ | `ChannelSelectionView` with scene items |
| Interactive channel picking | ✅ | Graphical selection on layout |
| **Co-registration** | | |
| Digitiser loading | ✅ | From FIFF measurement file |
| MRI fiducial loading | ✅ | LPA/NAS/RPA from file |
| Interactive fiducial picking in 3D | ✅ | Object picking in `View3D` |
| Fiducial-based alignment | ✅ | Weighted matched-point fitting |
| ICP refinement | ✅ | Iterative closest point with convergence control |
| Point weighting (LPA/NAS/RPA/HPI/HSP/EEG) | ✅ | Separate weight controls per type |
| Auto-scale option | ✅ | Uniform scaling during fit |
| Outlier omission distance | ✅ | Configurable threshold |
| Transform load/save | ✅ | `FiffCoordTrans` read/write |
| Fiducial save | ✅ | `FiffDigPointSet::write()` |
| **Dipole fitting** | | |
| ECD single dipole fit | ✅ | Full `InvDipoleFit::calculateFit()` |
| Sequential (moving) dipole fits | ✅ | Time range with step |
| BEM model selection | ✅ | From loaded models |
| Noise covariance selection | ✅ | From loaded `CovarianceModel` |
| Modality selection (MEG/EEG) | ✅ | Per-type toggle |
| Regularisation controls | ✅ | Per sensor type |
| Sphere model fallback | ✅ | Configurable origin + radius |
| Dipole display in 3D | ✅ | Via `BrainTreeModel::addDipoles()` |
| **3D visualisation** | | |
| BEM surface rendering | ✅ | `BrainTreeModel::addBemSurface()` |
| Digitiser point cloud display | ✅ | In 3D view |
| Dipole result display | ✅ | Arrow rendering |
| Camera rotation | ✅ | Interactive mouse control |
| Coordinate axis toggle | ✅ | Show/hide axes |
| Fullscreen mode | ✅ | Toggle |
| Light colour/intensity | ✅ | Adjustable |
| 3D screenshot export | ✅ | `BrainView::saveSnapshot()` |
| **Export** | | |
| 2D screenshot (PNG/SVG) | ✅ | From raw view and butterfly |
| 3D screenshot | ✅ | From 3D view |
| Save raw data to FIFF | ✅ | Via `DataLoader` |
| Save events to `.eve` | ✅ | Via `EventModel` |
| **Architecture (mne-cpp advantages)** | | |
| Plugin architecture | ✅ | Dynamic plugin loading |
| WebAssembly deployment | ✅ | Browser-based operation |
| Real-time MNE Scan integration | ✅ | Live data from acquisition |
| BIDS tree view | ✅ | Native BIDS data management |
| QRhi GPU rendering | ✅ | Modern GPU API (Metal/Vulkan/D3D12/OpenGL) |

> **Implementation roadmap:** Concrete plugin-level tasks (H1–H6, M1–M14,
> L1–L5) live in
> [`optimization-requirements.md` § 15](optimization-requirements.md#15-mne_analyze-feature-parity-tasks).
> Use the
> [`/code-optimization`](../../.github/prompts/code-optimization.prompt.md)
> Copilot prompt to pick a task and run a focused optimization pass.

### Gaps: Features in MNE-C `mne_analyze` missing from mne-cpp

| Feature | Priority | Description |
|---|---|---|
| **Source estimation** | | |
| FreeSurfer cortical surface loading | High | Load `lh.inflated`, `rh.pial`, etc. from `SUBJECTS_DIR` |
| STC/W-file overlay loading | High | Load source estimate overlays (`.stc`, `.w`) on brain surface |
| Source estimate computation in GUI | High | MNE/dSPM/sLORETA from inverse operator + evoked data |
| Source overlay colour scale controls | High | `fthresh`/`fmid`/`fmax`, heat/cool maps, sign retention |
| Source overlay time animation | High | Step through STC time points interactively |
| Source overlay smoothing | Medium | Configurable smoothing steps on cortical surface |
| Source overlay transparency | Medium | Per-overlay alpha control |
| Inverse operator management | Medium | Load, select, and apply inverse operators within GUI |
| Normal component extraction | Medium | Extract cortical-normal component only |
| Predicted data computation | Low | Forward prediction from current source estimate |
| Regularisation methods (GCV, Pearson) | Low | Alternative regularisation beyond SNR-based |
| Source data simulation (PSF) | Low | Simulate sensor data from picked surface points |
| **Surface & label management** | | |
| Label loading (single / batch / annotation) | High | FreeSurfer `.label` and `.annot` file loading |
| Label list dialog | High | Browse, select, colour labels |
| Label timecourse extraction | High | Average, max, L2, L1, time-L2 collapse methods |
| Interactive vertex marking | Medium | Click to mark vertices, grow regions |
| Create label from marked vertices | Medium | Save marked regions as `.label` files |
| Surface patch display | Low | Toggle cortical patch visibility |
| Morphed surface loading | Low | Cross-subject surface morphing display |
| **Field mapping** | | |
| MEG field interpolation on helmet | Medium | Sphere-model-based MEG field map |
| MEG scalp field maps | Medium | MEG field mapped to head surface |
| EEG potential maps | Medium | EEG scalp potential interpolation |
| Contour maps on surfaces | Medium | Iso-contour overlays with configurable step |
| **Interactive tools** | | |
| Vertex picking with data readout | High | Click on surface → report source estimate value |
| Timecourse at picked vertex | High | Display full time series at selected vertex |
| Timecourse manager | Medium | Save, name, colour, export multiple timecourses |
| Multi-dipole fitting | Medium | Multiple dipoles at single time point |
| **2D plotting** | | |
| SNR display | Medium | Signal-to-noise ratio plot |
| MNE amplitude plot at vertex | Medium | Source estimate amplitude trace |
| Plot hardcopy export (PS, AI, SVG) | Low | Vector format export from 2D plots |
| **Visualisation extras** | | |
| MEG sensor coil display | Medium | Draw coil geometries in 3D |
| EEG electrode display | Medium | Show electrode locations on head |
| Left/right/both hemisphere toggle | Medium | Quick hemisphere selection |
| View presets (lateral, medial, dorsal, etc.) | Medium | Named camera positions |
| Curvature colouring | Low | Sulci/gyri shading on cortical surface |
| Overlay histogram | Low | Value distribution display |
| Scale bar display | Low | Colour scale legend |
| **MRI integration** | | |
| MRI orthogonal viewer | Low | Axial/coronal/sagittal slice display |
| Show location in MRI slices | Low | Picked vertex → MRI coordinates |
| Surface data as MRI overlay | Low | Source estimates as `.mgh` overlay |
| **HPI** | | |
| Continuous HPI visualisation | Low | Head movement over time (QUAT channels) |
| Angular velocity display | Low | Rotation rate analysis |
| **Interoperability** | | |
| Remote control / scripting | Low | Pipe-based command interface |
| Movie / QuickTime export | Low | Automated frame capture to movie file |
| Drag-and-drop to external tools | Low | Send dipoles to tkmedit/mrilab |

### Priority summary for `mne_analyze` parity

| Priority | Count | Key items |
|---|---|---|
| **High** | 9 | Cortical surface loading, STC overlay, source estimation in GUI, label management, vertex picking with readout, timecourse at vertex |
| **Medium** | 14 | Field mapping, overlay smoothing/transparency, inverse operator management, MEG/EEG sensor display, timecourse manager, view presets, multi-dipole, SNR plot |
| **Low** | 12 | MRI viewer, HPI visualisation, remote control, movie export, curvature, histogram, scale bar, morphed surfaces, PSF simulation |

---

## Summary: Gap Counts by Priority

| Priority | Count | Key items |
|---|---|---|
| **High** | 12 | TF-MxNE, standard montages, Ledoit-Wolf covariance, Maxwell movement compensation, **mne_analyze**: cortical surface loading, STC overlay, source estimation in GUI, label management, vertex picking |
| **Medium** | 31 | Multitaper TFR, directed connectivity, Picard ICA, auto ICA classification, cluster permutation stats, SourceMorph, depth priors, resolution metrics, volume rendering, simulation, surface Laplacian, topomap sequences, **mne_analyze**: field mapping, overlay controls, sensor display, timecourse manager, view presets |
| **Low** | 32 | Niche I/O formats (EEGLAB, EGI, SNIRF, etc.), Stockwell TFR, FEM forward, OTP, AR PSD, decoding/ML wrappers, **mne_analyze**: MRI viewer, HPI vis, remote control, movie export, curvature |

### Closed since v2.1.0 (now in staging)

| Former Gap | Resolution |
|---|---|
| MxNE / Gamma-MAP sparse inverse | Implemented in `inv/sparse_source_estimators/` |
| Multitaper PSD (DPSS) | Implemented in `dsp/dpss_multitaper.h/.cpp` |
| Cross-Spectral Density (CSD) | Implemented in `dsp/cross_spectral_density.h/.cpp` |
| Extended Infomax ICA | Implemented in `dsp/extended_infomax.h/.cpp` |
| FiffAnnotations class | Implemented in `fiff/fiff_annotations.h/.cpp` |
| Channel derivations | Implemented in `dsp/channel_derivation.h/.cpp` |
| MNA/MNX project format | Full library in `src/libraries/mna/` with lossless round-trip |
| CMNE (ML-based inverse) | ONNX Runtime inference + PyTorch training bridge |
| `mne_make_movie` | Ported in `src/tools/inverse/mne_make_movie` — STC frame export |
| `mne_convert_lspcov` | Ported in `src/tools/conversion/mne_convert_lspcov` — LISP cov to FIFF |
| `mne_convert_ncov` | Ported in `src/tools/conversion/mne_convert_ncov` — ASCII ncov to FIFF |
| `mne_dacq_annotator` | Ported in `src/tools/preprocessing/mne_dacq_annotator` — CLI event annotation |
| 22 missing MNE-C CLI tools | All implemented in `src/tools/` (TASK 25): `mne_dipole_fit`, `mne_label_ssp`, `mne_average_estimates`, `mne_process_stc`, `mne_make_uniform_stc`, `mne_map_data`, `mne_epochs2mat`, `mne_sensor_locations`, `mne_evoked_data_summary`, `mne_transform_points`, `mne_check_eeg_locations`, `mne_fit_sphere_to_surf`, `mne_list_coil_def`, `mne_list_proj`, `mne_change_baselines`, `mne_change_nave`, `mne_add_triggers`, `mne_fix_stim14`, `mne_toggle_skips`, `mne_copy_processing_history`, `mne_make_derivations`, `mne_convert_ctf_markers` |

### CLI Tool Coverage

mne-cpp has ported **82 CLI tools** in `src/tools/` — achieving complete MNE-C
feature parity and beyond. Plus 6 GUI applications, a real-time server, and 4 new
MNA-specific tools (`mne_show_mna`, `mne_inverse_pipeline`,
`mne_mna_bids_converter`, `mne_compute_cmne`) that go beyond MNE-C.
All MNE-C CLI tools have been ported — no remaining gaps.
