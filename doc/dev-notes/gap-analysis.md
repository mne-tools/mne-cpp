# MNE-CPP Gap Analysis — Comprehensive Feature Comparison

Internal developer reference. Compares mne-cpp against MNE-C (SVN) and MNE-Python
to identify all features and algorithms not yet ported.

Last updated: 10 April 2026

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

### Gaps

| Algorithm | Source | Priority | Description |
|---|---|---|---|
| Mixed-Norm (MxNE) | Py | High | L1/L2 sparse inverse for focal source estimation |
| TF-MxNE | Py | High | Time-frequency mixed-norm sparse inverse |
| Gamma-MAP | Py | High | Hierarchical Bayesian sparse inverse (automatic relevance determination) |
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
| FastICA | ✅ | Deflationary, logcosh nonlinearity |
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
| Multitaper PSD (DPSS) | Py | High | Thomson's multitaper — higher spectral resolution than Welch |
| Multitaper TFR | Py | High | DPSS-windowed time-frequency representation |
| Stockwell TFR (S-transform) | Py | Medium | Adaptive time-frequency analysis |
| Extended Infomax ICA | Py | High | Handles super- and sub-Gaussian sources (default in many EEG labs) |
| Picard ICA | Py | Medium | Faster, more robust ICA via preconditioned gradient descent |
| Cross-Spectral Density (CSD) | Py | High | CSD via Fourier, multitaper, or Morlet — required for improved DICS |
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

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Annotations class | Py | High | Time-stamped labels for raw data (BAD, EDGE, etc.) — mne-cpp has event matrices but no `Annotations` equivalent |
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
| Channel derivations | C | Medium | Mathematical channel combinations (bipolar montages, `mne_make_derivations`) |

---

## 5. Connectivity (CONNLIB)

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

### Gaps

| Algorithm | Source | Priority | Description |
|---|---|---|---|
| Directed Transfer Function (DTF) | Py (mne-connectivity) | Medium | Granger-causality-based directed connectivity in frequency domain |
| Partial Directed Coherence (PDC) | Py (mne-connectivity) | Medium | Normalised directed coherence based on MVAR models |
| Granger Causality (GC) | Py (mne-connectivity) | Medium | Time-domain Granger causality |
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
| ICA: Extended Infomax | Py | High | Handles sub-Gaussian sources; default in many EEG labs |
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

Nothing in this category. This is a complete gap relative to MNE-Python.

### Gaps

| Feature | Source | Priority | Description |
|---|---|---|---|
| Permutation t-test | Py | Medium | Non-parametric univariate testing |
| Cluster-level permutation tests | Py | High | Spatio-temporal cluster statistics (the most-used MNE-Python stats feature) |
| F-tests (1-way, repeated measures) | Py | Medium | Parametric ANOVA |
| FDR / Bonferroni correction | Py | Medium | Multiple comparison correction |
| Bootstrap confidence intervals | Py | Low | Non-parametric confidence intervals |
| Linear regression (channel-wise) | Py | Low | Regression on raw/epochs data |
| Adjacency matrices | Py | Medium | Spatial/temporal neighbourhood graphs for cluster tests |
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
| STC processing (arithmetic) | C | Medium | `mne_process_stc` — manipulate, threshold, combine STC files |
| STC uniform resampling | C | Low | `mne_make_uniform_stc` — uniform time grid |
| STC averaging | C | Low | `mne_average_estimates` — weighted cross-subject averaging |
| Surface data mapping | C | Low | `mne_map_data` — map between arbitrary surface meshes |

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
| Movie / animation export | Py | Low | `Brain.save_movie()` — automated video export |
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

MNE-C has ~78 CLI tools. mne-cpp has ported a large majority of them into
`src/tools/` (52 tools), plus 5 GUI applications and `mne_dipole_fit`.

### Tools already ported (in `src/tools/`)

**Conversion** (`src/tools/conversion/`):
`mne_brain_vision2fiff`, `mne_ctf2fiff`, `mne_ctf_dig2fiff`, `mne_edf2fiff`,
`mne_eximia2fiff`, `mne_kit2fiff`, `mne_tufts2fiff`, `mne_raw2mat`,
`mne_convert_dig_data`, `mne_convert_surface`, `mne_make_cor_set`

