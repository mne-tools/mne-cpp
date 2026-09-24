//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements the mne_setup_forward_model command-line application.
 *
 *           This tool sets up the BEM (Boundary Element Model) for forward
 *           modeling. It reads triangulated surface files (inner skull,
 *           outer skull, scalp), creates a BEM geometry FIFF file with
 *           conductivity assignments, exports .pnt and .surf files for
 *           visualization, and optionally computes the BEM solution matrix.
 *
 *           Ported from the original MNE shell script by Matti Hamalainen:
 *             - mne_setup_forward_model (shell script, SVN $Id: mne_setup_forward_model 3282)
 *
 *           The original script called mne_surf2bem, mne_list_bem, and
 *           mne_prepare_bem_model as external tools. This C++ port uses
 *           mne-cpp library classes directly for all three steps.
 *
 *           Cross-referenced with MNE-Python's mne.make_bem_model() and
 *           mne.make_bem_solution().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_setup_forward_model_settings.h"
#include "setupforwardmodel.h"

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPFORWARDMODEL;
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
 * The function main marks the entry point of the mne_setup_forward_model application.
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
    QCoreApplication::setApplicationName("mne_setup_forward_model");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    MNESetupForwardModelSettings settings(&argc, argv);
    SetupForwardModel setupFwdModel(settings);

    return setupFwdModel.run();
}
