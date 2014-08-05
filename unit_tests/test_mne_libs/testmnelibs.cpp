//=============================================================================================================
/**
* @file     mnelibtests.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the MNELibTests Class checkup routines.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "testmnelibs.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEUNITTESTS;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TestMNELibs::TestMNELibs(QObject *parent)
: QObject(parent)
{
    MNEForwardSolution::SPtr t_pFwd(new MNEForwardSolution);

    QSharedPointer<MNEForwardSolution>  t_pFwdNew(new MNEForwardSolution);





    MNEForwardSolution* t_pFwdOld = new MNEForwardSolution();
    delete t_pFwdOld;




}


//*************************************************************************************************************

bool TestMNELibs::checkFwdRead()
{

    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
    QFile t_File(t_sFileName);

    double res = 1.792287958513768e+07;
    double eps = res * 0.00001; // result might differ around 52 %

    int bads = 2;

    MNEForwardSolution t_ForwardSolution;
    if(MNE::read_forward_solution(t_File, t_ForwardSolution))
    {
        double sum = t_ForwardSolution.sol->data.sum();

        // data rows okay?
        if(t_ForwardSolution.sol->nrow != 366-bads || t_ForwardSolution.sol->data.rows() != 366-bads)
        {
            printf("Number of rows not correct!\n");
            emit checkupFailed(1);
            return false;
        }
        // data cols okay?
        else if(t_ForwardSolution.sol->ncol != 22494 || t_ForwardSolution.sol->data.cols() != 22494)
        {
            printf("Number of cols not correct!\n");
            emit checkupFailed(1);
            return false;
        }
        // checksum okay ?
        else if(sum < res-eps || sum > res + eps)
        {
            printf("Check sum not correct!\n");
            emit checkupFailed(1);
            return false;
        }

        printf("\nChecksum MATLAB (excluding 2 bads) aim: %f; is: %f\n", res, sum);
        return true;
    }
    else
    {
        emit checkupFailed(1);
        return false;
    }
}
