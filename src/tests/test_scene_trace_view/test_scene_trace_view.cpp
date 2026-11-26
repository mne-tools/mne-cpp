//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_trace_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests TraceView, the trace geometry, TraceMarkers and ViewAnimator of mne_scene, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/raster_image.h>
#include <mne/scene/canvas.h>
#include <mne/scene/software_renderer.h>
#include <mne/scene/svg_renderer.h>
#include <mne/scene/text_metrics.h>
#include <mne/scene/trace_frame.h>
#include <mne/scene/trace_geometry.h>
#include <mne/scene/trace_markers.h>
#include <mne/scene/trace_view.h>
#include <mne/scene/view_animator.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

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

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

// Golden SVG comparison as in test_scene_render, without Qt
bool matches_golden(const std::string& svg, const std::string& path)
{
    const char* update = std::getenv("MNE_UPDATE_GOLDEN");
    if (update && std::strcmp(update, "1") == 0) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << svg;
        return static_cast<bool>(out);
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "missing golden file %s (run with MNE_UPDATE_GOLDEN=1)\n", path.c_str());
        return false;
    }
    const std::string expected((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (expected == svg) {
        return true;
    }
    std::size_t at = 0;
    while (at < expected.size() && at < svg.size() && expected[at] == svg[at]) {
        ++at;
    }
    const std::size_t from = at > 60 ? at - 60 : 0;
    std::fprintf(stderr, "%s differs at byte %zu\n expected: %s\n actual:   %s\n", path.c_str(), at,
                 expected.substr(from, 160).c_str(), svg.substr(from, 160).c_str());
    return false;
}

// Golden PNG comparison without Qt; small differences from floating-point contraction on other CPUs are tolerated
bool matches_golden_png(const Image& image, const std::string& path)
{
    mne::RasterImage rgb(image.width, image.height);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Color c = image.pixel(x, y);
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

// 30 channels: three MEG magnetometers, then gradiometers and EEG; value = index + sample / 1000
class FakeSource : public TraceSource
{
public:
    FakeSource()
    {
        for (int i = 0; i < 30; ++i) {
            if (i < 3) {
                m_channels.push_back({"MEG 01" + std::to_string(11 + i), "MEG mag", 2e-12f, Color::black(), false});
            } else if (i < 20) {
                m_channels.push_back({"MEG 02" + std::to_string(i), "MEG grad", 4e-11f, Color::black(), false});
            } else {
                m_channels.push_back({"EEG 0" + std::to_string(i), "EEG", 5e-5f, Color::black(), false});
            }
        }
    }

    int channel_count() const override
    {
        return static_cast<int>(m_channels.size());
    }

    std::optional<Channel> channel(int index) const override
    {
        if (index < 0 || index >= channel_count()) {
            return std::nullopt;
        }
        return m_channels[static_cast<std::size_t>(index)];
    }

    float value_at(int index, int sample) const override
    {
        return 1e-12f * static_cast<float>(index + 1) * static_cast<float>(sample);
    }

    std::vector<Group> groups() const override
    {
        return {{"MEG mag", 2e-12f}, {"MEG grad", 4e-11f}, {"EEG", 5e-5f}};
    }

    int first_sample() const override
    {
        return m_firstSample;
    }

    int m_firstSample = 0;

private:
    std::vector<Channel> m_channels;
};

// Everything the view asked its host to do
struct Recorder
{
    std::vector<double> scrolls;
    std::vector<double> zooms;
    std::vector<int> offsets;
    std::vector<TraceView::ScrollAnimation> scrollAnimations;
    std::vector<TraceView::ZoomAnimation> zoomAnimations;
    int stops = 0;
    std::vector<int> clicks;
    std::vector<std::pair<int, int>> ranges;
    struct Boundary
    {
        int index;
        bool start;
        int sample;
    };
    std::vector<Boundary> boundaries;
    int annotationChanges = 0;
    std::vector<int> doubleClicks;
    std::vector<CursorShape> cursors;
    std::vector<TraceView::CursorData> cursorData;
    int redraws = 0;

    TraceView::Effects effects()
    {
        TraceView::Effects e;
        e.scroll_changed = [this](double s) {
            scrolls.push_back(s);
        };
        e.zoom_changed = [this](double spp) {
            zooms.push_back(spp);
        };
        e.channel_offset_changed = [this](int c) {
            offsets.push_back(c);
        };
        e.animate_scroll = [this](const TraceView::ScrollAnimation& a) {
            scrollAnimations.push_back(a);
        };
        e.animate_zoom = [this](const TraceView::ZoomAnimation& a) {
            zoomAnimations.push_back(a);
        };
        e.stop_animations = [this] {
            ++stops;
        };
        e.sample_clicked = [this](int s) {
            clicks.push_back(s);
        };
        e.range_selected = [this](int a, int b) {
            ranges.emplace_back(a, b);
        };
        e.annotation_boundary_moved = [this](int i, bool start, int s) {
            boundaries.push_back({i, start, s});
        };
        e.annotations_changed = [this] {
            ++annotationChanges;
        };
        e.channel_double_clicked = [this](int c) {
            doubleClicks.push_back(c);
        };
        e.cursor_changed = [this](CursorShape c) {
            cursors.push_back(c);
        };
        e.cursor_data_changed = [this](const TraceView::CursorData& d) {
            cursorData.push_back(d);
        };
        e.redraw = [this] {
            ++redraws;
        };
        return e;
    }
};

PointerEvent pointer(PointerEvent::Type type, double x, double y, MouseButton button, std::int64_t t = 0,
                     Modifier modifiers = Modifier::None)
{
    PointerEvent event;
    event.type = type;
    event.x = x;
    event.y = y;
    event.button = button;
    event.modifiers = modifiers;
    event.timestamp_ms = t;
    return event;
}

PointerEvent press(double x, double y, MouseButton button, std::int64_t t = 0, Modifier modifiers = Modifier::None)
{
    return pointer(PointerEvent::Type::Press, x, y, button, t, modifiers);
}

PointerEvent move(double x, double y, std::int64_t t = 0)
{
    return pointer(PointerEvent::Type::Move, x, y, MouseButton::None, t);
}

PointerEvent release(double x, double y, MouseButton button, std::int64_t t = 0)
{
    return pointer(PointerEvent::Type::Release, x, y, button, t);
}

WheelEvent wheel(double dx, double dy, Modifier modifiers = Modifier::None)
{
    WheelEvent event;
    event.angle_delta_x = dx;
    event.angle_delta_y = dy;
    event.modifiers = modifiers;
    return event;
}

// A 400 x 300 view at 0.5 samples per pixel (200 visible samples), 12 of 30 channels, 1 kHz from sample 100
struct Fixture
{
    FakeSource source;
    Recorder recorder;
    TraceView view;

    Fixture()
    {
        view.set_source(&source);
        view.set_effects(recorder.effects());
        view.set_view_size(400, 300);
        view.set_samples_per_pixel(0.5);
        view.set_sfreq(1000.0);
        view.set_first_file_sample(100);
        view.set_scroll_sample(1000.0);
        recorder = Recorder();
        view.set_effects(recorder.effects());
    }
};

//=============================================================================================================

void test_clamping()
{
    Fixture f;
    TraceView& view = f.view;
    CHECK(view.visible_sample_count() == 200);
    CHECK(view.visible_first_sample() == 1000);

    // Without a known file end only the lower bound applies: the first loaded sample
    CHECK(view.clamp_scroll_sample(-5.0) == 0.0);
    f.source.m_firstSample = 100;
    CHECK(view.clamp_scroll_sample(-5.0) == 100.0);
    CHECK(view.clamp_scroll_sample(1e9) == 1e9);

    // With the file end the last view ends at the last sample, but never starts before the file
    view.set_last_file_sample(1099);
    CHECK(view.clamp_scroll_sample(5000.0) == 900.0);
    view.set_last_file_sample(150);
    CHECK(view.clamp_scroll_sample(5000.0) == 100.0);
    view.set_last_file_sample(-1);

    // Setting the same position again reports nothing; small steps far into a file still count
    CHECK(!view.set_scroll_sample(1000.0));
    CHECK(f.recorder.scrolls.empty());
    CHECK(view.set_scroll_sample(3600000.0));
    CHECK(view.set_scroll_sample(3600005.0));
    CHECK(f.recorder.scrolls.size() == 2 && f.recorder.scrolls.back() == 3600005.0);

    // Zoom has a floor and ignores relative changes below 1e-5
    CHECK(view.set_samples_per_pixel(0.0));
    CHECK(view.samples_per_pixel() == 1e-4);
    CHECK(!view.set_samples_per_pixel(1e-4 * (1.0 + 1e-7)));

    // Channel window: at most total - visible, at least 0
    CHECK(view.set_first_visible_channel(100));
    CHECK(view.first_visible_channel() == 18);
    CHECK(f.recorder.offsets == std::vector<int>{18});
    CHECK(view.visible_rows() == 12);
    CHECK(view.set_first_visible_channel(-3));
    CHECK(view.first_visible_channel() == 0);
    CHECK(view.set_visible_channel_count(0));
    CHECK(view.visible_channel_count() == 1);
}

//=============================================================================================================

void test_wheel()
{
    Fixture f;
    TraceView& view = f.view;
    Recorder& r = f.recorder;

    // Ctrl zooms in (wheel up) and out over 150 ms
    CHECK(view.handle_wheel(wheel(0, 120, Modifier::Ctrl)));
    CHECK(r.zoomAnimations.size() == 1 && near(r.zoomAnimations[0].to, 0.4) && r.zoomAnimations[0].duration_ms == 150);
    view.handle_wheel(wheel(0, -120, Modifier::Ctrl));
    CHECK(r.zoomAnimations.size() == 2 && near(r.zoomAnimations[1].to, 0.625));

    // Vertical wheel scrolls channel rows by default, clamped to the window
    view.handle_wheel(wheel(0, -120));
    CHECK(view.first_visible_channel() == 1);
    view.handle_wheel(wheel(0, 120));
    view.handle_wheel(wheel(0, 120));
    CHECK(view.first_visible_channel() == 0);
    CHECK(r.offsets == (std::vector<int>{1, 0}));

    // In time mode the vertical wheel scrolls 15 % of the view; ticks during an animation add up
    view.set_wheel_scrolls_channels(false);
    view.handle_wheel(wheel(0, -120));
    CHECK(r.scrollAnimations.size() == 1 && near(r.scrollAnimations[0].to, 1030.0) && r.scrollAnimations[0].duration_ms == 100);
    CHECK(!r.scrollAnimations[0].inertial && view.scroll_animation_running());
    view.handle_wheel(wheel(0, -120));
    CHECK(near(r.scrollAnimations.back().from, 1000.0) && near(r.scrollAnimations.back().to, 1060.0));
    // Once finished, the next tick starts from the position reached
    view.set_scroll_sample(1060.0);
    view.scroll_animation_finished();
    view.handle_wheel(wheel(0, 120));
    CHECK(near(r.scrollAnimations.back().to, 1030.0));

    // A horizontal swipe scrolls 10 %, scaled by the speed factor (clamped to 0.25 .. 4)
    view.set_scroll_speed_factor(10.0);
    CHECK(view.scroll_speed_factor() == 4.0);
    view.set_scroll_speed_factor(2.0);
    view.scroll_animation_finished();
    view.handle_wheel(wheel(-120, 0));
    CHECK(near(r.scrollAnimations.back().to, 1060.0 + 40.0));

    // Frozen: freezing stops the animations, the wheel no longer scrolls in time
    const int stops = r.stops;
    view.set_frozen(true);
    CHECK(r.stops == stops + 1 && !view.scroll_animation_running());
    const std::size_t animations = r.scrollAnimations.size();
    view.handle_wheel(wheel(0, -120));
    view.handle_wheel(wheel(-120, 0));
    CHECK(r.scrollAnimations.size() == animations);
}

//=============================================================================================================

void test_drag_scroll()
{
    Fixture f;
    TraceView& view = f.view;
    Recorder& r = f.recorder;

    // A tap reports its sample and does not scroll
    CHECK(view.handle_pointer(press(40, 50, MouseButton::Left)));
    CHECK(view.handle_pointer(release(40, 50, MouseButton::Left)));
    CHECK(r.clicks == std::vector<int>{1020});
    CHECK(r.scrolls.empty());

    // Moves within 5 px do not scroll; past the threshold the content follows the pointer
    view.handle_pointer(press(100, 50, MouseButton::Left, 0));
    CHECK(view.handle_pointer(move(103, 50, 10)) == false);
    CHECK(r.scrolls.empty());
    CHECK(view.handle_pointer(move(90, 50, 20)));
    CHECK(view.scroll_sample() == 1005.0);
    view.handle_pointer(move(50, 50, 40));
    view.handle_pointer(move(10, 50, 60));
    CHECK(view.scroll_sample() == 1045.0);
    // Release: 90 px in 60 ms at 0.5 spp is 0.75 samples/ms; it glides 500 ms on, a third of v * t further
    view.handle_pointer(release(10, 50, MouseButton::Left, 60));
    CHECK(r.clicks.size() == 1);
    CHECK(r.scrollAnimations.size() == 1);
    const TraceView::ScrollAnimation& glide = r.scrollAnimations.back();
    CHECK(glide.inertial && glide.duration_ms == 500 && near(glide.from, 1045.0) && near(glide.to, 1045.0 + 125.0));

    // Only the last 100 ms count: a slow end of the drag does not glide
    view.handle_pointer(press(100, 50, MouseButton::Left, 1000));
    view.handle_pointer(move(50, 50, 1010));
    view.handle_pointer(move(49, 50, 1300));
    view.handle_pointer(move(48, 50, 1400));
    view.handle_pointer(release(48, 50, MouseButton::Left, 1400));
    CHECK(r.scrollAnimations.size() == 1);

    // Middle button: no threshold, no inertia
    const double start = view.scroll_sample();
    CHECK(view.handle_pointer(press(200, 50, MouseButton::Middle)));
    view.handle_pointer(move(198, 50, 5));
    CHECK(view.scroll_sample() == start + 1.0);
    view.handle_pointer(move(150, 50, 10));
    CHECK(view.scroll_sample() == start + 25.0);
    CHECK(view.handle_pointer(release(150, 50, MouseButton::Middle, 10)));
    CHECK(r.scrollAnimations.size() == 1);

    // Alt + left drags like the middle button
    view.handle_pointer(press(200, 50, MouseButton::Left, 0, Modifier::Alt));
    view.handle_pointer(move(202, 50, 5));
    CHECK(view.scroll_sample() == start + 24.0);
    view.handle_pointer(release(202, 50, MouseButton::Left, 5));

    // Frozen: a press reports the sample at once, moves do not scroll
    view.set_frozen(true);
    const double frozenAt = view.scroll_sample();
    r.clicks.clear();
    CHECK(view.handle_pointer(press(40, 50, MouseButton::Left)));
    CHECK(r.clicks == std::vector<int>{static_cast<int>(frozenAt + 20.0)});
    view.handle_pointer(move(0, 50, 50));
    view.handle_pointer(release(0, 50, MouseButton::Left, 50));
    view.handle_pointer(press(200, 50, MouseButton::Middle));
    view.handle_pointer(move(100, 50, 10));
    CHECK(view.scroll_sample() == frozenAt);
}

//=============================================================================================================

void test_ruler()
{
    CHECK(TraceView::ruler_snap(3, 2) == TraceView::RulerSnap::Free);
    CHECK(TraceView::ruler_snap(7, -7) == TraceView::RulerSnap::Free);
    CHECK(TraceView::ruler_snap(20, 5) == TraceView::RulerSnap::Horizontal);
    CHECK(TraceView::ruler_snap(-5, -20) == TraceView::RulerSnap::Vertical);
    CHECK(TraceView::ruler_snap(20, 15) == TraceView::RulerSnap::Free);

    Fixture f;
    TraceView& view = f.view;
    CHECK(!view.overlays_visible());
    CHECK(view.handle_pointer(press(100, 30, MouseButton::Right)));
    CHECK(f.recorder.stops == 1);
    CHECK(view.ruler().active && view.overlays_visible());

    // Mostly horizontal: the end keeps the start's y
    CHECK(view.handle_pointer(move(180, 36)));
    CHECK(view.ruler().snap == TraceView::RulerSnap::Horizontal && view.ruler().x1 == 180 && view.ruler().y1 == 30);
    // Mostly vertical: the end keeps the start's x
    view.handle_pointer(move(104, 90));
    CHECK(view.ruler().snap == TraceView::RulerSnap::Vertical && view.ruler().x1 == 100 && view.ruler().y1 == 90);
    // Diagonal: free
    view.handle_pointer(move(160, 75));
    CHECK(view.ruler().snap == TraceView::RulerSnap::Free && view.ruler().x1 == 160 && view.ruler().y1 == 75);
    // The release keeps the last axis lock
    CHECK(view.handle_pointer(release(161, 76, MouseButton::Right)));
    CHECK(!view.ruler().active && view.ruler().x1 == 161 && view.ruler().y1 == 76);
    CHECK(!view.overlays_visible());
    CHECK(f.recorder.redraws == 5);
}

//=============================================================================================================

void test_annotations()
{
    Fixture f;
    TraceView& view = f.view;
    Recorder& r = f.recorder;
    view.set_scroll_sample(100.0);
    view.set_last_file_sample(2099);
    // Samples 200 .. 219 lie between 200 px and 240 px
    view.set_annotations({{200, 219, {200, 0, 0}, "blink"}});

    bool isStart = false;
    CHECK(view.hit_test_annotation_boundary(203, isStart) == 0 && isStart);
    CHECK(view.hit_test_annotation_boundary(244.6, isStart) == 0 && !isStart);
    CHECK(view.hit_test_annotation_boundary(220, isStart) == -1);

    // Without annotation mode a left press on a boundary scrolls as usual
    view.handle_pointer(press(201, 50, MouseButton::Left));
    CHECK(!view.annotation_dragging());
    view.handle_pointer(release(201, 50, MouseButton::Left));
    r.clicks.clear();

    // Hovering a boundary shows the resize cursor, leaving it the default one
    view.set_annotation_selection_enabled(true);
    view.handle_pointer(move(240, 50));
    view.handle_pointer(move(241, 50));
    view.handle_pointer(move(300, 50));
    CHECK(r.cursors == (std::vector<CursorShape>{CursorShape::SizeHorizontal, CursorShape::Default}));

    // Dragging the start boundary edits the span live and reports the final sample
    CHECK(view.handle_pointer(press(201, 50, MouseButton::Left)));
    CHECK(view.annotation_dragging());
    CHECK(view.handle_pointer(move(180, 50)));
    CHECK(view.annotations()[0].start_sample == 190 && r.annotationChanges == 1);
    CHECK(view.handle_pointer(release(180, 50, MouseButton::Left)));
    CHECK(r.boundaries.size() == 1 && r.boundaries[0].index == 0 && r.boundaries[0].start && r.boundaries[0].sample == 190);
    CHECK(!view.annotation_dragging() && r.clicks.empty());

    // The end boundary is clamped to the file
    view.set_last_file_sample(250);
    view.handle_pointer(press(240, 50, MouseButton::Left));
    view.handle_pointer(move(399, 50));
    view.handle_pointer(release(399, 50, MouseButton::Left));
    CHECK(view.annotations()[0].end_sample == 250 && r.boundaries.back().sample == 250 && !r.boundaries.back().start);
    view.set_last_file_sample(2099);

    // A right drag selects a new range; tiny ones are ignored
    CHECK(view.handle_pointer(press(360, 50, MouseButton::Right)));
    CHECK(view.selection().active && !view.ruler().active);
    view.handle_pointer(move(300, 50));
    CHECK(view.selection().x1 == 300);
    CHECK(view.handle_pointer(release(300, 50, MouseButton::Right)));
    CHECK(!view.selection().active);
    CHECK(r.ranges == (std::vector<std::pair<int, int>>{{250, 280}}));
    view.handle_pointer(press(100, 50, MouseButton::Right));
    view.handle_pointer(release(103, 50, MouseButton::Right));
    CHECK(r.ranges.size() == 1);
}

//=============================================================================================================

void test_crosshair_and_double_click()
{
    Fixture f;
    TraceView& view = f.view;
    Recorder& r = f.recorder;
    view.set_scroll_sample(100.0);

    // Hover moves without the crosshair report nothing
    CHECK(!view.handle_pointer(move(40, 10)));
    CHECK(r.cursorData.empty() && !view.overlays_visible());

    CHECK(view.set_crosshair_enabled(true));
    CHECK(!view.handle_pointer(move(40, 30)));
    CHECK(view.crosshair_x() == 40 && view.crosshair_y() == 30 && view.overlays_visible());
    // Row 1 of 12 (25 px each), sample 120 = 20 ms after the first sample
    CHECK(r.cursorData.size() == 1);
    CHECK(r.cursorData[0].channel_name == "MEG 0112" && r.cursorData[0].unit_label == "T");
    CHECK(r.cursorData[0].time_sec == 0.02f && r.cursorData[0].amplitude == 2.f * 120.f * 1e-12f);

    // Butterfly: lanes are the channel types
    view.set_butterfly_mode(true);
    view.handle_pointer(move(40, 150));
    CHECK(r.cursorData.back().channel_name == "MEG grad" && r.cursorData.back().unit_label == "T/m");
    CHECK(r.cursorData.back().amplitude == 0.f);
    view.set_butterfly_mode(false);

    CHECK(view.set_crosshair_enabled(false));
    CHECK(view.crosshair_x() == -1 && !view.overlays_visible());

    // Double click on row 3 of the window starting at channel 5
    view.set_first_visible_channel(5);
    CHECK(view.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 10, 80, MouseButton::Left)));
    CHECK(r.doubleClicks == std::vector<int>{8});
    CHECK(!view.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 10, 80, MouseButton::Right)));
}

