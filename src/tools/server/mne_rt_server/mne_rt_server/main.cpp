//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Implements the main() application function.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return 0 once a client has closed the server.
 */
int main(int argc, char* argv[])
{
    // Command Line Parser
    mne::CommandLineParser parser("MNE Rt Server");
    parser.add_help_option();
    parser.add_option({{"f", "file"}, "File to stream.", "filePath", {}});
    parser.process(argc, argv, "mne_rt_server", MNE_CPP_VERSION);

    // Parse input file for mne_rt_server's FiffSimulator
    if (parser.is_set("file")) {
        const std::string t_sConfig = std::format("{}/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg", mne::utf8_from_path(mne::executable_dir()));
        const std::string t_sFile = parser.value("file");
        std::error_code t_error;

        if (!t_sFile.empty() && std::filesystem::exists(mne::path_from_utf8(t_sFile), t_error)) {
            std::ofstream file(mne::path_from_utf8(t_sConfig), std::ios::trunc);
            if (file) {
                file << "simFile = " << t_sFile;
                file.close();

                mne::Log::info("{}", std::format("[MNERtServer::main] Streaming file {}", t_sFile));
            } else {
                mne::Log::warning("{}", std::format("[MNERtServer::main] Could not open {}", t_sConfig));
            }
        } else {
            mne::Log::warning("[MNERtServer::main] Provided file does not exist. Falling back to default one.");

            std::ofstream file(mne::path_from_utf8(t_sConfig), std::ios::trunc);
            if (file) {
                file << "simFile = <pathTo>/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
                file.close();
            } else {
                mne::Log::warning("{}", std::format("[MNERtServer::main] Could not open {}", t_sConfig));
            }
        }
    }

    MNERTServer t_MneRtServer;
    t_MneRtServer.waitForClose();

    return 0;
}
