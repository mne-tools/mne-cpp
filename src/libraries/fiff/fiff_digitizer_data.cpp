//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_digitizer_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FiffDigitizerData: digitization data plus device→head transform and HPI fit metadata.
 *
 * Persists ``*-dig.fif'' / ``*-fiducials.fif'' files and feeds the
 * forward / inverse pipeline with a self-contained registration record.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_digitizer_data.h"
#include <base/log.h>
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dig_point.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigitizerData::FiffDigitizerData(const std::filesystem::path& path)
: FiffDigitizerData(read(path).value())
{
}

//=============================================================================================================

mne::Expected<FiffDigitizerData> FiffDigitizerData::read(const std::filesystem::path& path)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    return stream->read_digitizer_data(stream->dirtree());
}

//=============================================================================================================

void FiffDigitizerData::print() const
{
    mne::Log::info("Number of digitizer points: {}", points.size());

    switch (coord_frame) {
        case FIFFV_COORD_MRI:
            mne::Log::info("Coord. Frame: FIFFV_COORD_MRI");
            break;
        case FIFFV_COORD_HEAD:
            mne::Log::info("Coord. Frame: FIFFV_COORD_HEAD");
            break;
    }

    for (auto& point : points) {
        if (point.kind == FIFFV_POINT_HPI) {
            mne::Log::info("HPI Point {} - {}, {}, {}", point.ident, point.r[0], point.r[1], point.r[2]);
        }
    }

    mne::Log::info("Number of MRI fiducials: {}", nfids());
}

//=============================================================================================================

void FiffDigitizerData::pick_cardinal_fiducials()
{
    mri_fids.clear();
    if (!head_mri_t_adj) {
        return;
    }
    for (FiffDigPoint fid : points.pick_types({FIFFV_POINT_CARDINAL})) {
        FiffCoordTrans::apply_trans(fid.r, *head_mri_t_adj, true);
        mri_fids.push_back(fid);
    }
}
