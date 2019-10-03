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
    void compareForwardMEG();
    void compareForwardEEG();
    void compareForwardMEGEEG();

    double epsilon;

    QSharedPointer<ComputeFwd> m_pFwdMEGComputed;
    QSharedPointer<MNEForwardSolution> m_pFwdMEGRef;

    QSharedPointer<ComputeFwd> m_pFwdEEGComputed;
    QSharedPointer<MNEForwardSolution> m_pFwdEEGRef;

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
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compute MEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Read reference forward solution
    QString fwdMEGFileRef("./mne-cpp-test-data/Result/ref_sample_audvis-meg-oct-6-fwd.fif");
    QFile fileFwdMEGRef(fwdMEGFileRef);
    m_pFwdMEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGRef));

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --accurate
    // --src ./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif
    // --mri ./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif
    // --bem ./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif
    // --mindist 5
    // --fwd ./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif

    ComputeFwdSettings settingsMEG;

    settingsMEG.include_meg = true;
    settingsMEG.accurate = true;
    settingsMEG.srcname = "./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    settingsMEG.measname = "./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif";
    settingsMEG.mriname = "./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
    settingsMEG.mri_head_ident = false;
    settingsMEG.transname.clear();
    settingsMEG.bemname = "./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    settingsMEG.mindist = 5.0f/1000.0f;
    settingsMEG.solname = "./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif";

    settingsMEG.checkIntegrity();

    m_pFwdMEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(&settingsMEG));
    m_pFwdMEGComputed->calculateFwd();

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compute EMEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Read reference forward solution
    QString fwdEEGFileRef("./mne-cpp-test-data/Result/ref_sample_audvis-eeg-oct-6-fwd.fif");
    QFile fileFwdEEGRef(fwdEEGFileRef);
    m_pFwdEEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdEEGRef));

    //Following is equivalent to:
    //mne_forward_solution
    // --eeg
    // --accurate
    // --src ./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif
    // --mri ./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif
    // --bem ./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif
    // --mindist 5
    // --fwd ./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif

    ComputeFwdSettings settingsEEG;

    settingsEEG.include_eeg = true;
    settingsEEG.accurate = true;
    settingsEEG.srcname = "./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    settingsEEG.measname = "./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif";
    settingsEEG.mriname = "./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
    settingsEEG.mri_head_ident = false;
    settingsEEG.transname.clear();
    settingsEEG.bemname = "./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    settingsEEG.mindist = 5.0f/1000.0f;
    settingsEEG.solname = "./mne-cpp-test-data/Result/sample_audvis-eeg-oct-6-fwd.fif";

    settingsEEG.checkIntegrity();

    m_pFwdEEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(&settingsEEG));
    m_pFwdEEGComputed->calculateFwd();

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

//    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compute MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

//    // Read reference forward solution
//    QString fwdMEGEEGFileRef("./mne-cpp-test-data/Result/ref_sample_audvis-meg-eeg-oct-6-fwd.fif");
//    QFile fileFwdMEGEEGRef(fwdMEGEEGFileRef);
//    m_pFwdMEGEEGRef = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fileFwdMEGEEGRef));

//    //Following is equivalent to:
//    //mne_forward_solution
//    // --meg
//    // --eeg
//    // --accurate
//    // --src ./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
//    // --meas ./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif
//    // --mri ./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif
//    // --bem ./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif
//    // --mindist 5
//    // --fwd ./mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif

//    ComputeFwdSettings settingsMEGEEG;

//    settingsMEGEEG.include_meg = true;
//    settingsMEGEEG.include_eeg = true;
//    settingsMEGEEG.accurate = true;
//    settingsMEGEEG.srcname = "./mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
//    settingsMEGEEG.measname = "./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif";
//    settingsMEGEEG.mriname = "./mne-cpp-test-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
//    settingsMEGEEG.mri_head_ident = false;
//    settingsMEGEEG.transname.clear();
//    settingsMEGEEG.bemname = "./mne-cpp-test-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
//    settingsMEGEEG.mindist = 5.0f/1000.0f;
//    settingsMEGEEG.solname = "./mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";

//    settingsMEGEEG.checkIntegrity();

//    m_pFwdMEGEEGComputed = QSharedPointer<ComputeFwd>(new ComputeFwd(&settingsMEGEEG));
//    m_pFwdMEGEEGComputed->calculateFwd();

//    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compute MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

    compareForwardMEG();
    compareForwardEEG();
    // compareForwardMEGEEG();
}


//*************************************************************************************************************

