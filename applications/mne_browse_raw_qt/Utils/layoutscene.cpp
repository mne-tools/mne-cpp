//=============================================================================================================
/**
* @file     layoutscene.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the LayoutScene class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutscene.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LayoutScene::LayoutScene(QGraphicsView* view, QObject* parent, int sceneType)
: QGraphicsScene(parent)
, m_qvView(view)
, m_iSceneType(sceneType)
{

}


//*************************************************************************************************************

void LayoutScene::setNewLayout(QMap<QString,QVector<double>> layoutMap)
{
    m_layoutMap = layoutMap;

    //Redraw all items
    repaintItems();
}


//*************************************************************************************************************

void LayoutScene::hideItems(QStringList list)
{
    QList<QGraphicsItem *> itemList = this->items();

    switch(m_iSceneType) {
        case 0: //Channel selection plot scene
            for(int i = 0; i<itemList.size(); i++) {
                ChannelSceneItem* item = static_cast<ChannelSceneItem*>(itemList.at(i));

                if(!list.contains(item->getElectrodeName()))
                    item->hide();
                else
                    item->show();
            }

        break;

        case 1: //Average plot scene


        break;
    }
}


//*************************************************************************************************************

void LayoutScene::repaintItems()
{
    QMapIterator<QString,QVector<double>> i(m_layoutMap);

    switch(m_iSceneType) {
        case 0: //Channel selection plot scene
            this->clear();

            while (i.hasNext()) {
                i.next();
                ChannelSceneItem* ChannelSceneItemTemp = new ChannelSceneItem(i.key(), QPointF(i.value().at(0), i.value().at(1)));

                this->addItem(ChannelSceneItemTemp);
            }

        break;

        case 1: //Average plot scene
//            this->clear();

//            QMapIterator<QString,QVector<double>> i(map);
//            while (i.hasNext()) {
//                i.next();
//                AverageSceneItem* AverageSceneItemTemp = new AverageSceneItem(i.key(), QPointF(i.value().at(0), i.value().at(1)));

//                this->addItem(AverageSceneItemTemp);
//            }

        break;
    }
}


//*************************************************************************************************************

void LayoutScene::wheelEvent(QGraphicsSceneWheelEvent* event) {
    m_qvView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Scale the view / do the zoom
    double scaleFactor = 1.15;
    if(event->delta() > 0) {
        // Zoom in
        m_qvView->scale(scaleFactor, scaleFactor);
    } else {
        // Zooming out
        m_qvView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}


//*************************************************************************************************************

void LayoutScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* mouseEvent)
{
    m_qvView->fitInView(this->itemsBoundingRect(), Qt::KeepAspectRatio);
}




