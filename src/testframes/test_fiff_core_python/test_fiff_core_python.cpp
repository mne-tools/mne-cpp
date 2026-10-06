//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_core_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates FIFF averaging, rejection, SSP, compensation and transforms against mne-python.
 *
 * These are the FIFF routines whose mistakes look like data: an average built
 * from the wrong epochs, an SSP vector spanning the wrong subspace or a
 * compensator with its reference columns swapped all produce plausible output.
 * Every expected value here comes from mne-python (1.x, ../mne-python) run on
 * the same files, never from MNE-CPP itself.
 *
 * Averages and rejection (sample_audvis_trunc_raw.fif, events from
 * mne.find_events(raw, 'STI 014'), tmin -0.1 s, tmax 0.2 s, proj=False):
 *
 *   e = mne.Epochs(raw, ev, event_id=code, tmin=-0.1, tmax=0.2, proj=False,
 *                  baseline=bl, reject=rej, flat=flat, preload=True)
 *   d = e.get_data();  d = np.abs(d) if do_abs else d
 *   [np.abs(d.mean(0)[types == t]).sum() for t in (grad, mag, eeg, eog)]
 *
 * SSP vectors (same epochs, raw.del_proj()):
 *
 *   mne.compute_proj_epochs(e, n_grad=2, n_mag=1, n_eeg=1)
 *
 * Compensators: mne._fiff.compensator.make_compensator on a synthetic
 * two-MEG / two-reference info whose compensation columns are listed in the
 * opposite order to the channels, so a reader that ignores column names fails.
 *
 * Evoked reading: mne.read_evokeds(sample_audvis-ave.fif, condition=...,
 * baseline=..., proj=...). Transforms: the FIFF_COORD_TRANS tags of
 * all-trans.fif read with mne._fiff.tag.read_tag.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_coord_trans_set.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_sparse_matrix.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_events.h>

#include <cmath>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================

