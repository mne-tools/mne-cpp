//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     2004
 * @brief    Copy processing history block between FIFF files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Copy processing history block from one FIFF file to another.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"from"}, "Source FIFF file containing processing history.", "file", {}});
    parser.add_option({{"to"}, "Destination FIFF file (will be modified).", "file", {}});
    parser.process(argc, argv, "mne_copy_processing_history", PROGRAM_VERSION);

    const std::string fromFile = parser.value("from");
    const std::string toFile = parser.value("to");

    if (fromFile.empty()) {
        Log::critical("--from is required.");
        return 1;
    }
    if (toFile.empty()) {
        Log::critical("--to is required.");
        return 1;
    }

    if (!FiffStream::copy_processing_history(path_from_utf8(fromFile), path_from_utf8(toFile))) {
        Log::critical("Failed to copy processing history from {} to {}", fromFile, toFile);
        return 1;
    }

    Log::info("Successfully copied processing history from {} to {}", fromFile, toFile);
    return 0;
}
