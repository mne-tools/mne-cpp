//=============================================================================================================
/**
 * @file     ml_scaler.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MlScaler class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_scaler.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlScaler::MlScaler(ScalerType type)
: m_type(type)
{
}

//=============================================================================================================

void MlScaler::fit(const MlTensor& data)
{
    auto X = data.matrix();

    if (m_type == StandardScaler) {
        m_mean = X.colwise().mean();
        VectorXf variance = ((X.rowwise() - m_mean.transpose()).array().square().colwise().sum() /
                             static_cast<float>(X.rows())).matrix().transpose();
        m_std = variance.array().sqrt().max(1e-10f).matrix();
    } else {
        m_min   = X.colwise().minCoeff();
        VectorXf maxVals = X.colwise().maxCoeff();
        m_range = (maxVals - m_min).array().max(1e-10f).matrix();
    }

    m_fitted = true;
}

//=============================================================================================================

MlTensor MlScaler::transform(const MlTensor& data) const
{
    if (!m_fitted) {
        throw std::runtime_error("MlScaler::transform – scaler has not been fitted.");
    }

    auto X = data.matrix();

    if (m_type == StandardScaler) {
        MatrixXf result = ((X.rowwise() - m_mean.transpose()).array().rowwise() / m_std.transpose().array()).matrix();
        return MlTensor(result);
    } else {
        MatrixXf result = ((X.rowwise() - m_min.transpose()).array().rowwise() / m_range.transpose().array()).matrix();
        return MlTensor(result);
    }
}

//=============================================================================================================

MlTensor MlScaler::fitTransform(const MlTensor& data)
{
    fit(data);
    return transform(data);
}

//=============================================================================================================

MlTensor MlScaler::inverseTransform(const MlTensor& data) const
{
    if (!m_fitted) {
        throw std::runtime_error("MlScaler::inverseTransform – scaler has not been fitted.");
    }

    auto X = data.matrix();

    if (m_type == StandardScaler) {
        MatrixXf result = ((X.array().rowwise() * m_std.transpose().array()).rowwise() + m_mean.transpose().array()).matrix();
        return MlTensor(result);
    } else {
        MatrixXf result = ((X.array().rowwise() * m_range.transpose().array()).rowwise() + m_min.transpose().array()).matrix();
        return MlTensor(result);
    }
}
