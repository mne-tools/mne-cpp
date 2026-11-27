//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_qt_plots.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for disp plot widgets — ColorMap verification,
 *           LinePlot, ImageSc, Plot, Spline, Bar, TFplot, Graph,
 *           and ScalingView helper functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/plots/helpers/colormap.h>
#include <mne/viz/backends/qt/plots/helpers/figureexporter.h>
#include <mne/viz/backends/qt/plots/lineplot.h>
#include <mne/viz/backends/qt/plots/imagesc.h>
#include <mne/viz/backends/qt/plots/plot.h>
#include <mne/viz/backends/qt/plots/spline.h>
#include <mne/viz/backends/qt/plots/bar.h>
#include <mne/viz/backends/qt/plots/tfplot.h>
#include <mne/viz/backends/qt/plots/graph.h>
#include <mne/viz/backends/qt/viewers/scalingview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QApplication>
#include <QObject>
#include <QtTest>
#include <QVector>
#include <QVector3D>
#include <QSignalSpy>
#include <QFileInfo>
#include <QImageReader>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <filesystem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for disp plot widget classes.
 */
class TestVizQtPlots : public QObject
{
    Q_OBJECT

private slots:
    // ── ColorMap thorough tests ───────────────────────────────────────
    void testJetMonotonicity();
    void testHotMonotonicity();
    void testBoneMonotonicity();
    void testCoolTransitions();
    void testViridisLUT();
    void testViridisNegatedInverse();
    void testValueToColorDispatcher();
    void testValueToColorUnknownMap();
    void testColormapBoundaryClamp();
    void testRedBlueSymmetry();
    void testJetGreenAtCenter();

    // ── LinePlot ──────────────────────────────────────────────────────
    void testLinePlotDefaultCtor();
    void testLinePlotWithYData();
    void testLinePlotWithXYData();
    void testLinePlotUpdateData();
    void testLinePlotSetLabels();
    void testLinePlotAddSeries();

    // ── FigureExporter ──────────────────────────────────────────────────────────────
    void testFigureExporter();

    // ── Graph base class ──────────────────────────────────────────────
    void testGraphInit();

    // ── ImageSc ───────────────────────────────────────────────────────
    void testImageScDefaultCtor();
    void testImageScDoubleMatrix();
    void testImageScFloatMatrix();
    void testImageScIntMatrix();
    void testImageScUpdateData();
    void testImageScColorRangeAndExtent();

    // ── Plot ──────────────────────────────────────────────────────────
    void testPlotDefaultCtor();
    void testPlotWithVector();
    void testPlotUpdateData();

    // ── Spline ────────────────────────────────────────────────────────
    void testSplineThreshold();

    // ── Bar ───────────────────────────────────────────────────────────
    void testBarSetData();

    // ── TFplot ────────────────────────────────────────────────────────
    void testTFplotConstruction();
    void testTFplotWithFreqRange();

    // ── ScalingView helpers ───────────────────────────────────────────
    void testGetDefaultScalingValue();
    void testGetScalingValueFromMap();
};

//=============================================================================================================
// ColorMap thorough tests
//=============================================================================================================

void TestVizQtPlots::testJetMonotonicity()
{
    // Jet red channel increases from 0.375 to 0.625, holds at 255 until 0.875,
    // then decreases. Test the increasing region only (0.375 to 0.875).
    int prevR = 0;
    bool generallyIncreasing = true;
    for (double v = 0.375; v <= 0.875; v += 0.05) {
        QRgb c = ColorMap::valueToJet(v);
        int r = qRed(c);
        if (r < prevR - 10) { // Allow small dips due to interpolation
            generallyIncreasing = false;
        }
        prevR = r;
    }
    QVERIFY(generallyIncreasing);
}

//=============================================================================================================

void TestVizQtPlots::testHotMonotonicity()
{
    // Hot colormap: brightness should generally increase from 0 to 1
    double prevBrightness = 0;
    bool increasing = true;
    for (double v = 0.0; v <= 1.0; v += 0.1) {
        QRgb c = ColorMap::valueToHot(v);
        double brightness = (qRed(c) + qGreen(c) + qBlue(c)) / 3.0;
        if (brightness < prevBrightness - 5) {
            increasing = false;
        }
        prevBrightness = brightness;
    }
    QVERIFY(increasing);
}

