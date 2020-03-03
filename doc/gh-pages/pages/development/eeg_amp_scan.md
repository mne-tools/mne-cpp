---
title: EEG amplifiers in MNE Scan
parent: MNE Scan Development
grand_parent: Develop
nav_order: 1
---
# EEG amplifiers in MNE Scan

|**Please note:** The information provided on this page might be outdated and will be updated soon.|

## Available amplifier plugins in MNE Scan

Following driver setup tutorials are offered for MNE Scan:

EEGoSports, TMSI Refa, gUSBAmp, Natus and BrainAmp

## Save to a FIFF file

Every acquisition plugin provides the possibility of saving the acquired data stream to a FIFF file. For that purpose two icons can be seen in the status bar of an acquisition plugin:

By clicking the database button, ![](../../images/eeg_amp_scan1.png), all storage information of the FIFF file can be set. In addition, an electrode layout file (.elc) can be deposited to the stream. The settings are saved at once and the window can be closed.


![](../../images/eeg_amp_scan2.jpg "GUI of the FIFF file storage settings")

After the acquisition has been started, the record icon, ![](../../images/eeg_amp_scan3.png), can be clicked which will initialize the recording. After that it will blink continuously in order to signalize its record status. By clicking on it again, the recording will stop and the FIFF file is saved to the desired location with all its settings.

## Structure of an acquisition plugin

The acquisition plugins of MNE Scan have similiar structure and therefore can be explained on the basis of the gUSBamp EEG driver example. The following flow sheet describes the structure of this acquisition plugin.
![](../../images/eeg_amp_scan4.png "Flow sheet of the gUSBamp acquisition plugin")

On the left side the border of MNE Scan can be seen, whereas on the right side the border to the actual device, in this case the gUSBamp amplifier, is depicted. In between, according to their hierarchical order, the three classes are shown as the interface between program and device:

* **gUSBamp**: regulates the communication between MNE Scan and the driver and acts as the main plugin class.
* **gUSBampproducer**: controls the data acquisition and manages the interface between driver and gUSBamp
* **gUSBampdriver**: actual driver which provides data acquisition and controls the communication between the project and the device

However, gUSBampdriver is the actual class, communicating and exchanging data with the device. The gUSBamp and gUSBampproducer classes can be seen as a way, how to integrate the gUSBampdriver class properly into the project.

When establishing a new plugin to the plugin-box like shown in the gUSBAmp example, all three classes are initialized one after another by calling the constructors. During this process, all default parameter are generated. After that, the main thread returns to the program and the driver plugin is waiting for the start command or further changing instructions of the parameter by the GUI.

By starting the acquisition, one class invokes the next. At the end, the "gUSBampdriver" class initializes the device with the new parameters and sets the device status to "run". After that, "gUSBamp" and "gUSBampproducer" class are each starting an internal thread which call repetitively for new data packages from the subordinate class and returning them to the overlying class until the data packages reach the MNE Scan environment. This is achieved with so called ring buffers and leads to a continuous data stream.

When stopping the acquisition, both threads are interrupted by putting the "is_running" parameter to false and the "gUSBampdriver" class puts the device into standby mode.
