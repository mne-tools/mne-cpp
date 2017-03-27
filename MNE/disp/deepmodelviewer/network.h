//=============================================================================================================
/**
* @file     network.h
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
* @brief    Network class declaration.
*
*/

#ifndef NETWORK_H
#define NETWORK_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGraphicsItem>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// CNTK INCLUDES
//=============================================================================================================

#include <CNTKLibrary.h>


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

class Node;
class Edge;


//=============================================================================================================
/**
* A CNTK Network representation ready for visualization
*
* @brief A CNTK Network representation
*/
class Network : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<Network> SPtr;               /**< Shared pointer type for Network. */
    typedef QSharedPointer<const Network> ConstSPtr;    /**< Const shared pointer type for Network. */

    //=========================================================================================================
    /**
    * Constructs an Network with parent object parent.
    *
    * @param [in] parent   The parent of the Network
    */
    Network(QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Constructs an Network with parent object parent.
    *
    * @param [in] model     The CNTK model
    * @param [in] parent    The parent of the Network
    */
    Network(CNTK::FunctionPtr model, QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Network destructor
    */
    virtual ~Network() {}

    //=========================================================================================================
    /**
    * Sets the CNTK Model v2
    *
    * @param [in] model     Model to set
    */
    void setModel(CNTK::FunctionPtr& model);

    QList<QList<Node *> > layerNodes() const;
    void setLayerNodes(const QList<QList<Node *> > &listLayerNodes);

    QList<QList<Edge *> > edges() const;
    void setEdges(const QList<QList<Edge *> > &listEdges);

    bool isSetup() const;

    inline Qt::PenStyle getPenStyle() const;
    void setSolidLine();
    void setDashLine();
    void setDotLine();

    void setWeightThreshold(int thr);
    inline float weightThreshold() const;

signals:
    void update_signal();

    void updateWeightThreshold_signal();

protected:
    void generateNetwork();

private:
    CNTK::FunctionPtr   m_pModel;   /**< The CNTK model v2 */

    Qt::PenStyle        m_penStyle; /**< Current weight pen style */

    float               m_weightThreshold;  /**< Threshold of weights to show [0.00, 1.00] */


    QList< QList<Node*> > m_listLayerNodes; /**< List containing layer-wise Nodes */
    QList< QList<Edge*> > m_listEdges;      /**< List containing between-layer-wise Edges */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

Qt::PenStyle Network::getPenStyle() const
{
    return m_penStyle;
}


//*************************************************************************************************************

float Network::weightThreshold() const
{
    return m_weightThreshold;
}


} // NAMESPACE

#endif // NETWORK_H
