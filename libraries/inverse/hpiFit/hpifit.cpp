//=============================================================================================================
/**
 * @file     hpifit.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <doerfelruben@aol.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    HPIFit class defintion.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpifit.h"
#include "hpifitdata.h"
#include "sensorset.h"
#include "hpidataupdater.h"
#include "signalmodel.h"
#include "hpimodelparameters.h"

#include <utils/ioutils.h>
#include <utils/mnemath.h>

#include <iostream>
#include <vector>
#include <numeric>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_dig_point_set.h>
#include <fstream>

#include <fwd/fwd_coil_set.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

HPIFit::HPIFit(const SensorSet& sensorSet)
    : m_sensors(sensorSet),
      m_signalModel(SignalModel())
{

}

//=============================================================================================================

void HPIFit::fit(const MatrixXd& matProjectedData,
                 const MatrixXd& matProjectors,
                 const HpiModelParameters& hpiModelParameters,
                 const MatrixXd& matCoilsHead,
                 HpiFitResult& hpiFitResult)
{
    fit(matProjectedData,matProjectors,hpiModelParameters,matCoilsHead,false,hpiFitResult);
}

//=============================================================================================================

void HPIFit::fit(const MatrixXd& matProjectedData,
                 const MatrixXd& matProjectors,
                 const HpiModelParameters& hpiModelParameters,
                 const MatrixXd& matCoilsHead,
                 const bool bOrderFrequencies,
                 HpiFitResult& hpiFitResult)
{
    if(matProjectedData.rows() != matProjectors.rows()) {
        std::cout<< "HPIFit::fit - Projector and data dimensions do not match. Returning."<<std::endl;
        return;
    } else if(hpiModelParameters.iNHpiCoils()!= matCoilsHead.rows()) {
        std::cout<< "HPIFit::fit - Number of coils and hpi digitizers do not match. Returning."<<std::endl;
        return;
    } else if(matProjectedData.rows()==0 || matProjectors.rows()==0) {
        std::cout<< "HPIFit::fit - No data or Projectors passed. Returning."<<std::endl;
        return;
    } else if(m_sensors.ncoils() != matProjectedData.rows()) {
        std::cout<< "HPIFit::fit - Number of channels in sensors and data do not match. Returning."<<std::endl;
        return;
    }

    const MatrixXd matAmplitudes = computeAmplitudes(matProjectedData,
                                                     hpiModelParameters);

    CoilParam fittedCoilParams = computeCoilLocation(matAmplitudes,
                                                     matProjectors,
                                                     hpiFitResult.devHeadTrans,
                                                     hpiFitResult.errorDistances,
                                                     matCoilsHead);
    if(bOrderFrequencies) {
        const std::vector<int> vecOrder = findCoilOrder(fittedCoilParams.pos,
                                                        matCoilsHead);

        fittedCoilParams.pos = order(vecOrder,fittedCoilParams.pos);
        hpiFitResult.hpiFreqs = order(vecOrder,hpiModelParameters.vecHpiFreqs());
    }

    hpiFitResult.GoF = computeGoF(fittedCoilParams.dpfiterror);

    hpiFitResult.fittedCoils = getFittedPointSet(fittedCoilParams.pos);

    hpiFitResult.devHeadTrans = computeDeviceHeadTransformation(fittedCoilParams.pos,
                                                                matCoilsHead);

    hpiFitResult.errorDistances = computeEstimationError(fittedCoilParams.pos,
                                                         matCoilsHead,
                                                         hpiFitResult.devHeadTrans);
}

//=============================================================================================================

Eigen::MatrixXd HPIFit::computeAmplitudes(const Eigen::MatrixXd& matProjectedData,
                                          const HpiModelParameters& hpiModelParameters)
{
    // fit model
    MatrixXd matTopo = m_signalModel.fitData(hpiModelParameters,matProjectedData);
    matTopo.transposeInPlace();

    // split into sine and cosine amplitudes
    const int iNumCoils = hpiModelParameters.iNHpiCoils();

    MatrixXd matAmpSine(matProjectedData.cols(), iNumCoils);
    MatrixXd matAmpCosine(matProjectedData.cols(), iNumCoils);

    matAmpSine = matTopo.leftCols(iNumCoils);
    matAmpCosine = matTopo.middleCols(iNumCoils,iNumCoils);

    // Select sine or cosine component depending on their contributions to the amplitudes
    for(int j = 0; j < iNumCoils; ++j) {
        float fNS = 0.0;
        float fNC = 0.0;
        fNS = matAmpSine.col(j).array().square().sum();
        fNC = matAmpCosine.col(j).array().square().sum();
        if(fNC > fNS) {
            matAmpSine.col(j) = matAmpCosine.col(j);
        }
    }

    return matAmpSine;
}

//=============================================================================================================

CoilParam HPIFit::computeCoilLocation(const Eigen::MatrixXd& matAmplitudes,
                                      const MatrixXd& matProjectors,
                                      const FIFFLIB::FiffCoordTrans& transDevHead,
                                      const QVector<double>& vecError,
                                      const MatrixXd& matCoilsHead,
                                      const int iMaxIterations,
                                      const float fAbortError)
{
    const int iNumCoils = matAmplitudes.cols();
    MatrixXd matCoilsSeed = MatrixXd::Zero(iNumCoils,3);

    const double dError = std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();

    if(transDevHead.trans != MatrixXd::Identity(4,4).cast<float>() && dError < 0.010) {
        // if good last fit, use old trafo
        matCoilsSeed = transDevHead.apply_inverse_trans(matCoilsHead.cast<float>()).cast<double>();
    } else {
        // if not, find max amplitudes in channels
        VectorXi vecChIdcs(iNumCoils);

        for (int j = 0; j < iNumCoils; j++) {
            int iChIdx = 0;
            VectorXd::Index indMax;
            matAmplitudes.col(j).maxCoeff(&indMax);
            if(indMax < m_sensors.ncoils()) {
                iChIdx = indMax;
            }
            vecChIdcs(j) = iChIdx;
        }
        // and go 3 cm inwards from max channels
        for (int j = 0; j < vecChIdcs.rows(); ++j) {
            if(vecChIdcs(j) < m_sensors.ncoils()) {
                Vector3d r0 = m_sensors.r0(vecChIdcs(j));
                Vector3d ez = m_sensors.ez(vecChIdcs(j));
                matCoilsSeed.row(j) = (-1 * ez * 0.03 + r0);
            }
        }
    }

    // dipole fit
    return dipfit(matCoilsSeed,
                  m_sensors,
                  matAmplitudes,
                  iNumCoils,
                  matProjectors,
                  iMaxIterations,
                  fAbortError);

}

//=============================================================================================================

Eigen::VectorXd HPIFit::computeGoF(const Eigen::VectorXd& vecDipFitError)
{
    VectorXd vecGoF(vecDipFitError.size());
    for(int i = 0; i < vecDipFitError.size(); ++i) {
        vecGoF(i) = 1 - vecDipFitError(i);
    }
    return vecGoF;
}

//=============================================================================================================

FIFFLIB::FiffCoordTrans HPIFit::computeDeviceHeadTransformation(const Eigen::MatrixXd& matCoilsDev,
                                                                const Eigen::MatrixXd& matCoilsHead)
{
    MatrixXd matTrans = computeTransformation(matCoilsHead,matCoilsDev);
    return FiffCoordTrans::make(1,4,matTrans.cast<float>(),true);
}

//=============================================================================================================

QVector<double> HPIFit::computeEstimationError(const Eigen::MatrixXd& matCoilsDev,
                                               const Eigen::MatrixXd& matCoilsHead,
                                               const FIFFLIB::FiffCoordTrans& transDevHead)
{
    //Calculate Error
    MatrixXd matTemp = matCoilsDev;
    MatrixXd matTestPos = transDevHead.apply_trans(matTemp.cast<float>()).cast<double>();
    MatrixXd matDiffPos = matTestPos - matCoilsHead;

    // compute error
    int iNumCoils = matCoilsDev.rows();
    QVector<double> vecError(iNumCoils);
    for(int i = 0; i < matDiffPos.rows(); ++i) {
        vecError[i] = matDiffPos.row(i).norm();
    }
    return vecError;
}

//=============================================================================================================

FIFFLIB::FiffDigPointSet HPIFit::getFittedPointSet(const Eigen::MatrixXd& matCoilsDev)
{
    FiffDigPointSet fittedPointSet;
    int iNumCoils = matCoilsDev.rows();

    for(int i = 0; i < iNumCoils; ++i) {
        FiffDigPoint digPoint;
        digPoint.kind = FIFFV_POINT_EEG; //Store as EEG so they have a different color
        digPoint.ident = i;
        digPoint.r[0] = matCoilsDev(i,0);
        digPoint.r[1] = matCoilsDev(i,1);
        digPoint.r[2] = matCoilsDev(i,2);

        fittedPointSet << digPoint;
    }
    return fittedPointSet;
}


//=============================================================================================================

std::vector<int> HPIFit::findCoilOrder(const MatrixXd& matCoilsDev,
                                       const MatrixXd& matCoilsHead)
{
    // extract digitized and fitted coils
    MatrixXd matDigTemp = matCoilsHead;
    MatrixXd matCoilTemp = matCoilsDev;
    int iNumCoils = matCoilsDev.rows();

    std::vector<int> vecOrder(iNumCoils);
    std::iota(vecOrder.begin(), vecOrder.end(), 0);;

    // maximum 10 mm mean error
    double dErrorMin = 0.010;
    double dErrorActual = 0.0;
    double dErrorBest = dErrorMin;

    MatrixXd matTrans(4,4);
    std::vector<int> vecOrderBest = vecOrder;

    bool bSuccess = false;
    // permutation
    do {
        for(int i = 0; i < iNumCoils; i++) {
            matCoilTemp.row(i) =  matCoilsDev.row(vecOrder[i]);
        }
        matTrans = computeTransformation(matCoilsHead,matCoilTemp);
        dErrorActual = objectTrans(matCoilsHead,matCoilTemp,matTrans);
        if(dErrorActual < dErrorMin && dErrorActual < dErrorBest) {
            // exit
            dErrorBest = dErrorActual;
            vecOrderBest = vecOrder;
            bSuccess = true;
        }
    } while (std::next_permutation(vecOrder.begin(), vecOrder.end()));
    return vecOrderBest;
}

//=============================================================================================================

Eigen::MatrixXd HPIFit::order(const std::vector<int>& vecOrder,
                              const Eigen::MatrixXd& matToOrder)
{
    int iNumCoils = vecOrder.size();
    MatrixXd matToOrderTemp = matToOrder;

    for(int i = 0; i < iNumCoils; i++) {
        matToOrderTemp.row(i) =  matToOrder.row(vecOrder[i]);
    }
    return matToOrderTemp;
}

//=============================================================================================================

QVector<int> HPIFit::order(const std::vector<int>& vecOrder,
                           const QVector<int>& vecToOrder)
{
    int iNumCoils = vecOrder.size();
    QVector<int> vecToOrderTemp = vecToOrder;

    for(int i = 0; i < iNumCoils; i++) {
        vecToOrderTemp[i] =  vecToOrder[vecOrder[i]];
    }
    return vecToOrderTemp;
}

//=============================================================================================================

CoilParam HPIFit::dipfit(const MatrixXd matCoilsSeed,
                         const SensorSet& sensors,
                         const MatrixXd& matData,
                         const int iNumCoils,
                         const MatrixXd& matProjectors,
                         const int iMaxIterations,
                         const float fAbortError)
{
    //Do this in conncurrent mode
    //Generate QList structure which can be handled by the QConcurrent framework
    QList<HPIFitData> lCoilData;

    for(qint32 i = 0; i < iNumCoils; ++i) {
        HPIFitData coilData;
        coilData.m_coilPos = matCoilsSeed.row(i);
        coilData.m_sensorData = matData.col(i);
        coilData.m_sensors = sensors;
        coilData.m_matProjector = matProjectors;
        coilData.m_iMaxIterations = iMaxIterations;
        coilData.m_fAbortError = fAbortError;

        lCoilData.append(coilData);
    }
    //Do the concurrent filtering
    CoilParam coil(iNumCoils);

    if(!lCoilData.isEmpty()) {
//        //Do sequential
//        for(int l = 0; l < lCoilData.size(); ++l) {
//            doDipfitConcurrent(lCoilData[l]);
//        }

        //Do concurrent
        QFuture<void> future = QtConcurrent::map(lCoilData,
                                                 &HPIFitData::doDipfitConcurrent);
        future.waitForFinished();


        //Transform results to final coil information
        for(qint32 i = 0; i < lCoilData.size(); ++i) {
            coil.pos.row(i) = lCoilData.at(i).m_coilPos;
            coil.mom = lCoilData.at(i).m_errorInfo.moment.transpose();
            coil.dpfiterror(i) = lCoilData.at(i).m_errorInfo.error;
            coil.dpfitnumitr(i) = lCoilData.at(i).m_errorInfo.numIterations;

            //std::cout<<std::endl<< "HPIFit::dipfit - Itr steps for coil " << i << " =" <<coil.dpfitnumitr(i);
        }
    }
    return coil;
}

//=============================================================================================================

Eigen::Matrix4d HPIFit::computeTransformation(Eigen::MatrixXd matNH, MatrixXd matBT)
{
    MatrixXd matXdiff, matYdiff, matZdiff, matC, matQ;
    Matrix4d matTransFinal = Matrix4d::Identity(4,4);
    Matrix4d matRot = Matrix4d::Zero(4,4);
    Matrix4d matTrans = Matrix4d::Identity(4,4);
    double dMeanX,dMeanY,dMeanZ,dNormf;

    for(int i = 0; i < 15; ++i) {
        // Calculate mean translation for all points -> centroid of both data sets
        matXdiff = matNH.col(0) - matBT.col(0);
        matYdiff = matNH.col(1) - matBT.col(1);
        matZdiff = matNH.col(2) - matBT.col(2);

        dMeanX = matXdiff.mean();
        dMeanY = matYdiff.mean();
        dMeanZ = matZdiff.mean();

        // Apply translation -> bring both data sets to the same center location
        for (int j = 0; j < matBT.rows(); ++j) {
            matBT(j,0) = matBT(j,0) + dMeanX;
            matBT(j,1) = matBT(j,1) + dMeanY;
            matBT(j,2) = matBT(j,2) + dMeanZ;
        }

        // Estimate rotation component
        matC = matBT.transpose() * matNH;

        JacobiSVD< MatrixXd > svd(matC ,Eigen::ComputeThinU | ComputeThinV);

        matQ = svd.matrixU() * svd.matrixV().transpose();

        //Handle special reflection case
        if(matQ.determinant() < 0) {
            matQ(0,2) = matQ(0,2) * -1;
            matQ(1,2) = matQ(1,2) * -1;
            matQ(2,2) = matQ(2,2) * -1;
        }

        // Apply rotation on translated points
        matBT = matBT * matQ;

        // Calculate GOF
        dNormf = (matNH.transpose()-matBT.transpose()).norm();

        // Store rotation part to transformation matrix
        matRot(3,3) = 1;
        for(int j = 0; j < 3; ++j) {
            for(int k = 0; k < 3; ++k) {
                matRot(j,k) = matQ(k,j);
            }
        }

        // Store translation part to transformation matrix
        matTrans(0,3) = dMeanX;
        matTrans(1,3) = dMeanY;
        matTrans(2,3) = dMeanZ;

        // Safe rotation and translation to final matrix for next iteration step
        // This step is safe to do since we change one of the input point sets (matBT)
        // ToDo: Replace this for loop with a least square solution process
        matTransFinal = matRot * matTrans * matTransFinal;
    }
    return matTransFinal;
}

//=============================================================================================================

void HPIFit::storeHeadPosition(float fTime,
                               const Eigen::MatrixXf& transDevHead,
                               Eigen::MatrixXd& matPosition,
                               const Eigen::VectorXd& vecGoF,
                               const QVector<double>& vecError)

{
    Matrix3f matRot = transDevHead.block(0,0,3,3);

    Eigen::Quaternionf quatHPI(matRot);
    double dError = std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();     // HPI estimation Error

    matPosition.conservativeResize(matPosition.rows()+1, 10);
    matPosition(matPosition.rows()-1,0) = fTime;
    matPosition(matPosition.rows()-1,1) = quatHPI.x();
    matPosition(matPosition.rows()-1,2) = quatHPI.y();
    matPosition(matPosition.rows()-1,3) = quatHPI.z();
    matPosition(matPosition.rows()-1,4) = transDevHead(0,3);
    matPosition(matPosition.rows()-1,5) = transDevHead(1,3);
    matPosition(matPosition.rows()-1,6) = transDevHead(2,3);
    matPosition(matPosition.rows()-1,7) = vecGoF.mean();
    matPosition(matPosition.rows()-1,8) = dError;
    matPosition(matPosition.rows()-1,9) = 0;
}

//=============================================================================================================

double HPIFit::objectTrans(const MatrixXd& matHeadCoil,
                           const MatrixXd& matCoil,
                           const MatrixXd& matTrans)
{
    // Compute the fiducial registration error - the lower, the better.
    int iNumCoils = matHeadCoil.rows();
    MatrixXd matTemp = matCoil;

    // homogeneous coordinates
    matTemp.conservativeResize(matCoil.rows(),matCoil.cols()+1);
    matTemp.block(0,3,iNumCoils,1).setOnes();
    matTemp.transposeInPlace();

    // apply transformation
    MatrixXd matTestPos = matTrans * matTemp;

    // remove
    MatrixXd matDiff = matTestPos.block(0,0,3,iNumCoils) - matHeadCoil.transpose();
    VectorXd vecError = matDiff.colwise().norm();

    // compute error
    double dError = matDiff.colwise().norm().mean();;

    return dError;
}
