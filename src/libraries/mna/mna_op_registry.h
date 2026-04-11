//=============================================================================================================
/**
 * @file     mna_op_registry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaOpRegistry class declaration — singleton catalog of operation schemas.
 *
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
 * @brief Operation registry for the MNA graph model.
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

private:
    MnaOpRegistry();
    void registerBuiltInOps();

    QMap<QString, MnaOpSchema> m_schemas;
    QMap<QString, OpFunc> m_funcs;
};

} // namespace MNALIB

#endif // MNA_OP_REGISTRY_H
