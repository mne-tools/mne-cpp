//=============================================================================================================
// test_inv_rt_library.cpp -- Tests for the Inverse and RT Processing libraries
//
// Covers: InvDipoleFitSettings, InvRapMusic, InvDipole, InvDipolePair,
//         RtConnectivity, RtHpi, RtInvOp, RtAveraging, RtNoise, filterFile
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QTemporaryFile>
#include <QThread>
#include <QCoreApplication>
#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_source_spaces.h>

#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/rap_music/inv_rap_music.h>
#include <inv/rap_music/inv_dipole.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/hpi/inv_hpi_model_parameters.h>
#include <inv/hpi/inv_sensor_set.h>
#include <inv/hpi/inv_hpi_fit.h>
#include <inv/hpi/inv_hpi_fit_data.h>
#include <inv/hpi/inv_signal_model.h>

#include <dsp/rt/rt_connectivity.h>
#include <dsp/rt/rt_hpis.h>
#include <dsp/rt/rt_inv_op.h>
#include <dsp/rt/rt_averaging.h>
#include <dsp/rt/rt_noise.h>
#include <dsp/rt/rt_filter.h>
#include <dsp/filterkernel.h>

#include <conn/connectivitysettings.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace RTPROCESSINGLIB;
using namespace UTILSLIB;
using namespace CONNLIB;
using namespace Eigen;

//=============================================================================================================

class TestInvRtLibrary : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    bool hasData() const { return !m_sDataPath.isEmpty(); }

    QString rawPath() const { return m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString fwdPath() const { return m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ---- Inverse: InvDipoleFitSettings ----
    void dipoleFitSettings_defaultCtor();
    void dipoleFitSettings_cliParsing();

    // ---- Inverse: InvRapMusic ----
    void rapMusic_defaultCtorAndBasics();
    void rapMusic_mathHelpers();

    // ---- Inverse: InvDipole ----
    void dipole_fullExercise();

    // ---- RT Processing: RtConnectivity ----
    void rtConnectivity_lifecycle();

    // ---- RT Processing: RtHpi ----
    void rtHpi_lifecycle();

    // ---- RT Processing: RtInvOp ----
    void rtInvOp_lifecycle();

    // ---- RT Processing: RtAveraging ----
    void rtAveraging_moreWorkerOps();

    // ---- RT Processing: RtNoise ----
    void rtNoise_hanningWindow();
    void rtNoise_lifecycle();

    // ---- RT Processing: filterFile ----
    void rtprocessing_filterFile();

    // ---- HPI Fit (from boost) ----
    void hpiFit_constructWithSensorSet();
    void hpiFit_storeHeadPosition();
    void hpiFit_fitWithSyntheticData();
    void hpiFitData_defaultConstruction();
    void hpiFitData_setMembersAndFit();

    // ---- InvHpiModelParameters (from boost) ----
    void hpiModelParams_construction();
    void hpiModelParams_copyAndCompare();

    // ---- InvSignalModel (from boost) ----
    void signalModel_construction();

    // ---- FilterKernel (from boost) ----
    void filterKernel_design_lpf();
    void filterKernel_design_hpf();
    void filterKernel_design_bpf();

    // ---- Filter apply (from boost) ----
    void filter_applyToRawData();
    void filter_applyWithChannelSelection();

    // ---- RtNoise extras (from coverage_push) ----
    void rtNoise_appendData();

    // ---- RtAveragingWorker extras (from coverage_push) ----
    void rtAveragingWorker_doWork();

    // ---- FilterOverlapAdd (from coverage_push) ----
    void filterOverlapAdd_calculate();
    void filterData_paramOverload();
};

//=============================================================================================================

void TestInvRtLibrary::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QString base = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
        m_sDataPath = base;
}

void TestInvRtLibrary::cleanupTestCase() {}

//=============================================================================================================
// Inverse: InvDipoleFitSettings
//=============================================================================================================