//=============================================================================================================

void test_overlay_golden()
{
    const ApproximateTextMetrics metrics;

    // Crosshair, scalebars and a free ruler
    Fixture f;
    TraceView& view = f.view;
    view.set_scroll_sample(100.0);
    view.set_crosshair_enabled(true);
    view.set_scalebars_visible(true);
    view.handle_pointer(move(60, 40));
    view.handle_pointer(press(150, 60, MouseButton::Right));
    view.handle_pointer(move(230, 110));
    CHECK(view.ruler().snap == TraceView::RulerSnap::Free && view.overlays_visible());
    const DisplayList overlays = view.render_overlays(metrics);
    CHECK(overlays.width == 400.0 && overlays.height == 300.0 && !overlays.background);
    CHECK(matches_golden(to_svg(overlays), GOLDEN_DIR "/trace_view_overlays.svg"));
    view.handle_pointer(release(230, 110, MouseButton::Right));

    // Annotation selection with its duration, and the scalebars of the butterfly lanes
    view.set_crosshair_enabled(false);
    view.set_butterfly_mode(true);
    view.set_annotation_selection_enabled(true);
    view.handle_pointer(press(100, 60, MouseButton::Right));
    view.handle_pointer(move(160, 60));
    const std::string svg = to_svg(view.render_overlays(metrics));
    CHECK(svg.find(">30 ms<") != std::string::npos);
    CHECK(matches_golden(svg, GOLDEN_DIR "/trace_view_selection.svg"));

    // Nothing to draw: an empty figure
    view.handle_pointer(release(160, 60, MouseButton::Right));
    view.set_scalebars_visible(false);
    CHECK(view.render_overlays(metrics).commands.empty());
}

