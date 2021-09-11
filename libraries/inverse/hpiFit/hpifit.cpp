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

#include <utils/ioutils.h>
#include <utils/mnemath.h>

#include <iostream>
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

HPIFit::HPIFit(FiffInfo::SPtr pFiffInfo,
               bool bDoFastFit)
    : m_bDoFastFit(bDoFastFit)
{
    // init member variables
    m_lChannels = QList<FIFFLIB::FiffChInfo>();
    m_vecInnerind = QVector<int>();
    m_sensors = SensorSet();
    m_lBads = pFiffInfo->bads;
    m_matModel = MatrixXd(0,0);
    m_vecFreqs = QVector<int>();

    updateChannels(pFiffInfo);
    updateSensor();
}

//=============================================================================================================

void HPIFit::fitHPI(const MatrixXd& t_mat,
                    const MatrixXd& t_matProjectors,
                    FiffCoordTrans& transDevHead,
                    const QVector<int>& vecFreqs,
                    QVector<double>& vecError,
                    VectorXd& vecGoF,
                    FiffDigPointSet& fittedPointSet,
                    FiffInfo::SPtr pFiffInfo,
                    bool bDoDebug,
                    const QString& sHPIResourceDir,
                    int iMaxIterations,
                    float fAbortError)
{
    //Check if data was passed
    if(t_mat.rows() == 0 || t_mat.cols() == 0 ) {
        std::cout<<std::endl<< "HPIFit::fitHPI - No data passed. Returning.";
        return;
    }
    //Check if projector was passed
    if(t_matProjectors.rows() == 0 || t_matProjectors.cols() == 0 ) {
        std::cout<<std::endl<< "HPIFit::fitHPI - No projector passed. Returning.";
        return;
    }

    bool bUpdateModel = false;

    // check if bads have changed and update coils/channellist if so
    if(!(m_lBads == pFiffInfo->bads) || m_lChannels.isEmpty()) {
        m_lBads = pFiffInfo->bads;
        updateChannels(pFiffInfo);
        updateSensor();
        bUpdateModel = true;
    }

    if(m_lChannels.isEmpty()) {
        qWarning() << "HPIFit::fitHPI - Channel list is empty. Returning.";
        return;
    }

    // check if we have to update the model
    if(bUpdateModel || (m_matModel.rows() == 0) || (m_vecFreqs != vecFreqs) || (t_mat.cols() != m_matModel.cols())) {
        updateModel(pFiffInfo->sfreq, t_mat.cols(), pFiffInfo->linefreq, vecFreqs);
        m_vecFreqs = vecFreqs;
        bUpdateModel = false;
    }

    // Make sure the fitted digitzers are empty
    fittedPointSet.clear();

    // init coil parameters
    struct CoilParam coil;

    //Get HPI coils from digitizers and set number of coils
    int iNumCoils = 0;
    QList<FiffDigPoint> lHPIPoints;

    for(int i = 0; i < pFiffInfo->dig.size(); ++i) {
        if(pFiffInfo->dig[i].kind == FIFFV_POINT_HPI) {
            iNumCoils++;
            lHPIPoints.append(pFiffInfo->dig[i]);
        }
    }

    //Set coil frequencies
    VectorXd vecCoilfreq(iNumCoils);

    if(vecFreqs.size() >= iNumCoils) {
        for(int i = 0; i < iNumCoils; ++i) {
            vecCoilfreq[i] = vecFreqs.at(i);
            //std::cout<<std::endl << vecCoilfreq[i] << "Hz";
        }
    } else {
        std::cout<<std::endl<< "HPIFit::fitHPI - Not enough coil frequencies specified. Returning.";
        return;
    }

    // Initialize HPI coils location and moment
    coil.pos = MatrixXd::Zero(iNumCoils,3);
    coil.mom = MatrixXd::Zero(iNumCoils,3);
    coil.dpfiterror = VectorXd::Zero(iNumCoils);
    coil.dpfitnumitr = VectorXd::Zero(iNumCoils);

    // Create digitized HPI coil position matrix
    MatrixXd matHeadHPI(iNumCoils,3);

    // check the pFiffInfo->dig information. If dig is empty, set the matHeadHPI is 0;
    if (lHPIPoints.size() > 0) {
        for (int i = 0; i < lHPIPoints.size(); ++i) {
            matHeadHPI(i,0) = lHPIPoints.at(i).r[0];
            matHeadHPI(i,1) = lHPIPoints.at(i).r[1];
            matHeadHPI(i,2) = lHPIPoints.at(i).r[2];
        }
    } else {
        matHeadHPI.fill(0);
    }

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vecInnerind.size(),t_matProjectors.cols());
    MatrixXd matProjectorsInnerind(m_vecInnerind.size(),m_vecInnerind.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = t_matProjectors.row(m_vecInnerind.at(i));
    }

    for (int i = 0; i < matProjectorsInnerind.cols(); ++i) {
        matProjectorsInnerind.col(i) = matProjectorsRows.col(m_vecInnerind.at(i));
    }

    // Get the data from inner layer channels
    MatrixXd matInnerdata(m_vecInnerind.size(), t_mat.cols());

    for(int j = 0; j < m_vecInnerind.size(); ++j) {
        matInnerdata.row(j) << t_mat.row(m_vecInnerind[j]);
    }

    // Calculate topo
    MatrixXd matTopo;
    MatrixXd matAmp(m_vecInnerind.size(), iNumCoils);
    MatrixXd matAmpC(m_vecInnerind.size(), iNumCoils);

    matTopo = m_matModel * matInnerdata.transpose(); // topo: # of good inner channel x 8

    if(m_bDoFastFit) {
        // Select sine or cosine component depending on the relative size
        matTopo.transposeInPlace();
        matAmp = matTopo.leftCols(iNumCoils);
        matAmpC = matTopo.rightCols(iNumCoils);
        for(int j = 0; j < iNumCoils; ++j) {
           float fNS = 0.0;
           float fNC = 0.0;
           fNS = matAmp.col(j).array().square().sum();
           fNC = matAmpC.col(j).array().square().sum();
           if(fNC > fNS) {
               matAmp.col(j) = matAmpC.col(j);
           }
        }
    } else {
        // estimate the sinusoid phase
        for(int i = 0; i < iNumCoils; ++i) {
            int from = 2*i;
            MatrixXd m = matTopo.block(from,0,2,matTopo.cols());
            JacobiSVD<MatrixXd> svd(m, ComputeThinU | ComputeThinV);
            matAmp.col(i) = svd.singularValues()(0) * svd.matrixV().col(0);
        }
    }

    //Find good seed point/starting point for the coil position in 3D space
    //Find biggest amplitude per pickup coil (sensor) and store corresponding sensor channel index
    VectorXi vecChIdcs(iNumCoils);

    for (int j = 0; j < iNumCoils; j++) {
        int iChIdx = 0;
        VectorXd::Index indMax;
        matAmp.col(j).maxCoeff(&indMax);
        if(indMax < m_vecInnerind.size()) {
            iChIdx = m_vecInnerind.at(indMax);
        }
        vecChIdcs(j) = iChIdx;
    }

    vecError.resize(iNumCoils);
    double dError = std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();
    MatrixXd matCoilPos = MatrixXd::Zero(iNumCoils,3);

    // Generate seed point by projection the found channel position 3cm inwards if previous transDevHead is identity or bad fit
    if(transDevHead.trans == MatrixXd::Identity(4,4).cast<float>() || dError > 0.010) {
        for (int j = 0; j < vecChIdcs.rows(); ++j) {
            if(vecChIdcs(j) < pFiffInfo->chs.size()) {
                Vector3f r0 = pFiffInfo->chs.at(vecChIdcs(j)).chpos.r0;
                matCoilPos.row(j) = (-1 * pFiffInfo->chs.at(vecChIdcs(j)).chpos.ez * 0.03 + r0).cast<double>();
            }
        }
    } else {
        matCoilPos = transDevHead.apply_inverse_trans(matHeadHPI.cast<float>()).cast<double>();
    }

    coil.pos = matCoilPos;

    // Perform actual localization
    coil = dipfit(coil,
                  m_sensors,
                  matAmp,
                  iNumCoils,
                  matProjectorsInnerind,
                  iMaxIterations,
                  fAbortError);

    Matrix4d matTrans = computeTransformation(matHeadHPI, coil.pos);
    transDevHead.setTransform(1,4,matTrans.cast<float>());

    //Calculate Error
    MatrixXd matTemp = coil.pos;
    MatrixXd matTestPos = transDevHead.apply_trans(matTemp.cast<float>()).cast<double>();
    MatrixXd matDiffPos = matTestPos - matHeadHPI;

    for(int i = 0; i < matDiffPos.rows(); ++i) {
        vecError[i] = matDiffPos.row(i).norm();
    }

    // store Goodness of Fit
    vecGoF = coil.dpfiterror;
    for(int i = 0; i < vecGoF.size(); ++i) {
        vecGoF(i) = 1 - vecGoF(i);
    }

    //Generate final fitted points and store in digitizer set
    for(int i = 0; i < coil.pos.rows(); ++i) {
        FiffDigPoint digPoint;
        digPoint.kind = FIFFV_POINT_EEG; //Store as EEG so they have a different color
        digPoint.ident = i;
        digPoint.r[0] = coil.pos(i,0);
        digPoint.r[1] = coil.pos(i,1);
        digPoint.r[2] = coil.pos(i,2);

        fittedPointSet << digPoint;
    }

    if(bDoDebug) {
        // DEBUG HPI fitting and write debug results
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - dpfiterror" << coil.dpfiterror << std::endl << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Initial seed point for HPI coils" << std::endl << matCoilPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - temp" << std::endl << matTemp << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - testPos" << std::endl << matTestPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Diff fitted - original" << std::endl << matDiffPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - dev/head trans" << std::endl << matTrans << std::endl;

        QString sTimeStamp = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");

        if(!QDir(sHPIResourceDir).exists()) {
            QDir().mkdir(sHPIResourceDir);
        }

        UTILSLIB::IOUtils::write_eigen_matrix(matCoilPos, QString("%1/%2_coilPosSeed_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(vecGoF, QString("%1/%2_gof_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(coil.pos, QString("%1/%2_coilPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(matHeadHPI, QString("%1/%2_headHPI_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd testPosCut = matTestPos.transpose();//block(0,0,3,4);
        UTILSLIB::IOUtils::write_eigen_matrix(testPosCut, QString("%1/%2_testPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXi matIdx(vecChIdcs.rows(),1);
        matIdx.col(0) = vecChIdcs;
        UTILSLIB::IOUtils::write_eigen_matrix(matIdx, QString("%1/%2_idx_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd matCoilFreq(vecCoilfreq.rows(),1);
        matCoilFreq.col(0) = vecCoilfreq;
        UTILSLIB::IOUtils::write_eigen_matrix(matCoilFreq, QString("%1/%2_coilFreq_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(matDiffPos, QString("%1/%2_diffPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(matAmp, QString("%1/%2_amp_mat").arg(sHPIResourceDir).arg(sTimeStamp));
    }
}

//=============================================================================================================

void HPIFit::findOrder(const MatrixXd& t_mat,
                       const MatrixXd& t_matProjectors,
                       FiffCoordTrans& transDevHead,
                       QVector<int>& vecFreqs,
                       QVector<double>& vecError,
                       VectorXd& vecGoF,
                       FiffDigPointSet& fittedPointSet,
                       FiffInfo::SPtr pFiffInfo)
{
    // create temporary copies that are necessary to reset values that are passed to fitHpi()
    fittedPointSet.clear();
    transDevHead.clear();
    vecError.fill(0);

    FiffDigPointSet fittedPointSetTemp = fittedPointSet;
    FiffCoordTrans transDevHeadTemp = transDevHead;
    FiffInfo::SPtr pFiffInfoTemp = pFiffInfo;
    QVector<int> vecToOrder = vecFreqs;
    QVector<int> vecFreqTemp(vecFreqs.size());
    QVector<double> vecErrorTemp = vecError;
    VectorXd vecGoFTemp = vecGoF;
    bool bIdentity = false;

    MatrixXf matTrans = transDevHead.trans;
    if(transDevHead.trans == MatrixXf::Identity(4,4).cast<float>()) {
        // avoid identity since this leads to problems with this method in fitHpi.
        // the hpi fit is robust enough to handle bad starting points
        transDevHeadTemp.trans(3,0) = 0.000001;
        bIdentity = true;
    }

    // perform vecFreqs.size() hpi fits with same frequencies in each iteration
    for(int i = 0; i < vecFreqs.size(); i++){
        vecFreqTemp.fill(vecFreqs[i]);

        // hpi Fit
        fitHPI(t_mat, t_matProjectors, transDevHeadTemp, vecFreqTemp, vecErrorTemp, vecGoFTemp, fittedPointSetTemp, pFiffInfoTemp);

        // get location of maximum GoF -> correct assignment of coil - frequency
        VectorXd::Index indMax;
        vecGoFTemp.maxCoeff(&indMax);
        vecToOrder[indMax] = vecFreqs[i];

        // std::cout << vecGoFTemp[0] << " " << vecGoFTemp[1] << " " << vecGoFTemp[2] << " " << vecGoFTemp[3] << " " << std::endl;

        // reset values that are edidet by fitHpi()
        fittedPointSetTemp = fittedPointSet;
        pFiffInfoTemp = pFiffInfo;
        transDevHeadTemp = transDevHead;

        if(bIdentity) {
            transDevHeadTemp.trans(3,0) = 0.000001;
        }
        vecErrorTemp = vecError;
        vecGoFTemp = vecGoF;
    }
    // check if still all frequencies are represented and update model
    if(std::accumulate(vecFreqs.begin(), vecFreqs.end(), .0) ==  std::accumulate(vecToOrder.begin(), vecToOrder.end(), .0)) {
        vecFreqs = vecToOrder;
    } else {
        qWarning() << "HPIFit::findOrder: frequencie ordering went wrong";
    }
    qInfo() << "HPIFit::findOrder: vecFreqs = " << vecFreqs;
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

void HPIFit::createSensorSet(SensorSet& sensors,
                             QSharedPointer<FWDLIB::FwdCoilSet> coils)
{
    int iNchan = coils->ncoil;

    // init sensor struct
    int iNp = coils->coils[0]->np;
    sensors.w = RowVectorXd(iNchan*iNp);
    sensors.r0 = MatrixXd(iNchan,3);
    sensors.cosmag = MatrixXd(iNchan*iNp,3);
    sensors.rmag = MatrixXd(iNchan*iNp,3);
    sensors.ncoils = iNchan;
    sensors.tra = MatrixXd::Identity(iNchan,iNchan);
    sensors.np = iNp;

    for(int i = 0; i < iNchan; i++){
        FwdCoil* coil = (coils->coils[i]);
        MatrixXd matRmag = MatrixXd::Zero(iNp,3);
        MatrixXd matCosmag = MatrixXd::Zero(iNp,3);
        RowVectorXd vecW(iNp);

        sensors.r0(i,0) = coil->r0[0];
        sensors.r0(i,1) = coil->r0[1];
        sensors.r0(i,2) = coil->r0[2];

        for (int p = 0; p < iNp; p++){
            sensors.w(i*iNp+p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                matRmag(p,c)   = coil->rmag[p][c];
                matCosmag(p,c) = coil->cosmag[p][c];
            }
        }

        sensors.cosmag.block(i*iNp,0,iNp,3) = matCosmag;
        sensors.rmag.block(i*iNp,0,iNp,3) = matRmag;
    }
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

    double dError = std::accumulate(vecError.begin(), vecError.end(), .0) / vecError.size();     // HPI estimation Error
    Eigen::Quaternionf quatHPI(matRot);

//    qDebug() << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z ";
//    qDebug() << quatHPI.x() << quatHPI.y() << quatHPI.z() << transDevHead(0,3) << transDevHead(1,3) << transDevHead(2,3);

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

void HPIFit::updateSensor()
{
    // Create MEG-Coils and read data
    int iAcc = 2;
    int iNch = m_lChannels.size();

    if(iNch == 0) {
        return;
    }

    FiffCoordTransOld* t = NULL;

    if(m_pCoilTemplate.isNull()) {
        // read coil_def.dat
        QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
        m_pCoilTemplate = QSharedPointer<FWDLIB::FwdCoilSet>(FwdCoilSet::read_coil_defs(qPath));
    }

    // create sensor set
    m_pCoilMeg = QSharedPointer<FWDLIB::FwdCoilSet>(m_pCoilTemplate->create_meg_coils(m_lChannels, iNch, iAcc, t));
    createSensorSet(m_sensors, m_pCoilMeg);
}

//=============================================================================================================

void HPIFit::updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    // Get the indices of inner layer channels and exclude bad channels and create channellist
    int iNumCh = pFiffInfo->nchan;

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

    m_lBads = pFiffInfo->bads;
}

//=============================================================================================================

void HPIFit::updateModel(const int iSamF,
                         const int iSamLoc,
                         int iLineF,
                         const QVector<int>& vecFreqs)
{
    int iNumCoils = vecFreqs.size();
    MatrixXd matSimsig;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    if(m_bDoFastFit){
        // Generate simulated data Matrix
        matSimsig.conservativeResize(iSamLoc,iNumCoils*2);

        for(int i = 0; i < iNumCoils; ++i) {
            matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
            matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
        }
        m_matModel = UTILSLIB::MNEMath::pinv(matSimsig);
        return;

    } else {
        // add linefreq + harmonics + DC part to model
        matSimsig.conservativeResize(iSamLoc,iNumCoils*4);
        for(int i = 0; i < iNumCoils; ++i) {
            matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
            matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
            matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*iLineF*i*vecTime.array());
            matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*iLineF*i*vecTime.array());
        }
        matSimsig.col(14) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
        matSimsig.col(15).fill(1);
    }
    m_matModel = UTILSLIB::MNEMath::pinv(matSimsig);

    // reorder for faster computation
    MatrixXd matTemp = m_matModel;
    RowVectorXi vecIndex(2*iNumCoils);
    vecIndex << 0,4,1,5,2,6,3,7;
    for(int i = 0; i < vecIndex.size(); ++i) {
        matTemp.row(i) = m_matModel.row(vecIndex(i));
    }
    m_matModel = matTemp;
}
