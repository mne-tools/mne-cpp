//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_layout_port.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    One viewport of an @ref MNELIB::MNELayout, i.e. a box in layout coordinates and the channels drawn in it.
 */

#ifndef MNE_LAYOUT_PORT_H
#define MNE_LAYOUT_PORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <QRectF>
#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Single viewport of a plotter layout (one line of an MNE-C @c .lout file).
 *
 * Channel names are stored in matching form: lower case, without spaces and without
 * anything after a dash (MNE-C @c mne_lout_clean_name), so "MEG 0113" and "meg0113"
 * select the same port and a CTF "MLC11-2622" matches "MLC11".
 *
 * @snippet ex_mne_api/main.cpp mne_layout_usage
 */
class MNESHARED_EXPORT MNELayoutPort
{
public:
    //=========================================================================================================
    /**
     * Returns @p name in matching form.
     *
     * @param[in] name   A channel name.
     *
     * @return The name in lower case, without spaces and cut at the first dash.
     */
    static QString cleanName(const QString& name);

    //=========================================================================================================
    /**
     * Returns whether @p channel (any spelling) is one of the port's channels.
     *
     * @param[in] channel   A channel name.
     *
     * @return True if the cleaned name equals one of the port's channel names.
     */
    bool contains(const QString& channel) const;

    //=========================================================================================================
    /**
     * Returns the viewport box.
     *
     * @return The box from (xmin, ymin) with the port's width and height.
     */
    QRectF rect() const
    {
        return QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
    }

    int portno = 0;       /**< Running number of this viewport. */
    bool invert = false;  /**< Draw the signal upside down (negative height in the file). */
    float xmin = 0;       /**< Viewport left bound. */
    float xmax = 0;       /**< Viewport right bound. */
    float ymin = 0;       /**< Viewport bottom bound. */
    float ymax = 0;       /**< Viewport top bound. */
    QString label;        /**< Channel names as written in the file (e.g. "MEG 0113"). */
    QStringList names;    /**< Channel names in matching form (see cleanName()). */
    bool matched = false; /**< Set by the last MNELayout::matchPorts call. */
};

} // namespace MNELIB

#endif // MNE_LAYOUT_PORT_H