//=============================================================================================================
// Trace geometry
//=============================================================================================================

// Five channels of 2000 samples from sample 100: MEG mag, MEG grad, MEG mag (bad), EEG, MEG grad
class GeometrySource : public TraceSource
{
public:
    GeometrySource()
    {
        const char* types[] = {"MEG mag", "MEG grad", "MEG mag", "EEG", "MEG grad"};
        for (int i = 0; i < 5; ++i) {
            Channel channel{"CH " + std::to_string(i), types[i], 1.f + static_cast<float>(i), Color{static_cast<std::uint8_t>(10 * i), 20, 30}, i == 2};
            m_channels.push_back(channel);
            std::vector<float> data;
            data.reserve(2000);
            for (int s = 0; s < 2000; ++s) {
                data.push_back(static_cast<float>(std::sin(0.05 * s + i)) * (1.f + static_cast<float>(i)));
            }
            m_data.push_back(std::move(data));
        }
        // Channel 3 is clipped: far beyond its scale
        std::fill(m_data[3].begin() + 1000, m_data[3].end(), -12.f);
    }

    int channel_count() const override
    {
        return static_cast<int>(m_channels.size());
    }

    std::optional<Channel> channel(int index) const override
    {
        if (index < 0 || index >= channel_count()) {
            return std::nullopt;
        }
        return m_channels[static_cast<std::size_t>(index)];
    }

