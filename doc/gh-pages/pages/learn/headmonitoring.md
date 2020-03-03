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
2. Make sure you have digitized the subjects head accordingly and you have access to the data. Be aware of the order you digitize the subject's HPI coils. The following picture shows in wich order MNE-CPP assigns the head positions. Try to stay consistent.

![](../../images/mne_scan_hpi_pos.png)

## Setup

The HpiView is a functionality that can be added to all Sensor Plugins like `FiffSimulator` or `FieldTripBuffer`. The following steps will show you the necessary steps to set up the real-time head monitoring in MNE Scan: 

1. Setup the data streaming
    * Stream pre-recorded data via [FiffSimulator](/prerecordeddata.md).
    * Stream data from a MEG device connected via the [FieldTripBuffer-Plugin](../development/ftbufferplugin.md).

2. Open the HPI View

    The `HPI` Icon appears after you clicked on the Sensor Plugin. To Open the HPI View click on this Icon.

    ![](../../images/mne_scan_hpi_icon.png)

## The HPI View

The HPI-View panel can be divided into two sections, namely the monitoring section and the control section. The monitoring section shows an average head model that is aligned and scaled to tracked landmarks like LPA, RPA, Nasion and HPI coils. 

![](../../images/mne_scan_hpi_view.png)

The control panel can be found on the right side and is divided into several sections. How to use them and what you can control is exaplained in the fwollowing.    

### Load Digitizers

![](../../images/mne_scan_hpi_load.png)

* Click the button `Load Digitizers` and navigate to the subjects digitized data which is stored in `.fif` format.  
* The display will show how many digitizers of each kind are loaded. 

### HPI-Fitting

![](../../images/mne_scan_hpi_fit.png)

1. Choose if you want to Signal Space Projection (`SSP`) or `Compensators`.

2. Enter the HPI coil frequencies. The labeling 1,2,3,4 refers to the positions mentioned at the beginning of this guide.

3. Do an initial HPI fit or enable continuous HPi fitting. Make sure you have started the measuring pipeline via the play button first.

4. The `Fitting errors` in mm are shown for each coil and in form of an average over all coils. The error is calculated as the distance between the estimated HPI coil and the digitized HPI positions.

5. Choose a threshold that defines an acceptable error. 

6. For convenience the last device to head transformation matrix is shown.

### 3D-Control

![](../../images/mne_scan_hpi_control.png)

Here you can choose what elements you want to visualize in the monitoring section. These elements include:

 * Device > `VectorView` or `BabyMEG` features the different helmet surfaces.
 * Head > `Average`, `Tracked` and `Fitted` features the averaged head surface, digitized and aligned landmarks as well as the estimated HPI coil locations, respectivley.