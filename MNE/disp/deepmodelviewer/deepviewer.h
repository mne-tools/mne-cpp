//=============================================================================================================
/**
* @file     deepviewer.h
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
* @brief    DeepViewer class declaration.
*
*/

#ifndef DEEPVIEWER_H
#define DEEPVIEWER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QGraphicsScene;
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
class Network;
class View;
class Controls;


//=============================================================================================================
/**
* Implementing the CNTK Network Viewer
*
* @brief The CNTK Network Viewer
*/
class DISPSHARED_EXPORT DeepViewer : public QWidget
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs the DeepViewer which is a child of parent
    *
    * @param[in] embeddedControl    Whether the Deep Viewer Control should be embedded, i.e., generated within the Viewer
    * @param[in] parent             The parent widget
    */
    DeepViewer(bool embeddedControl = true, QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Returns the view
    *
    * @return the view
    */
    View* getView() const;

    //=========================================================================================================
    /**
    * Returns the CNTK Network UI representation
    *
    * @return the view
    */
    Network* getNetwork() const;

    //=========================================================================================================
    /**
    * The CNTK model which should be represented by the viewer
    *
    * @param[in] model      The CNTK model which should be represented by the viewer
    */
    void setModel(const QSharedPointer<DEEPLIB::Deep>& model);

    //=========================================================================================================
    /**
    * Updates the view according to the current model set
    */
    void updateModel();

private:
    //=========================================================================================================
    /**
    * Initializes the GraphicsScene and setups connections
    */
    void initScene();

    //=========================================================================================================
    /**
    * Remove scene items
    */
    void removeSceneItems();

    //=========================================================================================================
    /**
    * Update scene items, i.e, attaches or removes items according to their weights
    */
    void updateSceneItems();

    //=========================================================================================================
    /**
    * Redraw scene - only the part which is within the view rectangle
    */
    void redrawScene();

private:
    View*               m_pView;    /**< The View Port */

    QGraphicsScene*     m_pScene;   /**< The Scene Containing the graphic item */

    Network*            m_pNetwork; /**< The CNTK visual network representation */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DEEPVIEWER_H
