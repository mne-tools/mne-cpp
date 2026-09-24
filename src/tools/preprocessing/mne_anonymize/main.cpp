//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2020
 * @brief     Application for anonymizing patient and personal health information from a fiff file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_anonymize.h"
#include "apphandler.h"
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QDebug>
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNEANONYMIZE {
    class SettingsControllerCl;
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the.
 * command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects.
 * are null-terminated strings, representing the arguments that were entered on the command line when the
 * program was started.
 *
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);

    QScopedPointer<MNEANONYMIZE::AppHandler> h(new MNEANONYMIZE::AppHandler);
    QScopedPointer<QCoreApplication> qtApp(h->createApplication(argc, argv));

    qtApp->setOrganizationName(APPLICATION_ORG);
    qtApp->setApplicationName(APPLICATION_NAME);
    qtApp->setApplicationVersion(APPLICATION_VERSION);

    QScopedPointer<MNEANONYMIZE::SettingsControllerCl> controller(h->createController(qtApp->arguments()));

    if(controller->run())
    {
        return 1;
    }

    return qtApp->exec();
}