namespace
{

enum class ChType
{
    Grad,
    Mag,
    Eeg,
    Eog,
    Other
};

ChType chType(const FiffChInfo& ch)
{
    if (ch.kind == FIFFV_MEG_CH) {
        return ch.unit == FIFF_UNIT_T ? ChType::Mag : ChType::Grad;
    }
    if (ch.kind == FIFFV_EEG_CH) {
        return ChType::Eeg;
    }
    if (ch.kind == FIFFV_EOG_CH) {
        return ChType::Eog;
    }
    return ChType::Other;
}

double absSum(const MatrixXd& data, const FiffInfo& info, ChType type)
{
    double sum = 0.0;
    for (int c = 0; c < info.nchan; ++c) {
        if (chType(info.chs[c]) == type) {
            sum += data.row(c).cwiseAbs().sum();
        }
    }
    return sum;
}

bool relClose(double actual, double expected, double tol)
{
    return std::fabs(actual - expected) <= tol * std::fabs(expected);
}

QString mismatch(const QString& what, double actual, double expected)
{
    return QString("%1 is %2, mne-python says %3").arg(what).arg(actual, 0, 'g', 17).arg(expected, 0, 'g', 17);
}

// A rejection setting with every limit off; each test row switches one on.
RejectionParams noRejection()
{
    RejectionParams rej;
    rej.megGradReject = 0.0f;
    rej.megMagReject = 0.0f;
    rej.eegReject = 0.0f;
    rej.eogReject = 0.0f;
    rej.ecgReject = 0.0f;
    return rej;
}

FiffChInfo makeChannel(const QString& name, int kind, int unit)
{
    FiffChInfo ch;
    ch.ch_name = name;
    ch.kind = kind;
    ch.unit = unit;
    return ch;
}

FiffInfo makeInfo(const QList<FiffChInfo>& chs)
{
    FiffInfo info;
    info.chs = chs;
    for (const FiffChInfo& ch : chs) {
        info.ch_names << ch.ch_name;
    }
    info.nchan = static_cast<int>(chs.size());
    return info;
}

FiffCtfComp makeComp(int kind, const MatrixXd& data)
{
    // Column order deliberately reversed relative to the channel list.
    FiffCtfComp comp;
    comp.kind = kind;
    comp.ctfkind = kind;
    comp.data = FiffNamedMatrix::SDPtr(new FiffNamedMatrix(2, 2, {"MEG1", "MEG2"}, {"REF2", "REF1"}, data));
    return comp;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCorePython
 *
 * @brief Checks FIFF averaging, rejection, SSP, compensation and transforms against mne-python.
 */
class TestFiffCorePython : public QObject
{
    Q_OBJECT

public:
    TestFiffCorePython() = default;

private:
    static QString dataPath(const QString& file);
    MatrixXi events(bool withBoundaryEvents) const;

    std::unique_ptr<QFile> m_rawFile;
    std::unique_ptr<FiffRawData> m_raw;

private slots:
    void initTestCase();

    void computeAverages_matchPython_data();
    void computeAverages_matchPython();
    void computeAverages_rejection_data();
    void computeAverages_rejection();
    void computeAverages_noMatchingEvents();
    void checkArtifacts_reason_data();
    void checkArtifacts_reason();
    void checkArtifacts_skipsBadChannels();

    void computeFromRaw_matchesPython();
    void computeFromRaw_withRejection();
    void computeFromRaw_invalidInput();

    void makeCompensator_matchesPython_data();
    void makeCompensator_matchesPython();
    void makeCompensator_errors();

    void readEvoked_selection_data();
    void readEvoked_selection();
    void readEvoked_invalidSelector_data();
    void readEvoked_invalidSelector();

    void readTransformFromNode_data();
    void readTransformFromNode();
    void readTransform_fromFile();
    void readTransformAscii_errors_data();
    void readTransformAscii_errors();
    void combine_orientations_data();
    void combine_orientations();
    void applyTrans_withoutMove();
    void transSet_headToMniMatchesPython();
    void events_matchPython();

    void sparse_createAndConvert();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffCorePython::dataPath(const QString& file)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/" + file;
}

//=============================================================================================================

MatrixXi TestFiffCorePython::events(bool withBoundaryEvents) const
{
    // mne.find_events(raw, stim_channel='STI 014') on the truncated file.
    static const int ev[][3] = {
        {13988, 0, 2}, {14172, 0, 3}, {14385, 0, 1}, {14609, 0, 4}, {14826, 0, 2}, {15012, 0, 3}, {15225, 0, 1}, {15419, 0, 4}, {15620, 0, 2}, {15832, 0, 3}, {16050, 0, 1}, {16259, 0, 4}, {16467, 0, 2}, {16662, 0, 3}, {16856, 0, 1}, {17044, 0, 5}, {17266, 0, 2}, {17324, 0, 32}, {17478, 0, 3}, {17714, 0, 1}, {17925, 0, 4}, {18105, 0, 2}, {18288, 0, 3}, {18503, 0, 1}, {18730, 0, 4}};
    const int n = static_cast<int>(sizeof(ev) / sizeof(ev[0]));
    const int extra = withBoundaryEvents ? 3 : 0;

    MatrixXi m(n + extra, 3);
    for (int k = 0; k < n; ++k) {
        m.row(k) << ev[k][0], ev[k][1], ev[k][2];
    }
    if (withBoundaryEvents) {
        // Matching code but the epoch would start before the first sample or
        // end after the last; mne-python drops these as well. The third has a
        // non-zero "from" value and must not match code 1.
        m.row(n) << m_raw->first_samp + 5, 0, 1;
        m.row(n + 1) << m_raw->last_samp - 5, 0, 1;
        m.row(n + 2) << 16000, 4, 1;
    }
    return m;
}

//=============================================================================================================

void TestFiffCorePython::initTestCase()
{
    if (!QFile::exists(dataPath("sample_audvis_trunc_raw.fif"))) {
        QSKIP("Sample test data not found");
    }
    m_rawFile = std::make_unique<QFile>(dataPath("sample_audvis_trunc_raw.fif"));
    m_raw = std::make_unique<FiffRawData>(*m_rawFile);

    QCOMPARE(m_raw->info.nchan, 376);
    QCOMPARE(static_cast<int>(m_raw->first_samp), 12900);
    QCOMPARE(static_cast<int>(m_raw->last_samp), 18906);
    // No projector or compensator is installed, which is what proj=False means.
    QCOMPARE(static_cast<int>(m_raw->proj.size()), 0);
}

//=============================================================================================================

void TestFiffCorePython::computeAverages_matchPython_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<bool>("baseline");
    QTest::addColumn<bool>("doAbs");
    QTest::addColumn<double>("grad");
    QTest::addColumn<double>("mag");
    QTest::addColumn<double>("eeg");
    QTest::addColumn<double>("eog");

    QTest::newRow("aud-l plain") << 1 << false << false << 3.0735398324496474e-11 << 1.3488598299753733e-12 << 1.1904711247586373e-05 << 2.10270790722666e-07;
    QTest::newRow("aud-l baseline") << 1 << true << false << 1.106505155895055e-11 << 3.4322098202687137e-13 << 5.219959607330511e-06 << 6.750108741353492e-08;
    QTest::newRow("aud-l baseline abs") << 1 << true << true << 2.4025158100731896e-11 << 7.313975510984896e-13 << 1.0660417524381542e-05 << 1.172659157847277e-07;
    QTest::newRow("vis-l plain") << 3 << false << false << 3.187044517053265e-11 << 1.2775743913580438e-12 << 1.1555635151039213e-05 << 2.109665960127415e-07;
    QTest::newRow("vis-l baseline") << 3 << true << false << 1.1349163329993472e-11 << 2.9724458139401987e-13 << 5.918986900247635e-06 << 4.792135262861953e-08;
    QTest::newRow("vis-l baseline abs") << 3 << true << true << 2.390662131508878e-11 << 6.894236532405748e-13 << 1.1248023196839169e-05 << 1.0021399633288837e-07;
}

//=============================================================================================================

void TestFiffCorePython::computeAverages_matchPython()
{
    QFETCH(int, code);
    QFETCH(bool, baseline);
    QFETCH(bool, doAbs);
    QFETCH(double, grad);
    QFETCH(double, mag);
    QFETCH(double, eeg);
    QFETCH(double, eog);

    AverageCategory cat;
    cat.comment = QString("code %1").arg(code);
    cat.events = {static_cast<unsigned int>(code)};
    cat.tmin = -0.1f;
    cat.tmax = 0.2f;
    cat.doBaseline = baseline;
    cat.bmin = -0.1f;
    cat.bmax = 0.0f;
    cat.doAbs = doAbs;

    AverageDescription desc;
    desc.comment = "oracle";
    desc.categories << cat;
    desc.rej = noRejection();

    QString log;
    const FiffEvokedSet set = FiffEvokedSet::computeAverages(*m_raw, desc, events(true), log);
    QCOMPARE(set.evoked.size(), 1);
    const FiffEvoked& e = set.evoked.first();

    // Six epochs: the two out-of-range events and the one with a non-zero
    // "from" value are dropped, exactly as mne-python drops them.
    QCOMPARE(static_cast<int>(e.nave), 6);
    QCOMPARE(e.first, -30);
    QCOMPARE(e.last, 60);
    QCOMPARE(static_cast<int>(e.data.cols()), 91);
    QCOMPARE(static_cast<int>(e.times.size()), 91);

    // The log once printed the literal text "%.1f" because printf
    // placeholders were handed to QString::arg.
    QVERIFY2(log.contains("t = -100.0 ... 200.0 ms"), qPrintable(log.left(200)));
    QVERIFY(!log.contains("%."));

    // Both sides average the same float32 samples in double precision, so the
    // only difference is summation order: they agree to ~1e-15 relative. 1e-9
    // leaves room for platform BLAS differences while a single misplaced
    // epoch or a one-sample window shift moves these sums by more than 1e-3.
    const double tol = 1e-9;
    QVERIFY2(relClose(absSum(e.data, e.info, ChType::Grad), grad, tol), qPrintable(mismatch("grad", absSum(e.data, e.info, ChType::Grad), grad)));
    QVERIFY2(relClose(absSum(e.data, e.info, ChType::Mag), mag, tol), qPrintable(mismatch("mag", absSum(e.data, e.info, ChType::Mag), mag)));
    QVERIFY2(relClose(absSum(e.data, e.info, ChType::Eeg), eeg, tol), qPrintable(mismatch("eeg", absSum(e.data, e.info, ChType::Eeg), eeg)));
    QVERIFY2(relClose(absSum(e.data, e.info, ChType::Eog), eog, tol), qPrintable(mismatch("eog", absSum(e.data, e.info, ChType::Eog), eog)));
}

//=============================================================================================================

void TestFiffCorePython::computeAverages_rejection_data()
{
    // Thresholds were chosen between the per-epoch peak-to-peak values that
    // mne-python reports for the six code-1 epochs, at least 0.2% away from
    // any of them, so each row keeps a different subset.
    QTest::addColumn<QString>("field");
    QTest::addColumn<float>("limit");
    QTest::addColumn<int>("nave");
    QTest::addColumn<double>("eeg");

    QTest::newRow("grad reject") << "megGradReject" << 1.2e-14f << 2 << 1.2561669862604944e-05;
    QTest::newRow("mag reject") << "megMagReject" << 7e-16f << 3 << 1.2338104143969415e-05;
    QTest::newRow("eeg reject") << "eegReject" << 1.6e-8f << 4 << 1.1877121950010974e-05;
    QTest::newRow("eog reject") << "eogReject" << 5e-9f << 3 << 1.476797365256261e-05;
    QTest::newRow("grad flat") << "megGradFlat" << 3.2e-15f << 3 << 1.457740461087094e-05;
    QTest::newRow("mag flat") << "megMagFlat" << 2.5e-16f << 4 << 1.2049436000001193e-05;
    QTest::newRow("eeg flat") << "eegFlat" << 5e-9f << 5 << 1.3200858643199586e-05;
    QTest::newRow("eog flat") << "eogFlat" << 4e-9f << 4 << 1.1971019973695706e-05;
}

//=============================================================================================================

void TestFiffCorePython::computeAverages_rejection()
{
    QFETCH(QString, field);
    QFETCH(float, limit);
    QFETCH(int, nave);
    QFETCH(double, eeg);

    RejectionParams rej = noRejection();
    const QMap<QString, float*> fields{
        {"megGradReject", &rej.megGradReject}, {"megMagReject", &rej.megMagReject}, {"eegReject", &rej.eegReject}, {"eogReject", &rej.eogReject}, {"megGradFlat", &rej.megGradFlat}, {"megMagFlat", &rej.megMagFlat}, {"eegFlat", &rej.eegFlat}, {"eogFlat", &rej.eogFlat}};
    QVERIFY(fields.contains(field));
    *fields.value(field) = limit;

    AverageCategory cat;
    cat.comment = "aud-l";
    cat.events = {1u};
    cat.tmin = -0.1f;
    cat.tmax = 0.2f;

    AverageDescription desc;
    desc.categories << cat;
    desc.rej = rej;

    QString log;
    const FiffEvokedSet set = FiffEvokedSet::computeAverages(*m_raw, desc, events(false), log);
    const FiffEvoked& e = set.evoked.first();

    QCOMPARE(static_cast<int>(e.nave), nave);
    QCOMPARE(static_cast<int>(log.count("[omit]")), 6 - nave);
    QVERIFY(!log.contains("%."));

    const double sum = absSum(e.data, e.info, ChType::Eeg);
    QVERIFY2(relClose(sum, eeg, 1e-9), qPrintable(mismatch("eeg", sum, eeg)));
}

//=============================================================================================================

void TestFiffCorePython::computeAverages_noMatchingEvents()
{
    // A category nothing matches still yields an evoked entry, with nave 0 and
    // zero data, rather than a division by zero.
    AverageCategory cat;
    cat.events = {99u};
    cat.tmin = -0.1f;
    cat.tmax = 0.2f;

    AverageDescription desc;
    desc.categories << cat;

    QString log;
    const FiffEvokedSet set = FiffEvokedSet::computeAverages(*m_raw, desc, events(false), log);
    QCOMPARE(set.evoked.size(), 1);
    QCOMPARE(static_cast<int>(set.evoked.first().nave), 0);
    QCOMPARE(set.evoked.first().data.cwiseAbs().sum(), 0.0);
    QVERIFY(log.contains("nave = 0"));
}

//=============================================================================================================

void TestFiffCorePython::checkArtifacts_reason_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("unit");
    QTest::addColumn<double>("pp");
    QTest::addColumn<QString>("field");
    QTest::addColumn<float>("limit");
    QTest::addColumn<QString>("reason");

