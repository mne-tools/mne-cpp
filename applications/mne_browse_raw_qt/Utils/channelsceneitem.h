//=============================================================================================================
/**
* @file     channelsceneitem.h
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
* @brief    Contains the declaration of the ChannelSceneItem class.
*
*/

#ifndef ChannelSceneItem_H
#define ChannelSceneItem_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsItem>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QStaticText>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* ChannelSceneItem...
*
* @brief The ChannelSceneItem class provides a new data structure for impedance values.
*/
class ChannelSceneItem : public QGraphicsItem
{

public:
    //=========================================================================================================
    /**
    * Constructs a ChannelSceneItem.
    */
    ChannelSceneItem(QString electrodeName, QPointF electrodePosition, QColor electrodeColor = Qt::blue);

    //=========================================================================================================
    /**
    * Sets the color of the electrode item.
    */
    void setColor(QColor electrodeColor);

    //=========================================================================================================
    /**
    * Returns the bounding rect of the electrode item. This rect describes the area which the item uses to plot in.
    */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
    * Reimplemented paint function.
    */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    //=========================================================================================================
    /**
    * Returns the electrode name.
    */
    QString getElectrodeName();

    //=========================================================================================================
    /**
    * Updates the electrodes position.
    */
    void setPosition(QPointF newPosition);

    //=========================================================================================================
    /**
    * Updates the electrodes position.
    */
    QPointF getPosition();

private:
    QString     m_sElectrodeName;           /**< Holds the electrode name.*/
    QPointF     m_qpElectrodePosition;      /**< Holds the electrode 2D position in the scene.*/
    QColor      m_cElectrodeColor;          /**< Holds the current electrode color.*/
    bool        m_bHighlight;               /**< Highlight item flag.*/
};

} // NAMESPACE MNEBrowseRawQt

#endif // ChannelSceneItem_H
