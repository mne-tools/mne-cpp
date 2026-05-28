//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     meshfactory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Static factory for procedural Qt-RHI meshes (spheres, plates, barbells, cylinders, batched point clouds).
 *
 * MeshFactory builds the small primitive geometries used throughout
 * disp3D &mdash; subdivided icosahedron spheres for source-space and
 * digitizer points, oriented plates for MEG magnetometers, barbell
 * shapes for gradiometers, cylinders for sensor leads and connectivity
 * edges. Each primitive is returned as a fully populated @ref BrainSurface
 * with interleaved position / normal / colour vertex data so it
 * drops straight into the standard render pipeline.
 *
 * For point clouds (source-space, head-shape digitization), all
 * spheres are merged into a single BrainSurface so the GPU draws
 * thousands of points with one @c drawIndexed call &mdash; this is the
 * same single-draw constraint imposed by the WebGL backend that the
 * rest of disp3D obeys.
 */

#ifndef MESHFACTORY_H
#define MESHFACTORY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>
#include <Eigen/Core>
#include <memory>

class BrainSurface;

//=============================================================================================================
/**
 * @brief Static factory for procedural mesh primitives.
 *
 * All methods return fully constructed BrainSurface objects ready for rendering.
 * This eliminates duplicated icosahedron / plate / barbell code that was spread
 * across BrainView::onRowsInserted.
 */
class DISP3DSHARED_EXPORT MeshFactory
{
public:
    MeshFactory() = delete; // Static-only class

    //=========================================================================================================
    /**
     * Create a subdivided icosahedron sphere.
     *
     * @param[in] center     World-space center of the sphere.
     * @param[in] radius     Radius in meters.
     * @param[in] color      FsSurface color.
     * @param[in] subdivisions  Number of subdivision passes (0 = 20 tris, 1 = 80 tris).
     * @return Shared pointer to the created BrainSurface.
     */
    static std::shared_ptr<BrainSurface> createSphere(const QVector3D &center,
                                                       float radius,
                                                       const QColor &color,
                                                       int subdivisions = 1);

    //=========================================================================================================
    /**
     * Create a thin oriented rectangular plate (used for MEG magnetometers).
     *
     * @param[in] center     World-space center.
     * @param[in] orientation  3×3 rotation embedded in a 4×4 matrix (coil_trans).
     * @param[in] color      FsSurface color.
     * @param[in] size       Side length in meters (plate is size × size × thin).
     * @return Shared pointer to the created BrainSurface.
     */
    static std::shared_ptr<BrainSurface> createPlate(const QVector3D &center,
                                                      const QMatrix4x4 &orientation,
                                                      const QColor &color,
                                                      float size);

    //=========================================================================================================
    /**
     * Create a barbell shape (two spheres connected by a thin rod), used for
     * MEG gradiometers.
     *
     * @param[in] center     World-space center.
     * @param[in] orientation  3×3 rotation embedded in a 4×4 matrix (coil_trans).
     * @param[in] color      FsSurface color.
     * @param[in] size       Overall size scale in meters.
     * @return Shared pointer to the created BrainSurface.
     */
    static std::shared_ptr<BrainSurface> createBarbell(const QVector3D &center,
                                                        const QMatrix4x4 &orientation,
                                                        const QColor &color,
                                                        float size);

    //=========================================================================================================
    /**
     * Create a batched mesh of identical spheres at multiple positions (used
     * for source space points, digitizer points, etc.).
     *
     * All spheres are merged into a single BrainSurface for efficient rendering.
     *
     * @param[in] positions  World-space positions for each sphere.
     * @param[in] radius     Sphere radius in meters.
     * @param[in] color      Sphere color.
     * @param[in] subdivisions  Number of subdivision passes per sphere.
     * @return Shared pointer to the batched BrainSurface.
     */
    static std::shared_ptr<BrainSurface> createBatchedSpheres(const QVector<QVector3D> &positions,
                                                               float radius,
                                                               const QColor &color,
                                                               int subdivisions = 1);

    //=========================================================================================================
    /**
     * Create a cylinder between two world-space endpoints.
     *
     * @param[in] from     Start point (metres).
     * @param[in] to       End point (metres).
     * @param[in] radius   Cylinder radius (metres).
     * @param[in] color    Surface color.
     * @param[in] sides    Number of sides around the circumference (min 3).
     * @return Shared pointer to the created BrainSurface.
     */
    static std::shared_ptr<BrainSurface> createCylinder(const QVector3D &from,
                                                         const QVector3D &to,
                                                         float radius,
                                                         const QColor &color,
                                                         int sides = 12);

    //=========================================================================================================
    /**
     * Number of vertices in a subdivided icosahedron (for picking calculations).
     *
     * @param[in] subdivisions  Number of subdivision passes.
     * @return Number of vertices.
     */
    static int sphereVertexCount(int subdivisions = 1);

private:
    //=========================================================================================================
    /**
     * Build a base icosahedron and optionally subdivide it.
     *
     * @param[out] vertices  Unit-sphere vertex positions (normalized).
     * @param[out] faces     Triangle index triplets.
     * @param[in]  subdivisions  Number of subdivision passes.
     */
    static void buildIcosphere(QVector<Eigen::Vector3f> &vertices,
                               QVector<Eigen::Vector3i> &faces,
                               int subdivisions);
};

#endif // MESHFACTORY_H
