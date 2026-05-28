//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sphara.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Definition of the Sphara class
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sphara.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL UTILSLIB METHODS
//=============================================================================================================

MatrixXd UTILSLIB::makeSpharaProjector(const MatrixXd& matBaseFct,
                                     const VectorXi& vecIndices,
                                     int iOperatorDim,
                                     int iNBaseFct,
                                     int iSkip)
{
    MatrixXd matSpharaOperator = MatrixXd::Identity(iOperatorDim, iOperatorDim);

    if(matBaseFct.size() == 0) {
        qWarning()<<"[UTILSLIB::makeSpharaProjector] Basis function matrix was empty. Returning identity matrix instead.";
        return matSpharaOperator;
    }

    //Remove unwanted base functions
    MatrixXd matSpharaGradCut = matBaseFct.block(0,0,matBaseFct.rows(),iNBaseFct);
    MatrixXd matSpharaMultGrad = matSpharaGradCut * matSpharaGradCut.transpose().eval();

    //Create the SPHARA operator
    int rowIndex = 0;
    int colIndex = 0;

    for(int i = 0; i<=iSkip; i++) {
        for(int r = i; r<vecIndices.rows(); r+=1+iSkip) {
            for(int c = i; c<vecIndices.rows(); c+=1+iSkip) {
                if((r < vecIndices.rows() || c < vecIndices.rows()) && (rowIndex < matSpharaMultGrad.rows() || colIndex < matSpharaMultGrad.cols())) {
                    matSpharaOperator(vecIndices(r),vecIndices(c)) = matSpharaMultGrad(rowIndex,colIndex);
                } else {
                    qWarning()<<"UTILSLIB::makeSpharaProjector - Index is out of range. Returning identity matrix.";
                    //matSpharaOperator.setZero();
                    matSpharaOperator = MatrixXd::Identity(iOperatorDim, iOperatorDim);
                    return matSpharaOperator;
                }

                ++colIndex;
            }

            colIndex = 0;
            ++rowIndex;
        }

        rowIndex = 0;
    }

    return matSpharaOperator;
}

