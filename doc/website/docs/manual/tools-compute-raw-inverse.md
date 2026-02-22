---
title: mne_compute_raw_inverse
sidebar_label: mne_compute_raw_inverse
sidebar_position: 7
---

# mne_compute_raw_inverse

## Overview

`mne_compute_raw_inverse` computes inverse solutions (MNE, dSPM, or sLORETA) from raw or evoked FIFF data using a pre-computed inverse operator. It supports label-restricted source estimation and outputs results as STC files.

This is a C++ port of the original MNE-C tool by Matti Hämäläinen.

## Usage

```bash
mne_compute_raw_inverse [options]
```

## Options

| Option | Description |
|---|---|
| `--in <file>` | Raw or evoked data input file **(required)** |
| `--inv <file>` | Inverse operator file **(required)** |
| `--snr <value>` | SNR value to use for regularization (default: 1.0) |
| `--nave <number>` | Number of averages (default: 1 for raw, from data for evoked) |
| `--set <number>` | Evoked data set number to process (default: process all) |
| `--bmin <time>` | Baseline starting time in milliseconds |
| `--bmax <time>` | Baseline ending time in milliseconds |
| `--label <file>` | Label file to restrict processing to (can specify multiple) |
| `--labeldir <dir>` | Process all labels in directory, compute average waveform per label |
| `--out <file>` | Output file name (needed when using `--labeldir`) |
| `--picknormalcomp` | Pick current component normal to cortex only |
| `--spm` | Use dSPM noise-normalization method |
| `--sloreta` | Use sLORETA noise-normalization method |
| `--mricoord` | Output source locations in MRI coordinates |
| `--orignames` | Use original label file names in channel names |
| `--align_z` | Align waveform signs using surface normal information |
| `--labellist <file>` | Output label name list to specified file |

## Description

This tool applies a pre-computed inverse operator to MEG/EEG data to produce source estimates on the cortical surface. The inverse operator must be pre-computed and stored in a FIFF file.

### Inverse Methods

Three methods are available for computing the source estimates:

- **MNE (default)** — Standard minimum-norm estimate. Provides current amplitude estimates in physical units (Am).

- **dSPM** (`--spm` flag) — Dynamic Statistical Parametric Mapping. Produces noise-normalized estimates that are dimensionless statistical test variables. Reduces location bias compared to MNE.

- **sLORETA** (`--sloreta` flag) — Standardized Low-Resolution Electromagnetic Tomography. Another noise normalization approach that uses the resolution matrix diagonal for variance estimation.

For mathematical details on these methods, see [The Minimum-Norm Estimates](inverse).

### Regularization (SNR)

The `--snr` parameter controls the regularization of the inverse solution. The regularization parameter $\lambda^2$ is related to the SNR by $\lambda^2 \approx 1/\text{SNR}^2$.

- **Higher SNR** → less regularization → noisier but potentially more detailed estimates
- **Lower SNR** → more regularization → smoother estimates

For averaged evoked data, typical SNR values are 1.0–3.0. The number of averages (`--nave`) is automatically taken into account.

### Label-Based Analysis

The `--label` option restricts the inverse computation to a specific cortical region defined by a FreeSurfer label file. Multiple labels can be specified. This is useful for ROI-based analyses.

The `--labeldir` option processes all labels in a directory and computes the average source waveform for each label, which is useful for atlas-based analyses.

### Output

The output is written as STC (source estimate) files, which contain the estimated source activity at each source space location over time. These files can be visualized using `mne_inspect` or other MNE visualization tools.

## Examples

Compute dSPM source estimates from evoked data:

```bash
mne_compute_raw_inverse \
    --in sample_audvis-ave.fif \
    --inv sample_audvis-meg-oct6-inv.fif \
    --snr 3.0 \
    --spm
```

Compute MNE estimates restricted to a label:

```bash
mne_compute_raw_inverse \
    --in sample_audvis-ave.fif \
    --inv sample_audvis-meg-oct6-inv.fif \
    --snr 2.0 \
    --label auditory-lh.label \
    --picknormalcomp
```

Process all labels in a directory:

```bash
mne_compute_raw_inverse \
    --in sample_audvis-ave.fif \
    --inv sample_audvis-meg-oct6-inv.fif \
    --snr 3.0 --spm \
    --labeldir labels/ \
    --out label_timecourses
```
