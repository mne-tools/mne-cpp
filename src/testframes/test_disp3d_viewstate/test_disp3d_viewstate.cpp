//=============================================================================================================
/**
 * @file     test_disp3d_viewstate.cpp
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
 * @brief    Tests for disp3D ViewState components — ViewVisibilityProfile,
 *           SubView, rendertypes packABGR helper, surfacekeys constants,
 *           and enum/string conversion utilities.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/core/viewstate.h>
#include <disp3D/core/rendertypes.h>
#include <disp3D/core/surfacekeys.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
/**
 * @brief Tests for ViewVisibilityProfile, SubView, rendertypes, and surfacekeys.
 */
class TestDisp3dViewState : public QObject
{
    Q_OBJECT

private slots:
    // ── packABGR helper ───────────────────────────────────────────────
    void testPackABGR_basicColors();
    void testPackABGR_alphaDefault();
    void testPackABGR_customAlpha();

    // ── ViewVisibilityProfile ─────────────────────────────────────────
    void testVisibilityDefaults();
    void testSetObjectVisibleByName();
    void testIsObjectVisibleByName();
    void testVisibilityLoadSave();

    // ── SubView ───────────────────────────────────────────────────────
    void testSubViewDefaults();
    void testIsBrainSurfaceKey();
    void testMatchesSurfaceType();
    void testShouldRenderSurface();
    void testDefaultForIndex();

    // ── Enum/string conversion ────────────────────────────────────────
    void testShaderModeFromName();
    void testShaderModeName();
    void testVisualizationModeFromName();
    void testVisualizationModeName();

    // ── multiViewPresetName ───────────────────────────────────────────
    void testMultiViewPresetNames();
    void testMultiViewPresetIsPerspective();

    // ── surfacekeys constants ─────────────────────────────────────────
    void testSurfaceKeyConstants();
    void testSensorTypeToObjectKey();

    // ── mneAnalyzeColor ───────────────────────────────────────────────
    void testMneAnalyzeColorRange();
};

//=============================================================================================================
// packABGR tests
//=============================================================================================================

void TestDisp3dViewState::testPackABGR_basicColors()
{
    // Pure red: R=255, G=0, B=0
    uint32_t red = packABGR(255, 0, 0);
    QCOMPARE(red & 0xFF, (uint32_t)255);         // R in low byte
    QCOMPARE((red >> 8) & 0xFF, (uint32_t)0);    // G
    QCOMPARE((red >> 16) & 0xFF, (uint32_t)0);   // B
    QCOMPARE((red >> 24) & 0xFF, (uint32_t)255); // A default

    // Pure green
    uint32_t green = packABGR(0, 255, 0);
    QCOMPARE(green & 0xFF, (uint32_t)0);
    QCOMPARE((green >> 8) & 0xFF, (uint32_t)255);

    // Pure blue
    uint32_t blue = packABGR(0, 0, 255);
    QCOMPARE((blue >> 16) & 0xFF, (uint32_t)255);
}

//=============================================================================================================

void TestDisp3dViewState::testPackABGR_alphaDefault()
{
    uint32_t c = packABGR(128, 64, 32);
    QCOMPARE((c >> 24) & 0xFF, (uint32_t)0xFF); // Default alpha = 255
}

//=============================================================================================================

void TestDisp3dViewState::testPackABGR_customAlpha()
{
    uint32_t c = packABGR(10, 20, 30, 128);
    QCOMPARE(c & 0xFF, (uint32_t)10);
    QCOMPARE((c >> 8) & 0xFF, (uint32_t)20);
    QCOMPARE((c >> 16) & 0xFF, (uint32_t)30);
    QCOMPARE((c >> 24) & 0xFF, (uint32_t)128);
}

//=============================================================================================================
// ViewVisibilityProfile tests
//=============================================================================================================

void TestDisp3dViewState::testVisibilityDefaults()
{
    ViewVisibilityProfile vp;
    // Brain hemispheres and BEM surfaces on by default
    QVERIFY(vp.lh);
    QVERIFY(vp.rh);
    QVERIFY(vp.bemHead);
    QVERIFY(vp.bemOuterSkull);
    QVERIFY(vp.bemInnerSkull);
    // Sensors off by default
    QVERIFY(!vp.sensMeg);
    QVERIFY(!vp.sensEeg);
    QVERIFY(!vp.dig);
    QVERIFY(!vp.sourceSpace);
    QVERIFY(!vp.network);
    // Dipoles on
    QVERIFY(vp.dipoles);
}

//=============================================================================================================

void TestDisp3dViewState::testSetObjectVisibleByName()
{
    ViewVisibilityProfile vp;

    // Toggle sensors on
    vp.setObjectVisible("sens_meg", true);
    QVERIFY(vp.sensMeg);

    // Toggle off
    vp.setObjectVisible("sens_meg", false);
    QVERIFY(!vp.sensMeg);

    // Toggle lh off
    vp.setObjectVisible("lh", false);
    QVERIFY(!vp.lh);
}

