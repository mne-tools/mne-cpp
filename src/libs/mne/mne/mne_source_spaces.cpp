//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_source_spaces.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     July 2012
 * @brief    Implementation of @ref mne::MNESourceSpaces.
 *
 * Defines whole-file FIFF readers that iterate every
 * @c FIFFB_MNE_SOURCE_SPACE block under the root @c FIFFB_MNE block and
 * delegate the per-space parsing to @ref MNESourceSpace::read. Also
 * implements iteration helpers used by @ref MNEForwardSolution to find
 * the matching source space for a given leadfield column.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_source_spaces.h"

#include "mne_nearest.h"
#include <mne/base/log.h>
#include <mne/math/linalg.h>
#include <mne/fs/fs_label.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <vector>

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

Expected<MNEHemisphere> read_source_space(FiffStream& stream, const std::shared_ptr<FiffDirNode>& p_Tree, const std::string& name)
{
    MNEHemisphere p_Hemisphere;

    std::optional<FiffTag> t_pTag;

    //=====================================================================
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_ID); !t_pTag)
        p_Hemisphere.id = FIFFV_MNE_SURF_UNKNOWN;
    else
        p_Hemisphere.id = *t_pTag->to_int();


    //=====================================================================
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NPOINTS); !t_pTag) {
        return fail(ErrorCode::Format, "a source space in {} has no vertex count", name);
    }
    p_Hemisphere.np = *t_pTag->to_int();

    //=====================================================================
    if (t_pTag = p_Tree->find_tag(stream, FIFF_BEM_SURF_NTRI); !t_pTag) {
        if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NTRI); !t_pTag)
            p_Hemisphere.ntri = 0;
        else
            p_Hemisphere.ntri = *t_pTag->to_int();
    } else {
        p_Hemisphere.ntri = *t_pTag->to_int();
    }

    //=====================================================================
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_COORD_FRAME); !t_pTag) {
        return fail(ErrorCode::Format, "a source space in {} has no coordinate frame", name);
    }
    p_Hemisphere.coord_frame = *t_pTag->to_int();

    //=====================================================================
    //
    //   Vertices, normals, and triangles
    //
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_POINTS); !t_pTag) {
        return fail(ErrorCode::Format, "a source space in {} has no vertices", name);
    }

    p_Hemisphere.rr = t_pTag->to_float_matrix().transpose();
    std::int32_t rows_rr = p_Hemisphere.rr.rows();

    if (rows_rr != p_Hemisphere.np) {
        return fail(ErrorCode::Format, "a source space in {} has a vertex count that does not match its vertices", name);
    }

    //=====================================================================
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NORMALS); !t_pTag) {
        return fail(ErrorCode::Format, "a source space in {} has no vertex normals", name);
    }

    p_Hemisphere.nn = t_pTag->to_float_matrix().transpose();
    std::int32_t rows_nn = p_Hemisphere.nn.rows();

    if (rows_nn != p_Hemisphere.np) {
        return fail(ErrorCode::Format, "a source space in {} has a normal count that does not match its vertices", name);
    }

    //=====================================================================
    if (p_Hemisphere.ntri > 0) {
        if (t_pTag = p_Tree->find_tag(stream, FIFF_BEM_SURF_TRIANGLES); !t_pTag) {
            if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES); !t_pTag) {
                return fail(ErrorCode::Format, "a source space in {} has no triangles", name);
            } else {
                p_Hemisphere.itris = t_pTag->to_int_matrix().transpose();
                p_Hemisphere.itris.array() -= 1; //0 based indizes
            }
        } else {
            p_Hemisphere.itris = t_pTag->to_int_matrix().transpose();
            p_Hemisphere.itris.array() -= 1; //0 based indizes
        }
        if (p_Hemisphere.itris.rows() != p_Hemisphere.ntri) {
            return fail(ErrorCode::Format, "a source space in {} has a triangle count that does not match its triangles", name);
        }
    } else {
        p_Hemisphere.itris.resize(0, 3);
    }

    //
    //   Which vertices are active
    //
    if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE); !t_pTag) {
        p_Hemisphere.nuse = 0;
        p_Hemisphere.inuse = VectorXi::Zero(p_Hemisphere.nuse);
        VectorXi p_defaultVector;
        p_Hemisphere.vertno = p_defaultVector;
    } else {
        p_Hemisphere.nuse = *t_pTag->to_int();
        if (t_pTag = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_SELECTION); !t_pTag) {
            return fail(ErrorCode::Format, "a source space in {} has no source selection", name);
        }
        p_Hemisphere.inuse = VectorXi(Map<const VectorXi>(t_pTag->to_int(), t_pTag->size() / 4, 1)); //use copy constructor, for the sake of easy memory management

        p_Hemisphere.vertno = VectorXi::Zero(p_Hemisphere.nuse);
        if (p_Hemisphere.inuse.rows() != p_Hemisphere.np) {
            return fail(ErrorCode::Format, "a source space in {} has a selection that does not match its vertices", name);
        }
        int pp = 0;
        for (int p = 0; p < p_Hemisphere.np; ++p) {
            if (p_Hemisphere.inuse(p) == 1) {
                p_Hemisphere.vertno(pp) = p;
                ++pp;
            }
        }
    }

    //
    //   Use triangulation
    //
    std::optional<FiffTag> t_pTag1;
    std::optional<FiffTag> t_pTag2;
    t_pTag1 = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NUSE_TRI);
    t_pTag2 = t_pTag1 ? p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES) : std::nullopt;
    if (!t_pTag1 || !t_pTag2) {
        MatrixX3i p_defaultMatrix;
        p_Hemisphere.nuse_tri = 0;
        p_Hemisphere.use_itris = p_defaultMatrix;
    } else {
        p_Hemisphere.nuse_tri = *t_pTag1->to_int();
        p_Hemisphere.use_itris = t_pTag2->to_int_matrix().transpose();
        p_Hemisphere.use_itris.array() -= 1; //0 based indizes
    }

    //
    //   Patch-related information
    //
    t_pTag1 = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST);
    t_pTag2 = t_pTag1 ? p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NEAREST_DIST) : std::nullopt;
    if (!t_pTag1 || !t_pTag2) {
        p_Hemisphere.nearest.clear();
    } else {
        //res.nearest = tag1.data + 1;
        VectorXi nearestIdx = VectorXi(Map<const VectorXi>(t_pTag1->to_int(), t_pTag1->size() / 4, 1));
        VectorXd nearestDist = VectorXd((Map<const VectorXf>(t_pTag2->to_float(), t_pTag2->size() / 4, 1)).cast<double>());
        p_Hemisphere.setNearestData(nearestIdx, nearestDist);
    }

    //    patch_info(p_Hemisphere.nearest, p_Hemisphere.pinfo);
    if (p_Hemisphere.compute_patch_info())
        Log::info("\tPatch information added...");
    //
    // Distances
    //
    t_pTag1 = p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST);
    t_pTag2 = t_pTag1 ? p_Tree->find_tag(stream, FIFF_MNE_SOURCE_SPACE_DIST_LIMIT) : std::nullopt;
    if (!t_pTag1 || !t_pTag2) {
        p_Hemisphere.dist = FiffSparseMatrix();
        p_Hemisphere.dist_limit = 0;
    } else {
        auto dist_lower = FiffSparseMatrix::fiff_get_float_sparse_matrix(*t_pTag1);
        if (dist_lower) {
            auto dist_full = dist_lower->mne_add_upper_triangle_rcs();
            if (dist_full) {
                p_Hemisphere.dist = std::move(*dist_full);
            }
        }
        p_Hemisphere.dist_limit = *t_pTag2->to_float(); //ToDo Check if this is realy always a float and not a matrix
    }

    return p_Hemisphere;
}

