//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     apphandler.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2020
 * @brief     AppHandler class declaration.
 */

#ifndef MNEANONYMIZE_APPHANDLER_H
#define MNEANONYMIZE_APPHANDLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "mne_anonymize.h"
#include "settingscontrollercl.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QCoreApplication>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE {

//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Apphandler class serves as a factory class that generates heap located objects to main. This class generates
 * a QCoreApplication object and a Controller object.
 *
 * @brief Object factory class.
 */
class AppHandler
{

public:
    typedef QSharedPointer<AppHandler> SPtr;            /**< Shared pointer type for AppHandler. */
    typedef QSharedPointer<const AppHandler> ConstSPtr; /**< Const shared pointer type for AppHandler. */

    //=========================================================================================================
    /**
    * Constructs a AppHandler object.
    */
    AppHandler();

    //=========================================================================================================
    /**
     * @brief Creates a QApplication or a QCoreApplication according to user's preference for a command line or a
     * GUI application.
     *
     * @details Handles input arguments and searches for a "--no-gui" option. If found, this will create a
     *  QCoreApplication so that main can execute the appplication as a command line one. If not found, it creates a
     *  QApplication so that main can execute a GUI. Boolean member variable m_bGuiMode is updated here.
     *
     * @see QT Documentation
     * @see https://doc.qt.io/qt-5/qapplication.html#details
     *
     * @param[in] argc (argument count) number of arguments on the command line.
     * @param[in] argv (argument vector) an array of pointers to arrays of characters.
     *
     * @return Pointer to a QCoreApplication.
     */
    QCoreApplication* createApplication(int& argc, char * argv[]);

    //=========================================================================================================
    /**
     * @brief Creates a controller object according tu user's preference for a command line or a
     *  GUI application.
     *
     * @details Depending on the state of the bool member variable m_bGuiMode, the method creates a command line or a
     * GUI oriented controller object of the SettingsControllerCL or the SettingsControllerGui class.
     *
     * @see MNE-CPP Documentation
     * @see https://mne-cpp.github.io/
     *
     * @param[in] argc (argument count) number of arguments on the command line.
     * @param[in] argv (argument vector) an array of pointers to arrays of characters.
     *
     * @return Pointer to a SettingsControllerCl object.
     */
    SettingsControllerCl* createController(const QStringList& args);

protected:

private:

bool m_bGuiMode;  /**< GUI based app, or a command line one.*/

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_APPHANDLER_H