//=============================================================================================================

void TestVizQtPlots::testBoneMonotonicity()
{
    // Bone: brightness should increase from 0 to 1
    double prevBrightness = 0;
    bool increasing = true;
    for (double v = 0.0; v <= 1.0; v += 0.1) {
        QRgb c = ColorMap::valueToBone(v);
        double brightness = (qRed(c) + qGreen(c) + qBlue(c)) / 3.0;
        if (brightness < prevBrightness - 5) {
            increasing = false;
        }
        prevBrightness = brightness;
    }
    QVERIFY(increasing);
}

//=============================================================================================================

void TestVizQtPlots::testCoolTransitions()
{
    // Cool: at 0 should be cyan-ish (high G, high B), at 1 should be magenta-ish (high R, high B)
    QRgb c0 = ColorMap::valueToCool(0.0);
    QRgb c1 = ColorMap::valueToCool(1.0);

    // At 0: expect low red, high blue
    QVERIFY(qBlue(c0) >= qRed(c0));
    // At 1: expect high red
    QVERIFY(qRed(c1) >= qBlue(c1) - 50); // Allow tolerance
}

//=============================================================================================================

void TestVizQtPlots::testViridisLUT()
{
    // Viridis at 0 should be dark purple, at 1 should be bright yellow
    QRgb c0 = ColorMap::valueToViridis(0.0);
    QRgb c1 = ColorMap::valueToViridis(1.0);

    // At 0: relatively dark
    double b0 = (qRed(c0) + qGreen(c0) + qBlue(c0)) / 3.0;
    // At 1: bright yellow (high R, high G, low B)
    double b1 = (qRed(c1) + qGreen(c1) + qBlue(c1)) / 3.0;

    QVERIFY(b1 > b0);
    QVERIFY(qRed(c1) > 200); // Should be yellow-ish
    QVERIFY(qGreen(c1) > 200);
}

//=============================================================================================================

void TestVizQtPlots::testViridisNegatedInverse()
{
    // ViridisNegated reverses the 256-entry LUT direction.
    // Due to floor-based discretization, floor(v*255)+floor((1-v)*255)
    // can equal 254 instead of 255, producing ±1 index offset.
    // Verify exact equality at endpoints and near-equality elsewhere.
    QRgb v0 = ColorMap::valueToViridis(0.0);
    QRgb n1 = ColorMap::valueToViridisNegated(1.0);
    QCOMPARE(qRed(v0), qRed(n1));
    QCOMPARE(qGreen(v0), qGreen(n1));
    QCOMPARE(qBlue(v0), qBlue(n1));

    QRgb v1 = ColorMap::valueToViridis(1.0);
    QRgb n0 = ColorMap::valueToViridisNegated(0.0);
    QCOMPARE(qRed(v1), qRed(n0));
    QCOMPARE(qGreen(v1), qGreen(n0));
    QCOMPARE(qBlue(v1), qBlue(n0));

    for (double v = 0.1; v < 1.0; v += 0.1) {
        QRgb normal = ColorMap::valueToViridis(v);
        QRgb negated = ColorMap::valueToViridisNegated(1.0 - v);
        QVERIFY(std::abs(qRed(normal) - qRed(negated)) <= 3);
        QVERIFY(std::abs(qGreen(normal) - qGreen(negated)) <= 3);
        QVERIFY(std::abs(qBlue(normal) - qBlue(negated)) <= 3);
    }
}

//=============================================================================================================

void TestVizQtPlots::testValueToColorDispatcher()
{
    // Test all named colormaps through the dispatcher
    QStringList maps = {"Jet", "Hot", "HotNegative1", "HotNegative2",
                        "Bone", "RedBlue", "Cool", "Viridis"};
    for (const auto& name : maps) {
        QRgb c = ColorMap::valueToColor(0.5, name);
        QVERIFY2(qRed(c) >= 0 && qRed(c) <= 255,
                 qPrintable(QString("Invalid red for map %1").arg(name)));
        QVERIFY(qGreen(c) >= 0 && qGreen(c) <= 255);
        QVERIFY(qBlue(c) >= 0 && qBlue(c) <= 255);
    }
}

//=============================================================================================================

