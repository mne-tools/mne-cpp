//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Disp plots: colour maps, bar and spline histograms, line plots, image and time-frequency maps.
 *
 * Colour maps are compared with matplotlib 3.10; every widget is rendered
 * offscreen with QWidget::grab() and its pixels are checked against the data
 * it was given. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/plots/bar.h>
#include <mne/viz/backends/qt/plots/filterplot.h>
#include <mne/viz/backends/qt/plots/graph.h>
#include <mne/viz/backends/qt/plots/helpers/colormap.h>
#include <mne/viz/backends/qt/plots/helpers/figureexporter.h>
#include <mne/viz/backends/qt/plots/helpers/svgwriter.h>
#include <mne/viz/backends/qt/plots/imagesc.h>
#include <mne/viz/backends/qt/plots/lineplot.h>
#include <mne/viz/backends/qt/plots/plot.h>
#include <mne/viz/backends/qt/plots/spline.h>
#include <mne/viz/backends/qt/plots/tfplot.h>
#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <print>
#include <string_view>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;

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

bool close(QRgb a, QRgb b, int tol)
{
    return std::abs(qRed(a) - qRed(b)) <= tol && std::abs(qGreen(a) - qGreen(b)) <= tol && std::abs(qBlue(a) - qBlue(b)) <= tol;
}

/** Number of pixels in @p image within @p tol of @p color. */
int countColor(const QImage& image, QRgb color, int tol)
{
    int n = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            n += close(image.pixel(x, y), color, tol);
        }
    }
    return n;
}

