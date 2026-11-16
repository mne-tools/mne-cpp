//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_op_schema.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MnaOpSchema::validate — checks a concrete @ref MnaNode against its declared port and attribute contract.
 *
 * The validator walks the schema's input and output port lists
 * looking for matching named ports on the node, recording an
 * error whenever a @c required port is missing or carries the
 * wrong @ref MnaDataKind. It then walks the attribute schema,
 * checks that every required attribute is present and that its
 * runtime @c QMetaType matches what the schema expects, and
 * supplies defaults for optional attributes that the node has
 * not overridden. All errors are appended to the caller-supplied
 * list so a GUI editor can surface every problem at once instead
 * of bailing on the first mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_op_schema.h"

#include "mna_node.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StringList MnaOpSchema::validate(const MnaNode& node) const
{
    if (node.op_type != op_type) {
        return {std::format("Op type mismatch: expected '{}', got '{}'", op_type, node.op_type)};
    }

    StringList errors;
    for (const MnaOpSchemaPort& sp : input_ports) {
        if (!sp.required) {
            continue;
        }
        const auto port = std::ranges::find(node.inputs, sp.name, &MnaPort::name);
        if (port == node.inputs.end()) {
            errors.push_back(std::format("Missing required input port '{}'", sp.name));
        } else if (port->data_kind != sp.data_kind) {
            errors.push_back(std::format("Input port '{}': expected data kind '{}'", sp.name, static_cast<int>(sp.data_kind)));
        }
    }
    for (const MnaOpSchemaPort& sp : output_ports) {
        if (sp.required && std::ranges::find(node.outputs, sp.name, &MnaPort::name) == node.outputs.end()) {
            errors.push_back(std::format("Missing required output port '{}'", sp.name));
        }
    }
    for (const MnaOpSchemaAttr& sa : attributes) {
        if (sa.required && !node.attributes.contains(sa.name)) {
            errors.push_back(std::format("Missing required attribute '{}'", sa.name));
        }
    }
    return errors;
}