    float value_at(int index, int sample) const override
    {
        return m_data[static_cast<std::size_t>(index)][static_cast<std::size_t>(sample - 100)];
    }

    int first_sample() const override
    {
        return 100;
    }

    int sample_count() const override
    {
        return 2000;
    }

    TraceVertices vertices(int index, int first, int last, int pixel_width) const override
    {
        m_requests.push_back({index, first, last, pixel_width});
        const int begin = std::clamp(first - 100, 0, 2000);
        const int end = std::clamp(last - 100, 0, 2000);
        TraceVertices result{100 + begin, {}};
        if (end > begin) {
            const std::vector<float>& data = m_data[static_cast<std::size_t>(index)];
            result.xy = decimate_trace(std::span<const float>(data).subspan(static_cast<std::size_t>(begin), static_cast<std::size_t>(end - begin)), pixel_width);
        }
        return result;
    }

    std::vector<Channel> m_channels;
    std::vector<std::vector<float>> m_data;
    mutable std::vector<std::array<int, 4>> m_requests;
};

void test_decimation()
{
    // Deterministic noise
    std::vector<float> samples;
    unsigned state = 12345;
    for (int i = 0; i < 1003; ++i) {
        state = state * 1103515245u + 12345u;
        samples.push_back(static_cast<float>((state >> 8) % 2001) / 1000.f - 1.f);
    }

    // More than two samples per pixel: max then min of every bin, at the bin start
    const int pixels = 100;
    const std::vector<float> envelope = decimate_trace(samples, pixels);
    CHECK(envelope.size() == static_cast<std::size_t>(pixels) * 4);
    const float spp = static_cast<float>(samples.size()) / static_cast<float>(pixels);
    bool allMatch = true;
    for (int px = 0; px < pixels; ++px) {
        const int begin = static_cast<int>(static_cast<float>(px) * spp);
        const int end = std::min(static_cast<int>(static_cast<float>(px + 1) * spp), static_cast<int>(samples.size()));
        float minV = samples[static_cast<std::size_t>(begin)];
        float maxV = minV;
        for (int s = begin; s < end; ++s) {
            minV = std::min(minV, samples[static_cast<std::size_t>(s)]);
            maxV = std::max(maxV, samples[static_cast<std::size_t>(s)]);
        }
        const std::size_t at = static_cast<std::size_t>(px) * 4;
        allMatch = allMatch && envelope[at] == static_cast<float>(px) * spp && envelope[at + 1] == maxV && envelope[at + 2] == envelope[at] &&
            envelope[at + 3] == minV;
    }
    CHECK(allMatch);
    // Every sample lies within the envelope of its bin: the first and last bins cover the window ends
    CHECK(envelope[1] >= samples[0] && envelope[3] <= samples[0]);

    // Up to two samples per pixel: every sample a vertex at its offset
    const std::vector<float> raw = decimate_trace(std::span<const float>(samples).first(200), 100);
    CHECK(raw.size() == 400 && raw[0] == 0.f && raw[1] == samples[0] && raw[398] == 199.f && raw[399] == samples[199]);

    // Detrending removes the mean or a straight line
    const std::vector<float> line = {3.f, 5.f, 7.f, 9.f, 11.f};
    const std::vector<float> demeaned = decimate_trace(line, 10, Detrend::Mean);
    CHECK(demeaned[1] == -4.f && demeaned[9] == 4.f);
    const std::vector<float> detrended = decimate_trace(line, 10, Detrend::Linear);
    bool flat = true;
    for (std::size_t i = 1; i < detrended.size(); i += 2) {
        flat = flat && std::abs(detrended[i]) < 1e-5f;
    }
    CHECK(flat);

    CHECK(decimate_trace(samples, 0).empty());
    CHECK(decimate_trace({}, 10).empty());

    // Z-scores; constant traces become 0, single vertices stay
    std::vector<float> xy = {0.f, 1.f, 1.f, 2.f, 2.f, 3.f, 3.f, 4.f};
    z_score(xy);
    CHECK(near(xy[1], -1.5 / std::sqrt(1.25), 1e-6) && near(xy[7], 1.5 / std::sqrt(1.25), 1e-6) && xy[0] == 0.f && xy[6] == 3.f);
    std::vector<float> constant = {0.f, 5.f, 1.f, 5.f};
    z_score(constant);
    CHECK(constant[1] == 0.f && constant[3] == 0.f);
    std::vector<float> single = {0.f, 5.f};
    z_score(single);
    CHECK(single[1] == 5.f);
}

