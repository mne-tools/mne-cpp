#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <utils/generics/mne_logger.h>

#include <dsp/rt/rt_averaging.h>
#include <dsp/rt/rt_noise.h>
#include <dsp/rt/rt_connectivity.h>
#include <dsp/rt/rt_inv_op.h>
#include <dsp/rt/rt_filter.h>
#include <dsp/rt/rt_detect_trigger.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked_set.h>
#include <mne/mne_forward_solution.h>

using namespace RTPROCESSINGLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;
using namespace Eigen;

// Helper to create a minimal synthetic FiffInfo
static FiffInfo::SPtr createSyntheticFiffInfo(int nChannels, float sfreq)
{
    FiffInfo::SPtr info = FiffInfo::SPtr::create();
    info->sfreq = sfreq;

    for (int i = 0; i < nChannels; ++i) {
        FiffChInfo ch;
        ch.ch_name = QString("MEG%1").arg(i + 1, 4, 10, QChar('0'));
        ch.kind = FIFFV_MEG_CH;
        ch.unit = FIFF_UNIT_T;
        ch.cal = 1.0;
        ch.range = 1.0;
        ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
        info->chs.append(ch);
        info->ch_names.append(ch.ch_name);
    }
    // Add one STI channel as trigger
    FiffChInfo sti;
    sti.ch_name = "STI014";
    sti.kind = FIFFV_STIM_CH;
    sti.unit = FIFF_UNIT_NONE;
    sti.cal = 1.0;
    sti.range = 1.0;
    info->chs.append(sti);
    info->ch_names.append(sti.ch_name);

    info->nchan = info->chs.size();
    return info;
}

class TestRtProcessingAveraging : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const { return !m_sDataPath.isEmpty(); }

