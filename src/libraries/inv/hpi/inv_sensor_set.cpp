//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_sensor_set.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref INVLIB::InvSensorSet and the @c FwdCoilSet → @c InvSensorSet builder.
 *
 * Implements the @c FwdCoilSet → @c InvSensorSet conversion (sizing
 * the per-integration-point matrices, copying positions / normals /
 * weights and the channel transform), the per-channel accessors and
 * the equality operators that drive the cache logic in
 * @ref InvHpiDataUpdater.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_sensor_set.h"
#include <iostream>
#include <fwd/fwd_coil_set.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace FIFFLIB;
using namespace FWDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvSensorSet::InvSensorSet(const FwdCoilSet::SPtr pFwdSensorSet)
{
    if(pFwdSensorSet!=nullptr) {
        initFromFwdCoilSet(pFwdSensorSet);
    }
}

//=============================================================================================================

void InvSensorSet::initFromFwdCoilSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdSensorSet)
{
    m_ncoils = pFwdSensorSet->ncoil();
    m_np = pFwdSensorSet->coils[0]->np;
    initMatrices(m_ncoils,m_np);

    // get data froms Fwd Coilset
    for(int i = 0; i < m_ncoils; i++){
        const FwdCoil* coil = pFwdSensorSet->coils[i].get();
        MatrixXd matRmag = MatrixXd::Zero(m_np,3);
        MatrixXd matCosmag = MatrixXd::Zero(m_np,3);
        RowVectorXd vecW(m_np);

        m_r0(i,0) = coil->r0[0];
        m_r0(i,1) = coil->r0[1];
        m_r0(i,2) = coil->r0[2];

        m_ez(i,0) = coil->ez[0];
        m_ez(i,1) = coil->ez[1];
        m_ez(i,2) = coil->ez[2];

        for (int p = 0; p < m_np; p++){
            m_w(i*m_np+p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                matRmag(p,c)   = coil->rmag(p, c);
                matCosmag(p,c) = coil->cosmag(p, c);
            }
        }

        m_cosmag.block(i*m_np,0,m_np,3) = matCosmag;
        m_rmag.block(i*m_np,0,m_np,3) = matRmag;
    }
    m_tra = MatrixXd::Identity(m_ncoils,m_ncoils);
}

//=============================================================================================================

void InvSensorSet::initMatrices(int ncoils, int np)
{
    m_ez = MatrixXd(ncoils,3);
    m_r0 = MatrixXd(ncoils,3);
    m_rmag = MatrixXd(ncoils*np,3);
    m_cosmag = MatrixXd(ncoils*np,3);
    m_cosmag = MatrixXd(ncoils*np,3);
    m_tra = MatrixXd(ncoils,ncoils);
    m_w = RowVectorXd(ncoils*np);
}

//=============================================================================================================

InvSensorSetCreator::InvSensorSetCreator()
{
    const QString qPath = QString(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
    m_pCoilDefinitions = FwdCoilSet::SPtr(FwdCoilSet::read_coil_defs(qPath).release());
}

//=============================================================================================================

InvSensorSet InvSensorSetCreator::updateSensorSet(const QList<FIFFLIB::FiffChInfo>& channelList,
                                            const Accuracy& accuracy)
{
    if(channelList.isEmpty()) {
        return InvSensorSet();
    } else {
        auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(channelList, channelList.size(), static_cast<int>(accuracy)).release());
        return InvSensorSet(pCoilMeg);
    }
}

//=============================================================================================================
