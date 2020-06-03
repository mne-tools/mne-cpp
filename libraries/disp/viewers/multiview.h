//=============================================================================================================
/**
 * @file     multiview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    MultiView class declaration.
 *
 */

#ifndef MULTIVIEW_H
#define MULTIVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QSharedPointer>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class MultiViewWindow;

//=============================================================================================================
/**
 * @brief The MultiView class inherits from QMainWindow and provides a view which supports dock widgets.
 */
class DISPSHARED_EXPORT MultiView : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<MultiView> SPtr;            /**< Shared pointer type for MultiView. */
    typedef QSharedPointer<const MultiView> ConstSPtr; /**< Const shared pointer type for MultiView. */

    //=========================================================================================================
    /**
     * Constructs an MultiView.
     */
    explicit MultiView(const QString& sSettingsPath = "",
                       QWidget *parent = Q_NULLPTR,
                       Qt::WindowFlags flags = Qt::Widget);

    //=========================================================================================================
    /**
     * Destructs an MultiView.
     */
    ~MultiView();

    //=========================================================================================================
    /**
     * Adds a QWidget to the top docking area.
     *
     * @param[in] pWidget   The widget to be added.
     * @param[in] sName     The window title shown in the QSplitter.
     *
     * @return Returns a pointer to the added widget in form of a MultiViewWindow.
     */
    MultiViewWindow* addWidgetTop(QWidget* pWidget,
                                  const QString &sName);

    //=========================================================================================================
    /**
     * Adds a QWidget to the bottom docking area. Please note that all bottom dock widgets are tabbified by default.
     *
     * @param[in] pWidget   The widget to be added.
     * @param[in] sName     The window title shown in the QSplitter.
     *
     * @return Returns a pointer to the added widget in form of a MultiViewWindow.
     */
    MultiViewWindow* addWidgetBottom(QWidget* pWidget,
                                     const QString& sName);

    //=========================================================================================================
    /**
     * Saves geometry and state of GUI dock widgets that have given a name with setObjectName()
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Restores geometry and state as saved by saveSettings()
     */
    void loadSettings();

private:
    QList<MultiViewWindow *>    m_lDockWidgets;
    QString                     m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

signals:
    void dockLocationChanged(QWidget* pWidget);
};

}// NAMESPACE

#endif // MULTIVIEW_H