    QTest::newRow("mag reject") << FIFFV_MEG_CH << FIFF_UNIT_T << 5e-12 << "megMagReject" << 4e-12f << "X : 5000.0 fT > 4000.0 fT";
    QTest::newRow("mag flat") << FIFFV_MEG_CH << FIFF_UNIT_T << 1e-15 << "megMagFlat" << 2e-15f << "X : 1.0 fT < 2.0 fT (flat)";
    QTest::newRow("grad reject") << FIFFV_MEG_CH << FIFF_UNIT_T_M << 3e-10 << "megGradReject" << 2e-10f << "X : 3000.0 fT/cm > 2000.0 fT/cm";
    QTest::newRow("grad flat") << FIFFV_MEG_CH << FIFF_UNIT_T_M << 1e-14 << "megGradFlat" << 2e-14f << "X : 0.1 fT/cm < 0.2 fT/cm (flat)";
    QTest::newRow("eeg reject") << FIFFV_EEG_CH << FIFF_UNIT_V << 2e-4 << "eegReject" << 1e-4f << "X : 200.0 uV > 100.0 uV";
    QTest::newRow("eeg flat") << FIFFV_EEG_CH << FIFF_UNIT_V << 1e-7 << "eegFlat" << 1e-6f << "X : 0.1 uV < 1.0 uV (flat)";
    QTest::newRow("eog reject") << FIFFV_EOG_CH << FIFF_UNIT_V << 3e-4 << "eogReject" << 1.5e-4f << "X : 300.0 uV > 150.0 uV (EOG)";
    QTest::newRow("eog flat") << FIFFV_EOG_CH << FIFF_UNIT_V << 1e-7 << "eogFlat" << 1e-6f << "X : EOG flat";
    QTest::newRow("ecg reject") << FIFFV_ECG_CH << FIFF_UNIT_V << 2e-3 << "ecgReject" << 1e-3f << "X : 2.00 mV > 1.00 mV (ECG)";
    QTest::newRow("ecg flat") << FIFFV_ECG_CH << FIFF_UNIT_V << 1e-7 << "ecgFlat" << 1e-6f << "X : ECG flat";
    QTest::newRow("stim ignored") << FIFFV_STIM_CH << FIFF_UNIT_V << 5.0 << "eegReject" << 1e-6f << "";
}

//=============================================================================================================

