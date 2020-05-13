---
title: Deployment
parent: Continuous Integration
grand_parent: Develop
nav_order: 2
---

# Deployment

This page explains how MNE-CPP handles builds, exernal as well as internal dependencies, resources, and packaging.

## Build rules



## Dependency solving

Realized in three scripts in mne-cpp.pri and CI workflow. linuxdeployqt is not part of Qt

### Internal dependencies (MNE-CPP libraries) 

Applications, tests, and examples link against MNE-CPP libraries (internal dependencies). Dependencies between MNE-CPP libraries exist as well and can be seen in the [libraries.pro file](https://github.com/mne-tools/mne-cpp/blob/master/libraries/libraries.pro){:target="_blank" rel="noopener"}. The following table describes how we solve for internal dependencies:

| Platform                    | MNE-CPP Libraries                      | MNE-CPP Applications                  | MNE-CPP Examples                 |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
| Windows | Build into `mne-cpp/lib` and copied afterwards to `mne-cpp/bin`. | Build to `mne-cpp/bin`. | Build to `mne-cpp/bin`. |
| Linux |Build into `mne-cpp/lib`. | Build to `mne-cpp/bin/resources`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start applications from navigator. | Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start examples from navigator. |
| MacOS |Build into `mne-cpp/lib`. | Build to `mne-cpp/bin`. Build as .app or .dmg (MNE Scan, MNE Analyze). | Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `DYLD_LIBRARY_PATH`. |

### External dependencies (Qt, Eigen, and system)

As of right now MNE-CPP depends on [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}. Eigen, as a lightweight tempalte library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/master/include/3rdParty/eigen3){:target="_blank" rel="noopener"}. Here, no dependency solving is needed, since we build Eigen from source on the corresponding built setup.

| Platform                    | MNE-CPP Libraries                      | MNE-CPP Applications                  | MNE-CPP Examples                 |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
| Windows |Copied to `mne-cpp/bin`. windeployqt on MNE-CPP libraries which were already copied to `mne-cpp/bin`. Afterwards, Qt libs reside in `mne-cpp/bin` as well. | windeployqt on MNE-CPP all applications. | windeployqt on MNE-CPP all examples. |
| Linux |Reside in `mne-cpp/lib`. |[linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} on MNE Scan. The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. | The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. |
| MacOS |Reside in `mne-cpp/lib`. |Some applications are created as .app or .dmg (MNE Scan, MNE Analyze) via macdeployqt. macdeployqt also copies in Qt and MNE-CPP libraries to the .app and .dmg bundles. | Examples are not build as .app or .dmg bundles. The Qt and MNE-CPP libs are looked up via `DYLD_LIBRARY_PATH`. |

## Resource handling

Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. In case of .app images on MacOS, e.g., MNE Scan, the needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/mne_scan.app/MacOs/resources`. If you need to add new resources, please add them to `mne-cpp/resources` and NOT `mne-cpp/bin/resources`.

## Packaging

| Platform                    | MNE-CPP Libraries                      | MNE-CPP Applications                  | MNE-CPP Examples                 |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
| Windows |Libraries are packaged as well since they reside in the `mne-cpp/bin` folder. |The `mne-cpp/bin` folder including the MNE-CPP libraries is is zipped as a whole and uploaded as a release asset to GitHub. | Examples are packaged for dynamically linked versions only. |
| Linux |Libraries are compressed to a tar.gz file and uploaded as a release asset to GitHub.. |The whole `mne-cpp/bin` and `mne-cpp/lib` folder are compressed into a single .tar.gz file and uploaded as a release asset to GitHub. |Examples are packaged for dynamically linked versions only. |
| MacOS |Libraries are not packaged. They can only be found as part of the .dmg bundles. |MNE Scan and MNE Analyze created as .dmg files and uploaded as a release asset to GitHub. |Examples are packaged for dynamically linked versions only. |
