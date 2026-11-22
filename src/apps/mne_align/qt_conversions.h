// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

// Conversions between the Eigen types of the digitiser and the Qt types the 3D view draws with.
#ifndef MNE_ALIGN_QT_CONVERSIONS_H
#define MNE_ALIGN_QT_CONVERSIONS_H

#include <Eigen/Geometry>
#include <QQuaternion>
#include <QVector3D>

namespace MNEALIGN
{
inline QVector3D toQt(const Eigen::Vector3f& v)
{
    return QVector3D(v.x(), v.y(), v.z());
}

inline QQuaternion toQt(const Eigen::Quaternionf& q)
{
    return QQuaternion(q.w(), q.x(), q.y(), q.z());
}

inline Eigen::Vector3f toEigen(const QVector3D& v)
{
    return Eigen::Vector3f(v.x(), v.y(), v.z());
}
} // namespace MNEALIGN

#endif // MNE_ALIGN_QT_CONVERSIONS_H
