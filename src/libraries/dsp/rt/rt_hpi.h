//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     rt_hpi.h
 * @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March, 2015
 * @brief    Continuous head position tracking from the HPI coil signals of streaming MEG data.
 *
 * RtHpi fits the HPI coils of @ref mne::InvHpiFit over a window of the newest samples each time a hop of
 * new samples has arrived, so the head position is current to within one hop rather than one window. Each
 * fit starts from the last accepted head position; a fit whose mean coil error exceeds the limit is
 * reported but neither moves the head position nor seeds the next fit. It is synchronous: call it from
 * whichever thread receives the data.
 */

#ifndef RT_HPI_RTPROCESSING_H
#define RT_HPI_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_info.h>
#include <inv/hpi/inv_hpi_data_updater.h>
#include <inv/hpi/inv_hpi_fit.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Head position of a streaming MEG recording, fitted from its HPI coils over a sliding window.
 *
 * @snippet ex_dsp_rt/main.cpp rt_hpi_usage
 */
class DSPSHARED_EXPORT RtHpi
{
public:
    /** @brief Coil frequencies and the sliding window. */
    struct Settings
    {
        std::vector<int> coil_frequencies; /**< HPI coil frequencies in Hz, in the order of the digitized coils. */
        int window = 1000;                 /**< Samples per fit. */
        int hop = 200;                     /**< New samples between fits; at most the window. */
        double max_error = 0.01;           /**< Mean coil error in m up to which a fit is accepted. */
    };

    //=========================================================================================================
    /**
     * @param[in] info       Measurement info with the MEG channels, the digitized HPI coils and the initial
     *                       device to head transformation.
     * @param[in] settings   Coil frequencies and the sliding window.
     */
    RtHpi(std::shared_ptr<FiffInfo> info, Settings settings);

    //=========================================================================================================
    /**
     * Adds a block of data and fits the newest window once a hop of new samples has arrived.
     *
     * @param[in] data   All channels of the measurement info by samples.
     *
     * @return The fit of the newest window, accepted or not, if one was due; none if fewer than three coil
     *         frequencies are set or the data has another channel count.
     */
    [[nodiscard]] std::optional<HpiFitResult> update(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * @param[in] fit   A fit returned by update().
     *
     * @return Whether the fit was accepted as the new head position.
     */
    [[nodiscard]] bool accepted(const HpiFitResult& fit) const;

    //=========================================================================================================
    /**
     * @param[in] maxError   Mean coil error in m up to which a fit is accepted.
     */
    void set_max_error(double maxError);

    //=========================================================================================================
    /**
     * @return The device to head transformation of the last accepted fit, or the initial one.
     */
    [[nodiscard]] const FiffCoordTrans& head_transform() const;

    //=========================================================================================================
    /**
     * @param[in] projectors   SSP and compensation projectors, all channels by all channels; identity by default.
     */
    void set_projectors(Eigen::MatrixXd projectors);

    //=========================================================================================================
    /**
     * @param[in] frequencies   HPI coil frequencies in Hz.
     */
    void set_coil_frequencies(std::vector<int> frequencies);

    //=========================================================================================================
    /**
     * @return The coil frequencies, reordered to the digitized coils after order_coil_frequencies().
     */
    [[nodiscard]] const std::vector<int>& coil_frequencies() const;

    //=========================================================================================================
    /**
     * Assigns the coil frequencies to the digitized coils on the next fit.
     */
    void order_coil_frequencies();

    //=========================================================================================================
    /**
     * Takes over changed bad channels of the measurement info.
     *
     * @param[in] info   Measurement info of the data.
     */
    void set_info(std::shared_ptr<FiffInfo> info);

    //=========================================================================================================
    /**
     * @param[in] window   Samples per fit.
     * @param[in] hop      New samples between fits; at most the window.
     */
    void set_window(int window, int hop);

    //=========================================================================================================
    /**
     * Forgets the buffered samples and starts the next fit without a head position.
     */
    void reset();

private:
    std::shared_ptr<FiffInfo> m_info; /**< Measurement info of the data. */
    Settings m_settings;              /**< Coil frequencies and the sliding window. */
    InvHpiDataUpdater m_updater;      /**< Channel selection, projectors and digitized coils. */
    InvHpiFit m_fit;                  /**< The coil fit. */
    Eigen::MatrixXd m_projectors;     /**< SSP and compensation projectors. */
    Eigen::MatrixXd m_ring;           /**< The newest samples, written cyclically. */
    Eigen::Index m_next = 0;          /**< Column of the ring the next sample goes to. */
    Eigen::Index m_filled = 0;        /**< Samples in the ring. */
    Eigen::Index m_sinceFit = 0;      /**< Samples since the last fit. */
    bool m_order = false;             /**< Whether the next fit orders the coil frequencies. */
    HpiFitResult m_last;              /**< Last accepted fit, the start of the next one. */
    FiffCoordTrans m_initial;         /**< Device to head transformation of the measurement info. */
};

} // namespace mne

#endif // RT_HPI_RTPROCESSING_H
