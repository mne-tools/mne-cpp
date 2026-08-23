# Changelog

All notable changes to MNE-CPP will be documented in this file.

## [2.3.0] - 2026-06-13

### Highlights

- **MNE Inspect multimodal visualization** — Unified `MultimodalScene` integrating sEEG depth electrodes, ECoG grids/strips (`ElectrodeObject`), MRI orthogonal slices (`SliceObject`), cortical surfaces with STC overlays, dipoles, connectivity arcs, and sensor locations; cross-modality picking with shared `PickResult` and timeline; data-driven colormap controls.
- **MNE Scan production hardening** — Plugin audit, real-time CMNE wiring (MNE/dSPM/sLORETA/CMNE method selector), recording/replay/resume, quickstart/hardware/deployment documentation, packaging refinements.
- **MNE Analyze cortical workflow** — FreeSurfer surface plugin, STC overlay with threshold/mid/max colormap and time slider, vertex picking and timecourse display.
- **MNE Align coregistration wizard** — 7-step wizard-guided coregistration tool with Polhemus FASTRAK serial driver, fiducial/HSP/EEG-cap digitization, Kabsch SVD + ICP alignment, `-trans.fif` export; session persistence; optical calibration engine with constrained solver.
- **Video overlay & probe tracking** — Tracker-driven video overlay projection on head surface with billboard fallback and vertex-displaced depth relief; catheter probe visualization (station 3) with tip-only crosshair, glow aura, and ethereal instrument shader.
- **MRI slice rendering** — 3D orthogonal MRI slice rendering with per-viewport visibility, intensity windowing controls, voxel-to-world transforms, and session persistence.
- **MNE-Python algorithm parity (~92%)** — Covariance estimation (auto/shrunk/OAS/FA/PCA), inverse convenience API, preprocessing & artifact detection, standard montages (10-20/10-10/10-05), Maxwell movement compensation, multitaper TFR, Picard ICA, surface Laplacian, auto ICA classification, depth priors, resolution metrics, SourceMorph, TRAP-MUSIC, TF-MxNE, CSP/SPoC/SSD decoding, simulation API.
- **Skigen integration** — ML algorithms (StandardScaler, MinMaxScaler, LOF, LedoitWolf, OAS, EmpiricalCovariance, FactorAnalysis, CSP, SPoC, SSD) delegated to skigen; API provenance registry with CI validation.
- **SPDX license-header normalization** — 1013 files migrated to unified SPDX+Doxygen headers; CI gate enforces compliance on every PR.
- **Doxygen-to-Docusaurus API docs** — Complete automated pipeline (`doxy2mdx.py`) generating 355 MDX pages (336 classes + 19 library indexes) with Mermaid inheritance diagrams and cross-reference hyperlinks.

### New Features

