
//=============================================================================================================
/**
* @file     mne_hemisphere.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    ToDo Documentation...
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
, rr(MatrixX3f::Zero(0,3))
, nn(MatrixX3f::Zero(0,3))
, tris(MatrixX3i::Zero(0,3))
, nuse(-1)
, inuse(VectorXi::Zero(0))
, vertno(VectorXi::Zero(0))
, nuse_tri(-1)
, use_tris(MatrixX3i::Zero(0,3))
, nearest(VectorXi::Zero(0))
, nearest_dist(VectorXf::Zero(0))
, pinfo(QList<VectorXi>())
, dist_limit(-1)
, dist(MatrixXf::Zero(0,0))
, tri_cent(MatrixX3f::Zero(0,3))
, tri_nn(MatrixX3f::Zero(0,3))
, tri_area(VectorXf::Zero(0))
, use_tri_cent(MatrixX3f::Zero(0,3))
, use_tri_nn(MatrixX3f::Zero(0,3))
, use_tri_area(VectorXf::Zero(0))
, m_pTriCoords(NULL)
//, m_pGeometryData(NULL)
{
}


//*************************************************************************************************************

MNEHemisphere::MNEHemisphere(MNEHemisphere* p_pMNEHemisphere)
: id(p_pMNEHemisphere->id)
, np(p_pMNEHemisphere->np)
, ntri(p_pMNEHemisphere->ntri)
, coord_frame(p_pMNEHemisphere->coord_frame)
, rr(MatrixX3f(p_pMNEHemisphere->rr))
, nn(MatrixX3f(p_pMNEHemisphere->nn))
, tris(MatrixX3i(p_pMNEHemisphere->tris))
, nuse(p_pMNEHemisphere->nuse)
, inuse(VectorXi(p_pMNEHemisphere->inuse))
, vertno(VectorXi(p_pMNEHemisphere->vertno))
, nuse_tri(p_pMNEHemisphere->nuse_tri)
, use_tris(MatrixX3i(p_pMNEHemisphere->use_tris))
, nearest(VectorXi(p_pMNEHemisphere->nearest))
, nearest_dist(VectorXf(p_pMNEHemisphere->nearest_dist))
, pinfo(p_pMNEHemisphere->pinfo)
, dist_limit(p_pMNEHemisphere->dist_limit)
, dist(MatrixXf(p_pMNEHemisphere->dist))
, tri_cent(MatrixX3f(p_pMNEHemisphere->tri_cent))
, tri_nn(MatrixX3f(p_pMNEHemisphere->tri_nn))
, tri_area(VectorXf(p_pMNEHemisphere->tri_area))
, use_tri_cent(MatrixX3f(p_pMNEHemisphere->use_tri_cent))
, use_tri_nn(MatrixX3f(p_pMNEHemisphere->use_tri_nn))
, use_tri_area(VectorXf(p_pMNEHemisphere->use_tri_area))
, m_pTriCoords(p_pMNEHemisphere->m_pTriCoords ? new MatrixXf(*(p_pMNEHemisphere->m_pTriCoords)) : NULL)
{
    //*m_pGeometryData = *p_pMNEHemisphere->m_pGeometryData;
}


//*************************************************************************************************************

MNEHemisphere::~MNEHemisphere()
{
    qDebug() << "MNEHemisphere Destructor";
    if(m_pTriCoords)
        delete m_pTriCoords;

//    if(m_pGeometryData)
//        delete m_pGeometryData;

}


//*************************************************************************************************************

bool MNEHemisphere::transform_hemisphere_to(fiff_int_t dest, FiffCoordTrans* trans)
{
    if (this->coord_frame == dest)
    {
//            res = src;
        return true;
    }

    if (trans->to == this->coord_frame && trans->from == dest)
        trans->invert_transform();
    else if(trans->from != this->coord_frame || trans->to != dest)
    {
        printf("Cannot transform the source space using this coordinate transformation");//Consider throw
        return false;
    }

    MatrixXf t = trans->trans.block(0,0,3,4);
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


//*************************************************************************************************************

MatrixXf* MNEHemisphere::getTriCoords(float p_fScaling)
{
    if(m_pTriCoords == NULL)
    {
        m_pTriCoords = new MatrixXf(3,3*tris.rows());
        for(qint32 i = 0; i < tris.rows(); ++i)
        {
            m_pTriCoords->block(0,i*3,3,1) = rr.row( tris(i,0) ).transpose();
            m_pTriCoords->block(0,i*3+1,3,1) = rr.row( tris(i,1) ).transpose();
            m_pTriCoords->block(0,i*3+2,3,1) = rr.row( tris(i,2) ).transpose();
        }
    }

    *m_pTriCoords *= p_fScaling;

    return m_pTriCoords;
}



////*************************************************************************************************************

//QGeometryData* MNEHemisphere::getGeometryData(float p_fScaling)
//{
//    if(m_pGeometryData == NULL)
//    {
//        m_pGeometryData = new QGeometryData();

//        MatrixXf* triCoords = getTriCoords(p_fScaling);

//        m_pGeometryData->appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(triCoords->data()), triCoords->cols() ));
//    }

//    return m_pGeometryData;
//}
