---
title: Overview
parent: Home
nav_order: 1
---
# Overview

Standalone MEG/EEG applications, especially for clinical use, e.g., acquisition, real-time processing or data browsing, comprise more or less fixed procedures, which have the need of being straightforward to use and fast in processing required tasks.

MNE-CPP can be used to build such standalone software applications, which offer a variety of neuroscientific tools and are easy to use. MNE-CPP provides a cross-platform library which allows the processing of MEG/EEG data. It can be used to develop cross-platform applications, which next to the common operation systems also include mobile devices and embedded systems.

Besides the basic library (MNE Lib) we deliver a number of applications, e.g., an acquisition and real-time processing application (MNE Scan), as well as an offline processing application (MNE Analzye). MNE-CPP is open source BSD licensed (clause 3) and can be freely accessed. MNE-CPP has two external dependencies, namely [Qt](https://www.qt.io/) and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page). 

We are targeting two user groups:
* Front-End users with little or no coding background. These users are interested in straight-forward setups and applications with an easy to use GUI, such as MNE Scan and MNE Analyze.
* Back-End users who are experienced with programming in C++ and want to use MNE-CPP functionalities to build their own applications. 

MNE-CPP is a community project. Contributions by new developers are always welcomed and greatly appreciated. If you are planning to contribute to MNE-CPP our [contributor page](pages/contribute/contribute.md) is a good point to start.

Currently, MNE-CPP's development efforts can roughly be divided into the following projects:

| ![MNELib](images/icon_mne-lib_256x256.png) | **MNE Lib** describes the core libraries provided by the MNE-CPP project. MNE Scan and MNE Analyze are solely based on these libraries. |
| ![MNEScan](images/icon_mne_scan_256x256.png) | **MNE Scan** (former MNE-X) is an open-source and cross platform application built with tools provided by the MNE-CPP framework. Currently, it is in active clinical use at the Boston Children’s Hospital. With its acquisition and algorithmic plugin architecture it is able to acquire and process data of several MEG/EEG systems (TMSI Refa, eegosports, gtec, brainamp, LSL, VectorView, BabyMEG) in real-time. |
| ![MNEAnalyze](images/icon_mne-analyze_256x256.png) | **MNE Analyze** is currently in an early development state. When finished it will provide easy to use GUI for sensor and source level analysis. |


