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

#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_ch_selection.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_filter_def.h>

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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

namespace
{


//=============================================================================================================
/**
 * Owns a channels x samples buffer and the row pointers MNERawData writes through.
 */
struct PickBuffer
{
    PickBuffer(int nchan, int ns)
    : values(nchan, ns)
    , rows(nchan)
    {
        values.setConstant(12345.0f);
        for (int c = 0; c < nchan; ++c)
            rows[c] = values.row(c).data();
    }

    Matrix<float, Dynamic, Dynamic, RowMajor> values;
    std::vector<float*> rows;
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
    m_raw.reset(MNERawData::open_file(m_rawPath, false, false, MNEFilterDef()));
    QVERIFY(m_raw);
}

//=============================================================================================================

MNEChSelection TestMneRawDataPython::makeSelection() const
{
    const QStringList picks{"MEG 0113", "MEG 0112", "MEG 0111", "EEG 001", "STI 014", "EOG 061"};
    MNEChSelection sel;
    sel.name = "picks";
    sel.chspick = picks;
    sel.chspick_nospace = picks;
    sel.nchan = static_cast<int>(picks.size());
    sel.ndef = sel.nchan;
    sel.pick = VectorXi::Constant(sel.nchan, -1);
    sel.pick_deriv = VectorXi::Constant(sel.nchan, -1);
    // Channel names are stored without spaces ("MEG0113").
    for (int c = 0; c < sel.nchan; ++c) {
        sel.chspick_nospace[c] = QString(picks[c]).remove(' ');
        sel.pick[c] = static_cast<int>(m_raw->ch_names.indexOf(sel.chspick_nospace[c]));
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
    QCOMPARE(m_raw->nbad, 2);
    QCOMPARE(m_raw->bad.sum(), 2);
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
    PickBuffer buf(sel.nchan, ns);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + offset, ns, buf.rows.data()), 0);

    QVERIFY2(closeTo(buf.values.cast<double>().cwiseAbs().sum(), absSum, 1e-5),
             qPrintable(QStringLiteral("abs sum %1").arg(buf.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    QVERIFY(closeTo(buf.values(3, 0), eegFirst, 1e-5));
    QVERIFY(closeTo(buf.values(5, ns - 1), eogLast, 1e-5));

    // The same data without a selection, every channel in file order.
    PickBuffer all(m_raw->info->nchan, ns);
    QCOMPARE(m_raw->pick_data(nullptr, m_raw->first_samp + offset, ns, all.rows.data()), 0);
    for (int c = 0; c < sel.nchan; ++c)
        QCOMPARE(all.values.row(sel.pick[c]), buf.values.row(c));
}

//=============================================================================================================

void TestMneRawDataPython::padsOutsideData()
{
    // Before the first sample the data are zero; past the end the last sample repeats.
    MNEChSelection sel = makeSelection();
    PickBuffer before(sel.nchan, 8);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp - 3, 8, before.rows.data()), 0);
    QVERIFY(before.values.leftCols(3).isZero());
    QVERIFY(closeTo(before.values(3, 3), 4.491140926785156e-10, 1e-5));

    PickBuffer after(sel.nchan, 10);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + 6000, 10, after.rows.data()), 0);
    for (int s = 7; s < 10; ++s)
        QCOMPARE(after.values.col(s), after.values.col(6));

    // A trigger channel is read uncalibrated: the first event (sample 13988) has code 2.
    PickBuffer stim(sel.nchan, 3);
    QCOMPARE(m_raw->pick_data(&sel, 13987, 3, stim.rows.data()), 0);
    QCOMPARE(stim.values(4, 0), 0.0f);
    QCOMPARE(stim.values(4, 1), 2.0f);
    QCOMPARE(stim.values(4, 2), 2.0f);
}

//=============================================================================================================

