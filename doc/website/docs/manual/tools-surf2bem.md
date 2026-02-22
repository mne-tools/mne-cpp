---
title: mne_surf2bem
sidebar_label: mne_surf2bem
sidebar_position: 3
---

# mne_surf2bem

## Overview

`mne_surf2bem` converts FreeSurfer surfaces and/or ASCII triangle files into BEM FIFF files. It supports multiple surface inputs with separate IDs and conductivities, vertex shifting along normals, surface reordering, and topology checks.

This is a C++ port of the original MNE-C tool by Matti Hämäläinen.

## Usage

```bash
mne_surf2bem [options]
```

## Options

| Option | Description |
|---|---|
| `--surf <name>` | Input FreeSurfer binary surface file |
| `--tri <name>` | Input ASCII triangle file |
| `--fif <name>` | Output FIFF BEM surface file |
| `--id <id>` | BEM surface ID: 1 = brain, 3 = skull, 4 = head. Applies to the last `--surf` or `--tri` specified. |
| `--swap` | Swap vertex winding order (for ASCII files) |
| `--meters` | Coordinates are in meters (default: millimeters) |
| `--coordf <no>` | Coordinate frame for ASCII vertices |
| `--shift <val>` | Shift vertices along normals by specified amount (mm) |
| `--ico <number>` | Downsample to icosahedron subdivision level (0–6) |
| `--sigma <val>` | Compartment conductivity (S/m) |
| `--force` | Load the surface despite topological defects |
| `--check` | Perform topology and solid angle checks |
| `--checkmore` | Also check skull/skin thicknesses |

## Description

`mne_surf2bem` is used to convert surfaces from FreeSurfer binary format or ASCII triangle format into the FIFF BEM format required by the MNE forward solution computation. Multiple surfaces can be combined into a single BEM file by specifying multiple `--surf` or `--tri` options with corresponding `--id` values.

### Surface IDs

| ID | Surface |
|---|---|
| 1 | Brain (inner skull) |
| 3 | Skull (outer skull) |
| 4 | Head (scalp) |

### Topology Checks

When `--check` is specified, the following checks are performed:

- **Completeness** — The total solid angle subtended by all triangles should be close to $4\pi$
- **Surface ordering** — Surfaces are verified to be properly nested (inner skull inside outer skull inside scalp)
- **Extent** — Surface extent must be at least 50 mm (to detect meters/millimeters confusion)

## Example

Convert a FreeSurfer inner skull surface to BEM format:

```bash
mne_surf2bem --surf inner_skull.surf --id 1 --fif inner_skull-bem.fif --check
```

Create a three-layer BEM from FreeSurfer surfaces:

```bash
mne_surf2bem \
    --surf inner_skull.surf --id 1 --sigma 0.3 \
    --surf outer_skull.surf --id 3 --sigma 0.006 \
    --surf outer_skin.surf --id 4 --sigma 0.3 \
    --fif three-layer-bem.fif --check
```
