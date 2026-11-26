//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_render.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the display list, the canvas and the SVG and raster renderers of mne_scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../common/golden.h"
#include <mne/viz/figures/average_layout.h>
#include <mne/viz/figures/average_tile.h>
#include <mne/viz/figures/bar_chart.h>
#include <mne/viz/figures/butterfly_plot.h>
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/colormap.h>
#include <mne/viz/figures/filter_response_plot.h>
#include <mne/viz/figures/image_plot.h>
#include <mne/viz/figures/line_chart.h>
#include <mne/viz/figures/raster_renderer.h>
#include <mne/viz/figures/sensor_layout.h>
#include <mne/viz/figures/svg_renderer.h>
#include <mne/viz/figures/text_metrics.h>
#include <mne/viz/figures/spline_plot.h>
#include <mne/viz/figures/ticks.h>
#include <mne/viz/figures/vector_plot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTransform>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

namespace
{

// A figure with every kind of command: shapes, a gradient, clipped and rotated text, an image
DisplayList sampleFigure()
{
    Canvas canvas(160, 100, Color::white());
    canvas.set_antialiasing(true);

    canvas.set_brush(Brush::solid({70, 130, 180}));
    canvas.set_pen(Pen{{50, 100, 150}, 1.0});
    canvas.draw_rect({10, 10, 40, 30});

    canvas.set_pen(Pen{{196, 78, 82}, 2.5, LineStyle::Solid, LineCap::Round, LineJoin::Round});
    const std::array<Point, 4> line = {{{10, 90}, {40, 55}, {70, 80}, {100, 50}}};
    canvas.draw_polyline(line);

    canvas.set_pen(Pen{{85, 168, 104}, 1.5, LineStyle::Dash, LineCap::Flat});
    canvas.draw_line({60, 10}, {150, 10});

    canvas.set_pen(Pen::none());
    canvas.set_brush(Brush::linear_gradient({110, 0}, {150, 0}, {{0.0, {0, 0, 255}}, {0.5, {255, 255, 255}}, {1.0, {255, 0, 0}}}));
    canvas.draw_rect({110, 20, 40, 20});

    canvas.set_pen(Pen{{0, 0, 0}, 1.0});
    canvas.set_brush(Brush::solid({255, 255, 255, 225}));
    canvas.draw_rounded_rect({60, 25, 40, 20}, 4, 4);
    canvas.set_brush(Brush::solid({218, 139, 195}));
    canvas.draw_ellipse({130, 75}, 12, 8);

    auto image = std::make_shared<Image>(Image::transparent(2, 2));
    image->set_pixel(0, 0, {255, 0, 0});
    image->set_pixel(1, 0, {0, 255, 0});
    image->set_pixel(0, 1, {0, 0, 255});
    image->set_pixel(1, 1, {255, 255, 0});
    canvas.draw_image({110, 50, 16, 16}, image);

    canvas.set_pen(Color{51, 51, 51});
    canvas.set_font(Font{"", 0.0, 12.0});
    canvas.draw_text(Rect{60, 25, 40, 20}, Align::Center, "a<b");
    canvas.save();
    canvas.clip_rect({0, 0, 80, 100});
    canvas.translate(6, 50);
    canvas.rotate(-90);
    canvas.draw_text(Rect{-30, 0, 60, 12}, Align::Center, "volts");
    canvas.restore();
    canvas.draw_text(Point{12, 54}, "x=1");
    return canvas.take();
}

// The plots of mne_scene with fixed data, as the disp widgets show them
DisplayList plotFigure(const QString& name)
{
    const ApproximateTextMetrics metrics;
    const Font font{"", 0.0, 13.0};
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> y2;
    for (int i = 0; i < 50; ++i) {
        x.push_back(i * 0.1);
        y.push_back(std::sin(i * 0.2));
        y2.push_back(std::cos(i * 0.15) * 0.5);
    }
    if (name == "line_chart") {
        LineChart plot;
        plot.set_title("Line plot");
        plot.set_x_label("time (s)");
        plot.set_y_label("amplitude");
        plot.add_series(x, y, "sine");
        plot.add_series(x, y2, "cosine", std::nullopt, LineStyle::Dash);
        return plot.render(480, 320, metrics, font);
    }
    if (name == "bar_chart") {
        BarChart chart;
        chart.set_title("Histogram");
        chart.set_x_label("bins");
        chart.set_y_label("count");
        chart.set_bars({"0 to 1", "1 to 2", "2 to 3", "3 to 4", "a long category label"}, {3, 7, 12, 5, 1});
        return chart.render(480, 320, metrics, font);
    }
    if (name == "spline_plot") {
        SplinePlot plot;
        plot.set_title("Spline");
        plot.set_x_label("value");
        plot.set_y_label("count");
        plot.set_classes({{0.5, 3}, {1.5, 7}, {2.5, 12}, {3.5, 5}, {4.5, 1}}, 0.0, 5.0);
        plot.set_thresholds(1.5, 2.5, 3.5);
        plot.set_colormap("Jet");
        return plot.render(480, 320, metrics, font);
    }
    if (name == "image_plot") {
        ImagePlot plot;
        std::vector<double> values;
        for (int r = 0; r < 20; ++r) {
            for (int c = 0; c < 30; ++c) {
                values.push_back(std::sin(r * 0.3) * std::cos(c * 0.2));
            }
        }
        plot.set_data(20, 30, values);
        plot.set_colormap("Viridis");
        plot.set_extent(0.0, 2.9, 5.0, 40.0);
        plot.set_title("ImagePlot");
        plot.set_x_label("Time [s]");
        plot.set_y_label("Frequency [Hz]");
        plot.set_colorbar_label("value");
        return plot.render(480, 320, metrics, font);
    }
    if (name == "butterfly_plot") {
        ButterflyPlot plot;
        plot.set_timing(120, 30, 600.0f);
        plot.set_baseline(-0.05f, 0.0f);
        plot.set_background({250, 250, 240});
        std::vector<ButterflyPlot::Channel> channels;
        for (int c = 0; c < 6; ++c) {
            ButterflyPlot::Channel channel;
            channel.bad = c == 5;
            std::vector<double> samples;
            samples.reserve(120);
            for (int i = 0; i < 120; ++i) {
                samples.push_back(i < 30 ? 0.0 : (0.2 + 0.1 * c) * std::sin((i - 30) * 0.08) * std::exp(-(i - 30) * 0.02));
            }
            channel.traces.push_back({samples, map_color(Colormap::Jet, c / 5.0)});
            channels.push_back(channel);
        }
        plot.set_channels(channels);
        return plot.render(480, 320, font);
    }
    if (name == "filter_response_plot") {
        // Band pass 1 to 40 Hz at 250 Hz sampling: a smooth pass band and a notch above the upper edge
        std::vector<double> magnitude;
        magnitude.reserve(2001);
        for (int i = 0; i <= 2000; ++i) {
            const double f = i * 125.0 / 2000;
            const double band = 1.0 / std::sqrt(1.0 + std::pow(1.0 / std::max(f, 1e-3), 8)) / std::sqrt(1.0 + std::pow(f / 40.0, 8));
            magnitude.push_back(i == 1000 ? 0.0 : band);
        }
        FilterResponsePlot plot;
        plot.set_title("BPF_1_40");
        plot.set_response(magnitude, 125.0);
        plot.set_cutoffs({1.0, 40.0, 200.0});
        plot.set_colors({30, 30, 30});
        return plot.render(480, 320, metrics, font);
    }
    if (name == "sensor_layout") {
        SensorLayout layout;
        std::vector<SensorLayout::Sensor> sensors;
        for (int i = 0; i < 9; ++i) {
            SensorLayout::Sensor sensor;
            sensor.name = "MEG " + std::to_string(111 + i);
            const int row = i / 3; // rows of three
            sensor.position = {(i % 3) * 6.0, -row * 6.0};
            sensors.push_back(sensor);
        }
        sensors[1].bad = true;
        sensors[2].selectable = false;
        sensors[4].selected = true;
        sensors[5].highlighted = true;
        sensors[8].visible = false;
        layout.set_sensors(sensors);
        Canvas canvas(480, 320, Color::white());
        canvas.translate(120, 60);
        canvas.scale(1.5, 1.5);
        layout.draw(canvas, metrics, font);
        return canvas.take();
    }
    if (name == "average_layout") {
        std::vector<AverageTile> tiles;
        for (int t = 0; t < 4; ++t) {
            AverageTile tile;
            const int row = t / 2; // two rows
            tile.box = AverageLayout::box_at({(t % 2) * 1.0, -row * 0.6});
            tile.name = "EEG 00" + std::to_string(t + 1);
            tile.bad = t == 3;
            tile.stim_sample = -20;
            tile.columns = 100;
            AverageTile::Trace trace;
            trace.color = {0, 80, 180};
            for (int i = 0; i < 100; ++i) {
                trace.samples.push_back((0.4 + 0.2 * t) * std::sin(i * 0.12));
            }
            tile.traces.push_back(trace);
            tiles.push_back(tile);
        }
        AverageLayout layout;
        layout.set_tiles(tiles);
        const Rect bounds = layout.bounds();
        Canvas canvas(480, 320, Color::white());
        const double zoom = std::min(460 / bounds.width, 300 / bounds.height);
        canvas.translate(240 - bounds.center().x * zoom, 160 - bounds.center().y * zoom);
        canvas.scale(zoom, zoom);
        layout.draw(canvas, font);
        return canvas.take();
    }
    if (name == "average_tile") {
        Canvas canvas(480, 320, Color::white());
        for (int t = 0; t < 2; ++t) {
            AverageTile tile;
            tile.box = {-60, -30, 120, 60};
            tile.name = t == 0 ? "MEG 0111" : "MEG 0113";
            tile.name_size = 6;
            tile.bad = t == 1;
            tile.stim_sample = -20;
            tile.columns = 120;
            for (int k = 0; k < 2; ++k) {
                AverageTile::Trace trace;
                trace.color = k == 0 ? Color{20, 120, 40} : Color{0, 0, 255};
                for (int i = 0; i < 120; ++i) {
                    trace.samples.push_back((k == 0 ? 1.3 : 0.6) * std::sin(i * 0.1 + k));
                }
                tile.traces.push_back(trace);
            }
            canvas.save();
            canvas.translate(120 + 240 * t, 160);
            canvas.scale(1.8, 1.8);
            tile.draw(canvas, font);
            canvas.restore();
        }
        return canvas.take();
    }
    VectorPlot plot;
    plot.set_data(y);
    plot.set_title("Plot");
    plot.set_x_label("x");
    plot.set_y_label("y");
    return plot.render(480, 320, metrics);
}

QTransform qt(const Transform& t)
{
    return QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy);
}

} // namespace

