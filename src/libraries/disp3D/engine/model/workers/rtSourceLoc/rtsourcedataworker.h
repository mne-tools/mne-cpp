//=============================================================================================================
/**
 * @file     rtsourcedataworker.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief     RtSourceDataWorker class declaration.
 *
 */

#ifndef DISP3DLIB_RTSOURCEDATAWORKER_H
#define DISP3DLIB_RTSOURCEDATAWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

#include <disp/plots/helpers/colormap.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QRgb>
#include <QSharedPointer>
#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

struct VisualizationInfo {
    double                      dThresholdX;
    double                      dThresholdZ;

    Eigen::VectorXd             vecSensorValues;
    Eigen::MatrixX4f            matOriginalVertColor;
    Eigen::MatrixX4f            matFinalVertColor;

    QSharedPointer<Eigen::SparseMatrix<float> >  pMatInterpolationMatrix;         /**< The interpolation matrix. */

    QString sColormapType;
    QRgb (*functionHandlerColorMap)(double v, const QString& sColorMap) = DISPLIB::ColorMap::valueToColor;
}; /**< The struct specifing visualization info. */

struct ColorComputationInfo {
    double                      dThresholdX;
    double                      dThresholdZ;
    int                         iFinalMatSize;

    Eigen::VectorXf             vecData;

    QRgb (*functionHandlerColorMap)(double v);
};

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This worker streams either interpolated or raw data.
 *
 * @brief This worker streams either interpolated or raw data.
 */
class DISP3DSHARED_EXPORT RtSourceDataWorker : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSourceDataWorker> SPtr;            /**< Shared pointer type for RtSourceDataWorker class. */
    typedef QSharedPointer<const RtSourceDataWorker> ConstSPtr; /**< Const shared pointer type for RtSourceDataWorker class. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit RtSourceDataWorker();

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
     * Set the color of the vertices for the left and right hemisphere.
     *
     * @param[in] matColorLeft      The color of the vertices for the left hemisphere.
     * @param[in] matColorRight     The color of the vertices for the right hemisphere.
     */
    void setSurfaceColor(const Eigen::MatrixX4f &matColorLeft,
                         const Eigen::MatrixX4f &matColorRight);

    //=========================================================================================================
    /**
     * Set the number of average to take after emitting the data to the listening threads.
     *
     * @param[in] iNumAvr                The new number of averages.
     */
    void setNumberAverages(int iNumAvr);

    //=========================================================================================================
    /**
     * Sets the state whether to stream smoothed or raw data
     *
     * @param[in] bStreamSmoothedData                 The new state.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

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
     * Set the interpolation matrix for the left hemisphere.
     *
     * @param[in] pMatInterpolationMatrixLeft                 The new interpolation matrix.
     */
    void setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeft);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the right hemisphere.
     *
     * @param[in] pMatInterpolationMatrixRight                 The new interpolation matrix.
     */
    void setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRight);

    //=========================================================================================================
    /**
     * Streams the data.
     */
    void streamData();

protected:
    //=========================================================================================================
    /**
     * @brief normalizeAndTransformToColor  This method normalizes final values for all vertices of the mesh and converts them to rgb using the specified color converter
     *
     * @param[in] vecData                       The final values for each vertex of the surface.
     * @param[in, out] matFinalVertColor         The color matrix which the results are to be written to.
     * @param[in] dThresholdX                   Lower threshold for normalizing.
     * @param[in] dThresholdZ                   Upper threshold for normalizing.
     * @param[in] functionHandlerColorMap       The pointer to the function which converts scalar values to rgb.
     * @param[in] sColorMap                     The color map to use.
     */
    static void normalizeAndTransformToColor(const Eigen::VectorXf& vecData,
                                             Eigen::MatrixX4f &matFinalVertColor,
                                             double dThresholdX,
                                             double dThresholdZ,
                                             QRgb (*functionHandlerColorMap)(double v, const QString& sColorMap),
                                             const QString& sColorMap);

    //=========================================================================================================
    /**
     * @brief generateColorsFromSensorValues     Produces the final color matrix that is to be emitted
     *
     * @param[in/out] visualizationInfoHemi      The needed visualization info.
     */
    static void generateColorsFromSensorValues(VisualizationInfo &visualizationInfoHemi);

    QList<Eigen::VectorXd>                              m_lDataQ;                           /**< List that holds the matrix data <n_channels x n_samples>. */
    QList<Eigen::VectorXd>                              m_lDataLoopQ;                       /**< List that holds the matrix data <n_channels x n_samples> for looping. */
    Eigen::VectorXd                                     m_vecAverage;                       /**< The averaged data to be streamed. */

    bool                                                m_bIsLooping;                       /**< Flag if this thread should repeat sending the same data over and over again. */
    bool                                                m_bStreamSmoothedData;              /**< Flag if this thread's streams the raw or already smoothed data. Latter are produced by multiplying the smoothing operator here in this thread. */

    int                                                 m_iCurrentSample;                   /**< Iterator to current sample which is/was streamed. */
    int                                                 m_iAverageSamples;                  /**< Number of average to compute. */
    int                                                 m_iSampleCtr;                       /**< The sample counter. */

    double                                              m_dSFreq;                           /**< The current sampling frequency. */

    QList<VisualizationInfo>                            m_lHemiVisualizationInfo;           /**< The visualization info for each hemisphere. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever this item should stream new raw data to its listeners.
     *
     * @param[in] vecDataVectorLeftHemi          The new streamed raw data for the left hemispehre.
     * @param[in] vecDataVectorRightHemi         The new streamed raw data for the right hemispehre.
     */
    void newRtRawData(const Eigen::VectorXd &vecDataVectorLeftHemi,
                      const Eigen::VectorXd &vecDataVectorRightHemi);

    //=========================================================================================================
    /**
     * Emit this signal whenever this item should stream interpolated raw data to its listeners.
     *
     * @param[in] matColorMatrixLeftHemi          The new streamed interpolated raw data in form of RGB colors per vertex for the left hemisphere.
     * @param[in] matColorMatrixRightHemi         The new streamed interpolated raw data in form of RGB colors per vertex for the right hemisphere.
     */
    void newRtSmoothedData(const Eigen::MatrixX4f &matColorMatrixLeftHemi,
                           const Eigen::MatrixX4f &matColorMatrixRightHemi);
};
} // NAMESPACE

#endif //DISP3DLIB_RTSOURCEDATAWORKER_H
