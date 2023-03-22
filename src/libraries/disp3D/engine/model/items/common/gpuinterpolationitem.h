 //=============================================================================================================
/**
 * @file     gpuinterpolationitem.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief     GpuInterpolationItem class declaration.
 *
 */

#ifndef DISP3DLIB_GPUINTERPOLATIONITEM_H
#define DISP3DLIB_GPUINTERPOLATIONITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define QT_NAMESPACE_3D Qt3DRender
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#else
#define QT_NAMESPACE_3D Qt3DCore
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QAttribute>
#endif


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DRender {
    class QComputeCommand;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class CustomMesh;
class GpuInterpolationMaterial;

//=============================================================================================================
/**
 * This item is used for signal interpolation with GPU support.
 *
 * @brief This item is used for signal interpolation with GPU support.
 */

class DISP3DSHARED_EXPORT GpuInterpolationItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<GpuInterpolationItem> SPtr;            /**< Shared pointer type for GpuInterpolationItem. */
    typedef QSharedPointer<const GpuInterpolationItem> ConstSPtr; /**< Const shared pointer type for GpuInterpolationItem. */

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] p3DEntityParent    The parent 3D entity.
     * @param[in] iType              The type of the item. See types.h for declaration and definition.
     * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
     */
    explicit GpuInterpolationItem(Qt3DCore::QEntity* p3DEntityParent = Q_NULLPTR,
                                  int iType = Data3DTreeModelItemTypes::GpuInterpolationItem,
                                  const QString& text = "3D Plot");

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~GpuInterpolationItem();

    //=========================================================================================================
    /**
     * Initialize interpolation data of this item.
     *
     * @param[in] matVertices       The surface vertices.
     * @param[in] matNormals        The surface normals.
     * @param[in] matTriangles      The surface triangles.
     */
    virtual void initData(const Eigen::MatrixX3f &matVertices,
                          const Eigen::MatrixX3f &matNormals,
                          const Eigen::MatrixX3i &matTriangles);

    //=========================================================================================================
    /**
     * Set the new Interpolation matrix for the interpolation.
     *
     * @param[in] pMatInterpolationMatrix  The new Interpolation matrix for interpolation on the bem surface.
     */
    virtual void setInterpolationMatrix(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix);

    //=========================================================================================================
    /**
     * Add a new vector with signal data form the sensors.
     *
     * @param[in] tSignalVec              Vector with one float value for each sensor.
     */
    virtual void addNewRtData(const Eigen::VectorXf &tSignalVec);

    //=========================================================================================================
    /**
     * This function set the normalization value.
     *
     * @param[in] vecThresholds       The new threshold values used for normalizing the data.
     */
    virtual void setThresholds(const QVector3D& tVecThresholds);

    //=========================================================================================================
    /**
     * This function sets the colormap type
     *
     * @param[in] tColormapType           The new colormap name.
     */
    virtual void setColormapType(const QString& tColormapType);

protected:
    //=========================================================================================================
    /**
     * Build the content of the Interpolation matrix buffer.
     *
     * @param[in] pMatInterpolationMatrix    The Interpolation matrix.
     *
     * @return                          Interpolation matrix is byte array form.
     */
    virtual QByteArray buildInterpolationMatrixBuffer(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix);

    //=========================================================================================================
    /**
     * Build buffer filled with 0.0f.
     *
     * @param[in] tSize         Number of zeros.
     *
     * @return              Buffer content.
     */
    virtual QByteArray buildZeroBuffer(const uint tSize);

    bool                                    m_bIsDataInit;                  /**< The data initialization flag. */

    QPointer<GpuInterpolationMaterial>      m_pGPUMaterial;                 /**< Compute material used for the process. */

    QPointer<CustomMesh>                    m_pCustomMesh;                  /**< The actual mesh information (vertices, normals, colors). */
    QPointer<Qt3DRender::QComputeCommand>   m_pComputeCommand;              /**< The compute command defines the work group size for the compute shader code execution . */

    QPointer<QT_NAMESPACE_3D::QBuffer>           m_pInterpolationMatBuffer;      /**< The QBuffer/GLBuffer holding the interpolation matrix data. */
    QPointer<QT_NAMESPACE_3D::QBuffer>           m_pOutputColorBuffer;           /**< The QBuffer/GLBuffer holding the output color (interpolated) data. */
    QPointer<QT_NAMESPACE_3D::QBuffer>           m_pSignalDataBuffer;            /**< The QBuffer/GLBuffer holding the signal data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace DISP3DLIB

#endif // DISP3DLIB_GPUINTERPOLATIONITEM_H
