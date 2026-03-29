#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <algorithm>
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
#include <mne/mne_epoch_data_list.h>

#include "../../applications/mne_browse/Utils/filteroperator.h"
#include "../../applications/mne_browse/Utils/sessionfilter.h"

using namespace RTPROCESSINGLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace MNEBROWSE;
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

static MatrixXi deriveStimEvents(const FiffRawData& raw)
{
    int stimIdx = -1;
    for (int channelIndex = 0; channelIndex < raw.info.nchan; ++channelIndex) {
        if (raw.info.chs[channelIndex].kind == FIFFV_STIM_CH) {
            stimIdx = channelIndex;
            if (raw.info.ch_names.value(channelIndex).remove(QLatin1Char(' ')) == QLatin1String("STI014")) {
                break;
            }
        }
    }

    if (stimIdx < 0) {
        return MatrixXi();
    }

    RowVectorXi picks(1);
    picks << stimIdx;

    MatrixXd stimData;
    MatrixXd stimTimes;
    if (!raw.read_raw_segment(stimData, stimTimes, raw.first_samp, raw.last_samp, picks) || stimData.rows() != 1) {
        return MatrixXi();
    }

    QVector<Vector3i> detectedEvents;
    detectedEvents.reserve(static_cast<int>(stimData.cols() / 32));

    int previousValue = 0;
    for (int sampleOffset = 0; sampleOffset < stimData.cols(); ++sampleOffset) {
        const int currentValue = qRound(stimData(0, sampleOffset));
        if (currentValue != previousValue && currentValue != 0) {
            detectedEvents.append(Vector3i(raw.first_samp + sampleOffset,
                                           previousValue,
                                           currentValue));
        }
        previousValue = currentValue;
    }

    MatrixXi events(detectedEvents.size(), 3);
    for (int row = 0; row < detectedEvents.size(); ++row) {
        events(row, 0) = detectedEvents.at(row)(0);
        events(row, 1) = detectedEvents.at(row)(1);
        events(row, 2) = detectedEvents.at(row)(2);
    }

    return events;
}

static QList<int> uniqueEventCodes(const MatrixXi& events, int maxCodes = -1)
{
    QList<int> codes;
    for (int row = 0; row < events.rows(); ++row) {
        const int code = events(row, 2);
        if (code == 0 || codes.contains(code)) {
            continue;
        }

        codes.append(code);
        if (maxCodes > 0 && codes.size() >= maxCodes) {
            break;
        }
    }

    return codes;
}

