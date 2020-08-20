---
title: About
parent: Home
nav_order: 1
---

## What is MNE?

MNE-CPP is part of MNE, a tool suite for neurophysiology encompassing various projects that draw from MNE-C, originally written by Matti Hämäläinen (documented [here](https://mne.tools/mne-c-manual/MNE-manual-2.7.3.pdf))

Other projects include:

 - [MNE-Python](https://mne.tools/stable/index.html) - a python reimplementation of MNE-C, also extended with new visualization and analysis capabilities.
 - [MNE-MATLAB](https://mne.tools/stable/overview/matlab.html#mne-matlab) - a MATLAB interface for MNE structures and functions.
 - [MNE-C](http://www.nmr.mgh.harvard.edu/martinos/userInfo/data/MNE_register/index.php) - the original implementation of the MNE toolset in C.

## What is MNE-CPP?

MNE-CPP is both an open-source collection of cross-platform software tools and a framework for development. We provide two main GUI applications, MNE Scan and MNE Analyze, as well as several command line tools. MNE Scan is used for acquisition and real-time processing, while MNE Analyze is used for visualization and analysis of pre-recorded data.

MNE-CPP provides a cross-platform library which allows the processing of MEG/EEG data. It can be used to develop new applications, or to add functionality to existing ones.

MNE-CPP is open-source licensed under BSD (clause 3). MNE-CPP has two external dependencies, namely [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}.

## Why use MNE-CPP?

We are targeting two user groups:
* Front-End users with little or no coding background. These users are interested in straight-forward setups and applications with an easy to use GUI, such as MNE Scan and MNE Analyze. For more information see our [Learn section](pages/learn/learn.md).
* Back-End users who are experienced with programming in C++ and want to use MNE-CPP functionalities to build their own applications. For more information see our [Development section](pages/development/development.md).

To that end, we aim to create software that allows users to do as much or as little coding as they are comfortable with, by making components of our software modular and making the process of adding functionality as straight-forward as possible.

## What's next?

MNE-CPP is a community project. Contributions by new developers are always welcomed and greatly appreciated. If you are planning to contribute to MNE-CPP our [contributor page](pages/contribute/contribute.md) is a good point to start.

Currently, MNE-CPP's main development efforts can be divided into the following projects:

| ![MNEScan](images/icon_mne_scan_256x256.png) | **MNE Scan** is the real time component of MNE CPP. With its acquisition and algorithmic plugin architecture it is able to acquire and process data of several MEG/EEG systems (TMSI Refa, eegosports, gtec, brainamp, LSL, VectorView, BabyMEG) in real-time. It is in active clinical use at the Boston Children’s Hospital. |
| ![MNEAnalyze](images/icon_mne-analyze_256x256.png) | **MNE Analyze** provides an easy to use GUI for sensor and source level analysis. |
| ![LibraryAPI](images/icon_mne-lib_256x256.png) | **Library API** describes the core libraries and their APIs provided by the MNE-CPP project. All MNE-CPP applications, e.g., MNE Scan and MNE Analyze, are solely based on these libraries, Qt5 and Eigen. |

Releases are versioned based on [semantic versioning](https://semver.org/). Currently all features are subject to change as we are still making broad changes to the libraries and applications of MNE-CPP.
