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
* @brief   Warp class definition.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "warp.h"
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/LU>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>


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
    MatrixXd warpWeight, polWeight;
    calcWeighting(sLm, dLm, warpWeight, polWeight);
    MatrixXd wVert = warpVertices(sVert, sLm, warpWeight, polWeight);
    return wVert;
}

//*************************************************************************************************************

MNELIB::MNEBem Warp::calculate(const MatrixXd & sLm, const MatrixXd &dLm, const MNELIB::MNEBem &p_MNEBem)
{
    MatrixXd warpWeight, polWeight;
    calcWeighting(sLm, dLm, warpWeight, polWeight);

    MNELIB::MNEBemSurface head;
    MNELIB::MNEBemSurface outer_skull;
    MNELIB::MNEBemSurface inner_skull;
    head = p_MNEBem[0];
    outer_skull = p_MNEBem[1];
    inner_skull = p_MNEBem[2];
    MatrixXd wVert = warpVertices(sVert, sLm, warpWeight, polWeight);
    return wVert;
}

//*************************************************************************************************************

bool Warp::calcWeighting(const MatrixXd &sLm, const MatrixXd &dLm, MatrixXd& warpWeight, MatrixXd& polWeight)
{
    MatrixXd K = MatrixXd::Zero(sLm.rows(),sLm.rows());     //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sLm.rows(); i++)
        K.col(i)=((sLm.rowwise()-sLm.row(i)).rowwise().norm());

//    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    MatrixXd P (sLm.rows(),4);                              //P=[ones,sLm]
    P << MatrixXd::Ones(sLm.rows(),1),sLm;
//    std::cout << "Here is the matrix P:" << std::endl << P << std::endl;

    MatrixXd L ((sLm.rows()+4),(sLm.rows()+4));             //L=Full Matrix of the linear eq.
    L <<    K,P,
            P.transpose(),MatrixXd::Zero(4,4);
//    std::cout << "Here is the matrix L:" << std::endl << L << std::endl;

    MatrixXd Y ((dLm.rows()+4),3);                          //Y=[dLm,Zero]
    Y <<    dLm,
            MatrixXd::Zero(4,3);
//    std::cout << "Here is the matrix Y:" << std::endl << Y << std::endl;

    //
    // calculate the weighting matrix (Y=L*W)
    //
    MatrixXd W ((dLm.rows()+4),3);                          //W=[warpWeight,polWeight]
    Eigen::FullPivLU <MatrixXd> Lu(L);                      //LU decomposition is one method to solve lin. eq.
    W=Lu.solve(Y);
//    std::cout << "Here is the matrix W:" << std::endl << W << std::endl;

    warpWeight = W.topRows(sLm.rows());
    polWeight = W.bottomRows(4);

    return true;
}


//*************************************************************************************************************

MatrixXd Warp::warpVertices(const MatrixXd &sVert, const MatrixXd & sLm, const MatrixXd& warpWeight, const MatrixXd& polWeight)
{
    MatrixXd wVert = sVert * polWeight.bottomRows(3);         //Pol. Warp
    wVert.rowwise() += polWeight.row(0);                      //Translation

    //
    // TPS Warp
    //
    MatrixXd K = MatrixXd::Zero(sVert.rows(),sLm.rows());     //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sVert.rows(); i++)
        K.row(i)=((sLm.rowwise()-sVert.row(i)).rowwise().norm().transpose());
//    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    wVert += K*warpWeight;
//    std::cout << "Here is the matrix wVert:" << std::endl << wVert << std::endl;
    return wVert;
}

//*************************************************************************************************************

MatrixXd Warp::readsLm(const QString &electrodeFileName)
{
    MatrixXd electrodes;
    QFile file(electrodeFileName);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening file";
//        return false;
    }

    //Start reading from file
    double numberElectrodes;
    QTextStream in(&file);
    int i=0;

    while(!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(QRegExp("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        //Read number of electrodes
        if(i == 0){
            numberElectrodes = fields.at(fields.size()-1).toDouble();
            electrodes = MatrixXd::Zero(numberElectrodes, 3);
        }
        //Read actual electrode positions
        else{
            Vector3d x;
            x << fields.at(fields.size()-3).toDouble(),fields.at(fields.size()-2).toDouble(),fields.at(fields.size()-1).toDouble();
            electrodes.row(i-1)=x.transpose();
        }
        i++;
    }
    return electrodes;
}
