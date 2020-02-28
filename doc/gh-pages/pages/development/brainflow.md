---
title: BrainFlow
parent: EEG amplifier in MNE Scan
grand_parent: Develop
nav_order: 5
---
# BrainFlow

This plugin adds BrainFlow data acquisition SDK to MNE Scan app.

**Links:**

* [BrainFlow Docs](https://brainflow.readthedocs.io/en/stable/){:target="_blank" rel="noopener"}
* [BrainFlow Repo](https://github.com/Andrey1994/brainflow){:target="_blank" rel="noopener"}

## Compilation of BrainFlow submodule

* Make sure that you have brainflow git submodule by typing

```
git clone --recursive https://github.com/mne-tools/mne-cpp
# if you cloned the repo without recursive flag you will need to run
git submodule update --init
```

* Build it as a regular Cmake project but for MSVC you need to ensure that you use exactly the same Cmake Generator as for MNE-CPP, also you need to specify MSVC_RUNTIME dynamic(default is static). And specify -DCMAKE_INSTALL_PREFIX=..\installed

Example of compilation:
```
cd applications\mne_scan\plugins\brainflowboard\brainflow\
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" -DMSVC_RUNTIME=dynamic -DCMAKE_SYSTEM_VERSION=8.1 -DCMAKE_INSTALL_PREFIX=..\\installed ..
cmake --build . --target install --config Release
cd ..
```

## BrainFlowBoard plugin setup

* After steps above make sure that you have brainflowboard uncommented in plugins.pro
* Build MNE Scan application
* BrainFlow has several dynamic libraries and JSON file which must be in your search path before you run BrainFlow based app, so you need to copypaste BoardController.dll DataHandler.dll and brainflow_boards.json to your executable folder from brainflow\installed\lib

## BrainFlowBoard plugin UI

* You need to provide all inputs required for selected board and click 'Submit Params and Prepare Session' button. For information about inputs in BraiFlowBoard plugin widget use this [table](https://brainflow.readthedocs.io/en/stable/SupportedBoards.html)
* After that you can start data streaming using play button
* If you need to change board or other parameters click 'Release Session' button and create a new one.
* [Optional] if you need to send config to a board, open setting widget and enter a config.
