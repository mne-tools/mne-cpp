---
title: Build/Run & Deployment & Packaging/Distribution
parent: Develop
nav_order: 2
---
# Build/Run & Deployment & Packaging/Distribution

## Build/Run
Compiling MNE-CPP and running applications.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Build into mne-cpp/lib and copied afterwards to mne-cpp/bin. Managed in libraries .pro files. | Build to mne-cpp/bin. Either Qt's lib folder needs to be added to global variables to start applications from explorer or application needs to be started from inside QtCreator.|Build to mne-cpp/bin. Either Qt's lib folder needs to be added to global variables to start examples from explorer or examples needs to be started from inside QtCreator. |
|Linux |Build into mne-cpp/lib. Managed in libraries .pro files. |Build to mne-cpp/bin. Qt's and MNE-CPP's lib folder need to be added to LD_LIBRARY_PATH to start applications from navigator. |Build to mne-cpp/bin. Qt's and MNE-CPP's lib folder need to be added to LD_LIBRARY_PATH to start examples from navigator. |
|MacOS |Build into mne-cpp/lib. Managed in libraries .pro files. |Build to mne-cpp/bin. Build as .app or .dmg (mne_scan, mne_analyze and mne_browse). |Build to mne-cpp/bin. Qt's and MNE-CPP's lib folder need to be added to LD_LIBRARY_PATH. |

## Deployment
Handling dependencies.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Copied to mne-cpp/bin. Managed in libraries .pro files. windeployqt on MNE-CPP libraries which were already copied to mne-cpp/bin. Afterwards, Qt libs reside in mne-cpp/bin as well.  |windeployqt on MNE-CPP applications. Needed resources are copied from mne-cpp/resources to mne-cpp/bin. Managed in .pro files.  |windeployqt on MNE-CPP examples. Managed in .pro files.  |
|Linux |Reside in mne-cpp/lib. Managed in .pro files. |No real individual deployment so far. The Qt and MNE-CPP libs are looked up via LD_LIBRARY_PATH. Needed resources are copied from mne-cpp/resources to mne-cpp/bin. RPATH is set to point to mne-cpp/lib folder. Managed in .pro files.  |No real individual deployment so far. The Qt and MNE-CPP libs are looked up via LD_LIBRARY_PATH. Needed resources are copied from mne-cpp/resources to mne-cpp/bin. RPATH is set to point to mne-cpp/lib folder. Managed in .pro files.  |
|MacOS |Reside in mne-cpp/lib. Managed in .pro files. |Some applications are created as .app or .dmg (mne_scan, mne_analyze and mne_browse). macdeployqt also copies in Qt and MNE-CPP libraries to the .app and .dmg bundles. Needed resources are copied from mne-cpp/resources to .app folders. Managed in .pro files. |No real individual deployment so far. Examples are not build as .app or .dmg bundles. The Qt and MNE-CPP libs are looked up via LD_LIBRARY_PATH. Needed resources are copied from mne-cpp/resources to mne-cpp/bin. Managed in .pro files.  |

## Packaging/Distribution
Creating setups, zipped folders for uploading and distribution.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Libraries are packaged as well since they reside in the mne-cpp/bin folder. Managed via windows_release AppVeyor job. |The mne-cpp/bin folder including the MNE-CPP libraries is is zipped as a whole and uploaded to website. QtInstallerFramework packages everything it needs from mne-cpp/bin into a setup file. Upload setup to website. Managed via windows_release AppVeyor job. |Examples are packaged as well since they reside in the mne-cpp/bin folder. Managed via windows_release AppVeyor job. |
|Linux |Libraries are compressed to a tar.gz file. Managed via travis linux_release (after_success.sh) job. |The whole mne-cpp/bin and mne-cpp/lib folder are compressed into a single .tar.gz file and uploaded to website. Managed via travis linux_release (after_success.sh) job. |Examples are compressed to a tar.gz file. Managed via travis linux_release (after_success.sh) job. |
|MacOS |Libraries are not packaged. They can only be found as part of the .dmg bundles. |mne_scan, mne_analyze and mne_browse created as .dmg files and uploaded to website. Managed via travis osx_release (after_success.sh) job. |Examples are not packaged. |
