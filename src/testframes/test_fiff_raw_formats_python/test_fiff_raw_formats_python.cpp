//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_raw_formats_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Reads raw FIFF data stored as short, int, single and double against mne-python.
 *
 * data/raw_{short,int,single,double}_raw.fif hold 12 channels x 120 samples of
 * the sample raw file, written by mne-python in each storage format
 * (make_raw_format_fixtures.py). Each file is read whole and with a channel
 * selection, with and without SSP, and compared with mne-python's get_data()
 * on the same file (values printed by the fixture script).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <mne/mne.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>

#include <cmath>
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

using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

namespace
{

QString fixture(const QString& fmt)
{
    return QStringLiteral(MNE_RAW_FORMATS_DATA_DIR "/raw_%1_raw.fif").arg(fmt);
}

bool closeTo(double actual, double expected, double rel)
{
    return std::abs(actual - expected) <= rel * std::abs(expected);
}

} // namespace

//=============================================================================================================
/**
 * Reads raw data in every FIFF storage format.
 */
class TestFiffRawFormatsPython : public QObject
{
    Q_OBJECT

private slots:
    void readsFormat_data();
    void readsFormat();
    void readsAcquisitionSkip();
    void mneRawDataMatches_data();
    void mneRawDataMatches();
    void mneRawDataNormalisesChannels();
};

//=============================================================================================================

void TestFiffRawFormatsPython::readsFormat_data()
{
    QTest::addColumn<QString>("fmt");
    QTest::addColumn<double>("absSum");
    QTest::addColumn<double>("eeg7");
    QTest::addColumn<double>("projAbsSum");
    QTest::addColumn<double>("projMeg5");
    QTest::addColumn<double>("selSum");
    QTest::addColumn<double>("selProjSum");

    // mne-python: data channels 0-8 abs sum, EEG 001 sample 7, the same after apply_proj(),
    // MEG 0113 sample 5 projected, and samples 20-39 of (EOG 061, MEG 0113, EEG 001).
    QTest::newRow("short (DAU_PACK16)") << "short" << 3.7340294521826274e-06 << -8.299999966834837e-09 << 1.0532204256066103e-06 << -6.230000285419382e-15 << 2.28944088735189e-07 << 1.9395908887498217e-07;
    QTest::newRow("int") << "int" << 3.7340294521826274e-06 << -8.299999966834837e-09 << 1.0532204256066103e-06 << -6.230000285419382e-15 << 2.28944088735189e-07 << 1.9395908887498217e-07;
    QTest::newRow("single") << "single" << 3.734759701859785e-06 << -8.301816373077579e-09 << 1.0535215929356122e-06 << -6.230249308868291e-15 << 2.2902833386270715e-07 << 1.9403356743244809e-07;
    QTest::newRow("double") << "double" << 3.734759713188641e-06 << -8.301816550715775e-09 << 1.0535215955914416e-06 << -6.2302493568295474e-15 << 2.2902833419616957e-07 << 1.9403356787203518e-07;
}

void TestFiffRawFormatsPython::readsFormat()
{
    QFETCH(QString, fmt);
    QFETCH(double, absSum);
    QFETCH(double, eeg7);
    QFETCH(double, projAbsSum);
    QFETCH(double, projMeg5);
    QFETCH(double, selSum);
    QFETCH(double, selProjSum);

    QFile file(fixture(fmt));
    FiffRawData raw(file);
    QCOMPARE(raw.first_samp, 13900);
    QCOMPARE(raw.last_samp, 14019);
    QCOMPARE(raw.info.nchan, 12);

    MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    QCOMPARE(data.rows(), 12);
    QCOMPARE(data.cols(), 120);
    QVERIFY2(closeTo(data.topRows(9).cwiseAbs().sum(), absSum, 1e-6), qPrintable(QString::number(data.topRows(9).cwiseAbs().sum(), 'g', 12)));
    QVERIFY(closeTo(data(6, 7), eeg7, 1e-6));
    QCOMPARE(data.row(9).maxCoeff(), 2.0);
    QVERIFY(std::abs(times(0, 0) - 13900 / raw.info.sfreq) < 1e-6);

    // Channel selection in a different order, part of the record only.
    RowVectorXi sel(3);
    sel << 10, 0, 6;
    MatrixXd selData, selTimes;
    QVERIFY(raw.read_raw_segment(selData, selTimes, raw.first_samp + 20, raw.first_samp + 39, sel));
    QCOMPARE(selData.rows(), 3);
    QCOMPARE(selData.cols(), 20);
    QVERIFY2(closeTo(selData.cwiseAbs().sum(), selSum, 1e-6), qPrintable(QString::number(selData.cwiseAbs().sum(), 'g', 12)));
    QCOMPARE(selData.row(1), data.block(0, 20, 1, 20));

    // The time-based reader picks the same samples.
    MatrixXd timeData, timeTimes;
    QVERIFY(raw.read_raw_segment_times(timeData, timeTimes, (raw.first_samp + 20) / raw.info.sfreq, (raw.first_samp + 39) / raw.info.sfreq, sel));
    QCOMPARE(timeData, selData);

    // SSP as mne-python's apply_proj() (PCA-v1..3 and the average EEG reference).
    MNE::setup_compensators(raw, 0, true);
    QVERIFY(raw.proj.size() > 0);
    MatrixXd proj, projTimes;
    QVERIFY(raw.read_raw_segment(proj, projTimes));
    QVERIFY2(closeTo(proj.topRows(9).cwiseAbs().sum(), projAbsSum, 1e-5), qPrintable(QString::number(proj.topRows(9).cwiseAbs().sum(), 'g', 12)));
    QVERIFY2(closeTo(proj(0, 5), projMeg5, 1e-4), qPrintable(QString::number(proj(0, 5), 'g', 12)));
    MatrixXd selProj, selProjTimes;
    QVERIFY(raw.read_raw_segment(selProj, selProjTimes, raw.first_samp + 20, raw.first_samp + 39, sel));
    QVERIFY2(closeTo(selProj.cwiseAbs().sum(), selProjSum, 1e-5), qPrintable(QString::number(selProj.cwiseAbs().sum(), 'g', 12)));

    // Out-of-range requests fail cleanly.
    MatrixXd none, noneTimes;
    QVERIFY(!raw.read_raw_segment(none, noneTimes, raw.last_samp + 10, raw.last_samp + 20));
}

