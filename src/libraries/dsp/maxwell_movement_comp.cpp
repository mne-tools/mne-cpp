//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     maxwell_movement_comp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Maxwell movement compensation implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "maxwell_movement_comp.h"
#include <base/byte_stream.h>
#include <base/log.h>
#include <base/string_utils.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfo MaxwellMovementComp::transformFiffInfo(const FiffInfo& fiffInfo,
                                                const HeadPosEntry& headPosRef,
                                                const HeadPosEntry& headPosCurrent)
{
    FiffInfo info(fiffInfo);

    // Compute relative rotation: from current → ref
    // R_rel = R_ref * R_current^-1
    Matrix3d rotRef = headPosRef.rotation.toRotationMatrix();
    Matrix3d rotCur = headPosCurrent.rotation.toRotationMatrix();
    Matrix3d rotRel = rotRef * rotCur.transpose();

    // Translation: t_rel = t_ref - R_rel * t_current
    Vector3d tRel = headPosRef.translation - rotRel * headPosCurrent.translation;

    // Apply transform to MEG channel positions
    for (int i = 0; i < std::ssize(info.chs); ++i) {
        if (info.chs[i].kind == FIFFV_MEG_CH || info.chs[i].kind == FIFFV_REF_MEG_CH) {
            // Transform position
            Vector3f r0 = info.chs[i].chpos.r0;
            Vector3d r0d = r0.cast<double>();
            r0d = rotRel * r0d + tRel;
            info.chs[i].chpos.r0 = r0d.cast<float>();

            // Transform orientation vectors
            Vector3f ex = info.chs[i].chpos.ex;
            Vector3d exd = ex.cast<double>();
            exd = rotRel * exd;
            info.chs[i].chpos.ex = exd.cast<float>();

            Vector3f ey = info.chs[i].chpos.ey;
            Vector3d eyd = ey.cast<double>();
            eyd = rotRel * eyd;
            info.chs[i].chpos.ey = eyd.cast<float>();

            Vector3f ez = info.chs[i].chpos.ez;
            Vector3d ezd = ez.cast<double>();
            ezd = rotRel * ezd;
            info.chs[i].chpos.ez = ezd.cast<float>();
        }
    }

    return info;
}

//=============================================================================================================

MatrixXd MaxwellMovementComp::apply(const MatrixXd& matData,
                                    const FiffInfo& fiffInfo,
                                    const std::vector<HeadPosEntry>& headPos,
                                    double dSFreq,
                                    const MaxwellMoveCompParams& params)
{
    if (headPos.empty()) {
        Log::warning("[MaxwellMovementComp::apply] No head positions provided.");
        return matData;
    }

    const int nSamples = matData.cols();

    // Determine reference head position
    HeadPosEntry refPos;
    if (params.iRefIdx >= 0 && params.iRefIdx < std::ssize(headPos)) {
        refPos = headPos[params.iRefIdx];
    } else {
        // Mean position
        Vector3d meanTrans = Vector3d::Zero();
        for (const auto& hp : headPos) {
            meanTrans += hp.translation;
        }
        meanTrans /= headPos.size();
        refPos.translation = meanTrans;
        refPos.rotation = headPos[0].rotation; // Use first rotation as reference
    }

    // Compute reference SSS basis
    SSSParams sssParams;
    sssParams.iOrderIn = params.iOrderIn;
    sssParams.iOrderOut = params.iOrderOut;
    sssParams.origin = params.origin;
    sssParams.dRegIn = params.dRegIn;

    SSS::Basis basisRef = SSS::computeBasis(fiffInfo, sssParams);

    MatrixXd result = matData;

    // Process each time segment
    for (int iSeg = 0; iSeg < std::ssize(headPos); ++iSeg) {
        // Determine sample range for this segment
        int iStart = static_cast<int>(headPos[iSeg].dTime * dSFreq);
        int iEnd;
        if (iSeg + 1 < std::ssize(headPos)) {
            iEnd = static_cast<int>(headPos[iSeg + 1].dTime * dSFreq);
        } else {
            iEnd = nSamples;
        }

        iStart = std::clamp(iStart, 0, nSamples);
        iEnd = std::clamp(iEnd, iStart, nSamples);
        if (iStart >= iEnd)
            continue;

        int nSegSamples = iEnd - iStart;

        // Transform FiffInfo to current head position
        FiffInfo infoCurrentToRef = transformFiffInfo(fiffInfo, refPos, headPos[iSeg]);

        // Compute SSS basis at current head position (as seen from reference)
        SSS::Basis basisCurrent = SSS::computeBasis(infoCurrentToRef, sssParams);

        // Extract segment data
        MatrixXd segData = matData.middleCols(iStart, nSegSamples);

        // Project into multipole space using current-position basis
        // multipoles = pinv_current * data_meg
        MatrixXd megData(basisCurrent.megChannelIdx.size(), nSegSamples);
        for (int ch = 0; ch < std::ssize(basisCurrent.megChannelIdx); ++ch) {
            megData.row(ch) = segData.row(basisCurrent.megChannelIdx[ch]);
        }

        MatrixXd multipoles = basisCurrent.matPinvAll * megData;

        // Reconstruct at reference position using only internal multipoles
        MatrixXd reconstructed = basisRef.matSin * multipoles.topRows(basisRef.iNin);

        // Place reconstructed data back
        for (int ch = 0; ch < std::ssize(basisRef.megChannelIdx); ++ch) {
            result.row(basisRef.megChannelIdx[ch]).segment(iStart, nSegSamples) = reconstructed.row(ch);
        }
    }

    return result;
}