//=============================================================================================================
/**
 * @brief Tests mne_scene's canvas and renderers.
 */
class TestSceneRender : public QObject
{
    Q_OBJECT

private slots:
    void transformsComposeLikeQTransform();
    void canvasRecordsStateWithEachCommand();
    void invisibleDrawingRecordsNothing();
    void gradientInterpolatesBetweenStops();
    void approximateMetricsScaleWithTheFont();
    void ticksFallOnRoundNumbers();
    void svgMatchesGoldenFile();
    void svgEscapesTextAndWritesFiles();
    void rasterMatchesGoldenFile();
    void rasterFillsWholePixelsOfAlignedRects();
    void rasterClipsShapesAndText();
    void rasterScalesTheFigure();
    void cubicPathsFlattenAndExport();
    void colormapsMatchTheirDefinition();
    void plotsMatchGoldenFiles_data();
    void plotsMatchGoldenFiles();
    void linePlotPicksUnusedPaletteColors();
    void filterResponseIsRelativeToItsMaximum();
    void rectsContainIntersectAndUnite();
    void sensorLayoutSelectsLikeAGraphicsScene();
    void averageLayoutPlacesTilesOnTheLayout();
    void splinePlotMapsPixelsAndData();
};

//=============================================================================================================

void TestSceneRender::transformsComposeLikeQTransform()
{
    Canvas canvas(10, 10);
    canvas.translate(5, 7);
    canvas.rotate(30);
    canvas.scale(2, 0.5);
    canvas.translate(-1, 3);

    QTransform expected;
    expected.translate(5, 7);
    expected.rotate(30);
    expected.scale(2, 0.5);
    expected.translate(-1, 3);

    const Transform t = canvas.transform();
    QCOMPARE(qt(t).m11(), expected.m11());
    QVERIFY(qFuzzyCompare(qt(t).m12(), expected.m12()));
    QVERIFY(qFuzzyCompare(qt(t).m21(), expected.m21()));
    QVERIFY(qFuzzyCompare(qt(t).m22(), expected.m22()));
    QVERIFY(qFuzzyCompare(qt(t).dx(), expected.dx()));
    QVERIFY(qFuzzyCompare(qt(t).dy(), expected.dy()));

    const Point p = t.map({3, 4});
    const QPointF q = expected.map(QPointF(3, 4));
    QVERIFY(std::abs(p.x - q.x()) < 1e-12 && std::abs(p.y - q.y()) < 1e-12);
    const Point back = t.inverted().map(p);
    QVERIFY(std::abs(back.x - 3) < 1e-12 && std::abs(back.y - 4) < 1e-12);

    // Quarter turns are exact
    QCOMPARE(Transform::rotation(-90).map({1, 0}), (Point{0, -1}));
    QCOMPARE(Transform::rotation(180).map({1, 2}), (Point{-1, -2}));
}

