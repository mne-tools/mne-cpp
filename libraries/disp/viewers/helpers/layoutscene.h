//=============================================================================================================
/**
 * @file     layoutscene.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 *
 * @brief    Contains the declaration of the LayoutScene class.
 *
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
 * DECLARE CLASS LayoutScene
 *
 * @brief The LayoutScene class provides a reimplemented QGraphicsScene for 2D layout plotting. This class handles all the user interaction features (subclass in order to use).
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
