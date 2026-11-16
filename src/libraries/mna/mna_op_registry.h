//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_op_registry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Process-wide singleton catalog mapping @c op_type strings to their @ref mne::MnaOpSchema and (for built-in ops) executable implementation.
 *
 * @ref mne::MnaOpRegistry is the lookup table that the rest of the
 * library hits whenever an @c op_type string needs to be resolved
 * into something runnable. It stores two parallel maps: one from
 * op type to @ref mne::MnaOpSchema (used by @ref mne::MnaGraph::validate "MnaGraph::validate" and
 * by GUI editors to render attribute forms), and one from op type
 * to an @ref mne::MnaOpRegistry::OpFunc "OpFunc" lambda (used by @ref mne::MnaGraphExecutor to run
 * the node). External / CLI / Script ops typically only register a
 * schema; in-process ops register both.
 *
 * @ref mne::MnaOpRegistry::load_registry_files "load_registry_files" searches upward from the application
 * directory for @c resources/mna/ and asks @ref mne::MnaRegistryLoader
 * to ingest @c mna-registry.json plus every drop-in under
 * @c mna-registry.d/, so new operations can be added without
 * rebuilding the library. @ref mne::MnaOpRegistry::missing_ops "missing_ops" reports the op types
 * referenced by a loaded project that the current process cannot
 * resolve — the simplest way to detect a stale or partial install.
 */

#ifndef MNA_OP_REGISTRY_H
#define MNA_OP_REGISTRY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_op_schema.h"
#include "mna_types.h"

#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Process-wide catalog of operation schemas and, for built-in operations, their implementations.
 *
 * @brief Process-wide lookup from @c op_type to @ref MnaOpSchema and implementation function.
 *
 * @snippet ex_mna/main.cpp mna_registry_loader_usage
 */
class MNASHARED_EXPORT MnaOpRegistry
{
public:
    /** Implementation of an operation: input values by port name and the node attributes in, output values out. */
    using OpFunc = std::function<MnaValues(const MnaValues& inputs, const JsonObject& attributes)>;

    /** @return The registry, loaded from resources/mna on first use. */
    static MnaOpRegistry& instance();

    /** @param[in] schema   A schema; replaces the schema of the same op type. */
    void register_op(const MnaOpSchema& schema);

    /** @param[in] op_type   An op type. @return Whether it has a schema. */
    bool has_op(std::string_view op_type) const;

    /** @param[in] op_type   An op type. @return Its schema, or an empty schema if there is none. */
    const MnaOpSchema& schema(std::string_view op_type) const;

    /** @return The op types with a schema in ascending order. */
    StringList registered_ops() const;

    /** @param[in] op_type   An op type. @param[in] func   Its implementation; replaces an earlier one. */
    void register_op_func(std::string_view op_type, OpFunc func);

    /** @param[in] op_type   An op type. @return Its implementation, or an empty function if there is none. */
    const OpFunc& op_func(std::string_view op_type) const;

    //=========================================================================================================
    /**
     * Loads the schemas of the resources/mna directory found next to the executable, up to five directories
     * above it, or in the working directory.
     *
     * @return The number of schemas loaded.
     */
    int load_registry_files();

    /** @param[in] tools   Op types a pipeline needs. @return Those without a schema. */
    StringList missing_ops(const StringList& tools) const;

private:
    MnaOpRegistry();

    std::map<std::string, MnaOpSchema, std::less<>> m_schemas;
    std::map<std::string, OpFunc, std::less<>> m_funcs;
};

} // namespace mne

#endif // MNA_OP_REGISTRY_H
