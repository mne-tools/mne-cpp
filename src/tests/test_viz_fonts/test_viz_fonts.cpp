//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_fonts.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests FontFace, FontLibrary, SystemFonts, FontTextMetrics and text in the software renderer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../common/scene_golden.h"
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/font_face.h>
#include <mne/viz/figures/font_library.h>
#include <mne/viz/figures/font_text_metrics.h>
#include <mne/viz/figures/raster_renderer.h>
#include <mne/viz/figures/system_fonts.h>
#include <mne/viz/gpu/software_renderer.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

namespace
{

int g_failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                                      \
        }                                                                                      \
    } while (false)

std::vector<std::uint8_t> read_bytes(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Uses every reader of a face, which must not crash whatever the data
void exercise(const FontFace& face)
{
    for (const char32_t c : std::u32string(U"AVTWa0#\u00e9\u4e2d\U0001F600")) {
        const std::uint32_t glyph = face.glyph_index(c);
        static_cast<void>(face.advance(glyph));
        static_cast<void>(face.kerning(glyph, face.glyph_index(U'V')));
    }
    const int count = std::min(face.glyph_count(), 400);
    for (int g = 0; g < count; ++g) {
        static_cast<void>(face.flattened_outline(static_cast<std::uint32_t>(g), 16.0));
    }
}

void test_robustness()
{
    // Not fonts at all
    CHECK(!FontFace::from_bytes({}));
    CHECK(!FontFace::from_bytes(std::vector<std::uint8_t>(100, 0xFF)));
    CHECK(!FontFace::from_bytes({'t', 't', 'c', 'f', 0, 1, 0, 0, 0, 0, 0, 9}));
    CHECK(!FontFace::load("/nonexistent/font.ttf"));
    const auto cff = FontFace::from_bytes({'O', 'T', 'T', 'O', 0, 0});
    CHECK(!cff && cff.error().code == ErrorCode::Format);

    const std::vector<std::uint8_t> font = read_bytes(FONT_DIR "/SourceSans3-Regular.ttf");
    CHECK(font.size() > 100000 && FontFace::face_count(font) == 1);
    CHECK(!FontFace::from_bytes(font, 1));

    // Truncated: fails or reads what is there
    for (std::size_t length = 0; length < font.size(); length += length < 4096 ? 13 : 9973) {
        if (const auto face = FontFace::from_bytes(std::vector<std::uint8_t>(font.begin(), font.begin() + static_cast<std::ptrdiff_t>(length)))) {
            exercise(**face);
        }
    }
    // Garbled bytes anywhere, in the tables too
    std::mt19937 random(42); // NOLINT(bugprone-random-generator-seed): a fixed seed keeps the garbled fonts reproducible
    int loaded = 0;
    for (int round = 0; round < 60; ++round) {
        std::vector<std::uint8_t> garbled = font;
        const int flips = round < 30 ? 20 : 2000;
        // Every third round hits the table directory and the small tables at the front
        const std::size_t span = round % 3 == 0 ? std::min<std::size_t>(font.size(), 2048) : font.size();
        for (int k = 0; k < flips; ++k) {
            garbled[random() % span] = static_cast<std::uint8_t>(random());
        }
        if (const auto face = FontFace::from_bytes(std::move(garbled))) {
            ++loaded;
            exercise(**face);
        }
    }
    std::printf("Robustness: %d of 60 garbled fonts still loaded, none crashed\n", loaded);
}

void test_bundled_faces()
{
    const auto loaded = FontFace::load(FONT_DIR "/SourceSans3-Regular.ttf");
    CHECK(loaded.has_value());
    const FontFace& face = **loaded;
    const std::uint32_t t = face.glyph_index(U'T');
    const std::uint32_t o = face.glyph_index(U'o');
    const double kern = face.kerning(t, o) * 16.0 / face.units_per_em(); // pixels at 16 px
    CHECK(face.family() == "Source Sans 3" && face.style() == "Regular" && face.weight() == 400 && !face.italic());
    CHECK(face.units_per_em() > 0 && face.ascender() > 0 && face.descender() < 0 && face.line_gap() >= 0);
    // sCapHeight of the OS/2 table, as fontTools reads it
    CHECK(face.cap_height() == 660);
    CHECK(t != 0 && o != 0 && face.glyph_index(U'\u4e2d') == 0);
    // GPOS pair kerning (class pairs in extension lookups), as fontTools reads it
    CHECK(kern < -1.0 && face.kerning(t, o) == -66);
    const auto pair = [&](char32_t left, char32_t right) {
        return face.kerning(face.glyph_index(left), face.glyph_index(right));
    };
    CHECK(pair(U'A', U'V') == -14 && pair(U'A', U'T') == -55 && pair(U'L', U'T') == -120 && pair(U'o', U'o') == 0);
    const auto outline = face.outline(face.glyph_index(U'B'));
    CHECK(outline.size() == 3); // the outer shape and two counters
    CHECK(face.outline(face.glyph_index(U' ')).empty() && face.advance(face.glyph_index(U' ')) > 0);
    const std::uint32_t upperH = face.glyph_index(U'H');
    const std::uint32_t lowerH = face.glyph_index(U'h');
    CHECK(upperH != lowerH && face.advance(upperH) != face.advance(lowerH));
    CHECK(*face.flattened_outline(upperH, 16.0) != *face.flattened_outline(lowerH, 16.0));
    // Composite glyph: e acute from e and the accent
    CHECK(face.outline(face.glyph_index(U'\u00e9')).size() >= 2);
    // Finer flattening at larger sizes
    const std::uint32_t round = face.glyph_index(U'O');
    CHECK(face.flattened_outline(round, 200.0)->front().size() > face.flattened_outline(round, 10.0)->front().size());

    const auto semibold = FontLibrary::sans(true);
    const auto mono = FontLibrary::mono();
    CHECK(semibold && semibold->family() == "Source Sans 3" && semibold->weight() == 600);
    CHECK(mono && mono->family() == "Source Code Pro" && mono->cap_height() == 660);
    CHECK(mono->advance(mono->glyph_index(U'i')) == mono->advance(mono->glyph_index(U'W')));
    CHECK(FontFace::from_bytes(read_bytes(FONT_DIR "/SourceCodePro-Regular.ttf")).has_value());

    CHECK(decode_utf8("a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80") == U"a\u00e9\u4e2d\U0001F600");
    CHECK(decode_utf8("\xC3(\xFF\xE4\xB8") == U"\uFFFD(\uFFFD\uFFFD");
}

void test_font_library()
{
    const auto fonts = std::make_shared<FontLibrary>(); // bundled fonts only, identical everywhere
    const FontTextMetrics metrics(fonts);
    RasterOptions options;
    options.fonts = fonts.get();
    Canvas canvas(200, 40, Color::white());
    canvas.set_font(Font{{}, 0.0, 16.0});
    canvas.draw_text(Rect{0, 0, 200, 40}, Align::Left | Align::VCenter, "Hamburgefonstiv");
    const RasterImage image = to_raster(canvas.take(), options);
    CHECK(image.width() == 200);
    CHECK(fonts->face(Font{}) == FontLibrary::sans(false));
    CHECK(fonts->face(Font{"Source Sans 3", 0, 0, true}) == FontLibrary::sans(true));
    CHECK(fonts->face(Font{"sans-serif"}) == FontLibrary::sans(false));
    CHECK(fonts->face(Font{"monospace"}) == FontLibrary::mono());
    CHECK(fonts->face(Font{"Source Code Pro"}) == FontLibrary::mono());
    CHECK(fonts->face(Font{"No Such Family"}) == FontLibrary::sans(false));
    CHECK(fonts->face(Font{"No Such Mono"}) == FontLibrary::mono());
    CHECK(!fonts->fallback_face(Font{}, U'\u4e2d'));

    const Font font{{}, 0.0, 16.0};
    CHECK(metrics.pixel_size(font) == 16.0 && metrics.pixel_size(Font{}) == 13.0 && metrics.pixel_size(Font{{}, 12.0}) == 16.0);
    CHECK(metrics.ascent(font) > metrics.descent(font) && metrics.descent(font) > 0.0);
    CHECK(metrics.advance(font, "") == 0.0 && metrics.advance(font, "W") > metrics.advance(font, "i"));
    CHECK(metrics.advance(Font{{}, 0.0, 16.0, true}, "Hamburg") > metrics.advance(font, "Hamburg"));
    // Kerning is part of the advance
    CHECK(metrics.advance(font, "To") < metrics.advance(font, "T") + metrics.advance(font, "o") - 1.0);
}

// Columns and rows of the pixels darker than the white ground
struct Extent
{
    int left = 1 << 20;
    int right = -1;
    int top = 1 << 20;
    int bottom = -1;
    int partial = 0;
};

Extent ink(const RasterImage& image)
{
    Extent e;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const int value = image.pixel(x, y).r;
            if (value < 250) {
                e.left = std::min(e.left, x);
                e.right = std::max(e.right, x);
                e.top = std::min(e.top, y);
                e.bottom = std::max(e.bottom, y);
            }
            e.partial += value > 20 && value < 235 ? 1 : 0;
        }
    }
    return e;
}

