//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_text_metrics.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FontTextMetrics measures text with the faces of a FontSource, as the software renderer draws it.
 */

#ifndef VIZ_FONT_TEXT_METRICS_H
#define VIZ_FONT_TEXT_METRICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "font_source.h"
#include "text_metrics.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Text metrics of real fonts: the layouts of views match the text the software renderer draws with the
 * same FontSource (RasterOptions::fonts, SoftwareRenderer::set_fonts()).
 *
 * Advances are fractional and include kerning; ascent and descent are whole pixels. Fonts the source has no
 * face for are measured like ApproximateTextMetrics.
 *
 * @snippet ex_scene/main.cpp font_library_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FontTextMetrics : public TextMetrics
{
public:
    //=========================================================================================================
    /**
     * @param[in] fonts                Source of the faces, e.g. FontLibrary::shared().
     * @param[in] default_pixel_size   Pixel size of a font without size.
     *
     * @since 3.0.0
     */
    explicit FontTextMetrics(std::shared_ptr<const FontSource> fonts, double default_pixel_size = 13.0);

    double pixel_size(const Font& font) const override;
    double advance(const Font& font, std::string_view text) const override;
    double ascent(const Font& font) const override;
    double descent(const Font& font) const override;

private:
    std::shared_ptr<const FontSource> m_fonts;
    ApproximateTextMetrics m_approximate;
    double m_defaultPixelSize;
};

} // namespace mne::viz

#endif // VIZ_FONT_TEXT_METRICS_H
