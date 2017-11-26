//=============================================================================================================
/**
* @file     rtsensordataworker.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     RtSensorDataWorker class declaration.
*
*/

#ifndef DISP3DLIB_RTSENSORDATAWORKER_H
#define DISP3DLIB_RTSENSORDATAWORKER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qrgb>
#include <QSharedPointer>
#include <QLinkedList>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* This worker streams either interpolated or raw data.
*
* @brief This worker streams either interpolated or raw data.
*/
class DISP3DSHARED_EXPORT RtSensorDataWorker : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSensorDataWorker> SPtr;            /**< Shared pointer type for RtSensorDataWorker class. */
    typedef QSharedPointer<const RtSensorDataWorker> ConstSPtr; /**< Const shared pointer type for RtSensorDataWorker class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] bStreamSmoothedData      Flag whether to stream raw or interpolated raw data. This is used by the GPU and CPU usage of the SensorDataTreeItem.
    */
    explicit RtSensorDataWorker(bool bStreamSmoothedData = true);

    //=========================================================================================================
    /**
    * Add data which is to be streamed.
    *
    * @param[in] data         The new data.
    */
    void addData(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
    * Clear this worker, empties the m_lData field that holds the current block of sensor activity
    */
    void clear();
    
    //=========================================================================================================
    /**
    * Set number of vertices.
    *
    * @param[in] iNumberVerts      The number of vertices.
    */
    void setNumberVertices(int iNumberVerts);

    //=========================================================================================================
    /**
    * Set the number of average to take after emitting the data to the listening threads.
    *
    * @param[in] iNumAvr                The new number of averages.
    */
    void setNumberAverages(int iNumAvr);

    //=========================================================================================================
    /**
    * Set the type of the colormap.
    *
    * @param[in] sColormapType          The new colormap type.
    */
    void setColormapType(const QString& sColormapType);

    //=========================================================================================================
    /**
    * Set the normalization value.
    *
    * @param[in] vecThresholds          The new threshold values used for normalizing the data.
    */
    void setThresholds(const QVector3D &vecThresholds);
    
    //=========================================================================================================
    /**
    * Set the loop functionality on or off.
    *
    * @param[in] bLoopState                The new looping state.
    */
    void setLoopState(bool bLoopState);

    //=========================================================================================================
    /**
    * Set the sampling frequency.
    *
    * @param[in] dSFreq                 The new sampling frequency.
    */
    void setSFreq(const double dSFreq);

    //=========================================================================================================
    /**
    * Set the interpolation matrix.
    *
    * @param[in] matInterpolationOperator                 The new interpolation matrix.
    */
    void setInterpolationMatrix(QSharedPointer<Eigen::SparseMatrix<float>> matInterpolationOperator);

    //=========================================================================================================
    /**
    * Streams teh data.
    */
    void streamData();

protected:
    //=========================================================================================================
    /**
    * @brief normalizeAndTransformToColor  This method normalizes final values for all vertices of the mesh and converts them to rgb using the specified color converter
    *
    * @param[in] vecData                       The final values for each vertex of the surface
    * @param[in,out] matFinalVertColor         The color matrix which the results are to be written to
    * @param[in] dThresholdX                   Lower threshold for normalizing
    * @param[in] dThreholdZ                    Upper threshold for normalizing
    * @param[in] functionHandlerColorMap       The pointer to the function which converts scalar values to rgb
    */
    void normalizeAndTransformToColor(const Eigen::VectorXf& vecData,
                                      Eigen::MatrixX3f& matFinalVertColor,
                                      double dThresholdX,
                                      double dThreholdZ,
                                      QRgb (*functionHandlerColorMap)(double v));

    //=========================================================================================================
    /**
    * @brief generateColorsFromSensorValues        Produces the final color matrix that is to be emitted
    *
    * @param[in] vecSensorValues                   A vector of sensor signals
    *
    * @return The final color values for the underlying mesh surface
    */
    Eigen::MatrixX3f generateColorsFromSensorValues(const Eigen::VectorXd& vecSensorValues);

    QLinkedList<Eigen::VectorXd>                        m_lDataQ;                           /**< List that holds the fiff matrix data <n_channels x n_samples>. */
    QLinkedList<Eigen::VectorXd>::const_iterator        m_itCurrentSample;                  /**< Iterator to current sample which is/was streamed. */
    Eigen::VectorXd                                     m_vecAverage;                       /**< The averaged data to be streamed. */
    QSharedPointer<Eigen::SparseMatrix<float>>          m_matInterpolationOperator;         /**< The interpolation matrix. */

    bool                                                m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                                                m_bStreamSmoothedData;              /**< Flag if this thread's streams the raw or already smoothed data. Latter are produced by multiplying the smoothing operator here in this thread. */

    int                                                 m_iAverageSamples;                  /**< Number of average to compute. */
    int                                                 m_iSampleCtr;                       /**< The sample counter. */

    double                                              m_dSFreq;                           /**< The current sampling frequency. */

    //=========================================================================================================
    /**
    * The struct specifing visualization info.
    */
    struct VisualizationInfo {
        double                      dThresholdX;
        double                      dThresholdZ;

        Eigen::MatrixX3f            matOriginalVertColor;
        Eigen::MatrixX3f            matFinalVertColor;

        QRgb (*functionHandlerColorMap)(double v);
    } m_lVisualizationInfo;               /**< Container for the visualization info. */


signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever this item should stream new raw data to its listeners.
    *
    * @param[in] vecDataVector     The raw data.
    */
    void newRtRawData(const Eigen::VectorXd &vecDataVector);

    //=========================================================================================================
    /**
    * Emit this signal whenever this item should stream interpolated raw data to its listeners.
    *
    * @param[in] matColorMatrix     The interpolated raw data in form of rgb colors for each vertex.
    */
    void newRtSmoothedData(const Eigen::MatrixX3f &matColorMatrix);
};

} // NAMESPACE

#endif //RTSENSORDATAWORKER_H
