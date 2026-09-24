//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsiimpedancescene.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the TMSIImpedanceScene class.
 */

#ifndef TMSIIMPEDANCESCENE_H
#define TMSIIMPEDANCESCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "tmsielectrodeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
/**
 * TMSIImpedanceScene...
 *
 * @brief The TMSIImpedanceScene class provides a reimplemented QGraphicsScene.
 */
class TMSIImpedanceScene : public QGraphicsScene
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSIImpedanceScene.
     */
    explicit TMSIImpedanceScene(QGraphicsView* view, QObject *parent = 0);

private:
    QPointF         m_mousePosition;                /**< Holds the mouse position.*/
    bool            m_bRightMouseKeyPressed;        /**< Whether the right mouse button was pressed.*/
    QGraphicsView*  m_qvView;                       /**< Holds the view which visualizes this scene.*/

    //=========================================================================================================
    /**
     * Reimplemented mouse press event handler.
     */
    void mousePressEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     * Reimplemented mouse move event handler.
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     * Reimplemented mouse release event handler.
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     * Updates position of all electrodes in the scene.
     */
    void scaleElectrodePositions(double scaleFactor);
};
} // NAMESPACE

#endif // TMSIIMPEDANCESCENE_H
