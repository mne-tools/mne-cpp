//=============================================================================================================
/**
 * @file     shot_mne_align_app.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Renderer declaration for the `mne_align_app` shot kind.
 */

#ifndef DOCSHOTS_SHOT_MNE_ALIGN_APP_H
#define DOCSHOTS_SHOT_MNE_ALIGN_APP_H

#include "shot_runner.h"

namespace DOCSHOTS
{

//=============================================================================================================
/**
 * Render a screenshot of the live mne_align MneAlign window at @p spec.size,
 * driven by the manifest entry's setup block. Supported setup keys:
 *
 *  - `wizard_step`          (int, 0..6 — Setup, Fiducials, EegCap, HeadShape,
 *                            Verify, Save, Done)
 *  - `load_demo_bem`        (bool, default false — currently only flips the
 *                            "BEM loaded" status label via a synthetic path,
 *                            since BEM construction requires on-disk surfaces)
 *  - `load_demo_cap`        ("10-20" | null — selects synthetic EEG fixture)
 *  - `load_demo_digitisation` ("demo" | null — pushes the full demo
 *                            digitisation session into the point store)
 *  - `simulate_capture`     ({ "kind": "fiducial"|"eeg"|"hsp", "count": N })
 *  - `fixtures`             (array of registered fixture names — generic
 *                            escape hatch identical to mne_inspect_app)
 *
 * Sets @p skipped only if a hard precondition cannot be met (e.g. missing
 * QApplication). All rendering happens against the QRhi `Null` backend.
 */
bool renderMneAlignApp(const ShotSpec& spec,
                       const QString& outPath,
                       bool& skipped,
                       QString& err);

}  // namespace DOCSHOTS

#endif  // DOCSHOTS_SHOT_MNE_ALIGN_APP_H
