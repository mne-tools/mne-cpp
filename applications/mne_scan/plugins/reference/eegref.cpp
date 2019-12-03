//=============================================================================================================
/**
* @file     eegref.cpp
* @author   Viktor Klüber <v.klueber@gmx.net>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    EEGRef class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegref.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REFERENCEPLUGIN;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGRef::EEGRef()
{
}


//*************************************************************************************************************

MatrixXd EEGRef::applyCAR(MatrixXd &matIER, FIFFLIB::FiffInfo::SPtr &pFiffInfo)
{
    unsigned int numTrueCh  = 0;
    unsigned int numCh      = pFiffInfo->chs.size();
    MatrixXd matOnes        = MatrixXd::Ones(numCh, numCh);
    MatrixXd matCenter      = MatrixXd::Identity(numCh, numCh);

    //determine the number of true channels
    for(unsigned int i = 0; i < numCh; ++i)
    {
        if(pFiffInfo->chs.at(i).ch_name.contains("EEG") && !pFiffInfo->bads.contains(pFiffInfo->chs.at(i).ch_name))
        {
            numTrueCh++;
        }
        else
        {
            // excluding non-EEG channels from the centering matrix
            matOnes.row(i).setZero();
            matOnes.col(i).setZero();
            matCenter.row(i).setZero();
            matCenter.col(i).setZero();
        }
    }

    //detrmine centering matrix
    if (numTrueCh != 0){
        matCenter = matCenter - (1/double(numTrueCh))*matOnes;
    } else {
        qDebug() << "Unable to determine centering matrix";
    }

    // determine EEG CAR data matrix
    MatrixXd matCAR = matCenter*matIER;

    //add former excluded non-EEG channels to the EEG CAR data matrix
    for(unsigned int i = 0; i < numCh; ++i)
    {
        if(!pFiffInfo->chs.at(i).ch_name.contains("EEG"))
        {
            matCAR.row(i) = matIER.row(i);
        }
    }

    return matCAR;
}
