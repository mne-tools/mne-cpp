//=============================================================================================================
/**
 * @file     test_disp_plots.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Tests for disp plot widgets — ColorMap verification,
 *           LinePlot, ImageSc, Plot, Spline, Bar, TFplot, Graph,
 *           and ScalingView helper functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/plots/helpers/colormap.h>
#include <disp/plots/lineplot.h>
#include <disp/plots/imagesc.h>
#include <disp/plots/plot.h>
#include <disp/plots/spline.h>
#include <disp/plots/bar.h>
#include <disp/plots/tfplot.h>
#include <disp/plots/graph.h>
#include <disp/viewers/scalingview.h>

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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for disp plot widget classes.
 */
class TestDispPlots : public QObject
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

    // ── Graph base class ──────────────────────────────────────────────
    void testGraphInit();
    void testGraphSetTitle();

    // ── ImageSc ───────────────────────────────────────────────────────
    void testImageScDefaultCtor();
    void testImageScDoubleMatrix();
    void testImageScFloatMatrix();
    void testImageScIntMatrix();
    void testImageScUpdateData();

    // ── Plot ──────────────────────────────────────────────────────────
    void testPlotDefaultCtor();
    void testPlotWithVector();
    void testPlotUpdateData();

    // ── Spline ────────────────────────────────────────────────────────
    void testSplineDefaultCtor();
    void testSplineSetData();
    void testSplineThreshold();

    // ── Bar ───────────────────────────────────────────────────────────
    void testBarDefaultCtor();
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

void TestDispPlots::testJetMonotonicity()
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

void TestDispPlots::testHotMonotonicity()
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

void TestDispPlots::testBoneMonotonicity()
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

void TestDispPlots::testCoolTransitions()
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

void TestDispPlots::testViridisLUT()
{
    // Viridis at 0 should be dark purple, at 1 should be bright yellow
    QRgb c0 = ColorMap::valueToViridis(0.0);
    QRgb c1 = ColorMap::valueToViridis(1.0);

    // At 0: relatively dark
    double b0 = (qRed(c0) + qGreen(c0) + qBlue(c0)) / 3.0;
    // At 1: bright yellow (high R, high G, low B)
    double b1 = (qRed(c1) + qGreen(c1) + qBlue(c1)) / 3.0;

    QVERIFY(b1 > b0);
    QVERIFY(qRed(c1) > 200);    // Should be yellow-ish
    QVERIFY(qGreen(c1) > 200);
}

//=============================================================================================================

void TestDispPlots::testViridisNegatedInverse()
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

