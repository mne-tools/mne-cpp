//=============================================================================================================
/**
 * @file     sensorfieldmapper.h
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
 * @brief    SensorFieldMapper class declaration.
 *
 */

#ifndef SENSORFIELDMAPPER_H
#define SENSORFIELDMAPPER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include "core/viewstate.h"

#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>

#include <Eigen/Core>
#include <QMap>
#include <QVector>
#include <QSharedPointer>
#include <QString>
#include <memory>

class BrainSurface;

//=============================================================================================================
/**
 * Encapsulates sensor-to-surface field mapping for MEG and EEG sensors.
 *
 * This class owns the mapping state (evoked data, mapping matrices, channel
 * picks, contour prefixes, surface key references) and provides methods to
 * build mapping matrices and apply them to produce per-vertex field colours
 * and iso-contour meshes.
 *
 * The class operates on surface and contour surfaces stored in the caller's
 * surface map. It reads from / writes to those surfaces but does not own
 * them, keeping ownership in the BrainView.
 *
 * @brief Sensor-to-surface field mapper that interpolates MEG/EEG measurements onto cortical meshes and generates iso-contour overlays.
 */
class DISP3DRHISHARED_EXPORT SensorFieldMapper
{
public:
    //=========================================================================================================
    /**
     * Construct an empty (unloaded) mapper.
     */
    SensorFieldMapper() = default;

    //=========================================================================================================
    /**
     * Set the evoked data that supplies measurements and channel info.
     *
     * After calling this, @c isLoaded() returns @c true when non-empty.
     *
     * @param[in] evoked    The evoked dataset (contains info + data matrix).
     */
    void setEvoked(const FIFFLIB::FiffEvoked &evoked);

    //=========================================================================================================
    /**
     * @return @c true if evoked data has been loaded and is non-empty.
     */
    bool isLoaded() const { return m_loaded; }

    //=========================================================================================================
    /**
     * Set the active time point index into the evoked data matrix.
     *
     * @param[in] tp    Zero-based time index.
     */
    void setTimePoint(int tp) { m_timePoint = tp; }

    /**
     * @return Current time point index.
     */
    int timePoint() const { return m_timePoint; }

    //=========================================================================================================
    /**
     * Whether the MEG field should be mapped onto the head (BEM) surface
     * rather than the helmet surface.
     *
     * @param[in] onHead    @c true to map onto head, @c false for helmet.
     */
    void setMegFieldMapOnHead(bool onHead) { m_megOnHead = onHead; }

    bool megFieldMapOnHead() const { return m_megOnHead; }

    //=========================================================================================================
    /**
     * Set the colormap name used when painting field values.
     *
     * @param[in] name  Colormap identifier (e.g. "MNE", "hot", "RdBu_r").
     */
    void setColormap(const QString &name) { m_colormap = name; }

    const QString &colormap() const { return m_colormap; }

    //=========================================================================================================
    /**
     * @return Const reference to the loaded evoked data.
     */
    const FIFFLIB::FiffEvoked &evoked() const { return m_evoked; }

    //=========================================================================================================
    /**
     * Check whether the current mapping matrices can be reused for a new
     * evoked data set (i.e., same sensor configuration: same channels,
     * same SSP projectors, same bad channels).
     *
     * @param[in] newEvoked     The candidate new evoked data.
     * @return @c true if the existing mapping is compatible, @c false if rebuild is needed.
     */
    bool hasMappingFor(const FIFFLIB::FiffEvoked &newEvoked) const;

    /**
     * @return Const reference to the MEG channel pick indices.
     */
    const QVector<int> &megPick() const { return m_megPick; }

    /**
     * @return Const reference to the EEG channel pick indices.
     */
    const QVector<int> &eegPick() const { return m_eegPick; }

    /**
     * @return Shared pointer to the MEG mapping matrix (may be null).
     */
    QSharedPointer<Eigen::MatrixXf> megMapping() const { return m_megMapping; }

    /**
     * @return Shared pointer to the EEG mapping matrix (may be null).
     */
    QSharedPointer<Eigen::MatrixXf> eegMapping() const { return m_eegMapping; }

    //=========================================================================================================
    /**
     * Build the MEG/EEG sensor-to-surface mapping matrices.
     *
     * Needs the caller's surface map to locate the helmet and/or head
     * surface.  Also needs the head-to-MRI transformation and whether
     * sensor transforms should be applied.
     *
     * @param[in] surfaces          All surfaces keyed by name.
     * @param[in] headToMriTrans    Head → MRI coordinate transform.
     * @param[in] applySensorTrans  Whether to apply the sensor → MRI transform.
     * @return @c true on success (at least one mapping built).
     */
    bool buildMapping(const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                      const FIFFLIB::FiffCoordTrans &headToMriTrans,
                      bool applySensorTrans);

