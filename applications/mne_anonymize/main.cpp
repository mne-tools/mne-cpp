//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 * @date     May, 2020
 * @since    0.1.0
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, Wayne Mead, John C. Mosher. All rights reserved.
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
 * @brief     Application for anonymizing patient and personal health information from a fiff file.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "apphandler.h"
#include <utils/generics/applicationlogger.h>

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
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);

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
