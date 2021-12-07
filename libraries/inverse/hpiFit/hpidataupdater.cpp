//=============================================================================================================
/**
 * @file     hpidataupdater.cpp
 * @author   Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief    HpiDataUpdater class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpidataupdater.h"
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_info.h>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HpiDataUpdater::HpiDataUpdater(FiffInfo::SPtr pFiffInfo)
{
    updateBadChannels(pFiffInfo);
    updateChannels(pFiffInfo);
    updateHpiDigitizer(pFiffInfo->dig);
    updateSensors(m_lChannels);
}

//=============================================================================================================

void HpiDataUpdater::updateBadChannels(FiffInfo::SPtr pFiffInfo)
{
    m_lBads = pFiffInfo->bads;
}

//=============================================================================================================

void HpiDataUpdater::updateChannels(FiffInfo::SPtr pFiffInfo)
{
    // Get the indices of inner layer channels and exclude bad channels and create channellist
    int iNumCh = pFiffInfo->nchan;
    m_lChannels.clear();
    m_vecInnerind.clear();
    for (int i = 0; i < iNumCh; ++i) {
        if(pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
            pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {

            // Check if the sensor is bad, if not append to innerind

            if(!(pFiffInfo->bads.contains(pFiffInfo->ch_names.at(i)))) {
                m_vecInnerind.append(i);
                m_lChannels.append(pFiffInfo->chs[i]);
            }
        }
    }
}

//=============================================================================================================

void HpiDataUpdater::updateSensors(const QList<FIFFLIB::FiffChInfo> lChannelList)
{
    int iAccuracy = 2;
    m_sensors.updateSensorSet(lChannelList,iAccuracy);
}

//=============================================================================================================

void HpiDataUpdater::updateHpiDigitizer(const QList<FiffDigPoint>& lDig)
{
    // extract hpi coils from digitizer
    QList<FiffDigPoint> lHPIPoints;
    int iNumCoils = 0;

    for(int i = 0; i < lDig.size(); ++i) {
        if(lDig[i].kind == FIFFV_POINT_HPI) {
            iNumCoils++;
            lHPIPoints.append(lDig[i]);
        }
    }

    // convert to matrix iNumCoils x 3
    if (lHPIPoints.size() > 0) {
        m_matHpiDigitizer = MatrixXd(iNumCoils,3);
        for (int i = 0; i < lHPIPoints.size(); ++i) {
            m_matHpiDigitizer(i,0) = lHPIPoints.at(i).r[0];
            m_matHpiDigitizer(i,1) = lHPIPoints.at(i).r[1];
            m_matHpiDigitizer(i,2) = lHPIPoints.at(i).r[2];
        }
    } else {
        std::cout << "HPIFit::updateHpiDigitizer - No HPI coils digitized. Returning." << std::endl;
        return;
    }
}

//=============================================================================================================

void HpiDataUpdater::prepareData(const Eigen::MatrixXd& matData)
{
    // extract data for channels to use
    m_matInnerdata = MatrixXd(m_vecInnerind.size(), matData.cols());

    for(int j = 0; j < m_vecInnerind.size(); ++j) {
        m_matInnerdata.row(j) << matData.row(m_vecInnerind[j]);
    }
}

//=============================================================================================================

void HpiDataUpdater::prepareProjectors(const Eigen::MatrixXd& matProjectors)
{
    // check if m_vecInnerInd is alreadz initialized
    if(m_vecInnerind.size() == 0) {
        std::cout << "HPIFit::updateProjectors - No channels. Returning." << std::endl;
        return;
    }

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vecInnerind.size(),matProjectors.cols());
    MatrixXd matProjectorsInnerind(m_vecInnerind.size(),m_vecInnerind.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = matProjectors.row(m_vecInnerind.at(i));
    }

    for (int i = 0; i < matProjectorsInnerind.cols(); ++i) {
        matProjectorsInnerind.col(i) = matProjectorsRows.col(m_vecInnerind.at(i));
    }
    m_matProjectors = matProjectorsInnerind;
    return;
}
