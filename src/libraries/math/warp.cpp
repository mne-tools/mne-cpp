//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file warp.cpp
 * @since 2026
 * @date  March 2026
 * @brief Thin-plate-spline fit and evaluation kernels and the landmark-file reader.
 *
 * Implements the TPS solver declared in @ref warp.h. Fitting assembles
 * the @c (n+4)×(n+4) block-symmetric matrix
 * @f$\begin{bmatrix}K & P\\P^T & 0\end{bmatrix}@f$ from the pairwise
 * kernel evaluations @f$U(r)=r@f$ and the affine constraints, factorises it
 * once via @c Eigen::FullPivLU, and recovers the warp weights @c w and
 * affine part @c (A, t). Evaluation at @c m new vertices then costs
 * @c O(m*n + m) and is performed batchwise so a complete cortical mesh
 * can be warped in one pass.
 *
 * The companion @ref Warp::readsLm parses the simple @c x y z per-line
 * electrode landmark format used by the MRI-side calibration tools so
 * the class can be driven directly from a digitiser export without
 * needing a separate IO layer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "warp.h"

#include <iostream>
#include <fstream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QList>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXf Warp::calculate(const MatrixXf &sLm, const MatrixXf &dLm, const MatrixXf &sVert)
{
    MatrixXf warpWeight, polWeight;
    calcWeighting(sLm, dLm, warpWeight, polWeight);
    MatrixXf wVert = warpVertices(sVert, sLm, warpWeight, polWeight);
    return wVert;
}

//=============================================================================================================

void Warp::calculate(const MatrixXf & sLm, const MatrixXf &dLm, QList<MatrixXf> & vertList)
{
    MatrixXf warpWeight, polWeight;
    calcWeighting(sLm, dLm, warpWeight, polWeight);

    for (int i=0; i<vertList.size(); i++)
    {
        vertList.replace(i,warpVertices(vertList.at(i), sLm, warpWeight, polWeight));
    }
    return;
}

//=============================================================================================================

bool Warp::calcWeighting(const MatrixXf &sLm, const MatrixXf &dLm, MatrixXf& warpWeight, MatrixXf& polWeight)
{
    MatrixXf K = MatrixXf::Zero(sLm.rows(),sLm.rows());     //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sLm.rows(); i++)
        K.col(i)=((sLm.rowwise()-sLm.row(i)).rowwise().norm());

//    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    MatrixXf P (sLm.rows(),4);                              //P=[ones,sLm]
    P << MatrixXf::Ones(sLm.rows(),1),sLm;
//    std::cout << "Here is the matrix P:" << std::endl << P << std::endl;

    MatrixXf L ((sLm.rows()+4),(sLm.rows()+4));             //L=Full Matrix of the linear eq.
    L <<    K,P,
            P.transpose(),MatrixXf::Zero(4,4);
//    std::cout << "Here is the matrix L:" << std::endl << L << std::endl;

    MatrixXf Y ((dLm.rows()+4),3);                          //Y=[dLm,Zero]
    Y <<    dLm,
            MatrixXf::Zero(4,3);
//    std::cout << "Here is the matrix Y:" << std::endl << Y << std::endl;

    //
    // calculate the weighting matrix (Y=L*W)
    //
    MatrixXf W ((dLm.rows()+4),3);                          //W=[warpWeight,polWeight]
    Eigen::FullPivLU <MatrixXf> Lu(L);                      //LU decomposition is one method to solve lin. eq.
    W=Lu.solve(Y);
//    std::cout << "Here is the matrix W:" << std::endl << W << std::endl;

    warpWeight = W.topRows(sLm.rows());
    polWeight = W.bottomRows(4);

    return true;
}

//=============================================================================================================

MatrixXf Warp::warpVertices(const MatrixXf &sVert, const MatrixXf & sLm, const MatrixXf& warpWeight, const MatrixXf& polWeight)
{
    MatrixXf wVert = sVert * polWeight.bottomRows(3);         //Pol. Warp
    wVert.rowwise() += polWeight.row(0);                      //Translation

    //
    // TPS Warp
    //
    MatrixXf K = MatrixXf::Zero(sVert.rows(),sLm.rows());     //K(i,j)=||sLm(i)-sLm(j)||
    for (int i=0; i<sVert.rows(); i++)
        K.row(i)=((sLm.rowwise()-sVert.row(i)).rowwise().norm().transpose());
//    std::cout << "Here is the matrix K:" << std::endl << K << std::endl;

    wVert += K*warpWeight;
//    std::cout << "Here is the matrix wVert:" << std::endl << wVert << std::endl;
    return wVert;
}

//=============================================================================================================

MatrixXf Warp::readsLm(const QString &electrodeFileName)
{
    MatrixXf electrodes;
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
        QStringList fields = line.split(QRegularExpression("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        //Read number of electrodes
        if(i == 0){
            numberElectrodes = fields.at(fields.size()-1).toDouble();
            electrodes = MatrixXf::Zero(numberElectrodes, 3);
        }
        //Read actual electrode positions
        else{
            Vector3f x;
            x << fields.at(fields.size()-3).toFloat(),fields.at(fields.size()-2).toFloat(),fields.at(fields.size()-1).toFloat();
            electrodes.row(i-1)=x.transpose();
        }
        i++;
    }
    return electrodes;
}

//=============================================================================================================

MatrixXf Warp::readsLm(const std::string &electrodeFileName)
{
    MatrixXf electrodes;
    std::ifstream inFile(electrodeFileName);

    if(!inFile.is_open()) {
        qDebug()<<"Error opening file";
        //Why are we not returning?
//        return false;
    }

    //Start reading from file
    double numberElectrodes;
    int i = 0;

    std::string line;
    while(std::getline(inFile, line)){
        std::vector<std::string> fields;
        std::stringstream stream{line};
        std::string element;

        stream >> std::ws;
        while(stream >> element){
            fields.push_back(std::move(element));
            stream >> std::ws;
        }

        //Read number of electrodes
        if(i == 0){
            numberElectrodes = std::stod(fields.at(fields.size()-1));
            electrodes = MatrixXf::Zero(numberElectrodes, 3);
        }

        //Read actual electrode positions
        else{
            Vector3f x;
            x << std::stof(fields.at(fields.size()-3)), std::stof(fields.at(fields.size()-2)), std::stof(fields.at(fields.size()-1));
            electrodes.row(i-1)=x.transpose();
        }
        i++;
    }

    return electrodes;
}
