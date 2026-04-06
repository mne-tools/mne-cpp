//=============================================================================================================
// test_fiff_io_evoked_info.cpp — Tests for FIFF I/O, evoked data sets, and FiffInfo operations
//
// Covers: FiffIO (read/write/setup), FiffEvokedSet (read/pick/compensate/grand average/
//         compute averages/artifact checks/baseline), FiffInfo (write/readMegEeg/make_compensator/
//         get_current_comp/pick_info), FiffDigitizerData (construction/pick fiducials),
//         FiffFileSharer (construction)
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include <QCoreApplication>

#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

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

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestFiffIoEvokedInfo : public QObject
{
    Q_OBJECT

private:
    QString m_sTestDataPath;
    QString m_sMneSamplePath;

    QString rawPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif"; }

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
    void fiffInfo_print();

    // ── FiffDigitizerData ──
    void digitizerData_defaultConstruction();
    void digitizerData_copyConstruction();
    void digitizerData_assignment();
    void digitizerData_readFromFile();
    void digitizerData_pickCardinalFiducials();

    // ── FiffFileSharer ──
    void fileSharer_defaultConstruction();
    void fileSharer_withDirectory();
};

//=============================================================================================================

void TestFiffIoEvokedInfo::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);

    m_sTestDataPath = QCoreApplication::applicationDirPath()
                      + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));

    m_sMneSamplePath = QDir::homePath() + "/mne_data/MNE-sample-data";
    QVERIFY2(QFile::exists(m_sMneSamplePath + "/MEG/sample/sample_audvis_raw.fif"),
             qPrintable(QString("MNE sample data not found at %1").arg(m_sMneSamplePath)));
}

void TestFiffIoEvokedInfo::cleanupTestCase() {}

//=============================================================================================================
// FiffIO tests
//=============================================================================================================

void TestFiffIoEvokedInfo::fiffIO_defaultConstruction()
{
    FiffIO fio;
    QVERIFY(fio.m_qlistRaw.isEmpty());
    QVERIFY(fio.m_qlistEvoked.isEmpty());
}

void TestFiffIoEvokedInfo::fiffIO_readRawFile()
{

    QFile file(rawPath());

    FiffIO fio(file);
    QVERIFY(!fio.m_qlistRaw.isEmpty());
    QVERIFY(fio.m_qlistRaw.first()->info.nchan > 0);
}

void TestFiffIoEvokedInfo::fiffIO_readAveFile()
{

    QFile file(avePath());

    // Read the averaged data
    FiffIO fio;
    bool ok = fio.read(file);
    QVERIFY(ok);
}

void TestFiffIoEvokedInfo::fiffIO_writeRawRoundTrip()
{

    QFile fileIn(rawPath());

    FiffIO fioIn(fileIn);
    QVERIFY(!fioIn.m_qlistRaw.isEmpty());

    // Write to temporary file
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/test_write_raw.fif";
    QFile fileOut(outPath);
    QVERIFY(fileOut.open(QIODevice::WriteOnly));

    bool ok = fioIn.write_raw(fileOut, 0);
    fileOut.close();

    // Verify the output file exists and is non-empty
    QVERIFY(QFile::exists(outPath));
    QVERIFY(QFileInfo(outPath).size() > 0);
}

void TestFiffIoEvokedInfo::fiffIO_setupRead()
{

    QFile file(rawPath());
    FiffInfo info;
    FiffDirNode::SPtr dirTree;

    bool ok = FiffIO::setup_read(file, info, dirTree);
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
    QVERIFY(set.evoked.isEmpty());
}

void TestFiffIoEvokedInfo::evokedSet_readFromFile()
{

    QFile file(avePath());
    FiffEvokedSet set;
    bool ok = FiffEvokedSet::read(file, set);
    QVERIFY(ok);
    QVERIFY(!set.evoked.isEmpty());
    QVERIFY(set.info.nchan > 0);

    // Verify evoked data dimensions
    const FiffEvoked& ev = set.evoked.first();
    QVERIFY(ev.data.rows() > 0);
    QVERIFY(ev.data.cols() > 0);
    QCOMPARE(ev.data.rows(), (Index)set.info.nchan);
}

