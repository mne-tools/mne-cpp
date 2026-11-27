//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcedatacontroller.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Real-time source-estimate streaming controller that owns the data worker and the per-hemisphere interpolation workers.
 *
 * RtSourceDataController bundles a @ref mne::viz::RtSourceDataWorker (queue,
 * averaging, sparse mat-vec per hemisphere, colormap packing) with
 * two @ref mne::viz::RtSourceInterpolationMatWorker instances (one per
 * hemisphere) that recompute the sparse vertex-to-source weight
 * matrix when surfaces, source vertices or the interpolation
 * function change.
 *
 * The GUI sees a single object: push source vectors with @c addData,
 * scrub / play / loop via the streaming setters, and listen on
 * @c newSmoothedDataAvailable for per-vertex colour buffers.
 */

#ifndef VIZ_QT_RTSOURCEDATACONTROLLER_H
#define VIZ_QT_RTSOURCEDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/brain/realtime/rtsourcestream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief Qt face of @ref mne::viz::RtSourceStream as a QObject: the same API, with the results as signals on this object's thread.
 *
 * The streaming and interpolation threads of the stream hand their results over to this object's thread, so
 * slots connected to the signals may touch the GUI.
 */
class VIZQTSHARED_EXPORT RtSourceDataController : public QObject, public viz::RtSourceStream
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSourceDataController(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Stops the threads of the stream before the signals go away.
     */
    ~RtSourceDataController() override;

signals:
    //=========================================================================================================
    /**
     * Emitted when new interpolated per-vertex color data is available for rendering.
     *
     * @param[in] colorsLh   Per-vertex ABGR color array for the left hemisphere.
     * @param[in] colorsRh   Per-vertex ABGR color array for the right hemisphere.
     */
    void newSmoothedDataAvailable(const std::vector<uint32_t>& colorsLh,
                                  const std::vector<uint32_t>& colorsRh);

    //=========================================================================================================
    /**
     * Emitted when raw (non-interpolated) data is available.
     *
     * @param[in] dataLh     Source values for left hemisphere.
     * @param[in] dataRh     Source values for right hemisphere.
     */
    void newRawDataAvailable(const Eigen::VectorXd& dataLh,
                             const Eigen::VectorXd& dataRh);

    //=========================================================================================================
    /**
     * Emitted when a new left hemisphere interpolation matrix has been computed.
     *
     * @param[in] interpMat   Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixLeftAvailable(std::shared_ptr<Eigen::SparseMatrix<float>> interpMat);

    //=========================================================================================================
    /**
     * Emitted when a new right hemisphere interpolation matrix has been computed.
     *
     * @param[in] interpMat   Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixRightAvailable(std::shared_ptr<Eigen::SparseMatrix<float>> interpMat);
};

} // namespace mne::viz::qt

#endif // VIZ_QT_RTSOURCEDATACONTROLLER_H
