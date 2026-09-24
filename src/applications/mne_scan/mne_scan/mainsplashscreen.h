//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     mainsplashscreen.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the MainSplashScreen class.
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