void TestFiffCorePython::checkArtifacts_reason()
{
    QFETCH(int, kind);
    QFETCH(int, unit);
    QFETCH(double, pp);
    QFETCH(QString, field);
    QFETCH(float, limit);
    QFETCH(QString, reason);

    const FiffInfo info = makeInfo({makeChannel("X", kind, unit)});
    MatrixXd epoch = MatrixXd::Zero(1, 10);
    epoch(0, 3) = pp;

    RejectionParams rej = noRejection();
    const QMap<QString, float*> fields{
        {"megGradReject", &rej.megGradReject}, {"megMagReject", &rej.megMagReject}, {"eegReject", &rej.eegReject}, {"eogReject", &rej.eogReject}, {"ecgReject", &rej.ecgReject}, {"megGradFlat", &rej.megGradFlat}, {"megMagFlat", &rej.megMagFlat}, {"eegFlat", &rej.eegFlat}, {"eogFlat", &rej.eogFlat}, {"ecgFlat", &rej.ecgFlat}};
    QVERIFY(fields.contains(field));
    *fields.value(field) = limit;

    QString actual;
    const bool clean = FiffEvokedSet::checkArtifacts(epoch, info, QStringList(), rej, actual);
    QCOMPARE(clean, reason.isEmpty());
    QCOMPARE(actual, reason);
}

//=============================================================================================================

void TestFiffCorePython::checkArtifacts_skipsBadChannels()
{
    const FiffInfo info = makeInfo({makeChannel("EEG1", FIFFV_EEG_CH, FIFF_UNIT_V), makeChannel("EEG2", FIFFV_EEG_CH, FIFF_UNIT_V)});
    MatrixXd epoch = MatrixXd::Zero(2, 10);
    epoch(0, 2) = 1.0; // far beyond any limit, but on a bad channel
    epoch(1, 2) = 1e-6;

    RejectionParams rej = noRejection();
    rej.eegReject = 1e-4f;

    QString reason;
    QVERIFY(FiffEvokedSet::checkArtifacts(epoch, info, {"EEG1"}, rej, reason));
    QVERIFY(!FiffEvokedSet::checkArtifacts(epoch, info, {}, rej, reason));
    QVERIFY(reason.startsWith("EEG1 : "));
}

//=============================================================================================================

void TestFiffCorePython::computeFromRaw_matchesPython()
{
    const QList<FiffProj> projs = FiffProj::compute_from_raw(*m_raw, events(true), 1, -0.1f, 0.2f, 2, 1, 1);
    QCOMPARE(projs.size(), 4);

    // mne.compute_proj_epochs(e, n_grad=2, n_mag=1, n_eeg=1): absolute sum of
    // each vector, its peak channel and the peak magnitude. Sign is arbitrary
    // for an SVD, so only magnitudes are compared. Bad channels are excluded
    // on both sides (203 gradiometers, 59 EEG).
    struct Expected
    {
        const char* desc;
        double absSum;
        const char* peakName;
        double peak;
    };
    const Expected expected[] = {
        {"PCA-grad-v1", 10.091097564301108, "MEG1823", 0.2501957576044427},
        {"PCA-grad-v2", 10.190314393972283, "MEG1032", 0.48101060726726025},
        {"PCA-mag-v1", 8.580154535344956, "MEG2011", 0.19903630986928558},
        {"PCA-eeg-v1", 6.735075674238848, "EEG008", 0.30820096830837057},
    };

    // The smallest singular-value gap mne-python reports for these vectors is
    // 1.06 (second gradiometer component), so float-level noise in the data
    // moves a vector by ~1e-12. 1e-6 is far above that and far below the 1.3%
    // change that centring the data before the SVD used to cause.
    for (int i = 0; i < 4; ++i) {
        const FiffProj& p = projs.at(i);
        QCOMPARE(p.desc, QString(expected[i].desc));
        QCOMPARE(p.kind, FIFFV_PROJ_ITEM_FIELD);
        QVERIFY(!p.active);

        const RowVectorXd v = p.data->data.row(0);
        QCOMPARE(static_cast<int>(v.size()), 376);
        QVERIFY2(std::fabs(v.norm() - 1.0) < 1e-12, "projection vector is not unit length");

        Index peakIdx = 0;
        const double peak = v.cwiseAbs().maxCoeff(&peakIdx);
        QCOMPARE(m_raw->info.ch_names.at(static_cast<int>(peakIdx)), QString(expected[i].peakName));
        QVERIFY2(relClose(v.cwiseAbs().sum(), expected[i].absSum, 1e-6), qPrintable(mismatch(p.desc + " abs sum", v.cwiseAbs().sum(), expected[i].absSum)));
        QVERIFY2(relClose(peak, expected[i].peak, 1e-6), qPrintable(mismatch(p.desc + " peak", peak, expected[i].peak)));

        // Bad channels must not take part in the projection.
        for (const QString& bad : m_raw->info.bads) {
            QCOMPARE(v(m_raw->info.ch_names.indexOf(bad)), 0.0);
        }
    }

    // The two gradiometer vectors span a two dimensional subspace.
    QVERIFY(std::fabs(projs[0].data->data.row(0).dot(projs[1].data->data.row(0))) < 1e-12);
}

//=============================================================================================================

void TestFiffCorePython::computeFromRaw_withRejection()
{
    // mne-python with reject=dict(eeg=1.6e-8) keeps four of the six epochs.
    const QList<FiffProj> projs = FiffProj::compute_from_raw(*m_raw, events(false), 1, -0.1f, 0.2f, 0, 0, 1, {{"eeg", 1.6e-8}});
    QCOMPARE(projs.size(), 1);

    const RowVectorXd v = projs.first().data->data.row(0);
    Index peakIdx = 0;
    const double peak = v.cwiseAbs().maxCoeff(&peakIdx);
    QCOMPARE(m_raw->info.ch_names.at(static_cast<int>(peakIdx)), QString("EEG008"));
    QVERIFY2(relClose(v.cwiseAbs().sum(), 6.897932646721715, 1e-6), qPrintable(mismatch("abs sum", v.cwiseAbs().sum(), 6.897932646721715)));
    QVERIFY2(relClose(peak, 0.30228902469573343, 1e-6), qPrintable(mismatch("peak", peak, 0.30228902469573343)));
}

//=============================================================================================================

void TestFiffCorePython::computeFromRaw_invalidInput()
{
    // Reversed window.
    QVERIFY(FiffProj::compute_from_raw(*m_raw, events(false), 1, 0.2f, -0.1f, 1, 1, 1).isEmpty());
    // No epoch matches.
    QVERIFY(FiffProj::compute_from_raw(*m_raw, events(false), 99, -0.1f, 0.2f, 1, 1, 1).isEmpty());
    // Every epoch rejected.
    QVERIFY(FiffProj::compute_from_raw(*m_raw, events(false), 1, -0.1f, 0.2f, 1, 1, 1, {{"mag", 1e-18}}).isEmpty());
    // Nothing requested.
    QVERIFY(FiffProj::compute_from_raw(*m_raw, events(false), 1, -0.1f, 0.2f, 0, 0, 0).isEmpty());
}

//=============================================================================================================

