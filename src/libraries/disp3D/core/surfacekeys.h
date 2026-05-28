//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file surfacekeys.h
 * @since 2026
 * @date  March 2026
 * @brief String key constants for surfaces in the @ref DISP3DLIB scene map.
 *
 * BrainView and the scene controllers store all renderable surfaces
 * in a single @c QMap<QString, std::shared_ptr<BrainSurface>>. The
 * keys in this namespace are the canonical identifiers used by every
 * controller to look up, replace or remove a surface without race
 * conditions caused by typo'd magic strings.
 *
 * Keys cover the two cortical hemispheres (@c lh / @c rh on each of
 * pial, white, inflated, smoothwm), the three BEM compartments
 * (brain, inner-skull, outer-skull, scalp), MEG helmet / iso-contour
 * surfaces, and a few diagnostic primitives (origin marker,
 * orientation gizmo).
 */

#ifndef SURFACEKEYS_H
#define SURFACEKEYS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QLatin1String>
#include <QHash>
#include <QString>
#include <QMatrix4x4>
#include <Eigen/Core>

//=============================================================================================================
/**
 * @namespace SURFACEKEYS
 * @brief     Centralised string constants for surface map keys.
 *
 * Every surface stored in BrainView's `m_surfaces` map is identified by a
 * string key formed from one of these prefixes plus an object-specific
 * suffix.  Using these constants instead of raw string literals ensures
 * consistency and enables compile-time typo detection.
 */
namespace SURFACEKEYS
{
    // ── Hemisphere prefixes ────────────────────────────────────────────
    inline constexpr QLatin1String kLhPrefix       {"lh_"};
    inline constexpr QLatin1String kRhPrefix       {"rh_"};

    // ── BEM ────────────────────────────────────────────────────────────
    inline constexpr QLatin1String kBemPrefix       {"bem_"};
    inline constexpr QLatin1String kBemHead         {"bem_head"};

    // ── Sensors ────────────────────────────────────────────────────────
    inline constexpr QLatin1String kSensPrefix      {"sens_"};
    inline constexpr QLatin1String kSensMegGrad     {"sens_meg_grad_"};
    inline constexpr QLatin1String kSensMegMag      {"sens_meg_mag_"};
    inline constexpr QLatin1String kSensMeg         {"sens_meg_"};
    inline constexpr QLatin1String kSensEeg         {"sens_eeg_"};
    inline constexpr QLatin1String kSensDig         {"sens_dig_"};
    inline constexpr QLatin1String kHelmet          {"sens_surface_meg"};

    // ── Digitiser ──────────────────────────────────────────────────────
    inline constexpr QLatin1String kDigPrefix       {"dig_"};
    inline constexpr QLatin1String kDigCardinal     {"dig_cardinal"};
    inline constexpr QLatin1String kDigHpi          {"dig_hpi"};
    inline constexpr QLatin1String kDigEeg          {"dig_eeg"};
    inline constexpr QLatin1String kDigExtra        {"dig_extra"};

    // ── Source space ───────────────────────────────────────────────────
    inline constexpr QLatin1String kSrcSpPrefix     {"srcsp_"};

    // ── Contour overlays ───────────────────────────────────────────────
    inline constexpr QLatin1String kContourMeg      {"sens_contour_meg"};
    inline constexpr QLatin1String kContourEeg      {"sens_contour_eeg"};

    // ── Inflated surface keys ──────────────────────────────────────────
    inline constexpr QLatin1String kLhInflated      {"lh_inflated"};
    inline constexpr QLatin1String kRhInflated      {"rh_inflated"};

//=============================================================================================================
/**
 * Map from user-facing sensor type name (e.g. "MEG/Grad") to the
 * visibility-profile object key (e.g. "sens_meg_grad").
 *
 * The map is built once on first call and cached.
 *
 * @param[in] uiType   UI-facing type string.
 * @return              Corresponding object key, or empty string if unknown.
 */
inline QString sensorTypeToObjectKey(const QString &uiType)
{
    static const QHash<QString, QString> map = {
        { QStringLiteral("MEG"),                QStringLiteral("sens_meg")         },
        { QStringLiteral("MEG/Grad"),           QStringLiteral("sens_meg_grad")    },
        { QStringLiteral("MEG/Mag"),            QStringLiteral("sens_meg_mag")     },
        { QStringLiteral("MEG Helmet"),         QStringLiteral("sens_meg_helmet")  },
        { QStringLiteral("EEG"),                QStringLiteral("sens_eeg")         },
        { QStringLiteral("Digitizer"),          QStringLiteral("dig")              },
        { QStringLiteral("Digitizer/Cardinal"), QStringLiteral("dig_cardinal")     },
        { QStringLiteral("Digitizer/HPI"),      QStringLiteral("dig_hpi")          },
        { QStringLiteral("Digitizer/EEG"),      QStringLiteral("dig_eeg")          },
        { QStringLiteral("Digitizer/Extra"),    QStringLiteral("dig_extra")        },
    };
    return map.value(uiType, QString());
}

//=============================================================================================================
/**
 * Map a parent-item display text (e.g. "MEG/Grad") to the surface-map key
 * prefix used when registering individual sensor surfaces.
 *
 * @param[in] parentText   Text from the parent QStandardItem.
 * @return                 Key prefix string.
 */
inline QString sensorParentToKeyPrefix(const QString &parentText)
{
    if (parentText.contains(QLatin1String("MEG/Grad")))  return QStringLiteral("sens_meg_grad_");
    if (parentText.contains(QLatin1String("MEG/Mag")))   return QStringLiteral("sens_meg_mag_");
    if (parentText.contains(QLatin1String("MEG")))       return QStringLiteral("sens_meg_");
    if (parentText.contains(QLatin1String("EEG")))       return QStringLiteral("sens_eeg_");
    if (parentText.contains(QLatin1String("Digitizer"))) return QStringLiteral("sens_dig_");
    return QStringLiteral("sens_");
}

//=============================================================================================================
/**
 * Convert an Eigen 4x4 matrix to a QMatrix4x4.
 *
 * This eliminates the duplicated nested for-loop that appears throughout
 * the codebase.
 *
 * @param[in] m   Eigen 4x4 float matrix.
 * @return        Equivalent QMatrix4x4.
 */
inline QMatrix4x4 toQMatrix4x4(const Eigen::Matrix4f &m)
{
    QMatrix4x4 q;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            q(r, c) = m(r, c);
    return q;
}

} // namespace SURFACEKEYS

#endif // SURFACEKEYS_H
