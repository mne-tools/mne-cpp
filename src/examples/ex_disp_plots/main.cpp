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

#include <disp/plots/bar.h>
#include <disp/plots/graph.h>
#include <disp/plots/helpers/colormap.h>
#include <disp/plots/imagesc.h>
#include <disp/plots/lineplot.h>
#include <disp/plots/plot.h>
#include <disp/plots/spline.h>
#include <disp/plots/tfplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <cstdio>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
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
                 QString("Bar heights %1, %2, %3, %4 px for counts 5, 20, 10, 15").arg(heights[0]).arg(heights[1]).arg(heights[2]).arg(heights[3]));

    //! [spline_usage]
    Spline spline(nullptr, "Amplitude spline");
    spline.setData(classLimits, counts);
    spline.setThreshold(QVector3D(1.0f, 2.0f, 3.0f)); // three vertical marker lines
    //! [spline_usage]
    ok &= expect(countColor(render(spline), steelBlue, 30) > 50, "Spline draws its curve");

    //! [line_plot_usage]
    QVector<double> x;
    QVector<double> y;
    for (int i = 0; i <= 100; ++i) {
        x.append(i / 100.0);
        y.append(std::sin(2.0 * M_PI * x.last()));
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
    ok &= expect(bottom > top + 100, QString("LinePlot spans %1 px vertically for a full sine period").arg(bottom - top));

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
    //! [image_sc_usage]
    const QImage scImage = render(image);
    // Data area: x in [60, 340); first and last columns are the colour map ends
    ok &= expect(close(scImage.pixel(70, 150), qRgb(68, 1, 84), 4) && close(scImage.pixel(330, 150), qRgb(253, 231, 37), 4),
                 "ImageSc maps the minimum and maximum columns to the ends of viridis");

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
    MatrixXd tf = MatrixXd::Zero(50, 40);   // frequency rows x time columns
    tf.middleRows(20, 10).setConstant(1.0); // 20-30 Hz band at 100 Hz sampling
    TFplot tfPlot(tf, 100.0, 0.0, 50.0, Jet);
    //! [tf_plot_usage]
    const QImage tfImage = render(tfPlot);
    ok &= expect(countColor(tfImage, ColorMap::valueToJet(1.0), 10) > 100 && countColor(tfImage, ColorMap::valueToJet(0.0), 10) > countColor(tfImage, ColorMap::valueToJet(1.0), 10),
                 "TFplot draws the 20-30 Hz band in the top colour over a bottom-colour background");

    qInfo().noquote() << (ok ? "All disp plot checks passed." : "disp plot checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