private slots:
    void initTestCase()
    {
        qInstallMessageHandler(MNELogger::customLogWriter);
        QString base = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
            m_sDataPath = base;
    }
    //=========================================================================
    // RtAveraging - construction and lifecycle
    //=========================================================================
    void rtAveraging_construct()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(10, 1000.0f);

        RtAveraging avg(20,  // numAverages
                        100, // preStim samples
                        200, // postStim samples
                        0,   // baseline from
                        0,   // baseline to
                        10,  // trigger channel index (STI014)
                        info);
        // Just verify no crash during construction
        QVERIFY(true);
    }

    void rtAveraging_setters()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(5, 500.0f);

        RtAveraging avg(10, 50, 100, 0, 0, 5, info);

        avg.setAverageNumber(30);
        avg.setPreStim(150, 300);
        avg.setPostStim(250, 500);
        avg.setTriggerChIndx(5);
        avg.setBaselineActive(true);
        avg.setBaselineFrom(0, 0);
        avg.setBaselineTo(50, 100);

        QMap<QString, double> thresholds;
        thresholds["eeg"] = 150e-6;
        avg.setArtifactReduction(thresholds);

        avg.reset();
        // No crash
        QVERIFY(true);
    }

    void rtAveraging_appendData()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(5, 500.0f);

        RtAveraging avg(10, 50, 100, 0, 0, 5, info);

        // Append some synthetic data blocks
        MatrixXd data = MatrixXd::Zero(6, 200); // 5 MEG + 1 STI channel
        avg.append(data);
        QVERIFY(true);
    }

    void rtAveraging_stop()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(5, 500.0f);

        RtAveraging avg(10, 50, 100, 0, 0, 5, info);
        avg.stop();
        QVERIFY(true);
    }

    //=========================================================================
    // RtConnectivity - construction and lifecycle
    //=========================================================================
    void rtConnectivity_construct()
    {
        RtConnectivity conn;
        // Just verify construction
        QVERIFY(true);
    }

    void rtConnectivity_stopEmpty()
    {
        RtConnectivity conn;
        conn.stop();
        QVERIFY(true);
    }

    void rtConnectivity_restart()
    {
        RtConnectivity conn;
        conn.restart();
        QVERIFY(true);
    }

    //=========================================================================
    // RtInvOp - construction and lifecycle
    //=========================================================================
    void rtInvOp_construct()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(10, 1000.0f);
        QSharedPointer<MNEForwardSolution> fwd(new MNEForwardSolution());

        RtInvOp invOp(info, fwd);
        QVERIFY(true);
    }

    void rtInvOp_stop()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(10, 1000.0f);
        QSharedPointer<MNEForwardSolution> fwd(new MNEForwardSolution());

        RtInvOp invOp(info, fwd);
        invOp.stop();
        QVERIFY(true);
    }

    void rtInvOp_setFwd()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(10, 1000.0f);
        QSharedPointer<MNEForwardSolution> fwd(new MNEForwardSolution());
        QSharedPointer<MNEForwardSolution> fwd2(new MNEForwardSolution());

        RtInvOp invOp(info, fwd);
        invOp.setFwdSolution(fwd2);
        QVERIFY(true);
    }

    //=========================================================================
    // FiffInfo synthetic - coverage for FiffInfo methods
    //=========================================================================
    void fiffInfo_syntheticChannels()
    {
        FiffInfo::SPtr info = createSyntheticFiffInfo(20, 1000.0f);
        QCOMPARE(info->nchan, 21); // 20 MEG + 1 STI
        QVERIFY(qFuzzyCompare(info->sfreq, 1000.0f));
    }

    //=========================================================================
    // DATA-DRIVEN: RtAveraging with real raw data
    //=========================================================================
    void data_rtAveraging_withRealInfo()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        FiffInfo::SPtr info = FiffInfo::SPtr::create(raw.info);

        // Find STI channel index
        int stiIdx = -1;
        for (int i = 0; i < info->nchan; ++i) {
            if (info->chs[i].kind == FIFFV_STIM_CH) {
                stiIdx = i;
                break;
            }
        }
        if (stiIdx < 0) QSKIP("No stimulus channel found");

        RtAveraging avg(10, 50, 100, 0, 0, stiIdx, info);
        avg.setBaselineActive(true);
        avg.setBaselineFrom(0, 0);
        avg.setBaselineTo(50, 100);

        // Read a segment and feed it
        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + 2000, raw.last_samp);
        raw.read_raw_segment(data, times, from, to);

        avg.append(data);
        avg.stop();
        QVERIFY(true);
    }

    //=========================================================================
    // DATA-DRIVEN: RtInvOp with real forward solution
    //=========================================================================
    void data_rtInvOp_withRealFwd()
    {
        if (!hasData()) QSKIP("No test data");

        QString fwdPath = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";

        QFile fwdFile(fwdPath), rawFile(rawPath);
        if (!fwdFile.exists() || !rawFile.exists()) QSKIP("Required files not found");

        MNEForwardSolution fwdSol(fwdFile);
        FiffRawData raw(rawFile);

        if (fwdSol.isEmpty()) QSKIP("Forward solution empty");

        FiffInfo::SPtr info = FiffInfo::SPtr::create(raw.info);
        QSharedPointer<MNEForwardSolution> fwd(new MNEForwardSolution(fwdSol));

        RtInvOp invOp(info, fwd);
        invOp.stop();
        QVERIFY(true);
    }

    //=========================================================================
    // DATA-DRIVEN: Filter real raw data
    //=========================================================================
    void data_filterRealRawData()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);

        // Read a small segment
        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + 1000, raw.last_samp);
        if (!raw.read_raw_segment(data, times, from, to)) QSKIP("Segment read failed");

        // Apply bandpass filter to a subset of channels
        int nCh = qMin(10, (int)data.rows());
        MatrixXd subset = data.topRows(nCh);

        FilterKernel kernel = FilterKernel("data_bp", 2, // BPF=2
                                           256, 10.0/raw.info.sfreq, 30.0/raw.info.sfreq, 1.0/raw.info.sfreq,
                                           raw.info.sfreq, 0);

        MatrixXd filtered = subset;
        // Apply filter row-by-row
        for (int r = 0; r < filtered.rows(); ++r) {
            RowVectorXd row = filtered.row(r);
            kernel.applyFftFilter(row);
            filtered.row(r) = row;
        }

        QVERIFY(filtered.allFinite());
        QCOMPARE(filtered.rows(), (Index)nCh);
    }

    //=========================================================================
    // DATA-DRIVEN: Detect triggers in real raw data
    //=========================================================================
    void data_detectTriggers()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);

        // Read full data
        MatrixXd data, times;
        if (!raw.read_raw_segment(data, times)) QSKIP("Read failed");

        // Find STI channel
        int stiIdx = -1;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_STIM_CH) {
                stiIdx = i;
                break;
            }
        }
        if (stiIdx < 0) QSKIP("No STI channel");

        // Detect triggers
        QList<QPair<int,double>> triggers = RTPROCESSINGLIB::detectTriggerFlanksMax(data, stiIdx, 0, 0.0, false, 100);
        // May or may not find triggers depending on data content
        QVERIFY(true); // Exercise the code path
    }

    //=========================================================================
    // DATA-DRIVEN: Read evoked and verify structure
    //=========================================================================
    void data_evokedSetReadAndPick()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Evoked file not found");

        FiffEvokedSet evokedSet(file);
        QVERIFY(evokedSet.evoked.size() > 0);

        // Pick first 50 channels
        QStringList picks;
        for (int i = 0; i < qMin(50, evokedSet.info.nchan); ++i)
            picks << evokedSet.info.ch_names[i];

        FiffEvokedSet picked = evokedSet.pick_channels(picks);
        QCOMPARE(picked.info.nchan, picks.size());
    }

    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestRtProcessingAveraging)
#include "test_rtprocessing_averaging.moc"