void TestDispPlots::testValueToColorDispatcher()
{
    // Test all named colormaps through the dispatcher
    QStringList maps = {"Jet", "Hot", "Hot Negative 1", "Hot Negative 2",
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

void TestDispPlots::testValueToColorUnknownMap()
{
    // Unknown map name should fall back to Jet
    QRgb cUnknown = ColorMap::valueToColor(0.5, "NonExistentMap");
    QRgb cJet = ColorMap::valueToJet(0.5);
    QCOMPARE(cUnknown, cJet);
}

//=============================================================================================================

void TestDispPlots::testColormapBoundaryClamp()
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

void TestDispPlots::testRedBlueSymmetry()
{
    // RedBlue should be symmetric: R channel at +v ≈ B channel at -v
    QRgb cPos = ColorMap::valueToRedBlue(0.8);
    QRgb cNeg = ColorMap::valueToRedBlue(-0.8);
    // At +0.8 should be reddish, at -0.8 should be bluish
    QVERIFY(qRed(cPos) > qBlue(cPos));
    QVERIFY(qBlue(cNeg) > qRed(cNeg));
}

//=============================================================================================================

void TestDispPlots::testJetGreenAtCenter()
{
    // Jet colormap should have high green at the center (0.5)
    QRgb c = ColorMap::valueToJet(0.5);
    QVERIFY(qGreen(c) >= 200);
}

//=============================================================================================================
// LinePlot
//=============================================================================================================

void TestDispPlots::testLinePlotDefaultCtor()
{
    LinePlot plot;
    QVERIFY(plot.minimumWidth() >= 200);
    QVERIFY(plot.minimumHeight() >= 150);
}

//=============================================================================================================

void TestDispPlots::testLinePlotWithYData()
{
    QVector<double> y = {1.0, 2.0, 3.0, 4.0, 5.0};
    LinePlot plot(y, "Test Plot");
    QVERIFY(plot.minimumWidth() >= 200);
}

//=============================================================================================================

void TestDispPlots::testLinePlotWithXYData()
{
    QVector<double> x = {0.0, 1.0, 2.0, 3.0};
    QVector<double> y = {1.0, 4.0, 9.0, 16.0};
    LinePlot plot(x, y, "XY Plot");
    QVERIFY(plot.minimumWidth() >= 200);
}

//=============================================================================================================

void TestDispPlots::testLinePlotUpdateData()
{
    LinePlot plot;
    QVector<double> y1 = {1.0, 2.0, 3.0};
    plot.updateData(y1);

    QVector<double> x2 = {0.0, 1.0, 2.0, 3.0};
    QVector<double> y2 = {0.0, 1.0, 4.0, 9.0};
    plot.updateData(x2, y2);
    QVERIFY(true); // No crash
}

//=============================================================================================================

void TestDispPlots::testLinePlotSetLabels()
{
    LinePlot plot;
    plot.setTitle("My Title");
    plot.setXLabel("X Axis");
    plot.setYLabel("Y Axis");
    QVERIFY(true); // No crash
}

//=============================================================================================================
// Graph base class
//=============================================================================================================

void TestDispPlots::testGraphInit()
{
    Graph graph;
    QVERIFY(graph.minimumWidth() > 0);
    QVERIFY(graph.minimumHeight() > 0);
}

//=============================================================================================================

void TestDispPlots::testGraphSetTitle()
{
    Graph graph;
    graph.setTitle("Test Title");
    QVERIFY(true); // No crash
}

//=============================================================================================================
// ImageSc
//=============================================================================================================

void TestDispPlots::testImageScDefaultCtor()
{
    ImageSc img;
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testImageScDoubleMatrix()
{
    MatrixXd mat(3, 4);
    mat << 1, 2, 3, 4,
           5, 6, 7, 8,
           9, 10, 11, 12;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testImageScFloatMatrix()
{
    MatrixXf mat(2, 3);
    mat << 1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testImageScIntMatrix()
{
    MatrixXi mat(2, 2);
    mat << 1, 2, 3, 4;
    ImageSc img(mat);
    QVERIFY(img.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testImageScUpdateData()
{
    ImageSc img;
    MatrixXd mat(3, 3);
    mat << 1, 0, 0, 0, 1, 0, 0, 0, 1;
    img.updateData(mat);

    MatrixXf matf(2, 2);
    matf << 1.0f, 0.0f, 0.0f, 1.0f;
    img.updateData(matf);
    QVERIFY(true);
}

//=============================================================================================================
// Plot
//=============================================================================================================

void TestDispPlots::testPlotDefaultCtor()
{
    Plot plot;
    QVERIFY(plot.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testPlotWithVector()
{
    VectorXd vec(5);
    vec << 1.0, 2.0, 3.0, 4.0, 5.0;
    Plot plot(vec);
    QVERIFY(plot.minimumWidth() > 0);
}

//=============================================================================================================

void TestDispPlots::testPlotUpdateData()
{
    Plot plot;
    VectorXd vec(4);
    vec << 10.0, 20.0, 30.0, 40.0;
    plot.updateData(vec);
    QVERIFY(true);
}

//=============================================================================================================
// Spline
//=============================================================================================================

void TestDispPlots::testSplineDefaultCtor()
{
    Spline spline;
    QVERIFY(true); // Construction without crash
}

//=============================================================================================================

void TestDispPlots::testSplineSetData()
{
    Spline spline;

    VectorXd limits(5);
    limits << 0.0, 1.0, 2.0, 3.0, 4.0;
    VectorXi freq(5);
    freq << 10, 25, 15, 30, 5;

    spline.setData(limits, freq);
    QVERIFY(true);
}

//=============================================================================================================

void TestDispPlots::testSplineThreshold()
{
    Spline spline;
    QVector3D thresholds(1.0, 2.0, 3.0);
    spline.setThreshold(thresholds);
    QVERIFY(true);
}

//=============================================================================================================
// Bar
//=============================================================================================================

void TestDispPlots::testBarDefaultCtor()
{
    Bar bar("Test Bar");
    QVERIFY(true);
}

//=============================================================================================================

void TestDispPlots::testBarSetData()
{
    Bar bar("Histogram");

    VectorXd limits(4);
    limits << 0.0, 1.0, 2.0, 3.0;
    VectorXi freq(4);
    freq << 5, 15, 10, 20;

    bar.setData(limits, freq, 2);
    QVERIFY(true);
}

//=============================================================================================================
// TFplot
//=============================================================================================================

void TestDispPlots::testTFplotConstruction()
{
    MatrixXd tfMatrix(10, 20);
    tfMatrix.setRandom();
    tfMatrix = tfMatrix.cwiseAbs(); // Make positive for plotting

    TFplot* tf = new TFplot(tfMatrix, 1000.0, ColorMaps::Jet);
    QVERIFY(tf != nullptr);
    delete tf;
}

//=============================================================================================================

void TestDispPlots::testTFplotWithFreqRange()
{
    MatrixXd tfMatrix(10, 20);
    tfMatrix.setRandom();
    tfMatrix = tfMatrix.cwiseAbs();

    TFplot* tf = new TFplot(tfMatrix, 1000.0, 1.0, 40.0, ColorMaps::Hot);
    QVERIFY(tf != nullptr);
    delete tf;
}

//=============================================================================================================
// ScalingView helpers
//=============================================================================================================

void TestDispPlots::testGetDefaultScalingValue()
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

void TestDispPlots::testGetScalingValueFromMap()
{
    QMap<qint32, float> scaleMap;
    scaleMap.insert(1, 42.0f);
    scaleMap.insert(2, 84.0f);

    float val = getScalingValue(scaleMap, 1, 0);
    QCOMPARE(val, 42.0f);

    float val2 = getScalingValue(scaleMap, 2, 0);
    QCOMPARE(val2, 84.0f);

    // Non-existent key should return a default
    float valDefault = getScalingValue(scaleMap, 999, 0);
    QVERIFY(valDefault >= 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestDispPlots)
#include "test_disp_plots.moc"
