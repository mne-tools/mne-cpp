//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_capture.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Deterministic capture for documentation screenshots: pinned process environment and theme,
 *           exact window size, a readiness barrier, and atomic opaque PNG output.
 */

#ifndef DOCSHOTS_SHOT_CAPTURE_H
#define DOCSHOTS_SHOT_CAPTURE_H

#include <QSize>
#include <QString>

class QApplication;
class QImage;
class QWidget;

namespace DOCSHOTS
{

/** Frames must stay pixel-identical for this long before a window counts as completely rendered. */
constexpr int kSettleMs = 300;

/** A window that has not settled after this long fails its shot. */
constexpr int kSettleTimeoutMs = 15000;

/** Pixel size of the pinned application font; pixels, not points, so the screen DPI cannot change it. */
constexpr int kFontPixelSize = 13;

//=============================================================================================================
/**
 * Pin the offscreen platform, a device pixel ratio of 1, 96 DPI and the en_US locale.
 * Must be called before the QApplication is constructed.
 */
void prepareProcessEnvironment();

//=============================================================================================================
/**
 * Install a Qt message handler that prints each distinct message once; the readiness barrier grabs many
 * frames and would otherwise repeat per-frame warnings hundreds of times.
 */
void installDeduplicatingMessageHandler();

//=============================================================================================================
/**
 * Pin the Fusion style with its light palette, the bundled DejaVu Sans application font, a non-blinking
 * text cursor and no UI animations.
 *
 * @param[in] app    The application instance.
 * @param[out] err   Diagnostics if the bundled font cannot be loaded or is not the font Qt resolves.
 * @return           True if the theme is fully pinned.
 */
bool applyDeterministicTheme(QApplication& app, QString& err);

//=============================================================================================================
/**
 * @return One line describing the capture environment (Qt version, platform, style, resolved font, pixel
 *         ratio, locale), printed with every run so differing images can be traced to their environment.
 */
QString environmentSummary();

//=============================================================================================================
/**
 * Resize @p window to @p size, show it and wait until it has settled.
 *
 * @param[in] window     Top-level window.
 * @param[in] size       Requested size in pixels.
 * @param[out] err       Diagnostics if the window does not settle or cannot take the requested size.
 * @return               True if the window is shown at exactly @p size.
 */
bool showAtSize(QWidget& window, const QSize& size, QString& err);

//=============================================================================================================
/**
 * Readiness barrier: process events until no thread-pool work is running and consecutive grabs of
 * @p window stay identical for @p settleMs.
 *
 * @param[in] window     Shown top-level window.
 * @param[out] err       Diagnostics on timeout.
 * @param[in] settleMs   Time the frame must stay unchanged.
 * @param[in] timeoutMs  Upper bound for the whole wait.
 * @return               True once the window has settled.
 */
bool waitUntilSettled(QWidget& window, QString& err, int settleMs = kSettleMs, int timeoutMs = kSettleTimeoutMs);

//=============================================================================================================
/**
 * Wait until @p window has settled, verify it is exactly @p size at pixel ratio 1, and save it as PNG.
 *
 * @param[in] window     Shown top-level window.
 * @param[in] size       Size the image must have.
 * @param[in] outPath    Destination PNG.
 * @param[out] err       Diagnostics on failure; no file is left at @p outPath then.
 * @return               True if the PNG was written.
 */
bool captureWindow(QWidget& window, const QSize& size, const QString& outPath, QString& err);

//=============================================================================================================
/**
 * Write @p image as an opaque 8-bit RGB PNG through QSaveFile, so @p outPath never holds a partial image.
 *
 * @param[in] image      Image to save.
 * @param[in] outPath    Destination PNG.
 * @param[out] err       Diagnostics on failure.
 * @return               True if the PNG was written.
 */
bool savePng(const QImage& image, const QString& outPath, QString& err);

} // namespace DOCSHOTS

#endif // DOCSHOTS_SHOT_CAPTURE_H
