//=============================================================================================================
/**
 * @file     mvar_model.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    MvarModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mvar_model.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtMath>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MvarModel::fit(const MatrixXd& data, int p)
{
    m_nChannels = static_cast<int>(data.rows());

    if(p <= 0) {
        p = selectOrderBIC(data);
    }

    fitLevinsonDurbin(data, p);
}

//=============================================================================================================

QVector<MatrixXd> MvarModel::coefficients() const
{
    return m_coeffs;
}

//=============================================================================================================

MatrixXd MvarModel::noiseCov() const
{
    return m_noiseCov;
}

//=============================================================================================================

int MvarModel::order() const
{
    return m_order;
}

//=============================================================================================================

QVector<MatrixXcd> MvarModel::transferFunction(const VectorXd& freqs) const
{
    QVector<MatrixXcd> vecH;
    vecH.reserve(static_cast<int>(freqs.size()));

    const int nCh = m_nChannels;
    const MatrixXcd matI = MatrixXcd::Identity(nCh, nCh);
    const std::complex<double> j(0.0, 1.0);

    for(int fi = 0; fi < freqs.size(); ++fi) {
        MatrixXcd matA = matI;

        for(int k = 0; k < m_order; ++k) {
            const double phase = -2.0 * M_PI * freqs(fi) * (k + 1);
            const std::complex<double> expVal = std::exp(j * phase);
            matA -= m_coeffs[k].cast<std::complex<double>>() * expVal;
        }

        vecH.append(matA.inverse());
    }

    return vecH;
}

//=============================================================================================================

QVector<MatrixXcd> MvarModel::spectralMatrix(const VectorXd& freqs) const
{
    QVector<MatrixXcd> vecH = transferFunction(freqs);
    QVector<MatrixXcd> vecS;
    vecS.reserve(vecH.size());

    const MatrixXcd matSigma = m_noiseCov.cast<std::complex<double>>();

    for(int fi = 0; fi < vecH.size(); ++fi) {
        vecS.append(vecH[fi] * matSigma * vecH[fi].adjoint());
    }

    return vecS;
}

//=============================================================================================================

void MvarModel::fitLevinsonDurbin(const MatrixXd& data, int p)
{
    m_order = p;
    m_nChannels = static_cast<int>(data.rows());

    const int nCh = m_nChannels;
    const int nSamples = static_cast<int>(data.cols());
    const int nObs = nSamples - p;

    if(nObs <= 0) {
        qWarning() << "MvarModel::fitLevinsonDurbin - Not enough samples for model order" << p;
        m_coeffs.clear();
        m_noiseCov = MatrixXd::Identity(nCh, nCh);
        return;
    }

    // Subtract mean from each channel
    MatrixXd dataCentered = data;
    for(int i = 0; i < nCh; ++i) {
        dataCentered.row(i).array() -= dataCentered.row(i).mean();
    }

    // Build regression matrices for OLS solution of Yule-Walker equations
    // Y = data(:, p:end)              -> nCh x nObs
    // Z = [data(:, p-1:end-1);        -> nCh*p x nObs
    //      data(:, p-2:end-2);
    //      ...
    //      data(:, 0:end-p)]
    MatrixXd matY = dataCentered.rightCols(nObs);
    MatrixXd matZ(nCh * p, nObs);

    for(int k = 0; k < p; ++k) {
        matZ.middleRows(k * nCh, nCh) = dataCentered.middleCols(p - 1 - k, nObs);
    }

    // Solve Y = A * Z via least squares: A = Y * Z^T * (Z * Z^T)^{-1}
    MatrixXd matZZT = matZ * matZ.transpose();
    MatrixXd matYZT = matY * matZ.transpose();
    MatrixXd matA = matYZT * matZZT.ldlt().solve(MatrixXd::Identity(nCh * p, nCh * p));

    // Extract coefficient matrices A_1..A_p
    m_coeffs.clear();
    m_coeffs.reserve(p);
    for(int k = 0; k < p; ++k) {
        m_coeffs.append(matA.middleCols(k * nCh, nCh));
    }

    // Compute noise covariance from residuals
    MatrixXd matE = matY - matA * matZ;
    m_noiseCov = (matE * matE.transpose()) / static_cast<double>(nObs);
}

//=============================================================================================================

int MvarModel::selectOrderBIC(const MatrixXd& data, int maxOrder) const
{
    const int nCh = static_cast<int>(data.rows());
    const int nSamples = static_cast<int>(data.cols());

    // Limit max order to avoid underdetermined systems
    maxOrder = qMin(maxOrder, nSamples / (nCh + 1));
    if(maxOrder < 1) {
        maxOrder = 1;
    }

    // Subtract mean
    MatrixXd dataCentered = data;
    for(int i = 0; i < nCh; ++i) {
        dataCentered.row(i).array() -= dataCentered.row(i).mean();
    }

    double bestBIC = std::numeric_limits<double>::max();
    int bestOrder = 1;

    for(int p = 1; p <= maxOrder; ++p) {
        const int nObs = nSamples - p;
        if(nObs <= nCh * p) {
            break;
        }

        // Build regression matrices
        MatrixXd matY = dataCentered.rightCols(nObs);
        MatrixXd matZ(nCh * p, nObs);

        for(int k = 0; k < p; ++k) {
            matZ.middleRows(k * nCh, nCh) = dataCentered.middleCols(p - 1 - k, nObs);
        }

        // Solve via OLS
        MatrixXd matZZT = matZ * matZ.transpose();
        MatrixXd matYZT = matY * matZ.transpose();
        MatrixXd matA = matYZT * matZZT.ldlt().solve(MatrixXd::Identity(nCh * p, nCh * p));

        // Residual covariance
        MatrixXd matE = matY - matA * matZ;
        MatrixXd matSigma = (matE * matE.transpose()) / static_cast<double>(nObs);

        // BIC = n * ln(det(Sigma)) + k * ln(n), where k = p * nCh^2
        double detSigma = matSigma.determinant();
        if(detSigma <= 0.0) {
            continue;
        }

        const int nParams = p * nCh * nCh;
        double bic = nObs * std::log(detSigma) + nParams * std::log(static_cast<double>(nObs));

        if(bic < bestBIC) {
            bestBIC = bic;
            bestOrder = p;
        }
    }

    return bestOrder;
}
