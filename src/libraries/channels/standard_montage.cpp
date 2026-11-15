//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     standard_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::StandardMontage.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "standard_montage.h"
#include "layoutloader.h"

#include <base/log.h>

#include <algorithm>
#include <array>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

// MNE-Python's standard_1005.elc, embedded by CMake from data/standard_1005.elc
constexpr std::string_view standard1005Elc =
#include "standard_1005_elc.inc"
    ;

// Every position of standard_1005.elc in metres, fiducials included, read as mne's _mgh_or_standard does.
const std::vector<ElectrodePosition>& standard1005Table()
{
    static const std::vector<ElectrodePosition> table = [] {
        std::vector<ElectrodePosition> positions;
        const auto layout = LayoutLoader::parse_elc(standard1005Elc);
        if (!layout) {
            Log::warning("[StandardMontage] the built-in standard_1005 table is malformed");
            return positions;
        }
        positions.reserve(layout->names.size());
        for (std::size_t k = 0; k < layout->names.size(); ++k)
            positions.push_back({layout->names[k], layout->positions_3d[k] / 1000.0});
        return positions;
    }();
    return table;
}

// The positions of the named electrodes, in the order given.
template<std::size_t N>
std::vector<ElectrodePosition> pick(const std::array<std::string_view, N>& names)
{
    std::vector<ElectrodePosition> positions;
    positions.reserve(N);
    for (const std::string_view name : names) {
        const auto& table = standard1005Table();
        const auto it = std::ranges::find(table, name, &ElectrodePosition::name);
        if (it != table.end())
            positions.push_back(*it);
    }
    return positions;
}

constexpr auto names1020 = std::to_array<std::string_view>({"Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "T7", "C3", "Cz", "C4",
                                                            "T8", "P7", "P3", "Pz", "P4", "P8", "O1", "O2", "A1", "A2"});

constexpr auto names1010 = std::to_array<std::string_view>({"Fp1", "Fp2", "F7", "F3", "Fz", "F4", "F8", "T7", "C3", "Cz", "C4", "T8", "P7", "P3", "Pz", "P4", "P8", "O1", "O2",
                                                            "A1", "A2", "AF7", "AF3", "AFz", "AF4", "AF8", "F5", "F1", "F2", "F6", "FT7", "FC5", "FC3", "FC1", "FCz", "FC2",
                                                            "FC4", "FC6", "FT8", "C5", "C1", "C2", "C6", "TP7", "CP5", "CP3", "CP1", "CPz", "CP2", "CP4", "CP6", "TP8", "P5",
                                                            "P1", "P2", "P6", "PO7", "PO3", "POz", "PO4", "PO8", "Oz", "FT9", "FT10", "TP9", "TP10", "Fpz", "Iz", "Nz", "F9",
                                                            "F10", "P9", "P10"});

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<ElectrodePosition> StandardMontage::montage(System system)
{
    switch (system) {
        case System::Standard_1020:
            return pick(names1020);
        case System::Standard_1010:
            return pick(names1010);
        case System::Standard_1005: {
            // mne's standard_1005 montage: every electrode, the fiducials are not channels
            std::vector<ElectrodePosition> positions = standard1005Table();
            std::erase_if(positions, [](const ElectrodePosition& electrode) {
                return electrode.name == "LPA" || electrode.name == "RPA" || electrode.name == "Nz";
            });
            return positions;
        }
    }
    return {};
}

//=============================================================================================================

StringList StandardMontage::electrode_names(System system)
{
    StringList names;
    for (ElectrodePosition& electrode : montage(system))
        names.push_back(std::move(electrode.name));
    return names;
}

//=============================================================================================================

std::optional<Vector3d> StandardMontage::find_electrode(std::string_view name)
{
    for (const ElectrodePosition& electrode : standard1005Table()) {
        if (equalsIgnoreCase(electrode.name, name))
            return electrode.pos;
    }
    return std::nullopt;
}

//=============================================================================================================

std::size_t StandardMontage::electrode_count(System system)
{
    return montage(system).size();
}
