# MNE-CPP v2.4.0 coverage baseline

Commit: `96413f7ccc7dabed2c6d40a511960ce8805e0407`
Coverage source: https://github.com/mne-tools/mne-cpp/actions/runs/36243010551

## Totals

- Lines: 60,091 / 123,014 (48.85%)
- Branches: 0 / 0 (not reported)
- Files: 1,227
- Codecov model (partials as hits): 60,091 / 123,014 (48.85%)

## Scope

| Scope | Lines | Coverage | Branches | Branch coverage |
|---|---:|---:|---:|---:|
| `src/libraries` | 39,652 / 63,199 | 62.74% | 0 / 0 | not reported |
| `src/applications` | 11,137 / 44,733 | 24.90% | 0 / 0 | not reported |
| `src/tools` | 9,302 / 15,082 | 61.68% | 0 / 0 | not reported |

## Exclusions

| Pattern | Reported files | Reported lines | Reason |
|---|---:|---:|---|
| `*_autogen/*` | 339 | 15,600 | Generated in the build tree by Qt AUTOMOC/AUTOUIC/AUTORCC; not maintained source, not in the repository, and not mappable by Codecov. |
| `src/applications/mne_scan/plugins/brainamp/*` | 0 | 0 | Built only with -DWITH_BRAINAMP=ON; the driver needs Windows device I/O (windows.h, CreateFileA) and a BrainAmp amplifier. |
| `src/applications/mne_scan/plugins/eegosports/*` | 0 | 0 | Built only with -DWITH_EEGO=ON; needs the proprietary eemagine SDK and an eego amplifier. |
| `src/applications/mne_scan/plugins/gusbamp/*` | 0 | 0 | Built only with -DWITH_GUSBAMP=ON; needs the g.tec gUSBamp SDK and device. |
| `src/applications/mne_scan/plugins/tmsi/*` | 0 | 0 | Built only with -DWITH_TMSI=ON; needs Windows (windows.h), the TMSi SDK DLL and a TMSi amplifier. |
| `src/applications/mne_scan/plugins/dummytoolbox/*` | 0 | 0 | Template for plugin authors; no CMakeLists.txt adds its directory, so no target compiles it. |
| `src/applications/mne_analyze/plugins/sampleplugin/*` | 0 | 0 | Template for plugin authors; its add_subdirectory() is commented out in mne_analyze/plugins/CMakeLists.txt. |
| `src/applications/mne_analyze/plugins/view3d/3dview.cpp` | 0 | 0 | Orphan: in no target's sources, and not compilable ('using namespace 3DVIEWPLUGIN' is not an identifier). |
| `src/applications/mne_browse/Utils/mnxproject.cpp` | 0 | 0 | Orphan: in no target's sources. |
| `src/applications/mne_browse/Utils/newparksmcclellan.cpp` | 0 | 0 | Orphan: in no target's sources. |

## Completeness

740 of 792 tracked translation units are in the report and 43 are excluded. 9 contain no executable code; 0 are missing; 0 reported files are untracked.
- no executable code: `src/applications/mne_scan/libs/scShared/Management/mna_scan_types.cpp`
- no executable code: `src/libraries/inv/dipole_fit/inv_dipole_forward.cpp`
- no executable code: `src/libraries/inv/minimum_norm/inv_cmne_settings.cpp`
- no executable code: `src/libraries/mne/mne_mgh_tag.cpp`
- no executable code: `src/libraries/mne/mne_mgh_tag_group.cpp`
- no executable code: `src/libraries/mne/mne_mne_data.cpp`
- no executable code: `src/libraries/mne/mne_msh_eyes.cpp`
- no executable code: `src/libraries/mne/mne_msh_light_set.cpp`
- no executable code: `src/libraries/utils/ioutils.cpp`

## Priority files

| Uncovered lines | Covered / found | File |
|---:|---:|---|
| 6,517 | 0 / 6,517 | `src/applications/mne_analyze_studio/workbench/mainwindow.cpp` |
| 2,140 | 594 / 2,734 | `src/applications/mne_browse/Windows/mainwindow.cpp` |
| 1,357 | 648 / 2,005 | `src/libraries/disp3D/view/brainview.cpp` |
| 1,249 | 453 / 1,702 | `src/libraries/disp/viewers/helpers/channelrhiview.cpp` |
| 1,194 | 940 / 2,134 | `src/applications/mne_inspect/app/mainwindow.cpp` |
| 945 | 0 / 945 | `src/libraries/disp3D/view/brainrenderer.cpp` |
| 888 | 118 / 1,006 | `src/applications/mne_browse/Windows/datawindow.cpp` |
| 640 | 466 / 1,106 | `src/applications/mne_analyze/plugins/cortical_surface/cortical_surface.cpp` |
| 585 | 0 / 585 | `src/applications/mne_scan/mne_scan/mainwindow.cpp` |
| 539 | 271 / 810 | `src/libraries/utils/polhemus/polhemus_coregistration.cpp` |
| 531 | 1,215 / 1,746 | `src/libraries/fwd/fwd_bem_model.cpp` |
| 512 | 489 / 1,001 | `src/applications/mne_browse/Models/annotationmodel.cpp` |
| 445 | 163 / 608 | `src/applications/mne_browse/Windows/averagewindow.cpp` |
| 427 | 955 / 1,382 | `src/libraries/mne/mne_source_space.cpp` |
| 423 | 210 / 633 | `src/applications/mne_browse/Models/rawmodel.cpp` |
| 418 | 0 / 418 | `src/applications/mne_scan/plugins/babymeg/FormFiles/babymegsquidcontroldgl.cpp` |
| 415 | 0 / 415 | `src/applications/mne_analyze/plugins/coregistration/coregistration.cpp` |
| 398 | 572 / 970 | `src/libraries/mne/mne_forward_solution.cpp` |
| 391 | 245 / 636 | `src/applications/mne_analyze_studio/extensions/fiff_browser_extension/browsercore/Models/rawmodel.cpp` |
| 391 | 0 / 391 | `src/applications/mne_scan/plugins/rtcmne/rtcmne.cpp` |
| 389 | 33 / 422 | `src/applications/mne_browse/Models/virtualchannelmodel.cpp` |
| 385 | 0 / 385 | `src/applications/mne_analyze/mne_analyze/mainwindow.cpp` |
| 385 | 1,344 / 1,729 | `src/libraries/fiff/fiff_stream.cpp` |
| 379 | 0 / 379 | `src/applications/mne_scan/plugins/hpi/hpi.cpp` |
| 373 | 61 / 434 | `src/applications/mne_analyze_studio/core/llmtoolplanner.cpp` |