    //=========================================================================================================
    /**
     * Apply the precomputed mapping to the current time point.
     *
     * Paints per-vertex colours onto the target surfaces and optionally
     * generates iso-contour line meshes.
     *
     * @param[in,out] surfaces      Surface map (contour surfaces may be added).
     * @param[in]     singleView    The single-view SubView for visibility flags.
     * @param[in]     subViews      The four multi-view SubViews.
     */
    void apply(QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
               const SubView &singleView,
               const QVector<SubView> &subViews);

    //=========================================================================================================
    /**
     * @return The surface key used for the MEG mapping target.
     */
    const QString &megSurfaceKey() const { return m_megSurfaceKey; }

    /**
     * @return The surface key used for the EEG mapping target.
     */
    const QString &eegSurfaceKey() const { return m_eegSurfaceKey; }

    //=========================================================================================================
    // ── Static utilities ───────────────────────────────────────────────

    /**
     * Find a BEM head surface key in the given surface map.
     *
     * Looks for "bem_head" first, then any BEM surface whose tissue type
     * is @c BrainSurface::TissueSkin.
     */
    static QString findHeadSurfaceKey(const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces);

    /**
     * Find the MEG helmet surface key ("sens_surface_meg") if present.
     */
    static QString findHelmetSurfaceKey(const QMap<QString, std::shared_ptr<BrainSurface>> &surfaces);

    /**
     * Compute a "nice" iso-contour step given a value range and target number of ticks.
     */
    static float contourStep(float minVal, float maxVal, int targetTicks);

    /**
     * Fit a sphere to the digitization headshape points.
     *
     * Replicates MNE-Python's ``fit_sphere_to_headshape`` (linear least-
     * squares sphere fit).  Uses FIFFV_POINT_EXTRA digitization points by
     * default; falls back to EXTRA + EEG if fewer than 4 extra points
     * exist.  Nose/face points (z < 0 && y > 0) are excluded.
     *
     * @param[in]  info      FiffInfo containing the digitization points.
     * @param[out] radius    Fitted sphere radius (metres).  May be @c nullptr.
     * @return The sphere centre in head coordinates (metres).
     *         Returns (0, 0, 0.04) if the fit cannot be performed.
     */
    static Eigen::Vector3f fitSphereOrigin(const FIFFLIB::FiffInfo &info,
                                           float *radius = nullptr);

    /**
     * Compute the normalization range anchored at the peak-GFP time.
     *
     * For each modality the Global Field Power (GFP = sqrt(mean(V_i^2)))
     * is evaluated across all time samples and we pick the time of
     * maximum GFP.  vmax = max(|mapped|) is then computed at that peak
     * time, giving the colour-map range [-vmax, +vmax].
     *
     * This matches MNE-Python's plot_field behaviour, where the initial
     * view shows the evoked peak, so the colour scale is anchored to it.
     * The range stays fixed during time scrubbing until explicitly
     * recomputed (e.g. by switching to a new evoked data set).
     *
     * Must be called after building or reusing a mapping matrix with
     * new evoked data.  @c buildMapping() calls this automatically.
     */
    void computeNormRange();

private:
    //=========================================================================================================
    // ── Contour surface generation ─────────────────────────────────────

    /**
     * Generate or update iso-contour line meshes for a given surface.
     *
     * Three contour sets are generated per prefix: _neg, _zero, _pos.
     *
     * @param[in,out] surfaces  Surface map (contour meshes are created/updated here).
     * @param[in]     prefix    Key prefix for contour surfaces.
     * @param[in]     surface   The surface whose geometry is iso-contoured.
     * @param[in]     values    Per-vertex scalar values.
     * @param[in]     step      Iso-step distance.
     * @param[in]     visible   Whether contours should be shown.
     */
    void updateContourSurfaces(QMap<QString, std::shared_ptr<BrainSurface>> &surfaces,
                               const QString &prefix,
                               const BrainSurface &surface,
                               const QVector<float> &values,
                               float step,
                               bool visible);

    //=========================================================================================================
    // ── Internal data ──────────────────────────────────────────────────

    FIFFLIB::FiffEvoked m_evoked;
    bool   m_loaded    = false;
    int    m_timePoint = 0;
    bool   m_megOnHead = false;
    QString m_colormap = QStringLiteral("MNE");

    QString m_megSurfaceKey;
    QString m_eegSurfaceKey;
    QString m_megContourPrefix = QStringLiteral("sens_contour_meg");
    QString m_eegContourPrefix = QStringLiteral("sens_contour_eeg");

    QVector<int>              m_megPick;
    QVector<int>              m_eegPick;
    QVector<Eigen::Vector3f>  m_megPositions;
    QVector<Eigen::Vector3f>  m_eegPositions;
    QSharedPointer<Eigen::MatrixXf> m_megMapping;
    QSharedPointer<Eigen::MatrixXf> m_eegMapping;

    float m_megVmax = 0.0f;      /**< Colour-map normalisation: max |mapped| at peak-GFP time for MEG. */
    float m_eegVmax = 0.0f;      /**< Colour-map normalisation: max |mapped| at peak-GFP time for EEG. */
};

#endif // SENSORFIELDMAPPER_H