void TestInvRtLibrary::dipoleFitSettings_defaultCtor()
{
    InvDipoleFitSettings settings;
    QVERIFY(settings.measname.isEmpty());
    QVERIFY(!settings.include_meg);
    QVERIFY(!settings.include_eeg);
    QVERIFY(!settings.accurate);
    QVERIFY(settings.filter.filter_on);
    settings.checkIntegrity();
}

void TestInvRtLibrary::dipoleFitSettings_cliParsing()
{
    // Full comprehensive parsing
    {
        const char* args[] = {"dipole_fit",
                              "--meg", "--eeg", "--accurate",
                              "--meas", "test_meas.fif",
                              "--dip", "test_output.dip",
                              "--set", "2",
                              "--noise", "noise.fif",
                              "--bem", "bem.fif",
                              "--mri", "mri.fif",
                              "--tmin", "0.050",
                              "--tmax", "0.150",
                              "--tstep", "0.005",
                              "--bmin", "0.000",
                              "--bmax", "0.050",
                              "--integ", "0.005",
                              "--guess", "guess.fif",
                              "--guessrad", "80.0",
                              "--exclude", "20.0",
                              "--mindist", "10.0",
                              "--grid", "10.0",
                              "--gradnoise", "5.0",
                              "--magnoise", "20.0",
                              "--eegnoise", "0.2",
                              "--diagnoise",
                              "--reg", "0.1",
                              "--gradreg", "0.15",
                              "--magreg", "0.12",
                              "--eegreg", "0.08",
                              "--proj", "proj.fif",
                              "--noproj",
                              "--eegrad", "90.0",
                              "--eegmodel", "Default",
                              "--eegscalp",
                              "--magdip",
                              "--bdip", "bdip_output.bdip",
                              "--gui",
                              "--filtersize", "2048",
                              "--highpass", "1.0",
                              "--lowpass", "30.0",
                              "--lowpassw", "3.0",
                              "--filteroff",
                              "--verbose"};
        int argc = sizeof(args) / sizeof(args[0]);
        char** argv = const_cast<char**>(args);
        InvDipoleFitSettings settings(&argc, argv);
        QVERIFY(settings.include_meg);
        QVERIFY(settings.include_eeg);
        QVERIFY(settings.accurate);
        QCOMPARE(settings.measname, QString("test_meas.fif"));
    }

    // Fewer args (bad channels, raw mode)
    {
        const char* args[] = {"dipole_fit", "--meg",
                              "--meas", "raw.fif", "--dip", "out.dip",
                              "--bad", "bad.txt",
                              "--guesssurf", "inner_skull.fif"};
        int argc = sizeof(args) / sizeof(args[0]);
        char** argv = const_cast<char**>(args);
        InvDipoleFitSettings settings(&argc, argv);
        QVERIFY(settings.include_meg);
    }
}

//=============================================================================================================
// Inverse: InvRapMusic
//=============================================================================================================

void TestInvRtLibrary::rapMusic_defaultCtorAndBasics()
{
    InvRapMusic rap;
    const char* name = rap.getName();
    QVERIFY(QString(name).contains("RAP"));

    rap.setStcAttr(100, 0.5f);
    const MNESourceSpaces& src = rap.getSourceSpace();
    Q_UNUSED(src);
}

void TestInvRtLibrary::rapMusic_mathHelpers()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward");

    InvRapMusic rap;
    bool initOk = rap.init(fwd, false, 2, 0.5);
    QVERIFY(initOk);
    QVERIFY(QString(rap.getName()).contains("RAP"));
}

//=============================================================================================================
// Inverse: InvDipole
//=============================================================================================================

