//=============================================================================================================
/**
 * @file     tmsiimpedancescene.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the TMSIImpedanceScene class.
 *
 */

#ifndef TMSIIMPEDANCESCENE_H
#define TMSIIMPEDANCESCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <tmsielectrodeitem.h>

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
