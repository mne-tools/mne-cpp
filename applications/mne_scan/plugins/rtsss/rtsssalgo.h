//=============================================================================================================
/**
 * @file     rtsssalgo.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     June, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the RtSssAlgo class.
 *
 * @remarks  This rtsssalgo is implemented based on a paper, 'Real-Time Robust Signal Space Separation for Magnetoencephalography',
 *           authored by Chenlei Guo, Xin Li, Samu Taulu, Wei Wang, and Douglas J. Weber,
 *           published in IEEE Transactions on Biomedical Engieering Vol 57, p1856~1866, August 2010.

 */

#ifndef RTSSSALGO_H
#define RTSSSALGO_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QList>
#include <Eigen/Dense>
#include <iostream>
#include <QString>
#include <QDebug>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
//#include <scMeas/Measurement/realtimemultisamplearray_new.h>

#define BABYMEG 1
#define VECTORVIEW 2

using namespace Eigen;
using namespace std;
using namespace FIFFLIB;
//using namespace SCMEASLIB;

typedef std::complex<double> cplxd;

MatrixXd legendre(int, VectorXd);
float plgndr(int l, int m, float x);
double factorial(int);
//QList<MatrixXd> getSSSRR(MatrixXd, MatrixXd, MatrixXd, MatrixXd, MatrixXd);
VectorXd hypot(VectorXd, VectorXd);
VectorXd atan2vec(VectorXd, VectorXd);
VectorXd find(MatrixXd, int);

double stdev(VectorXd);
VectorXd eigen_LTE(VectorXd V, double tol);
VectorXd eigen_LT_index(VectorXd V, double tol);
VectorXd eigen_LT_index_test(int);
VectorXd eigen_GT(VectorXd V, double tol);
VectorXd eigen_AND(VectorXd V1, VectorXd V2);

class RtSssAlgo
{
public:
    RtSssAlgo();
    ~RtSssAlgo();

//    QList<MatrixXd> buildLinearEqn();
    MatrixXd buildLinearEqn();

//    QList<MatrixXd> getSSSRR(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnARR, MatrixXd EqnA, MatrixXd EqnB);
//    QList<MatrixXd> getSSSRR(MatrixXd EqnB);
    MatrixXd getSSSRR(MatrixXd EqnB);

//    QList<MatrixXd> getSSSOLS(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnA, MatrixXd EqnB);
//    QList<MatrixXd> getSSSOLS(MatrixXd EqnB);
    MatrixXd getSSSOLS(MatrixXd EqnB);

    QList<MatrixXd> getLinEqn();

    void setMEGInfo(FiffInfo::SPtr fiffinfo, RowVectorXi);
    void setSSSParameter(QList<int>);
    qint32 getNumMEGChan();
    qint32 getNumMEGChanUsed();
    qint32 getNumMEGBadChan();
    VectorXi getBadChan();

private:
    void getCoilInfoVectorView();
    void getCoilInfoVectorView4Sim();
    void getCoilInfoBabyMEG4Sim();
    QList<MatrixXd> getSSSEqn(qint32, qint32);
//    QList<MatrixXd> getSSSEqn(VectorXi Lexp);
    void getSSSBasis(VectorXd, VectorXd, VectorXd, qint32, qint32);
    void getCartesianToSpherCoordinate(VectorXd, VectorXd, VectorXd);
    void getSphereToCartesianVector();
    int strmatch(char, char);

    qint32 NumMEGChan, NumCoil, NumBadCoil;
    VectorXi BadChan;
    QList<MatrixXd> CoilT;
    QList<QString> CoilName, CoilTk;
    QList<MatrixXd> CoilRk, CoilWk;
    VectorXi CoilNk, CoilGrad;
    VectorXd MEGIn, MEGOut, MEGNoise;
    MatrixXd MEGData;

    qint32 LInRR, LOutRR, LInOLS, LOutOLS;
    Vector3d Origin;
    MatrixXd BInX, BInY, BInZ, BOutX, BOutY, BOutZ;
    MatrixXd EqnInRR, EqnOutRR, EqnIn, EqnOut, EqnARR, EqnA, EqnB;

    VectorXd R, PHI, THETA;
    VectorXd R_X, R_Y, R_Z;
    VectorXd PHI_X, PHI_Y, PHI_Z;
    VectorXd THETA_X, THETA_Y, THETA_Z;

//    FiffInfo::SPtr m_pFiffInfo;     /**< Fiff information. */
};

#endif // RTSSSALGO_H
