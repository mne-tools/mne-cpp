//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_hemisphere.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref mne::MNEHemisphere.
 *
 * Implements FIFF read of one @c FIFFB_MNE_SOURCE_SPACE block into the
 * rendering-ready hemisphere representation, recomputing triangle normals
 * and centroids when the FIFF file omits them and attaching patch
 * information from a companion @c .pri file when available.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_hemisphere.h"

#include "mne_nearest.h"
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>
#include <mne/math/linalg.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
namespace
{

// Chunks of a loop over n items for ThreadPool::parallel_for; small loops stay on the calling thread
std::size_t chunk_count(int n)
{
    return static_cast<std::size_t>(std::clamp(n / 4096, 1, 64));
}

// Centroid, normal (unit length if normalize) and area of the first n triangles, in parallel chunks
void triangle_geometry(const MNESurfaceOrVolume::PointsT& rr,
                       const MNESurfaceOrVolume::TrianglesT& tris,
                       int n,
                       MatrixX3d& cent,
                       MatrixX3d& nn,
                       VectorXd& area,
                       bool normalize)
{
    cent = MatrixX3d::Zero(n, 3);
    nn = MatrixX3d::Zero(n, 3);
    area = VectorXd::Zero(n);
    const std::size_t chunks = chunk_count(n);
    ThreadPool::global().parallel_for(0, chunks, [&](std::size_t chunk) {
        const int first = static_cast<int>(chunk * static_cast<std::size_t>(n) / chunks);
        const int last = static_cast<int>((chunk + 1) * static_cast<std::size_t>(n) / chunks);
        Matrix3d r;
        Vector3d a, b;
        for (int i = first; i < last; ++i) {
            for (int j = 0; j < 3; ++j) {
                const int k = tris(i, j);
                r(j, 0) = rr(k, 0);
                r(j, 1) = rr(k, 1);
                r(j, 2) = rr(k, 2);
                cent(i, 0) += rr(k, 0);
                cent(i, 1) += rr(k, 1);
                cent(i, 2) += rr(k, 2);
            }
            cent.row(i) /= 3.0f;

            // cross((r2 - r1), (r3 - r1))
            a = (r.row(1) - r.row(0)).transpose();
            b = (r.row(2) - r.row(0)).transpose();
            nn(i, 0) = a(1) * b(2) - a(2) * b(1);
            nn(i, 1) = a(2) * b(0) - a(0) * b(2);
            nn(i, 2) = a(0) * b(1) - a(1) * b(0);

            float size = nn.row(i) * nn.row(i).transpose();
            size = std::pow(size, 0.5f);
            area(i) = size / 2.0f;
            if (normalize)
                nn.row(i) /= size;
        }
    });
}

} // namespace

//=============================================================================================================

MNEHemisphere::MNEHemisphere()
: MNESourceSpace(0)
, patch_inds(VectorXi::Zero(0))
, tri_cent(MatrixX3d::Zero(0, 3))
, tri_nn(MatrixX3d::Zero(0, 3))
, tri_area(VectorXd::Zero(0))
, use_tri_cent(MatrixX3d::Zero(0, 3))
, use_tri_nn(MatrixX3d::Zero(0, 3))
, use_tri_area(VectorXd::Zero(0))
{
    // Override some base class defaults to match MNEHemisphere semantics
    this->type = 1;
    this->id = -1;
    this->np = -1;
    this->ntri = -1;
    this->coord_frame = -1;
    this->nuse = -1;
    this->nuse_tri = -1;
    this->dist_limit = -1;
}

//=============================================================================================================

