---
title: Real-time Head Monitoring
parent: MNE Scan
grand_parent: Learn
nav_order: 3
---
# Real time Head Monitoring

This guide gives introductions on how to enable and use real-time head monitoring during your measurement. Prerequisitory are enabled head position indicator (HPI) coils. These are coils, driven at a specific frequency, that help estimating the current head position. 

## Prerequisite

Before you can visualize the head movement during your measurement, you have to ensure two things:

1. Enable continious HPI (cHPi) during your measurement.
2. Make sure you have digitized the subjects head accordingly and you have access to the data. Be aware of the order you digitize the subjects hpi coils. Following picture shows in wich order MNE-CPP assignes the head positions. Try to stay consistend.

![](../../images/mne_scan_hpi_pos.png)

## Setup

The HpiView is a functionality that can be added to all Sensor Plugins like `FiffSimulator` or `FieldTripBuffer`. Following steps will show you the necessary steps to setup the real time head monitoring. 

1. Setup Datastream
    * Streaming Simulated Data via [FiffSimulator](/prerecordeddata.md).

    * Streaming data from a MEG device in real-time, i.e. with the [FieldTripBuffer-Plugin](../development/ftbufferplugin.md).

2. Open the HPI View

    The `HPI` Icon appears after you clicked on your Sensor Plugin. To Open the HPI View click on this Icon.

    ![](../../images/mne_scan_hpi_icon.png)

## The HPI View

The HPI-View panel can be divided in two sections, namely the monitoring section and the control section. The monitoring sections shows an average head model that is aligned and scaled to tracked landmarks like LPA, RPA, Nasion and hpi coils. 

![](../../images/mne_scan_hpi_view.png)

The control panel is on the right side and can also be divided into several sections. How to use them and what you can controle with them is shown below.    

### Load Digitizers

![](../../images/mne_scan_hpi_load.png)

* Click the button `Load Digitizers` and navigate to the subjects digitzed data wich is stored in `.fif` format.  
* The display will show how many digitizers of each kind were have loaded. 

### HPI-Fitting

![](../../images/mne_scan_hpi_fit.png)

1. Choose if you want to use:
    * Signal Space Projection `SSP`
    * `Compensators`

2. Enter the hpi coil frequencies. The labeling 1,2,3,4 referes to the positions mentioned in the beginning of this guide.

3. Do an intitial hpi fit or enable continious HPi fitting. 

4. The Error in mm is shown for each coil and in average. The error is namely the distance between the calculated hpi position and the digitized hpi position. 

5. Choose a threshold which defines until which error the calculated hpi-fit should be applied. 

6. The calculated device to head transformation matrix is also shown.

### 3D-Control

![](../../images/mne_scan_hpi_control.png)

Here you can choose what elemnts you want to visualize in the monitoring section. These elements are following:

 * Device: `VectorView` or `BabyMEG`
 * Head: `Average` and `Tracked`
 
In `Head` you can choose if you want to see the scled average head visualiztion and tracked digitizers. In `Device` if and which sensor layout you want to visualize.