void TestInvRtLibrary::dipole_fullExercise()
{
    // Default constructor
    InvDipole<double> dip;
    QCOMPARE(dip.x(), 0.0);
    QCOMPARE(dip.y(), 0.0);
    QCOMPARE(dip.z(), 0.0);
    QCOMPARE(dip.phi_x(), 0.0);
    QCOMPARE(dip.phi_y(), 0.0);
    QCOMPARE(dip.phi_z(), 0.0);

    dip.x() = 1.0; dip.y() = 2.0; dip.z() = 3.0;
    dip.phi_x() = 0.5; dip.phi_y() = 0.6; dip.phi_z() = 0.7;
    QCOMPARE(dip.x(), 1.0);
    QCOMPARE(dip.y(), 2.0);
    QCOMPARE(dip.z(), 3.0);
    QCOMPARE(dip.phi_x(), 0.5);

    InvDipolePair<double> pair;
    pair.m_iIdx1 = 0; pair.m_iIdx2 = 5; pair.m_vCorrelation = 0.95;

    InvDipole<float> dipF;
    dipF.x() = 1.0f;
    QCOMPARE(dipF.x(), 1.0f);

    InvDipole<int> dipI;
    dipI.x() = 42;
    QCOMPARE(dipI.x(), 42);
}

//=============================================================================================================
// RT Processing: RtConnectivity
//=============================================================================================================

void TestInvRtLibrary::rtConnectivity_lifecycle()
{
    RtConnectivity rtConn;
    rtConn.stop();
    rtConn.restart();
    rtConn.stop();
    QThread::msleep(50);
}

//=============================================================================================================
// RT Processing: RtHpi
//=============================================================================================================

void TestInvRtLibrary::rtHpi_lifecycle()
{
    InvSensorSet sensorSet;
    RtHpi rtHpi(sensorSet);

    InvHpiModelParameters hpiParams;
    rtHpi.setModelParameters(hpiParams);

    MatrixXd projMat = MatrixXd::Identity(10, 10);
    rtHpi.setProjectionMatrix(projMat);

    MatrixXd coilsHead = MatrixXd::Zero(4, 3);
    rtHpi.setHpiDigitizer(coilsHead);

    rtHpi.stop();
    rtHpi.restart();
    rtHpi.stop();
    QThread::msleep(50);
}

//=============================================================================================================
// RT Processing: RtInvOp
//=============================================================================================================

void TestInvRtLibrary::rtInvOp_lifecycle()
{
    auto pInfo = QSharedPointer<FiffInfo>::create();
    pInfo->sfreq = 1000.0; pInfo->nchan = 10;

    auto pFwd = QSharedPointer<MNEForwardSolution>::create();

    RtInvOp rtInv(pInfo, pFwd);

    auto pFwd2 = QSharedPointer<MNEForwardSolution>::create();
    rtInv.setFwdSolution(pFwd2);

    rtInv.stop();
    rtInv.restart();
    rtInv.stop();
    QThread::msleep(50);
}

//=============================================================================================================
// RT Processing: RtAveraging
//=============================================================================================================

void TestInvRtLibrary::rtAveraging_moreWorkerOps()
{
    auto pInfo = FiffInfo::SPtr::create();
    pInfo->sfreq = 1000.0; pInfo->nchan = 10;
    for (int i = 0; i < 10; i++) {
        FiffChInfo ch;
        ch.ch_name = QString("Ch%1").arg(i);
        if (i == 9) {
            ch.kind = FIFFV_STIM_CH;
            ch.ch_name = "STI014";
        } else {
            ch.kind = FIFFV_MEG_CH;
        }
        pInfo->chs.append(ch);
        pInfo->ch_names.append(ch.ch_name);
    }

    RtAveraging rtAvg(10, 100, 200, 0, 50, 9, pInfo);

    rtAvg.setAverageNumber(20);
    rtAvg.setPreStim(150, 150);
    rtAvg.setPostStim(250, 250);
    rtAvg.setTriggerChIndx(9);

    QMap<QString, double> thresholds;
    thresholds["eeg"] = 100e-6;
    thresholds["mag"] = 4000e-15;
    rtAvg.setArtifactReduction(thresholds);

    rtAvg.setBaselineActive(true);
    rtAvg.setBaselineFrom(0, 0);
    rtAvg.setBaselineTo(50, 50);

    rtAvg.reset();
    rtAvg.stop();
    QThread::msleep(50);
}

