//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2005
 * @brief    Prepare a BEM model for forward computation (compute BEM solution matrix).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fwd/fwd_bem_model.h>
#include <mne/mne/mne_surface.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/log.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <print>
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
    CommandLineParser parser("Compute the BEM solution matrix for forward modeling.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"bem"}, "Input BEM model FIFF file.", "file", ""});
    parser.add_option({{"sol"}, "Output solution FIFF file (default: derived from --bem).", "file", ""});
    parser.add_option({{"method"}, "BEM method: linear or constant.", "name", "linear"});

    parser.process(argc, argv, "mne_prepare_bem_model", PROGRAM_VERSION);

    const std::string bemFile = parser.value("bem");
    std::string solFile = parser.value("sol");
    const std::string methodStr = to_lower(parser.value("method"));

    if (bemFile.empty()) {
        Log::critical("--bem is required.");
        std::print("{}", parser.help_text("mne_prepare_bem_model"));
        return 1;
    }

    int bemMethod;
    if (methodStr == "linear")
        bemMethod = FWD_BEM_LINEAR_COLL;
    else if (methodStr == "constant")
        bemMethod = FWD_BEM_CONSTANT_COLL;
    else {
        Log::critical("Unknown method: {} (use 'linear' or 'constant')", methodStr);
        return 1;
    }

    // Derive solution filename  if not specified
    if (solFile.empty())
        solFile = utf8_from_path(FwdBemModel::fwd_bem_make_bem_sol_name(path_from_utf8(bemFile)));

    Log::info("BEM model file:    {}", bemFile);
    Log::info("BEM solution file: {}", solFile);
    Log::info("BEM method:        {}", FwdBemModel::fwd_bem_explain_method(bemMethod));

    // Determine how many layers we have
    // Try three-layer first, fall back to single-layer
    std::unique_ptr<FwdBemModel> bemModel;
    bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(path_from_utf8(bemFile));
    if (!bemModel) {
        Log::info("Not a three-layer model, trying single-layer (homogeneous)...");
        bemModel = FwdBemModel::fwd_bem_load_homog_surface(path_from_utf8(bemFile));
    }

    if (!bemModel) {
        Log::critical("Cannot load BEM surfaces from: {}", bemFile);
        return 1;
    }

    Log::info("Loaded {} BEM surface(s)", bemModel->nsurf);
    for (int k = 0; k < bemModel->nsurf; ++k) {
        Log::info("  Surface {}: {}  ({} vertices, {} triangles)", k + 1, FwdBemModel::fwd_bem_explain_surface(bemModel->surfs[k]->id), bemModel->surfs[k]->np, bemModel->surfs[k]->ntri);
    }

    // Compute the BEM solution
    Log::info("\nComputing BEM solution...");
    if (bemModel->fwd_bem_compute_solution(bemMethod) != 0) {
        Log::critical("BEM solution computation failed.");
        return 1;
    }
    Log::info("BEM solution computed successfully.");

    // Save the solution
    Log::info("Writing solution to: {}", solFile);

    if (bemModel->fwd_bem_save_model(path_from_utf8(solFile)) != 0) {
        Log::critical("Cannot write output file: {}", solFile);
        return 1;
    }

    Log::info("BEM solution saved.");
    return 0;
}
