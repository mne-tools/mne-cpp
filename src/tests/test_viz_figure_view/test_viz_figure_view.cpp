//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_figure_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests FigureView, the toolkit-free host of interactive figures, the sensor and average layout figures
 *           and AverageLayoutPanel, with synthetic input and a golden image.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../common/scene_golden.h"
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/viz/figures/average_layout_figure.h>
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/channel_scale.h>
#include <mne/viz/figures/font_library.h>
#include <mne/viz/figures/font_text_metrics.h>
#include <mne/viz/figures/sensor_layout_figure.h>
#include <mne/viz/gpu/software_renderer.h>
#include <mne/viz/host/surface.h>
#include <mne/viz/panels/average_layout_panel.h>
#include <mne/viz/panels/figure_view.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

class CountingSurface : public Surface
{
public:
    void request_update() override
    {
        ++updates;
    }

    std::int64_t now_ms() const override
    {
        return 0;
    }

    int updates = 0;
};

/** A 100 x 50 rectangle at the origin that records what reaches it. */
class RecordingFigure : public SceneFigure
{
public:
    Rect bounds() const override
    {
        return area;
    }

    void draw(Canvas& canvas, const TextMetrics& /*metrics*/, const Font& /*font*/) const override
    {
        canvas.set_brush(Brush::solid(Color{200, 0, 0}));
        canvas.draw_rect(area);
    }

    bool zoomable() const override
    {
        return zoom;
    }

    void resize(double width, double height) override
    {
        size = {width, height};
        if (!zoom) {
            area = {0, 0, width, 3 * height};
        }
    }

    bool press(Point point, Modifier /*modifiers*/) override
    {
        presses.push_back(point);
        return area.contains(point) && takes_press;
    }

    void release(Point point) override
    {
        releases.push_back(point);
    }

    void hover(std::optional<Point> point) override
    {
        hovered = point;
    }

    bool double_click(Point /*point*/) override
    {
        return takes_double_click;
    }

    bool band_selection_enabled() const override
    {
        return true;
    }

    void band_selected(const Rect& rect, Modifier modifiers) override
    {
        band = rect;
        band_modifiers = modifiers;
    }

    void grow()
    {
        area.width *= 2;
        changed(Change::Bounds);
    }

    Rect area{0, 0, 100, 50};
    bool zoom = true;
    bool takes_press = false;
    bool takes_double_click = false;
    Point size;
    std::vector<Point> presses;
    std::vector<Point> releases;
    std::optional<Point> hovered;
    std::optional<Rect> band;
    Modifier band_modifiers = Modifier::None;
};

PointerEvent pointer(PointerEvent::Type type, double x, double y, MouseButton button = MouseButton::None,
                     Modifier modifiers = Modifier::None)
{
    PointerEvent event;
    event.type = type;
    event.x = x;
    event.y = y;
    event.button = button;
    event.modifiers = modifiers;
    return event;
}

WheelEvent wheel(double x, double y, double delta)
{
    WheelEvent event;
    event.x = x;
    event.y = y;
    event.angle_delta_y = delta;
    return event;
}

void drag(FigureView& view, MouseButton button, Point from, Point to, Modifier modifiers = Modifier::None)
{
    view.handle_pointer(pointer(PointerEvent::Type::Press, from.x, from.y, button, modifiers));
    view.handle_pointer(pointer(PointerEvent::Type::Move, to.x, to.y));
    view.handle_pointer(pointer(PointerEvent::Type::Release, to.x, to.y, button));
}

//=============================================================================================================