- **Covariance estimation parity** (TASK 12): Auto/shrunk/OAS/FA/PCA/regularize/whitener methods; `apply_inverse_epochs/raw/cov`, beamformer epochs, source PSD/power convenience API
- **Preprocessing & artifact detection parity** (TASK 13): Maxwell bad-channel detection, EOG regression, EEG re-referencing, cHPI filter, stim artifact removal, muscle/amplitude annotation, bridged electrode detection, LOF bad-channel detection, fine calibration, annotation↔event conversion
- **Source estimate types** (TASK 14): `VectorSourceEstimate`, `VolSourceEstimate`, `MixedSourceEstimate`; label operations (`extract_label_time_course` with 5 modes, stc↔label, grow/split labels)
- **Decoding** (TASK 15): CSP/SPoC/SSD/SlidingEstimator/GeneralizingEstimator/ReceptiveField; I/O export (EDF/BrainVision), AEC/PEC connectivity, HTML Report, linear regression, RM-ANOVA, bootstrap CI, fixed-length epochs
- **Standard montages** (TASK 6): 10-20/10-10/10-05 electrode montage support
- **Maxwell movement compensation** (TASK 6): Full Maxwell filter movement compensation
- **Multitaper TFR** (TASK 7): Multitaper time-frequency representation
- **Picard ICA** (TASK 7): Picard-based ICA decomposition algorithm
- **Surface Laplacian** (TASK 7): Current source density via surface Laplacian
- **Auto ICA classification** (TASK 7): Automated ICA component labeling
- **Depth priors & resolution metrics** (TASK 8): PSF/CTF resolution metrics for source estimation
- **Full SourceMorph** (TASK 8): Complete cross-subject cortical morphing
- **TRAP-MUSIC** (TASK 8): TRAP-MUSIC source localization algorithm
- **TF-MxNE** (TASK 6): Time-frequency mixed-norm estimates
- **Simulation API** (TASK 9): `simulate_raw`/`_stc`/`_evoked`, `addNoise`/`Ecg`/`Eog`/`Chpi` in new `sim` library
- **MNE Inspect plugins**: Electrode visualization plugin (`inspect_electrodes`), MRI slices plugin (`inspect_mri_slices`)
- **MNE Analyze Studio skills**: MNA-aware agent skills extension (`mna_skills`)
- **MNE Analyze cortical surface plugin**: STC load with fThresh/fMid/fMax colormap, time slider, play/pause
- **Real-time CMNE**: CMNE method added to MNE Scan minimum norm settings (combo: MNE/dSPM/sLORETA/CMNE) with model-checkpoint picker
- **Polhemus FASTRAK driver**: Serial backend with auto-detection, pivot calibration, optical path calibration engine, Kabsch SVD registration with vertex disambiguation
- **NIfTI reader**: NIfTI-1 single-file reader (`.nii`/`.nii.gz`) for MRI slice plugins
- **Documentation screenshot tool**: `mne_doc_shots` manifest-driven screenshot generation for manual pages
- **macOS .dmg generation**: `create-dmg` integration with codesign and notarization hooks
- **API provenance registry**: `doc/api_registry.json` with 90+ entries; `tools/validate_api_registry.py` CI validation

### MNE Inspect

- **Multimodal visualization** — Integrated `MultimodalScene` with electrode, MRI slice, brain surface, STC overlay, dipole, connectivity, and sensor renderables
- **Electrode plugin** — sEEG depth electrodes and ECoG grid/strip visualization with instanced contact spheres
- **MRI slices plugin** — 3D orthogonal MRI slice rendering with per-viewport visibility, intensity windowing controls, voxel-to-world transforms, and session persistence
- **Video overlay** — Tracker-driven video texture projection on head surface with billboard fallback and vertex-displaced depth relief
- **Probe tracking** — Catheter probe visualization (station 3) with tip-only rotating crosshair, glow aura, and ethereal instrument shader

### MNE Scan

- **Real-time CMNE** — Cascaded MNE method added to minimum norm plugin with MNE/dSPM/sLORETA/CMNE selector and ONNX model-checkpoint picker
- **Quickstart documentation** — Hardware setup, deployment, and quickstart manual pages

### MNE Align

- **New coregistration application** — 7-step wizard: Load FIFF digitizer points → Verify → Acquire fiducials → Acquire head shape → Acquire EEG cap → ICP alignment → Save `-trans.fif`
- **Polhemus FASTRAK integration** — Live serial acquisition with auto-detect, pivot calibration, hemisphere configuration
- **3D visualization** — Interactive BrainView with signal-driven sync, session persistence, marker rendering
- **Session persistence** — Full save/restore of coregistration state including acquired points and nested group prefixes
- **Optical calibration** — Constrained solver with known tracker-to-objective distance, sample persistence, direct objective center capture

### Skigen Integration

- **Delegated algorithms**: StandardScaler, MinMaxScaler, LOF, LedoitWolf, OAS, EmpiricalCovariance, FactorAnalysis, CSP, SPoC, SSD
- **Policy**: Never reimplement sklearn algorithms in mne-cpp; implement in skigen first
- **Init scripts**: Updated `init.sh`/`init.bat` to clone skigen automatically
- **CI**: API registry validation workflow

### CI/CD

