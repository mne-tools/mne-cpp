//=============================================================================================================
/**
 * @file     sensorset.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     November, 2021
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
 * @brief    SensorSet class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/hpiFit/sensorset.h>
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

using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FWDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorSet::SensorSet(const FwdCoilSet::SPtr pFwdSensorSet)
{
    if(pFwdSensorSet!=nullptr) {
        convertFromFwdCoilSet(pFwdSensorSet);
    }
}

//=============================================================================================================

void SensorSet::convertFromFwdCoilSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdSensorSet)
{
    m_ncoils = pFwdSensorSet->ncoil;
    m_np = pFwdSensorSet->coils[0]->np;
    initMatrices(m_ncoils,m_np);

    // get data froms Fwd Coilset
    for(int i = 0; i < m_ncoils; i++){
        FwdCoil* coil = (pFwdSensorSet->coils[i]);
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
                matRmag(p,c)   = coil->rmag[p][c];
                matCosmag(p,c) = coil->cosmag[p][c];
            }
        }

        m_cosmag.block(i*m_np,0,m_np,3) = matCosmag;
        m_rmag.block(i*m_np,0,m_np,3) = matRmag;
    }
    m_tra = MatrixXd::Identity(m_ncoils,m_ncoils);
}

//=============================================================================================================

void SensorSet::initMatrices(int m_ncoils, int m_np)
{
    m_ez = MatrixXd(m_ncoils,3);
    m_r0 = MatrixXd(m_ncoils,3);
    m_rmag = MatrixXd(m_ncoils*m_np,3);
    m_cosmag = MatrixXd(m_ncoils*m_np,3);
    m_cosmag = MatrixXd(m_ncoils*m_np,3);
    m_tra = MatrixXd(m_ncoils,m_ncoils);
    m_w = RowVectorXd(m_ncoils*m_np);
}

//=============================================================================================================

SensorSetCreator::SensorSetCreator()
{
    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    m_pCoilDefinitions = FwdCoilSet::SPtr(FwdCoilSet::read_coil_defs(qPath));
}

//=============================================================================================================

SensorSet SensorSetCreator::updateSensorSet(const QList<FIFFLIB::FiffChInfo>& channelList,
                                            const Accuracy accuracy)
{
    if(channelList.isEmpty()) {
        return SensorSet();
    } else {
        auto pCoilMeg = FwdCoilSet::SPtr(m_pCoilDefinitions->create_meg_coils(channelList, channelList.size(), static_cast<int>(accuracy), nullptr));
        return SensorSet(pCoilMeg);
    }
}

//=============================================================================================================