void TestMneRawDataPython::projectsSegment()
{
    MNEChSelection sel = makeSelection();

    // Without an operator the projected read is the plain read.
    PickBuffer plain(sel.nchan, 25);
    QCOMPARE(m_raw->pick_data_proj(&sel, m_raw->first_samp + 2990, 25, plain.rows.data()), 0);
    QVERIFY(closeTo(plain.values.cast<double>().cwiseAbs().sum(), 1.8063671136130195e-07, 1e-5));

    // The file stores its projectors as inactive; makeProjection switches them on, as MNE-C does.
    std::unique_ptr<MNEProjOp> idle = MNEProjOp::read(m_rawPath);
    QVERIFY(idle);
    QCOMPARE(idle->nitems, 4);
    QCOMPARE(idle->items[0].active, false);
    std::unique_ptr<MNEProjOp> proj;
    QVERIFY(MNEProjOp::makeProjection({m_rawPath}, m_raw->info->chInfo, m_raw->info->nchan, proj));
    QVERIFY(proj);
    QCOMPARE(proj->nitems, 4);
    QCOMPARE(proj->assign_channels(m_raw->ch_names, m_raw->info->nchan), 0);
    QCOMPARE(proj->make_proj(), 0);
    QCOMPARE(proj->nvec, 4);
    m_raw->proj = std::move(proj);

    PickBuffer buf(sel.nchan, 25);
    QCOMPARE(m_raw->pick_data_proj(&sel, m_raw->first_samp + 2990, 25, buf.rows.data()), 0);
    QVERIFY2(closeTo(buf.values.cast<double>().cwiseAbs().sum(), 1.8631954154371812e-07, 1e-4),
             qPrintable(QStringLiteral("projected abs sum %1").arg(buf.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    QVERIFY(closeTo(buf.values(2, 0), -1.4097316494010995e-16, 1e-3));
    QVERIFY(closeTo(buf.values(3, 0), -4.107782047805238e-10, 1e-4));
    QCOMPARE(buf.values(4, 0), 0.0f);

    PickBuffer all(m_raw->info->nchan, 25);
    QCOMPARE(m_raw->pick_data_proj(nullptr, m_raw->first_samp + 2990, 25, all.rows.data()), 0);
    QVERIFY2(closeTo(all.values.cast<double>().cwiseAbs().sum(), 4.027555929286992e-06, 1e-4),
             qPrintable(QStringLiteral("all projected %1").arg(all.values.cast<double>().cwiseAbs().sum(), 0, 'g', 12)));
    m_raw->proj.reset();
}

//=============================================================================================================

void TestMneRawDataPython::filterIsTransparentWhenOff()
{
    // With the filter switched off pick_data_filt must return the raw data.
    MNEChSelection sel = makeSelection();
    PickBuffer raw(sel.nchan, 25);
    PickBuffer filt(sel.nchan, 25);
    QCOMPARE(m_raw->pick_data(&sel, m_raw->first_samp + 2990, 25, raw.rows.data()), 0);
    QVERIFY(m_raw->filter);
    QVERIFY(!m_raw->filter->filter_on);
    QCOMPARE(m_raw->pick_data_filt(&sel, m_raw->first_samp + 2990, 25, filt.rows.data()), 0);
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
    std::unique_ptr<MNERawData> raw(MNERawData::open_file(m_rawPath, false, false, filter));
    QVERIFY(raw);

    MNEChSelection sel = makeSelection();
    PickBuffer buf(sel.nchan, ns);
    QCOMPARE(raw->pick_data_filt(&sel, raw->first_samp + offset, ns, buf.rows.data()), 0);

    double sum = 0.0;
    for (int c = 0; c < sel.nchan; ++c)
        if (c != 4)
            sum += buf.values.row(c).cast<double>().cwiseAbs().sum();
    QVERIFY2(closeTo(sum, absSum, 1e-4), qPrintable(QStringLiteral("abs sum %1").arg(sum, 0, 'g', 12)));
    QVERIFY2(closeTo(buf.values(3, 0), eegFirst, 1e-3), qPrintable(QStringLiteral("EEG %1").arg(buf.values(3, 0), 0, 'g', 12)));
    QVERIFY2(closeTo(buf.values(5, ns - 1), eogLast, 1e-3), qPrintable(QStringLiteral("EOG %1").arg(buf.values(5, ns - 1), 0, 'g', 12)));

    // The stimulus channel passes through unfiltered.
    PickBuffer plain(sel.nchan, ns);
    QCOMPARE(raw->pick_data(&sel, raw->first_samp + offset, ns, plain.rows.data()), 0);
    QCOMPARE(buf.values.row(4), plain.values.row(4));
}

//=============================================================================================================

void TestMneRawDataPython::rejectsMissingFile()
{
    std::unique_ptr<MNERawData> missing(MNERawData::open_file(m_rawPath + ".missing", false, false, MNEFilterDef()));
    QVERIFY(!missing);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneRawDataPython)
#include "test_mne_raw_data_python.moc"