//=============================================================================================================

void TestSceneRender::canvasRecordsStateWithEachCommand()
{
    Canvas canvas(100, 50);
    canvas.set_pen(Color{1, 2, 3});
    canvas.save();
    canvas.translate(10, 20);
    canvas.clip_rect({0, 0, 5, 5});
    canvas.set_antialiasing(true);
    canvas.set_pen(Pen{{9, 9, 9}, 3.0});
    canvas.draw_line({0, 0}, {1, 1});
    canvas.restore();
    canvas.draw_text(Rect{0, 0, 10, 10}, Align::Right | Align::VCenter, "t");

    const DisplayList list = canvas.take();
    QCOMPARE(list.width, 100.0);
    QCOMPARE(list.commands.size(), std::size_t(2));

    const Command& line = list.commands[0];
    QCOMPARE(line.transform, Transform::translation(10, 20));
    QCOMPARE(line.clips.size(), std::size_t(1));
    QCOMPARE(line.clips[0].transform, Transform::translation(10, 20));
    QVERIFY(line.antialias);
    QCOMPARE(std::get<LineShape>(line.shape).pen.width, 3.0);

    const Command& text = list.commands[1];
    QVERIFY(text.transform.identity());
    QVERIFY(text.clips.empty());
    QVERIFY(!text.antialias);
    const TextShape& shape = std::get<TextShape>(text.shape);
    QCOMPARE(shape.color, (Color{1, 2, 3}));
    QVERIFY(has_flag(shape.align, Align::Right));
    QVERIFY(!has_flag(shape.align, Align::HCenter));

    // take() leaves an empty canvas of the same size
    QVERIFY(canvas.display_list().commands.empty());
    QCOMPARE(canvas.width(), 100.0);
}

