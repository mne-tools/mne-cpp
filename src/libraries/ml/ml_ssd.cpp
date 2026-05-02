//=============================================================================================================
/**
 * @file     ml_ssd.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    MlSsd class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_ssd.h"

//=============================================================================================================
// SKIGEN INCLUDES
//=============================================================================================================

#include <Skigen/Decomposition>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlSsd::MlSsd(int nComponents)
: m_nComponents(nComponents)
{
}

//=============================================================================================================

void MlSsd::fit(const MatrixXd& matData,
                double dSFreq,
                const QPair<double, double>& signalBand,
                const QPair<double, double>& noiseBand,
                double dRegParam)
{
    const int nCh = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());

    if (nCh < 2 || nTimes < 2) {
        qWarning() << "[MlSsd::fit] Insufficient data dimensions.";
        return;
    }

    if (m_nComponents > nCh)
        m_nComponents = nCh;

    Skigen::SSD<double> ssd(m_nComponents);
    ssd.fit(matData, dSFreq,
            signalBand.first, signalBand.second,
            noiseBand.first, noiseBand.second,
            dRegParam);

    m_filters     = ssd.filters();
    m_patterns    = ssd.patterns();
    m_eigenvalues = ssd.eigenvalues();
    m_bFitted     = true;
}

//=============================================================================================================

MatrixXd MlSsd::transform(const MatrixXd& matData) const
{
    if (!m_bFitted) {
        qWarning() << "[MlSsd::transform] SSD not fitted.";
        return MatrixXd();
    }

    return m_filters * matData;
}

//=============================================================================================================

MatrixXd MlSsd::fitTransform(const MatrixXd& matData,
                               double dSFreq,
                               const QPair<double, double>& signalBand,
                               const QPair<double, double>& noiseBand,
                               double dRegParam)
{
    fit(matData, dSFreq, signalBand, noiseBand, dRegParam);
    return transform(matData);
}
