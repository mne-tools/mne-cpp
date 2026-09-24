//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsimpedancescene.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the EEGoSportsImpedanceScene class.
 */

#ifndef EEGOSPORTSIMPEDANCESCENE_H
#define EEGOSportsIMPEDANCESCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <eegosportselectrodeitem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
/**
 * EEGoSportsImpedanceScene...
 *
 * @brief The EEGoSportsImpedanceScene class provides a reimplemented QGraphicsScene.
 */
class EEGoSportsImpedanceScene : public QGraphicsScene
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     *  Constructs a EEGoSportsImpedanceScene.
     */
    explicit EEGoSportsImpedanceScene(QGraphicsView* view, QObject *parent = 0);

private:
    //=========================================================================================================
    /**
     *  Reimplemented mouse press event handler.
     */
    void mousePressEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     *  Reimplemented mouse move event handler.
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     *  Reimplemented mouse release event handler.
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    //=========================================================================================================
    /**
     *  Updates position of all electrodes in the scene.
     */
    void scaleElectrodePositions(double scaleFactor);

    QPointF         m_mousePosition;                /**< Holds the mouse position.*/
    bool            m_bRightMouseKeyPressed;        /**< Whether the right mouse button was pressed.*/
    QGraphicsView*  m_qvView;                       /**< Holds the view which visualizes this scene.*/
};
} // NAMESPACE

#endif // EEGOSPORTSIMPEDANCESCENE_H
