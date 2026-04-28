# Roadmap

The detailed task sheets live under
[`doc/dev-notes/`](https://github.com/mne-tools/mne-cpp/tree/main/doc/dev-notes)
in the main repo. This page is the high-level summary.

## v2.2.0 — *Intracranial + Intelligence + Real-Time* — April 2026 (released)

- New libraries: **`mna`** (analysis container `.mna`/`.mnx`),
  **`ml`** (ONNX Runtime + Python training bridge), **`sts`**
  (statistical testing — clusters, permutation, TFCE, Ledoit-Wolf).
- New inverse methods: **CMNE** (Contextual MNE), **MxNE**, **Gamma-MAP**.
- DSP: multitaper PSD/TFR, CSD, Extended Infomax ICA, bipolar
  re-referencing, channel derivations.
- Connectivity: directed measures **Granger**, **DTF**, **PDC** with
  shared MVAR backbone.
- Source-space cluster permutation tests + TFCE.
- sEEG / iEEG: stereotactic depth-electrode visualization, contact
  localization, MRI volume slicing.
- **MNE Inspect UI overhaul** — native menu bar, dock widgets, Loaded
  Files panel, recent projects, WASM compatibility.
- **MNE Scan now uses MNA/MNX** as its project storage format
  (was: ad-hoc XML).
- **MNE-C CLI parity** — 82 command-line tools, complete coverage of
  the original MNE-C utility suite.
- WASM **Progressive Web App** deployment.
- Lossless MNA/MNX round-trip (forward-compatible `extras` fields).

## v2.3.0 — Draft

Tracking sheet: [`doc/dev-notes/v2.3.0-requirements.md`](https://github.com/mne-tools/mne-cpp/blob/main/doc/dev-notes/v2.3.0-requirements.md)

- **`mne-mna` Python package** (deferred from 2.2) — pip-installable
  Python loader for `.mna`/`.mnx`, joint publication with MNE-Python.
- **NeuralSet interoperability** (deferred from 2.2) — Meta's NeuralSet
  format released April 2026; event-table import, BIDS importer,
  segmenter op, Chain export.
- Continued performance sweeps via the
  [code-optimization prompt](https://github.com/mne-tools/mne-cpp/blob/main/.github/prompts/code-optimization.prompt.md).
- `mne_analyze` plugin parity (Sections H/M/L in
  [`optimization-requirements.md`](https://github.com/mne-tools/mne-cpp/blob/main/doc/dev-notes/optimization-requirements.md)).

## How to influence the roadmap

- File a feature request: https://github.com/mne-tools/mne-cpp/issues/new/choose
- Discuss design: https://github.com/mne-tools/mne-cpp/discussions
- Submit a PR against `staging`: see [Release process](Release-Process)
