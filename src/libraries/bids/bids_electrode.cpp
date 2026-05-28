//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_electrode.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BidsElectrode — TSV I/O and FIFF digitizer-point conversion for ``_electrodes.tsv``.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_electrode.h"
#include "bids_tsv.h"

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{
static const QString NA = QStringLiteral("n/a");

QString naToEmpty(const QString& s)
{
    return (s == NA) ? QString() : s;
}
} // anonymous namespace

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

QList<BidsElectrode> BidsElectrode::readTsv(const QString& sFilePath)
{
    QStringList headers;
    QList<BidsTsvRow> rawRows = BidsTsv::readTsv(sFilePath, headers);

    QList<BidsElectrode> electrodes;
    electrodes.reserve(rawRows.size());

    for(const auto& row : rawRows) {
        BidsElectrode elec;
        elec.name       = row.value(QStringLiteral("name"));
        elec.x          = row.value(QStringLiteral("x"), NA);
        elec.y          = row.value(QStringLiteral("y"), NA);
        elec.z          = row.value(QStringLiteral("z"), NA);
        elec.size       = naToEmpty(row.value(QStringLiteral("size")));
        elec.type       = naToEmpty(row.value(QStringLiteral("type")));
        elec.material   = naToEmpty(row.value(QStringLiteral("material")));
        elec.impedance  = naToEmpty(row.value(QStringLiteral("impedance")));
        electrodes.append(elec);
    }

    return electrodes;
}

//=============================================================================================================

bool BidsElectrode::writeTsv(const QString& sFilePath,
                             const QList<BidsElectrode>& electrodes)
{
    QStringList headers = {
        QStringLiteral("name"),
        QStringLiteral("x"),
        QStringLiteral("y"),
        QStringLiteral("z"),
        QStringLiteral("size"),
        QStringLiteral("type"),
        QStringLiteral("material"),
        QStringLiteral("impedance"),
    };

    QList<BidsTsvRow> rows;
    rows.reserve(electrodes.size());

    for(const auto& elec : electrodes) {
        BidsTsvRow row;
        row[QStringLiteral("name")]      = elec.name;
        row[QStringLiteral("x")]         = elec.x;
        row[QStringLiteral("y")]         = elec.y;
        row[QStringLiteral("z")]         = elec.z;
        row[QStringLiteral("size")]      = elec.size;
        row[QStringLiteral("type")]      = elec.type;
        row[QStringLiteral("material")]  = elec.material;
        row[QStringLiteral("impedance")] = elec.impedance;
        rows.append(row);
    }

    return BidsTsv::writeTsv(sFilePath, headers, rows);
}

//=============================================================================================================

FIFFLIB::FiffDigPointSet BidsElectrode::toFiffDigPoints(
    const QList<BidsElectrode>& electrodes,
    const FIFFLIB::FiffCoordTrans& trans)
{
    using namespace FIFFLIB;

    FiffDigPointSet digSet;
    int ident = 1;

    const bool hasTransform = (trans.from != 0 || trans.to != 0);

    for (const BidsElectrode& elec : electrodes) {
        // Skip electrodes with "n/a" coordinates
        bool xOk = false, yOk = false, zOk = false;
        float xVal = elec.x.toFloat(&xOk);
        float yVal = elec.y.toFloat(&yOk);
        float zVal = elec.z.toFloat(&zOk);

        if (!xOk || !yOk || !zOk) {
            continue;
        }

        FiffDigPoint point;
        point.kind  = FIFFV_POINT_EEG;
        point.ident = ident++;
        point.r[0]  = xVal;
        point.r[1]  = yVal;
        point.r[2]  = zVal;

        // Apply coordinate transform if provided
        if (hasTransform) {
            Eigen::Vector3f pos(point.r[0], point.r[1], point.r[2]);
            Eigen::Vector3f transformed = (trans.trans.block<3,3>(0,0).cast<float>() * pos
                                           + trans.trans.block<3,1>(0,3).cast<float>());
            point.r[0] = transformed(0);
            point.r[1] = transformed(1);
            point.r[2] = transformed(2);
        }

        digSet << point;
    }

    return digSet;
}
