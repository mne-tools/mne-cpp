//=============================================================================================================
/**
 * @file     sourceestimatemanager.cpp
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
 * @brief    SourceEstimateManager class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimatemanager.h"
#include "../renderable/sourceestimateoverlay.h"
#include "../renderable/brainsurface.h"
#include "../workers/stcloadingworker.h"
#include "../workers/rtsourcedatacontroller.h"
#include "../core/viewstate.h"

#include <QThread>
#include <QDebug>
#include <QSet>
#include <cmath>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateManager::SourceEstimateManager(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

SourceEstimateManager::~SourceEstimateManager()
{
    if (m_loadingThread) {
        m_loadingThread->quit();
        m_loadingThread->wait();
    }
}

//=============================================================================================================

bool SourceEstimateManager::load(const QString &lhPath, const QString &rhPath,
                                  const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                                  const QString &activeSurfaceType)
{
    if (m_isLoading) {
        qWarning() << "SourceEstimateManager: STC loading already in progress";
        return false;
    }

    // Find surfaces for the active surface type
    BrainSurface *lhSurface = nullptr;
    BrainSurface *rhSurface = nullptr;

    const QString lhKey = "lh_" + activeSurfaceType;
    const QString rhKey = "rh_" + activeSurfaceType;

    if (surfaces.contains(lhKey))
        lhSurface = surfaces[lhKey].get();
    if (surfaces.contains(rhKey))
        rhSurface = surfaces[rhKey].get();

    // Fallback: search for any lh_*/rh_* brain surface
    if (!lhSurface || !rhSurface) {
        for (auto it = surfaces.begin(); it != surfaces.end(); ++it) {
            if (it.value() && it.value()->tissueType() == BrainSurface::TissueBrain) {
                if (!lhSurface && it.key().startsWith("lh_")) {
                    lhSurface = it.value().get();
                    qDebug() << "SourceEstimateManager: Using fallback LH surface:" << it.key();
                } else if (!rhSurface && it.key().startsWith("rh_")) {
                    rhSurface = it.value().get();
                    qDebug() << "SourceEstimateManager: Using fallback RH surface:" << it.key();
                }
            }
        }
    }

    if (!lhSurface && !rhSurface) {
        qWarning() << "SourceEstimateManager: No surfaces available for STC loading."
                    << "Active surface type:" << activeSurfaceType
                    << "Available keys:" << surfaces.keys();
        return false;
    }

    // Clean up any previous loading thread
    if (m_loadingThread) {
        m_loadingThread->quit();
        m_loadingThread->wait();
        delete m_loadingThread;
        m_loadingThread = nullptr;
    }

    // Create overlay for results
    m_overlay = std::make_unique<SourceEstimateOverlay>();

    // Create worker and thread
    m_loadingThread = new QThread(this);
    m_stcWorker = new StcLoadingWorker(lhPath, rhPath, lhSurface, rhSurface);
    m_stcWorker->moveToThread(m_loadingThread);

    connect(m_loadingThread, &QThread::started, m_stcWorker, &StcLoadingWorker::process);
    connect(m_stcWorker, &StcLoadingWorker::progress, this, &SourceEstimateManager::loadingProgress);
    connect(m_stcWorker, &StcLoadingWorker::finished, this, &SourceEstimateManager::onStcLoadingFinished);
    connect(m_stcWorker, &StcLoadingWorker::finished, m_loadingThread, &QThread::quit);
    connect(m_loadingThread, &QThread::finished, m_stcWorker, &QObject::deleteLater);

    m_isLoading = true;
    m_loadingThread->start();

    return true;
}

//=============================================================================================================

bool SourceEstimateManager::isLoaded() const
{
    return m_overlay && m_overlay->isLoaded();
}

//=============================================================================================================

void SourceEstimateManager::onStcLoadingFinished(bool success)
{
    m_isLoading = false;

    if (!success || !m_stcWorker) {
        qWarning() << "SourceEstimateManager: Async STC loading failed";
        m_overlay.reset();
        return;
    }

    // Transfer data from worker to overlay
    if (m_stcWorker->hasLh()) {
        m_overlay->setStcData(m_stcWorker->stcLh(), 0);
        if (m_stcWorker->interpolationMatLh())
            m_overlay->setInterpolationMatrix(m_stcWorker->interpolationMatLh(), 0);
    }

    if (m_stcWorker->hasRh()) {
        m_overlay->setStcData(m_stcWorker->stcRh(), 1);
        if (m_stcWorker->interpolationMatRh())
            m_overlay->setInterpolationMatrix(m_stcWorker->interpolationMatRh(), 1);
    }

    m_overlay->updateThresholdsFromData();
    emit thresholdsUpdated(m_overlay->thresholdMin(),
                           m_overlay->thresholdMid(),
                           m_overlay->thresholdMax());

    if (m_overlay->isLoaded()) {
        emit loaded(m_overlay->numTimePoints());
    } else {
        m_overlay.reset();
    }
}

//=============================================================================================================

