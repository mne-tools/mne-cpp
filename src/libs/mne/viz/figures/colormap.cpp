//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     colormap.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Definition of the colour maps.
 *
 * Each channel is a piecewise linear function of the value, scaled to 0-255 and rounded down.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "colormap.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

constexpr std::array<std::array<std::uint8_t, 3>, 256> kViridis = {{
    {68, 1, 84},
    {68, 2, 86},
    {69, 4, 87},
    {69, 5, 89},
    {70, 7, 90},
    {70, 8, 92},
    {70, 10, 93},
    {70, 11, 94},
    {71, 13, 96},
    {71, 14, 97},
    {71, 16, 99},
    {71, 17, 100},
    {71, 19, 101},
    {72, 20, 103},
    {72, 22, 104},
    {72, 23, 105},
    {72, 24, 106},
    {72, 26, 108},
    {72, 27, 109},
    {72, 28, 110},
    {72, 29, 111},
    {72, 31, 112},
    {72, 32, 113},
    {72, 33, 115},
    {72, 35, 116},
    {72, 36, 117},
    {72, 37, 118},
    {72, 38, 119},
    {72, 40, 120},
    {72, 41, 121},
    {71, 42, 122},
    {71, 44, 122},
    {71, 45, 123},
    {71, 46, 124},
    {71, 47, 125},
    {70, 48, 126},
    {70, 50, 126},
    {70, 51, 127},
    {70, 52, 128},
    {69, 53, 129},
    {69, 55, 129},
    {69, 56, 130},
    {68, 57, 131},
    {68, 58, 131},
    {68, 59, 132},
    {67, 61, 132},
    {67, 62, 133},
    {66, 63, 133},
    {66, 64, 134},
    {66, 65, 134},
    {65, 66, 135},
    {65, 68, 135},
    {64, 69, 136},
    {64, 70, 136},
    {63, 71, 136},
    {63, 72, 137},
    {62, 73, 137},
    {62, 74, 137},
    {62, 76, 138},
    {61, 77, 138},
    {61, 78, 138},
    {60, 79, 138},
    {60, 80, 139},
    {59, 81, 139},
    {59, 82, 139},
    {58, 83, 139},
    {58, 84, 140},
    {57, 85, 140},
    {57, 86, 140},
    {56, 88, 140},
    {56, 89, 140},
    {55, 90, 140},
    {55, 91, 141},
    {54, 92, 141},
    {54, 93, 141},
    {53, 94, 141},
    {53, 95, 141},
    {52, 96, 141},
    {52, 97, 141},
    {51, 98, 141},
    {51, 99, 141},
    {50, 100, 142},
    {50, 101, 142},
    {49, 102, 142},
    {49, 103, 142},
    {49, 104, 142},
    {48, 105, 142},
    {48, 106, 142},
    {47, 107, 142},
    {47, 108, 142},
    {46, 109, 142},
    {46, 110, 142},
    {46, 111, 142},
    {45, 112, 142},
    {45, 113, 142},
    {44, 113, 142},
    {44, 114, 142},
    {44, 115, 142},
    {43, 116, 142},
    {43, 117, 142},
    {42, 118, 142},
    {42, 119, 142},
    {42, 120, 142},
    {41, 121, 142},
    {41, 122, 142},
    {41, 123, 142},
    {40, 124, 142},
    {40, 125, 142},
    {39, 126, 142},
    {39, 127, 142},
    {39, 128, 142},
    {38, 129, 142},
    {38, 130, 142},
    {38, 130, 142},
    {37, 131, 142},
    {37, 132, 142},
    {37, 133, 142},
    {36, 134, 142},
    {36, 135, 142},
    {35, 136, 142},
    {35, 137, 142},
    {35, 138, 141},
    {34, 139, 141},
    {34, 140, 141},
    {34, 141, 141},
    {33, 142, 141},
    {33, 143, 141},
    {33, 144, 141},
    {33, 145, 140},
    {32, 146, 140},
    {32, 146, 140},
    {32, 147, 140},
    {31, 148, 140},
    {31, 149, 139},
    {31, 150, 139},
    {31, 151, 139},
    {31, 152, 139},
    {31, 153, 138},
    {31, 154, 138},
    {30, 155, 138},
    {30, 156, 137},
    {30, 157, 137},
    {31, 158, 137},
    {31, 159, 136},
    {31, 160, 136},
    {31, 161, 136},
    {31, 161, 135},
    {31, 162, 135},
    {32, 163, 134},
    {32, 164, 134},
    {33, 165, 133},
    {33, 166, 133},
    {34, 167, 133},
    {34, 168, 132},
    {35, 169, 131},
    {36, 170, 131},
    {37, 171, 130},
    {37, 172, 130},
    {38, 173, 129},
    {39, 173, 129},
    {40, 174, 128},
    {41, 175, 127},
    {42, 176, 127},
    {44, 177, 126},
    {45, 178, 125},
    {46, 179, 124},
    {47, 180, 124},
    {49, 181, 123},
    {50, 182, 122},
    {52, 182, 121},
    {53, 183, 121},
    {55, 184, 120},
    {56, 185, 119},
    {58, 186, 118},
    {59, 187, 117},
    {61, 188, 116},
    {63, 188, 115},
    {64, 189, 114},
    {66, 190, 113},
    {68, 191, 112},
    {70, 192, 111},
    {72, 193, 110},
    {74, 193, 109},
    {76, 194, 108},
    {78, 195, 107},
    {80, 196, 106},
    {82, 197, 105},
    {84, 197, 104},
    {86, 198, 103},
    {88, 199, 101},
    {90, 200, 100},
    {92, 200, 99},
    {94, 201, 98},
    {96, 202, 96},
    {99, 203, 95},
    {101, 203, 94},
    {103, 204, 92},
    {105, 205, 91},
    {108, 205, 90},
    {110, 206, 88},
    {112, 207, 87},
    {115, 208, 86},
    {117, 208, 84},
    {119, 209, 83},
    {122, 209, 81},
    {124, 210, 80},
    {127, 211, 78},
    {129, 211, 77},
    {132, 212, 75},
    {134, 213, 73},
    {137, 213, 72},
    {139, 214, 70},
    {142, 214, 69},
    {144, 215, 67},
    {147, 215, 65},
    {149, 216, 64},
    {152, 216, 62},
    {155, 217, 60},
    {157, 217, 59},
    {160, 218, 57},
    {162, 218, 55},
    {165, 219, 54},
    {168, 219, 52},
    {170, 220, 50},
    {173, 220, 48},
    {176, 221, 47},
    {178, 221, 45},
    {181, 222, 43},
    {184, 222, 41},
    {186, 222, 40},
    {189, 223, 38},
    {192, 223, 37},
    {194, 223, 35},
    {197, 224, 33},
    {200, 224, 32},
    {202, 225, 31},
    {205, 225, 29},
    {208, 225, 28},
    {210, 226, 27},
    {213, 226, 26},
    {216, 226, 25},
    {218, 227, 25},
    {221, 227, 24},
    {223, 227, 24},
    {226, 228, 24},
    {229, 228, 25},
    {231, 228, 25},
    {234, 229, 26},
    {236, 229, 27},
    {239, 229, 28},
    {241, 229, 29},
    {244, 230, 30},
    {246, 230, 32},
    {248, 230, 33},
    {251, 231, 35},
    {253, 231, 37},
}};