- Fixed polyglot `.bat` SPDX headers causing bash syntax errors on Linux/macOS — `REM Copyright (c)` replaced with `:;#` comment style valid in both shells
- Fixed `mne_doc_shots` link errors on Linux with code coverage (`__gcov_exit` unresolved) — added `--coverage` compile/link flags
- Fixed `mne_doc_shots` MSVC RuntimeLibrary mismatch on Windows (`MD_DynamicRelease` vs `MT_StaticRelease`) — set `MSVC_RUNTIME_LIBRARY` to match project libraries
- Added SPDX license-header CI gate (`license-headers.yml`)
- Added API registry validation CI gate (`api-registry-validate.yml`)
- Added Doxygen API docs CI pipeline (`api-docs.yml`)
- Added screenshot regression CI (`screenshot-regression.yml`)
- Fixed CI deploy auth: replaced expired GIT_CREDENTIALS with GH_PAT
- Fixed Linux Qt builds: added XKB/XCB/EGL/GLES dev library dependencies
- Fixed Windows CI: use `vswhere` for VS toolchain discovery
- Fixed macOS CI: clear stale pip cache before setup-python
- Fixed 15 pipeline annotations in staging/codeql workflows
- Removed broken auto-fix-build workflow
- Consolidated license-headers workflow into staging pipeline
- Qt 6.11.2 toolchain across all CI workflows

### Documentation