void test_visible_channels()
{
    const auto isBad = [](int channel) {
        return channel == 1 || channel == 3;
    };
    CHECK(visible_channels(4, {}, false, isBad) == (std::vector<int>{0, 1, 2, 3}));
    CHECK(visible_channels(4, {}, true, isBad) == (std::vector<int>{0, 2}));
    const std::vector<int> filter = {3, -1, 2, 1};
    CHECK(visible_channels(4, filter, false, isBad) == filter);
    CHECK(visible_channels(4, filter, true, isBad) == (std::vector<int>{2}));
}

void test_streaming_window()
{
    GeometrySource source;
    TraceView view;
    view.set_source(&source);
    view.set_view_size(400, 300);
    view.set_samples_per_pixel(0.5);
    view.set_scroll_sample(1000.0);

    // The window spans one view width of 200 samples on each side, decimated at the view's scale
    TraceGeometry geometry;
    CHECK(geometry.needs_stream(view));
    CHECK(geometry.stream(view, source, false));
    CHECK(geometry.window_first() == 800 && geometry.window_last() == 1400);
    CHECK(source.m_requests.size() == 5 && source.m_requests[0] == (std::array<int, 4>{0, 800, 1400, 1200}));
    CHECK(geometry.channels().size() == 5 && geometry.channels()[4].first_sample == 800 && geometry.channels()[4].vertex_count() == 600);
    CHECK(!geometry.needs_stream(view));

    // Scrolling refetches only once the view comes closer than half the prefetch to a window edge
    view.set_scroll_sample(1001.0);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(1100.0);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(1101.0);
    CHECK(geometry.needs_stream(view));
    view.set_scroll_sample(900.0);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(899.0);
    CHECK(geometry.needs_stream(view));
    view.set_scroll_sample(1000.0);
    CHECK(!geometry.needs_stream(view));
    geometry.invalidate();
    CHECK(geometry.needs_stream(view));

    // At the data start the window is clamped to the loaded samples; there is nothing more to fetch on that side
    view.set_scroll_sample(100.0);
    geometry.stream(view, source, false);
    CHECK(geometry.window_first() == 100 && geometry.window_last() == 500);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(200.0);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(201.0);
    CHECK(geometry.needs_stream(view));
    // Near the end, too
    view.set_scroll_sample(2000.0);
    geometry.stream(view, source, false);
    CHECK(geometry.window_first() == 1800 && geometry.window_last() == 2100);
    CHECK(!geometry.needs_stream(view));
    view.set_scroll_sample(1899.0);
    CHECK(geometry.needs_stream(view));
    view.set_scroll_sample(2000.0);

    // Without data in the window nothing is fetched and the old window stays
    view.set_scroll_sample(5000.0);
    source.m_requests.clear();
    CHECK(!geometry.stream(view, source, false));
    CHECK(source.m_requests.empty() && geometry.window_first() == 1800);

    // Zoomed out beyond two samples per pixel the vertices are min/max pairs; z-scores are applied
    view.set_samples_per_pixel(4.0);
    view.set_scroll_sample(800.0);
    geometry.stream(view, source, true);
    CHECK(geometry.window_first() == 100 && geometry.window_last() == 2100);
    CHECK(geometry.channels()[0].vertex_count() == 1000);
    float maxAbs = 0.f;
    for (std::size_t i = 1; i < geometry.channels()[0].xy.size(); i += 2) {
        maxAbs = std::max(maxAbs, std::abs(geometry.channels()[0].xy[i]));
    }
    CHECK(maxAbs > 1.f && maxAbs < 2.f);
}