void TestFiffCorePython::makeCompensator_matchesPython_data()
{
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<bool>("exclude");
    QTest::addColumn<QList<double>>("expected");

    // Rows of mne._fiff.compensator.make_compensator(info, from_, to, exclude).
    // The reference rows of C are zero, so mne-python's inv(I - C1) equals
    // MNE-CPP's (I + C1) exactly; 1e-15 absolute only absorbs rounding.
    const QList<double> refRows{0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    QTest::newRow("0 -> 1") << 0 << 1 << false << (QList<double>{1.0, 0.0, -0.2, -0.1, 0.0, 1.0, -0.4, -0.3} + refRows);
    QTest::newRow("0 -> 3") << 0 << 3 << false << (QList<double>{1.0, 0.0, 0.1, -0.5, 0.0, 1.0, -0.05, -0.2} + refRows);
    QTest::newRow("1 -> 0") << 1 << 0 << false << (QList<double>{1.0, 0.0, 0.2, 0.1, 0.0, 1.0, 0.4, 0.3} + refRows);
    QTest::newRow("3 -> 0") << 3 << 0 << false << (QList<double>{1.0, 0.0, -0.1, 0.5, 0.0, 1.0, 0.05, 0.2} + refRows);
    QTest::newRow("1 -> 3") << 1 << 3 << false << (QList<double>{1.0, 0.0, 0.30000000000000004, -0.4, 0.0, 1.0, 0.35000000000000003, 0.09999999999999998} + refRows);
    QTest::newRow("3 -> 1") << 3 << 1 << false << (QList<double>{1.0, 0.0, -0.30000000000000004, 0.4, 0.0, 1.0, -0.35000000000000003, -0.09999999999999998} + refRows);
    QTest::newRow("1 -> 3 exclude refs") << 1 << 3 << true << QList<double>{1.0, 0.0, 0.30000000000000004, -0.4, 0.0, 1.0, 0.35000000000000003, 0.09999999999999998};
}

//=============================================================================================================

void TestFiffCorePython::makeCompensator_matchesPython()
{
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(bool, exclude);
    QFETCH(QList<double>, expected);

    FiffInfo info = makeInfo({makeChannel("MEG1", FIFFV_MEG_CH, FIFF_UNIT_T), makeChannel("MEG2", FIFFV_MEG_CH, FIFF_UNIT_T), makeChannel("REF1", FIFFV_REF_MEG_CH, FIFF_UNIT_T), makeChannel("REF2", FIFFV_REF_MEG_CH, FIFF_UNIT_T)});
    info.comps << makeComp(1, (MatrixXd(2, 2) << 0.1, 0.2, 0.3, 0.4).finished());
    info.comps << makeComp(3, (MatrixXd(2, 2) << 0.5, -0.1, 0.2, 0.05).finished());

    FiffCtfComp comp;
    QVERIFY(info.make_compensator(from, to, comp, exclude));

    const MatrixXd& m = comp.data->data;
    QCOMPARE(static_cast<int>(m.rows()), static_cast<int>(expected.size() / 4));
    QCOMPARE(static_cast<int>(m.cols()), 4);
    for (int r = 0; r < m.rows(); ++r) {
        for (int c = 0; c < 4; ++c) {
            QVERIFY2(std::fabs(m(r, c) - expected.at(r * 4 + c)) < 1e-15, qPrintable(QString("(%1,%2) is %3, mne-python says %4").arg(r).arg(c).arg(m(r, c), 0, 'g', 17).arg(expected.at(r * 4 + c), 0, 'g', 17)));
        }
    }
}

//=============================================================================================================

void TestFiffCorePython::makeCompensator_errors()
{
    const QList<FiffChInfo> chs{makeChannel("MEG1", FIFFV_MEG_CH, FIFF_UNIT_T), makeChannel("MEG2", FIFFV_MEG_CH, FIFF_UNIT_T), makeChannel("REF1", FIFFV_REF_MEG_CH, FIFF_UNIT_T), makeChannel("REF2", FIFFV_REF_MEG_CH, FIFF_UNIT_T)};
    FiffInfo info = makeInfo(chs);
    info.comps << makeComp(1, MatrixXd::Ones(2, 2));

    FiffCtfComp comp;
    // Grade not present, on either side.
    QVERIFY(!info.make_compensator(0, 2, comp));
    QVERIFY(!info.make_compensator(2, 0, comp));

    // A compensation column naming a channel that is not in the data.
    FiffInfo missing = makeInfo(chs.mid(0, 3));
    missing.comps << makeComp(1, MatrixXd::Ones(2, 2));
    QVERIFY(!missing.make_compensator(0, 1, comp));

    // Two channels with the same name make the column ambiguous.
    FiffInfo ambiguous = makeInfo(chs + QList<FiffChInfo>{makeChannel("REF1", FIFFV_REF_MEG_CH, FIFF_UNIT_T)});
    ambiguous.comps << makeComp(1, MatrixXd::Ones(2, 2));
    QVERIFY(!ambiguous.make_compensator(0, 1, comp));

    // Excluding the reference channels from a reference-only info leaves nothing.
    FiffInfo refsOnly = makeInfo({makeChannel("REF1", FIFFV_REF_MEG_CH, FIFF_UNIT_T), makeChannel("REF2", FIFFV_REF_MEG_CH, FIFF_UNIT_T)});
    FiffCtfComp refComp;
    refComp.kind = 1;
    refComp.data = FiffNamedMatrix::SDPtr(new FiffNamedMatrix(1, 2, {"REF1"}, {"REF2", "REF1"}, MatrixXd::Ones(1, 2)));
    refsOnly.comps << refComp;
    QVERIFY(refsOnly.make_compensator(0, 1, comp, false));
    QVERIFY(!refsOnly.make_compensator(0, 1, comp, true));
}

//=============================================================================================================

void TestFiffCorePython::readEvoked_selection_data()
{
    QTest::addColumn<QVariant>("setno");
    QTest::addColumn<float>("bmin");
    QTest::addColumn<float>("bmax");
    QTest::addColumn<bool>("proj");
    QTest::addColumn<int>("nave");
    QTest::addColumn<double>("sum");
    QTest::addColumn<double>("absSum");

    // mne.read_evokeds(f, condition=..., baseline=..., proj=...).
    // The file's projectors are already applied to the stored data, so
    // proj=True must leave it unchanged: projectors are idempotent.
    QTest::newRow("by name") << QVariant(QString("Right Auditory")) << -1.0f << -1.0f << false << 61 << 79.10733502294158 << 79.26608239688679;
    QTest::newRow("by index") << QVariant(1) << -1.0f << -1.0f << false << 61 << 79.10733502294158 << 79.26608239688679;
    QTest::newRow("projected") << QVariant(QString("Right Auditory")) << -1.0f << -1.0f << true << 61 << 79.10733502294158 << 79.26608239688679;
    QTest::newRow("baseline") << QVariant(QString("Right Auditory")) << -0.2f << 0.0f << false << 61 << 4.109845103910902 << 142.4768634370533;
}

//=============================================================================================================

void TestFiffCorePython::readEvoked_selection()
{
    QFETCH(QVariant, setno);
    QFETCH(float, bmin);
    QFETCH(float, bmax);
    QFETCH(bool, proj);
    QFETCH(int, nave);
    QFETCH(double, sum);
    QFETCH(double, absSum);

    QFile file(dataPath("sample_audvis-ave.fif"));
    FiffEvoked e;
    QVERIFY(FiffEvoked::read(file, e, setno, qMakePair(bmin, bmax), proj));

    QCOMPARE(e.comment, QString("Right Auditory"));
    QCOMPARE(static_cast<int>(e.nave), nave);
    QCOMPARE(proj, e.proj.rows() > 0);

    // The epoch tags are float32 (relative precision ~1.2e-7); the two
    // libraries agree to ~1e-9 relative on these sums, see
    // test_fiff_evoked_python. The baseline row sums a signed quantity that
    // nearly cancels, so it gets the same tolerance on the abs sum but a
    // looser one on the signed sum.
    QVERIFY2(relClose(e.data.cwiseAbs().sum(), absSum, 1e-7), qPrintable(mismatch("abs sum", e.data.cwiseAbs().sum(), absSum)));
    QVERIFY2(relClose(e.data.sum(), sum, 1e-6), qPrintable(mismatch("sum", e.data.sum(), sum)));
}

//=============================================================================================================

void TestFiffCorePython::readEvoked_invalidSelector_data()
{
    QTest::addColumn<QVariant>("setno");
    QTest::addColumn<int>("aspectKind");

    QTest::newRow("unknown name") << QVariant(QString("No such condition")) << static_cast<int>(FIFFV_ASPECT_AVERAGE);
    QTest::newRow("wrong aspect") << QVariant(QString("Right Auditory")) << static_cast<int>(FIFFV_ASPECT_STD_ERR);
    QTest::newRow("bad aspect kind") << QVariant(QString("Right Auditory")) << 12345;
    QTest::newRow("index too large") << QVariant(4) << static_cast<int>(FIFFV_ASPECT_AVERAGE);
    QTest::newRow("negative index") << QVariant(-1) << static_cast<int>(FIFFV_ASPECT_AVERAGE);
    QTest::newRow("unset with four sets") << QVariant() << static_cast<int>(FIFFV_ASPECT_AVERAGE);
}

//=============================================================================================================

void TestFiffCorePython::readEvoked_invalidSelector()
{
    QFETCH(QVariant, setno);
    QFETCH(int, aspectKind);

    QFile file(dataPath("sample_audvis-ave.fif"));
    FiffEvoked e;
    QVERIFY(!FiffEvoked::read(file, e, setno, defaultFloatPair, false, aspectKind));
}

//=============================================================================================================

void TestFiffCorePython::readTransformFromNode_data()
{
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<bool>("found");
    QTest::addColumn<float>("tx");
    QTest::addColumn<float>("tz");

    // all-trans.fif holds device->head (1 -> 4) and MRI->head (5 -> 4).
    // Translations from read_tag; float32 on disk, so compared exactly.
    QTest::newRow("stored direction") << FIFFV_COORD_DEVICE << FIFFV_COORD_HEAD << true << -0.006129309069365263f << 0.06474152207374573f;
    QTest::newRow("inverse direction") << FIFFV_COORD_HEAD << FIFFV_COORD_MRI << true << -0.0031674494966864586f << 0.02888404205441475f;
    QTest::newRow("not stored") << FIFFV_COORD_DEVICE << FIFFV_COORD_MRI << false << 0.0f << 0.0f;
}

//=============================================================================================================

void TestFiffCorePython::readTransformFromNode()
{
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(bool, found);
    QFETCH(float, tx);
    QFETCH(float, tz);

    QFile file(dataPath("all-trans.fif"));
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    const FiffCoordTrans t = FiffCoordTrans::readTransformFromNode(stream, stream->dirtree(), from, to);
    stream->close();

    QCOMPARE(!t.isEmpty(), found);
    if (!found) {
        return;
    }
    QCOMPARE(t.from, from);
    QCOMPARE(t.to, to);

    // For the inverted case the stored matrix becomes the inverse.
    const bool stored = (from == FIFFV_COORD_DEVICE);
    const auto& m = stored ? t.trans : t.invtrans;
    QCOMPARE(m(0, 3), tx);
    QCOMPARE(m(2, 3), tz);
    QVERIFY((t.trans * t.invtrans - Matrix4f::Identity()).cwiseAbs().maxCoeff() < 1e-6f);
}

//=============================================================================================================

void TestFiffCorePython::readTransform_fromFile()
{
    const QString path = dataPath("all-trans.fif");

    // Stored as 1 -> 4; asking for 4 -> 1 must invert it.
    const FiffCoordTrans fwd = FiffCoordTrans::readTransform(path, FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD);
    const FiffCoordTrans inv = FiffCoordTrans::readTransform(path, FIFFV_COORD_HEAD, FIFFV_COORD_DEVICE);
    QCOMPARE(fwd.trans(0, 3), -0.006129309069365263f);
    QCOMPARE(inv.from, FIFFV_COORD_HEAD);
    QCOMPARE(inv.to, FIFFV_COORD_DEVICE);
    QVERIFY((inv.trans - fwd.invtrans).cwiseAbs().maxCoeff() < 1e-6f);

    QVERIFY(FiffCoordTrans::readTransform(path, FIFFV_COORD_DEVICE, FIFFV_COORD_MRI).isEmpty());
    QVERIFY(FiffCoordTrans::readTransform(dataPath("does-not-exist.fif"), FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD).isEmpty());
}

//=============================================================================================================

void TestFiffCorePython::readTransformAscii_errors_data()
{
    QTest::addColumn<QString>("content");
    QTest::addColumn<bool>("valid");

    QTest::newRow("valid with comments") << "# head to MRI\n1 0 0 10\n0 1 0 20 # mm\n\n0 0 1 30\n0 0 0 1\n"
                                         << true;
    QTest::newRow("three columns") << "1 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n"
                                   << false;
    QTest::newRow("not a number") << "1 0 0 x\n0 1 0 0\n0 0 1 0\n0 0 0 1\n"
                                  << false;
    QTest::newRow("only three rows") << "1 0 0 0\n0 1 0 0\n0 0 1 0\n"
                                     << false;
    QTest::newRow("empty") << "" << false;
}

//=============================================================================================================

void TestFiffCorePython::readTransformAscii_errors()
{
    QFETCH(QString, content);
    QFETCH(bool, valid);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("head2mri.txt");
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream(&f) << content;
    }

    const FiffCoordTrans t = FiffCoordTrans::readFShead2mriTransform(path);
    QCOMPARE(!t.isEmpty(), valid);
    if (valid) {
        QCOMPARE(t.from, FIFFV_COORD_HEAD);
        QCOMPARE(t.to, FIFFV_COORD_MRI);
        // Translations are given in millimetres and stored in metres.
        QCOMPARE(t.trans(0, 3), 0.01f);
        QCOMPARE(t.trans(1, 3), 0.02f);
        QCOMPARE(t.trans(2, 3), 0.03f);
    }
}

