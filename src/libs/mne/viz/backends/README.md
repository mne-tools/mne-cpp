# Backends of mne_viz

`mne_viz` (`src/libs/mne/viz`) describes what the viewers show and how they behave without a GUI toolkit:
figures as display lists, the logic of the trace and brain views, GPU frame descriptions (`gpu/GpuFrame`)
with a built-in software renderer, settings forms and view models. A backend makes that visible with one
toolkit or graphics API. It provides one or both of:

- **a renderer** that executes `mne::viz::GpuFrame`s (and draws display lists) on a GPU API, and
- **a host**: windows, input translated to `mne::viz` input events, timers, and widgets built from
  `mne::viz::SettingsForm`s and the view models.

| Backend | Library | Namespace | Provides |
|---|---|---|---|
| `qt/` | `mne_viz_qt` | `mne::viz::qt` | host (Qt windows and widgets) and renderer (QRhi: Metal, Vulkan, Direct3D, OpenGL) |

Rules, enforced by `scripts/quality/audit_dependencies.py` through `scripts/quality/dependency_policy.json`:

- A backend depends on `mne_viz` (and the data libraries), never on another backend.
- `mne_viz` never includes from `backends/`.
- A backend is the only library that may use its toolkit; only `qt/` uses Qt.

A new backend goes into `backends/<name>/` as library `mne_viz_<name>` with namespace `mne::viz::<name>`, and
into the "visualization backends" layer of `dependency_policy.json`.
