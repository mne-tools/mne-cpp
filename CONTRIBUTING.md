# Contributing to MNE-CPP

Thanks for your interest in MNE-CPP. The full workflow is described in the
[Contribution Guide](https://mne-cpp.github.io/docs/development/contr-guide) and the
[Coding Conventions](https://mne-cpp.github.io/docs/development/contr-style).

## Before you open a pull request

- **Open or reference an issue first** for anything beyond a small, self-evident fix, so the change can be discussed
  before you invest time in it.
- Target the `staging` branch.
- Run `python3 scripts/quality/check_format.py` and the affected tests locally.

## Pull requests we close without review

- **New third-party dependencies without prior approval.** Adding a library (FetchContent, ExternalProject,
  submodules, package manifests, downloaded binaries) requires an issue and a maintainer's agreement beforehand.
- **Promotion of the author's own packages or services**, disclosed or not.
- **Unsolicited, mass-generated or AI-generated changes** that are not tied to an issue or a reproducible problem in
  MNE-CPP, including the same change submitted to many repositories.
- Changes to CI workflows from outside contributors that were not requested by a maintainer.

## CI for outside contributors

CI runs on pull requests from outside contributors only after a maintainer approves the run. The
**Dependency Guard** check fails pull requests from outside contributors that touch dependencies or CI until a
maintainer has reviewed them and added the `dependency-approved` label.
