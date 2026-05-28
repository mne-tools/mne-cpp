//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_hpi_data_updater.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of the HPI pre-processing front-end.
 *
 * Implements the cached state machine: detect @c FiffInfo changes
 * (channel list, bad-channel list, dig-point list), rebuild the
 * channel selection, project the SSP matrix into the good-channel
 * subspace, apply it to the data buffer and rebuild the
 * @ref InvSensorSet from the updated coil list. Exposes pre-projected
 * data, projectors, sensors and digitised coil positions as read-only
 * accessors consumed by @ref InvHpiFit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_hpi_data_updater.h"
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

using namespace INVLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

InvHpiDataUpdater::InvHpiDataUpdater(const FiffInfo::SPtr pFiffInfo)
    : m_sensors(InvSensorSet())
{
    updateBadChannels(pFiffInfo);
    updateChannels(pFiffInfo);
    updateHpiDigitizer(pFiffInfo->dig);
    updateSensors(m_lChannels);
}

//=============================================================================================================

void InvHpiDataUpdater::updateBadChannels(FiffInfo::SPtr pFiffInfo)
{
    m_lBads = pFiffInfo->bads;
}

//=============================================================================================================

void InvHpiDataUpdater::updateChannels(FiffInfo::SPtr pFiffInfo)
{
    // Get the indices of inner layer channels and exclude bad channels and create channellist
    const int iNumCh = pFiffInfo->nchan;
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

void InvHpiDataUpdater::updateSensors(const QList<FIFFLIB::FiffChInfo>& lChannels)
{
    const Accuracy accuracy = Accuracy::high;
    m_sensors = m_sensorSetCreator.updateSensorSet(lChannels,accuracy);
}

//=============================================================================================================

void InvHpiDataUpdater::updateHpiDigitizer(const QList<FiffDigPoint>& lDig)
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
        std::cout << "InvHpiFit::updateHpiDigitizer - No HPI coils digitized. Returning." << std::endl;
        return;
    }
}

//=============================================================================================================

void InvHpiDataUpdater::checkForUpdate(const FiffInfo::SPtr pFiffInfo)
{
    const bool bUpdate = checkIfChanged(pFiffInfo->bads,pFiffInfo->chs);
    if(bUpdate)
    {
        updateBadChannels(pFiffInfo);
        updateChannels(pFiffInfo);
        updateHpiDigitizer(pFiffInfo->dig);
        updateSensors(m_lChannels);
    }
}

//=============================================================================================================

bool InvHpiDataUpdater::checkIfChanged(const QList<QString>& lBads, const QList<FIFFLIB::FiffChInfo>& lChannels)
{
    bool bUpdate = false;
    if(!(m_lBads == lBads) || !(m_lChannels == lChannels)) {
        bUpdate = true;
    }
    return bUpdate;
}

//=============================================================================================================

void InvHpiDataUpdater::prepareDataAndProjectors(const MatrixXd &matData, const MatrixXd &matProjectors)
{
    prepareData(matData);
    prepareProjectors(matProjectors);
    m_matDataProjected = m_matProjectors * m_matInnerdata;
}

//=============================================================================================================

void InvHpiDataUpdater::prepareData(const Eigen::MatrixXd& matData)
{
    // extract data for channels to use
    m_matInnerdata = MatrixXd(m_vecInnerind.size(), matData.cols());

    for(int j = 0; j < m_vecInnerind.size(); ++j) {
        m_matInnerdata.row(j) << matData.row(m_vecInnerind[j]);
    }
}

//=============================================================================================================

void InvHpiDataUpdater::prepareProjectors(const Eigen::MatrixXd& matProjectors)
{
    // check if m_vecInnerInd is alreadz initialized
    if(m_vecInnerind.size() == 0) {
        std::cout << "InvHpiFit::updateProjectors - No channels. Returning." << std::endl;
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
