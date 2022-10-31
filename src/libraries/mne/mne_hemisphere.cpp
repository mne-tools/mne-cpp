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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEHemisphere::MNEHemisphere()
: type(1)
, id(-1)
, np(-1)
, ntri(-1)
, coord_frame(-1)
, rr(MatrixX3f::Zero(0,3))
, nn(MatrixX3f::Zero(0,3))
, tris(MatrixX3i::Zero(0,3))
, nuse(-1)
, inuse(VectorXi::Zero(0))
, vertno(VectorXi::Zero(0))
, nuse_tri(-1)
, use_tris(MatrixX3i::Zero(0,3))
, nearest(VectorXi::Zero(0))
, nearest_dist(VectorXd::Zero(0))
, patch_inds(VectorXi::Zero(0))
, dist_limit(-1)
, dist(SparseMatrix<double>())
, tri_cent(MatrixX3d::Zero(0,3))
, tri_nn(MatrixX3d::Zero(0,3))
, tri_area(VectorXd::Zero(0))
, use_tri_cent(MatrixX3d::Zero(0,3))
, use_tri_nn(MatrixX3d::Zero(0,3))
, use_tri_area(VectorXd::Zero(0))
//, m_TriCoords()
//, m_pGeometryData(NULL)
{
}

//=============================================================================================================

MNEHemisphere::MNEHemisphere(const MNEHemisphere& p_MNEHemisphere)
: type(p_MNEHemisphere.type)
, id(p_MNEHemisphere.id)
, np(p_MNEHemisphere.np)
, ntri(p_MNEHemisphere.ntri)
, coord_frame(p_MNEHemisphere.coord_frame)
, rr(p_MNEHemisphere.rr)
, nn(p_MNEHemisphere.nn)
, tris(p_MNEHemisphere.tris)
, nuse(p_MNEHemisphere.nuse)
, inuse(p_MNEHemisphere.inuse)
, vertno(p_MNEHemisphere.vertno)
, nuse_tri(p_MNEHemisphere.nuse_tri)
, use_tris(p_MNEHemisphere.use_tris)
, nearest(p_MNEHemisphere.nearest)
, nearest_dist(p_MNEHemisphere.nearest_dist)
, pinfo(p_MNEHemisphere.pinfo)
, patch_inds(p_MNEHemisphere.patch_inds)
, dist_limit(p_MNEHemisphere.dist_limit)
, dist(p_MNEHemisphere.dist)
, tri_cent(p_MNEHemisphere.tri_cent)
, tri_nn(p_MNEHemisphere.tri_nn)
, tri_area(p_MNEHemisphere.tri_area)
, use_tri_cent(p_MNEHemisphere.use_tri_cent)
, use_tri_nn(p_MNEHemisphere.use_tri_nn)
, use_tri_area(p_MNEHemisphere.use_tri_area)
, neighbor_tri(p_MNEHemisphere.neighbor_tri)
, neighbor_vert(p_MNEHemisphere.neighbor_vert)
, cluster_info(p_MNEHemisphere.cluster_info)
, m_TriCoords(p_MNEHemisphere.m_TriCoords)
{
    //*m_pGeometryData = *p_MNEHemisphere.m_pGeometryData;
}

//=============================================================================================================

MNEHemisphere::~MNEHemisphere()
{
}

//=============================================================================================================