void test_rendering()
{
    const auto fonts = std::make_shared<FontLibrary>();
    const FontTextMetrics metrics(fonts);
    const Font font{{}, 0.0, 16.0};
    const std::string text = "Hamburgefonstiv 0123 #";
    Canvas canvas(300, 40, Color::white());
    canvas.set_font(font);
    canvas.set_pen(Color::black());
    canvas.draw_text(Point{10, 25}, text);
    const DisplayList list = canvas.take();

    RasterOptions options;
    options.fonts = fonts.get();
    const RasterImage image = to_raster(list, options);
    const Extent e = ink(image);
    // Anti-aliased: many pixels between ground and ink
    CHECK(e.partial > 50);
    // The drawn width is the measured advance, give or take the side bearings
    const double advance = metrics.advance(font, text);
    CHECK(e.left >= 10 && e.left <= 12);
    CHECK(e.right + 1 <= 10 + advance + 1 && e.right + 1 >= 10 + advance - 3);
    // On the baseline: caps reach up towards the ascent, descenders below it
    CHECK(e.bottom > 25 && e.bottom <= 25 + metrics.descent(font));
    CHECK(e.top >= 25 - metrics.ascent(font) && e.top < 25 - 9);
    // Without fonts the bitmap font draws: no anti-aliasing, narrower
    const Extent bitmap = ink(to_raster(list));
    CHECK(bitmap.partial == 0 && bitmap.right != e.right);
    // Clips apply
    Canvas clipped(300, 40, Color::white());
    clipped.set_font(font);
    clipped.set_pen(Color::black());
    clipped.clip_rect(Rect{0, 0, 60, 40});
    clipped.draw_text(Point{10, 25}, text);
    const Extent cut = ink(to_raster(clipped.take(), options));
    CHECK(cut.right < 61 && cut.right > 50);

    // A software-rendered texture uses the given fonts
    const Image rgba = to_rgba_image(list, 1.0, fonts.get());
    CHECK(rgba.width == 300 && rgba.pixel(0, 0) == Color::white());

    // Performance: a 400 x 600 panel with 40 strings
    Canvas panel(400, 600, Color::white());
    for (int row = 0; row < 40; ++row) {
        panel.set_font(Font{{}, 0.0, 13.0, row % 5 == 0});
        panel.set_pen(Color::black());
        panel.draw_text(Rect{12, 15.0 * row, 380, 15}, Align::Left | Align::VCenter, "Band-pass filter order " + std::to_string(row) + " Hz");
    }
    const DisplayList panelList = panel.take();
    static_cast<void>(to_raster(panelList, options)); // fills the outline cache
    const auto start = std::chrono::steady_clock::now();
    static_cast<void>(to_raster(panelList, options));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("Rendering 40 strings into 400 x 600 pixels took %.1f ms\n", ms);
    CHECK(ms < 500.0);
}

