//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiff_io.cpp
 * @author   Florian Schlembach <fschlembach@web.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     November 2013
 * @brief    Implementation of @ref FiffIO: one-call FIFF reader/writer that dispatches to the right specialized loader per top-level block.
 *
 * Sniffs the FIFF block tree, instantiates @ref FiffRawData /
 * @ref FiffEvokedSet / @ref FiffCov / @ref FiffInfo as appropriate and
 * returns shared pointers ready for the rest of the pipeline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_io.h"
#include "fiff_evoked_set.h"
#include "fiff_stream.h"

#include <base/log.h>
#include <base/string_utils.h>

#include <cmath>
#include <cstdint>
#include <exception>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffIO::FiffIO(const std::filesystem::path& path)
{
    read(path);
}

//=============================================================================================================

FiffIO::FiffIO(const std::vector<std::filesystem::path>& paths)
{
    for (const std::filesystem::path& path : paths) {
        read(path);
    }
}

//=============================================================================================================

bool FiffIO::setup_read(const std::filesystem::path& path, FiffInfo& info, std::shared_ptr<FiffDirNode>& dirTree)
{
    mne::Log::info("Opening fiff data {}...\n", mne::utf8_from_path(path));
    FiffStream stream(path);
    return stream.open() && stream.read_meas_info(stream.dirtree(), info, dirTree);
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> FiffIO::readRaw(FiffStream& stream, std::unique_ptr<mne::ByteStream> rawBytes)
{
    FiffInfo info;
    std::shared_ptr<FiffDirNode> infoNode;
    if (!stream.open() || !stream.read_meas_info(stream.dirtree(), info, infoNode) || !infoNode) {
        mne::Log::warning("[FiffIO::read] Dir tree could not be read");
        return nullptr;
    }
    const std::shared_ptr<FiffDirNode>& tree = stream.dirtree();
    if (tree->has_kind(FIFFB_RAW_DATA) || tree->has_kind(FIFFB_PROCESSED_DATA) || tree->has_kind(FIFFB_CONTINUOUS_DATA) ||
        tree->has_kind(FIFFB_SMSH_RAW_DATA)) {
        try {
            raw.push_back(std::make_shared<FiffRawData>(std::move(rawBytes)));
            mne::Log::info("Finished reading raw data!\n");
        } catch (const std::exception& e) {
            mne::Log::warning("[FiffIO::read] Could not read raw data: {}", e.what());
        }
    }
    return tree;
}

//=============================================================================================================

bool FiffIO::read(const std::filesystem::path& path)
{
    FiffStream stream(path);
    const std::shared_ptr<FiffDirNode> tree = readRaw(stream, mne::FileByteStream::open(path, mne::FileByteStream::Mode::Read));
    if (!tree) {
        return false;
    }
    if (tree->has_kind(FIFFB_EVOKED)) {
        if (auto set = FiffEvokedSet::read(path)) {
            for (const FiffEvoked& one : set->evoked) {
                evoked.push_back(std::make_shared<FiffEvoked>(one));
            }
        } else {
            mne::Log::warning("[FiffIO::read] {}", set.error().what());
        }
    }
    return true;
}

//=============================================================================================================

bool FiffIO::read(std::vector<std::byte> bytes)
{
    FiffStream stream(std::make_unique<mne::MemoryByteStream>(bytes));
    const std::shared_ptr<FiffDirNode> tree = readRaw(stream, std::make_unique<mne::MemoryByteStream>(std::move(bytes)));
    if (tree && tree->has_kind(FIFFB_EVOKED)) {
        mne::Log::warning("[FiffIO::read] Evoked data can only be read from a file.");
    }
    return tree != nullptr;
}

//=============================================================================================================

bool FiffIO::write(const std::filesystem::path& path, fiff_int_t type, fiff_int_t idx) const
{
    switch (type) {
        case FIFFB_RAW_DATA: {
            if (idx >= 0) {
                return write_raw(path, idx);
            }
            bool ok = true;
            for (fiff_int_t i = 0; i < static_cast<fiff_int_t>(raw.size()); ++i) {
                std::filesystem::path one = path;
                one.replace_filename(std::format("{}_raw-{}{}", mne::utf8_from_path(path.stem()), i, mne::utf8_from_path(path.extension())));
                mne::Log::info("Writing raw data set {} to {} ...", i, mne::utf8_from_path(one));
                ok &= write_raw(one, i);
            }
            return ok;
        }
        case FIFFB_EVOKED: {
            if (evoked.empty() || idx >= static_cast<fiff_int_t>(evoked.size())) {
                return false;
            }
            FiffEvokedSet set;
            set.info = evoked.front()->info;
            for (fiff_int_t i = 0; i < static_cast<fiff_int_t>(evoked.size()); ++i) {
                if (idx < 0 || i == idx) {
                    set.evoked.push_back(*evoked[i]);
                }
            }
            if (auto saved = set.save(path); !saved) {
                mne::Log::warning("[FiffIO::write] {}", saved.error().what());
                return false;
            }
            return true;
        }
        default:
            return false;
    }
}

//=============================================================================================================

bool FiffIO::write_raw(const std::filesystem::path& path, fiff_int_t idx) const
{
    if (idx < 0 || idx >= static_cast<fiff_int_t>(raw.size())) {
        return false;
    }
    const FiffRawData& data = *raw[idx];
    RowVectorXd cals;
    auto outfid = FiffStream::start_writing_raw(path, data.info, cals);
    if (!outfid) {
        return false;
    }

    // Read and write in 30 s chunks
    const fiff_int_t quantum = static_cast<fiff_int_t>(std::ceil(30.0 * data.info.sfreq));
    SparseMatrix<double> mult;
    RowVectorXi sel;
    MatrixXd buffer;
    MatrixXd times;
    for (fiff_int_t first = data.first_samp; first < data.last_samp; first += quantum) {
        const fiff_int_t last = std::min(first + quantum - 1, data.last_samp);
        if (!data.read_raw_segment(buffer, times, mult, first, last, sel)) {
            mne::Log::warning("error during read_raw_segment\n");
            return false;
        }
        if (first == data.first_samp && first > 0) {
            fiff_int_t firstSample = first;
            outfid->write_int(FIFF_FIRST_SAMPLE, &firstSample);
        }
        outfid->write_raw_buffer(buffer, cals);
    }
    outfid->finish_writing_raw();
    return true;
}
