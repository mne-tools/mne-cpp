//=============================================================================================================
/**
 * @file     test_mne_forward_solution.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
    void compareForward();
    void cleanupTestCase();

private:
    double dEpsilon;

    QSharedPointer<MNEForwardSolution> m_pFwdMEGEEGRead;
    QSharedPointer<MNEForwardSolution> m_pFwdMEGEEGRef;
};

//=============================================================================================================

TestMneForwardSolution::TestMneForwardSolution()
: dEpsilon(0.0001)
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
    QString fwdMEGEEGFileRef(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile fileFwdMEGEEGRef(fwdMEGEEGFileRef);
    m_pFwdMEGEEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRef));

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --eeg
    // --accurate
    // --src ./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ./mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif
    // --mri ./mne-cpp-test-data/MEG/sample/all-trans.fif
    // --bem ./mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif
    // --mindist 5
    // --fwd ./mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif

    ComputeFwdSettings settingsMEGEEG;

    settingsMEGEEG.include_meg = true;
    settingsMEGEEG.include_eeg = true;
    settingsMEGEEG.accurate = true;
    settingsMEGEEG.srcname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    settingsMEGEEG.measname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    settingsMEGEEG.mriname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif";
    settingsMEGEEG.transname.clear();
    settingsMEGEEG.bemname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    settingsMEGEEG.mindist = 5.0f/1000.0f;
    settingsMEGEEG.solname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

    settingsMEGEEG.checkIntegrity();

    QSharedPointer<ComputeFwd> pFwdMEGEEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(&settingsMEGEEG));
    pFwdMEGEEGComputed->calculateFwd();

    // Read newly created fwd
    QFile fileFwdMEGEEGRead(settingsMEGEEG.solname);
    m_pFwdMEGEEGRead = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRead));

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute/Write/Read MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneForwardSolution::compareForward()
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // The following is equal to QVERIFY(*m_pFwdMEGEEGRead == *m_pFwdMEGEEGRef);
    // This just gives more information on what might be wrong if failing
    QVERIFY(m_pFwdMEGEEGRead->info == m_pFwdMEGEEGRef->info);
    QVERIFY(m_pFwdMEGEEGRead->source_ori == m_pFwdMEGEEGRef->source_ori);
    QVERIFY(m_pFwdMEGEEGRead->surf_ori == m_pFwdMEGEEGRef->surf_ori);
    QVERIFY(m_pFwdMEGEEGRead->coord_frame == m_pFwdMEGEEGRef->coord_frame);
    QVERIFY(m_pFwdMEGEEGRead->nsource == m_pFwdMEGEEGRef->nsource);
    QVERIFY(m_pFwdMEGEEGRead->nchan == m_pFwdMEGEEGRef->nchan);
    QVERIFY(*m_pFwdMEGEEGRead->sol == *m_pFwdMEGEEGRef->sol);
    QVERIFY(*m_pFwdMEGEEGRead->sol_grad == *m_pFwdMEGEEGRef->sol_grad);
    QVERIFY(m_pFwdMEGEEGRead->mri_head_t == m_pFwdMEGEEGRef->mri_head_t);
    QVERIFY(m_pFwdMEGEEGRead->src == m_pFwdMEGEEGRef->src);
    QVERIFY(m_pFwdMEGEEGRead->source_rr == m_pFwdMEGEEGRef->source_rr);
    QVERIFY(m_pFwdMEGEEGRead->source_nn == m_pFwdMEGEEGRef->source_nn);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneForwardSolution::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneForwardSolution)
#include "test_mne_forward_solution.moc"
