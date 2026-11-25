//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qteigen_p.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Private conversions between the Eigen types of mne_scene and the Qt vector types of the QRhi backend.
 */

#ifndef DISP3D_QTEIGEN_P_H
#define DISP3D_QTEIGEN_P_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

inline Eigen::Vector3f toEigen(const QVector3D& v)
{
    return {v.x(), v.y(), v.z()};
}

inline Eigen::Vector2f toEigen(const QVector2D& v)
{
    return {v.x(), v.y()};
}

inline Eigen::Quaternionf toEigen(const QQuaternion& q)
{
    return {q.scalar(), q.x(), q.y(), q.z()};
}

inline Eigen::Matrix4f toEigen(const QMatrix4x4& m)
{
    Eigen::Matrix4f result;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            result(r, c) = m(r, c);
        }
    }
    return result;
}

inline QVector3D toQVector3D(const Eigen::Vector3f& v)
{
    return {v.x(), v.y(), v.z()};
}

inline QVector2D toQVector2D(const Eigen::Vector2f& v)
{
    return {v.x(), v.y()};
}

inline QQuaternion toQQuaternion(const Eigen::Quaternionf& q)
{
    return {q.w(), q.x(), q.y(), q.z()};
}

inline QMatrix4x4 toQMatrix(const Eigen::Matrix4f& m)
{
    QMatrix4x4 result;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            result(r, c) = m(r, c);
        }
    }
    return result;
}

} // namespace mne::disp

#endif // DISP3D_QTEIGEN_P_H
