//=============================================================================================================
/**
 * @file     test_cortical_surface_stc.cpp
 * @brief    Tests for STC overlay support in the cortical_surface mne_analyze plugin.
 *
 * Covers the v2.3.0 plugin extensions added in cortical_surface.{h,cpp}:
 *   - Loading a stub source estimate and verifying overlay row count vs. surface mapping.
 *   - Colormap threshold ordering invariant (fThresh <= fMid <= fMax).
 *   - Time-index clamping for the playback slider.
 */

#include <QtTest>
#include <QApplication>

#include <Eigen/Core>

#include <applications/mne_analyze/plugins/cortical_surface/cortical_surface.h>
#include <inv/inv_source_estimate.h>

using namespace CORTICALSURFACEPLUGIN;
using namespace INVLIB;

namespace {

InvSourceEstimate makeStubStc(int nVertices, int nTimes)
{
    Eigen::MatrixXd data = Eigen::MatrixXd::Zero(nVertices, nTimes);
    for (int v = 0; v < nVertices; ++v) {
        for (int t = 0; t < nTimes; ++t) {
            data(v, t) = static_cast<double>(v + 1) * 0.1 * static_cast<double>(t + 1);
        }
    }
    Eigen::VectorXi vertices(nVertices);
    for (int v = 0; v < nVertices; ++v) {
        vertices[v] = v;
    }
    return InvSourceEstimate(data, vertices, 0.0f, 1.0e-3f);
}

} // namespace

//=============================================================================================================

class TestCorticalSurfaceStc : public QObject
{
    Q_OBJECT

private slots:
    void testOverlayRowCount();
    void testColormapThresholdOrdering();
    void testTimeIndexClamping();
};

//=============================================================================================================

void TestCorticalSurfaceStc::testOverlayRowCount()
{
    CorticalSurface plugin;
    plugin.init();

    const int nVerts = 64;
    const int nTimes = 5;
    InvSourceEstimate stc = makeStubStc(nVerts, nTimes);

    QSignalSpy loadSpy(&plugin, &CorticalSurface::sourceEstimateLoaded);
    plugin.setSourceEstimate(stc);

    QCOMPARE(plugin.overlayRowCount(), nVerts);
    QCOMPARE(plugin.totalTimeSamples(), nTimes);
    // No FsSurface loaded in this unit context: overlayMatchesSurface
    // returns true as a vacuous match.
    QCOMPARE(plugin.surfaceVertexCount(), 0);
    QVERIFY(plugin.overlayMatchesSurface());

    QCOMPARE(loadSpy.count(), 1);
    const QList<QVariant> args = loadSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), nVerts);
    QCOMPARE(args.at(1).toInt(), nTimes);
}

//=============================================================================================================

void TestCorticalSurfaceStc::testColormapThresholdOrdering()
{
    CorticalSurface plugin;
    plugin.init();

    // Deliberately pass out-of-order values: the API must sort them so the
    // invariant fThresh <= fMid <= fMax always holds.
    plugin.setColormapThresholds(5.0f, 2.0f, 3.0f);
    QCOMPARE(plugin.fThresh(), 2.0f);
    QCOMPARE(plugin.fMid(),    3.0f);
    QCOMPARE(plugin.fMax(),    5.0f);
    QVERIFY(plugin.fThresh() <= plugin.fMid());
    QVERIFY(plugin.fMid()    <= plugin.fMax());

    // Another permutation.
    plugin.setColormapThresholds(9.0f, -1.0f, 4.0f);
    QCOMPARE(plugin.fThresh(), -1.0f);
    QCOMPARE(plugin.fMid(),     4.0f);
    QCOMPARE(plugin.fMax(),     9.0f);
    QVERIFY(plugin.fThresh() <= plugin.fMid());
    QVERIFY(plugin.fMid()    <= plugin.fMax());
}

//=============================================================================================================

void TestCorticalSurfaceStc::testTimeIndexClamping()
{
    CorticalSurface plugin;
    plugin.init();

    const int nTimes = 10;
    InvSourceEstimate stc = makeStubStc(/*nVertices=*/8, nTimes);
    plugin.setSourceEstimate(stc);
    // setSourceEstimate initialises the current sample to 0.
    QCOMPARE(plugin.currentTimeSample(), 0);

    plugin.setCurrentTimeSample(5);
    QCOMPARE(plugin.currentTimeSample(), 5);

    plugin.setCurrentTimeSample(-42);
    QCOMPARE(plugin.currentTimeSample(), 0);

    plugin.setCurrentTimeSample(99999);
    QCOMPARE(plugin.currentTimeSample(), nTimes - 1);

    plugin.setCurrentTimeSample(3);
    QCOMPARE(plugin.currentTimeSample(), 3);
}

//=============================================================================================================

QTEST_MAIN(TestCorticalSurfaceStc)
#include "test_cortical_surface_stc.moc"
