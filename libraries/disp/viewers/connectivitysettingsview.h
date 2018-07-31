//=============================================================================================================
/**
* @file     connectivitysettingsview.h
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
* @brief    Declaration of the ConnectivitySettingsView Class.
*
*/

#ifndef CONNECTIVITYSETTINGSVIEW_H
#define CONNECTIVITYSETTINGSVIEW_H


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


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class ConnectivitySettingsViewWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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


//=============================================================================================================
/**
* DECLARE CLASS ConnectivitySettingsView
*
* @brief The ConnectivitySettingsView class provides a view to control settings for estiamting functional connectivity
*/
class DISPSHARED_EXPORT ConnectivitySettingsView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<ConnectivitySettingsView> SPtr;              /**< Shared pointer type for ConnectivitySettingsView. */
    typedef QSharedPointer<const ConnectivitySettingsView> ConstSPtr;   /**< Const shared pointer type for ConnectivitySettingsView. */

    //=========================================================================================================
    /**
    * Constructs a ConnectivitySettingsView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    ConnectivitySettingsView(QWidget *parent = 0,
                Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Destroys the ConnectivitySettingsView.
    */
    ~ConnectivitySettingsView();

protected:
    //=========================================================================================================
    /**
    * Slot called when the metric changed.
    *
    * @param [in] metric        The new metric
    */
    void onMetricChanged(const QString& metric);

    //=========================================================================================================
    /**
    * Slot called when the window type changed.
    *
    * @param [in] windowType        The new window type
    */
    void onWindowTypeChanged(const QString& windowType);

    Ui::ConnectivitySettingsViewWidget* ui;

signals:
    //=========================================================================================================
    /**
    * Emit signal whenever the connectivity metric changed
    *
    * @param [in] metric        The new metric
    */
    void connectivityMetricChanged(const QString& metric);

    //=========================================================================================================
    /**
    * Emit signal whenever the window type changed
    *
    * @param [in] windowType        The new window type
    */
    void windowTypeChanged(const QString& windowType);

};

} // NAMESPACE

#endif // CONNECTIVITYSETTINGSVIEW_H
