//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_coordinate_system.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::BidsCoordinateSystem — JSON I/O and @c FiffCoordTrans bridge for ``_coordsystem.json``.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_coordinate_system.h"
#include <base/log.h>

#include <base/byte_stream.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

BidsCoordinateSystem BidsCoordinateSystem::read_json(const std::filesystem::path& path)
{
    BidsCoordinateSystem cs;
    cs.transform = Eigen::Matrix4d::Identity();

    const mne::Expected<std::string> text = mne::read_text_file(path);
    if (!text) {
        mne::Log::warning("[BidsCoordinateSystem::read_json] Cannot open {}", mne::utf8FromPath(path));
        return cs;
    }

    mne::JsonParseError error;
    const mne::JsonDocument doc = mne::JsonDocument::from_json(*text, &error);
    if (error.error != mne::JsonParseError::NoError) {
        mne::Log::warning("[BidsCoordinateSystem::read_json] Parse error in {} : {}", mne::utf8FromPath(path), error.error_string());
        return cs;
    }

    const mne::JsonObject& json = doc.object();

    // iEEG fields first, then EEG, then MEG
    const auto field = [&json](std::string_view name) {
        for (const char* prefix : {"iEEG", "EEG", "MEG"}) {
            const std::string_view value = json.value(std::string(prefix).append(name)).to_string();
            if (!value.empty())
                return std::string(value);
        }
        return std::string();
    };
    cs.system = field("CoordinateSystem");
    cs.units = field("CoordinateUnits");
    cs.description = field("CoordinateSystemDescription");

    cs.processingDescription = json.value("iEEGCoordinateProcessingDescription").to_string();
    if (cs.processingDescription.empty())
        cs.processingDescription = json.value("EEGCoordinateProcessingDescription").to_string();

    cs.associatedImagePath = json.value("IntendedFor").to_string();

    // Parse 4x4 transform if provided as "iEEGCoordinateProcessingTransform" or "Transform"
    const std::string_view transformKey = json.contains("iEEGCoordinateProcessingTransform") ? "iEEGCoordinateProcessingTransform"
        : json.contains("Transform")                                                         ? "Transform"
                                                                                             : "";
    if (!transformKey.empty()) {
        const mne::JsonArray& rows = json.value(transformKey).to_array();
        if (rows.size() == 4) {
            for (int r = 0; r < 4; ++r) {
                const mne::JsonArray& cols = rows[static_cast<std::size_t>(r)].to_array();
                if (cols.size() == 4) {
                    for (int c = 0; c < 4; ++c)
                        cs.transform(r, c) = cols[static_cast<std::size_t>(c)].to_double();
                }
            }
        }
    }

    return cs;
}

//=============================================================================================================

bool BidsCoordinateSystem::write_json(const std::filesystem::path& path,
                                      const BidsCoordinateSystem& cs,
                                      std::string_view sModality)
{
    const std::string modality(sModality);
    mne::JsonObject json{{modality + "CoordinateSystem", cs.system},
                         {modality + "CoordinateUnits", cs.units}};

    if (!cs.description.empty())
        json.insert(modality + "CoordinateSystemDescription", cs.description);
    if (!cs.processingDescription.empty() && sModality == "iEEG")
        json.insert("iEEGCoordinateProcessingDescription", cs.processingDescription);
    if (!cs.associatedImagePath.empty())
        json.insert("IntendedFor", cs.associatedImagePath);

    // Serialize the 4x4 transform matrix
    if (!cs.transform.isIdentity(1e-15) && sModality == "iEEG") {
        mne::JsonArray rows;
        rows.reserve(4);
        for (int r = 0; r < 4; ++r) {
            mne::JsonArray cols;
            cols.reserve(4);
            for (int c = 0; c < 4; ++c)
                cols.push_back(cs.transform(r, c));
            rows.push_back(std::move(cols));
        }
        json.insert("iEEGCoordinateProcessingTransform", std::move(rows));
    }

    if (!mne::write_text_file(path, mne::JsonDocument(std::move(json)).to_json())) {
        mne::Log::warning("[BidsCoordinateSystem::write_json] Cannot open {} for writing", mne::utf8FromPath(path));
        return false;
    }
    return true;
}

//=============================================================================================================

mne::FiffCoordTrans BidsCoordinateSystem::to_fiff_coord_trans(int fromFrame, int toFrame) const
{
    Eigen::Matrix4f matTrans = transform.cast<float>();
    return mne::FiffCoordTrans(fromFrame, toFrame, matTrans);
}
