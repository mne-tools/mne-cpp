// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

//=============================================================================================================
// test_fiff_io_evoked_info.cpp — Tests for FIFF I/O, evoked data sets, and FiffInfo operations
//
// Covers: FiffIO (read/write/setup), FiffEvokedSet (read/pick/compensate/grand average/
//         compute averages/artifact checks/baseline), FiffInfo (write/readMegEeg/make_compensator/
//         get_current_comp/pick_info), FiffDigitizerData (construction/pick fiducials),
//         FiffFileSharer (construction)
//=============================================================================================================

#include <algorithm>
#include <QtTest/QtTest>
#include <memory>
#include <base/string_utils.h>
#include <QDir>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include <QCoreApplication>

#include <Eigen/Dense>

#include <base/log.h>
#include <base/log_qt.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_io.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_file_sharer.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>

using namespace mne;
using namespace Eigen;

//=============================================================================================================

class TestFiffIoEvokedInfo : public QObject
{
    Q_OBJECT

private:
    QString m_sTestDataPath;

    QString rawPath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
    }
    QString avePath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif";
    }
    QString covPath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif";
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── FiffIO ──
    void fiffIO_defaultConstruction();
    void fiffIO_readRawFile();
    void fiffIO_readAveFile();
    void fiffIO_writeRawRoundTrip();
    void fiffIO_setupRead();

    // ── FiffEvokedSet ──
    void evokedSet_defaultConstruction();
    void evokedSet_readFromFile();
    void evokedSet_pickChannels();
    void evokedSet_clear();
    void evokedSet_copyConstruction();
    void evokedSet_saveAndReload();
    void evokedSet_subtractBaseline();
    void evokedSet_grandAverage();
    void evoked_combineMatchesMnePython();
    void evokedSet_checkArtifacts();

    // ── FiffInfo extended ──
    void fiffInfo_defaultConstruction();
    void fiffInfo_copyConstruction();
    void fiffInfo_clear();
    void fiffInfo_pickInfo();
    void fiffInfo_getCurrentComp();
    void fiffInfo_setCurrentComp();
    void fiffInfo_makeCompensator();
    void fiffInfo_writeToStream();
    void fiffInfo_readMegEegChannels();

    // ── FiffDigitizerData ──
    void digitizerData_defaultConstruction();
    void digitizerData_copyConstruction();
    void digitizerData_assignment();
    void digitizerData_readFromFile();
    void digitizerData_pickCardinalFiducials();

    // ── FiffFileSharer ──
    void fileSharer_withDirectory();
};

//=============================================================================================================

void TestFiffIoEvokedInfo::initTestCase()
{
    mne::install_qt_message_handler();

    m_sTestDataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));
}

void TestFiffIoEvokedInfo::cleanupTestCase()
{
}

//=============================================================================================================
// FiffIO tests
//=============================================================================================================

void TestFiffIoEvokedInfo::fiffIO_defaultConstruction()
{
    FiffIO fio;
    QVERIFY(fio.raw.empty());
    QVERIFY(fio.evoked.empty());
}