MNEHemisphere::MNEHemisphere(const MNEHemisphere& p_MNEHemisphere)
: MNESourceSpace(0)
, pinfo(p_MNEHemisphere.pinfo)
, patch_inds(p_MNEHemisphere.patch_inds)
, tri_cent(p_MNEHemisphere.tri_cent)
, tri_nn(p_MNEHemisphere.tri_nn)
, tri_area(p_MNEHemisphere.tri_area)
, use_tri_cent(p_MNEHemisphere.use_tri_cent)
, use_tri_nn(p_MNEHemisphere.use_tri_nn)
, use_tri_area(p_MNEHemisphere.use_tri_area)
, cluster_info(p_MNEHemisphere.cluster_info)
{
    // Copy base class (MNESurfaceOrVolume) fields that MNEHemisphere uses
    this->type = p_MNEHemisphere.type;
    this->id = p_MNEHemisphere.id;
    this->np = p_MNEHemisphere.np;
    this->ntri = p_MNEHemisphere.ntri;
    this->coord_frame = p_MNEHemisphere.coord_frame;
    this->rr = p_MNEHemisphere.rr;
    this->nn = p_MNEHemisphere.nn;
    this->nuse = p_MNEHemisphere.nuse;
    this->inuse = p_MNEHemisphere.inuse;
    this->vertno = p_MNEHemisphere.vertno;
    this->itris = p_MNEHemisphere.itris;
    this->use_itris = p_MNEHemisphere.use_itris;
    this->nuse_tri = p_MNEHemisphere.nuse_tri;
    this->dist_limit = p_MNEHemisphere.dist_limit;
    this->dist = p_MNEHemisphere.dist;
    this->nearest = p_MNEHemisphere.nearest;
    this->neighbor_tri = p_MNEHemisphere.neighbor_tri;
    this->neighbor_vert = p_MNEHemisphere.neighbor_vert;
}

//=============================================================================================================

MNEHemisphere::MNEHemisphere(MNEHemisphere&& other) noexcept
: MNESourceSpace(0)
{
    *this = std::move(other);
}

//=============================================================================================================

MNEHemisphere& MNEHemisphere::operator=(MNEHemisphere&& other) noexcept
{
    if (this != &other) {
        // The fields the copy operations copy
        this->type = other.type;
        this->id = other.id;
        this->np = other.np;
        this->ntri = other.ntri;
        this->coord_frame = other.coord_frame;
        this->rr = std::move(other.rr);
        this->nn = std::move(other.nn);
        this->nuse = other.nuse;
        this->inuse = std::move(other.inuse);
        this->vertno = std::move(other.vertno);
        this->itris = std::move(other.itris);
        this->use_itris = std::move(other.use_itris);
        this->nuse_tri = other.nuse_tri;
        this->dist_limit = other.dist_limit;
        this->dist = std::move(other.dist);
        this->nearest = std::move(other.nearest);
        this->neighbor_tri = std::move(other.neighbor_tri);
        this->neighbor_vert = std::move(other.neighbor_vert);
        this->pinfo = std::move(other.pinfo);
        this->patch_inds = std::move(other.patch_inds);
        this->tri_cent = std::move(other.tri_cent);
        this->tri_nn = std::move(other.tri_nn);
        this->tri_area = std::move(other.tri_area);
        this->use_tri_cent = std::move(other.use_tri_cent);
        this->use_tri_nn = std::move(other.use_tri_nn);
        this->use_tri_area = std::move(other.use_tri_area);
        this->cluster_info = std::move(other.cluster_info);
    }
    return *this;
}

//=============================================================================================================

MNEHemisphere& MNEHemisphere::operator=(const MNEHemisphere& other)
{
    if (this != &other) {
        // Copy base class (MNESurfaceOrVolume) fields
        this->type = other.type;
        this->id = other.id;
        this->np = other.np;
        this->ntri = other.ntri;
        this->coord_frame = other.coord_frame;
        this->rr = other.rr;
        this->nn = other.nn;
        this->nuse = other.nuse;
        this->inuse = other.inuse;
        this->vertno = other.vertno;
        this->nuse_tri = other.nuse_tri;
        this->dist_limit = other.dist_limit;
        this->neighbor_tri = other.neighbor_tri;
        this->neighbor_vert = other.neighbor_vert;

        // Copy triangle index fields (inherited)
        this->itris = other.itris;
        this->use_itris = other.use_itris;

        // Copy inherited fields with value semantics
        this->dist = other.dist;
        this->nearest = other.nearest;

        // Copy MNEHemisphere fields
        this->pinfo = other.pinfo;
        this->patch_inds = other.patch_inds;
        this->tri_cent = other.tri_cent;
        this->tri_nn = other.tri_nn;
        this->tri_area = other.tri_area;
        this->use_tri_cent = other.use_tri_cent;
        this->use_tri_nn = other.use_tri_nn;
        this->use_tri_area = other.use_tri_area;
        this->cluster_info = other.cluster_info;
    }
    return *this;
}

