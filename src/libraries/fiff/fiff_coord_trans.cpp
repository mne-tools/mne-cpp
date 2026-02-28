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

#include <QFile>
#include <QTextStream>

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
    MatrixX4f rr_ones(rr.rows(), 4);
    if(do_move) {
        rr_ones.setOnes();
    } else {
        rr_ones.setZero();
    }
    rr_ones.block(0,0,rr.rows(),3) = rr;
    return rr_ones*trans.block<3,4>(0,0).transpose();
}

//=============================================================================================================

MatrixX3f FiffCoordTrans::apply_inverse_trans(const MatrixX3f& rr, bool do_move) const
{
    MatrixX4f rr_ones(rr.rows(), 4);
    if(do_move) {
        rr_ones.setOnes();
    } else {
        rr_ones.setZero();
    }
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

FiffCoordTrans::FiffCoordTrans(int from, int to, const Matrix3f& rot, const Vector3f& move)
{
    this->trans = MatrixXf::Zero(4,4);

    this->from = from;
    this->to   = to;

    this->trans.block<3,3>(0,0) = rot;
    this->trans.block<3,1>(0,3) = move;
    this->trans(3,3) = 1.0f;

    FiffCoordTrans::addInverse(*this);
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(int from, int to, const Matrix4f& matTrans, bool bStandard)
{
    this->trans = matTrans;
    this->from = from;
    this->to   = to;

    if(bStandard) {
        // make sure that it is a standard transform if requested
        this->trans.row(3) = Vector4f(0,0,0,1);
    }

    FiffCoordTrans::addInverse(*this);
}

//=============================================================================================================

bool FiffCoordTrans::addInverse(FiffCoordTrans &t)
{
    t.invtrans = t.trans.inverse().eval();
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

void FiffCoordTrans::apply_trans(float r[3], const FiffCoordTrans& t, bool do_move)
{
    float res[3];
    for (int j = 0; j < 3; j++) {
        res[j] = do_move ? t.trans(j,3) : 0.0f;
        for (int k = 0; k < 3; k++)
            res[j] += t.trans(j,k) * r[k];
    }
    for (int j = 0; j < 3; j++)
        r[j] = res[j];
}

//=============================================================================================================

void FiffCoordTrans::apply_inverse_trans(float r[3], const FiffCoordTrans& t, bool do_move)
{
    float res[3];
    for (int j = 0; j < 3; j++) {
        res[j] = do_move ? t.invtrans(j,3) : 0.0f;
        for (int k = 0; k < 3; k++)
            res[j] += t.invtrans(j,k) * r[k];
    }
    for (int j = 0; j < 3; j++)
        r[j] = res[j];
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::identity(int from, int to)
{
    FiffCoordTrans t;
    t.from = from;
    t.to = to;
    // trans and invtrans are already identity from default constructor
    return t;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::inverted() const
{
    FiffCoordTrans t;
    t.from = this->to;
    t.to = this->from;
    t.trans = this->invtrans;
    t.invtrans = this->trans;
    return t;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::combine(int from, int to, const FiffCoordTrans& t1, const FiffCoordTrans& t2)
{
    FiffCoordTrans a, b;
    bool found = false;

    for (int swapped = 0; swapped < 2 && !found; swapped++) {
        const FiffCoordTrans& s1 = (swapped == 0) ? t1 : t2;
        const FiffCoordTrans& s2 = (swapped == 0) ? t2 : t1;

        if (s1.to == to && s2.from == from) {
            a = s1; b = s2; found = true;
        } else if (s1.from == to && s2.from == from) {
            a = s1.inverted(); b = s2; found = true;
        } else if (s1.to == to && s2.to == from) {
            a = s1; b = s2.inverted(); found = true;
        } else if (s1.from == to && s2.to == from) {
            a = s1.inverted(); b = s2.inverted(); found = true;
        }
    }

    if (!found || a.from != b.to) {
        qCritical("Cannot combine coordinate transforms");
        return FiffCoordTrans();
    }

    // Catenate: result = a * b (apply b first, then a)
    FiffCoordTrans result;
    result.from = b.from;
    result.to = a.to;
    result.trans = a.trans * b.trans;
    result.trans.row(3) << 0.0f, 0.0f, 0.0f, 1.0f;
    addInverse(result);
    return result;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::fromCardinalPoints(int from, int to,
                                                  const float* rL,
                                                  const float* rN,
                                                  const float* rR)
{
    Map<const Vector3f> L(rL);
    Map<const Vector3f> N(rN);
    Map<const Vector3f> R(rR);

    Vector3f diff1 = N - L;
    Vector3f diff2 = R - L;

    float alpha = diff1.dot(diff2) / diff2.dot(diff2);
    Vector3f r0 = (1.0f - alpha) * L + alpha * R;
    Vector3f ex = diff2.normalized();
    Vector3f ey = (N - r0).normalized();
    Vector3f ez = ex.cross(ey);

    FiffCoordTrans result;
    result.from = from;
    result.to = to;
    result.rot().col(0) = ex;
    result.rot().col(1) = ey;
    result.rot().col(2) = ez;
    result.move() = r0;
    addInverse(result);
    return result;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readTransform(const QString& name, int from, int to)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if (!stream->open()) {
        return FiffCoordTrans();
    }

    FiffTag::SPtr t_pTag;
    for (int k = 0; k < stream->dir().size(); k++) {
        if (stream->dir()[k]->kind == FIFF_COORD_TRANS) {
            if (!stream->read_tag(t_pTag, stream->dir()[k]->pos))
                continue;

            FiffCoordTrans t = t_pTag->toCoordTrans();
            if (t.from == from && t.to == to) {
                stream->close();
                return t;
            } else if (t.from == to && t.to == from) {
                t.invert_transform();
                stream->close();
                return t;
            }
        }
    }

    qCritical("No suitable coordinate transformation found in %s.", name.toUtf8().constData());
    stream->close();
    return FiffCoordTrans();
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readMriTransform(const QString& name)
{
    return readTransform(name, FIFFV_COORD_MRI, FIFFV_COORD_HEAD);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readMeasTransform(const QString& name)
{
    return readTransform(name, FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readTransformAscii(const QString& name, int from, int to)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open %s", name.toUtf8().constData());
        return FiffCoordTrans();
    }

    QTextStream in(&file);
    Matrix3f rot;
    Vector3f moveVec;

    int row = 0;
    while (!in.atEnd() && row < 4) {
        QString line = in.readLine();
        // Strip comments and whitespace
        int commentIdx = line.indexOf('#');
        if (commentIdx >= 0)
            line = line.left(commentIdx);
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        if (row < 3) {
            QStringList parts = line.simplified().split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 4) {
                qCritical("Cannot read the coordinate transformation from %s",
                          name.toUtf8().constData());
                return FiffCoordTrans();
            }
            bool ok1, ok2, ok3, ok4;
            rot(row, 0) = parts[0].toFloat(&ok1);
            rot(row, 1) = parts[1].toFloat(&ok2);
            rot(row, 2) = parts[2].toFloat(&ok3);
            moveVec[row] = parts[3].toFloat(&ok4) / 1000.0f;
            if (!ok1 || !ok2 || !ok3 || !ok4) {
                qCritical("Bad floating point number in coordinate transformation");
                return FiffCoordTrans();
            }
        }
        // Row 3 (the last row of the 4x4 matrix) is consumed but ignored
        row++;
    }

    if (row < 4) {
        qCritical("Cannot read the coordinate transformation from %s",
                  name.toUtf8().constData());
        return FiffCoordTrans();
    }

    return FiffCoordTrans(from, to, rot, moveVec);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readFShead2mriTransform(const QString& name)
{
    return readTransformAscii(name, FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readFromTag(const QSharedPointer<FiffTag>& tag)
{
    FiffCoordTrans t;
    if (tag->isMatrix() || tag->getType() != FIFFT_COORD_TRANS_STRUCT || tag->data() == nullptr)
        return t;

    qint32* t_pInt32 = (qint32*)tag->data();
    t.from = t_pInt32[0];
    t.to   = t_pInt32[1];

    float* t_pFloat = (float*)tag->data();
    int count = 0;
    int r, c;
    for (r = 0; r < 3; ++r) {
        t.trans(r, 3) = t_pFloat[11 + r];
        for (c = 0; c < 3; ++c) {
            t.trans(r, c) = t_pFloat[2 + count];
            ++count;
        }
    }
    t.trans(3, 0) = 0.0f; t.trans(3, 1) = 0.0f; t.trans(3, 2) = 0.0f; t.trans(3, 3) = 1.0f;

    count = 0;
    for (r = 0; r < 3; ++r) {
        t.invtrans(r, 3) = t_pFloat[23 + r];
        for (c = 0; c < 3; ++c) {
            t.invtrans(r, c) = t_pFloat[14 + count];
            ++count;
        }
    }
    t.invtrans(3, 0) = 0.0f; t.invtrans(3, 1) = 0.0f; t.invtrans(3, 2) = 0.0f; t.invtrans(3, 3) = 1.0f;

    return t;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::readTransformFromNode(FiffStream::SPtr& stream,
                                                     const FiffDirNode::SPtr& node,
                                                     int from, int to)
{
    FiffTag::SPtr t_pTag;
    fiff_int_t kind, pos;
    int k;

    for (k = 0; k < node->nent(); k++)
        kind = node->dir[k]->kind;
    pos  = node->dir[k]->pos;
    if (kind == FIFF_COORD_TRANS) {
        if (!stream->read_tag(t_pTag, pos))
            return FiffCoordTrans();
        FiffCoordTrans res = readFromTag(t_pTag);
        if (res.isEmpty())
            return FiffCoordTrans();
        if (res.from == from && res.to == to) {
            return res;
        }
        else if (res.from == to && res.to == from) {
            return res.inverted();
        }
    }
    printf("No suitable coordinate transformation found");
    return FiffCoordTrans();
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::procrustesAlign(int   from_frame,
                                                int   to_frame,
                                                float **fromp,
                                                float **top,
                                                float *w,
                                                int   np,
                                                float max_diff)
{
    /*
     * Map the raw C arrays into Eigen matrices for the computation.
     * fromp and top are np×3 C matrices (float**)
     */
    Eigen::MatrixXf fromPts(np, 3), toPts(np, 3);
    for (int j = 0; j < np; ++j)
        for (int c = 0; c < 3; ++c) {
            fromPts(j, c) = fromp[j][c];
            toPts(j, c)   = top[j][c];
        }

    /* Calculate centroids and subtract */
    Eigen::Vector3f from0 = fromPts.colwise().mean();
    Eigen::Vector3f to0   = toPts.colwise().mean();

    Eigen::MatrixXf fromC = fromPts.rowwise() - from0.transpose();
    Eigen::MatrixXf toC   = toPts.rowwise() - to0.transpose();

    /* Compute the cross-covariance matrix S */
    Eigen::Matrix3f S;
    if (w) {
        Eigen::VectorXf wVec = Eigen::Map<Eigen::VectorXf>(w, np);
        S = fromC.transpose() * wVec.asDiagonal() * toC;
    } else {
        S = fromC.transpose() * toC;
    }

    /* SVD of S to solve the orthogonal Procrustes problem */
    Eigen::JacobiSVD<Eigen::Matrix3f> svd(S, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix3f R = svd.matrixV() * svd.matrixU().transpose();

    /* Translation */
    Eigen::Vector3f moveVec = to0 - R * from0;

    /* Test the transformation */
    for (int p = 0; p < np; ++p) {
        Eigen::Vector3f rr = R * fromPts.row(p).transpose() + moveVec;
        float diff = (toPts.row(p).transpose() - rr).norm();
        if (diff > max_diff) {
            printf("Too large difference in matching : %7.1f > %7.1f mm",
                   1000.0f * diff, 1000.0f * max_diff);
            return FiffCoordTrans();
        }
    }

    return FiffCoordTrans(from_frame, to_frame, R, moveVec);
}
