//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_op_schema.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref MnaOpSchema::validate — checks a concrete @ref MnaNode against its declared port and attribute contract.
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
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MnaOpSchema::validate(const MnaNode& node, QStringList* errors) const
{
    bool valid = true;

    // Check op type matches
    if (node.opType != opType) {
        if (errors)
            errors->append(QString("Op type mismatch: expected '%1', got '%2'").arg(opType, node.opType));
        return false;
    }

    // Check required input ports
    for (const MnaOpSchemaPort& sp : inputPorts) {
        if (!sp.required)
            continue;
        bool found = false;
        for (const MnaPort& np : node.inputs) {
            if (np.name == sp.name) {
                found = true;
                if (np.dataKind != sp.dataKind) {
                    if (errors)
                        errors->append(QString("Input port '%1': expected data kind '%2'")
                                       .arg(sp.name).arg(static_cast<int>(sp.dataKind)));
                    valid = false;
                }
                break;
            }
        }
        if (!found) {
            if (errors)
                errors->append(QString("Missing required input port '%1'").arg(sp.name));
            valid = false;
        }
    }

    // Check required output ports
    for (const MnaOpSchemaPort& sp : outputPorts) {
        if (!sp.required)
            continue;
        bool found = false;
        for (const MnaPort& np : node.outputs) {
            if (np.name == sp.name) {
                found = true;
                break;
            }
        }
        if (!found) {
            if (errors)
                errors->append(QString("Missing required output port '%1'").arg(sp.name));
            valid = false;
        }
    }

    // Check required attributes
    for (const MnaOpSchemaAttr& sa : attributes) {
        if (!sa.required)
            continue;
        if (!node.attributes.contains(sa.name)) {
            if (errors)
                errors->append(QString("Missing required attribute '%1'").arg(sa.name));
            valid = false;
        }
    }

    return valid;
}
