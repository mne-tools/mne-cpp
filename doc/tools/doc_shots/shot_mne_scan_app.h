//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_mne_scan_app.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Renderer for the `mne_scan_app` shot kind.
 */

#ifndef DOCSHOTS_SHOT_MNE_SCAN_APP_H
#define DOCSHOTS_SHOT_MNE_SCAN_APP_H

#include <QString>

namespace DOCSHOTS
{

struct ShotSpec;

//=============================================================================================================
/**
 * Build the real MNE Scan `MainWindow` with the plugins of this build, optionally load a pipeline, select a
 * plugin or open a plugin menu, and capture it.
 *
 * Setup keys: `pipeline` (array of {"plugin", "x", "y"}; consecutive entries are connected), `select`
 * (plugin name whose setup widget is shown), `open_menu` ("sensor" | "algorithm"; the menu is rendered into
 * the image next to its tool button).
 *
 * @param[in] spec       Shot specification.
 * @param[in] outPath    Destination PNG.
 * @param[out] skipped   Always false.
 * @param[out] err       Diagnostics on failure.
 * @return               True if the PNG was written.
 */
bool renderMneScanApp(const ShotSpec& spec, const QString& outPath, bool& skipped, QString& err);

} // namespace DOCSHOTS

#endif // DOCSHOTS_SHOT_MNE_SCAN_APP_H
