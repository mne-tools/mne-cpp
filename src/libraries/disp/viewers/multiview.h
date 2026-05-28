//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multiview.h
 * @since February 2020
 * @brief QMainWindow container hosting an arbitrary number of @ref MultiViewWindow dock widgets.
 *
 * MultiView is the top-level layout primitive used by every MNE-Scan
 * plugin GUI: it inherits @c QMainWindow purely to gain the dock
 * infrastructure, lets callers add named views through
 * @c addWidgetTop / @c addWidgetBottom and persists / restores its
 * dock geometry via @c QSettings. Views are added as
 * @ref MultiViewWindow instances so the user can detach, retab and
 * stack them freely.
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
 * @brief QMainWindow container hosting an arbitrary number of @ref MultiViewWindow dock widgets.
 *
 * Inherits @c QMainWindow purely for the dock infrastructure; named
 * views are added with @c addWidgetTop / @c addWidgetBottom and
 * geometry is persisted via @c QSettings.
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
