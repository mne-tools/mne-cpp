//=============================================================================================================
/**
* @file     edge.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Edge class declaration.
*
*/

#ifndef EDGE_H
#define EDGE_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGraphicsItem>


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

class Node;
class Network;


//=============================================================================================================
/**
* An Edge is a graphical representation of a weight in the deep viewer
*
* @brief A weight representation
*/
class Edge : public QGraphicsItem
{
public:

    //=========================================================================================================
    /**
    * Constructs a Edge representing a weight
    *
    * @param [in] network       The network of which this edge is part of
    * @param [in] sourceNode    The Source Node of this Edge
    * @param [in] destNode      The Destination Node of this Edge
    */
    Edge(Network *network, Node *sourceNode, Node *destNode);

    //=========================================================================================================
    /**
    * Returns the source node
    *
    * @return the source node
    */
    Node *sourceNode() const;

    //=========================================================================================================
    /**
    * Returns the destination node
    *
    * @return the destination node
    */
    Node *destNode() const;

    //=========================================================================================================
    /**
    * Recalculates the edge coordinates after one of its connected nodes was moved
    */
    void adjust();

    //=========================================================================================================
    /**
    * Sets the weight to the node
    *
    * @param [in] weight    The weight to set
    */
    void setWeight(float weight);

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void updateLineWidth();
    void updateColor();

private:
    Node *m_pSource;            /**< The Edges Source Node */
    Node *m_pDest;              /**< The Edges Destination Node */

    QPointF m_sourcePoint;      /**< The calculated Source Point */
    QPointF m_destPoint;        /**< The calculated Destination Point */
    qreal   m_arrowSize;        /**< The Arrow Size */
    QColor  m_color;            /**< The Color derived from the weight */
    qreal   m_penWidth;         /**< The Edges width derived from the weight */

    float   m_weight;           /**< The Weight related to the edge */


    Network* m_pNetwork;        /**< The network this edge is part of*/
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // EDGE_H