//=============================================================================================================

void TestSceneRender::invisibleDrawingRecordsNothing()
{
    Canvas canvas(10, 10);
    canvas.set_pen(Pen::none());
    canvas.draw_line({0, 0}, {5, 5});
    canvas.draw_rect({0, 0, 5, 5});
    canvas.fill_rect({0, 0, 5, 5}, Brush{});
    canvas.draw_text(Rect{0, 0, 5, 5}, Align::Left, "");
    canvas.draw_image({0, 0, 5, 5}, nullptr);
    QVERIFY(canvas.display_list().commands.empty());
}

//=============================================================================================================

void TestSceneRender::gradientInterpolatesBetweenStops()
{
    const Brush brush = Brush::linear_gradient({0, 0}, {1, 0}, {{0.25, {0, 0, 0}}, {0.75, {200, 100, 50, 0}}});
    QCOMPARE(brush.color_at(0.0), (Color{0, 0, 0}));
    QCOMPARE(brush.color_at(0.5), (Color{100, 50, 25, 128}));
    QCOMPARE(brush.color_at(2.0), (Color{200, 100, 50, 0}));
}

//=============================================================================================================

void TestSceneRender::approximateMetricsScaleWithTheFont()
{
    const ApproximateTextMetrics metrics;
    QCOMPARE(metrics.pixel_size(Font{}), 13.0);
    QCOMPARE(metrics.pixel_size(Font{"", 9.0}), 12.0);
    QCOMPARE(metrics.advance(Font{"", 0.0, 10.0}, "abcd"), 24.0);
    QCOMPARE(metrics.advance(Font{"", 0.0, 10.0}, "äö"), 12.0);
    QCOMPARE(metrics.ascent(Font{"", 0.0, 10.0}), 8.0);
    QCOMPARE(metrics.height(Font{"", 0.0, 10.0}), 10.0);
}

