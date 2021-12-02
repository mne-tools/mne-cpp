//=============================================================================================================
/**
 * @file     sensorset.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
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

#include "sensorset.h"
#include <iostream>
#include <Eigen/Dense>
#include <fwd/fwd_coil_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

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

SensorSet::SensorSet()
{

    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    m_pCoilTemplate = FwdCoilSet::SPtr(FwdCoilSet::read_coil_defs(qPath));

    this->w = RowVectorXd(0);
    this->r0 = MatrixXd(0);
    this->cosmag = MatrixXd(0);
    this->rmag = MatrixXd(0);
    this->ncoils = 0;
    this->tra = MatrixXd::Identity(0,0);
    this->np = 0;

}

//=============================================================================================================

void SensorSet::updateSensorSet(const QList<FiffChInfo> channelList, const int iAccuracy)
{
    if(channelList.size() == 0) {
        std::cout<<std::endl<< "HPIFit::updateSensor - No channels. Returning.";
        return;
    }

    FiffCoordTransOld* t = NULL;

    FwdCoilSet::SPtr pCoilMeg = FwdCoilSet::SPtr(m_pCoilTemplate->create_meg_coils(channelList, channelList.size(), iAccuracy, t));

    convertFromFwdCoilSet(pCoilMeg);
}

//=============================================================================================================

void SensorSet::convertFromFwdCoilSet(const FwdCoilSet::SPtr pCoilMeg)
{
    int iNchan = pCoilMeg->ncoil;
    int iNp = pCoilMeg->coils[0]->np;

    // init sensor struct
    for(int i = 0; i < iNchan; i++){
        FwdCoil* coil = (pCoilMeg->coils[i]);
        MatrixXd matRmag = MatrixXd::Zero(iNp,3);
        MatrixXd matCosmag = MatrixXd::Zero(iNp,3);
        RowVectorXd vecW(iNp);

        this->r0(i,0) = coil->r0[0];
        this->r0(i,1) = coil->r0[1];
        this->r0(i,2) = coil->r0[2];

        for (int p = 0; p < iNp; p++){
            this->w(i*iNp+p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                matRmag(p,c)   = coil->rmag[p][c];
                matCosmag(p,c) = coil->cosmag[p][c];
            }
        }

        this->cosmag.block(i*iNp,0,iNp,3) = matCosmag;
        this->rmag.block(i*iNp,0,iNp,3) = matRmag;
    }
}