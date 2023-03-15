//=============================================================================================================
/**
 * @file     quickcontrolview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include "helpers/draggableframelesswidget.h"
#include "butterflyview.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class QuickControlViewWidget;
}

class QVBoxLayout;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

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
     * @param[in] name          The name to be displayed on the minimize button.
     * @param[in] flags         The window flags.
     * @param[in] parent        The parent of widget.
     * @param[in] bDraggable    Flag specifying whether this widget is draggable.
     */
    QuickControlView(const QString& sSettingsPath = "",
                     const QString& name = "",
                     Qt::WindowFlags flags = Qt::Window | Qt::WindowStaysOnTopHint,
                     QWidget *parent = Q_NULLPTR,
                     bool bDraggable = true);

    //=========================================================================================================
    /**
     * Destructs a QuickControlView
     */
    ~QuickControlView();

    //=========================================================================================================
    /**
     * Clear this widget by removing all of its child widgets.
     */
    void clear();

    QVBoxLayout* findTabWidgetLayout(const QString& sTabName);

    //=========================================================================================================
    /**
     * This convenience function adds control widgets to the QuickControlView based on their set objects names.
     * Takes ownership of the QWidgets.
     *
     * @param[in] lWidgets     The widget which are supposed to be added. The widgets will be categorized based on their.
     *                          object names: "widget_", "group_", "group_tab_".
     * @param[in] sTabName     The tab to which the widgets are supposed to be added to.
     * @param[in] bAddToEnd    Whether to add the new widgets to the end of the layout.
     *                          If true, the widgets will inserted in the beginning of the layout.
     */
    void addWidgets(const QList<QWidget*>& lWidgets,
                    const QString& sTabName,
                    bool bAddToEnd = false);

    //=========================================================================================================
    /**
     * Add a new group box to this Widget. Takes ownership of the passed widget. Takes ownership of the QWidget.
     *
     * @param[in] pWidget           The widget which is supposed to be added.
     * @param[in] sTabName          The tab to which the widget is supposed to be added to.
     * @param[in] bAddToEnd         Whether to add the new widgets to the end of the layout.
     *                               If true, the widgets will inserted in the beginning of the layout.
     */
    void addWidget(QWidget* pWidget,
                   const QString& sTabName,
                   bool bAddToEnd = false);

    //=========================================================================================================
    /**
     * Add a new group box to this Widget. Takes ownership of the passed widget. Takes ownership of the QWidget.
     *
     * @param[in] pWidget           The widgets which will be put into the new group box.
     * @param[in] sGroupBoxName     The name of the new group box.
     * @param[in] sTabName          The tab to which the group box is supposed to be added to.
     * @param[in] bAddToEnd         Whether to add the new widgets to the end of the layout.
     *                               If true, the widgets will inserted in the beginning of the layout.
     */
    void addGroupBox(QWidget* pWidget,
                     const QString& sGroupBoxName,
                     const QString& sTabName,
                     bool bAddToEnd = false);

    //=========================================================================================================
    /**
     * Add a new group box with tabs to this Widget. If the group box already exists, a new tab will be added to its QTabWidget.
     *  Takes ownership of the QWidget.
     *
     * @param[in] pWidget           The widgets which will be put into the new group box.
     * @param[in] sGroupBoxName     The name of the new group box.
     * @param[in] sTabNameGroupBox  The tab name inside the tab widget of the group box.
     * @param[in] sTabName          The tab to which the group box with the tab widget is supposed to be added to.
     * @param[in] bAddToEnd         Whether to add the new widgets to the end of the layout.
     *                               If true, the widgets will inserted in the beginning of the layout.
     */
    void addGroupBoxWithTabs(QWidget* pWidget,
                             const QString& sGroupBoxName,
                             const QString& sTabNameGroupBox,
                             const QString& sTabName,
                             bool bAddToEnd = false);

    //=========================================================================================================
    /**
     * Sets the values of the opacity slider. Choose value between 0.0 and 1.0.
     *
     * @param[in] opactiy       the new opacity value.
     */
    void setOpacityValue(int opactiy);

    //=========================================================================================================
    /**
     * Get current opacity value. Value between 0.0 and 1.0.
     *
     * @return the current set opacity value of this window.
     */
    int getOpacityValue();

    //=========================================================================================================
    /**
     * Sets the visibility of the hide/show and close button
     *
     * @param[in] bVisibility       the new visiblity state.
     */
    void setVisiblityHideOpacityClose(bool bVisibility);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

protected:
    //=========================================================================================================
    /**
     * Slot called when opacity slider was changed
     *
     * @param[in] value opacity value.
     */
    void onOpacityChange(qint32 value);

private:       
    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */
    QString                                             m_sName;                        /**< Name of the widget which uses this quick control. */
    Ui::QuickControlViewWidget*                         m_pUi;                          /**< The generated UI file. */

signals:
};
} // NAMESPACE DISPLIB

#endif // QUICKCONTROLVIEW_H
