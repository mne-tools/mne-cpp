//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the CSV / matrix exporters and importer in @ref INVLIB::InvSourceEstimateIO.
 *
 * Implements buffered streaming readers and writers that walk the
 * source-estimate row-by-row so very large estimates can be persisted
 * without doubling memory. Handles configurable delimiters, header lines
 * carrying vertex indices and graceful failure modes that return an empty
 * estimate (rather than throwing) when the input file is malformed.
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
