//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_annotation_event_utils.cpp
 * @since May 2026
 * @brief Implementation of the annotation ↔ event conversion helpers declared in @ref fiff_annotation_event_utils.h.
 *
 * Mirrors @c mne.events_from_annotations / @c mne.annotations_from_events
 * in MNE-Python.
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
