---
title: Changelog
parent: Download
nav_exclude: true
---

# Changelog

## Version 0.1.9 - 2021/03/02

### Applications

MNE Analyze
 * Handle removal and deletion of child item models
 * Add event for model removal
 * Changed event documentation format
 * Gave AnalyzeData a communicator to send events
 * Implemented clearing of views in plugins that use models
 * Data Loader now saves last folder data was loaded from.
 * Code cleanup to fix some compiler warnings and memory leaks
 * Show file and filter info in signal viewer.

MNE Anonymize
 * Fix bug when anonymizing dates
 * Update date command line option
 * Fix link to documentation web page
 * Use QDate instead of QDateTime when referring to birthday date
 * add console to CONFIG in pro file

MNE Scan
* Change saving to file to account for calibration values when saving data

EDF-To-Fiff Converter
* Added edf to fiff converter command line application

### API librariers

Disp
 * Added clearView() function to AbstractView
 * Change default loaded values for ScalingView and FiffRawViewSettings
 * Made FilterSettingsView and FilterDesignView reflect filter parameter changes made in each other
 * Fix updating filter parameters in FilterDesignView after loading filter from file
 * Update scaling of FilterDesignView plotting and removed scroll bars.
 * Made updateFilterPlot public in FilterDesignView
 * Text color in the FilterPlotScene class is now dependant on the FilterDesignView colors, dependent on the Qt stylesheet

Disp3D
 * Fix applying the same rotation animation on the parent and children of a Renderable3DEntity.
 * Make use of parallel animation starting via QParallelAnimationGroup
 * Orbit camera instead of rotating objects in 3D view

RtProcessing
 * Changed enums in FilterKernel to static class members of new FilterParameter class and replaced static functions

### Continuous Integration

 * Update CI to use Qt 5.15.2, which solves a problem with Qt3D on MacOS
 * Remove setting the noOpenGLWidget flag when Qt version is > 5.15.0 to fix signal plotting in macOs
 * Add scripts (/tools/testing) and outsource calling of tests from CI so devs can run tests locally more easily.
 * Make test_win powershell script callable from anywhere

### Documentation

 * fixed some layout issues in the changelog
 * fixed website page links with wrong path
 * Docu fixes in Renderable3DEntity
 * Add CI testing documentation to contribution guide
 * Update eegosports docu
 * Include more about event system and using event system when creating plugin
 * Update E-Mail addresses in the contact page
 * Reorganize and restructure webpage

 ### Authors

 People who contributed to this release (preceded by number of commits):

(89) Juan GPC,
(49) Gabriel Motta,
(22) Simon Heinke,
(17) Lorenz Esch,
(10) Alex Rockhill

## Version 0.1.8 - 2020/11/19

### Applications

MNE Analyze
 * Moved View3D controls to settings view
 * New event and struct for sending settings
 * Fix GUI spacing in averaging plugin
 * New Dipole Fit plugin for MNE Analyze
 * Loading new files: *trans.fif, *cov-fif
 * New data models for dipole fit, mri transform, noise
 * Website documentation for new plugin
 * loading *ave.fif without raw file parent

MNE Scan
 * Remove no longer needed applications/mne_analyze/libs/anShared/Interfaces/IStandardView.h
 * Remove no longer needed applications/mne_analyze/libs/anShared/Management/analyzesettings.cpp
 * Rename interfaces to abstract classes, since they are described better as abstract classes
 * Improve Babymeg plugin to feature connection GUI elements. Also fix some bugs which led to crashing when
   connecting to the acquisition boards.
 * Update include path of ui_ files
 * Rename data function in measurement classes to measurementData() since QSharedPointer already has a data() function.
 * Comment out set_current_comp function in noise plugin since this lead to multiple calls and errors when bit wise shifting the coil type.
 * Add error catching when channels are empty in HPIFit's fitHPI function
 * Update dummy toolbox pro and source/header files
 * Bump up BrainFlow version
 * Make clustering in the MNE Scan forward solution plugin the default

MNE RtServer
 * Add file command line argument

### API librariers

Disp3D
 * Fix fiducial coloring problem in Disp3D
 * Change fiducial colors to match the ones of mne-python
 * Add remove option to context menu (right click) in the Control3DView. Subsequently, the 3D data is removed from the 3D data model and view