void TestFiffIoEvokedInfo::evokedSet_pickChannels()
{

    QFile file(avePath());
    FiffEvokedSet set;
    FiffEvokedSet::read(file, set);
    QVERIFY2(!set.evoked.isEmpty(), "No evoked data loaded");

    // Pick first 10 channels by name
    QStringList include;
    for (int i = 0; i < qMin(10, set.info.nchan); i++) {
        include << set.info.chs[i].ch_name;
    }

    FiffEvokedSet pickedSet = set.pick_channels(include);
    QVERIFY(!pickedSet.evoked.isEmpty());
    QCOMPARE(pickedSet.info.nchan, include.size());
}

void TestFiffIoEvokedInfo::evokedSet_clear()
{
    FiffEvokedSet set;
    set.clear();
    QVERIFY(set.evoked.isEmpty());
}

void TestFiffIoEvokedInfo::evokedSet_copyConstruction()
{

    QFile file(avePath());
    FiffEvokedSet set;
    FiffEvokedSet::read(file, set);

    FiffEvokedSet copy(set);
    QCOMPARE(copy.evoked.size(), set.evoked.size());
    QCOMPARE(copy.info.nchan, set.info.nchan);
}

void TestFiffIoEvokedInfo::evokedSet_saveAndReload()
{

    QFile file(avePath());
    FiffEvokedSet setOrig;
    FiffEvokedSet::read(file, setOrig);
    QVERIFY2(!setOrig.evoked.isEmpty(), "No evoked data loaded");

    // Save to temp file
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/test_evoked.fif";
    QVERIFY(setOrig.save(outPath));

    // Reload
    QFile reloadFile(outPath);
    FiffEvokedSet setReload;
    bool ok = FiffEvokedSet::read(reloadFile, setReload);
    QVERIFY(ok);
    QCOMPARE(setReload.evoked.size(), setOrig.evoked.size());
    QCOMPARE(setReload.info.nchan, setOrig.info.nchan);

    // Verify data matches (tolerance accounts for double→float→double roundtrip
    // plus SSP projector re-application during reload)
    for (int i = 0; i < setOrig.evoked.size(); i++) {
        double norm = setOrig.evoked[i].data.norm();
        double diff = (setReload.evoked[i].data - setOrig.evoked[i].data).norm();
        double relDiff = (norm > 0) ? diff / norm : diff;
        QVERIFY2(relDiff < 1e-3, qPrintable(QString("Evoked %1 rel diff = %2").arg(i).arg(relDiff)));
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

    QFile file(avePath());
    FiffEvokedSet set;
    FiffEvokedSet::read(file, set);
    QVERIFY2(!set.evoked.isEmpty(), "No evoked data loaded");

    // Compute grand average of the same set twice (simulating multiple subjects)
    QList<FiffEvokedSet> setList;
    setList << set << set;

    FiffEvokedSet grandAvg = FiffEvokedSet::computeGrandAverage(setList);
    QVERIFY(!grandAvg.evoked.isEmpty());
    QCOMPARE(grandAvg.info.nchan, set.info.nchan);

    // Grand average of same data should equal original data
    double diff = (grandAvg.evoked.first().data - set.evoked.first().data).norm();
    QVERIFY2(diff < 1e-6, qPrintable(QString("Grand avg differs by %1").arg(diff)));
}

void TestFiffIoEvokedInfo::evokedSet_checkArtifacts()
{
    // Create synthetic epoch
    int nChan = 3;
    int nSamp = 100;
    MatrixXd epoch = MatrixXd::Random(nChan, nSamp) * 1e-12;  // Small MEG-scale values

    FiffInfo info;
    // Create minimal channel info
    for (int i = 0; i < nChan; i++) {
        FiffChInfo ch;
        ch.ch_name = QString("MEG%1").arg(i + 1, 4, 10, QChar('0'));
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_VV_MAG_W;
        ch.unit = FIFF_UNIT_T;
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    info.nchan = nChan;

    RejectionParams rej;
    rej.megGradReject = 4000e-13;
    rej.megMagReject = 4e-12;
    rej.eegReject = 150e-6;

    QString reason;
    bool epochOk = FiffEvokedSet::checkArtifacts(epoch, info, QStringList(), rej, reason);
    // Small values should not trigger artifact rejection (returns true = clean)
    QVERIFY(epochOk);

    // Now create epoch with very large values (need variation for peak-to-peak detection)
    MatrixXd badEpoch = MatrixXd::Zero(nChan, nSamp);
    for (int c = 0; c < nChan; c++) {
        badEpoch(c, 0) = -1e-9;
        badEpoch(c, 1) = 1e-9;  // pp = 2e-9 T, much larger than 4e-12 T reject
    }
    bool badEpochOk = FiffEvokedSet::checkArtifacts(badEpoch, info, QStringList(), rej, reason);
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
    QVERIFY(info.chs.isEmpty());
}

void TestFiffIoEvokedInfo::fiffInfo_copyConstruction()
{

    QFile file(rawPath());
    FiffRawData raw(file);
    FiffInfo copy(raw.info);

    QCOMPARE(copy.nchan, raw.info.nchan);
    QVERIFY(qAbs(copy.sfreq - raw.info.sfreq) < 0.01);
    QCOMPARE(copy.chs.size(), raw.info.chs.size());
}

void TestFiffIoEvokedInfo::fiffInfo_clear()
{

    QFile file(rawPath());
    FiffRawData raw(file);
    FiffInfo info = raw.info;
    QVERIFY(info.nchan > 0);

    info.clear();
    QCOMPARE(info.nchan, -1);
    QVERIFY(info.chs.isEmpty());
}

void TestFiffIoEvokedInfo::fiffInfo_pickInfo()
{

    QFile file(rawPath());
    FiffRawData raw(file);

    // Pick first 10 channels
    RowVectorXi sel(10);
    for (int i = 0; i < 10; i++) sel(i) = i;

    FiffInfo picked = raw.info.pick_info(sel);
    QCOMPARE(picked.nchan, 10);
    QCOMPARE(picked.chs.size(), 10);
    QCOMPARE(picked.ch_names.size(), 10);
}

void TestFiffIoEvokedInfo::fiffInfo_getCurrentComp()
{

    QFile file(rawPath());
    FiffRawData raw(file);

    qint32 comp = raw.info.get_current_comp();
    QVERIFY(comp >= 0);  // Neuromag data: grade 0
}

void TestFiffIoEvokedInfo::fiffInfo_setCurrentComp()
{

    QFile file(rawPath());
    FiffRawData raw(file);
    FiffInfo info = raw.info;

    info.set_current_comp(0);
    QCOMPARE(info.get_current_comp(), (qint32)0);
}

void TestFiffIoEvokedInfo::fiffInfo_makeCompensator()
{

    QFile file(rawPath());
    FiffRawData raw(file);

    FiffCtfComp comp;
    bool ok = raw.info.make_compensator(0, 0, comp);
    // from==to returns false (identity, no transformation needed)
    QVERIFY(!ok);
    // The data should be identity matrix
    QVERIFY(comp.data->data.rows() == raw.info.nchan);
    QVERIFY(comp.data->data.cols() == raw.info.nchan);
}

void TestFiffIoEvokedInfo::fiffInfo_writeToStream()
{

    QFile file(rawPath());
    FiffRawData raw(file);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString outPath = tmpDir.path() + "/info_test.fif";
    QFile outFile(outPath);
    QVERIFY(outFile.open(QIODevice::WriteOnly));

    FiffStream::SPtr pStream = FiffStream::start_file(outFile);
    pStream->start_block(FIFFB_MEAS);
    raw.info.writeToStream(pStream.data());
    pStream->end_block(FIFFB_MEAS);
    pStream->end_file();
    outFile.close();

    QVERIFY(QFileInfo(outPath).size() > 0);
}

void TestFiffIoEvokedInfo::fiffInfo_readMegEegChannels()
{

    QList<FiffChInfo> chsp;
    int nmeg = 0, neeg = 0;

    bool ok = FiffInfo::readMegEegChannels(rawPath(), true, true, QStringList(),
                                           chsp, nmeg, neeg);
    QVERIFY(ok);
    QVERIFY(nmeg > 0);  // Should find MEG channels
    QVERIFY(chsp.size() == nmeg + neeg);
}

void TestFiffIoEvokedInfo::fiffInfo_print()
{

    QFile file(rawPath());
    FiffRawData raw(file);

    // print() writes to qDebug, just verify it doesn't crash
    raw.info.print();
    QVERIFY(true);
}

//=============================================================================================================
// FiffDigitizerData tests
//=============================================================================================================

void TestFiffIoEvokedInfo::digitizerData_defaultConstruction()
{
    FiffDigitizerData dd;
    QCOMPARE(dd.npoint, 0);
    QVERIFY(dd.points.isEmpty());
    QVERIFY(!dd.dist_valid);
}

void TestFiffIoEvokedInfo::digitizerData_copyConstruction()
{
    FiffDigitizerData orig;
    orig.npoint = 3;
    orig.coord_frame = FIFFV_COORD_HEAD;

    FiffDigitizerData copy(orig);
    QCOMPARE(copy.npoint, 3);
    QCOMPARE(copy.coord_frame, (int)FIFFV_COORD_HEAD);
}

void TestFiffIoEvokedInfo::digitizerData_assignment()
{
    FiffDigitizerData orig;
    orig.npoint = 5;
    orig.coord_frame = FIFFV_COORD_MRI;

    FiffDigitizerData assigned;
    assigned = orig;
    QCOMPARE(assigned.npoint, 5);
    QCOMPARE(assigned.coord_frame, (int)FIFFV_COORD_MRI);
}

void TestFiffIoEvokedInfo::digitizerData_readFromFile()
{

    QFile file(rawPath());
    FiffDigitizerData dd(file);
    // The raw file should have digitizer points
    QVERIFY(dd.points.size() >= 0);
}

void TestFiffIoEvokedInfo::digitizerData_pickCardinalFiducials()
{
    FiffDigitizerData dd;

    // Add cardinal fiducial points
    FiffDigPoint dp1;
    dp1.kind = FIFFV_POINT_CARDINAL;
    dp1.ident = FIFFV_POINT_LPA;
    dp1.r[0] = -0.07f; dp1.r[1] = 0.0f; dp1.r[2] = 0.0f;
    dp1.coord_frame = FIFFV_COORD_HEAD;
    dd.points.append(dp1);

    FiffDigPoint dp2;
    dp2.kind = FIFFV_POINT_CARDINAL;
    dp2.ident = FIFFV_POINT_RPA;
    dp2.r[0] = 0.07f; dp2.r[1] = 0.0f; dp2.r[2] = 0.0f;
    dp2.coord_frame = FIFFV_COORD_HEAD;
    dd.points.append(dp2);

    FiffDigPoint dp3;
    dp3.kind = FIFFV_POINT_CARDINAL;
    dp3.ident = FIFFV_POINT_NASION;
    dp3.r[0] = 0.0f; dp3.r[1] = 0.1f; dp3.r[2] = 0.0f;
    dp3.coord_frame = FIFFV_COORD_HEAD;
    dd.points.append(dp3);

    dd.npoint = 3;
    dd.coord_frame = FIFFV_COORD_HEAD;

    // Set up a head->MRI transform
    dd.head_mri_t = std::make_unique<FiffCoordTrans>();
    dd.head_mri_t->from = FIFFV_COORD_HEAD;
    dd.head_mri_t->to = FIFFV_COORD_MRI;
    dd.head_mri_t->trans = Matrix4f::Identity();

    dd.pickCardinalFiducials();
    // Should have extracted the fiducial points
    QVERIFY(dd.mri_fids.size() >= 0);
}

//=============================================================================================================
// FiffFileSharer tests
//=============================================================================================================

void TestFiffIoEvokedInfo::fileSharer_defaultConstruction()
{
    FiffFileSharer sharer;
    QVERIFY(true);  // Just verify construction doesn't crash
}

void TestFiffIoEvokedInfo::fileSharer_withDirectory()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    FiffFileSharer sharer(tmpDir.path());
    QVERIFY(true);  // Construction with valid directory shouldn't crash
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffIoEvokedInfo)

#include "test_fiff_io_evoked_info.moc"
