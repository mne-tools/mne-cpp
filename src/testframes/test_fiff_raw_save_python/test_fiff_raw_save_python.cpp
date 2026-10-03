//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_raw_save_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates FiffRawData::save (sub-ranges, picks, decimation) against mne-python.
 *
 * FiffRawData::save writes a sub-range of a recording, optionally picking
 * channels and decimating. The saved files must read back with the right
 * first sample, sampling frequency, channel set and data. Expected values come
 * from mne-python on sample_audvis_trunc_raw.fif (first_samp 12900):
 *
 *   d = read_raw_fif(f, preload=True).get_data()
 *   seg = d[:, 1000:3000]                 # samples 13900 ... 15899
 *   np.abs(seg[data_picks]).sum(), np.abs(seg[:, ::3][data_picks]).sum()
 *
 * with data_picks = MEG 0113, MEG 0112, MEG 0111, EEG 001, EOG 061 and
 * STI 014 carried along as a trigger channel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * Cross validates FiffRawData::save against mne-python.
 */
class TestFiffRawSavePython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void savesSubRange_data();
    void savesSubRange();
    void rejectsInvalidRange();

private:
    RowVectorXi picks() const;

    QString m_rawPath;
    QTemporaryDir m_dir;
    QFile m_file; /**< FiffRawData reads lazily from this device. */
    FiffRawData m_raw;
};

//=============================================================================================================

void TestFiffRawSavePython::initTestCase()
{
    m_rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    QVERIFY2(QFile::exists(m_rawPath), "test data missing");
    QVERIFY(m_dir.isValid());
    m_file.setFileName(m_rawPath);
    m_raw = FiffRawData(m_file);
    QCOMPARE(m_raw.first_samp, 12900);
    QCOMPARE(m_raw.last_samp, 12900 + 6007 - 1);
}

//=============================================================================================================

RowVectorXi TestFiffRawSavePython::picks() const
{
    // FIFF readers store channel names without spaces.
    const QStringList names{"MEG0113", "MEG0112", "MEG0111", "EEG001", "STI014", "EOG061"};
    RowVectorXi sel(names.size());
    for (int k = 0; k < names.size(); ++k)
        sel[k] = static_cast<int>(m_raw.info.ch_names.indexOf(names[k]));
    return sel;
}

//=============================================================================================================

void TestFiffRawSavePython::savesSubRange_data()
{
    QTest::addColumn<int>("decim");
    QTest::addColumn<int>("nsamp");
    QTest::addColumn<double>("dataAbsSum");
    QTest::addColumn<int>("stimNonZero");

    QTest::newRow("full rate") << 1 << 2000 << 1.3197711899707521e-05 << -1;
    QTest::newRow("decim 3") << 3 << 667 << 4.401910028848629e-06 << 15;
}

void TestFiffRawSavePython::savesSubRange()
{
    QFETCH(int, decim);
    QFETCH(int, nsamp);
    QFETCH(double, dataAbsSum);
    QFETCH(int, stimNonZero);

    const RowVectorXi sel = picks();
    QVERIFY(sel.minCoeff() >= 0);
    const QString path = m_dir.filePath(QStringLiteral("save-decim%1-raw.fif").arg(decim));
    {
        QFile out(path);
        QVERIFY(m_raw.save(out, sel, decim, 13900, 15899));
    }

    QFile in(path);
    FiffRawData back(in);
    QCOMPARE(back.info.nchan, 6);
    QCOMPARE(back.info.ch_names[4], QString("STI014"));
    QCOMPARE(back.first_samp, decim == 1 ? 13900 : 13900 / decim);
    QCOMPARE(back.last_samp - back.first_samp + 1, nsamp);
    QVERIFY(std::abs(back.info.sfreq - 300.3074951171875 / decim) < 1e-3);

    MatrixXd data;
    MatrixXd times;
    QVERIFY(back.read_raw_segment(data, times, back.first_samp, back.last_samp));
    QCOMPARE(static_cast<int>(data.cols()), nsamp);

    double sum = 0.0;
    for (int c : {0, 1, 2, 3, 5})
        sum += data.row(c).cwiseAbs().sum();
    QVERIFY2(std::abs(sum - dataAbsSum) < 1e-5 * dataAbsSum, qPrintable(QStringLiteral("data abs sum %1").arg(sum, 0, 'g', 12)));

    // The trigger keeps its integer codes.
    QVERIFY((data.row(4).array() == data.row(4).array().round()).all());
    if (stimNonZero >= 0)
        QCOMPARE(static_cast<int>((data.row(4).array() != 0.0).count()), stimNonZero);

    // Times start at the first written sample.
    QVERIFY(std::abs(times(0, 0) - back.first_samp / back.info.sfreq) < 1e-6);
}

//=============================================================================================================

void TestFiffRawSavePython::rejectsInvalidRange()
{
    QFile out(m_dir.filePath("invalid-raw.fif"));
    QVERIFY(!m_raw.save(out, picks(), 1, 15000, 14000));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRawSavePython)
#include "test_fiff_raw_save_python.moc"
