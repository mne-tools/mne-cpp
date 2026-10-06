//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_coord_trans_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FIFFLIB::FiffCoordTransSet: reading the Talairach @c .xfm file,
 *           collecting the chain from FIFF files and mapping head coordinates to MNI and Talairach.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans_set.h"
#include "fiff_constants.h"
#include "fiff_stream.h"
#include "fiff_tag.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTrans FiffCoordTransSet::readMniTransform(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return FiffCoordTrans();
    }
    // "Transform_Type = Linear; Linear_Transform = <3 x 4 matrix, translation in mm> ;"
    const QString text = QTextStream(&file).readAll();
    static const QRegularExpression linear(QStringLiteral("Transform_Type\\s*=\\s*Linear\\s*;\\s*Linear_Transform\\s*=([^;]*);"),
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = linear.match(text);
    const QStringList values = match.captured(1).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (!match.hasMatch() || values.size() != 12) {
        return FiffCoordTrans();
    }
    Matrix3f rot;
    Vector3f move;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            bool ok = false;
            const float value = values[4 * row + col].toFloat(&ok);
            if (!ok) {
                return FiffCoordTrans();
            }
            if (col < 3) {
                rot(row, col) = value;
            } else {
                move[row] = value / 1000.0f;
            }
        }
    }
    return FiffCoordTrans(FIFFV_MNE_COORD_RAS, FIFFV_MNE_COORD_MNI_TAL, rot, move);
}

//=============================================================================================================

bool FiffCoordTransSet::addTalairach(const QString& xfmPath)
{
    const FiffCoordTrans rasMni = readMniTransform(xfmPath);
    if (rasMni.isEmpty()) {
        return false;
    }
    // M. Brett's MNI -> Talairach approximation, separately above and below the AC-PC plane.
    Matrix3f above;
    above << 0.99f, 0.0f, 0.0f, 0.0f, 0.9688f, 0.046f, 0.0f, -0.0485f, 0.9189f;
    Matrix3f below;
    below << 0.99f, 0.0f, 0.0f, 0.0f, 0.9688f, 0.042f, 0.0f, -0.0485f, 0.839f;
    RAS_MNI_tal_t = rasMni;
    MNI_tal_tal_gtz_t = FiffCoordTrans(FIFFV_MNE_COORD_MNI_TAL, FIFFV_MNE_COORD_FS_TAL_GTZ, above, Vector3f::Zero());
    MNI_tal_tal_ltz_t = FiffCoordTrans(FIFFV_MNE_COORD_MNI_TAL, FIFFV_MNE_COORD_FS_TAL_LTZ, below, Vector3f::Zero());
    return true;
}

//=============================================================================================================

int FiffCoordTransSet::read(const QString& path)
{
    QFile file(path);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        return -1;
    }
    const struct
    {
        int from;
        int to;
        FiffCoordTrans FiffCoordTransSet::* member;
    } links[] = {
        {FIFFV_COORD_HEAD, FIFFV_COORD_MRI, &FiffCoordTransSet::head_surf_RAS_t},
        {FIFFV_COORD_MRI, FIFFV_MNE_COORD_RAS, &FiffCoordTransSet::surf_RAS_RAS_t},
        {FIFFV_MNE_COORD_RAS, FIFFV_MNE_COORD_MNI_TAL, &FiffCoordTransSet::RAS_MNI_tal_t},
        {FIFFV_MNE_COORD_MNI_TAL, FIFFV_MNE_COORD_FS_TAL_GTZ, &FiffCoordTransSet::MNI_tal_tal_gtz_t},
        {FIFFV_MNE_COORD_MNI_TAL, FIFFV_MNE_COORD_FS_TAL_LTZ, &FiffCoordTransSet::MNI_tal_tal_ltz_t},
    };
    int count = 0;
    FiffTag::UPtr tag;
    for (const auto& entry : stream->dir()) {
        if (entry->kind != FIFF_COORD_TRANS || !stream->read_tag(tag, entry->pos)) {
            continue;
        }
        const FiffCoordTrans t = tag->toCoordTrans();
        for (const auto& link : links) {
            if (t.from == link.from && t.to == link.to) {
                this->*link.member = t;
                ++count;
            } else if (t.from == link.to && t.to == link.from) {
                this->*link.member = t.inverted();
                ++count;
            }
        }
    }
    stream->close();
    return count;
}

//=============================================================================================================

void FiffCoordTransSet::write(FiffStream& stream) const
{
    for (const FiffCoordTrans* t : {&head_surf_RAS_t, &surf_RAS_RAS_t, &RAS_MNI_tal_t, &MNI_tal_tal_gtz_t, &MNI_tal_tal_ltz_t}) {
        if (!t->isEmpty()) {
            stream.write_coord_trans(*t);
        }
    }
}

//=============================================================================================================

MatrixX3f FiffCoordTransSet::headToMni(const MatrixX3f& rr) const
{
    if (head_surf_RAS_t.isEmpty() || surf_RAS_RAS_t.isEmpty() || RAS_MNI_tal_t.isEmpty()) {
        return MatrixX3f();
    }
    return RAS_MNI_tal_t.apply_trans(surf_RAS_RAS_t.apply_trans(head_surf_RAS_t.apply_trans(rr)));
}

//=============================================================================================================

MatrixX3f FiffCoordTransSet::mniToTalairach(const MatrixX3f& rr) const
{
    if (MNI_tal_tal_gtz_t.isEmpty() || MNI_tal_tal_ltz_t.isEmpty()) {
        return MatrixX3f();
    }
    const MatrixX3f above = MNI_tal_tal_gtz_t.apply_trans(rr);
    const MatrixX3f below = MNI_tal_tal_ltz_t.apply_trans(rr);
    MatrixX3f result(rr.rows(), 3);
    for (Index k = 0; k < rr.rows(); ++k) {
        result.row(k) = rr(k, 2) > 0.0f ? above.row(k) : below.row(k);
    }
    return result;
}
