//=============================================================================================================
/**
 * @file     test_cortical_surface_labels.cpp
 * @brief    Tests the CorticalSurface plugin's Labels dock plumbing.
 *
 * Builds an in-memory @ref FSLIB::FsLabel, round-trips it through
 * `saveLabel` + @ref FSLIB::FsLabel::read, and extracts an ROI mean
 * trace via @ref CorticalSurface::extractLabelTimeCourse.
 */

#include <QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>

#include <Eigen/Core>

#include <applications/mne_analyze/plugins/cortical_surface/cortical_surface.h>
#include <fs/fs_label.h>
#include <inv/inv_source_estimate.h>

using namespace CORTICALSURFACEPLUGIN;
using namespace FSLIB;
using namespace INVLIB;

namespace {

FsLabel makeLabel(int hemi)
{
    Eigen::VectorXi v(3);
    v << 0, 1, 2;
    Eigen::MatrixX3f pos(3, 3);
    pos << 0.001f, 0.0f, 0.0f,
           0.002f, 0.0f, 0.0f,
           0.003f, 0.0f, 0.0f;
    Eigen::VectorXd vals = Eigen::VectorXd::Ones(3);
    return FsLabel(v, pos, vals, hemi, QStringLiteral("test-roi"));
}

InvSourceEstimate makeStcCovering(const Eigen::VectorXi& verts, int nTimes)
{
    Eigen::MatrixXd data(verts.size(), nTimes);
    for (int r = 0; r < verts.size(); ++r) {
        for (int t = 0; t < nTimes; ++t) {
            data(r, t) = static_cast<double>(r + 1);
        }
    }
    return InvSourceEstimate(data, verts, 0.0f, 1e-3f);
}

} // namespace

//=============================================================================================================

class TestCorticalSurfaceLabels : public QObject
{
    Q_OBJECT

private slots:
    void testSaveAndReloadLabel();
    void testExtractTimeCourseMean();
    void testLoadMissingFiles();
};

//=============================================================================================================

void TestCorticalSurfaceLabels::testSaveAndReloadLabel()
{
    CorticalSurface plugin;
    plugin.init();

    FsLabel lbl = makeLabel(/*hemi=*/0);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString outPath = dir.filePath(QStringLiteral("lh.test.label"));
    QVERIFY(plugin.saveLabel(lbl, outPath));
    QVERIFY(QFile::exists(outPath));

    FsLabel reloaded;
    QVERIFY(FsLabel::read(outPath, reloaded));
    QCOMPARE(static_cast<int>(reloaded.vertices.size()), 3);
    QCOMPARE(reloaded.vertices[0], 0);
    QCOMPARE(reloaded.vertices[1], 1);
    QCOMPARE(reloaded.vertices[2], 2);
    QCOMPARE(reloaded.hemi, 0);
}

//=============================================================================================================

void TestCorticalSurfaceLabels::testExtractTimeCourseMean()
{
    CorticalSurface plugin;
    plugin.init();

    Eigen::VectorXi stcVerts(3);
    stcVerts << 0, 1, 2;
    const int nTimes = 4;
    plugin.setSourceEstimate(makeStcCovering(stcVerts, nTimes));

    FsLabel lbl = makeLabel(/*hemi=*/0);
    QSignalSpy spy(&plugin, &CorticalSurface::labelsChanged);
    QCOMPARE(plugin.extractLabelTimeCourse(lbl, QStringLiteral("mean")), true);
    // Sanity check that the extracted trace shows up as a non-pick entry
    // (vertex == -1) and that no pickedVertices were added.
    QCOMPARE(plugin.pickedVertexCount(), 0);

    // All five reduction modes must succeed against the same STC.
    const QStringList modes = { QStringLiteral("auto"),
                                QStringLiteral("mean_flip"),
                                QStringLiteral("pca_flip"),
                                QStringLiteral("max") };
    for (const QString& mode : modes) {
        QVERIFY2(plugin.extractLabelTimeCourse(lbl, mode),
                 qPrintable(QStringLiteral("mode '%1' failed").arg(mode)));
    }
}

//=============================================================================================================

void TestCorticalSurfaceLabels::testLoadMissingFiles()
{
    CorticalSurface plugin;
    plugin.init();
    QCOMPARE(plugin.loadLabel(QStringLiteral("/does/not/exist.label")), 0);
    QCOMPARE(plugin.loadAnnot(QStringLiteral("/does/not/exist.annot")), 0);
}

//=============================================================================================================

QTEST_MAIN(TestCorticalSurfaceLabels)
#include "test_cortical_surface_labels.moc"
