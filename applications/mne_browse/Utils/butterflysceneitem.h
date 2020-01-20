//=============================================================================================================
/**
 * @file     butterflysceneitem.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the ButterflySceneItem class.
 *
 */

#ifndef BUTTERFLYSCENEITEM_H
#define BUTTERFLYSCENEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <Eigen/Core>
#include <fiff/fiff.h>
#include "types.h"


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
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
 * ButterflySceneItem...
 *
 * @brief The ButterflySceneItem class provides a new data structure for visualizing averages in a 2D layout.
 */
class ButterflySceneItem : public QGraphicsItem
{

public:
    //=========================================================================================================
    /**
    * Constructs a ButterflySceneItem.
    */
    ButterflySceneItem(QString setName, int setKind = FIFFV_MEG_CH, int setUnit = FIFF_UNIT_T_M, const QList<QColor> &defaultColors = QList<QColor>());

    //=========================================================================================================
    /**
    * Returns the bounding rect of the electrode item. This rect describes the area which the item uses to plot in.
    */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
    * Reimplemented paint function.
    */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QString                 m_sSetName;                 /**< The set name.*/
    fiff_int_t              m_iSetKind;                 /**< The set kind which is to be plotted (MEG or EEG).*/
    fiff_int_t              m_iSetUnit;                 /**< The set unit. Used to determine whether mag or grad channels are to be plotted.*/
    const FiffInfo*         m_pFiffInfo;                /**< The fiff info.*/

    QList<QColor>           m_cAverageColors;           /**< The current average color.*/
    RowVectorPair           m_lAverageData;             /**< The channels average data which is to be plotted.*/
    QPair<int,int>          m_firstLastSample;          /**< The first and last sample.*/
    QMap<QString,double>    m_scaleMap;                 /**< Map with all channel types and their current scaling value.*/

protected:
    //=========================================================================================================
    /**
    * Create a plot path and paint the average data
    *
    * @param [in] painter The painter used to plot in this item.
    */
    void paintAveragePaths(QPainter *painter);
};

} // NAMESPACE MNEBROWSE

#endif // BUTTERFLYSCENEITEM_H
