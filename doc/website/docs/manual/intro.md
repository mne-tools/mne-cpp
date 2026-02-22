---
title: Introduction
sidebar_label: Introduction
sidebar_position: 1
---

# MNE-CPP Manual

## Overview

MNE-CPP provides a comprehensive set of tools for preprocessing and averaging of MEG and EEG data and for constructing cortically-constrained minimum-norm estimates. The software is based on anatomical MRI processing, forward modelling, and source estimation methods. The anatomical MRI processing routines depend on tools provided by the [FreeSurfer](https://surfer.nmr.mgh.harvard.edu/) software.

This manual gives an overview of the software modules included with MNE-CPP, describes a typical workflow for a novice user, and provides detailed information about the individual software modules.

:::info Heritage
MNE-CPP is a C++ port of the original MNE software suite created by Matti Hämäläinen at the Martinos Center for Biomedical Imaging. This manual is adapted from the original MNE Manual and the [MNE-Python documentation](https://mne.tools/), preserving the valuable background information and mathematical descriptions while documenting the C++ implementations available in MNE-CPP.
:::

## Software Components

The MNE-CPP software suite includes the following main components:

### GUI Applications

| Application | Description |
|---|---|
| [**mne_scan**](../documentation/scan) | Real-time acquisition and processing of MEG/EEG data with plugin-based architecture |
| [**mne_analyze**](../documentation/analyze) | Sensor- and source-level analysis of pre-recorded MEG/EEG data *(under development)* |
| [**mne_anonymize**](../documentation/anonymize) | Removes or modifies personal health information from FIFF files (GUI and CLI) |
| [**mne_inspect**](../documentation/inspect) | 3D brain visualization and source analysis |
| [**mne_browse**](browse-raw) | Browsing and visualization of raw MEG/EEG FIFF data files *(under development)* |

### Command-Line Tools

| Tool | Description |
|---|---|
| [**mne_compute_raw_inverse**](tools-compute-raw-inverse) | Computes inverse solutions (MNE/dSPM/sLORETA) from raw or evoked data |
| [**mne_dipole_fit**](tools-dipole-fit) | Performs electric current dipole fitting on MEG/EEG data |
| [**mne_edf2fiff**](tools-edf2fiff) | Converts EDF (European Data Format) files to FIFF format |
| [**mne_forward_solution**](tools-forward-solution) | Computes the MEG/EEG forward solution |
| [**mne_show_fiff**](tools-show-fiff) | Lists the contents of a FIFF file |
| [**mne_flash_bem**](tools-flash-bem) | Creates BEM surfaces using multi-echo FLASH MRI sequences |
| [**mne_setup_forward_model**](tools-setup-forward-model) | Sets up the BEM for forward modeling |
| [**mne_setup_mri**](tools-setup-mri) | Sets up FreeSurfer MRI data for MNE processing |
| [**mne_surf2bem**](tools-surf2bem) | Converts FreeSurfer surfaces to BEM FIFF files |
| [**mne_watershed_bem**](tools-watershed-bem) | Creates BEM surfaces using FreeSurfer's watershed algorithm |
| [**mne_rt_server**](tools-rt-server) | Real-time FIFF data streaming server |

## Prerequisites

To make full use of the MNE-CPP software, you will need:

- **FreeSurfer**: Required for anatomical MRI processing, cortical surface reconstruction, and the creation of BEM meshes. Available at [https://surfer.nmr.mgh.harvard.edu/](https://surfer.nmr.mgh.harvard.edu/).

- **MEG/EEG Data**: Data in FIFF format (the native format of Neuromag/Elekta/MEGIN MEG systems). Data from other systems can be converted using appropriate tools (e.g., `mne_edf2fiff` for EDF data).

- **Environment Variables**: Several MNE-CPP tools rely on the following environment variables:
  - `SUBJECTS_DIR` — The directory containing FreeSurfer subject reconstructions
  - `SUBJECT` — The name of the current subject
  - `FREESURFER_HOME` — The FreeSurfer installation directory

## Chapter Overview

- **[The Typical MEG/EEG Workflow](workflow)** — A step-by-step guide through a typical analysis pipeline
- **[The Forward Solution](forward)** — Coordinate systems, BEM models, and forward computation
- **[The Minimum-Norm Estimates](inverse)** — Mathematical background of inverse estimation methods
- **[Signal-Space Projection](ssp)** — The SSP method for noise rejection
- **[Command-Line Tools Reference](tools-overview)** — Detailed reference for all command-line tools
- **[MNE Browse Raw](browse-raw)** — The raw data browser application
- **[MNE Analyze](analyze-manual)** — The analysis and visualization application

## Acknowledgments

The development of the MNE software has been supported by the National Center for Research Resources (NCRR), the NIH (grants R01EB009048, R01EB006385, R01HD40712), and the U.S. Department of Energy. We are grateful to the MNE software users at the Martinos Center and other institutions for their collaboration and valuable feedback on the software and its documentation.
