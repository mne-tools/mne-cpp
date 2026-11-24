//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     2006
 * @version  dev
 * @brief    Implements the mne_show_fiff application.
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_show_fiff_settings.h"
#include "mne_fiff_contents.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;

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
int main(int argc, char* argv[])
{
    MNEShowFiffSettings settings(&argc, argv);
    if (!MNEFiffContents::show_fiff_contents(stdout, settings)) {
        mne::Log::critical("Cannot read {}", settings.inname);
        return 1;
    }

    return 0;
}
