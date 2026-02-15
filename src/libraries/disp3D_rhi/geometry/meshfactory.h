//=============================================================================================================
/**
 * @file     meshfactory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    MeshFactory class declaration — static utilities for generating
 *           primitive meshes (spheres, plates, barbells, batched point clouds).
 *
 */

#ifndef MESHFACTORY_H
#define MESHFACTORY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include "renderable/brainsurface.h"

#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>
#include <memory>

//=============================================================================================================
/**
 * @brief Static factory for procedural mesh primitives.
 *
 * All methods return fully constructed BrainSurface objects ready for rendering.
 * This eliminates duplicated icosahedron / plate / barbell code that was spread
 * across BrainView::onRowsInserted.
 */
class DISP3DRHISHARED_EXPORT MeshFactory
{
public:
    MeshFactory() = delete; // Static-only class

    //=========================================================================================================
    /**
     * Create a subdivided icosahedron sphere.
     *
     * @param[in] center     World-space center of the sphere.
     * @param[in] radius     Radius in meters.
     * @param[in] color      Surface color.
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
     * @param[in] color      Surface color.
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
     * @param[in] color      Surface color.
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