//=============================================================================================================

void TestDisp3dViewState::testIsObjectVisibleByName()
{
    ViewVisibilityProfile vp;

    QCOMPARE(vp.isObjectVisible("lh"), true);
    QCOMPARE(vp.isObjectVisible("rh"), true);
    QCOMPARE(vp.isObjectVisible("sens_meg"), false);
    QCOMPARE(vp.isObjectVisible("dig"), false);
    QCOMPARE(vp.isObjectVisible("dipoles"), true);

    vp.sensEeg = true;
    QCOMPARE(vp.isObjectVisible("sens_eeg"), true);
}

//=============================================================================================================

void TestDisp3dViewState::testVisibilityLoadSave()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString settingsPath = tmpDir.path() + "/test_vis.ini";
    QString prefix = "testVis_";

    // Save
    {
        QSettings settings(settingsPath, QSettings::IniFormat);
        ViewVisibilityProfile vp;
        vp.lh = false;
        vp.sensMeg = true;
        vp.network = true;
        vp.save(settings, prefix);
    }

    // Load
    {
        QSettings settings(settingsPath, QSettings::IniFormat);
        ViewVisibilityProfile vp;
        vp.load(settings, prefix);
        QCOMPARE(vp.lh, false);
        QCOMPARE(vp.sensMeg, true);
        QCOMPARE(vp.network, true);
        // rh should have been saved as default (true)
        QCOMPARE(vp.rh, true);
    }
}

//=============================================================================================================
// SubView tests
//=============================================================================================================

void TestDisp3dViewState::testSubViewDefaults()
{
    SubView sv;
    QCOMPARE(sv.surfaceType, QString("pial"));
    QCOMPARE(sv.brainShader, Standard);
    QCOMPARE(sv.overlayMode, ModeSurface);
    QCOMPARE(sv.zoom, 0.0f);
    QCOMPARE(sv.preset, 1);
    QVERIFY(sv.enabled);
}

//=============================================================================================================

void TestDisp3dViewState::testIsBrainSurfaceKey()
{
    // Brain surface keys start with lh_ or rh_
    QVERIFY(SubView::isBrainSurfaceKey("lh_pial"));
    QVERIFY(SubView::isBrainSurfaceKey("rh_inflated"));
    QVERIFY(SubView::isBrainSurfaceKey("lh_white"));

    // Non-brain keys
    QVERIFY(!SubView::isBrainSurfaceKey("bem_head"));
    QVERIFY(!SubView::isBrainSurfaceKey("sens_meg_grad_0"));
    QVERIFY(!SubView::isBrainSurfaceKey("dig_cardinal"));
    QVERIFY(!SubView::isBrainSurfaceKey(""));
}

//=============================================================================================================

void TestDisp3dViewState::testMatchesSurfaceType()
{
    SubView sv;
    sv.surfaceType = "pial";

    QVERIFY(sv.matchesSurfaceType("lh_pial"));
    QVERIFY(sv.matchesSurfaceType("rh_pial"));
    QVERIFY(!sv.matchesSurfaceType("lh_inflated"));
    QVERIFY(!sv.matchesSurfaceType("lh_white"));

    sv.surfaceType = "inflated";
    QVERIFY(sv.matchesSurfaceType("lh_inflated"));
    QVERIFY(!sv.matchesSurfaceType("lh_pial"));
}

//=============================================================================================================

void TestDisp3dViewState::testShouldRenderSurface()
{
    SubView sv;
    sv.surfaceType = "pial";
    sv.visibility.lh = true;
    sv.visibility.rh = false;

    // LH pial should render (lh visible, matches surface type)
    QVERIFY(sv.shouldRenderSurface("lh_pial"));
    // RH pial should not render (rh not visible)
    QVERIFY(!sv.shouldRenderSurface("rh_pial"));
    // LH inflated should not render (doesn't match surface type)
    QVERIFY(!sv.shouldRenderSurface("lh_inflated"));
}

//=============================================================================================================

void TestDisp3dViewState::testDefaultForIndex()
{
    // defaultForIndex should return valid SubView for each index
    for (int i = 0; i < 7; ++i) {
        SubView sv = SubView::defaultForIndex(i);
        QVERIFY(!sv.surfaceType.isEmpty());
        QVERIFY(sv.enabled);
    }

    // Different indices should have different presets
    SubView sv0 = SubView::defaultForIndex(0);
    SubView sv1 = SubView::defaultForIndex(1);
    QVERIFY(sv0.preset != sv1.preset || sv0.brainShader != sv1.brainShader);
}

//=============================================================================================================
// Enum/string conversion
//=============================================================================================================

void TestDisp3dViewState::testShaderModeFromName()
{
    QCOMPARE(shaderModeFromName("Standard"), Standard);
    QCOMPARE(shaderModeFromName("Holographic"), Holographic);
    QCOMPARE(shaderModeFromName("Anatomical"), Anatomical);
    QCOMPARE(shaderModeFromName("XRay"), XRay);
}

