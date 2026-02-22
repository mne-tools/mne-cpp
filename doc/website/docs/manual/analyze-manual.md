---
title: MNE Analyze
sidebar_label: MNE Analyze
sidebar_position: 14
---

# MNE Analyze

:::caution Under Development
MNE Analyze is currently being ported to MNE-CPP and is not yet feature-complete compared to the original MNE-C `mne_analyze` application. The information below describes the current and planned functionality.
:::

## Overview

`mne_analyze` is a GUI application for comprehensive sensor- and source-level analysis of pre-recorded MEG/EEG data. It provides tools for data visualization, preprocessing, source localization, and interactive exploration of results.

## Current Plugins

The current MNE Analyze implementation uses a plugin-based architecture with the following plugins available:

| Plugin | Description |
|---|---|
| **Data Loader** | Loading FIFF data files |
| **Data Manager** | Managing loaded datasets |
| **Raw Data Viewer** | Browsing raw time-series data |
| **Annotation Manager** | Event marking and annotation |
| **Averaging** | Computing evoked responses |
| **Filtering** | Data filtering |
| **Channel Selection** | Selecting channel subsets |
| **Co-registration** | MEG/MRI coordinate alignment |
| **Scaling** | Signal amplitude scaling |
| **Dipole Fit** | Interactive dipole fitting |
| **Real-time** | Connection to real-time data streams |

For detailed documentation on the current MNE Analyze implementation, see the [MNE Analyze documentation](../documentation/analyze).

## Planned Features from Original MNE Analyze

The original MNE-C `mne_analyze` provided extensive functionality that is planned for future MNE-CPP releases:

- Full 3D visualization of source estimates on cortical surfaces
- Interactive ROI (Region of Interest) definition
- Time-frequency analysis displays
- Connectivity visualization
- Movie generation of source activity
- Surface morphing and group analysis visualization

## See Also

- [MNE Analyze Documentation](../documentation/analyze) — Current MNE Analyze plugin documentation
- [MNE Scan](../documentation/scan) — Real-time acquisition and processing
- [mne_inspect](tools-overview) — 3D brain visualization tool
