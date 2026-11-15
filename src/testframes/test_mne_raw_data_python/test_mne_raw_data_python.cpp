//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_raw_data_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates MNERawData buffered reading and SSP projection against mne-python.
 *
 * MNERawData is the buffered raw reader behind the MNE-C style tools (raw
 * dipole fits, mne_browse_raw ports). It calibrates, picks across buffer
 * boundaries, pads outside the data and projects on the fly. Expected values
 * come from mne-python on sample_audvis_trunc_raw.fif (first_samp 12900,
 * 6007 samples, 376 channels):
 *
 *   d = read_raw_fif(f, preload=True).get_data()                 # calibrated
 *   d[picks, s0:s0+n] for (s0, n) in (0, 5), (2990, 25), (6000, 7)
 *   P = make_projector(info['projs'], ch_names, bads=[])[0]; P @ d[:, 2990:3015]
 *
 * with picks = MEG 0113, MEG 0112, MEG 0111, EEG 001, STI 014, EOG 061.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include <mne/mne_raw_data.h>
#include <base/string_utils.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_ch_selection.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_deriv.h>
#include <mne/mne_deriv_set.h>
#include <mne/mne_sparse_named_matrix.h>

#include <fiff/fiff_sparse_matrix.h>

#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

namespace
{


//=============================================================================================================
/**
 * A channels x samples buffer prefilled with a sentinel, so samples MNERawData leaves untouched stand out.
 */
struct PickBuffer
{
    PickBuffer(int nchan, int ns)
    : values(nchan, ns)
    {
        values.setConstant(12345.0f);
    }

    MNERawData::RowMajorMatrixXf values;
};

bool closeTo(double actual, double expected, double rel)
{
    return std::abs(actual - expected) <= rel * std::abs(expected);
}

} // namespace

//=============================================================================================================
/**
 * Cross validates MNERawData against mne-python.
 */
class TestMneRawDataPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readsLayout();
    void picksSegments_data();
    void picksSegments();
    void padsOutsideData();
    void projectsSegment();
    void filterIsTransparentWhenOff();
    void filtersSegments_data();
    void filtersSegments();
    void rejectsMissingFile();
    void picksDerivedChannel();

private:
    MNEChSelection makeSelection() const;

    QString m_rawPath;
    std::unique_ptr<MNERawData> m_raw;
};

//=============================================================================================================

void TestMneRawDataPython::initTestCase()
{
    m_rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    QVERIFY2(QFile::exists(m_rawPath), "test data missing");
    m_raw.reset(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, MNEFilterDef()));
    QVERIFY(m_raw);
}

//=============================================================================================================

MNEChSelection TestMneRawDataPython::makeSelection() const
{
    const StringList picks{"MEG 0113", "MEG 0112", "MEG 0111", "EEG 001", "STI 014", "EOG 061"};
    MNEChSelection sel;
    sel.name = "picks";
    sel.chspick = picks;
    sel.chspick_nospace = picks;
    sel.pick = VectorXi::Constant(sel.nchan(), -1);
    sel.pick_deriv = VectorXi::Constant(sel.nchan(), -1);
    for (int c = 0; c < sel.nchan(); ++c) {
        std::erase(sel.chspick_nospace[c], ' ');
        sel.pick[c] = static_cast<int>(mne::index_of(m_raw->ch_names, sel.chspick[c]));
    }
    return sel;
}

//=============================================================================================================

