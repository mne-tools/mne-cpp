//=============================================================================================================
/**
 * @file     fiff_digitizer_data.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the FiffDigitizerData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_digitizer_data.h"
#include "fiff_coord_trans.h"
#include "fiff_stream.h"
#include "fiff_dig_point.h"
#include <iostream>

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
            qWarning() << "Warning in FiffDigitizerData::FiffDigitizerData - Could not open the didigitzer data file"; // ToDo throw error
            return;
        }

        open_here = true;
    }

    // If device is open read the data
    if(!t_pStream->read_digitizer_data(t_pStream->dirtree(), *this)) {
        qWarning() << "Warning in FiffDigitizerData::FiffDigitizerData - Could not read the FiffDigitizerData"; // ToDo throw error
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