void TestForwardSolution::compareForwardMEG()
{
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compare MEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Access public members of the old mne-c fwd computation.
    // This is just temporary until we can use the new refactored fwd object to easily compare via == operator.
    // Read/write is always not supported yet since we currently have two MNESourceSpace classes: MNESourceSpace and MNESourceSpaceOld
    qDebug() << "m_pFwdMEGComputed->meg_forward->nrow" << m_pFwdMEGComputed->meg_forward->nrow;
    qDebug() << "m_pFwdMEGRef->sol->nrow" << m_pFwdMEGRef->sol->nrow;
    qDebug() << "";
    qDebug() << "m_pFwdMEGComputed->meg_forward->ncol" << m_pFwdMEGComputed->meg_forward->ncol;
    qDebug() << "m_pFwdMEGRef->sol->ncol" << m_pFwdMEGRef->sol->ncol;
    qDebug() << "";

    // Please note that the solution matrix is transposed once it is read from the data file
    QVERIFY(m_pFwdMEGComputed->meg_forward->nrow == m_pFwdMEGRef->sol->ncol);
    QVERIFY(m_pFwdMEGComputed->meg_forward->ncol == m_pFwdMEGRef->sol->nrow);

    double sumComputed = 0;
    for(int i = 0; i < m_pFwdMEGComputed->meg_forward->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGComputed->meg_forward->ncol; ++j) {
            sumComputed += m_pFwdMEGComputed->meg_forward->data[i][j];
        }
    }

    double sumRef = 0;
    for(int i = 0; i < m_pFwdMEGRef->sol->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGRef->sol->ncol; ++j) {
            sumRef += m_pFwdMEGRef->sol->data(i,j);
        }
    }

    qDebug() << "sumComputed" << sumComputed;
    qDebug() << "sumRef" << sumRef;
    qDebug() << "";

    QVERIFY(sumComputed-sumRef <= epsilon);



//    bool                res = false;
//    MNELIB::MneSourceSpaceOld*  *spaces = NULL;  /* The source spaces */
//    int                 nspace  = 0;
//    int                 nsource = 0;     /* Number of source space points */

//    FIFFLIB::FiffCoordTransOld* mri_head_t = NULL;   /* MRI <-> head coordinate transformation */
//    FIFFLIB::FiffCoordTransOld* meg_head_t = NULL;   /* MEG <-> head coordinate transformation */

//    FIFFLIB::fiffChInfo     megchs   = NULL; /* The MEG channel information */
//    int            nmeg     = 0;
//    FIFFLIB::fiffChInfo     eegchs   = NULL; /* The EEG channel information */
//    int            neeg     = 0;
//    FIFFLIB::fiffChInfo     compchs = NULL;
//    int            ncomp    = 0;

//    FwdCoilSet*             megcoils = NULL;     /* The coil descriptions */
//    FwdCoilSet*             compcoils = NULL;    /* MEG compensation coils */
//    MNELIB::MneCTFCompDataSet*      comp_data  = NULL;
//    FwdCoilSet*             eegels = NULL;
//    FwdEegSphereModelSet*   eeg_models = NULL;

//    MNELIB::MneNamedMatrix* meg_forward      = NULL;    /* Result of the MEG forward calculation */
//    MNELIB::MneNamedMatrix* eeg_forward      = NULL;    /* Result of the EEG forward calculation */
//    MNELIB::MneNamedMatrix* meg_forward_grad = NULL;    /* Result of the MEG forward gradient calculation */
//    MNELIB::MneNamedMatrix* eeg_forward_grad = NULL;    /* Result of the EEG forward gradient calculation */
//    int            k;
//    FIFFLIB::fiffId         mri_id  = NULL;
//    FIFFLIB::fiffId         meas_id = NULL;
//    FILE           *out = NULL;     /* Output filtered points here */

//    FwdCoilSet*       templates = NULL;
//    FwdEegSphereModel* eeg_model = NULL;
//    FwdBemModel*       bem_model = NULL;

//    QString qPath;
//    QFile file;




//    FiffInfoBase info;                  /**< light weighted measurement info */
//    fiff_int_t source_ori;              /**< Source orientation: fixed or free */
//    bool surf_ori;                      /**< If surface oriented */
//    fiff_int_t coord_frame;             /**< Coil coordinate system definition */
//    fiff_int_t nsource;                 /**< Number of source dipoles */
//    fiff_int_t nchan;                   /**< Number of channels */
//    FiffNamedMatrix::SDPtr sol;         /**< Forward solution */
//    FiffNamedMatrix::SDPtr sol_grad;    /**< ToDo... */
//    FiffCoordTrans mri_head_t;          /**< MRI head coordinate transformation */
//    MNESourceSpace src;                 /**< Geometric description of the source spaces (hemispheres) */
//    MatrixX3f source_rr;                /**< Source locations */
//    MatrixX3f source_nn;                /**< Source normals (number depends on fixed or free orientation) */