void TestMneRawDataPython::readsLayout()
{
    QCOMPARE(m_raw->first_samp, 12900);
    QCOMPARE(m_raw->nsamp, 6007);
    QCOMPARE(m_raw->info->nchan, 376);
    QVERIFY(std::abs(m_raw->info->sfreq - 300.3074951171875) < 1e-4);
    QCOMPARE(m_raw->badlist, (StringList{"MEG 2443", "EEG 053"}));
    QCOMPARE(m_raw->bad.sum(), 2);
    {
        mne::LogCapture log;
        std::unique_ptr<MNERawData> again(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, MNEFilterDef()));
        QVERIFY(again);
        QCOMPARE(log.count(mne::Log::Level::Info, ": MEG 2443 EEG 053"), 1);
    }
    QCOMPARE(makeSelection().pick.minCoeff() >= 0, true);

    // Buffers tile the recording without gaps.
    int expected = m_raw->first_samp;
    int total = 0;
    for (const MNERawBufDef& buf : m_raw->bufs) {
        QCOMPARE(buf.firsts, expected);
        expected = buf.lasts + 1;
        total += buf.ns;
    }
    QCOMPARE(total, m_raw->nsamp);
    QVERIFY(m_raw->bufs.size() > 1);
}

//=============================================================================================================

void TestMneRawDataPython::picksSegments_data()
{
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("ns");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("eegFirst");
    QTest::addColumn<double>("eogLast");

    QTest::newRow("start") << 0 << 5 << 1.9254763827887736e-08 << 4.491140926785156e-10 << -1.8442066106497779e-10;
    QTest::newRow("across buffers") << 2990 << 25 << 1.8063671136130195e-07 << -1.0122332562412516e-09 << -4.235911209260794e-09;
    QTest::newRow("last samples") << 6000 << 7 << 3.362881495091471e-08 << -3.0773814992155044e-09 << -4.98549522435683e-09;
}

void TestMneRawDataPython::picksSegments()
{
    QFETCH(int, offset);
    QFETCH(int, ns);
    QFETCH(double, absSum);
    QFETCH(double, eegFirst);
    QFETCH(double, eogLast);

    MNEChSelection sel = makeSelection();
    PickBuffer buf(sel.nchan(), ns);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + offset, buf.values), 0);

    QVERIFY2(closeTo(buf.values.cast<double>().cwiseAbs().sum(), absSum, 1e-5),
             qPrintable(QStringLiteral("abs sum %1").arg(buf.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    QVERIFY(closeTo(buf.values(3, 0), eegFirst, 1e-5));
    QVERIFY(closeTo(buf.values(5, ns - 1), eogLast, 1e-5));

    // The same data without a selection, every channel in file order.
    PickBuffer all(m_raw->info->nchan, ns);
    QCOMPARE(m_raw->pick_data(nullptr, m_raw->first_samp + offset, all.values), 0);
    for (int c = 0; c < sel.nchan(); ++c)
        QCOMPARE(all.values.row(sel.pick[c]), buf.values.row(c));
}

//=============================================================================================================

void TestMneRawDataPython::padsOutsideData()
{
    // Before the first sample the data are zero; past the end the last sample repeats.
    MNEChSelection sel = makeSelection();
    PickBuffer before(sel.nchan(), 8);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp - 3, before.values), 0);
    QVERIFY((before.values.leftCols(3).array() == 0.0f).all());
    QVERIFY(closeTo(before.values(3, 3), 4.491140926785156e-10, 1e-5));

    PickBuffer after(sel.nchan(), 10);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + 6000, after.values), 0);
    for (int s = 7; s < 10; ++s)
        QCOMPARE(after.values.col(s), after.values.col(6));
    PickBuffer allAfter(m_raw->info->nchan, 10);
    QCOMPARE(m_raw->pick_data(nullptr, m_raw->first_samp + 6000, allAfter.values), 0);
    for (int s = 7; s < 10; ++s)
        QCOMPARE(allAfter.values.col(s), allAfter.values.col(6));
    // A request that starts past the end has no sample to repeat
    PickBuffer beyond(sel.nchan(), 4);
    beyond.values.setConstant(1.0f);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + 7000, beyond.values), 0);
    QVERIFY((beyond.values.array() == 0.0f).all());

    // A buffer whose row count does not match the selection is refused, not overrun
    for (const int nrow : {sel.nchan() - 1, sel.nchan() + 1}) {
        PickBuffer wrong(nrow, 4);
        mne::LogCapture log;
        QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp, wrong.values), -1);
        QCOMPARE(log.count(mne::Log::Level::Warning, "rows"), 1);
        QVERIFY((wrong.values.array() == 12345.0f).all());
    }

    // Projected reads pad the same way
    auto made = MNEProjOp::make_projection({m_rawPath.toStdU16String()}, m_raw->info->chInfo);
    QVERIFY(made && *made);
    std::unique_ptr<MNEProjOp> proj = std::move(*made);
    proj->assign_channels(m_raw->ch_names);
    QVERIFY(proj->make_proj());
    m_raw->proj = std::move(proj);
    for (MNEChSelection* picks : {&sel, static_cast<MNEChSelection*>(nullptr)}) {
        const int nchan = picks ? sel.nchan() : m_raw->info->nchan;
        PickBuffer projected(nchan, 10);
        QCOMPARE(m_raw->pick_data_proj(picks, m_raw->first_samp + 6000, projected.values), 0);
        for (int s = 7; s < 10; ++s)
            QCOMPARE(projected.values.col(s), projected.values.col(6));
        PickBuffer projectedBeyond(nchan, 4);
        projectedBeyond.values.setConstant(1.0f);
        QCOMPARE(m_raw->pick_data_proj(picks, m_raw->first_samp + 7000, projectedBeyond.values), 0);
        QVERIFY((projectedBeyond.values.array() == 0.0f).all());
    }
    m_raw->proj.reset();

    // A trigger channel is read uncalibrated: the first event (sample 13988) has code 2.
    PickBuffer stim(sel.nchan(), 3);
    QCOMPARE(m_raw->pick_data(&sel, 13987, stim.values), 0);
    QCOMPARE(stim.values(4, 0), 0.0f);
    QCOMPARE(stim.values(4, 1), 2.0f);
    QCOMPARE(stim.values(4, 2), 2.0f);
}

