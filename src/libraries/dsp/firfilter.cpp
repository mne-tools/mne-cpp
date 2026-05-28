//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file firfilter.cpp
 * @since March 2026
 * @brief Implementation of FirFilter.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "firfilter.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

FilterKernel FirFilter::design(int          iOrder,
                                FilterType   type,
                                double       dCutoffLow,
                                double       dCutoffHigh,
                                double       dSFreq,
                                double       dTransition,
                                DesignMethod method)
{
    // FilterKernel frequency encoding (all values normalised to Nyquist = sFreq/2):
    //
    //   LPF  : dCenterfreq = dCutoffLow  / nyquist,  dBandwidth = 0
    //   HPF  : dCenterfreq = dCutoffLow  / nyquist,  dBandwidth = 0
    //   BPF  : dCenterfreq = (lo+hi)     / sFreq,    dBandwidth = (hi-lo) / nyquist
    //   NOTCH: dCenterfreq = (lo+hi)     / sFreq,    dBandwidth = (hi-lo) / nyquist
    //
    // dParkswidth = dTransition / nyquist

    const double nyquist = dSFreq / 2.0;

    double dCenterfreq  = 0.0;
    double dBandwidth   = 0.0;
    const double dParkswidth = dTransition / nyquist;

    int iFilterType = static_cast<int>(type);  // LPF=0, HPF=1, BPF=2, NOTCH=3

    switch (type) {
    case LowPass:
    case HighPass:
        dCenterfreq = dCutoffLow / nyquist;
        dBandwidth  = 0.0;
        break;
    case BandPass:
    case BandStop:
        dCenterfreq = (dCutoffLow + dCutoffHigh) / dSFreq;   // = centre / nyquist normalised to [0,1]
        dBandwidth  = (dCutoffHigh - dCutoffLow) / nyquist;
        break;
    }

    QString sName;
    switch (type) {
    case LowPass:   sName = QStringLiteral("LP_%1Hz").arg(dCutoffLow);  break;
    case HighPass:  sName = QStringLiteral("HP_%1Hz").arg(dCutoffLow);  break;
    case BandPass:  sName = QStringLiteral("BP_%1-%2Hz").arg(dCutoffLow).arg(dCutoffHigh); break;
    case BandStop:  sName = QStringLiteral("BS_%1-%2Hz").arg(dCutoffLow).arg(dCutoffHigh); break;
    }

    return FilterKernel(sName,
                        iFilterType,
                        iOrder,
                        dCenterfreq,
                        dBandwidth,
                        dParkswidth,
                        dSFreq,
                        static_cast<int>(method));
}

//=============================================================================================================

RowVectorXd FirFilter::apply(const RowVectorXd& vecData,
                               FilterKernel&     kernel)
{
    RowVectorXd work = vecData;
    kernel.applyFftFilter(work, /*bKeepOverhead=*/false);
    return work;
}

//=============================================================================================================

RowVectorXd FirFilter::applyZeroPhase(const RowVectorXd& vecData,
                                       FilterKernel&     kernel)
{
    // Forward pass
    RowVectorXd work = vecData;
    kernel.applyFftFilter(work, false);

    // Reverse pass
    RowVectorXd rev = work.reverse();
    kernel.applyFftFilter(rev, false);

    return rev.reverse();
}

//=============================================================================================================

MatrixXd FirFilter::applyZeroPhaseMatrix(const MatrixXd&    matData,
                                          FilterKernel&      kernel,
                                          const RowVectorXi& vecPicks)
{
    MatrixXd result = matData;

    if (vecPicks.size() == 0) {
        // All rows
        for (int i = 0; i < result.rows(); ++i) {
            RowVectorXd row = result.row(i);
            result.row(i) = applyZeroPhase(row, kernel);
        }
    } else {
        for (int k = 0; k < vecPicks.size(); ++k) {
            int i = vecPicks(k);
            if (i < 0 || i >= result.rows()) continue;
            RowVectorXd row = result.row(i);
            result.row(i) = applyZeroPhase(row, kernel);
        }
    }

    return result;
}
