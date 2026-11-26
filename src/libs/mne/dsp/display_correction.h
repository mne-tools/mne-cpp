//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     display_correction.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    DisplayCorrection applies the SSP projector and the CTF compensator to data shown in a viewer.
 *
 * A viewer shows raw or evoked data with the active SSP projectors and the selected CTF compensation grade
 * applied. DisplayCorrection builds both operators from the measurement info, keeps them as sparse matrices
 * and applies them to a block of channels by samples. Projector columns of bad channels are zeroed, so a bad
 * channel never leaks into the projected data. It has no GUI dependency; the Qt models of the viewers use it.
 */

#ifndef DSP_DISPLAY_CORRECTION_H
#define DSP_DISPLAY_CORRECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffInfo;

//=============================================================================================================
/**
 * @brief Builds and applies the SSP projector and the CTF compensator of a viewer.
 *
 * @since 3.0.0
 */
class DSPSHARED_EXPORT DisplayCorrection
{
public:
    //=========================================================================================================
    /**
     * Resets both operators to the identity of @p channels channels and deactivates them.
     *
     * @param[in] channels  Number of channels.
     *
     * @since 3.0.0
     */
    void reset(int channels);

    //=========================================================================================================
    /**
     * Rebuilds the SSP projector from the projectors and bad channels of @p info. The projector is active
     * when at least one projector of @p info is active.
     *
     * @param[in] info  Measurement info with the projectors and the bad channels.
     *
     * @since 3.0.0
     */
    void update_projection(const FiffInfo& info);

    //=========================================================================================================
    /**
     * Rebuilds the compensator that takes the data from grade 0 to grade @p to. Grade 0 deactivates it.
     *
     * @param[in] info  Measurement info with the compensation data.
     * @param[in] to    Target compensation grade.
     *
     * @since 3.0.0
     */
    void update_compensator(const FiffInfo& info, int to);

    //=========================================================================================================
    /**
     * Applies the active operators to @p data. An operator whose size does not match the rows of @p data,
     * or that is inactive, is skipped.
     *
     * @param[in] data  Data, channels by samples.
     *
     * @return The corrected data.
     *
     * @since 3.0.0
     */
    Eigen::MatrixXd apply(const Eigen::MatrixXd& data) const;

    //=========================================================================================================
    /**
     * @return Whether at least one SSP projector is active.
     *
     * @since 3.0.0
     */
    bool projection_active() const
    {
        return m_projActive;
    }

    //=========================================================================================================
    /**
     * @return Whether a compensation grade other than 0 is selected.
     *
     * @since 3.0.0
     */
    bool compensator_active() const
    {
        return m_compActive;
    }

    //=========================================================================================================
    /**
     * @return The dense SSP projector, with the columns of bad channels zeroed.
     *
     * @since 3.0.0
     */
    const Eigen::MatrixXd& projector() const
    {
        return m_proj;
    }

    //=========================================================================================================
    /**
     * @return The dense compensator.
     *
     * @since 3.0.0
     */
    const Eigen::MatrixXd& compensator() const
    {
        return m_comp;
    }

private:
    void combine();

    bool m_projActive = false;
    bool m_compActive = false;
    Eigen::MatrixXd m_proj;
    Eigen::MatrixXd m_comp;
    Eigen::SparseMatrix<double> m_sparseProj;
    Eigen::SparseMatrix<double> m_sparseComp;
    Eigen::SparseMatrix<double> m_sparseProjComp;
};

} // namespace mne

#endif // DSP_DISPLAY_CORRECTION_H
