//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcedatacontroller.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    RtSourceDataController hands the results of viz::RtSourceStream over to its thread as Qt signals.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedatacontroller.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataController::RtSourceDataController(QObject* parent)
: QObject(parent)
{
    // The callbacks run on the streaming and interpolation threads; the signals are emitted on this object's thread
    setNewSmoothedDataAvailableCallback([this](const std::vector<uint32_t>& colorsLh, const std::vector<uint32_t>& colorsRh) {
        QMetaObject::invokeMethod(this, [this, colorsLh, colorsRh] { emit newSmoothedDataAvailable(colorsLh, colorsRh); }, Qt::QueuedConnection);
    });
    setNewRawDataAvailableCallback([this](const Eigen::VectorXd& dataLh, const Eigen::VectorXd& dataRh) {
        QMetaObject::invokeMethod(this, [this, dataLh, dataRh] { emit newRawDataAvailable(dataLh, dataRh); }, Qt::QueuedConnection);
    });
    setNewInterpolationMatrixLeftAvailableCallback([this](const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat) {
        QMetaObject::invokeMethod(this, [this, interpMat] { emit newInterpolationMatrixLeftAvailable(interpMat); }, Qt::QueuedConnection);
    });
    setNewInterpolationMatrixRightAvailableCallback([this](const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat) {
        QMetaObject::invokeMethod(this, [this, interpMat] { emit newInterpolationMatrixRightAvailable(interpMat); }, Qt::QueuedConnection);
    });
}

//=============================================================================================================

RtSourceDataController::~RtSourceDataController()
{
    // The stream base joins its threads next; results they still hand over are dropped with this object
}
