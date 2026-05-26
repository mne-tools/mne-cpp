//=============================================================================================================
/**
 * @file     shot_mne_inspect_app.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Renderer declaration for the `mne_inspect_app` shot kind.
 */

#ifndef DOCSHOTS_SHOT_MNE_INSPECT_APP_H
#define DOCSHOTS_SHOT_MNE_INSPECT_APP_H

#include "shot_runner.h"

namespace DOCSHOTS
{

//=============================================================================================================
/**
 * Render a screenshot of the live mne_inspect MainWindow at @p spec.size,
 * driven by the manifest entry's setup block. Supported setup keys:
 *
 *  - `load_demo_electrodes` (bool, default false)
 *  - `load_demo_mri`        (bool, default false)
 *  - `focus_dock`           ("pick" | "layers" | "overlay")
 *  - `simulate_pick`        ({ "kind": "contact"|"voxel"|"vertex",
 *                              "target": [array, contact] })
 *  - `fixtures`             (array of registered fixture names — generic
 *                            escape hatch for future apps that hook in via
 *                            `DOCSHOTS::AppFixtureLoaders`)
 *
 * Sets @p skipped only if a hard precondition cannot be met (e.g. missing
 * QApplication). All rendering happens against the QRhi `Null` backend,
 * which the helper switches on before `show()` to keep things headless.
 */
bool renderMneInspectApp(const ShotSpec& spec,
                         const QString& outPath,
                         bool& skipped,
                         QString& err);

}  // namespace DOCSHOTS

#endif  // DOCSHOTS_SHOT_MNE_INSPECT_APP_H
