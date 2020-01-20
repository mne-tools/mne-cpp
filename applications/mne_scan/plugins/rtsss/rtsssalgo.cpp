//=============================================================================================================
/**
 * @file     rtsssalgo.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     June, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lars Debor, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RtSssAlgo class.
 *
 * @remarks  This rtsssalgo is implemented based on a paper, 'Real-Time Robust Signal Space Separation for Magnetoencephalography',
 *           authored by Chenlei Guo, Xin Li, Samu Taulu, Wei Wang, and Douglas J. Weber,
 *           published in IEEE Transactions on Biomedical Engieering Vol 57, p1856~1866, August 2010.
 */

#include "rtsssalgo.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrentMap>
#include <QFile>
//#include "FormFiles/rtssssetupwidget.h"

RtSssAlgo::RtSssAlgo()
: NumMEGChan(0)
, NumCoil(0)
, NumBadCoil(0)
, LInRR(0)
, LOutRR(0)
, LInOLS(0)
, LOutOLS(0)
{

}

RtSssAlgo::~RtSssAlgo()
{

}

//QList<MatrixXd> RtSssAlgo::buildLinearEqn()
MatrixXd RtSssAlgo::buildLinearEqn()
{
    //qDebug() << "buildLinearEqn START";

    QList<MatrixXd> Eqn, EqnRR;
    QList<MatrixXd> LinEqn;
    qint32 LIn, LOut;
//    MatrixXd EqnInRR, EqnOutRR, EqnIn, EqnOut;

//    int MagScale;
//    int MACHINE_TYPE = VECTORVIEW;

//    if (MACHINE_TYPE == VECTORVIEW)
//    {
//        MagScale = 100;
//        std::cout << "loading coil information (VectorView).....";
//        getCoilInfoVectorView();
//        std::cout << "loading coil information (VectorView)..... ** finished **"  << endl;
//    }
//    else if (MACHINE_TYPE == BABYMEG)
//    {
//        MagScale = 1;
//        std::cout << "loading coil information (BabyMEG).....";
//        getCoilInfoBabyMEG4Sim();
//        std::cout << "loading coil information (BabyMEG)..... ** finisehd **" << endl;
//    }


//  Compute SSS equation
    LIn = LInRR;
    LOut = LOutRR;
    EqnRR = getSSSEqn(LIn, LOut);
    EqnInRR = EqnRR[0];
    EqnOutRR = EqnRR[1];

    LIn = LInOLS;
    LOut = LOutOLS;
    Eqn = getSSSEqn(LIn, LOut);
    EqnIn = Eqn[0];
    EqnOut = Eqn[1];

//
//    Vector2i  LexpRR, LexpOLS;
//    LexpRR(0) =LInRR;
//    LexpRR(1) = LOutRR;

//    LexpOLS(0) =LInOLS;
//    LexpOLS(1) = LOutOLS;

//    QList<Vector2i> Lexp;
//    Lexp.append(LexpRR);
//    Lexp.append(LexpOLS);

//    QFuture<QList> res = QtConcurrent::mapped(Lexp,getSSSEqn);
//    res.waitForFinished();


//  build linear equation
//    MatrixXd EqnARR(NumCoil, EqnInRR.cols()+EqnOutRR.cols());
//    MatrixXd EqnA(NumCoil, EqnIn.cols()+EqnOut.cols());
//    MatrixXd EqnB(NumCoil,1);
    EqnARR.resize(NumCoil, EqnInRR.cols()+EqnOutRR.cols());
    EqnA.resize(NumCoil, EqnIn.cols()+EqnOut.cols());
    EqnB.resize(NumCoil,1);

    // Find out if coils are all gradiometers, all magnetometers, or both.
    // When both gradiometers and magnetometers are used,
    //      MagScale facor of 100 must be appiled to magnetomters.
    float MagScale;
    if ((0 < CoilGrad.sum()) && (CoilGrad.sum() < NumCoil))  MagScale = 100;
    else MagScale = 1;

    VectorXd CoilScale;
    CoilScale.setOnes(NumCoil);
    for(int i=0; i<NumCoil; i++)
    {
        if (CoilGrad(i) == 0) CoilScale(i) = MagScale;
//        std::cout <<  "i=" << i << "CoilGrad: " << CoilGrad(i) << ",  CoilScale: " << CoilScale(i) << std::endl;
    }

    EqnARR << EqnInRR, EqnOutRR;
    EqnA << EqnIn, EqnOut;

    EqnARR = CoilScale.asDiagonal() * EqnARR;
    EqnA = CoilScale.asDiagonal() * EqnA;
//        std::cout << "pass 1" << std::endl;
//        std::cout << "MEGData: " << MEGData.rows() << " x " << MEGData.cols() << std::endl;
//    EqnB = CoilScale.asDiagonal() * MEGData;
//        std::cout << "EqnB: " << EqnB.rows() << " x " << EqnB.cols() << std::endl;
//    std::cout << "pass 2" << std::endl;

//    LinEqn.append(EqnInRR);
//    LinEqn.append(EqnOutRR);
    LinEqn.append(EqnIn);
    LinEqn.append(EqnOut);
    LinEqn.append(EqnARR);
    LinEqn.append(EqnA);
//    LinEqn.append(EqnB);
    LinEqn.append(CoilScale.asDiagonal());


//    std::cout << "EqnInRR *********************************" << endl << EqnInRR << endl << endl;
//    std::cout << "EqnOutRR ********************************" << endl << EqnOutRR << endl << endl;
//    std::cout << "EqnIn ***********************************" << endl << EqnIn << endl << endl;
//    std::cout << "EqnOut **********************************" << endl << EqnOut << endl << endl;
//    std::cout << "EqnARR **********************************" << endl << EqnARR << endl << endl;
//    std::cout << "EqnA ************************************" << endl << EqnA << endl << endl;
//    std::cout << "EqnB ************************************" << endl << EqnB.transpose() << endl;

//    std::cout << "building SSS linear equation .....finished !" << endl;

    //qDebug() << "buildLinearEqn END";

//    return LinEqn;
    return CoilScale.asDiagonal();
}

void RtSssAlgo::setSSSParameter(QList<int> expansionOrder)
{
//    LInRR = 5;
//    LOutRR = 4;
//    LInOLS = 8;
//    LOutOLS = 4;

    LInRR = expansionOrder[0];
    LOutRR = expansionOrder[1];
    LInOLS = expansionOrder[2];
    LOutOLS = expansionOrder[3];
}

