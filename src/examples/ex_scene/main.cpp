//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Scene library: draw a figure on a Canvas, plot a line chart, and write both as SVG and PNG without Qt.
 *
 * Every step checks its result and the example exits non-zero on a mismatch.
 * All files are written to a temporary directory.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/scene/average_layout.h>
#include <mne/scene/canvas.h>
#include <mne/scene/filter_response_plot.h>
#include <mne/scene/line_chart.h>
#include <mne/scene/raster_renderer.h>
#include <mne/scene/sensor_layout.h>
#include <mne/scene/settings_form.h>
#include <mne/scene/svg_renderer.h>
#include <mne/scene/text_metrics.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <print>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::scene;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * Draws a figure and a line plot and writes them as SVG and PNG files.
 *
 * @return EXIT_SUCCESS if every step gave the expected result.
 */
int main()
{
    bool ok = true;
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "mne_cpp_ex_scene";
    std::filesystem::create_directories(dir);

    //! [scene_usage]
    // Qt:   QPainter p(&widget); p.setPen(...); p.drawRect(...); p.drawText(...);
    Canvas canvas(200, 120, Color::white());
    canvas.set_antialiasing(true);
    canvas.set_pen(Pen{{50, 100, 150}, 1.5});
    canvas.set_brush(Brush::solid({70, 130, 180}));
    canvas.draw_rounded_rect({20, 20, 80, 50}, 6, 6);
    canvas.set_pen(Color::black());
    canvas.draw_text(Rect{0, 80, 200, 30}, Align::Center, "Hello scene");
    const DisplayList figure = canvas.take();

    // A plot: the same model a disp widget shows on screen
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < 100; ++i) {
        x.push_back(i * 0.01);
        y.push_back(std::sin(2 * 3.14159265358979 * 5 * x.back()));
    }
    LineChart plot;
    plot.set_title("5 Hz sine");
    plot.set_x_label("time (s)");
    plot.add_series(x, y, "signal");
    const ApproximateTextMetrics metrics;
    const DisplayList chart = plot.render(480, 320, metrics, Font{"", 0.0, 13.0});

    const Expected<void> svg = write_svg(chart, dir / "plot.svg");
    const Expected<void> png = to_raster(chart).save_png(dir / "plot.png");
    //! [scene_usage]

    //! [filter_response_usage]
    // Magnitude of a 5-pole Butterworth low pass at 30 Hz, sampled at 250 Hz
    std::vector<double> magnitude(501);
    for (std::size_t i = 0; i < magnitude.size(); ++i) {
        magnitude[i] = 1.0 / std::sqrt(1.0 + std::pow(static_cast<double>(i) * 0.25 / 30.0, 10));
    }
    FilterResponsePlot response;
    response.set_title("Low pass 30 Hz");
    response.set_response(magnitude, 125.0);
    response.set_cutoffs({30.0});
    const DisplayList responseFigure = response.render(480, 320, metrics, Font{"", 0.0, 13.0});
    //! [filter_response_usage]
    ok &= expect(std::abs(response.magnitude_db()[120] + 3.0103) < 1e-3 && !responseFigure.commands.empty(),
                 "the filter response is -3 dB at its cut-off");
    ok &= expect(figure.commands.size() == 2 && svg && png && std::filesystem::file_size(dir / "plot.svg") > 0,
                 "a figure was recorded and a line plot written as SVG and PNG");
    ok &= expect(to_svg(figure).find("Hello scene") != std::string::npos, "the figure's text is in its SVG document");
    ok &= expect(to_raster(figure).pixel(60, 45) == Rgb{70, 130, 180}, "the figure's rectangle is filled in its image");

    //! [sensor_layout_usage]
    // Three sensors of an .lout layout; the click selects the middle one, as in the channel selection view
    SensorLayout sensors;
    sensors.set_sensors({{"MEG 0111", {0.0, 0.0}}, {"MEG 0121", {6.0, 0.0}}, {"MEG 0131", {12.0, 0.0}}});
    sensors.click(sensors.scene_position(1), false);
    Canvas sensorCanvas(240, 100, Color::white());
    sensorCanvas.translate(40, 60);
    sensors.draw(sensorCanvas, metrics, Font{"", 0.0, 13.0});
    //! [sensor_layout_usage]
    ok &= expect(sensors.selected() == std::vector<std::size_t>{1}, "a click selects the sensor under it");

    //! [average_layout_usage]
    // One tile per channel at its layout position, as plot_evoked_topo arranges them
    AverageTile tile;
    tile.box = AverageLayout::box_at({0.5, 0.25});
    tile.name = "EEG 001";
    tile.columns = 100;
    tile.traces.push_back({std::vector<double>(100, 0.5), Color{0, 80, 180}});
    AverageLayout topo;
    topo.set_tiles({tile});
    //! [average_layout_usage]
    ok &= expect(topo.tile_at(topo.tiles()[0].box.center()) == 0, "the tile is found at its centre");

    //! [settings_form_usage]
    // A settings panel described once; any toolkit builds its widgets from it (disp::SettingsPanel for Qt)
    SettingsForm form("Averaging");
    form.add_int("trials", "Number of trials", 10, 1, 1000, 1, {}, "General");
    form.add_bool("baseline", "Baseline correction", false, "Baseline");
    SettingField from;
    from.key = "baseline_from";
    from.kind = SettingField::Kind::Int;
    from.label = "From";
    from.unit = " ms";
    from.minimum = -1000;
    from.maximum = 0;
    from.group = "Baseline";
    from.enabled_if = "baseline";
    form.add(from);
    int heard = 0;
    form.add_listener([&heard](const std::string&, const std::optional<SettingValue>&) {
        ++heard;
    });
    form.set_value("trials", 5000);
    //! [settings_form_usage]
    ok &= expect(form.get<int>("trials") == 1000 && heard == 1, "a value beyond the range is clamped and heard once");
    ok &= expect(!form.is_enabled("baseline_from"), "the baseline range waits for the baseline switch");

    std::error_code removeError;
    std::filesystem::remove_all(dir, removeError);
    ok &= expect(!removeError, std::format("the example's files were removed{}", removeError ? ": " + removeError.message() : std::string()));
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