Disp
 * New Dipole Fit View.

### Continuous Integration

 * Cleanup qmake files for transition to CMake (Qt6 only supports CMake)
 * Move deployment to CI
 * Outsource deployment commands to scripts
 * Update ubuntu runners
 * Update dropbox links
 * Update mne-cpp-test-data submodule to newest commit
 * Create new static deployment scripts
 * Use linuxdeployqt for linux static builds as well
 * Fix mnecpp library copying on macos.
 * Fix mkdir in windows workflow.
 * Add testci workflow script which allows convenient CI testing without actually creating a GH PR
 * Fix setting PATH variable via the now deprecated add-path in GH Actions scripts
 * Remove hub install action in release.yml since HUB CLI is already a part of all runners.
 * Release macOS dynamic build based on Qt 5.14.2. This should fix QOpenGlWidget problems.

### Documentation

 * Update resource and dependency documentation
 * Update settings plugin documentation to include 3D controls
 * Update deployment documentation
 * Renamed website section Install to Download
 * Website section Contribute is now a subsection of Develop
 * Link download links on our website to always point to dynamic versions
 * Update documentation for the real-time source localization pipeline

### Other

 * Make building without app bundles on macOS the default (can be reverted by using the new flag withAppBundles)
 * Fix Wasm problems introduced by PR #713 (View3D controls in control manager plugin)
 * Cleanup mne-cpp.pri file
 * Move resources folder back to bin folder to reduce complexity in the qmake/CMake files
 * Add @executable_path to rpath setup when building for macOS. (can start the executable from the explorer)

### Authors

People who contributed to this release (preceded by number of commits):

(52) Lorenz Esch,
(48) Gabriel Motta,
(1) Andrey Parfenov

## Version 0.1.7 - 2020/10/26

### Applications

MNE Analyze
 * Add new plugin to perform source localization in MNE Analyze. This is just the skeleton of the plugin and is not functional yet.
 * Add new plugin to perform 3D visualization. This plugin provides a Disp3D View3D window in a QWidget container The data can be passed to the plugin via MNE Analyze's event subscription system.
 * Fixes to Channel Select plugin (preserve state in-between sessions, change default layouts)
 * Create control plugin and merge functionality with scaling plugin
 * Increased max visible channels in Signal Viewer plugin to 100
 * Added AveragingDataModel which stores averaging data
 * Ability to add and switch between averaging data using the Data Manager
 * Add loading bar with corresponding message
 * Move trigger detection to a new thread in order to not block the application
 * Split View menu into new View and Control menus
 * Block EventManager run() with semaphore
 * UsedProgressView class which displays loading bar and message when loading/saving models to files
 * Mutithreaded Averaging + Trigger detect calculations
 * Co-Registration plugin
 * AnalyzeDataModel, DataManagerControlView are now BidsViewModel, BidsView
 * Reworked hierarchy of stored items in model to fit bids. Items now hold their own place in structure
 * Added session, and data type folders to view
 * Move sessions between subjects, move data between sessions
 * Loading .eve files
 * Loading -ave.fif files
 * Add item for annotation model in bidsviewmodel
 * reworked instantiation and handling of annotationmodel (no longer crated in fiffrawviewmodel)

MNE Scan
 * Improved impedance measurement in Eegosports plugin
 * Use a toolbar for each view using the same control mechanism actions (channel slection view, hide/show bad channels)
 * Rename Filter plugin to Noise plugin

### API Libraries

All
 * Remove some no longer needed compiler flags in library .pro files

Disp
 * Added ProgressView class which displays loading bar and message
 * Make 2D layout scene in ChannelSelectionView fill free GUI space
 * Make ScalingView hide scaling sliders of chnnel types which are not present in the actual data

Disp3D
 * Add support for vertex picking

Fiff
 * Add write functionality to FiffCoordTrans
 * Add convenience function to FiffInfoBase to return present channel types as QStringList

Utils
 * Remove dll export/import for circular buffer typenames

### Tests

 * Add read write read test for FiffCoordTrans
 * Add MNEProjectToSurface test

### Continuous Integration

 * Bump CI Qt version to Qt 5.15.1

