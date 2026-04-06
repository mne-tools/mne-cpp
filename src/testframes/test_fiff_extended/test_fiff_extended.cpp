#include <QtTest/QtTest>
#include <QBuffer>
#include <QTemporaryFile>
#include <Eigen/Dense>

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_info_base.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_coord_trans.h>

using namespace FIFFLIB;
using namespace Eigen;

class TestFiffExtended : public QObject
{
    Q_OBJECT

private:
    QString dataPath;

    FiffInfo createSyntheticInfo(int nCh = 10) {
        FiffInfo info;
        info.sfreq = 1000.0f;
        info.highpass = 0.1f;
        info.lowpass = 100.0f;
        info.linefreq = 60.0f;

        for (int i = 0; i < nCh; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("EEG %1").arg(i + 1, 3, 10, QChar('0'));
            ch.kind = FIFFV_EEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
            ch.unit = FIFF_UNIT_V;
            ch.cal = 1.0;
            ch.range = 1.0;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }
        info.nchan = nCh;
        return info;
    }

    FiffCov createSyntheticCov(int dim = 5) {
        FiffCov cov;
        cov.kind = FIFFV_MNE_NOISE_COV;
        cov.dim = dim;
        cov.diag = false;
        cov.nfree = 100;
        MatrixXd covData = MatrixXd::Identity(dim, dim) * 1e-12;
        // Add some off-diagonal elements
        for (int i = 0; i < dim - 1; ++i) {
            covData(i, i + 1) = 0.5e-12;
            covData(i + 1, i) = 0.5e-12;
        }
        cov.data = covData;
        for (int i = 0; i < dim; ++i) {
            cov.names.append(QString("EEG %1").arg(i + 1, 3, 10, QChar('0')));
        }
        return cov;
    }

private slots:
    void initTestCase() {
        dataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    }

    // === FiffCov tests ===
    void testFiffCovDefaultCtor()
    {
        FiffCov cov;
        QVERIFY(cov.isEmpty());
    }

    void testFiffCovCopyCtor()
    {
        FiffCov cov = createSyntheticCov(5);
        FiffCov cov2(cov);
        QCOMPARE(cov2.dim, 5);
        QCOMPARE(cov2.names.size(), 5);
    }

    void testFiffCovAssignment()
    {
        FiffCov cov = createSyntheticCov(5);
        FiffCov cov2;
        cov2 = cov;
        QCOMPARE(cov2.dim, cov.dim);
        QCOMPARE(cov2.data.rows(), cov.data.rows());
    }

    void testFiffCovClear()
    {
        FiffCov cov = createSyntheticCov(5);
        cov.clear();
        QVERIFY(cov.isEmpty());
    }

    void testFiffCovPickChannels()
    {
        FiffCov cov = createSyntheticCov(5);
        QStringList include;
        include << "EEG 001" << "EEG 003";
        FiffCov picked = cov.pick_channels(include);
        QCOMPARE(picked.dim, 2);
        QCOMPARE(picked.names.size(), 2);
        QCOMPARE(picked.data.rows(), 2);
        QCOMPARE(picked.data.cols(), 2);
    }

    void testFiffCovPickChannelsExclude()
    {
        FiffCov cov = createSyntheticCov(5);
        QStringList exclude;
        exclude << "EEG 001";
        FiffCov picked = cov.pick_channels(QStringList(), exclude);
        QCOMPARE(picked.dim, 4);
    }

    void testFiffCovSave()
    {
        FiffCov cov = createSyntheticCov(4);
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString tmpPath = tmpFile.fileName();
        tmpFile.close();

        bool saved = cov.save(tmpPath);
        QVERIFY(saved);
        QVERIFY(QFileInfo(tmpPath).size() > 0);
    }

    void testFiffCovGrandAverage()
    {
        FiffCov cov1 = createSyntheticCov(3);
        cov1.nfree = 50;
        FiffCov cov2 = createSyntheticCov(3);
        cov2.nfree = 100;
        cov2.data *= 2.0;

        QList<FiffCov> covList;
        covList << cov1 << cov2;

        FiffCov avg = FiffCov::computeGrandAverage(covList);
        QCOMPARE(avg.dim, 3);
        QCOMPARE(avg.nfree, 150);
        // Weighted average: (50*1e-12 + 100*2e-12) / 150 = 250e-12/150
        double expected = (50.0 * 1e-12 + 100.0 * 2e-12) / 150.0;
        QVERIFY(std::abs(avg.data(0, 0) - expected) < 1e-20);
    }

    void testFiffCovStreamOutput()
    {
        FiffCov cov = createSyntheticCov(3);
        std::ostringstream oss;
        oss << cov;
        QVERIFY(!oss.str().empty());
    }