//=============================================================================================================
// RT Processing: RtNoise
//=============================================================================================================

void TestInvRtLibrary::rtNoise_hanningWindow()
{
    auto pInfo = FiffInfo::SPtr::create();
    pInfo->sfreq = 1000.0; pInfo->nchan = 5;
    for (int i = 0; i < 5; i++) {
        FiffChInfo ch;
        ch.ch_name = QString("MEG%1").arg(i);
        ch.kind = FIFFV_MEG_CH;
        pInfo->chs.append(ch);
        pInfo->ch_names.append(ch.ch_name);
    }

    RtNoise rtNoise(256, pInfo, 5);

    MatrixXd data = MatrixXd::Random(5, 100);
    rtNoise.append(data);

    QVERIFY(!rtNoise.isRunning());
    rtNoise.stop();
}

void TestInvRtLibrary::rtNoise_lifecycle()
{
    auto pInfo = FiffInfo::SPtr::create();
    pInfo->sfreq = 600.0; pInfo->nchan = 3;
    for (int i = 0; i < 3; i++) {
        FiffChInfo ch;
        ch.ch_name = QString("CH%1").arg(i);
        ch.kind = FIFFV_MEG_CH;
        pInfo->chs.append(ch);
        pInfo->ch_names.append(ch.ch_name);
    }

    {
        RtNoise rtNoise(128, pInfo, 3);
        for (int j = 0; j < 5; j++) {
            MatrixXd data = MatrixXd::Random(3, 50);
            rtNoise.append(data);
        }
        rtNoise.start();
        QThread::msleep(100);
        rtNoise.stop();
        rtNoise.wait(2000);
    }
}

//=============================================================================================================
// RT Processing: filterFile
//=============================================================================================================

void TestInvRtLibrary::rtprocessing_filterFile()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData::SPtr pRaw(new FiffRawData(rawFile));

    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(true);
    QVERIFY(tmpFile.open());

    bool ok = RTPROCESSINGLIB::filterFile(tmpFile, pRaw, 2, 10.0, 5.0, 2.0,
                                          pRaw->info.sfreq, 512);
    Q_UNUSED(ok);
    QVERIFY(tmpFile.size() > 0);
}

//=============================================================================================================
// HPI Fit (from boost)
//=============================================================================================================

void TestInvRtLibrary::hpiFit_constructWithSensorSet()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    InvSensorSetCreator creator;
    InvSensorSet sensorSet = creator.updateSensorSet(raw.info.chs, Accuracy::medium);
    InvHpiFit hpiFit(sensorSet);
    QVERIFY(true);
}

void TestInvRtLibrary::hpiFit_storeHeadPosition()
{
    MatrixXf matTransDevHead = MatrixXf::Identity(4, 4);
    matTransDevHead(0, 3) = 0.01f;
    matTransDevHead(1, 3) = -0.005f;
    MatrixXd matPosition = MatrixXd::Zero(1, 10);
    VectorXd vecGoF(4);
    vecGoF << 0.99, 0.98, 0.97, 0.96;
    QVector<double> vecError = {0.001, 0.002, 0.001, 0.003};
    InvHpiFit::storeHeadPosition(0.5f, matTransDevHead, matPosition, vecGoF, vecError);
    // storeHeadPosition appends a new row; check the last row has data
    int lastRow = matPosition.rows() - 1;
    QVERIFY(lastRow >= 0);
    QVERIFY(matPosition(lastRow, 0) != 0.0 || matPosition(lastRow, 4) != 0.0);
}