void test_vertical_centring()
{
    // Capitals centred in a box sit in its middle in every face and size, so labels and values in Source Sans 3
    // and Source Code Pro line up: the centre of the ink of "HEH" lies at the centre of the box
    const auto fonts = std::make_shared<FontLibrary>();
    RasterOptions options;
    options.fonts = fonts.get();
    const auto inkCentre = [&options](const Font& font, const Rect& box) {
        Canvas canvas(80, 40, Color::white());
        canvas.set_font(font);
        canvas.set_pen(Color::black());
        canvas.draw_text(box, Align::Left | Align::VCenter, "HEH");
        const RasterImage image = to_raster(canvas.take(), options);
        double weight = 0.0;
        double sum = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const double ink = 255.0 - image.pixel(x, y).r;
                weight += ink;
                sum += ink * (y + 0.5);
            }
        }
        return weight > 0.0 ? sum / weight : -1.0;
    };
    for (const double size : {12.0, 13.0, 15.0, 16.0}) {
        for (const double height : {20.0, 21.0, 24.0, 27.0}) {
            const Rect box{4, 5, 70, height};
            const double sans = inkCentre(Font{{}, 0.0, size}, box);
            const double mono = inkCentre(Font{"Source Code Pro", 0.0, size}, box);
            const double middle = box.center().y;
            if (std::abs(sans - middle) > 0.55 || std::abs(mono - middle) > 0.55) {
                std::fprintf(stderr, "size %.0f, height %.0f: sans %.2f, mono %.2f, box %.2f\n", size, height, sans, mono, middle);
            }
            CHECK(std::abs(sans - middle) <= 0.55);
            CHECK(std::abs(mono - middle) <= 0.55);
            CHECK(std::abs(sans - mono) <= 0.5);
        }
    }
}