- **SPDX license headers**: 1013 files migrated to unified SPDX+Doxygen format with chronological author order and rename history
- **Doxygen API docs pipeline**: `tools/doxy2mdx/doxy2mdx.py` (1394 lines) generating 355 MDX pages with Mermaid inheritance diagrams, XML-backed cross-references, and per-library indexes
- **MNE Align manual** (`mne-align.mdx`): 7-step wizard documentation
- **MNE Scan manual** (`mne-scan-hardware.mdx`, `mne-scan-quickstart.mdx`, `mne-scan-deployment.mdx`): 18 plugins enumerated
- **MNE Analyze Studio manual** (`mne-analyze-studio.mdx`): Agent-based workflow documentation
- **MNE Inspect multimodal manual** (`mne-inspect-multimodal.mdx`): Multimodal visualization guide
- **Build guide**: Updated for init-script workflow, Skigen dependency, and script paths
- **API sidebar**: Restructured — Namespaces/Classes/Files/Hierarchy/Members nested under Overview category
- **Thin-plate-spline warp example** (Closes #475)

### Refactoring

- Renamed `conn` library to `connectivity` to match mne-python naming
- Removed redundant null checks before `delete` (Closes #857)
- Modernized header format: unified v3 SPDX+Doxygen with `@author`/`@since`/`@date` tags
- Renamed FastTrak → Fastrak (correct product name)

### Bug Fixes

- Fixed `QWARN` deprecation warnings in `test_mne_io_coverage` (replaced with `qWarning()`)
- Fixed FIFF coordinate transformations: retain all transforms read from a file
- Fixed high-DPI rendering on fractional-scale monitors
- Fixed HPI digitizer sphere visibility in disp3D
- Fixed MNE Scan record action label/tooltip toggle
- Fixed stale static-analysis findings
- Fixed init.sh index error (#994)
- Fixed segfault when opening FIF files saved from MNE Python (gantry angle type mismatch, #993)
- Fixed cortical_surface plugin DLL resolution on Windows
- Fixed connectivity registry header paths after library rename
- Fixed WASM SerialPort, Doxygen MathJax, and deploy auth
- Fixed WASM file dialog guard for MRI loading (`onLoadMri`)
- Fixed Polhemus serial port disconnect, vertex-based frame flip, calibration flow
- Fixed session restore: group existence check for nested prefix, repopulate AcquiredPoints
- Fixed API docs: escaped `operator[]` headings causing broken Markdown links

## [2.2.1] - 2026-04-28

### Bug Fixes

- **mne_inspect — STC scrolling broken on desktop** (regression introduced in 2.2.0). Scrubbing the STC time-slider froze playback, produced flicker / "stretched-triangle" geometry artefacts, and could crash the application after a few seconds.
  - Root cause: commit `b58c18a8c2` ("Energy/computational savings…") switched the per-surface vertex buffer from `QRhiBuffer::Immutable` to `QRhiBuffer::Dynamic` to enable in-place colour updates. `Dynamic` is documented for *small, frequently-updated* payloads (UBOs, a few hundred bytes); using it for a multi-MB brain-surface VBO triggers two failure modes on Metal/Vulkan:
    1. Each in-flight frame slot keeps its own physical buffer. Skipping the per-frame `updateDynamicBuffer()` (because `m_gpu->dirty` was false) caused the GPU to sample the wrong slot — visible as stuck frames and wildly out-of-range vertex positions.
    2. Allocating a 4-5 MB Dynamic buffer × in-flight slots × every visible surface multiplied GPU memory pressure by roughly 3×, leading to allocation failures during interactive scrubbing.
  - The WebAssembly build was unaffected because the WASM merged-surface renderer uses `Immutable` buffers re-uploaded every frame.
  - Fix: revert just the desktop VBO type to `QRhiBuffer::Immutable` (re-created when vertex data is dirty). All other improvements from `b58c18a8c2` (demand-driven render, cached vertex count, IBO upload gating, removal of redundant `updateBuffers` in `renderSurface`) are kept.
  - Files: `src/libraries/disp3D/renderable/brainsurface.cpp`.

## [2.2.0] - 2026-04-26

### Highlights

- **MNX as MNE Scan project storage** — Pipeline configurations now load and save as `.mna` (JSON) or `.mnx` (CBOR) files via `MnaIO`; legacy XML pipeline format removed. Default auto-save path is `default.mna`.
- **Unified File-menu naming across applications** — `mne_scan`, `mne_inspect`, and `mne_browse` now use the same "Open Project… / Save Project… / Export Project as MNX…" pattern.
- **MNE-C CLI parity (82 tools)** — every command-line tool from MNE-C has a mne-cpp counterpart; final 4 ports landed in this release (`mne_make_movie`, `mne_convert_lspcov`, `mne_convert_ncov`, `mne_dacq_annotator`).
- **Lossless MNA / MNX round-trip** — every MNA struct preserves unknown JSON/CBOR keys via an `extras` field, so newer schema fields survive a round-trip through older software.
- **Performance as a co-equal optimization goal** — periodic `code-optimization` prompt and matching `optimization-requirements.md` doc cover both idiomatic-code refactors and quantitative perf wins (≥ 5 % wall-clock).

### New Features

- **MNA graph-execution library** (`mne_mna`): New library for the `.mna` analysis node-graph format with YAML-based operator definitions, topological execution, and `MnaOpRegistry` for built-in and user-registered operators
- **MNX binary container** — CBOR-based binary equivalent of `.mna` (`MNX1` magic header), with embedded-data support and identical key structure for lossless format conversion
- **MNA Scan integration** — `MnaGraph m_pPipelineGraph` mirrors the live plugin scene in `PluginSceneManager`; `MnaDataKind` ↔ `ConnectorDataType` mapping shipped in `mna_scan_types`; `PluginGui::loadConfig`/`saveConfig` route through `MnaIO::read`/`MnaIO::write`
- **Stream-mode MnaGraphExecutor** — `MnaGraphExecutor::startStream()` / `stopStream()` with `StreamContext` for real-time acquisition pipelines
- **Machine-learning library** (`mne_ml`): New library with `MlTrainer` for sklearn-compatible model training via embedded Python, supporting SVM, Random Forest, Logistic Regression, and custom estimators
- **STS covariance estimators**: Added Ledoit-Wolf and Oracle Approximating Shrinkage (OAS) estimators to the `sts` library with cross-validation support against scikit-learn
- **Source-space cluster permutation tests** — `StatsCluster::oneSamplePermutationTest()`, `fTestPermutationTest()`, `tfce()` (threshold-free cluster enhancement); `StatsAdjacency::fromSourceSpaceTemporal()` for spatio-temporal adjacency
- **Spectral connectivity expansion** — `GrangerCausality` (`"GC"`), `DirectedTransferFunction` (`"DTF"`), `PartialDirectedCoherence` (`"PDC"`); shared MVAR fitting via `MvarModel`
- **CMNE sparse inverse**: Added Cascaded MNE (`InvCMNE`) solver to the inverse library for spatially sparse source estimation
- **DSP Infomax ICA**: Extended the `dsp` library with an Infomax-based ICA decomposition algorithm
- **BIDS coordinate system**: Added `BidsCoordinateSystem` class for round-trip parsing and writing of `coordinatesystem.json`, and `BidsElectrode::toFiffDigPoints()` for converting BIDS electrodes to FIFF digitizer point sets
- **FreeSurfer atlas lookup**: Added `FsAtlasLookup` for Desikan-Killiany / Destrieux atlas region querying
- **MRI slicer**: Added `MriSlicer` class for extracting orthogonal slices (Axial/Sagittal/Coronal) from NIfTI volumes
- **3D electrode visualization**: `ElectrodeObject` now includes QRhi GPU buffer management (`updateBuffers`, `vertexBuffer`, `indexBuffer`, `instanceBuffer`) following the BrainSurface pattern; new `electrode.vert` / `electrode.frag` shaders with instanced contact spheres and Blinn-Phong lighting
- **3D MRI slice visualization**: New `SliceObject` renderable for textured MRI volume slices with voxel-to-world transforms and intensity windowing; new `slice.vert` / `slice.frag` shaders
- **FiffAnnotations integration**: `AnnotationModel` refactored to use `FiffAnnotations` as backing store instead of internal `QVector<AnnotationEntry>`, with `fiffAnnotations()` / `setFiffAnnotations()` public accessors
- **Python cross-validation tests**: Added `GUARD_PYTHON` / `GUARD_PYTHON_PACKAGE` macros ensuring Python-dependent tests fail in CI (`MNE_REQUIRE_PYTHON=true`) instead of silently skipping
- **PythonTestHelper extensions**: Added `writeMatrix()`, `readMatrix()`, and `evalMatrixViaFile()` convenience methods for Eigen↔NumPy round-trip validation
- **MNA Scan plugin registry**: Added `scan-plugins.json` schema and `MnaScanTypes` class for declaring MNE Scan plugins as MNA operator schemas
- **W-file I/O**: Added `read_w` and `write_w` methods to `FiffStream` for FreeSurfer `.w` format surface data
- **Cortical surface mapping**: New `MNECorticalMap` class with `icoDownsample()` for icosahedral downsampling, `smoothOperator()` for nearest-neighbor surface smoothing, and `computeMorphMaps()` for cross-subject cortical morphing
- **Scalp surface generation**: Added `makeScalpSurfaces()` to `MNEBemSurface` for creating decimated scalp meshes at multiple resolutions
- **Processing history**: Added `copyProcessingHistory()` to `FiffStream` for transferring processing provenance between FIFF files

### MNE Inspect

- **UI overhaul** — Native `QMenuBar` (File / View / Tools / Help) with macOS system-menu integration, Recent Projects submenu (max 10), Ctrl+O / Ctrl+Q / Space shortcuts; closable + movable Controls and Loaded Files dock widgets with custom flat title bars; status bar with progress; `saveState()` / `restoreState()` persistence
- **Loaded Files panel** — live `QTreeWidget` with Name / Type / Path columns and context menu (Remove, Copy Path, Show in Finder); double-click STC entry activates it in the source-estimate combo
- **Progressive-disclosure controls** — load buttons removed from sidebar; menu actions are the single entry point; control groups disabled until matching data loads; data unload via Loaded Files context menu
- **WASM compatibility** — menu bar renders inline; `QFileDialog` guarded for WASM; recent-files menu disabled on WASM

### MNE Scan

- **File menu uses Project terminology** — `&New Project` (Ctrl+N), `&Open Project…` (Ctrl+O, accepts `*.mna *.mnx`), `&Save Project…` (Ctrl+S, defaults `.mna`), `&Export Project as MNX…` (Ctrl+Shift+E, always binary CBOR)
- **Auto-save default path** — `default.xml` → `default.mna` (auto-loaded on startup if present)
- **Foreign-data preservation** — round-trip saves preserve unknown fields via `m_loadedMnaProject`
- **Modernised pipeline canvas** — rounded-rect nodes with gradient fills, drop shadows, type labels (`SENSOR` / `ALGORITHM`), centred name, hollow-input / filled-output ports, Bezier connections with tangent arrowheads, dot-grid background, Tailwind-inspired palette, selection glow

### MNE Browse

- **Project reset on standalone open** — opening a raw file without a project resets `m_mnxProject` to prevent stale-project contamination across all 5 file-open entry points (desktop menu, WASM picker, recent files, drag-drop, CLI)

### WASM Progressive Web App

- **PWA manifest + service worker** — `scripts/wasm/manifest.json`, `scripts/wasm/sw.js`, 192×192 / 512×512 icons; offline caching with versioned cache keys; combined COOP/COEP header injection
- **No server required** — deploy bundle to any static host; service worker injects required headers for `SharedArrayBuffer`

### Command-Line Tools

- **Final MNE-C CLI parity** (4 new): `mne_make_movie` (STC frames → PNG via `FsSurface` orthographic projection), `mne_convert_lspcov` (LISP S-expression covariance → FIFF `FiffCov`), `mne_convert_ncov` (ASCII ncov → FIFF), `mne_dacq_annotator` (event annotation replacing legacy Motif GUI)
- **Inverse tools** (7 new): `mne_compute_cmne`, `mne_inverse_pipeline`, `mne_label_ssp`, `mne_average_estimates`, `mne_process_stc`, `mne_make_uniform_stc`, `mne_map_data`
- **Preprocessing tools** (7 new): `mne_add_triggers`, `mne_change_baselines`, `mne_change_nave`, `mne_copy_processing_history`, `mne_fix_stim14`, `mne_make_derivations`, `mne_toggle_skips`
- **Info tools** (6 new): `mne_check_eeg_locations`, `mne_evoked_data_summary`, `mne_list_coil_def`, `mne_list_proj`, `mne_sensor_locations`, `mne_show_mna`
- **Forward tools** (3 new): `mne_fit_sphere_to_surf`, `mne_make_scalp_surfaces`, `mne_transform_points`
- **Conversion tools** (3 new): `mne_convert_ctf_markers`, `mne_epochs2mat`, `mne_mna_bids_converter`
- **Total CLI inventory: 82 tools** — full MNE-C feature parity

### CI/CD

- Upgraded `codecov/codecov-action` from v5 to v6 to resolve Node.js 20 deprecation warnings
- Added `MNE_REQUIRE_PYTHON: 'true'` environment variable to all platform test steps
- Added `scikit-learn` to Python test dependencies
- Updated lodash from 4.17.23 to 4.18.1 (Dependabot security fix)

### Bug Fixes

- Fixed potential silent test skips — Python-dependent tests now respect `MNE_REQUIRE_PYTHON` enforcement
- Fixed deprecated `qWarn` calls in `test_mne_source_analysis` (replaced with `qWarning()`)
- Fixed unused `[[nodiscard]]` return values in `test_tool_inverse_computation`

### Documentation

- Added 26 new command-line tool documentation pages
- Comprehensive MNA / MNX format manual page (`mna-format.mdx`, ~692 lines) — file types, BIDS-like data hierarchy, computational graph, execution modes, parameter system, op registry, verification & provenance, MNX byte layout, schema versioning, complete annotated JSON example
- Rewrote MNA API reference page (`api-mna.mdx`) with full class inventory (63 → 572 lines)
- Added ML library API reference page (`api-ml.mdx`)
- Added Statistics library API reference page (`api-sts.mdx`)
- Updated tools overview with all 26 new tools and new "Inspection and Information" section
- Updated overview and API pages with Mna, Ml, Sts library entries
- Expanded glossary with 14 new terms (CMNE, DICS, LCMV, MNA, MxNE, ONNX, etc.) and 3 file format entries (`.mna`, `.mnx`, `.onnx`)
- Fixed orphaned pages (`mna-format`, `api-mna`) by adding to sidebar navigation
- Added zero-tolerance warning policy to development conventions
- Added `mne_analyze` feature-parity gap analysis (Section 14 of `gap-analysis.md`); concrete plugin-level tasks moved to `optimization-requirements.md` § 15
- Added reusable `code-optimization` Copilot prompt (`.github/prompts/code-optimization.prompt.md`) and matching `optimization-requirements.md` backlog with explicit performance scope

## [2.1.0] - 2026-04-10

### New Features

- **MNE Browse overhaul**
  - QRhi-based 2D channel rendering for Metal/Vulkan/D3D12 performance
  - ICA browser with component visualization, rejection, and original-data reset
  - Epoch review workflow with evoked computation and histogram view
  - Annotation workflow with FIF interoperability and auto-merge
  - Virtual channel derivations with weighted reference workflows
  - Covariance matrix heatmap visualization and whitening controls (W key)
  - Projector dialog (J/Shift+J) and SSP toggle
  - 2D layout view with mouse-wheel zoom, auto-scale, and channel scaling spinboxes
  - Dark mode, drag-and-drop file open, recent files list
  - GPU clipping/z-score shaders, fullscreen (F11), zen mode (F)
  - Linear detrending (D key cycles None/DC/Linear)
  - GFP overlay on butterfly view
  - Interactive ruler tool with amplitude snap and semi-transparent overlay
  - Double-click bad-channel marking, click-to-toggle bad channels
  - Stimulus lane above time ruler with event markers
  - Per-description event/annotation filtering and visibility toggles
  - Channel sort by type, epoch grid lines (G), overview bar toggle (O), scroll speed control
  - Source-estimate export workflow
  - Session filtering workflow
  - Settings persistence across sessions via QSettings
  - Event file saving to FIF and ASCII formats
  - WASM loading screen and optimized WebAssembly build

- **MNE Analyze Studio**
  - Agent-based workflow engine with grounded LLM safety
  - Multi-provider planner profiles (OpenAI Responses API)
  - Reusable inspect 3D surface views
  - Extension-based view architecture with hosted view tooling
  - VS Code chat UX integration and `.mna` project format

- **BIDS library** (`mne_bids`): New library for reading and writing BIDS-formatted datasets with testframe

- **Beamformers**: Added beamformer classes to the `inv` library with testframes

- **DSP library enhancements**
  - `WelchPsd`: Welch power spectral density estimation
  - `MorletTfr`: Morlet wavelet time-frequency representation
  - `xDAWN`: xDAWN spatial filtering for ERP denoising
  - KMeans: Cosine and correlation distance normalization implemented
  - Real-time covariance: Channel picks applied (MEG/EEG only)

- **Logging**: Renamed `ApplicationLogger` to `MNELogger` with `MNE_LOG_LEVEL` and `MNE_LOG_MODE` environment variables

### CI/CD

- Qt 6.11 support with auto-detection in `init.sh`
- Build Qt Installer Framework 4.11.0 toolchains from source
- Offline self-contained installers (.run, .dmg, .exe)
- Nightly WASM builds deployed alongside stable releases
- Reusable Qt toolchain assets consumed by CI workflows
- Eigen downloaded at configure time (no longer bundled)

### Bug Fixes

- Fixed Metal/QRhi rendering on macOS: native window, infinite repaint loop
- Fixed butterfly whitening scale and circular channel arrangement
- Fixed 2D layout channel filtering, scale persistence, and item sizing
- Fixed crosshair repaint cascade, split MEG grad/mag scale labels
- Fixed DSP SSS and bad-channel regressions
- Fixed DSP pi constants on Windows, params defaults on Clang/GCC
- Fixed Windows linker error for nested `Params` struct exports
- Fixed macOS test flakiness and WASM build errors
- Fixed `mne_anonymize` unique_ptr usage
- Fixed ruler amplitude precision (1 → 3 decimal places)
- Fixed stim chip overlap, event line z-order, and resize resampling
- Fixed scroll pan boundary and smooth inertial scroll

### Documentation

- Updated website to reflect renamed/split libraries
- Updated build guide paths (`tools/` → `scripts/`)
- Fixed release download page for installers and WebAssembly
- Updated Zenodo DOI to 19139238

## [2.0.0] - 2026-03-20

### Versioning

- **MNE-CPP libraries**: Version 2.0.0
- **CLI tools** (51 ported MNE-C tools, `mne_dipole_fit`, `mne_inspect`): Derive version from the MNE-CPP project version (2.0.0) via CMake compile definition
- **MNE Scan**: Version 1.0.0 (first stable release as a standalone application)
- **MNE Analyze**: Version 1.0.0 (first stable release as a standalone application)
- **IConnector plugin interface**: Bumped to `mne_rt_server/2.0`

### Breaking Changes

- **Library renames**: `connectivity` → `conn`, `communication` → `com`, `rtprocessing` → `dsp`, `inverse` → `inv`, `disp3D_rhi` → `disp3D`
- **Class renames**: Inverse library classes now use `Inv` prefix (`InvRapMusic`, `InvMinimumNorm`, `InvSourceEstimate`, `InvHpiFit`, …)
- **Moved classes**: `InvInverseOperator`, `InvMeasData`, `InvMeasDataSet` moved from `inv` to `mne` library (as `MNEInverseOperator`, `MNEMeasData`, `MNEMeasDataSet`); backward-compatible forwarding headers provided
- **Moved classes**: `FwdForwardSolution` moved to `MNEForwardSolution` in the `mne` library; forwarding header in `fwd/` for backward compatibility
- **Removed**: `InvCorSourceEstimate` class (superseded by unified `InvSourceEstimate`)
- **Split**: `MNEMath` split into `Linalg` and `Numerics` in the `math` library
- **Moved**: `events` library moved into `mne_analyze` application (no longer a standalone library)
- **Qt requirement**: Qt 6.10.0 minimum (Qt 5 no longer supported)
- **Eigen upgrade**: Eigen 5.0.1

### New Features

- **51 CLI tools**: Ported all MNE-C command-line tools to modern C++ with `QCommandLineParser` and `ApplicationLogger`
  - Conversion: `mne_edf2fiff`, `mne_ctf2fiff`, `mne_kit2fiff`, `mne_brain_vision2fiff`, and more
  - Forward modeling: `mne_forward_solution`, `mne_setup_forward_model`, `mne_prepare_bem_model`, `mne_make_source_space`, `mne_make_sphere_bem`, `mne_average_forward_solutions`
  - Inverse: `mne_compute_raw_inverse`, `mne_compute_mne`, `mne_inverse_operator`
  - Preprocessing: `mne_anonymize`, `mne_process_raw`, `mne_compensate_data`
  - Surface: `mne_surf2bem`, `mne_check_surface`, `mne_watershed_bem`, `mne_flash_bem`
  - Info: `mne_show_fiff`, `mne_compare_fif_files`, `mne_list_source_space`, `mne_list_bem`
  - Server: `mne_rt_server`
  - Simulation: `mne_simulate_data`
- **Unified InvSourceEstimate**: Single class with composition layers for grid data, focal dipoles, couplings, connectivity, positions, and metadata; includes tokenization support for streaming
- **RHI-based 3D rendering**: Migrated from Qt3D to QRhi pipeline with holographic rendering mode, Fresnel effects, dipole overlay, and async STC loading
- **Flexible clustering**: Variable cluster sizes in source space computation
- **MNE Scan improvements**: Plugin loading progress on splash screen, macOS app bundle icons
- **LSL library modernized**: Added `lsl_` prefix, comprehensive tests, updated to LSL 1.17.5

### Refactoring & Modernization

- **Forward library (fwd)**: Removed ~700 lines of dead code; replaced `printf` → `qInfo`/`qWarning`; replaced `goto` → early return/lambdas; replaced C macros (`ARSINH`, `EPS`) → `constexpr`/`inline`; modernized `FwdBemModel` members from raw pointers to Eigen types and `std::shared_ptr`
- **FIFF I/O consolidation**: Moved duplicated FIFF reading functions to library classes (`FiffStream::read_bad_channels`, `MNECovMatrix::read`, `MNERawInfo::find_meas_info`)
- **Thread safety**: Fixed thread-unsafe `static float*` in `MNEProjOp::project_vector` using `thread_local`
- **Network input validation**: Added bounds checking in BabyMEG client byte conversion functions
- **Sparse matrix crash fix**: Guarded `MNEHemisphere::operator==` against segfault on default-constructed sparse matrices
- **Copyright**: Updated to 2026 across all library headers
- **Qt6 serialization**: Updated `QDataStream` version from `Qt_5_1` to `Qt_6_0` in rt_server
- **Centralized CLI versioning**: All 51 CLI tools now derive their `PROGRAM_VERSION` from the CMake project version (`MNE_CPP_VERSION`) instead of hardcoded strings
- **Format specifier fix**: Fixed `%d` → `%lld` for `qsizetype` in BabyMEG client warnings

### CI/CD

- Restructured to Main-Staging-Feature branch model
- Build matrix: Ubuntu 24.04, macOS 26, Windows 2025 with Qt 6.10.0 and Qt 6.10.2
- CodeQL and Coverity static analysis (weekly)
- Code coverage tracking with 35% library threshold
- CPackIFW-based installer generation (.run, .dmg, .exe)
- WebAssembly build support

### Documentation

- Docusaurus-based documentation site
- Organized CLI tools by domain category in sidebar
- Added MNE handbook reference pages (cookbook, FIFF format, sample dataset, data conversion)
- Completed Doxygen documentation for `InvRapMusic`, `InvMinimumNorm`, `InvDipole`

### Bug Fixes

- Fixed crash in `make_inverse_operator` with non-surface-oriented forward solutions
- Fixed segfault in forward computation
- Fixed FIFF double I/O, `make_dir` EOF, `write_cov` packing, and `source_cov` kind
- Fixed `MNECTFCompDataSet` copy constructor member initialization
- Fixed coordinate transformation handling in 3D visualization
- Fixed inflated brain alignment and surface projection tolerances
- Fixed floating-point comparison tolerances in forward solution tests
- Fixed sphere simplex test convergence
