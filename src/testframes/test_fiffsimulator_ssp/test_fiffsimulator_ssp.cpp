//=============================================================================================================
/**
 * @file     test_fiffsimulator_ssp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Smoke test for the FiffSimulator → WriteToFile replay path:
 *           verifies that SSP projectors and the bad-channel list survive a full
 *           in-memory FIFF round trip through FiffStream::start_writing_raw +
 *           FiffStream::setup_read_raw (the exact APIs WriteToFile drives).
 *           QSKIPped when sample data is not available.
 */

#include <fiff/fiff_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_named_matrix.h>

#include <Eigen/Core>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

using namespace FIFFLIB;

namespace {

QString locateSampleRaw()
{
    QStringList candidates;
    const QByteArray env = qgetenv("MNE_SAMPLE_DATA_DIR");
    if (!env.isEmpty()) {
        candidates << QString::fromLocal8Bit(env) + "/MEG/sample/sample_audvis_raw.fif";
        candidates << QString::fromLocal8Bit(env) + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
    }
    candidates << QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";

    for (const QString& c : candidates) {
        if (QFileInfo::exists(c)) {
            return c;
        }
    }
    return QString();
}

} // namespace

class TestFiffSimulatorSsp : public QObject
{
    Q_OBJECT

private slots:
    void sspAndBadsRoundTripThroughFiffWrite();
};

//=============================================================================================================

void TestFiffSimulatorSsp::sspAndBadsRoundTripThroughFiffWrite()
{
    const QString src = locateSampleRaw();
    if (src.isEmpty()) {
        QSKIP("MNE-sample-data not found (set MNE_SAMPLE_DATA_DIR or place at ~/mne_data/MNE-sample-data)");
    }

    // ---------------------------------------------------------------------
    // 1. Read source raw + info, ensure it has SSP projectors and bad chans.
    // ---------------------------------------------------------------------
    QFile srcFile(src);
    FiffRawData srcRaw;
    QVERIFY2(FiffStream::setup_read_raw(srcFile, srcRaw),
             qPrintable(QStringLiteral("setup_read_raw failed for %1").arg(src)));

    const FiffInfo srcInfo = srcRaw.info;
    QVERIFY2(!srcInfo.projs.isEmpty(),
             "Sample data has no SSP projectors; cannot exercise round-trip.");

    // Inject a couple of bad channels for the round-trip (sample data may have
    // an empty bads list) — pick the first two valid channel names.
    QStringList expectedBads = srcInfo.bads;
    if (expectedBads.isEmpty() && srcInfo.ch_names.size() >= 2) {
        expectedBads << srcInfo.ch_names.at(0) << srcInfo.ch_names.at(1);
    }
    FiffInfo infoForWrite = srcInfo;
    infoForWrite.bads = expectedBads;

    // ---------------------------------------------------------------------
    // 2. Write a short segment via the same APIs used by WriteToFile.
    // ---------------------------------------------------------------------
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString outPath = tmp.filePath(QStringLiteral("ssp_roundtrip_raw.fif"));

    Eigen::RowVectorXd cals;
    {
        QFile outFile(outPath);
        FiffStream::SPtr pOut = FiffStream::start_writing_raw(outFile, infoForWrite, cals);
        QVERIFY2(!pOut.isNull(), "start_writing_raw returned null");

        const fiff_int_t firstSamp = srcRaw.first_samp;
        const fiff_int_t lastSamp  = std::min<fiff_int_t>(
            srcRaw.last_samp, firstSamp + static_cast<fiff_int_t>(srcInfo.sfreq) - 1);

        Eigen::MatrixXd block;
        Eigen::MatrixXd times;
        QVERIFY(srcRaw.read_raw_segment(block, times, firstSamp, lastSamp));
        QVERIFY(pOut->write_raw_buffer(block, cals));
        pOut->finish_writing_raw();
    }
    QVERIFY(QFileInfo::exists(outPath));

    // ---------------------------------------------------------------------
    // 3. Re-read output and compare projectors + bads byte-by-byte.
    // ---------------------------------------------------------------------
    QFile reopened(outPath);
    FiffRawData outRaw;
    QVERIFY(FiffStream::setup_read_raw(reopened, outRaw));

    const FiffInfo& outInfo = outRaw.info;

    // (a) Bad-channel list — exact string equality, same order.
    QCOMPARE(outInfo.bads, expectedBads);

    // (b) Projector list — count, then per-projector kind/active/desc/matrix shape + data.
    QCOMPARE(outInfo.projs.size(), infoForWrite.projs.size());
    for (int i = 0; i < outInfo.projs.size(); ++i) {
        const FiffProj& a = infoForWrite.projs.at(i);
        const FiffProj& b = outInfo.projs.at(i);
        QCOMPARE(b.kind,   a.kind);
        QCOMPARE(b.active, a.active);
        QCOMPARE(b.desc,   a.desc);
        QVERIFY(a.data);
        QVERIFY(b.data);
        QCOMPARE(b.data->nrow,     a.data->nrow);
        QCOMPARE(b.data->ncol,     a.data->ncol);
        QCOMPARE(b.data->col_names, a.data->col_names);
        QCOMPARE(b.data->row_names, a.data->row_names);
        // Byte-equivalent numeric coefficients (FIFF preserves float32 exactly).
        QVERIFY2(b.data->data.isApprox(a.data->data, 0.0),
                 qPrintable(QStringLiteral("Projector %1 (%2) coefficients differ").arg(i).arg(a.desc)));
    }
}

QTEST_MAIN(TestFiffSimulatorSsp)
#include "test_fiffsimulator_ssp.moc"
