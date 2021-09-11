//=============================================================================================================
/**
 * @file     fiff_coord_trans.cpp
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
 * @brief    Definition of the FiffCoordTrans Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans.h"

#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"

#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTrans::FiffCoordTrans()
: from(-1)
, to(-1)
, trans(MatrixXf::Identity(4,4))
, invtrans(MatrixXf::Identity(4,4))
{
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(QIODevice &p_IODevice)
: from(-1)
, to(-1)
, trans(MatrixXf::Identity(4,4))
, invtrans(MatrixXf::Identity(4,4))
{
    if(!read(p_IODevice, *this))
    {
        printf("\tCoordindate transform not found.\n");//ToDo Throw here
        return;
    }
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(const FiffCoordTrans &p_FiffCoordTrans)
: from(p_FiffCoordTrans.from)
, to(p_FiffCoordTrans.to)
, trans(p_FiffCoordTrans.trans)
, invtrans(p_FiffCoordTrans.invtrans)
{
}

//=============================================================================================================

FiffCoordTrans::~FiffCoordTrans()
{
}

//=============================================================================================================

void FiffCoordTrans::clear()
{
    from = -1;
    to = -1;
    trans.setIdentity();
    invtrans.setIdentity();
}

//=============================================================================================================

bool FiffCoordTrans::invert_transform()
{
    fiff_int_t from_new = this->to;
    this->to    = this->from;
    this->from  = from_new;
    this->trans = this->trans.inverse().eval();
    this->invtrans = this->invtrans.inverse().eval();

    return true;
}

//=============================================================================================================

bool FiffCoordTrans::read(QIODevice& p_IODevice, FiffCoordTrans& p_Trans)
{
    FiffStream::SPtr pStream(new FiffStream(&p_IODevice));

    printf("Reading coordinate transform from %s...\n", pStream->streamName().toUtf8().constData());
    if(!pStream->open())
        return false;

    //
    //   Locate and read the coordinate transformation
    //
    FiffTag::SPtr t_pTag;
    bool success = false;

    //
    //   Get the MRI <-> head coordinate transformation
    //
    for ( qint32 k = 0; k < pStream->dir().size(); ++k )
    {
        if ( pStream->dir()[k]->kind == FIFF_COORD_TRANS )
        {
            pStream->read_tag(t_pTag,pStream->dir()[k]->pos);
            p_Trans = t_pTag->toCoordTrans();
            success = true;
        }
    }

    return success;
}

//=============================================================================================================

void FiffCoordTrans::write(QIODevice &qIODevice)
{
    // Create the file and save the essentials
    FiffStream::SPtr pStream = FiffStream::start_file(qIODevice);
    printf("Write coordinate transform in %s...\n", pStream->streamName().toUtf8().constData());
    this->writeToStream(pStream.data());
    pStream->end_file();
    qIODevice.close();
}

//=============================================================================================================

void FiffCoordTrans::writeToStream(FiffStream* pStream)
{
    pStream->write_coord_trans(*this);
}

//=============================================================================================================

MatrixX3f FiffCoordTrans::apply_trans(const MatrixX3f& rr, bool do_move) const
{
    MatrixX4f rr_ones = do_move ? MatrixX4f::Ones(rr.rows(),4) : MatrixX4f::Zero(rr.rows(),4);
    rr_ones.block(0,0,rr.rows(),3) = rr;
    return rr_ones*trans.block<3,4>(0,0).transpose();
}

//=============================================================================================================

MatrixX3f FiffCoordTrans::apply_inverse_trans(const MatrixX3f& rr, bool do_move) const
{
    MatrixX4f rr_ones = do_move ? MatrixX4f::Ones(rr.rows(),4) : MatrixX4f::Zero(rr.rows(),4);
    rr_ones.block(0,0,rr.rows(),3) = rr;
    return rr_ones*invtrans.block<3,4>(0,0).transpose();
}

//=============================================================================================================

QString FiffCoordTrans::frame_name (int frame)
{
    switch(frame) {
        case FIFFV_COORD_UNKNOWN: return "unknown";
        case FIFFV_COORD_DEVICE: return "MEG device";
        case FIFFV_COORD_ISOTRAK: return "isotrak";
        case FIFFV_COORD_HPI: return "hpi";
        case FIFFV_COORD_HEAD: return "head";
        case FIFFV_COORD_MRI: return "MRI (surface RAS)";
        case FIFFV_MNE_COORD_MRI_VOXEL: return "MRI voxel";
        case FIFFV_COORD_MRI_SLICE: return "MRI slice";
        case FIFFV_COORD_MRI_DISPLAY: return "MRI display";
        case FIFFV_MNE_COORD_CTF_DEVICE: return "CTF MEG device";
        case FIFFV_MNE_COORD_CTF_HEAD: return "CTF/4D/KIT head";
        case FIFFV_MNE_COORD_RAS: return "RAS (non-zero origin)";
        case FIFFV_MNE_COORD_MNI_TAL: return "MNI Talairach";
        case FIFFV_MNE_COORD_FS_TAL_GTZ: return "Talairach (MNI z > 0)";
        case FIFFV_MNE_COORD_FS_TAL_LTZ: return "Talairach (MNI z < 0)";
        default: return "unknown";
    }
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::make(int from, int to, const Matrix3f& rot, const VectorXf& move)
{
    FiffCoordTrans t;
    t.trans = MatrixXf::Zero(4,4);

    t.from = from;
    t.to   = to;

    t.trans.block<3,3>(0,0) = rot;
    t.trans.block<3,1>(0,3) = move;
    t.trans(3,3) = 1.0f;

    FiffCoordTrans::addInverse(t);

    return t;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::make(int from, int to, const Matrix4f& matTrans)
{
    FiffCoordTrans t;

    t.trans = matTrans;
    t.trans.row(3) = Vector4f(0,0,0,1); // make sure that it is affine
    t.from = from;
    t.to   = to;

    FiffCoordTrans::addInverse(t);

    return t;
}

//=============================================================================================================

bool FiffCoordTrans::addInverse(FiffCoordTrans &t)
{
    t.invtrans = t.trans.inverse().eval();
    t.invtrans.row(3) = Vector4f(0,0,0,1); // make sure that it is affine
    return true;
}

//=============================================================================================================

void FiffCoordTrans::print() const
{
    std::cout << "Coordinate transformation: ";
    std::cout << (QString("%1 -> %2\n").arg(frame_name(this->from)).arg(frame_name(this->to))).toUtf8().data();

    for (int p = 0; p < 3; p++)
        printf("\t% 8.6f % 8.6f % 8.6f\t% 7.2f mm\n", trans(p,0),trans(p,1),trans(p,2),1000*trans(p,3));
    printf("\t% 8.6f % 8.6f % 8.6f   % 7.2f\n",trans(3,0),trans(3,1),trans(3,2),trans(3,3));
}

//=============================================================================================================

void FiffCoordTrans::setTransform(const Eigen::Matrix4f& matTrans)
{
    this->trans = matTrans;
    this->trans.row(3) = Vector4f(0,0,0,1); // make sure that it is affine
    addInverse(*this);
}

//=============================================================================================================

float FiffCoordTrans::angleTo(Eigen::MatrixX4f  mTransDest)
{
    MatrixX4f mDevHeadT = this->trans;
    Matrix3f mRot = mDevHeadT.block(0,0,3,3);
    Matrix3f mRotNew = mTransDest.block(0,0,3,3);

    Quaternionf quat(mRot);
    Quaternionf quatNew(mRotNew);

    float fAngle;

    // calculate rotation
    Quaternionf quatCompare;

    quatCompare = quat*quatNew.inverse();
    fAngle = quat.angularDistance(quatNew);
    fAngle = fAngle * 180 / M_PI;

    return fAngle;
}

//=============================================================================================================

float FiffCoordTrans::translationTo(Eigen::MatrixX4f  mTransDest)
{
    VectorXf vTrans = this->trans.col(3);
    VectorXf vTransDest = mTransDest.col(3);

    float fMove = (vTrans-vTransDest).norm();
    return fMove;
}

//=============================================================================================================

FiffCoordTransOld FiffCoordTrans::toOld()
{
    FiffCoordTransOld tOld;
    tOld.from = this->from;
    tOld.to = this->to;
    tOld.rot = this->trans.block(0,0,3,3);
    tOld.move = this->trans.block(0,3,3,1);
    tOld.invrot = this->invtrans.block(0,0,3,3);
    tOld.invmove = this->invtrans.block(0,3,3,1);

    return tOld;
}
