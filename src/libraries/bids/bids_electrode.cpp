//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_electrode.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BidsElectrode — TSV I/O and FIFF digitizer-point conversion for ``_electrodes.tsv``.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_electrode.h"
#include "bids_tsv.h"

#include <Eigen/Core>

#include <base/string_utils.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{

std::string naToEmpty(std::string_view s)
{
    return s == "n/a" ? std::string() : std::string(s);
}

} // anonymous namespace

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

std::vector<BidsElectrode> BidsElectrode::read_tsv(const std::filesystem::path& path)
{
    std::vector<std::string> headers;
    const std::vector<BidsTsvRow> rawRows = BidsTsv::read_tsv(path, headers);

    std::vector<BidsElectrode> electrodes;
    electrodes.reserve(rawRows.size());

    for (const BidsTsvRow& row : rawRows) {
        BidsElectrode elec;
        elec.name = BidsTsv::value(row, "name");
        elec.x = BidsTsv::value(row, "x", "n/a");
        elec.y = BidsTsv::value(row, "y", "n/a");
        elec.z = BidsTsv::value(row, "z", "n/a");
        elec.size = naToEmpty(BidsTsv::value(row, "size"));
        elec.type = naToEmpty(BidsTsv::value(row, "type"));
        elec.material = naToEmpty(BidsTsv::value(row, "material"));
        elec.impedance = naToEmpty(BidsTsv::value(row, "impedance"));
        elec.group = naToEmpty(BidsTsv::value(row, "group"));
        elec.dimension = naToEmpty(BidsTsv::value(row, "dimension"));
        electrodes.push_back(std::move(elec));
    }

    return electrodes;
}

//=============================================================================================================

bool BidsElectrode::write_tsv(const std::filesystem::path& path,
                              std::span<const BidsElectrode> electrodes)
{
    const std::vector<std::string> headers{"name", "x", "y", "z", "size",
                                           "type", "material", "impedance", "group", "dimension"};

    std::vector<BidsTsvRow> rows;
    rows.reserve(electrodes.size());

    for (const BidsElectrode& elec : electrodes) {
        rows.push_back({{"name", elec.name},
                        {"x", elec.x},
                        {"y", elec.y},
                        {"z", elec.z},
                        {"size", elec.size},
                        {"type", elec.type},
                        {"material", elec.material},
                        {"impedance", elec.impedance},
                        {"group", elec.group},
                        {"dimension", elec.dimension}});
    }

    return BidsTsv::write_tsv(path, headers, rows);
}

//=============================================================================================================

FIFFLIB::FiffDigPointSet BidsElectrode::to_fiff_dig_points(
    std::span<const BidsElectrode> electrodes,
    const FIFFLIB::FiffCoordTrans& trans)
{
    using namespace FIFFLIB;

    FiffDigPointSet digSet;
    int ident = 1;

    const bool hasTransform = !trans.empty();

    for (const BidsElectrode& elec : electrodes) {
        // Skip electrodes with "n/a" coordinates
        const std::optional<double> x = BASELIB::toDouble(elec.x);
        const std::optional<double> y = BASELIB::toDouble(elec.y);
        const std::optional<double> z = BASELIB::toDouble(elec.z);
        if (!x || !y || !z) {
            continue;
        }

        FiffDigPoint point;
        point.kind = FIFFV_POINT_EEG;
        point.ident = ident++;
        point.coord_frame = hasTransform ? trans.to : FIFFV_COORD_UNKNOWN;
        point.r[0] = static_cast<float>(*x);
        point.r[1] = static_cast<float>(*y);
        point.r[2] = static_cast<float>(*z);

        // Apply coordinate transform if provided
        if (hasTransform) {
            Eigen::Vector3f pos(point.r[0], point.r[1], point.r[2]);
            Eigen::Vector3f transformed = (trans.trans.block<3, 3>(0, 0).cast<float>() * pos + trans.trans.block<3, 1>(0, 3).cast<float>());
            point.r[0] = transformed(0);
            point.r[1] = transformed(1);
            point.r[2] = transformed(2);
        }

        digSet.push_back(point);
    }

    return digSet;
}
