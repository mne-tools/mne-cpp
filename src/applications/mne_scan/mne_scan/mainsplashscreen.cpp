//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     mainsplashscreen.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the MainSplashScreen class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainsplashscreen.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainSplashScreen::MainSplashScreen ()
: MainSplashScreen(QPixmap())
{
}

//=============================================================================================================

MainSplashScreen::MainSplashScreen (const QPixmap & pixmap)
: MainSplashScreen(pixmap, Qt::Widget)
{
}

//=============================================================================================================

MainSplashScreen::MainSplashScreen (const QPixmap & pixmap, Qt::WindowFlags f)
: QSplashScreen(pixmap, f)
{
}

//=============================================================================================================

MainSplashScreen::~MainSplashScreen()
{
    //ToDo cleanup work
}
