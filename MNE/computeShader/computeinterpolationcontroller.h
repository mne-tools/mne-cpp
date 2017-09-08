//=============================================================================================================
/**
* @file     computeinterpolationcontroller.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     ComputeInterpolationController class declaration.
*
*/

#ifndef CSH_COMPUTEINTERPOLATIONCONTROLLER_H
#define CSH_COMPUTEINTERPOLATIONCONTROLLER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computeShader_global.h"
#include <fiff/fiff_types.h>
#include <interpolation/interpolation.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QHash>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Sparse>
#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffEvoked;
}

namespace MNELIB {
    class MNEBemSurface;
}

namespace DISP3DLIB {
    class CustomMesh;
}

namespace Qt3DRender {
    class QComputeCommand;
    class QCamera;
    class QParameter;
    class QBuffer;
    class QAttribute;
}

namespace Qt3DExtras {
    class QFirstPersonCameraController;
}

namespace Qt3DCore {
    class QEntity;
    class QTransform;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSH
//=============================================================================================================

namespace CSH {


//*************************************************************************************************************
//=============================================================================================================
// CSH FORWARD DECLARATIONS
//=============================================================================================================

class ComputeMaterial;
class ComputeFramegraph;

//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class COMPUTE_SHADERSHARED_EXPORT ComputeInterpolationController
{

public:
    typedef QSharedPointer<ComputeInterpolationController> SPtr;            /**< Shared pointer type for ComputeInterpolationController. */
    typedef QSharedPointer<const ComputeInterpolationController> ConstSPtr; /**< Const shared pointer type for ComputeInterpolationController. */

    //=========================================================================================================
    /**
    * Constructs a ComputeInterpolationController object.
    */
    //const MNELIB::MNEBemSurface &tMneBemSurface, const FIFFLIB::FiffEvoked &tEvoked,
    explicit ComputeInterpolationController( );

    //=========================================================================================================
    /**
     * @brief getRootEntity
     * @return
     */
    Qt3DCore::QEntity * getRootEntity() const;

    //=========================================================================================================
    /**
     * @brief getComputeFramegraph
     * @return
     */
    ComputeFramegraph *getComputeFramegraph() const;

    void setInterpolationData(const MNELIB::MNEBemSurface &tMneBemSurface,
                              const FIFFLIB::FiffEvoked &tEvoked,
                              double (*tInterpolationFunction) (double),
                              const qint32 tSensorType = FIFFV_EEG_CH,
                              const double tCancelDist = DOUBLE_INFINITY);


    void addSignalData(const Eigen::MatrixXf &tSensorData);

protected:

private:

    void init();

    QByteArray createWeightMatBuffer(QSharedPointer<Eigen::SparseMatrix<double> > tInterpolationMatrix);

    //=========================================================================================================
    /**
     * Create a buffer with tBufferSize floats with the value 0.0f
     * @param tBufferSize
     * @return
     */
    QByteArray createZeroBuffer(const uint tBufferSize);

    //=========================================================================================================
    /**
     * Create a Matrix with rgb values for each vertex
     * @param tVertices
     * @param tColor
     * @return
     */
    Eigen::MatrixX3f createColorMat(const Eigen::MatrixXf& tVertices, const QColor& tColor);

    QPointer<Qt3DCore::QEntity> m_pRootEntity;
    QPointer<CSH::ComputeFramegraph> m_pFramegraph;
    QPointer<CSH::ComputeMaterial> m_pMaterial;
    QPointer<DISP3DLIB::CustomMesh> m_pCustomMesh;

    QPointer<Qt3DCore::QEntity> m_pComputeEntity;
    QPointer<Qt3DRender::QComputeCommand> m_pComputeCommand;

    QPointer<Qt3DCore::QEntity> m_pMeshRenderEntity;

    QPointer<Qt3DRender::QCamera> m_pCamera;
    QPointer<Qt3DExtras::QFirstPersonCameraController> m_pCamController;
    QPointer<Qt3DCore::QTransform> m_pTransform;

    QPointer<Qt3DRender::QAttribute> m_pInterpolatedSignalAttrib;


    QVector<uint> m_iSensorsBad;    /**< Store bad channel indexes.*/
    QVector<uint> m_iUsedSensors;   /**< Stores the indices of channels inside the passed fiff evoked that are used for interpolation. */

    QHash<QString, QPointer<Qt3DRender::QParameter>> m_pParameters;  /**< Stores all Parameters with their name.*/
    QHash<QString, QPointer<Qt3DRender::QBuffer>> m_pBuffers;        /**< Stores all Buffers with their name.*/

    //The threshold values used for normalizing the data
    float m_fThresholdX;     /**< Lower threshold.*/
    float m_fThresholdZ;     /**< Upper threshold.*/

    QPointer<Qt3DRender::QParameter> m_pThresholdXUniform;
    QPointer<Qt3DRender::QParameter> m_pThresholdZUniform;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#endif // CSH_COMPUTEINTERPOLATIONCONTROLLER_H
