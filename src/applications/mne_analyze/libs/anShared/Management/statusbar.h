//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     statusbar.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2018
 * @brief     StatusBar class declaration.
 */

#ifndef ANSHAREDLIB_STATUSBAR_H
#define ANSHAREDLIB_STATUSBAR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStatusBar>
#include <QLabel>
#include <QStack>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QLabel;
class QProgressBar;

namespace DISPLIB {
    class ProgressView;
}

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

class Communicator;
class Event;

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class ANSHAREDSHARED_EXPORT StatusBar : public QStatusBar
{
    Q_OBJECT

public:
    typedef QSharedPointer<StatusBar> SPtr;            /**< Shared pointer type for StatusBar. */
    typedef QSharedPointer<const StatusBar> ConstSPtr; /**< Const shared pointer type for StatusBar. */

    //=========================================================================================================
    /**
     * Constructs a StatusBar object.
     */
    StatusBar(QWidget *pParent = nullptr);

    //=========================================================================================================
    /**
     * Destructs a StatusBar object.
     */
    ~StatusBar();

    //=========================================================================================================
    /**
     * Sets a new timeout for messages in milliseconds.
     *
     * @param[in] iTimeout       New timeout.
     */
    void setMsgTimeout(int iTimeout);

private:
    //=========================================================================================================
    /**
     * This functions gets called when a new message is received from the event system.
     *
     * @param[in] pEvent        The received event.
     */
    void onNewMessageReceived(const QSharedPointer<ANSHAREDLIB::Event> pEvent);

    //=========================================================================================================
    /**
     * Receives event when status bar is hovered over, shows hover widget if there are background processes
     *
     * @param[in] event    event of type QHoverEvent that holds mouse position.
     */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent* event) override;
#else
    void enterEvent(QEnterEvent* event) override;
#endif

    //=========================================================================================================
    /**
     * Receives event when status bar is no longer hovered over, hides hover widget
     *
     * @param[in] event    event of type QHoverEvent that holds mouse position.
     */
    void leaveEvent(QEvent* event) override;

    ANSHAREDLIB::Communicator*          m_pCommunicator;            /**< Vector containing all plugins. */

    int                                 m_iMsgTimeout;              /**< Timeout of one message in milliseconds. */

    QStack<QString>                     m_LoadingStack;             /** Keeps the currently loading messages */

    QPointer<QWidget>                   m_pHoverWidget;             /** Widget for showing all current loading messageswhen hovering over the loading bar */
    QPointer<DISPLIB::ProgressView>     m_pProgressView;            /** Widget for showiung loading bar and loadoing message */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace ANSHAREDLIB

#endif // ANSHAREDLIB_STATUSBAR_H
