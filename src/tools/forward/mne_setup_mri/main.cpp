//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements the mne_setup_mri command-line application.
 *
 *           This tool sets up FreeSurfer MRI data for use with MNE software
 *           by creating the Neuromag directory structure and converting
 *           FreeSurfer MRI volumes (COR files or .mgz/.mgh) into COR.fif
 *           FIFF files.
 *
 *           Ported from the original MNE C tools by Matti Hamalainen:
 *             - mne_setup_mri (shell script, SVN $Id: mne_setup_mri 2970)
 *             - mne_make_cor_set (C program)
 *
 *           Cross-referenced with MNE-Python, which never ported this tool
 *           as modern MNE-Python reads .mgz files directly via nibabel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_setup_mri_settings.h"
#include "setupmri.h"

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;
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
 * The function main marks the entry point of the mne_setup_mri application.
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
    QCoreApplication::setApplicationName("mne_setup_mri");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNESetupMriSettings settings(&argc, argv);
    SetupMri setupMri(settings);

    return setupMri.run();
}
