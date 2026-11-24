//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_coord_trans.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffCoordTrans: 4x4 FIFF affine I/O, composition, inversion and (from, to) frame lookup.
 *
 * Backs the @c FIFFV_COORD_DEVICE / @c FIFFV_COORD_HEAD /
 * @c FIFFV_COORD_MRI / @c FIFFV_COORD_HPI transforms threaded through
 * @ref FiffInfo and through ``-trans.fif'' files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_coord_trans.h"

#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <format>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using mne::ErrorCode;
using mne::fail;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

// The first transform among entries from @p from to @p to, inverted if stored the other way round.
mne::Expected<FiffCoordTrans> find_transform(FiffStream& stream, const std::vector<FiffDirEntry>& entries, int from, int to)
{
    for (const FiffDirEntry& entry : entries) {
        if (entry.kind != FIFF_COORD_TRANS)
            continue;
        const auto tag = stream.read_tag(entry.pos);
        if (!tag)
            continue;
        FiffCoordTrans t = FiffCoordTrans::from_tag(*tag);
        if (t.empty())
            continue;
        if (t.from == from && t.to == to)
            return t;
        if (t.from == to && t.to == from)
            return t.inverted();
    }
    return fail(ErrorCode::NotFound, "No {} -> {} coordinate transform", FiffCoordTrans::frame_name(from), FiffCoordTrans::frame_name(to));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCoordTrans::FiffCoordTrans()
: from(-1)
, to(-1)
, trans(MatrixXf::Identity(4, 4))
, invtrans(MatrixXf::Identity(4, 4))
{
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(const std::filesystem::path& path)
: FiffCoordTrans(read(path).value())
{
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(const std::filesystem::path& path, int from, int to)
: FiffCoordTrans(read(path, from, to).value())
{
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(const FiffCoordTrans& p_FiffCoordTrans)
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
    this->to = this->from;
    this->from = from_new;
    this->trans = this->trans.inverse().eval();
    this->invtrans = this->invtrans.inverse().eval();

    return true;
}

//=============================================================================================================

mne::Expected<FiffCoordTrans> FiffCoordTrans::read(const std::filesystem::path& path)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    for (const FiffDirEntry& entry : stream->dir()) {
        if (entry.kind != FIFF_COORD_TRANS) {
            continue;
        }
        if (const auto tag = stream->read_tag(entry.pos)) {
            return from_tag(*tag);
        }
    }
    return fail(ErrorCode::NotFound, "No coordinate transform in {}", mne::utf8_from_path(path));
}

//=============================================================================================================

mne::Expected<FiffCoordTrans> FiffCoordTrans::read(const std::filesystem::path& path, int from, int to)
{
    auto stream = FiffStream::open(path);
    if (!stream) {
        return std::unexpected(stream.error());
    }
    return find_transform(*stream, stream->dir(), from, to).transform_error([&path](const mne::Error& error) {
        return mne::Error(error.code, std::format("{} in {}", error.what(), mne::utf8_from_path(path)));
    });
}

//=============================================================================================================

mne::Expected<void> FiffCoordTrans::write(const std::filesystem::path& path) const
{
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return fail(ErrorCode::Io, "Cannot write {}", mne::utf8_from_path(path));
    }
    write_to_stream(*stream);
    stream->end_file();
    stream->close();
    return {};
}

//=============================================================================================================

void FiffCoordTrans::write_to_stream(FiffStream& stream) const
{
    stream.write_coord_trans(*this);
}

//=============================================================================================================

MatrixX3f FiffCoordTrans::apply_trans(const MatrixX3f& rr, bool do_move) const
{
    MatrixX4f rr_ones(rr.rows(), 4);
    if (do_move) {
        rr_ones.setOnes();
    } else {
        rr_ones.setZero();
    }
    rr_ones.block(0, 0, rr.rows(), 3) = rr;
    return rr_ones * trans.block<3, 4>(0, 0).transpose();
}

//=============================================================================================================

MatrixX3f FiffCoordTrans::apply_inverse_trans(const MatrixX3f& rr, bool do_move) const
{
    MatrixX4f rr_ones(rr.rows(), 4);
    if (do_move) {
        rr_ones.setOnes();
    } else {
        rr_ones.setZero();
    }
    rr_ones.block(0, 0, rr.rows(), 3) = rr;
    return rr_ones * invtrans.block<3, 4>(0, 0).transpose();
}

//=============================================================================================================

std::string_view FiffCoordTrans::frame_name(int frame)
{
    switch (frame) {
        case FIFFV_COORD_UNKNOWN:
            return "unknown";
        case FIFFV_COORD_DEVICE:
            return "MEG device";
        case FIFFV_COORD_ISOTRAK:
            return "isotrak";
        case FIFFV_COORD_HPI:
            return "hpi";
        case FIFFV_COORD_HEAD:
            return "head";
        case FIFFV_COORD_MRI:
            return "MRI (surface RAS)";
        case FIFFV_MNE_COORD_MRI_VOXEL:
            return "MRI voxel";
        case FIFFV_COORD_MRI_SLICE:
            return "MRI slice";
        case FIFFV_COORD_MRI_DISPLAY:
            return "MRI display";
        case FIFFV_MNE_COORD_CTF_DEVICE:
            return "CTF MEG device";
        case FIFFV_MNE_COORD_CTF_HEAD:
            return "CTF/4D/KIT head";
        case FIFFV_MNE_COORD_RAS:
            return "RAS (non-zero origin)";
        case FIFFV_MNE_COORD_MNI_TAL:
            return "MNI Talairach";
        case FIFFV_MNE_COORD_FS_TAL_GTZ:
            return "Talairach (MNI z > 0)";
        case FIFFV_MNE_COORD_FS_TAL_LTZ:
            return "Talairach (MNI z < 0)";
        default:
            return "unknown";
    }
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(int from, int to, const Matrix3f& rot, const Vector3f& move)
{
    this->trans = MatrixXf::Zero(4, 4);

    this->from = from;
    this->to = to;

    this->trans.block<3, 3>(0, 0) = rot;
    this->trans.block<3, 1>(0, 3) = move;
    this->trans(3, 3) = 1.0f;

    add_inverse(*this);
}

//=============================================================================================================

FiffCoordTrans::FiffCoordTrans(int from, int to, const Matrix4f& matTrans, bool bStandard)
{
    this->trans = matTrans;
    this->from = from;
    this->to = to;

    if (bStandard) {
        // make sure that it is a standard transform if requested
        this->trans.row(3) = Vector4f(0, 0, 0, 1).transpose();
    }

    add_inverse(*this);
}

//=============================================================================================================

bool FiffCoordTrans::add_inverse(FiffCoordTrans& t)
{
    t.invtrans = t.trans.inverse().eval();
    return true;
}

//=============================================================================================================

void FiffCoordTrans::print() const
{
    mne::Log::info("Coordinate transformation: {} -> {}", frame_name(from), frame_name(to));
    for (int p = 0; p < 3; p++)
        mne::Log::info("\t{: 8.6f} {: 8.6f} {: 8.6f}\t{: 7.2f} mm", trans(p, 0), trans(p, 1), trans(p, 2), 1000 * trans(p, 3));
    mne::Log::info("\t{: 8.6f} {: 8.6f} {: 8.6f}   {: 7.2f}", trans(3, 0), trans(3, 1), trans(3, 2), trans(3, 3));
}

//=============================================================================================================

float FiffCoordTrans::angle_to(const Eigen::MatrixX4f& mTransDest) const
{
    MatrixX4f mDevHeadT = this->trans;
    Matrix3f mRot = mDevHeadT.block(0, 0, 3, 3);
    Matrix3f mRotNew = mTransDest.block(0, 0, 3, 3);

    Quaternionf quat(mRot);
    Quaternionf quatNew(mRotNew);

    float fAngle;

    // calculate rotation
    Quaternionf quatCompare;

    quatCompare = quat * quatNew.inverse();
    fAngle = quat.angularDistance(quatNew);
    fAngle = fAngle * 180 / std::numbers::pi;

    return fAngle;
}

//=============================================================================================================

float FiffCoordTrans::translation_to(const Eigen::MatrixX4f& mTransDest) const
{
    VectorXf vTrans = this->trans.col(3);
    VectorXf vTransDest = mTransDest.col(3);

    float fMove = (vTrans - vTransDest).norm();
    return fMove;
}

//=============================================================================================================

void FiffCoordTrans::apply_trans(float r[3], const FiffCoordTrans& t, bool do_move)
{
    float res[3];
    for (int j = 0; j < 3; j++) {
        res[j] = do_move ? t.trans(j, 3) : 0.0f;
        for (int k = 0; k < 3; k++)
            res[j] += t.trans(j, k) * r[k];
    }
    for (int j = 0; j < 3; j++)
        r[j] = res[j];
}

//=============================================================================================================

void FiffCoordTrans::apply_inverse_trans(float r[3], const FiffCoordTrans& t, bool do_move)
{
    float res[3];
    for (int j = 0; j < 3; j++) {
        res[j] = do_move ? t.invtrans(j, 3) : 0.0f;
        for (int k = 0; k < 3; k++)
            res[j] += t.invtrans(j, k) * r[k];
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
            a = s1;
            b = s2;
            found = true;
        } else if (s1.from == to && s2.from == from) {
            a = s1.inverted();
            b = s2;
            found = true;
        } else if (s1.to == to && s2.to == from) {
            a = s1;
            b = s2.inverted();
            found = true;
        } else if (s1.from == to && s2.to == from) {
            a = s1.inverted();
            b = s2.inverted();
            found = true;
        }
    }

    if (!found || a.from != b.to) {
        mne::Log::critical("Cannot combine coordinate transforms");
        return FiffCoordTrans();
    }

    // Catenate: result = a * b (apply b first, then a)
    FiffCoordTrans result;
    result.from = b.from;
    result.to = a.to;
    result.trans = a.trans * b.trans;
    result.trans.row(3) << 0.0f, 0.0f, 0.0f, 1.0f;
    add_inverse(result);
    return result;
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::from_cardinal_points(int from, int to,
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
    add_inverse(result);
    return result;
}

//=============================================================================================================

mne::Expected<FiffCoordTrans> FiffCoordTrans::read_ascii(const std::filesystem::path& path, int from, int to)
{
    const std::string name = mne::utf8_from_path(path);
    std::ifstream in(path);
    if (!in) {
        return fail(ErrorCode::Io, "Cannot open {}", name);
    }
    Matrix3f rot;
    Vector3f move;
    int row = 0;
    std::string line;
    while (row < 4 && std::getline(in, line)) {
        line = line.substr(0, line.find('#'));
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        // The last row of the 4 x 4 matrix is read but not used.
        if (row < 3) {
            std::istringstream values(line);
            if (!(values >> rot(row, 0) >> rot(row, 1) >> rot(row, 2) >> move[row])) {
                return fail(ErrorCode::Format, "Bad number in the coordinate transform in {}", name);
            }
        }
        ++row;
    }
    if (row < 4) {
        return fail(ErrorCode::Format, "Fewer than 4 rows of coordinate transform in {}", name);
    }
    return FiffCoordTrans(from, to, rot, move / 1000.0f);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::from_tag(const FiffTag& tag)
{
    FiffCoordTrans t;
    if (tag.is_matrix() || tag.data_type() != FIFFT_COORD_TRANS_STRUCT || tag.data() == nullptr)
        return t;

    const auto* t_pInt32 = reinterpret_cast<const std::int32_t*>(tag.data());
    t.from = t_pInt32[0];
    t.to = t_pInt32[1];

    const auto* t_pFloat = reinterpret_cast<const float*>(tag.data());
    int count = 0;
    int r, c;
    for (r = 0; r < 3; ++r) {
        t.trans(r, 3) = t_pFloat[11 + r];
        for (c = 0; c < 3; ++c) {
            t.trans(r, c) = t_pFloat[2 + count];
            ++count;
        }
    }
    t.trans(3, 0) = 0.0f;
    t.trans(3, 1) = 0.0f;
    t.trans(3, 2) = 0.0f;
    t.trans(3, 3) = 1.0f;

    count = 0;
    for (r = 0; r < 3; ++r) {
        t.invtrans(r, 3) = t_pFloat[23 + r];
        for (c = 0; c < 3; ++c) {
            t.invtrans(r, c) = t_pFloat[14 + count];
            ++count;
        }
    }
    t.invtrans(3, 0) = 0.0f;
    t.invtrans(3, 1) = 0.0f;
    t.invtrans(3, 2) = 0.0f;
    t.invtrans(3, 3) = 1.0f;

    return t;
}

//=============================================================================================================

mne::Expected<FiffCoordTrans> FiffCoordTrans::read_from_node(FiffStream& stream,
                                                             const std::shared_ptr<FiffDirNode>& node,
                                                             int from, int to)
{
    return find_transform(stream, node->dir, from, to);
}

//=============================================================================================================

FiffCoordTrans FiffCoordTrans::procrustes_align(int from_frame,
                                                int to_frame,
                                                const Eigen::MatrixXf& fromPts,
                                                const Eigen::MatrixXf& toPts,
                                                const Eigen::VectorXf& w,
                                                float max_diff)
{
    int np = fromPts.rows();

    /* Calculate centroids and subtract */
    Eigen::Vector3f from0 = fromPts.colwise().mean();
    Eigen::Vector3f to0 = toPts.colwise().mean();

    Eigen::MatrixXf fromC = fromPts.rowwise() - from0.transpose();
    Eigen::MatrixXf toC = toPts.rowwise() - to0.transpose();

    /* Compute the cross-covariance matrix S */
    Eigen::Matrix3f S;
    if (w.size() > 0) {
        S = fromC.transpose() * w.asDiagonal() * toC;
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
            mne::Log::warning("Too large difference in matching : {:7.1f} > {:7.1f} mm", 1000.0f * diff, 1000.0f * max_diff);
            return FiffCoordTrans();
        }
    }

    return FiffCoordTrans(from_frame, to_frame, R, moveVec);
}