//=============================================================================================================

void TestFiffCorePython::combine_orientations_data()
{
    QTest::addColumn<bool>("invertFirst");
    QTest::addColumn<bool>("invertSecond");
    QTest::addColumn<bool>("swap");

    for (int i = 0; i < 8; ++i) {
        const bool a = i & 1, b = i & 2, s = i & 4;
        QTest::newRow(qPrintable(QString("inv1=%1 inv2=%2 swap=%3").arg(a).arg(b).arg(s))) << a << b << s;
    }
}

//=============================================================================================================

void TestFiffCorePython::combine_orientations()
{
    QFETCH(bool, invertFirst);
    QFETCH(bool, invertSecond);
    QFETCH(bool, swap);

    // device -> head and MRI -> head; whichever way round they are stored or
    // passed, device -> MRI must come out the same.
    const Matrix3f r1 = AngleAxisf(0.3f, Vector3f(0.0f, 0.0f, 1.0f)).toRotationMatrix();
    const Matrix3f r2 = AngleAxisf(-0.2f, Vector3f(1.0f, 0.0f, 0.0f)).toRotationMatrix();
    const FiffCoordTrans devHead(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, r1, Vector3f(0.01f, -0.02f, 0.04f));
    const FiffCoordTrans mriHead(FIFFV_COORD_MRI, FIFFV_COORD_HEAD, r2, Vector3f(-0.003f, 0.005f, 0.02f));

    const FiffCoordTrans t1 = invertFirst ? devHead.inverted() : devHead;
    const FiffCoordTrans t2 = invertSecond ? mriHead.inverted() : mriHead;
    const FiffCoordTrans res = swap ? FiffCoordTrans::combine(FIFFV_COORD_DEVICE, FIFFV_COORD_MRI, t2, t1)
                                    : FiffCoordTrans::combine(FIFFV_COORD_DEVICE, FIFFV_COORD_MRI, t1, t2);

    QCOMPARE(res.from, FIFFV_COORD_DEVICE);
    QCOMPARE(res.to, FIFFV_COORD_MRI);

    MatrixX3f p(2, 3);
    p << 0.05f, 0.01f, -0.02f, -0.03f, 0.07f, 0.04f;
    const MatrixX3f viaHead = mriHead.apply_inverse_trans(devHead.apply_trans(p));
    QVERIFY((res.apply_trans(p) - viaHead).cwiseAbs().maxCoeff() < 1e-6f);

    // Frames that cannot be chained.
    QVERIFY(FiffCoordTrans::combine(FIFFV_COORD_DEVICE, FIFFV_MNE_COORD_MNI_TAL, t1, t2).isEmpty());
}