//    // Read computed forward solution
//    QString fwdFileNameComp("./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif");
//    QFile fileForwardSolutionComp(fwdFileNameComp);
//    MNEForwardSolution fwdComp(fileForwardSolutionComp);

//    // Compare fwd solutions
//    QVERIFY(fwdRef == fwdComp);

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//*************************************************************************************************************

void TestForwardSolution::compareForwardEEG()
{
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compare EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Access public members of the old mne-c fwd computation.
    // This is just temporary until we can use the new refactored fwd object to easily compare via == operator.
    // Read/write is always not supported yet since we currently have two MNESourceSpace classes: MNESourceSpace and MNESourceSpaceOld
    qDebug() << "m_pFwdEEGComputed->eeg_forward->nrow" << m_pFwdEEGComputed->eeg_forward->nrow;
    qDebug() << "m_pFwdEEGRef->sol->nrow" << m_pFwdEEGRef->sol->nrow;
    qDebug() << "";
    qDebug() << "m_pFwdEEGComputed->eeg_forward->ncol" << m_pFwdEEGComputed->eeg_forward->ncol;
    qDebug() << "m_pFwdEEGRef->sol->ncol" << m_pFwdEEGRef->sol->ncol;
    qDebug() << "";

    // Please note that the solution matrix is transposed once it is read from the data file
    QVERIFY(m_pFwdEEGComputed->eeg_forward->nrow == m_pFwdEEGRef->sol->ncol);
    QVERIFY(m_pFwdEEGComputed->eeg_forward->ncol == m_pFwdEEGRef->sol->nrow);

    double sumComputed = 0;
    for(int i = 0; i < m_pFwdEEGComputed->eeg_forward->nrow; ++i) {
        for(int j = 0; j < m_pFwdEEGComputed->eeg_forward->ncol; ++j) {
            sumComputed += m_pFwdEEGComputed->eeg_forward->data[i][j];
        }
    }

    double sumRef = 0;
    for(int i = 0; i < m_pFwdEEGRef->sol->nrow; ++i) {
        for(int j = 0; j < m_pFwdEEGRef->sol->ncol; ++j) {
            sumRef += m_pFwdEEGRef->sol->data(i,j);
        }
    }

    qDebug() << "sumComputed" << sumComputed;
    qDebug() << "sumRef" << sumRef;
    qDebug() << "";

    QVERIFY(sumComputed-sumRef <= epsilon);

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compare EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestForwardSolution::compareForwardMEGEEG()
{
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compare MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // This is rather hard to test since we need to combien the two forward solutions. This is normally done when reading the combined fwd solutions.

    // Access public members of the old mne-c fwd computation.
    // This is just temporary until we can use the new refactored fwd object to easily compare via == operator.
    // Read/write is always not supported yet since we currently have two MNESourceSpace classes: MNESourceSpace and MNESourceSpaceOld
    qDebug() << "m_pFwdMEGEEGComputed->meg_forward->nrow" << m_pFwdMEGEEGComputed->meg_forward->nrow;
    qDebug() << "m_pFwdMEGEEGRef->sol->nrow" << m_pFwdMEGRef->sol->nrow;
    qDebug() << "";
    qDebug() << "m_pFwdMEGEEGComputed->meg_forward->ncol" << m_pFwdMEGEEGComputed->meg_forward->ncol;
    qDebug() << "m_pFwdMEGEEGRef->sol->ncol" << m_pFwdMEGEEGRef->sol->ncol;
    qDebug() << "";

    // Please note that the solution matrix is transposed once it is read from the data file
    QVERIFY(m_pFwdMEGEEGComputed->meg_forward->nrow == m_pFwdMEGEEGRef->sol->ncol);
    QVERIFY(m_pFwdMEGEEGComputed->meg_forward->ncol == m_pFwdMEGEEGRef->sol->nrow);

    double sumComputed = 0;
    for(int i = 0; i < m_pFwdMEGEEGComputed->meg_forward->nrow; ++i) {
        for(int j = 0; j < m_pFwdMEGEEGComputed->meg_forward->ncol; ++j) {
            sumComputed += m_pFwdMEGEEGComputed->meg_forward->data[i][j];
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

    QVERIFY(sumComputed-sumRef <= epsilon);

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
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