//=============================================================================================================

void TestSceneRender::ticksFallOnRoundNumbers()
{
    QCOMPARE(nice_ticks(0.0, 1.0, 5), (std::vector<double>{0.0, 0.2, 0.4, 0.6, 0.8, 1.0}));
    QCOMPARE(nice_ticks(-200.0, 500.0, 8), (std::vector<double>{-200.0, -100.0, 0.0, 100.0, 200.0, 300.0, 400.0, 500.0}));
    QCOMPARE(nice_ticks(-3.0, 17.0, 4), (std::vector<double>{0.0, 5.0, 10.0, 15.0}));
    // Index axes of a 3 x 3 image span [-0.5, 2.5]: whole numbers only
    QCOMPARE(nice_ticks(-0.5, 2.5, 8, 1.0), (std::vector<double>{0.0, 1.0, 2.0}));
    QCOMPARE(nice_ticks(0.0, 3.0, 10, 1.0), (std::vector<double>{0.0, 1.0, 2.0, 3.0}));
    // A step of 1 would put ticks closer than minStep: the next nice step up
    QCOMPARE(nice_ticks(0.0, 10.0, 20, 1.43), (std::vector<double>{0.0, 2.0, 4.0, 6.0, 8.0, 10.0}));
    QVERIFY(nice_ticks(1.0, 1.0, 5).empty());
    QCOMPARE(tick_label(0.6), std::string("0.6"));
    QCOMPARE(tick_label(1234567.0), std::string("1.23457e+06"));
}

//=============================================================================================================

void TestSceneRender::svgMatchesGoldenFile()
{
    SvgOptions options;
    options.title = "Scene sample";
    QVERIFY2(golden::matches_svg(to_svg(sampleFigure(), options), GOLDEN_DIR "/sample.svg"), golden::last_message().constData());
}

//=============================================================================================================