//=============================================================================================================

} // namespace

//=============================================================================================================

MNESourceSpaces::MNESourceSpaces()
{
}

//=============================================================================================================

MNESourceSpaces::MNESourceSpaces(const MNESourceSpaces& p_MNESourceSpaces)
{
    m_sourceSpaces.reserve(p_MNESourceSpaces.m_sourceSpaces.size());
    for (const auto& sp : p_MNESourceSpaces.m_sourceSpaces)
        m_sourceSpaces.push_back(sp->clone());
}

//=============================================================================================================

MNESourceSpaces& MNESourceSpaces::operator=(const MNESourceSpaces& other)
{
    if (this != &other) {
        MNESourceSpaces copy(other);
        m_sourceSpaces = std::move(copy.m_sourceSpaces);
    }
    return *this;
}

//=============================================================================================================

MNESourceSpaces::MNESourceSpaces(const std::filesystem::path& path)
: MNESourceSpaces(read(path).value())
{
}

//=============================================================================================================

MNESourceSpaces::~MNESourceSpaces()
{
}

//=============================================================================================================

void MNESourceSpaces::clear()
{
    m_sourceSpaces.clear();
}

//=============================================================================================================

void MNESourceSpaces::append(const MNESourceSpace& space)
{
    m_sourceSpaces.push_back(space.clone());
}

