//=============================================================================================================
/**
 * @file     sourceestimateoverlay.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
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
 * @brief    SourceEstimateOverlay class declaration.
 *
 */

#ifndef SOURCEESTIMATEOVERLAY_H
#define SOURCEESTIMATEOVERLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <mne/mne_sourceestimate.h>
#include <QVector>
#include <QString>
#include <QSharedPointer>
#include <Eigen/Sparse>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainSurface;

//=============================================================================================================
/**
 * SourceEstimateOverlay manages source estimate data (.stc files) and applies
 * colormap visualization to brain surfaces.
 *
 * @brief Color-mapped source estimate overlay that interpolates activation values onto a cortical surface mesh.
 */
class DISP3DRHISHARED_EXPORT SourceEstimateOverlay
{
public:
    //=========================================================================================================
    /**
     * Constructor
     */
    SourceEstimateOverlay();

    //=========================================================================================================
    /**
     * Destructor
     */
    ~SourceEstimateOverlay();

    //=========================================================================================================
    /**
     * Load a source estimate file (.stc) for a hemisphere.
     *
     * @param[in] path       Path to the .stc file.
     * @param[in] hemi       Hemisphere index (0=lh, 1=rh).
     * @return True if successful.
     */
    bool loadStc(const QString &path, int hemi);

    //=========================================================================================================
    /**
     * Check if source estimate data is loaded.
     *
     * @return True if data is loaded for at least one hemisphere.
     */
    bool isLoaded() const;

    //=========================================================================================================
    /**
     * Apply source estimate colors to a brain surface at a given time index.
     *
     * @param[in] surface    Pointer to the brain surface.
     * @param[in] timeIndex  Time sample index.
     */
    void applyToSurface(BrainSurface *surface, int timeIndex);

    //=========================================================================================================
    /**
     * Compute interpolation matrix for a surface.
     * This spreads sparse source values to all surface vertices.
     *
     * @param[in] surface       Pointer to the brain surface.
     * @param[in] hemi          Hemisphere index (0=lh, 1=rh).
     * @param[in] cancelDist    Cancel distance for interpolation (default 0.05m).
     */
    void computeInterpolationMatrix(BrainSurface *surface, int hemi, double cancelDist = 0.05);

    //=========================================================================================================
    /**
     * Set the colormap to use for visualization.
     *
     * @param[in] name       Colormap name ("Hot", "Jet", "Viridis", "Cool", "RedBlue").
     */
    void setColormap(const QString &name);

    //=========================================================================================================
    /**
     * Get the current colormap name.
     *
     * @return The colormap name.
     */
    QString colormap() const { return m_colormap; }

    //=========================================================================================================
    /**
     * Set threshold values for normalization.
     * Values below min are transparent, values above max are clamped.
     *
     * @param[in] min        Minimum threshold.
     * @param[in] mid        Mid-point threshold (used for some colormaps).
     * @param[in] max        Maximum threshold.
     */
    void setThresholds(float min, float mid, float max);

    //=========================================================================================================
    /**
     * Get the current threshold values.
     */
    float thresholdMin() const { return m_threshMin; }
    float thresholdMid() const { return m_threshMid; }
    float thresholdMax() const { return m_threshMax; }

    //=========================================================================================================
    /**
     * Get the number of time points in the source estimate.
     *
     * @return Number of time samples.
     */
    int numTimePoints() const;

    //=========================================================================================================
    /**
     * Get the time value at a given index.
     *
     * @param[in] idx        Time sample index.
     * @return Time in seconds.
     */
    float timeAtIndex(int idx) const;

    //=========================================================================================================
    /**
     * Get the minimum time value.
     *
     * @return Time in seconds.
     */
    float tmin() const;

    //=========================================================================================================
    /**
     * Get the time step between samples.
     *
     * @return Time step in seconds.
     */
    float tstep() const;

    //=========================================================================================================
    /**
     * Get the data range (min/max values) across all time points for auto-thresholding.
     *
     * @param[out] minVal    Minimum data value.
     * @param[out] maxVal    Maximum data value.
     */
    void getDataRange(double &minVal, double &maxVal) const;

    //=========================================================================================================
    /**
     * Set the source estimate data directly (used by async loader).
     *
     * @param[in] stc        The source estimate data.
     * @param[in] hemi       Hemisphere index (0=lh, 1=rh).
     */
    void setStcData(const MNELIB::MNESourceEstimate &stc, int hemi);

    //=========================================================================================================
    /**
     * Set the interpolation matrix directly (used by async loader).
     *
     * @param[in] mat        The interpolation matrix.
     * @param[in] hemi       Hemisphere index (0=lh, 1=rh).
     */
    void setInterpolationMatrix(QSharedPointer<Eigen::SparseMatrix<float>> mat, int hemi);

    //=========================================================================================================
    /**
     * Update thresholds based on loaded data range.
     */
    void updateThresholdsFromData();

    //=========================================================================================================
    /**
     * Get the interpolation matrix for the left hemisphere.
     *
     * @return Shared pointer to the LH interpolation matrix (may be null).
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatLh() const { return m_interpolationMatLh; }

    //=========================================================================================================
    /**
     * Get the interpolation matrix for the right hemisphere.
     *
     * @return Shared pointer to the RH interpolation matrix (may be null).
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatRh() const { return m_interpolationMatRh; }

    //=========================================================================================================
    /**
     * Get a single column of concatenated source data (LH + RH) at a given time index.
     * Used by the real-time streaming controller to feed data into the queue.
     *
     * @param[in] timeIndex  Time sample index.
     * @return Concatenated source values vector (nSourcesLH + nSourcesRH).
     */
    Eigen::VectorXd sourceDataColumn(int timeIndex) const;

private:
    //=========================================================================================================
    /**
     * Convert a normalized value [0,1] to a color using the current colormap.
     *
     * @param[in] value      Normalized value in [0,1].
     * @param[in] alpha      Alpha value for the color.
     * @return Packed ABGR color.
     */
    uint32_t valueToColor(double value, uint8_t alpha = 255) const;

    MNELIB::MNESourceEstimate m_stcLh;      /**< Left hemisphere source estimate. */
    MNELIB::MNESourceEstimate m_stcRh;      /**< Right hemisphere source estimate. */
    bool m_hasLh = false;                    /**< Flag indicating LH data loaded. */
    bool m_hasRh = false;                    /**< Flag indicating RH data loaded. */

    QString m_colormap = "Hot";              /**< Current colormap name. */
    float m_threshMin = 0.0f;                /**< Minimum threshold. */
    float m_threshMid = 0.5f;                /**< Mid threshold. */
    float m_threshMax = 1.0f;                /**< Maximum threshold. */

    QSharedPointer<Eigen::SparseMatrix<float>> m_interpolationMatLh;  /**< LH interpolation matrix. */
    QSharedPointer<Eigen::SparseMatrix<float>> m_interpolationMatRh;  /**< RH interpolation matrix. */
};

#endif // SOURCEESTIMATEOVERLAY_H
