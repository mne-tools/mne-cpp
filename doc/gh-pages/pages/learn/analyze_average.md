---
title: Averaging
has_children: false
parent: MNE Analyze GUI
grand_parent: Learn
nav_order: 5
---
#Averaging

The averaging plugin lets you compute averages based on created annotations. To do this, first create annotations for the points which are to be averaged, like in the figure below:

![](../../images/analyze/mne_an_avg1.png)

Next, using the `Parameters` tab of the Averaging plugin, input your desired settings and hit the `COMPUTE` button.
Prestimulus, poststimulus, and baseline min/max are all relative to the set annotations. `Drop Rejected` will discard data points with artifacts"

![](../../images/analyze/mne_an_avg2.png)

You can control which channels to display using the Channel Selection tool, which lets you select from presets or choose your own selection.

![](../../images/analyze/mne_an_avg3.png)

Make sure to select the correct layout from the bottom left dropdown.