void test_draws()
{
    GeometrySource source;
    TraceView view;
    view.set_source(&source);
    view.set_view_size(400, 300);
    view.set_samples_per_pixel(0.5);
    view.set_scroll_sample(1000.0);
    view.set_visible_channel_count(3);
    view.set_first_visible_channel(1);
    TraceGeometry geometry;
    geometry.stream(view, source, false);
    TraceStyle style;
    style.background = {250, 250, 250};

    // Rows 1 .. 3 from the top; the bad channel 2 is red, channel 3's clipping is shown
    CHECK(TraceGeometry::slot_count(view, source) == 3);
    std::vector<TraceDraw> draws = geometry.draws(view, source, style);
    CHECK(draws.size() == 3 && draws[0].channel == 1 && draws[2].channel == 3);
    const TraceUniforms& top = draws[0].uniforms;
    CHECK(near(top.channel_y_center, 1.0 - 1.0 / 3.0, 1e-6) && near(top.channel_y_range, 2.0 / 3.0, 1e-6));
    CHECK(top.color[0] == 10.f / 255.f && top.color[3] == 1.f && top.amplitude_max == 2.f && top.show_clipping == 1.f);
    CHECK(top.first_sample == 800.f && top.scroll_sample == 1000.f && top.samples_per_pixel == 0.5f && top.view_width == 400.f &&
          top.view_height == 300.f);
    CHECK(draws[1].uniforms.color[0] == 200.f / 255.f && draws[1].uniforms.color[3] == 180.f / 255.f && draws[1].uniforms.show_clipping == 0.f);
    CHECK(near(draws[2].uniforms.channel_y_center, -2.0 / 3.0, 1e-6));

    // Hidden bad channels are flat and of the background colour
    style.hide_bad_channels = true;
    draws = geometry.draws(view, source, style);
    CHECK(draws[1].uniforms.channel_y_range == 0.f && draws[1].uniforms.color[0] == 250.f / 255.f);

    // Z-score: fixed scale, no clipping
    style.hide_bad_channels = false;
    style.z_score = true;
    draws = geometry.draws(view, source, style);
    CHECK(draws[0].uniforms.amplitude_max == 4.f && draws[0].uniforms.show_clipping == 0.f);
    style.z_score = false;

    // Butterfly: every channel in the lane of its type (mag, grad, EEG)
    view.set_butterfly_mode(true);
    CHECK(TraceGeometry::slot_count(view, source) == 5);
    draws = geometry.draws(view, source, style);
    CHECK(draws.size() == 5);
    const double laneRange = 2.0 / 3.0;
    const int lanes[] = {0, 1, 0, 2, 1};
    bool lanesMatch = true;
    for (int i = 0; i < 5; ++i) {
        lanesMatch = lanesMatch && draws[static_cast<std::size_t>(i)].channel == i &&
            near(draws[static_cast<std::size_t>(i)].uniforms.channel_y_center, 1.0 - laneRange * (lanes[i] + 0.5), 1e-6) &&
            near(draws[static_cast<std::size_t>(i)].uniforms.channel_y_range, laneRange, 1e-6);
    }
    CHECK(lanesMatch);
    const std::vector<TraceSource::Group> groups = source.groups();
    CHECK(groups.size() == 3 && groups[1].type_label == "MEG grad" && groups[1].amplitude_max == 2.f);

    // No channels: one slot, nothing to draw
    view.set_source(nullptr);
    GeometrySource empty;
    empty.m_channels.clear();
    CHECK(TraceGeometry::slot_count(view, empty) == 1);
    CHECK(geometry.draws(view, empty, style).empty());
}

