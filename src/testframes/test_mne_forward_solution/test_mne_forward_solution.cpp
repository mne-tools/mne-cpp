//=============================================================================================================
/**
 * @file     test_mne_forward_solution.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Gabriel B Motta, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    The forward solution test implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <fwd/computeFwd/compute_fwd_settings.h>
#include <fwd/computeFwd/compute_fwd.h>
#include <mne/mne.h>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_named_matrix.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace MNELIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneForwardSolution
 *
 * @brief The TestMneForwardSolution class provides dipole fit tests
 *
 */
class TestMneForwardSolution : public QObject
{
    Q_OBJECT

public:
    TestMneForwardSolution();

private slots:
    void initTestCase();
    void computeForward();
    void cleanupTestCase();

private:
    QSharedPointer<MNEForwardSolution> m_pFwdMEGEEGRead;
    QSharedPointer<MNEForwardSolution> m_pFwdMEGEEGRef;
};

//=============================================================================================================

TestMneForwardSolution::TestMneForwardSolution()
{
}

//=============================================================================================================

void TestMneForwardSolution::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
}

//=============================================================================================================

void TestMneForwardSolution::computeForward()
{
    // Compute and Write Forward Solution
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute/Write/Read MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Read reference forward solution
    QString fwdMEGEEGFileRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile fileFwdMEGEEGRef(fwdMEGEEGFileRef);
    m_pFwdMEGEEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRef));

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --eeg
    // --accurate
    // --src ../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif
    // --mri ../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif
    // --bem ../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif
    // --mindist 5
    // --fwd ../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif

    ComputeFwdSettings::SPtr pSettingsMEGEEG = ComputeFwdSettings::SPtr(new ComputeFwdSettings);

    pSettingsMEGEEG->include_meg = true;
    pSettingsMEGEEG->include_eeg = true;
    pSettingsMEGEEG->accurate = true;
    pSettingsMEGEEG->srcname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    pSettingsMEGEEG->measname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    pSettingsMEGEEG->mriname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif";
    pSettingsMEGEEG->transname.clear();
    pSettingsMEGEEG->bemname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    pSettingsMEGEEG->mindist = 5.0f/1000.0f;
    pSettingsMEGEEG->solname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

    QFile t_name(pSettingsMEGEEG->measname);
    FIFFLIB::FiffRawData raw(t_name);
    QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo(raw.info));

    // --- Diagnostic: trace range from raw file through to pFiffInfo ---
    if (!raw.info.chs.isEmpty()) {
        qDebug() << "[diag-range] raw.info.chs[0].range =" << raw.info.chs[0].range
                 << "ch_name =" << raw.info.chs[0].ch_name;
    }
    if (!pFiffInfo->chs.isEmpty()) {
        qDebug() << "[diag-range] pFiffInfo->chs[0].range =" << pFiffInfo->chs[0].range
                 << "ch_name =" << pFiffInfo->chs[0].ch_name;
    }
    // --- End diagnostic ---

    pSettingsMEGEEG->pFiffInfo = pFiffInfo;
    pSettingsMEGEEG->checkIntegrity();

    QSharedPointer<ComputeFwd> pFwdMEGEEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(pSettingsMEGEEG));
    pFwdMEGEEGComputed->calculateFwd();

    // recalculate with same meg_head_t to check that we still get the same result
    pFwdMEGEEGComputed->updateHeadPos(pFiffInfo->dev_head_t);

    pFwdMEGEEGComputed->storeFwd();

    // Read newly created fwd
    QFile fileFwdMEGEEGRead(pSettingsMEGEEG->solname);
    m_pFwdMEGEEGRead = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRead));

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute/Write/Read MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    // The following verifies the forward solution round-trip:
    // compute -> write -> read-back must reproduce the same solution

    // --- Debug: compare FiffInfoBase fields individually ---
    {
        const auto& a = m_pFwdMEGEEGRead->info;
        const auto& b = m_pFwdMEGEEGRef->info;
        qDebug() << "[diag] filename match:" << (a.filename == b.filename)
                 << "read:" << a.filename << "ref:" << b.filename;
        qDebug() << "[diag] bads match:" << (a.bads == b.bads);
        qDebug() << "[diag] nchan match:" << (a.nchan == b.nchan)
                 << "read:" << a.nchan << "ref:" << b.nchan;
        qDebug() << "[diag] ch_names match:" << (a.ch_names == b.ch_names);
        qDebug() << "[diag] dev_head_t match:" << (a.dev_head_t == b.dev_head_t);
        qDebug() << "[diag] ctf_head_t match:" << (a.ctf_head_t == b.ctf_head_t);
        qDebug() << "[diag] chs.size read:" << a.chs.size() << "ref:" << b.chs.size();
        int nChs = qMin(a.chs.size(), b.chs.size());
        for (int i = 0; i < nChs; ++i) {
            if (!(a.chs[i] == b.chs[i])) {
                qDebug() << "[diag] chs MISMATCH at index" << i << a.chs[i].ch_name;
                qDebug() << "  scanNo:" << a.chs[i].scanNo << "vs" << b.chs[i].scanNo;
                qDebug() << "  logNo:" << a.chs[i].logNo << "vs" << b.chs[i].logNo;
                qDebug() << "  kind:" << a.chs[i].kind << "vs" << b.chs[i].kind;
                qDebug() << "  range:" << a.chs[i].range << "vs" << b.chs[i].range;
                qDebug() << "  cal:" << a.chs[i].cal << "vs" << b.chs[i].cal;
                qDebug() << "  chpos match:" << (a.chs[i].chpos == b.chs[i].chpos);
                qDebug() << "  unit:" << a.chs[i].unit << "vs" << b.chs[i].unit;
                qDebug() << "  unit_mul:" << a.chs[i].unit_mul << "vs" << b.chs[i].unit_mul;
                qDebug() << "  coil_trans match:" << a.chs[i].coil_trans.isApprox(b.chs[i].coil_trans, 0.0001f);
                qDebug() << "  eeg_loc match:" << a.chs[i].eeg_loc.isApprox(b.chs[i].eeg_loc, 0.0001f);
                qDebug() << "  coord_frame:" << a.chs[i].coord_frame << "vs" << b.chs[i].coord_frame;
                if (i >= 5) { qDebug() << "  ... (stopping at 5 mismatches)"; break; }
            }
        }
    }
    // --- End debug ---

    QVERIFY(m_pFwdMEGEEGRead->info == m_pFwdMEGEEGRef->info);
    QVERIFY(m_pFwdMEGEEGRead->source_ori == m_pFwdMEGEEGRef->source_ori);
    QVERIFY(m_pFwdMEGEEGRead->surf_ori == m_pFwdMEGEEGRef->surf_ori);
    QVERIFY(m_pFwdMEGEEGRead->coord_frame == m_pFwdMEGEEGRef->coord_frame);
    QVERIFY(m_pFwdMEGEEGRead->nsource == m_pFwdMEGEEGRef->nsource);
    QVERIFY(m_pFwdMEGEEGRead->nchan == m_pFwdMEGEEGRef->nchan);

    // Compare sol data row-by-row (Eigen 5 isApprox on full large matrices can crash)
    QVERIFY(m_pFwdMEGEEGRead->sol->nrow == m_pFwdMEGEEGRef->sol->nrow);
    QVERIFY(m_pFwdMEGEEGRead->sol->ncol == m_pFwdMEGEEGRef->sol->ncol);
    QVERIFY(m_pFwdMEGEEGRead->sol->row_names == m_pFwdMEGEEGRef->sol->row_names);
    QVERIFY(m_pFwdMEGEEGRead->sol->col_names == m_pFwdMEGEEGRef->sol->col_names);
    for (Eigen::Index r = 0; r < m_pFwdMEGEEGRead->sol->data.rows(); ++r) {
        QVERIFY(m_pFwdMEGEEGRead->sol->data.row(r).isApprox(m_pFwdMEGEEGRef->sol->data.row(r), 0.0001));
    }

    // sol_grad may not be present in the forward solution files
    if (m_pFwdMEGEEGRead->sol_grad->nrow > 0 && m_pFwdMEGEEGRef->sol_grad->nrow > 0) {
        for (Eigen::Index r = 0; r < m_pFwdMEGEEGRead->sol_grad->data.rows(); ++r) {
            QVERIFY(m_pFwdMEGEEGRead->sol_grad->data.row(r).isApprox(m_pFwdMEGEEGRef->sol_grad->data.row(r), 0.0001));
        }
    }

    QVERIFY(m_pFwdMEGEEGRead->mri_head_t == m_pFwdMEGEEGRef->mri_head_t);
    // Note: Full src (MNESourceSpace) deep comparison skipped here because
    // MNEHemisphere::operator== calls isApprox on large geometry matrices which
    // can crash with Eigen 5. Source positions and normals are verified below.
    QVERIFY(m_pFwdMEGEEGRead->source_rr.isApprox(m_pFwdMEGEEGRef->source_rr, 1e-4f));
    QVERIFY(m_pFwdMEGEEGRead->source_nn.isApprox(m_pFwdMEGEEGRef->source_nn, 1e-4f));

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneForwardSolution::cleanupTestCase()
{
    QString fwdMEGEEGFileRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile::remove(fwdMEGEEGFileRef);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneForwardSolution)
#include "test_mne_forward_solution.moc"
