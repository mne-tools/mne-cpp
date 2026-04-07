//=============================================================================================================
/**
 * @file     mne_hemisphere.cpp
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
 * @brief     MNEHemisphere class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_hemisphere.h"
#include "mne_nearest.h"

#include <math/linalg.h>

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEHemisphere::MNEHemisphere()
: MNESourceSpace(0)
, patch_inds(VectorXi::Zero(0))
, tri_cent(MatrixX3d::Zero(0,3))
, tri_nn(MatrixX3d::Zero(0,3))
, tri_area(VectorXd::Zero(0))
, use_tri_cent(MatrixX3d::Zero(0,3))
, use_tri_nn(MatrixX3d::Zero(0,3))
, use_tri_area(VectorXd::Zero(0))
//, m_TriCoords()
//, m_pGeometryData(NULL)
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
, m_TriCoords(p_MNEHemisphere.m_TriCoords)
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
        this->m_TriCoords = other.m_TriCoords;
    }
    return *this;
}

//=============================================================================================================

MNEHemisphere::~MNEHemisphere()
{
}

//=============================================================================================================

MNESourceSpace::SPtr MNEHemisphere::clone() const
{
    return std::make_shared<MNEHemisphere>(*this);
}

//=============================================================================================================

bool MNEHemisphere::complete_source_space_info()
{
    //
    //   Main triangulation
    //
    qInfo("\tCompleting triangulation info...");
    tri_cent = MatrixX3d::Zero(ntri,3);
    tri_nn = MatrixX3d::Zero(ntri,3);
    tri_area = VectorXd::Zero(ntri);

    Matrix3d r;
    Vector3d a, b;
    int k = 0;
    float size = 0;
    for (int i = 0; i < ntri; ++i)
    {
        for ( int j = 0; j < 3; ++j)
        {
            k = itris(i, j);

            r(j,0) = rr(k, 0);
            r(j,1) = rr(k, 1);
            r(j,2) = rr(k, 2);

            tri_cent(i, 0) += rr(k, 0);
            tri_cent(i, 1) += rr(k, 1);
            tri_cent(i, 2) += rr(k, 2);
        }
        tri_cent.row(i) /= 3.0f;

        //cross product {cross((r2-r1),(r3-r1))}
        a = (r.row(1) - r.row(0 )).transpose();
        b = (r.row(2) - r.row(0)).transpose();
        tri_nn(i,0) = a(1)*b(2)-a(2)*b(1);
        tri_nn(i,1) = a(2)*b(0)-a(0)*b(2);
        tri_nn(i,2) = a(0)*b(1)-a(1)*b(0);

        //area
        size = tri_nn.row(i)*tri_nn.row(i).transpose();
        size = std::pow(size, 0.5f );

        tri_area(i) = size/2.0f;
        tri_nn.row(i) /= size;

    }
    qInfo("[done]\n");

    //
    //   Selected triangles
    //
    qInfo("\tCompleting selection triangulation info...");
    if (nuse_tri > 0)
    {
        use_tri_cent = MatrixX3d::Zero(nuse_tri,3);
        use_tri_nn = MatrixX3d::Zero(nuse_tri,3);
        use_tri_area = VectorXd::Zero(nuse_tri);

        for (int i = 0; i < nuse_tri; ++i)
        {
            for ( int j = 0; j < 3; ++j)
            {
                k = use_itris(i, j);

                r(j,0) = rr(k, 0);
                r(j,1) = rr(k, 1);
                r(j,2) = rr(k, 2);

                use_tri_cent(i, 0) += rr(k, 0);
                use_tri_cent(i, 1) += rr(k, 1);
                use_tri_cent(i, 2) += rr(k, 2);
            }
            use_tri_cent.row(i) /= 3.0f;

            //cross product {cross((r2-r1),(r3-r1))}
            a = r.row(1) - r.row(0 );
            b = r.row(2) - r.row(0);
            use_tri_nn(i,0) = a(1)*b(2)-a(2)*b(1);
            use_tri_nn(i,1) = a(2)*b(0)-a(0)*b(2);
            use_tri_nn(i,2) = a(0)*b(1)-a(1)*b(0);

            //area
            size = use_tri_nn.row(i)*use_tri_nn.row(i).transpose();
            size = std::pow(size, 0.5f );

            use_tri_area(i) = size/2.0f;
        }

    }
    qInfo("[done]\n");

    qInfo("\tCompleting triangle and vertex neighboring info...");
    add_geometry_info();
    qInfo("[done]\n");

    return true;
}

//=============================================================================================================

bool MNEHemisphere::compute_patch_info()
{
    if (nearest.empty())
    {
       pinfo.clear();
       patch_inds = VectorXi();
       return false;
    }

    qInfo("\tComputing patch statistics...");

    std::vector< std::pair<int,int> > t_vIndn;

    for(size_t i = 0; i < nearest.size(); ++i)
    {
        std::pair<int,int> t_pair(static_cast<int>(i), nearest[i].nearest);
        t_vIndn.push_back(t_pair);
    }
    std::sort(t_vIndn.begin(),t_vIndn.end(), Linalg::compareIdxValuePairSmallerThan<int> );

    VectorXi nearest_sorted(t_vIndn.size());

    int current = 0;
    std::vector<int> t_vfirsti;
    t_vfirsti.push_back(current);
    std::vector<int> t_vlasti;

    for(int i = 0; i < static_cast<int>(t_vIndn.size()); ++i)
    {
        nearest_sorted[i] = t_vIndn[i].second;
        if (t_vIndn[current].second != t_vIndn[i].second)
        {
            current = i;
            t_vlasti.push_back(i-1);
            t_vfirsti.push_back(current);
        }
    }
    t_vlasti.push_back(static_cast<int>(t_vIndn.size()-1));

    for(int k = 0; k < static_cast<int>(t_vfirsti.size()); ++k)
    {
        Eigen::VectorXi t_vIndex(t_vlasti[k] - t_vfirsti[k] + 1);

        for(int l = t_vfirsti[k]; l <= t_vlasti[k]; ++l)
            t_vIndex[l - t_vfirsti[k]] = t_vIndn[l].first;

        std::sort(t_vIndex.data(), t_vIndex.data() + t_vIndex.size());

        pinfo.append(t_vIndex);
    }

    // compute patch indices of the in-use source space vertices
    Eigen::VectorXi patch_verts(t_vlasti.size());
    for(int i = 0; i < static_cast<int>(t_vlasti.size()); ++i)
        patch_verts[i] = nearest_sorted[t_vlasti[i]];

    patch_inds.resize(vertno.size());
    for(int i = 0; i < vertno.size(); ++i)
    {
        const int* ptr = std::find(patch_verts.data(), patch_verts.data() + patch_verts.size(), vertno[i]);
        patch_inds[i] = static_cast<int>(ptr - patch_verts.data());
    }

    return true;
}

//=============================================================================================================

bool MNEHemisphere::add_geometry_info()
{
    int k,c,p,q;
    bool found;

    //Create neighboring triangle vector using temporary std::vector for efficient appending
    {
        std::vector<std::vector<int>> temp_ntri(this->itris.rows());
        for (p = 0; p < this->itris.rows(); p++) {
            for (k = 0; k < 3; k++) {
                temp_ntri[this->itris(p,k)].push_back(p);
            }
        }
        neighbor_tri.resize(this->itris.rows());
        for (k = 0; k < static_cast<int>(temp_ntri.size()); k++) {
            neighbor_tri[k] = Eigen::Map<Eigen::VectorXi>(temp_ntri[k].data(), temp_ntri[k].size());
        }
    }

    //Create the neighboring vertices vector using temporary std::vector
    {
        std::vector<std::vector<int>> temp_nvert(this->np);
        for (k = 0; k < this->np; k++) {
            for (p = 0; p < neighbor_tri[k].size(); p++) {
                //Fit in the other vertices of the neighboring triangle
                for (c = 0; c < 3; c++) {
                    int vert = this->itris(neighbor_tri[k][p], c);

                    if (vert != k) {
                        found = false;

                        for (q = 0; q < static_cast<int>(temp_nvert[k].size()); q++) {
                            if (temp_nvert[k][q] == vert) {
                                found = true;
                                break;
                            }
                        }

                        if(!found) {
                            temp_nvert[k].push_back(vert);
                        }
                    }
                }
            }
        }
        neighbor_vert.resize(this->np);
        for (k = 0; k < this->np; k++) {
            neighbor_vert[k] = Eigen::Map<Eigen::VectorXi>(temp_nvert[k].data(), temp_nvert[k].size());
        }
    }

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
    rr = PointsT::Zero(0,3);
    nn = NormalsT::Zero(0,3);
    nuse = -1;
    inuse = VectorXi::Zero(0);
    vertno = VectorXi::Zero(0);
    nuse_tri = -1;
    dist_limit = -1;
    neighbor_tri.clear();
    neighbor_vert.clear();

    // Reset inherited value-semantic fields
    itris = TrianglesT::Zero(0,3);
    use_itris = TrianglesT::Zero(0,3);
    dist = FiffSparseMatrix();
    nearest.clear();

    // Reset MNEHemisphere fields
    pinfo.clear();
    patch_inds = VectorXi::Zero(0);
    tri_cent = MatrixX3d::Zero(0,3);
    tri_nn = MatrixX3d::Zero(0,3);
    tri_area = VectorXd::Zero(0);
    use_tri_cent = MatrixX3d::Zero(0,3);
    use_tri_nn = MatrixX3d::Zero(0,3);
    use_tri_area = VectorXd::Zero(0);

    cluster_info.clear();

    m_TriCoords = MatrixXf();
}

//=============================================================================================================

MatrixXf& MNEHemisphere::getTriCoords(float p_fScaling)
{
    if(m_TriCoords.size() == 0)
    {
        m_TriCoords = MatrixXf(3,3*itris.rows());
        for(int i = 0; i < itris.rows(); ++i)
        {
            m_TriCoords.col(i*3) = rr.row( itris(i,0) ).transpose().cast<float>();
            m_TriCoords.col(i*3+1) = rr.row( itris(i,1) ).transpose().cast<float>();
            m_TriCoords.col(i*3+2) = rr.row( itris(i,2) ).transpose().cast<float>();
        }
    }

    m_TriCoords *= p_fScaling;

    return m_TriCoords;
}

//=============================================================================================================

bool MNEHemisphere::transform_hemisphere_to(fiff_int_t dest, const FiffCoordTrans &p_Trans)
{
    FiffCoordTrans trans(p_Trans);

    if (this->coord_frame == dest)
    {
//            res = src;
        return true;
    }

    if (trans.to == this->coord_frame && trans.from == dest)
        trans.invert_transform();
    else if(trans.from != this->coord_frame || trans.to != dest)
    {
        qWarning("Cannot transform the source space using this coordinate transformation");//Consider throw
        return false;
    }

    MatrixXf t = trans.trans.block(0,0,3,4);
//        res             = src;
    this->coord_frame = dest;
    MatrixXf t_rr = MatrixXf::Ones(this->np, 4);
    t_rr.block(0, 0, this->np, 3) = this->rr;
    MatrixXf t_nn = MatrixXf::Zero(this->np, 4);
    t_nn.block(0, 0, this->np, 3) = this->nn;

    this->rr    = (t*t_rr.transpose()).transpose();
    this->nn    = (t*t_nn.transpose()).transpose();

    return true;
}

//=============================================================================================================
//ToDo
void MNEHemisphere::writeToStream(FiffStream* p_pStream)
{
    if(this->type == 1 || this->type == 2)
        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_TYPE, &this->type);
    else
        qWarning("Unknown source space type (%d)", this->type);
    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_ID, &this->id);

//    data = this.get('subject_his_id', None)
//    if data:
//        write_string(fid, FIFF.FIFF_SUBJ_HIS_ID, data)
    p_pStream->write_int(FIFF_MNE_COORD_FRAME, &this->coord_frame);

    if(this->type == 2) //2 = Vol
    {
        qDebug() << "ToDo: Write Volume not implemented yet!!!!!!!!";
//        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS, this->shape)
//        p_pStream->write_coord_trans(this->src_mri_t);

        p_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);
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

        p_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
    }

    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &this->np);
    p_pStream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_POINTS, this->rr);
    p_pStream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_NORMALS, this->nn);

    //   Which vertices are active
    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_SELECTION, this->inuse.data(), this->inuse.size());
    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE, &this->nuse);

    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NTRI, &this->ntri);
    if (this->ntri > 0)
        p_pStream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, (this->itris.array() + 1).matrix());

    if (this->type != 2 && this->use_itris.rows() > 0)
    {
        //   Use triangulation
        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE_TRI, &this->nuse_tri);
        p_pStream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, (this->use_itris.array() + 1).matrix());
    }

    //   Patch-related information
    if (!this->nearest.empty())
    {
        Eigen::VectorXi nearestIdx = this->nearestVertIdx();
        Eigen::VectorXf nearestDistF = this->nearestDistVec().cast<float>();
        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NEAREST, nearestIdx.data(), nearestIdx.size());
        p_pStream->write_float(FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, nearestDistF.data(), nearestDistF.size());
    }

    //   Distances
    if (!this->dist.is_empty())
    {
        // Convert FiffSparseMatrix to Eigen and save only upper triangular portion
        Eigen::SparseMatrix<double> eigenDist = this->dist.toEigenSparse();
        typedef Eigen::Triplet<float> T;
        std::vector<T> tripletList;
        tripletList.reserve(eigenDist.nonZeros());
        for (int k=0; k < eigenDist.outerSize(); ++k)
            for (Eigen::SparseMatrix<double>::InnerIterator it(eigenDist,k); it; ++it)
                if(it.col() >= it.row())//only upper triangle -> todo iteration can be optimized
                    tripletList.push_back(T(it.row(), it.col(), (float)it.value()));
        Eigen::SparseMatrix<float> dists(eigenDist.rows(), eigenDist.cols());
        dists.setFromTriplets(tripletList.begin(), tripletList.end());

        p_pStream->write_float_sparse_rcs(FIFF_MNE_SOURCE_SPACE_DIST, dists);
        //ToDo check if write_float_matrix or write float is okay
        p_pStream->write_float(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, &this->dist_limit); //p_pStream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT, this->dist_limit);
    }
}

////=============================================================================================================

//QGeometryData* MNEHemisphere::getGeometryData(float p_fScaling)
//{
//    if(m_pGeometryData == NULL)
//    {
//        m_pGeometryData = new QGeometryData();

//        MatrixXd* triCoords = getTriCoords(p_fScaling);

//        m_pGeometryData->appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(triCoords->data()), triCoords->cols() ));
//    }

//    return m_pGeometryData;
//}
