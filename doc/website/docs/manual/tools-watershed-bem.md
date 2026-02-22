---
title: mne_watershed_bem
sidebar_label: mne_watershed_bem
sidebar_position: 1
---

# mne_watershed_bem

## Overview

`mne_watershed_bem` creates BEM (Boundary Element Model) surfaces using FreeSurfer's `mri_watershed` algorithm. It produces brain, inner skull, outer skull, and outer skin surfaces and writes the head surface as a FIFF BEM file.

This is a C++ port of the original MNE shell script by Matti Hämäläinen.

## Usage

```bash
mne_watershed_bem [options]
```

## Options

| Option | Description |
|---|---|
| `--subject <name>` | Subject name. Defaults to the `SUBJECT` environment variable. |
| `--subjects-dir <dir>` | Subjects directory. Defaults to `SUBJECTS_DIR` environment variable. |
| `--volume <name>` | MRI volume name (default: T1) |
| `--overwrite` | Overwrite existing watershed files |
| `--atlas` | Pass `--atlas` flag to `mri_watershed` |
| `--gcaatlas` | Use subcortical atlas for `mri_watershed` |
| `--preflood <number>` | Change preflood height for `mri_watershed` |
| `--verbose` | Verbose output |

## Environment Variables

| Variable | Description |
|---|---|
| `FREESURFER_HOME` | FreeSurfer installation directory (required) |
| `SUBJECTS_DIR` | Directory containing FreeSurfer subject reconstructions |
| `SUBJECT` | Current subject name |

## Description

The watershed algorithm segments the MRI volume and creates the following triangulated surfaces:

- **Brain surface** — The brain-CSF boundary
- **Inner skull surface** — The inner surface of the skull
- **Outer skull surface** — The outer surface of the skull
- **Outer skin surface** — The outer surface of the head

These surfaces are stored in the subject's `bem/watershed/` directory and are used by `mne_setup_forward_model` to create the BEM geometry needed for forward solution computation.

## Example

```bash
export SUBJECTS_DIR=/path/to/subjects
export SUBJECT=sample
mne_watershed_bem --overwrite --verbose
```