void TestFiffIoEvokedInfo::fiffIO_readRawFile()
{
    FiffIO fio(rawPath().toStdU16String());
    QCOMPARE(fio.raw.size(), std::size_t(1));
    QVERIFY(fio.raw.front()->info.nchan > 0);

    // The same file held in memory gives the same raw data
    QFile file(rawPath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray contents = file.readAll();
    const auto* begin = reinterpret_cast<const std::byte*>(contents.constData());
    FiffIO inMemory;
    QVERIFY(inMemory.read(std::vector<std::byte>(begin, begin + contents.size())));
    QCOMPARE(inMemory.raw.size(), std::size_t(1));
    QCOMPARE(inMemory.raw.front()->last_samp, fio.raw.front()->last_samp);
    QVERIFY(!inMemory.read(std::vector<std::byte>(16)));
}

void TestFiffIoEvokedInfo::fiffIO_readAveFile()
{
    FiffIO fio;
    QVERIFY(fio.read(avePath().toStdU16String()));
    QCOMPARE(fio.evoked.size(), std::size_t(4));
    QVERIFY(fio.raw.empty());

    // Evoked data sets are written to one file, all of them or the one asked for
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const std::string all = tmpDir.filePath("all-ave.fif").toStdString();
    const std::string second = tmpDir.filePath("second-ave.fif").toStdString();
    QVERIFY(fio.write(all, FIFFB_EVOKED));
    QVERIFY(fio.write(second, FIFFB_EVOKED, 1));
    QVERIFY(!fio.write(second, FIFFB_EVOKED, 4));
    const FiffIO back(all);
    QCOMPARE(back.evoked.size(), std::size_t(4));
    QCOMPARE(back.evoked[2]->comment, fio.evoked[2]->comment);
    QVERIFY(back.evoked[2]->data.isApprox(fio.evoked[2]->data, 1e-6));
    const FiffIO one(second);
    QCOMPARE(one.evoked.size(), std::size_t(1));
    QCOMPARE(one.evoked[0]->comment, fio.evoked[1]->comment);
}

void TestFiffIoEvokedInfo::fiffIO_writeRawRoundTrip()
{
    FiffIO fioIn(rawPath().toStdU16String());
    QVERIFY(!fioIn.raw.empty());

    // Write to temporary file
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/test_write_raw.fif";
    QVERIFY(fioIn.write_raw(outPath.toStdString(), 0));
    const FiffIO back(outPath.toStdString());
    QCOMPARE(back.raw.size(), std::size_t(1));
    QCOMPARE(back.raw.front()->first_samp, fioIn.raw.front()->first_samp);
    QCOMPARE(back.raw.front()->last_samp, fioIn.raw.front()->last_samp);

    // All raw data sets at once go to one file each, named after the given one
    QVERIFY(fioIn.write(outPath.toStdString(), FIFFB_RAW_DATA));
    QVERIFY(QFileInfo(tmpDir.path() + "/test_write_raw_raw-0.fif").size() > 0);

    QVERIFY(!fioIn.write_raw(tmpDir.filePath(QStringLiteral("no/such/dir/out.fif")).toStdString(), 0));
    QVERIFY(!fioIn.write_raw(outPath.toStdString(), 1));
}

void TestFiffIoEvokedInfo::fiffIO_setupRead()
{
    FiffInfo info;
    std::shared_ptr<FiffDirNode> dirTree;

    bool ok = FiffIO::setup_read(rawPath().toStdU16String(), info, dirTree);
    QVERIFY(ok);
    QVERIFY(info.nchan > 0);
    QVERIFY(dirTree != nullptr);
}

//=============================================================================================================
// FiffEvokedSet tests
//=============================================================================================================

void TestFiffIoEvokedInfo::evokedSet_defaultConstruction()
{
    FiffEvokedSet set;
    QVERIFY(set.evoked.empty());
}

void TestFiffIoEvokedInfo::evokedSet_readFromFile()
{
    FiffEvokedSet set(avePath().toStdU16String());
    QVERIFY(!set.evoked.empty());
    QVERIFY(set.info.nchan > 0);

    // Verify evoked data dimensions
    const FiffEvoked& ev = set.evoked.front();
    QVERIFY(ev.data.rows() > 0);
    QVERIFY(ev.data.cols() > 0);
    QCOMPARE(ev.data.rows(), (Index)set.info.nchan);
}

void TestFiffIoEvokedInfo::evokedSet_pickChannels()
{
    FiffEvokedSet set(avePath().toStdU16String());
    QVERIFY2(!set.evoked.empty(), "No evoked data loaded");

    // Pick first 10 channels by name
    StringList include;
    for (int i = 0; i < qMin(10, set.info.nchan); i++) {
        include.push_back(set.info.chs[i].ch_name);
    }

    FiffEvokedSet pickedSet = set.pick_channels(include);
    QVERIFY(!pickedSet.evoked.empty());
    QCOMPARE(pickedSet.info.nchan, include.size());
}

void TestFiffIoEvokedInfo::evokedSet_clear()
{
    FiffEvokedSet set;
    set.clear();
    QVERIFY(set.evoked.empty());
}

void TestFiffIoEvokedInfo::evokedSet_copyConstruction()
{
    const FiffEvokedSet set(avePath().toStdU16String());

    const FiffEvokedSet copy(set); // NOLINT(performance-unnecessary-copy-initialization): the copy is under test
    QCOMPARE(copy.evoked.size(), set.evoked.size());
    QCOMPARE(copy.info.nchan, set.info.nchan);
}

void TestFiffIoEvokedInfo::evokedSet_saveAndReload()
{
    FiffEvokedSet setOrig(avePath().toStdU16String());
    QVERIFY2(!setOrig.evoked.empty(), "No evoked data loaded");

    // Save to temp file
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/test_evoked.fif";
    QVERIFY(setOrig.save(outPath.toStdU16String()));

    // Reload
    FiffEvokedSet setReload(outPath.toStdU16String());
    QCOMPARE(setReload.evoked.size(), setOrig.evoked.size());
    QCOMPARE(setReload.info.nchan, setOrig.info.nchan);

    // Verify data matches (tolerance accounts for double→float→double roundtrip
    // plus SSP projector re-application during reload)
    for (int i = 0; i < std::ssize(setOrig.evoked); i++) {
        double norm = setOrig.evoked[i].data.norm();
        double diff = (setReload.evoked[i].data - setOrig.evoked[i].data).norm();
        double relDiff = (norm > 0) ? diff / norm : diff;
        QVERIFY2(relDiff < 1e-3, qPrintable(QString("Evoked %1 rel diff = %2").arg(i).arg(relDiff)));

        // Per channel, so stim channels (cal = 1) cannot mask MEG/EEG channels
        // whose calibration was applied twice.
        for (int c = 0; c < setOrig.info.nchan; ++c) {
            const double chNorm = setOrig.evoked[i].data.row(c).norm();
            if (chNorm == 0.0)
                continue;
            const double chDiff = (setReload.evoked[i].data.row(c) - setOrig.evoked[i].data.row(c)).norm();
            QVERIFY2(chDiff < 1e-3 * chNorm,
                     qPrintable(QString("Evoked %1 channel %2 rel diff = %3").arg(i).arg(setOrig.info.ch_names[c]).arg(chDiff / chNorm)));
        }
    }
}

void TestFiffIoEvokedInfo::evokedSet_subtractBaseline()
{
    // Create synthetic epoch data
    int nChan = 5;
    int nSamp = 100;
    MatrixXd epoch = MatrixXd::Ones(nChan, nSamp) * 10.0;

    // Add some variation
    for (int c = 0; c < nChan; c++) {
        for (int s = 0; s < nSamp; s++) {
            epoch(c, s) += static_cast<double>(c) + static_cast<double>(s) * 0.01;
        }
    }

    // Subtract baseline from samples 0 to 20
    FiffEvokedSet::subtractBaseline(epoch, 0, 20);

    // After baseline subtraction, the mean of columns 0..20 should be ~0
    for (int c = 0; c < nChan; c++) {
        double baselineMean = epoch.row(c).segment(0, 21).mean();
        QVERIFY2(qAbs(baselineMean) < 1e-10,
                 qPrintable(QString("Ch %1 baseline mean = %2").arg(c).arg(baselineMean)));
    }
}

void TestFiffIoEvokedInfo::evokedSet_grandAverage()
{
    // Reference values produced by mne.grand_average([e0, e1], interpolate_bads=False,
    // drop_bads=True) with MNE-Python 1.11, where e0/e1 are conditions 0 and 1 of
    // sample_audvis-ave.fif read with proj=False.
    FiffEvokedSet set0;
    FiffEvokedSet set1;
    set0.evoked.push_back(FiffEvoked(avePath().toStdU16String(), 0, {}, false));
    set1.evoked.push_back(FiffEvoked(avePath().toStdU16String(), 1, {}, false));
    set0.info = set0.evoked.front().info;
    set1.info = set1.evoked.front().info;

    const FiffEvokedSet grandAvg = FiffEvokedSet::computeGrandAverage({set0, set1});
    QCOMPARE(grandAvg.evoked.size(), 1);
    const FiffEvoked& g = grandAvg.evoked.front();
    QCOMPARE(g.nave, 2);
    QCOMPARE(g.comment, QString("Grand average (n = 2)"));
    QCOMPARE(g.info.nchan, 374);
    QCOMPARE(grandAvg.info.nchan, 374);
    QVERIFY(g.info.bads.empty());
    QVERIFY(!std::ranges::contains(g.info.ch_names, "MEG 2443"));
    QVERIFY(qAbs(g.data(0, 100) / -9.523296577431598e-12 - 1.0) < 1e-6);
    QVERIFY(qAbs(g.data.norm() / 11.161421411316139 - 1.0) < 1e-6);
}

void TestFiffIoEvokedInfo::evoked_combineMatchesMnePython()
{
    // Reference values produced by mne.combine_evoked([e0, e1], weights) with
    // MNE-Python 1.11, where e0/e1 are conditions 0 and 1 of sample_audvis-ave.fif
    // read with proj=False (nave 55 and 61, bads MEG 2443 and EEG 053 in both).
    const FiffEvoked e0(avePath().toStdU16String(), 0, {}, false);
    const FiffEvoked e1(avePath().toStdU16String(), 1, {}, false);
    QCOMPARE(e0.nave, 55);
    QCOMPARE(e1.nave, 61);

    struct Case
    {
        FiffEvoked combined;
        int nave;
        QString comment;
        double d0, d1, norm;
    };
    const QList<Case> cases = {
        {FiffEvoked::combine({e0, e1}, FiffEvoked::CombineWeights::Nave), 116,
         QString::fromUtf8("0.474 × Left Auditory + 0.526 × Right Auditory"),
         -9.4794355164812276e-12, -9.2243206818032336e-06, 11.208676396587776},
        {FiffEvoked::combine({e0, e1}, FiffEvoked::CombineWeights::Equal), 116,
         QString::fromUtf8("0.500 × Left Auditory + 0.500 × Right Auditory"),
         -9.523296577431598e-12, -9.3156511854427821e-06, 11.161421413674288},
        {FiffEvoked::combine({e0, e1}, Vector2d(1.0, -1.0)), 29,
         QString("Left Auditory - Right Auditory"),
         -1.6959610234143224e-12, -3.5314461407291546e-06, 20.796793758029278},
    };
    for (const Case& c : cases) {
        QCOMPARE(c.combined.nave, c.nave);
        QCOMPARE(c.combined.comment, c.comment);
        QCOMPARE(c.combined.info.bads.size(), 2);
        QVERIFY(std::ranges::contains(c.combined.info.bads, "MEG 2443"));
        QVERIFY(std::ranges::contains(c.combined.info.bads, "EEG 053"));
        QVERIFY(qAbs(c.combined.data(0, 100) / c.d0 - 1.0) < 1e-6);
        QVERIFY(qAbs(c.combined.data(315, 200) / c.d1 - 1.0) < 1e-6);
        QVERIFY(qAbs(c.combined.data.norm() / c.norm - 1.0) < 1e-6);
    }

    // Mismatched weights are rejected.
    QVERIFY(FiffEvoked::combine({e0, e1}, VectorXd::Ones(3)).empty());
}

void TestFiffIoEvokedInfo::evokedSet_checkArtifacts()
{
    // Create synthetic epoch
    int nChan = 3;
    int nSamp = 100;
    MatrixXd epoch = MatrixXd::Random(nChan, nSamp) * 1e-12; // Small MEG-scale values

    FiffInfo info;
    // Create minimal channel info
    for (int i = 0; i < nChan; i++) {
        FiffChInfo ch;
        ch.ch_name = (QString("MEG%1").arg(i + 1, 4, 10, QChar('0'))).toStdString();
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_VV_MAG_W;
        ch.unit = FIFF_UNIT_T;
        info.chs.push_back(ch);
        info.ch_names.push_back(ch.ch_name);
    }
    info.nchan = nChan;

    RejectionParams rej;
    rej.megGradReject = 4000e-13f;
    rej.megMagReject = 4e-12f;
    rej.eegReject = 150e-6f;

    std::string reason;
    bool epochOk = FiffEvokedSet::checkArtifacts(epoch, info, {}, rej, reason);
    // Small values should not trigger artifact rejection (returns true = clean)
    QVERIFY(epochOk);

    // Now create epoch with very large values (need variation for peak-to-peak detection)
    MatrixXd badEpoch = MatrixXd::Zero(nChan, nSamp);
    for (int c = 0; c < nChan; c++) {
        badEpoch(c, 0) = -1e-9;
        badEpoch(c, 1) = 1e-9; // pp = 2e-9 T, much larger than 4e-12 T reject
    }
    bool badEpochOk = FiffEvokedSet::checkArtifacts(badEpoch, info, {}, rej, reason);
    // Large peak-to-peak should trigger rejection (returns false = artifact found)
    QVERIFY(!badEpochOk);
}

//=============================================================================================================
// FiffInfo extended tests
//=============================================================================================================

void TestFiffIoEvokedInfo::fiffInfo_defaultConstruction()
{
    FiffInfo info;
    QCOMPARE(info.nchan, -1);
    QCOMPARE(info.sfreq, -1.0f);
    QVERIFY(info.chs.empty());
}

void TestFiffIoEvokedInfo::fiffInfo_copyConstruction()
{
    FiffRawData raw(rawPath().toStdString());
    FiffInfo copy(raw.info);

    QCOMPARE(copy.nchan, raw.info.nchan);
    QVERIFY(qAbs(copy.sfreq - raw.info.sfreq) < 0.01);
    QCOMPARE(copy.chs.size(), raw.info.chs.size());
}

void TestFiffIoEvokedInfo::fiffInfo_clear()
{
    FiffRawData raw(rawPath().toStdString());
    FiffInfo info = raw.info;
    QVERIFY(info.nchan > 0);

    info.clear();
    QCOMPARE(info.nchan, -1);
    QVERIFY(info.chs.empty());
}

void TestFiffIoEvokedInfo::fiffInfo_pickInfo()
{
    FiffRawData raw(rawPath().toStdString());

    // Pick first 10 channels
    RowVectorXi sel(10);
    for (int i = 0; i < 10; i++)
        sel(i) = i;

    FiffInfo picked = raw.info.pick_info(sel);
    QCOMPARE(picked.nchan, 10);
    QCOMPARE(picked.chs.size(), 10);
    QCOMPARE(picked.ch_names.size(), 10);
}

void TestFiffIoEvokedInfo::fiffInfo_getCurrentComp()
{
    FiffRawData raw(rawPath().toStdString());

    qint32 comp = raw.info.get_current_comp();
    QVERIFY(comp >= 0); // Neuromag data: grade 0
}

void TestFiffIoEvokedInfo::fiffInfo_setCurrentComp()
{
    FiffRawData raw(rawPath().toStdString());
    FiffInfo info = raw.info;

    info.set_current_comp(0);
    QCOMPARE(info.get_current_comp(), (qint32)0);
}

void TestFiffIoEvokedInfo::fiffInfo_makeCompensator()
{
    FiffRawData raw(rawPath().toStdString());

    FiffCtfComp comp;
    bool ok = raw.info.make_compensator(0, 0, comp);
    // from==to returns false (identity, no transformation needed)
    QVERIFY(!ok);
    // The data should be identity matrix
    QVERIFY(comp.data.data.rows() == raw.info.nchan);
    QVERIFY(comp.data.data.cols() == raw.info.nchan);
}

void TestFiffIoEvokedInfo::fiffInfo_writeToStream()
{
    FiffRawData raw(rawPath().toStdString());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/info_test.fif";
    QFile outFile(outPath);
    QVERIFY(outFile.open(QIODevice::WriteOnly));

    auto pStream = FiffStream::start_file(outPath.toStdU16String());
    pStream->start_block(FIFFB_MEAS);
    raw.info.writeToStream(*pStream);
    pStream->end_block(FIFFB_MEAS);
    pStream->end_file();
    outFile.close();

    QVERIFY(QFileInfo(outPath).size() > 0);
}

void TestFiffIoEvokedInfo::fiffInfo_readMegEegChannels()
{
    std::vector<FiffChInfo> chsp;
    int nmeg = 0, neeg = 0;

    bool ok = FiffInfo::readMegEegChannels(rawPath().toStdString(), true, true, {},
                                           chsp, nmeg, neeg);
    QVERIFY(ok);
    QVERIFY(nmeg > 0); // Should find MEG channels
    QVERIFY(std::ssize(chsp) == nmeg + neeg);
}

//=============================================================================================================
// FiffDigitizerData tests
//=============================================================================================================

void TestFiffIoEvokedInfo::digitizerData_defaultConstruction()
{
    FiffDigitizerData dd;
    QVERIFY(dd.points.empty());
    QCOMPARE(dd.coord_frame, FIFFV_COORD_UNKNOWN);
    QVERIFY(!dd.dist_valid);
}

void TestFiffIoEvokedInfo::digitizerData_copyConstruction()
{
    FiffDigitizerData orig;
    orig.points.push_back(FiffDigPoint());
    orig.coord_frame = FIFFV_COORD_HEAD;
    orig.head_mri_t = FiffCoordTrans(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, Matrix4f::Identity());

    FiffDigitizerData copy(orig);
    QCOMPARE(copy.points.size(), std::size_t{1});
    QVERIFY(copy.head_mri_t.has_value());
    QCOMPARE(copy.coord_frame, (int)FIFFV_COORD_HEAD);
}

void TestFiffIoEvokedInfo::digitizerData_assignment()
{
    FiffDigitizerData orig;
    orig.filename = "dig.fif";
    orig.coord_frame = FIFFV_COORD_MRI;

    FiffDigitizerData assigned;
    assigned = orig;
    QCOMPARE(assigned.filename, std::filesystem::path("dig.fif"));
    QCOMPARE(assigned.coord_frame, (int)FIFFV_COORD_MRI);
}

void TestFiffIoEvokedInfo::digitizerData_readFromFile()
{
    const FiffDigitizerData dd(rawPath().toStdString());
    QCOMPARE(dd.points.size(), std::size_t{146});
    QCOMPARE(dd.coord_frame, FIFFV_COORD_HEAD);
}

void TestFiffIoEvokedInfo::digitizerData_pickCardinalFiducials()
{
    FiffDigitizerData dd;

    // Add cardinal fiducial points
    FiffDigPoint dp1;
    dp1.kind = FIFFV_POINT_CARDINAL;
    dp1.ident = FIFFV_POINT_LPA;
    dp1.r[0] = -0.07f;
    dp1.r[1] = 0.0f;
    dp1.r[2] = 0.0f;
    dp1.coord_frame = FIFFV_COORD_HEAD;
    dd.points.push_back(dp1);

    FiffDigPoint dp2;
    dp2.kind = FIFFV_POINT_CARDINAL;
    dp2.ident = FIFFV_POINT_RPA;
    dp2.r[0] = 0.07f;
    dp2.r[1] = 0.0f;
    dp2.r[2] = 0.0f;
    dp2.coord_frame = FIFFV_COORD_HEAD;
    dd.points.push_back(dp2);

    FiffDigPoint dp3;
    dp3.kind = FIFFV_POINT_CARDINAL;
    dp3.ident = FIFFV_POINT_NASION;
    dp3.r[0] = 0.0f;
    dp3.r[1] = 0.1f;
    dp3.r[2] = 0.0f;
    dp3.coord_frame = FIFFV_COORD_HEAD;
    dd.points.push_back(dp3);

    dd.coord_frame = FIFFV_COORD_HEAD;

    // Without the adjusted transform there are no MRI fiducials
    dd.pick_cardinal_fiducials();
    QCOMPARE(dd.nfids(), 0);

    Matrix4f shift = Matrix4f::Identity();
    shift(2, 3) = 0.01f;
    dd.head_mri_t_adj = FiffCoordTrans(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, shift);
    dd.pick_cardinal_fiducials();
    QCOMPARE(dd.nfids(), 3);
    QCOMPARE(dd.mri_fids[2].ident, FIFFV_POINT_NASION);
    QCOMPARE(dd.mri_fids[2].r[2], 0.01f);
}

//=============================================================================================================
// FiffFileSharer tests
//=============================================================================================================

void TestFiffIoEvokedInfo::fileSharer_withDirectory()
{
    // A producer copies two recordings into the shared directory; the consumer gets both, in order, each
    // complete (it reads back with all samples), and nothing twice.
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const std::filesystem::path shared = std::filesystem::path(tmpDir.path().toStdU16String()) / "shared";
    FiffFileSharer consumer(shared);
    QVERIFY(consumer.clear());
    QCOMPARE(consumer.directory(), shared);
    FiffFileSharer producer(shared);
    QVERIFY(producer.copy_realtime_file(rawPath().toStdU16String()));
    const auto second = producer.copy_realtime_file(rawPath().toStdU16String());
    QVERIFY(second);
    QCOMPARE(second->filename().string(), std::string("realtime_file1_raw.fif"));

    const std::vector<std::filesystem::path> arrived = consumer.new_files();
    QCOMPARE(arrived.size(), std::size_t(2));
    QCOMPARE(arrived[0].filename().string(), std::string("realtime_file0_raw.fif"));
    QCOMPARE(arrived[1], *second);
    for (const std::filesystem::path& path : arrived) {
        const FiffRawData raw(path);
        QCOMPARE(raw.last_samp - raw.first_samp + 1, 6007);
    }
    QVERIFY(consumer.new_files().empty());

    // A source that is missing leaves nothing behind; clear() empties the directory and starts over
    QCOMPARE(producer.copy_realtime_file(shared / "missing.fif").error().code, mne::ErrorCode::Io);
    QVERIFY(consumer.new_files().empty());
    QVERIFY(consumer.clear());
    QVERIFY(std::filesystem::is_empty(shared));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffIoEvokedInfo)

#include "test_fiff_io_evoked_info.moc"

#include <vector>