//=============================================================================================================

void TestMneRawDataPython::projectsSegment()
{
    MNEChSelection sel = makeSelection();

    // Without an operator the projected read is the plain read.
    PickBuffer plain(sel.nchan(), 25);
    QCOMPARE(m_raw->pick_data_proj(&sel, m_raw->first_samp + 2990, plain.values), 0);
    QVERIFY(closeTo(plain.values.cast<double>().cwiseAbs().sum(), 1.8063671136130195e-07, 1e-5));

    // The file stores its projectors as inactive; make_projection switches them on, as MNE-C does.
    std::unique_ptr<MNEProjOp> idle = MNEProjOp::read(m_rawPath.toStdU16String());
    QVERIFY(idle);
    QCOMPARE(idle->items.size(), std::size_t{4});
    QCOMPARE(idle->items[0].active, false);
    auto made = MNEProjOp::make_projection({m_rawPath.toStdU16String()}, m_raw->info->chInfo);
    QVERIFY(made && *made);
    std::unique_ptr<MNEProjOp> proj = std::move(*made);
    QCOMPARE(proj->items.size(), std::size_t{4});
    proj->assign_channels(m_raw->ch_names);
    QVERIFY(proj->make_proj());
    QCOMPARE(proj->nvec, 4);
    m_raw->proj = std::move(proj);

    PickBuffer buf(sel.nchan(), 25);
    QCOMPARE(m_raw->pick_data_proj(&sel, m_raw->first_samp + 2990, buf.values), 0);
    QVERIFY2(closeTo(buf.values.cast<double>().cwiseAbs().sum(), 1.8631954154371812e-07, 1e-4),
             qPrintable(QStringLiteral("projected abs sum %1").arg(buf.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    QVERIFY(closeTo(buf.values(2, 0), -1.4097316494010995e-16, 1e-3));
    QVERIFY(closeTo(buf.values(3, 0), -4.107782047805238e-10, 1e-4));
    QCOMPARE(buf.values(4, 0), 0.0f);

    PickBuffer all(m_raw->info->nchan, 25);
    QCOMPARE(m_raw->pick_data_proj(nullptr, m_raw->first_samp + 2990, all.values), 0);
    QVERIFY2(closeTo(all.values.cast<double>().cwiseAbs().sum(), 4.027555929286992e-06, 1e-4),
             qPrintable(QStringLiteral("all projected %1").arg(all.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    m_raw->proj.reset();

    // Without projection files EEG channels still get MNE-C's average reference; MEG alone needs nothing
    std::vector<FiffChInfo> eeg, meg, stim;
    for (const FiffChInfo& ch : std::as_const(m_raw->info->chInfo)) {
        (ch.kind == FIFFV_EEG_CH ? eeg : ch.kind == FIFFV_MEG_CH ? meg
                                                                 : stim)
            .push_back(ch);
    }
    made = MNEProjOp::make_projection({}, eeg);
    QVERIFY(made && *made);
    proj = std::move(*made);
    QCOMPARE(proj->items.size(), std::size_t{1});
    QCOMPARE(proj->items[0].kind, FIFFV_MNE_PROJ_ITEM_EEG_AVREF);
    QCOMPARE(proj->items[0].vecs.ncol(), static_cast<int>(eeg.size()));
    QVERIFY(std::fabs(proj->items[0].vecs.data(0, 0) - 1.0 / std::sqrt(static_cast<double>(eeg.size()))) < 1e-7);
    std::ostringstream out;
    proj->report_data(out, "#", true, {eeg[1].ch_name});
    const QString listing = QString::fromStdString(out.str());
    QVERIFY2(listing.startsWith(QStringLiteral("#\n## 1 : Average EEG reference : 1 vecs : %1 chs EEG active\n#\n").arg(eeg.size())), qPrintable(listing));
    // MNE-C mne_proj_op_report_data: names "%-10s", weights "%10.5g "; the excluded channel's weight is zero
    QVERIFY(listing.contains(QString::fromStdString(std::format("{:<10} {:<10} ", eeg[0].ch_name, eeg[1].ch_name))));
    const std::string weight = std::format("{:10.5g}  ", 1.0 / std::sqrt(static_cast<double>(eeg.size())));
    QVERIFY2(listing.contains(QString::fromStdString(weight + "         0  " + weight)), qPrintable(listing));
    made = MNEProjOp::make_projection({}, meg);
    QVERIFY(made && !*made);
    // Projectors that touch none of the channels are dropped, and an unreadable file fails
    made = MNEProjOp::make_projection({m_rawPath.toStdU16String()}, stim);
    QVERIFY(made && !*made);
    made = MNEProjOp::make_projection({(m_rawPath + QStringLiteral(".missing")).toStdU16String()}, meg);
    QVERIFY(!made);
    QCOMPARE(made.error().code, mne::ErrorCode::Io);

    // Bad channels are left out of the vectors before orthogonalisation (mne make_projector(projs, ch_names, bads)):
    // the average EEG reference then spans 59 channels, and with every EEG channel bad it disappears
    const auto projector = [&](const StringList& bads, int& nvec) {
        std::unique_ptr<MNEProjOp> op = std::move(MNEProjOp::make_projection({m_rawPath.toStdU16String()}, m_raw->info->chInfo).value());
        op->assign_channels(m_raw->ch_names);
        nvec = op->make_proj(bads) ? op->nvec : -1;
        const Eigen::MatrixXd u = op->proj_data.cast<double>();
        return Eigen::MatrixXd(Eigen::MatrixXd::Identity(u.cols(), u.cols()) - u.transpose() * u);
    };
    const int eeg1 = mne::index_of(m_raw->ch_names, "EEG 001");
    const int meg2443 = mne::index_of(m_raw->ch_names, "MEG 2443");
    QVERIFY(eeg1 >= 0 && meg2443 >= 0);
    int nvec = 0;
    Eigen::MatrixXd p = projector({}, nvec);
    QCOMPARE(nvec, 4);
    QVERIFY(std::fabs(p(eeg1, eeg1) - 0.9833333333333333) < 1e-6);
    p = projector({"MEG 2443", "EEG 053"}, nvec);
    QCOMPARE(nvec, 4);
    QVERIFY(std::fabs(p.trace() - 372.0) < 1e-4);
    QVERIFY2(std::fabs(p(eeg1, eeg1) - 0.9830508474576272) < 1e-6, qPrintable(QString::number(p(eeg1, eeg1), 'g', 10)));
    QVERIFY(std::fabs(p(meg2443, meg2443) - 1.0) < 1e-6);
    StringList allEeg;
    for (const std::string& name : m_raw->ch_names) {
        if (name.starts_with("EEG")) {
            allEeg.push_back(name);
        }
    }
    p = projector(allEeg, nvec);
    QCOMPARE(nvec, 3);
    QVERIFY(std::fabs(p.trace() - 373.0) < 1e-4);
    QVERIFY(std::fabs(p(eeg1, eeg1) - 1.0) < 1e-6);
}

//=============================================================================================================

void TestMneRawDataPython::filterIsTransparentWhenOff()
{
    // With the filter switched off pick_data_filt must return the raw data.
    MNEChSelection sel = makeSelection();
    PickBuffer raw(sel.nchan(), 25);
    PickBuffer filt(sel.nchan(), 25);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + 2990, raw.values), 0);
    QVERIFY(m_raw->filter);
    QVERIFY(!m_raw->filter->filter_on);
    QCOMPARE(m_raw->pick_data_filt(&sel, m_raw->first_samp + 2990, filt.values), 0);
    QCOMPARE(filt.values, raw.values);
}

//=============================================================================================================

void TestMneRawDataPython::filtersSegments_data()
{
    QTest::addColumn<double>("highpass");
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("ns");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("eegFirst");
    QTest::addColumn<double>("eogLast");

    // numpy port of MNE-C's overlap-add filter (mne_apply_filter.c, mne_raw_routines.c):
    // 4096-sample blocks with 2048-sample zero tapers, the first sample's value removed,
    // rfft * cos^2-edged response (lowpass 40 Hz, width 5 Hz), stim channels unfiltered.
    // Like the reader, it repeats the last sample past the end. absSum excludes the stim channel.
    QTest::newRow("lowpass, start") << 0.0 << 0 << 5 << 2.023919891690633e-08 << -1.5030550064406802e-09 << 6.185775627031829e-11;
    QTest::newRow("lowpass, across buffers") << 0.0 << 2990 << 25 << 1.7383534600305496e-07 << -1.064022285676407e-09 << -3.919694732307241e-09;
    QTest::newRow("lowpass, end") << 0.0 << 6000 << 7 << 3.491180168507228e-08 << -3.666942281729456e-09 << -4.807140128919311e-09;
    QTest::newRow("bandpass, start") << 1.0 << 0 << 5 << 1.5276783148546238e-08 << 2.396619036799164e-09 << -1.8021456675591414e-09;
    QTest::newRow("bandpass, across buffers") << 1.0 << 2990 << 25 << 5.940930901345105e-08 << 1.7552299025034232e-09 << 1.3481973317834655e-10;
    QTest::newRow("bandpass, end") << 1.0 << 6000 << 7 << 2.0515146096757774e-08 << -7.04241517427401e-10 << -1.2819215478145205e-09;
}

void TestMneRawDataPython::filtersSegments()
{
    QFETCH(double, highpass);
    QFETCH(int, offset);
    QFETCH(int, ns);
    QFETCH(double, absSum);
    QFETCH(double, eegFirst);
    QFETCH(double, eogLast);

    MNEFilterDef filter;
    filter.filter_on = true;
    filter.size = 4096;
    filter.taper_size = 2048;
    filter.highpass = filter.eog_highpass = static_cast<float>(highpass);
    filter.lowpass = filter.eog_lowpass = 40.0f;
    filter.lowpass_width = filter.eog_lowpass_width = 5.0f;
    std::unique_ptr<MNERawData> raw(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, filter));
    QVERIFY(raw);

    MNEChSelection sel = makeSelection();
    PickBuffer buf(sel.nchan(), ns);
    QCOMPARE(raw->pick_data_filt(&sel, raw->first_samp + offset, buf.values), 0);

    double sum = 0.0;
    for (int c = 0; c < sel.nchan(); ++c)
        if (c != 4)
            sum += buf.values.row(c).cast<double>().cwiseAbs().sum();
    QVERIFY2(closeTo(sum, absSum, 1e-4), qPrintable(QStringLiteral("abs sum %1").arg(sum, 0, 'g', 12)));
    QVERIFY2(closeTo(buf.values(3, 0), eegFirst, 1e-3), qPrintable(QStringLiteral("EEG %1").arg(buf.values(3, 0), 0, 'g', 12)));
    QVERIFY2(closeTo(buf.values(5, ns - 1), eogLast, 1e-3), qPrintable(QStringLiteral("EOG %1").arg(buf.values(5, ns - 1), 0, 'g', 12)));

    // The stimulus channel passes through unfiltered.
    PickBuffer plain(sel.nchan(), ns);
    QCOMPARE(raw->pick_data(&sel, raw->first_samp + offset, plain.values), 0);
    QCOMPARE(buf.values.row(4), plain.values.row(4));

    // Without a selection every channel is filtered, in file order; a fresh reader has no cached buffers.
    std::unique_ptr<MNERawData> fresh(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, filter));
    PickBuffer all(fresh->info->nchan, ns);
    QCOMPARE(fresh->pick_data_filt(nullptr, fresh->first_samp + offset, all.values), 0);
    for (int c = 0; c < sel.nchan(); ++c)
        QCOMPARE(all.values.row(sel.pick[c]), buf.values.row(c));

    // Switching to this filter from another one, after data was filtered with that, gives the same data
    MNEFilterDef other = filter;
    other.highpass = other.eog_highpass = static_cast<float>(1.0 - highpass);
    std::unique_ptr<MNERawData> switched(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, other));
    PickBuffer before(sel.nchan(), ns);
    QCOMPARE(switched->pick_data_filt(&sel, switched->first_samp + offset, before.values), 0);
    QVERIFY(before.values != buf.values);
    QVERIFY(switched->setFilter(filter));
    QVERIFY(!switched->setFilter(filter));
    PickBuffer after(sel.nchan(), ns);
    QCOMPARE(switched->pick_data_filt(&sel, switched->first_samp + offset, after.values), 0);
    QCOMPARE(after.values, buf.values);
}

//=============================================================================================================

void TestMneRawDataPython::rejectsMissingFile()
{
    std::unique_ptr<MNERawData> missing(MNERawData::open_file((m_rawPath + ".missing").toStdU16String(), false, false, MNEFilterDef()));
    QVERIFY(!missing);
}

//=============================================================================================================

void TestMneRawDataPython::picksDerivedChannel()
{
    // A bipolar derivation MEG0111 - MEG0121: every read path is linear, so it must equal the difference of the reads.
    MNEFilterDef filter;
    filter.filter_on = true;
    filter.size = 4096;
    filter.taper_size = 2048;
    filter.lowpass = filter.eog_lowpass = 40.0f;
    filter.lowpass_width = filter.eog_lowpass_width = 5.0f;
    std::unique_ptr<MNERawData> raw(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, filter));
    QVERIFY(raw);
    const int a = static_cast<int>(mne::index_of(raw->ch_names, "MEG 0111"));
    const int b = static_cast<int>(mne::index_of(raw->ch_names, "MEG 0121"));
    // A derivation set with one usable row and one whose input is not recorded.
    const MNEDerivSet set = MNEDerivSet::fromDefinitions({{"BIP", {{"MEG 0111", 1.0}, {"MEG 0121", -1.0}}}, {"MISSING", {{"MEG 0111", 1.0}, {"EEG 999", -1.0}}}});
    std::unique_ptr<MNEDeriv> deriv = set.match(raw->ch_names);
    QVERIFY(deriv);
    // Matching keeps the usable row, takes the data's channel order and counts per channel how many rows use it.
    QCOMPARE(deriv->deriv_data->rowlist, StringList({"BIP"}));
    QCOMPARE(deriv->deriv_data->collist, raw->ch_names);
    VectorXi inUse = VectorXi::Zero(raw->info->nchan);
    inUse(a) = inUse(b) = 1;
    QCOMPARE(deriv->in_use, inUse);
    QVERIFY(!MNEDerivSet::fromDefinitions({{"MISSING", {{"EEG 999", 1.0}}}}).match(raw->ch_names));
    // Attaching matches and validates in one step; both inputs are magnetometers, which the projectors act on, so BIP is valid.
    QCOMPARE(raw->attachDerivations(set), 1);
    QCOMPARE(raw->deriv_matched->in_use, inUse);
    QCOMPARE(raw->deriv_matched->chs[0].ch_name, QString("MEG 0111"));

    // Channel 0 is derived, 1 and 2 are its inputs.
    MNEChSelection sel;
    sel.pick = VectorXi(3);
    sel.pick << -1, a, b;
    sel.pick_deriv = VectorXi(3);
    sel.pick_deriv << 0, -1, -1;
    sel.chspick = sel.chspick_nospace = {"BIP", "MEG 0111", "MEG 0121"};

    // The projected path derives from projected inputs; the read starts before the data to cover the zero padding
    auto made = MNEProjOp::make_projection({m_rawPath.toStdU16String()}, raw->info->chInfo);
    QVERIFY(made && *made);
    std::unique_ptr<MNEProjOp> proj = std::move(*made);
    proj->assign_channels(raw->ch_names);
    QVERIFY(proj->make_proj());

    const int first = raw->first_samp + 2990;
    PickBuffer unprojected(3, 25);
    for (int path = 0; path < 3; ++path) {
        PickBuffer buf(3, 25);
        if (path == 0) {
            QCOMPARE(raw->pick_data(&sel, first, buf.values), 0);
            unprojected.values = buf.values;
        } else if (path == 1) {
            raw->proj = std::move(proj);
            PickBuffer padded(3, 28);
            QCOMPARE(raw->pick_data_proj(&sel, first - 3 - 2990, padded.values), 0);
            QVERIFY((padded.values.leftCols(3).array() == 0.0f).all());
            QVERIFY(padded.values.col(3).cwiseAbs().minCoeff() > 0.0f);
            QCOMPARE(raw->pick_data_proj(&sel, first, buf.values), 0);
            raw->proj.reset();
            QVERIFY(!buf.values.isApprox(unprojected.values));
        } else {
            QCOMPARE(raw->pick_data_filt(&sel, first, buf.values), 0);
        }
        const RowVectorXf expected = buf.values.row(1) - buf.values.row(2);
        QVERIFY2((buf.values.row(0) - expected).cwiseAbs().maxCoeff() <= 1e-5f * expected.cwiseAbs().maxCoeff(),
                 qPrintable(QStringLiteral("path %1").arg(path)));
        QVERIFY(expected.cwiseAbs().maxCoeff() > 0.0f);
    }

    // The derived channel alone: its inputs must still be filtered (a fresh reader has nothing cached).
    std::unique_ptr<MNERawData> fresh(MNERawData::open_file(m_rawPath.toStdU16String(), false, false, filter));
    fresh->deriv_matched = std::move(raw->deriv_matched);
    MNEChSelection only = sel;
    only.chspick.resize(1);
    only.chspick_nospace.resize(1);
    only.pick.conservativeResize(1);
    only.pick_deriv.conservativeResize(1);
    PickBuffer alone(1, 25);
    QCOMPARE(fresh->pick_data_filt(&only, first, alone.values), 0);
    PickBuffer inputs(3, 25);
    QCOMPARE(raw->pick_data_filt(&sel, first, inputs.values), 0);
    QVERIFY((alone.values.row(0) - (inputs.values.row(1) - inputs.values.row(2))).cwiseAbs().maxCoeff() <= 1e-5f * inputs.values.row(1).cwiseAbs().maxCoeff());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneRawDataPython)
#include "test_mne_raw_data_python.moc"
