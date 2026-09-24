//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements the mne_flash_bem command-line application.
 *
 *           This tool creates BEM (Boundary Element Model) surfaces using
 *           multi-echo FLASH MRI sequences. It orchestrates FreeSurfer
 *           tools for DICOM conversion, parameter map fitting, flash volume
 *           synthesis/averaging, registration, and triangulation, then
 *           converts the resulting surfaces with proper coordinate transforms.
 *
 *           Ported from the original MNE shell script by Matti Hamalainen:
 *             - mne_flash_bem (shell script, SVN $Id: mne_flash_bem 3255)
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_flash_bem().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_flash_bem_settings.h"
#include "flashbem.h"

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEFLASHBEM;
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
 * The function main marks the entry point of the mne_flash_bem application.
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
    QCoreApplication::setApplicationName("mne_flash_bem");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNEFlashBemSettings settings(&argc, argv);
    FlashBem flashBem(settings);

    return flashBem.run();
}