void TestVizQtPlots::testValueToColorUnknownMap()
{
    // Unknown map name should fall back to Jet
    QRgb cUnknown = ColorMap::valueToColor(0.5, "NonExistentMap");
    QRgb cJet = ColorMap::valueToJet(0.5);
    QCOMPARE(cUnknown, cJet);
}

//=============================================================================================================

void TestVizQtPlots::testColormapBoundaryClamp()
{
    // Test that extreme values don't cause crashes
    for (double v : {-100.0, -1.0, 0.0, 1.0, 100.0}) {
        QRgb jet = ColorMap::valueToJet(v);
        QVERIFY(qRed(jet) >= 0 && qRed(jet) <= 255);

        QRgb hot = ColorMap::valueToHot(v);
        QVERIFY(qRed(hot) >= 0 && qRed(hot) <= 255);

        QRgb bone = ColorMap::valueToBone(v);
        QVERIFY(qRed(bone) >= 0 && qRed(bone) <= 255);
    }
}

//=============================================================================================================

void TestVizQtPlots::testRedBlueSymmetry()
{
    // RedBlue should be symmetric: R channel at +v ≈ B channel at -v
    QRgb cPos = ColorMap::valueToRedBlue(0.8);
    QRgb cNeg = ColorMap::valueToRedBlue(-0.8);
    // At +0.8 should be reddish, at -0.8 should be bluish
    QVERIFY(qRed(cPos) > qBlue(cPos));
    QVERIFY(qBlue(cNeg) > qRed(cNeg));

    // valueToColor takes [0, 1] for every map: RedBlue runs from blue through white to red
    QCOMPARE(ColorMap::valueToColor(0.0, "RedBlue"), ColorMap::valueToRedBlue(-1.0));
    QCOMPARE(ColorMap::valueToColor(0.5, "RedBlue"), qRgb(255, 255, 255));
    QCOMPARE(ColorMap::valueToColor(1.0, "RedBlue"), ColorMap::valueToRedBlue(1.0));
    QVERIFY(qBlue(ColorMap::valueToColor(0.0, "RedBlue")) > qRed(ColorMap::valueToColor(0.0, "RedBlue")));
}

//=============================================================================================================

void TestVizQtPlots::testJetGreenAtCenter()
{
    // Jet colormap should have high green at the center (0.5)
    QRgb c = ColorMap::valueToJet(0.5);
    QVERIFY(qGreen(c) >= 200);
}

//=============================================================================================================
// LinePlot
//=============================================================================================================

void TestVizQtPlots::testLinePlotDefaultCtor()
{
    LinePlot plot;
    QVERIFY(plot.minimumWidth() >= 200);
    QVERIFY(plot.minimumHeight() >= 150);
}

//=============================================================================================================

void TestVizQtPlots::testLinePlotWithYData()
{
    std::vector<double> y = {1.0, 2.0, 3.0, 4.0, 5.0};
    LinePlot plot(y, "Test Plot");
    QVERIFY(plot.minimumWidth() >= 200);
}

//=============================================================================================================

void TestVizQtPlots::testLinePlotWithXYData()
{
    std::vector<double> x = {0.0, 1.0, 2.0, 3.0};
    std::vector<double> y = {1.0, 4.0, 9.0, 16.0};
    LinePlot plot(x, y, "XY Plot");
    QVERIFY(plot.minimumWidth() >= 200);
}

//=============================================================================================================

