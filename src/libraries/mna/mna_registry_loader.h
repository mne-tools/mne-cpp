//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_registry_loader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Declarative loader for @ref mne::MnaOpRegistry contents — ingests @c mna-registry.json manifests and overrides them with drop-ins under @c mna-registry.d/.
 *
 * @ref mne::MnaRegistryLoader is the bridge between the on-disk
 * representation of MNA op schemas (JSON manifests authored by
 * library and tool maintainers) and the in-memory
 * @ref mne::MnaOpRegistry consulted by the validator and executor. A
 * manifest carries an @c mna_registry_version field for future-
 * proofing, a @c provider name for diagnostics, and an @c ops
 * array whose entries are deserialised into @ref mne::MnaOpSchema
 * instances.
 *
 * @ref mne::MnaRegistryLoader::load_file "load_file" handles a single manifest, @ref mne::MnaRegistryLoader::load_directory "load_directory"
 * loads the master @c mna-registry.json first and then every
 * @c *.json drop-in from @c mna-registry.d/ in alphabetical order;
 * later files overwrite earlier definitions for the same op type,
 * which is the mechanism plug-ins use to extend or shadow built-in
 * operations. @ref mne::MnaRegistryLoader::save_file "save_file" re-emits the current registry to a
 * canonical manifest so a running process can publish the exact
 * op surface it offers.
 */

#ifndef MNA_REGISTRY_LOADER_H
#define MNA_REGISTRY_LOADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string_view>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class MnaOpRegistry;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Reads op-schema manifests ("mna_registry_version", "provider", "ops") into an @ref MnaOpRegistry.
 *
 * @brief Reads MNA op-schema manifests and feeds them into @ref MnaOpRegistry, with drop-in directory merge support.
 *
 * @snippet ex_mna/main.cpp mna_registry_loader_usage
 */
class MNASHARED_EXPORT MnaRegistryLoader
{
public:
    //=========================================================================================================
    /**
     * @param[in]     path       A manifest file.
     * @param[in,out] registry   Receives the schemas.
     *
     * @return The number of schemas loaded, or -1 if the file cannot be read or is not a manifest.
     */
    static int load_file(const std::filesystem::path& path, MnaOpRegistry& registry);

    //=========================================================================================================
    /**
     * Loads mna-registry.json and then every *.json in mna-registry.d/ in name order, so drop-ins override.
     *
     * @param[in]     dir        The registry directory.
     * @param[in,out] registry   Receives the schemas.
     *
     * @return The number of schemas loaded.
     */
    static int load_directory(const std::filesystem::path& dir, MnaOpRegistry& registry);

    //=========================================================================================================
    /**
     * @param[in] path       The manifest file to write.
     * @param[in] provider   Who provides the operations.
     * @param[in] registry   The schemas to write.
     *
     * @return Whether the file was written.
     */
    static bool save_file(const std::filesystem::path& path, std::string_view provider, const MnaOpRegistry& registry);
};

} // namespace mne

#endif // MNA_REGISTRY_LOADER_H
