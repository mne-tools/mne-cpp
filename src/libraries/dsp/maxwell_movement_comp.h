//=============================================================================================================
/**
 * @file     maxwell_movement_comp.h
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
 * @brief    Maxwell movement compensation for MEG.
 *
 * Implements head movement compensation via SSS. Equivalent to
 * MNE-Python's mne.preprocessing.maxwell_filter with head_pos parameter.
 *
 * Approach: For each time window, compute the SSS basis at the actual head
 * position, project data into multipole space, then reconstruct at the
 * reference head position.
 */

#ifndef MAXWELL_MOVEMENT_COMP_DSP_H
#define MAXWELL_MOVEMENT_COMP_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
#include "sss.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief A head position entry (time, translation, rotation quaternion).
 */
struct DSPSHARED_EXPORT HeadPosEntry
{
    double  dTime = 0.0;                       /**< Time in seconds. */
    Eigen::Vector3d translation{0, 0, 0};      /**< Translation from device to head (metres). */
    Eigen::Quaterniond rotation{1, 0, 0, 0};   /**< Rotation quaternion (device → head). */
    double  dGof = 0.0;                        /**< Goodness of fit for this head position. */
};

//=============================================================================================================
/**
 * @brief Parameters for Maxwell movement compensation.
 */
struct DSPSHARED_EXPORT MaxwellMoveCompParams
{
    int     iOrderIn  = 8;                             /**< Internal SSS order. */
    int     iOrderOut = 3;                             /**< External SSS order. */
    Eigen::Vector3d origin{0.0, 0.0, 0.04};           /**< SSS expansion origin (metres). */
    int     iRefIdx = 0;                               /**< Index of reference head position in headPos list. Use -1 for mean position. */
    double  dRegIn = 1e-5;                             /**< Regularisation parameter. */
};

//=============================================================================================================
/**
 * @brief Maxwell movement compensation using SSS.
 *
 * Usage:
 * @code
 *   QList<HeadPosEntry> headPos = ...; // from cHPI fitting
 *   MaxwellMoveCompParams params;
 *   Eigen::MatrixXd compensated = MaxwellMovementComp::apply(
 *       matData, fiffInfo, headPos, params);
 * @endcode
 */
class DSPSHARED_EXPORT MaxwellMovementComp
{
public:
    //=========================================================================================================
    /**
     * @brief Apply movement compensation via SSS.
     *
     * For each time segment (defined by headPos entries), the data is:
     * 1. Projected into SSS multipole space using the actual head position.
     * 2. Reconstructed at the reference head position.
     *
     * @param[in] matData   Full sensor data (n_channels × n_samples).
     * @param[in] fiffInfo  Measurement info (channel positions, etc.).
     * @param[in] headPos   Head positions over time (from cHPI).
     * @param[in] dSFreq    Sampling frequency (Hz).
     * @param[in] params    Compensation parameters.
     *
     * @return Movement-compensated data (n_channels × n_samples).
     */
    static Eigen::MatrixXd apply(const Eigen::MatrixXd& matData,
                                  const FIFFLIB::FiffInfo& fiffInfo,
                                  const QList<HeadPosEntry>& headPos,
                                  double dSFreq,
                                  const MaxwellMoveCompParams& params = MaxwellMoveCompParams());

    //=========================================================================================================
    /**
     * @brief Read head positions from a file.
     *
     * File format: whitespace-separated columns per line:
     *   time  q1  q2  q3  tx  ty  tz  gof
     *
     * @param[in] sPath  Path to head position file.
     *
     * @return List of head position entries.
     */
    static QList<HeadPosEntry> readHeadPos(const QString& sPath);

    //=========================================================================================================
    /**
     * @brief Write head positions to a file.
     *
     * @param[in] sPath    Output file path.
     * @param[in] headPos  Head positions to write.
     *
     * @return true if successful.
     */
    static bool writeHeadPos(const QString& sPath,
                              const QList<HeadPosEntry>& headPos);

private:
    /**
     * @brief Create a modified FiffInfo with sensor positions transformed by a head position.
     */
    static FIFFLIB::FiffInfo transformFiffInfo(const FIFFLIB::FiffInfo& fiffInfo,
                                                const HeadPosEntry& headPosRef,
                                                const HeadPosEntry& headPosCurrent);
};

} // namespace UTILSLIB

#endif // MAXWELL_MOVEMENT_COMP_DSP_H