void test_golden()
{
    const auto fonts = std::make_shared<FontLibrary>();
    Canvas canvas(360, 150, Color::white());
    canvas.set_pen(Color{30, 30, 30});
    canvas.set_font(Font{{}, 0.0, 22.0});
    canvas.draw_text(Point{10, 30}, "AVATAR To Wa Ty fi");
    canvas.set_font(Font{{}, 0.0, 15.0});
    canvas.draw_text(Point{10, 56}, "Regular Hamburgefonstiv 0123");
    canvas.set_font(Font{{}, 0.0, 15.0, true});
    canvas.draw_text(Point{10, 78}, "Semibold Hamburgefonstiv 0123");
    canvas.set_pen(Color{20, 80, 170});
    canvas.set_font(Font{"monospace", 0.0, 13.0});
    canvas.draw_text(Point{10, 102}, "iiii 0123 | x = 1.5e-6;");
    canvas.draw_text(Point{10, 120}, "WWWW 4567 | y = -2.0e+3;");
    canvas.set_pen(Color{110, 110, 110});
    canvas.set_font(Font{{}, 0.0, 11.0});
    canvas.draw_text(Rect{10, 128, 340, 18}, Align::Right | Align::VCenter, "small, right-aligned \u00e9\u00fc\u00df \u2013 \u00b5V");
    CHECK(scene_golden::matches_png(to_rgba_image(canvas.take(), 1.0, fonts.get()), GOLDEN_DIR "/fonts_sample.png"));
}

void test_system_fonts()
{
    const auto start = std::chrono::steady_clock::now();
    const std::shared_ptr<const SystemFonts> system = SystemFonts::shared();
    const std::size_t installed = system->faces().size();
    const std::shared_ptr<const FontFace> face = system->face(Font{});
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (!face) {
        std::printf("PASS (skipped): no installed TrueType font found (%zu faces indexed)\n", installed);
        return;
    }
    std::printf("System fonts: %zu faces indexed and resolved in %.0f ms; default face %s %s\n", installed, ms, face->family().c_str(),
                face->style().c_str());
    CHECK(face->units_per_em() > 0 && face->ascender() > 0 && face->glyph_index(U'A') != 0);
    CHECK(!face->outline(face->glyph_index(U'A')).empty());
    static_cast<void>(face->kerning(face->glyph_index(U'A'), face->glyph_index(U'V')));
    CHECK(system->face(Font{}) == face); // cached
    CHECK(system->find(face->family(), false, false) != nullptr && !system->find("No Such Family", false, false));

    // An installed family through the library; characters the bundled fonts lack from the system
    const FontLibrary library(system);
    CHECK(library.face(Font{face->family()})->family() == face->family());
    if (const auto cjk = library.fallback_face(Font{}, U'\u4e2d')) {
        std::printf("Fallback for U+4E2D: %s\n", cjk->family().c_str());
        CHECK(cjk->glyph_index(U'\u4e2d') != 0);
    }
    RasterOptions options;
    options.fonts = &library;
    Canvas canvas(200, 30, Color::white());
    canvas.set_font(Font{face->family(), 0.0, 16.0});
    canvas.set_pen(Color::black());
    canvas.draw_text(Point{5, 20}, "Hamburgefonstiv \u4e2d");
    CHECK(ink(to_raster(canvas.take(), options)).partial > 20);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    test_robustness();
    test_bundled_faces();
    test_font_library();
    test_rendering();
    test_vertical_centring();
    test_golden();
    test_system_fonts();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("All font tests passed\n");
    return 0;
}