//=============================================================================================================

void TestDisp3dViewState::testShaderModeName()
{
    QCOMPARE(shaderModeName(Standard), QString("Standard"));
    QCOMPARE(shaderModeName(Holographic), QString("Holographic"));
    QCOMPARE(shaderModeName(Anatomical), QString("Anatomical"));
}

//=============================================================================================================

void TestDisp3dViewState::testVisualizationModeFromName()
{
    QCOMPARE(visualizationModeFromName("Surface"), ModeSurface);
    QCOMPARE(visualizationModeFromName("Annotation"), ModeAnnotation);
    QCOMPARE(visualizationModeFromName("Scientific"), ModeScientific);
    QCOMPARE(visualizationModeFromName("Source Estimate"), ModeSourceEstimate);
}

//=============================================================================================================

void TestDisp3dViewState::testVisualizationModeName()
{
    QCOMPARE(visualizationModeName(ModeSurface), QString("Surface"));
    QCOMPARE(visualizationModeName(ModeAnnotation), QString("Annotation"));
}

//=============================================================================================================
// multiViewPresetName
//=============================================================================================================

void TestDisp3dViewState::testMultiViewPresetNames()
{
    // All 7 presets (0..6) should have non-empty names
    for (int i = 0; i <= 6; ++i) {
        QString name = multiViewPresetName(i);
        QVERIFY2(!name.isEmpty(), qPrintable(QString("Empty name for preset %1").arg(i)));
    }

    // Preset 1 should be "Perspective"
    QCOMPARE(multiViewPresetName(1), QString("Perspective"));
}

//=============================================================================================================

void TestDisp3dViewState::testMultiViewPresetIsPerspective()
{
    // Preset 1 (Perspective) should be perspective
    QVERIFY(multiViewPresetIsPerspective(1));
    // Preset 0 (Top) should not be perspective
    QVERIFY(!multiViewPresetIsPerspective(0));
}

//=============================================================================================================
// surfacekeys
//=============================================================================================================

void TestDisp3dViewState::testSurfaceKeyConstants()
{
    // Verify key string constants are non-empty and match expected prefixes
    QCOMPARE(QString(SURFACEKEYS::kLhPrefix), QString("lh_"));
    QCOMPARE(QString(SURFACEKEYS::kRhPrefix), QString("rh_"));
    QCOMPARE(QString(SURFACEKEYS::kBemHead), QString("bem_head"));
    QCOMPARE(QString(SURFACEKEYS::kBemPrefix), QString("bem_"));
    QCOMPARE(QString(SURFACEKEYS::kSensPrefix), QString("sens_"));
    QCOMPARE(QString(SURFACEKEYS::kDigPrefix), QString("dig_"));
    QCOMPARE(QString(SURFACEKEYS::kDigCardinal), QString("dig_cardinal"));
    QCOMPARE(QString(SURFACEKEYS::kDigHpi), QString("dig_hpi"));
    QCOMPARE(QString(SURFACEKEYS::kDigEeg), QString("dig_eeg"));
    QCOMPARE(QString(SURFACEKEYS::kDigExtra), QString("dig_extra"));
}

//=============================================================================================================

void TestDisp3dViewState::testSensorTypeToObjectKey()
{
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("MEG"), QString("sens_meg"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("MEG/Grad"), QString("sens_meg_grad"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("MEG/Mag"), QString("sens_meg_mag"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("EEG"), QString("sens_eeg"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("Digitizer"), QString("dig"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("Digitizer/Cardinal"), QString("dig_cardinal"));
    QCOMPARE(SURFACEKEYS::sensorTypeToObjectKey("Digitizer/HPI"), QString("dig_hpi"));

    // Unknown type should return empty string
    QVERIFY(SURFACEKEYS::sensorTypeToObjectKey("Unknown").isEmpty());
}

//=============================================================================================================
// mneAnalyzeColor
//=============================================================================================================

void TestDisp3dViewState::testMneAnalyzeColorRange()
{
    // Test that the colormap returns valid colors across the full range
    for (double v = 0.0; v <= 1.0; v += 0.1) {
        QRgb c = mneAnalyzeColor(v);
        QVERIFY(qRed(c) >= 0 && qRed(c) <= 255);
        QVERIFY(qGreen(c) >= 0 && qGreen(c) <= 255);
        QVERIFY(qBlue(c) >= 0 && qBlue(c) <= 255);
    }

    // At 0.5 (zero field) should be gray
    QRgb mid = mneAnalyzeColor(0.5);
    int r = qRed(mid), g = qGreen(mid), b = qBlue(mid);
    // All channels should be similar (grayish)
    QVERIFY(qAbs(r - g) < 30);
    QVERIFY(qAbs(g - b) < 30);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDisp3dViewState)
#include "test_disp3d_viewstate.moc"
