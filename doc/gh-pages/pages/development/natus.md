---
title: Natus Amplifier
parent: EEG amplifier in MNE Scan
grand_parent: Develop
nav_order: 1
---
# Natus Amplifier

|**Please note:** We are working with the SDK v 8.4 so we are only able to connect to a Quantum system. These are typically used in phase 2 surgery of epilepsy.|

Follow the steps below to connect to a Natus amplifier:

* Select streaming machine. If not present add it to the SelectStreamingMachine Visual studio project and rebuild.
* Start the startDataTransmit.bat .
* Check if it connects and streams data based on the message in the left lower corner.
* Start MNE Scan. Find out number of channels and sampling frequency. Start the Natus plugin.