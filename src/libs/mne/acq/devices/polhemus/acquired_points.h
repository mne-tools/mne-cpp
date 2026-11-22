//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acquired_points.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Shared digitised-point store for Polhemus digitizer sessions.
 *
 *           Plain model object passed by reference between the wizard,
 *           the 3-D view, and the FIFF exporter. Points are stored in
 *           head/sensor frame metres exactly as received from the
 *           Polhemus Fastrak. A change callback tells the owner when the
 *           points change; a Qt application turns it into a signal.
 */

#ifndef ACQ_ACQUIRED_POINTS_H
#define ACQ_ACQUIRED_POINTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_polhemus_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Logical category of a captured point.
 *
 * Maps onto the FIFF point-kind constants on export
 * (`FIFFV_POINT_CARDINAL` / `FIFFV_POINT_EEG` / `FIFFV_POINT_EXTRA`).
 */
enum class PointKind
{
    Fiducial = 0, ///< Anatomical landmark (NAS / LPA / RPA).
    Eeg,          ///< Named EEG electrode contact.
    HeadShape     ///< Free head-shape point (continuous mode).
};

//=============================================================================================================
/**
 * @brief Identifier for the three cardinal fiducials.
 *
 * Numerically aligned with the FIFF `ident` field used for cardinal points
 * (1 = LPA, 2 = NAS, 3 = RPA).
 */
enum class FiducialId
{
    LPA = 1,
    NAS = 2,
    RPA = 3
};

//=============================================================================================================
/**
 * @brief A single digitised point captured during the alignment session.
 *
 * @snippet ex_acq_polhemus/main.cpp acquired_points_usage
 */
struct ACQPOLHEMUSSHARED_EXPORT DigitizedPoint
{
    PointKind kind = PointKind::HeadShape;
    std::string label;                                  ///< Human label ("NAS", "Cz", "HSP-42", …).
    int ident_number = 0;                               ///< 1-based id used by FIFF on export.
    Eigen::Vector3f position = Eigen::Vector3f::Zero(); ///< Position in metres, sensor frame.
};

//=============================================================================================================
/**
 * @brief In-memory store of all points captured during a session.
 *
 * The wizard hands one of these to the 3-D view (for visualisation) and
 * later to the export step (for FIFF write). Single-threaded, no locking.
 * Every change calls the callback set with set_on_changed().
 *
 * @snippet ex_acq_polhemus/main.cpp acquired_points_usage
 */
class ACQPOLHEMUSSHARED_EXPORT AcquiredPoints
{
public:
    /**
     * Sets the function called after every change of the points or twin fiducials, e.g. to emit a Qt signal.
     *
     * @param[in] on_changed   The callback; empty for none.
     */
    void set_on_changed(std::function<void()> on_changed)
    {
        m_on_changed = std::move(on_changed);
    }

    const std::vector<DigitizedPoint>& points() const
    {
        return m_points;
    }

    /**
     * Append a new point and report the change.
     *
     * @param[in] p Captured point to append.
     */
    void append(const DigitizedPoint& p)
    {
        m_points.push_back(p);
        notify_changed();
    }

    /**
     * Remove the most recently captured point matching @p kind, if any.
     *
     * @param[in] kind Point kind whose most recent entry is removed.
     */
    void undo_last(PointKind kind)
    {
        for (auto it = m_points.rbegin(); it != m_points.rend(); ++it) {
            if (it->kind == kind) {
                m_points.erase(std::next(it).base());
                notify_changed();
                return;
            }
        }
    }

    /** Drop every captured point. */
    void clear()
    {
        if (m_points.empty())
            return;
        m_points.clear();
        notify_changed();
    }

    /** Convenience accessor: returns the captured fiducial position for
     *  @p id, or a zero vector if not yet captured.
     *
     * @param[in] id Fiducial to look up.
     *
     * @return Captured fiducial position, or a zero vector if not yet captured.
     */
    Eigen::Vector3f fiducial(FiducialId id) const
    {
        for (const auto& p : m_points) {
            if (p.kind == PointKind::Fiducial && p.ident_number == static_cast<int>(id)) {
                return p.position;
            }
        }
        return Eigen::Vector3f::Zero();
    }

    /**
     * True iff a fiducial of the given id has been captured.
     *
     * @param[in] id Fiducial to check.
     *
     * @return True if the fiducial has been captured, false otherwise.
     */
    bool has_fiducial(FiducialId id) const
    {
        for (const auto& p : m_points) {
            if (p.kind == PointKind::Fiducial && p.ident_number == static_cast<int>(id)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Remove a previously captured fiducial so it can be re-recorded.
     *
     * @param[in] id Fiducial whose captured points are all removed.
     */
    void remove_fiducial(FiducialId id)
    {
        std::erase_if(m_points, [id](const DigitizedPoint& p) { return p.kind == PointKind::Fiducial && p.ident_number == static_cast<int>(id); });
        notify_changed();
    }

    /**
     * True iff all three cardinal fiducials have been captured.
     *
     * @return True if NAS, LPA, and RPA have all been captured.
     */
    bool has_all_fiducials() const
    {
        return has_fiducial(FiducialId::NAS) && has_fiducial(FiducialId::LPA) && has_fiducial(FiducialId::RPA);
    }

    int count_of(PointKind kind) const
    {
        int n = 0;
        for (const auto& p : m_points)
            if (p.kind == kind)
                ++n;
        return n;
    }

    /**
     * Moves a point without reporting it; call notify_changed() after a batch of moves.
     *
     * @param[in] index   Index of the point in points().
     * @param[in] pos     New position in metres.
     */
    void set_position(int index, const Eigen::Vector3f& pos)
    {
        if (index >= 0 && index < static_cast<int>(m_points.size()))
            m_points[static_cast<std::size_t>(index)].position = pos;
    }

    /** Calls the change callback. */
    void notify_changed() const
    {
        if (m_on_changed)
            m_on_changed();
    }

    // ── Twin (BEM-space) fiducials — clicked on digital twin surface ──

    void set_twin_fiducial(FiducialId id, const Eigen::Vector3f& pos)
    {
        m_twin_fiducials[static_cast<int>(id)] = pos;
        notify_changed();
    }

    void clear_twin_fiducial(FiducialId id)
    {
        m_twin_fiducials.erase(static_cast<int>(id));
        notify_changed();
    }

    bool has_twin_fiducial(FiducialId id) const
    {
        return m_twin_fiducials.contains(static_cast<int>(id));
    }

    Eigen::Vector3f twin_fiducial(FiducialId id) const
    {
        const auto it = m_twin_fiducials.find(static_cast<int>(id));
        return it == m_twin_fiducials.end() ? Eigen::Vector3f::Zero() : it->second;
    }

    bool has_all_twin_fiducials() const
    {
        return has_twin_fiducial(FiducialId::NAS) && has_twin_fiducial(FiducialId::LPA) && has_twin_fiducial(FiducialId::RPA);
    }

private:
    std::vector<DigitizedPoint> m_points;
    std::map<int, Eigen::Vector3f> m_twin_fiducials; ///< BEM-space fiducials keyed by FiducialId
    std::function<void()> m_on_changed;
};

} // namespace mne

#endif // ACQ_ACQUIRED_POINTS_H
