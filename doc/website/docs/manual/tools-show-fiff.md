---
title: mne_show_fiff
sidebar_label: mne_show_fiff
sidebar_position: 10
---

# mne_show_fiff

## Overview

`mne_show_fiff` lists the contents of a FIFF file to standard output, showing the block structure and tag information in a human-readable format.

## Usage

```bash
mne_show_fiff [options]
```

## Options

| Option | Description |
|---|---|
| `--in <name>` | Input FIFF file |
| `--fif <name>` | Synonym for `--in` |
| `--blocks` | Only list blocks (tree structure), omitting individual tags |
| `--verbose` | Verbose output with detailed tag contents |
| `--indent <no>` | Number of indentation spaces (default: 3 for terse, 0 for verbose) |
| `--tag <no>` | Show information about specific tag numbers (can be specified multiple times) |
| `--long` | Print long strings in full (normally truncated) |
| `--help` | Print usage help |
| `--version` | Print version information |

## Description

`mne_show_fiff` is a diagnostic utility for inspecting FIFF files. It uses FIFF tag definitions to produce human-readable output describing the file's contents, including:

- **Block structure** — The hierarchical tree of data blocks
- **Tag information** — Individual data elements with their types, sizes, and values
- **Tag descriptions** — Human-readable names for standard FIFF tags

### Use Cases

- Verifying the contents of a FIFF file after conversion or processing
- Debugging data pipeline issues
- Understanding the structure of FIFF files
- Checking whether specific data (e.g., projectors, bad channels, events) are present in a file

## Examples

List the block structure of a FIFF file:

```bash
mne_show_fiff --in sample_audvis_raw.fif --blocks
```

Show detailed contents:

```bash
mne_show_fiff --in sample_audvis_raw.fif --verbose
```

Inspect specific tags:

```bash
mne_show_fiff --in sample_audvis_raw.fif --tag 102 --tag 200
```
