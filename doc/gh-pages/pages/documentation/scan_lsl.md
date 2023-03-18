---
title: LSL
parent: MNE Scan
grand_parent: Documentation
nav_order: 12
---
# Lab Streaming Layer (LSL)

This plugin adds support for LSL data streams to MNE Scan. For more information about the LSL project please see:

* [LSL on Github](https://github.com/sccn/labstreaminglayer){:target="_blank" rel="noopener"}
* [Building the LSL library from source](https://labstreaminglayer.readthedocs.io/dev/lib_dev.html#building-liblsl){:target="_blank" rel="noopener"}

## Compilation of the LSL Submodule

Make sure that you have the LSL git submodule by typing:

```
git submodule update --init applications\mne_scan\plugins\lsladapter\liblsl
```

Build it as a regular Cmake project. For MSVC you need to ensure that you use exactly the same Cmake generator as for MNE-CPP. For compilation with MSVC 2015 on a 64bit system do:

```
cd mne-cpp\applications\mne_scan\plugins\lsladapter\liblsl\
mkdir build
cd build
cmake .. -G "Visual Studio 14 2015 Win64"
cmake --build . --config Release --target install
```

For a MSVC 2017 build you need to use `Visual Studio 15 2017 Win64` instead.

## LSL Plugin Setup

* After the steps above make sure that you use the `MNECPP_CONFIG` flag `withLsl`. You can also set the flag manually in the [mne-cpp.pri file](https://github.com/mne-tools/mne-cpp/blob/main/mne-cpp.pri#L135){:target="_blank" rel="noopener"}.
* Build MNE Scan.
* LSL has a dynamic library which must be in your search path before you run MNE Scan. You need to copy `lsl.dll` from `mne-cpp\applications\mne_scan\plugins\lsladapter\liblsl\install\bin` to your executable folder `mne-cpp\out\Release\apps`.
* Start MNE Scan