void SourceEstimateManager::setTimePoint(int index,
                                          const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                                          const SubView &singleView,
                                          const QVector<SubView> &subViews)
{
    if (!m_overlay || !m_overlay->isLoaded()) return;

    m_currentTimePoint = qBound(0, index, m_overlay->numTimePoints() - 1);

    // Collect all distinct surface types used across single + multi views
    QSet<QString> activeTypes;
    activeTypes.insert(singleView.surfaceType);
    for (int i = 0; i < subViews.size(); ++i)
        activeTypes.insert(subViews[i].surfaceType);

    // Apply source estimate to surfaces matching ANY active type
    for (auto it = surfaces.begin(); it != surfaces.end(); ++it) {
        for (const QString &type : activeTypes) {
            if (it.key().endsWith(type)) {
                m_overlay->applyToSurface(it.value().get(), m_currentTimePoint);
                break;
            }
        }
    }

    emit timePointChanged(m_currentTimePoint, m_overlay->timeAtIndex(m_currentTimePoint));
}

//=============================================================================================================

float SourceEstimateManager::tstep() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tstep() : 0.0f;
}

//=============================================================================================================

float SourceEstimateManager::tmin() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->tmin() : 0.0f;
}

//=============================================================================================================

int SourceEstimateManager::numTimePoints() const
{
    return (m_overlay && m_overlay->isLoaded()) ? m_overlay->numTimePoints() : 0;
}

//=============================================================================================================

int SourceEstimateManager::closestIndex(float timeSec) const
{
    if (!m_overlay || !m_overlay->isLoaded()) return -1;

    const float t0 = m_overlay->tmin();
    const float dt = m_overlay->tstep();
    const int numPts = m_overlay->numTimePoints();
    if (numPts <= 0 || dt <= 0.0f) return -1;

    const int idx = qRound((timeSec - t0) / dt);
    return qBound(0, idx, numPts - 1);
}

//=============================================================================================================

void SourceEstimateManager::setColormap(const QString &name)
{
    if (m_overlay)
        m_overlay->setColormap(name);
}

//=============================================================================================================

void SourceEstimateManager::setThresholds(float min, float mid, float max)
{
    if (m_overlay)
        m_overlay->setThresholds(min, mid, max);

    if (m_rtController)
        m_rtController->setThresholds(min, mid, max);
}

//=============================================================================================================

void SourceEstimateManager::startStreaming(const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                                            const SubView &singleView,
                                            const QVector<SubView> &subViews)
{
    Q_UNUSED(surfaces)
    Q_UNUSED(singleView)
    Q_UNUSED(subViews)

    if (m_isStreaming) {
        qDebug() << "SourceEstimateManager: Real-time streaming already active";
        return;
    }

    if (!m_overlay || !m_overlay->isLoaded()) {
        qWarning() << "SourceEstimateManager: Cannot start streaming — no source estimate loaded";
        return;
    }

    // Create controller on first use
    if (!m_rtController) {
        m_rtController = std::make_unique<RtSourceDataController>(this);
        connect(m_rtController.get(), &RtSourceDataController::newSmoothedDataAvailable,
                this, &SourceEstimateManager::realtimeColorsAvailable);
    }

    // Propagate interpolation matrices from the overlay
    m_rtController->setInterpolationMatrixLeft(m_overlay->interpolationMatLh());
    m_rtController->setInterpolationMatrixRight(m_overlay->interpolationMatRh());

    // Propagate current visualization parameters
    m_rtController->setColormapType(m_overlay->colormap());
    m_rtController->setThresholds(m_overlay->thresholdMin(),
                                   m_overlay->thresholdMid(),
                                   m_overlay->thresholdMax());
    m_rtController->setSFreq(1.0 / m_overlay->tstep());

    // Feed all STC time-points into the queue
    const int nTimePoints = m_overlay->numTimePoints();
    qDebug() << "SourceEstimateManager: Feeding" << nTimePoints << "time points into real-time queue";
    m_rtController->clearData();

    for (int t = 0; t < nTimePoints; ++t) {
        Eigen::VectorXd col = m_overlay->sourceDataColumn(t);
        if (col.size() > 0)
            m_rtController->addData(col);
    }

    m_rtController->setStreamingState(true);
    m_isStreaming = true;

    qDebug() << "SourceEstimateManager: Real-time streaming started";
}

//=============================================================================================================

void SourceEstimateManager::stopStreaming()
{
    if (!m_isStreaming) return;

    if (m_rtController)
        m_rtController->setStreamingState(false);

    m_isStreaming = false;
    qDebug() << "SourceEstimateManager: Real-time streaming stopped";
}

//=============================================================================================================

void SourceEstimateManager::pushData(const Eigen::VectorXd &data)
{
    if (m_rtController)
        m_rtController->addData(data);
}

//=============================================================================================================

void SourceEstimateManager::setInterval(int msec)
{
    if (m_rtController)
        m_rtController->setTimeInterval(msec);
}

//=============================================================================================================

void SourceEstimateManager::setLooping(bool enabled)
{
    if (m_rtController)
        m_rtController->setLoopState(enabled);
}

//=============================================================================================================

const SourceEstimateOverlay *SourceEstimateManager::overlay() const
{
    return m_overlay.get();
}
