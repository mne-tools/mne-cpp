//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_trace_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the viewport, input handling and overlays of mne::scene::TraceView, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/scene/canvas.h>
#include <mne/scene/svg_renderer.h>
#include <mne/scene/text_metrics.h>
#include <mne/scene/trace_view.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
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

// 30 channels: three MEG magnetometers, then gradiometers and EEG; value = index + sample / 1000
class FakeSource : public TraceSource
{
public:
    FakeSource()
    {
        for (int i = 0; i < 30; ++i) {
            if (i < 3) {
                m_channels.push_back({"MEG 01" + std::to_string(11 + i), "MEG mag", 2e-12f});
            } else if (i < 20) {
                m_channels.push_back({"MEG 02" + std::to_string(i), "MEG grad", 4e-11f});
            } else {
                m_channels.push_back({"EEG 0" + std::to_string(i), "EEG", 5e-5f});
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
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
