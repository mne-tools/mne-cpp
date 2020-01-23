---
title: MNE-SCAN FieldTrip Buffer Plugin
parent: Develop
nav_order: 7
---

# MNE-SCAN FieldTrip Buffer Plugin

The plugin is meant to interface with a FieldTrip Buffer already running at a given address. It does not implement the buffer itself, only a client for the buffer, and currently only a client that reads from the buffer.

It works by connecting to a given address where a FieldTrip buffer is running after having set some data parameters. The plugin will then continuously send requests to receive data from the buffer, and upon receiving it will output that data.

## Building and Running:

These steps assume you already have MNE-Scan. If not please start [here](https://mne-cpp.github.io/pages/started/started.html).

1. Download FieldTrip Buffer source code:

   You can get the source code either from their [website](http://www.fieldtriptoolbox.org/download/) or their [github page](https://github.com/fieldtrip/fieldtrip). The website download gives you the option to download only the buffer source code. The github repository has the buffer source code located under `realtime/src/buffer`.

   The plugin was made using buffer source code dated to 2019-12-13, and as the fieldtrip c++ wrappers are still under development, different version may not be compatible.

2. Move the source code to your plugin directory:

   The `src` and `cpp` folders need to be placed in `applications/mne_scan/plugins/ftbuffer/ftsrc`.

3. Include ftbuffer in the build process:

   The lines of code that include the ftbuffer plugin in `applications/mne_scan/libs/scShared/Management/pluginmanager.cpp`, `applications/mne_scan/libs/scShared/scShared.pro`, `applications/mne_scan/mne_scan.pro`, and `applications/mne_scan/plugins/plugins.pro` are commented out by default.

4. Build:

   Build MNE-Scan again. You may have to restart QtCreator if it is not finding the newly added files automatically. Once that is done, you should have a new plugin in `bin/mne_scan_plugins`.

5. Run:

   To use it, run MNE-Scan and select the FtBuffer plugin from the toolbar on the left. Set the data parameters and the buffer address and click "Connect". The data parameters need to be set before connecting or else the plugin will not work correctly.

This gets you as far as you need to interface with a buffer that is already running. The fieldtrip source code provides [implementations for various platforms](http://www.fieldtriptoolbox.org/development/realtime/implementation/) under `realtime/src/acquisition`.
To run the plugin using the FieldTrip Buffer example executables, continue with the following steps:

6. Download the FtBuffer examples:

   The examples are also available through their [website](http://www.fieldtriptoolbox.org/download/) or their [github page](https://github.com/fieldtrip/fieldtrip), under `realtime/bin` in the folder corresponding to your operating system.

7. Run examples:

   In separate command line windows, run `buffer` and `sine2ft`.

8. Set up plugin:

   Make sure the data generation parameters in your plugin match those of "sine2ft", and the address matches that of "buffer". Once they do, click connect.

9. Send and receive data:

   In MNE-Scan, click the green start button on the top left to start the plugin. In "sine2ft", click the start button to start generating data. If everything was set up correctly, you should see the data in MNE-Scan.

## Classes:

### FtBuffer

This class serves as an interface between all the others, and mne scan. It is, in essence the plugin itself. It handles the outputting of the acquired data, as well as all the inherited features of ISensor, to be run as a plugin. It has, as a member, an instance of FtBuffProducer.

### FtBuffProducer

This is a worker class to do the buffer requests in a new thread. The instance of this class gets moved to a new thread and communicates back to the main plugin thread through slots and signals. It has, as a member, an instance of FtBuffClient.

### FtBuffClient

This extends the provided FtBuffer c++ wrapped c code, as well as incorporating elements from the ftbuffer example programs. This class handles the direct communication with the buffer by establishing a connection and sending requests and receiving data, as well and formatting that data to then be sent out to FtBuff Producer, which in turn sends it to FtBuffer.
