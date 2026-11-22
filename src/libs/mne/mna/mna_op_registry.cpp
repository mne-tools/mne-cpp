//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_op_registry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the @ref MnaOpRegistry singleton, including the upward search for @c resources/mna/ and the drop-in registry merge logic.
 *
 * The registry stores schemas and op functions in two
 * @c QMap members keyed by op type. @ref register_op and
 * @ref register_op_func both replace any existing entry so plug-ins
 * loaded later can shadow built-ins by design. @ref load_registry_files
 * walks upward from @c QCoreApplication::applicationDirPath looking
 * for a @c resources/mna/ directory, then delegates to
 * @ref MnaRegistryLoader::load_directory, which in turn reads the
 * master @c mna-registry.json plus every drop-in under
 * @c mna-registry.d/ in alphabetical order. @ref missing_ops walks
 * a supplied tool list and returns those entries with no
 * registered schema, which is the simplest way to detect a stale
 * or partial install before a graph is executed.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_op_registry.h"

#include "mna_registry_loader.h"

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaOpRegistry::MnaOpRegistry()
{
    load_registry_files();
}

//=============================================================================================================

MnaOpRegistry& MnaOpRegistry::instance()
{
    static MnaOpRegistry registry;
    return registry;
}

//=============================================================================================================

void MnaOpRegistry::register_op(const MnaOpSchema& schema)
{
    m_schemas.insert_or_assign(schema.op_type, schema);
}

//=============================================================================================================

bool MnaOpRegistry::has_op(std::string_view op_type) const
{
    return m_schemas.contains(op_type);
}

//=============================================================================================================

const MnaOpSchema& MnaOpRegistry::schema(std::string_view op_type) const
{
    static const MnaOpSchema empty;
    const auto it = m_schemas.find(op_type);
    return it != m_schemas.end() ? it->second : empty;
}

//=============================================================================================================

StringList MnaOpRegistry::registered_ops() const
{
    StringList ops;
    ops.reserve(m_schemas.size());
    for (const auto& [op, schema] : m_schemas) {
        ops.push_back(op);
    }
    return ops;
}

//=============================================================================================================

void MnaOpRegistry::register_op_func(std::string_view op_type, OpFunc func)
{
    m_funcs.insert_or_assign(std::string(op_type), std::move(func));
}

//=============================================================================================================

const MnaOpRegistry::OpFunc& MnaOpRegistry::op_func(std::string_view op_type) const
{
    static const OpFunc none;
    const auto it = m_funcs.find(op_type);
    return it != m_funcs.end() ? it->second : none;
}

//=============================================================================================================

int MnaOpRegistry::load_registry_files()
{
    std::error_code ec;
    std::filesystem::path dir = executable_dir();
    for (int i = 0; i < 6 && !dir.empty(); ++i) {
        if (const std::filesystem::path candidate = dir / "resources" / "mna"; std::filesystem::is_directory(candidate, ec)) {
            return MnaRegistryLoader::load_directory(candidate, *this);
        }
        dir = dir.parent_path();
    }
    if (std::filesystem::is_directory("resources/mna", ec)) {
        return MnaRegistryLoader::load_directory(std::filesystem::current_path(ec) / "resources" / "mna", *this);
    }
    Log::warning("MnaOpRegistry: Could not find resources/mna/ registry directory");
    return 0;
}

//=============================================================================================================

StringList MnaOpRegistry::missing_ops(const StringList& tools) const
{
    StringList missing;
    for (const std::string& tool : tools) {
        if (!m_schemas.contains(tool)) {
            missing.push_back(tool);
        }
    }
    return missing;
}
