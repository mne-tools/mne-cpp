//=============================================================================================================
/**
* @file     imagcoherence.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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
* @note Notes:
* - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
*
* @brief     Imaginary coherence class declaration.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
#include "imagcoherence.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"
#include "../connectivitysettings.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ImagCoherence::ImagCoherence()
{
}


//*******************************************************************************************************

Network ImagCoherence::imagCoherence(const ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("Imaginary Coherence");

    if(connectivitySettings.m_matDataList.empty()) {
        qDebug() << "ImagCoherence::imagcoherence - Input data is empty";
        return finalNetwork;
    }

    //Create nodes
    int rows = connectivitySettings.m_matDataList.first().rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        if(connectivitySettings.m_matNodePositions.rows() != 0 && i < connectivitySettings.m_matNodePositions.rows()) {
            rowVert(0) = connectivitySettings.m_matNodePositions.row(i)(0);
            rowVert(1) = connectivitySettings.m_matNodePositions.row(i)(1);
            rowVert(2) = connectivitySettings.m_matNodePositions.row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    //Calculate all-to-all imaginary coherence matrix over epochs
    Coherency::computeCoherencyImag(finalNetwork,
                                    connectivitySettings.m_matDataList,
                                    connectivitySettings.m_iNfft,
                                    connectivitySettings.m_sWindowType);

    return finalNetwork;
}


//*************************************************************************************************************

QVector<MatrixXd> ImagCoherence::computeImagCoherence(const QList<MatrixXd> &matDataList,
                                                      int iNfft,
                                                      const QString &sWindowType)
{
    QVector<QPair<int,Eigen::MatrixXcd> > vecCoh;

    Coherency::computeCoherency(vecCoh,
                                matDataList,
                                iNfft,
                                sWindowType);


    QVector<MatrixXd> result;

    for(int i = 0; i < vecCoh.length(); ++i) {
        result << vecCoh.at(i).second.imag();
    }

    return result;
}
