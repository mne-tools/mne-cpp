---
title: LSL
parent: MNE Scan Development
grand_parent: Develop
nav_order: 8
---
# Lab Streaming Layer (LSL)

This plugin adds support for LSL streams to MNE Scan.

**Links:**

* [LSL on Github](https://github.com/sccn/labstreaminglayer){:target="_blank" rel="noopener"}
* [Building the LSL library from source](https://labstreaminglayer.readthedocs.io/dev/lib_dev.html#building-liblsl){:target="_blank" rel="noopener"}

## Compilation of the LSL submodule

* Make sure that you have the LSL git submodule by typing

```
git submodule update --init applications\mne_scan\plugins\lsladapter\liblsl
```

* Build it as a regular Cmake project. For MSVC you need to ensure that you use exactly the same Cmake generator as for MNE-CPP. For example:

```
cd mne-cpp\applications\mne_scan\plugins\lsladapter\liblsl\
mkdir build
cd build
cmake .. -G "Visual Studio 14 2015 Win64"
cmake --build . --config Release --target install
```

## LSL plugin setup

* After the steps above make sure that you use the `MNECPP_CONFIG` flag `withLsl`. You can also set the flag manually in the [mne-cpp.pri file](https://github.com/mne-tools/mne-cpp/blob/6dcf4fecbf4eb983c7925ad63fb743aaa215bb36/mne-cpp.pri#L135).
* Build MNE Scan.
* LSL has a dynamic library which must be in your search path before you run MNE Scan. You need to copy `lsl.dll` from `mne-cpp\applications\mne_scan\plugins\lsladapter\liblsl\install\bin` to your executable folder `mne-cpp\bin`.
* Start MNE Scan