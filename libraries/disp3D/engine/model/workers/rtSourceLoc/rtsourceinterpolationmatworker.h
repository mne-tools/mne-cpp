//=============================================================================================================
/**
 * @file     rtsourceinterpolationmatworker.h
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
 * @brief     RtSourceInterpolationMatWorker class declaration.
 *
 */

#ifndef DISP3DLIB_RTSOURCEINTERPOLATIONMATWORKER_H
#define DISP3DLIB_RTSOURCEINTERPOLATIONMATWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

#include <fs/label.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This worker calculates the interpolation matrix.
 *
 * @brief This worker calculates the interpolation matrix.
 */
class DISP3DSHARED_EXPORT RtSourceInterpolationMatWorker : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtSourceInterpolationMatWorker> SPtr;            /**< Shared pointer type for RtSourceInterpolationMatWorker class. */
    typedef QSharedPointer<const RtSourceInterpolationMatWorker> ConstSPtr; /**< Const shared pointer type for RtSourceInterpolationMatWorker class. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    RtSourceInterpolationMatWorker();

    //=========================================================================================================
    /**
     * This function sets the function that is used in the interpolation process.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] sInterpolationFunction     Function that computes interpolation coefficients using the distance values.
     */
    void setInterpolationFunction(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Set the visualization type.
     *
     * @param[in] iVisType               The new visualization type.
     */
    void setVisualizationType(int iVisType);

    //=========================================================================================================
    /**
     * This function sets the cancel distance used in distance calculations for the interpolation.
     * Distances higher than this are ignored, i.e. the respective coefficients are set to zero.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] dCancelDist           The new cancel distance value in meters.
     */
    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
     * Sets the information needed creating the interpolation matrix.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] matVertices               The mesh information in form of vertices.
     * @param[in] vecNeighborVertices       The neighbor vertex information.
     * @param[in] vecMappedSubset           Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to.
     *
     * @return Returns the created interpolation matrix.
     */
    void setInterpolationInfo(const Eigen::MatrixX3f &matVertices,
                              const QVector<QVector<int> > &vecNeighborVertices,
                              const QVector<int> &vecMappedSubset);

    //=========================================================================================================
    /**
     * Set annotation data.
     *
     * @param[in] vecLabelIds       The labels ids for each of the right hemipshere surface vertex idx.
     * @param[in] lLabels           The label information for the right hemipshere.
     * @param[in] vecVertNo         The vertNos for the right hemisphere.
     */
    void setAnnotationInfo(const Eigen::VectorXi &vecLabelIds,
                           const QList<FSLIB::Label> &lLabels,
                           const Eigen::VectorXi &vecVertNo);

protected:    
    //=========================================================================================================
    /**
     * Calculate the interpolation operator based on the set interpolation info.
     */
    void calculateInterpolationOperator();

    //=========================================================================================================
    /**
     * Calculate the annotation operator based on the set annotation info.
     */
    void calculateAnnotationOperator();

    //=========================================================================================================
    /**
     * Emit the interpolation matrix.
     */
    void emitMatrix();

    //=============================================================================================================
    /**
     * The struct specifing all data that is used in the interpolation process
     */
    struct InterpolationData {
        double                          dCancelDistance;                /**< Cancel distance for the interpolaion in meters. */

        QSharedPointer<Eigen::MatrixXd> matDistanceMatrix;              /**< Distance matrix that holds distances from sensors positions to the near vertices in meters. */
        Eigen::MatrixX3f                matVertices;                    /**< Holds all vertex information. */

        QList<FSLIB::Label>             lLabels;                        /**< The annotation labels. */
        QList<int>                      vertNos;
        QMap<qint32, qint32>            mapLabelIdSources;              /**< The mapped label ID to sources. */

        QVector<int>                 vecMappedSubset;                /**< Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to. */
        QVector<QVector<int> >          vecNeighborVertices;            /**< The neighbor vertex information. */

        double (*interpolationFunction) (double);                   /**< Function that computes interpolation coefficients using the distance values. */
    }                           m_lInterpolationData;               /**< Container for the interpolation data. */

    bool                        m_bInterpolationInfoIsInit;         /**< Flag if this thread's interpoaltion data was initialized. */
    bool                        m_bAnnotationInfoIsInit;            /**< Flag if this thread's annotation data was initialized. This flag is used to decide whether specific visualization types can be computed. */

    int                         m_iVisualizationType;               /**< The visualization type (smoothing or annotation based). */

    QSharedPointer<Eigen::SparseMatrix<float> >  m_pMatInterpolationMat;              /**< The current itnerpolation matrix (keep this as member so we can easily switch between interpolation and annotation based visualization). */
    QSharedPointer<Eigen::SparseMatrix<float> >  m_pMatAnnotationMat;                 /**< The current itnerpolation matrix (keep this as member so we can easily switch between interpolation and annotation based visualization). */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever a new interpolation matrix was calcualted.
     *
     * @param[in] pMatInterpolationMatrix     The new interpolation matrix.
     */
    void newInterpolationMatrixCalculated(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrix);
};
} // NAMESPACE

#endif //DISP3DLIB_RTSOURCEINTERPOLATIONMATWORKER_H