/** Renders @p widget at 400 x 300 without showing it on screen. */
QImage render(QWidget& widget)
{
    widget.resize(400, 300);
    widget.setAttribute(Qt::WA_DontShowOnScreen);
    widget.show();
    QApplication::processEvents();
    return widget.grab().toImage();
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    bool ok = true;
    const QRgb steelBlue = qRgb(70, 130, 180);

    //! [colormap_usage]
    const QRgb low = ColorMap::valueToColor(0.0, "Viridis"); // v in [0, 1]
    const QRgb mid = ColorMap::valueToColor(0.5, "Viridis");
    const QRgb jet = ColorMap::valueToJet(0.25);
    //! [colormap_usage]
    // matplotlib.colormaps["viridis"](0, 0.5, 1) and ["jet"](0.25, 0.5), ["cool"](0.5); bone is MNE-C's piecewise ramp, not matplotlib's
    ok &= expect(close(low, qRgb(68, 1, 84), 1) && close(mid, qRgb(33, 145, 140), 1) && close(ColorMap::valueToColor(1.0, "Viridis"), qRgb(253, 231, 37), 1) && close(jet, qRgb(0, 128, 255), 1) && close(ColorMap::valueToJet(0.5), qRgb(125, 255, 122), 5) && close(ColorMap::valueToCool(0.5), qRgb(128, 127, 255), 1),
                 "ColorMap viridis, jet and cool match matplotlib to 1-5 levels");

    //! [bar_usage]
    Bar bar("Amplitude histogram");
    VectorXd classLimits(5); // class edges
    classLimits << 0.0, 1.0, 2.0, 3.0, 4.0;
    VectorXi counts(4); // counts per class
    counts << 5, 20, 10, 15;
    bar.setData(classLimits, counts, 2); // 2 digits on the axis labels
    bar.setXLabel("Amplitude");
    bar.setYLabel("Count");
    //! [bar_usage]
    const QImage barImage = render(bar);
    // Plot height 300 - 40 - 100 = 160 px and the tallest bar (20 counts) fills it, so heights follow the counts
    QList<int> heights;
    for (int b = 0; b < 4; ++b) {
        int h = 0;
        for (int py = 0; py < barImage.height(); ++py) {
            h += close(barImage.pixel(60 + 80 * b + 40, py), steelBlue, 2);
        }
        heights.append(h);
    }
    ok &= expect(std::abs(heights[1] - 160) <= 2 && std::abs(heights[0] - 40) <= 2 && std::abs(heights[2] - 80) <= 2 && std::abs(heights[3] - 120) <= 2,
                 std::format("Bar heights {}, {}, {}, {} px for counts 5, 20, 10, 15", heights[0], heights[1], heights[2], heights[3]));

    //! [spline_usage]
    Spline spline(nullptr, "Amplitude spline");
    spline.setData(classLimits, counts);
    spline.setThreshold(QVector3D(1.0f, 2.0f, 3.0f)); // three vertical marker lines
    spline.setXLabel("Amplitude");
    spline.setYLabel("Count");
    //! [spline_usage]
    ok &= expect(countColor(render(spline), steelBlue, 30) > 50, "Spline draws its curve");

    //! [line_plot_usage]
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i <= 100; ++i) {
        x.push_back(i / 100.0);
        y.push_back(std::sin(2.0 * std::numbers::pi * x.back()));
    }
    LinePlot line(x, y, "sin(2 pi t)");
    line.setXLabel("t [s]");
    line.setYLabel("amplitude");
    //! [line_plot_usage]
    const QImage lineImage = render(line);
    int top = lineImage.height();
    int bottom = 0;
    for (int py = 0; py < lineImage.height(); ++py) {
        for (int px = 0; px < lineImage.width(); ++px) {
            if (close(lineImage.pixel(px, py), steelBlue, 40)) {
                top = std::min(top, py);
                bottom = std::max(bottom, py);
            }
        }
    }
    ok &= expect(bottom > top + 100, std::format("LinePlot spans {} px vertically for a full sine period", bottom - top));

    //! [image_sc_usage]
    MatrixXd ramp(4, 8);
    for (int r = 0; r < ramp.rows(); ++r) {
        for (int c = 0; c < ramp.cols(); ++c) {
            ramp(r, c) = c; // rises left to right
        }
    }
    ImageSc image(ramp); // min..max mapped to 0..1, "Hot" by default
    image.setColorMap("Viridis");
    image.setTitle("ramp");
    image.setExtent(0.0, 0.7, 4.0, 1.0); // columns at 0 .. 0.7 s, rows 1 .. 4 from the top
    image.setXLabel("Time [s]");
    image.setYLabel("Trial");
    image.setColorbarLabel("Value");
    //! [image_sc_usage]
    const QImage scImage = render(image);
    // Along the middle row the first viridis-dark pixel sits left of the first viridis-yellow pixel
    int darkX = -1;
    int brightX = -1;
    for (int px = 0; px < scImage.width(); ++px) {
        if (darkX < 0 && close(scImage.pixel(px, 150), qRgb(68, 1, 84), 4)) {
            darkX = px;
        }
        if (brightX < 0 && close(scImage.pixel(px, 150), qRgb(253, 231, 37), 4)) {
            brightX = px;
        }
    }
    ok &= expect(darkX >= 0 && brightX > darkX + 150, std::format("ImageSc maps the first and last columns to the ends of viridis (x {} and {})", darkX, brightX));

    //! [plot_usage]
    VectorXd samples = VectorXd::LinSpaced(50, -1.0, 1.0);
    Plot plot(samples); // Plot is a Graph with title and axis labels
    plot.setTitle("ramp");
    plot.setXLabel("sample");
    plot.setYLabel("value");
    //! [plot_usage]
    const QImage plotImage = render(plot);
    ok &= expect(countColor(plotImage, qRgb(255, 255, 255), 0) < plotImage.width() * plotImage.height(), "Plot renders axes and data");

    //! [tf_plot_usage]
    MatrixXd tf = MatrixXd::Zero(50, 100);  // 100 samples at 100 Hz: 50 frequency rows of 1 Hz x 100 times
    tf.middleRows(20, 10).setConstant(1.0); // power in the 20-29 Hz bins
    TFplot tfPlot(tf, 100.0, 0.0, 50.0, Jet);
    //! [tf_plot_usage]
    const QImage tfImage = render(tfPlot);
    ok &= expect(countColor(tfImage, ColorMap::valueToJet(1.0), 10) > 100 && countColor(tfImage, ColorMap::valueToJet(0.0), 10) > countColor(tfImage, ColorMap::valueToJet(1.0), 10),
                 "TFplot draws the 20-30 Hz band in the top colour over a bottom-colour background");

    //! [filter_plot_usage]
    // Band pass 1-40 Hz at 250 Hz, 256 taps; frequencies are normed to the Nyquist frequency of 125 Hz
    const FilterKernel bandPass("BPF_1_40", FilterKernel::filter_type_index("BPF"), 256, 20.5 / 125.0, 39.0 / 125.0,
                                5.0 / 125.0, 250.0, FilterKernel::design_method_index("Cosine"));
    FilterPlot filterPlot;
    filterPlot.setFilter(bandPass);
    //! [filter_plot_usage]
    ok &= expect(filterPlot.plot().cutoffs().size() == 2 && filterPlot.plot().nyquist() == 125.0 && countColor(render(filterPlot), qRgb(220, 0, 0), 30) > 100,
                 "FilterPlot marks the two edges of the band pass in red");

    //! [figure_export]
    // Written only when MNE_CPP_FIGURE_DIR names a directory
    FigureExporter::save(line, "line_plot", "LinePlot: one period of a sine wave.");
    //! [figure_export]
    FigureExporter::save(bar, "bar", "Bar: counts of four amplitude classes.");
    FigureExporter::save(spline, "spline", "Spline: the same histogram as a smooth curve with three threshold markers.");
    FigureExporter::save(image, "image_sc", "ImageSc: a left-to-right ramp in the viridis colour map.");
    FigureExporter::save(tfPlot, "tf_plot", "TFplot: power confined to the 20-29 Hz bins.");
    FigureExporter::save(filterPlot, "filter_plot", "FilterPlot: magnitude response of a 1-40 Hz band pass.");

    //! [svg_export]
    QBuffer svgBuffer; // or svg.setFileName("line_plot.svg")
    SvgWriter svg;
    svg.setOutputDevice(&svgBuffer);
    svg.setSize(line.size());
    svg.setTitle("LinePlot");
    QPainter svgPainter(&svg);
    line.render(&svgPainter); // render(QPainter*) keeps child widgets in the same document
    svgPainter.end();
    //! [svg_export]
    ok &= expect(svgBuffer.data().startsWith("<?xml") && svgBuffer.data().contains("<path") && svgBuffer.data().trimmed().endsWith("</svg>"),
                 "SvgWriter writes the line plot as an SVG document with paths");

    std::println("{}", ok ? "All disp plot checks passed." : "disp plot checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
