---
title: FieldTrip Buffer
parent: MNE Scan Development
grand_parent: Develop
nav_order: 7
---

# FieldTrip Buffer

The plugin interface with a FieldTrip Buffer running at a given address. It does not implement the buffer itself, only a client. It is non-invasive, and does not even have access to the MEG device directly, only the data pushed to the buffer.

## Running with Neuromag/Elekta/MEGIN

1. Download FieldTrip `neuromag2ft`

   Download the fieldtrip source code on your acquisition computer, either from their [website](http://www.fieldtriptoolbox.org/download/){:target="_blank" rel="noopener"} or their [GitHub page](https://github.com/fieldtrip/fieldtrip){:target="_blank" rel="noopener"}. We are interested in the executable `neuromag2ft`, which will serve as both our buffer host and interface to push data to the buffer.

2. Get `neuromag2ft` set up

   We will need to be able to run `neuromag2ft` on the acquisition computer for the Neuromag. Depending on your system, you might have to build it locally. You can do so by running the makefile in `/realtime/src/acquisition/neuromag/` with the command `make`. If in doubt, follow the [documentation](http://www.fieldtriptoolbox.org/development/realtime/neuromag/){:target="_blank" rel="noopener"} on the fieldtrip buffer website.

3. Run `neuromag2ft`

   Run `neuromag2ft` on the aquisition computer. `neuromag2ft` can be run with different parameters, but for this example we will be running it with all default settings. For more options, run it with the `--help` flag. Run the executable in `/realtime/src/acquisition/neuromag/bin/<YOUR_OS>`. This should both create a buffer and start an interface with the neuromag. This buffer will, by default, be hosted on port `1972`.

4. Start data collection

   Start collecting data and ensure it is being sent into the buffer through `neuromag2ft`. It is important that `neuromag2ft` already be running before data collection starts, otherwise `neuromag2ft` will not work.

5. Set up plugin in MNE Scan

   Run MNE Scan and select the `FtBuffer` plugin from the toolbar on the left. Set the buffer address, which will be the IP address of your aquisition computer, and port, which by default is `1972`. Click the `Set` button. This sets the `Address` and `Port` fields, and attempts to acquire a sample fif file created by `neuromag2ft`.

6. Receive data

   In MNE Scan, click the green start button on the top left to start the plugin.
