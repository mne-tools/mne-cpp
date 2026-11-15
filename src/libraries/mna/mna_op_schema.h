//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_op_schema.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Declarative contract for an MNA operation — expected input/output ports, attributes, binding kind and validation against a concrete @ref mne::MnaNode.
 *
 * @ref mne::MnaOpSchema is the type information an MNA operation
 * publishes to the rest of the system. It declares the
 * @c opType identifier, version, category and binding mode
 * (@c internal in-process function, @c cli external executable
 * driven by @c cliTemplate, or @c script delegating to an inline
 * @ref mne::MnaScript), plus the @ref mne::MnaOpSchemaPort and
 * @ref mne::MnaOpSchemaAttr lists that describe expected I/O and
 * parameters with their data kinds, default values and required
 * flags.
 *
 * The @ref mne::MnaOpSchema::validate "MnaOpSchema::validate" method is the gatekeeper used by
 * @ref mne::MnaGraph::validate "MnaGraph::validate" and the GUI editor: it checks that a
 * concrete @ref mne::MnaNode declares every required port, that the
 * attribute types match @c QMetaType expectations, and that no
 * unknown attribute leaks through. Schemas are normally authored
 * declaratively in @c mna-registry.json and loaded by
 * @ref mne::MnaRegistryLoader rather than written in C++.
 */

#ifndef MNA_OP_SCHEMA_H
#define MNA_OP_SCHEMA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QMetaType>
#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
struct MnaNode;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Port descriptor within an operation schema.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
struct MNASHARED_EXPORT MnaOpSchemaPort
{
    QString name;         ///< Port name
    MnaDataKind dataKind; ///< Expected data kind
    bool required = true; ///< Must be connected?
    QString description;  ///< Human-readable description
};

//=============================================================================================================
/**
 * Attribute descriptor within an operation schema.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
struct MNASHARED_EXPORT MnaOpSchemaAttr
{
    QString name;          ///< Attribute key
    QMetaType::Type type;  ///< Expected value type
    bool required = false; ///< Must be set?
    QVariant defaultValue; ///< Default when not set
    QString description;   ///< Human-readable description
};

//=============================================================================================================
/**
 * Contract declaring an operation's expected inputs, outputs, and attributes.
 *
 * @brief Operation schema for graph validation.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
class MNASHARED_EXPORT MnaOpSchema
{
public:
    QString opType;      ///< Operation type string
    QString version;     ///< Version of the operation (e.g. "2.2.0")
    QString binding;     ///< Binding type: "internal", "cli", or "script"
    QString category;    ///< Category: "io", "preprocessing", "source_estimation", etc.
    QString description; ///< What the operation does
    QString library;     ///< Which library provides the implementation
    QString executable;  ///< For CLI ops: executable name (e.g. "recon-all")
    QString cliTemplate; ///< For CLI ops: command template with {{placeholder}} tokens

    QList<MnaOpSchemaPort> inputPorts;  ///< Expected input ports
    QList<MnaOpSchemaPort> outputPorts; ///< Expected output ports
    QList<MnaOpSchemaAttr> attributes;  ///< Expected attributes

    /**
     * Validate that a node conforms to this schema.
     *
     * @param[in] node      Node to validate.
     * @param[out] errors   Optional list of validation error messages.
     * @return True if the node conforms to the schema.
     */
    bool validate(const MnaNode& node, QStringList* errors = nullptr) const;
};

} // namespace mne

#endif // MNA_OP_SCHEMA_H
