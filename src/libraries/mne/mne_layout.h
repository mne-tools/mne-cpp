//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_layout.h
 * @since March 2026
 * @brief 2D sensor layout (channel name -> [x,y,w,h]) used by topographic plots.
 *
 * @ref MNELIB::MNELayout mirrors the @c .lay files distributed with the
 * MNE tools and Neuromag systems. Each entry maps a channel name to a
 * rectangle in normalised plotting coordinates so 2D topomaps stay
 * consistent across viewers.
 */

#ifndef MNE_LAYOUT_H
#define MNE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_layout_port.h"

#include <QString>
#include <vector>

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Full plotter layout with viewports.
 *
 * Describes the visual arrangement of signal channels in a
 * plotter display, loaded from a layout file.
 */
class MNESHARED_EXPORT MNELayout
{
public:
    MNELayout() = default;
    ~MNELayout() = default;

    QString   name;              /**< Source file name this layout was loaded from. */
    float     xmin = 0;          /**< VDC left limit. */
    float     xmax = 0;          /**< VDC right limit. */
    float     ymin = 0;          /**< VDC bottom limit. */
    float     ymax = 0;          /**< VDC top limit. */
    float     cxmin = 0;         /**< Confined VDC left limit. */
    float     cxmax = 0;         /**< Confined VDC right limit. */
    float     cymin = 0;         /**< Confined VDC bottom limit. */
    float     cymax = 0;         /**< Confined VDC top limit. */
    std::vector<MNELayoutPort> ports; /**< Viewports. */
    Eigen::MatrixXi match;       /**< Channel-to-port matching matrix (nchan x nport). */

    /**
     * @brief Returns the number of channels (rows in match matrix).
     */
    int nmatch() const { return match.rows(); }
};

} // namespace MNELIB

#endif // MNE_LAYOUT_H
