---
title: Data Manager
has_children: false
parent: MNE Analyze
grand_parent: Learn
nav_order: 2
---
# Data Manager

The Data Manager keeps track of and organizes files data in MNE Analyze.

![](../../images/analyze/mne_an_datamanager_1.png)

Any data loaded from files, or generated within the application will appear here. The organization follows [BIDS](https://bids.neuroimaging.io/) formatting, separating data into subjects and sessions. Any data derived from other data will appear as sub item, like events or averages that correspond to a file.

![](../../images/analyze/mne_an_datamanager_3.png)

Items can be removed by right clicking and selecting `Remove`, and can be moved into other available sessions or subjects.

You can select between .fif files and events to pick which one to display in the [Data Viewer](analyze_rawdataviewer.md), and averages to pick which will be displayed in [Averaging](analyze_average.md).
