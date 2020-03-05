//=============================================================================================================
/**
 * @file     hpifit.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
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

HPIFit::HPIFit()
{
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
    }
    //Check if projector was passed
    if(t_matProjectors.rows() == 0 || t_matProjectors.cols() == 0 ) {
        std::cout<<std::endl<< "HPIFit::fitHPI - No projector passed. Returning.";
    }

    // Setup Constructors for Coil Set
    FwdCoilSet* templates = NULL;
    FwdCoilSet* megCoils = NULL;

    //struct SensorInfo sensors;
    struct CoilParam coil;
    int numCh = pFiffInfo->nchan;
    int samF = pFiffInfo->sfreq;
    int samLoc = t_mat.cols(); // minimum samples required to localize numLoc times in a second

    //Get HPI coils from digitizers and set number of coils
    int numCoils = 0;
    QList<FiffDigPoint> lHPIPoints;

    for(int i = 0; i < pFiffInfo->dig.size(); ++i) {
        if(pFiffInfo->dig[i].kind == FIFFV_POINT_HPI) {
            numCoils++;
            lHPIPoints.append(pFiffInfo->dig[i]);
        }
    }

    //Set coil frequencies
    VectorXd coilfreq(numCoils);

    if(vFreqs.size() >= numCoils) {
        for(int i = 0; i < numCoils; ++i) {
            coilfreq[i] = vFreqs.at(i);
            //std::cout<<std::endl << coilfreq[i] << "Hz";
        }
    } else {
        std::cout<<std::endl<< "HPIFit::fitHPI - Not enough coil frequencies specified. Returning.";
        return;
    }

    // Initialize HPI coils location and moment
    coil.pos = MatrixXd::Zero(numCoils,3);
    coil.mom = MatrixXd::Zero(numCoils,3);
    coil.dpfiterror = VectorXd::Zero(numCoils);
    coil.dpfitnumitr = VectorXd::Zero(numCoils);

    // Generate simulated data
    MatrixXd simsig(samLoc,numCoils*2);
    VectorXd time(samLoc);

    for (int i = 0; i < samLoc; ++i) {
        time[i] = i*1.0/samF;
    }

    for(int i = 0; i < numCoils; ++i) {
        for(int j = 0; j < samLoc; ++j) {
            simsig(j,i) = sin(2*M_PI*coilfreq[i]*time[j]);
            simsig(j,i+numCoils) = cos(2*M_PI*coilfreq[i]*time[j]);
        }
    }

    // Create digitized HPI coil position matrix
    MatrixXd headHPI(numCoils,3);

    // check the pFiffInfo->dig information. If dig is empty, set the headHPI is 0;
    if (lHPIPoints.size() > 0) {
        for (int i = 0; i < lHPIPoints.size(); ++i) {
            headHPI(i,0) = lHPIPoints.at(i).r[0];
            headHPI(i,1) = lHPIPoints.at(i).r[1];
            headHPI(i,2) = lHPIPoints.at(i).r[2];
        }
    } else {
        for (int i = 0; i < numCoils; ++i) {
            headHPI(i,0) = 0;
            headHPI(i,1) = 0;
            headHPI(i,2) = 0;
        }
    }

    // Get the indices of inner layer channels and exclude bad channels and create channellist
    QList<FiffChInfo> channels;
    QVector<int> innerind(0);

    for (int i = 0; i < numCh; ++i) {
        if(pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_BABY_MAG ||
                pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 ||
                pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T2 ||
                pFiffInfo->chs[i].chpos.coil_type == FIFFV_COIL_VV_PLANAR_T3) {
            // Check if the sensor is bad, if not append to innerind
            if(!(pFiffInfo->bads.contains(pFiffInfo->ch_names.at(i)))) {
                innerind.append(i);
                channels.append(pFiffInfo->chs[i]);
            }
        }
    }

    // Create MEG-Coils and read doil_def.dat
    QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
    // Create MEG-Coils and read data
    int acc = 2;
    int nch = channels.size();
    FiffCoordTransOld* t = NULL;

    templates = FwdCoilSet::read_coil_defs(qPath);

    QList<Sensor> sensorSet;
    megCoils = templates->create_meg_coils(channels,nch,acc,t);
    createSensorSet(sensorSet,megCoils);

    //Create new projector based on the excluded channels, first exclude the rows then the columns
    MatrixXd matProjectorsRows(innerind.size(),t_matProjectors.cols());
    MatrixXd matProjectorsInnerind(innerind.size(),innerind.size());

    for (int i = 0; i < matProjectorsRows.rows(); ++i) {
        matProjectorsRows.row(i) = t_matProjectors.row(innerind.at(i));
    }

    for (int i = 0; i < matProjectorsInnerind.cols(); ++i) {
        matProjectorsInnerind.col(i) = matProjectorsRows.col(innerind.at(i));
    }

    MatrixXd topo(innerind.size(), numCoils*2);
    MatrixXd amp(innerind.size(), numCoils);
    MatrixXd ampC(innerind.size(), numCoils);

    // Get the data from inner layer channels
    MatrixXd innerdata(innerind.size(), t_mat.cols());

    for(int j = 0; j < innerind.size(); ++j) {
        innerdata.row(j) << t_mat.row(innerind[j]);
    }

    // Calculate topo
    topo = innerdata * UTILSLIB::MNEMath::pinv(simsig).transpose(); // topo: # of good inner channel x 8

    // Select sine or cosine component depending on the relative size
    amp  = topo.leftCols(numCoils); // amp: # of good inner channel x 4
    ampC = topo.rightCols(numCoils);

    for(int j = 0; j < numCoils; ++j) {
       float nS = 0.0;
       float nC = 0.0;
       for(int i = 0; i < innerind.size(); ++i) {
           nS += amp(i,j)*amp(i,j);
           nC += ampC(i,j)*ampC(i,j);
       }

       if(nC > nS) {
         for(int i = 0; i < innerind.size(); ++i) {
           amp(i,j) = ampC(i,j);
         }
       }
    }

    //Find good seed point/starting point for the coil position in 3D space
    //Find biggest amplitude per pickup coil (sensor) and store corresponding sensor channel index
    VectorXi chIdcs(numCoils);

    for (int j = 0; j < numCoils; j++) {
        double maxVal = 0;
        int chIdx = 0;

        for (int i = 0; i < amp.rows(); ++i) {
            if(std::fabs(amp(i,j)) > maxVal) {
                maxVal = std::fabs(amp(i,j));
                if(chIdx < innerind.size()) {
                    chIdx = innerind.at(i);
                }
            }
        }
        chIdcs(j) = chIdx;
    }

    //Generate seed point by projection the found channel position 3cm inwards
    double error = std::accumulate(vError.begin(), vError.end(), .0) / vError.size();
    MatrixXd coilPos = MatrixXd::Zero(numCoils,3);

    if(transDevHead.trans == MatrixXd::Identity(4,4).cast<float>() || error > 0.003){
        for (int j = 0; j < chIdcs.rows(); ++j) {
            int chIdx = chIdcs(j);
            if(chIdx < pFiffInfo->chs.size()) {
                double x = pFiffInfo->chs.at(chIdcs(j)).chpos.r0[0];
                double y = pFiffInfo->chs.at(chIdcs(j)).chpos.r0[1];
                double z = pFiffInfo->chs.at(chIdcs(j)).chpos.r0[2];

                coilPos(j,0) = -1 * pFiffInfo->chs.at(chIdcs(j)).chpos.ez[0] * 0.03 + x;
                coilPos(j,1) = -1 * pFiffInfo->chs.at(chIdcs(j)).chpos.ez[1] * 0.03 + y;
                coilPos(j,2) = -1 * pFiffInfo->chs.at(chIdcs(j)).chpos.ez[2] * 0.03 + z;
            }
        }
        //std::cout << "HPIFit::fitHPI - Coil " << j << " max value index " << chIdx << std::endl;
    } else {
        coilPos = transDevHead.apply_inverse_trans(headHPI.cast<float>()).cast<double>();
    }

    coil.pos = coilPos;

    // Perform actual localization
    coil = dipfit(coil, sensorSet, amp, numCoils, matProjectorsInnerind);

    Matrix4d trans = computeTransformation(headHPI, coil.pos);
    //Eigen::Matrix4d trans = computeTransformation(coil.pos, headHPI);

    // Store the final result to fiff info
    // Set final device/head matrix and its inverse to the fiff info
    transDevHead.from = 1;
    transDevHead.to = 4;
    transDevHead.trans = trans.cast<float>();

    // Also store the inverse
    transDevHead.invtrans = transDevHead.trans.inverse();

    //Calculate Error
    MatrixXd temp = coil.pos;
    temp.conservativeResize(coil.pos.rows(),coil.pos.cols()+1);

    temp.block(0,3,numCoils,1).setOnes();
    temp.transposeInPlace();

    MatrixXd testPos = trans * temp;
    MatrixXd diffPos = testPos.block(0,0,3,numCoils) - headHPI.transpose();

    for(int i = 0; i < diffPos.cols(); ++i) {
        vError.append(diffPos.col(i).norm());
    }

    // store Goodness of Fit
    vGoF = coil.dpfiterror;
    for(int i = 0; i < vGoF.size(); ++i) {
        vGoF(i) = 1 - vGoF(i);
    }

    //Generate final fitted points and store in digitizer set
    for(int i = 0; i < coil.pos.rows(); ++i) {
        FiffDigPoint digPoint;
        digPoint.kind = FIFFV_POINT_EEG;
        digPoint.ident = i;
        digPoint.r[0] = coil.pos(i,0);
        digPoint.r[1] = coil.pos(i,1);
        digPoint.r[2] = coil.pos(i,2);

        fittedPointSet << digPoint;
    }

    if(bDoDebug) {
        // DEBUG HPI fitting and write debug results
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - dpfiterror" << coil.dpfiterror << std::endl << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Initial seed point for HPI coils" << std::endl << coilPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - temp" << std::endl << temp << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - testPos" << std::endl << testPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - Diff fitted - original" << std::endl << diffPos << std::endl;
        std::cout << std::endl << std::endl << "HPIFit::fitHPI - dev/head trans" << std::endl << trans << std::endl;

        QString sTimeStamp = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");

        if(!QDir(sHPIResourceDir).exists()) {
            QDir().mkdir(sHPIResourceDir);
        }

        UTILSLIB::IOUtils::write_eigen_matrix(coilPos, QString("%1/%2_coilPosSeed_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(coil.pos, QString("%1/%2_coilPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(headHPI, QString("%1/%2_headHPI_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd testPosCut = testPos.transpose();//block(0,0,3,4);
        UTILSLIB::IOUtils::write_eigen_matrix(testPosCut, QString("%1/%2_testPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXi idx_mat(chIdcs.rows(),1);
        idx_mat.col(0) = chIdcs;
        UTILSLIB::IOUtils::write_eigen_matrix(idx_mat, QString("%1/%2_idx_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        MatrixXd coilFreq_mat(coilfreq.rows(),1);
        coilFreq_mat.col(0) = coilfreq;
        UTILSLIB::IOUtils::write_eigen_matrix(coilFreq_mat, QString("%1/%2_coilFreq_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(diffPos, QString("%1/%2_diffPos_mat").arg(sHPIResourceDir).arg(sTimeStamp));

        UTILSLIB::IOUtils::write_eigen_matrix(amp, QString("%1/%2_amp_mat").arg(sHPIResourceDir).arg(sTimeStamp));
    }
}

//=============================================================================================================

CoilParam HPIFit::dipfit(struct CoilParam coil,
                         const QList<Sensor>& sensorSet,
                         const MatrixXd& data,
                         int numCoils,
                         const MatrixXd& t_matProjectors)
{
    //Do this in conncurrent mode
    //Generate QList structure which can be handled by the QConcurrent framework
    QList<HPIFitData> lCoilData;

    for(qint32 i = 0; i < numCoils; ++i) {
        HPIFitData coilData;
        coilData.coilPos = coil.pos.row(i);
        coilData.sensorData = data.col(i);
        coilData.sensorSet = sensorSet;
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

Eigen::Matrix4d HPIFit::computeTransformation(Eigen::MatrixXd NH, MatrixXd BT)
{
    MatrixXd xdiff, ydiff, zdiff, C, Q;
    Matrix4d transFinal = Matrix4d::Identity(4,4);
    Matrix4d Rot = Matrix4d::Zero(4,4);
    Matrix4d Trans = Matrix4d::Identity(4,4);
    double meanx,meany,meanz,normf;

    for(int i = 0; i < 15; ++i) {
        // Calculate mean translation for all points -> centroid of both data sets
        xdiff = NH.col(0) - BT.col(0);
        ydiff = NH.col(1) - BT.col(1);
        zdiff = NH.col(2) - BT.col(2);

        meanx = xdiff.mean();
        meany = ydiff.mean();
        meanz = zdiff.mean();

        // Apply translation -> bring both data sets to the same center location
        for (int j = 0; j < BT.rows(); ++j) {
            BT(j,0) = BT(j,0) + meanx;
            BT(j,1) = BT(j,1) + meany;
            BT(j,2) = BT(j,2) + meanz;
        }

        // Estimate rotation component
        C = BT.transpose() * NH;

        JacobiSVD< MatrixXd > svd(C ,Eigen::ComputeThinU | ComputeThinV);

        Q = svd.matrixU() * svd.matrixV().transpose();

        //Handle special reflection case
        if(Q.determinant() < 0) {
            Q(0,2) = Q(0,2) * -1;
            Q(1,2) = Q(1,2) * -1;
            Q(2,2) = Q(2,2) * -1;
        }

        // Apply rotation on translated points
        BT = BT * Q;

        // Calculate GOF
        normf = (NH.transpose()-BT.transpose()).norm();

        // Store rotation part to transformation matrix
        Rot(3,3) = 1;
        for(int j = 0; j < 3; ++j) {
            for(int k = 0; k < 3; ++k) {
                Rot(j,k) = Q(k,j);
            }
        }

        // Store translation part to transformation matrix
        Trans(0,3) = meanx;
        Trans(1,3) = meany;
        Trans(2,3) = meanz;

        // Safe rotation and translation to final matrix for next iteration step
        // This step is safe to do since we change one of the input point sets (BT)
        // ToDo: Replace this for loop with a least square solution process
        transFinal = Rot * Trans * transFinal;
    }
    return transFinal;
}

//=============================================================================================================

void HPIFit::createSensorSet(QList<struct Sensor>& sensors, FwdCoilSet* coils) {
    int nchan = coils->ncoil;
    for(int i = 0; i < nchan; i++){
        Sensor s;
        FwdCoil* coil = (coils->coils[i]);
        int np = coil->np;
        MatrixXd rmag = MatrixXd::Zero(np,3);
        MatrixXd cosmag = MatrixXd::Zero(np,3);
        RowVectorXd w(np);

        s.r0(0) = coil->r0[0];
        s.r0(1) = coil->r0[1];
        s.r0(2) = coil->r0[2];

        for (int p = 0; p < np; p++){
            w(p) = coil->w[p];
            for (int c = 0; c < 3; c++) {
                rmag(p,c)   = coil->rmag[p][c];
                cosmag(p,c) = coil->cosmag[p][c];
            }
        }
        s.tra = MatrixXd::Identity(np,np);
        s.w = w;
        s.rmag = rmag;
        s.cosmag = cosmag;
        s.np = np;
        sensors.append(s);
    }
}

//=============================================================================================================

void HPIFit::storeHeadPosition(float time,
                               const Eigen::MatrixXf& devHeadT,
                               Eigen::MatrixXd& position,
                               const Eigen::VectorXd& vGoF,
                               const QVector<double>& vError)

{
    // Write quaternions and time in position matrix. Format is the same like MaxFilter's .pos files.
    Matrix3f rot = devHeadT.block(0,0,3,3);

    double error = std::accumulate(vError.begin(), vError.end(), .0) / vError.size();     // HPI estimation Error
    Eigen::Quaternionf quatHPI(rot);

//    qDebug() << "quatHPI.x() " << "quatHPI.y() " << "quatHPI.y() " << "trans x " << "trans y " << "trans z ";
//    qDebug() << quatHPI.x() << quatHPI.y() << quatHPI.z() << devHeadT(0,3) << devHeadT(1,3) << devHeadT(2,3);
    float x = quatHPI.x();
    float y = quatHPI.y();
    float z = quatHPI.z();
    position.conservativeResize(position.rows()+1, 10);
    position(position.rows()-1,0) = time;
    position(position.rows()-1,1) = x;
    position(position.rows()-1,2) = y;
    position(position.rows()-1,3) = z;
    position(position.rows()-1,4) = devHeadT(0,3);
    position(position.rows()-1,5) = devHeadT(1,3);
    position(position.rows()-1,6) = devHeadT(2,3);
    position(position.rows()-1,7) = vGoF.mean();
    position(position.rows()-1,8) = error;
    position(position.rows()-1,9) = 0;
}