void TestSceneRender::svgEscapesTextAndWritesFiles()
{
    Canvas canvas(10, 10);
    canvas.draw_text(Point{0, 5}, "<&\">");
    const std::string svg = to_svg(canvas.display_list());
    QVERIFY(svg.find("&lt;&amp;&quot;&gt;") != std::string::npos);

    QTemporaryDir dir;
    const std::filesystem::path path = dir.filePath("figure.svg").toStdU16String();
    QVERIFY(write_svg(canvas.display_list(), path));
    QFile file(dir.filePath("figure.svg"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll().toStdString(), svg);
    QVERIFY(!write_svg(canvas.display_list(), std::filesystem::path(dir.path().toStdU16String()) / "missing" / "figure.svg"));
}

//=============================================================================================================

void TestSceneRender::rasterMatchesGoldenFile()
{
    QVERIFY2(golden::matches_png(to_raster(sampleFigure()), GOLDEN_DIR "/sample.png"), golden::last_message().constData());
}

//=============================================================================================================

void TestSceneRender::rasterFillsWholePixelsOfAlignedRects()
{
    Canvas canvas(8, 8, Color::black());
    canvas.fill_rect({2, 3, 4, 2}, Brush::solid({255, 255, 255}));
    canvas.fill_rect({0, 0, 2, 2}, Brush::solid({255, 0, 0, 128}));
    const RasterImage image = to_raster(canvas.display_list());
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const bool white = x >= 2 && x < 6 && y >= 3 && y < 5;
            const bool red = x < 2 && y < 2;
            QCOMPARE(image.pixel(x, y), (white ? Rgb{255, 255, 255} : red ? Rgb{128, 0, 0}
                                                                          : Rgb{}));
        }
    }
}

//=============================================================================================================

void TestSceneRender::rasterClipsShapesAndText()
{
    Canvas canvas(20, 20, Color::white());
    canvas.clip_rect({0, 0, 10, 20});
    canvas.fill_rect({0, 0, 20, 20}, Brush::solid({0, 0, 0}));
    canvas.set_font(Font{"", 0.0, 24.0});
    canvas.draw_text(Rect{0, 0, 20, 20}, Align::Left | Align::Top, "WW");
    const RasterImage image = to_raster(canvas.display_list());
    for (int y = 0; y < 20; ++y) {
        QCOMPARE(image.pixel(5, y), (Rgb{0, 0, 0}));
        QCOMPARE(image.pixel(15, y), (Rgb{255, 255, 255}));
    }
}

//=============================================================================================================

void TestSceneRender::rasterScalesTheFigure()
{
    Canvas canvas(4, 3, Color{10, 20, 30});
    canvas.fill_rect({1, 1, 1, 1}, Brush::solid({255, 255, 255}));
    RasterOptions options;
    options.scale = 2.0;
    const RasterImage image = to_raster(canvas.display_list(), options);
    QCOMPARE(image.width(), 8);
    QCOMPARE(image.height(), 6);
    QCOMPARE(image.pixel(2, 2), (Rgb{255, 255, 255}));
    QCOMPARE(image.pixel(3, 3), (Rgb{255, 255, 255}));
    QCOMPARE(image.pixel(4, 4), (Rgb{10, 20, 30}));
}

//=============================================================================================================

void TestSceneRender::cubicPathsFlattenAndExport()
{
    Canvas canvas(40, 20, Color::white());
    canvas.set_pen(Pen{Color::black(), 2});
    const std::array<Point, 4> curve = {{{2, 18}, {12, -10}, {28, 30}, {38, 2}}};
    canvas.draw_bezier(curve);
    // Not 3 n + 1 points: nothing drawn
    canvas.draw_bezier(std::span(curve).first(3));
    QCOMPARE(canvas.display_list().commands.size(), std::size_t(1));
    QVERIFY(to_svg(canvas.display_list()).find("<path d=\"M2,18 C12,-10 28,30 38,2\"") != std::string::npos);

    // The curve passes through its midpoint B(0.5) = (20, 10)
    const RasterImage image = to_raster(canvas.display_list());
    QVERIFY(image.pixel(20, 9).r < 128);
    QCOMPARE(image.pixel(20, 18), (Rgb{255, 255, 255}));
}

//=============================================================================================================

