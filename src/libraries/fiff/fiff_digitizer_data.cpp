//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_digitizer_data.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref FiffDigitizerData: digitization data plus device→head transform and HPI fit metadata.
 *
 * Persists ``*-dig.fif'' / ``*-fiducials.fif'' files and feeds the
 * forward / inverse pipeline with a self-contained registration record.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_digitizer_data.h"
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dig_point.h"
#include <iostream>
#include <QDebug>

#include <stdexcept>
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigitizerData::FiffDigitizerData()
: coord_frame(FIFFV_COORD_UNKNOWN)
, npoint(0)
, show(false)
, show_minimal(false)
, dist_valid(false)
{
}

//=============================================================================================================

FiffDigitizerData::~FiffDigitizerData() = default;

//=============================================================================================================

FiffDigitizerData::FiffDigitizerData(const FiffDigitizerData& p_FiffDigitizerData)
: head_mri_t(p_FiffDigitizerData.head_mri_t ? std::make_unique<FiffCoordTrans>(*p_FiffDigitizerData.head_mri_t) : nullptr)
, head_mri_t_adj(p_FiffDigitizerData.head_mri_t_adj ? std::make_unique<FiffCoordTrans>(*p_FiffDigitizerData.head_mri_t_adj) : nullptr)
, points(p_FiffDigitizerData.points)
, coord_frame(p_FiffDigitizerData.coord_frame)
, active(p_FiffDigitizerData.active)
, discard(p_FiffDigitizerData.discard)
, npoint(p_FiffDigitizerData.npoint)
, mri_fids(p_FiffDigitizerData.mri_fids)
, show(p_FiffDigitizerData.show)
, show_minimal(p_FiffDigitizerData.show_minimal)
, dist(p_FiffDigitizerData.dist)
, closest(p_FiffDigitizerData.closest)
, closest_point(p_FiffDigitizerData.closest_point)
, dist_valid(p_FiffDigitizerData.dist_valid)
{
}

//=============================================================================================================

FiffDigitizerData& FiffDigitizerData::operator=(const FiffDigitizerData& rhs)
{
    if (this != &rhs) {
        head_mri_t     = rhs.head_mri_t     ? std::make_unique<FiffCoordTrans>(*rhs.head_mri_t)     : nullptr;
        head_mri_t_adj = rhs.head_mri_t_adj ? std::make_unique<FiffCoordTrans>(*rhs.head_mri_t_adj) : nullptr;
        filename       = rhs.filename;
        points         = rhs.points;
        coord_frame    = rhs.coord_frame;
        active         = rhs.active;
        discard        = rhs.discard;
        npoint         = rhs.npoint;
        mri_fids       = rhs.mri_fids;
        show           = rhs.show;
        show_minimal   = rhs.show_minimal;
        dist           = rhs.dist;
        closest        = rhs.closest;
        closest_point  = rhs.closest_point;
        dist_valid     = rhs.dist_valid;
    }
    return *this;
}

//=============================================================================================================

FiffDigitizerData::FiffDigitizerData(QIODevice &p_IODevice)
: coord_frame(FIFFV_COORD_UNKNOWN)
, npoint(0)
, show(false)
, show_minimal(false)
, dist_valid(false)
{
    // Open the io device
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    bool open_here = false;

    //Open if the device and stream have not been openend already
    if (!t_pStream->device()->isOpen()) {
        if(!t_pStream->open()) {
            throw std::runtime_error("Could not open the digitizer data file");
        }

        open_here = true;
    }

    // If device is open read the data
    if(!t_pStream->read_digitizer_data(t_pStream->dirtree(), *this)) {
        throw std::runtime_error("Could not read the FiffDigitizerData");
    }

    // If stream has been opened in this function also close here again
    if(open_here) {
        t_pStream->close();
    }
}

//=============================================================================================================

void FiffDigitizerData::print() const
{
    std::cout << "Number of digitizer points: " << points.size() << "\n";

    switch(coord_frame){
    case FIFFV_COORD_MRI:
        std::cout << "Coord. Frame: FIFFV_COORD_MRI \n";
         break;
    case FIFFV_COORD_HEAD:
        std::cout << "Coord. Frame: FIFFV_COORD_HEAD \n";
        break;
    }

    for (auto& point : points){
        if (point.kind == FIFFV_POINT_HPI){
            std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
        }
    }

    std::cout << "Number of MRI fiducials: " << nfids() << "\n";

    if (head_mri_t){

    }
}

//=============================================================================================================

void FiffDigitizerData::pickCardinalFiducials()
{
    // Clear any existing MRI fiducials
    mri_fids.clear();

    if (!head_mri_t_adj) {
        return;
    }

    // Extract cardinal points and transform them into MRI coordinates.
    // This mirrors the original C function update_fids_from_dig_data
    // from mne_analyze/adjust_alignment.c.
    for (int k = 0; k < npoint; ++k) {
        if (points[k].kind == FIFFV_POINT_CARDINAL) {
            FiffDigPoint fid = points[k];
            FiffCoordTrans::apply_trans(fid.r, *head_mri_t_adj, true);
            mri_fids.append(fid);
        }
    }
}
