//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_dataset_description.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Reader/writer for ``dataset_description.json`` — the REQUIRED root sidecar of every BIDS dataset.
 *
 * Every BIDS validator refuses to walk a tree that does not have a
 * @c dataset_description.json at the root, so this is also the first
 * file mne-cpp writes when exporting a dataset and the last sanity
 * check performed on import. @ref BIDSLIB::BidsDatasetDescription captures the
 * two REQUIRED fields (@c Name and @c BIDSVersion) plus the two most
 * commonly populated recommended fields (@c DatasetType,
 * @c License); additional recommended/optional fields can be added
 * without breaking the JSON-pass-through wire format.
 *
 * I/O goes through BASELIB::JsonDocument; missing required fields fall
 * back to safe defaults (BIDS version pinned to the spec version this
 * library was tested against) so a partially-populated dataset still
 * round-trips.
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/modality-agnostic-files.html#dataset-description
 */

#ifndef BIDS_DATASET_DESCRIPTION_H
#define BIDS_DATASET_DESCRIPTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Dataset-level metadata from dataset_description.json.
 *
 * Every BIDS dataset must contain a dataset_description.json at its root.
 * This struct captures the required and recommended fields.
 *
 * @snippet ex_bids/main.cpp bids_sidecar_records
 */
struct BIDSSHARED_EXPORT BidsDatasetDescription
{
    std::string name;        /**< Human-readable dataset name (REQUIRED). */
    std::string bidsVersion; /**< BIDS specification version, e.g. "1.9.0" (REQUIRED). */
    std::string datasetType; /**< "raw" or "derivative" (RECOMMENDED). */
    std::string license;     /**< License identifier (RECOMMENDED). */

    /**
     * @brief Read a dataset_description.json file.
     * @param[in] path       Path to dataset_description.json.
     * @return Populated description, or default if file cannot be read.
     */
    static BidsDatasetDescription read(const std::filesystem::path& path);

    /**
     * @brief Write a dataset_description.json file.
     * @param[in] path       Output path.
     * @param[in] desc       Dataset description to write.
     * @return true on success.
     */
    static bool write(const std::filesystem::path& path,
                      const BidsDatasetDescription& desc);
};

} // namespace BIDSLIB

#endif // BIDS_DATASET_DESCRIPTION_H
