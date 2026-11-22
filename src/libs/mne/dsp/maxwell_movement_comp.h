//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     maxwell_movement_comp.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
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
#include <mne/base/error.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A head position entry (time, translation, rotation quaternion).
 *
 * @snippet ex_dsp_maxwell/main.cpp maxwell_movement_comp_usage
 */
struct DSPSHARED_EXPORT HeadPosEntry
{
    double dTime = 0.0;                      /**< Time in seconds. */
    Eigen::Vector3d translation{0, 0, 0};    /**< Translation from device to head (metres). */
    Eigen::Quaterniond rotation{1, 0, 0, 0}; /**< Rotation quaternion (device → head). */
    double dGof = 0.0;                       /**< Goodness of fit for this head position. */
    double dError = 0.0;                     /**< Mean coil position error (metres). */
    double dVelocity = 0.0;                  /**< Head velocity (metres per second). */
};

//=============================================================================================================
/**
 * @brief Parameters for Maxwell movement compensation.
 *
 * @snippet ex_dsp_maxwell/main.cpp maxwell_movement_comp_usage
 */
struct DSPSHARED_EXPORT MaxwellMoveCompParams
{
    int iOrderIn = 8;                       /**< Internal SSS order. */
    int iOrderOut = 3;                      /**< External SSS order. */
    Eigen::Vector3d origin{0.0, 0.0, 0.04}; /**< SSS expansion origin (metres). */
    int iRefIdx = 0;                        /**< Index of reference head position in headPos list. Use -1 for mean position. */
    double dRegIn = 1e-5;                   /**< Regularisation parameter. */
};

//=============================================================================================================
/**
 * @brief Maxwell movement compensation using SSS.
 *
 * @snippet ex_dsp_maxwell/main.cpp maxwell_movement_comp_usage
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
                                 const mne::FiffInfo& fiffInfo,
                                 const std::vector<HeadPosEntry>& headPos,
                                 double dSFreq,
                                 const MaxwellMoveCompParams& params = MaxwellMoveCompParams());

    //=========================================================================================================
    /**
     * @brief Reads a MaxFilter head position file (maxfilter -headpos), like mne.chpi.read_head_pos.
     *
     * After a header line, each line holds time, q1 q2 q3 (rotation quaternion without its scalar part),
     * q4 q5 q6 (translation), goodness of fit, error and velocity; lines starting with '#' are skipped.
     *
     * @param[in] path   Path to the .pos file.
     *
     * @return The head positions, or an error if the file cannot be read or a line has another layout.
     */
    static Expected<std::vector<HeadPosEntry>> read_head_pos(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * @brief Writes a MaxFilter head position file, like mne.chpi.write_head_pos.
     *
     * @param[in] path      Output file path.
     * @param[in] headPos   Head positions to write.
     *
     * @return Nothing, or an error if the file cannot be written.
     */
    static Expected<void> write_head_pos(const std::filesystem::path& path, std::span<const HeadPosEntry> headPos);

private:
    /**
     * @brief Create a modified FiffInfo with sensor positions transformed by a head position.
     */
    static mne::FiffInfo transformFiffInfo(const mne::FiffInfo& fiffInfo,
                                           const HeadPosEntry& headPosRef,
                                           const HeadPosEntry& headPosCurrent);
};

} // namespace mne

#endif // MAXWELL_MOVEMENT_COMP_DSP_H