//=============================================================================================================

std::vector<VectorXi> MNESourceSpaces::get_vertno() const
{
    std::vector<VectorXi> p_vertices;
    p_vertices.reserve(m_sourceSpaces.size());
    for (const auto& space : m_sourceSpaces)
        p_vertices.push_back(space->vertno);
    return p_vertices;
}

//=============================================================================================================

std::vector<VectorXi> MNESourceSpaces::label_src_vertno_sel(const FsLabel& p_label, VectorXi& src_sel) const
{
    //    if(src[0].['type'] != 'surf')
    //        return Exception('FsLabel are only supported with surface source spaces')

    std::vector<VectorXi> vertno;
    vertno.push_back(this->m_sourceSpaces[0]->vertno);
    vertno.push_back(this->m_sourceSpaces[1]->vertno);

    if (p_label.hemi == 0) //lh
    {
        VectorXi vertno_sel = Linalg::intersect(vertno[0], p_label.vertices, src_sel);
        vertno[0] = vertno_sel;
        vertno[1] = VectorXi();
    } else if (p_label.hemi == 1) //rh
    {
        VectorXi vertno_sel = Linalg::intersect(vertno[1], p_label.vertices, src_sel);
        src_sel.array() += static_cast<int>(vertno[0].size());
        vertno[0] = VectorXi();
        vertno[1] = vertno_sel;
    }

    //    if (p_label.hemi == 0) //lh
    //    {
    //        VectorXi vertno_sel = Linalg::intersect(vertno[0], p_label.vertices[0], src_sel);
    //        vertno[0] = vertno_sel;
    //        vertno[1] = VectorXi();
    //    }
    //    else if (p_label.hemi == 1) //rh
    //    {
    //        VectorXi vertno_sel = Linalg::intersect(vertno[1], p_label.vertices[1], src_sel);
    //        src_sel.array() += p_label.vertices[0].size();
    //        vertno[0] = VectorXi();
    //        vertno[1] = vertno_sel;
    //    }
    //    else if (p_label.hemi == 2) //both
    //    {
    //        VectorXi src_sel_lh, src_sel_rh;
    //        VectorXi vertno_sel_lh = Linalg::intersect(vertno[0], p_label.vertices[0], src_sel_lh);
    //        VectorXi vertno_sel_rh = Linalg::intersect(vertno[1], p_label.vertices[1], src_sel_rh);
    //        src_sel.resize(src_sel_lh.size() + src_sel_rh.size());
    //        src_sel.block(0,0,src_sel_lh.size(),1) = src_sel_lh;
    //        src_sel.block(src_sel_lh.size(),0,src_sel_rh.size(),1) = src_sel_rh;
    //        vertno[0] = vertno_sel_lh;
    //        vertno[0] = vertno_sel_rh;
    //    }
    else {
        Log::warning("Unknown hemisphere type\n");
        vertno[0] = VectorXi::Zero(0);
        vertno[1] = VectorXi::Zero(0);
    }

    return vertno;
}

//=============================================================================================================