//=============================================================================================================

MNEHemisphere::~MNEHemisphere()
{
}

//=============================================================================================================

std::shared_ptr<MNESourceSpace> MNEHemisphere::clone() const
{
    return std::make_shared<MNEHemisphere>(*this);
}

//=============================================================================================================

bool MNEHemisphere::complete_source_space_info()
{
    //
    //   Main triangulation
    //
    Log::info("\tCompleting triangulation info...");
    triangle_geometry(rr, itris, ntri, tri_cent, tri_nn, tri_area, true);
    Log::info("[done]\n");

    //
    //   Selected triangles
    //
    Log::info("\tCompleting selection triangulation info...");
    if (nuse_tri > 0) {
        triangle_geometry(rr, use_itris, nuse_tri, use_tri_cent, use_tri_nn, use_tri_area, false);
    }
    Log::info("[done]\n");

    Log::info("\tCompleting triangle and vertex neighboring info...");
    add_geometry_info();
    Log::info("[done]\n");

    return true;
}

//=============================================================================================================

bool MNEHemisphere::compute_patch_info()
{
    if (nearest.empty()) {
        pinfo.clear();
        patch_inds = VectorXi();
        return false;
    }

    Log::info("\tComputing patch statistics...");

    std::vector<std::pair<int, int>> t_vIndn;

    for (size_t i = 0; i < nearest.size(); ++i) {
        std::pair<int, int> t_pair(static_cast<int>(i), nearest[i].nearest);
        t_vIndn.push_back(t_pair);
    }
    std::sort(t_vIndn.begin(), t_vIndn.end(), Linalg::compareIdxValuePairSmallerThan<int>);

    VectorXi nearest_sorted(t_vIndn.size());

    int current = 0;
    std::vector<int> t_vfirsti;
    t_vfirsti.push_back(current);
    std::vector<int> t_vlasti;

    for (int i = 0; i < static_cast<int>(t_vIndn.size()); ++i) {
        nearest_sorted[i] = t_vIndn[i].second;
        if (t_vIndn[current].second != t_vIndn[i].second) {
            current = i;
            t_vlasti.push_back(i - 1);
            t_vfirsti.push_back(current);
        }
    }
    t_vlasti.push_back(static_cast<int>(t_vIndn.size() - 1));

    for (int k = 0; k < static_cast<int>(t_vfirsti.size()); ++k) {
        Eigen::VectorXi t_vIndex(t_vlasti[k] - t_vfirsti[k] + 1);

        for (int l = t_vfirsti[k]; l <= t_vlasti[k]; ++l)
            t_vIndex[l - t_vfirsti[k]] = t_vIndn[l].first;

        std::sort(t_vIndex.data(), t_vIndex.data() + t_vIndex.size());

        pinfo.push_back(t_vIndex);
    }

    // compute patch indices of the in-use source space vertices
    Eigen::VectorXi patch_verts(t_vlasti.size());
    for (int i = 0; i < static_cast<int>(t_vlasti.size()); ++i)
        patch_verts[i] = nearest_sorted[t_vlasti[i]];

    patch_inds.resize(vertno.size());
    for (int i = 0; i < vertno.size(); ++i) {
        const int* ptr = std::find(patch_verts.data(), patch_verts.data() + patch_verts.size(), vertno[i]);
        patch_inds[i] = static_cast<int>(ptr - patch_verts.data());
    }

    return true;
}

//=============================================================================================================

