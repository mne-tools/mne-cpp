//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     selectionitem.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Channel-selection descriptor (@ref mne::disp::SelectionItem): the channels picked in a sensor layout.
 *
 * Moved out of selectionsceneitem.h when SelectionSceneItem gave way to @ref mne::disp::SensorLayoutWidget.
 */

#ifndef SELECTIONITEM_H
#define SELECTIONITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointF>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief The channels picked in a sensor layout: names, numbers, kinds, units and layout positions, one entry
 * per channel in each vector.
 *
 * @ref ChannelSelectionView emits it when the selection changes; @ref AverageLayoutView and the analysis plugins
 * show only these channels.
 */
class DISPSHARED_EXPORT SelectionItem
{
public:
    std::vector<std::string> m_sViewsToApply; /**< Names of the views the selection applies to. */
    std::vector<std::string> m_sChannelName;  /**< The channel's name.*/
    std::vector<int> m_iChannelNumber;        /**< The channel number.*/
    std::vector<int> m_iChannelKind;          /**< The channel kind.*/
    std::vector<int> m_iChannelUnit;          /**< The channel unit.*/
    std::vector<QPointF> m_qpChannelPosition; /**< The channel's 2D position in the layout.*/
    bool m_bShowAll = false;                  /**< Whether to show all channels. */
};

} // namespace mne::disp

#endif // SELECTIONITEM_H