void test_markers_and_figure_golden()
{
    const ApproximateTextMetrics metrics;

    // The overlay texture: 3 view widths from sample 600, at 2 px per sample
    TraceMarkers markers;
    markers.set_events({{1050, 1, {220, 0, 0}, "1"}, {5000, 2, {0, 0, 220}, "2"}});
    markers.set_epoch_markers({1100, 1150});
    const std::vector<TraceView::Annotation> annotations = {{1010, 1029, {0, 90, 200}, " blink "}, {990, 1001, {0, 160, 0}, ""}};
    Canvas overlay(600, 120);
    markers.draw(overlay, metrics, Font{}, annotations, 900.0, 2.0);
    const std::string svg = to_svg(overlay.take());
    CHECK(svg.find(">blink<") != std::string::npos);
    CHECK(matches_golden(svg, GOLDEN_DIR "/trace_markers.svg"));

    // Hidden kinds are left out
    CHECK(markers.set_events_visible(false) && !markers.set_events_visible(false));
    markers.set_annotations_visible(false);
    markers.set_epoch_markers_visible(false);
    Canvas none(600, 120);
    markers.draw(none, metrics, Font{}, annotations, 900.0, 2.0);
    CHECK(none.display_list().commands.empty());
    markers.set_events_visible(true);
    markers.set_annotations_visible(true);
    markers.set_epoch_markers_visible(true);

    // The image export: bands, grid, annotations, traces with a clipped segment, events and epochs
    GeometrySource source;
    TraceView view;
    view.set_source(&source);
    view.set_view_size(200, 120);
    view.set_samples_per_pixel(1.0);
    view.set_sfreq(100.0);
    view.set_first_file_sample(100);
    view.set_scroll_sample(960.0);
    view.set_visible_channel_count(4);
    view.set_annotations(annotations);
    TraceStyle style;
    style.background = Color::white();
    Canvas figure(200, 120, Color::white());
    draw_trace_figure(figure, metrics, Font{}, view, source, markers, style);
    CHECK(matches_golden(to_svg(figure.take()), GOLDEN_DIR "/trace_figure.svg"));

    // No rows: only the background
    view.set_source(nullptr);
    GeometrySource empty;
    empty.m_channels.clear();
    Canvas blank(200, 120, Color::white());
    draw_trace_figure(blank, metrics, Font{}, view, empty, markers, style);
    CHECK(blank.display_list().commands.empty());
}

//=============================================================================================================
// Animator
//=============================================================================================================

void test_animator()
{
    using Easing = ViewAnimator::Easing;
    CHECK(ViewAnimator::ease(Easing::OutCubic, 0.0) == 0.0 && ViewAnimator::ease(Easing::OutCubic, 1.0) == 1.0);
    CHECK(near(ViewAnimator::ease(Easing::OutCubic, 0.5), 0.875) && near(ViewAnimator::ease(Easing::OutCubic, 0.25), 1.0 - 0.75 * 0.75 * 0.75));
    CHECK(ViewAnimator::ease(Easing::Linear, 0.3) == 0.3 && ViewAnimator::ease(Easing::Linear, 2.0) == 1.0);

    // The view's animation requests drive the animator
    ViewAnimator animator;
    std::int64_t now = 0;
    TraceView view;
    TraceView::Effects effects;
    effects.animate_scroll = [&](const TraceView::ScrollAnimation& a) {
        animator.start_scroll(a, now);
    };
    effects.animate_zoom = [&](const TraceView::ZoomAnimation& a) {
        animator.start_zoom(a, now);
    };
    effects.stop_animations = [&] {
        animator.stop_scroll();
    };
    view.set_effects(effects);
    view.set_view_size(400, 300);
    view.set_samples_per_pixel(0.5);
    view.set_scroll_sample(1000.0);

    // A requested scroll eases out and lands exactly on its target
    view.scroll_to(1100.0, 100);
    CHECK(animator.running() && view.scroll_animation_running());
    now = 50;
    CHECK(animator.tick(now, view));
    CHECK(near(view.scroll_sample(), 1087.5));
    now = 100;
    CHECK(!animator.tick(now, view));
    CHECK(view.scroll_sample() == 1100.0 && !view.scroll_animation_running());

    // Wheel steps during a scroll add up to one target
    view.scroll_by(30.0, 100);
    now = 120;
    animator.tick(now, view);
    view.scroll_by(30.0, 100);
    CHECK(animator.scroll()->to == 1160.0 && animator.scroll()->start_ms == 120);
    now = 300;
    animator.tick(now, view);
    CHECK(view.scroll_sample() == 1160.0);

    // Interruption: stopping keeps the reached position
    view.scroll_to(1300.0, 100);
    now = 350;
    animator.tick(now, view);
    const double reached = view.scroll_sample();
    CHECK(reached > 1160.0 && reached < 1300.0);
    view.stop_scroll_animations();
    CHECK(!animator.running());
    now = 1000;
    CHECK(!animator.tick(now, view));
    CHECK(view.scroll_sample() == reached);

    // Inertia: a drag released at 0.75 samples/ms glides on at that speed, slowing down to rest after 500 ms
    view.set_scroll_sample(1000.0);
    now = 2000;
    view.handle_pointer(press(100, 50, MouseButton::Left, 2000));
    view.handle_pointer(move(90, 50, 2020));
    view.handle_pointer(move(50, 50, 2040));
    view.handle_pointer(move(10, 50, 2060));
    now = 2060;
    view.handle_pointer(release(10, 50, MouseButton::Left, 2060));
    CHECK(animator.scroll() && animator.scroll()->inertial && animator.scroll()->duration_ms == 500);
    const double start = view.scroll_sample();
    animator.tick(now + 1, view);
    const double firstStep = view.scroll_sample() - start;
    CHECK(near(firstStep, 0.75, 0.01));
    animator.tick(now + 450, view);
    const double late = view.scroll_sample();
    animator.tick(now + 451, view);
    CHECK(view.scroll_sample() - late < 0.01 && view.scroll_sample() - late > 0.0);
    CHECK(!animator.tick(now + 500, view));
    CHECK(near(view.scroll_sample(), start + 125.0));

    // A press stops a glide
    view.handle_pointer(release(10, 50, MouseButton::Left, 2060));
    animator.start_scroll({view.scroll_sample(), view.scroll_sample() + 100.0, 500, true}, now);
    view.handle_pointer(press(10, 50, MouseButton::Left, now));
    CHECK(!animator.running());
    view.handle_pointer(release(10, 50, MouseButton::Left, now));

    // Zoom eases to its target; a new zoom replaces the running one
    view.zoom_to(0.4, 150);
    view.zoom_to(0.25, 150);
    CHECK(animator.zoom()->to == 0.25);
    now += 150;
    CHECK(!animator.tick(now, view));
    CHECK(view.samples_per_pixel() == 0.25);

    // A zero duration jumps on the first tick
    animator.start_zoom({0.25, 1.0, 0}, now);
    CHECK(!animator.tick(now, view) && view.samples_per_pixel() == 1.0);
}

