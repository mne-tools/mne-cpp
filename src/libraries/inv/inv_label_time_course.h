//=============================================================================================================
/**
 * @file     inv_label_time_course.h
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
 * @brief    InvLabelTimeCourse — extract ROI-level time courses from source estimates.
 *
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
