//=============================================================================================================
/**
* @file     butterflyscene.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     October, 2014
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
* @brief    Contains the declaration of the ButterflyScene class.
*
*/

#ifndef BUTTERFLYSCENE_H
#define BUTTERFLYSCENE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflysceneitem.h"

#include <disp/viewers/helpers/layoutscene.h>
#include <disp/viewers/helpers/selectionsceneitem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//=============================================================================================================
/**
* ButterflyScene...
*
* @brief The ButterflyScene class provides a reimplemented QGraphicsScene for 2D layout plotting.
*/
class ButterflyScene : public LayoutScene
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a ButterflyScene.
    */
    explicit ButterflyScene(QGraphicsView* view, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Sets the scale map to scaleMap.
    *
    * @param [in] scaleMap map with all channel types and their current scaling value.
    */
    void setScaleMap(const QMap<QString,double> &scaleMap);

    //=========================================================================================================
    /**
    * Repaints all items from the layout data in the scene.
    *
    *  @param [in] selectedChannelItems items which are to painted to the average scene
    */
    void repaintItems(const QList<QGraphicsItem*> &selectedChannelItems);

};

} // NAMESPACE

#endif // BUTTERFLYSCENE_H
