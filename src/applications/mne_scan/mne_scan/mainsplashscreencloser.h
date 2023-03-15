//=============================================================================================================
/**
 * @file     mainsplashscreencloser.h
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     May, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Class responsible for closing the splash screen.
 *
 */

#ifndef MAINSPLASHSCREENCLOSER_H
#define MAINSPLASHSCREENCLOSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QWeakPointer>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "mainsplashscreen.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
// MNESCAN FORWARD DECLARATIONS
//=============================================================================================================

/**
 * Provides a class for QThread to work on a separate thread, just to hide the splash screen window whenever
 * found convenient.
 */
class MainSplashScreenCloser : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<MainSplashScreenCloser> SPtr;               /**< Shared pointer type for MainSplashScreenHider. */
    typedef QSharedPointer<const MainSplashScreenCloser> ConstSPtr;    /**< Const shared pointer type for MainSplashScreenHider. */
    //=========================================================================================================
    MainSplashScreenCloser(MainSplashScreen& splashScreen);

    //=========================================================================================================
    MainSplashScreenCloser(MainSplashScreen& splashScreen, unsigned long sleepTime);

    //=========================================================================================================
    ~MainSplashScreenCloser();

signals:

    //=========================================================================================================
    /**
     * Notifies that splash window should be closed
     */
    void closeSplashscreen();

protected:
    //=========================================================================================================
    /**
     * Method to be run from another thread.
     */
    void run();

    MainSplashScreen& m_pSplashScreenToHide;       /**< Reference to the slpash screen to hide.*/
    unsigned long   m_iSecondsToSleep;             /**< Time to wait before hiding the splash window.*/
};

} // namespace MNESCAN
#endif // MAINSPLASHSCREENCLOSER_H
