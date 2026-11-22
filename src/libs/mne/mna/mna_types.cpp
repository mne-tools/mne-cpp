//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_types.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Names of the MNA file roles and data kinds.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_types.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::array<std::pair<MnaFileRole, std::string_view>, 15> kFileRoles{{
    {MnaFileRole::Raw, "raw"},
    {MnaFileRole::Forward, "forward"},
    {MnaFileRole::Inverse, "inverse"},
    {MnaFileRole::Covariance, "covariance"},
    {MnaFileRole::SourceEstimate, "source_estimate"},
    {MnaFileRole::Bem, "bem"},
    {MnaFileRole::Surface, "surface"},
    {MnaFileRole::Annotation, "annotation"},
    {MnaFileRole::Digitizer, "digitizer"},
    {MnaFileRole::Transform, "transform"},
    {MnaFileRole::SourceSpace, "source_space"},
    {MnaFileRole::Evoked, "evoked"},
    {MnaFileRole::Event, "event"},
    {MnaFileRole::VirtualChannel, "virtual_channel"},
    {MnaFileRole::Custom, "custom"},
}};

/** Data kind, registry name, file name. */
struct DataKindName
{
    MnaDataKind kind;
    std::string_view name;
    std::string_view key;
};

constexpr std::array<DataKindName, 15> kDataKinds{{
    {MnaDataKind::FiffRaw, "FiffRaw", "fiff_raw"},
    {MnaDataKind::Forward, "Forward", "forward"},
    {MnaDataKind::Inverse, "Inverse", "inverse"},
    {MnaDataKind::Covariance, "Covariance", "covariance"},
    {MnaDataKind::SourceEstimate, "SourceEstimate", "source_estimate"},
    {MnaDataKind::Epochs, "Epochs", "epochs"},
    {MnaDataKind::Evoked, "Evoked", "evoked"},
    {MnaDataKind::Matrix, "Matrix", "matrix"},
    {MnaDataKind::Volume, "Volume", "volume"},
    {MnaDataKind::Surface, "Surface", "surface"},
    {MnaDataKind::Bem, "Bem", "bem"},
    {MnaDataKind::Annotation, "Annotation", "annotation"},
    {MnaDataKind::Label, "Label", "label"},
    {MnaDataKind::RealTimeStream, "RealTimeStream", "real_time_stream"},
    {MnaDataKind::Custom, "Custom", "custom"},
}};

const DataKindName& entry(MnaDataKind kind)
{
    const auto it = std::ranges::find(kDataKinds, kind, &DataKindName::kind);
    return it != kDataKinds.end() ? *it : kDataKinds.back();
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::string_view mne::to_string(MnaFileRole role)
{
    const auto it = std::ranges::find(kFileRoles, role, &std::pair<MnaFileRole, std::string_view>::first);
    return it != kFileRoles.end() ? it->second : "custom";
}

//=============================================================================================================

MnaFileRole mne::file_role_from_string(std::string_view text)
{
    const auto it = std::ranges::find(kFileRoles, text, &std::pair<MnaFileRole, std::string_view>::second);
    return it != kFileRoles.end() ? it->first : MnaFileRole::Custom;
}

//=============================================================================================================

std::string_view mne::to_string(MnaDataKind kind)
{
    return entry(kind).name;
}

//=============================================================================================================

std::string_view mne::data_kind_key(MnaDataKind kind)
{
    return entry(kind).key;
}

//=============================================================================================================

MnaDataKind mne::data_kind_from_string(std::string_view text)
{
    const auto it = std::ranges::find_if(kDataKinds, [&](const DataKindName& e) {
        return e.name == text || e.key == text;
    });
    return it != kDataKinds.end() ? it->kind : MnaDataKind::Custom;
}