bool MNEHemisphere::add_geometry_info()
{
    // The triangles of each vertex in increasing order; like the triangle list, it has ntri entries
    const auto ntris = static_cast<int>(this->itris.rows());
    std::vector<int> counts(static_cast<std::size_t>(ntris), 0);
    for (int t = 0; t < ntris; ++t)
        for (int corner = 0; corner < 3; ++corner)
            ++counts[static_cast<std::size_t>(this->itris(t, corner))];
    neighbor_tri.assign(static_cast<std::size_t>(ntris), Eigen::VectorXi());
    for (int vertex = 0; vertex < ntris; ++vertex)
        neighbor_tri[static_cast<std::size_t>(vertex)].resize(counts[static_cast<std::size_t>(vertex)]);
    std::ranges::fill(counts, 0);
    for (int t = 0; t < ntris; ++t) {
        for (int corner = 0; corner < 3; ++corner) {
            const auto vertex = static_cast<std::size_t>(this->itris(t, corner));
            neighbor_tri[vertex][counts[vertex]++] = t;
        }
    }

    // The neighboring vertices of each vertex, in the order its triangles name them; vertices are independent
    neighbor_vert.resize(this->np);
    const std::size_t chunks = chunk_count(this->np);
    ThreadPool::global().parallel_for(0, chunks, [&](std::size_t chunk) {
        const int first = static_cast<int>(chunk * static_cast<std::size_t>(this->np) / chunks);
        const int last = static_cast<int>((chunk + 1) * static_cast<std::size_t>(this->np) / chunks);
        std::vector<int> verts;
        for (int vertex = first; vertex < last; ++vertex) {
            verts.clear();
            for (Eigen::Index t = 0; t < neighbor_tri[vertex].size(); ++t) {
                for (int corner = 0; corner < 3; ++corner) {
                    const int vert = this->itris(neighbor_tri[vertex][t], corner);
                    if (vert != vertex && std::find(verts.begin(), verts.end(), vert) == verts.end())
                        verts.push_back(vert);
                }
            }
            neighbor_vert[vertex] = Eigen::Map<Eigen::VectorXi>(verts.data(), static_cast<Eigen::Index>(verts.size()));
        }
    });

    return true;
}

//=============================================================================================================

void MNEHemisphere::clear()
{
    // Reset base class fields
    type = 1;
    id = -1;
    np = -1;
    ntri = -1;
    coord_frame = -1;
    rr = PointsT::Zero(0, 3);
    nn = NormalsT::Zero(0, 3);
    nuse = -1;
    inuse = VectorXi::Zero(0);
    vertno = VectorXi::Zero(0);
    nuse_tri = -1;
    dist_limit = -1;
    neighbor_tri.clear();
    neighbor_vert.clear();

    // Reset inherited value-semantic fields
    itris = TrianglesT::Zero(0, 3);
    use_itris = TrianglesT::Zero(0, 3);
    dist = FiffSparseMatrix();
    nearest.clear();

    // Reset MNEHemisphere fields
    pinfo.clear();
    patch_inds = VectorXi::Zero(0);
    tri_cent = MatrixX3d::Zero(0, 3);
    tri_nn = MatrixX3d::Zero(0, 3);
    tri_area = VectorXd::Zero(0);
    use_tri_cent = MatrixX3d::Zero(0, 3);
    use_tri_nn = MatrixX3d::Zero(0, 3);
    use_tri_area = VectorXd::Zero(0);

    cluster_info.clear();
}

//=============================================================================================================

bool MNEHemisphere::transform_hemisphere_to(fiff_int_t dest, const FiffCoordTrans& p_Trans)
{
    FiffCoordTrans trans(p_Trans);

    if (this->coord_frame == dest) {
        //            res = src;
        return true;
    }

    if (trans.to == this->coord_frame && trans.from == dest)
        trans.invert_transform();
    else if (trans.from != this->coord_frame || trans.to != dest) {
        Log::warning("Cannot transform the source space using this coordinate transformation"); //Consider throw
        return false;
    }

    MatrixXf t = trans.trans.block(0, 0, 3, 4);
    //        res             = src;
    this->coord_frame = dest;
    MatrixXf t_rr = MatrixXf::Ones(this->np, 4);
    t_rr.block(0, 0, this->np, 3) = this->rr;
    MatrixXf t_nn = MatrixXf::Zero(this->np, 4);
    t_nn.block(0, 0, this->np, 3) = this->nn;

    this->rr = (t * t_rr.transpose()).transpose();
    this->nn = (t * t_nn.transpose()).transpose();

    return true;
}

