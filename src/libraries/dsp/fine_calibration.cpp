//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fine_calibration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
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

        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() != 14 && parts.size() != 16) {
            qWarning() << "[FineCalibration::read] Skipping malformed line:" << line;
            continue;
        }

        FineCalEntry entry;
        bool ok = false;
        entry.chNumber = QString(parts[0]).remove(QStringLiteral("MEG")).toInt(&ok);
        VectorXd values(parts.size() - 1);
        for (int k = 1; ok && k < parts.size(); ++k) {
            values(k - 1) = parts[k].toDouble(&ok);
        }
        if (!ok) {
            qWarning() << "[FineCalibration::read] Skipping malformed line:" << line;
            continue;
        }
        entry.position = values.head<3>();
        entry.orientation = Map<const Matrix<double, 3, 3, RowMajor>>(values.data() + 3);
        entry.imbalance = values.tail(values.size() - 12);

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
    for (const auto& entry : m_entries) {
        out << QString("%1").arg(entry.chNumber, 4, 10, QChar('0'));
        const Matrix<double, 3, 3, RowMajor> axes = entry.orientation;
        for (const double v : {entry.position(0), entry.position(1), entry.position(2)}) {
            out << ' ' << QString::number(v, 'f', 6);
        }
        for (int k = 0; k < 9; ++k) {
            out << ' ' << QString::number(axes.data()[k], 'f', 6);
        }
        for (Index k = 0; k < entry.imbalance.size(); ++k) {
            out << ' ' << QString::number(entry.imbalance(k), 'f', 6);
        }
        out << '\n';
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
    VectorXd gains = VectorXd::Ones(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].chNumber % 10 == 1) {
            gains(i) = m_entries[i].imbalance(0);
        }
    }
    return gains;
}

//=============================================================================================================

MatrixXd FineCalibration::imbalanceMatrix() const
{
    MatrixXd imb = MatrixXd::Zero(m_entries.size(), 3);
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].chNumber % 10 != 1) {
            imb.row(i).head(m_entries[i].imbalance.size()) = m_entries[i].imbalance.transpose();
        }
    }
    return imb;
}