void test_trace_frame()
{
    const ApproximateTextMetrics metrics;
    GeometrySource source;
    TraceView view;
    view.set_source(&source);
    view.set_view_size(240, 120);
    view.set_samples_per_pixel(1.0);
    view.set_sfreq(100.0);
    view.set_first_file_sample(100);
    view.set_scroll_sample(960.0);
    view.set_visible_channel_count(4);
    view.set_annotations({{1010, 1029, {0, 90, 200}, " blink "}});
    view.set_scalebars_visible(true);
    TraceMarkers markers;
    markers.set_events({{1050, 1, {220, 0, 0}, "1"}});
    markers.set_epoch_markers({1100});
    TraceStyle style;
    style.background = Color::white();

    // One pass cleared to the background: four traces, the marker overlay with the bands and the scalebars
    TraceFrameBuilder builder;
    GpuFrameTarget target;
    target.pixel_width = 240;
    target.pixel_height = 120;
    GpuFrame frame = builder.build(view, source, style, markers, metrics, target);
    CHECK(frame.pixel_width == 240 && frame.passes.size() == 1 && frame.passes[0].clear_color == style.background);
    CHECK(frame.passes[0].draws.size() == 6 && builder.hud_texture() != 0);
    const GpuResources& resources = builder.resources();
    CHECK(resources.pipeline(frame.passes[0].draws[0].pipeline)->shader == "channeldata");
    CHECK(resources.pipeline(frame.passes[0].draws[4].pipeline)->shader == "overlay");
    CHECK(frame.passes[0].draws[5].textures[0].texture == builder.hud_texture());
    SoftwareRenderer renderer;
    CHECK(matches_golden_png(renderer.render(frame, resources), GOLDEN_DIR "/trace_frame.png"));

    // A small scroll reuses the vertices and the marker texture; the scalebars are drawn again
    const GpuHandle vertices = frame.passes[0].draws[0].vertex_buffers[0];
    const GpuHandle overlay = frame.passes[0].draws[4].textures[0].texture;
    const std::uint64_t vertexRevision = resources.buffer(vertices)->revision;
    const std::uint64_t overlayRevision = resources.texture(overlay)->revision;
    const std::uint64_t hudRevision = resources.texture(builder.hud_texture())->revision;
    view.set_scroll_sample(990.0);
    frame = builder.build(view, source, style, markers, metrics, target);
    CHECK(resources.buffer(vertices)->revision == vertexRevision && resources.texture(overlay)->revision == overlayRevision);
    CHECK(resources.texture(builder.hud_texture())->revision > hudRevision);

    // A zoom or invalidate_overlay() draws the marker texture again; at twice the pixel ratio it has twice the pixels
    view.set_samples_per_pixel(2.0);
    builder.geometry().invalidate();
    target.pixel_width = 480;
    target.pixel_height = 240;
    target.device_pixel_ratio = 2.0;
    frame = builder.build(view, source, style, markers, metrics, target);
    CHECK(resources.buffer(vertices)->revision > vertexRevision && resources.texture(overlay)->revision > overlayRevision);
    CHECK(resources.texture(overlay)->pixel_width() == 3 * 240 * 2 && resources.texture(overlay)->pixel_height() == 240);
    const std::uint64_t zoomedRevision = resources.texture(overlay)->revision;
    builder.invalidate_overlay();
    builder.build(view, source, style, markers, metrics, target);
    CHECK(resources.texture(overlay)->revision > zoomedRevision);

    // Without overlays there is no scalebar draw
    view.set_scalebars_visible(false);
    CHECK(builder.build(view, source, style, markers, metrics, target).passes[0].draws.size() == 5);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"clamping", test_clamping},
        {"wheel", test_wheel},
        {"drag_scroll", test_drag_scroll},
        {"ruler", test_ruler},
        {"annotations", test_annotations},
        {"crosshair_and_double_click", test_crosshair_and_double_click},
        {"overlay_golden", test_overlay_golden},
        {"decimation", test_decimation},
        {"visible_channels", test_visible_channels},
        {"streaming_window", test_streaming_window},
        {"draws", test_draws},
        {"markers_and_figure_golden", test_markers_and_figure_golden},
        {"animator", test_animator},
        {"trace_frame", test_trace_frame},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
