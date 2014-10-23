//=============================================================================================================
/**
* @file     sensoritem.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the SensorItem Class.
*
*/

#ifndef SENSORITEM_H
#define SENSORITEM_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QPainter>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//=============================================================================================================
/**
* DECLARE CLASS SensorItem
*
* @brief The SensorItem class represents a channel item, plottet at the graphics scene
*/
class SensorItem : public QGraphicsObject
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a SensorItem which is a child of parent.
    *
    * @param [in] dispChName    channel name to display
    * @param [in] chNumber      channel number to store
    * @param [in] coordinate    coordinates of the item
    * @param [in] channelColor  coordinates of the item
    * @param [in] parent        parent of item
    */
    SensorItem(const QString& dispChName, qint32 chNumber, const QPointF& coordinate, const QColor& channelColor, QGraphicsItem *parent = 0);

    //=========================================================================================================
    /**
    * Sets the color of the channel item.
    *
    * @param [in] channelColor  color to set
    */
    void setColor(const QColor& channelColor);

    //=========================================================================================================
    /**
    * The outer bounds of the item as a rectangle.
    *
    * @return the outer bounds of the item as a rectangle.
    */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
    * The shape of this item as a QPainterPath in local coordinates.
    *
    * @return the shape of this item as a QPainterPath in local coordinates.
    */
    QPainterPath shape() const;

    //=========================================================================================================
    /**
    * Paints the contents of an item in local coordinates.
    *
    * @param [in] painter   painter used to paint
    * @param [in] option    style options for the item
    * @param [in] widget    points to the widget that is being painted on
    */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    //=========================================================================================================
    /**
    * Returns the channel display name.
    *
    * @return the name to display
    */
    inline const QString& getDisplayChName() const;

    //=========================================================================================================
    /**
    * Returns the channel number.
    *
    * @return the channel number
    */
    inline qint32 getChNumber() const;

    //=========================================================================================================
    /**
    * Updates the channels position.
    *
    * @param [in] newPosition   new channel position
    */
    inline void setPosition(QPointF newPosition);

    //=========================================================================================================
    /**
    * Returns the channel position.
    *
    * @return position
    */
    inline QPointF getPosition();

    //=========================================================================================================
    /**
    * Returns the channel selection state
    *
    * @return whether channel is selected
    */
    inline bool isHighlighted() const;

    //=========================================================================================================
    /**
    * Set the selection state
    *
    * @param [in] highlight     the new selection state
    */
    inline void setHighlighted(bool highlight);


protected:
    //=========================================================================================================
    /**
    * Receive mouse press events for this item.
    *
    * @param [in] event     the mouse event
    */
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    //=========================================================================================================
    /**
    * Receive mouse move events for this item.
    *
    * @param [in] event     the mouse event
    */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    //=========================================================================================================
    /**
    * Receive mouse release events for this item.
    *
    * @param [in] event     the mouse event
    */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
    //=========================================================================================================
    /**
    * If item changed
    *
    * @param [in] item      the sensor item itself
    */
    void itemChanged(SensorItem* item);

private:
    QString m_sDisplayChName;   /**< channel name to display */
    qint32 m_iChNumber;         /**< channel number */
    QPointF m_qPointFCoord;     /**< item coordinates */
    QColor m_qColorChannel;     /**< The current channel color.*/
    bool m_bIsHighlighted;      /**< channel higlighting state */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& SensorItem::getDisplayChName() const
{
    return m_sDisplayChName;
}


//*************************************************************************************************************

inline qint32 SensorItem::getChNumber() const
{
    return m_iChNumber;
}


//*************************************************************************************************************

inline void SensorItem::setPosition(QPointF newPosition)
{
    m_qPointFCoord = newPosition;
}


//*************************************************************************************************************

inline QPointF SensorItem::getPosition()
{
    return m_qPointFCoord;
}


//*************************************************************************************************************

inline bool SensorItem::isHighlighted() const
{
    return m_bIsHighlighted;
}


//*************************************************************************************************************

inline void SensorItem::setHighlighted(bool highlight)
{
    m_bIsHighlighted = highlight;
    update();
}

} // NAMESPACE

#endif // SENSORITEM_H