//=============================================================================================================

void TestFiffCorePython::applyTrans_withoutMove()
{
    // do_move = false transforms directions: rotation only, no translation.
    const Matrix3f r = AngleAxisf(0.5f, Vector3f(0.0f, 1.0f, 0.0f)).toRotationMatrix();
    const FiffCoordTrans t(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, r, Vector3f(1.0f, 2.0f, 3.0f));

    MatrixX3f v(1, 3);
    v << 0.0f, 0.0f, 1.0f;
    const MatrixX3f rotated = t.apply_trans(v, false);
    QVERIFY((rotated.row(0).transpose() - r * Vector3f(0.0f, 0.0f, 1.0f)).norm() < 1e-6f);
    QVERIFY((t.apply_inverse_trans(rotated, false) - v).norm() < 1e-6f);
}

//=============================================================================================================

void TestFiffCorePython::transSet_headToMniMatchesPython()
{
    // Sample subject: COR.fif carries surface RAS -> RAS, all-trans.fif the coregistration and
    // data/sample-talairach.xfm is the subject's mri/transforms/talairach.xfm.
    const QString corFile = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
    FiffCoordTransSet chain;
    QCOMPARE(chain.read(corFile), 2); // head -> MRI (identity) and surface RAS -> RAS
    QCOMPARE(chain.headToMni(MatrixX3f::Zero(1, 3)).rows(), 0);
    QCOMPARE(chain.read(dataPath("all-trans.fif")), 1);
    QVERIFY(chain.addTalairach(QStringLiteral(FIFF_CORE_DATA_DIR "/sample-talairach.xfm")));
    QVERIFY(!chain.addTalairach(corFile));

    MatrixX3f head(3, 3);
    head << 0.0f, 0.0f, 0.0f, 0.03f, -0.02f, 0.12f, -0.04f, 0.01f, -0.03f;
    // mne.head_to_mni(head, "sample", mri_head_t) / 1000 and M. Brett's Talairach matrices on top
    MatrixX3f mni(3, 3);
    mni << 0.004136082f, -0.023704993f, -0.066782980f, 0.036469229f, -0.065504959f, 0.057827104f, -0.037269056f, -0.010483342f, -0.095647763f;
    MatrixX3f talairach(3, 3);
    talairach << 0.004094721f, -0.025770282f, -0.054881228f, 0.036104536f, -0.060801158f, 0.056314317f, -0.036896366f, -0.014173468f, -0.079740031f;
    QVERIFY((chain.headToMni(head) - mni).cwiseAbs().maxCoeff() < 1e-6f);
    QVERIFY((chain.mniToTalairach(mni) - talairach).cwiseAbs().maxCoeff() < 1e-6f);

    // Written in MNE-C mne_collect_transforms order and read back, also through an inverted transform.
    QTemporaryDir dir;
    const QString file = dir.filePath("chain-trans.fif");
    {
        QFile out(file);
        FiffStream::SPtr stream = FiffStream::start_file(out);
        chain.write(*stream);
        stream->write_coord_trans(chain.RAS_MNI_tal_t.inverted());
        stream->end_file();
    }
    FiffCoordTransSet reread;
    QCOMPARE(reread.read(file), 6);
    QVERIFY((reread.headToMni(head) - mni).cwiseAbs().maxCoeff() < 1e-6f);
}

