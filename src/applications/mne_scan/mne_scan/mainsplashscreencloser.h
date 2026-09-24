//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     mainsplashscreencloser.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     May, 2021
 * @brief    Class responsible for closing the splash screen.
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