void test_zoom_pan_and_fit()
{
    RecordingFigure figure;
    FigureView view(figure);
    CountingSurface surface;
    view.attach(&surface);

    // A 100 x 50 figure fills a 204 x 104 view but for the 2 px margin: 2 px per unit, centred
    view.set_view_size(204, 104);
    CHECK(view.fitted() && near(view.zoom(), 2.0));
    CHECK(near(view.map_from_scene({50, 25}).x, 102) && near(view.map_from_scene({50, 25}).y, 52));

    // One notch in zooms by 1.15 around the pointer, one notch out undoes it
    const Point anchor = view.map_to_scene({40, 30});
    CHECK(view.handle_wheel(wheel(40, 30, 120)));
    CHECK(!view.fitted() && near(view.zoom(), 2.3));
    CHECK(near(view.map_from_scene(anchor).x, 40) && near(view.map_from_scene(anchor).y, 30));
    view.handle_wheel(wheel(40, 30, -120));
    CHECK(near(view.zoom(), 2.0));

    // The right button pans
    const Point before = view.map_from_scene({0, 0});
    drag(view, MouseButton::Right, {100, 100}, {130, 80});
    CHECK(near(view.map_from_scene({0, 0}).x - before.x, 30) && near(view.map_from_scene({0, 0}).y - before.y, -20));

    // A resize keeps the zoom of a figure that is not fitted; a double click fits again
    view.set_view_size(404, 204);
    CHECK(near(view.zoom(), 2.0));
    CHECK(view.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 1, 1, MouseButton::Left)));
    CHECK(view.fitted() && near(view.zoom(), 4.0));

    // A figure that grows is refitted and repainted
    const int updates = surface.updates;
    figure.grow();
    CHECK(near(view.zoom(), 2.0) && surface.updates > updates);

    // A figure that takes the double click is not refitted
    view.handle_wheel(wheel(10, 10, 120));
    figure.takes_double_click = true;
    view.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 1, 1, MouseButton::Left));
    CHECK(!view.fitted());
}

//=============================================================================================================

void test_clicks_hovers_and_bands()
{
    RecordingFigure figure;
    FigureView view(figure);
    view.set_view_size(204, 104);

    // Hovers reach the figure in scene coordinates, and so does leaving the view
    view.handle_pointer(pointer(PointerEvent::Type::Move, 102, 52));
    CHECK(figure.hovered && near(figure.hovered->x, 50) && near(figure.hovered->y, 25));
    view.leave();
    CHECK(!figure.hovered);

    // A press the figure takes is released to it and spans no band
    figure.takes_press = true;
    drag(view, MouseButton::Left, {22, 12}, {42, 22});
    CHECK(figure.presses.size() == 1 && near(figure.presses[0].x, 10) && near(figure.presses[0].y, 5));
    CHECK(figure.releases.size() == 1 && near(figure.releases[0].x, 20) && !figure.band);

    // Otherwise a left drag spans a band, drawn while it is spanned and reported in scene coordinates
    figure.takes_press = false;
    view.handle_pointer(pointer(PointerEvent::Type::Press, 62, 42, MouseButton::Left, Modifier::Ctrl));
    view.handle_pointer(pointer(PointerEvent::Type::Move, 22, 2));
    CHECK(view.band() && near(view.band()->x, 22) && near(view.band()->width, 40));
    view.handle_pointer(pointer(PointerEvent::Type::Release, 22, 2, MouseButton::Left));
    CHECK(!view.band());
    CHECK(figure.band && near(figure.band->x, 10) && near(figure.band->y, 0) && near(figure.band->width, 20) &&
          near(figure.band->height, 20));
    CHECK(figure.band_modifiers == Modifier::Ctrl);
}

//=============================================================================================================

void test_scrolling_figure()
{
    // A figure that is not zoomable lays itself out to the view and only scrolls
    RecordingFigure figure;
    figure.zoom = false;
    FigureView view(figure);
    view.set_view_size(200, 100);
    CHECK(near(figure.size.x, 200) && near(figure.size.y, 100));
    CHECK(near(view.zoom(), 1.0) && near(view.map_to_scene({10, 10}).y, 10));

    // A notch scrolls 48 px; the scroll stops at both ends of the 300 px tall figure
    CHECK(view.handle_wheel(wheel(10, 10, -120)));
    CHECK(near(view.map_to_scene({10, 10}).y, 58));
    view.handle_wheel(wheel(10, 10, -1200));
    CHECK(near(view.map_to_scene({0, 0}).y, 200));
    CHECK(!view.handle_wheel(wheel(10, 10, -120)));
    view.handle_wheel(wheel(10, 10, 2400));
    CHECK(near(view.map_to_scene({0, 0}).y, 0));
    view.handle_wheel(wheel(10, 10, 120));
    CHECK(near(view.zoom(), 1.0));

    // A picture lays the figure out to its own size and restores the view's layout afterwards
    const DisplayList picture = view.picture(50, 40);
    CHECK(!picture.commands.empty());
    CHECK(near(figure.size.x, 200));
}