void TestVizQtPlots::testLinePlotUpdateData()
{
    LinePlot plot;
    std::vector<double> y1 = {1.0, 2.0, 3.0};
    plot.updateData(y1);

    std::vector<double> x2 = {0.0, 1.0, 2.0, 3.0};
    std::vector<double> y2 = {0.0, 1.0, 4.0, 9.0};
    plot.updateData(x2, y2);
    QCOMPARE(plot.seriesCount(), 1);

    // Trace the same data in pure red on top: no other plot element is red on any platform style
    plot.addSeries(x2, y2, QString(), QColor(255, 0, 0));

    // Subpixel-antialiased labels (Linux) have coloured fringes that would be taken for the curve
    QFont font = plot.font();
    font.setStyleStrategy(QFont::NoAntialias);
    plot.setFont(font);

    // The curve rises from (0, 0) at the bottom-left of its extent to (3, 9) at the top-right
    plot.resize(400, 300);
    const QImage image = plot.grab().toImage();
    const auto isCurve = [&image](int x, int y) {
        const QColor c = image.pixelColor(x, y);
        return c.red() > 200 && c.green() < 90 && c.blue() < 90;
    };
    QRect extent;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (isCurve(x, y)) {
                extent |= QRect(x, y, 1, 1);
            }
        }
    }
    const QString where = QString("curve extent %1,%2 %3x%4").arg(extent.x()).arg(extent.y()).arg(extent.width()).arg(extent.height());
    QVERIFY2(extent.width() > 250 && extent.height() > 150, qPrintable(where));
    // Mean row of the curve in column x, so anti-aliasing and line width do not matter
    const auto curveY = [&](int x) {
        double sum = 0.0;
        int n = 0;
        for (int y = extent.top(); y <= extent.bottom(); ++y) {
            if (isCurve(x, y)) {
                sum += y;
                ++n;
            }
        }
        return n > 0 ? sum / n : -1.0;
    };
    const double first = curveY(extent.left() + 2);
    const double last = curveY(extent.right() - 2);
    QVERIFY2(first > extent.bottom() - 6, qPrintable(where + QString(", first column at y %1").arg(first)));
    QVERIFY2(last >= 0 && last < extent.top() + 6, qPrintable(where + QString(", last column at y %1").arg(last)));
    // y = x^2: at x = 2 the curve is at 4/9 of the height
    const double mid = curveY(extent.left() + extent.width() * 2 / 3);
    QVERIFY2(qAbs(extent.bottom() - mid - extent.height() * 4.0 / 9.0) <= 6, qPrintable(where + QString(", x = 2 at y %1").arg(mid)));

    // A constant series is drawn instead of dividing by a zero range
    plot.updateData(std::vector<double>{5.0, 5.0, 5.0});
    QVERIFY(!plot.grab().isNull());
}

//=============================================================================================================

void TestVizQtPlots::testLinePlotSetLabels()
{
    LinePlot plot;
    plot.resize(400, 300);
    plot.updateData(std::vector<double>{1.0, 2.0, 3.0});
    const QImage plain = plot.grab().toImage();
    plot.setTitle("My Title");
    plot.setXLabel("X Axis");
    plot.setYLabel("Y Axis");
    QVERIFY(plot.grab().toImage() != plain);
}

//=============================================================================================================

void TestVizQtPlots::testLinePlotAddSeries()
{
    LinePlot plot;
    plot.resize(400, 300);
    plot.addSeries({0.0, 0.5}, {0.0, 0.5}, QString(), QColor(255, 0, 0));
    plot.addSeries({0.0, 1.0}, {1.0, 0.0}, "falling", QColor(0, 0, 255));
    QCOMPARE(plot.seriesCount(), 2);
    const auto count = [](const QImage& image, QRgb color) {
        int n = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x, y);
                n += qAbs(c.red() - qRed(color)) < 60 && qAbs(c.green() - qGreen(color)) < 60 && qAbs(c.blue() - qBlue(color)) < 60;
            }
        }
        return n;
    };
    const QImage image = plot.grab().toImage();
    QVERIFY(count(image, qRgb(0, 0, 255)) > 250);
    QVERIFY(count(image, qRgb(255, 0, 0)) > 120);

    // Only the labelled series gets a legend swatch, in the free top-right corner
    LinePlot unlabelled;
    unlabelled.resize(400, 300);
    unlabelled.addSeries({0.0, 0.5}, {0.0, 0.5}, QString(), QColor(255, 0, 0));
    unlabelled.addSeries({0.0, 1.0}, {1.0, 0.0}, QString(), QColor(0, 0, 255));
    QVERIFY(count(image, qRgb(0, 0, 255)) > count(unlabelled.grab().toImage(), qRgb(0, 0, 255)) + 10);

    // A series that crosses every corner pushes the legend out to the right of the axes
    const auto frameRight = [](const QImage& frame) {
        for (int x = frame.width() - 1; x >= 0; --x) {
            int run = 0;
            for (int y = 0; y < frame.height(); ++y) {
                const QColor c = frame.pixelColor(x, y);
                run += c.red() == c.green() && c.green() == c.blue() && c.red() < 200;
            }
            if (run > frame.height() / 2) {
                return x;
            }
        }
        return -1;
    };
    std::vector<double> x(200);
    std::vector<double> zigzag(200);
    for (int i = 0; i < 200; ++i) {
        x[i] = i;
        zigzag[i] = i % 2;
    }
    LinePlot crowded;
    crowded.resize(400, 300);
    crowded.addSeries(x, zigzag);
    const int inside = frameRight(crowded.grab().toImage());
    crowded.addSeries(x, zigzag, "zigzag");
    QVERIFY(frameRight(crowded.grab().toImage()) < inside - 40);

    // A default colour skips the palette colours already in use; a dashed line leaves gaps
    LinePlot palette;
    palette.resize(400, 300);
    palette.addSeries({0.0, 1.0}, {0.0, 0.0}, QString(), QColor(221, 132, 82));
    palette.addSeries({0.0, 1.0}, {1.0, 1.0});
    const int solidPixels = count(palette.grab().toImage(), qRgb(70, 130, 180));
    QVERIFY(solidPixels > 100);
    LinePlot dashed;
    dashed.resize(400, 300);
    dashed.addSeries({0.0, 1.0}, {0.0, 0.0}, QString(), QColor(70, 130, 180));
    dashed.addSeries({0.0, 1.0}, {1.0, 1.0}, QString(), QColor(70, 130, 180), Qt::DashLine);
    QVERIFY(count(dashed.grab().toImage(), qRgb(70, 130, 180)) < 1.8 * solidPixels);

    plot.updateData(std::vector<double>{1.0, 2.0});
    QCOMPARE(plot.seriesCount(), 1);
}

