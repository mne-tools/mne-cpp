//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     mne_bem.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     June 2015
 * @brief    Implementation of @ref mne::MNEBem.
 *
 * Implements FIFF read/write of the @c FIFFB_BEM block, dispatching the
 * @c FIFFB_BEM_SURF child blocks to @ref MNEBemSurface::read and
 * preserving the coordinate frame and conductivity metadata used by the
 * forward solver.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_bem.h"

#include <mne/base/log.h>
#include <mne/math/warp.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

using mne::ErrorCode;
using mne::Expected;
using mne::fail;

//=============================================================================================================

std::optional<FiffTag> find_tag(FiffStream& stream, const std::shared_ptr<FiffDirNode>& node, std::initializer_list<fiff_int_t> kinds)
{
    for (const fiff_int_t kind : kinds) {
        if (auto tag = node->find_tag(stream, kind)) {
            return tag;
        }
    }
    return std::nullopt;
}

//=============================================================================================================

Expected<MNEBemSurface> read_surface(FiffStream& stream, const std::shared_ptr<FiffDirNode>& node, const std::string& name)
{
    MNEBemSurface surface;
    const auto id = find_tag(stream, node, {FIFF_BEM_SURF_ID});
    surface.id = id ? *id->to_int() : FIFFV_BEM_SURF_ID_UNKNOWN;
    const auto sigma = find_tag(stream, node, {FIFF_BEM_SIGMA});
    surface.sigma = sigma ? *sigma->to_float() : 1.0f;

    const auto np = find_tag(stream, node, {FIFF_BEM_SURF_NNODE});
    if (!np) {
        return fail(ErrorCode::Format, "a BEM surface in {} has no vertex count", name);
    }
    surface.np = *np->to_int();
    const auto ntri = find_tag(stream, node, {FIFF_BEM_SURF_NTRI});
    if (!ntri) {
        return fail(ErrorCode::Format, "a BEM surface in {} has no triangle count", name);
    }
    surface.ntri = *ntri->to_int();

    // MNE-C files carry FIFF_BEM_COORD_FRAME, newer ones FIFF_MNE_COORD_FRAME
    const auto frame = find_tag(stream, node, {FIFF_MNE_COORD_FRAME, FIFF_BEM_COORD_FRAME});
    if (!frame) {
        return fail(ErrorCode::Format, "a BEM surface in {} has no coordinate frame", name);
    }
    surface.coord_frame = *frame->to_int();

    const auto nodes = find_tag(stream, node, {FIFF_BEM_SURF_NODES});
    if (!nodes) {
        return fail(ErrorCode::Format, "a BEM surface in {} has no vertices", name);
    }
    surface.rr = nodes->to_float_matrix().transpose();
    if (surface.rr.rows() != surface.np) {
        return fail(ErrorCode::Format, "a BEM surface in {} has {} vertices instead of {}", name, surface.rr.rows(), surface.np);
    }

    // Stored normals are optional (mne-python, MNE-C); read() computes missing ones
    if (const auto normals = find_tag(stream, node, {FIFF_BEM_SURF_NORMALS, FIFF_MNE_SOURCE_SPACE_NORMALS})) {
        surface.nn = normals->to_float_matrix().transpose();
        if (surface.nn.rows() != surface.np) {
            return fail(ErrorCode::Format, "a BEM surface in {} has {} normals for {} vertices", name, surface.nn.rows(), surface.np);
        }
    } else {
        surface.nn.resize(0, 3);
    }

    if (surface.ntri > 0) {
        const auto triangles = find_tag(stream, node, {FIFF_BEM_SURF_TRIANGLES, FIFF_MNE_SOURCE_SPACE_TRIANGLES});
        if (!triangles) {
            return fail(ErrorCode::Format, "a BEM surface in {} has no triangles", name);
        }
        surface.itris = triangles->to_int_matrix().transpose().array() - 1; // 1-based in the file
        if (surface.itris.rows() != surface.ntri) {
            return fail(ErrorCode::Format, "a BEM surface in {} has {} triangles instead of {}", name, surface.itris.rows(), surface.ntri);
        }
    } else {
        surface.itris.resize(0, 3);
    }
    return surface;
}

} // namespace

//=============================================================================================================

MNEBem::MNEBem(const std::filesystem::path& path)
: MNEBem(read(path).value())
{
}

//=============================================================================================================

Expected<MNEBem> MNEBem::read(const std::filesystem::path& path, bool patch_stats)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    return read(*stream, patch_stats);
}

//=============================================================================================================

Expected<MNEBem> MNEBem::read(FiffStream& stream, bool patch_stats)
{
    const std::string name = stream.stream_name();
    if (!stream.is_open() && !stream.open()) {
        return fail(ErrorCode::Io, "cannot open {}", name);
    }
    if (stream.dirtree()->dir_tree_find(FIFFB_BEM).empty()) {
        return fail(ErrorCode::NotFound, "no BEM in {}", name);
    }
    const std::vector<std::shared_ptr<FiffDirNode>> nodes = stream.dirtree()->dir_tree_find(FIFFB_BEM_SURF);
    if (nodes.empty()) {
        return fail(ErrorCode::NotFound, "no BEM surfaces in {}", name);
    }

    MNEBem bem;
    bem.m_surfaces.reserve(nodes.size());
    for (const std::shared_ptr<FiffDirNode>& node : nodes) {
        auto surface = read_surface(stream, node, name);
        if (!surface) {
            return std::unexpected(surface.error());
        }
        surface->addTriangleData();
        if (patch_stats || surface->nn.rows() == 0) {
            surface->addVertexNormals();
        }
        bem.m_surfaces.push_back(std::move(*surface));
    }
    Log::info("\t{} BEM surfaces read", bem.size());
    return bem;
}

//=============================================================================================================

Expected<void> MNEBem::write(const std::filesystem::path& path) const
{
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return fail(ErrorCode::Io, "cannot write {}", utf8_from_path(path));
    }
    write_to_stream(*stream);
    stream->end_file();
    return {};
}

//=============================================================================================================

void MNEBem::write_to_stream(FiffStream& stream) const
{
    stream.start_block(FIFFB_BEM);
    for (const MNEBemSurface& surface : m_surfaces) {
        stream.start_block(FIFFB_BEM_SURF);
        surface.write_to_stream(stream);
        stream.end_block(FIFFB_BEM_SURF);
    }
    stream.end_block(FIFFB_BEM);
}

//=============================================================================================================

void MNEBem::warp(const MatrixXf& sLm, const MatrixXf& dLm)
{
    std::vector<MatrixXf> vertices;
    vertices.reserve(m_surfaces.size());
    for (const MNEBemSurface& surface : m_surfaces) {
        vertices.push_back(surface.rr);
    }
    Warp().calculate(sLm, dLm, vertices);
    for (std::size_t i = 0; i < m_surfaces.size(); ++i) {
        m_surfaces[i].rr = vertices[i];
    }
}

//=============================================================================================================

void MNEBem::transform(const FiffCoordTrans& trans)
{
    for (MNEBemSurface& surface : m_surfaces) {
        surface.rr = trans.apply_trans(surface.rr);
    }
}

//=============================================================================================================

void MNEBem::inverse_transform(const FiffCoordTrans& trans)
{
    for (MNEBemSurface& surface : m_surfaces) {
        surface.rr = trans.apply_inverse_trans(surface.rr);
    }
}
