//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fs_label.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Implementation of @ref mne::FsLabel: parses FreeSurfer/MNE ASCII .label files into vertex / position / value arrays.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_label.h"
#include <mne/base/log.h>
#include "fs_surface.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsLabel::FsLabel()
: hemi(-1)
, label_id(-1)
{
}

//=============================================================================================================

FsLabel::FsLabel(const VectorXi& p_vertices,
                 const MatrixX3f& p_pos,
                 const VectorXd& p_values,
                 std::int32_t p_hemi,
                 const std::string& p_name,
                 std::int32_t p_id)
: vertices(p_vertices)
, pos(p_pos)
, values(p_values)
, hemi(p_hemi)
, name(p_name)
, label_id(p_id)
{
}

//=============================================================================================================

FsLabel::~FsLabel()
{
}

//=============================================================================================================

void FsLabel::clear()
{
    comment.clear();
    hemi = -1;
    name.clear();
    vertices = VectorXi();
    pos = MatrixX3f(0, 3);
    values = VectorXd();

    label_id = -1;
}

//=============================================================================================================

MatrixX3i FsLabel::selectTris(const FsSurface& p_Surface)
{
    return this->selectTris(p_Surface.tris());
}

//=============================================================================================================

MatrixX3i FsLabel::selectTris(const MatrixX3i& p_matTris)
{
    //check whether there are data to create the tris
    if (this->vertices.size() == 0)
        return MatrixX3i(0, 3);

    MatrixX3i tris(p_matTris.rows(), 3);

    const std::unordered_set<int> verts(this->vertices.data(), this->vertices.data() + this->vertices.size());

    //
    // Search for all the tris where is at least one corner part of the label
    //
    std::int32_t t_size = 0;
    for (std::int32_t i = 0; i < p_matTris.rows(); ++i) {
        if (verts.contains(p_matTris(i, 0)) || verts.contains(p_matTris(i, 1)) || verts.contains(p_matTris(i, 2))) {
            tris.row(t_size) = p_matTris.row(i);
            ++t_size;
        }
    }

    tris.conservativeResize(t_size, 3);

    return tris;
}

//=============================================================================================================

bool FsLabel::read(const std::filesystem::path& p_sFileName, FsLabel& p_Label)
{
    p_Label.clear();

    const std::string fileName = mne::utf8_from_path(p_sFileName.filename());
    if (!fileName.ends_with(".label")) {
        mne::Log::warning("Given file ({}) is not a .label file!\n", mne::utf8_from_path(p_sFileName));
        return false;
    }

    mne::Log::info("Reading label...");
    std::ifstream file(p_sFileName);
    if (!file) {
        mne::Log::warning("\tError: Couldn't open the label file\n");
        return false;
    }

    std::string comment;
    std::string line;
    std::getline(file, comment);
    std::getline(file, line);
    const int nv = mne::to_int(line).value_or(0);

    // One row per vertex: vertex number, x, y, z (mm) and value
    MatrixXd data = MatrixXd::Zero(nv, 5);
    for (std::int32_t i = 0; i < nv && std::getline(file, line); ++i) {
        std::istringstream fields(line);
        for (std::int32_t j = 0; j < 5 && fields >> data(i, j); ++j) {
        }
    }

    p_Label.comment = comment.empty() ? comment : comment.substr(1);
    // As mne.read_label
    p_Label.hemi = fileName.ends_with("lh.label") || fileName.starts_with("lh.") ? 0 : 1;
    p_Label.vertices = data.col(0).cast<int>();
    p_Label.pos = data.middleCols(1, 3).cast<float>().array() * 1e-3f;
    p_Label.values = data.col(4);

    // <name>-lh.label or lh.<name>.label
    if (const std::size_t at = fileName.find("-lh.label"); at != std::string::npos)
        p_Label.name = fileName.substr(0, at);
    else if (const std::size_t rh = fileName.find("-rh.label"); rh != std::string::npos)
        p_Label.name = fileName.substr(0, rh);
    else if (fileName.size() > 3 + 6 && (fileName.starts_with("lh.") || fileName.starts_with("rh.")))
        p_Label.name = fileName.substr(3, fileName.size() - 3 - 6);

    mne::Log::info("[done]\n");

    return true;
}
