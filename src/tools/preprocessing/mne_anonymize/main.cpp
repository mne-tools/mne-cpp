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

#include "settingscontrollercl.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

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
 * @return 0 on success, 1 otherwise.
 */
int main(int argc, char* argv[])
{
    MNEANONYMIZE::SettingsControllerCl controller(std::vector<std::string>(argv, argv + argc));

    return controller.run() ? 1 : 0;
}
