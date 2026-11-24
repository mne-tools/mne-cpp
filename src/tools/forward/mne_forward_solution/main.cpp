//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     2001
 * @brief    Implements the mne_forward_solution application.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fwd/compute_fwd/compute_fwd_settings.h>
#include <mne/fwd/compute_fwd/compute_fwd.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/base/log.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
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
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_forward_solution application.
 * By default, main has the storage class extern.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    CommandLineParser parser(
        "Compute the MEG/EEG forward solution.\n"
        "Port of the original MNE-C mne_forward_solution by Matti Hamalainen.");
    parser.add_help_option();
    parser.add_version_option();

    // Source & sensor options
    parser.add_option({{"src"}, "Source space file.", "file", ""});
    parser.add_option({{"meas"}, "MEG/EEG measurement file (sensor & electrode locations).", "file", ""});
    parser.add_option({{"fwd"}, "Output forward solution file.", "file", ""});

    // Transform options
    parser.add_option({{"mri"}, "MRI description file for head/MRI transform.", "file", ""});
    parser.add_option({{"trans"}, "Text file for head/MRI transform.", "file", ""});
    parser.add_option({{"notrans"}, "Head and MRI coordinate systems are identical.", "", ""});

    // BEM & sphere model options
    parser.add_option({{"bem"}, "BEM model file.", "file", ""});
    parser.add_option({{"origin"}, "Sphere model origin in head coordinates (x:y:z in mm).", "x:y:z", ""});
    parser.add_option({{"eegscalp"}, "Scale electrode locations to the scalp surface (sphere model).", "", ""});
    parser.add_option({{"eegmodels"}, "File of EEG sphere model specifications.", "file", ""});
    parser.add_option({{"eegmodel"}, "Name of the EEG sphere model to use (default: Default).", "name", ""});
    parser.add_option({{"eegrad"}, "Scalp radius for EEG sphere model in mm (default: 90.0).", "radius", ""});

    // Modality flags
    parser.add_option({{"meg"}, "Compute the MEG forward solution.", "", ""});
    parser.add_option({{"eeg"}, "Compute the EEG forward solution.", "", ""});
    parser.add_option({{"grad"}, "Compute the gradient of the field with respect to dipole coordinates.", "", ""});
    parser.add_option({{"fixed"}, "Calculate only for the source orientation given by surface normals.", "", ""});
    parser.add_option({{"accurate"}, "Use accurate coil definitions in MEG forward computation.", "", ""});
    parser.add_option({{"mricoord"}, "Do calculations in MRI coordinates instead of head coordinates.", "", ""});
    parser.add_option({{"all"}, "Calculate forward solution in all nodes instead of selected ones only.", "", ""});

    // Source space filtering
    parser.add_option({{"label"}, "FsLabel file to select sources (can be specified multiple times).", "file", ""});
    parser.add_option({{"mindist"}, "Minimum distance of sources from the inner skull surface (mm).", "dist", ""});
    parser.add_option({{"mindistout"}, "Output file for omitted source space points.", "file", ""});
    parser.add_option({{"includeall"}, "Omit all source space checks.", "", ""});

    parser.process(argc, argv, "mne_forward_solution", PROGRAM_VERSION);

    //=========================================================================================================
    // Populate settings from parsed arguments
    //=========================================================================================================

    std::shared_ptr<ComputeFwdSettings> settings = std::make_shared<ComputeFwdSettings>();

    // Build the command string for FIFF stamping
    StringList arguments(argv, argv + argc);
    settings->command = join(arguments, " ");

    // Source & sensor files
    if (parser.is_set("src"))
        settings->srcname = parser.value("src");
    if (parser.is_set("meas"))
        settings->measname = parser.value("meas");
    if (parser.is_set("fwd"))
        settings->solname = parser.value("fwd");

    // Transform options (--mri and --trans are mutually exclusive with --notrans)
    if (parser.is_set("notrans")) {
        settings->mri_head_ident = true;
        settings->mriname.clear();
        settings->transname.clear();
    } else if (parser.is_set("mri")) {
        settings->mri_head_ident = false;
        settings->mriname = parser.value("mri");
        settings->transname.clear();
    } else if (parser.is_set("trans")) {
        settings->mri_head_ident = false;
        settings->transname = parser.value("trans");
        settings->mriname.clear();
    }

    // BEM model
    if (parser.is_set("bem"))
        settings->bemname = parser.value("bem");

    // Sphere model origin (x:y:z in mm, converted to meters)
    if (parser.is_set("origin")) {
        const StringList parts = split(parser.value("origin"), ':');
        if (parts.size() != 3) {
            Log::critical("Could not interpret the origin.");
            return 1;
        }
        const std::optional<double> x = to_double(parts[0]);
        const std::optional<double> y = to_double(parts[1]);
        const std::optional<double> z = to_double(parts[2]);
        if (!x || !y || !z) {
            Log::critical("Could not interpret the origin.");
            return 1;
        }
        settings->r0[0] = static_cast<float>(*x) / 1000.0f;
        settings->r0[1] = static_cast<float>(*y) / 1000.0f;
        settings->r0[2] = static_cast<float>(*z) / 1000.0f;
    }

    // EEG sphere model options
    if (parser.is_set("eegscalp"))
        settings->scale_eeg_pos = true;
    if (parser.is_set("eegmodels"))
        settings->eeg_model_file = parser.value("eegmodels");
    if (parser.is_set("eegmodel"))
        settings->eeg_model_name = parser.value("eegmodel");
    if (parser.is_set("eegrad")) {
        const std::optional<double> rad = to_double(parser.value("eegrad"));
        if (!rad || *rad <= 0) {
            Log::critical("Radius must be a positive number.");
            return 1;
        }
        settings->eeg_sphere_rad = static_cast<float>(*rad) / 1000.0f;
    }

    // Modality flags
    if (parser.is_set("meg"))
        settings->include_meg = true;
    if (parser.is_set("eeg"))
        settings->include_eeg = true;
    if (parser.is_set("grad"))
        settings->compute_grad = true;
    if (parser.is_set("fixed"))
        settings->fixed_ori = true;
    if (parser.is_set("accurate"))
        settings->accurate = true;
    if (parser.is_set("mricoord"))
        settings->coord_frame = FIFFV_COORD_MRI;
    if (parser.is_set("all"))
        settings->do_all = true;

    // Source space filtering
    if (parser.is_set("label")) {
        for (const std::string& label : parser.values("label"))
            settings->labels.push_back(path_from_utf8(label));
        settings->nlabel = static_cast<int>(settings->labels.size());
    }
    if (parser.is_set("mindist")) {
        const std::optional<double> dist = to_double(parser.value("mindist"));
        if (!dist) {
            Log::critical("Could not interpret the distance.");
            return 1;
        }
        const auto distMm = static_cast<float>(*dist);
        settings->mindist = (distMm <= 0.0f) ? 0.0f : distMm / 1000.0f;
    }
    if (parser.is_set("mindistout"))
        settings->mindistoutname = parser.value("mindistout");
    if (parser.is_set("includeall"))
        settings->filter_spaces = false;

    //=========================================================================================================
    // Run forward computation
    //=========================================================================================================

    if (!settings->checkIntegrity()) {
        return 1;
    }

    ComputeFwd computer(settings);
    auto fwdSolution = computer.calculateFwd();
    if (!fwdSolution) {
        Log::critical("The forward solution could not be computed.");
        return 1;
    }

    if (!fwdSolution->write(mne::path_from_utf8(settings->solname))) {
        Log::critical("The forward solution could not be written to {}.", settings->solname);
        return 1;
    }

    return 0;
}
