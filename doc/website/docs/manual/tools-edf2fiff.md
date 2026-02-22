---
title: mne_edf2fiff
sidebar_label: mne_edf2fiff
sidebar_position: 9
---

# mne_edf2fiff

## Overview

`mne_edf2fiff` converts EDF (European Data Format) files to FIFF format. It supports variable channel frequencies.

## Usage

```bash
mne_edf2fiff [options]
```

## Options

| Option | Description |
|---|---|
| `--fileIn <file>` | Input EDF file **(required)** |
| `--fileOut <file>` | Output FIFF file (default: same name with `.fif` extension) |
| `--scaleFactor <value>` | Raw value scaling factor (default: 1e6) |

## Description

The European Data Format (EDF) is a widely used standard for storing multi-channel electrophysiological signals. This tool converts EDF files to the FIFF format used by the MNE software suite.

### Supported Features

- Variable sampling frequencies across channels
- Standard EDF header information is preserved in the FIFF output

### Limitations

- Interrupted recordings are **not** supported
- EDF+ annotations are not converted

## Example

Convert an EDF file to FIFF:

```bash
mne_edf2fiff --fileIn recording.edf --fileOut recording.fif
```

With a custom scaling factor:

```bash
mne_edf2fiff --fileIn recording.edf --scaleFactor 1e3
```