void TestSceneRender::colormapsMatchTheirDefinition()
{
    QCOMPARE(map_color(Colormap::Jet, 0.5), (Color{127, 255, 127}));
    QCOMPARE(map_color(Colormap::Jet, 0.0), (Color{0, 0, 127}));
    QCOMPARE(map_color(Colormap::Hot, 1.0), (Color{255, 255, 255}));
    QCOMPARE(map_color(Colormap::RedBlue, 0.0), (Color{255, 255, 255}));
    QCOMPARE(map_color(Colormap::Viridis, 0.0), (Color{68, 1, 84}));
    QCOMPARE(map_color(Colormap::ViridisNegated, 0.0), (Color{253, 231, 37}));
    QCOMPARE(map_color(Colormap::Viridis, 1.5), Color::transparent());
    // By name: RedBlue takes [0, 1] like the others, unknown names are Jet
    QCOMPARE(map_color("RedBlue", 0.5), (Color{255, 255, 255}));
    QCOMPARE(map_color("Unknown", 0.5), map_color(Colormap::Jet, 0.5));
    QVERIFY(!colormap_from_name("jet"));
}

//=============================================================================================================

void TestSceneRender::plotsMatchGoldenFiles_data()
{
    QTest::addColumn<QString>("name");
    for (const char* name : {"line_chart", "bar_chart", "spline_plot", "image_plot", "vector_plot", "butterfly_plot", "average_tile", "filter_response_plot", "sensor_layout", "average_layout"}) {
        QTest::newRow(name) << QString(name);
    }
}

//=============================================================================================================

void TestSceneRender::plotsMatchGoldenFiles()
{
    QFETCH(QString, name);
    const DisplayList figure = plotFigure(name);
    QVERIFY(!figure.commands.empty());
    QVERIFY2(golden::matches_svg(to_svg(figure), QString(GOLDEN_DIR "/%1.svg").arg(name)), golden::last_message().constData());
    QVERIFY2(golden::matches_png(to_raster(figure), QString(GOLDEN_DIR "/%1.png").arg(name)), golden::last_message().constData());
}

//=============================================================================================================

void TestSceneRender::linePlotPicksUnusedPaletteColors()
{
    LineChart plot;
    plot.add_series({0, 1}, {0, 1}, "a", Color{70, 130, 180});
    plot.add_series({0, 1}, {1, 0}, "b");
    plot.add_series({0, 1, 2}, {1, 2});
    QCOMPARE(plot.series().size(), std::size_t(3));
    QCOMPARE(plot.series()[1].color, (Color{221, 132, 82}));
    // The shorter of x and y sets the length
    QCOMPARE(plot.series()[2].x.size(), std::size_t(2));
    plot.set_data({4, 5, 6});
    QCOMPARE(plot.series().size(), std::size_t(1));
    QCOMPARE(plot.series()[0].x, (std::vector<double>{0, 1, 2}));
}

//=============================================================================================================

void TestSceneRender::filterResponseIsRelativeToItsMaximum()
{
    FilterResponsePlot plot;
    plot.set_response({2.0, 1.0, 0.02, 0.0}, 50.0);
    const std::vector<double> db = plot.magnitude_db();
    QCOMPARE(db.size(), std::size_t{4});
    QCOMPARE(db[0], 0.0);
    QVERIFY(std::abs(db[1] + 20.0 * std::log10(2.0)) < 1e-12);
    QVERIFY(std::abs(db[2] + 40.0) < 1e-9);
    QCOMPARE(db[3], -100.0);
    plot.set_floor_db(-30.0);
    QCOMPARE(plot.magnitude_db()[2], -30.0);
    QVERIFY(FilterResponsePlot{}.magnitude_db().empty());
}

//=============================================================================================================

void TestSceneRender::rectsContainIntersectAndUnite()
{
    const Rect a{0, 0, 10, 10};
    QVERIFY(a.contains({10, 10}) && !a.contains({10.5, 5}));
    QVERIFY(a.intersects({10, 10, 5, 5}) && !a.intersects({11, 0, 5, 5}) && !a.intersects(Rect{}));
    QCOMPARE(a.united({20, -5, 5, 5}), (Rect{0, -5, 25, 15}));
    QCOMPARE(Rect{}.united(a), a);
    QCOMPARE(a.united(Rect{}), a);
}

