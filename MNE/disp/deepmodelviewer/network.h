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
// FORWARD DECLARATIONS
//=============================================================================================================


QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace DEEPLIB
{
class Deep;
}


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
    * @param [in] deepModel     The CNTK Model Wrapper
    * @param [in] parent        The parent of the Network
    */
    Network(QSharedPointer<DEEPLIB::Deep>& deepModel, QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Network destructor
    */
    virtual ~Network() {}

    //=========================================================================================================
    /**
    * Sets the CNTK Model Wrapper
    *
    * @param [in] deepModel     CNTK Model Wrapper
    */
    void setModel(QSharedPointer<DEEPLIB::Deep>& deepModel);

    //=========================================================================================================
    /**
    * Returns the CNTK Model v2
    *
    * @return the current CNTK model v2
    */
    inline QSharedPointer<DEEPLIB::Deep> model() const;

    //=========================================================================================================
    /**
    * Returns the layer-wise lists of nodes
    *
    * @return layer-wise lists of nodes
    */
    QList<QList<Node *> > layerNodes() const;
    //=========================================================================================================
    /**
    * Sets the layer-wise lists of nodes
    *
    * @param [in] listLayerNodes   layer-wise lists of nodes
    */
    void setLayerNodes(const QList<QList<Node *> > &listLayerNodes);

    //=========================================================================================================
    /**
    * Returns the layer-connection-wise lists of edges, representing weights
    *
    * @return layer-connection-wise lists of edges
    */
    QList<QList<Edge *> > edges() const;
    //=========================================================================================================
    /**
    * Sets the layer-connection-wise lists of edges, representing weights
    *
    * @param [in] listEdges     layer-connection-wise lists of edges
    */
    void setEdges(const QList<QList<Edge *> > &listEdges);

    //=========================================================================================================
    /**
    * Returns whether Network UI representation was setup
    *
    * @return true if Network UI representation was setup, false otherwise
    */
    bool isSetup() const;

    //=========================================================================================================
    /**
    * Returns the current setup pen style
    *
    * @return the current pen style
    */
    inline Qt::PenStyle getPenStyle() const;
    //=========================================================================================================
    /**
    * Sets solid line as the current pen style
    */
    void setSolidLine();
    //=========================================================================================================
    /**
    * Sets dash line as the current pen style
    */
    void setDashLine();
    //=========================================================================================================
    /**
    * Sets dot line as the current pen style
    */
    void setDotLine();

    //=========================================================================================================
    /**
    * Sets the weight threshold, i.e., the threshold over which edges should be attached to the scene
    *
    * @param [in] thr   the weight threshold
    */
    void setWeightThreshold(int thr);
    //=========================================================================================================
    /**
    * Returns the weight threshold, i.e., the threshold over which edges should be attached to the scene
    *
    * @return the weight threshold
    */
    inline float weightThreshold() const;

    //=========================================================================================================
    /**
    * Sets the weight strength, i.e., the basic strength multiplier
    *
    * @param [in] strength   the weight strength
    */
    void setWeightStrength(int strength);
    //=========================================================================================================
    /**
    * Returns the weight strength, i.e., the basic strength multiplier for the edges pen width
    *
    * @return the weight strength
    */
    inline float weightStrength() const;

    //=========================================================================================================
    /**
    * Update the weights according to the current model
    */
    void updateWeights();

signals:
    //=========================================================================================================
    /**
    * Signal emitted when the CNTK network UI representation got updated
    */
    void update_signal();

    //=========================================================================================================
    /**
    * Signal emitted when the threshold weights got updated
    */
    void updateWeightThreshold_signal();

    //=========================================================================================================
    /**
    * Signal emitted when the weight strength got updated
    */
    void updateWeightStrength_signal();

protected:
    //=========================================================================================================
    /**
    * Generates the UI representation of the CNTK Network
    */
    void generateNetwork();

private:
    QSharedPointer<DEEPLIB::Deep>   m_pModel;    /**< CNTK Model Wrapper */

    Qt::PenStyle        m_penStyle;     /**< Current weight pen style */

    float               m_weightThreshold;  /**< Threshold of weights to show [0.00, 1.00] */
    float               m_weightStrength;   /**< The pen stroke size */

    QList< QList<Node*> > m_listLayerNodes; /**< List containing layer-wise Nodes */
    QList< QList<Edge*> > m_listEdges;      /**< List containing between-layer-wise Edges */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


QSharedPointer<DEEPLIB::Deep> Network::model() const
{
    return m_pModel;
}


//*************************************************************************************************************

Qt::PenStyle Network::getPenStyle() const
{
    return m_penStyle;
}


//*************************************************************************************************************

float Network::weightThreshold() const
{
    return m_weightThreshold;
}


//*************************************************************************************************************

float Network::weightStrength() const
{
    return m_weightStrength;
}


} // NAMESPACE

#endif // NETWORK_H
