//=============================================================================================================
/**
* @file     warp.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Jana Kiesel. All rights reserved.
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
* @brief    Implementation of a thin plate spline warping class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "warp.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd Warp::calculate(const MatrixXd &sLm, const MatrixXd &dLm, const MatrixXd &sVert)
{
     MatrixXd warpWeight = MatrixXd::Zero(sLm.rows(),3);
     MatrixXd polWeight = MatrixXd::Zero(4,3);

     std::cout << "Here is the matrix sLm:" << std::endl << sLm << std::endl;

     calcWeighting(sLm, dLm, warpWeight, polWeight);

     MatrixXd wVert = warpVertices(sVert);
     return wVert;
}


//*************************************************************************************************************

MatrixXd Warp::calculate(const MatrixXd &sVert)
{
    MatrixXd wVert = MatrixXd::Zero(3,sVert.rows());
    return wVert;
}


//*************************************************************************************************************

bool Warp::calcWeighting(const MatrixXd &sLm, const MatrixXd &dLm, MatrixXd &warpWeight, MatrixXd &polWeight)
{
    MatrixXd K = MatrixXd::Zero(sLm.rows(),sLm.rows());             //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sLm.rows(); i++)
        K.col(i)=((sLm.rowwise()-sLm.row(i)).rowwise().norm());

    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    MatrixXd P (sLm.rows(),4);                                      //P=[ones,sLm]
    P << MatrixXd::Ones(sLm.rows(),1),sLm;
    std::cout << "Here is the matrix P:" << std::endl << P << std::endl;

    MatrixXd L ((sLm.rows()+4),(sLm.rows()+4));
    L <<    K,P,
            P.transpose(),MatrixXd::Zero(4,4);
    std::cout << "Here is the matrix L:" << std::endl << L << std::endl;

    MatrixXd Y ((dLm.rows()+4),3);                                  //Y=[dLm,Zero]
    Y <<    dLm,
            MatrixXd::Zero(4,3);
    std::cout << "Here is the matrix Y:" << std::endl << Y << std::endl;

    // calculate the weighting matrix
    MatrixXd W ((dLm.rows()+4),3);                                  //W=[warpWeight,polWeight]
    Eigen::FullPivLU <MatrixXd> Lu(L);
    W=Lu.solve(Y);
    std::cout << "Here is the matrix W:" << std::endl << W << std::endl;

    warpWeight = W.topRows(sLm.rows());
    polWeight = W.bottomRows(4);
    this->warpWeight = W.topRows(sLm.rows());
    this->polWeight = W.bottomRows(4);
    this->sLm=sLm;

    return true;
}


//*************************************************************************************************************

MatrixXd Warp::warpVertices(const MatrixXd &sVert)
{
    MatrixXd wVert = sVert * this->polWeight.bottomRows(3);
    std::cout << "Here is the matrix wVert 1.Step:" << std::endl << wVert << std::endl;

    wVert.rowwise() += this->polWeight.row(0);
    std::cout << "Here is the matrix wVert 2.Step:" << std::endl << wVert << std::endl;

    MatrixXd K = MatrixXd::Zero(sVert.rows(),this->sLm.rows());             //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sVert.rows(); i++)
        K.row(i)=((this->sLm.rowwise()-sVert.row(i)).rowwise().norm().transpose());
    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    wVert += K*this->warpWeight;
    std::cout << "Here is the matrix wVert 3.Step:" << std::endl << wVert << std::endl;
    return wVert;
}