MNESourceSpaces MNESourceSpaces::pick_regions(const std::vector<FsLabel>& p_qListLabels) const
{
    MNESourceSpaces selectedSrc(*this);

    for (std::int32_t h = 0; h < 2; ++h) {
        auto& srcSpace = *selectedSrc.m_sourceSpaces[h];
        const auto& origSpace = *this->m_sourceSpaces[h];
        auto* selHemi = dynamic_cast<MNEHemisphere*>(&srcSpace);
        auto* origHemi = dynamic_cast<const MNEHemisphere*>(&origSpace);

        VectorXi selVertices;

        //get vertices indeces for new selection
        std::int32_t iSize = 0;
        for (const auto& label : p_qListLabels) {
            if (label.hemi == h) {
                VectorXi currentSelection;

                Linalg::intersect(origSpace.vertno, label.vertices, currentSelection);

                selVertices.conservativeResize(iSize + currentSelection.size());
                selVertices.block(iSize, 0, currentSelection.size(), 1) = currentSelection;
                iSize = selVertices.size();
            }
        }

        Linalg::sort(selVertices, false);

        VectorXi newVertno(selVertices.size());

        srcSpace.inuse = VectorXi::Zero(srcSpace.np);

        for (std::int32_t i = 0; i < selVertices.size(); ++i) {
            newVertno[i] = origSpace.vertno[selVertices[i]];
            srcSpace.inuse[newVertno[i]] = 1;
        }

        srcSpace.nuse = selVertices.size();
        srcSpace.vertno = newVertno;

        //
        // Tris
        //
        VectorXi idx_select = VectorXi::Zero(origSpace.use_itris.rows());
        for (std::int32_t i = 0; i < 3; ++i) {
            VectorXi tri_dim = origSpace.use_itris.col(i);
            VectorXi idx_dim;
            Linalg::intersect(tri_dim, newVertno, idx_dim);

            for (std::int32_t j = 0; j < idx_dim.size(); ++j)
                idx_select[idx_dim[j]] = 1;
        }

        std::int32_t countSel = 0;
        for (std::int32_t i = 0; i < idx_select.size(); ++i)
            if (idx_select[i] == 1)
                ++countSel;

        srcSpace.nuse_tri = countSel;

        MatrixX3i use_tris_new(countSel, 3);
        MatrixX3d use_tri_cent_new(countSel, 3);
        MatrixX3d use_tri_nn_new(countSel, 3);
        VectorXd use_tri_area_new(countSel);

        countSel = 0;
        for (std::int32_t i = 0; i < idx_select.size(); ++i) {
            if (idx_select[i] == 1) {
                use_tris_new.row(countSel) = origSpace.use_itris.row(i);
                if (origHemi && selHemi) {
                    use_tri_cent_new.row(countSel) = origHemi->use_tri_cent.row(i);
                    use_tri_nn_new.row(countSel) = origHemi->use_tri_nn.row(i);
                    use_tri_area_new[countSel] = origHemi->use_tri_area[i];
                }
                ++countSel;
            }
        }

        srcSpace.use_itris = use_tris_new;
        if (selHemi) {
            selHemi->use_tri_cent = use_tri_cent_new;
            selHemi->use_tri_nn = use_tri_nn_new;
            selHemi->use_tri_area = use_tri_area_new;
            // patch_inds is indexed by used source, so it must follow the selection
            if (origHemi && origHemi->patch_inds.size() == origSpace.nuse) {
                selHemi->patch_inds.resize(selVertices.size());
                for (std::int32_t i = 0; i < selVertices.size(); ++i)
                    selHemi->patch_inds[i] = origHemi->patch_inds[selVertices[i]];
            }
        }
    }

    return selectedSrc;
}

//=============================================================================================================

Expected<MNESourceSpaces> MNESourceSpaces::read(const std::filesystem::path& path, bool add_geom)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    return read(*stream, add_geom);
}

//=============================================================================================================

Expected<MNESourceSpaces> MNESourceSpaces::read(FiffStream& stream, bool add_geom)
{
    const std::string name = stream.stream_name();
    if (!stream.is_open() && !stream.open()) {
        return fail(ErrorCode::Io, "cannot open {}", name);
    }
    const std::vector<std::shared_ptr<FiffDirNode>> nodes = stream.dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
    if (nodes.empty()) {
        return fail(ErrorCode::NotFound, "no source spaces in {}", name);
    }

    MNESourceSpaces spaces;
    spaces.m_sourceSpaces.reserve(nodes.size());
    for (const std::shared_ptr<FiffDirNode>& node : nodes) {
        auto hemisphere = read_source_space(stream, node, name);
        if (!hemisphere) {
            return std::unexpected(hemisphere.error());
        }
        if (add_geom) {
            hemisphere->complete_source_space_info();
        }
        spaces.m_sourceSpaces.push_back(std::make_shared<MNEHemisphere>(std::move(*hemisphere)));
    }
    Log::info("\t{} source spaces read", nodes.size());
    return spaces;
}

