---
title: Changelog
parent: Releases
grand_parent: Install
nav_order: 1
---

# Changelog

## Version 0.1.1

### Applications

MNE Analyze:
* Fix bug during deployment of dynamically linked MNE Analyze version on macOS
* Renamed MNE Analyze extensions to plugins 
* Fix issue with display width, now displays only full seconds as selected 
* Jump viewer to selected annotation with 'J' key 
* Removed seemingly unused timer debug outputs 
* Documentations and variable name changes for readability 
* Add plugin control views to menu bar 
* Clean up command line output 
* Add time information on the y-axis

MNE Scan:
* Fix problems with Source Localization and Connectivity plugins
* Fix problems where the QuickControlView was not populated with plugin control GUI widgets correctly

MNE Anonymize:
* Overall improvements and bug fixes to MNE Anonymize

### API libraries

All:
* Rename libraries and fix versioning

Fiff:
* Fix bug when reading gantry_angle from Fiff file

### Tools
* Fix template class in Qt Creator wizard for MNE-CPP classes
* Update test and example Qt Creator wizards
	
### Continuous Integration
* Only branch off when a minor or major version bump occurred
* Remove folders which we do not want to ship from dynamic builds

### Documentation
* Updated information on continuous integration
* Improved build from source guide
* Updated guide on streaming pre-recorded data in MNE Scan

### Authors

People who contributed to this release (preceded by number of commits):

(115) Juan Garcia-Prieto,
(43) Lorenz Esch,
(7) Gabriel Motta,
(2) Ruben Dörfel,

## Version 0.1.0

### Changes

New applications:

* **MNE Scan** including new plugins: fiffsimulator, ftbuffer, babymeg, natus, brainamp, eegosports, gusbamp, tmsi, brainflowboard, lsladapter, dummytoolbox, rtcmne, averaging, covariance, noisereduction, neuronalconnectivity, writetofile, hpi
* **MNE Analyze** including new plugins: dataloader, datamanager, rawdataviewer, annotationmanager
* **MNE Rt Server**
* **MNE Forward Solution**
* **MNE Dipole Fit**

New API libraries:

* **utils** - Design patterns, generlaized classes, mathematical routines, I/O helpers
* **fs** - FreeSurfer I/O routines
* **fiff** - Fiff I/O routines
* **mne** - I/O routines for MNE objects 
* **fwd** - Forward modeling
* **inverse** - Inverse modeling
* **communication** - Tools for real-time communication
* **rtprocessing** - Tools for real-time data processing
* **connectivity** - Functional connectivity metrics
* **disp** - 2D visualization routines
* **disp3D** - 3D visualization routines

### Authors

People who contributed to this release (preceded by number of commits):

(3118) Lorenz Esch,
(2264) Christoph Dinh,
(384) Gabriel Motta,
(275) Ruben Doerfel,
(253) Lars Debor,
(160) Juan Garcia-Prieto,
(149) Viktor Klueber,
(113) Jana Kiesel,
(107) Ricky Tjen,
(105) Martin Henfling,
(92) Limin Sun,
(71) Daniel Knobl,
(69) Florian Schlembach,
(61) Daniel Strohmeier,
(56) Simon Heinke,
(37) Andrey Parfenov,
(35) Tim Kunze,
(31) Wayne Mead,
(24) Felix Arndt,
(16) Louise Eichhorst,
(13) Seok Lew,
(11) Christof Pieloth,
(9) Felix Griesau,
(9) Chiran Doshi,
(8) Robert Dicamillo,
(6) Johannes Vorwerk,
(6) Erik Hornberger,
(6) Sugandha Sachdeva,
(5) Faris Yahya,
(4) Blerta Hamzallari,
(4) Marco Klamke,
(4) Julius Lerm,
(3) Mainak Jas,
(3) Franco Polo,
(3) Benjamin Kay,
(3) Petros Simidyan,
(2) Martin Luessi,
(2) Eric Larson