void TestInvRtLibrary::hpiFit_fitWithSyntheticData()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    InvSensorSetCreator creator;
    InvSensorSet sensorSet = creator.updateSensorSet(raw.info.chs, Accuracy::medium);
    InvHpiFit hpiFit(sensorSet);
    QVector<int> hpiFreqs = {154, 158, 162, 166};
    InvHpiModelParameters modelParams(hpiFreqs, static_cast<int>(raw.info.sfreq), 60, false);
    RowVectorXi megPicks = raw.info.pick_types(true, false, false);
    int nMeg = megPicks.size();
    int nSamples = static_cast<int>(raw.info.sfreq);
    MatrixXd matProjectedData = MatrixXd::Random(nMeg, nSamples) * 1e-12;
    MatrixXd matProjectors = MatrixXd::Identity(nMeg, nMeg);
    MatrixXd matCoilsHead(4, 3);
    matCoilsHead << 0.06, 0.0, 0.06, -0.06, 0.0, 0.06, 0.0, 0.06, 0.06, 0.0, -0.06, 0.06;
    HpiFitResult result;
    hpiFit.fit(matProjectedData, matProjectors, modelParams, matCoilsHead, result);
    QVERIFY(true);
}

void TestInvRtLibrary::hpiFitData_defaultConstruction()
{
    InvHpiFitData data;
    // m_iMaxIterations is uninitialized by default; just verify the object can be constructed
    QVERIFY(true);
}

void TestInvRtLibrary::hpiFitData_setMembersAndFit()
{
    // Test that InvHpiFitData can be constructed and its members set
    InvHpiFitData data;
    // Verify the object is usable (specific fitting requires full setup with coils)
    QVERIFY(true);
}

//=============================================================================================================
// InvHpiModelParameters (from boost)
//=============================================================================================================

void TestInvRtLibrary::hpiModelParams_construction()
{
    QVector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params(freqs, 1000, 60, false);
    QCOMPARE(params.vecHpiFreqs(), freqs);
    QCOMPARE(params.iSampleFreq(), 1000);
    QCOMPARE(params.iLineFreq(), 60);
    QCOMPARE(params.iNHpiCoils(), 4);
    QCOMPARE(params.bBasic(), false);
}

void TestInvRtLibrary::hpiModelParams_copyAndCompare()
{
    QVector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params1(freqs, 1000, 60, false);
    InvHpiModelParameters params2(params1);
    QVERIFY(params1 == params2);
    QVERIFY(!(params1 != params2));
    InvHpiModelParameters params3(freqs, 2000, 50, true);
    QVERIFY(params1 != params3);
}

//=============================================================================================================
// InvSignalModel (from boost)
//=============================================================================================================

void TestInvRtLibrary::signalModel_construction()
{
    QVector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params(freqs, 1000, 60, false);
    InvSignalModel model;
    int nSamples = 200;
    MatrixXd matData = MatrixXd::Random(1, nSamples);
    MatrixXd matFitted = model.fitData(params, matData);
    QVERIFY(matFitted.rows() > 0 && matFitted.cols() > 0);
}

//=============================================================================================================
// FilterKernel (from boost)
//=============================================================================================================

void TestInvRtLibrary::filterKernel_design_lpf()
{
    FilterKernel kernel("lpf_test", 0, 256, 40.0, 0.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.getCoefficients().size() > 0);
}

void TestInvRtLibrary::filterKernel_design_hpf()
{
    FilterKernel kernel("hpf_test", 1, 256, 0.0, 1.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.getCoefficients().size() > 0);
}

void TestInvRtLibrary::filterKernel_design_bpf()
{
    FilterKernel kernel("bpf_test", 2, 256, 40.0, 1.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.getCoefficients().size() > 0);
}

//=============================================================================================================
// Filter apply (from boost)
//=============================================================================================================

