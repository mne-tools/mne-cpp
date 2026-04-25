---
mode: agent
description: >
  Implement missing mne_analyze features to close gaps against MNE-C SVN
  mne_analyze (v2.55). Work through High → Medium → Low priority items from
  doc/dev-notes/gap-analysis.md Section 14.
---

# mne_analyze Feature Parity Optimization

## Context

mne-cpp's `mne_analyze` (`src/applications/mne_analyze/`) is a modern
plugin-based Qt application. The full gap analysis is in
[doc/dev-notes/gap-analysis.md § 14](../../doc/dev-notes/gap-analysis.md).

### Plugin architecture

Every feature lives in its own plugin under
`src/applications/mne_analyze/plugins/<name>/`. Each plugin follows the same
pattern — copy an existing plugin as a starting point:

| Example plugin | Teaches |
|---|---|
| `coregistration/` | 3D interaction, `FiffCoordTrans`, `View3D` integration |
| `dipolefit/` | Inverse computation, BEM/cov selection, 3D display |
| `rawdataviewer/` | FIFF browsing, `FiffRawView`, event overlay |
| `averaging/` | Epoch-based evoked computation, butterfly/layout views |
| `view3d/` | BEM surfaces, digitizers, 3D scene management |

Plugin skeleton:
```
src/applications/mne_analyze/plugins/<name>/
├── CMakeLists.txt          # target: <name>extension
├── <name>extension.h
├── <name>extension.cpp
├── <name>view.h            # optional QWidget panel
└── <name>view.cpp
```

Register the plugin in
`src/applications/mne_analyze/extensions/extensionmanager.cpp`.

### Coding conventions

- Namespace: `MNEANLYZEEXTLIB`
- License header: BSD-3-Clause, copyright Christoph Dinh, `@since 2.2.0`,
  `@date April, 2026`
- Qt 6.11+, C++17, zero warnings
- 3D rendering: `DISP3DLIB::View3D`, `BrainTreeModel`, `BrainView`
- Surface I/O: `FSLIB::FsSurface` (`rr()` → `MatrixX3f`, `tris()` → `MatrixX3i`)
- Source estimates: `MNELIB::MNESourceEstimate`
- Forward solution: `MNELIB::MNEForwardSolution`
- Inverse operator: `MNELIB::MNEInverseOperator(file)`, check `nsource > 0`
- Covariance: `FIFFLIB::FiffCov`, save via `cov.save(fileName)`

---

## HIGH Priority (implement first)

### H1 — Cortical surface loading

**Goal:** Add a "Load Surface" action that reads a FreeSurfer surface file
(e.g. `lh.pial`, `rh.inflated`) and displays it in `View3D`.

- Parse surface path from a file dialog, use `FsSurface::read(path)`
- Build a `Qt3DCore::QEntity` mesh from `rr()` + `tris()`
- Add mesh to `BrainTreeModel` (model: `surface`)
- Control group: hemisphere toggle (left / right / both)

**Files to create/edit:**
- `src/applications/mne_analyze/plugins/surfaceloader/` (new plugin)
- `src/applications/mne_analyze/mne_analyze.pro` — add plugin

---

### H2 — STC overlay on cortical surface

**Goal:** Map a `MNESourceEstimate` (`.stc`) onto the loaded cortical surface
using a colour map and animate across time.

- Load STC via file dialog: `MNESourceEstimate::read(fileName)`
- Map vertex values to colour with configurable colour map (hot / cool / RdBu)
- Push updated vertex colours to the `Qt3DRender` geometry each frame
- Add a time slider + play/pause controls (reuse `FiffRawView` time logic)
- Respect the existing subject/hemisphere selection from H1

**Files to create/edit:**
- `src/applications/mne_analyze/plugins/stcoverlay/` (new plugin)

---

### H3 — Source estimation inside mne_analyze

**Goal:** Complete the stub `SourceLocalization` plugin
(`src/applications/mne_analyze/plugins/sourcelocalization/`) so the user
can compute dSPM/MNE/sLORETA from within the GUI without leaving mne_analyze.

- Load forward solution: `MNEForwardSolution(forwardFile)`
- Load inverse operator: `MNEInverseOperator invOp(invFile)`, check `invOp.nsource > 0`
- Select evoked or covariance data from the current analysis context
- Call `MNEInverse::prepareInverseOperator` + `MNEInverse::imagingKernel`
- Feed result into the STC overlay (H2)
- Method selector: dSPM / MNE / sLORETA radio buttons

**Files to edit:**
- `src/applications/mne_analyze/plugins/sourcelocalization/sourcelocalizationextension.h/.cpp`

---

### H4 — Label / ROI management

**Goal:** Load FreeSurfer label files (`.label`, `aparc.annot`) and display
named regions of interest on the cortical surface.

- File dialog for `.label` (text format: vertex index + RAS coords) and
  `.annot` (binary)
- Parse with `FSLIB::Label::read(path)` if available, otherwise implement
  minimal ASCII parser
- Highlight labelled vertices with a configurable solid colour overlay
- List loaded labels in a dock widget; toggle visibility per label
- "Create label from selection": save currently marked vertices to `.label`

