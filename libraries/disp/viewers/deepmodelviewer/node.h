//=============================================================================================================
/**
 * @file     node.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Node class declaration.
 *
 */

#ifndef NODE_H
#define NODE_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGraphicsItem>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Edge;
class Network;


//=============================================================================================================
/**
 * A Node is a graphical representation of a neuron in the deep viewer
 *
 * @brief A neuron representation
 */
class Node : public QGraphicsItem
{
public:
    //=========================================================================================================
    /**
    * Constructs a Node representing a Neuron
    *
    * @param [in] network   The network  of which this node is part of
    */
    Node(Network *network);

    //=========================================================================================================
    /**
    * Adds a connected edge to the node
    *
    * @param [in] edge      A connected edge
    */
    void addEdge(Edge *edge);

    //=========================================================================================================
    /**
    * Returns a list of all connected edges
    *
    * @return A list of all connected edges
    */
    QList<Edge *> edges() const;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    //=========================================================================================================
    /**
    * This event handler is reimplemented to receive mouse press events for the widget.
    *
    * @param [in] event     Mouse press events for the widget
    */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    //=========================================================================================================
    /**
    * This event handler is reimplemented to receive mouse release events for the widget.
    *
    * @param [in] event     Mouse release events for the widget
    */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    float m_fDiameter;              /**< The diameter */
    float m_fRadius;                /**< Half of diameter - for drawing speed already calculated beforehand */

    QList<Edge *> m_qListEdges;     /**< The list of connected edges */
    Network* m_pNetwork;            /**< The network this node is part of */

    bool m_bIsAttached;             /**< If node is attached to a scene  TODO: Move this to Edge, Node basis class*/
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================



} // NAMESPACE

#endif // NODE_H
