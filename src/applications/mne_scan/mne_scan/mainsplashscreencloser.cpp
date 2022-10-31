//=============================================================================================================
/**
 * @file     mainsplashscreenhider.cpp
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
 * @brief    Class responsible of hiding the splash screen.
 *
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