//=============================================================================================================

void test_sensor_layout_figure()
{
    SensorLayoutFigure figure;
    int changes = 0;
    figure.set_selection_callback([&changes]() {
        ++changes;
    });
    figure.set_layout({{"MEG 0111", {0, 0}}, {"MEG 0121", {10, 0}}, {"EEG 001", {5, -5}}}, {"MEG 0121"});
    CHECK(figure.sensor_layout().sensors().size() == 3);
    CHECK(figure.sensor_layout().sensors()[0].kind == FIFFV_EEG_CH && figure.sensor_layout().sensors()[2].bad);

    // Without a selection, all shown sensors are the selection
    CHECK(figure.selection().size() == 3);
    FigureView view(figure);
    view.set_view_size(300, 300);
    const auto at = [&view](double x, double y) {
        return view.map_from_scene({10 * x, -10 * y});
    };
    const Point first = at(0, 0);
    view.handle_pointer(pointer(PointerEvent::Type::Press, first.x, first.y, MouseButton::Left));
    view.handle_pointer(pointer(PointerEvent::Type::Release, first.x, first.y, MouseButton::Left));
    CHECK(figure.selected_names() == std::vector<std::string>({"MEG 0111"}));
    CHECK(changes == 1 && figure.selection().size() == 1);

    // Ctrl+band adds to the selection
    drag(view, MouseButton::Left, at(6, 6), at(12, -1), Modifier::Ctrl);
    CHECK(figure.selected_names() == std::vector<std::string>({"MEG 0111", "MEG 0121"}));

    // Showing only some MEG sensors drops the others from the selection
    figure.set_channel_kind(FIFFV_MEG_CH);
    figure.set_selectable({"MEG 0111"});
    CHECK(figure.selected_names() == std::vector<std::string>({"MEG 0111"}));
    figure.highlight_names({"MEG 0121"});
    CHECK(figure.sensor_layout().sensors()[2].highlighted);
    figure.clear();
    CHECK(figure.sensor_layout().sensors().empty() && changes == 4);
}

//=============================================================================================================

void test_channel_scale()
{
    CHECK(default_channel_scale(FIFFV_MEG_CH, FIFF_UNIT_T_M) == 1e-15f);
    CHECK(default_channel_scale(FIFFV_MEG_CH, FIFF_UNIT_T) == 1e-12f);
    CHECK(default_channel_scale(FIFFV_EEG_CH, 0) == 1e-5f);
    CHECK(default_channel_scale(-1, 0) == 1e-9f);
    // MEG channels are keyed by unit, the others by kind; zero and NaN fall back to the default
    const std::map<int, float> scales = {{FIFF_UNIT_T, 2e-12f}, {FIFFV_EEG_CH, 0.0f}, {FIFFV_EOG_CH, std::numeric_limits<float>::quiet_NaN()}};
    CHECK(channel_scale(scales, FIFFV_MEG_CH, FIFF_UNIT_T) == 2e-12f);
    CHECK(channel_scale(scales, FIFFV_EEG_CH, 0) == 1e-5f);
    CHECK(channel_scale(scales, FIFFV_EOG_CH, 0) == 1e-6f);
}

//=============================================================================================================

