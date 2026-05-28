//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file layoutscene.h
 * @since 2022
 * @date  October 2022
 * @brief Base QGraphicsScene providing pan, pinch-zoom and swipe gesture handling for the sensor-layout scenes.
 *
 * LayoutScene installs gesture recognisers on the active view and
 * translates pan / pinch / swipe events into the same view-zoom /
 * view-pan operations regardless of input device. @ref SelectionScene,
 * @ref AverageScene and @ref FilterPlotScene derive from it to
 * inherit the gesture-driven navigation behaviour.
 */

#ifndef LAYOUTSCENE_H
#define LAYOUTSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGestureEvent;
class QPanGesture;
class QPinchGesture;
class QSwipeGesture;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Base QGraphicsScene providing pan, pinch-zoom and swipe gesture handling for sensor-layout scenes.
 *
 * Installs gesture recognisers on the active view and translates
 * pan / pinch / swipe events into view-zoom / view-pan operations
 * regardless of input device.
 */
class DISPSHARED_EXPORT LayoutScene : public QGraphicsScene
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a LayoutScene.
     */
    LayoutScene(QGraphicsView* view,
                QObject *parent = 0);

protected:
    //=========================================================================================================
    /**
     * Reimplemented wheel event.
     */
    void wheelEvent(QGraphicsSceneWheelEvent* event);

    //=========================================================================================================
    /**
     * Reimplemented double mouse press event.
     */
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);

    //=========================================================================================================
    /**
     * Reimplemented mouse press event.
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);

    //=========================================================================================================
    /**
     * Reimplemented double mouse move event.
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);

    //=========================================================================================================
    /**
     * Reimplemented double mouse release event.
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

    //=========================================================================================================
    /**
     * Reimplemented key press event.
     */
    void keyPressEvent(QKeyEvent *keyEvent);

    //=========================================================================================================
    /**
     * Reimplemented key release event.
     */
    void keyReleaseEvent(QKeyEvent *keyEvent);

    //=========================================================================================================
    /**
     * reimplemented event function - intercepts touch gestures
     */
    bool event(QEvent *event);

    //=========================================================================================================
    /**
     * gestureEvent processes gesture events
     */
    bool gestureEvent(QGestureEvent *event);

    //=========================================================================================================
    /**
     * pinchTriggered processes pan gesture events
     */
    void panTriggered(QPanGesture*);

    //=========================================================================================================
    /**
     * pinchTriggered processes pinch gesture events
     */
    void pinchTriggered(QPinchGesture*);

    //=========================================================================================================
    /**
     * pinchTriggered processes swipe gesture events
     */
    void swipeTriggered(QSwipeGesture*);

    //=========================================================================================================
    /**
     * Installed event filter.
     *
     * @param[in] obj the qt object for which the event was intercpeted.
     * @param[in] event the current event.
     */
    bool eventFilter(QObject *object, QEvent *event);

    QGraphicsView*                  m_qvView;                       /**< The view which visualizes this scene.*/
    //QList<QGraphicsItem *>          m_selectedItems;                /**< The currently selected items during extended selection mode.*/

    bool                            m_bDragMode;                    /**< Flag whether the drag mode is activated.*/
    //bool                            m_bExtendedSelectionMode;       /**< Flag whether the extended selection mode.*/
    QPointF                         m_mousePressPosition;           /**< The current mouse press location.*/
};
} // NAMESPACE DISPLIB

#endif // LAYOUTSCENE_H
