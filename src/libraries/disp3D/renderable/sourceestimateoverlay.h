//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimateoverlay.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Colour-mapped source-time-course overlay that interpolates STC activation onto a cortical mesh and uploads per-vertex RGBA.
 *
 * SourceEstimateOverlay loads a pair of @c .stc files (one per
 * hemisphere), holds the source-time-course matrix and the sparse
 * vertex interpolation matrix produced by @ref GeometryInfo /
 * @ref Interpolation, and for any given time index multiplies the
 * two to obtain a per-vertex activation vector.
 *
 * The vector is then mapped through a configurable colormap (Hot,
 * Jet, MNE) with adjustable normalisation thresholds (fmin, fmid,
 * fmax) and the resulting ABGR bytes are written into the
 * secondary colour slot of the target @ref BrainSurface &mdash; the
 * renderer simply switches @ref VisualizationMode to
 * @c ModeSourceEstimate and the cortex lights up without any
 * geometry change.
 */

#ifndef SOURCEESTIMATEOVERLAY_H
#define SOURCEESTIMATEOVERLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <inv/inv_source_estimate.h>
#include <QVector>
#include <QString>
#include <QSharedPointer>
#include <QHash>
#include <QPair>
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
class DISP3DSHARED_EXPORT SourceEstimateOverlay
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
    void setStcData(const INVLIB::InvSourceEstimate &stc, int hemi);

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

    INVLIB::InvSourceEstimate m_stcLh;      /**< Left hemisphere source estimate. */
    INVLIB::InvSourceEstimate m_stcRh;      /**< Right hemisphere source estimate. */
    bool m_hasLh = false;                    /**< Flag indicating LH data loaded. */
    bool m_hasRh = false;                    /**< Flag indicating RH data loaded. */

    QString m_colormap = "Hot";              /**< Current colormap name. */
    float m_threshMin = 0.0f;                /**< Minimum threshold. */
    float m_threshMid = 0.5f;                /**< Mid threshold. */
    float m_threshMax = 1.0f;                /**< Maximum threshold. */

    QSharedPointer<Eigen::SparseMatrix<float>> m_interpolationMatLh;  /**< LH interpolation matrix. */
    QSharedPointer<Eigen::SparseMatrix<float>> m_interpolationMatRh;  /**< RH interpolation matrix. */

    // ── Per-time-point color cache ─────────────────────────────────────
    // Keyed by (hemi, vertexCount). The inner hash maps timeIndex →
    // pre-computed packed-ABGR color buffer. Invalidated whenever the
    // colormap, thresholds or source data change. This makes scrubbing
    // and auto-loop playback effectively free after the first pass: only
    // the per-vertex color blend + GPU upload happens, never the sparse
    // interpolation matvec or the per-vertex colormap calculation.
    using ColorCacheKey = QPair<int, int>;                                  /**< (hemi, vertexCount). */
    using ColorCacheBucket = QHash<int, QVector<uint32_t>>;                 /**< timeIndex → ABGR colors. */
    mutable QHash<ColorCacheKey, ColorCacheBucket> m_colorCache;            /**< Lazy color cache. */
    void invalidateColorCache() { m_colorCache.clear(); }
};

#endif // SOURCEESTIMATEOVERLAY_H
