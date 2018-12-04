//=============================================================================================================
/**
* @file     quickcontrolview.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the QuickControlView Class.
*
*/

#ifndef QUICKCONTROLVIEW_H
#define QUICKCONTROLVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include "helpers/draggableframelesswidget.h"
#include "butterflyview.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class QuickControlViewWidget;
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


//=============================================================================================================
/**
* DECLARE CLASS QuickControlView
*
* @brief The QuickControlView class provides a quick control view for scaling, filtering, projector and other control options.
*/
class DISPSHARED_EXPORT QuickControlView : public DISPLIB::DraggableFramelessWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<QuickControlView> SPtr;              /**< Shared pointer type for QuickControlView. */
    typedef QSharedPointer<const QuickControlView> ConstSPtr;   /**< Const shared pointer type for QuickControlView. */

    //=========================================================================================================
    /**
    * Constructs a QuickControlView which is a child of parent.
    *
    * @param [in] name          The name to be displayed on the minimize button.
    * @param [in] flags         The window flags.
    * @param [in] parent        The parent of widget.
    * @param [in] bDraggable    Flag specifying whether this widget is draggable.
    */
    QuickControlView(const QString& name = "",
                     Qt::WindowFlags flags = Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint,
                     QWidget *parent = Q_NULLPTR,
                     bool bDraggable = true);

    //=========================================================================================================
    /**
    * Destructs a QuickControlView
    */
    ~QuickControlView();

    //=========================================================================================================
    /**
    * Add a new group box to this Widget. Takes ownership of the passed widget.
    *
    * @param [in] pWidget           The widgets which will be put into the new group box.
    * @param [in] sGroupBoxName     The name of the new group box.
    */
    void addWidget(QWidget* pWidget);

    //=========================================================================================================
    /**
    * Add a new group box to this Widget. Takes ownership of the passed widget.
    * This function will store the shared pointer to a member list and deparent this list as soon as this class is destroyed.
    * This way the memory management stays with the QSharedPointer.
    *
    * @param [in] pWidget           The widgets which will be put into the new group box.
    * @param [in] sGroupBoxName     The name of the new group box.
    */
    void addGroupBox(QSharedPointer<QWidget> pWidget,
                     const QString& sGroupBoxName);

    //=========================================================================================================
    /**
    * Add a new group box to this Widget. Takes ownership of the passed widget.
    *
    * @param [in] pWidget           The widgets which will be put into the new group box.
    * @param [in] sGroupBoxName     The name of the new group box.
    */
    void addGroupBox(QWidget* pWidget,
                     const QString& sGroupBoxName);

    //=========================================================================================================
    /**
    * Add a new group box with tabs to this Widget. If the group box already exists, a new tab will be added to its QTabWidget.
    * This function will store the shared pointer to a member list and deparent this list as soon as this class is destroyed.
    * This way the memory management stays with the QSharedPointer.
    *
    * @param [in] pWidget           The widgets which will be put into the new group box.
    * @param [in] sGroupBoxName     The name of the new group box.
    * @param [in] sTabName          The name of the new tab.
    */
    void addGroupBoxWithTabs(QSharedPointer<QWidget> pWidget,
                             const QString& sGroupBoxName,
                             const QString& sTabName);

    //=========================================================================================================
    /**
    * Add a new group box with tabs to this Widget. If the group box already exists, a new tab will be added to its QTabWidget.
    * Takes ownership of the passed widget.
    *
    * @param [in] pWidget           The widgets which will be put into the new group box.
    * @param [in] sGroupBoxName     The name of the new group box.
    * @param [in] sTabName          The name of the new tab.
    */
    void addGroupBoxWithTabs(QWidget* pWidget,
                             const QString& sGroupBoxName,
                             const QString& sTabName);

    //=========================================================================================================
    /**
    * Sets the values of the opacity slider
    *
    * @param [in] opactiy       the new opacity value
    */
    void setOpacityValue(int opactiy);

    //=========================================================================================================
    /**
    * Get current opacity value.
    *
    * @return thecurrent set opacity value of this window.
    */
    int getOpacityValue();

    //=========================================================================================================
    /**
    * Sets the visibility of the hide/show and close button
    *
    * @param [in] bVisibility       the new visiblity state.
    */
    void setVisiblityHideOpacityClose(bool bVisibility);

protected:
    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed
    *
    * @param [in] value opacity value.
    */
    void onOpacityChange(qint32 value);

    //=========================================================================================================
    /**
    * Is called when the minimize or maximize button was pressed.
    *
    * @param [in] state toggle state.
    */
    void onToggleHideAll(bool state);

private:       
    QString                                             m_sName;                        /**< Name of the widget which uses this quick control. */
    Ui::QuickControlViewWidget*                         ui;                             /**< The generated UI file. */

    QList<QSharedPointer<QWidget> >                     m_lControlWidgets;              /**< The quick control view widgets. Note that these are managed elsewhere. */

signals:

};

} // NAMESPACE DISPLIB

#endif // QUICKCONTROLVIEW_H
