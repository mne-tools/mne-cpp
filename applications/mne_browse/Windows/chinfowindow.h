//=============================================================================================================
/**
 * @file     chinfowindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2014
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
 * @brief    Contains the declaration of the ChInfoWindow class.
 *
 */

#ifndef CHINFOWINDOW_H
#define CHINFOWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_chinfowindow.h"

#include <fiff/fiff.h>

#include <disp/viewers/helpers/channelinfomodel.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================


/**
 * DECLARE CLASS ChInfoWindow
 *
 * @brief The ChInfoWindow class provides a dock window for informations about every loaded channel.
 */
class ChInfoWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a ChInfoWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new ChInfoWindow becomes a window. If parent is another widget, ChInfoWindow becomes a child window inside parent. ChInfoWindow is deleted when its parent is deleted.
    */
    ChInfoWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ChInfoWindow.
    * All ChInfoWindow's children are deleted first. The application exits if ChInfoWindow is the main widget.
    */
    ~ChInfoWindow();

    //=========================================================================================================
    /**
    * Returns the ChannelInfoModel of this window
    */
    ChannelInfoModel::SPtr getDataModel();

private:
    //=========================================================================================================
    /**
    * Inits the model view controller pattern of this window.
    *
    */
    void initMVC();

    //=========================================================================================================
    /**
    * Inits all QTableViews of this window.
    *
    */
    void initTableViews();

    Ui::ChInfoWindow*   ui;                 /**< Pointer to the qt designer generated ui class.*/

    ChannelInfoModel::SPtr   m_pChannelInfoModel;     /**< The channel info model.*/
};

} // NAMESPACE MNEBROWSE

#endif // CHINFOWINDOW_H
