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
 * @brief    Channel-selection descriptor (@ref mne::viz::qt::SelectionItem): the channels picked in a sensor layout.
 *
 * Moved out of selectionsceneitem.h when SelectionSceneItem gave way to @ref mne::viz::SensorLayoutFigure.
 */

#ifndef SELECTIONITEM_H
#define SELECTIONITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/figures/average_layout_figure.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointF>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief The channels picked in a sensor layout: names, numbers, kinds, units and layout positions, one entry
 * per channel in each vector.
 *
 * @ref ChannelSelectionView emits it when the selection changes; the average layout and the analysis plugins
 * show only these channels.
 */
class VIZQTSHARED_EXPORT SelectionItem
{
public:
    std::vector<std::string> m_sViewsToApply; /**< Names of the views the selection applies to. */
    std::vector<std::string> m_sChannelName;  /**< The channel's name.*/
    std::vector<int> m_iChannelNumber;        /**< The channel number.*/
    std::vector<int> m_iChannelKind;          /**< The channel kind.*/
    std::vector<int> m_iChannelUnit;          /**< The channel unit.*/
    std::vector<QPointF> m_qpChannelPosition; /**< The channel's 2D position in the layout.*/
    bool m_bShowAll = false;                  /**< Whether to show all channels. */

    //=========================================================================================================
    /**
     * @return One channel of an average layout per entry, without averages.
     *
     * @since 3.0.0
     */
    std::vector<viz::AverageLayoutFigure::Channel> averageChannels() const
    {
        std::vector<viz::AverageLayoutFigure::Channel> channels;
        for (std::size_t i = 0; i < m_sChannelName.size(); ++i) {
            viz::AverageLayoutFigure::Channel channel;
            channel.name = m_sChannelName[i];
            channel.number = i < m_iChannelNumber.size() ? m_iChannelNumber[i] : -1;
            if (i < m_qpChannelPosition.size()) {
                channel.position = {m_qpChannelPosition[i].x(), m_qpChannelPosition[i].y()};
            }
            channel.kind = i < m_iChannelKind.size() ? m_iChannelKind[i] : 0;
            channel.unit = i < m_iChannelUnit.size() ? m_iChannelUnit[i] : 0;
            channels.push_back(std::move(channel));
        }
        return channels;
    }
};

} // namespace mne::viz::qt

#endif // SELECTIONITEM_H
