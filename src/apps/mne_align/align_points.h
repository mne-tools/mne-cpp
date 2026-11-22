// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

// The digitised points of the session as a QObject that signals every change to the wizard and the 3D view.
#ifndef MNE_ALIGN_ALIGN_POINTS_H
#define MNE_ALIGN_ALIGN_POINTS_H

#include <mne/acq/devices/polhemus/acquired_points.h>

#include <QObject>

namespace MNEALIGN
{
class AlignPoints : public QObject, public mne::AcquiredPoints
{
    Q_OBJECT

public:
    explicit AlignPoints(QObject* parent = nullptr)
    : QObject(parent)
    {
        set_on_changed([this] { emit pointsChanged(); });
    }

signals:
    void pointsChanged();
};
} // namespace MNEALIGN

#endif // MNE_ALIGN_ALIGN_POINTS_H