    void testFiffCovFromSampleFile()
    {
        QString covPath = dataPath + "/MEG/sample/sample_audvis-cov.fif";
        if (!QFile::exists(covPath)) {
            QSKIP("Sample covariance file not found");
        }
        QFile file(covPath);
        FiffCov cov(file);
        QVERIFY(!cov.isEmpty());
        QVERIFY(cov.dim > 0);
        QVERIFY(cov.names.size() > 0);
    }

    void testFiffCovRegularize()
    {
        QString covPath = dataPath + "/MEG/sample/sample_audvis-cov.fif";
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(covPath) || !QFile::exists(rawPath)) {
            QSKIP("Sample data not found");
        }
        QFile covFile(covPath);
        FiffCov cov(covFile);

        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);

        FiffCov reg = cov.regularize(raw.info, 0.1, 0.1, 0.1, true);
        QCOMPARE(reg.dim, cov.dim);
    }

    void testFiffCovPrepareNoiseCov()
    {
        QString covPath = dataPath + "/MEG/sample/sample_audvis-cov.fif";
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(covPath) || !QFile::exists(rawPath)) {
            QSKIP("Sample data not found");
        }
        QFile covFile(covPath);
        FiffCov cov(covFile);

        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);

        // prepare_noise_cov requires only MEG+EEG channels that exist in both info and cov
        // Must also exclude bad channels since prepare_noise_cov excludes them internally
        RowVectorXi picks = raw.info.pick_types(true, true, false, QStringList(), raw.info.bads);
        QStringList chNames;
        for(int i = 0; i < picks.size(); ++i) {
            if(cov.names.contains(raw.info.ch_names[picks(i)]))
                chNames << raw.info.ch_names[picks(i)];
        }
        FiffCov prepared = cov.prepare_noise_cov(raw.info, chNames);
        QVERIFY(!prepared.isEmpty());
    }

    // === FiffEvents tests ===
    void testFiffEventsDefaultCtor()
    {
        FiffEvents events;
        QCOMPARE(events.num_events(), 0);
    }

    void testFiffEventsWriteReadAsciiRoundTrip()
    {
        // Create events
        MatrixXi eventData(3, 3);
        eventData << 100, 0, 1,
                     200, 0, 2,
                     300, 0, 1;

        FiffEvents events;
        events.events = eventData;

        // Write to buffer
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        events.write_to_ascii(buffer, 1000.0f);

        // Read back
        buffer.seek(0);
        FiffEvents eventsRead;
        bool ok = FiffEvents::read_from_ascii(buffer, eventsRead);
        QVERIFY(ok);
        QCOMPARE(eventsRead.num_events(), 3);
    }

    void testFiffEventsWriteReadFifRoundTrip()
    {
        MatrixXi eventData(3, 3);
        eventData << 100, 0, 1,
                     200, 0, 2,
                     300, 0, 1;

        FiffEvents events;
        events.events = eventData;

        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        events.write_to_fif(buffer);

        buffer.seek(0);
        FiffEvents eventsRead;
        bool ok = FiffEvents::read_from_fif(buffer, eventsRead);
        QVERIFY(ok);
        QCOMPARE(eventsRead.num_events(), 3);
    }

    // === FiffInfo tests ===
    void testFiffInfoDefaultCtor()
    {
        FiffInfo info;
        QCOMPARE(info.nchan, -1);
    }

    void testFiffInfoPickInfo()
    {
        FiffInfo info = createSyntheticInfo(10);
        RowVectorXi sel(3);
        sel << 0, 3, 7;
        FiffInfo picked = info.pick_info(sel);
        QCOMPARE(picked.nchan, 3);
        QCOMPARE(picked.chs.size(), 3);
        QCOMPARE(picked.ch_names.size(), 3);
        QCOMPARE(picked.ch_names[0], QString("EEG 001"));
        QCOMPARE(picked.ch_names[1], QString("EEG 004"));
    }

    void testFiffInfoGetCurrentComp()
    {
        FiffInfo info = createSyntheticInfo(5);
        qint32 comp = info.get_current_comp();
        QCOMPARE(comp, 0); // No compensation set
    }

    void testFiffInfoSetCurrentComp()
    {
        FiffInfo info = createSyntheticInfo(5);
        // set_current_comp(value) sets the comp field on MEG channels
        // Our channels are EEG so this should work but have no effect
        info.set_current_comp(3);
        // Static version
        QList<FiffChInfo> chs = info.chs;
        QList<FiffChInfo> result = FiffInfo::set_current_comp(chs, 0);
        QCOMPARE(result.size(), 5);
    }

    void testFiffInfoPrint()
    {
        FiffInfo info = createSyntheticInfo(5);
        // Just verify it doesn't crash
        info.print();
        QVERIFY(true);
    }

    void testFiffInfoWriteToStream()
    {
        FiffInfo info = createSyntheticInfo(5);
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        RowVectorXd cals;
        FiffStream::SPtr pStream = FiffStream::start_writing_raw(buffer, info, cals);
        QVERIFY(!pStream.isNull());
        pStream->finish_writing_raw();
    }

    void testFiffInfoFromRawFile()
    {
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);
        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);

        // Test pick_info
        RowVectorXi sel(5);
        sel << 0, 1, 2, 3, 4;
        FiffInfo picked = raw.info.pick_info(sel);
        QCOMPARE(picked.nchan, 5);
    }

    // === FiffProj tests ===
    void testFiffProjActivate()
    {
        FiffProj proj;
        proj.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF;
        proj.active = false;
        proj.desc = "Average EEG reference";
        // Build a simple 2-channel projector
        FiffNamedMatrix::SDPtr projData(new FiffNamedMatrix());
        projData->nrow = 1;
        projData->ncol = 3;
        projData->row_names << "EEG avg";
        projData->col_names << "EEG 001" << "EEG 002" << "EEG 003";
        projData->data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        proj.data = projData;

        QList<FiffProj> projList;
        projList << proj;

        FiffProj::activate_projs(projList);
        QVERIFY(projList[0].active);
    }

    void testFiffProjMakeProjector()
    {
        // Create a simple EEG average reference projector
        FiffProj proj;
        proj.kind = FIFFV_MNE_PROJ_ITEM_EEG_AVREF;
        proj.active = true;
        proj.desc = "Average EEG reference";

        FiffNamedMatrix::SDPtr projData(new FiffNamedMatrix());
        projData->nrow = 1;
        projData->ncol = 3;
        projData->row_names << "EEG avg";
        projData->col_names << "EEG 001" << "EEG 002" << "EEG 003";
        projData->data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        proj.data = projData;

        QList<FiffProj> projList;
        projList << proj;

        QStringList chNames;
        chNames << "EEG 001" << "EEG 002" << "EEG 003";

        MatrixXd projMat;
        fiff_int_t nProj = FiffProj::make_projector(projList, chNames, projMat);

        QVERIFY(nProj > 0);
        QCOMPARE(projMat.rows(), 3);
        QCOMPARE(projMat.cols(), 3);
    }

    // === FiffEvokedSet tests ===
    void testFiffEvokedSetReadFromFile()
    {
        QString avePath = dataPath + "/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath)) {
            QSKIP("Sample evoked file not found");
        }
        QFile file(avePath);
        FiffEvokedSet evokedSet(file);
        QVERIFY(evokedSet.evoked.size() > 0);
    }

    void testFiffEvokedSetPickChannels()
    {
        QString avePath = dataPath + "/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath)) {
            QSKIP("Sample evoked file not found");
        }
        QFile file(avePath);
        FiffEvokedSet evokedSet(file);

        QStringList include;
        include << evokedSet.info.ch_names[0] << evokedSet.info.ch_names[1];
        FiffEvokedSet picked = evokedSet.pick_channels(include);
        QCOMPARE(picked.info.nchan, 2);
    }

    void testFiffEvokedSetSubtractBaseline()
    {
        int nCh = 3, nSamples = 100;
        MatrixXd epoch = MatrixXd::Random(nCh, nSamples);
        // Add a known offset
        epoch.array() += 5.0;
        QPair<float, float> baseline(0, 50);

        FiffEvokedSet::subtractBaseline(epoch, 0, 49);

        // Mean of baseline region (samples 0..49) per channel should be ~0
        for (int ch = 0; ch < nCh; ++ch) {
            double mean = epoch.row(ch).head(50).mean();
            QVERIFY(std::abs(mean) < 1e-10);
        }
    }

    // === FiffRawData tests ===
    void testFiffRawDataClear()
    {
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);
        QVERIFY(raw.info.nchan > 0);
        raw.clear();
        QCOMPARE(raw.info.nchan, -1);
    }

    void testFiffRawDataReadSegment()
    {
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);

        MatrixXd data;
        MatrixXd times;
        raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 999);
        QCOMPARE(data.cols(), 1000);
        QCOMPARE(data.rows(), raw.info.nchan);
    }

    void testFiffRawDataReadSegmentWithPicks()
    {
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);

        RowVectorXi sel(5);
        sel << 0, 1, 2, 3, 4;

        MatrixXd data;
        MatrixXd times;
        raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 499, sel);
        QCOMPARE(data.rows(), 5);
        QCOMPARE(data.cols(), 500);
    }

    void testFiffRawDataSaveInfo()
    {
        QString rawPath = dataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile rawFile(rawPath);
        FiffRawData raw(rawFile);

        // Verify info was loaded correctly
        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);
        QVERIFY(!raw.info.chs.isEmpty());
    }

    // === FiffDigPointSet tests ===
    void testFiffDigPointSetCtor()
    {
        FiffDigPointSet dps;
        QCOMPARE(dps.size(), 0);
    }

    // === FiffCoordTrans tests ===
    void testFiffCoordTransIdentity()
    {
        FiffCoordTrans trans;
        QVERIFY(trans.trans.isIdentity(1e-10));
    }
};

QTEST_GUILESS_MAIN(TestFiffExtended)
#include "test_fiff_extended.moc"
