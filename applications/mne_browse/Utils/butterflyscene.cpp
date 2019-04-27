//=============================================================================================================
/**
* @file     butterflyscene.cpp
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
* @brief    Definition of the ButterflyScene class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflyscene.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflyScene::ButterflyScene(QGraphicsView* view, QObject* parent)
: LayoutScene(view, parent)
{
}


//*************************************************************************************************************

void ButterflyScene::setScaleMap(const QMap<QString,double> &scaleMap)
{
    QList<QGraphicsItem*> itemList = this->items();

    QListIterator<QGraphicsItem*> i(itemList);
    while (i.hasNext()) {
        ButterflySceneItem* ButterflySceneItemTemp = static_cast<ButterflySceneItem*>(i.next());
        ButterflySceneItemTemp->m_scaleMap = scaleMap;
    }

    this->update();
}


//*************************************************************************************************************

void ButterflyScene::repaintItems(const QList<QGraphicsItem *> &selectedChannelItems)
{
    this->clear();

    QListIterator<QGraphicsItem*> i(selectedChannelItems);
    while (i.hasNext()) {
        SelectionSceneItem* SelectionSceneItemTemp = static_cast<SelectionSceneItem*>(i.next());
        ButterflySceneItem* ButterflySceneItemTemp = new ButterflySceneItem(SelectionSceneItemTemp->m_sChannelName,
                                                                          SelectionSceneItemTemp->m_iChannelKind);

        this->addItem(ButterflySceneItemTemp);
    }
}
