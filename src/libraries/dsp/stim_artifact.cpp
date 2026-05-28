//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file stim_artifact.cpp
 * @since May 2026
 * @brief Implementation of fixStimArtifact.
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