//=============================================================================================================

void TestVizQtPlots::testFigureExporter()
{
    LinePlot plot(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0}, "ramp");
    qunsetenv("MNE_CPP_FIGURE_DIR");
    QVERIFY(FigureExporter::save(plot, "ramp", "A ramp").empty());

    QTemporaryDir dir;
    qputenv("MNE_CPP_FIGURE_DIR", QFile::encodeName(dir.filePath("figures")));
    const std::filesystem::path first = FigureExporter::save(plot, "ramp", "A ramp \u00b5V", QSize(320, 200));
    QImage plain(10, 10, QImage::Format_RGB32);
    plain.fill(Qt::white);
    const std::filesystem::path second = FigureExporter::save(plain, "a b/c", "Plain");
    qunsetenv("MNE_CPP_FIGURE_DIR");

    QCOMPARE(first.filename(), std::filesystem::path("01_ramp.png"));
    QCOMPARE(second.filename(), std::filesystem::path("02_a_b_c.png"));
    QImageReader reader(QString::fromStdString(first.string()));
    QCOMPARE(reader.text("Description"), QString("A ramp \u00b5V"));
    const QImage image = reader.read();
    QCOMPARE(image.size(), QSize(640, 400));
    QVERIFY(plot.isHidden());
}

//=============================================================================================================
// Graph base class
//=============================================================================================================

void TestVizQtPlots::testGraphInit()
{
    Graph graph;
    QVERIFY(graph.minimumWidth() > 0);
    QVERIFY(graph.minimumHeight() > 0);
}

//=============================================================================================================
// ImageSc
//=============================================================================================================

