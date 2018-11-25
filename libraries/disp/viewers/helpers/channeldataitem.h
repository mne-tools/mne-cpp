//=============================================================================================================
/**
* @file     ChannelDataItem.h
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
* @brief    Contains the declaration of the ChannelDataItem class.
*
*/

#ifndef ChannelDataItem_H
#define ChannelDataItem_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsObject>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ChannelDataItem
*
* @brief The ChannelDataItem class provides a new data structure for visualizing averages in a 2D layout.
*/
class DISPSHARED_EXPORT ChannelDataItem : public QGraphicsObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a ChannelDataItem.
    */
    ChannelDataItem(int iChannelKind,
                    int iChannelUnit,
                    const QColor& color = Qt::black);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
    */
    QRectF boundingRect() const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void addData(const Eigen::RowVectorXd& data);

    QList<Eigen::RowVectorXd>                       m_data;

    QColor m_color;

    double          m_dScaleValue;

    int             m_iChannelKind;
    int             m_iChannelUnit;
    int             m_iIterator;

    QRectF                                          m_rectBoundingRect;         /**< The bounding rect. */

protected:
    //=========================================================================================================
    /**
    * Create a plot path and paint the data
    *
    * @param [in] painter The painter used to plot in this item.
    */
    void paintDataPath(QPainter *painter);

signals:    

};

} // NAMESPACE DISPLIB

#endif // ChannelDataItem_H
