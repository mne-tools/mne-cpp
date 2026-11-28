//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qtconversions.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Conversions between the types of mne_viz (Eigen vectors and matrices, viz::Color) and Qt's.
 */

#ifndef VIZ_QT_QTCONVERSIONS_H
#define VIZ_QT_QTCONVERSIONS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/figures/style.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
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
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

/**
 * @param[in] v   A Qt vector.
 * @return The Eigen vector.
 */
inline Eigen::Vector3f toEigen(const QVector3D& v)
{
    return {v.x(), v.y(), v.z()};
}

/**
 * @param[in] v   A Qt vector.
 * @return The Eigen vector.
 */
inline Eigen::Vector2f toEigen(const QVector2D& v)
{
    return {v.x(), v.y()};
}

/**
 * @param[in] q   A Qt quaternion.
 * @return The Eigen quaternion.
 */
inline Eigen::Quaternionf toEigen(const QQuaternion& q)
{
    return {q.scalar(), q.x(), q.y(), q.z()};
}

/**
 * @param[in] m   A Qt matrix.
 * @return The Eigen matrix.
 */
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

/**
 * @param[in] v   An Eigen vector.
 * @return The Qt vector.
 */
inline QVector3D toQVector3D(const Eigen::Vector3f& v)
{
    return {v.x(), v.y(), v.z()};
}

/**
 * @param[in] v   An Eigen vector.
 * @return The Qt vector.
 */
inline QVector2D toQVector2D(const Eigen::Vector2f& v)
{
    return {v.x(), v.y()};
}

/**
 * @param[in] q   An Eigen quaternion.
 * @return The Qt quaternion.
 */
inline QQuaternion toQQuaternion(const Eigen::Quaternionf& q)
{
    return {q.w(), q.x(), q.y(), q.z()};
}

/**
 * @param[in] m   An Eigen matrix.
 * @return The Qt matrix.
 */
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

/**
 * @param[in] v   Qt vectors.
 * @return The Eigen vectors.
 */
inline std::vector<Eigen::Vector3f> toEigen(const std::vector<QVector3D>& v)
{
    std::vector<Eigen::Vector3f> result;
    result.reserve(v.size());
    for (const auto& p : v) {
        result.push_back(toEigen(p));
    }
    return result;
}

/**
 * @param[in] c   A Qt colour.
 * @return The mne_viz colour.
 */
inline viz::Color toColor(const QColor& c)
{
    return {static_cast<std::uint8_t>(c.red()), static_cast<std::uint8_t>(c.green()), static_cast<std::uint8_t>(c.blue()),
            static_cast<std::uint8_t>(c.alpha())};
}

/**
 * @param[in] c   An mne_viz colour.
 * @return The Qt colour.
 */
inline QColor toQColor(const viz::Color& c)
{
    return QColor(c.r, c.g, c.b, c.a);
}

/**
 * @param[in] colors   Qt colours by name.
 * @return The mne_viz colours by name.
 */
inline std::map<std::string, viz::Color> toColors(const std::map<std::string, QColor>& colors)
{
    std::map<std::string, viz::Color> result;
    for (const auto& [name, color] : colors) {
        result.emplace(name, toColor(color));
    }
    return result;
}

} // namespace mne::viz::qt

#endif // VIZ_QT_QTCONVERSIONS_H