bool MNEHemisphere::add_geometry_info()
{
    int k,c,p,q;
    bool found;

    //Create neighboring triangle vector
    neighbor_tri = QVector<QVector<int> >(this->tris.rows());

    //Create neighbor_tri information
    for (p = 0; p < this->tris.rows(); p++) {
        for (k = 0; k < 3; k++) {
            this->neighbor_tri[this->tris(p,k)].append(p);
        }
    }

    //Create the neighboring vertices vector
    neighbor_vert = QVector<QVector<int> >(this->np);

    for (k = 0; k < this->np; k++) {
        for (p = 0; p < this->neighbor_tri[k].size(); p++) {
            //Fit in the other vertices of the neighboring triangle
            for (c = 0; c < 3; c++) {
                int vert = this->tris(this->neighbor_tri[k][p], c);

                if (vert != k) {
                    found = false;

                    for (q = 0; q < this->neighbor_vert[k].size(); q++) {
                        if (this->neighbor_vert[k][q] == vert) {
                            found = true;
                            break;
                        }
                    }

                    if(!found) {
                        this->neighbor_vert[k].append(vert);
                    }
                }
            }
        }
    }

    return true;
}

//=============================================================================================================

void MNEHemisphere::clear()
{
    type = 1;
    id = -1;
    np = -1;
    ntri = -1;
    coord_frame = -1;
    rr = MatrixX3f::Zero(0,3);
    nn = MatrixX3f::Zero(0,3);
    tris = MatrixX3i::Zero(0,3);
    nuse = -1;
    inuse = VectorXi::Zero(0);
    vertno = VectorXi::Zero(0);
    nuse_tri = -1;
    use_tris = MatrixX3i::Zero(0,3);
    nearest = VectorXi::Zero(0);
    nearest_dist = VectorXd::Zero(0);
    pinfo.clear();
    patch_inds = VectorXi::Zero(0);
    dist_limit = -1;
    dist = SparseMatrix<double>();
    tri_cent = MatrixX3d::Zero(0,3);
    tri_nn = MatrixX3d::Zero(0,3);
    tri_area = VectorXd::Zero(0);
    use_tri_cent = MatrixX3d::Zero(0,3);
    use_tri_nn = MatrixX3d::Zero(0,3);
    use_tri_area = VectorXd::Zero(0);

    neighbor_tri.clear();
    neighbor_vert.clear();

    cluster_info.clear();

    m_TriCoords = MatrixXf();
}

//=============================================================================================================

MatrixXf& MNEHemisphere::getTriCoords(float p_fScaling)
{
    if(m_TriCoords.size() == 0)
    {
        m_TriCoords = MatrixXf(3,3*tris.rows());
        for(qint32 i = 0; i < tris.rows(); ++i)
        {
            m_TriCoords.col(i*3) = rr.row( tris(i,0) ).transpose().cast<float>();
            m_TriCoords.col(i*3+1) = rr.row( tris(i,1) ).transpose().cast<float>();
            m_TriCoords.col(i*3+2) = rr.row( tris(i,2) ).transpose().cast<float>();
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
        printf("Cannot transform the source space using this coordinate transformation");//Consider throw
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
        printf("Unknown source space type (%d)\n", this->type);
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
        p_pStream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, this->tris.array() + 1);

    if (this->type != 2 && this->use_tris.rows() > 0)
    {
        //   Use triangulation
        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE_TRI, &this->nuse_tri);
        p_pStream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, this->use_tris.array() + 1);
    }

    //   Patch-related information
    if (this->nearest.size() > 0)
    {
        p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NEAREST, this->nearest.data(), this->nearest.size());
        p_pStream->write_float_matrix(FIFF_MNE_SOURCE_SPACE_NEAREST_DIST, this->nearest_dist.cast<float>());
    }

    //   Distances
    if (this->dist.rows() > 0)
    {
        // Save only upper triangular portion of the matrix
        typedef Eigen::Triplet<float> T;
        std::vector<T> tripletList;
        tripletList.reserve(this->dist.nonZeros());
        for (int k=0; k < this->dist.outerSize(); ++k)
            for (SparseMatrix<double>::InnerIterator it(this->dist,k); it; ++it)
                if(it.col() >= it.row())//only upper triangle -> todo iteration can be optimized
                    tripletList.push_back(T(it.row(), it.col(), (float)it.value()));
        SparseMatrix<float> dists(this->dist.rows(), this->dist.cols());
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