//=============================================================================================================

Expected<std::vector<HeadPosEntry>> MaxwellMovementComp::read_head_pos(const std::filesystem::path& path)
{
    const Expected<std::string> text = read_text_file(path);
    if (!text) {
        return std::unexpected(text.error());
    }
    std::vector<HeadPosEntry> positions;
    std::string_view rest = *text;
    for (bool header = true; !rest.empty(); header = false) {
        const std::size_t end = rest.find('\n');
        const std::string_view line = rest.substr(0, end);
        rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
        const std::vector<std::string_view> fields = split_whitespace(line);
        if (header || fields.empty() || fields.front().starts_with('#')) {
            continue;
        }
        std::array<double, 10> values{};
        bool ok = fields.size() == values.size();
        for (std::size_t k = 0; ok && k < values.size(); ++k) {
            const std::optional<double> value = to_double(fields[k]);
            ok = value.has_value();
            values[k] = value.value_or(0.0);
        }
        if (!ok) {
            return fail(ErrorCode::Format, "{} has no 10 head position values in line \"{}\"", utf8_from_path(path),
                        line);
        }
        HeadPosEntry& entry = positions.emplace_back();
        entry.dTime = values[0];
        const Vector3d q(values[1], values[2], values[3]);
        entry.rotation = Quaterniond(std::sqrt(std::max(0.0, 1.0 - q.squaredNorm())), q.x(), q.y(), q.z());
        entry.translation = Vector3d(values[4], values[5], values[6]);
        entry.dGof = values[7];
        entry.dError = values[8];
        entry.dVelocity = values[9];
    }
    return positions;
}

//=============================================================================================================

Expected<void> MaxwellMovementComp::write_head_pos(const std::filesystem::path& path,
                                                   std::span<const HeadPosEntry> headPos)
{
    std::string text = " Time       q1       q2       q3       q4       q5       q6       g-value  error    velocity\n";
    for (const HeadPosEntry& entry : headPos) {
        // The file holds the quaternion with a non-negative scalar part: q and -q are the same rotation
        const Quaterniond q = entry.rotation.w() < 0.0 ? Quaterniond(-entry.rotation.coeffs()) : entry.rotation;
        std::format_to(std::back_inserter(text), " {: 9.3f}", entry.dTime);
        for (const double value : {q.x(), q.y(), q.z(), entry.translation.x(), entry.translation.y(),
                                   entry.translation.z(), entry.dGof, entry.dError, entry.dVelocity}) {
            std::format_to(std::back_inserter(text), " {: 8.5f}", value);
        }
        text += '\n';
    }
    return write_text_file(path, text);
}
