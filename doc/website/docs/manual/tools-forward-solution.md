---
title: mne_forward_solution
sidebar_label: mne_forward_solution
sidebar_position: 6
---

# mne_forward_solution

## Overview

`mne_forward_solution` computes the MEG/EEG forward solution for source localization. It calculates the magnetic fields and electric potentials at the measurement sensors and electrodes due to dipole sources located on the cortex or in a volume source space.

This is a C++ port of the original MNE-C tool by Matti Hämäläinen.

## Usage

```bash
mne_forward_solution [options]
```

## Options

The command-line options are defined by the `ComputeFwdSettings` class and follow the standard MNE-C forward solution options. Key options include:

| Option | Description |
|---|---|
| `--src <file>` | Source space file |
| `--bem <file>` | BEM model file |
| `--meas <file>` | Measurement data file (for sensor info) |
| `--fwd <file>` | Output forward solution file |
| `--trans <file>` | Head-MRI coordinate transform file |
| `--meg` | Compute MEG forward solution |
| `--eeg` | Compute EEG forward solution |
| `--mindist <dist>` | Minimum source-to-inner-skull distance (mm) |
| `--accurate` | Use accurate coil geometry descriptions |
| `--fixed` | Fixed source orientations (normal to cortex) |
| `--all` | Compute for all source space points |

## Description

The forward solution is a critical component of the MEG/EEG source localization pipeline. It maps the relationship between source currents on the cortex (or in the volume) and the measured signals at the MEG sensors and EEG electrodes.

### Prerequisites

Before computing the forward solution, the following must be available:

1. **Source space** — Created from FreeSurfer surface reconstruction
2. **BEM model** — Created using `mne_watershed_bem` or `mne_flash_bem`, then set up with `mne_setup_forward_model`
3. **Coordinate transformation** — The head-to-MRI transform stored in a `-trans.fif` file
4. **Measurement info** — A FIFF file containing sensor locations and orientations

### BEM vs. Sphere Model

The forward solution can be computed using either:

- **BEM (Boundary Element Model)** — More accurate, requires BEM surfaces
- **Sphere model** — Simpler, uses concentric sphere approximation

### Software Gradient Compensation

For CTF and 4D Neuroimaging data that has been subjected to software gradient compensation, the forward solution automatically accounts for the reference sensor array and computes a compensated forward solution.

## Example

Compute a MEG forward solution:

```bash
mne_forward_solution \
    --src sample-oct6-src.fif \
    --bem sample-5120-5120-5120-bem-sol.fif \
    --meas sample_audvis_raw.fif \
    --trans sample-trans.fif \
    --meg \
    --fwd sample-meg-fwd.fif
```

Compute a combined MEG+EEG forward solution with accurate coil descriptions:

```bash
mne_forward_solution \
    --src sample-oct6-src.fif \
    --bem sample-5120-5120-5120-bem-sol.fif \
    --meas sample_audvis_raw.fif \
    --trans sample-trans.fif \
    --meg --eeg --accurate \
    --fwd sample-meg-eeg-fwd.fif
```
