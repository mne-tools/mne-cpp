//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scenepainter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The QPainter backend of mne_scene: draws display lists and measures text with Qt's fonts.
 *
 * Widgets keep their figures as Qt-free mne::viz models, build a display list in paintEvent() with
 * QtTextMetrics and draw it with paintScene(). The same models render to SVG or PNG without Qt.
 */

#ifndef DISP_SCENEPAINTER_H
#define DISP_SCENEPAINTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include <mne/viz/figures/display_list.h>
#include <mne/viz/figures/text_metrics.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFont>

class QColor;
class QPainter;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

/**
 * Draws @p list with @p painter, on top of the painter's current transform.
 *
 * @param[in] painter    Active painter; its state is restored afterwards.
 * @param[in] list       Figure; its background, if any, fills (0, 0, width, height).
 * @param[in] baseFont   Font whose family, style strategy and size fill in what a scene font leaves unset.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT void paintScene(QPainter& painter, const viz::DisplayList& list, const QFont& baseFont);

/**
 * @param[in] font       Scene font.
 * @param[in] baseFont   Font that fills in what @p font leaves unset.
 *
 * @return The QFont for @p font.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT QFont toQFont(const viz::Font& font, const QFont& baseFont);

/**
 * @param[in] font   Qt font.
 *
 * @return @p font as a scene font of the same size, weight and style.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT viz::Font toSceneFont(const QFont& font);

/**
 * @param[in] color   Qt colour.
 *
 * @return @p color as a scene colour; an invalid colour is opaque black, as QPainter draws it.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT viz::Color toSceneColor(const QColor& color);

/**
 * @param[in] style   Qt pen style.
 *
 * @return The scene dash pattern of @p style; custom dashes become solid.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT viz::LineStyle toSceneLineStyle(Qt::PenStyle style);

/**
 * @brief Measures scene text with QFontMetrics, so layouts match what QPainter draws.
 *
 * Widths and heights are whole pixels, as QFontMetrics reports them.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT QtTextMetrics : public viz::TextMetrics
{
public:
    /**
     * @param[in] baseFont   Font that fills in what a scene font leaves unset, usually the widget's font.
     */
    explicit QtTextMetrics(const QFont& baseFont);

    double pixel_size(const viz::Font& font) const override;
    double advance(const viz::Font& font, std::string_view text) const override;
    double ascent(const viz::Font& font) const override;
    double descent(const viz::Font& font) const override;
    double height(const viz::Font& font) const override;

private:
    QFont m_baseFont;
};

} // namespace mne::viz::qt

#endif // DISP_SCENEPAINTER_H
