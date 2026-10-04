//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_ctf_comp_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    CTF gradient compensation cross-validated against mne-python.
 *
 * data/ctf_grade{0,1,3}_raw.fif hold the same 36 channels x 21 samples of
 * mne-python's test_ctf_comp_raw.fif (7 MEG + 29 reference channels).
 * make_ctf_comp_fixtures.py wrote the grade-3 original and mne-python's
 * apply_gradient_compensation(0 | 1) results. Each test opens one file,
 * switches it to another grade in MNE-CPP and compares the data with the
 * file mne-python wrote at that grade. Both MNE-CPP paths are covered:
 * FiffRawData + MNE::setup_compensators (mne-python's make_compensator) and
 * MNERawData::open_file_comp (MNECTFCompDataSet, MNE-C's mne_ctf_comp.c).
 *
 * Errors are relative to the larger channel peak of input and output: going
 * to grade 3 cancels ~99% of the signal, so float32 rounding of the input
 * sets the floor. MNE-C compensates MEG channels only, whereas mne-python
 * also applies grade 1 to the reference rows of the grade-1 matrix (a
 * relative change of < 1e-4). The MNERawData cases therefore compare MEG
 * channels to mne-python and require the reference channels to stay as read,
 * and start only from files whose reference channels are uncompensated.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_constants.h>
#include <mne/mne.h>
#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_ctf_comp_data_set.h>
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

using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

namespace
{

constexpr int kNChan = 36;
constexpr int kNMeg = 7;
constexpr int kNSamp = 21;

QString fixture(int grade)
{
    return QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade%1_raw.fif").arg(grade);
}

//=============================================================================================================
/**
 * Largest per-channel error relative to the larger peak of expected and source.
 */
double relError(const MatrixXd& actual, const MatrixXd& expected, const MatrixXd& source)
{
    double worst = 0.0;
    for (int c = 0; c < expected.rows(); ++c) {
        const double peak = std::max(expected.row(c).cwiseAbs().maxCoeff(), source.row(c).cwiseAbs().maxCoeff());
        worst = std::max(worst, (actual.row(c) - expected.row(c)).cwiseAbs().maxCoeff() / peak);
    }
    return worst;
}

//=============================================================================================================
/**
 * Reads a whole fixture with FiffRawData, optionally switching its grade.
 */
MatrixXd readFiff(int fileGrade, int destGrade = -1)
{
    QFile file(fixture(fileGrade));
    FiffRawData raw(file);
    if (destGrade >= 0)
        MNE::setup_compensators(raw, destGrade, false);
    MatrixXd data, times;
    if (!raw.read_raw_segment(data, times))
        return MatrixXd();
    return data;
}

//=============================================================================================================
/**
 * Reads a whole fixture with MNERawData at the requested grade (-1 = as stored).
 */
MatrixXd readMne(int fileGrade, int destGrade)
{
    std::unique_ptr<MNERawData> raw(MNERawData::open_file_comp(fixture(fileGrade), false, false, MNEFilterDef(), destGrade));
    if (!raw)
        return MatrixXd();
    Matrix<float, Dynamic, Dynamic, RowMajor> values(raw->info->nchan, kNSamp);
    std::vector<float*> rows(raw->info->nchan);
    for (int c = 0; c < raw->info->nchan; ++c)
        rows[c] = values.row(c).data();
    if (raw->pick_data(nullptr, raw->first_samp, kNSamp, rows.data()) != 0)
        return MatrixXd();
    return values.cast<double>();
}

} // namespace

//=============================================================================================================
/**
 * Cross-validates CTF gradient compensation against mne-python.
 */
class TestMneCtfCompPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readsCompensationData();
    void fiffRawDataChangesGrade_data();
    void fiffRawDataChangesGrade();
    void mneRawDataChangesGrade_data();
    void mneRawDataChangesGrade();
    void mneRawDataRejectsMissingGrade();

private:
    MatrixXd m_reference[4];
};

//=============================================================================================================

void TestMneCtfCompPython::initTestCase()
{
    for (int grade : {0, 1, 3}) {
        m_reference[grade] = readFiff(grade);
        QCOMPARE(m_reference[grade].rows(), kNChan);
        QCOMPARE(m_reference[grade].cols(), kNSamp);
    }
    // The grade must matter, otherwise every comparison below is vacuous.
    QVERIFY(relError(m_reference[0].topRows(kNMeg), m_reference[3].topRows(kNMeg), m_reference[3].topRows(kNMeg)) > 0.5);
    QVERIFY(relError(m_reference[0].topRows(kNMeg), m_reference[1].topRows(kNMeg), m_reference[1].topRows(kNMeg)) > 1e-3);
    // Reference channels are never compensated.
    QCOMPARE(m_reference[0].bottomRows(kNChan - kNMeg), m_reference[3].bottomRows(kNChan - kNMeg));
}

//=============================================================================================================

void TestMneCtfCompPython::readsCompensationData()
{
    QFile file(fixture(3));
    FiffRawData raw(file);
    QCOMPARE(raw.info.get_current_comp(), 3);
    QCOMPARE(raw.info.comps.size(), 5);
    QCOMPARE(raw.info.comps[0].kind, 1);
    QCOMPARE(raw.info.comps[3].kind, 3);

    // mne-python: info["ctf_head_t"] and info["dev_ctf_t"] of the fixture.
    QCOMPARE(raw.info.ctf_head_t.from, FIFFV_MNE_COORD_CTF_HEAD);
    QCOMPARE(raw.info.ctf_head_t.to, FIFFV_COORD_HEAD);
    QCOMPARE(raw.info.dev_ctf_t.from, FIFFV_COORD_DEVICE);
    QCOMPARE(raw.info.dev_ctf_t.to, FIFFV_MNE_COORD_CTF_HEAD);
    Matrix4f devCtf;
    devCtf << 6.720469243e-02f, 9.886326147e-01f, 1.344953576e-01f, -4.123601733e-04f,
        -9.971782317e-01f, 6.203418125e-02f, 4.227717525e-02f, 1.184050301e-04f,
        3.345328769e-02f, -1.369570575e-01f, 9.900119895e-01f, 6.580017899e-02f,
        0.0f, 0.0f, 0.0f, 1.0f;
    QVERIFY((raw.info.dev_ctf_t.trans - devCtf).cwiseAbs().maxCoeff() < 1e-6f);
    QVERIFY((raw.info.dev_ctf_t.trans * raw.info.dev_ctf_t.invtrans - Matrix4f::Identity()).cwiseAbs().maxCoeff() < 1e-5f);

    auto set = MNECTFCompDataSet::read(fixture(3));
    QVERIFY(set);
    QCOMPARE(set->ncomp, 5);
    QCOMPARE(set->nch, kNChan);
    QCOMPARE(MNECTFCompDataSet::get_comp(set->chs, set->nch), 3);
    QCOMPARE(MNECTFCompDataSet::explain_comp(MNECTFCompDataSet::map_comp_kind(3)), QString("third order gradiometer"));
    // Both readers calibrate the stored matrix the same way.
    QVERIFY((set->comps[3]->data->data.cast<double>() - raw.info.comps[3].data->data).cwiseAbs().maxCoeff() <= 1e-6 * raw.info.comps[3].data->data.cwiseAbs().maxCoeff());
}

//=============================================================================================================

void TestMneCtfCompPython::fiffRawDataChangesGrade_data()
{
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::newRow("3 -> 0") << 3 << 0;
    QTest::newRow("3 -> 1") << 3 << 1;
    QTest::newRow("0 -> 3") << 0 << 3;
    QTest::newRow("1 -> 3") << 1 << 3;
    QTest::newRow("0 -> 1") << 0 << 1;
}

void TestMneCtfCompPython::fiffRawDataChangesGrade()
{
    QFETCH(int, from);
    QFETCH(int, to);
    const MatrixXd data = readFiff(from, to);
    QCOMPARE(data.rows(), kNChan);
    const double err = relError(data, m_reference[to], m_reference[from]);
    // Float32 file storage limits this to ~5e-7; the old (I + C1) inverse gave 1.3e-6.
    QVERIFY2(err < 1e-6, qPrintable(QString::number(err)));
}

//=============================================================================================================

void TestMneCtfCompPython::mneRawDataChangesGrade_data()
{
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::newRow("3 -> 0") << 3 << 0;
    QTest::newRow("3 -> 1") << 3 << 1;
    QTest::newRow("0 -> 3") << 0 << 3;
    QTest::newRow("0 -> 1") << 0 << 1;
    QTest::newRow("3 as stored") << 3 << -1;
}

void TestMneCtfCompPython::mneRawDataChangesGrade()
{
    QFETCH(int, from);
    QFETCH(int, to);
    const MatrixXd data = readMne(from, to);
    QCOMPARE(data.rows(), kNChan);
    const MatrixXd& expected = m_reference[to < 0 ? from : to];
    const double err = relError(data.topRows(kNMeg), expected.topRows(kNMeg), m_reference[from].topRows(kNMeg));
    QVERIFY2(err < 1e-5, qPrintable(QString::number(err)));
    const int nref = kNChan - kNMeg;
    const double refErr = relError(data.bottomRows(nref), m_reference[from].bottomRows(nref), m_reference[from].bottomRows(nref));
    QVERIFY2(refErr < 1e-6, qPrintable(QString::number(refErr)));
}

//=============================================================================================================

void TestMneCtfCompPython::mneRawDataRejectsMissingGrade()
{
    // The fixture carries grades 1, 2, 3 and two 4D-style kinds, but no 101 (4D comp 1).
    std::unique_ptr<MNERawData> raw(MNERawData::open_file_comp(fixture(3), false, false, MNEFilterDef(), 101));
    QVERIFY(!raw);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneCtfCompPython)
#include "test_mne_ctf_comp_python.moc"
