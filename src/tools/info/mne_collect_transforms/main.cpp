//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2008
 * @brief    Collect coordinate transformations into one FIFF file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_coord_trans_set.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>
#include <mne/mri/mri_mgh_io.h>
#include <mne/mri/mri_vol_data.h>

#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <print>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static void printTransform(const FiffCoordTrans& t)
{
    Log::info("{} -> {} transform:", std::string(FiffCoordTrans::frame_name(t.from)), std::string(FiffCoordTrans::frame_name(t.to)));
    for (int i = 0; i < 3; i++) {
        Log::info("  {:10.6f} {:10.6f} {:10.6f}  {:10.4f} mm", t.trans(i, 0), t.trans(i, 1), t.trans(i, 2), 1000.0f * t.trans(i, 3));
    }
    Log::info("{}", "");
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Collect coordinate transforms into one FIFF file.\n\n"
                             "Reads the device -> head transform of a measurement, the MRI -> head transform "
                             "of a coregistration, and the surface RAS -> RAS -> MNI Talairach -> Talairach "
                             "chain of an MRI set or of an mgh/mgz volume with its transforms/talairach.xfm.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"meas"}, "MEG measurement file (device->head transform).", "name", {}});
    parser.add_option({{"mri"}, "FIFF MRI description or -trans.fif file (MRI->head transform and, without --mgh, the Talairach chain).", "name", {}});
    parser.add_option({{"mgh"}, "mgh/mgz MRI volume; its centre and transforms/talairach.xfm give the Talairach chain.", "name", {}});
    parser.add_option({{"out"}, "Output file name.", "name", {}});
    parser.process(argc, argv, "mne_collect_transforms", PROGRAM_VERSION);

    const std::string measName = parser.value("meas");
    const std::string mriName = parser.value("mri");
    const std::string mghName = parser.value("mgh");
    const std::string outName = parser.value("out");
    if (measName.empty() && mriName.empty() && mghName.empty()) {
        Log::critical("At least one of --meas, --mri or --mgh must be specified.");
        std::print("{}", parser.help_text("mne_collect_transforms"));
        return 1;
    }

    FiffCoordTrans devHeadT;
    if (!measName.empty()) {
        const auto read = FiffCoordTrans::read(path_from_utf8(measName), FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD);
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        devHeadT = *read;
    }

    FiffCoordTransSet chain;
    if (!mghName.empty()) {
        MriVolData volume;
        std::vector<FiffCoordTrans> volumeTrans;
        const std::filesystem::path mriDir = std::filesystem::absolute(path_from_utf8(mghName)).lexically_normal().parent_path();
        if (!MriMghIO::read(mghName, volume, volumeTrans, utf8_from_path(mriDir))) {
            Log::critical("Cannot read {}", mghName);
            return 1;
        }
        for (const FiffCoordTrans& t : volumeTrans) {
            if (t.from == FIFFV_COORD_MRI && t.to == FIFFV_MNE_COORD_RAS) {
                chain.surf_RAS_RAS_t = t;
            }
        }
        // FreeSurfer keeps the Talairach transform next to the volume (MNE-C mne_mri_get_mgh_xform_file_name).
        if (!chain.addTalairach(mriDir / "transforms" / "talairach.xfm")) {
            Log::warning("No Talairach transform found for {}", mghName);
        }
    }
    if (!mriName.empty()) {
        FiffCoordTransSet fromFile;
        if (fromFile.read(path_from_utf8(mriName)) < 0 || fromFile.head_surf_RAS_t.empty()) {
            Log::critical("No MRI->head transform in {}", mriName);
            return 1;
        }
        chain.head_surf_RAS_t = fromFile.head_surf_RAS_t;
        if (mghName.empty()) {
            chain = fromFile;
        }
    }

    if (!devHeadT.empty()) {
        printTransform(devHeadT);
    }
    for (const FiffCoordTrans* t : {&chain.head_surf_RAS_t, &chain.surf_RAS_RAS_t, &chain.RAS_MNI_tal_t, &chain.MNI_tal_tal_gtz_t, &chain.MNI_tal_tal_ltz_t}) {
        if (!t->empty()) {
            printTransform(*t);
        }
    }

    if (!outName.empty()) {
        auto outStream = FiffStream::start_file(path_from_utf8(outName));
        if (!outStream) {
            Log::critical("Cannot open output file: {}", outName);
            return 1;
        }
        if (!devHeadT.empty()) {
            outStream->write_coord_trans(devHeadT);
        }
        chain.write(*outStream);
        outStream->end_file();
        Log::info("Written {}", outName);
    }
    return 0;
}