### Documentation

 * Add guide on how to install the MNE Sample Data Set
 * Added page on the event system
 * Added page on creating data models

### Other

 * Fix rpath on macos builds
 * Fix Qt3D renderer plugin deployment on macos
 * Rename noOpenGL flag to noQOpenGLWidget flag

### Authors

People who contributed to this release (preceded by number of commits):

(141) Gabriel Motta,
(119) Ruben Dörfel,
(27) Lorenz Esch,
(2) Johannes Vorwerk,
(1) Andrey Parfenov

## Version 0.1.6 - 2020/08/21

### Applications

MNE Analyze
 * Add cmd line argument support for file loading
 * Add channel selection plugin
 * Remove channel selection in averaging plugin
 * Add scaling plugin
 * Add example plugin

MNE Scan
 * Remove no longer needed recording icons and move them to the write to file plugin instead
 * Remove no longer needed readme.txt in some of the plugins' resources

### API Libraries

Disp
 * Move *Apply to View* from channel selection to library layer to be used by both channel selection and scaling

RtProcessing
 * Add performIcp() namespace function to register a point cloud with a surface
 * Add fitMatchedPoints() namespace function to register two point clouds of same size using ICP and quaternions
 * Add discard3DPointOutliers() namespace function to discard outliers from digitizer set compared to a given 3D surface

Utils
 * Rename IOBUFFER to UTILSLIB namespace
 * Remove circularbuffer.cpp file since it was empty

### Documentation
 * Add documentation for MNE Analyze channel selection plugin
 * Add documentation for Disp3D library
 * Add documentation for connectivity library
 * Add documentation for MNE Analyze scaling plugin
 * Add documentation for plugin creation in MNE Analyze
 * Restructure static and dynamic build guides
 * Rename .md page files to better indicate what subject they belong to
 * Rename documentation pages for MNE Scan, MNE Analyze and MNE Anonymize
 * Rename MNE Lib to library API
 * Improve DoxyGen docu by making use of namespaces for MNE-C types
 * Improve documentation for MNE Scan eeg amplifiers and fix some typos
 * Hide Learn pages for MNE Dipole Fit and MNE Forward Solution for now (until they have been completley refactored)

### Authors

People who contributed to this release (preceded by number of commits):

(47) Gabriel Motta,
(47) Ruben Dörfel,
(16) Lorenz Esch

## Version 0.1.5 - 2020/07/30

### Applications

MNE Analyze
 * Refactored DataManagerView to DataManagerControlView.
 * Added a break case to switch statement in datamanager.cpp.
 * Added triggerdetectview to Event plugin.
 * Added functionality to Event plugin to create event groups by type and sort detected events.
 * Tweaked/added get/set functions dealing with event groups.
 * Added functionality to Averaging plugin to read events from user-selected event group.
 * Added jump to event to right click menu.
 * Changed key event to keyReleaseEvent in Event plugin to avoid event being accepted elsewhere.
 * FiffRawView now inherits from AbstractView.
 * Window size and number of channels preserved while resizing window.
 * Fix data plotting when scrolling to the left (raw and filtering). Fixed a problem where the current fiff curser of the beginning data block was substracted by the filter delay even if filtering was disabled.
 * Remove filterAllData function and replace usage by reloadAllData, which leads to an performance improvement.
 * Rename updateDisplayData to reloadAllData.
 * Comment out some qInfo outputs.
 * Rename getWindowSizeBlocks to getTotalBlockCount in FiffRawViewModel.
 * Disable downsampling when plotting since it introduced aliasing effects. Performance is not affected by this change.

### API Libraries

Disp
 * Added offline mode functionality to triggerdetectview.
 * Added event group selection to offline mode in averagingsettingsview.

RtProcessing
 * Remove data copies when filtering in order to speed up filtering.

### Documentation

 * Update MNE Analyze docu.
 * Update MNE Scan HPI and forward plugin docu.

### Authors

People who contributed to this release (preceded by number of commits):

(34) Gabriel Motta,
(13) Ruben Dörfel,
(11) Lorenz Esch,
(2) Wayne Mead

## Version 0.1.4 - 2020/07/07

### Applications

