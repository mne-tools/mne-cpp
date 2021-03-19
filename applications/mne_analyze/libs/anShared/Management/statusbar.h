//=============================================================================================================
/**
 * @file     statusbar.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief     StatusBar class declaration.
 *
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
    void enterEvent(QEvent* event);

    //=========================================================================================================
    /**
     * Receives event when status bar is no longer hovered over, hides hover widget
     *
     * @param[in] event    event of type QHoverEvent that holds mouse position.
     */
    void leaveEvent(QEvent* event);

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
