---
title: mne_dipole_fit
sidebar_label: mne_dipole_fit
sidebar_position: 8
---

# mne_dipole_fit

## Overview

`mne_dipole_fit` performs electric current dipole fitting on MEG/EEG data. It computes dipole fits, saves results in `.dip` and `.bdip` formats, and can display results in a 3D brain viewer with BEM surfaces.

This is a C++ port of the original MNE-C dipole fitting tool by Matti Hämäläinen.

## Usage

```bash
mne_dipole_fit [options]
```

## Options

The command-line options are defined by the `DipoleFitSettings` class. Key options include:

| Option | Description |
|---|---|
| `--meas <file>` | Input measurement data file |
| `--set <number>` | Data set number |
| `--tmin <time>` | Start time for fitting (ms) |
| `--tmax <time>` | End time for fitting (ms) |
| `--tstep <time>` | Time step for fitting (ms) |
| `--bem <file>` | BEM model file |
| `--origin <x:y:z>` | Sphere model origin (mm) |
| `--cov <file>` | Noise covariance matrix file |
| `--mri <file>` | MRI-head coordinate transform file |
| `--dip <file>` | Output dipole file |
| `--bdip <file>` | Output binary dipole file |
| `--meg` | Use MEG data |
| `--eeg` | Use EEG data |
| `--fixed` | Fixed dipole position |
| `--accurate` | Use accurate coil descriptions |

## Description

Dipole fitting is a parametric source localization approach that models the neural activity as one or more equivalent current dipoles. At each time point, the algorithm finds the dipole location, orientation, and amplitude that best explain the measured MEG/EEG data.

### Key Concepts

- **Single dipole fit**: At each time point, a single equivalent current dipole is fitted to explain the data. This is appropriate when the brain activity of interest can be reasonably modeled as a focal source.

- **BEM vs. sphere model**: The forward model used for fitting can be either a full BEM model or a simpler sphere model. The BEM model is generally more accurate.

- **Noise covariance**: A noise-covariance matrix is used to whiten the data and weight the channels appropriately in the fitting procedure.

### Output

- **`.dip` file** — Human-readable text file with dipole parameters (time, position, orientation, amplitude, goodness-of-fit)
- **`.bdip` file** — Binary format for efficient storage and loading

### Goodness of Fit

The goodness-of-fit (GOF) value indicates how well the fitted dipole explains the measured data. Values close to 100% indicate an excellent fit, while low values suggest that a single dipole model may not be appropriate.

## Example

Fit dipoles to evoked data using a BEM model:

```bash
mne_dipole_fit \
    --meas sample_audvis-ave.fif \
    --set 1 \
    --tmin 50 --tmax 150 --tstep 1 \
    --bem sample-5120-5120-5120-bem-sol.fif \
    --mri sample-trans.fif \
    --cov sample_audvis-cov.fif \
    --meg \
    --dip sample_audvis.dip
```
