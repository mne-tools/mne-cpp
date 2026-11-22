//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2015
 * @brief    Project the 3D sensor positions of a FIFF file onto a 2D channel layout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/string_utils.h>
#include <mne/channels/layoutloader.h>
#include <mne/channels/layoutmaker.h>
#include <mne/fiff/fiff.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * Reads the sensor positions of one coil type from a FIFF file, or the electrodes of an .elc file, and writes
 * their 2-D layout.
 *
 * Usage: ex_make_layout [--fileIn <fif|elc>] [--fileOut <lout>] [--coilType <type>] [--mirrorX 0|1] [--mirrorY 0|1]
 */
int main(int argc, char* argv[])
{
    // 3012 = FIFFV_COIL_VV_PLANAR_T1: the coil type, unlike the kind, tells sensor layers apart
    std::map<std::string, std::string, std::less<>> options{
        {"--fileIn", (std::filesystem::path(argv[0]).parent_path() / "../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif").string()},
        {"--fileOut", "makeLayout_default.lout"},
        {"--coilType", "3012"},
        {"--mirrorX", "0"},
        {"--mirrorY", "0"}};
    for (int i = 1; i < argc; ++i) {
        const auto option = options.find(std::string_view(argv[i]));
        if (option == options.end() || i + 1 == argc) {
            std::println(stderr, "Usage: {} [--fileIn <fif|elc>] [--fileOut <lout>] [--coilType <type>] [--mirrorX 0|1] [--mirrorY 0|1]", argv[0]);
            return EXIT_FAILURE;
        }
        option->second = argv[++i];
    }

    const std::filesystem::path input = path_from_utf8(options["--fileIn"]);
    const std::string extension = to_lower(input.extension().string());
    std::vector<Vector3d> positions;
    StringList names;

    if (extension == ".fif") {
        const FiffRawData raw(input);
        const int coilType = to_int(options["--coilType"]).value_or(3012);
        for (std::size_t i = 0; i < raw.info.ch_names.size(); ++i) {
            const auto& position = raw.info.chs[i].chpos;
            if (position.coil_type == coilType) {
                // z flipped so the projection looks onto the top of the head
                positions.emplace_back(position.r0[0], position.r0[1], -position.r0[2]);
                names.push_back(raw.info.ch_names[i]);
            }
        }
    } else if (extension == ".elc") {
        if (const auto cap = LayoutLoader::read_elc(input)) {
            positions = cap->positions_3d;
            names = cap->names;
        }
    } else {
        std::println(stderr, "Input file type not supported: {}", input.string());
        return EXIT_FAILURE;
    }
    std::println("Read {} sensor positions", positions.size());

    const LayoutOptions layoutOptions{.radius = 60.0,
                                      .box_width = 5.0,
                                      .box_height = 4.0,
                                      .mirror_x = options["--mirrorX"] == "1",
                                      .mirror_y = options["--mirrorY"] == "1"};
    const auto corners = LayoutMaker::make_layout(positions, layoutOptions);
    if (!corners || !LayoutMaker::write_lout(path_from_utf8(options["--fileOut"]), *corners, names, layoutOptions)) {
        std::println(stderr, "Could not make the layout");
        return EXIT_FAILURE;
    }
    std::println("Wrote {}", options["--fileOut"]);
    return EXIT_SUCCESS;
}
