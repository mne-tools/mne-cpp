//=============================================================================================================
/**
 * @file     fine_calibration.cpp
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
 * @brief    Fine calibration implementation.
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