// f(x) = m * x + n scaled to 0-255 and rounded down
int slope(double x, double m, double n)
{
    return static_cast<int>(std::floor((m * x + n) * 255));
}

constexpr int kFull = 255;

// Channels wrap to 8 bits like qRgb
Color rgb(int r, int g, int b)
{
    return {static_cast<std::uint8_t>(r & 0xff), static_cast<std::uint8_t>(g & 0xff), static_cast<std::uint8_t>(b & 0xff)};
}

Color jet(double x)
{
    const int r = x < 0.375 ? 0 : x < 0.625 ? slope(x, 4, -1.5)
        : x < 0.875                         ? kFull
                                            : slope(x, -4, 4.5);
    const int g = x < 0.125 ? 0 : x < 0.375 ? slope(x, 4, -0.5)
        : x < 0.625                         ? kFull
        : x < 0.875                         ? slope(x, -4, 3.5)
                                            : 0;
    const int b = x < 0.125 ? slope(x, 4, 0.5) : x < 0.375 ? kFull
        : x < 0.625                                        ? slope(x, -4, 2.5)
                                                           : 0;
    return rgb(r, g, b);
}

Color hot(double x)
{
    const int r = x < 0.375 ? slope(x, 2.5621, 0.0392) : kFull;
    const int g = x < 0.375 ? 0 : x < 0.75 ? slope(x, 2.6667, -1.0)
                                           : kFull;
    const int b = x < 0.75 ? 0 : slope(x, 4, -3);
    return rgb(r, g, b);
}

Color hotNegative1(double x)
{
    const int r = x < 0.2188 ? 0 : x < 0.5781 ? slope(x, 2.7832, -0.6090)
                                              : kFull;
    const int g = x < 0.5781 ? 0 : x < 0.8125 ? slope(x, 4.2662, -2.4663)
                                              : kFull;
    const int b = x < 0.8125 ? 0 : slope(x, 5.3333, -4.3333);
    return rgb(r, g, b);
}

Color hotNegative2(double x)
{
    const int r = x < 0.5625 ? 0 : x < 0.8438 ? slope(x, 3.5549, -1.9996)
                                              : kFull;
    const int g = x < 0.8438 ? 0 : x < 0.9531 ? slope(x, 9.1491, -7.72)
                                              : kFull;
    const int b = x < 0.9531 ? 0 : slope(x, 21.3220, -20.3220);
    return rgb(r, g, b);
}

