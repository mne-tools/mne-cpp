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

HPIFit::HPIFit()
{
    // init member variables
    m_lChannels = QList<FIFFLIB::FiffChInfo>();
    m_vecInnerind = QVector<int>();
    m_sensors = SensorSet();
    m_lBads = QList<QString>();
    m_matModel = MatrixXd(0,0);
    m_vecFreqs = QVector<int>();

    // read coil_def.dat
    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    m_pCoilTemplate = QSharedPointer<FWDLIB::FwdCoilSet>(FwdCoilSet::read_coil_defs(qPath));

}

//=============================================================================================================

HPIFit::HPIFit(FiffInfo::SPtr pFiffInfo)
{
    // init member variables
    m_lChannels = QList<FIFFLIB::FiffChInfo>();
    m_vecInnerind = QVector<int>();
    m_sensors = SensorSet();
    m_lBads = pFiffInfo->bads;
    m_matModel = MatrixXd(0,0);
    m_vecFreqs = QVector<int>();

    // read coil_def.dat
    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    m_pCoilTemplate = QSharedPointer<FWDLIB::FwdCoilSet>(FwdCoilSet::read_coil_defs(qPath));

    updateChannels(pFiffInfo);
    int iAcc = 2;
    updateSensor(iAcc);
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
                    bool bDrop,
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
        updateModel(pFiffInfo->sfreq, t_mat.cols(), pFiffInfo->linefreq, vecFreqs, m_bDoFastFit);
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

    // get gof
    vecGoF = coil.dpfiterror;
    for(int i = 0; i < vecGoF.size(); ++i) {
        vecGoF(i) = 1 - vecGoF(i);
    }

    // prepare dropping coils
    MatrixXd matTrans(4,4);
    VectorXi vecInd = VectorXi::LinSpaced(iNumCoils,0,iNumCoils);

    if((vecGoF.minCoeff() < 0.98) && bDrop) {
        // hard coded, can potentially be passed as variable
        matTrans = dropCoils(vecGoF,
                             coil.pos,
                             matHeadHPI,
                             vecInd);
    } else {
        matTrans = computeTransformation(matHeadHPI, coil.pos);
    }

    int iKeep = vecInd.size();
    MatrixXd matHeadHPITemp(iKeep,3);
    MatrixXd matCoilTemp(iKeep,3);
    VectorXd vecGofTemp(iKeep);
    for(int i = 0; i < iKeep; i++) {
        matHeadHPITemp.row(i) = matHeadHPI.row(vecInd(i));
        matCoilTemp.row(i) = coil.pos.row(vecInd(i));
        vecGofTemp(i) = vecGoF(vecInd(i));
    }

    coil.pos = matCoilTemp;
    matHeadHPI = matHeadHPITemp;
    vecGoF = vecGofTemp;

    //Eigen::Matrix4d matTrans = computeTransformation(coil.pos, matHeadHPI);

    // Store the final result to fiff info
    // Set final device/head matrix and its inverse to the fiff info
    transDevHead.from = 1;
    transDevHead.to = 4;
    transDevHead.trans = matTrans.cast<float>();

    // Also store the inverse
    transDevHead.invtrans = transDevHead.trans.inverse();

    //Calculate Error
    MatrixXd matTemp = coil.pos;
    MatrixXd matTestPos = transDevHead.apply_trans(matTemp.cast<float>()).cast<double>();
    MatrixXd matDiffPos = matTestPos - matHeadHPI;

    vecError = QVector<double>(iKeep);
    for(int i = 0; i < matDiffPos.cols(); ++i) {
        vecError[i] = matDiffPos.col(i).norm();
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

void HPIFit::computeAmplitudes(const Eigen::MatrixXd& matData,
                               const QVector<int>& vecFreqs,
                               const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                               Eigen::MatrixXd& matAmplitudes,
                               const int iLineFreq,
                               const bool bBasic)
{
    // meta data
    int iNumCoils = vecFreqs.size();

    //Check if data was passed
    if(matData.rows() == 0 || matData.cols() == 0 ) {
        std::cout<<std::endl<< "HPIFit::fitHPI - No data passed. Returning.";
        return;
    }

//    // check if number of freuencies matches data
//    if(iNumCoils != matData.cols()) {
//        std::cout<<std::endl<< "HPIFit::fitHPI - Not enough coil frequencies specified. Returning.";
//        return;
//    }

    // check if we need to update the model (bads, frequencies)
    if(!(m_lBads == pFiffInfo->bads) || m_lChannels.isEmpty() || m_matModel.rows()==0) {
        m_lBads = pFiffInfo->bads;
        updateChannels(pFiffInfo);
        updateSensor();
        updateModel(pFiffInfo->sfreq, matData.cols(), pFiffInfo->linefreq, vecFreqs, bBasic);
    }

    // extract data for channels to use
    MatrixXd matInnerdata(m_vecInnerind.size(), matData.cols());

    for(int j = 0; j < m_vecInnerind.size(); ++j) {
        matInnerdata.row(j) << matData.row(m_vecInnerind[j]);
    }

    // fit linear model
    MatrixXd matTopo;
    MatrixXd matAmp(m_vecInnerind.size(), iNumCoils);   // sine part
    MatrixXd matAmpC(m_vecInnerind.size(), iNumCoils);  // cosine part

    matTopo = m_matModel * matInnerdata.transpose();

    // select sine or cosine part

    if(bBasic) {
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

    // return data
    matAmplitudes = matAmp;
}

//=============================================================================================================

void HPIFit::computeHeadPos(const Eigen::MatrixXd& matCoilsDev,
                            const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                            FIFFLIB::FiffCoordTrans& transDevHead,
                            QVector<double> &vecError,
                            Eigen::VectorXd& vecGoF,
                            FIFFLIB::FiffDigPointSet& fittedPointSet,
                            bool bDrop)
{

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
    // clear points
    fittedPointSet.clear();
    transDevHead.clear();
    vecError.fill(0);

    // casual fit to get fitted coils
    fitHPI(t_mat, t_matProjectors, transDevHead, vecFreqs, vecError, vecGoF, fittedPointSet, pFiffInfo);

    // extract digitized and fitted coils
    int iNumCoils = vecFreqs.length();
    QVector<int> vecToOrder = vecFreqs;
    VectorXd vecOrder(iNumCoils);
    Vector4d vecInit(0,1,2,3);
    MatrixXd matCoil = MatrixXd::Zero(iNumCoils,3);
    MatrixXd matDig = MatrixXd::Zero(iNumCoils,3);
    VectorXd vecDist = VectorXd::Zero(iNumCoils);
    QList<FiffDigPoint> lHPIPoints;

    for(int i = 0; i < iNumCoils; ++i) {
        matCoil(i,0) = fittedPointSet[i].r[0];
        matCoil(i,1) = fittedPointSet[i].r[1];
        matCoil(i,2) = fittedPointSet[i].r[2];
    }

    for(int i = 0; i < pFiffInfo->dig.size(); ++i) {
        if(pFiffInfo->dig[i].kind == FIFFV_POINT_HPI) {
            lHPIPoints.append(pFiffInfo->dig[i]);
        }
    }

    for (int i = 0; i < lHPIPoints.size(); ++i) {
        matDig(i,0) = lHPIPoints.at(i).r[0];
        matDig(i,1) = lHPIPoints.at(i).r[1];
        matDig(i,2) = lHPIPoints.at(i).r[2];
    }

    // Find closest point to each dig and find with that the order

    for(int i = 0; i < iNumCoils; i++) {
        // distances from of fitted point to all digitized
        vecDist = (matCoil.rowwise() - matDig.row(i)).rowwise().norm();
        Eigen::MatrixXf::Index min_index;
        vecDist.minCoeff(&min_index);
        vecOrder(i) = vecInit(min_index);
        vecToOrder[i] = vecFreqs[min_index];
    }

    // check if still all frequencies are represented and update model
    if(std::accumulate(vecFreqs.begin(), vecFreqs.end(), .0) ==  std::accumulate(vecToOrder.begin(), vecToOrder.end(), .0)) {
        vecFreqs = vecToOrder;
    } else {
        qWarning() << "HPIFit::findOrder: frequency ordering was not succesfull.";
    }
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

    // don't take values below 0 into account (-1 means dropped coil)
    std::vector<double> vecTargetError;
    std::copy_if(vecError.begin(), vecError.end(), std::back_inserter(vecTargetError),[](double n ){ return  n >= 0.0;});
    double dError = std::accumulate(vecTargetError.begin(), vecTargetError.end(), .0) / vecTargetError.size();     // HPI estimation Error

    std::vector<double> vecTargetGof;
    std::copy_if(vecGoF.data(), vecGoF.data() + vecGoF.size(), std::back_inserter(vecTargetGof),[](double n ){ return  n >= 0.0;});
    double dGoF = std::accumulate(vecTargetGof.begin(), vecTargetGof.end(), .0) / vecTargetGof.size();     // HPI estimation Error

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
    matPosition(matPosition.rows()-1,7) = dGoF;
    matPosition(matPosition.rows()-1,8) = dError;
    matPosition(matPosition.rows()-1,9) = 0;
}

//=============================================================================================================

void HPIFit::updateSensor(const int iAcc)
{
    // Create MEG-Coils and read data
    int iNchan = m_lChannels.size();

    if(iNchan == 0) {
        std::cout<<std::endl<< "HPIFit::updateSensor - No channels. Returning.";
        return;
    }

    FiffCoordTransOld* t = NULL;

    // create sensor set
    QSharedPointer<FWDLIB::FwdCoilSet> pCoilMeg = QSharedPointer<FWDLIB::FwdCoilSet>(m_pCoilTemplate->create_meg_coils(m_lChannels, iNchan, iAcc, t));

    // init sensor struct
    int iNp = pCoilMeg->coils[0]->np;
    m_sensors.w = RowVectorXd(iNchan*iNp);
    m_sensors.r0 = MatrixXd(iNchan,3);
    m_sensors.cosmag = MatrixXd(iNchan*iNp,3);
    m_sensors.rmag = MatrixXd(iNchan*iNp,3);
    m_sensors.ncoils = iNchan;
    m_sensors.tra = MatrixXd::Identity(iNchan,iNchan);
    m_sensors.np = iNp;

    for(int i = 0; i < iNchan; i++){
        FwdCoil* coil = (pCoilMeg->coils[i]);
        MatrixXd matRmag = MatrixXd::Zero(iNp,3);
        MatrixXd matCosmag = MatrixXd::Zero(iNp,3);
        RowVectorXd vecW(iNp);

        m_sensors.r0(i,0) = coil->r0[0];
        m_sensors.r0(i,1) = coil->r0[1];
        m_sensors.r0(i,2) = coil->r0[2];

        for (int p = 0; p < iNp; p++){
            m_sensors.w(i*iNp+p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                matRmag(p,c)   = coil->rmag[p][c];
                matCosmag(p,c) = coil->cosmag[p][c];
            }
        }

        m_sensors.cosmag.block(i*iNp,0,iNp,3) = matCosmag;
        m_sensors.rmag.block(i*iNp,0,iNp,3) = matRmag;
    }
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
                         const int iLineF,
                         const QVector<int>& vecFreqs,
                         bool bBasic)
{
    int iNumCoils = vecFreqs.size();
    MatrixXd matSimsig;
    VectorXd vecTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    if(bBasic){
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
        matSimsig.conservativeResize(iSamLoc,iNumCoils*4+2);
        for(int i = 0; i < iNumCoils; ++i) {
            matSimsig.col(i) = sin(2*M_PI*vecFreqs[i]*vecTime.array());
            matSimsig.col(i+iNumCoils) = cos(2*M_PI*vecFreqs[i]*vecTime.array());
            matSimsig.col(i+2*iNumCoils) = sin(2*M_PI*iLineF*(i+1)*vecTime.array());
            matSimsig.col(i+3*iNumCoils) = cos(2*M_PI*iLineF*(i+1)*vecTime.array());
        }
        matSimsig.col(iNumCoils*4) = RowVectorXd::LinSpaced(iSamLoc, -0.5, 0.5);
        matSimsig.col(iNumCoils*4+1).fill(1);
    }
    m_matModel = UTILSLIB::MNEMath::pinv(matSimsig);
    MatrixXd matTemp = m_matModel;
    // reorder so that sin and cos with same freq. are next to each other
    int iC = 0;
    std::vector<int> vec(2*iNumCoils);
    for(int i = 1; i < 2*iNumCoils; i=i+2) {
        vec[i-1] = iC;
        vec[i] = iC+iNumCoils;
        iC++;
    }
    for(int i = 0; i < 2*iNumCoils; ++i) {
        matTemp.row(i) = m_matModel.row(vec[i]);
    }

    m_matModel = matTemp;
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
    double dFre = objectTrans(matHeadCoil,matCoil,matTrans); // fiducial registration error
    double dFreNew = 0.0;

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
        dFreNew = objectTrans(matHeadDrop,matCoilDrop,matTransNew);

        iNumUsed--;

        // update if dFreNew < dFreOld;
        if(dFreNew < dFre) {
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
    double dFRE = 0.0;
    MatrixXd matTemp = matCoil;

    // homogeneous coordinates
    matTemp.conservativeResize(matCoil.rows(),matCoil.cols()+1);
    matTemp.block(0,3,iNumCoils,1).setOnes();
    matTemp.transposeInPlace();

    // apply transformation
    MatrixXd matTestPos = matTrans * matTemp;

    // remove
    MatrixXd matDiff = matTestPos.block(0,0,3,iNumCoils) - matHeadCoil.transpose();
    dFRE = std::sqrt(1.0/(2.0*iNumCoils) * (matDiff*matDiff.transpose()).trace());
    qDebug() << dFRE;
    return dFRE;
}