//=============================================================================================================

Expected<void> MNESourceSpaces::write(const std::filesystem::path& path) const
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

bool MNESourceSpaces::transform_source_space_to(fiff_int_t dest, FiffCoordTrans& trans)
{
    for (size_t k = 0; k < this->m_sourceSpaces.size(); ++k) {
        auto* hemi = dynamic_cast<MNEHemisphere*>(m_sourceSpaces[k].get());
        if (hemi) {
            if (!hemi->transform_hemisphere_to(dest, trans)) {
                Log::warning("Could not transform source space.");
                return false;
            }
        }
    }
    return true;
}

//=============================================================================================================

void MNESourceSpaces::write_to_stream(FiffStream& stream) const
{
    for (size_t h = 0; h < m_sourceSpaces.size(); ++h) {
        Log::info("\tWrite a source space... ");
        stream.start_block(FIFFB_MNE_SOURCE_SPACE);
        const auto* hemi = dynamic_cast<const MNEHemisphere*>(m_sourceSpaces[h].get());
        if (hemi)
            hemi->writeToStream(stream);
        stream.end_block(FIFFB_MNE_SOURCE_SPACE);
        Log::info("[done]\n");
    }
    Log::info("\t{} source spaces written\n", m_sourceSpaces.size());
}

//=============================================================================================================

MNESourceSpace& MNESourceSpaces::operator[](std::int32_t idx)
{
    if (static_cast<std::int32_t>(m_sourceSpaces.size()) > idx)
        return *m_sourceSpaces[idx];
    else {
        Log::warning("Warning: Index out of bound! Returning last element.");
        return *m_sourceSpaces.back();
    }
}

//=============================================================================================================

const MNESourceSpace& MNESourceSpaces::operator[](std::int32_t idx) const
{
    if (static_cast<std::int32_t>(m_sourceSpaces.size()) > idx)
        return *m_sourceSpaces[idx];
    else {
        Log::warning("Warning: Index out of bound! Returning last element.");
        return *m_sourceSpaces.back();
    }
}

//=============================================================================================================

MNESourceSpace& MNESourceSpaces::operator[](std::string_view idt)
{
    if (idt.compare("lh") == 0)
        return *m_sourceSpaces[0];
    else if (idt.compare("rh") == 0)
        return *m_sourceSpaces[1];
    else {
        Log::warning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return *m_sourceSpaces[0];
    }
}

//=============================================================================================================

const MNESourceSpace& MNESourceSpaces::operator[](std::string_view idt) const
{
    if (idt.compare("lh") == 0)
        return *m_sourceSpaces[0];
    else if (idt.compare("rh") == 0)
        return *m_sourceSpaces[1];
    else {
        Log::warning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return *m_sourceSpaces[0];
    }
}

//=============================================================================================================

MNEHemisphere* MNESourceSpaces::hemisphereAt(std::int32_t idx)
{
    if (idx >= 0 && idx < static_cast<std::int32_t>(m_sourceSpaces.size()))
        return dynamic_cast<MNEHemisphere*>(m_sourceSpaces[idx].get());
    return nullptr;
}

//=============================================================================================================

const MNEHemisphere* MNESourceSpaces::hemisphereAt(std::int32_t idx) const
{
    if (idx >= 0 && idx < static_cast<std::int32_t>(m_sourceSpaces.size()))
        return dynamic_cast<const MNEHemisphere*>(m_sourceSpaces[idx].get());
    return nullptr;
}

//=============================================================================================================

std::shared_ptr<MNESourceSpace>& MNESourceSpaces::at(std::int32_t idx)
{
    return m_sourceSpaces.at(idx);
}

//=============================================================================================================

const std::shared_ptr<MNESourceSpace>& MNESourceSpaces::at(std::int32_t idx) const
{
    return m_sourceSpaces.at(idx);
}
