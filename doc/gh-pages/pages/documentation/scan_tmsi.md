---
title: TMSI
parent: MNE Scan
grand_parent: Documentation
nav_order: 13
---
# TMSI

The TMSI plugin is only available on the Windows operating system; the TMSI SDK is only available for that platform. Currently the TMSI plugin is not available in the compiled binary releases of MNE-CPP. To get the plugin, it is necessary to build mne_scan from source while having the tmsi drivers installed (you need to make sure that the `TMSiSDK.dll` is present in `C:/Windows/System32/TMSiSDK.dll`.). Follow the instruction in the [build guide](../development/buildguide_cmake.md). If building with the command line script, add `tmsi` after the call, like so:

```
./tools/build_project.bat tmsi
```

If building in QtCreator or calling CMake manually, add `-DWITH_TMSI=ON` to your CMake step.

