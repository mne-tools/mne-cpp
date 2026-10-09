//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     standard_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Standard 10-20 / 10-10 / 10-05 electrode positions from MNE-Python's standard_1005 table.
 *
 * The positions are those of MNE-Python's standard_1005.elc (mne.channels.make_standard_montage),
 * compiled into the library as a Qt resource so the standard caps are available without data
 * files on disk (static installers, WebAssembly). The 10-20 and 10-10 systems are the subsets of
 * that table named in the two lists below.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "standard_montage.h"

#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

// Q_INIT_RESOURCE must not be expanded inside a namespace; static builds need it.
static void initStandardMontageResource()
{
    Q_INIT_RESOURCE(standard_montage);
}

namespace
{

// Every position of standard_1005.elc in metres, fiducials included, read as mne's _mgh_or_standard does.
const QList<ElectrodePosition>& standard1005Table()
{
    static const QList<ElectrodePosition> table = [] {
        initStandardMontageResource();
        QList<ElectrodePosition> positions;
        QFile file(QStringLiteral(":/utils/montages/standard_1005.elc"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning("[StandardMontage] the built-in standard_1005 table is missing");
            return positions;
        }
        QTextStream in(&file);
        while (!in.atEnd() && in.readLine() != QLatin1String("Positions")) {
        }
        QList<Vector3d> coordinates;
        for (QString line = in.readLine(); !in.atEnd() && line != QLatin1String("Labels"); line = in.readLine()) {
            const QStringList fields = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (fields.size() == 3)
                coordinates.append(Vector3d(fields[0].toDouble(), fields[1].toDouble(), fields[2].toDouble()) / 1000.0);
        }
        for (const Vector3d& r : coordinates) {
            const QString name = in.readLine().trimmed();
            if (name.isEmpty())
                break;
            positions.append({name, r});
        }
        return positions;
    }();
    return table;
}

// The positions of the named electrodes, in the order given.
QList<ElectrodePosition> pick(const QStringList& names)
{
    QList<ElectrodePosition> positions;
    for (const QString& name : names) {
        for (const ElectrodePosition& electrode : standard1005Table()) {
            if (electrode.name == name) {
                positions.append(electrode);
                break;
            }
        }
    }
    return positions;
}

const QStringList& names1020()
{
    static const QStringList names{"Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "T7", "C3", "Cz", "C4",
                                   "T8", "P7", "P3", "Pz", "P4", "P8", "O1", "O2", "A1", "A2"};
    return names;
}

const QStringList& names1010()
{
    static const QStringList names = names1020() + QStringList{"AF7", "AF3", "AFz", "AF4", "AF8", "F5", "F1", "F2", "F6", "FT7", "FC5", "FC3", "FC1", "FCz", "FC2", "FC4", "FC6", "FT8", "C5", "C1", "C2", "C6", "TP7", "CP5", "CP3", "CP1", "CPz", "CP2", "CP4", "CP6", "TP8", "P5", "P1", "P2", "P6", "PO7", "PO3", "POz", "PO4", "PO8", "Oz", "FT9", "FT10", "TP9", "TP10", "Fpz", "Iz", "Nz", "F9", "F10", "P9", "P10"};
    return names;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QList<ElectrodePosition> StandardMontage::getMontage(System system)
{
    switch (system) {
        case System::Standard_1020:
            return pick(names1020());
        case System::Standard_1010:
            return pick(names1010());
        case System::Standard_1005: {
            // mne's standard_1005 montage: every electrode, the fiducials are not channels
            QList<ElectrodePosition> positions = standard1005Table();
            positions.removeIf([](const ElectrodePosition& electrode) {
                return electrode.name == QLatin1String("LPA") || electrode.name == QLatin1String("RPA") || electrode.name == QLatin1String("Nz");
            });
            return positions;
        }
    }
    return {};
}

//=============================================================================================================

QStringList StandardMontage::getElectrodeNames(System system)
{
    QStringList names;
    for (const auto& ep : getMontage(system)) {
        names << ep.name;
    }
    return names;
}

//=============================================================================================================

bool StandardMontage::findElectrode(const QString& name, Vector3d& pos)
{
    for (const auto& ep : standard1005Table()) {
        if (ep.name.compare(name, Qt::CaseInsensitive) == 0) {
            pos = ep.pos;
            return true;
        }
    }
    return false;
}

//=============================================================================================================

int StandardMontage::electrodeCount(System system)
{
    return getMontage(system).size();
}
