//=============================================================================================================
/**
 * @file     sphara.cpp
 * @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Robert Dicamillo, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the Sphara class
 *
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

using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL RTPROCESSINGLIB METHODS
//=============================================================================================================

MatrixXd RTPROCESSINGLIB::makeSpharaProjector(const MatrixXd& matBaseFct,
                                     const VectorXi& vecIndices,
                                     int iOperatorDim,
                                     int iNBaseFct,
                                     int iSkip)
{
    MatrixXd matSpharaOperator = MatrixXd::Identity(iOperatorDim, iOperatorDim);

    if(matBaseFct.size() == 0) {
        qWarning()<<"[RTPROCESSINGLIB::makeSpharaProjector] Basis function matrix was empty. Returning identity matrix instead.";
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
                    qWarning()<<"RTPROCESSINGLIB::makeSpharaProjector - Index is out of range. Returning identity matrix.";
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

