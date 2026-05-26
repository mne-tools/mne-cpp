# mne_inspect plugins

This directory contains the first generation of `mne_inspect` plugins. They
are introduced as part of the v2.3.0 quality push (tasks 10.1 and 10.2):

| Folder         | Static-lib target      | Purpose                                                |
|----------------|------------------------|--------------------------------------------------------|
| `electrodes/`  | `inspect_electrodes`   | Load EEG / sEEG / ECoG digitiser data from FIFF or CSV, expose `Disp3D::ElectrodeArray`s and surface picks via `Disp3D::PickResult`. |
| `mri_slices/`  | `inspect_mri_slices`   | Load an MRI volume (MGH / MGZ), publish axial / coronal / sagittal `Disp3D::SliceObject`s, and follow `PickKind::MriVoxel` picks with a shared crosshair. |

## Why static libraries (and not plugins)?

At the time of writing, `mne_inspect` does **not** ship a plugin loader. Its
`app/` directory only contains `mainwindow.{h,cpp}` and there is no concept
of dynamically discovered or `IPlugin`-style entry points. The only
plugin-loader pattern in the repository (`anShared::AbstractPlugin`) lives in
`mne_analyze`, which `mne_inspect` does not link against.

Rather than inventing a parallel mechanism, both plugins are scaffolded as
**self-contained QObject-based static libraries** that compile against
`mne_disp3D`, `mne_fiff` and `mne_mri`. The library entry point is the plugin
class itself (`ElectrodesPlugin`, `MriSlicesPlugin`); the host instantiates
it, attaches it to a `MultimodalScene`, and drives it via direct method
calls.

## Host-side wiring still required

The build wires both static libraries into the `mne_inspect` executable, but
nothing in `mainwindow.cpp` currently calls them. To finish the integration
the host needs to:

1. Construct a `DISP3DLIB::MultimodalScene` (single instance per main
   window).
2. Construct `ELECTRODESPLUGIN::ElectrodesPlugin` and
   `MRISLICESPLUGIN::MriSlicesPlugin`, then call `attachScene(&scene)` on
   each.
3. Hook menu / toolbar actions:
   - `File → Open digitiser…` →
     `ElectrodesPlugin::loadFiff(path)` (or `loadCsv` for CSV).
   - `File → Open MRI volume…` →
     `MriSlicesPlugin::loadVolume(path)`.
4. Connect plugin signals to GUI elements:
   - `ElectrodesPlugin::contactPicked(QString)` →
     status-bar / inspector label.
   - `MriSlicesPlugin::crosshairChanged(QVector3D)` →
     ortho-view crosshair widgets.
5. Forward picks from whichever rendering backend the host uses by emitting
   `MultimodalScene::picked` on the scene the plugins are attached to.

## Open questions

- **Plugin loader design.** Should `mne_inspect` adopt
  `anShared::AbstractPlugin` (and therefore link `mne_analyze_shared`) or
  should the loader be factored out into its own library? Until that
  decision is made, the entry point of every new `mne_inspect` plugin is the
  C++ class itself — there are no plugin metadata files, JSON manifests, or
  `Q_PLUGIN_METADATA` macros in this directory.
- **NIfTI loading.** `MriSlicesPlugin::loadVolume` currently rejects
  non-MGH/MGZ paths because `MRILIB::MriVolData` only ships MGH/MGZ IO at
  v2.3.0. Wiring NIfTI is tracked separately.
