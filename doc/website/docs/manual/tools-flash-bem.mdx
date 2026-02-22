---
title: mne_flash_bem
sidebar_label: mne_flash_bem
sidebar_position: 2
---

# mne_flash_bem

## Overview

`mne_flash_bem` creates BEM (Boundary Element Model) surfaces using multi-echo FLASH MRI sequences. It orchestrates FreeSurfer tools for DICOM conversion, parameter map fitting, flash volume synthesis, registration, and triangulation.

This is a C++ port of the original MNE shell script by Matti Hämäläinen.

## Usage

```bash
mne_flash_bem [options]
```

## Options

| Option | Description |
|---|---|
| `--noflash30` | Only 5-degree flash data available; average flash-5 echoes instead of combining with flash-30 |
| `--noconvert` | Skip DICOM-to-MGZ conversion step |
| `--unwarp <option>` | Apply gradient distortion unwarping |
| `--subject <name>` | Subject name. Overrides `SUBJECT` environment variable. |
| `--subjects-dir <dir>` | Subjects directory. Overrides `SUBJECTS_DIR` environment variable. |
| `--flash-dir <dir>` | Flash data directory (default: current working directory) |

## Environment Variables

| Variable | Description |
|---|---|
| `FREESURFER_HOME` | FreeSurfer installation directory (required) |
| `SUBJECTS_DIR` | Directory containing FreeSurfer subject reconstructions |
| `SUBJECT` | Current subject name |

## Description

The FLASH-based BEM surface creation typically provides better results than the watershed approach, especially for the skull surfaces. The method uses multi-echo FLASH MRI sequences acquired at different flip angles (typically 5° and 30°) to estimate tissue parameters and segment the head into different compartments.

The processing pipeline includes:

1. **DICOM conversion** — Converting raw DICOM images to FreeSurfer MGZ format
2. **Parameter map fitting** — Computing parameter maps from multi-echo data
3. **Flash volume synthesis** — Creating a synthetic volume from the parameter maps
4. **Registration** — Aligning flash volumes with the FreeSurfer reconstruction
5. **Triangulation** — Creating triangulated surfaces for each compartment

The resulting surfaces (inner skull, outer skull, and outer skin) are stored in the subject's `bem/flash/` directory.

## Example

```bash
export SUBJECTS_DIR=/path/to/subjects
export SUBJECT=sample
cd /path/to/flash/dicoms
mne_flash_bem --subject sample
```

If only 5-degree flash data is available:

```bash
mne_flash_bem --noflash30 --subject sample
```
