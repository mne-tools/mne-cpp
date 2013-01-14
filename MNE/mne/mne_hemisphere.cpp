
//=============================================================================================================
/**
* @file     mne_hemisphere.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the MNEHemisphere class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_hemisphere.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEHemisphere::MNEHemisphere()
: id(-1)
, np(-1)
, ntri(-1)
, coord_frame(-1)
, rr(MatrixX3d::Zero(0,3))
, nn(MatrixX3d::Zero(0,3))
, tris(MatrixX3i::Zero(0,3))
, nuse(-1)
, inuse(VectorXi::Zero(0))
, vertno(VectorXi::Zero(0))
, nuse_tri(-1)
, use_tris(MatrixX3i::Zero(0,3))
, nearest(VectorXi::Zero(0))
, nearest_dist(VectorXd::Zero(0))
, dist_limit(-1)
, dist(MatrixXd())
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


//*************************************************************************************************************

MNEHemisphere::MNEHemisphere(const MNEHemisphere& p_MNEHemisphere)
: id(p_MNEHemisphere.id)
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
, dist_limit(p_MNEHemisphere.dist_limit)
, dist(p_MNEHemisphere.dist)
, tri_cent(p_MNEHemisphere.tri_cent)
, tri_nn(p_MNEHemisphere.tri_nn)
, tri_area(p_MNEHemisphere.tri_area)
, use_tri_cent(p_MNEHemisphere.use_tri_cent)
, use_tri_nn(p_MNEHemisphere.use_tri_nn)
, use_tri_area(p_MNEHemisphere.use_tri_area)
, m_TriCoords(p_MNEHemisphere.m_TriCoords)
, cluster_vertnos(p_MNEHemisphere.cluster_vertnos)
, cluster_distances(p_MNEHemisphere.cluster_distances)
{
    //*m_pGeometryData = *p_MNEHemisphere.m_pGeometryData;
}


//*************************************************************************************************************

MNEHemisphere::~MNEHemisphere()
{

}


//*************************************************************************************************************

void MNEHemisphere::clear()
{
    id = -1;
    np = -1;
    ntri = -1;
    coord_frame = -1;
    rr = MatrixX3d::Zero(0,3);
    nn = MatrixX3d::Zero(0,3);
    tris = MatrixX3i::Zero(0,3);
    nuse = -1;
    inuse = VectorXi::Zero(0);
    vertno = VectorXi::Zero(0);
    nuse_tri = -1;
    use_tris = MatrixX3i::Zero(0,3);
    nearest = VectorXi::Zero(0);
    nearest_dist = VectorXd::Zero(0);
    pinfo.clear();
    dist_limit = -1;
    dist = MatrixXd();
    tri_cent = MatrixX3d::Zero(0,3);
    tri_nn = MatrixX3d::Zero(0,3);
    tri_area = VectorXd::Zero(0);
    use_tri_cent = MatrixX3d::Zero(0,3);
    use_tri_nn = MatrixX3d::Zero(0,3);
    use_tri_area = VectorXd::Zero(0);

    cluster_vertnos.clear();
    cluster_distances.clear();

    m_TriCoords = MatrixXf();
}




//*************************************************************************************************************

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

    MatrixXd t = trans.trans.block(0,0,3,4);
//        res             = src;
    this->coord_frame = dest;
    MatrixXd t_rr = MatrixXd::Ones(this->np, 4);
    t_rr.block(0, 0, this->np, 3) = this->rr;
    MatrixXd t_nn = MatrixXd::Zero(this->np, 4);
    t_nn.block(0, 0, this->np, 3) = this->nn;

    this->rr    = (t*t_rr.transpose()).transpose();
    this->nn    = (t*t_nn.transpose()).transpose();

    return true;
}


//*************************************************************************************************************

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



////*************************************************************************************************************

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
