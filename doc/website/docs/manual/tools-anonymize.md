---
title: mne_anonymize
sidebar_label: mne_anonymize
sidebar_position: 11
---

# mne_anonymize

## Overview

`mne_anonymize` removes or modifies Personal Health Information (PHI) and Personal Identifiable Information (PII) from FIFF files. It can operate in both GUI and command-line modes.

## Usage

```bash
mne_anonymize [options]
```

## Options

| Option | Description |
|---|---|
| `--no-gui` | Run in command-line mode (no GUI) |
| `--version` | Show version information |
| `-i, --in <infile>` | Input FIFF file to anonymize **(required)** |
| `-o, --out <outfile>` | Output file (default: input file with `_anonymized.fif` suffix) |
| `-v, --verbose` | Verbose output |
| `-s, --silent` | Suppress all terminal output |
| `-d, --delete_input_file_after` | Delete input file after successful anonymization |
| `-f, --avoid_delete_confirmation` | Skip deletion confirmation dialog |
| `-b, --brute` | Also anonymize weight, height, sex, handedness, and project data |
| `--md, --measurement_date <DDMMYYYY>` | Set measurement date to specified value |
| `--mdo, --measurement_date_offset <days>` | Offset measurement date by N days |
| `--sb, --subject_birthday <DDMMYYYY>` | Set subject birthday to specified value |
| `--sbo, --subject_birthday_offset <days>` | Offset subject birthday by N days |
| `--his <id>` | Set Hospital Information System subject ID |
| `--mne_environment` | Also anonymize MNE working directory and command line tags |

## Description

`mne_anonymize` is an essential tool for preparing MEG/EEG data for sharing while complying with data privacy regulations (e.g., HIPAA, GDPR). It modifies or removes the following information from FIFF files:

### Standard Anonymization (default)

- **Measurement date** — Replaced with a default date or user-specified value
- **MAC address** — Removed
- **Experimenter name** — Cleared
- **Subject information** — Name, birthday, ID cleared
- **MNE toolbox metadata** — Working directory and command history cleared

### Brute Mode (`--brute` flag)

In addition to the standard anonymization, brute mode also removes:

- Subject weight and height
- Subject sex and handedness
- Project information (ID, name, aim, persons, comment)

### Date Offsetting

Instead of replacing dates with default values, you can use offset options to shift dates by a fixed number of days. This preserves temporal relationships (e.g., age at measurement) while anonymizing absolute dates:

```bash
# Shift all dates back by 365 days
mne_anonymize --in data.fif --mdo -365 --sbo -365
```

## Examples

Basic anonymization:

```bash
mne_anonymize --no-gui --in subject01_raw.fif
```

Full anonymization with specific output file:

```bash
mne_anonymize --no-gui --in subject01_raw.fif --out anon_raw.fif --brute
```

Anonymize with date offset (preserving age):

```bash
mne_anonymize --no-gui --in subject01_raw.fif \
    --mdo -1000 --sbo -1000 --brute
```

Anonymize and delete original:

```bash
mne_anonymize --no-gui --in subject01_raw.fif \
    --delete_input_file_after --avoid_delete_confirmation --brute
```
