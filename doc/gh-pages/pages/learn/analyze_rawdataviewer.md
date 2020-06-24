---
title: Raw Data Viewer
has_children: false
parent: MNE Analyze GUI
grand_parent: Learn
nav_order: 3
---
# Signal Viewer

The signal viewer controls the plotting and displaying of the currently selected data file. It is separated into two components: the control widget and the viewer

![](../../images/analyze/mne_an_1.png)

The control widget allows for defining the plotting and display parameters.

![](../../images/analyze/mne_an_rawdataviewer_1.png)

The top set of control sliders define the channel plotting range for each type of channel. It is used for scaling the plotted data. The bottom set of controls is used for controlling channel display parameters, like window size, spacers, number of channels. Signal and background colors can also be adjusted. Screenshots can be taken of the current viewer window.

The viewer portion plots the data and annotations, controlled by the [Annotation Manager](analyze_annotationmanager.md)
