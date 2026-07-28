//=============================================================================================================
/**
 * @file     test_fiff_proj_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     July, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Checks SSP projections and the projector against mne-python.
 *
 * A wrong projector does not crash anything, it quietly removes the wrong
 * signal subspace from every later result, so this is worth pinning to an
 * independent implementation rather than to MNE-CPP's own output.
 *
 * Reference values were produced with:
 *
 *   import mne
 *   from mne._fiff.proj import make_projector
 *   raw = mne.io.read_raw_fif('sample_audvis_trunc_raw.fif')
 *   for p in raw.info['projs']:
 *       print(p['desc'], p['kind'], p['data']['ncol'], p['data']['data'].sum())
 *   proj, nproj, U = make_projector(raw.info['projs'], raw.info['ch_names'],
 *                                   bads=raw.info['bads'])
 *   print(nproj, proj.shape, np.trace(proj), proj.sum())
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QtTest>

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
 * DECLARE CLASS TestFiffProjPython
 *
 * @brief Compares FiffProj against mne-python reference values.
 */
class TestFiffProjPython : public QObject
{
    Q_OBJECT

public:
    TestFiffProjPython() = default;

private:
    static QString rawPath();

    FiffInfo m_info;

private slots:
    void initTestCase();

    void projections_matchPython_data();
    void projections_matchPython();

    void projector_matchesPython();
    void projector_isIdempotentAndSymmetric();
    void projector_withoutProjsIsIdentity();
    void projector_ignoresInactiveProjections();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffProjPython::rawPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

void TestFiffProjPython::initTestCase()
{
    if(!QFile::exists(rawPath())) {
        QSKIP("Raw test data not found");
    }

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    m_info = raw.info;

    // mne-python reports four projections and 376 channels for this file.
    QCOMPARE(m_info.projs.size(), 4);
    QCOMPARE(m_info.ch_names.size(), 376);
}

//=============================================================================================================

void TestFiffProjPython::projections_matchPython_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("desc");
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("ncol");
    QTest::addColumn<double>("dataSum");

    // Straight from mne-python. The sum pins all 102 or 60 weights of each
    // vector, so a reader that dropped or reordered entries still fails.
    QTest::newRow("PCA-v1") << 0 << "PCA-v1"                << 1  << 102 <<  2.479997158;
    QTest::newRow("PCA-v2") << 1 << "PCA-v2"                << 1  << 102 << -7.375615597;
    QTest::newRow("PCA-v3") << 2 << "PCA-v3"                << 1  << 102 <<  1.150934458;
    QTest::newRow("EEG ref") << 3 << "Average EEG reference" << 10 << 60  <<  7.745965958;
}

//=============================================================================================================

void TestFiffProjPython::projections_matchPython()
{
    QFETCH(int, index);
    QFETCH(QString, desc);
    QFETCH(int, kind);
    QFETCH(int, ncol);
    QFETCH(double, dataSum);

    const FiffProj& p = m_info.projs.at(index);

    QCOMPARE(p.desc, desc);
    QCOMPARE(static_cast<int>(p.kind), kind);
    QVERIFY2(p.data, "projection carries no data");
    QCOMPARE(static_cast<int>(p.data->ncol), ncol);
    QCOMPARE(static_cast<int>(p.data->nrow), 1);

    QCOMPARE(static_cast<int>(p.data->data.cols()), ncol);
    QCOMPARE(static_cast<int>(p.data->data.rows()), 1);

    const double sum = p.data->data.sum();
    QVERIFY2(std::fabs(sum - dataSum) < 1.0e-6,
             qPrintable(QString("%1 weights sum to %2, mne-python says %3")
                        .arg(desc).arg(sum, 0, 'g', 12).arg(dataSum, 0, 'g', 12)));

    // The vector names have to line up with the weights, otherwise the
    // projector is applied to the wrong channels.
    QCOMPARE(p.data->col_names.size(), ncol);

    // Projections come out of the file inactive; activation is a later step.
    QCOMPARE(p.active, false);
}

//=============================================================================================================

