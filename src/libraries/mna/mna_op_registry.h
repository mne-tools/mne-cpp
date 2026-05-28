//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_op_registry.h
 * @since 2026
 * @date  April 2026
 * @brief Process-wide singleton catalog mapping @c opType strings to their @ref MnaOpSchema and (for built-in ops) executable implementation.
 *
 * @ref MnaOpRegistry is the lookup table that the rest of the
 * library hits whenever an @c opType string needs to be resolved
 * into something runnable. It stores two parallel maps: one from
 * op type to @ref MnaOpSchema (used by @ref MnaGraph::validate and
 * by GUI editors to render attribute forms), and one from op type
 * to an @ref OpFunc lambda (used by @ref MnaGraphExecutor to run
 * the node). External / CLI / Script ops typically only register a
 * schema; in-process ops register both.
 *
 * @ref loadRegistryFiles searches upward from the application
 * directory for @c resources/mna/ and asks @ref MnaRegistryLoader
 * to ingest @c mna-registry.json plus every drop-in under
 * @c mna-registry.d/, so new operations can be added without
 * rebuilding the library. @ref missingOps reports the op types
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
 */
class MNASHARED_EXPORT MnaOpRegistry
{
public:
    /// Operation implementation callback type.
    using OpFunc = std::function<QVariantMap(const QVariantMap& inputs,
                                             const QVariantMap& attributes)>;

    /**
     * Access the singleton instance.
     */
    static MnaOpRegistry& instance();

    /**
     * Register an operation schema.
     */
    void registerOp(const MnaOpSchema& schema);

    /**
     * Check if an operation type is registered.
     */
    bool hasOp(const QString& opType) const;

    /**
     * Get the schema for an operation type.
     */
    MnaOpSchema schema(const QString& opType) const;

    /**
     * List all registered operation types.
     */
    QStringList registeredOps() const;

    /**
     * Register an implementation function for an operation type.
     */
    void registerOpFunc(const QString& opType, OpFunc func);

    /**
     * Get the implementation function for an operation type.
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
     * @param[in] project  The MNA project to check.
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
