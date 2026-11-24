//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Fix magnetometer coil types in FIFF files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_dir_entry.h>
#include <mne/fiff/fiff_ch_info.h>

#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
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

static int processFile(const std::string& name, bool doMagnes, std::string& summary)
{
    auto stream = FiffStream::open_update(path_from_utf8(name));
    if (!stream) {
        Log::critical("Cannot open file for update: {}", name);
        return -1;
    }

    int found = 0;
    int nMagnes = 0;
    int nCoilNone = 0;

    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k].kind == FIFF_CH_INFO) {
            std::optional<FiffTag> tag;
            tag = stream->read_tag(stream->dir()[k].pos);
            FiffChInfo ch = tag->to_ch_info();

            bool modified = false;

            // Fix Vectorview magnetometer coil type
            if (ch.chpos.coil_type == FIFFV_COIL_VV_MAG_T1) {
                ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
                found++;
                modified = true;
            }

            // Set COIL_NONE for non-MEG/EEG channels
            if (ch.kind != FIFFV_MEG_CH &&
                ch.kind != FIFFV_EEG_CH &&
                ch.kind != FIFFV_MCG_CH &&
                ch.kind != FIFFV_REF_MEG_CH) {
                if (ch.chpos.coil_type != FIFFV_COIL_NONE) {
                    ch.chpos.coil_type = FIFFV_COIL_NONE;
                    nCoilNone++;
                    modified = true;
                }
            }

            // Fix Magnes coil types
            if (doMagnes) {
                if (ch.chpos.coil_type == FIFFV_COIL_MAGNES_MAG) {
                    ch.chpos.coil_type = FIFFV_COIL_MAGNES_GRAD;
                    found++;
                    nMagnes++;
                    modified = true;
                }
            }

            if (modified) {
                stream->byte_stream().seek(stream->dir()[k].pos);
                stream->write_ch_info(ch);
            }
        }
    }

    summary = found ? std::format("fixed {} magnetometer coil types", found)
                    : std::string("no magnetometer coil type fixes");
    if (nCoilNone)
        summary += std::format(" / fixed {} coil_type fields in non MEG/EEG channels", nCoilNone);
    if (doMagnes)
        summary += std::format(" / {} Magnes fixes", nMagnes);

    stream->close();
    return 0;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "Fix magnetometer coil types in FIFF files.\n\n"
        "Replaces FIFFV_COIL_VV_MAG_T1 with FIFFV_COIL_VV_MAG_T3.\n"
        "Sets FIFFV_COIL_NONE for non-MEG/EEG channels.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"magnes"}, "Also fix Magnes WH magnetometer coil types.", {}, {}});
    parser.add_positional_argument("files", "FIFF file(s) to process.");
    parser.process(argc, argv, "mne_fix_mag_coil_types", PROGRAM_VERSION);

    bool doMagnes = parser.is_set("magnes");
    const StringList& files = parser.positional_arguments();

    if (files.empty()) {
        Log::critical("No input files specified.");
        std::print("{}", parser.help_text("mne_fix_mag_coil_types"));
        return 1;
    }

    int failed = 0;
    for (const std::string& fname : files) {
        std::string summary;
        if (processFile(fname, doMagnes, summary) != 0) {
            Log::critical("{} ... [failed]", fname);
            ++failed;
        } else {
            Log::info("{} ... {} [ok]", fname, summary);
        }
    }

    // A file that could not be updated is a failure of the run, not a remark in
    // passing. Reporting success here made the tool unusable from a script: a
    // batch in which every single file failed still exited zero.
    if (failed) {
        Log::critical("{} of {} file(s) could not be updated.", failed, static_cast<long long>(files.size()));
        return 1;
    }

    return 0;
}
