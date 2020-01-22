# BrainFlow Plugin

This plugin adds BrainFlow data acquisition SDK to MNE-SCAN app.

**Links:**

* [BrainFlow Docs](https://brainflow.readthedocs.io/en/stable/)
* [BrainFlow Repo](https://github.com/Andrey1994/brainflow)

**Compilation of BrainFlow submodule:**

* Make sure that you have brainflow git submodule in this folder
* Build it as a regular Cmake project but for MSVC you need to ensure that you use exactly the same Cmake Generator as for MNE-CPP, also you need to specify MSVC_RUNTIME dynamic(default is static). And specify -DCMAKE_INSTALL_PREFIX=..\installed

```
cd brainflow
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" -DMSVC_RUNTIME=dynamic -DCMAKE_SYSTEM_VERSION=8.1 -DCMAKE_INSTALL_PREFIX=..\\installed ..
cmake --build . --target install --config Release
cd ..
```

**BrainFlowBoard Plugin Setup:**

* After steps above make sure that you have brainflowboard uncommented in plugins.pro
* Build MNE-SCAN application
* BrainFlow has several dynamic libraries and JSON file which must be in your search path before you run BrainFlow based add, so you need to copypaste BoardController.dll DataHandler.dll and brainflow_boards.json to your executable folder from brainflow\installed\lib
* For information about inputs in BraiFlowBoard plugin widget use this [table](https://brainflow.readthedocs.io/en/stable/SupportedBoards.html)
