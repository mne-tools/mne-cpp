//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_dataset_description.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BidsDatasetDescription — JSON I/O for the root ``dataset_description.json`` sidecar.
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

using namespace BIDSLIB;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

BidsDatasetDescription BidsDatasetDescription::read(const std::filesystem::path& path)
{
    BidsDatasetDescription desc;

    const BASELIB::Expected<std::string> text = BASELIB::read_text_file(path);
    if (!text) {
        BASELIB::Log::warning("[BidsDatasetDescription::read] Cannot open {}", BASELIB::utf8FromPath(path));
        return desc;
    }

    BASELIB::JsonParseError error;
    const BASELIB::JsonDocument doc = BASELIB::JsonDocument::from_json(*text, &error);
    if (error.error != BASELIB::JsonParseError::NoError) {
        BASELIB::Log::warning("[BidsDatasetDescription::read] Parse error in {} : {}", BASELIB::utf8FromPath(path), error.error_string());
        return desc;
    }

    const BASELIB::JsonObject& json = doc.object();
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
    BASELIB::JsonObject json{{"Name", desc.name}, {"BIDSVersion", desc.bidsVersion}};
    if (!desc.datasetType.empty())
        json.insert("DatasetType", desc.datasetType);
    if (!desc.license.empty())
        json.insert("License", desc.license);

    if (!BASELIB::write_text_file(path, BASELIB::JsonDocument(std::move(json)).to_json())) {
        BASELIB::Log::warning("[BidsDatasetDescription::write] Cannot open {} for writing", BASELIB::utf8FromPath(path));
        return false;
    }
    return true;
}
