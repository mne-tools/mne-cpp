//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_layout.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    2-D plotter layout read from an MNE-C @c .lout file: viewports, channel matching and zoom.
 *
 * @ref MNELIB::MNELayout is the C++ counterpart of MNE-C's @c mneLayout
 * (@c mne_layout.c) and reads the same files as @c mne.channels.read_layout.
 * The first line holds the extent of the drawing area; every further line
 * describes one viewport: number, lower-left corner, width, height (negative
 * to invert the trace) and the channel names drawn in it, separated by colons.
 */

#ifndef MNE_LAYOUT_H
#define MNE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_layout_port.h"

#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Plotter layout with viewports, channel matching and zoom.
 *
 * @snippet ex_mne_api/main.cpp mne_layout_usage
 */
class MNESHARED_EXPORT MNELayout
{
public:
    //=========================================================================================================
    /**
     * Reads an MNE-C @c .lout file.
     *
     * @param[in] path   The layout file.
     *
     * @return The layout, or no value if the file cannot be read or a line is malformed.
     */
    static std::optional<MNELayout> read(const QString& path);

    //=========================================================================================================
    /**
     * Returns the lower-left corner of every channel's viewport, keyed by the name in the file.
     *
     * @return Channel name -> viewport origin; a port listing several channels contributes each of them.
     */
    QMap<QString, QPointF> channelPositions() const;

    //=========================================================================================================
    /**
     * Marks the visible ports that show @p channel (MNE-C @c mne_lout_match_ports).
     *
     * @param[in] channel   A channel name in any spelling.
     *
     * @return The number of ports marked.
     */
    int matchPorts(const QString& channel);

    //=========================================================================================================
    /**
     * Matches many channels at once (MNE-C @c mne_lout_match_ports_many) and fills matches().
     *
     * @param[in] channels   Channel names in any spelling.
     *
     * @return The number of port/channel pairs found.
     */
    int matchPorts(const QStringList& channels);

    //=========================================================================================================
    /**
     * Returns which port shows which channel after matchPorts(const QStringList&).
     *
     * @return Ports x channels, 1 where the port shows the channel.
     */
    const Eigen::MatrixXi& matches() const
    {
        return m_matches;
    }

    //=========================================================================================================
    /**
     * Returns the port whose interior contains a point.
     *
     * @param[in] point   A point in layout coordinates.
     *
     * @return The index into ports, or -1.
     */
    int portAt(const QPointF& point) const;

    //=========================================================================================================
    /**
     * Zooms to the ports inside @p area (MNE-C @c mne_lout_confine).
     *
     * The visible area becomes the bounding box of those ports plus a 1 % margin;
     * without any port inside it stays unchanged.
     *
     * @param[in] area   The requested area; corners may be given in either order.
     *
     * @return The number of ports inside the area.
     */
    int confine(const QRectF& area);

    //=========================================================================================================
    /**
     * Shows the whole layout again.
     */
    void resetConfine()
    {
        m_visible = m_extent;
    }

    //=========================================================================================================
    /**
     * Returns whether a port lies completely inside the visible area.
     *
     * @param[in] port   A port of this layout.
     *
     * @return True if the port is visible.
     */
    bool isVisible(const MNELayoutPort& port) const;

    //=========================================================================================================
    /**
     * Returns the drawing area given on the first line of the file.
     *
     * @return The extent in layout coordinates.
     */
    QRectF extent() const
    {
        return m_extent;
    }

    //=========================================================================================================
    /**
     * Returns the area currently shown (the extent unless confine() zoomed in).
     *
     * @return The visible area in layout coordinates.
     */
    QRectF visibleArea() const
    {
        return m_visible;
    }

    QString fileName;                 /**< File this layout was read from. */
    std::vector<MNELayoutPort> ports; /**< Viewports in file order. */

private:
    QRectF m_extent;           /**< Drawing area from the first line of the file. */
    QRectF m_visible;          /**< Currently shown area. */
    Eigen::MatrixXi m_matches; /**< Ports x channels of the last matchPorts(const QStringList&). */
};

} // namespace MNELIB

#endif // MNE_LAYOUT_H
