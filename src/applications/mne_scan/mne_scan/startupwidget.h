//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     startupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains declaration of StartUpWidget class.
 */

#ifndef STARTUPWIDGET_H
#define STARTUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QLabel;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
/**
 * DECLARE CLASS StartUpWidget
 *
 * @brief The StartUpWidget class provides the widget which is shown at start up in the central widget of the main application.
 */
class StartUpWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<StartUpWidget> SPtr;               /**< Shared pointer type for StartUpWidget. */
    typedef QSharedPointer<const StartUpWidget> ConstSPtr;    /**< Const shared pointer type for StartUpWidget. */

    //=========================================================================================================
    /**
     * Constructs a StartUpWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new StartUpWidget becomes a window. If parent is another widget, StartUpWidget becomes a child window inside parent. StartUpWidget is deleted when its parent is deleted.
     */
    StartUpWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the StartUpWidget.
     */
    ~StartUpWidget();

private:
    QLabel* m_pLabel_Info;      /**< Holds the start up widget label. */
};
}//NAMESPACE

#endif // STARTUPWIDGET_H