void test_average_layout_panel()
{
    // Three channels: MEG 0111 (row 0), MEG 0112 (row 1, bad), EEG 001 (row 2)
    auto info = std::make_shared<mne::FiffInfo>();
    const char* names[] = {"MEG 0111", "MEG 0112", "EEG 001"};
    for (int i = 0; i < 3; ++i) {
        mne::FiffChInfo ch;
        ch.ch_name = names[i];
        ch.kind = i < 2 ? FIFFV_MEG_CH : FIFFV_EEG_CH;
        ch.unit = i < 2 ? FIFF_UNIT_T : FIFF_UNIT_V;
        info->chs.push_back(ch);
        info->ch_names.push_back(names[i]);
    }
    info->nchan = 3;
    Eigen::MatrixXd aud = Eigen::MatrixXd::Zero(3, 40);
    aud.row(0).tail(20).setConstant(0.5e-12);
    aud.row(2).tail(20).setConstant(-0.5e-5);

    AverageLayoutPanel panel;
    CHECK(panel.figure().get() == &panel.layout());
    CHECK(panel.form().fields().empty());

    // The selection names the layout positions; without info nothing is drawn yet
    std::vector<AverageLayoutFigure::Channel> selection(2);
    selection[0].name = "MEG 0111";
    selection[0].position = {-1, 0};
    selection[1].name = "EEG 001";
    selection[1].position = {1, 0};
    panel.set_selection(selection);
    EvokedAverages evoked;
    evoked.averages = {{"aud", aud.data(), 40, 3}};
    evoked.first_sample = -20;
    evoked.bad = {false, true, false};
    panel.set_evoked(evoked);
    CHECK(panel.layout().channels()[0].averages.empty());

    // The lookup gives the rows, the info the kinds and units
    panel.set_info(info);
    panel.set_channel_lookup([&info](const std::string& name) {
        for (std::size_t i = 0; i < info->ch_names.size(); ++i) {
            if (info->ch_names[i] == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    });
    const AverageLayoutFigure& layout = panel.layout();
    CHECK(layout.channels()[1].number == 2 && layout.channels()[1].kind == FIFFV_EEG_CH);
    CHECK(layout.channels()[1].averages.size() == 1 && layout.channels()[1].averages[0].data == aud.data() + 2);
    CHECK(layout.channels()[0].first_sample == -20 && !layout.channels()[0].bad);

    // The tiles take the scale of their channel kind and the colour of their average
    panel.set_scale_map({{FIFF_UNIT_T, 1e-12f}, {FIFFV_EEG_CH, 1e-5f}});
    panel.set_average_colors({{"aud", Color{255, 0, 0}}});
    AverageLayout tiles = layout.average_layout();
    CHECK(tiles.tiles().size() == 2 && near(tiles.tiles()[0].max_value, 1e-12, 1e-20));
    CHECK(tiles.tiles()[0].traces.size() == 1 && tiles.tiles()[0].traces[0].color == (Color{255, 0, 0}));
    CHECK(near(tiles.tiles()[0].traces[0].samples.back(), 0.5e-12, 1e-20));
    CHECK(near(tiles.tiles()[1].traces[0].samples.back(), -0.5e-5, 1e-12));

    panel.set_single_average_color(Color{0, 0, 0});
    CHECK(panel.average_colors().at("aud") == (Color{0, 0, 0}) && layout.signal_color() == (Color{0, 0, 0}));
    panel.set_average_activation({{"aud", false}});
    CHECK(layout.average_layout().tiles()[0].traces.empty() && !panel.average_activation().at("aud"));

    // A bad row is marked; clearing the evoked set keeps the tiles but drops the averages
    panel.set_channel_lookup([](const std::string& name) {
        return name == "MEG 0111" ? 1 : -1;
    });
    CHECK(layout.channels()[0].bad && layout.channels()[1].averages.empty());
    panel.clear_evoked();
    CHECK(layout.channels().size() == 2 && layout.channels()[0].averages.empty());

    // Pressing a tile enlarges its name until the release
    FigureView view(panel.layout());
    view.set_view_size(400, 200);
    const Point centre = view.map_from_scene(AverageLayout::box_at({-1, 0}).center());
    view.handle_pointer(pointer(PointerEvent::Type::Press, centre.x, centre.y, MouseButton::Left));
    CHECK(near(layout.average_layout().tiles()[0].name_size, 6.0));
    view.handle_pointer(pointer(PointerEvent::Type::Release, centre.x, centre.y, MouseButton::Left));
    CHECK(near(layout.average_layout().tiles()[0].name_size, 3.0));
}

//=============================================================================================================

void test_golden()
{
    // A sensor layout with a selection and a rubber band, through the GPU frame of the view
    SensorLayoutFigure figure;
    std::map<std::string, Point> layout;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            layout.emplace("MEG " + std::to_string(row * 4 + column + 1), Point{8.0 * column, -8.0 * row});
        }
    }
    figure.set_layout(layout, {"MEG 6"});
    figure.select_names({"MEG 1", "MEG 2"});
    figure.highlight_names({"MEG 12"});

    const auto fonts = std::make_shared<FontLibrary>();
    const FontTextMetrics metrics(fonts);
    FigureView view(figure);
    view.set_text_metrics(&metrics);
    GpuFrameTarget target;
    target.pixel_width = 320;
    target.pixel_height = 240;
    const GpuFrame frame = view.frame(target);
    CHECK(frame.passes.size() == 1 && frame.passes[0].draws.size() == 1);
    // A band from the gap between the sensors over the lower right ones
    const Point from = view.map_from_scene({40, 120});
    const Point to = view.map_from_scene({230, 190});
    view.handle_pointer(pointer(PointerEvent::Type::Press, from.x, from.y, MouseButton::Left, Modifier::Ctrl));
    view.handle_pointer(pointer(PointerEvent::Type::Move, to.x, to.y));
    CHECK(view.band().has_value());

    SoftwareRenderer renderer;
    renderer.set_fonts(fonts);
    const Image image = renderer.render(view.frame(target), view.resources());
    CHECK(image.width == 320 && image.pixel(1, 1) == (Color{255, 255, 255}));
    CHECK(scene_golden::matches_png(image, GOLDEN_DIR "/figure_view.png"));

    // Averages of a 3 x 2 grid of EEG sensors, one of them bad
    auto info = std::make_shared<mne::FiffInfo>();
    Eigen::MatrixXd evoked = Eigen::MatrixXd::Zero(6, 200);
    std::vector<AverageLayoutFigure::Channel> selection;
    for (int i = 0; i < 6; ++i) {
        mne::FiffChInfo ch;
        ch.ch_name = "EEG " + std::to_string(i + 1);
        ch.kind = FIFFV_EEG_CH;
        info->chs.push_back(ch);
        for (int s = 50; s < 200; ++s) {
            evoked(i, s) = std::sin((s - 50) / 25.0) * (i + 1) * 1.5e-6;
        }
        AverageLayoutFigure::Channel tile;
        tile.name = ch.ch_name;
        tile.number = i;
        const int row = i / 3;
        tile.position = {1.4 * (i % 3), -1.0 * row};
        selection.push_back(std::move(tile));
    }
    AverageLayoutPanel panel;
    panel.set_info(info);
    panel.set_selection(std::move(selection));
    panel.set_scale_map({{FIFFV_EEG_CH, 1e-5f}});
    EvokedAverages averages;
    averages.averages = {{"aud", evoked.data(), 200, 6}};
    averages.first_sample = -50;
    averages.bad = {false, false, false, false, true, false};
    panel.set_evoked(std::move(averages));
    FigureView averageView(*panel.figure());
    averageView.set_text_metrics(&metrics);
    const Image tiles = renderer.render(averageView.frame(target), averageView.resources());
    CHECK(scene_golden::matches_png(tiles, GOLDEN_DIR "/average_layout_panel.png"));
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    test_zoom_pan_and_fit();
    test_clicks_hovers_and_bands();
    test_scrolling_figure();
    test_sensor_layout_figure();
    test_channel_scale();
    test_average_layout_panel();
    test_golden();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("All figure view tests passed\n");
    return 0;
}
