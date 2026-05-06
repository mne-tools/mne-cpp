//=============================================================================================================
/**
 * @file     acquired_points.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Shared digitised-point store for the MNE Align app.
 *
 *           Plain Qt model object passed by reference between the wizard,
 *           the 3-D view, and the FIFF exporter. Points are stored in
 *           head/sensor frame metres exactly as received from the
 *           Polhemus FastTrak.
 */

#ifndef MNE_ALIGN_ACQUIRED_POINTS_H
#define MNE_ALIGN_ACQUIRED_POINTS_H
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE MNEALIGN
//=============================================================================================================

namespace MNEALIGN
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
struct DigitizedPoint
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
class AcquiredPoints : public QObject
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

} // namespace MNEALIGN

#endif // MNE_ALIGN_ACQUIRED_POINTS_H