void TestVizQtPlots::testImageScDefaultCtor()
{
    ImageSc img;
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testImageScDoubleMatrix()
{
    MatrixXd mat(3, 4);
    mat << 1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testImageScFloatMatrix()
{
    MatrixXf mat(2, 3);
    mat << 1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testImageScIntMatrix()
{
    MatrixXi mat(2, 2);
    mat << 1, 2, 3, 4;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testImageScUpdateData()
{
    ImageSc img;
    MatrixXd mat(3, 3);
    mat << 1, 0, 0, 0, 1, 0, 0, 0, 1;
    img.updateData(mat);

    MatrixXf matf(2, 2);
    matf << 1.0f, 0.0f, 0.0f, 1.0f;
    img.updateData(matf);

    // A 1 x 2 matrix [min, max] fills the left half with the low end and the right half with the high end of the map
    img.resize(400, 300);
    img.setColorMap(QStringLiteral("Jet"));
    MatrixXd gradient(1, 2);
    gradient << -3.0, 5.0;
    img.updateData(gradient);
    QImage image = img.grab().toImage();
    const QColor low = image.pixelColor(110, 150);
    const QColor high = image.pixelColor(240, 150);
    QCOMPARE(low.rgb(), ColorMap::valueToColor(0.0, QStringLiteral("Jet")));
    QCOMPARE(high.rgb(), ColorMap::valueToColor(1.0, QStringLiteral("Jet")));

    // A constant matrix has no range: it maps to one colour instead of NaN
    MatrixXd constant = MatrixXd::Constant(2, 2, 7.0);
    img.updateData(constant);
    image = img.grab().toImage();
    QCOMPARE(image.pixelColor(110, 150), image.pixelColor(240, 150));
    QCOMPARE(image.pixelColor(110, 150).rgb(), ColorMap::valueToColor(0.0, QStringLiteral("Jet")));
}

//=============================================================================================================

void TestVizQtPlots::testImageScColorRangeAndExtent()
{
    ImageSc img;
    img.resize(400, 300);
    img.setColorMap(QStringLiteral("Jet"));
    MatrixXd values(1, 3);
    values << -1.0, 0.0, 4.0;
    img.updateData(values);
    // A fixed range maps 0 to the middle of the map and clips 4 to its high end
    img.setColorRange(-1.0, 1.0);
    const QImage image = img.grab().toImage();
    QList<QRgb> colors;
    for (int x = 0; x < image.width(); ++x) {
        const QRgb c = image.pixel(x, 150);
        if (colors.isEmpty() ? c == ColorMap::valueToColor(0.0, QStringLiteral("Jet")) : c != colors.last()) {
            colors.append(c);
        }
    }
    QVERIFY(colors.size() >= 3);
    QCOMPARE(colors[1], ColorMap::valueToColor(0.5, QStringLiteral("Jet")));
    QCOMPARE(colors[2], ColorMap::valueToColor(1.0, QStringLiteral("Jet")));

    // Axis ticks follow the extent: the plot gains a label wider than any column index
    QImage indices = img.grab().toImage();
    img.setExtent(-200.0, 600.0, 10.0, 1.0);
    QVERIFY(img.grab().toImage() != indices);
}

//=============================================================================================================
// Plot
//=============================================================================================================

void TestVizQtPlots::testPlotDefaultCtor()
{
    Plot plot;
    QVERIFY(plot.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testPlotWithVector()
{
    VectorXd vec(5);
    vec << 1.0, 2.0, 3.0, 4.0, 5.0;
    Plot plot(vec);
    QVERIFY(plot.minimumWidth() > 0);
}

//=============================================================================================================

void TestVizQtPlots::testPlotUpdateData()
{
    Plot plot;
    plot.resize(400, 300);
    VectorXd vec(4);
    vec << 10.0, 20.0, 30.0, 40.0;
    plot.updateData(vec);
    // Grabbed without a title: subpixel-antialiased text has blue fringes on Linux
    QImage image = plot.grab().toImage();

    // The curve stays inside the 200 x 200 plot area (borders 100/50) and rises to the right
    const auto isCurve = [](QRgb c) {
        return qBlue(c) > 150 && qRed(c) < 120 && qGreen(c) < 120;
    };
    int leftY = -1, rightY = -1, outside = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!isCurve(image.pixel(x, y)))
                continue;
            if (x < 100 || x > 300 || y < 50 || y > 250)
                ++outside;
            if (x <= 103)
                leftY = y;
            if (x >= 297 && rightY < 0)
                rightY = y;
        }
    }
    QCOMPARE(outside, 0);
    QVERIFY2(leftY > 0 && rightY > 0 && rightY < leftY, qPrintable(QString("left %1 right %2").arg(leftY).arg(rightY)));

    // The title is drawn above the plot area
    plot.setTitle("Ramp");
    image = plot.grab().toImage();
    int titlePixels = 0;
    for (int y = 0; y < 45; ++y)
        for (int x = 150; x < 250; ++x)
            titlePixels += qGray(image.pixel(x, y)) < 100;
    QVERIFY(titlePixels > 0);
}

//=============================================================================================================
// Spline
//=============================================================================================================

void TestVizQtPlots::testSplineThreshold()
{
    Spline spline;
    spline.resize(320, 250);
    // No data yet: thresholds are ignored
    spline.setThreshold(QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(spline.getThreshold(), QVector3D());

    // Class limits 0..4 with frequencies per class
    VectorXd limits(5);
    limits << 0.0, 1.0, 2.0, 3.0, 4.0;
    VectorXi freq(4);
    freq << 10, 25, 15, 30;
    spline.setData(limits, freq);

    // In-range thresholds are sorted into left <= middle <= right
    spline.setThreshold(QVector3D(3.0f, 1.0f, 2.0f));
    QCOMPARE(spline.getThreshold(), QVector3D(1.0f, 2.0f, 3.0f));

    // Out of range: fall back to thresholds inside the axis range [min, max]
    spline.setThreshold(QVector3D(-5.0f, 2.0f, 3.0f));
    QVector3D fallback = spline.getThreshold();
    QVERIFY(fallback.x() >= 0.0f && fallback.x() < fallback.y() && fallback.y() < fallback.z() && fallback.z() <= 4.0f);

    // Negative limits: the fallback must stay inside [-4, -1]
    VectorXd negative(4);
    negative << -4.0, -3.0, -2.0, -1.0;
    VectorXi negativeFreq(3);
    negativeFreq << 1, 2, 3;
    spline.setData(negative, negativeFreq);
    spline.setThreshold(QVector3D(10.0f, 20.0f, 30.0f));
    fallback = spline.getThreshold();
    QVERIFY2(fallback.x() >= -4.0f && fallback.x() < fallback.y() && fallback.y() < fallback.z() && fallback.z() <= -1.0f,
             qPrintable(QStringLiteral("%1 %2 %3").arg(fallback.x()).arg(fallback.y()).arg(fallback.z())));

    // Clicking sets a threshold at the clicked data value: left button sets the left one
    spline.setData(limits, freq);
    QSignalSpy borderSpy(&spline, &Spline::borderChanged);
    const double plotWidth = spline.width() - 60 - 20;
    const QPointF at1(60 + plotWidth / 4.0, 100);
    QMouseEvent press(QEvent::MouseButtonPress, at1, spline.mapToGlobal(at1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&spline, &press);
    QCOMPARE(borderSpy.size(), 1);
    QVERIFY(std::fabs(borderSpy.at(0).at(0).toDouble() - 1.0) < 1e-6);
    QVERIFY(!spline.grab().isNull());

    // The title is drawn above the plot area
    Spline titled(nullptr, QStringLiteral("Amplitudes"));
    titled.resize(320, 250);
    titled.setData(limits, freq);
    const QImage top = titled.grab().toImage().copy(80, 0, 200, 24);
    int dark = 0;
    for (int y = 0; y < top.height(); ++y) {
        for (int x = 0; x < top.width(); ++x) {
            dark += qGray(top.pixel(x, y)) < 128;
        }
    }
    QVERIFY(dark > 20);
}

//=============================================================================================================
// Bar
//=============================================================================================================

void TestVizQtPlots::testBarSetData()
{
    Bar bar("Histogram");
    bar.resize(380, 300);

    // Four class limits bound three classes; the fourth frequency has no class
    VectorXd limits(4);
    limits << 0.0, 1.0, 2.0, 3.0;
    VectorXi freq(4);
    freq << 5, 15, 10, 20;
    bar.setData(limits, freq, 2);
    const QImage image = bar.grab().toImage();

    // Plot area x 60..360, y 40..200: bar heights relative to the largest class (15)
    const auto isBar = [&](double x, int y) {
        return image.pixelColor(static_cast<int>(x), y) == QColor(70, 130, 180);
    };
    const double barWidth = 300.0 / 3;
    const int halfHeight = 40 + 80;
    QVERIFY(isBar(60 + 0.5 * barWidth, 195) && !isBar(60 + 0.5 * barWidth, halfHeight));
    QVERIFY(isBar(60 + 1.5 * barWidth, 195) && isBar(60 + 1.5 * barWidth, 45));
    QVERIFY(isBar(60 + 2.5 * barWidth, 195) && isBar(60 + 2.5 * barWidth, halfHeight));
    QVERIFY(!isBar(60 + 2.5 * barWidth, 60));
}

//=============================================================================================================
// TFplot
//=============================================================================================================

namespace
{
struct TFplotProbe : public TFplot
{
    using TFplot::TFplot;
    MatrixXd data() const
    {
        MatrixXd matrix(m_plot.rows(), m_plot.cols());
        for (int r = 0; r < m_plot.rows(); ++r) {
            for (int c = 0; c < m_plot.cols(); ++c) {
                matrix(r, c) = m_plot.values()[static_cast<std::size_t>(r) * static_cast<std::size_t>(m_plot.cols()) + static_cast<std::size_t>(c)];
            }
        }
        return matrix;
    }
    std::vector<double> extent() const
    {
        const auto e = m_plot.extent();
        return {e[0], e[1], e[2], e[3]};
    }
    QString colorbarLabel() const
    {
        return QString::fromStdString(m_plot.colorbar_label());
    }
};
} // namespace

void TestVizQtPlots::testTFplotConstruction()
{
    // Spectrogram shape: N/2 frequency rows of an N-point FFT x N time samples
    MatrixXd tfMatrix = MatrixXd::Random(10, 20).cwiseAbs();
    TFplotProbe tf(tfMatrix, 1000.0, ColorMaps::Jet);
    QCOMPARE(tf.data().rows(), 10);
    QCOMPARE(tf.extent(), std::vector<double>({0.0, 19 / 1000.0, 0.0, 9 * 1000.0 / 20}));
    QCOMPARE(tf.colorbarLabel(), QStringLiteral("Power (normalized)"));
    QVERIFY(!tf.grab().isNull());
}

//=============================================================================================================

void TestVizQtPlots::testTFplotWithFreqRange()
{
    // 100 samples at 100 Hz: row k is k Hz
    MatrixXd tfMatrix = MatrixXd::Zero(50, 100);
    tfMatrix.row(40).setConstant(2.0);
    TFplotProbe tf(tfMatrix, 100.0, 1.0, 40.0, ColorMaps::Hot, -0.2);
    // Rows 1..40 Hz, highest frequency first, normalized to the maximum
    QCOMPARE(tf.data().rows(), 40);
    QCOMPARE(tf.data().row(0).minCoeff(), 1.0);
    // Element-wise: GCC 14 flags Eigen's vectorised maxCoeff() over a block as maybe-uninitialized
    double rest = 0.0;
    for (Eigen::Index r = 1; r < tf.data().rows(); ++r) {
        for (Eigen::Index c = 0; c < tf.data().cols(); ++c) {
            rest = std::max(rest, tf.data()(r, c));
        }
    }
    QCOMPARE(rest, 0.0);
    // Times start at tmin; frequencies are the bin centres, not the requested bounds
    QCOMPARE(tf.extent(), std::vector<double>({-0.2, -0.2 + 99 / 100.0, 1.0, 40.0}));

    // Odd sample count: bins are sfreq / N apart, not (sfreq / 2) / (N / 2)
    const double sfreq = 600.614990234375;
    TFplotProbe odd(MatrixXd::Ones(210, 421), sfreq, 1.0, 50.0, ColorMaps::Jet);
    QCOMPARE(odd.extent()[2], sfreq / 421);
    QCOMPARE(odd.extent()[3], 35 * sfreq / 421);
}

//=============================================================================================================
// ScalingView helpers
//=============================================================================================================

void TestVizQtPlots::testGetDefaultScalingValue()
{
    // FIFFV_MEG_CH = 1, FIFFV_EEG_CH = 2 etc. - test that we get nonzero defaults
    float megGradScale = getDefaultScalingValue(1, 201); // MEG, T/m unit
    float megMagScale = getDefaultScalingValue(1, 112);  // MEG, T unit
    float eegScale = getDefaultScalingValue(2, 107);     // EEG, V unit

    QVERIFY(megGradScale > 0);
    QVERIFY(megMagScale > 0);
    QVERIFY(eegScale > 0);
}

//=============================================================================================================

void TestVizQtPlots::testGetScalingValueFromMap()
{
    std::map<int, float> scaleMap;
    // Use EEG (kind=2) and STIM (kind=3) — non-MEG kinds so the lookup
    // uses iChannelKind directly (MEG channels look up by unit instead).
    scaleMap[2] = 42.0f;
    scaleMap[3] = 84.0f;

    float val = getScalingValue(scaleMap, 2, 0);
    QCOMPARE(val, 42.0f);

    float val2 = getScalingValue(scaleMap, 3, 0);
    QCOMPARE(val2, 84.0f);

    // Non-existent key should return a default
    float valDefault = getScalingValue(scaleMap, 999, 0);
    QVERIFY(valDefault >= 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestVizQtPlots)
#include "test_viz_qt_plots.moc"
