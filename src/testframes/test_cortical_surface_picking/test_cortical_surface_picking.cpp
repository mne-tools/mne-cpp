//=============================================================================================================
/**
 * @file     test_cortical_surface_picking.cpp
 * @brief    Tests vertex-picking + Time Course dock for the
 *           cortical_surface mne_analyze plugin.
 */

#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QVector3D>

#include <Eigen/Core>

#include <applications/mne_analyze/plugins/cortical_surface/cortical_surface.h>
#include <fs/fs_surface.h>
#include <inv/inv_source_estimate.h>

using namespace CORTICALSURFACEPLUGIN;
using namespace INVLIB;
using namespace FSLIB;

namespace {

QString findSampleSubjectsDir()
{
    QStringList candidates;
    candidates << QDir::homePath() + "/mne_data/MNE-sample-data/subjects";
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p + "/sample/surf/lh.inflated")) {
            return p;
        }
    }
    return QString();
}

} // namespace

//=============================================================================================================

class TestCorticalSurfacePicking : public QObject
{
    Q_OBJECT

private slots:
    void testPickVertexAndTimeCourse();
    void testExportCsvAndRemove();
    void testPickOnRealSurface();
};

//=============================================================================================================

void TestCorticalSurfacePicking::testPickVertexAndTimeCourse()
{
    CorticalSurface plugin;
    plugin.init();

    // With no surface loaded, pickVertex returns false but exposes a
    // well-defined empty manager state.
    QCOMPARE(plugin.pickedVertexCount(), 0);
    QCOMPARE(plugin.pickVertex(QVector3D(0.0f, 0.0f, 0.0f)), false);
    QCOMPARE(plugin.pickedVertexCount(), 0);

    // Load a tiny STC so timeCourseAt has something to return: vertex i
    // maps to row i, value = i * (t + 1).
    const int nVerts = 4;
    const int nTimes = 6;
    Eigen::MatrixXd data(nVerts, nTimes);
    for (int v = 0; v < nVerts; ++v) {
        for (int t = 0; t < nTimes; ++t) {
            data(v, t) = static_cast<double>(v) * static_cast<double>(t + 1);
        }
    }
    Eigen::VectorXi vertices(nVerts);
    for (int v = 0; v < nVerts; ++v) {
        vertices[v] = v;
    }
    plugin.setSourceEstimate(InvSourceEstimate(data, vertices, 0.0f, 1e-3f));

    // timeCourseAt without a loaded surface still works because the STC
    // vertex list contains the queried indices directly.
    QVector<double> tc = plugin.timeCourseAt(/*hemi=*/0, /*vertex=*/2);
    QCOMPARE(tc.size(), nTimes);
    QCOMPARE(tc.front(), 2.0);            // v=2, t=0 -> 2*1
    QCOMPARE(tc.back(),  2.0 * nTimes);   // v=2, t=last
}

//=============================================================================================================

void TestCorticalSurfacePicking::testExportCsvAndRemove()
{
    CorticalSurface plugin;
    plugin.init();

    const int nTimes = 5;
    Eigen::MatrixXd data(2, nTimes);
    data << 0, 1, 2, 3, 4,
            5, 6, 7, 8, 9;
    Eigen::VectorXi verts(2); verts << 10, 20;
    plugin.setSourceEstimate(InvSourceEstimate(data, verts, 0.0f, 1e-3f));

    // Exporting with no traces fails gracefully.
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();
    QCOMPARE(plugin.exportTimeCoursesCsv(path), false);
}

//=============================================================================================================

void TestCorticalSurfacePicking::testPickOnRealSurface()
{
    const QString subjectsDir = findSampleSubjectsDir();
    if (subjectsDir.isEmpty()) {
        QSKIP("MNE-sample-data subjects/sample/surf/lh.inflated not found.");
    }

    CorticalSurface plugin;
    plugin.init();

    QVERIFY(plugin.loadSurfaces(subjectsDir, QStringLiteral("sample"),
                                HemisphereChoice::LeftOnly,
                                CorticalSurfaceType::Inflated));
    QVERIFY(plugin.surfaceVertexCount() > 0);

    // Read the loaded surface again so we have ground-truth vertex
    // positions to aim the pick at (RayPicker would emit an intersection
    // point near a real triangle).
    FsSurface ref;
    QVERIFY(FsSurface::read(subjectsDir + "/sample/surf/lh.inflated", ref, false));
    QVERIFY(ref.rr().rows() > 0);
    const int targetVertex = ref.rr().rows() / 2;
    const QVector3D worldHit(ref.rr()(targetVertex, 0),
                             ref.rr()(targetVertex, 1),
                             ref.rr()(targetVertex, 2));

    // Synthesise a per-vertex STC so the pick has data to plot.
    const int nVerts = static_cast<int>(ref.rr().rows());
    const int nTimes = 8;
    Eigen::MatrixXd data(nVerts, nTimes);
    for (int v = 0; v < nVerts; ++v) {
        for (int t = 0; t < nTimes; ++t) {
            data(v, t) = static_cast<double>(v + t);
        }
    }
    Eigen::VectorXi vertices(nVerts);
    for (int v = 0; v < nVerts; ++v) {
        vertices[v] = v;
    }
    plugin.setSourceEstimate(InvSourceEstimate(data, vertices, 0.0f, 1e-3f));

    QSignalSpy spy(&plugin, &CorticalSurface::vertexPicked);
    QVERIFY(plugin.pickVertex(worldHit));
    QCOMPARE(spy.count(), 1);
    const auto args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 0);              // lh
    QCOMPARE(args.at(1).toInt(), targetVertex);   // closest vertex is the queried one

    QCOMPARE(plugin.pickedVertexCount(), 1);
    const auto picks = plugin.pickedVertices();
    QCOMPARE(picks.size(), 1);
    QCOMPARE(picks.first().vertex, targetVertex);

    // TimeCoursesDock receives the pick's trace through appendTrace().
    QDockWidget* dock = plugin.getTimeCourseDock();
    QVERIFY(dock != nullptr);
    QVERIFY(dock->widget() != nullptr);
}

//=============================================================================================================

QTEST_MAIN(TestCorticalSurfacePicking)
#include "test_cortical_surface_picking.moc"
