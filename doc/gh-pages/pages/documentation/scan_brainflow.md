---
title: BrainFlow
parent: MNE Scan
grand_parent: Documentation
nav_order: 10
---
# BrainFlow

The BrainFlow plugin adds data acquisition for several EEG amplifiers to MNE Scan. For more information about the BrainFlow project please see:

* [BrainFlow Docs](https://brainflow.readthedocs.io/en/stable/){:target="_blank" rel="noopener"}
* [BrainFlow Repo](https://github.com/Andrey1994/brainflow){:target="_blank" rel="noopener"}

## Compilation of the BrainFlow Submodule

Make sure that you have brainflow git submodule by typing:

```
git submodule update --init applications/mne_scan/plugins/brainflowboard/brainflow
```

Build it as a regular Cmake project. For MSVC you need to ensure that you use exactly the same Cmake generator as for MNE-CPP. Also, you need to specify `MSVC_RUNTIME` as dynamic (default is static) and set the `-DCMAKE_INSTALL_PREFIX=..\installed` flag. For compilation with MSVC 2015 on a 64bit system do:

```
cd mne-cpp\applications\mne_scan\plugins\brainflowboard\brainflow\
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" -DMSVC_RUNTIME=dynamic -DCMAKE_SYSTEM_VERSION=8.1 -DCMAKE_INSTALL_PREFIX="..\\installed" ..
cmake --build . --target install --config Release
```

For a MSVC 2017 build you need to use `Visual Studio 15 2017 Win64` instead.

## Compilation of the BrainFlowBoard Plugin in MNE Scan

* After the steps above make sure that you use the `MNECPP_CONFIG` flag `withBrainFlow`. You can also set the flag manually in the [mne-cpp.pri file](https://github.com/mne-tools/mne-cpp/blob/main/mne-cpp.pri#L135){:target="_blank" rel="noopener"}.
* Build MNE Scan.
* BrainFlow has several dynamic libraries and a JSON file which must be in your search path before you run MNE Scan. You need to copypaste all dynamic libraries and the brainflow_boards.json file to your executable folder `mne-cpp\bin` from `mne-cpp\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib`.

## BrainFlowBoard Plugin GUI

* You need to provide all inputs required for the selected board and click `Submit Params and Prepare Session` button. For information about inputs in BraiFlowBoard plugin widget use this [table](https://brainflow.readthedocs.io/en/stable/SupportedBoards.html).
* You can now start data streaming using the play button.
* If you need to change board or other parameters click `Release Session` button and create a new one.
* [Optional] if you need to send a config command to a board, open setting widget and enter a config command.
