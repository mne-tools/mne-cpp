---
title: Library API
sidebar_label: Library API
---

# MNE-CPP Library API

MNE-CPP provides a modular set of cross-platform C++ libraries built on [Qt](https://www.qt.io/) and [Eigen](http://eigen.tuxfamily.org/). All MNE-CPP applications (MNE Scan, MNE Analyze, MNE Anonymize, and the command-line tools) are built solely on these libraries.

Below you will find an overview of each library and its capabilities, followed by guides for using the APIs in your own applications.

## Library Overview

### Utils

- Basic mathematical routines (`MNEMath`)
- Tapered spectral representations: FFT, CSD, PSD (`Spectral`)
- Spectrogram computation (`Spectrogram`)
- Thin Plate Spline warping of 3D surfaces (`Warp`) — e.g., warping template BEM models to digitizer points
- Sphere fitting (`Sphere`)
- Simplex solver (`SimplexAlgorithm`)
- MNE `.sel` selection group file I/O (`SelectionIO`)
- 2D layout generation from 3D layouts (`LayoutMaker`)
- 2D layout loading from ANT `.elc` and MNE `.lout` files (`LayoutLoader`)
- K-Means clustering (`KMeans`)
- Binary data stream I/O and Eigen matrix `.txt` file I/O (`IOUtils`)
- Circular buffer for arbitrary data types (`CircularBuffer`)
- Design patterns: command pattern, observer pattern
- Application logger based on `QDebug` (`ApplicationLogger`)

### Fs

- Load and save FreeSurfer-generated surfaces and annotations

### Fiff

- Load and save FIFF (Functional Imaging File Format) files

### Mne

- Data structures matching MNE-C: `BemSurface`, `Bem`, `EpochData`, `EpochDataList`, `ForwardSolution`, `Hemisphere`, `InverseOperator`, `ProjectToSurface`, `SourceEstimate`, `SourceSpace`, `Surface`

### Fwd

- Forward solution computation based on FreeSurfer reconstructions for MEG/EEG source localization

### Inverse

- Single dipole fitting
- HPI coil fitting
- MNE source localization
- RAP-MUSIC source localization (GPU/CUDA implementation available)

### Communication

- Client and command specifications for communicating with a running `mne_rt_server`

### RtProcessing

- General electrophysiological data processing: averaging, filtering, ICP, trigger detection, SPHARA
- Asynchronous processing classes for: averaging, functional connectivity, HPI fitting, inverse operator creation, noise estimation — these outsource computation to separate threads via Qt's signal/slot system

### Connectivity

- Network data storage (nodes, edges) with basic graph measures: in-degree, clustering, thresholding, connectivity matrix generation
- Functional connectivity methods: Correlation, Cross-Correlation, Coherence, Imaginary Coherence, PLV, PLI, WPLI, Unbiased Squared PLI, Debiased Squared WPLI

### Disp

- Plot types: bar, graph, imagesc, lineplot, spline, tfplot
- Ready-to-use viewers with GUI for `QWidget`-based applications

### Disp3D

- Model/delegate/view architecture for 3D visualization
- Supported data: source estimates, functional connectivity networks, digitizers, BEM models, FreeSurfer surfaces, 3D topoplots (MEG/EEG), sensor information, source space information

## Development Guides
