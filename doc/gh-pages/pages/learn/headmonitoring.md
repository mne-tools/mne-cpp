---
title: Real-time Head Monitoring
parent: MNE Scan
grand_parent: Learn
nav_order: 3
---
# Real time Head Monitoring

This guide gives introductions on how to enable and use real-time head monitoring during your measurement. Prerequisitory are enabled head position indicator (HPI) coils. These are coils, driven at a specific frequency, that help estimating the current head position. A general explanation about the method can be found in the appendix of this guide. 

## Prerequisories

Before you can vosualize the head movement during your measurement you have to make sure two things:

1. Enable continious HPI (cHPi) during your measurement.
2. Make sure you have digitized the subjects head accordingly and you have access to the data. Be aware of the order you digitize the subjects hpi coils. Following picture shows in wich order MNE-CPP assignes the head positions. Try to stay consistend.

## Setup
The HpiView is a functionality that can be added to all Sensor Plugins like `FiffSimulator` or `FieldTripBuffer`. Following steps will show you the necessary steps to setup the real time Head Monitoring. To visualize a subjects head movement, you need to setup a datastream first. There are following usecases:

1. Setup Datastream
    * Streaming Simulated Data
    
    To visualize the head position in prerecordet data, follow the steps described [here](/prerecordeddata.md)

    * Streaming data from a MEG device in real-time

    To connect the VectorView MEG-Device to MNE-Scan you should use the [FieldTripBuffer](../development/ftbufferplugin.md).

2. Open the HPI View

    The `HPI` Icon appears after you clicked on your Sensor Plugin. To Open the HPI View click on this Icon.

    ![](../../images/hpi_icon.png)

## The HPI View

The HPI-View panel can be divided in two sections, namely the monitoring section and the control section. The monitoring sections shows an average head model that is aligned and scaled to tracked landmarks like LPA, RPA and Nasion. 

picture average head aligned with fiducials and hpi coils

The control panel is on the right side and can also be divided into several sections. How to use them and what you can controle with them is shown below.    

### Load Digitizers

Picture

* Click the button `Load Digitizers` and navigate to the subjects digitzed data wich is stored in `.fif` format.  
* The display will how you how many digitizers of each kind you have loaded. 

### HPI-Fitting

Picture

1. Choose if you want to use:
    * Signa Space Projection
    * Compensators

2. Enter the frequencies each coil is driven with. The labeling 1,2,3,4 also refering to the position mentioned in the beginning of this guide.

3. Do an intitial hpi fit or enable continious HPi fitting. 

4. The Error in mm is shown for each coil and in average. The error is namely the distance between the calculated hpi position and the digitized hpi position. 

5. Choose a threshold which defines until which error the calculated hpi-fit should be applied. 

6. The calculated device to head transformation matrix is also shown.

### 3D-Control

Picture

Here you can choose what elemnts you want to visualize in the monitoring section. These elements are following:

 * Device: VectorView or BabyMEG
 * Head
 
In `Head` you can choose if you want to see the head visualiztion and tracked digitizers. 




