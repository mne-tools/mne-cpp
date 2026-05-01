//=============================================================================================================
/**
 * @file     inv_source_estimate_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    InvSourceEstimateIO implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate_io.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool InvSourceEstimateIO::writeCsv(const InvSourceEstimate& stc,
                                    const QString& sPath,
                                    char cDelim)
{
    if (stc.isEmpty()) {
        qWarning() << "[InvSourceEstimateIO::writeCsv] Source estimate is empty.";
        return false;
    }

    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[InvSourceEstimateIO::writeCsv] Cannot open:" << sPath;
        return false;
    }

    QTextStream out(&file);
    const QString delim(QChar::fromLatin1(cDelim));

    // Header: time, vertex_0, vertex_1, ...
    out << "time";
    for (int v = 0; v < stc.vertices.size(); ++v) {
        out << delim << stc.vertices(v);
    }
    out << "\n";

    // Data rows: one per time point
    for (int t = 0; t < stc.data.cols(); ++t) {
        double time = stc.tmin + t * stc.tstep;
        out << QString::number(time, 'f', 6);
        for (int v = 0; v < stc.data.rows(); ++v) {
            out << delim << QString::number(stc.data(v, t), 'e', 8);
        }
        out << "\n";
    }

    file.close();
    return true;
}

//=============================================================================================================

InvSourceEstimate InvSourceEstimateIO::readCsv(const QString& sPath,
                                                char cDelim)
{
    QFile file(sPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[InvSourceEstimateIO::readCsv] Cannot open:" << sPath;
        return InvSourceEstimate();
    }

    QTextStream in(&file);
    const QString delim(QChar::fromLatin1(cDelim));

    // Read header
    QString header = in.readLine();
    QStringList headerParts = header.split(delim, Qt::SkipEmptyParts);
    if (headerParts.size() < 2) {
        qWarning() << "[InvSourceEstimateIO::readCsv] Invalid header.";
        return InvSourceEstimate();
    }

    // Parse vertex indices from header (skip "time")
    int nVertices = headerParts.size() - 1;
    VectorXi vertices(nVertices);
    for (int v = 0; v < nVertices; ++v) {
        vertices(v) = headerParts[v + 1].toInt();
    }

    // Read data rows
    QList<VectorXd> timePoints;
    QList<double> times;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(delim, Qt::SkipEmptyParts);
        if (parts.size() < nVertices + 1) continue;

        times.append(parts[0].toDouble());
        VectorXd row(nVertices);
        for (int v = 0; v < nVertices; ++v) {
            row(v) = parts[v + 1].toDouble();
        }
        timePoints.append(row);
    }

    file.close();

    if (timePoints.isEmpty()) {
        return InvSourceEstimate();
    }

    // Build data matrix (n_vertices × n_times)
    MatrixXd data(nVertices, timePoints.size());
    for (int t = 0; t < timePoints.size(); ++t) {
        data.col(t) = timePoints[t];
    }

    float tmin = static_cast<float>(times[0]);
    float tstep = (times.size() > 1)
                  ? static_cast<float>(times[1] - times[0])
                  : 0.001f;

    return InvSourceEstimate(data, vertices, tmin, tstep);
}

//=============================================================================================================

bool InvSourceEstimateIO::writeMatrix(const InvSourceEstimate& stc,
                                       const QString& sPath)
{
    if (stc.isEmpty()) {
        qWarning() << "[InvSourceEstimateIO::writeMatrix] Source estimate is empty.";
        return false;
    }

    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[InvSourceEstimateIO::writeMatrix] Cannot open:" << sPath;
        return false;
    }

    QTextStream out(&file);
    for (int v = 0; v < stc.data.rows(); ++v) {
        for (int t = 0; t < stc.data.cols(); ++t) {
            if (t > 0) out << "\t";
            out << QString::number(stc.data(v, t), 'e', 8);
        }
        out << "\n";
    }

    file.close();
    return true;
}
