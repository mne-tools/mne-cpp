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
 *           Plain Qt model object passed by reference between the wizard,
 *           the 3-D view, and the FIFF exporter. Points are stored in
 *           head/sensor frame metres exactly as received from the
 *           Polhemus Fastrak.
 */

#ifndef UTILS_ACQUIRED_POINTS_H
#define UTILS_ACQUIRED_POINTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Logical category of a captured point.
 *
 * Maps onto the FIFF point-kind constants on export
 * (`FIFFV_POINT_CARDINAL` / `FIFFV_POINT_EEG` / `FIFFV_POINT_EXTRA`).
 */
enum class PointKind {
    Fiducial = 0,    ///< Anatomical landmark (NAS / LPA / RPA).
    Eeg,             ///< Named EEG electrode contact.
    HeadShape        ///< Free head-shape point (continuous mode).
};

//=============================================================================================================
/**
 * @brief Identifier for the three cardinal fiducials.
 *
 * Numerically aligned with the FIFF `ident` field used for cardinal points
 * (1 = LPA, 2 = NAS, 3 = RPA).
 */
enum class FiducialId {
    LPA = 1,
    NAS = 2,
    RPA = 3
};

//=============================================================================================================
/**
 * @brief A single digitised point captured during the alignment session.
 */
struct UTILSSHARED_EXPORT DigitizedPoint
{
    PointKind   kind = PointKind::HeadShape;
    QString     label;            ///< Human label ("NAS", "Cz", "HSP-42", …).
    int         identNumber = 0;  ///< 1-based id used by FIFF on export.
    QVector3D   position;         ///< Position in metres, sensor frame.
};

//=============================================================================================================
/**
 * @brief In-memory store of all points captured during a session.
 *
 * The wizard hands one of these to the 3-D view (for visualisation) and
 * later to the export step (for FIFF write). Single-threaded, no locking.
 */
class UTILSSHARED_EXPORT AcquiredPoints : public QObject
{
    Q_OBJECT
public:
    explicit AcquiredPoints(QObject* parent = nullptr) : QObject(parent) {}

    const QVector<DigitizedPoint>& points() const { return m_points; }

    /** Append a new point and emit @ref pointsChanged. */
    void append(const DigitizedPoint& p) {
        m_points.append(p);
        emit pointsChanged();
    }

    /** Remove the most recently captured point matching @p kind, if any. */
    void undoLast(PointKind kind) {
        for (int i = m_points.size() - 1; i >= 0; --i) {
            if (m_points[i].kind == kind) {
                m_points.removeAt(i);
                emit pointsChanged();
                return;
            }
        }
    }

    /** Drop every captured point. */
    void clear() {
        if (m_points.isEmpty()) return;
        m_points.clear();
        emit pointsChanged();
    }

    /** Convenience accessor: returns the captured fiducial position for
     *  @p id, or a default-constructed @c QVector3D if not yet captured. */
    QVector3D fiducial(FiducialId id) const {
        for (const auto& p : m_points) {
            if (p.kind == PointKind::Fiducial && p.identNumber == static_cast<int>(id)) {
                return p.position;
            }
        }
        return {};
    }

    /** True iff a fiducial of the given id has been captured. */
    bool hasFiducial(FiducialId id) const {
        for (const auto& p : m_points) {
            if (p.kind == PointKind::Fiducial && p.identNumber == static_cast<int>(id)) {
                return true;
            }
        }
        return false;
    }

    /** Remove a previously captured fiducial so it can be re-recorded. */
    void removeFiducial(FiducialId id) {
        for (int i = m_points.size() - 1; i >= 0; --i) {
            if (m_points[i].kind == PointKind::Fiducial
                && m_points[i].identNumber == static_cast<int>(id)) {
                m_points.removeAt(i);
            }
        }
        emit pointsChanged();
    }

    /** True iff all three cardinal fiducials have been captured. */
    bool hasAllFiducials() const {
        return hasFiducial(FiducialId::NAS)
            && hasFiducial(FiducialId::LPA)
            && hasFiducial(FiducialId::RPA);
    }

    int countOf(PointKind kind) const {
        int n = 0;
        for (const auto& p : m_points) if (p.kind == kind) ++n;
        return n;
    }

    void setPosition(int index, const QVector3D& pos) {
        if (index >= 0 && index < m_points.size())
            m_points[index].position = pos;
    }

    void emitChanged() { emit pointsChanged(); }

    // ── Twin (BEM-space) fiducials — clicked on digital twin surface ──

    void setTwinFiducial(FiducialId id, const QVector3D& pos) {
        m_twinFiducials[static_cast<int>(id)] = pos;
        emit pointsChanged();
    }

    void clearTwinFiducial(FiducialId id) {
        m_twinFiducials.remove(static_cast<int>(id));
        emit pointsChanged();
    }

    bool hasTwinFiducial(FiducialId id) const {
        return m_twinFiducials.contains(static_cast<int>(id));
    }

    QVector3D twinFiducial(FiducialId id) const {
        return m_twinFiducials.value(static_cast<int>(id));
    }

    bool hasAllTwinFiducials() const {
        return hasTwinFiducial(FiducialId::NAS)
            && hasTwinFiducial(FiducialId::LPA)
            && hasTwinFiducial(FiducialId::RPA);
    }

signals:
    void pointsChanged();

private:
    QVector<DigitizedPoint> m_points;
    QMap<int, QVector3D> m_twinFiducials; ///< BEM-space fiducials keyed by FiducialId
};

} // namespace UTILSLIB

#endif // UTILS_ACQUIRED_POINTS_H