void RtSssAlgo::setMEGInfo(FiffInfo::SPtr fiffInfo, RowVectorXi pickedChannels)
{
    //qDebug() << "setMEGInfo START";

    // Set origin of head(?) coordinate
    Origin.resize(3);
    Origin << 0.0, 0.0, 0.04;

//    // Find the number of MEG channels
//    qint32 nmegchan = 0;
//    for (qint32 i=0; i<fiffInfo->nchan; ++i)
//        if(fiffInfo->chs[i].kind == FIFFV_MEG_CH)
//            nmegchan ++;

//    // Find the number of MEG channels
//    NumMEGChan = 0,
//    NumBadCoil = 0;
//    BadChan.setZero(fiffInfo->nchan);
//    for (qint32 i=0; i<fiffInfo->nchan; ++i)
//        if(fiffInfo->chs[i].kind == FIFFV_MEG_CH)
//        {
//            for(qint32 j = 0; j < fiffInfo->bads.size(); j++)
//                if(fiffInfo->chs[i].ch_name == fiffInfo->bads[j])
//                {
//                    BadChan(i) = 1;
//                    qDebug() << "bad ch: " << i << "(" << fiffInfo->chs[i].ch_name << ")";
//                    NumBadCoil ++;
////                    nmegbadchan ++;
//                }
//            NumMEGChan ++;
//        }

//    NumCoil =  NumMEGChan - NumBadCoil;
//    NumCoil = 249;
    NumCoil = pickedChannels.cols();

//    qDebug() << "number of meg channels: " << NumMEGChan;
//    qDebug() << "number of bad meg channels : " << NumBadCoil;
    qDebug() << "number of meg channels used for rtSSS: " << NumCoil;

//    CoilTk.append("VV_PLANAR_T1");
//    CoilTk.append("VV_MAG_T1");

    // Number of coil integration points for a coil
    CoilNk.resize(NumCoil);

    // Coordinate of coil integrating points:  This is assigned to CoilRk.
    //      7002 (babyMEG circular (10mm diameter) inlayer magnetometer): 7 pts
    //      7003 (babyMEG square (20x20mm) outlayer magnetometer): 7 pts
    //      3012 (VectorView planar grdiometer, Type 1): 8 pts
    //      3024 (VectorView magnetometer,Type 3): 9 pts
    //      initialization order X1.....Xn Y1.....Yn Z1.....Zn
    MatrixXd C7002intpts7(3,7), C7003intpts4(3,4), C7003intpts16(3,16), C3012intpts8(3,8), C3024intpts9(3,9);
    C7002intpts7 << 0.0000000, 0.0020412, -0.0020412, -0.0040825, -0.0020412,  0.0020412, 0.0040825, 0.0000000, 0.0035356,  0.0035356,  0.0000000, -0.0035356, -0.0035356, 0.0000000, 0.0000000, 0.0000000,  0.0000000,  0.0000000,  0.0000000,  0.0000000, 0.0000000;
    C7003intpts4 << 0.0050000, -0.0050000, -0.0050000, 0.0050000, 0.0050000, 0.0050000, 0.0050000,  0.0050000, 0.0000000, 0.0000000, 0.0000000, 0.0000000;
    C7003intpts16 << 0.006, -0.006, -0.006, 0.006, 0.006, -0.006, -0.006, 0.006, 0.002, -0.002, -0.002, 0.002, 0.002, -0.002, -0.002, 0.002, 0.006, 0.006, -0.006, -0.006, 0.002, 0.002, -0.002, -0.002, 0.006, 0.006, -0.006, -0.006, 0.002, 0.002, -0.002, -0.002, 0.000, 0.000, 0.000, 0.000,0.000, 0.000, 0.000, 0.000,0.000, 0.000, 0.000, 0.000,0.000, 0.000, 0.000, 0.000;

    C3012intpts8 << 0.0059, -0.0059, 0.0059, -0.0059, 0.0108, -0.0108, 0.0108, -0.0108, 0.0067, 0.0067, -0.0067, -0.0067, 0.0067, 0.0067, -0.0067, -0.0067, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;
    C3024intpts9 << 0.0000, 0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0100, -0.0100, 0.0000, 0.0100, 0.0100, -0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;

    // Coil point weight:  This is assinged to CoilWk
    MatrixXd C7002Wintpts7(1,7), C7003Wintpts4(1,4), C7003Wintpts16(1,16), C3012Wintpts8(1,8), C3024Wintpts9(1,9);
    C7002Wintpts7 << 0.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125;
    C7003Wintpts4 << 0.25, 0.25, 0.25, 0.25;
    C7003Wintpts16 << 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625;

    C3012Wintpts8 << 14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790;
    C3024Wintpts9 << 0.1975,    0.0772,    0.0772,    0.0772,    0.0772,    0.1235,    0.1235,    0.1235,    0.1235;

//    qint32 cid = 0;
    CoilGrad.setZero(NumCoil);

//    for (qint32 i=0; i<fiffInfo->nchan; ++i)
//    {
//        if(fiffInfo->chs[i].kind == FIFFV_MEG_CH && BadChan(i) == 0)
//        {

//            CoilT.append(fiffInfo->chs[i].coil_trans);
//            CoilName.append(fiffInfo->chs[i].ch_name);

////            qDebug() << "coil type= " << fiffInfo->chs[i].coil_type;

//            switch (fiffInfo->chs[i].coil_type)
//            {
//                case 3012:
//                    CoilGrad(cid) = 1;
//                    CoilNk(cid) = 8;
//                    CoilRk.append(C3012intpts8);
//                    CoilWk.append(C3012Wintpts8);
//                    break;

//                case 3024:
//                    CoilGrad(cid) = 0;
//                    CoilNk(cid) = 9;
//                    CoilRk.append(C3024intpts9);
//                    CoilWk.append(C3024Wintpts9);
//                    break;

//                case 7002:
//                    CoilGrad(cid) = 0;
//                    CoilNk(cid) = 7;
//                    CoilRk.append(C7002intpts7);
//                    CoilWk.append(C7002Wintpts7);
//                    break;

//                case 7003:
//                    CoilGrad(cid) = 0;
//                    CoilNk(cid) = 4;
//                    CoilRk.append(C7003intpts4);
//                    CoilWk.append(C7003Wintpts4);
////                    CoilRk.append(C7003intpts16);
////                    CoilWk.append(C7003Wintpts16);
//                    break;

//                default:
//                    qDebug() << " This coil type is NOT supported";
//                    break;
//            }
////        qDebug() << fiffInfo->chs[i].coil_type << ", CoilNk(" << cid << "): " << CoilNk(cid);
//            cid++;
//        }
//    }

    for (qint32 i=0; i<NumCoil; ++i)
    {
        CoilT.append(fiffInfo->chs[pickedChannels(i)].coil_trans.cast <double>());

//                std::cout << "PickedChID: " << pickedChannels(i) << ", ";
//                std::cout <<  fiffInfo->chs[pickedChannels(i)].coil_trans << std::endl;
//                std::cout <<  fiffInfo->chs[pickedChannels(i)].ch_name << std::endl;
//                std::cout <<  CoilT[i] << std::endl;

        CoilName.append(fiffInfo->chs[pickedChannels(i)].ch_name);

        switch (fiffInfo->chs[pickedChannels(i)].chpos.coil_type)
        {
            case 3012:
                qDebug() << " WARNING !!! Gradiometer is not supported for SSS !!!";
                CoilGrad(i) = 1;
                CoilNk(i) = 8;
                CoilRk.append(C3012intpts8);
                CoilWk.append(C3012Wintpts8);
                break;

            case 3024:
                CoilGrad(i) = 0;
                CoilNk(i) = 9;
                CoilRk.append(C3024intpts9);
                CoilWk.append(C3024Wintpts9);
                break;

            case 7002:
                CoilGrad(i) = 0;
                CoilNk(i) = 7;
                CoilRk.append(C7002intpts7);
                CoilWk.append(C7002Wintpts7);
                break;

            case 7003:
                CoilGrad(i) = 0;
                CoilNk(i) = 4;
                CoilRk.append(C7003intpts4);
                CoilWk.append(C7003Wintpts4);
                break;

            default:
                qDebug() << " This coil type is NOT supported";
                break;
        }
    }


    //qDebug() << "setMEGInfo END";

//    std::cout << "loading MEGData ....";
//    ifstream inMEGData("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGData.txt",ios::in);
//    MEGData.resize(NumCoil,1);
//    VectorXd tmpvec(NumCoil);
//    for(int k=0; k<NumCoil; k++)    inMEGData >> tmpvec(k);
//    MEGData.col(0) = tmpvec;
//    inMEGData.close();
//NumCoil = 275;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% build linear equation for SSS
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% Origin:           origin of device coordinate system [X; Y; Z];
//% LIn:              expansion order for internal basis functions
//% LOut:             expansion order for external basis functions
//% CoilTk{i,1}:      coil type - name
//% CoilNk(i,1):      coil type - number of samples
//% CoilWk{i,1}:      coil type - weight of samples
//% CoilRk{i,1}:      coil type - location of samples
//%                   i: i-th type
//% CoilName{i,1}:    name per coil
//% CoilT{i,1}:       transformation matrix per coil
//%                   i: i-th coil
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% EqnIn(i,1):       internal basis functions
//% EqnOut(i,1):      external basis functions
//%                   i: i-th coil
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//      function [EqnIn,EqnOut] = get_SSS_Eqn(Origin,LIn,LOut,CoilTk,CoilNk,CoilWk,CoilRk,CoilName,CoilT)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
QList<MatrixXd> RtSssAlgo::getSSSEqn(qint32 LIn, qint32 LOut)
//QList<MatrixXd> RtSssAlgo::getSSSEqn(VectorXi Lexp)
{
    //qDebug() << "getSSSEqn START";

    int NumBIn, NumBOut;
//    int coil_index=1;
    double RScale;
    VectorXd coil_distance, coil_clocation(4,1), coil_vector(4,1);
    MatrixXd coil_location;
    MatrixXd EqnIn, EqnOut;
    QList<MatrixXd> Eqn;

//    qint32 LIn,LOut;
//    LIn = Lexp(0);
//    LOut = Lexp(1);

    //  % initialization
    NumBIn = (LIn*LIn) + 2*LIn;
    NumBOut = (LOut*LOut) + 2*LOut;

//% calculate scaling factor for distance
    coil_distance.resize(NumCoil);

    for(int i = 0; i<NumCoil; i++)
    {
        coil_clocation = CoilT[i].block(0,3,3,1);
        coil_distance(i) = (coil_clocation-Origin).norm();
    }

    RScale = exp(coil_distance.array().log().mean());

//% build linear equation for internal/external basis functions
    EqnIn.setZero(NumCoil,NumBIn);
    EqnOut.setZero(NumCoil,NumBOut);

    for(int i = 0; i<NumCoil; i++)
    {
//        if (CoilName[i] == CoilTk[0])
//            coil_index = 0;
//        else if (CoilName[i] == CoilTk[1])
//            coil_index = 1;
//        else
//        {
//            cout<< "There is no matching coil" << endl;
//            exit(1);
//        }

//    % calculate coil orientation
        coil_vector = CoilT[i].block(0,2,3,1);

//    % calculate coil locations (multiple points)
//        int NumCoilPts = CoilNk(coil_index);
        qint32 NumCoilPts = CoilNk(i);
        MatrixXd tmpmat; tmpmat.setOnes(4,NumCoilPts);
//qDebug() << "pass 2";
//        tmpmat.topRows(3) = CoilRk[coil_index];
//        std::cout << "CoilRk: " << CoilRk[i].rows() << " x " << CoilRk[i].cols()  << std::endl;
        tmpmat.topRows(3) = CoilRk[i];
//qDebug() << "pass 3";
        coil_location = CoilT[i] * tmpmat;
        tmpmat = coil_location.topRows(3);   coil_location.resize(3,NumCoilPts);   coil_location = tmpmat;
        coil_location = coil_location - Origin.replicate(1,NumCoilPts);
        coil_location = coil_location / RScale;

//    % build linear equation
        getSSSBasis(coil_location.row(0).transpose(), coil_location.row(1).transpose(), coil_location.row(2).transpose(), LIn, LOut);
//        std::cout << "NumCoilPts: " << NumCoilPts  << ",    NumBIn: " << NumBIn << ",    NumBOut: " << NumBOut << endl;
//        std::cout << "BInX: " << BInX.rows()  << " x " << BInX.cols() << endl;

        MatrixXd b_in, b_out;
        b_in = coil_vector(0)*BInX + coil_vector(1)*BInY + coil_vector(2)*BInZ;
        b_out = coil_vector(0)*BOutX + coil_vector(1)*BOutY + coil_vector(2)*BOutZ;
//        std::cout << "b_in: Coil= " << i+1 << endl << b_in << endl;
//        std::cout << "b_out: Coil= " << i+1 << endl << b_out << endl;

//        EqnIn.block(i,0,1,NumBIn) = CoilWk[coil_index] * b_in;
//        EqnOut.block(i,0,1,NumBOut) = CoilWk[coil_index] * b_out;
        EqnIn.block(i,0,1,NumBIn) = CoilWk[i] * b_in;
        EqnOut.block(i,0,1,NumBOut) = CoilWk[i] * b_out;
    }

    Eqn.append(EqnIn);
    Eqn.append(EqnOut);
//    std::cout << "EqnIn: Coil 1 " << endl << Eqn[0].row(0) << endl;
//    std::cout << "EqnIn: Coil 306 " << endl << Eqn[0].row(305) << endl;
//    std::cout << "EqnOut: Coil 1 " << endl << Eqn[1].row(0) << endl;
//    std::cout << "EqnOut: Coil 306 " << endl << Eqn[1].row(305) << endl;

    //qDebug() << "getSSSEqn END";

    return Eqn;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% generate SSS basis vectors at given locations
//% -- real basis function
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% X/Y/Z(i,1):       Cartesian coordinate
//%                   i: i-th point
//% LIn:              expansion order for internal basis functions
//% LOut:             expansion order for external basis functions
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% BInX/Y/Z(i,j):    internal basis vectors
//% BOutX/Y/Z(i,j):   external basis vectors
//%                   i: i-th point
//%                   j: j-th basis function
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//      function [BInX,BInY,BInZ,BOutX,BOutY,BOutZ] = get_SSS_Basis(X,Y,Z,LIn,LOut)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void RtSssAlgo::getSSSBasis(VectorXd X, VectorXd Y, VectorXd Z, qint32 LIn, qint32 LOut)
{
    //qDebug() << "getSSSBasis START";

    int NumSample, NumBIn, NumBOut;
    int LMax;

    QList<MatrixXd> P, dP_dx;
    MatrixXd cur_p, cur_p1, cur_p2;

    QList<MatrixXcd> YP, dY_dTHETA;
    VectorXcd cur_phi;

    MatrixXd BInR, BInPHI, BInTHETA;
    int basis_index;
    VectorXd scale_r;
    VectorXcd cur_R, cur_PHI, cur_THETA;
    MatrixXd BOutR, BOutPHI, BOutTHETA;

//  % check input parameters
//  % -- R ~= 0 & THETA ~= 0
    for(int i=0; i<X.rows(); i++)
        if ( (std::fabs(X(i)) < 1e-30) && (std::fabs(Y(i)) < 1e-30) )
        {
//            std::cout << "Zero THETA detected!****  ";
            //std::cout << "X, Y: " << X(i) << ",  " << Y(i) << endl;
            break;
            //return;
            //exit(1);
        }

//  % initialization
    LMax = qMax(LIn,LOut);
    NumSample = X.size();
    NumBIn = (LIn*LIn) + 2*LIn;
    NumBOut = (LOut*LOut) + 2*LOut;

//  % Coordinate transdorm:  (X,Y,Z) --> (R, PHI, THETA)
    getCartesianToSpherCoordinate(X,Y,Z);
//    std::cout << "X, Y, Z: " << endl << X.transpose() << endl << Y.transpose() << endl << Z.transpose() << endl;
//    std::cout << "R, PHI, THETA: " << endl << R.transpose() << endl << PHI.transpose() << endl << THETA.transpose() << endl;

//  % calculate P
    for(int l=1; l<=LMax; l++)
    {
        cur_p = legendre(l, THETA.array().cos());
        P.append(cur_p.transpose());
    }
//    std::cout << "Legendre Polynomial 1-----" << endl << P[0].transpose() << endl;
//    std::cout << "Legendre Polynomial 2-----" << endl << P[1].transpose() << endl;
//    std::cout << "Legendre Polynomial 3-----" << endl << P[2].transpose() << endl;
//    std::cout << "Legendre Polynomial 4-----" << endl << P[3].transpose() << endl;
//    std::cout << "Legendre Polynomial 5-----" << endl << P[4].transpose() << endl;

//  % calculate dP_dx
    for(int l=0; l<LMax; l++)
    {
        dP_dx.append(MatrixXd::Zero(NumSample,(l+1)+1));
        for(int m=0; m <= l+1; m++)
        {
            cur_p1 = P[l].col(m);
//            std::cout << "cur_p1. m= " << m << endl << cur_p1.transpose() << endl;
            if (m == 0)
            {
                cur_p2 = -factorial((l+1)-1)/factorial((l+1)+1) * P[l].col(1);
            }
            else
            {
                cur_p2 = P[l].col(m-1);
            }
//            std::cout << "cur_p2. m=." << m << endl << cur_p2.transpose() << endl;

            //            std::cout << "THETA: " << THETA.rows() << " x " << THETA.cols() << endl;
            //            std::cout << "cur_p1: " << cur_p1.rows() << " x " << cur_p1.cols() << endl;
            //            std::cout << "P[0]: " << P[0].col(1) << endl;
//            std::cout << "dp_dx[" << l << "]: " << endl << dP_dx[l] << endl;
            dP_dx[l].col(m) = (m*THETA.array().cos() * cur_p1.array() + (((l+1)+m)*((l+1)-m+1)*THETA.array().sin()) * cur_p2.array()) / THETA.array().sin().pow(2);
        }

//        std::cout << "dP_dx[" << l << "]" <<endl << dP_dx[l] << endl;
    }

//  % calculate Y & dY_dTHETA
    for(int l=0; l<LMax; l++)
    {
        YP.append(MatrixXcd::Zero(NumSample,(l+1)+1));
        dY_dTHETA.append(MatrixXcd::Zero(NumSample,(l+1)+1));

        for(int m=0; m <=l+1; m++)
        {
//         % cur_phi = sqrt((2*l+1)*factorial(l-m)/factorial(l+m)/2/pi) * exp(sqrt(-1)*m*PHI);
//         % Y{l,1}(:,m+1) = cur_phi .* P{l,1}(:,m+1);
//         % dY_dTHETA{l,1}(:,m+1) = -cur_phi .* sin(THETA) .* dP_dx{l,1}(:,m+1);
            cur_phi = sqrt((2*(l+1)+1)*factorial((l+1)-m)/factorial((l+1)+m)/2/M_PI) * (sqrt(cplxd(-1,0)) * cplxd(m,0) * PHI.cast<cplxd >()).array().exp();
//            std::cout << "cur_phi. m=." << m << endl << cur_phi.transpose() << endl;

            YP[l].col(m) = cur_phi.array() * P[l].col(m).array().cast<cplxd >();
            dY_dTHETA[l].col(m) = -cur_phi.array() * THETA.array().sin().cast<cplxd >() * dP_dx[l].col(m).array().cast<cplxd >();
        }
//        std::cout << "YP[" << l << "]" <<endl << YP[l] << endl;
//        std::cout << "dY_dTHETA[" << l << "]" <<endl << dY_dTHETA[l] << endl;
    }

//  % calculate BIn (spherical coordinate)
    BInR.setZero(NumSample,NumBIn);
    BInPHI.setZero(NumSample,NumBIn);
    BInTHETA.setZero(NumSample,NumBIn);
    basis_index = 0;
    for(int l=0; l<LIn; l++)
    {
        scale_r= R.array().pow((l+1)+2);

        BInR.col(basis_index) = -((l+1)+1) * YP[l].col(0).array().real() / scale_r.array();
        BInPHI.col(basis_index) = VectorXd::Zero(NumSample);
        BInTHETA.col(basis_index) = dY_dTHETA[l].col(0).array().real() / scale_r.array();
        basis_index = basis_index + 1;

//        std::cout << "BInR" << endl << BInR << endl;

        for(int m=0; m<=l; m++)
        {
            cur_R = -sqrt(2) * ((l+1)+1) * YP[l].col(m+1).array() / scale_r.array().cast<cplxd >();
            cur_PHI = sqrt(cplxd(-2,0)) * cplxd(m+1,0) * YP[l].col(m+1).array() / THETA.array().sin().cast<cplxd >() / scale_r.array().cast<cplxd >();
            cur_THETA = sqrt(2) * dY_dTHETA[l].col(m+1).array() / scale_r.array().cast<cplxd >();
//            std::cout << "cur_R,  m=" << m << endl << cur_R.transpose() << endl;

            BInR.col(basis_index) = cur_R.real();
            BInPHI.col(basis_index) = cur_PHI.real();
            BInTHETA.col(basis_index) = cur_THETA.real();
            basis_index = basis_index + 1;

            BInR.col(basis_index) = cur_R.imag();
            BInPHI.col(basis_index) = cur_PHI.imag();
            BInTHETA.col(basis_index) = cur_THETA.imag();
            basis_index = basis_index + 1;
        }
    }
//    std::cout << "BInR" << endl << BInR << endl;
//    std::cout << "BInPHI" << endl << BInPHI << endl;
//    std::cout << "BInTHETA" << endl << BInTHETA << endl;

//  % calculate BOut (spherical coordinate)
    BOutR.setZero(NumSample,NumBOut);
    BOutPHI.setZero(NumSample,NumBOut);
    BOutTHETA.setZero(NumSample,NumBOut);
    basis_index = 0;
    for(int l=0; l<LOut; l++)
    {
        scale_r= R.array().pow((l+1)-1);

        BOutR.col(basis_index) = (l+1) * YP[l].col(0).array().real() * scale_r.array();
        BOutPHI.col(basis_index) = VectorXd::Zero(NumSample);
        BOutTHETA.col(basis_index) = dY_dTHETA[l].col(0).array().real() * scale_r.array();
        basis_index = basis_index + 1;

        for(int m=0; m<=l; m++)
        {
            cur_R = sqrt(2) * (l+1) * YP[l].col(m+1).array() * scale_r.array().cast<cplxd >();
            cur_PHI = sqrt(cplxd(-2,0)) * cplxd(m+1,0) * YP[l].col(m+1).array() / THETA.array().sin().cast<cplxd >() * scale_r.array().cast<cplxd >();
            cur_THETA = sqrt(2) * dY_dTHETA[l].col(m+1).array() * scale_r.array().cast<cplxd >();
            BOutR.col(basis_index) = cur_R.real();
            BOutPHI.col(basis_index) = cur_PHI.real();
            BOutTHETA.col(basis_index) = cur_THETA.real();
            basis_index = basis_index + 1;
            BOutR.col(basis_index) = cur_R.imag();
            BOutPHI.col(basis_index) = cur_PHI.imag();
            BOutTHETA.col(basis_index) = cur_THETA.imag();
            basis_index = basis_index + 1;
        }
    }
//        std::cout << "BOutR" << endl << BOutR << endl;
//        std::cout << "BOutPHI" << endl << BOutPHI << endl;
//        std::cout << "BOutTHETA" << endl << BOutTHETA << endl;

//  % convert from spherical coordinate to Cartesian coordinate
//    [R_X,R_Y,R_Z,PHI_X,PHI_Y,PHI_Z,THETA_X,THETA_Y,THETA_Z] = get_Sph_2_Cart_Vector(R,PHI,THETA);
    getSphereToCartesianVector();
    BInX = BInR.array() * (R_X.replicate(1,NumBIn)).array() + BInPHI.array() * (PHI_X.replicate(1,NumBIn)).array() + BInTHETA.array() * (THETA_X.replicate(1,NumBIn)).array();
    BInY = BInR.array() * (R_Y.replicate(1,NumBIn)).array() + BInPHI.array() * (PHI_Y.replicate(1,NumBIn)).array() + BInTHETA.array() * (THETA_Y.replicate(1,NumBIn)).array();
    BInZ = BInR.array() * (R_Z.replicate(1,NumBIn)).array() + BInPHI.array() * (PHI_Z.replicate(1,NumBIn)).array() + BInTHETA.array() * (THETA_Z.replicate(1,NumBIn)).array();
    BOutX = BOutR.array() * (R_X.replicate(1,NumBOut)).array() + BOutPHI.array() * (PHI_X.replicate(1,NumBOut)).array() + BOutTHETA.array() * (THETA_X.replicate(1,NumBOut)).array();
    BOutY = BOutR.array() * (R_Y.replicate(1,NumBOut)).array() + BOutPHI.array() * (PHI_Y.replicate(1,NumBOut)).array() + BOutTHETA.array() * (THETA_Y.replicate(1,NumBOut)).array();
    BOutZ = BOutR.array() * (R_Z.replicate(1,NumBOut)).array() + BOutPHI.array() * (PHI_Z.replicate(1,NumBOut)).array() + BOutTHETA.array() * (THETA_Z.replicate(1,NumBOut)).array();

    //qDebug() << "getSSSBasis END";

//        std::cout << "BInX" << endl << BInX << endl;
//        std::cout << "BInY" << endl << BInY << endl;
//        std::cout << "BInZ" << endl << BInZ << endl;
//        std::cout << "BOutX" << endl << BOutX << endl;
//        std::cout << "BOutY" << endl << BOutY << endl;
//        std::cout << "BOutZ" << endl << BOutZ << endl;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%
//% RtSssAlgo::getCartesianToSpherCeoordinate(double X, double Y, double Z
//%
//%      convert point coordinate from Cartesian to spherical
//%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% X/Y/Z(i,1):       Cartesian coordinate
//%                   i: i-th point
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% R/PHI/THETA(i,1): spherical coordinate
//%                   i: i-th point
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//      function [R,PHI,THETA] = get_Cart_2_Sph_Coordinate(X,Y,Z)
//          hypotxy = hypot(X,Y);
//          R = hypot(hypotxy,Z);
//          PHI = atan2(Y,X);
//          THETA = atan2(hypotxy,Z);
//      return;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void RtSssAlgo::getCartesianToSpherCoordinate(VectorXd X, VectorXd Y, VectorXd Z)
{
    //qDebug() << "getCartesianToSpherCoordinate START";

    VectorXd hypotxy;
//    std::cout << "X: " << X.rows() << " x " << X.cols() << endl;
//    std::cout << "Y: " << Y.rows() << " x " << Y.cols() << endl;

    hypotxy = hypot(X,Y);
    R = hypot(hypotxy,Z);
    PHI = atan2vec(Y,X);
    THETA = atan2vec(hypotxy,Z);

    //qDebug() << "getCartesianToSpherCoordinate END";

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%
//% RtSssAlgo::getSphereToCartesianVector(double R, double PHI, double THETA)
//%
//%     convert unit vector from spherical to Cartesian
//%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% R/PHI/THETA(i,1):             spherical coordinate
//%                               i: i-th point
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% R_X/R_Y/R_Z(i,1):             unit vector R in Cartesian coordinate
//% PHI_X/PHI_Y/PHI_Z(i,1):       unit vector PHI in Cartesian coordinate
//% THETA_X/THETA_Y/THETA_Z(i,1): unit vector THETA in Cartesian coordinate
//%                               i: i-th point
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    function [R_X,R_Y,R_Z,PHI_X,PHI_Y,PHI_Z,THETA_X,THETA_Y,THETA_Z] = get_Sph_2_Cart_Vector(R,PHI,THETA)
//        NumExp = size(R,1);
//        R_X = sin(THETA) .* cos(PHI);
//        R_Y = sin(THETA) .* sin(PHI);
//        R_Z = cos(THETA);
//        PHI_X = -sin(PHI);
//        PHI_Y = cos(PHI);
//        PHI_Z = zeros(NumExp,1);
//        THETA_X = cos(THETA) .* cos(PHI);
//        THETA_Y = cos(THETA) .* sin(PHI);
//        THETA_Z = -sin(THETA);
//    return;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void RtSssAlgo::getSphereToCartesianVector()
{
    int NumExp = R.size();

    R_X = THETA.array().sin() * PHI.array().cos();
    R_Y = THETA.array().sin() * PHI.array().sin();
    R_Z = THETA.array().cos();

    PHI_X = -PHI.array().sin();
    PHI_Y = PHI.array().cos();
    PHI_Z = VectorXd::Zero(NumExp,1);


    THETA_X = THETA.array().cos() * PHI.array().cos();
    THETA_Y = THETA.array().cos() * PHI.array().sin();
    THETA_Z = -THETA.array().sin();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% SSS by robust regression (subspace iteration & low-rank update)
//% -- RR_K1 is determined by bi-square function (w = 0.75 at RR_K1)
//% -- RR_K2 is determined by bi-square function (95% statistical efficiency)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% EqnIn(i,j):       internal basis functions
//% EqnOut(i,j):      external basis functions
//%                   i: i-th coil
//%                   j: j-th basis function
//% EqnARR(i,j):      SSS equation (LHS) after scaling -- subspace
//%                   i: i-th coil
//%                   j: j-th basis function
//% EqnA(i,j):        SSS equation (LHS) after scaling -- full
//%                   i: i-th coil
//%                   j: j-th basis function
//% EqnB(i,j):        SSS equation (RHS) after scaling
//%                   i: i-th coil
//%                   j: j-th sample in time domain
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% SSSIn(i,j):       internal MEG signal recovered by SSS
//% SSSOut(i,j):      external MEG signal recovered by SSS
//% Weight(i,j):      optimal weight for MEG coil
//% ErrRel(1,j):      relative residual of over-determined linear equation
//%                   i: i-th coil
//%                   j: j-th sample in time domain
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  function [SSSIn,SSSOut,Weight,ErrRel] = get_SSS_RR(EqnIn,EqnOut,EqnARR,EqnA,EqnB)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//QList<MatrixXd> RtSssAlgo::getSSSRR(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnARR, MatrixXd EqnA, MatrixXd EqnB)
//QList<MatrixXd> RtSssAlgo::getSSSRR(MatrixXd EqnB)
MatrixXd RtSssAlgo::getSSSRR(MatrixXd EqnB)
{
    //qDebug() << "getSSSRR START";

    int NumBIn, NumBOut, NumCoil, NumExp;
    MatrixXd EqnRRInv, EqnInv, SSSIn, SSSOut, Weight; //, ErrRel;
    VectorXd ErrRel;
    double RR_K1, RR_K2, RR_K3;
    double eqn_scale0, eqn_scale;
    MatrixXd sol_X, sol_X_old, eqn_Y, eqn_D, temp_M, temp_N, sol_in, sol_out;
    MatrixXd diagMat;
    VectorXd eqn_err, weight_index;
    QList<MatrixXd> RRsss;

//  % error tolerance for robust regression
    double ErrTolRel = 1e-3;

//  % weight threshold for robust regression
    double WeightThres = 1 - 1e-6;

//  % initialization
    NumBIn = EqnIn.cols();
    NumBOut = EqnOut.cols();
    NumCoil = EqnB.rows();
    NumExp = EqnB.cols();

    EqnRRInv = (EqnARR.transpose() * EqnARR).inverse();
    EqnInv = (EqnA.transpose() * EqnA).inverse();

    SSSIn.setZero(NumCoil,NumExp);
    SSSOut.setZero(NumCoil,NumExp);
    Weight.setZero(NumCoil,NumExp);
    ErrRel.setZero(NumExp);
    RR_K3 = 3;
    RR_K2 = 4.685;
    RR_K1 = qSqrt(1-qSqrt(3)/2) * RR_K2;

    for(int i=0; i<NumExp; i++)
    {
//      % solve OLS solution
        sol_X = EqnRRInv * (EqnARR.transpose() * EqnB.col(i));
//        std::cout << "sol_X ************************" << endl << sol_X.transpose() << endl;

//      % scale linear equation
        eqn_err = EqnARR * sol_X - EqnB.col(i);
        eqn_scale0 = stdev(eqn_err);
        eqn_err = eqn_err.cwiseAbs() / eqn_scale0;

//      % solve iteratively re-weighted least squares (Bi-Square) -- subspace
        sol_X_old.setConstant(sol_X.rows(), sol_X.cols(), 1e30);
//        while (((sol_X-sol_X_old).norm() / sol_X.norm()) > ErrTolRel)
        int cnt=0;
        while (((sol_X.array()-sol_X_old.array()).matrix().norm() / sol_X.norm()) > ErrTolRel)
//        for (int h=0; h<2; h++)
        {
            cnt++;
//            cout << "while cnt: " << cnt << endl;
            sol_X_old = sol_X;
//            Weight(:,i) = (eqn_err <= RR_K1) + (eqn_err > RR_K1 & eqn_err <= RR_K2) .* (1-(eqn_err-RR_K1).^2/(RR_K2-RR_K1)^2).^2;
//            Weight.col(i) = eigen_LTE(eqn_err, RR_K1) + eigen_AND(eigen_GT(eqn_err, RR_K1),eigen_LTE(eqn_err, RR_K2));
            Weight.col(i) = eigen_LTE(eqn_err,RR_K1).array() + eigen_AND(eigen_GT(eqn_err,RR_K1),eigen_LTE(eqn_err,RR_K2)).array() * (1 - ((eqn_err.array()-RR_K1).pow(2)) / pow(RR_K2-RR_K1,2) ).pow(2);
//            std::cout << "Weight **************************" << endl << Weight.transpose() << endl;

//          % weight_index = find(Weight(:,i) < WeightThres);
            weight_index = eigen_LT_index(Weight.col(i), WeightThres);
//            weight_index = eigen_LT_index_test(Weight.col(i), h);
//            std:cout << "weight_index ********************* " << endl << weight_index << endl;

//          % eqn_Y = EqnARR(weight_index,:);   eqn_D = Weight(weight_index,i) - 1;
            eqn_Y.resize(weight_index.size(), EqnARR.cols());
            eqn_D.resize(weight_index.size(),1);
            for(int k=0; k<weight_index.size(); k++)
            {
                eqn_Y.row(k) = EqnARR.row(weight_index(k));
//                std::cout << "eqn_Y: " << eqn_Y.rows() << " x " << eqn_Y.cols() << endl;
//                std::cout << "eqn_D: "  << eqn_D.rows() << " x " << eqn_D.cols() << endl;
                eqn_D(k) = Weight(weight_index(k),i) - 1;
            }
//              std::cout << "eqn_D: " << endl << eqn_D.transpose() << endl << endl;
            temp_M = EqnARR.transpose() * (Weight.col(i).array() * EqnB.col(i).array()).matrix();
//              std::cout << "temp_M: " << endl << temp_M.transpose() << endl << endl;

//            std::cout << "EqnRRInv: " << EqnRRInv.rows() << " x " << EqnRRInv.cols() << endl;
//            std::cout << "eqn_Y: " << eqn_Y.rows() << " x " << eqn_Y.cols() << endl;
//              std::cout << "EqnRRInv: " << endl << EqnRRInv.transpose() << endl << endl;
//              std::cout << "eqn_Y: " << endl << eqn_Y.transpose() << endl << endl;

            temp_N = EqnRRInv * eqn_Y.transpose();
//              std::cout << "temp_N: " << endl << temp_N.transpose() << endl << endl;


//            diagMat = eqn_D.asDiagonal();
//        std::cout << "diagMat ************************" << endl << diagMat.transpose() << endl;
//            diagMat = 1 / diagMat.array();
//        std::cout << "diagMat ************************" << endl << diagMat.transpose() << endl;

            diagMat = (1 / eqn_D.array()).matrix().asDiagonal();
//            cout << "diagMat ************************" << i+1 << endl << diagMat.rows() << " x" << diagMat.cols() << endl;

            sol_X = EqnRRInv * temp_M - temp_N * (diagMat + eqn_Y * temp_N).inverse() * (temp_N.transpose() * temp_M);
            eqn_err = (EqnARR * sol_X - EqnB.col(i)).cwiseAbs();
            eqn_scale = qMin(eqn_scale0, RR_K3 * qSqrt((Weight.col(i).array() * eqn_err.array() * eqn_err.array()).mean()));
            eqn_err = eqn_err / eqn_scale;
//             std::cout << "temp_M: " << endl << temp_M.transpose() << endl << endl;

//            std::cout << "(sol_X.array()-sol_X_old.array()).matrix().norm(): " << (sol_X.array()-sol_X_old.array()).matrix().norm() << endl << endl;
//            std::cout << "sol_X.norm():  " << sol_X.norm()  << endl  << endl;
//        std::cout << "sol_X ************************" << endl << sol_X.transpose() << endl << endl;

        }
//      % solve weighted SSS - full
//      % eqn_Y = EqnA(weight_index,:); temp_M = EqnA' * (Weight(:,i).*EqnB(:,i)); temp_N = EqnInv * eqn_Y';
        eqn_Y.resize(weight_index.size(), NumBIn+NumBOut);
        for(int k=0; k<weight_index.size(); k++) eqn_Y.row(k) = EqnA.row(weight_index(k));
        temp_M = EqnA.transpose() * (Weight.col(i).array() * EqnB.col(i).array()).matrix();
        temp_N = EqnInv * eqn_Y.transpose();


//      % sol_X = EqnInv * temp_M - temp_N * ((diag(1./eqn_D) + eqn_Y * temp_N); // \ (temp_N'*temp_M));
//        diagMat = eqn_D.asDiagonal();
//        diagMat = 1 / diagMat.array();
        diagMat = (1 / eqn_D.array()).matrix().asDiagonal();
        sol_X = EqnInv * temp_M - temp_N * (diagMat + eqn_Y * temp_N).inverse() * (temp_N.transpose() * temp_M);

        ErrRel(i) = (EqnA * sol_X - EqnB.col(i)).norm() / EqnB.col(i).norm();

        sol_in = sol_X.block(0,0,NumBIn,1);
        sol_out = sol_X.block(NumBIn,0,NumBOut,1);  // sol_X(NumBIn+1:NumBIn+NumBOut,1);

//      % recover internal/external MEG siganl
        SSSIn.col(i) = EqnIn * sol_in;
        SSSOut.col(i) = EqnOut * sol_out;

    }
    RRsss.append(SSSIn);
    RRsss.append(SSSOut);
    RRsss.append(Weight);
    RRsss.append(ErrRel);

    //qDebug() << "getSSSRR END";

//    return RRsss;
    return SSSIn;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% SSS by OLS
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% EqnIn(i,j):       internal basis functions
//% EqnOut(i,j):      external basis functions
//%                   i: i-th coil
//%                   j: j-th basis function
//% EqnA(i,j):        SSS equation (LHS) after scaling
//%                   i: i-th coil
//%                   j: j-th basis function
//% EqnB(i,j):        SSS equation (RHS) after scaling
//%                   i: i-th coil
//%                   j: j-th sample in time domain
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//% SSSIn(i,j):       internal MEG signal recovered by SSS
//% SSSOut(i,j):      external MEG signal recovered by SSS
//% ErrRel(1,j):      relative residual of over-determined linear equation
//%                   i: i-th coil
//%                   j: j-th sample in time domain
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//function [SSSIn,SSSOut,ErrRel] = get_SSS_OLS(EqnIn,EqnOut,EqnA,EqnB)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//QList<MatrixXd> RtSssAlgo::getSSSOLS(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnA, MatrixXd EqnB)
//QList<MatrixXd> RtSssAlgo::getSSSOLS(MatrixXd EqnB)
MatrixXd RtSssAlgo::getSSSOLS(MatrixXd EqnB)
{
    //qDebug() << "getSSSOLS START";

    int NumBIn, NumBOut, NumCoil, NumExp;
    MatrixXd EqnRRInv, EqnInv, SSSIn, SSSOut;
    MatrixXd sol_X, sol_in, sol_out;
    VectorXd ErrRel;
    QList<MatrixXd> OLSsss;

//  % initialization
    NumBIn = EqnIn.cols();
    NumBOut = EqnOut.cols();
    NumCoil = EqnB.rows();
    NumExp = EqnB.cols();

    EqnRRInv = (EqnA.transpose() * EqnA).inverse();
    SSSIn.setZero(NumCoil,NumExp);
    SSSOut.setZero(NumCoil,NumExp);
    ErrRel.setZero(NumExp);

    for (int i=0; i<NumExp; i++)
    {
//      % solve OLS solution
        sol_X = EqnRRInv * (EqnA.transpose() * EqnB.col(i));

        ErrRel(i) = (EqnA * sol_X - EqnB.col(i)).norm() / EqnB.col(i).norm();

        sol_in = sol_X.block(0,0,NumBIn,1);

        sol_out = sol_X.block(NumBIn,0, NumBOut,1);    // probably NumBIn+0 is correct.  Please debug !!!!

//      % recover internal/external MEG siganl
        SSSIn.col(i) = EqnIn * sol_in;
        SSSOut.col(i) = EqnOut * sol_out;
    }

    OLSsss.append(SSSIn);
    OLSsss.append(SSSOut);
    OLSsss.append(ErrRel);

    //qDebug() << "getSSSOLS END";

//    return OLSsss;
    return SSSIn;
}

// Return number of meg channels
qint32 RtSssAlgo::getNumMEGChanUsed()
{
    return NumCoil;
}

qint32 RtSssAlgo::getNumMEGChan()
{
    return NumMEGChan;
}

VectorXi RtSssAlgo::getBadChan()
{
    return BadChan;
}

qint32 RtSssAlgo::getNumMEGBadChan()
{
    return NumBadCoil;
}

// Set MEG signal
//void RtSssAlgo::setMEGsignal(MatrixXd megfrombuffer)
//{
//    MEGData = megfrombuffer;
//    EqnB = megfrombuffer;
////        std::cout << "MEGData: " << MEGData.rows() << " x " << MEGData.cols() << std::endl;
//}

QList<MatrixXd> RtSssAlgo::getLinEqn()
{
    QList<MatrixXd> LinEqn;

    LinEqn.append(EqnIn);
    LinEqn.append(EqnOut);
    LinEqn.append(EqnARR);
    LinEqn.append(EqnA);
    LinEqn.append(EqnB);

    return LinEqn;
}


// Legendre Polyn0mial
MatrixXd legendre(int l, VectorXd x)
{
    MatrixXd outP;
//   std::cout << "x: " << x.rows() << " x " << x.cols() << endl;

    outP.setZero(l+1, x.size());
//   std::cout << " outP: " << outP.rows() << " x " << outP.cols() << endl;

    for(int m=0; m<=l; m++)
    {
        for(int k=0; k<x.size(); k++)
            outP(m,k) = plgndr(l,m,x(k));
    }

    return outP;
}


float plgndr(int l, int m, float x)
{
//    void nrerror(char error_text[]);
    float fact,pll,pmm,pmmp1,somx2;
    int i,ll;

    if (m < 0 || m > l || std::fabs(x) > 1.0)
        std::cout << "Bad arguments in routine plgndr";
//        nrerror("Bad arguments in routine plgndr");
    pmm=1.0;
    if (m > 0)
    {
        somx2=sqrt((1.0-x)*(1.0+x));
        fact=1.0;
        for (i=1;i<=m;i++)
        {
            pmm *= -fact*somx2;
            fact += 2.0;
        }
    }
    if (l == m)
        return pmm;
    else {
        pmmp1=x*(2*m+1)*pmm;
        if (l == (m+1))
            return pmmp1;
        else
        {
            for (ll=m+2;ll<=l;ll++)
            {
                pll=(x*(2*ll-1)*pmmp1-(ll+m-1)*pmm)/(ll-m);
                pmm=pmmp1;
                pmmp1=pll;
            }
            return pll;
        }
    }
}

//---------------------------------------------------------------------
/*  MATLAB: hypot.m
 Robust computation of the square root of the sum of squares.
 C = hypot(A,B) returns SQRT(ABS(A).^2+ABS(B).^2) carefully computed to
 avoid underflow and overflow.*/
VectorXd hypot(VectorXd X, VectorXd Y)
{
    VectorXd rst;
    rst = (X.array().abs().pow(2) + Y.array().abs().pow(2)).sqrt();
    return rst;
}

//---------------------------------------------------------------------
// atan2() for vector
// since no
VectorXd atan2vec(VectorXd a, VectorXd b)
{
    int n = a.size();
    VectorXd at2(n);

    for(int i=0; i<n; i++)
    {
        at2(i) = qAtan2(a(i),b(i));
    }

    return at2;
}

VectorXd find(MatrixXd M, int I)
{
    VectorXd outIDX;
    int k=0;
    for(int i=0; i<M.cols(); i++)
        for(int j=0; j<M.rows(); j++)
            if (M(i,j) == I)
            {
                outIDX(k) = i*M.rows() + j;
                k++;
            }
    return outIDX;
}



double factorial(int n)
{
    double fac=1;

    for(int i=2; i<=n; i++)  fac *= i;

    return fac;
}


//----------------------------------------------------------------------
// standard deviation of vector
double stdev(VectorXd V)
{
    int size = V.size();
    double ave, sum=0;

    ave = V.mean();
    for(int i=0; i<size; i++)
    {
       sum += pow((V(i)-ave),2);
    }
    return sqrt(sum/size);
}

//
// Check matrix elements to see if they are less than or equal to the given tolerance
// Returns a vector whose elements consists of 1 if true, or 0 if false
VectorXd eigen_LTE(VectorXd V, double tol)
{
    int row = V.rows();
    VectorXd outTrue = VectorXd::Zero(row);

    for(int i=0; i<row; i++)
            if(V(i) <= tol) outTrue(i) = 1;
//    std::cout << "LTE **********************" << outTrue.transpose() << endl;
    return outTrue;
}

//
// Check matrix elements to see if they are greater than the given tolerance
// Returns a vector whose elements consists of 1 if true, or 0 if false
VectorXd eigen_GT(VectorXd V, double tol)
{
    int row = V.rows();
    VectorXd outTrue = VectorXd::Zero(row);

    for(int i=0; i<row; i++)
            if(V(i) > tol) outTrue(i) = 1;
    return outTrue;
}

//
// Check if the elements of two matrices are both true
// Returns a vector whose elements consists of 1 if true, or 0 if false
VectorXd eigen_AND(VectorXd V1, VectorXd V2)
{
    int row = V1.rows();
    VectorXd outTrue = VectorXd::Zero(row);

    for(int i=0; i<row; i++)
            if(V1(i) && V2(i)) outTrue(i) = 1;
//    std::cout << "AND **********************" << outTrue.transpose() << endl;
    return outTrue;
}

// Check matrix elements to see if they are less than the given tolerance
// Returns a vector whose elements consists index of true
VectorXd eigen_LT_index(VectorXd V, double tol)
{
    int cnt = 0;
    int size = V.size();
    VectorXd outIdx; outIdx.setZero(size);

    for(int i=0; i<size; i++)
        if(V(i) < tol)
        {
            outIdx(cnt) = i;
            cnt++;
        }
    return outIdx.head(cnt);
}

//VectorXd eigen_LT_index_test(VectorXd V, int cnt)
VectorXd eigen_LT_index_test(int cnt)
{
    VectorXd outIdx = VectorXd::Zero(cnt+1);

    for (int i=0; i<cnt+1; i++)
        outIdx(i) = i+1;

//    std::cout << "outIdx: " << outIdx.rows() << " x " << outIdx.cols() << ",  cnt: " << cnt << endl;
            return outIdx;
}

//******************************************************************************************************************
//******************************************************************************************************************
//******************************************************************************************************************

//  Read FIFF coil configuration and assign them to the Coil class variables
//    such as CoilName,CoilT,CoilTk, CoilNk, CoilRk, CoilWk, CoilGrad.
void RtSssAlgo::getCoilInfoBabyMEG4Sim()
{
    int NumCoilIn = 270;
    int NumCoilOut = 105;

    NumCoil = NumCoilIn + NumCoilOut;
    cout << "Num of Coil = " << NumCoil << endl;

//    simulation data from test1_sim.mat
    Origin.resize(3);
    Origin << 0.0, 0.0, 0.04;

    std::cout << "loading MEGIn ....";
    ifstream inMEGIn("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/BabyMEG/MEGIn2.txt",ios::in);
    MEGIn.resize(NumCoil);
    for (int k=0; k<NumCoil; k++)   inMEGIn >> MEGIn(k);
    inMEGIn.close();
//    std::cout << MEGIn << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGOut ....";
    ifstream inMEGOut("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/BabyMEG/MEGOut2.txt",ios::in);
    MEGOut.resize(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGOut >> MEGOut(k);
    inMEGOut.close();
//    std::cout << MEGOut << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGNoise ....";
    ifstream inMEGNoise("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/BabyMEG/MEGNoise.txt",ios::in);
    MEGNoise.resize(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGNoise >> MEGNoise(k);
    inMEGNoise.close();
//    std::cout << MEGNoise << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGData ....";
    ifstream inMEGData("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/BabyMEG/MEGData2.txt",ios::in);
    MEGData.resize(NumCoil,1);
    VectorXd tmpvec(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGData >> tmpvec(k);
    MEGData.col(0) = tmpvec;
    inMEGData.close();
//    std::cout << MEGData << endl;
    std::cout << " finished !" << endl;

    // name per coil
    for (int i=0; i<NumCoilIn; i++)
        CoilName.append("BSQ2_In_Layer");
    for (int i=0; i<NumCoilOut; i++)
        CoilName.append("BSQ2_Out_Layer");

//    std:cout << CoilName[0] << ",  " << CoilName[1] << ",  " << CoilName[2] << endl;

    // transformation matrix per coil
    std::cout << "loading coil transformation matrix ....";
    ifstream inCT("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_coil_def/BabyMEG/CoilT.txt",ios::in);
    MatrixXd tmat(4,4);
    for(int k=0; k<NumCoil; k++)
    {
        for(int i=0; i<4; i++)
            for(int j=0; j<4; j++)
            {
                inCT >> tmat(i,j);
            }
        CoilT.append(tmat);
    }
    inCT.close();
//    std::cout << CoilT[305] << endl;
    std::cout << " finished !" << endl;


    // coil type - name
    CoilTk.append("BSQ2_In_Layer");
    CoilTk.append("BSQ2_Out_Layer");
//    std::cout << CoilTk[0] << ",  " << CoilTk[1] << endl;


    // coil type - Number of integration points for each coil type
    CoilNk.resize(2);
    CoilNk << 7, 7;

    // coil type - Coordinate of integrating points
    MatrixXd CintptsInlayer(3,7), CintptsOutlayer(3,7);
    CintptsInlayer << 0.000000, 0.000000, 0.000000, 0.0040825, 0.000000, 0.000000, -0.0040825, 0.000000, 0.000000,  0.0020412, 0.0035356, 0.000000, 0.0020412, -0.0035356, 0.000000, -0.0020412, 0.0035356, 0.000000, -0.0020412, -0.0035356, 0.000000;
    CintptsOutlayer << 0.000000, 0.000000, 0.000000, 0.0081650, 0.000000, 0.000000, -0.0081650, 0.000000, 0.000000,  0.0040824, 0.0070712, 0.000000, 0.0040824, -0.0070712, 0.000000, -0.0040824, 0.0070712, 0.000000, -0.0040824, -0.0070712, 0.000000;
    CoilRk.append(CintptsInlayer);
    CoilRk.append(CintptsOutlayer);

    // Coil type - Coil point weight
    MatrixXd CWintptsInlayer(1,7), CWintptsOutlayer(1,7);
    CWintptsInlayer << 0.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125;
    CWintptsOutlayer = CWintptsInlayer;
    CoilWk.append(CWintptsInlayer);
    CoilWk.append(CWintptsOutlayer);

    // BabyMEG consists of magnetometers only
//    CoilGrad.resize(NumCoil);
    CoilGrad.setZero(NumCoil);
//    std::cout << CoilGrad << endl;
}

void RtSssAlgo::getCoilInfoVectorView()
{
    Origin.resize(3);
    Origin << 0.0, 0.0, 0.04;

    // Load Fiff info
    QFile t_fileRaw("/autofs/cluster/fusion/slew/GitHub/mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    FiffRawData raw(t_fileRaw);

    cout << "number of chs: " << raw.info.nchan << endl;

    // coil type - name
    CoilTk.append(QString::number(3012));
    CoilTk.append(QString::number(3024));
    std::cout << CoilTk[0].toStdString() << "  " << CoilTk[1].toStdString() << endl;

    CoilGrad.resize(306);
    int nmegcoil=0;
    for (int i=0; i<raw.info.nchan; i++ )
    {
        // kind = 1(MEG), 2(EEG), 3(STI...)
        // coil_type = 3012(planar gradiometer),  3024(magnetometer)
        if ((raw.info.chs[i].kind == 1 ) && ((raw.info.chs[i].chpos.coil_type == 3012) || (raw.info.chs[i].chpos.coil_type == 3024)))
        {
            CoilName.append(QString::number(raw.info.chs[i].chpos.coil_type));
//            std::cout << "coil no[" << nmegcoil <<"] "<< CoilName[nmegcoil].toStdString() << "   ";

            CoilT.append(raw.info.chs[i].coil_trans.cast<double>());

            if (raw.info.chs[i].chpos.coil_type == 3012)  CoilGrad(nmegcoil) = 1;
            else    CoilGrad(nmegcoil) = 0;
//            std::cout << CoilGrad(nmegcoil) << "   ,";

            nmegcoil++;
        }
    }
    cout << endl << "meg coil cnt: " << nmegcoil << endl;
    NumCoil = nmegcoil;

    // Number of integration points for each coil type:  8 pts(3012),  9 pts(3024)
    CoilNk.resize(2);
    CoilNk << 8, 9;

    // Coordinate of coil integrating points:  8 pts(3012),  9 pts(3024)
    MatrixXd Cintpts8(3,8), Cintpts9(3,9);
    Cintpts8 << 0.0059, -0.0059, 0.0059, -0.0059, 0.0108, -0.0108, 0.0108, -0.0108, 0.0067, 0.0067, -0.0067, -0.0067, 0.0067, 0.0067, -0.0067, -0.0067, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;
    Cintpts9 << 0.0000, 0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0100, -0.0100, 0.0000, 0.0100, 0.0100, -0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;
    CoilRk.append(Cintpts8);
    CoilRk.append(Cintpts9);

    // Coil point weight: 8 pts(3012),  9 pts(3024)
    MatrixXd CWintpts8(1,8), CWintpts9(1,9);
    CWintpts8 << 14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790;
    CWintpts9 << 0.1975,    0.0772,    0.0772,    0.0772,    0.0772,    0.1235,    0.1235,    0.1235,    0.1235;
    CoilWk.append(CWintpts8);
    CoilWk.append(CWintpts9);

    std::cout << "loading MEGData ....";
    ifstream inMEGData("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGData.txt",ios::in);
    MEGData.resize(NumCoil,1);
    VectorXd tmpvec(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGData >> tmpvec(k);
    MEGData.col(0) = tmpvec;
    inMEGData.close();
//    std::cout << MEGData << endl;
    std::cout << " finished !" << endl;
}

//  Read FIFF coil configuration and assign them to the Coil class variables
//    such as CoilName,CoilT,CoilTk, CoilNk, CoilRk, CoilWk, CoilGrad.
void RtSssAlgo::getCoilInfoVectorView4Sim()
{
    QString str;

    NumCoil = 306;
    cout << "Num of Coil = " << NumCoil << endl;

//    simulation data from test1_sim.mat
    Origin.resize(3);
    Origin << 0.0, 0.0, 0.04;

    std::cout << "loading MEGIn ....";
    ifstream inMEGIn("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGIn.txt",ios::in);
    MEGIn.resize(NumCoil);
    for (int k=0; k<NumCoil; k++)   inMEGIn >> MEGIn(k);
    inMEGIn.close();
//    std::cout << MEGIn << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGOut ....";
    ifstream inMEGOut("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGOut.txt",ios::in);
    MEGOut.resize(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGOut >> MEGOut(k);
    inMEGOut.close();
//    std::cout << MEGOut << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGNoise ....";
    ifstream inMEGNoise("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGNoise.txt",ios::in);
    MEGNoise.resize(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGNoise >> MEGNoise(k);
    inMEGNoise.close();
//    std::cout << MEGNoise << endl;
    std::cout << " finished !" << endl;

    std::cout << "loading MEGData ....";
    ifstream inMEGData("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_test_sim/VectorView/MEGData.txt",ios::in);
    MEGData.resize(NumCoil,1);
    VectorXd tmpvec(NumCoil);
    for(int k=0; k<NumCoil; k++)    inMEGData >> tmpvec(k);
    MEGData.col(0) = tmpvec;
    inMEGData.close();
//    std::cout << MEGData << endl;
    std::cout << " finished !" << endl;

    // name per coil
    for (int i=0; i<NumCoil; i++)
    {
        if (i%3 == 0)
            CoilName.append("VV_PLANAR_T1");
        else if (i%3 ==1)
            CoilName.append("VV_PLANAR_T1");
        else
            CoilName.append("VV_MAG_T1");
    }
//    std:cout << CoilName[0] << ",  " << CoilName[1] << ",  " << CoilName[2] << endl;

    // transformation matrix per coil
    std::cout << "loading coil transformation matrix ....";
    ifstream inCT("/autofs/cluster/fusion/slew/MEG_realtime/mne-matlab-master/matlab/DATA_coil_def/VectorView/CoilT.txt",ios::in);
    MatrixXd tmat(4,4);
    for(int k=0; k<NumCoil; k++)
    {
        for(int i=0; i<4; i++)
            for(int j=0; j<4; j++)
            {
                inCT >> tmat(i,j);
            }
        CoilT.append(tmat);
    }
    inCT.close();
//    std::cout << CoilT[305] << endl;
    std::cout << " finished !" << endl;

    // coil type - name
    CoilTk.append("VV_PLANAR_T1");
    CoilTk.append("VV_MAG_T1");
//    std::cout << CoilTk[0] << ",  " << CoilTk[1] << endl;


    // coil type - Number of integration points for each coil type
    CoilNk.resize(2);
    CoilNk << 8, 9;

    // coil type - Coordinate of integrating points
    MatrixXd Cintpts8(3,8), Cintpts9(3,9);
    Cintpts8 << 0.0059, -0.0059, 0.0059, -0.0059, 0.0108, -0.0108, 0.0108, -0.0108, 0.0067, 0.0067, -0.0067, -0.0067, 0.0067, 0.0067, -0.0067, -0.0067, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;
    Cintpts9 << 0.0000, 0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0100, -0.0100, 0.0000, 0.0100, 0.0100, -0.0100, -0.0100, 0.0100, -0.0100, 0.0000, 0.0000, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003, 0.0003;
    CoilRk.append(Cintpts8);
    CoilRk.append(Cintpts9);

    // Coil type - Coil point weight
    MatrixXd CWintpts8(1,8), CWintpts9(1,9);
    CWintpts8 << 14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790,   14.9790,  -14.9790;
    CWintpts9 << 0.1975,    0.0772,    0.0772,    0.0772,    0.0772,    0.1235,    0.1235,    0.1235,    0.1235;
    CoilWk.append(CWintpts8);
    CoilWk.append(CWintpts9);

    // Gradiometer ?
    CoilGrad.resize(NumCoil);
    CoilGrad << 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0;
//    std::cout << CoilGrad << endl;
}

