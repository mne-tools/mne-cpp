---
title: Overview
has_children: false
nav_order: 4
---
# Overview

Standalone MEG/EEG applications, especially for clinical use, e.g., acquisition, real-time processing or data browsing software, comprise more or less fixed procedures, which have the need of being straightforward to use and fast in processing required tasks.

MNE-CPP can be used to build such standalone software applications, which offer a wide variety of neuroscientific tools and are easy to use. MNE-CPP provides a cross-platform library which allows the processing of MEG and EEG data. It can be used to develop performant cross-platform applications, which next to the common operation systems also include smartphones, tablets and embedded systems.

Besides the basic library (MNE Lib) we deliver a number of applications, e.g., an acquisition and real-time processing framework MNE Scan, connectors for the EEG and MEG systems, a raw data browser MNE Browse, to name a few. MNE-CPP is open source BSD licensed (clause 3) and can be freely accessed. MNE-CPP has two external dependencies, namely Qt and Eigen. We are targeting two user groups:

* Front-End users: Scientists with little or no coding background, who want to use rich GUI interfaces for their data analysis and research. These users are interested in convenient setups (see Download section) and easy to use standalone applications, such as MNE Scan, MNE Browse and MNE Analyze.
* Back-End users: Scientists who are more or less experienced with C++ programming and want to use MNE-CPP functionalities in their own applications. Contributions by new developers are always welcomed and greatly appreciated. If you are planning to develop with MNE-CPP our Development page is a good starting point.

Currently we are focusing on four key applications for online and offline processing of EEG and MEG data:

----------------

![MNEScan](../images/icon_mne_scan_256x256.png)

## MNE Scan

MNE Scan (former MNE-X) is an open-source and cross platform application built with tools provided by the MNE-CPP framework. Currently, it is in active clinical use at the Boston Children’s Hospital. With its acquisition and algorithmic plugin architecture it is the ideal tool for acquiring and processing data of the novel babyMEG system in real-time. Next to the babyMEG MNE Scan is also able to connect to the Neuromag VectorView system and EEG amplifiers (TMSI Refa, eegosports, gtec, brainamp) and provide MEG/EEG data streams for real-time processing.

----------------

![MNEBrowse](../images/icon_browse_raw_256x256.png)

## MNE Browse

MNE Browse is a MEG/EEG data browser for pre-processing and visualization. Some exemplary functionalities are the convenient way of selecting MEG/EEG channels of interest from a given 2D layout, the computation of averages and temporal filtering.

----------------

![MNEAnalyze](../images/icon_mne-analyze_256x256.png)

## MNE Analyze

MNE Analyze is currently undergoing development. When finished it will provide easy to use functionalities for source level analysis. All 3D and 2D visualizations are based on MNE-CPP’s Disp3D library, which is based on Qt’s Qt3D module.

----------------

![MNELib](../images/icon_mne-lib_256x256.png)

## MNE Lib

MNE Lib hosts the core libraries provided and maintained by the MNE-CPP project. All the above mentioned applications are based on these libraries.