void TestFiffProjPython::projector_matchesPython()
{
    // MNE-CPP and mne-python differ here, deliberately. FiffProj::make_projector
    // only counts projections whose active flag is set, while mne-python builds
    // the projector from every projection it is given regardless of the flag.
    // Projections come out of a file inactive, so MNE-CPP callers have to call
    // activate_projs first, which is what FiffEvoked does. Without it the
    // projector is the identity and the file's projections silently do nothing.
    FiffInfo info = m_info;
    FiffProj::activate_projs(info.projs);

    MatrixXd proj;
    const qint32 nproj = info.make_projector(proj);

    // mne-python: nproj 4, shape 376x376, trace 372, sum 255.125263745.
    QCOMPARE(static_cast<int>(nproj), 4);
    QCOMPARE(static_cast<int>(proj.rows()), 376);
    QCOMPARE(static_cast<int>(proj.cols()), 376);

    // The trace of a projector is its rank. Removing four vectors from 376
    // channels has to leave 372, so this single number confirms the projector
    // removes exactly as much as it should, no more and no less.
    const double trace = proj.trace();
    QVERIFY2(std::fabs(trace - 372.0) < 1.0e-6,
             qPrintable(QString("projector trace is %1, expected 372 (376 channels minus 4 projections)")
                        .arg(trace, 0, 'g', 12)));

    const double sum = proj.sum();
    QVERIFY2(std::fabs(sum - 255.125263745) < 1.0e-5,
             qPrintable(QString("projector sums to %1, mne-python says 255.125263745")
                        .arg(sum, 0, 'g', 12)));
}

//=============================================================================================================

void TestFiffProjPython::projector_isIdempotentAndSymmetric()
{
    FiffInfo info = m_info;
    FiffProj::activate_projs(info.projs);

    MatrixXd proj;
    QCOMPARE(static_cast<int>(info.make_projector(proj)), 4);

    // These are the defining properties of an orthogonal projector and they
    // hold whatever the data is, so they catch an error the reference numbers
    // above could only catch on this one file.
    const double idem = (proj * proj - proj).cwiseAbs().maxCoeff();
    QVERIFY2(idem < 1.0e-10,
             qPrintable(QString("P*P differs from P by up to %1, projector is not idempotent").arg(idem)));

    const double asym = (proj - proj.transpose()).cwiseAbs().maxCoeff();
    QVERIFY2(asym < 1.0e-12,
             qPrintable(QString("P differs from its transpose by up to %1").arg(asym)));

    // Applying the projector twice must equal applying it once, on real data
    // rather than on the matrix alone.
    VectorXd v = VectorXd::LinSpaced(proj.cols(), -1.0, 1.0);
    const VectorXd once = proj * v;
    const VectorXd twice = proj * once;
    QVERIFY2((once - twice).cwiseAbs().maxCoeff() < 1.0e-10,
             "projecting twice differs from projecting once");
}

//=============================================================================================================

void TestFiffProjPython::projector_withoutProjsIsIdentity()
{
    FiffInfo info = m_info;
    info.projs.clear();

    MatrixXd proj;
    const qint32 nproj = info.make_projector(proj);

    // With nothing to remove the projector has to be the identity, otherwise
    // it would alter data it was never asked to touch.
    QCOMPARE(static_cast<int>(nproj), 0);
    QCOMPARE(static_cast<int>(proj.rows()), 376);

    const double err = (proj - MatrixXd::Identity(proj.rows(), proj.cols())).cwiseAbs().maxCoeff();
    QVERIFY2(err < 1.0e-12,
             qPrintable(QString("empty projector differs from identity by up to %1").arg(err)));
}

//=============================================================================================================

void TestFiffProjPython::projector_ignoresInactiveProjections()
{
    // Pinning the behaviour that differs from mne-python, so a future change
    // to it is a deliberate decision rather than an accident. As loaded, the
    // projections are inactive and MNE-CPP builds no projector from them.
    for(const FiffProj& p : m_info.projs) {
        QCOMPARE(p.active, false);
    }

    MatrixXd proj;
    FiffInfo info = m_info;
    QCOMPARE(static_cast<int>(info.make_projector(proj)), 0);

    // Activating them is what makes the projector real.
    FiffProj::activate_projs(info.projs);
    for(const FiffProj& p : info.projs) {
        QCOMPARE(p.active, true);
    }
    QCOMPARE(static_cast<int>(info.make_projector(proj)), 4);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffProjPython)
#include "test_fiff_proj_python.moc"
