//=============================================================================================================
/**
 * @file     firfilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Implementation of FirFilter.
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
