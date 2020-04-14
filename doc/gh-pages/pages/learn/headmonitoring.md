---
title: Real-time Head Monitoring
parent: MNE Scan
grand_parent: Learn
nav_order: 3
---
# Real-time Head Monitoring

This guide shows how to enable and use real-time head monitoring during your MEG measurement. Currently, this only works with Neuromag/Elekta/MEGIN devices.

## Prerequisites

Before you can visualize the head movement during your measurement, you have to ensure two things:

1. Enable continuous HPI (cHPi) during your measurement.
2. Make sure you have digitized the subjects head accordingly and you have access to the data. 

## Setup

Real-time head monitoring is a functionality that can be added to all Sensor Plugins like `FiffSimulator` or `FieldTripBuffer`. The following steps will show you the necessary steps to set up the real-time head monitoring in MNE Scan: 

1. Setup the data streaming
    * Stream pre-recorded data via [FiffSimulator](/prerecordeddata.md).
    * Stream data from a MEG device connected via the [FieldTripBuffer-Plugin](../development/ftbufferplugin.md).

2. Add the HPI-Plugin to the plugin scene and connect your Sensor-Plugin to the HPI Plugin.
   ![](../../images/plugin.png)

3. Start the measurement

4. Control The HPI Fit
    You can open and control the settings for the hpi fitting via the Quick-Control widget. Open it and follow the steps described in the next section.

    ![](../../images/mne_scan_open_quick.png)

## The 3D View

MNE Scan should now look like following picture. You have the Plugin-Scene on the left, the 3D View in the upper part and the data stream in the lower part. The 3D View shows an average head model that is aligned and scaled to tracked landmarks like LPA, RPA, Nasion and HPI coils. It shows up after the first succesfull hpi fit. 

![](../../images/mne_scan_hpi_3Dview.png)

The control panel can be accessed via the `QUICK CTRL`  button. Here you can change the settings for the hpi fit and the 3D View. 

## The Quick Control Widget
Once opened, you can choose between different control panels. There you get i.e. acces to the HPI-control panel `HPI Fitting` or the 3D-cotrol panel `3D View`. Follow the described steps to setup the hpi fitting and control the apperance in the 3D View.

### HPI Fitting: Load Digitizers

![](../../images/mne_scan_hpi_load.png)

* Click the button `Load Digitizers` and navigate to the subjects digitized data which is stored in `.fif` format.  
* The display will show how many digitizers of each kind are loaded. 

### HPI Fitting: Fitting

![](../../images/mne_scan_hpi_fit.png)

1. Enter the HPI coil frequencies. You can add and remove new Frequencies, but keep in mind to only add used ones. 

2. Choose if you want to Signal Space Projection (`SSP`) or `Compensators`.

3. If you don't know how your coils and frequencies are ordered, do an initial frequency ordering by pressing `Order coil frequencies`.

4. Do an initial HPI fit or enable continuous HPi fitting. Make sure you have started the measuring pipeline via the play button first. After the first succesfull fit the average head shoul appear in the 3D View.

4. The `Fitting errors` in mm are shown for each coil and in form of an average over all coils. The error is calculated as the distance between the estimated HPI coil and the digitized HPI positions.

5. Choose a threshold that defines an acceptable error. 

### 3D View

![](../../images/mne_scan_hpi_control.png)

Here you can choose what elements you want to visualize in the monitoring section. These elements include:

 * Device > `VectorView` or `BabyMEG` features the different helmet surfaces.
 * Head > `Average`, `Tracked` and `Fitted` features the averaged head surface, digitized and aligned landmarks as well as the estimated HPI coil locations, respectivley.