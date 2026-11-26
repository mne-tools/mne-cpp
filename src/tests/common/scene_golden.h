//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scene_golden.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Golden PNG comparison of rendered scene images for the Qt-free tests.
 *
 * MNE_UPDATE_GOLDEN=1 rewrites the golden files from the current output.
 */

#ifndef TESTS_COMMON_SCENE_GOLDEN_H
#define TESTS_COMMON_SCENE_GOLDEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/raster_image.h>
#include <mne/viz/figures/display_list.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace scene_golden
{

//=============================================================================================================
/**
 * Compares an image with a golden PNG, ignoring alpha. Small differences from floating-point contraction on other
 * CPUs are tolerated: up to one pixel in 500 may differ by more than 2 in a channel.
 *
 * @param[in] image   Rendered image.
 * @param[in] path    Golden PNG file.
 *
 * @return Whether the image matches; the difference is printed otherwise.
 */
inline bool matches_png(const mne::viz::Image& image, const std::string& path)
{
    mne::RasterImage rgb(image.width, image.height);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const mne::viz::Color c = image.pixel(x, y);
            rgb.set_pixel(x, y, {c.r, c.g, c.b});
        }
    }
    const char* update = std::getenv("MNE_UPDATE_GOLDEN");
    if (update && std::strcmp(update, "1") == 0) {
        return rgb.save_png(path).has_value();
    }
    const mne::Expected<mne::RasterImage> expected = mne::RasterImage::load_png(path);
    if (!expected) {
        std::fprintf(stderr, "%s (run with MNE_UPDATE_GOLDEN=1)\n", expected.error().what());
        return false;
    }
    if (expected->width() != rgb.width() || expected->height() != rgb.height()) {
        std::fprintf(stderr, "%s: %d x %d instead of %d x %d\n", path.c_str(), rgb.width(), rgb.height(), expected->width(), expected->height());
        return false;
    }
    int differing = 0;
    for (int y = 0; y < rgb.height(); ++y) {
        for (int x = 0; x < rgb.width(); ++x) {
            const mne::Rgb a = rgb.pixel(x, y);
            const mne::Rgb b = expected->pixel(x, y);
            differing += std::max({std::abs(a.r - b.r), std::abs(a.g - b.g), std::abs(a.b - b.b)}) > 2 ? 1 : 0;
        }
    }
    const int allowed = rgb.width() * rgb.height() / 500;
    if (differing > allowed) {
        std::fprintf(stderr, "%s: %d pixels differ (%d allowed)\n", path.c_str(), differing, allowed);
        return false;
    }
    return true;
}

} // namespace scene_golden

#endif // TESTS_COMMON_SCENE_GOLDEN_H
