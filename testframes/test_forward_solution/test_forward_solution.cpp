//=============================================================================================================
/**
* @file     test_forward_solution.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/computeFwd/compute_fwd_settings.h>
#include <fwd/computeFwd/compute_fwd.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace MNELIB;


//=============================================================================================================
/**
* DECLARE CLASS TestForwardSolution
*
* @brief The TestForwardSolution class provides dipole fit tests
*
*/
class TestForwardSolution : public QObject
{
    Q_OBJECT

public:
    TestForwardSolution();

private slots:
    void initTestCase();
    void computeForward();
    void cleanupTestCase();

private:
    void compareForwardMEGEEG();

    double epsilon;

    QSharedPointer<ComputeFwd> m_pFwdMEGEEGComputed;
    QSharedPointer<MNEForwardSolution> m_pFwdMEGEEGRef;

};


//*************************************************************************************************************

TestForwardSolution::TestForwardSolution()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestForwardSolution::initTestCase()
{

}


//*************************************************************************************************************

void TestForwardSolution::computeForward()
{
    // Compute and Write Forward Solution
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Read reference forward solution
    QString fwdMEGEEGFileRef("./mne-cpp-test-data/Result/ref_sample_audvis-meg-eeg-oct-6-fwd.fif");
    //QString fwdMEGEEGFileRef("./mne-cpp-test-data/MEG/sample/sample_audvis_trunc-meg-eeg-oct-6-fwd.fif"); // Use this in conjunction with sample-1280-1280-1280-bem.fif
    QFile fileFwdMEGEEGRef(fwdMEGEEGFileRef);
    m_pFwdMEGEEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRef));

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --eeg
    // --accurate
    // --src ./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif
    // --mri ./mne-cpp-test-data/MEG/sample/all-trans.fif
    // --bem ./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif
    // --mindist 5
    // --fwd ./mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif

    ComputeFwdSettings settingsMEGEEG;

    settingsMEGEEG.include_meg = true;
    settingsMEGEEG.include_eeg = true;
    settingsMEGEEG.accurate = true;
    settingsMEGEEG.srcname = "./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    settingsMEGEEG.measname = "./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif";
    settingsMEGEEG.mriname = "./mne-cpp-test-data/MEG/sample/all-trans.fif";
    //settingsMEGEEG.mriname = "./mne-cpp-test-data/MEG/sample/sample_audvis_trunc-trans.fif";
    //settingsMEGEEG.mriname = "./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
    settingsMEGEEG.transname.clear();
    //settingsMEGEEG.bemname = "./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    settingsMEGEEG.bemname = "sample-5120-5120-5120-bem.fif";
    //settingsMEGEEG.bemname = "./mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    settingsMEGEEG.mindist = 5.0f/1000.0f;
    settingsMEGEEG.solname = "./mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

    settingsMEGEEG.checkIntegrity();

    m_pFwdMEGEEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(&settingsMEGEEG));
    m_pFwdMEGEEGComputed->calculateFwd();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

    compareForwardMEGEEG();
}


//*************************************************************************************************************

void TestForwardSolution::compareForwardMEGEEG()
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Access public members of the old mne-c fwd computation.
    // This is just temporary until we can use the new refactored fwd object to easily compare via == operator. See QVERIFY below.
    // Read/write is always not supported yet since we currently have two MNESourceSpace classes: MNESourceSpace and MNESourceSpaceOld
    qDebug() << "m_pFwdMEGEEGComputed->meg_forward->nrow" << m_pFwdMEGEEGComputed->meg_forward->nrow;
    qDebug() << "m_pFwdMEGEEGRef->sol->nrow" << m_pFwdMEGEEGRef->sol->nrow;
    qDebug() << "";
    qDebug() << "m_pFwdMEGEEGComputed->meg_forward->ncol + m_pFwdMEGEEGComputed->eeg_forward->ncol" << m_pFwdMEGEEGComputed->meg_forward->ncol + m_pFwdMEGEEGComputed->eeg_forward->ncol;
    qDebug() << "m_pFwdMEGEEGRef->sol->ncol" << m_pFwdMEGEEGRef->sol->ncol;
    qDebug() << "";

    // Summ up the solution matrix elements to compare them with the references
    double sumComputed = 0;
    for(int i = 0; i < m_pFwdMEGEEGComputed->meg_forward->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGEEGComputed->meg_forward->ncol; ++j) {
            sumComputed += m_pFwdMEGEEGComputed->meg_forward->data[i][j];
        }
    }
    for(int i = 0; i < m_pFwdMEGEEGComputed->eeg_forward->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGEEGComputed->eeg_forward->ncol; ++j) {
            sumComputed += m_pFwdMEGEEGComputed->eeg_forward->data[i][j];
        }
    }

    double sumRef = 0;
    for(int i = 0; i < m_pFwdMEGEEGRef->sol->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGEEGRef->sol->ncol; ++j) {
            sumRef += m_pFwdMEGEEGRef->sol->data(i,j);
        }
    }

    qDebug() << "sumComputed" << sumComputed;
    qDebug() << "sumRef" << sumRef;
    qDebug() << "";

    // Please note that the solution matrix is transposed once it is read from the data file
    QVERIFY(m_pFwdMEGEEGComputed->meg_forward->nrow == m_pFwdMEGEEGRef->sol->ncol);
    QVERIFY(m_pFwdMEGEEGComputed->meg_forward->ncol + m_pFwdMEGEEGComputed->eeg_forward->ncol  == m_pFwdMEGEEGRef->sol->nrow);

    //Compare the actual fwd solution matrix results
    QVERIFY(sumComputed-sumRef <= epsilon);

    // This is rather hard to test since we need to combien the two forward solutions.
    // This is normally done when reading the combined fwd solutions. Wait until everything is refactored.
    // QVERIFY(m_pFwdMEGEEGComputed == m_pFwdMEGEEGRef);

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestForwardSolution::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestForwardSolution)
#include "test_forward_solution.moc"
