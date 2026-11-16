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
 * @c op_type identifier, version, category and binding mode
 * (@c internal in-process function, @c cli external executable
 * driven by @c cli_template, or @c script delegating to an inline
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

#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

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

/** Value type of an operation attribute, as named in op registries. */
enum class MnaAttrType
{
    String, ///< "string" (also "path")
    Double, ///< "double"
    Float,  ///< "float"
    Int,    ///< "int"
    Bool    ///< "bool"
};

//=============================================================================================================
/**
 * @brief Expected port of an operation.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
struct MNASHARED_EXPORT MnaOpSchemaPort
{
    std::string name;                            ///< Port name
    MnaDataKind data_kind = MnaDataKind::Custom; ///< Expected data kind
    bool required = true;                        ///< Must be connected?
    std::string description;                     ///< Human-readable description
};

//=============================================================================================================
/**
 * @brief Expected attribute of an operation.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
struct MNASHARED_EXPORT MnaOpSchemaAttr
{
    std::string name;                              ///< Attribute key
    MnaAttrType type = MnaAttrType::String;        ///< Expected value type
    bool required = false;                         ///< Must be set?
    JsonValue default_value{JsonValue::Undefined}; ///< Default when not set; undefined if there is none
    std::string description;                       ///< Human-readable description
};

//=============================================================================================================
/**
 * Declarative contract of an operation: ports, attributes and how it is executed.
 *
 * @brief Operation schema for graph validation.
 *
 * @snippet ex_mna/main.cpp mna_op_schema_validate
 */
class MNASHARED_EXPORT MnaOpSchema
{
public:
    std::string op_type;      ///< Operation type string
    std::string version;      ///< Version of the operation (e.g. "2.2.0")
    std::string binding;      ///< Binding type: "internal", "cli", or "script"
    std::string category;     ///< Category: "io", "preprocessing", "source_estimation", etc.
    std::string description;  ///< What the operation does
    std::string library;      ///< Which library provides the implementation
    std::string executable;   ///< For CLI ops: executable name (e.g. "recon-all")
    std::string cli_template; ///< For CLI ops: command template with {{placeholder}} tokens

    std::vector<MnaOpSchemaPort> input_ports;  ///< Expected input ports
    std::vector<MnaOpSchemaPort> output_ports; ///< Expected output ports
    std::vector<MnaOpSchemaAttr> attributes;   ///< Expected attributes

    //=========================================================================================================
    /**
     * Checks a node against this schema: op type, required ports with their data kinds and required attributes.
     *
     * @param[in] node   The node.
     *
     * @return The problems found; empty if the node complies.
     */
    StringList validate(const MnaNode& node) const;
};

} // namespace mne

#endif // MNA_OP_SCHEMA_H
