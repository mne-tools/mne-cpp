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
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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
    * @param [in] size          size of the item
    * @param [in] parent        parent of item
    */
    SensorItem(const QString& dispChName, qint32 chNumber, const QPointF& coordinate, const QSizeF& size, QGraphicsItem *parent = 0);

    //=========================================================================================================
    /**
    * Constructs a SensorItem which is a child of parent.
    *
    * @param [in] dispChName    channel name to display
    * @param [in] chNumber      channel number to store
    * @param [in] coordinate    coordinates of the item
    * @param [in] size          size of the item
    * @param [in] parent        parent of item
    */
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);

    inline const QString& getDisplayChName() const;

    inline qint32 getChNumber() const;

    inline bool isSelected() const;

    inline void setSelected(bool selected);


protected:
//    void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
    void itemChanged(SensorItem* item);

private:
    QString m_sDisplayChName;
    qint32 m_iChNumber;
    QPointF m_qPointFCoord;
    QSizeF m_qSizeFDim;
    bool m_bIsSelected;
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

inline bool SensorItem::isSelected() const
{
    return m_bIsSelected;
}


//*************************************************************************************************************

inline void SensorItem::setSelected(bool selected)
{
    m_bIsSelected = selected;
    update();
}

#endif // SENSORITEM_H
