//=============================================================================================================
/**
 * @file     hpifit.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @version  dev
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

HPIFit::HPIFit(FiffInfo::SPtr pFiffInfo)
{
    // init channel list and lSensorSet
    m_lChannels = QList<FIFFLIB::FiffChInfo>();
    m_vInnerind = QVector<int>();
    m_sensors = Sensor ();
    m_lBads = pFiffInfo->bads;

    // init coils
    m_coilTemplate = NULL;
    m_coilMeg = NULL;

    updateChannels(pFiffInfo);
    updateCoils();

}

//=============================================================================================================

void HPIFit::fitHPI(const MatrixXd& t_mat,
                    const MatrixXd& t_matProjectors,
                    FiffCoordTrans& transDevHead,
                    const QVector<int>& vFreqs,
                    QVector<double>& vError,
                    VectorXd& vGoF,
                    FiffDigPointSet& fittedPointSet,
                    FiffInfo::SPtr pFiffInfo,
                    bool bDoDebug,
                    const QString& sHPIResourceDir)
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

    // check if bads have changed and update coils/channellist if so
    if(!(m_lBads == pFiffInfo->bads)) {
        m_lBads = pFiffInfo->bads;
        updateChannels(pFiffInfo);
        updateCoils();
    }

    // Make sure the fitted digitzers are empty
    fittedPointSet.clear();

    // init coil parameters
    struct CoilParam coil;

    int iSamF = pFiffInfo->sfreq;
    int iSamLoc = t_mat.cols(); // minimum samples required to localize numLoc times in a second

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
    VectorXd vCoilfreq(iNumCoils);

    if(vFreqs.size() >= iNumCoils) {
        for(int i = 0; i < iNumCoils; ++i) {
            vCoilfreq[i] = vFreqs.at(i);
            //std::cout<<std::endl << vCoilfreq[i] << "Hz";
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

    // Generate simulated data
    MatrixXd mSimsig(iSamLoc,iNumCoils*2);
    VectorXd vTime = VectorXd::LinSpaced(iSamLoc, 0, iSamLoc-1) *1.0/iSamF;

    for(int i = 0; i < iNumCoils; ++i) {
        mSimsig.col(i) = sin(2*M_PI*vCoilfreq[i]*vTime.array());
        mSimsig.col(i+iNumCoils) = cos(2*M_PI*vCoilfreq[i]*vTime.array());
    }

    // Create digitized HPI coil position matrix
    MatrixXd mHeadHPI(iNumCoils,3);

    // check the pFiffInfo->dig information. If dig is empty, set the mHeadHPI is 0;
    if (lHPIPoints.size() > 0) {
        for (int i = 0; i < lHPIPoints.size(); ++i) {
            mHeadHPI(i,0) = lHPIPoints.at(i).r[0];
            mHeadHPI(i,1) = lHPIPoints.at(i).r[1];
            mHeadHPI(i,2) = lHPIPoints.at(i).r[2];
        }
    } else {
        mHeadHPI.fill(0);
    }

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(m_vInnerind.size(),t_matProjectors.cols());
    MatrixXd matProjectorsInnerind(m_vInnerind.size(),m_vInnerind.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = t_matProjectors.row(m_vInnerind.at(i));
    }

    for (int i = 0; i < matProjectorsInnerind.cols(); ++i) {
        matProjectorsInnerind.col(i) = matProjectorsRows.col(m_vInnerind.at(i));
    }

    MatrixXd mTopo(m_vInnerind.size(), iNumCoils*2);
    MatrixXd mAmp(m_vInnerind.size(), iNumCoils);
    MatrixXd mAmpC(m_vInnerind.size(), iNumCoils);

    // Get the data from inner layer channels
    MatrixXd mInnerdata(m_vInnerind.size(), t_mat.cols());

    for(int j = 0; j < m_vInnerind.size(); ++j) {
        mInnerdata.row(j) << t_mat.row(m_vInnerind[j]);
    }

    // Calculate topo
    mTopo = mInnerdata * UTILSLIB::MNEMath::pinv(mSimsig).transpose(); // mTopo: # of good inner channel x 8

    // Select sine or cosine component depending on the relative size
    mAmp  = mTopo.leftCols(iNumCoils); // amp: # of good inner channel x 4
    mAmpC = mTopo.rightCols(iNumCoils);

    for(int j = 0; j < iNumCoils; ++j) {
       float fNS = 0.0;
       float fNC = 0.0;
       fNS = mAmp.col(j).array().square().sum();
       fNC = mAmpC.col(j).array().square().sum();

       if(fNC > fNS) {
           mAmp.col(j) = mAmpC.col(j);
       }
    }

    //Find good seed point/starting point for the coil position in 3D space
    //Find biggest amplitude per pickup coil (sensor) and store corresponding sensor channel index
    VectorXi vChIdcs(iNumCoils);

    for (int j = 0; j < iNumCoils; j++) {
        int iChIdx = 0;
        VectorXd::Index indMax;
        mAmp.col(j).maxCoeff(&indMax);
        if(indMax < m_vInnerind.size()) {
            iChIdx = m_vInnerind.at(indMax);
        }
        vChIdcs(j) = iChIdx;
    }

    //Generate seed point by projection the found channel position 3cm inwards
    vError.resize(iNumCoils);
    double dError = std::accumulate(vError.begin(), vError.end(), .0) / vError.size();
    MatrixXd mCoilPos = MatrixXd::Zero(iNumCoils,3);

    if(transDevHead.trans == MatrixXd::Identity(4,4).cast<float>() /*|| dError > 0.003*/){
        for (int j = 0; j < vChIdcs.rows(); ++j) {
            if(vChIdcs(j) < pFiffInfo->chs.size()) {
                Vector3f r0 = pFiffInfo->chs.at(vChIdcs(j)).chpos.r0;
                mCoilPos.row(j) = (-1 * pFiffInfo->chs.at(vChIdcs(j)).chpos.ez * 0.03 + r0).cast<double>();
            }
        }
    } else {
            mCoilPos = transDevHead.apply_inverse_trans(mHeadHPI.cast<float>()).cast<double>();
    }

    coil.pos = mCoilPos;

    // Perform actual localization
    coil = dipfit(coil, m_sensors, mAmp, iNumCoils, matProjectorsInnerind);

    Matrix4d mTrans = computeTransformation(mHeadHPI, coil.pos);
    //Eigen::Matrix4d mTrans = computeTransformation(coil.pos, mHeadHPI);

    // Store the final result to fiff info
    // Set final device/head matrix and its inverse to the fiff info
    transDevHead.from = 1;
    transDevHead.to = 4;
    transDevHead.trans = mTrans.cast<float>();

    // Also store the inverse
    transDevHead.invtrans = transDevHead.trans.inverse();

    //Calculate Error
    MatrixXd mTemp = coil.pos;
    mTemp.conservativeResize(coil.pos.rows(),coil.pos.cols()+1);

    mTemp.block(0,3,iNumCoils,1).setOnes();
    mTemp.transposeInPlace();

    MatrixXd mTestPos = mTrans * mTemp;
    MatrixXd mDiffPos = mTestPos.block(0,0,3,iNumCoils) - mHeadHPI.transpose();

    for(int i = 0; i < mDiffPos.cols(); ++i) {
        vError[i] = mDiffPos.col(i).norm();
    }

    // store Goodness of Fit
    vGoF = coil.dpfiterror;
    for(int i = 0; i < vGoF.size(); ++i) {
        vGoF(i) = 1 - vGoF(i);
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
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Initial seed point for HPI coils" << std::endl << mCoilPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - temp" << std::endl << mTemp << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - testPos" << std::endl << mTestPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Diff fitted - original" << std::endl << mDiffPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - dev/head trans" << std::endl << mTrans << std::endl;

        QString sTimeStamp = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");

        if(!QDir(sHPIResourceDir).exists()) {
            QDir().mkdir(sHPIResourceDir);
        }

        UTILSLIB::IOUtils::write_eigen_matrix(mCoilPos, QString("%1/%2_coilPosSeed_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(coil.pos, QString("%1/%2_coilPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(mHeadHPI, QString("%1/%2_mHeadHPI_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd testPosCut = mTestPos.transpose();//block(0,0,3,4);
        UTILSLIB::IOUtils::write_eigen_matrix(testPosCut, QString("%1/%2_testPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXi mIdx(vChIdcs.rows(),1);
        mIdx.col(0) = vChIdcs;
        UTILSLIB::IOUtils::write_eigen_matrix(mIdx, QString("%1/%2_mIdx_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd mCoilFreq(vCoilfreq.rows(),1);
        mCoilFreq.col(0) = vCoilfreq;
        UTILSLIB::IOUtils::write_eigen_matrix(mCoilFreq, QString("%1/%2_mCoilFreq_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(mDiffPos, QString("%1/%2_diffPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(mAmp, QString("%1/%2_amp_mat").arg(sHPIResourceDir).arg(sTimeStamp));
    }
}

//=============================================================================================================

void HPIFit::findOrder(const MatrixXd& t_mat,
                       const MatrixXd& t_matProjectors,
                       FiffCoordTrans& transDevHead,
                       QVector<int>& vFreqs,
                       QVector<double>& vError,
                       VectorXd& vGoF,
                       FiffDigPointSet& fittedPointSet,
                       FiffInfo::SPtr pFiffInfo)
{
    // create temporary copies that are necessary to reset values that are passed to fitHpi()
    fittedPointSet.clear();
    FiffDigPointSet fittedPointSetTemp = fittedPointSet;
    FiffCoordTrans transDevHeadTemp = transDevHead;
    FiffInfo::SPtr pFiffInfoTemp = pFiffInfo;
    QVector<int> vToOrder = vFreqs;
    QVector<int> vFreqTemp(vFreqs.size());
    QVector<double> vErrorTemp = vError;
    VectorXd vGoFTemp = vGoF;
    bool bIdentity = false;
    fittedPointSetTemp.clear();

    MatrixXf trans = transDevHead.trans;
    if(transDevHead.trans == MatrixXf::Identity(4,4).cast<float>()) {
        // avoid identity since this leads to problems with this method in fitHpi.
        // the hpi fit is robust enough to handle bad starting points
        transDevHeadTemp.trans(3,0) = 0.000001;
        bIdentity = true;
    }

    // perform vFreqs.size() hpi fits with same frequencies in each iteration
    for(int i = 0; i < vFreqs.size(); i++){
        vFreqTemp.fill(vFreqs[i]);

        // hpi Fit
        fitHPI(t_mat, t_matProjectors, transDevHeadTemp, vFreqTemp, vErrorTemp, vGoFTemp, fittedPointSetTemp, pFiffInfoTemp);

        // get location of maximum GoF -> correct assignment of coil - frequency
        VectorXd::Index indMax;
        vGoFTemp.maxCoeff(&indMax);
        vToOrder[indMax] = vFreqs[i];

        // std::cout << vGoFTemp[0] << " " << vGoFTemp[1] << " " << vGoFTemp[2] << " " << vGoFTemp[3] << " " << std::endl;

        // reset values that are edidet by fitHpi
        fittedPointSetTemp = fittedPointSet;
        pFiffInfoTemp = pFiffInfo;
        transDevHeadTemp = transDevHead;
        if(bIdentity) {
            transDevHeadTemp.trans(3,0) = 0.000001;
        }

        vErrorTemp = vError;
        vGoFTemp = vGoF;
    }
    // check if still all frequencies are represented
    if(std::accumulate(vFreqs.begin(), vFreqs.end(), .0) ==  std::accumulate(vToOrder.begin(), vToOrder.end(), .0)) {
        vFreqs = vToOrder;
    } else {
        qWarning() << "HPIFit::findOrder: frequencie ordering went wrong";
    }
    qInfo() << "HPIFit::findOrder: vFreqs = " << vFreqs;
}

//=============================================================================================================

CoilParam HPIFit::dipfit(struct CoilParam coil,
                         const Sensor& sensors,
                         const MatrixXd& mData,
                         int iNumCoils,
                         const MatrixXd& t_matProjectors)
{
    //Do this in conncurrent mode
    //Generate QList structure which can be handled by the QConcurrent framework
    QList<HPIFitData> lCoilData;

    for(qint32 i = 0; i < iNumCoils; ++i) {
        HPIFitData coilData;
        coilData.coilPos = coil.pos.row(i);
        coilData.sensorData = mData.col(i);
        coilData.sensors = sensors;
        coilData.matProjector = t_matProjectors;

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
            coil.pos.row(i) = lCoilData.at(i).coilPos;
            coil.mom = lCoilData.at(i).errorInfo.moment.transpose();
            coil.dpfiterror(i) = lCoilData.at(i).errorInfo.error;
            coil.dpfitnumitr(i) = lCoilData.at(i).errorInfo.numIterations;

            //std::cout<<std::endl<< "HPIFit::dipfit - Itr steps for coil " << i << " =" <<coil.dpfitnumitr(i);
        }
    }

    return coil;
}

//=============================================================================================================

Eigen::Matrix4d HPIFit::computeTransformation(Eigen::MatrixXd mNH, MatrixXd mBT)
{
    MatrixXd mXdiff, mYdiff, mZdiff, mC, mQ;
    Matrix4d mTransFinal = Matrix4d::Identity(4,4);
    Matrix4d mRot = Matrix4d::Zero(4,4);
    Matrix4d mTrans = Matrix4d::Identity(4,4);
    double dMeanX,dMeanY,dMeanZ,dNormf;

    for(int i = 0; i < 15; ++i) {
        // Calculate mean translation for all points -> centroid of both data sets
        mXdiff = mNH.col(0) - mBT.col(0);
        mYdiff = mNH.col(1) - mBT.col(1);
        mZdiff = mNH.col(2) - mBT.col(2);

        dMeanX = mXdiff.mean();
        dMeanY = mYdiff.mean();
        dMeanZ = mZdiff.mean();

        // Apply translation -> bring both data sets to the same center location
        for (int j = 0; j < mBT.rows(); ++j) {
            mBT(j,0) = mBT(j,0) + dMeanX;
            mBT(j,1) = mBT(j,1) + dMeanY;
            mBT(j,2) = mBT(j,2) + dMeanZ;
        }

        // Estimate rotation component
        mC = mBT.transpose() * mNH;

        JacobiSVD< MatrixXd > svd(mC ,Eigen::ComputeThinU | ComputeThinV);

        mQ = svd.matrixU() * svd.matrixV().transpose();

        //Handle special reflection case
        if(mQ.determinant() < 0) {
            mQ(0,2) = mQ(0,2) * -1;
            mQ(1,2) = mQ(1,2) * -1;
            mQ(2,2) = mQ(2,2) * -1;
        }

        // Apply rotation on translated points
        mBT = mBT * mQ;

        // Calculate GOF
        dNormf = (mNH.transpose()-mBT.transpose()).norm();

        // Store rotation part to transformation matrix
        mRot(3,3) = 1;
        for(int j = 0; j < 3; ++j) {
            for(int k = 0; k < 3; ++k) {
                mRot(j,k) = mQ(k,j);
            }
        }

        // Store translation part to transformation matrix
        mTrans(0,3) = dMeanX;
        mTrans(1,3) = dMeanY;
        mTrans(2,3) = dMeanZ;

        // Safe rotation and translation to final matrix for next iteration step
        // This step is safe to do since we change one of the input point sets (mBT)
        // ToDo: Replace this for loop with a least square solution process
        mTransFinal = mRot * mTrans * mTransFinal;
    }
    return mTransFinal;
}

//=============================================================================================================

void HPIFit::createSensorSet(Sensor& sensors, FwdCoilSet* coils)
{
    int iNchan = coils->ncoil;
    // init sensor struct
    int iNp = coils->coils[0]->np;
    sensors.w = RowVectorXd(iNchan*iNp);
    sensors.r0 = MatrixXd(iNchan,3);
    sensors.cosmag = MatrixXd(iNchan*iNp,3);
    sensors.rmag = MatrixXd(iNchan*iNp,3);
    sensors.ncoils = iNchan;
    sensors.tra = MatrixXd::Identity(iNp,iNp);
    sensors.np = iNp;
    for(int i = 0; i < iNchan; i++){
        FwdCoil* coil = (coils->coils[i]);
        MatrixXd mRmag = MatrixXd::Zero(iNp,3);
        MatrixXd mCosmag = MatrixXd::Zero(iNp,3);
        RowVectorXd vW(iNp);

        sensors.r0(i,0) = coil->r0[0];
        sensors.r0(i,1) = coil->r0[1];
        sensors.r0(i,2) = coil->r0[2];

        for (int p = 0; p < iNp; p++){
            sensors.w(i*iNp+p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                mRmag(p,c)   = coil->rmag[p][c];
                mCosmag(p,c) = coil->cosmag[p][c];
            }
        }
        sensors.cosmag.block(i*iNp,0,iNp,3) = mCosmag;
        sensors.rmag.block(i*iNp,0,iNp,3) = mRmag;
    }
}

//=============================================================================================================

void HPIFit::storeHeadPosition(float fTime,
                               const Eigen::MatrixXf& transDevHead,
                               Eigen::MatrixXd& mPosition,
                               const Eigen::VectorXd& vGoF,
                               const QVector<double>& vError)

{
    // Write quaternions and vTime in position matrix. Format is the same like MaxFilter's .pos files.
    Matrix3f mRot = transDevHead.block(0,0,3,3);

    double dError = std::accumulate(vError.begin(), vError.end(), .0) / vError.size();     // HPI estimation Error
    Eigen::Quaternionf quatHPI(mRot);

//    qDebug() << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z ";
//    qDebug() << quatHPI.x() << quatHPI.y() << quatHPI.z() << transDevHead(0,3) << transDevHead(1,3) << transDevHead(2,3);

    mPosition.conservativeResize(mPosition.rows()+1, 10);
    mPosition(mPosition.rows()-1,0) = fTime;
    mPosition(mPosition.rows()-1,1) = quatHPI.x();
    mPosition(mPosition.rows()-1,2) = quatHPI.y();
    mPosition(mPosition.rows()-1,3) = quatHPI.z();
    mPosition(mPosition.rows()-1,4) = transDevHead(0,3);
    mPosition(mPosition.rows()-1,5) = transDevHead(1,3);
    mPosition(mPosition.rows()-1,6) = transDevHead(2,3);
    mPosition(mPosition.rows()-1,7) = vGoF.mean();
    mPosition(mPosition.rows()-1,8) = dError;
    mPosition(mPosition.rows()-1,9) = 0;
}

//=============================================================================================================

void HPIFit::updateCoils()
{
    // Create MEG-Coils and read data
    int iAcc = 2;
    int iNch = m_lChannels.size();
    FiffCoordTransOld* t = NULL;

    if(!m_coilTemplate) {
        // read coil_def.dat
        QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
        m_coilTemplate = FwdCoilSet::read_coil_defs(qPath);
    }
    // create sensor set
    m_coilMeg = m_coilTemplate->create_meg_coils(m_lChannels,iNch,iAcc,t);
    createSensorSet(m_sensors,m_coilMeg);
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
                m_vInnerind.append(i);
                m_lChannels.append(pFiffInfo->chs[i]);
            }
        }
    }
    m_lBads = pFiffInfo->bads;
}
