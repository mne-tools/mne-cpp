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
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    // The following verifies the forward solution round-trip:
    // compute -> write -> read-back must reproduce the same solution

    // --- Verbose diagnostics for CI debugging ---
    printf("[diag] source_ori: read=%d ref=%d\n", m_pFwdMEGEEGRead->source_ori, m_pFwdMEGEEGRef->source_ori);
    printf("[diag] surf_ori: read=%d ref=%d\n", m_pFwdMEGEEGRead->surf_ori ? 1 : 0, m_pFwdMEGEEGRef->surf_ori ? 1 : 0);
    printf("[diag] coord_frame: read=%d ref=%d\n", m_pFwdMEGEEGRead->coord_frame, m_pFwdMEGEEGRef->coord_frame);
    printf("[diag] nsource: read=%d ref=%d\n", m_pFwdMEGEEGRead->nsource, m_pFwdMEGEEGRef->nsource);
    printf("[diag] nchan: read=%d ref=%d\n", m_pFwdMEGEEGRead->nchan, m_pFwdMEGEEGRef->nchan);
    printf("[diag] sol nrow: read=%d ref=%d\n", m_pFwdMEGEEGRead->sol->nrow, m_pFwdMEGEEGRef->sol->nrow);
    printf("[diag] sol ncol: read=%d ref=%d\n", m_pFwdMEGEEGRead->sol->ncol, m_pFwdMEGEEGRef->sol->ncol);
    printf("[diag] sol row_names match: %d\n", m_pFwdMEGEEGRead->sol->row_names == m_pFwdMEGEEGRef->sol->row_names ? 1 : 0);
    printf("[diag] sol col_names match: %d\n", m_pFwdMEGEEGRead->sol->col_names == m_pFwdMEGEEGRef->sol->col_names ? 1 : 0);
    printf("[diag] mri_head_t match: %d\n", (m_pFwdMEGEEGRead->mri_head_t == m_pFwdMEGEEGRef->mri_head_t) ? 1 : 0);
    printf("[diag] source_rr isApprox: %d\n", m_pFwdMEGEEGRead->source_rr.isApprox(m_pFwdMEGEEGRef->source_rr, 1e-4f) ? 1 : 0);
    printf("[diag] source_nn isApprox: %d\n", m_pFwdMEGEEGRead->source_nn.isApprox(m_pFwdMEGEEGRef->source_nn, 1e-4f) ? 1 : 0);
    printf("[diag] sol_grad read nrow=%d ref nrow=%d\n", m_pFwdMEGEEGRead->sol_grad->nrow, m_pFwdMEGEEGRef->sol_grad->nrow);
    fflush(stdout);

    // --- FiffInfoBase comparison with per-field diagnostics ---
    {
        const auto& infoR = m_pFwdMEGEEGRead->info;
        const auto& infoF = m_pFwdMEGEEGRef->info;
        printf("[diag] info.filename match: %d ('%s' vs '%s')\n",
               infoR.filename == infoF.filename ? 1 : 0,
               infoR.filename.toUtf8().constData(), infoF.filename.toUtf8().constData());
        printf("[diag] info.bads match: %d (read=%lld ref=%lld)\n",
               infoR.bads == infoF.bads ? 1 : 0, (long long)infoR.bads.size(), (long long)infoF.bads.size());
        printf("[diag] info.nchan: read=%d ref=%d\n", infoR.nchan, infoF.nchan);
        printf("[diag] info.ch_names match: %d\n", infoR.ch_names == infoF.ch_names ? 1 : 0);
        printf("[diag] info.dev_head_t match: %d\n", (infoR.dev_head_t == infoF.dev_head_t) ? 1 : 0);
        printf("[diag] info.ctf_head_t match: %d\n", (infoR.ctf_head_t == infoF.ctf_head_t) ? 1 : 0);
        printf("[diag] info.chs.size: read=%lld ref=%lld\n", (long long)infoR.chs.size(), (long long)infoF.chs.size());
        int nChMatch = qMin(infoR.chs.size(), infoF.chs.size());
        for (int c = 0; c < nChMatch; ++c) {
            if (!(infoR.chs[c] == infoF.chs[c])) {
                const auto& a = infoR.chs[c];
                const auto& b = infoF.chs[c];
                printf("[diag] info.chs[%d] MISMATCH: scanNo=%d/%d logNo=%d/%d kind=%d/%d "
                       "range=%g/%g cal=%g/%g chpos=%d unit=%d/%d unit_mul=%d/%d "
                       "ch_name='%s'/'%s' coord_frame=%d/%d\n",
                       c, a.scanNo, b.scanNo, a.logNo, b.logNo, a.kind, b.kind,
                       a.range, b.range, a.cal, b.cal,
                       (a.chpos == b.chpos) ? 1 : 0,
                       a.unit, b.unit, a.unit_mul, b.unit_mul,
                       a.ch_name.toUtf8().constData(), b.ch_name.toUtf8().constData(),
                       a.coord_frame, b.coord_frame);
                printf("[diag]   coil_trans isApprox: %d  eeg_loc isApprox: %d\n",
                       a.coil_trans.isApprox(b.coil_trans, 0.0001f) ? 1 : 0,
                       a.eeg_loc.isApprox(b.eeg_loc, 0.0001f) ? 1 : 0);
                if (!a.eeg_loc.isApprox(b.eeg_loc, 0.0001f)) {
                    printf("[diag]   eeg_loc read:\n");
                    for (int rr = 0; rr < 3; ++rr)
                        printf("[diag]     %g %g\n", a.eeg_loc(rr,0), a.eeg_loc(rr,1));
                    printf("[diag]   eeg_loc ref:\n");
                    for (int rr = 0; rr < 3; ++rr)
                        printf("[diag]     %g %g\n", b.eeg_loc(rr,0), b.eeg_loc(rr,1));
                }
                if (!a.coil_trans.isApprox(b.coil_trans, 0.0001f)) {
                    printf("[diag]   coil_trans read row0: %g %g %g %g\n",
                           a.coil_trans(0,0), a.coil_trans(0,1), a.coil_trans(0,2), a.coil_trans(0,3));
                    printf("[diag]   coil_trans ref  row0: %g %g %g %g\n",
                           b.coil_trans(0,0), b.coil_trans(0,1), b.coil_trans(0,2), b.coil_trans(0,3));
                }
                fflush(stdout);
                // Only log first 5 mismatches to avoid flooding
                if (c >= 4) { printf("[diag] ... (further ch mismatches suppressed)\n"); break; }
            }
        }
        printf("[diag] info == match: %d\n", (infoR == infoF) ? 1 : 0);
        fflush(stdout);
    }

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
        if (!m_pFwdMEGEEGRead->sol->data.row(r).isApprox(m_pFwdMEGEEGRef->sol->data.row(r), 0.0001)) {
            printf("[diag] sol->data row %lld MISMATCH (maxCoeff diff = %g)\n",
                   (long long)r,
                   (m_pFwdMEGEEGRead->sol->data.row(r) - m_pFwdMEGEEGRef->sol->data.row(r)).cwiseAbs().maxCoeff());
            fflush(stdout);
        }
        QVERIFY(m_pFwdMEGEEGRead->sol->data.row(r).isApprox(m_pFwdMEGEEGRef->sol->data.row(r), 0.0001));
    }

    // sol_grad may not be present in the forward solution files
    if (m_pFwdMEGEEGRead->sol_grad->nrow > 0 && m_pFwdMEGEEGRef->sol_grad->nrow > 0) {
        for (Eigen::Index r = 0; r < m_pFwdMEGEEGRead->sol_grad->data.rows(); ++r) {
            if (!m_pFwdMEGEEGRead->sol_grad->data.row(r).isApprox(m_pFwdMEGEEGRef->sol_grad->data.row(r), 0.0001)) {
                printf("[diag] sol_grad->data row %lld MISMATCH\n", (long long)r);
                fflush(stdout);
            }
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
