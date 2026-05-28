//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     multiviewwindow.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February 2020
 * @brief    QDockWidget wrapper used as the building block of @ref MultiView layouts.
 *
 * MultiViewWindow exposes the small amount of state @ref MultiView
 * needs to manage its docks (the wrapped widget, a stable name, a
 * tabbed/floating flag) and applies the disp-library default dock
 * styling (title-bar buttons, movable / floatable features) so all
 * viewers in a plugin get a consistent look.
 */

#ifndef MULTIVIEWWINDOW_H
#define MULTIVIEWWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDockWidget>
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

//=============================================================================================================
/**
 * @brief QDockWidget wrapper used as the building block of @ref MultiView layouts.
 *
 * Stores the wrapped widget plus a stable name / floating flag so
 * @ref MultiView can re-arrange, retab and persist the dock layout
 * consistently.
 */
class DISPSHARED_EXPORT MultiViewWindow : public QDockWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<MultiViewWindow> SPtr;            /**< Shared pointer type for MultiViewWindow. */
    typedef QSharedPointer<const MultiViewWindow> ConstSPtr; /**< Const shared pointer type for MultiViewWindow. */

    //=========================================================================================================
    /**
     * Constructs an MultiViewWindow.
     */
    explicit MultiViewWindow(QWidget *parent = Q_NULLPTR,
                             Qt::WindowFlags flags = Qt::WindowFlags());

    //=========================================================================================================
    /**
     * Destructs an MultiViewWindow.
     */
    ~MultiViewWindow();

private:
};

}// NAMESPACE

#endif // MULTIVIEWWINDOW_H
