//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file maxwell_movement_comp.cpp
 * @since May 2026
 * @brief Maxwell movement compensation implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "maxwell_movement_comp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
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
    for (int i = 0; i < info.chs.size(); ++i) {
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
                                     const QList<HeadPosEntry>& headPos,
                                     double dSFreq,
                                     const MaxwellMoveCompParams& params)
{
    if (headPos.isEmpty()) {
        qWarning() << "[MaxwellMovementComp::apply] No head positions provided.";
        return matData;
    }

    const int nChannels = matData.rows();
    const int nSamples = matData.cols();

    // Determine reference head position
    HeadPosEntry refPos;
    if (params.iRefIdx >= 0 && params.iRefIdx < headPos.size()) {
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
    for (int iSeg = 0; iSeg < headPos.size(); ++iSeg) {
        // Determine sample range for this segment
        int iStart = static_cast<int>(headPos[iSeg].dTime * dSFreq);
        int iEnd;
        if (iSeg + 1 < headPos.size()) {
            iEnd = static_cast<int>(headPos[iSeg + 1].dTime * dSFreq);
        } else {
            iEnd = nSamples;
        }

        iStart = qBound(0, iStart, nSamples);
        iEnd = qBound(iStart, iEnd, nSamples);
        if (iStart >= iEnd) continue;

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
        for (int ch = 0; ch < basisCurrent.megChannelIdx.size(); ++ch) {
            megData.row(ch) = segData.row(basisCurrent.megChannelIdx[ch]);
        }

        MatrixXd multipoles = basisCurrent.matPinvAll * megData;

        // Reconstruct at reference position using only internal multipoles
        MatrixXd reconstructed = basisRef.matSin * multipoles.topRows(basisRef.iNin);

        // Place reconstructed data back
        for (int ch = 0; ch < basisRef.megChannelIdx.size(); ++ch) {
            result.row(basisRef.megChannelIdx[ch]).segment(iStart, nSegSamples) = reconstructed.row(ch);
        }
    }

    return result;
}

//=============================================================================================================

QList<HeadPosEntry> MaxwellMovementComp::readHeadPos(const QString& sPath)
{
    QList<HeadPosEntry> positions;

    QFile file(sPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[MaxwellMovementComp::readHeadPos] Cannot open file:" << sPath;
        return positions;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 7) continue;

        bool ok = false;
        HeadPosEntry entry;
        entry.dTime = parts[0].toDouble(&ok); if (!ok) continue;

        // Quaternion (scalar-last or scalar-first depends on convention)
        double q1 = parts[1].toDouble(&ok); if (!ok) continue;
        double q2 = parts[2].toDouble(&ok); if (!ok) continue;
        double q3 = parts[3].toDouble(&ok); if (!ok) continue;

        double tx = parts[4].toDouble(&ok); if (!ok) continue;
        double ty = parts[5].toDouble(&ok); if (!ok) continue;
        double tz = parts[6].toDouble(&ok); if (!ok) continue;

        if (parts.size() >= 8) {
            entry.dGof = parts[7].toDouble();
        }

        // MNE convention: quaternion stored as (q1, q2, q3) with q0 computed
        double q0sq = 1.0 - q1*q1 - q2*q2 - q3*q3;
        double q0 = (q0sq > 0.0) ? std::sqrt(q0sq) : 0.0;
        entry.rotation = Quaterniond(q0, q1, q2, q3);
        entry.translation = Vector3d(tx, ty, tz);

        positions.append(entry);
    }

    file.close();
    return positions;
}

//=============================================================================================================

bool MaxwellMovementComp::writeHeadPos(const QString& sPath,
                                        const QList<HeadPosEntry>& headPos)
{
    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[MaxwellMovementComp::writeHeadPos] Cannot open file:" << sPath;
        return false;
    }

    QTextStream out(&file);
    out << "# Head position file\n";
    out << "# time  q1  q2  q3  tx  ty  tz  gof\n";

    for (const auto& entry : headPos) {
        // Store quaternion as (q1, q2, q3), dropping q0 (recoverable)
        out << QString::number(entry.dTime, 'f', 6) << " "
            << QString::number(entry.rotation.x(), 'f', 6) << " "
            << QString::number(entry.rotation.y(), 'f', 6) << " "
            << QString::number(entry.rotation.z(), 'f', 6) << " "
            << QString::number(entry.translation.x(), 'f', 6) << " "
            << QString::number(entry.translation.y(), 'f', 6) << " "
            << QString::number(entry.translation.z(), 'f', 6) << " "
            << QString::number(entry.dGof, 'f', 6) << "\n";
    }

    file.close();
    return true;
}
