//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     chinfowindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     November, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the ChInfoWindow class.
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

#include <memory>


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

    std::unique_ptr<Ui::ChInfoWindow> ui;               /**< Pointer to the qt designer generated ui class.*/

    ChannelInfoModel::SPtr   m_pChannelInfoModel;     /**< The channel info model.*/
};

} // NAMESPACE MNEBROWSE

#endif // CHINFOWINDOW_H
