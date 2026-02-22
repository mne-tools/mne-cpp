---
title: mne_setup_forward_model
sidebar_label: mne_setup_forward_model
sidebar_position: 4
---

# mne_setup_forward_model

## Overview

`mne_setup_forward_model` sets up the BEM for forward modeling. It reads triangulated surface files (inner skull, outer skull, scalp), creates a BEM geometry FIFF file with conductivity assignments, exports `.pnt` and `.surf` files, and optionally computes the BEM solution matrix.

This is a C++ port of the original MNE shell script by Matti Hämäläinen.

## Usage

```bash
mne_setup_forward_model [options]
```

## Options

| Option | Description |
|---|---|
| `--subject <name>` | Subject name. Defaults to `SUBJECT` environment variable. |
| `--subjects-dir <dir>` | Subjects directory. Defaults to `SUBJECTS_DIR` environment variable. |
| `--scalpc <value>` | Scalp conductivity in S/m (default: 0.3) |
| `--skullc <value>` | Skull conductivity in S/m (default: 0.006) |
| `--brainc <value>` | Brain conductivity in S/m (default: 0.3) |
| `--model <name>` | BEM model name (auto-generated if omitted) |
| `--homog` | Use a single-compartment (homogeneous) model |
| `--surf` | Use FreeSurfer `.surf` files instead of ASCII `.tri` files |
| `--ico <number>` | Downsample surfaces to icosahedron subdivision level (0–6) |
| `--nosol` | Skip BEM solution preparation |
| `--noswap` | Don't swap triangle winding order |
| `--meters` | Coordinates are in meters (for ASCII `.tri` files) |
| `--innershift <mm>` | Shift inner skull surface outward by specified amount |
| `--outershift <mm>` | Shift outer skull surface outward by specified amount |
| `--scalpshift <mm>` | Shift scalp surface outward by specified amount |
| `--overwrite` | Overwrite existing output files |

## Description

This tool is a critical step in the MEG/EEG source localization pipeline. It takes the triangulated BEM surfaces (created by `mne_watershed_bem` or `mne_flash_bem`) and sets up the boundary element model with appropriate conductivity values for each compartment.

### Conductivity Values

The default conductivity values are:

| Compartment | Default (S/m) | Typical ratio |
|---|---|---|
| Scalp | 0.3 | 1 |
| Brain | 0.3 | 1 |
| Skull | 0.006 | 1/50 |

The skull conductivity ratio has been a subject of considerable discussion in the literature. Recent publications report values ranging from 1:15 to 1:50 relative to brain/scalp conductivity.

### Single-Compartment Model

For MEG-only analyses, a single-compartment (homogeneous) BEM model using only the inner skull surface is often sufficient. Use the `--homog` flag for this:

```bash
mne_setup_forward_model --homog --subject sample
```

### Surface Shifting

The `--innershift`, `--outershift`, and `--scalpshift` options allow shifting surfaces outward along their normals. This can be useful when surfaces are too close together or intersecting.

## Example

Set up a three-layer BEM model:

```bash
export SUBJECTS_DIR=/path/to/subjects
mne_setup_forward_model --subject sample --surf --overwrite
```

Set up with custom conductivities:

```bash
mne_setup_forward_model --subject sample --surf \
    --scalpc 0.33 --skullc 0.0132 --brainc 0.33
```
