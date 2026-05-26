//=============================================================================================================
/**
 * @file     shot_kinds.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Renderer functions for each supported shot kind.
 */

#ifndef DOCSHOTS_SHOT_KINDS_H
#define DOCSHOTS_SHOT_KINDS_H

#include "shot_runner.h"

namespace DOCSHOTS
{

/** Render a widget-mockup shot: a stylised QMainWindow with named toolbar
 *  and docks driven entirely from the manifest's "setup" block. */
bool renderWidgetMockup(const ShotSpec& spec, const QString& outPath, QString& err);

/** Render a 3D scene-only shot. Currently produces a labelled placeholder
 *  image via QPainter; sets @p skipped = true if a real Metal/QRhi readback
 *  is requested but no GPU device is available. */
bool renderSceneOnly(const ShotSpec& spec, const QString& outPath,
                     bool& skipped, QString& err);

}  // namespace DOCSHOTS

#endif  // DOCSHOTS_SHOT_KINDS_H
