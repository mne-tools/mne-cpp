//=============================================================================================================
/**
* @file     deepviewerwidget.h
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
* @brief    DeepViewerWidget class declaration.
*
*/

#ifndef DEEPVIEWERWIDGET_H
#define DEEPVIEWERWIDGET_H

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
class QGraphicsScene;
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
class Controls;


//=============================================================================================================
/**
* Implementing the CNTK Network Viewer
*
* @brief The CNTK Network Viewer
*/
class DISPSHARED_EXPORT DeepViewerWidget : public QWidget
{
    Q_OBJECT
public:
    DeepViewerWidget(CNTK::FunctionPtr model, QWidget *parent = Q_NULLPTR);

    DeepViewerWidget(CNTK::FunctionPtr model, Controls *controls, QWidget *parent = Q_NULLPTR);


private:
    void populateScene();

    CNTK::FunctionPtr m_pModel;  /**< The CNTK model v2 */

    QGraphicsScene* m_pScene;

    QList< QList<Node*> > m_listLayers;
    QList< QList<Edge*> > m_listEdges;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DEEPVIEWERWIDGET_H
