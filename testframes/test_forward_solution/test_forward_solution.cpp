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
    void compareForward();

    double epsilon;

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
    // Forward Solution Settings
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Forward Solution Settings >>>>>>>>>>>>>>>>>>>>>>>>>");

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --accurate
    // --src ./MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif
    // --mri ./MNE-sample-data/subjects/sample/mri/brain-neuromag/sets/COR.fif
    // --bem ./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif
    // --mindist 5
    // --fwd ./MNE-sample-data/Result/sample_audvis-meg-oct-6-fwd.fif

    ComputeFwdSettings settings;

    settings.include_meg = true;
    settings.accurate = true;
    settings.srcname = "./MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif";
    settings.measname = "./MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
    settings.mriname = "./MNE-sample-data/subjects/sample/mri/brain-neuromag/sets/COR.fif";
    settings.mri_head_ident = false;
    settings.transname.clear();
    settings.bemname = "./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    settings.mindist = 5.0f/1000.0f;
    settings.solname = "./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif";

    settings.checkIntegrity();

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Forward Solution Settings Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

    // Compute and Write Forward Solution
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compute Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    ComputeFwd cmpFwd(&settings);
    cmpFwd.calculateFwd();

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compute Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<");

    // Compare fwd solutions
    compareForward();
}


//*************************************************************************************************************

void TestForwardSolution::compareForward()
{
    qInfo(">>>>>>>>>>>>>>>>>>>>>>>>> Compare Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>");

    // Read computed forward solution
    QString fwdFileNameComp("./mne-cpp-test-data/Result/sample_audvis-meg-oct-6-fwd.fif");
    QFile fileForwardSolutionComp(fwdFileNameComp);
    MNEForwardSolution fwdComp(fileForwardSolutionComp);

    // Read reference forward solution
    QString fwdFileRef("./mne-cpp-test-data/Result/ref-sample_audvis-meg-oct-6-fwd.fif");
    QFile fileForwardSolutionRef(fwdFileRef);
    MNEForwardSolution fwdRef(fileForwardSolutionRef);

    // Compare fwd solutions
    Q_VERIFY(fwdRef == fwdComp);

    qInfo("<<<<<<<<<<<<<<<<<<<<<<<<< Compare Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
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