//=============================================================================================================
//ToDo
void MNEHemisphere::writeToStream(FiffStream& stream) const
{
    if (this->type == 1 || this->type == 2)
        stream.write_int(FIFF_MNE_SOURCE_SPACE_TYPE, &this->type);
    else
        Log::warning("Unknown source space type ({})", this->type);
    stream.write_int(FIFF_MNE_SOURCE_SPACE_ID, &this->id);

    //    data = this.get('subject_his_id', None)
    //    if data:
    //        write_string(fid, FIFF.FIFF_SUBJ_HIS_ID, data)
    stream.write_int(FIFF_MNE_COORD_FRAME, &this->coord_frame);

    if (this->type == 2) //2 = Vol
    {
        Log::debug("ToDo: Write Volume not implemented yet!!!!!!!!");
        //        stream.write_int(FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, this->shape)
        //        stream.write_coord_trans(this->src_mri_t);

        stream.start_block(FIFFB_MNE_PARENT_MRI_FILE);
        //        write_coord_trans(fid, this['vox_mri_t'])

        //        write_coord_trans(fid, this['mri_ras_t'])

        //        write_float_sparse_rcs(fid, FIFF.FIFF_MNE_SOURCE_SPACE_INTERPOLATOR,
        //                            this['interpolator'])

        //        if 'mri_file' in this and this['mri_file'] is not None:
        //            write_string(fid, FIFF.FIFF_MNE_SOURCE_SPACE_MRI_FILE,
        //                         this['mri_file'])

        //        write_int(fid, FIFF.FIFF_MRI_WIDTH, this['mri_width'])
        //        write_int(fid, FIFF.FIFF_MRI_HEIGHT, this['mri_height'])
        //        write_int(fid, FIFF.FIFF_MRI_DEPTH, this['mri_depth'])

        stream.end_block(FIFFB_MNE_PARENT_MRI_FILE);
    }

    stream.write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &this->np);
    stream.write_float_matrix(FIFF_MNE_SOURCE_SPACE_POINTS, this->rr);
    stream.write_float_matrix(FIFF_MNE_SOURCE_SPACE_NORMALS, this->nn);

    //   Which vertices are active
    stream.write_int(FIFF_MNE_SOURCE_SPACE_SELECTION, this->inuse.data(), this->inuse.size());
    stream.write_int(FIFF_MNE_SOURCE_SPACE_NUSE, &this->nuse);

    stream.write_int(FIFF_MNE_SOURCE_SPACE_NTRI, &this->ntri);
    if (this->ntri > 0)
        stream.write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, (this->itris.array() + 1).matrix());

    if (this->type != 2 && this->use_itris.rows() > 0) {
        //   Use triangulation
        stream.write_int(FIFF_MNE_SOURCE_SPACE_NUSE_TRI, &this->nuse_tri);
        stream.write_int_matrix(FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, (this->use_itris.array() + 1).matrix());
    }

    //   Patch-related information
    if (!this->nearest.empty()) {
        Eigen::VectorXi nearestIdx = this->nearestVertIdx();
        Eigen::VectorXf nearestDistF = this->nearestDistVec().cast<float>();
        stream.write_int(FIFF_MNE_SOURCE_SPACE_NEAREST, nearestIdx.data(), nearestIdx.size());
        stream.write_float(FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, nearestDistF.data(), nearestDistF.size());
    }

    //   Distances
    if (!this->dist.empty()) {
        // Extract upper triangular portion from the dist matrix
        const Eigen::SparseMatrix<float>& eigenDist = this->dist.eigen();
        typedef Eigen::Triplet<float> T;
        std::vector<T> tripletList;
        tripletList.reserve(eigenDist.nonZeros());
        for (int k = 0; k < eigenDist.outerSize(); ++k)
            for (Eigen::SparseMatrix<float>::InnerIterator it(eigenDist, k); it; ++it)
                if (it.col() >= it.row()) //only upper triangle -> todo iteration can be optimized
                    tripletList.push_back(T(it.row(), it.col(), it.value()));
        Eigen::SparseMatrix<float> dists(eigenDist.rows(), eigenDist.cols());
        dists.setFromTriplets(tripletList.begin(), tripletList.end());

        stream.write_float_sparse_rcs(FIFF_MNE_SOURCE_SPACE_DIST, dists);
        //ToDo check if write_float_matrix or write float is okay
        stream.write_float(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, &this->dist_limit); //stream.write_float_matrix(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, this->dist_limit);
    }
}
