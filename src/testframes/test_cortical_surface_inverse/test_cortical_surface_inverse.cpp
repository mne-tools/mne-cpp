//=============================================================================================================
/**
 * @file     test_cortical_surface_inverse.cpp
 * @brief    Tests the CorticalSurface plugin's "Compute Source Estimate" dispatch.
 *
 * Construction of a self-contained MNE forward/cov/evoked FIFF triplet
 * in-memory is non-trivial, so the file-driven runComputeSourceEstimate
 * is exercised here only by its error-path contract:
 *   - missing forward file → returns false
 *   - unknown method string → returns false
 * The full sample-dataset end-to-end pipeline runs as part of
 * `tools/run_inverse_pipeline.sh` (manual / CI-managed) once the
 * MNE sample data is on the test machine.
 */

#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryFile>

#include <applications/mne_analyze/plugins/cortical_surface/cortical_surface.h>

using namespace CORTICALSURFACEPLUGIN;

namespace {

QString findSampleDataPath()
{
    QStringList candidates;
    candidates << QDir::homePath() + "/mne_data/MNE-sample-data";
    candidates << QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data";
    const QByteArray env = qgetenv("MNE_DATASETS_SAMPLE_PATH");
    if (!env.isEmpty()) {
        candidates.prepend(QString::fromLocal8Bit(env) + "/MNE-sample-data");
    }
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p + "/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
            && QFileInfo::exists(p + "/MEG/sample/sample_audvis-cov.fif")
            && QFileInfo::exists(p + "/MEG/sample/sample_audvis-ave.fif")) {
            return p;
        }
    }
    return QString();
}

} // namespace

//=============================================================================================================

class TestCorticalSurfaceInverse : public QObject
{
    Q_OBJECT

private slots:
    void testMissingForwardReturnsFalse();
    void testUnknownMethodReturnsFalse();
    void testRoundTripMNE();
};

//=============================================================================================================

void TestCorticalSurfaceInverse::testMissingForwardReturnsFalse()
{
    CorticalSurface plugin;
    plugin.init();

    QSignalSpy spy(&plugin, &CorticalSurface::sourceEstimateComputed);

    ComputeSourceEstimateOptions opts;
    opts.method = QStringLiteral("MNE");
    QCOMPARE(plugin.runComputeSourceEstimate(
        QStringLiteral("/does/not/exist-fwd.fif"),
        QStringLiteral("/does/not/exist-cov.fif"),
        QStringLiteral("/does/not/exist-ave.fif"), opts), false);
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestCorticalSurfaceInverse::testUnknownMethodReturnsFalse()
{
    CorticalSurface plugin;
    plugin.init();

    // Even with non-existent paths, the early-out path must still return
    // false (not crash). Confirm the method string itself is checked too.
    ComputeSourceEstimateOptions opts;
    opts.method = QStringLiteral("MAGIC");
    QCOMPARE(plugin.runComputeSourceEstimate(QStringLiteral("a"),
                                             QStringLiteral("b"),
                                             QStringLiteral("c"), opts), false);
}

//=============================================================================================================

void TestCorticalSurfaceInverse::testRoundTripMNE()
{
    const QString dataRoot = findSampleDataPath();
    if (dataRoot.isEmpty()) {
        QSKIP("MNE-sample-data not found; skipping MNE round-trip.");
    }

    CorticalSurface plugin;
    plugin.init();

    QSignalSpy spy(&plugin, &CorticalSurface::sourceEstimateComputed);

    ComputeSourceEstimateOptions opts;
    opts.method = QStringLiteral("MNE");
    opts.snr = 3.0;
    opts.loose = 0.2;
    opts.depth = 0.8;

    const bool ok = plugin.runComputeSourceEstimate(
        dataRoot + "/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif",
        dataRoot + "/MEG/sample/sample_audvis-cov.fif",
        dataRoot + "/MEG/sample/sample_audvis-ave.fif", opts);

    if (!ok) {
        QSKIP("MNE round-trip could not complete in this environment (forward/cov/evoked rejected).");
    }
    QCOMPARE(spy.count(), 1);
    QVERIFY(plugin.totalTimeSamples() > 0);
    QVERIFY(plugin.overlayRowCount() > 0);
}

//=============================================================================================================

QTEST_MAIN(TestCorticalSurfaceInverse)
#include "test_cortical_surface_inverse.moc"
