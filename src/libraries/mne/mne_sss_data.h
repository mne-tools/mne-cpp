//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_sss_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Signal Space Separation (Maxwell filter) basis metadata stored alongside MEG raw data.
 *
 * @ref mne::MNESssData captures the inside/outside basis orders
 * (@c LIn, @c LOut), the device-to-head transformation in effect at
 * the time of Maxwell filtering and the multipole moments computed by
 * MaxFilter. The information lives in @c FIFFB_SSS_INFO /
 * @c FIFFB_SSS_CAL_ADJUST blocks and is required to keep downstream
 * forward / inverse computations consistent with the SSS-filtered data.
 */

#ifndef MNESSSDATA_H
#define MNESSSDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <memory>
#include <ostream>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffStream;
class FiffDirNode;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Implements MNE SSS Data (Replaces *mneSssData,mneSssDataRec struct of MNE-C mne_types.h).
 *
 * @brief Container for Signal Space Separation (SSS/Maxwell filtering) expansion coefficients and metadata.
 *
 * @snippet ex_mne_api/main.cpp mne_sss_data_usage
 */
class MNESHARED_EXPORT MNESssData
{
public:
    //=========================================================================================================
    /**
     * Constructs the MNE SSS Data
     */
    MNESssData();

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: mne_dup_sss_data (mne_sss_data.c)
     *
     * @param[in] p_MneSssData   MNE SSS Data which should be copied.
     */
    MNESssData(const MNESssData& p_MneSssData);

    //=========================================================================================================
    /**
     * Destroys the MNE SSS Data description
     */
    ~MNESssData();

    //=========================================================================================================
    /**
     * Read SSS data from anywhere in a file
     * Refactored: mne_read (mne_sss_data.c)
     *
     * @param[in] name       Name of the file to read the SSS data from.
     *
     * @return   The read SSS data.
     */
    static std::unique_ptr<MNESssData> read(const std::filesystem::path& name);

    //=========================================================================================================
    /**
     * Read the SSS data from the given node of an open fiff stream
     * Refactored: mne_read_from_node (mne_sss_data.c)
     *
     * @param[in] stream     The open fiff stream.
     * @param[in] start      The node/tree to read the SSS data from.
     *
     * @return   The read SSS data.
     */
    static std::unique_ptr<MNESssData> read_from_node(mne::FiffStream& stream, const std::shared_ptr<mne::FiffDirNode>& start);

    //=========================================================================================================
    /**
     * Output the SSS information for debugging purposes.
     * Refactored: mne_print_sss_data (mne_sss_data.c)
     *
     * @param[in] out      The stream to write the SSS diagnostics to.
     */
    void print(std::ostream& out) const;

public:
    int job;                   /**< Value of FIFF_SSS_JOB tag. */
    int coord_frame;           /**< Coordinate frame. */
    float origin[3];           /**< The expansion origin. */
    int nchan;                 /**< How many channels. */
    int out_order;             /**< Order of the outside expansion. */
    int in_order;              /**< Order of the inside expansion. */
    Eigen::VectorXi comp_info; /**< Which components are included. */
    int in_nuse;               /**< How many components included in the inside expansion. */
    int out_nuse;              /**< How many components included in the outside expansion. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNESSSDATA_H