//=============================================================================================================

void TestSceneRender::sensorLayoutSelectsLikeAGraphicsScene()
{
    SensorLayout layout;
    std::vector<SensorLayout::Sensor> sensors(4);
    for (int i = 0; i < 4; ++i) {
        sensors[i].name = "S" + std::to_string(i);
        sensors[i].position = {i * 10.0, 0.0};
    }
    sensors[3].selectable = false;
    layout.set_sensors(sensors);
    QCOMPARE(layout.scene_position(2), (Point{200, 0}));
    QCOMPARE(layout.sensor_rect(0), (Rect{-25, -30, 50, 50}));
    QCOMPARE(layout.bounds(), (Rect{-25, -30, 350, 50}));

    // A click selects one sensor, Ctrl+click toggles, a click beside the sensors clears
    QVERIFY(layout.click({100, 0}, false));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({1}));
    QVERIFY(layout.click({0, 0}, true));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({0, 1}));
    QVERIFY(layout.click({0, 0}, true));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({1}));
    QVERIFY(!layout.click({100, 0}, false));
    QCOMPARE(layout.sensor_at({300, 0}), -1); // not selectable
    QVERIFY(layout.click({300, 0}, false));
    QVERIFY(layout.selected().empty());

    // A band replaces the selection, with Ctrl it adds; hidden and unselectable sensors stay out
    QVERIFY(layout.select_in({90, -5, 200, 10}, false));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({1, 2}));
    layout.sensors()[1].visible = false;
    QVERIFY(layout.select_in({-10, -5, 20, 10}, true));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({0, 1, 2}));
    QVERIFY(layout.select_in({-10, -5, 400, 10}, false));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({0, 2}));
    QCOMPARE(layout.bounds(), (Rect{-25, -30, 350, 50}));

    QVERIFY(layout.select_names({"S3", "S1"}));
    QCOMPARE(layout.selected(), std::vector<std::size_t>({1, 3}));
    QVERIFY(!layout.select_names({"S3", "S1"}));
}

//=============================================================================================================

void TestSceneRender::averageLayoutPlacesTilesOnTheLayout()
{
    QCOMPARE(AverageLayout::box_at({1.0, 0.5}), (Rect{100, -110, 120, 60}));
    AverageLayout layout;
    QVERIFY(layout.bounds().empty());
    std::vector<AverageTile> tiles(2);
    tiles[0].box = AverageLayout::box_at({0, 0});
    tiles[1].box = AverageLayout::box_at({0.25, 0});
    layout.set_tiles(tiles);
    QCOMPARE(layout.tile_at({-40, 0}), 0);
    QCOMPARE(layout.tile_at({0, 0}), 1); // the later tile lies on top
    QCOMPARE(layout.tile_at({0, 40}), -1);
    QCOMPARE(layout.bounds(), (Rect{-60, -33, 160, 63}));
}

//=============================================================================================================

void TestSceneRender::splinePlotMapsPixelsAndData()
{
    SplinePlot plot;
    plot.set_classes({{1, 4}, {3, 8}}, 0.0, 4.0);
    QVERIFY(!plot.has_thresholds());
    QCOMPARE(plot.middle_threshold(), 4.0);
    // 400 pixels wide: the axis spans 60 to 380
    QCOMPARE(plot.data_to_pixel_x(2.0, 400), 220.0);
    QCOMPARE(plot.pixel_to_data_x(220.0, 400), 2.0);
    // 300 pixels high: 210 pixels of plot below a margin of 30, the largest count at 1 / 1.1 of it
    QCOMPARE(plot.data_to_pixel_y(0.0, 300), 240.0);
    QVERIFY(std::abs(plot.data_to_pixel_y(8.8, 300) - 30.0) < 1e-9);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestSceneRender)
#include "test_scene_render.moc"
