//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_mne_analyze_studio_app.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Renderer for the `mne_analyze_studio_app` shot kind.
 */

#ifndef DOCSHOTS_SHOT_MNE_ANALYZE_STUDIO_APP_H
#define DOCSHOTS_SHOT_MNE_ANALYZE_STUDIO_APP_H

#include <QString>

namespace DOCSHOTS
{

struct ShotSpec;

//=============================================================================================================
/**
 * Build the real MNE Analyze Studio workbench in offline mode (no backend processes), open a workflow file
 * from `src/applications/mne_analyze_studio/examples/workflows/` and capture it.
 *
 * Setup keys: `workflow` (file name in the examples directory), `open_editor` (bool; also open the file in
 * an editor tab), `center` ("graph" | "editor"; which center tab is in front).
 *
 * @param[in] spec       Shot specification.
 * @param[in] outPath    Destination PNG.
 * @param[out] skipped   Always false.
 * @param[out] err       Diagnostics on failure.
 * @return               True if the PNG was written.
 */
bool renderMneAnalyzeStudioApp(const ShotSpec& spec, const QString& outPath, bool& skipped, QString& err);

} // namespace DOCSHOTS

#endif // DOCSHOTS_SHOT_MNE_ANALYZE_STUDIO_APP_H
