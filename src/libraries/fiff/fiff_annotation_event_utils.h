//=============================================================================================================
/**
 * @file     fiff_annotation_event_utils.h
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
 * @brief    Annotation / Event conversion utility functions.
 *
 */

#ifndef FIFF_ANNOTATION_EVENT_UTILS_H
#define FIFF_ANNOTATION_EVENT_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_annotations.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Convert a FIFF event matrix to FiffAnnotations.
 *
 * Each event becomes an annotation with onset = (sample - firstSample) / sfreq
 * and duration = 0.  The description is looked up from eventDescriptions
 * (event_id -> label).  If not found, the event id is used as a string.
 *
 * @param[in] events              FIFF events (Nx3: [sample, before, after]).
 * @param[in] sfreq               Sampling frequency in Hz.
 * @param[in] eventDescriptions   Map from event id (the "after" column) to description string.
 * @param[in] firstSample         First sample offset (default 0).
 * @return FiffAnnotations containing one annotation per event.
 */
FIFFSHARED_EXPORT FiffAnnotations annotationsFromEvents(
    const Eigen::MatrixXi& events,
    double sfreq,
    const QMap<int, QString>& eventDescriptions = QMap<int, QString>(),
    int firstSample = 0);

//=============================================================================================================
/**
 * Convert FiffAnnotations to a FIFF event matrix.
 *
 * Each annotation becomes an event row [sample, 0, event_id].
 * The event_id is looked up from eventIds (description -> id).
 * If not found, the description is parsed as an integer; if that also
 * fails, event_id is set to 0.
 *
 * @param[in] annotations   The annotations to convert.
 * @param[in] sfreq         Sampling frequency in Hz.
 * @param[in] eventIds      Map from description string to event id integer.
 * @param[in] firstSample   First sample offset (default 0).
 * @return Eigen::MatrixXi  Nx3 event matrix [sample, 0, event_id].
 */
FIFFSHARED_EXPORT Eigen::MatrixXi eventsFromAnnotations(
    const FiffAnnotations& annotations,
    double sfreq,
    const QMap<QString, int>& eventIds = QMap<QString, int>(),
    int firstSample = 0);

//=============================================================================================================
/**
 * Count annotations by description.
 *
 * @param[in] annotations   The annotations to count.
 * @return QMap<QString, int>  Map from description to count.
 */
FIFFSHARED_EXPORT QMap<QString, int> countAnnotations(const FiffAnnotations& annotations);

} // namespace FIFFLIB

#endif // FIFF_ANNOTATION_EVENT_UTILS_H
