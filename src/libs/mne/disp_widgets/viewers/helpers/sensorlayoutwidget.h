//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sensorlayoutwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SensorLayoutWidget: the sensors of a 2-D layout to pick channels by click or rubber band.
 *
 * Replaces SelectionScene and SelectionSceneItem, which drew the same dots as items of a QGraphicsScene.
 */

#ifndef SENSORLAYOUTWIDGET_H
#define SENSORLAYOUTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_widgets_global.h"
#include "sceneviewport.h"

#include <mne/scene/sensor_layout.h>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

class SelectionItem;

//=============================================================================================================
/**
 * @brief The sensors of a layout as labelled dots; clicks and rubber bands select them.
 *
 * A click selects one sensor, Ctrl+click toggles it, a rubber band selects the sensors it touches and Ctrl adds
 * them to the selection. Only sensors of the current channel kind are shown; those outside the visible group are
 * drawn faint and cannot be selected.
 *
 * @since 3.0.0
 */
class DISPWIDGETSSHARED_EXPORT SensorLayoutWidget : public SceneViewport
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty layout.
     *
     * @param[in] parent   Parent widget.
     */
    explicit SensorLayoutWidget(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Shows the sensors of a layout, all shown and selectable and none selected; EEG sensors are those whose name
     * contains "EEG". Emits selectionChanged() if sensors were selected before.
     *
     * @param[in] layoutMap     Sensor name to position in layout units.
     * @param[in] badChannels   Names of the bad sensors.
     */
    void setLayout(const std::map<std::string, QPointF>& layoutMap, const std::vector<std::string>& badChannels);

    //=========================================================================================================
    /**
     * Shows only the sensors of one channel kind.
     *
     * @param[in] kind   Channel kind, FIFFV_MEG_CH or FIFFV_EEG_CH.
     */
    void setChannelKind(int kind);

    //=========================================================================================================
    /**
     * @return The channel kind shown.
     */
    int channelKind() const;

    //=========================================================================================================
    /**
     * Lets only the sensors in @p names be selected; the other sensors of the shown kind are drawn faint. Sensors
     * that are hidden or cannot be selected lose their selection, which emits selectionChanged().
     *
     * @param[in] names   Names of the selectable sensors.
     */
    void setSelectable(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Selects the sensors in @p names and deselects the others.
     *
     * @param[in] names   Names of the sensors to select.
     */
    void selectNames(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Rings the sensors in @p names.
     *
     * @param[in] names   Names of the sensors to highlight.
     */
    void highlightNames(const std::vector<std::string>& names);

    //=========================================================================================================
    /**
     * Removes all sensors.
     */
    void clear();

    //=========================================================================================================
    /**
     * @return The names of the selected sensors, in layout order.
     */
    std::vector<std::string> selectedNames() const;

    //=========================================================================================================
    /**
     * The selected sensors, or with no selection the shown ones, as a SelectionItem.
     *
     * @return Names, kinds, units and positions; the channel numbers are 0.
     */
    SelectionItem selection() const;

    //=========================================================================================================
    /**
     * @return The layout model.
     */
    const scene::SensorLayout& sensorLayout() const;

signals:
    //=========================================================================================================
    /**
     * Emitted when the user changes the selection with the mouse, or selectNames() changes it.
     */
    void selectionChanged();

protected:
    //=========================================================================================================
    /**
     * @return The area of the shown sensors.
     */
    scene::Rect sceneBounds() const override;

    //=========================================================================================================
    /**
     * Draws the sensors.
     *
     * @param[in, out] canvas   Canvas in scene coordinates.
     */
    void drawScene(scene::Canvas& canvas) const override;

    //=========================================================================================================
    /**
     * Selects or toggles the sensor under the pointer; a press beside the sensors clears the selection unless Ctrl
     * is held.
     *
     * @param[in] point       Scene coordinates of the press.
     * @param[in] modifiers   Keyboard modifiers.
     *
     * @return Whether a sensor was hit.
     */
    bool scenePressed(scene::Point point, Qt::KeyboardModifiers modifiers) override;

    //=========================================================================================================
    /**
     * Selects the sensors the band touches.
     *
     * @param[in] band        The band in scene coordinates.
     * @param[in] modifiers   Ctrl adds to the selection.
     */
    void sceneBandSelected(const scene::Rect& band, Qt::KeyboardModifiers modifiers) override;

    //=========================================================================================================
    /**
     * @return True: a left drag beside the sensors spans a rubber band.
     */
    bool bandSelectionEnabled() const override;

private:
    scene::SensorLayout m_layout; /**< The sensors and their selection. */
    int m_kind;                   /**< Channel kind shown. */
};

} // namespace mne::disp

#endif // SENSORLAYOUTWIDGET_H