//=============================================================================================================

void TestFiffRawFormatsPython::readsAcquisitionSkip()
{
    // data/raw_skip_raw.fif (make_skip_fixture.py): ramp data whose samples 70-89 were written as a
    // two-buffer FIFF_DATA_SKIP; mne-python reads them as zeros.
    QFile file(fixture("skip"));
    FiffRawData raw(file);
    QCOMPARE(raw.first_samp, 50);
    QCOMPARE(raw.last_samp, 149);

    MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    QCOMPARE(data.cols(), 100);
    for (int s = 0; s < 100; ++s) {
        const double expected = (s >= 70 && s < 90) ? 0.0 : s * 1e-6;
        // Stored as float32.
        QVERIFY2(std::abs(data(0, s) - expected) < 1e-12 + 1e-7 * std::abs(expected), qPrintable(QString("sample %1: %2").arg(s).arg(data(0, s))));
    }
    QVERIFY(std::abs(data(1, 95) + 95e-6) < 1e-11);

    // A selection spanning the skip, starting and ending inside data buffers.
    RowVectorXi sel(2);
    sel << 2, 1;
    MatrixXd part, partTimes;
    QVERIFY(raw.read_raw_segment(part, partTimes, raw.first_samp + 65, raw.first_samp + 94, sel));
    QCOMPARE(part.rows(), 2);
    QCOMPARE(part.cols(), 30);
    QCOMPARE(part.row(1), data.block(1, 65, 1, 30));
    QVERIFY(std::abs(part(0, 0) - 7e-6) < 1e-12);
    QCOMPARE(part(0, 10), 0.0);
}

//=============================================================================================================

void TestFiffRawFormatsPython::mneRawDataMatches_data()
{
    QTest::addColumn<QString>("fmt");
    for (const char* fmt : {"short", "int", "single", "double", "skip"})
        QTest::newRow(fmt) << QString(fmt);
}

void TestFiffRawFormatsPython::mneRawDataMatches()
{
    // MNE-C's buffered reader must return what FiffRawData (checked against mne-python above) does.
    QFETCH(QString, fmt);
    QFile file(fixture(fmt));
    FiffRawData raw(file);
    MatrixXd ref, times;
    QVERIFY(raw.read_raw_segment(ref, times));

    std::unique_ptr<MNERawData> mne(MNERawData::open_file(fixture(fmt), false, false, MNEFilterDef()));
    QVERIFY(mne);
    QCOMPARE(mne->first_samp, raw.first_samp);
    const int nchan = mne->info->nchan;
    const int ns = static_cast<int>(ref.cols());
    Matrix<float, Dynamic, Dynamic, RowMajor> values(nchan, ns);
    std::vector<float*> rows(nchan);
    for (int c = 0; c < nchan; ++c)
        rows[c] = values.row(c).data();
    QCOMPARE(mne->pick_data(nullptr, mne->first_samp, ns, rows.data()), 0);
    const double scale = ref.cwiseAbs().maxCoeff();
    QVERIFY2((values.cast<double>() - ref).cwiseAbs().maxCoeff() <= 1e-6 * scale,
             qPrintable(QString::number((values.cast<double>() - ref).cwiseAbs().maxCoeff() / scale)));
}

//=============================================================================================================

void TestFiffRawFormatsPython::mneRawDataNormalisesChannels()
{
    // data/raw_unit_mul_raw.fif (make_unit_mul_fixture.py): EEG 002 has unit_mul -6 and STI 014 range 2.
    // As in MNE-C's mne_open_raw_data, unit_mul is folded into cal and the trigger range reset to 1.
    QFile file(fixture("unit_mul"));
    FiffRawData raw(file);
    MatrixXd ref, times;
    QVERIFY(raw.read_raw_segment(ref, times));

    std::unique_ptr<MNERawData> mne(MNERawData::open_file(fixture("unit_mul"), false, false, MNEFilterDef()));
    QVERIFY(mne);
    QCOMPARE(mne->info->chInfo[1].unit_mul, 0);
    QCOMPARE(mne->info->chInfo[2].range, 1.0f);
    Matrix<float, Dynamic, Dynamic, RowMajor> values(3, 20);
    std::vector<float*> rows = {values.row(0).data(), values.row(1).data(), values.row(2).data()};
    QCOMPARE(mne->pick_data(nullptr, mne->first_samp, 20, rows.data()), 0);
    QVERIFY((values.row(0).cast<double>() - ref.row(0)).cwiseAbs().maxCoeff() < 1e-12);
    QVERIFY((values.row(1).cast<double>() - 1e-6 * ref.row(1)).cwiseAbs().maxCoeff() < 1e-6 * 1e-6 * ref.row(1).cwiseAbs().maxCoeff());
    QVERIFY((values.row(2).cast<double>() - 0.5 * ref.row(2)).cwiseAbs().maxCoeff() < 1e-6);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRawFormatsPython)
#include "test_fiff_raw_formats_python.moc"
