---
title: Real-time Head monitoring
parent: MNE Scan
grand_parent: Learn
nav_order: 3
---
# Real time Head monitoring

This guide gives introductions on how to enable and use real-time head monitoring during your measurement. Prerequisitory are enabled head position indicator (HPI) coils. Thse are coils, driven at a specific frequency that help estimating the current head position. A general explanation about the method can be found in the appendix of this guide. 

## Setup
The HpiView is a functionality that can be added to all Sensor Plugins like `FiffSimulator` or `FieldTripBuffer`. Following steps will show you the necessary steps to setup the real time Head Monitoring. To visualize a subjects head movement, you need to setup a datastream first. There are following usecases:

1. Setup Datastream
* Streaming Simulated Data
    To visualize the head position in prerecordet data, follow the steps described [here](/prerecordeddata.md)

* Streaming data from a MEG device in real-time
    To connect the VectorView MEG-Device to MNE-Scan you should use the [FieldTripBuffer](../development/ftbufferplugin.md).

2. Open the HPI View
    The `HPI` Icon appears after you clicked on your Sensor Plugin. To Open the HPI View click on this Icon.

    ![](../../images/HPI_icon.png)

## The HPI View

 You can access it after you clicked on you sensor plugin and started the datastrem. How to stream pre-recordet data is shown [here](/prerecordeddata.md) or how to connect to a VectorView MEG device is shown [here](../development/ftbufferplugin.md). Following picture shows how to acces the HpiView and how it looks like. 

picture

### Elements

### Load Data

### Enter Frequencies