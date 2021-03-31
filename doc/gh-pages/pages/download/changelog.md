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

