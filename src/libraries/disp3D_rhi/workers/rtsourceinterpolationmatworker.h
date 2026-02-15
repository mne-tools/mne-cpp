//=============================================================================================================
/**
 * @file     rtsourceinterpolationmatworker.h
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
 * @brief    RtSourceInterpolationMatWorker class declaration.
 *
 */

#ifndef BRAINVIEW_RTSOURCEINTERPOLATIONMATWORKER_H
#define BRAINVIEW_RTSOURCEINTERPOLATIONMATWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QList>
#include <QString>
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class Label;
}

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DRHILIB {

//=============================================================================================================
/**
 * RtSourceInterpolationMatWorker computes sparse source-to-vertex interpolation
 * matrices in a background thread.
 *
 * This worker encapsulates the expensive GeometryInfo::scdc() (geodesic distance
 * computation) and Interpolation::createInterpolationMat() calls that were
 * previously performed either in StcLoadingWorker (at load time only) or not
 * recomputed at all when parameters changed at runtime.
 *
 * The worker for each hemisphere stores:
 *   - Surface vertex positions and neighbor topology
 *   - Source vertex indices (from the STC file)
 *   - Cancel distance and interpolation function
 *
 * When any parameter changes, computeInterpolationMatrix() can be called
 * (typically via a queued connection) to recompute the sparse interpolation
 * matrix on the worker thread. When done, the result is emitted via
 * newInterpolationMatrixAvailable().
 *
 * @brief Background worker for computing source interpolation matrices.
 */
class DISP3DRHISHARED_EXPORT RtSourceInterpolationMatWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Visualization type enum — selects which matrix the worker emits.
     */
    enum VisualizationType {
        InterpolationBased = 0,   /**< Smooth distance-based interpolation (default). */
        AnnotationBased    = 1    /**< Per-parcellation uniform coloring from annotation. */
    };

    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSourceInterpolationMatWorker(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Set the interpolation function used for weight computation.
     *
     * Available functions: "linear", "gaussian", "square", "cubic" (default).
     *
     * @param[in] sInterpolationFunction    Function name.
     */
    void setInterpolationFunction(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Set the cancel distance used in SCDC distance calculations.
     * Distances larger than this are ignored (coefficients set to zero).
     *
     * @param[in] dCancelDist    Cancel distance in meters (default: 0.05).
     */
    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
     * Set the surface and source geometry for the left hemisphere.
     *
     * @param[in] matVertices          Vertex positions (nVerts x 3).
     * @param[in] vecNeighborVertices  Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices    Source vertex indices into the surface.
     */
    void setInterpolationInfoLeft(const Eigen::MatrixX3f &matVertices,
                                  const QVector<QVector<int>> &vecNeighborVertices,
                                  const QVector<int> &vecSourceVertices);

    //=========================================================================================================
    /**
     * Set the surface and source geometry for the right hemisphere.
     *
     * @param[in] matVertices          Vertex positions (nVerts x 3).
     * @param[in] vecNeighborVertices  Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices    Source vertex indices into the surface.
     */
    void setInterpolationInfoRight(const Eigen::MatrixX3f &matVertices,
                                   const QVector<QVector<int>> &vecNeighborVertices,
                                   const QVector<int> &vecSourceVertices);

    //=========================================================================================================
    /**
     * Set the visualization type.
     * When set to AnnotationBased, the worker emits annotation matrices
     * instead of interpolation matrices.
     *
     * @param[in] iVisType    VisualizationType enum value.
     */
    void setVisualizationType(int iVisType);

    //=========================================================================================================
    /**
     * Set annotation info for the left hemisphere.
     * Required for AnnotationBased visualization.
     *
     * @param[in] vecLabelIds   Per-vertex label IDs for the surface.
     * @param[in] lLabels       List of FreeSurfer Labels.
     * @param[in] vecVertNo     Source-space vertex numbers.
     */
    void setAnnotationInfoLeft(const Eigen::VectorXi &vecLabelIds,
                               const QList<FSLIB::Label> &lLabels,
                               const Eigen::VectorXi &vecVertNo);

    //=========================================================================================================
    /**
     * Set annotation info for the right hemisphere.
     *
     * @param[in] vecLabelIds   Per-vertex label IDs for the surface.
     * @param[in] lLabels       List of FreeSurfer Labels.
     * @param[in] vecVertNo     Source-space vertex numbers.
     */
    void setAnnotationInfoRight(const Eigen::VectorXi &vecLabelIds,
                                const QList<FSLIB::Label> &lLabels,
                                const Eigen::VectorXi &vecVertNo);

public slots:
    //=========================================================================================================
    /**
     * Compute the interpolation matrices for both hemispheres.
     * This slot performs the heavy SCDC + interpolation matrix computation.
     * It should be called from a worker thread via a queued connection.
     */
    void computeInterpolationMatrix();

signals:
    //=========================================================================================================
    /**
     * Emitted when a new left hemisphere interpolation matrix is available.
     *
     * @param[in] interpMat    Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixLeftAvailable(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);

    //=========================================================================================================
    /**
     * Emitted when a new right hemisphere interpolation matrix is available.
     *
     * @param[in] interpMat    Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixRightAvailable(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);

private:
    //=========================================================================================================
    /**
     * Resolve the interpolation function pointer from the function name string.
     *
     * @param[in] name    Function name ("linear", "gaussian", "square", "cubic").
     * @return Function pointer for use with Interpolation::createInterpolationMat().
     */
    static double (*resolveInterpolationFunction(const QString &name))(double);

    //=========================================================================================================
    /**
     * Compute the interpolation matrix for one hemisphere.
     *
     * @param[in] matVertices          Vertex positions.
     * @param[in] vecNeighborVertices  Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices    Source vertex indices.
     * @param[in] dCancelDist          Cancel distance.
     * @param[in] interpFunc           Interpolation kernel function.
     * @return Shared pointer to the computed sparse interpolation matrix.
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> computeHemi(
        const Eigen::MatrixX3f &matVertices,
        const QVector<QVector<int>> &vecNeighborVertices,
        QVector<int> vecSourceVertices,
        double dCancelDist,
        double (*interpFunc)(double));

    //=========================================================================================================
    /**
     * Compute annotation operator for one hemisphere.
     * Builds a sparse matrix where each label's vertices share the average
     * of sources falling within that label.
     *
     * @param[in] lLabels       FreeSurfer labels.
     * @param[in] mapLabelIdSrc Map from source vertex number to label ID.
     * @param[in] vertNos       List of source vertex numbers.
     * @return Annotation matrix (nVertices x nSources).
     */
    static QSharedPointer<Eigen::SparseMatrix<float>> computeAnnotationOperator(
        const QList<FSLIB::Label> &lLabels,
        const QMap<qint32, qint32> &mapLabelIdSrc,
        const QList<int> &vertNos);

    mutable QMutex m_mutex;                                      /**< Protects all data members. */

    // LH geometry
    Eigen::MatrixX3f m_matVerticesLh;                            /**< LH vertex positions. */
    QVector<QVector<int>> m_vecNeighborsLh;                      /**< LH per-vertex neighbors. */
    QVector<int> m_vecSourceVerticesLh;                           /**< LH source vertex indices. */
    bool m_hasLh = false;                                        /**< Whether LH data has been set. */

    // RH geometry
    Eigen::MatrixX3f m_matVerticesRh;                            /**< RH vertex positions. */
    QVector<QVector<int>> m_vecNeighborsRh;                      /**< RH per-vertex neighbors. */
    QVector<int> m_vecSourceVerticesRh;                           /**< RH source vertex indices. */
    bool m_hasRh = false;                                        /**< Whether RH data has been set. */

    // Interpolation parameters
    double m_dCancelDist = 0.05;                                 /**< Cancel distance in meters. */
    QString m_sInterpolationFunction = QStringLiteral("cubic");  /**< Active interpolation function name. */

    // Visualization type
    int m_iVisualizationType = InterpolationBased;               /**< Current visualization type. */

    // LH annotation data
    QList<FSLIB::Label> m_lLabelsLh;                             /**< LH FreeSurfer labels. */
    QMap<qint32, qint32> m_mapLabelIdSourcesLh;                  /**< LH source vertex → label ID map. */
    QList<int> m_vertNosLh;                                      /**< LH source vertex numbers. */
    bool m_bAnnotationLhInit = false;                             /**< Whether LH annotation data is set. */

    // RH annotation data
    QList<FSLIB::Label> m_lLabelsRh;                             /**< RH FreeSurfer labels. */
    QMap<qint32, qint32> m_mapLabelIdSourcesRh;                  /**< RH source vertex → label ID map. */
    QList<int> m_vertNosRh;                                      /**< RH source vertex numbers. */
    bool m_bAnnotationRhInit = false;                             /**< Whether RH annotation data is set. */
};

} // namespace DISP3DRHILIB

#endif // BRAINVIEW_RTSOURCEINTERPOLATIONMATWORKER_H
