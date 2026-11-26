//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     averagelayoutwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayoutWidget: the evoked responses of the channels at their places in a sensor layout.
 *
 * Replaces AverageScene and AverageSceneItem, which drew the same tiles as items of a QGraphicsScene.
 */

#ifndef AVERAGELAYOUTWIDGET_H
#define AVERAGELAYOUTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "sceneviewport.h"

#include <mne/viz/figures/average_layout.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QPointF>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

class SelectionItem;

//=============================================================================================================
/**
 * @brief Evoked responses, one tile per channel at its layout position, as MNE-Python's plot_evoked_topo.
 *
 * The widget shows views of data it does not own: each average of a channel points into a column-major evoked
 * matrix that must outlive the next repaint. Pressing a tile enlarges its name until the button is released.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT AverageLayoutWidget : public SceneViewport
{
    Q_OBJECT

public:
    /** One average of a channel: values data[0], data[stride], ..., data[(samples - 1) stride]. */
    struct Average
    {
        std::string comment;          /**< Name of the average, the key of the colour and activation maps. */
        const double* data = nullptr; /**< First value. */
        int samples = 0;              /**< Number of values. */
        int stride = 1;               /**< Distance between values, the channel count of a column-major matrix. */
    };

    /** A channel and its averages. */
    struct Channel
    {
        std::string name;              /**< Name, drawn above the tile. */
        int number = -1;               /**< Row in the evoked data, -1 if unknown. */
        QPointF position;              /**< Position in layout units. */
        int kind = 0;                  /**< Channel kind, which picks the scale. */
        int unit = 0;                  /**< Channel unit, which picks the scale. */
        bool bad = false;              /**< Bad channels are drawn faint. */
        int firstSample = 0;           /**< First sample relative to the stimulus, which places the stimulus line. */
        std::vector<Average> averages; /**< The averages to draw. */
    };

    //=========================================================================================================
    /**
     * Constructs an empty layout.
     *
     * @param[in] parent   Parent widget.
     */
    explicit AverageLayoutWidget(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * @param[in] selection   Channels with names, numbers, kinds, units and positions.
     *
     * @return One channel per entry of @p selection, without averages.
     */
    static std::vector<Channel> channelsFrom(const SelectionItem& selection);

    //=========================================================================================================
    /**
     * Sets the channels and refits the view if it is kept fitted.
     *
     * @param[in] channels   The channels.
     */
    void setChannels(std::vector<Channel> channels);

    //=========================================================================================================
    /**
     * @return The channels.
     */
    const std::vector<Channel>& channels() const;

    //=========================================================================================================
    /**
     * Gives access to the channels to change their data; call update() afterwards.
     *
     * @return The channels.
     */
    std::vector<Channel>& channels();

    //=========================================================================================================
    /**
     * @param[in] scaleMap   Value at the top edge of a tile, by channel kind or unit.
     */
    void setScaleMap(const std::map<int, float>& scaleMap);

    //=========================================================================================================
    /**
     * @param[in] activation   Whether each average, by comment, is drawn; averages missing here are.
     */
    void setAverageActivation(const std::map<std::string, bool>& activation);

    //=========================================================================================================
    /**
     * @param[in] colors   Colour of each average, by comment; averages missing here take the signal colour.
     */
    void setAverageColors(const std::map<std::string, QColor>& colors);

    //=========================================================================================================
    /**
     * @param[in] color   Colour of averages without their own colour.
     */
    void setSignalColor(const QColor& color);

    //=========================================================================================================
    /**
     * @return The colour of averages without their own colour.
     */
    QColor signalColor() const;

    //=========================================================================================================
    /**
     * @return The tiles as they are drawn.
     */
    viz::AverageLayout averageLayout() const;

protected:
    //=========================================================================================================
    /**
     * @return The area of the tiles.
     */
    viz::Rect sceneBounds() const override;

    //=========================================================================================================
    /**
     * Draws the tiles.
     *
     * @param[in, out] canvas   Canvas in scene coordinates.
     */
    void drawScene(viz::Canvas& canvas) const override;

    //=========================================================================================================
    /**
     * Enlarges the name of the tile under the pointer.
     *
     * @param[in] point       Scene coordinates of the press.
     * @param[in] modifiers   Keyboard modifiers (unused).
     *
     * @return Whether a tile was hit.
     */
    bool scenePressed(viz::Point point, Qt::KeyboardModifiers modifiers) override;

    //=========================================================================================================
    /**
     * Shrinks the enlarged name again.
     *
     * @param[in] point   Scene coordinates of the release (unused).
     */
    void sceneReleased(viz::Point point) override;

private:
    std::vector<Channel> m_channels;          /**< The channels. */
    std::map<int, float> m_scaleMap;          /**< Value at the top edge, by kind or unit. */
    std::map<std::string, bool> m_activation; /**< Whether each average is drawn. */
    std::map<std::string, QColor> m_colors;   /**< Colour of each average. */
    QColor m_signalColor;                     /**< Colour of averages without their own. */
    int m_pressedTile = -1;                   /**< Tile whose name is enlarged. */
};

} // namespace mne::viz::qt

#endif // AVERAGELAYOUTWIDGET_H
