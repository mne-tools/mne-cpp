//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements the mne_watershed_bem command-line application.
 *
 *           This tool creates BEM (Boundary Element Model) surfaces using
 *           FreeSurfer's mri_watershed algorithm. It produces brain, inner
 *           skull, outer skull, and outer skin (head) surfaces and writes
 *           the head surface as a FIFF BEM file.
 *
 *           Ported from the original MNE shell script by Matti Hamalainen:
 *             - mne_watershed_bem (shell script, SVN $Id: mne_watershed_bem 3391)
 *
 *           The C++ version orchestrates the FreeSurfer mri_watershed binary,
 *           then uses the mne-cpp libraries to read FreeSurfer surfaces and
 *           write BEM FIFF files, replacing the need for the original MNE C
 *           tools mne_convert_surface and mne_surf2bem.
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_watershed_bem().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_watershed_bem_settings.h"
#include "watershedbem.h"

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEWATERSHEDBEM;
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
 * The function main marks the entry point of the mne_watershed_bem application.
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
    QCoreApplication::setApplicationName("mne_watershed_bem");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNEWatershedBemSettings settings(&argc, argv);
    WatershedBem watershedBem(settings);

    return watershedBem.run();
}
