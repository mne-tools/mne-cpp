//=============================================================================================================
/**
 * @file     mainsplashscreen.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the MainSplashScreen class.
 *
 */

#ifndef MAINSPLASHSCREEN_H
#define MAINSPLASHSCREEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSplashScreen>
#include <QSharedPointer>

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

//=============================================================================================================
/**
 * DECLARE CLASS MainSplashScreen
 *
 * @brief The MainSplashScreen class provides the main application splash screen.
 */
class MainSplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    typedef QSharedPointer<MainSplashScreen> SPtr;               /**< Shared pointer type for MainSplashScreen. */
    typedef QSharedPointer<const MainSplashScreen> ConstSPtr;    /**< Const shared pointer type for MainSplashScreen. */

    //=========================================================================================================
    /**
     * Construct a splash screen that will display the pixmap.
     *
     * @param[in] pixmap is the background of the splash screen.
     */
    MainSplashScreen ();

    //=========================================================================================================
    /**
     * Construct a splash screen that will display the pixmap.
     *
     * @param[in] pixmap is the background of the splash screen.
     */
    MainSplashScreen (const QPixmap & pixmap);

    //=========================================================================================================
    /**
     * Construct a splash screen that will display the pixmap.
     *
     * @param[in] pixmap is the background of the splash screen.
     * @param[in] f There should be no need to set the widget flags, f, except perhaps Qt::WindowStaysOnTopHint.
     */
    MainSplashScreen (const QPixmap & pixmap, Qt::WindowFlags f);
    //=========================================================================================================
    /**
     * Destroys the MainSplashScreen.
     */
    virtual ~MainSplashScreen ();
};
}// NAMESPACE

#endif // MAINSPLASHSCREEN_H
