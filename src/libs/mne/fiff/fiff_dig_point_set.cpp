//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fiff_dig_point_set.cpp
 * @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     July 2016
 * @brief    Implementation of @ref FiffDigPointSet: parses an entire FIFFB_ISOTRAK block into a list of @ref FiffDigPoint records.
 *
 * Mirrors the @c info['dig'] list in MNE-Python. Used by the
 * registration GUIs and by the head-shape coregistration paths.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_point_set.h"

#include "fiff_dig_point.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_types.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <algorithm>
#include <optional>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(std::vector<FiffDigPoint> points)
: m_points(std::move(points))
{
}

//=============================================================================================================

FiffDigPointSet::FiffDigPointSet(const std::filesystem::path& path)
: FiffDigPointSet(read(path).value())
{
}

//=============================================================================================================

mne::Expected<FiffDigPointSet> FiffDigPointSet::read(const std::filesystem::path& path)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    return read(*stream);
}

//=============================================================================================================

mne::Expected<FiffDigPointSet> FiffDigPointSet::read(FiffStream& stream)
{
    const bool openHere = !stream.is_open();
    if (openHere && !stream.open()) {
        return mne::fail(mne::ErrorCode::Io, "cannot open {}", stream.stream_name());
    }

    FiffDigPointSet dig;
    fiff_int_t coordFrame = FIFFV_COORD_HEAD;
    const std::vector<std::shared_ptr<FiffDirNode>> isotrak = stream.dirtree()->dir_tree_find(FIFFB_ISOTRAK);
    if (isotrak.size() == 1) {
        for (const FiffDirEntry& entry : isotrak[0]->dir) {
            if (entry.kind != FIFF_DIG_POINT && entry.kind != FIFF_MNE_COORD_FRAME) {
                continue;
            }
            const std::optional<FiffTag> tag = stream.read_tag(entry.pos);
            if (!tag) {
                return mne::fail(mne::ErrorCode::Format, "cannot read the digitizer tag at {}", entry.pos);
            }
            if (entry.kind == FIFF_DIG_POINT) {
                dig.m_points.push_back(tag->to_dig_point());
            } else {
                // Applies to every point of the block, as mne's _read_dig_fif
                coordFrame = *tag->to_int();
            }
        }
    }
    for (FiffDigPoint& point : dig.m_points) {
        point.coord_frame = coordFrame;
    }

    if (openHere) {
        stream.close();
    }
    return dig;
}

//=============================================================================================================

mne::Expected<void> FiffDigPointSet::write(const std::filesystem::path& path) const
{
    if (path.empty()) {
        return mne::fail(mne::ErrorCode::Io, "the output path is empty");
    }
    std::error_code ec;
    const std::filesystem::path dir = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
    if (!std::filesystem::is_directory(dir, ec)) {
        return mne::fail(mne::ErrorCode::NotFound, "the directory {} does not exist", mne::utf8_from_path(dir));
    }
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return mne::fail(mne::ErrorCode::Io, "cannot open {} for writing", mne::utf8_from_path(path));
    }
    write_to_stream(*stream);
    stream->end_file();
    stream->close();
    return {};
}

//=============================================================================================================

void FiffDigPointSet::write_to_stream(FiffStream& stream) const
{
    stream.start_block(FIFFB_MEAS);
    stream.start_block(FIFFB_MEAS_INFO);
    stream.start_block(FIFFB_ISOTRAK);
    for (const FiffDigPoint& point : m_points) {
        stream.write_dig_point(point);
    }
    stream.end_block(FIFFB_ISOTRAK);
    stream.end_block(FIFFB_MEAS_INFO);
    stream.end_block(FIFFB_MEAS);
}

//=============================================================================================================

FiffDigPointSet FiffDigPointSet::pick_types(std::span<const int> kinds) const
{
    FiffDigPointSet picked;
    for (const FiffDigPoint& point : m_points) {
        if (std::ranges::find(kinds, point.kind) != kinds.end()) {
            picked.m_points.push_back(point);
        }
    }
    return picked;
}

//=============================================================================================================

void FiffDigPointSet::apply_transform(const FiffCoordTrans& coordTrans, bool inverse)
{
    const auto& trans = inverse ? coordTrans.invtrans : coordTrans.trans;
    for (FiffDigPoint& point : m_points) {
        const Vector4f r = trans * Vector4f(point.r[0], point.r[1], point.r[2], 1.0f);
        point.r[0] = r(0);
        point.r[1] = r(1);
        point.r[2] = r(2);
    }
}

//=============================================================================================================

bool FiffDigPointSet::transform_to_head()
{
    const FiffDigPoint* fid[3] = {nullptr, nullptr, nullptr};
    for (const FiffDigPoint& p : m_points) {
        if (p.kind == FIFFV_POINT_CARDINAL && p.ident >= FIFFV_POINT_LPA && p.ident <= FIFFV_POINT_RPA) {
            fid[p.ident - FIFFV_POINT_LPA] = &p;
        }
    }
    if (!fid[0] || !fid[1] || !fid[2] || fid[0]->coord_frame != fid[1]->coord_frame || fid[0]->coord_frame != fid[2]->coord_frame) {
        mne::Log::warning("[FiffDigPointSet::transform_to_head] LPA, nasion and RPA in one frame are required.");
        return false;
    }
    const int frame = fid[0]->coord_frame;
    if (frame == FIFFV_COORD_HEAD) {
        return true;
    }
    // head -> native, built from the fiducials given in the native frame
    const FiffCoordTrans headNative = FiffCoordTrans::from_cardinal_points(FIFFV_COORD_HEAD, frame, fid[0]->r, fid[1]->r, fid[2]->r);
    for (FiffDigPoint& p : m_points) {
        if (p.coord_frame == frame) {
            FiffCoordTrans::apply_inverse_trans(p.r, headNative, true);
            p.coord_frame = FIFFV_COORD_HEAD;
        }
    }
    return true;
}
