//=============================================================================================================
/**
 * @file     stim_artifact.cpp
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
 * @brief    Implementation of fixStimArtifact.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stim_artifact.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void UTILSLIB::fixStimArtifact(MatrixXd& data,
                                const MatrixXi& events,
                                double sfreq,
                                int eventId,
                                double tmin,
                                double tmax,
                                StimArtifactMode mode)
{
    if (events.rows() == 0 || events.cols() < 3 || data.cols() == 0 || data.rows() == 0) {
        return;
    }

    const int iMinOffset = static_cast<int>(std::round(tmin * sfreq));
    const int iMaxOffset = static_cast<int>(std::round(tmax * sfreq));
    const int iNumCols = static_cast<int>(data.cols());
    const int iNumRows = static_cast<int>(data.rows());

    for (int e = 0; e < events.rows(); ++e) {
        // Filter by event ID if requested
        if (eventId >= 0 && events(e, 2) != eventId) {
            continue;
        }

        // Compute window boundaries
        int iStart = events(e, 0) + iMinOffset;
        int iEnd   = events(e, 0) + iMaxOffset;

        // Clamp to data bounds
        iStart = std::max(0, iStart);
        iEnd   = std::min(iNumCols - 1, iEnd);

        if (iEnd <= iStart) {
            continue;
        }

        const int iWindowLen = iEnd - iStart + 1;

        switch (mode) {
        case StimArtifactMode::Linear: {
            for (int ch = 0; ch < iNumRows; ++ch) {
                const double dStartVal = data(ch, iStart);
                const double dEndVal   = data(ch, iEnd);
                for (int s = 0; s < iWindowLen; ++s) {
                    const double dAlpha = static_cast<double>(s) / static_cast<double>(iWindowLen - 1);
                    data(ch, iStart + s) = dStartVal + dAlpha * (dEndVal - dStartVal);
                }
            }
            break;
        }
        case StimArtifactMode::Window: {
            data.block(0, iStart, iNumRows, iWindowLen).setZero();
            break;
        }
        }
    }
}
