//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_label_time_course.h
 * @since 2026
 * @date  May 2026
 * @brief ROI-level aggregation of vertex-wise source estimates — the C++ peer of MNE-Python's @c extract_label_time_course.
 *
 * @ref INVLIB::InvLabelTimeCourse provides the five standard aggregation
 * modes used by mne-python — @c mean, @c mean_flip, @c pca_flip, @c max
 * and @c auto — to collapse a per-vertex @ref InvSourceEstimate into one
 * time-course per FreeSurfer label. Sign-flip vectors are derived from
 * the dominant orientation of the vertices in the label so that
 * phase-locked averaging works on signed surface data without
 * cancellation. The output matrix is @c (n_labels × n_times) and feeds
 * directly into the connectivity, statistics and plotting libraries.
 */

#ifndef INV_LABEL_TIME_COURSE_H
#define INV_LABEL_TIME_COURSE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include "inv_source_estimate.h"

#include <fs/fs_label.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Extract ROI-level time courses from vertex-level source estimates.
 *
 * Provides the five standard aggregation modes used in MNE-Python's
 * `extract_label_time_course()`:
 * - **mean**: Simple average across vertices in the label.
 * - **mean_flip**: Mean with sign-flip so that dominant orientation is positive.
 * - **pca_flip**: First PCA component, flipped to align with dominant sign.
 * - **max**: Maximum absolute value at each time point.
 * - **auto**: mean_flip for scalar STCs.
 *
 * @code
 *   QList<FSLIB::FsLabel> labels = ...;
 *   InvSourceEstimate stc = ...;
 *   Eigen::MatrixXd tc = InvLabelTimeCourse::extract(stc, labels, "mean_flip");
 *   // tc: n_labels × n_times
 * @endcode
 */
class INVSHARED_EXPORT InvLabelTimeCourse
{
public:
    //=========================================================================================================
    /**
     * Extract label time courses from a source estimate.
     *
     * @param[in] stc              Source estimate (vertices × times).
     * @param[in] labels           List of labels defining ROIs.
     * @param[in] sMode            Aggregation mode: "mean", "mean_flip", "pca_flip", "max", "auto".
     * @param[in] bAllowEmpty      If true, empty labels produce zero rows; if false, skip them.
     * @return                     Matrix of shape (n_labels × n_times).
     */
    static Eigen::MatrixXd extract(const InvSourceEstimate& stc,
                                   const QList<FSLIB::FsLabel>& labels,
                                   const QString& sMode = "mean_flip",
                                   bool bAllowEmpty = false);

    //=========================================================================================================
    /**
     * Compute sign-flip vector for a label based on vertex normals.
     *
     * For surface source spaces, the sign of each vertex's contribution
     * should be flipped if its normal opposes the dominant direction in
     * the label. This ensures coherent averaging.
     *
     * @param[in] stcData          Source data for the label vertices (n_verts × n_times).
     * @return                     Sign vector (+1 or -1 per vertex).
     */
    static Eigen::VectorXd computeSignFlip(const Eigen::MatrixXd& stcData);
};

} // namespace INVLIB

#endif // INV_LABEL_TIME_COURSE_H
