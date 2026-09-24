//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements the mne_surf2bem command-line application.
 *
 *           This tool converts FreeSurfer surfaces and/or ASCII triangle files
 *           into BEM (Boundary Element Model) FIFF files. Supports multiple
 *           surface inputs with separate IDs and conductivities, optional
 *           vertex shifting along normals, surface reordering for proper BEM
 *           nesting, and topology checks.
 *
 *           Ported from the original MNE C tool by Matti Hamalainen:
 *             - mne_surf2bem (SVN $Id: mne_surf2bem.c 3351)
 *
 *           Cross-referenced with MNE-Python's mne.write_bem_surfaces().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_surf2bem_settings.h"
#include "surf2bem.h"

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_surf2bem application.
 * By default, main has the storage class extern.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_surf2bem");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNESurf2BemSettings settings(&argc, argv);
    if (settings.shouldExit())
        return settings.exitCode();

    Surf2Bem surf2bem(settings);

    return surf2bem.run();
}
