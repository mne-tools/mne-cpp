//=============================================================================================================
/**
 * @file     stcloadingworker.h
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
 * @brief    StcLoadingWorker class declaration.
 *
 */

#ifndef STCLOADINGWORKER_H
#define STCLOADINGWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <mne/mne_sourceestimate.h>
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <Eigen/Sparse>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainSurface;

//=============================================================================================================
/**
 * StcLoadingWorker performs source estimate loading and interpolation matrix computation
 * in a background thread to avoid blocking the UI.
 *
 * @brief Background worker that loads source estimate (STC) files and emits loaded data for visualization.
 */
class DISP3DRHISHARED_EXPORT StcLoadingWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor
     *
     * @param[in] lhPath         Path to left hemisphere STC file.
     * @param[in] rhPath         Path to right hemisphere STC file.
     * @param[in] lhSurface      Pointer to left hemisphere surface.
     * @param[in] rhSurface      Pointer to right hemisphere surface.
     * @param[in] cancelDist     Cancel distance for interpolation.
     * @param[in] parent         Parent object.
     */
    StcLoadingWorker(const QString &lhPath,
                     const QString &rhPath,
                     BrainSurface *lhSurface,
                     BrainSurface *rhSurface,
                     double cancelDist = 0.05,
                     QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~StcLoadingWorker();

    //=========================================================================================================
    /**
     * Get the loaded left hemisphere source estimate.
     *
     * @return The left hemisphere source estimate.
     */
    const MNELIB::MNESourceEstimate& stcLh() const { return m_stcLh; }

    //=========================================================================================================
    /**
     * Get the loaded right hemisphere source estimate.
     *
     * @return The right hemisphere source estimate.
     */
    const MNELIB::MNESourceEstimate& stcRh() const { return m_stcRh; }

    //=========================================================================================================
    /**
     * Get the computed left hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatLh() const { return m_interpMatLh; }

    //=========================================================================================================
    /**
     * Get the computed right hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatRh() const { return m_interpMatRh; }

    //=========================================================================================================
    /**
     * Check if left hemisphere data was loaded.
     *
     * @return True if LH data is loaded.
     */
    bool hasLh() const { return m_hasLh; }

    //=========================================================================================================
    /**
     * Check if right hemisphere data was loaded.
     *
     * @return True if RH data is loaded.
     */
    bool hasRh() const { return m_hasRh; }

public slots:
    //=========================================================================================================
    /**
     * Perform the loading and computation work.
     * This slot should be called after moving the worker to a thread.
     */
    void process();

signals:
    //=========================================================================================================
    /**
     * Emitted to report progress.
     *
     * @param[in] percent    Progress percentage (0-100).
     * @param[in] message    Status message describing current operation.
     */
    void progress(int percent, const QString &message);

    //=========================================================================================================
    /**
     * Emitted when loading is complete.
     *
     * @param[in] success    True if loading succeeded.
     */
    void finished(bool success);

    //=========================================================================================================
    /**
     * Emitted when an error occurs.
     *
     * @param[in] message    Error description.
     */
    void error(const QString &message);

private:
    QString m_lhPath;                                          /**< Path to LH STC file. */
    QString m_rhPath;                                          /**< Path to RH STC file. */
    BrainSurface *m_lhSurface;                                 /**< Pointer to LH surface. */
    BrainSurface *m_rhSurface;                                 /**< Pointer to RH surface. */
    double m_cancelDist;                                       /**< Cancel distance for interpolation. */

    MNELIB::MNESourceEstimate m_stcLh;                         /**< Loaded LH source estimate. */
    MNELIB::MNESourceEstimate m_stcRh;                         /**< Loaded RH source estimate. */
    bool m_hasLh = false;                                      /**< Flag indicating LH data loaded. */
    bool m_hasRh = false;                                      /**< Flag indicating RH data loaded. */

    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatLh;  /**< LH interpolation matrix. */
    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatRh;  /**< RH interpolation matrix. */
};

#endif // STCLOADINGWORKER_H
