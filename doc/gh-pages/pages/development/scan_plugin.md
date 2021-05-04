---
title: Creating a new plugin
parent: MNE Scan
grand_parent: Development
nav_order: 2
---
# Creating a New Plugin

|**Please note:** The information provided on this page might be outdated and will be updated soon.|

A detailed tutorial on how you can create your own MNE Scan plugin directly using the QtCreator can be found here in .pdf format.

<embed src="http://lorenzesch.de/mne-cpp/Creating_a_Plugin.pdf" width="800px" height="500px" type="application/pdf" />

Another good starting point is the DummyToolbox plugin, which you can find under `\mne-cpp\applications\mne_scan\plugins\dummytoolbox` or [here](https://github.com/mne-tools/mne-cpp/tree/main/applications/mne_scan/plugins/dummytoolbox){:target="_blank" rel="noopener"}. Please note that the DummyToolbox plugin is commented out by default in the [plugins.pro](https://github.com/mne-tools/mne-cpp/blob/main/applications/mne_scan/plugins/plugins.pro){:target="_blank" rel="noopener"} file. For example, you could make a copy of the DummyToolbox, rename it and make changes.