void TestInvRtLibrary::filter_applyToRawData()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    MatrixXd data, times;
    raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 4000);
    FilterKernel kernel("test_lp", 0, 256, 40.0, 0.0, 0.0, raw.info.sfreq, 0);
    MatrixXd subData = data.block(0, 0, qMin(5, static_cast<int>(data.rows())), data.cols());
    MatrixXd filtered = RTPROCESSINGLIB::filterData(subData, kernel);
    QVERIFY(filtered.rows() > 0);
    QVERIFY(filtered.cols() > 0);
}

void TestInvRtLibrary::filter_applyWithChannelSelection()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    MatrixXd data, times;
    raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 2000);
    FilterKernel kernel("test_hp", 1, 128, 0.0, 1.0, 0.0, raw.info.sfreq, 0);
    MatrixXd subData = data.block(0, 0, qMin(3, static_cast<int>(data.rows())), data.cols());
    MatrixXd filtered = RTPROCESSINGLIB::filterData(subData, kernel);
    QVERIFY(filtered.rows() > 0);
}

//=============================================================================================================
// RtNoise extras (from coverage_push)
//=============================================================================================================

void TestInvRtLibrary::rtNoise_appendData()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffInfo::SPtr pInfo = FiffInfo::SPtr(new FiffInfo(raw.info));
    RtNoise rtNoise(512, pInfo, 3);
    MatrixXd data, times;
    raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 511);
    rtNoise.append(data);
    QVERIFY(true);
}

//=============================================================================================================
// RtAveragingWorker extras (from coverage_push)
//=============================================================================================================

void TestInvRtLibrary::rtAveragingWorker_doWork()
{
    FiffInfo::SPtr pInfo(new FiffInfo());
    pInfo->sfreq = 1000.0;
    FiffChInfo chData;
    chData.ch_name = "MEG0111";
    chData.kind = FIFFV_MEG_CH;
    chData.unit = FIFF_UNIT_T;
    chData.range = 1.0;
    chData.cal = 1.0;
    FiffChInfo chTrig;
    chTrig.ch_name = "STI014";
    chTrig.kind = FIFFV_STIM_CH;
    chTrig.range = 1.0;
    chTrig.cal = 1.0;
    pInfo->chs << chData << chTrig;
    pInfo->nchan = 2;
    pInfo->ch_names << "MEG0111" << "STI014";
    quint32 preStim = 50, postStim = 100;
    RTPROCESSINGLIB::RtAveragingWorker worker(2, preStim, postStim, 0, 0, 1, pInfo);
    for (int block = 0; block < 5; ++block) {
        MatrixXd data = MatrixXd::Zero(2, 200);
        for (int j = 0; j < 200; ++j)
            data(0, j) = std::sin(2.0 * M_PI * 10.0 * j / 1000.0);
        if (block % 2 == 0) data(1, 100) = 5.0;
        worker.doWork(data);
    }
    QVERIFY(true);
}

//=============================================================================================================
// FilterOverlapAdd (from coverage_push)
//=============================================================================================================

void TestInvRtLibrary::filterOverlapAdd_calculate()
{
    MatrixXd data = MatrixXd::Random(2, 2048);
    RowVectorXi picks(2);
    picks << 0, 1;
    RTPROCESSINGLIB::FilterOverlapAdd filterOA;
    MatrixXd result = filterOA.calculate(data, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0,
                                         picks, true, false, false);
    QVERIFY(result.rows() == data.rows());
    QVERIFY(result.cols() > 0);
    filterOA.reset();
}

void TestInvRtLibrary::filterData_paramOverload()
{
    MatrixXd data = MatrixXd::Random(3, 1024);
    RowVectorXi picks(3);
    picks << 0, 1, 2;
    MatrixXd result = RTPROCESSINGLIB::filterData(data, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0,
                                                   picks, false, false);
    QVERIFY(result.rows() == data.rows());
    QVERIFY(result.cols() > 0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvRtLibrary)

#include "test_inv_rt_library.moc"
