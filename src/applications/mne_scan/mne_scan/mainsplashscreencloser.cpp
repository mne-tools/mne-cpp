//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     mainsplashscreencloser.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     May, 2021
 * @brief    Class responsible of hiding the splash screen.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainsplashscreencloser.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainsplashscreen.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

constexpr unsigned long defaultTimeToWait(3);

//=============================================================================================================

MainSplashScreenCloser::MainSplashScreenCloser(MainSplashScreen& splashScreen)
: MainSplashScreenCloser(splashScreen, defaultTimeToWait)
{ }

//=============================================================================================================

MainSplashScreenCloser::MainSplashScreenCloser(MainSplashScreen& splashScreen, unsigned long sleepTime)
: m_pSplashScreenToHide(splashScreen)
, m_iSecondsToSleep(sleepTime)
{
    connect(this, &MainSplashScreenCloser::closeSplashscreen,
            &m_pSplashScreenToHide, &QWidget::close);
}

//=============================================================================================================

MainSplashScreenCloser::~MainSplashScreenCloser()
{
  quit();
  #if QT_VERSION >= QT_VERSION_CHECK(5,2,0)
  requestInterruption();
  #endif
  wait();
}

//=============================================================================================================

void MainSplashScreenCloser::run()
{
    sleep(m_iSecondsToSleep);

    emit closeSplashscreen();
}

//=============================================================================================================
