//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_op_registry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Process-wide singleton catalog mapping @c opType strings to their @ref MNALIB::MnaOpSchema and (for built-in ops) executable implementation.
 *
 * @ref MNALIB::MnaOpRegistry is the lookup table that the rest of the
 * library hits whenever an @c opType string needs to be resolved
 * into something runnable. It stores two parallel maps: one from
 * op type to @ref MNALIB::MnaOpSchema (used by @ref MNALIB::MnaGraph::validate "MnaGraph::validate" and
 * by GUI editors to render attribute forms), and one from op type
 * to an @ref MNALIB::MnaOpRegistry::OpFunc "OpFunc" lambda (used by @ref MNALIB::MnaGraphExecutor to run
 * the node). External / CLI / Script ops typically only register a
 * schema; in-process ops register both.
 *
 * @ref MNALIB::MnaOpRegistry::loadRegistryFiles "loadRegistryFiles" searches upward from the application
 * directory for @c resources/mna/ and asks @ref MNALIB::MnaRegistryLoader
 * to ingest @c mna-registry.json plus every drop-in under
 * @c mna-registry.d/, so new operations can be added without
 * rebuilding the library. @ref MNALIB::MnaOpRegistry::missingOps "missingOps" reports the op types
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariantMap>
#include <functional>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Singleton catalog of all registered operation schemas.
 *
 * @brief Process-wide lookup from @c opType to @ref MnaOpSchema and implementation function.
 *
 * @snippet ex_mna/main.cpp mna_registry_loader_usage
 */
class MNASHARED_EXPORT MnaOpRegistry
{
public:
    /// Operation implementation callback type.
    using OpFunc = std::function<QVariantMap(const QVariantMap& inputs,
                                             const QVariantMap& attributes)>;

    /**
     * Access the singleton instance.
     *
     * @return Reference to the process-wide registry.
     */
    static MnaOpRegistry& instance();

    /**
     * Register an operation schema.
     *
     * @param[in] schema   Schema to store; replaces any existing schema with the same opType.
     */
    void registerOp(const MnaOpSchema& schema);

    /**
     * Check if an operation type is registered.
     *
     * @param[in] opType   Operation type name to look up.
     *
     * @return true if a schema is registered for opType, false otherwise.
     */
    bool hasOp(const QString& opType) const;

    /**
     * Get the schema for an operation type.
     *
     * @param[in] opType   Operation type name to look up.
     *
     * @return The registered schema, or a default-constructed schema if opType is unknown.
     */
    MnaOpSchema schema(const QString& opType) const;

    /**
     * List all registered operation types.
     *
     * @return Names of all operation types that have a registered schema.
     */
    QStringList registeredOps() const;

    /**
     * Register an implementation function for an operation type.
     *
     * @param[in] opType   Operation type name the function implements.
     * @param[in] func     Callback mapping inputs and attributes to outputs; replaces any existing one.
     */
    void registerOpFunc(const QString& opType, OpFunc func);

    /**
     * Get the implementation function for an operation type.
     *
     * @param[in] opType   Operation type name to look up.
     *
     * @return The registered callback, or an empty OpFunc if none is registered.
     */
    OpFunc opFunc(const QString& opType) const;

    /**
     * Load registry files from the standard resources/mna/ directory.
     *
     * Searches upward from the application directory to find resources/mna/,
     * then loads mna-registry.json and all drop-in files from mna-registry.d/.
     *
     * @return Number of ops loaded, or 0 if no registry directory was found.
     */
    int loadRegistryFiles();

    /**
     * Return op types referenced in a project that have no registered schema.
     *
     * @param[in] pipelineTools  The MNA project to check.
     * @return List of op type strings with no registered schema.
     */
    QStringList missingOps(const QStringList& pipelineTools) const;

private:
    MnaOpRegistry();

    QMap<QString, MnaOpSchema> m_schemas;
    QMap<QString, OpFunc> m_funcs;
};

} // namespace MNALIB

#endif // MNA_OP_REGISTRY_H
