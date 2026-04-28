# MNE-CPP wiki

**MNE-CPP** is an open-source, cross-platform C++17 framework for the
acquisition, processing, and visualization of MEG / EEG / sEEG / fNIRS data,
designed for real-time clinical and research workflows.

The authoritative documentation lives on the [project website](https://mne-cpp.github.io).
This wiki is a curated index of where to find what.

---

## Get started

| Step | Where to go |
|------|-------------|
| Install a binary | [Downloads](https://mne-cpp.github.io/pages/install/binaries) — Linux, macOS, Windows installers, plus a [WebAssembly demo](https://mne-cpp.github.io/wasm/) |
| Open sample data | [Sample dataset](https://mne-cpp.github.io/docs/manual/sample-dataset) |
| First MNE Inspect session | [Inspect quickstart](https://mne-cpp.github.io/docs/manual/inspect) |
| Build from source | [Developer setup](https://mne-cpp.github.io/docs/development/setup) |

---

## Documentation

| Resource | Link |
|----------|------|
| Project website | https://mne-cpp.github.io |
| User manual | https://mne-cpp.github.io/docs/manual/intro |
| Developer guide | https://mne-cpp.github.io/docs/development/intro |
| C++ API reference (Doxygen) | https://mne-cpp.github.io/api |
| MNA / MNX format specification | https://mne-cpp.github.io/docs/manual/mna-format |
| FIFF format reference | https://mne-cpp.github.io/docs/manual/fiff-format |
| Command-line tools overview | https://mne-cpp.github.io/docs/manual/tools-overview |

---

## Downloads and releases

| Channel | Link |
|---------|------|
| Latest stable installer | https://github.com/mne-tools/mne-cpp/releases/latest |
| All releases | https://github.com/mne-tools/mne-cpp/releases |
| Nightly dev builds | https://github.com/mne-tools/mne-cpp/releases/tag/dev_build |
| WebAssembly browser demo | https://mne-cpp.github.io/wasm/ |
| SDK (CMake `find_package(MNECPP)`) | Attached to each release as `mnecpp-<version>-sdk-*.tar.gz` |

---

## Citation

If you use MNE-CPP in academic work, please cite the framework. The
concept DOI always resolves to the latest version:

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.593102.svg)](https://doi.org/10.5281/zenodo.593102)

```bibtex
@software{mne_cpp,
  author       = {Dinh, Christoph and Esch, Lorenz and Larson, Eric and others},
  title        = {{MNE-CPP: A Framework for Electrophysiology}},
  publisher    = {Zenodo},
  doi          = {10.5281/zenodo.593102},
  url          = {https://github.com/mne-tools/mne-cpp}
}
```

A machine-readable [`CITATION.cff`](https://github.com/mne-tools/mne-cpp/blob/main/CITATION.cff)
ships in the repository root and is consumed by GitHub's *Cite this repository*
button.

Full publication list: https://mne-cpp.github.io/docs/cite

---

## Community and support

| Channel | Purpose |
|---------|---------|
| [GitHub Discussions](https://github.com/mne-tools/mne-cpp/discussions) | Q&A, ideas, show-and-tell |
| [GitHub Issues](https://github.com/mne-tools/mne-cpp/issues) | Bug reports, feature requests |
| [Pull requests](https://github.com/mne-tools/mne-cpp/pulls) | Code contributions |
| [`CONTRIBUTING.md`](https://github.com/mne-tools/mne-cpp/blob/main/CONTRIBUTING.md) | How to contribute |
| [`CODE_OF_CONDUCT.md`](https://github.com/mne-tools/mne-cpp/blob/main/CODE_OF_CONDUCT.md) | Community standards |
| MNE-Python community | https://mne.tools — sister project, shared formats and concepts |

---

## For maintainers

- [Release process](Release-Process) — how to cut a new MNE-CPP release.
- [Roadmap](Roadmap) — current release plans (v2.2.0 to v2.3.0).
- CI dashboards: [Staging](https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml)
  · [Release](https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml)
  · [CodeQL](https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml)
  · [Coverity](https://github.com/mne-tools/mne-cpp/actions/workflows/coverity.yml)
- Coverage: [![codecov](https://codecov.io/gh/mne-tools/mne-cpp/branch/main/graph/badge.svg)](https://codecov.io/gh/mne-tools/mne-cpp)

---

## Sister and upstream projects

| Project | Relationship |
|---------|--------------|
| [MNE-Python](https://github.com/mne-tools/mne-python) | Reference implementation; MNE-CPP shares the FIFF format and many algorithms |
| [MNE-BIDS](https://github.com/mne-tools/mne-bids) | BIDS layer used for MNE-CPP's BIDS importer/exporter |
| [FreeSurfer](https://github.com/freesurfer/freesurfer) | Source of subject anatomies (BEM, source spaces, cortical surfaces) |
| [Qt 6](https://www.qt.io) | GUI toolkit for desktop and WASM builds |
| [Eigen](https://eigen.tuxfamily.org) | Linear algebra backbone |
| [ONNX Runtime](https://onnxruntime.ai) | ML model inference for the `ml` library |

---

*This wiki is generated from
[`doc/wiki/`](https://github.com/mne-tools/mne-cpp/tree/main/doc/wiki) in the
main repo and auto-deployed by the
[`wiki.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/wiki.yml)
workflow on every push to `main`. Edit those files via pull request; direct
edits to the wiki UI will be overwritten.*
