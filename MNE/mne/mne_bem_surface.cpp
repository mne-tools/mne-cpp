//=============================================================================================================
/**
* @file     mne_bem_surface.cpp
* @author   Jana Kiesel<jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
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
* @brief     MNEBemSurface class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_bem_surface.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEBemSurface::MNEBemSurface()
: id(-1)
, np(-1)
, ntri(-1)
, coord_frame(-1)
, sigma(-1)
, rr(MatrixX3f::Zero(0,3))
, nn(MatrixX3f::Zero(0,3))
, tris(MatrixX3i::Zero(0,3))
{
}


//*************************************************************************************************************

MNEBemSurface::MNEBemSurface(const MNEBemSurface& p_MNEBemSurface)
: id(p_MNEBemSurface.id)
, np(p_MNEBemSurface.np)
, ntri(p_MNEBemSurface.ntri)
, coord_frame(p_MNEBemSurface.coord_frame)
, sigma(p_MNEBemSurface.sigma)
, rr(p_MNEBemSurface.rr)
, nn(p_MNEBemSurface.nn)
, tris(p_MNEBemSurface.tris)
{
    //*m_pGeometryData = *p_MNEBemSurface.m_pGeometryData;
}


//*************************************************************************************************************

MNEBemSurface::~MNEBemSurface()
{

}


//*************************************************************************************************************

void MNEBemSurface::clear()
{
    id = -1;
    np = -1;
    ntri = -1;
    coord_frame = -1;
    sigma = -1;
    rr = MatrixX3f::Zero(0,3);
    nn = MatrixX3f::Zero(0,3);
    tris = MatrixX3i::Zero(0,3);
}


//*************************************************************************************************************

//MatrixXf& MNEBemSurface::getTriCoords(float p_fScaling)
//{
//    if(m_TriCoords.size() == 0)
//    {
//        m_TriCoords = MatrixXf(3,3*tris.rows());
//        for(qint32 i = 0; i < tris.rows(); ++i)
//        {
//            m_TriCoords.col(i*3) = rr.row( tris(i,0) ).transpose().cast<float>();
//            m_TriCoords.col(i*3+1) = rr.row( tris(i,1) ).transpose().cast<float>();
//            m_TriCoords.col(i*3+2) = rr.row( tris(i,2) ).transpose().cast<float>();
//        }
//    }

//    m_TriCoords *= p_fScaling;

//    return m_TriCoords;
//}


//*************************************************************************************************************

//bool MNEBemSurface::transform_hemisphere_to(fiff_int_t dest, const FiffCoordTrans &p_Trans)
//{
//    FiffCoordTrans trans(p_Trans);

//    if (this->coord_frame == dest)
//    {
//            res = src;
//        return true;
//    }

//    if (trans.to == this->coord_frame && trans.from == dest)
//        trans.invert_transform();
//    else if(trans.from != this->coord_frame || trans.to != dest)
//    {
//        printf("Cannot transform the source space using this coordinate transformation");//Consider throw
//        return false;
//    }

//    MatrixXf t = trans.trans.block(0,0,3,4);
//        res             = src;
//    this->coord_frame = dest;
//    MatrixXf t_rr = MatrixXf::Ones(this->np, 4);
//    t_rr.block(0, 0, this->np, 3) = this->rr;
//    MatrixXf t_nn = MatrixXf::Zero(this->np, 4);
//    t_nn.block(0, 0, this->np, 3) = this->nn;

//    this->rr    = (t*t_rr.transpose()).transpose();
//    this->nn    = (t*t_nn.transpose()).transpose();

//    return true;
//}


//*************************************************************************************************************

//QGeometryData* MNEBemSurface::getGeometryData(float p_fScaling)
//{
//    if(m_pGeometryData == NULL)
//    {
//        m_pGeometryData = new QGeometryData();

//        MatrixXd* triCoords = getTriCoords(p_fScaling);

//        m_pGeometryData->appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(triCoords->data()), triCoords->cols() ));
//    }

//    return m_pGeometryData;
//}
