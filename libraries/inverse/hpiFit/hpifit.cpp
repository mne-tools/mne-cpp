//=============================================================================================================
/**
 * @file     hpifit.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
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

HPIFit::HPIFit()
{

}

//=============================================================================================================

HPIFit::HPIFit(FiffInfo::SPtr pFiffInfo)
{
    // init member variables
    m_HpiDataUpdater = HpiDataUpdater(pFiffInfo);
    m_lChannels = m_HpiDataUpdater.getChannels();
    m_sensors = SensorSet();
    m_sensors.updateSensorSet(m_lChannels,2);
    m_signalModel = SignalModel();
}

//=============================================================================================================

void HPIFit::fit(const MatrixXd& matData,
                 const MatrixXd& matProjectors,
                 const ModelParameters& modelParameters,
                 HpiFitResult& hpiFitResult)
{
    // TODO: Check for dimensions

    // prepare data
    m_HpiDataUpdater.prepareDataAndProjectors(matData,matProjectors);
    const auto& matProjectedData = m_HpiDataUpdater.getProjectedData();
    const auto& matPreparedProjector = m_HpiDataUpdater.getProjectors();
    const auto& matCoilsHead = m_HpiDataUpdater.getHpiDigitizer();

    MatrixXd matAmplitudes;
    computeAmplitudes(matProjectedData,
                      modelParameters,
                      matAmplitudes);

    int iNumCoils = modelParameters.vecHpiFreqs.size();
    MatrixXd matCoilPos = MatrixXd::Zero(iNumCoils,3);
    computeCoilLocation(matAmplitudes,
                        matPreparedProjector,
                        hpiFitResult.devHeadTrans,
                        hpiFitResult.errorDistances,
                        matCoilsHead,
                        matCoilPos,
                        hpiFitResult.GoF);

    computeHeadPosition(matCoilPos,
                        matCoilsHead,
                        hpiFitResult.devHeadTrans,
                        hpiFitResult.errorDistances,
                        hpiFitResult.fittedCoils);
}

//=============================================================================================================

void HPIFit::computeAmplitudes(const Eigen::MatrixXd& matProjectedData,
                               const ModelParameters& modelParameters,
                               Eigen::MatrixXd& matAmplitudes)
{
    //Check if data was passed
    if(matProjectedData.rows() == 0 || matProjectedData.cols() == 0 ) {
        std::cout<<std::endl<< "HPIFit::computeAmplitudes - No data passed. Returning.";
        return;
    }

    // fit model
    MatrixXd matTopo = m_signalModel.fitData(modelParameters,matProjectedData);
    matTopo.transposeInPlace();

    // split into sine and cosine amplitudes
    int iNumCoils = modelParameters.vecHpiFreqs.size();

    MatrixXd matAmp(matProjectedData.cols(), iNumCoils);   // sine part
    MatrixXd matAmpC(matProjectedData.cols(), iNumCoils);  // cosine part

    matAmp = matTopo.leftCols(iNumCoils);
    matAmpC = matTopo.middleCols(iNumCoils,iNumCoils);

    // Select sine or cosine component depending on their contributions to the amplitudes
    for(int j = 0; j < iNumCoils; ++j) {
        float fNS = 0.0;
        float fNC = 0.0;
        fNS = matAmp.col(j).array().square().sum();
        fNC = matAmpC.col(j).array().square().sum();
        if(fNC > fNS) {
            matAmp.col(j) = matAmpC.col(j);
        }
    }

    // return data
    matAmplitudes = std::move(matAmp);
}

//=============================================================================================================

void HPIFit::computeCoilLocation(const Eigen::MatrixXd& matAmplitudes,
                                 const MatrixXd& matProjectors,
                                 const FIFFLIB::FiffCoordTrans& transDevHead,
                                 const QVector<double>& vecError,
                                 const MatrixXd& matCoilsHead,
                                 Eigen::MatrixXd& matCoilLoc,
                                 Eigen::VectorXd& vecGoF,
                                 const int iMaxIterations,
                                 const float fAbortError)
{
    int iNumCoils = matAmplitudes.cols();
    MatrixXd matCoilPos = MatrixXd::Zero(iNumCoils,3);

    double dError = std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();

    if(transDevHead.trans != MatrixXd::Identity(4,4).cast<float>() && dError < 0.010) {
        // if good last fit, use old trafo
        matCoilPos = transDevHead.apply_inverse_trans(matCoilsHead.cast<float>()).cast<double>();
    } else {
        // if not, find max amplitudes in channels
        VectorXi vecChIdcs(iNumCoils);

        for (int j = 0; j < iNumCoils; j++) {
            int iChIdx = 0;
            VectorXd::Index indMax;
            matAmplitudes.col(j).maxCoeff(&indMax);
            if(indMax < m_lChannels.size()) {
                iChIdx = indMax;
            }
            vecChIdcs(j) = iChIdx;
        }
        // and go 3 cm inwards from max channels
        for (int j = 0; j < vecChIdcs.rows(); ++j) {
            if(vecChIdcs(j) < m_lChannels.size()) {
                Vector3f r0 = m_lChannels.at(vecChIdcs(j)).chpos.r0;
                matCoilPos.row(j) = (-1 * m_lChannels.at(vecChIdcs(j)).chpos.ez * 0.03 + r0).cast<double>();
            }
        }
    }

    // init coil parameters
    struct CoilParam coil;
    coil.pos = matCoilPos;
    coil.mom = MatrixXd::Zero(iNumCoils,3);
    coil.dpfiterror = VectorXd::Zero(iNumCoils);
    coil.dpfitnumitr = VectorXd::Zero(iNumCoils);

    // dipole fit
    coil = dipfit(coil,
                  m_sensors,
                  matAmplitudes,
                  iNumCoils,
                  matProjectors,
                  iMaxIterations,
                  fAbortError);

    // return data
    vecGoF = coil.dpfiterror;
    for(int i = 0; i < vecGoF.size(); ++i) {
        vecGoF(i) = 1 - vecGoF(i);
    }

    matCoilLoc = coil.pos;
}

//=============================================================================================================

void HPIFit::computeHeadPosition(const Eigen::MatrixXd& matCoilPos,
                                 const Eigen::MatrixXd& matCoilsHead,
                                 FIFFLIB::FiffCoordTrans& transDevHead,
                                 QVector<double> &vecError,
                                 FIFFLIB::FiffDigPointSet& fittedPointSet)
{

    // prepare dropping coils
    MatrixXd matTrans(4,4);
    matTrans = computeTransformation(matCoilsHead,matCoilPos);

    // Store the final result to fiff info
    // Set final device/head matrix and its inverse to the fiff info
    transDevHead = FiffCoordTrans::make(1,4,matTrans.cast<float>(),true);

    //Calculate Error
    MatrixXd matTemp = matCoilPos;
    MatrixXd matTestPos = transDevHead.apply_trans(matTemp.cast<float>()).cast<double>();
    MatrixXd matDiffPos = matTestPos - matCoilsHead;

    // compute error
    int iNumCoils = matCoilPos.rows();
    vecError = QVector<double>(iNumCoils);
    for(int i = 0; i < matDiffPos.rows(); ++i) {
        vecError[i] = matDiffPos.row(i).norm();
    }

    // sanity
    fittedPointSet.clear();

    // Generate final fitted points and store in digitizer set
    for(int i = 0; i < iNumCoils; ++i) {
        FiffDigPoint digPoint;
        digPoint.kind = FIFFV_POINT_EEG; //Store as EEG so they have a different color
        digPoint.ident = i;
        digPoint.r[0] = matCoilPos(i,0);
        digPoint.r[1] = matCoilPos(i,1);
        digPoint.r[2] = matCoilPos(i,2);

        fittedPointSet << digPoint;
    }
}

//=============================================================================================================

void HPIFit::findOrder(const MatrixXd& matData,
                       const MatrixXd& matProjectors,
                       QVector<int>& vecFreqs,
                       const FiffInfo::SPtr pFiffInfo)
{
    // predefinitions
    QVector<double> vecError;
    VectorXd vecGoF;
    int iNumCoils = vecFreqs.length();

    // compute amplitudes
    MatrixXd matAmplitudes;

    ModelParameters modelParameters;
    modelParameters.vecHpiFreqs = vecFreqs;
    modelParameters.iLineFreq = pFiffInfo->linefreq;
    modelParameters.iSampleFreq = pFiffInfo->sfreq;
    modelParameters.bBasic = false;

    // prepare data
    m_HpiDataUpdater.prepareDataAndProjectors(matData,matProjectors);
    const auto& matProjectedData = m_HpiDataUpdater.getProjectedData();
    const auto& matPreparedProjector = m_HpiDataUpdater.getProjectors();
    const auto& matCoilsHead = m_HpiDataUpdater.getHpiDigitizer();

    computeAmplitudes(matProjectedData,
                      modelParameters,
                      matAmplitudes);

    // compute coil position
    MatrixXd matCoilsDev = MatrixXd::Zero(iNumCoils,3);
    computeCoilLocation(matAmplitudes,
                        matPreparedProjector,
                        pFiffInfo->dev_head_t,
                        vecError,
                        matCoilsHead,
                        matCoilsDev,
                        vecGoF);

    // extract digitized and fitted coils
    QVector<int> vecFreqsOrder = vecFreqs;
    MatrixXd matDigTemp = matCoilsHead;
    MatrixXd matCoilTemp = matCoilsDev;

    std::vector<int> vecOrder(iNumCoils);
    std::iota(vecOrder.begin(), vecOrder.end(), 0);;

    // maximum 10 mm mean error
    double dErrorMin = 0.010;
    double dErrorActual = 0.0;
    double dErrorBest = dErrorMin;

    MatrixXd matTrans(4,4);
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
            for(int i = 0; i < iNumCoils; i++) {
                vecFreqs[i] =  vecFreqsOrder[vecOrder[i]];
            }
            dErrorBest = dErrorActual;
            bSuccess = true;
        }
    } while (std::next_permutation(vecOrder.begin(), vecOrder.end()) );
}

//=============================================================================================================

CoilParam HPIFit::dipfit(struct CoilParam coil,
                         const SensorSet& sensors,
                         const MatrixXd& matData,
                         int iNumCoils,
                         const MatrixXd& t_matProjectors,
                         int iMaxIterations,
                         float fAbortError)
{
    //Do this in conncurrent mode
    //Generate QList structure which can be handled by the QConcurrent framework
    QList<HPIFitData> lCoilData;

    for(qint32 i = 0; i < iNumCoils; ++i) {
        HPIFitData coilData;
        coilData.m_coilPos = coil.pos.row(i);
        coilData.m_sensorData = matData.col(i);
        coilData.m_sensors = sensors;
        coilData.m_matProjector = t_matProjectors;
        coilData.m_iMaxIterations = iMaxIterations;
        coilData.m_fAbortError = fAbortError;

        lCoilData.append(coilData);
    }
    //Do the concurrent filtering
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
    // Write quaternions and vecTime in position matrix. Format is the same like MaxFilter's .pos files.
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

MatrixXd HPIFit::dropCoils(const VectorXd vecGoF,
                           const MatrixXd matCoil,
                           const MatrixXd matHeadCoil,
                           VectorXi& vecInd)
{
    int iNumCoils = matCoil.rows();
    int iNumUsed = iNumCoils;
    VectorXi vecIndDrop = vecInd;
    // initial transformation
    MatrixXd matTransNew = MatrixXd(4,4);
    MatrixXd matTrans = computeTransformation(matHeadCoil,matCoil);
    double dErrorActual = objectTrans(matHeadCoil,matCoil,matTrans); // fiducial registration error
    double dErrorActualNew = 0.0;

    // resize elements
    MatrixXd matCoilDrop = matCoil;
    MatrixXd matHeadDrop = matHeadCoil;
    VectorXd vecGofDrop = vecGoF;
    matCoilDrop.conservativeResize(iNumUsed-1,3);
    matHeadDrop.conservativeResize(iNumUsed-1,3);
    vecGofDrop.conservativeResize(iNumUsed-1,1);
    vecIndDrop.conservativeResize(iNumUsed-1,1);

    // Drop Coils recursively
    if(iNumUsed > 3) {
        int iR = 0;
        // do not copy row coresponding to lowest gof -> drop it
        for(int i = 0; i < iNumUsed; i++){
            if(vecGoF[i] != vecGoF.minCoeff()) {
                matHeadDrop.row(iR) = matHeadCoil.row(i);
                matCoilDrop.row(iR) = matCoil.row(i);
                vecGofDrop(iR) = vecGoF(i);
                vecIndDrop(iR) = vecInd(i);
                iR++;
            } else {
                qInfo() << "Dropped coil: " << i << " with GoF: "  << vecGoF[i];
            }
        }
        // iterative update of transformation
        matTransNew = dropCoils(vecGofDrop, matCoilDrop, matHeadDrop, vecIndDrop);

        // fiducial registration error
        dErrorActualNew = objectTrans(matHeadDrop,matCoilDrop,matTransNew);

        iNumUsed--;

        // update if dErrorActualNew < dErrorActualOld;
        if(dErrorActualNew < dErrorActual) {
            matTrans = matTransNew;
            vecInd = vecIndDrop;
        }
    }
    return matTrans;
}

//=============================================================================================================

double HPIFit::objectTrans(const MatrixXd matHeadCoil,
                           const MatrixXd matCoil,
                           const MatrixXd matTrans)
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
    // std::cout << vecError.transpose() << std::endl;
    // compute error
    double dError = matDiff.colwise().norm().mean();;

    return dError;
}