MNE Analyze
* Add filter support. The user can now select/design a filter. If activated the filter is applied to the data as the user scrolls through the file. When activated the filtered data is also written to file and applied when computing an average.
* Add dark mode support to WASM version.
* Corrected saving/loading views inbetween sessions.
* Adjusted minimum allowed window size.
* Add support for computing averages/evoked responses.
* Closing main window now calls destructors for views in dockwidgets.
* Add support to delete loaded files from the data manager.
* Speed up data browsing by decreasing the pre loading buffer size to two blocks.
* Fix vertically overlapping signal plotting.
* Move AnalyzeDataModel to anshared/model folder.
* Annotations have been renamed to Events.
* Added Event Group functionality and struct.
* Event Groups can be renamed or have their color changed through right click context menu.
* Events can now be deleted from selecting any of the columns, double click to edit columns.
* Fixed bug where check boxes changing annotation model state were not updating the view.

MNE Scan
* Update inverse operator if new forward solution was calculated.
* Update brainflow submodule for the brainflowboard plugin.

Examples
* Add a new example for averaging.
* Change ex_read_epochs to only read epochs without averaging afterwards.

MNE Anonymize
* Add GUI mode.
* Add WASM version.
* Improve internal memory handling.

### API Libraries

Utils
* Move filter methods/classes to the RtProcessing library.
* Move DetectTrigger class to the RtProcessing library.
* Refactor baseline correction input from QPair<QVariant,QVariant> to QPair<float,float>.

RtProcessing
* Refactor header guards of all RtProcessing classes.
* Add function to set updated forward solution in RtInv.
* Rename RtFilter to Filter and FilterData to FilterKernel.
* Improve automatic slicing of data when filtering.
* Separate continous and one time overlap add filtering methods. Make some functions global RTPROCESSINGLIB namespace functions and was therefore removed.
* Remove processing of multiple filters at once. This feature was never really implemented.
* Rename RtAve to RtAveraging.
* Add new averaging functions.
* Implement convenience function in DetectTrigger to transform between detect trigger QMaps and MNE event matrices.
* Move detect trigger functions to global RTPROCESSINGLIB namespace.

Disp
* Refactor plotting of the filter's frequency response in the FilterPlotScene class.
* Fix saving/loading from FilterDesignView, FilterSettingsView and FiffRawView.
* Changed signal view control widget scaling to allow for window sizes.
* Remove filtering from EvokedSetModel in disp library. Filtering on short data lengths such as most epochs is difficult because of the edge effects. Filtering for epochs/evoked responses should happen before with appropriate filter lengths which is now supported by the RTPROCESSINGLIB::computeFilteredAverage() function.
* Improve the ScalingView with new default scaling values and convenience functions to retrieve the scale value for a given scale map and channel kind/unit. Make use of the new functions throughout MNE-CPP libraries and applications.

### Documentation

* Add averaging plugin documentation page.
* Update MNE Anonymize documentation page.

### Other

* Remove minimal version flag and corresponding CI build. This mode was needed to be able to build on the older Neuromag systems. Since we switched to the fieldtrip buffer, this mode is no longer needed.

### Authors

People who contributed to this release (preceded by number of commits):

(156) Juan Garcia-Prieto,
(76) Gabriel Motta,
(60) Lorenz Esch,
(6) Ruben Dörfel,
(2) Wayne Mead,
(2) Andrey Parfenov

## Version 0.1.3 - 2020/06/05

### Applications

MNE Analyze
* Add new AnalyzeDataModel, which allows subject based data organization.
* Improve data loading from QByteArray in AnalyzeData.
* Log Window now has location preserved between sessions.
* Files once again get selected and displayed automatically when first loaded.
* Disconnect everything from old model before loading new one. This solves performance issues when loading multiple files.
* Signal Viewer and Annotation settings scale depending on available size.
* Add different GUI modes (scientific and clinical) to appearance menu.
* Add dark and light mode to appearance menu.
* Include skeleton of new Filtering plugin in MNE Analyze. Please note that the actual filtering is still WIP and will follow in a future version.

