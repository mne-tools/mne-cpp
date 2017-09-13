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
#include <disp3D/helpers/interpolation/interpolation.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QTimer>


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
class CshDataWorker;

//=============================================================================================================
/**
* This class controlls all aspects that are needed for interpolation using compute shaders.
*
* @brief Interpoltion controller.
*/

class COMPUTE_SHADERSHARED_EXPORT ComputeInterpolationController : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<ComputeInterpolationController> SPtr;            /**< Shared pointer type for ComputeInterpolationController. */
    typedef QSharedPointer<const ComputeInterpolationController> ConstSPtr; /**< Const shared pointer type for ComputeInterpolationController. */

    //=========================================================================================================
    /**
    * Constructs a ComputeInterpolationController object.
    */
    explicit ComputeInterpolationController();

    //=========================================================================================================
    /**
     * Constructs a ComputeInterpolationController object and initialize it with interpolation data.
     *
     * @param tMneBemSurface        The Surface wich is used for interpolation.
     * @param tEvoked               Contains information about all sensors.
     * @param tSensorType           The type of sensor that is used for the interpolation.
     * @param tCancelDist           Distances higher than this are ignored in the calculation.
     */
    ComputeInterpolationController(const MNELIB::MNEBemSurface &tMneBemSurface,
                                   const FIFFLIB::FiffEvoked &tEvoked,
                                   double (*tInterpolationFunction) (double),
                                   const qint32 tSensorType = FIFFV_EEG_CH,
                                   const double tCancelDist = DOUBLE_INFINITY);


    //=========================================================================================================
    /**
    * Destructor, stops and deletes rtdata worker
    */
    ~ComputeInterpolationController();

    //=========================================================================================================
    /**
     * Returns a pointer to the root entity of the QEntity tree used for computing and rendering.
     *
     * @return          Pointer to root entity.
     */
    Qt3DCore::QEntity * getRootEntity() const;

    //=========================================================================================================
    /**
     * Returns a pointer to the framegraph used for computing and rendering.
     *
     * @return          Pointer to framegraph.
     */
    ComputeFramegraph *getComputeFramegraph() const;

    //=========================================================================================================
    /**
     * Initialize the object with all data needed for interpolation.
     *
     * @param tMneBemSurface        The Surface wich is used for interpolation.
     * @param tEvoked               Contains information about all sensors.
     * @param tSensorType           The type of sensor that is used for the interpolation.
     * @param tCancelDist           Distances higher than this are ignored in the calculation.
     */
    void setInterpolationData(const MNELIB::MNEBemSurface &tMneBemSurface,
                              const FIFFLIB::FiffEvoked &tEvoked,
                              double (*tInterpolationFunction) (double),
                              const qint32 tSensorType = FIFFV_EEG_CH,
                              const double tCancelDist = DOUBLE_INFINITY);

    //=========================================================================================================
    /**
     * Adds new real time data from the sensors.
     *
     * @param tSensorData           Sensor values n channels x m points in time.
     */
    void addSignalData(const Eigen::MatrixXf &tSensorData);

    //=========================================================================================================
    /**
    * Set the normalization value.
    *
    * @param[in] tVecThresholds          The new threshold values used for normalizing the data.
    */
    void setNormalization(const QVector3D &tVecThresholds);

    //=========================================================================================================
    /**
     * Starts the worker thread.
     */
    void startWorker();

    //=========================================================================================================
    /**
    * Sets the running flag to false and waits for the worker to stop.
    */
    void stopWorker();

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] tNumAvr                The new number of averages.
    */
    void setNumberAverages(const uint tNumAvr);

    //=========================================================================================================
    /**
    * Set the length in milli Seconds to wait inbetween data samples.
    *
    * @param[in] tMSec                  The new length in milli Seconds to wait inbetween data samples.
    */
    void setInterval(const uint tMSec);

    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] tLooping                The new looping state.
    */
    void setLoop(const bool tLooping);

    //=========================================================================================================
    /**
    * Set the sampling frequency.
    *
    * @param[in] tSFreq                 The new sampling frequency.
    */
    void setSFreq(const double tSFreq);


protected:

    //=========================================================================================================
    /**
    * This function gets called whenever this item receives new signal values for each sensor.
    *
    * @param[in] tSensorData         The signal values for each sensor.
    */
    void onNewRtData(const Eigen::VectorXf &tSensorData);

private:

    //=========================================================================================================
    /**
     * Init ComputeInterpolationController object.
     */
    void init();

    //=========================================================================================================
    /**
     * Creates data for the weight matrix buffer out of a interpolation matrix.
     *
     * @param tInterpolationMatrix      Weight matrix with n rows, where n is the number of vertices of the mesh
     *                                  and m columns, where m is the number of sensors.
     * @return                          Weight matrix in form of a QByteArray.
     */
    QByteArray createWeightMatBuffer(QSharedPointer<Eigen::SparseMatrix<double> > tInterpolationMatrix);

    //=========================================================================================================
    /**
     * Create a buffer with the value 0.0f for all entries.
     *
     * @param tBufferSize               Number of floats in the buffer.
     * @return                          Buffer data.
     */
    QByteArray createZeroBuffer(const uint tBufferSize);

    //=========================================================================================================
    /**
     * Create a Matrix with RGB values for each vertex.
     *
     * @param tVertices                 Number of vertices.
     * @param tColor                    Color for the vertices.
     * @return
     */
    Eigen::MatrixX3f createColorMat(const Eigen::MatrixXf& tVertices, const QColor& tColor);

    //=========================================================================================================
    bool m_bIsInit;

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

    QPointer<Qt3DRender::QBuffer> m_pWeightMatBuffer;
    QPointer<Qt3DRender::QParameter> m_pWeightMatParameter;

    QPointer<Qt3DRender::QBuffer> m_pInterpolatedSignalBuffer;

    QPointer<Qt3DRender::QParameter> m_pColsUniform;


    QVector<uint> m_iSensorsBad;    /**< Store bad channel indexes.*/
    QVector<uint> m_iUsedSensors;   /**< Stores the indices of channels inside the passed fiff evoked that are used for interpolation. */

    //The threshold values used for normalizing the data
    float m_fThresholdX;     /**< Lower threshold.*/
    float m_fThresholdZ;     /**< Upper threshold.*/   

    QPointer<Qt3DRender::QParameter> m_pThresholdXUniform;
    QPointer<Qt3DRender::QParameter> m_pThresholdZUniform;

    QPointer<CshDataWorker>     m_pRtDataWorker;             /**< The source data worker. This worker streams the rt data to this item.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CSH

#ifndef metatype_vectorxd
#define metatype_vectorxd
Q_DECLARE_METATYPE(Eigen::VectorXf);
#endif

#endif // CSH_COMPUTEINTERPOLATIONCONTROLLER_H