static double responseMagnitudeAt(const VectorXd& response,
                                  double samplingFrequency,
                                  double frequencyHz)
{
    if(response.size() == 0 || samplingFrequency <= 0.0) {
        return 0.0;
    }

    const double nyquist = samplingFrequency / 2.0;
    const double clampedFrequency = std::clamp(frequencyHz, 0.0, nyquist);
    const int index = qBound(0,
                             qRound((clampedFrequency / nyquist) * (response.size() - 1)),
                             static_cast<int>(response.size() - 1));
    return response(index);
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
    // DATA-DRIVEN: Browser filter operators must handle long QRHI blocks
    //=========================================================================
    void data_browserFilterOperator_handlesLongBlocks()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);

        MatrixXd data, times;
        const fiff_int_t from = raw.first_samp;
        const fiff_int_t to = qMin(raw.first_samp + static_cast<fiff_int_t>(raw.info.sfreq * 60.0f),
                                   raw.last_samp);
        if (!raw.read_raw_segment(data, times, from, to)) QSKIP("Segment read failed");

        QVERIFY(data.rows() > 0);
        QVERIFY(data.cols() > 4000);

        const RowVectorXd inputRow = data.row(0);
        const double nyquist = raw.info.sfreq / 2.0;
        const int fftLength = 8192;

        const QList<FilterOperator::DesignMethod> methods = {
            FilterOperator::Cosine,
            FilterOperator::Tschebyscheff
        };

        for (FilterOperator::DesignMethod method : methods) {
            FilterOperator filter(QStringLiteral("browser_filter"),
                                  FilterOperator::BPF,
                                  128,
                                  20.0 / nyquist,
                                  18.0 / nyquist,
                                  2.0 / nyquist,
                                  raw.info.sfreq,
                                  fftLength,
                                  method);

            const RowVectorXd filtered = filter.applyFFTFilter(inputRow);
            QCOMPARE(filtered.cols(), inputRow.cols());
            QVERIFY(filtered.allFinite());
        }
    }

    //=========================================================================
    // DATA-DRIVEN: Shared session filter supports FIR/IIR and leaves STI rows untouched
    //=========================================================================
    void data_sessionFilter_handlesLongBlocks()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);

        MatrixXd data, times;
        const fiff_int_t from = raw.first_samp;
        const fiff_int_t to = qMin(raw.first_samp + static_cast<fiff_int_t>(raw.info.sfreq * 60.0f),
                                   raw.last_samp);
        if (!raw.read_raw_segment(data, times, from, to)) QSKIP("Segment read failed");

        QVERIFY(data.rows() > 0);
        QVERIFY(data.cols() > 4000);

        int stimIndex = -1;
        int megIndex = -1;
        for (int channelIndex = 0; channelIndex < raw.info.nchan; ++channelIndex) {
            if (stimIndex < 0 && raw.info.chs[channelIndex].kind == FIFFV_STIM_CH) {
                stimIndex = channelIndex;
            }
            if (megIndex < 0 && raw.info.chs[channelIndex].kind == FIFFV_MEG_CH) {
                megIndex = channelIndex;
            }
        }

        QVERIFY(megIndex >= 0);

        struct FilterCase {
            SessionFilter::DesignMethod designMethod;
            SessionFilter::FilterType filterType;
            int order;
            double cutoffLow;
            double cutoffHigh;
            double transition;
        };

        const QList<FilterCase> filterCases = {
            {SessionFilter::DesignMethod::Cosine,        SessionFilter::FilterType::LowPass,  256, 40.0, 40.0, 5.0},
            {SessionFilter::DesignMethod::Cosine,        SessionFilter::FilterType::HighPass, 256, 10.0, 10.0, 5.0},
            {SessionFilter::DesignMethod::Tschebyscheff, SessionFilter::FilterType::BandPass, 256,  1.0, 40.0, 5.0},
            {SessionFilter::DesignMethod::Tschebyscheff, SessionFilter::FilterType::BandStop, 256, 48.0, 52.0, 2.0},
            {SessionFilter::DesignMethod::Butterworth,   SessionFilter::FilterType::BandPass,   4,  1.0, 40.0, 5.0},
            {SessionFilter::DesignMethod::Butterworth,   SessionFilter::FilterType::BandStop,   4, 48.0, 52.0, 2.0}
        };

        for (const FilterCase& filterCase : filterCases) {
            SessionFilter filter(QStringLiteral("session_filter"),
                                 filterCase.designMethod,
                                 filterCase.filterType,
                                 filterCase.order,
                                 filterCase.cutoffLow,
                                 filterCase.cutoffHigh,
                                 filterCase.transition,
                                 raw.info.sfreq,
                                 QStringLiteral("All"));

            QVERIFY(filter.isValid());
            const MatrixXd filtered = filter.applyToMatrix(data, raw.info);
            QCOMPARE(filtered.rows(), data.rows());
            QCOMPARE(filtered.cols(), data.cols());
            QVERIFY(filtered.allFinite());
            const double maxMegDifference = (filtered.row(megIndex) - data.row(megIndex)).cwiseAbs().maxCoeff();
            QVERIFY2(maxMegDifference > 1e-18,
                     qPrintable(QStringLiteral("Expected filtered MEG row to change for %1, max diff was %2")
                                    .arg(filter.displayName())
                                    .arg(maxMegDifference, 0, 'g', 6)));
            if (stimIndex >= 0) {
                QVERIFY(filtered.row(stimIndex).isApprox(data.row(stimIndex), 1e-12));
            }
        }
    }

    //=========================================================================
    // DATA-DRIVEN: Shared session filter response preserves passbands for FIR designs
    //=========================================================================
    void sessionFilter_defaultIsInvalid()
    {
        const SessionFilter filter;

        QVERIFY(!filter.isValid());
        QVERIFY(filter.magnitudeResponse(32).isZero(0.0));
        QVERIFY(filter.phaseResponse(32).isZero(0.0));
    }

    //=========================================================================
    // DATA-DRIVEN: Shared session filter response preserves passbands for FIR designs
    //=========================================================================
    void data_sessionFilter_responseSelectivity()
    {
        const double samplingFrequency = 1000.0;

        struct ResponseCase {
            SessionFilter::DesignMethod designMethod;
            SessionFilter::FilterType filterType;
            int order;
            double cutoffLow;
            double cutoffHigh;
            double transition;
            double passbandProbe;
            double stopbandProbe;
            double minPassbandMagnitude;
            double maxStopbandMagnitude;
        };

        const QList<ResponseCase> responseCases = {
            {SessionFilter::DesignMethod::Cosine,        SessionFilter::FilterType::LowPass,  80, 40.0, 40.0, 5.0,  5.0, 200.0, 0.70, 0.20},
            {SessionFilter::DesignMethod::Cosine,        SessionFilter::FilterType::HighPass, 80, 10.0, 10.0, 5.0, 120.0,  2.0, 0.70, 0.20},
            {SessionFilter::DesignMethod::Tschebyscheff, SessionFilter::FilterType::LowPass, 128, 40.0, 40.0, 5.0,  5.0, 200.0, 0.70, 0.20},
            {SessionFilter::DesignMethod::Tschebyscheff, SessionFilter::FilterType::BandPass, 128, 10.0, 40.0, 5.0, 20.0, 120.0, 0.50, 0.30}
        };

        for(const ResponseCase& responseCase : responseCases) {
            SessionFilter filter(QStringLiteral("response_filter"),
                                 responseCase.designMethod,
                                 responseCase.filterType,
                                 responseCase.order,
                                 responseCase.cutoffLow,
                                 responseCase.cutoffHigh,
                                 responseCase.transition,
                                 samplingFrequency,
                                 QStringLiteral("All"));

            QVERIFY(filter.isValid());

            const VectorXd response = filter.magnitudeResponse(4096);
            QVERIFY(response.size() == 4096);

            const double passbandMagnitude = responseMagnitudeAt(response,
                                                                 samplingFrequency,
                                                                 responseCase.passbandProbe);
            const double stopbandMagnitude = responseMagnitudeAt(response,
                                                                 samplingFrequency,
                                                                 responseCase.stopbandProbe);

            QVERIFY2(response.maxCoeff() > 0.5,
                     qPrintable(QStringLiteral("Unexpectedly weak max response for %1")
                                    .arg(filter.displayName())));
            QVERIFY2(passbandMagnitude >= responseCase.minPassbandMagnitude,
                     qPrintable(QStringLiteral("Passband magnitude too small for %1: %2")
                                    .arg(filter.displayName())
                                    .arg(passbandMagnitude, 0, 'g', 6)));
            QVERIFY2(stopbandMagnitude <= responseCase.maxStopbandMagnitude,
                     qPrintable(QStringLiteral("Stopband magnitude too large for %1: %2")
                                    .arg(filter.displayName())
                                    .arg(stopbandMagnitude, 0, 'g', 6)));
            QVERIFY2(passbandMagnitude > stopbandMagnitude * 2.0,
                     qPrintable(QStringLiteral("Passband/stopband separation too small for %1: %2 vs %3")
                                    .arg(filter.displayName())
                                    .arg(passbandMagnitude, 0, 'g', 6)
                                    .arg(stopbandMagnitude, 0, 'g', 6)));
        }
    }

    //=========================================================================
    // DATA-DRIVEN: Compute an evoked response from raw-trigger events
    //=========================================================================
    void data_computeAverageFromRawEvents()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        const MatrixXi events = deriveStimEvents(raw);
        QVERIFY(events.rows() > 0);

        const QList<int> codes = uniqueEventCodes(events, 1);
        QVERIFY(!codes.isEmpty());

        const FiffEvoked evoked = MNEEpochDataList::computeAverage(raw,
                                                                   events,
                                                                   -0.1f,
                                                                   0.3f,
                                                                   codes.first(),
                                                                   true,
                                                                   -0.1f,
                                                                   0.0f,
                                                                   {});

        QVERIFY(evoked.data.size() > 0);
        QCOMPARE(evoked.data.rows(), static_cast<Index>(raw.info.nchan));
        QVERIFY(evoked.nave > 0);
        QVERIFY(qAbs(evoked.baseline.first + 0.1f) < 1e-6f);
        QVERIFY(qAbs(evoked.baseline.second - 0.0f) < 1e-6f);
    }

    //=========================================================================
    // DATA-DRIVEN: Session filtering changes epoch-derived averages and covariance estimates
    //=========================================================================
    void data_sessionFilter_changesOfflineResults()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        const MatrixXi events = deriveStimEvents(raw);
        QVERIFY(events.rows() > 0);

        const QList<int> codes = uniqueEventCodes(events, 1);
        QVERIFY(!codes.isEmpty());

        MNEEpochDataList unfilteredEpochs = MNEEpochDataList::readEpochs(raw,
                                                                         events,
                                                                         -0.1f,
                                                                         0.3f,
                                                                         codes.first(),
                                                                         {});
        QVERIFY(!unfilteredEpochs.isEmpty());

        MNEEpochDataList filteredEpochs = MNEEpochDataList::readEpochs(raw,
                                                                       events,
                                                                       -0.1f,
                                                                       0.3f,
                                                                       codes.first(),
                                                                       {});
        QVERIFY(!filteredEpochs.isEmpty());

        SessionFilter filter(QStringLiteral("analysis_filter"),
                             SessionFilter::DesignMethod::Butterworth,
                             SessionFilter::FilterType::HighPass,
                             4,
                             20.0,
                             20.0,
                             5.0,
                             raw.info.sfreq,
                             QStringLiteral("MEG"));
        QVERIFY(filter.isValid());

        for (const auto& epoch : filteredEpochs) {
            QVERIFY(!epoch.isNull());
            epoch->epoch = filter.applyToMatrix(epoch->epoch, raw.info);
        }

        const MatrixXd originalEpoch = unfilteredEpochs.first()->epoch;
        const MatrixXd filteredEpoch = filteredEpochs.first()->epoch;
        QVERIFY((filteredEpoch - originalEpoch).cwiseAbs().maxCoeff() > 1e-15);

        const FiffEvoked evokedOriginal = unfilteredEpochs.average(raw.info,
                                                                   0,
                                                                   unfilteredEpochs.first()->epoch.cols());
        const FiffEvoked evokedFiltered = filteredEpochs.average(raw.info,
                                                                 0,
                                                                 filteredEpochs.first()->epoch.cols());
        QVERIFY((evokedFiltered.data - evokedOriginal.data).cwiseAbs().maxCoeff() > 1e-15);

        MatrixXd covOriginal = MatrixXd::Zero(raw.info.nchan, raw.info.nchan);
        MatrixXd covFiltered = MatrixXd::Zero(raw.info.nchan, raw.info.nchan);
        int sampleCount = 0;
        for (int i = 0; i < unfilteredEpochs.size(); ++i) {
            covOriginal += unfilteredEpochs.at(i)->epoch * unfilteredEpochs.at(i)->epoch.transpose();
            covFiltered += filteredEpochs.at(i)->epoch * filteredEpochs.at(i)->epoch.transpose();
            sampleCount += static_cast<int>(unfilteredEpochs.at(i)->epoch.cols());
        }

        QVERIFY(sampleCount > 1);
        covOriginal /= static_cast<double>(sampleCount - 1);
        covFiltered /= static_cast<double>(sampleCount - 1);
        QVERIFY((covFiltered - covOriginal).cwiseAbs().maxCoeff() > 1e-15);
    }

    //=========================================================================
    // DATA-DRIVEN: Compute a multi-condition evoked set from raw-trigger events
    //=========================================================================
    void data_averageCategoriesFromRawEvents()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        const MatrixXi events = deriveStimEvents(raw);
        QVERIFY(events.rows() > 0);

        const QList<int> codes = uniqueEventCodes(events, 4);
        QVERIFY(codes.size() >= 2);

        QStringList comments;
        for (int code : codes) {
            comments.append(QStringLiteral("event_%1").arg(code));
        }

        const FiffEvokedSet evokedSet = MNEEpochDataList::averageCategories(raw,
                                                                            events,
                                                                            codes,
                                                                            comments,
                                                                            -0.1f,
                                                                            0.3f,
                                                                            {},
                                                                            qMakePair(-0.1f, 0.0f),
                                                                            false);

        QCOMPARE(evokedSet.evoked.size(), codes.size());
        for (int index = 0; index < evokedSet.evoked.size(); ++index) {
            QVERIFY(evokedSet.evoked.at(index).data.size() > 0);
            QCOMPARE(evokedSet.evoked.at(index).comment, comments.at(index));
            QVERIFY(evokedSet.evoked.at(index).nave > 0);
        }
    }

    //=========================================================================
    // DATA-DRIVEN: Compute covariance from raw-trigger events
    //=========================================================================
    void data_computeCovarianceFromRawEvents()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        const MatrixXi events = deriveStimEvents(raw);
        QVERIFY(events.rows() > 0);

        const QList<int> codes = uniqueEventCodes(events, 4);
        QVERIFY(!codes.isEmpty());

        const FiffCov cov = FiffCov::compute_from_epochs(raw,
                                                         events,
                                                         codes,
                                                         -0.2f,
                                                         0.0f,
                                                         -0.2f,
                                                         0.0f,
                                                         true,
                                                         true);

        QVERIFY(!cov.isEmpty());
        QCOMPARE(cov.dim, raw.info.nchan);
        QCOMPARE(cov.data.rows(), static_cast<Index>(raw.info.nchan));
        QCOMPARE(cov.data.cols(), static_cast<Index>(raw.info.nchan));
        QVERIFY(cov.nfree > 0);
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