MNE Scan
* Update Brainflow plugin.
* Separated real-time source localization and forward calculation into two plugins.
* Recalculate forward solution if large head movement occurred.
* Fix thread safety in real-time source localization plugin.
* Save plugin pipeline in MNE Scan more often and everytime we start the pipeline.
* Beautify HPI plugin control settings view.
* Add different GUI modes (scientific and clinical) to appearance menu.
* Add dark and light mode to appearance menu.

### API Libraries

Disp
* Add new view for controlling the forward calculation.
* Make plugin tab bar show vertically in the Quick Control View.
* Create an abstract interface AbstractView for all Disp library viewers to enforce handling different GUI modes and the saving/loading of GUI settings.
* Refactor saving/settings of Disp viewers.
* Improve FilterSettingsView and add different GUI elements based on the currently set GUI mode. For example, scientific mode will enable advanced filter design tools, whereas in clinical mode only the lower and upper cut off frequencies can be defined.
* Move CovarianceSettingsView from MNE Scan's Covariance plugin to Disp/viewers.

Disp3D
* Add temporary fix on Windows for the Disp3D library and Qt 5.15.0 where the renderers plugin is deployed manually. This will be reverted once Qt 5.15.1 is released.

### Authors

People who contributed to this release (preceded by number of commits):

(64) Ruben Dörfel,
(51) Lorenz Esch,
(16) Gabriel Motta,
(2) Andrey Parfenov,
(1) Juan Garcia-Prieto

## Version 0.1.2 - 2020/05/21

### Applications

MNE Analyze:
* Fix application icon on Linux.
* Add timing labels below the signal viewer.
* Fix deployment of internal MNE Analyze libraries on Windows.
* Change list data model to child/parent item data model in AnalyzeData.
* Rename AnnotationView to AnnotationSettingsView.
* Update splashcreen to show full application name.
* Refactor RawDataViewer plugin. The controls are no longer destroyed when a different file is selected. This led to some visible glitches when switching between files.
* Do not allow  floating or movable dock widgets in the WASM version. The QDockWidget behavior is a bit buggy in the current Qt WASM versions.
* Use QOpenGLWidget instead of QOGLWidget. The latter is marked as deprecated.
* Remember dock states and sizes inbetween MNE Analyze sessions.

MNE Scan:
* Fix application icon on Linux.
* Fix deployment of internal MNE Scan libraries on Windows.
* Fix bug when receiving evoked data in source localization plugin.
* Update splashcreen to show full application name.

### API libraries

Disp:
* Remember dock states and sizes inbetween sessions in the MultiView.

Inverse:
* Fix versioning bug.

### Documentation

* Minor improvements and typo fixes.

### Authors

People who contributed to this release (preceded by number of commits):

(42) Lorenz Esch,
(9) Gabriel Motta,
(5) Juan Garcia-Prieto,
(1) Ruben Dörfel

## Version 0.1.1 - 2020/05/14

### Applications

MNE Analyze:
* Fix bug during deployment of dynamically linked MNE Analyze version on macOS.
* Renamed MNE Analyze extensions to plugins.
* Fix issue with display width, now displays only full seconds as selected.
* Jump viewer to selected annotation with 'J' key.
* Removed seemingly unused timer debug outputs.
* Documentations and variable name changes for readability.
* Add plugin control views to menu bar.
* Clean up command line output.
* Add time information on the y-axis.

MNE Scan:
* Fix problems with Source Localization and Connectivity plugins.
* Fix problems where the QuickControlView was not populated with plugin control GUI widgets correctly.

MNE Anonymize:
* Overall improvements and bug fixes to MNE Anonymize.

### API libraries

All:
* Rename libraries and fix versioning.

Fiff:
* Fix bug when reading gantry_angle from Fiff file.

### Tools
* Fix template class in Qt Creator wizard for MNE-CPP classes.
* Update test and example Qt Creator wizards.

### Continuous Integration
* Only branch off when a minor or major version bump occurred.
* Remove folders which we do not want to ship from dynamic builds.

### Documentation
* Updated information on continuous integration.
* Improved build from source guide.
* Updated guide on streaming pre-recorded data in MNE Scan.

### Authors

People who contributed to this release (preceded by number of commits):

(115) Juan Garcia-Prieto,
(43) Lorenz Esch,
(7) Gabriel Motta,
(2) Ruben Dörfel,

## Version 0.1.0 - 2020/05/04

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
