//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     December, 2016
 * @version  dev
 * @brief    Implements the mne_show_fiff application.
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_show_fiff_settings.h"
#include "mne_fiff_exp_set.h"
#include <stdio.h>

#include <utils/generics/mne_logger.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;
using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_dipole_fit application.
 * By default, main has the storage class extern.
 *
 * @param [in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param [in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_show_fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNEShowFiffSettings settings(&argc,argv);
    MNEFiffExpSet expSet = MNEFiffExpSet::read_fiff_explanations(QCoreApplication::applicationDirPath()+"/resources/general/explanations/fiff_explanations.txt");
    expSet.show_fiff_contents(stdout, settings);

    return 0;
}
