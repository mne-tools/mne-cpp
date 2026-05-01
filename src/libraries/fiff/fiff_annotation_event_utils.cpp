//=============================================================================================================
/**
 * @file     fiff_annotation_event_utils.cpp
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
 * @brief    Definition of Annotation / Event conversion utility functions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_annotation_event_utils.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

FiffAnnotations FIFFLIB::annotationsFromEvents(
    const MatrixXi& events,
    double sfreq,
    const QMap<int, QString>& eventDescriptions,
    int firstSample)
{
    FiffAnnotations annotations;

    if (events.rows() == 0 || events.cols() < 3) {
        return annotations;
    }

    if (sfreq <= 0.0) {
        qWarning("annotationsFromEvents: sfreq must be positive, got %f", sfreq);
        return annotations;
    }

    for (int i = 0; i < static_cast<int>(events.rows()); ++i) {
        const int sample  = events(i, 0);
        const int eventId = events(i, 2);

        const double onset = static_cast<double>(sample - firstSample) / sfreq;

        QString description;
        if (eventDescriptions.contains(eventId)) {
            description = eventDescriptions.value(eventId);
        } else {
            description = QString::number(eventId);
        }

        annotations.append(onset, 0.0, description);
    }

    return annotations;
}

//=============================================================================================================

MatrixXi FIFFLIB::eventsFromAnnotations(
    const FiffAnnotations& annotations,
    double sfreq,
    const QMap<QString, int>& eventIds,
    int firstSample)
{
    if (annotations.isEmpty()) {
        return MatrixXi(0, 3);
    }

    if (sfreq <= 0.0) {
        qWarning("eventsFromAnnotations: sfreq must be positive, got %f", sfreq);
        return MatrixXi(0, 3);
    }

    const int n = annotations.size();
    MatrixXi result(n, 3);

    for (int i = 0; i < n; ++i) {
        const FiffAnnotation& a = annotations[i];

        const int sample = static_cast<int>(std::round(a.onset * sfreq)) + firstSample;

        int eventId = 0;
        if (eventIds.contains(a.description)) {
            eventId = eventIds.value(a.description);
        } else {
            bool ok = false;
            const int parsed = a.description.toInt(&ok);
            if (ok) {
                eventId = parsed;
            }
        }

        result(i, 0) = sample;
        result(i, 1) = 0;
        result(i, 2) = eventId;
    }

    return result;
}

//=============================================================================================================

QMap<QString, int> FIFFLIB::countAnnotations(const FiffAnnotations& annotations)
{
    QMap<QString, int> counts;

    for (int i = 0; i < annotations.size(); ++i) {
        counts[annotations[i].description] += 1;
    }

    return counts;
}
