//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_dataset_description.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::BidsDatasetDescription — JSON I/O for the root ``dataset_description.json`` sidecar.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_dataset_description.h"
#include <base/log.h>

#include <base/byte_stream.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

BidsDatasetDescription BidsDatasetDescription::read(const std::filesystem::path& path)
{
    BidsDatasetDescription desc;

    const mne::Expected<std::string> text = mne::read_text_file(path);
    if (!text) {
        mne::Log::warning("[BidsDatasetDescription::read] Cannot open {}", mne::utf8_from_path(path));
        return desc;
    }

    mne::JsonParseError error;
    const mne::JsonDocument doc = mne::JsonDocument::from_json(*text, &error);
    if (error.error != mne::JsonParseError::NoError) {
        mne::Log::warning("[BidsDatasetDescription::read] Parse error in {} : {}", mne::utf8_from_path(path), error.error_string());
        return desc;
    }

    const mne::JsonObject& json = doc.object();
    desc.name = json.value("Name").to_string();
    desc.bidsVersion = json.value("BIDSVersion").to_string();
    desc.datasetType = json.value("DatasetType").to_string();
    desc.license = json.value("License").to_string();

    return desc;
}

//=============================================================================================================

bool BidsDatasetDescription::write(const std::filesystem::path& path,
                                   const BidsDatasetDescription& desc)
{
    mne::JsonObject json{{"Name", desc.name}, {"BIDSVersion", desc.bidsVersion}};
    if (!desc.datasetType.empty())
        json.insert("DatasetType", desc.datasetType);
    if (!desc.license.empty())
        json.insert("License", desc.license);

    if (!mne::write_text_file(path, mne::JsonDocument(std::move(json)).to_json())) {
        mne::Log::warning("[BidsDatasetDescription::write] Cannot open {} for writing", mne::utf8_from_path(path));
        return false;
    }
    return true;
}
