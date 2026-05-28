//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Daniel Strohmeier <daniel.strohmeier@gmail.com>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file coherence.cpp
 * @since April 2018
 * @brief Implementation of @ref CONNLIB::Coherence - magnitude-squared coherence between every channel pair, computed from the shared DPSS-tapered cross-spectral cache.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"
#include "coherence.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include "../connectivitysettings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Coherence::Coherence()
{
}

//*******************************************************************************************************

Network Coherence::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("COH");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "Coherence::calculate - Input data is empty";
        return finalNetwork;
    }

    if(AbstractMetric::m_bStorageModeIsActive == false) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.setSamplingFrequency(connectivitySettings.getSamplingFrequency());

    // Check if start and bin amount need to be reset to full spectrum
    int iNfft = connectivitySettings.getFFTSize();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    if(m_iNumberBinStart == -1 ||
       m_iNumberBinAmount == -1 ||
       m_iNumberBinStart > iNFreqs ||
       m_iNumberBinAmount > iNFreqs ||
       m_iNumberBinAmount + m_iNumberBinStart > iNFreqs) {
        qDebug() << "Coherence::calculate - Resetting to full spectrum";
        AbstractMetric::m_iNumberBinStart = 0;
        AbstractMetric::m_iNumberBinAmount = iNFreqs;
    }

    // Pass information about the FFT length. Use iNFreqs because we only use the half spectrum
    finalNetwork.setFFTSize(iNFreqs);
    finalNetwork.setUsedFreqBins(AbstractMetric::m_iNumberBinAmount);

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        rowVert = RowVectorXf::Zero(3);

        if(connectivitySettings.getNodePositions().rows() != 0 && i < connectivitySettings.getNodePositions().rows()) {
            rowVert(0) = connectivitySettings.getNodePositions().row(i)(0);
            rowVert(1) = connectivitySettings.getNodePositions().row(i)(1);
            rowVert(2) = connectivitySettings.getNodePositions().row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    //Calculate all-to-all coherence matrix over epochs
    Coherency::calculateAbs(finalNetwork,
                             connectivitySettings);;

    return finalNetwork;
}
