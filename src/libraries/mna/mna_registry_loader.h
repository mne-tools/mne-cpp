//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_registry_loader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Declarative loader for @ref MnaOpRegistry contents — ingests @c mna-registry.json manifests and overrides them with drop-ins under @c mna-registry.d/.
 *
 * @ref MnaRegistryLoader is the bridge between the on-disk
 * representation of MNA op schemas (JSON manifests authored by
 * library and tool maintainers) and the in-memory
 * @ref MnaOpRegistry consulted by the validator and executor. A
 * manifest carries an @c mna_registry_version field for future-
 * proofing, a @c provider name for diagnostics, and an @c ops
 * array whose entries are deserialised into @ref MnaOpSchema
 * instances.
 *
 * @ref loadFile handles a single manifest, @ref loadDirectory
 * loads the master @c mna-registry.json first and then every
 * @c *.json drop-in from @c mna-registry.d/ in alphabetical order;
 * later files overwrite earlier definitions for the same op type,
 * which is the mechanism plug-ins use to extend or shadow built-in
 * operations. @ref saveFile re-emits the current registry to a
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
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNALIB { class MnaOpRegistry; }

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Loads MNA operation schemas from declarative JSON registry manifests files.
 *
 * Registry files follow the mna-registry.json format with "mna_registry_version",
 * "provider", and "ops" array.  The loader supports a master manifest plus
 * drop-in files from a mna-registry.d/ directory.
 *
 * @brief Reads MNA op-schema manifests and feeds them into @ref MnaOpRegistry, with drop-in directory merge support.
 */
class MNASHARED_EXPORT MnaRegistryLoader
{
public:
    /**
     * Load a single registry manifest file and register all ops.
     *
     * @param[in] path          Path to the JSON manifest file.
     * @param[in,out] registry  Registry to populate.
     * @return Number of ops successfully loaded, or -1 on file error.
     */
    static int loadFile(const QString& path, MnaOpRegistry& registry);

    /**
     * Load the master manifest plus all drop-in files from a directory.
     *
     * Loads mna-registry.json first, then all *.json files from
     * mna-registry.d/ in alphabetical order.  Later files override
     * earlier entries for the same op type.
     *
     * @param[in] registryDir   Directory containing mna-registry.json.
     * @param[in,out] registry  Registry to populate.
     * @return Total number of ops loaded across all files.
     */
    static int loadDirectory(const QString& registryDir, MnaOpRegistry& registry);

    /**
     * Serialize current registry schemas to a JSON manifest file.
     *
     * @param[in] path      Output file path.
     * @param[in] provider  Provider name for the manifest header.
     * @param[in] registry  Registry to serialize.
     * @return True if writing succeeded.
     */
    static bool saveFile(const QString& path,
                         const QString& provider,
                         const MnaOpRegistry& registry);
};

} // namespace MNALIB

#endif // MNA_REGISTRY_LOADER_H