**Files to create/edit:**
- `src/applications/mne_analyze/plugins/labelmanager/` (new plugin)

---

### H5 — Vertex picking with data readout

**Goal:** Click on the cortical surface to report the source estimate value at
that vertex.

- Connect `View3D::objectPicked(QVector3D)` to nearest-vertex search on the
  loaded surface's `rr()` matrix
- Display: vertex index, RAS coordinates, current STC amplitude, time point
- Show a persistent crosshair / sphere marker at the picked location
- Emit a signal so the timecourse-at-vertex feature (H6) can subscribe

**Files to edit:**
- `src/applications/mne_analyze/plugins/stcoverlay/` (extend H2 plugin)
- or add to `view3d/` plugin if a more general picking hook is needed

---

### H6 — Timecourse at picked vertex

**Goal:** Display the full source estimate time series for the vertex selected
in H5.

- After picking fires, extract the row from `MNESourceEstimate::data` for that
  vertex index
- Plot in a `QCustomPlot` (or `DISPLIB::AbstractView`) docked panel: x = time
  (ms), y = amplitude
- Draw a vertical cursor line tracking the main time slider
- Allow pinning multiple traces for comparison

**Files to create/edit:**
- `src/applications/mne_analyze/plugins/sourcelocalization/` or a dedicated
  `vertextimecourse/` plugin

---

## MEDIUM Priority

### M1 — MEG/EEG field mapping

Map sensor signals onto the helmet / head surface using sphere-model interpolation.

- Compute MEG field map: `FWDLIB::FwdBemModel::sphere_field_map()` or spherical
  harmonics expansion
- Render as colour overlay on `helmetsurface.fif` or a generated sphere mesh
- Separate toggle for MEG field / EEG potential / contour iso-lines

### M2 — Overlay smoothing and transparency

For the STC overlay (H2):
- Add a "Smoothing steps" spinner (0–5): iteratively average vertex values with
  their neighbours using the surface adjacency matrix
- Add an opacity slider (0–100%) controlling the overlay material's alpha

### M3 — Inverse operator management dock

Dock widget listing loaded inverse operators with metadata (method, depth,
noise-cov file). Allows switching active operator without re-opening files.

### M4 — MEG/EEG sensor coil display

Draw MEG coil geometries and EEG electrode spheres in 3D using
`FIFFLIB::FiffChInfo` + `MNECoilDef`.

### M5 — View presets

Named camera positions (lateral, medial, dorsal, ventral, frontal, occipital)
as toolbar buttons. Save/restore as `QSettings` presets.

### M6 — Timecourse manager

Dock widget managing multiple saved vertex timecourses: name, colour, export to
CSV, overlay in a shared plot.

### M7 — Multi-dipole fitting

Extend `DipoleFit` plugin: run `InvDipoleFit::calculateFit()` in a loop over
a time range and accumulate results as a multi-dipole set.

### M8 — SNR display

Compute signal-to-noise ratio from baseline and active windows of evoked data.
Show a `QCustomPlot` trace in the averaging panel.

### M9 — EEG potential maps

Spherical spline interpolation of EEG scalp potentials mapped to a head surface
mesh. Use existing `ChannelInterpolation` if available.

### M10–M14 — (lower urgency)

Left/right/both hemisphere toggle; contour iso-lines on surface; MNE amplitude
trace at vertex; overlay histogram; plot hardcopy export (SVG).

---

## LOW Priority

### L1 — MRI orthogonal viewer

Load a `.mgz` / `.nii` via `MriVolData::read()` and display axial, coronal, and
sagittal slices in three `MriSlicer`-backed `QLabel` canvases.

### L2 — Show picked vertex in MRI slices

When H5 fires a pick, transform RAS surface coords → voxel indices and update
the MRI slice crosshair.

### L3 — Continuous HPI visualisation

Plot head movement from QUAT HPI channels over time. Reuse `FiffRawView`
channel access, plot quaternion magnitudes.

### L4 — Remote control / scripting

Open a named-pipe or local TCP socket accepting simple commands
(`load <file>`, `set-time <ms>`, `screenshot <path>`).

### L5 — Movie / frame export

Animate STC overlay across all time points and capture each frame via
`BrainView::saveSnapshot()`. Stitch frames with ffmpeg or output individual
PNGs (reuse `mne_make_movie` logic as reference).

---

## Acceptance Criteria (per feature)

1. All new code compiles with **zero warnings** under Qt 6.11, macOS/Linux/WASM.
2. Each plugin is independently loadable — application starts without it if
   the shared library is absent.
3. All file I/O uses the existing FIFF/FS library classes — no raw file parsing.
4. Plugin registers correctly in `ExtensionManager` and appears in the menu.
5. WASM-incompatible code is guarded with `#ifndef Q_OS_WASM`.
6. New files carry the BSD-3-Clause header with `@since 2.2.0`.

## Reference files

- Gap analysis: `doc/dev-notes/gap-analysis.md` (§ 14)
- v2.2.0 requirements: `doc/dev-notes/v2.2.0-requirements.md`
- Existing plugins: `src/applications/mne_analyze/plugins/`
- 3D library: `src/libraries/disp3D/`
- FS library: `src/libraries/fs/`
- MNE library: `src/libraries/mne/`