//=============================================================================================================

void TestFiffCorePython::events_matchPython()
{
    const MatrixXi onsets = events(false);

    FiffEvents found;
    QVERIFY(FiffEvents::detect_from_raw(*m_raw, found));
    QVERIFY(found.events == onsets);

    // find_events(output='step', consecutive=True): each pulse returns to 0 after this many samples.
    const QByteArray widths = "5555455545555545554445554";
    FiffEvents steps;
    QVERIFY(FiffEvents::detect_from_raw(*m_raw, steps, QStringLiteral("STI 014"), 0xFFFFFFFF, false));
    QCOMPARE(static_cast<int>(steps.events.rows()), 2 * static_cast<int>(onsets.rows()));
    for (int k = 0; k < onsets.rows(); ++k) {
        QVERIFY(steps.events.row(2 * k) == onsets.row(k));
        QCOMPARE(steps.events(2 * k + 1, 0), onsets(k, 0) + (widths[k] - '0'));
        QCOMPARE(steps.events(2 * k + 1, 1), onsets(k, 2));
        QCOMPARE(steps.events(2 * k + 1, 2), 0);
    }

    // find_events(mask=3, mask_type='and'): codes 4 and 32 vanish, 5 becomes 1.
    FiffEvents masked;
    QVERIFY(FiffEvents::detect_from_raw(*m_raw, masked, QString(), 3));
    const QList<int> maskedCodes{2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 1, 2, 3, 1, 2, 3, 1};
    QCOMPARE(static_cast<int>(masked.events.rows()), static_cast<int>(maskedCodes.size()));
    for (int k = 0; k < maskedCodes.size(); ++k)
        QCOMPARE(masked.events(k, 2), maskedCodes[k]);

    FiffEvents none;
    QVERIFY(!FiffEvents::detect_from_raw(*m_raw, none, QStringLiteral("STI 999")));

    // FIF and ASCII round trips; read() derives <raw>-eve.fif from the raw file name.
    QTemporaryDir dir;
    const QString fifPath = dir.filePath("run-eve.fif");
    {
        QFile file(fifPath);
        QVERIFY(found.write_to_fif(file));
    }
    QVERIFY(!FiffEvents().write_to_fif(*m_rawFile));
    FiffEvents back;
    QVERIFY(FiffEvents::read(QString(), dir.filePath("run.fif"), back));
    QVERIFY(back.events == onsets);
    FiffEvents byName;
    QVERIFY(FiffEvents::read(fifPath, QString(), byName));
    QVERIFY(byName.events == onsets);
    QVERIFY(!FiffEvents::read(QString(), dir.filePath("run.raw"), byName));
    QVERIFY(!FiffEvents::read(dir.filePath("missing-eve.fif"), QString(), byName));
    QVERIFY(!FiffEvents::read(QString(), dir.filePath("missing.fif"), byName));
    FiffEvents noBlock;
    QFile rawAgain(dataPath("sample_audvis_trunc_raw.fif"));
    QVERIFY(!FiffEvents::read_from_fif(rawAgain, noBlock));

    const QString evePath = dir.filePath("run.eve");
    {
        QFile file(evePath);
        QVERIFY(found.write_to_ascii(file, static_cast<float>(m_raw->info.sfreq)));
    }
    QFile eveFile(evePath);
    const FiffEvents ascii(eveFile);
    QVERIFY(ascii.events == onsets);
}

//=============================================================================================================

void TestFiffCorePython::sparse_createAndConvert()
{
    // create_sparse_rcs: row 0 -> (0, 2), row 1 -> (1)
    int nnz[2] = {2, 1};
    int row0[2] = {0, 2};
    int row1[1] = {1};
    float val0[2] = {1.5f, -2.0f};
    float val1[1] = {4.0f};
    int* cols[2] = {row0, row1};
    float* vals[2] = {val0, val1};

    const FiffSparseMatrix::UPtr m = FiffSparseMatrix::create_sparse_rcs(2, 3, nnz, cols, vals);
    QVERIFY(m != nullptr);
    QCOMPARE(m->nonZeros(), 3);
    const MatrixXf dense = MatrixXf(m->eigen());
    QCOMPARE(dense(0, 2), -2.0f);
    QCOMPARE(dense(1, 1), 4.0f);

    // Column out of range, and no non-zeros at all.
    int badRow[1] = {3};
    int* badCols[2] = {row0, badRow};
    QVERIFY(FiffSparseMatrix::create_sparse_rcs(2, 3, nnz, badCols, vals) == nullptr);
    int none[2] = {0, 0};
    QVERIFY(FiffSparseMatrix::create_sparse_rcs(2, 3, none, cols, vals) == nullptr);

    // Upper triangle only makes sense for a square matrix.
    QVERIFY(m->mne_add_upper_triangle_rcs() == nullptr);

    SparseMatrix<double> d(3, 3);
    d.insert(2, 0) = 0.25;
    d.insert(1, 1) = 3.0;
    FiffSparseMatrix fromD = FiffSparseMatrix::fromEigenSparse(d);
    QCOMPARE(fromD.nonZeros(), 2);
    QCOMPARE(fromD.coding, FIFFTS_MC_RCS);
    QCOMPARE(MatrixXf(fromD.eigen())(2, 0), 0.25f);

    // The full symmetric matrix keeps the diagonal once.
    const FiffSparseMatrix::UPtr full = fromD.mne_add_upper_triangle_rcs();
    QVERIFY(full != nullptr);
    const MatrixXf f = MatrixXf(full->eigen());
    QCOMPARE(f(0, 2), 0.25f);
    QCOMPARE(f(2, 0), 0.25f);
    QCOMPARE(f(1, 1), 3.0f);

    SparseMatrix<float> s(2, 2);
    s.insert(0, 1) = 7.0f;
    QCOMPARE(MatrixXf(FiffSparseMatrix::fromEigenSparse(s).eigen())(0, 1), 7.0f);

    QVERIFY(FiffSparseMatrix::fromEigenSparse(SparseMatrix<double>(4, 4)).is_empty());
    QVERIFY(FiffSparseMatrix::fromEigenSparse(SparseMatrix<float>(4, 4)).is_empty());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCorePython)
#include "test_fiff_core_python.moc"
