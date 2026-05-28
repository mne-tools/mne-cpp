//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file eog_regression.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of EogRegression.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eog_regression.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Cholesky>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void EogRegression::fit(const MatrixXd& data,
                        const FiffInfo& info,
                        const QStringList& eogChannels)
{
    //
    // Determine EOG channel indices
    //
    m_vecEogIndices.clear();
    m_vecTargetIndices.clear();
    m_bFitted = false;

    if (eogChannels.isEmpty()) {
        // Auto-detect EOG channels from info
        for (int i = 0; i < info.chs.size(); ++i) {
            if (info.chs[i].kind == FIFFV_EOG_CH) {
                m_vecEogIndices.append(i);
            }
        }
    } else {
        // Use explicitly specified channel names
        for (const QString& name : eogChannels) {
            int idx = info.ch_names.indexOf(name);
            if (idx >= 0) {
                m_vecEogIndices.append(idx);
            } else {
                qWarning() << "[EogRegression::fit] EOG channel not found:" << name;
            }
        }
    }

    if (m_vecEogIndices.isEmpty()) {
        qWarning() << "[EogRegression::fit] No EOG channels found. Data will not be modified.";
        return;
    }

    //
    // Build target index list: all channels NOT in the EOG set
    //
    for (int i = 0; i < info.chs.size(); ++i) {
        if (!m_vecEogIndices.contains(i)) {
            m_vecTargetIndices.append(i);
        }
    }

    const int nEog = m_vecEogIndices.size();
    const int nTargets = m_vecTargetIndices.size();
    const int nTimes = static_cast<int>(data.cols());

    //
    // Extract EOG submatrix E (n_eog x n_times)
    //
    MatrixXd E(nEog, nTimes);
    for (int i = 0; i < nEog; ++i) {
        E.row(i) = data.row(m_vecEogIndices[i]);
    }

    //
    // Extract target submatrix T (n_targets x n_times)
    //
    MatrixXd T(nTargets, nTimes);
    for (int i = 0; i < nTargets; ++i) {
        T.row(i) = data.row(m_vecTargetIndices[i]);
    }

    //
    // Compute beta via least squares:
    //   beta = T * E^T * (E * E^T)^{-1}
    //
    // More numerically stable: solve (E * E^T) * X = E * T^T, then beta = X^T
    //
    MatrixXd EET = E * E.transpose();   // n_eog x n_eog
    MatrixXd ETT = E * T.transpose();   // n_eog x n_targets

    m_matBeta = EET.ldlt().solve(ETT).transpose();  // n_targets x n_eog

    m_bFitted = true;
}

//=============================================================================================================

void EogRegression::apply(MatrixXd& data,
                          const FiffInfo& info) const
{
    Q_UNUSED(info)

    if (!m_bFitted) {
        qWarning() << "[EogRegression::apply] Not fitted yet. Call fit() first.";
        return;
    }

    const int nEog = m_vecEogIndices.size();
    const int nTargets = m_vecTargetIndices.size();
    const int nTimes = static_cast<int>(data.cols());

    //
    // Extract EOG submatrix E (n_eog x n_times)
    //
    MatrixXd E(nEog, nTimes);
    for (int i = 0; i < nEog; ++i) {
        E.row(i) = data.row(m_vecEogIndices[i]);
    }

    //
    // Subtract: for each target channel i, data.row(targetIdx) -= beta.row(i) * E
    //
    for (int i = 0; i < nTargets; ++i) {
        data.row(m_vecTargetIndices[i]) -= m_matBeta.row(i) * E;
    }
}

//=============================================================================================================

void EogRegression::fitApply(MatrixXd& data,
                             const FiffInfo& info,
                             const QStringList& eogChannels)
{
    EogRegression reg;
    reg.fit(data, info, eogChannels);
    reg.apply(data, info);
}
