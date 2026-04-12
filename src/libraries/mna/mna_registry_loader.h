//=============================================================================================================
/**
 * @file     mna_registry_loader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MnaRegistryLoader class declaration — loads MNA op schemas from JSON manifests.
 *
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
 * Loads MNA operation schemas from declarative JSON registry manifest files.
 *
 * Registry files follow the mna-registry.json format with "mna_registry_version",
 * "provider", and "ops" array.  The loader supports a master manifest plus
 * drop-in files from a mna-registry.d/ directory.
 *
 * @brief Declarative MNA registry file loader.
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