**Forward** (`src/tools/forward/`):
`mne_forward_solution`, `mne_prepare_bem_model`, `mne_make_source_space`,
`mne_make_sphere_bem`, `mne_surf2bem`, `mne_check_surface`,
`mne_average_forward_solutions`, `mne_setup_forward_model`, `mne_setup_mri`,
`mne_flash_bem`, `mne_watershed_bem`

**Inverse** (`src/tools/inverse/`):
`mne_inverse_operator`, `mne_compute_mne`, `mne_compute_raw_inverse`,
`mne_sensitivity_map`, `mne_smooth`, `mne_volume_data2mri`

**Preprocessing** (`src/tools/preprocessing/`):
`mne_anonymize`, `mne_mark_bad_channels`, `mne_rename_channels`,
`mne_compensate_data`, `mne_cov2proj`, `mne_create_comp_data`,
`mne_fix_mag_coil_types`, `mne_insert_4D_comp`, `mne_add_to_meas_info`,
`mne_process_raw`

**Info** (`src/tools/info/`):
`mne_show_fiff`, `mne_list_source_space`, `mne_list_bem`,
`mne_collect_transforms`, `mne_compare_fif_files`

**Surface** (`src/tools/surface/`):
`mne_annot2labels`, `mne_morph_labels`, `mne_make_morph_maps`,
`mne_add_patch_info`, `mne_make_eeg_layout`, `mne_volume_source_space`

**Simulation** (`src/tools/simulation/`): `mne_simu`

**Server** (`src/tools/server/`): `mne_rt_server`

**Applications** (`src/applications/`):
`mne_dipole_fit`, `mne_analyze`, `mne_analyze_studio`, `mne_browse`,
`mne_inspect`, `mne_scan`

### MNE-C tools with no mne-cpp equivalent

| MNE-C Tool | Priority | Description |
|---|---|---|
| `mne_process_stc` | Medium | STC file arithmetic, thresholding, combination |
| `mne_make_uniform_stc` | Low | Resample STC to uniform time grid |
| `mne_average_estimates` | Low | Weighted cross-subject STC averaging |
| `mne_map_data` | Low | Map data between arbitrary surface meshes |
| `mne_make_movie` | Low | Generate movie from STC data |
| `mne_label_ssp` | Low | SSP projectors from label-constrained data |
| `mne_make_derivations` | Low | Create derived channel definitions (bipolar montages) |
| `mne_epochs2mat` | Low | Export epochs to MATLAB format |
| `mne_add_triggers` | Low | Add trigger channels from external file |
| `mne_fix_stim14` | Low | Fix STIM14 channel issues |
| `mne_toggle_skips` | Low | Toggle skip flags in FIFF |
| `mne_check_eeg_locations` | Low | Validate EEG electrode locations |
| `mne_change_baselines` | Low | Modify baseline correction in evoked data |
| `mne_change_nave` | Low | Change number of averages in evoked data |
| `mne_convert_lspcov` | Low | Convert ASCII covariance to FIFF |
| `mne_convert_ncov` | Low | Convert noise covariance formats |
| `mne_evoked_data_summary` | Low | Summarise evoked data statistics |
| `mne_transform_points` | Low | Apply coordinate transformation to point sets |
| `mne_sensor_locations` | Low | Display sensor position information |
| `mne_dacq_annotator` | Low | Annotation editor for acquisition metadata |

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

## Summary: Gap Counts by Priority

| Priority | Count | Key items |
|---|---|---|
| **High** | 8 | MxNE/Gamma-MAP/TF-MxNE sparse inverse, Annotations class, standard montages, Ledoit-Wolf covariance, Extended Infomax ICA, Maxwell movement compensation |
| **Medium** | 20 | Multitaper PSD/TFR, CSD, directed connectivity, Picard ICA, auto ICA classification, cluster permutation stats, SourceMorph, depth priors, resolution metrics, volume rendering, simulation, surface Laplacian, topomap sequences |
| **Low** | 20 | Niche I/O formats (EEGLAB, EGI, SNIRF, etc.), Stockwell TFR, movie export, FEM forward, OTP, AR PSD, decoding/ML wrappers, remaining MNE-C utilities |

### CLI Tool Coverage

mne-cpp has ported **52 of ~78** MNE-C command-line tools (67%), plus 5 GUI applications
and a real-time server. Only ~20 low-priority MNE-C utilities remain unported.
