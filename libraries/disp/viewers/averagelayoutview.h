//=============================================================================================================
/**
* @file     averagelayoutview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the AverageLayoutView Class.
*
*/

#ifndef AVERAGELAYOUTVIEW_H
#define AVERAGELAYOUTVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsView;
class QGraphicsItem;

namespace FIFFLIB {
    class FiffInfo;
}


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

class AverageScene;
class EvokedSetModel;
class ChannelInfoModel;


//=============================================================================================================
/**
* DECLARE CLASS AverageLayoutView
*
* @brief The AverageLayoutView class provides a widget for a 2D average layout
*/
class DISPSHARED_EXPORT AverageLayoutView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<AverageLayoutView> SPtr;              /**< Shared pointer type for AverageLayoutView. */
    typedef QSharedPointer<const AverageLayoutView> ConstSPtr;   /**< Const shared pointer type for AverageLayoutView. */

    //=========================================================================================================
    /**
    * Constructs a AverageLayoutView which is a child of parent.
    *
    * @param [in] parent    parent of widget
    */
    AverageLayoutView(QWidget *parent = 0,
                      Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Sets the channel info model.
    *
    * @param [in] pChannelInfoModel     The new channel info model.
    */
    void setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel);

    //=========================================================================================================
    /**
    * Sets the evoked set model.
    *
    * @param [in] pEvokedSetModel     The new evoked set model.
    */
    void setEvokedSetModel(QSharedPointer<EvokedSetModel> &pEvokedSetModel);

    //=========================================================================================================
    /**
    * Sets the background color of the scene.
    *
    * @param [in] backgroundColor     The new background color.
    */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
    * Returns the background color of the scene.
    *
    * @return     The current background color.
    */
    QColor getBackgroundColor();

    //=========================================================================================================
    /**
    * Renders a screenshot of the scene and saves it to the passed path. SVG and PNG supported.
    *
    * @param [in] fileName     The file name and path where to store the screenshot.
    */
    void takeScreenshot(const QString& fileName);

    //=========================================================================================================
    /**
    * Sets the scale map to scaleMap.
    *
    * @param [in] scaleMap map with all channel types and their current scaling value.
    */
    void setScaleMap(const QMap<qint32, float> &scaleMap);

    //=========================================================================================================
    /**
    * Get the current average colors
    *
    * @return Pointer to the current average colors.
    */
    QSharedPointer<QMap<QString, QColor> > getAverageColor() const;

    //=========================================================================================================
    /**
    * Get the current average activations
    *
    * @return Pointer to the current average activations.
    */
    QSharedPointer<QMap<QString, bool> > getAverageActivation() const;

    //=========================================================================================================
    /**
    * Set the average colors
    *
    * @param [in] qMapAverageColor      Pointer to the new average colors
    */
    void setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
    * Set the average activations
    *
    * @param [in] qMapAverageActivation      Pointer to the new average activations
    */
    void setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);

    //=========================================================================================================
    /**
    * call this whenever the external channel selection manager changes
    *
    * * @param [in] selectedChannelItems list of selected graphic items
    */
    void channelSelectionManagerChanged(const QList<QGraphicsItem *> &selectedChannelItems);

    //=========================================================================================================
    /**
    * call this function whenever the items' data needs to be updated
    */
    void updateData();

protected:
    QSharedPointer<AverageScene>                                m_pAverageScene;            /**< The pointer to the average scene. */
    QSharedPointer<DISPLIB::EvokedSetModel>                     m_pEvokedSetModel;          /**< The data model */
    QSharedPointer<DISPLIB::ChannelInfoModel>                   m_pChannelInfoModel;        /**< Channel info model. */

    QPointer<QGraphicsView>                                     m_pAverageLayoutView;       /**< View for 2D average layout scene */

    QSharedPointer<QMap<QString, QColor> >                      m_qMapAverageColor;         /**< Average colors. */
    QSharedPointer<QMap<QString, bool> >                        m_qMapAverageActivation;    /**< Average activation status. */
    QMap<qint32,float>                                          m_scaleMap;                 /**< The current scaling map. */
};

} // NAMESPACE

#endif // AVERAGELAYOUTVIEW_H