Color bone(double x)
{
    const int r = x < 0.375 ? slope(x, 0.8471, 0) : x < 0.75 ? slope(x, 0.8889, -0.0157)
                                                             : slope(x, 1.396, -0.396);
    const int g = x < 0.375 ? slope(x, 0.8471, 0) : x < 0.75 ? slope(x, 1.2237, -0.1413)
                                                             : slope(x, 0.894, 0.106);
    const int b = x < 0.375 ? slope(x, 1.1712, 0.0039) : x < 0.75 ? slope(x, 0.8889, 0.1098)
                                                                  : slope(x, 0.8941, 0.1059);
    return rgb(r, g, b);
}

Color redBlue(double x)
{
    const int r = x < 0 ? slope(x, 1, 1) : kFull;
    const int g = x < 0 ? slope(x, 1, 1) : slope(x, -1, 1);
    const int b = x < 0 ? kFull : slope(x, -1, 1);
    return rgb(r, g, b);
}

Color cool(double x)
{
    return rgb(slope(x, 1, 0), slope(x, -1, 1), kFull);
}

Color viridis(double x, bool negated)
{
    // Truncation toward zero as in the unsigned conversion of the original table lookup
    if (!(x >= 0.0) || x * 255 >= 256.0) {
        return Color::transparent();
    }
    const auto index = static_cast<std::size_t>(x * 255);
    const std::size_t row = negated ? 255 - index : index;
    if (row >= kViridis.size()) {
        return Color::transparent();
    }
    return {kViridis[row][0], kViridis[row][1], kViridis[row][2]};
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::optional<Colormap> mne::viz::colormap_from_name(std::string_view name)
{
    if (name == "Jet") {
        return Colormap::Jet;
    }
    if (name == "Hot") {
        return Colormap::Hot;
    }
    if (name == "HotNegative1") {
        return Colormap::HotNegative1;
    }
    if (name == "HotNegative2") {
        return Colormap::HotNegative2;
    }
    if (name == "Bone") {
        return Colormap::Bone;
    }
    if (name == "RedBlue") {
        return Colormap::RedBlue;
    }
    if (name == "Cool") {
        return Colormap::Cool;
    }
    if (name == "Viridis") {
        return Colormap::Viridis;
    }
    if (name == "ViridisNegated") {
        return Colormap::ViridisNegated;
    }
    return std::nullopt;
}

//=============================================================================================================

Color mne::viz::map_color(Colormap map, double value)
{
    switch (map) {
        case Colormap::Hot:
            return hot(value);
        case Colormap::HotNegative1:
            return hotNegative1(value);
        case Colormap::HotNegative2:
            return hotNegative2(value);
        case Colormap::Bone:
            return bone(value);
        case Colormap::RedBlue:
            return redBlue(value);
        case Colormap::Cool:
            return cool(value);
        case Colormap::Viridis:
            return viridis(value, false);
        case Colormap::ViridisNegated:
            return viridis(value, true);
        default:
            return jet(value);
    }
}

//=============================================================================================================

Color mne::viz::map_color(std::string_view name, double value)
{
    const Colormap map = colormap_from_name(name).value_or(Colormap::Jet);
    // RedBlue spans [-1, 1]
    return map_color(map, map == Colormap::RedBlue ? 2.0 * value - 1.0 : value);
}

//=============================================================================================================

Color mne::viz::color_from_hsl(double hue, double saturation, double lightness, double alpha)
{
    const double h = (hue - std::floor(hue)) * 6.0;
    const double s = std::clamp(saturation, 0.0, 1.0);
    const double l = std::clamp(lightness, 0.0, 1.0);
    const double chroma = (1.0 - std::abs(2.0 * l - 1.0)) * s;
    const double x = chroma * (1.0 - std::abs(std::fmod(h, 2.0) - 1.0));
    const double m = l - chroma / 2.0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (h < 1.0) {
        r = chroma;
        g = x;
    } else if (h < 2.0) {
        r = x;
        g = chroma;
    } else if (h < 3.0) {
        g = chroma;
        b = x;
    } else if (h < 4.0) {
        g = x;
        b = chroma;
    } else if (h < 5.0) {
        r = x;
        b = chroma;
    } else {
        r = chroma;
        b = x;
    }
    auto byte = [](double v) {
        return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0, 1.0) * 255.0));
    };
    return {byte(r + m), byte(g + m), byte(b + m), byte(alpha)};
}

//=============================================================================================================

Color mne::viz::darker(Color color, int factor)
{
    if (factor <= 100) {
        return color;
    }
    // Hue and saturation stay, so scaling the value scales every channel alike
    auto scale = [factor](std::uint8_t v) {
        return static_cast<std::uint8_t>(std::lround(static_cast<double>(v) * 100.0 / factor));
    };
    return {scale(color.r), scale(color.g), scale(color.b), color.a};
}
