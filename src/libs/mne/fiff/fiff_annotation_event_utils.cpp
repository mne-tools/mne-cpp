//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_annotation_event_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the annotation ↔ event conversion helpers declared in @ref fiff_annotation_event_utils.h.
 *
 * Mirrors @c mne.events_from_annotations / @c mne.annotations_from_events
 * in MNE-Python.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_annotation_event_utils.h"

#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

FiffAnnotations mne::annotations_from_events(const MatrixXi& events,
                                             double sfreq,
                                             const std::map<int, std::string>& event_desc,
                                             int first_sample)
{
    FiffAnnotations annotations;
    if (events.rows() == 0 || events.cols() < 3) {
        return annotations;
    }
    if (sfreq <= 0.0) {
        mne::Log::warning("annotations_from_events: sfreq must be positive, got {:f}", sfreq);
        return annotations;
    }
    for (Index i = 0; i < events.rows(); ++i) {
        const int id = events(i, 2);
        const auto label = event_desc.find(id);
        annotations.append(static_cast<double>(events(i, 0) - first_sample) / sfreq, 0.0, label != event_desc.end() ? label->second : std::to_string(id));
    }
    return annotations;
}

//=============================================================================================================

MatrixXi mne::events_from_annotations(const FiffAnnotations& annotations,
                                      double sfreq,
                                      const std::map<std::string, int, std::less<>>& event_id,
                                      int first_sample)
{
    if (annotations.empty()) {
        return MatrixXi(0, 3);
    }
    if (sfreq <= 0.0) {
        mne::Log::warning("events_from_annotations: sfreq must be positive, got {:f}", sfreq);
        return MatrixXi(0, 3);
    }
    MatrixXi result(static_cast<Index>(annotations.size()), 3);
    Index row = 0;
    for (const FiffAnnotation& a : annotations) {
        const auto id = event_id.find(a.description);
        result(row, 0) = static_cast<int>(std::round(a.onset * sfreq)) + first_sample;
        result(row, 1) = 0;
        result(row, 2) = id != event_id.end() ? id->second : mne::to_int(a.description).value_or(0);
        ++row;
    }
    return result;
}

//=============================================================================================================

std::map<std::string, int, std::less<>> mne::count_annotations(const FiffAnnotations& annotations)
{
    std::map<std::string, int, std::less<>> counts;
    for (const FiffAnnotation& a : annotations) {
        ++counts[a.description];
    }
    return counts;
}
