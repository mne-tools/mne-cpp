//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fine_calibration.cpp
 * @since 2026
 * @date  May 2026
 * @brief Fine calibration implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fine_calibration.h"

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
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FineCalibration FineCalibration::read(const QString& sPath)
{
    FineCalibration cal;

    QFile file(sPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FineCalibration::read] Cannot open file:" << sPath;
        return cal;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 5) {
            qWarning() << "[FineCalibration::read] Skipping malformed line:" << line;
            continue;
        }

        FineCalEntry entry;
        bool ok = false;
        entry.chNumber = parts[0].toInt(&ok);
        if (!ok) continue;

        entry.dGain = parts[1].toDouble(&ok);
        if (!ok) continue;

        double ix = parts[2].toDouble(&ok); if (!ok) continue;
        double iy = parts[3].toDouble(&ok); if (!ok) continue;
        double iz = parts[4].toDouble(&ok); if (!ok) continue;
        entry.imbalance = Vector3d(ix, iy, iz);

        cal.addEntry(entry);
    }

    file.close();
    return cal;
}

//=============================================================================================================

bool FineCalibration::write(const QString& sPath) const
{
    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FineCalibration::write] Cannot open file for writing:" << sPath;
        return false;
    }

    QTextStream out(&file);
    out << "# Fine calibration file\n";
    out << "# channel_number  gain  imbalance_x  imbalance_y  imbalance_z\n";

    for (const auto& entry : m_entries) {
        out << entry.chNumber << " "
            << QString::number(entry.dGain, 'f', 6) << " "
            << QString::number(entry.imbalance.x(), 'e', 6) << " "
            << QString::number(entry.imbalance.y(), 'e', 6) << " "
            << QString::number(entry.imbalance.z(), 'e', 6) << "\n";
    }

    file.close();
    return true;
}

//=============================================================================================================

bool FineCalibration::findEntry(int chNumber, FineCalEntry& entry) const
{
    for (const auto& e : m_entries) {
        if (e.chNumber == chNumber) {
            entry = e;
            return true;
        }
    }
    return false;
}

//=============================================================================================================

VectorXd FineCalibration::gainVector() const
{
    VectorXd gains(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        gains(i) = m_entries[i].dGain;
    }
    return gains;
}

//=============================================================================================================

MatrixXd FineCalibration::imbalanceMatrix() const
{
    MatrixXd imb(m_entries.size(), 3);
    for (int i = 0; i < m_entries.size(); ++i) {
        imb.row(i) = m_entries[i].imbalance.transpose();
    }
    return imb;
}
