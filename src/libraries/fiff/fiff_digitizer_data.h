//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_digitizer_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    High-level digitization data: dig points plus the device→head transform and fitting metadata that together define a coregistration source.
 *
 * Where @ref mne::FiffDigPointSet is a flat point list, @ref mne::FiffDigitizerData
 * is the registration-ready view: the dig points themselves, the
 * device→head @ref mne::FiffCoordTrans recovered from HPI fits, the per-coil
 * goodness of fit, and any comments that travelled with the block. It is
 * the structure the coregistration tooling persists into
 * ``-dig.fif`` / ``-fiducials.fif`` files and the structure that the
 * forward / inverse pipeline consumes when registering a subject's MRI to
 * the MEG sensor frame.
 */

#ifndef FIFF_DIGITIZER_DATA_H
#define FIFF_DIGITIZER_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_coord_trans.h"
#include "fiff_dig_point_set.h"

#include <base/error.h>

#include <Eigen/Core>


#include <filesystem>
#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Registration-ready digitization data: dig points, device→head transform and HPI fit metadata.
 *
 * Pairs a @ref FiffDigPointSet with the @ref FiffCoordTrans that maps it
 * into the head frame, plus optional HPI goodness-of-fit values. Used by
 * the coregistration GUI and by every downstream tool that needs a single
 * self-contained record describing where a subject's head sat in the
 * helmet for one measurement.
 *
 * @snippet ex_fiff_api/main.cpp fiff_digitizer_data_usage
 */
class FIFFSHARED_EXPORT FiffDigitizerData
{
public:
    /** Constructs empty digitizer data. */
    FiffDigitizerData() = default;

    //=========================================================================================================
    /**
     * Reads the digitizer data of a FIFF file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The FIFF file.
     */
    explicit FiffDigitizerData(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the digitizer data of a FIFF file: the points of its isotrak block in head coordinates,
     * or of its MRI set in MRI coordinates.
     *
     * @param[in] path   The FIFF file.
     *
     * @return The data, or why the file cannot be read: Io if it cannot be opened, NotFound if it
     *         holds no digitizer points.
     */
    [[nodiscard]] static mne::Expected<FiffDigitizerData> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Prints the number of points, their frame, the HPI coils and the number of MRI fiducials.
     */
    void print() const;

    /** @return The number of MRI fiducials, as MNE-C's digitizerDataRec.nfids. */
    int nfids() const
    {
        return static_cast<int>(mri_fids.size());
    }

    //=========================================================================================================
    /**
     * Sets mri_fids to the cardinal points moved into MRI coordinates by head_mri_t_adj, as MNE-C's
     * update_fids_from_dig_data (mne_analyze/adjust_alignment.c); clears them without that transform.
     */
    void pick_cardinal_fiducials();

    std::filesystem::path filename;                                         /**< Source file. */
    std::optional<FiffCoordTrans> head_mri_t;                               /**< Head to MRI coordinate transformation. */
    std::optional<FiffCoordTrans> head_mri_t_adj;                           /**< Adjusted head to MRI transformation. */
    FiffDigPointSet points;                                                 /**< The digitizer points. */
    int coord_frame = FIFFV_COORD_UNKNOWN;                                  /**< The coordinate frame of the points. */
    std::vector<bool> active;                                               /**< Which points are active. */
    std::vector<bool> discard;                                              /**< Which points should be discarded. */
    FiffDigPointSet mri_fids;                                               /**< MRI coordinate system fiducials. */
    bool show = false;                                                      /**< Whether the digitizer data should be shown. */
    bool show_minimal = false;                                              /**< Show fiducials and coils only. */
    Eigen::VectorXf dist;                                                   /**< Distance of each point from the head surface. */
    Eigen::VectorXi closest;                                                /**< Closest vertex number on the head surface. */
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> closest_point; /**< Closest vertex locations on the head surface (points x 3). */
    bool dist_valid = false;                                                /**< Whether the above distance data is valid. */
};

} // NAMESPACE

#endif // FIFF_DIGITIZER_DATA_H
