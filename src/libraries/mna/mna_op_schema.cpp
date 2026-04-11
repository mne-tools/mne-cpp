//=============================================================================================================
/**
 * @file     mna_op_schema.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaOpSchema class implementation.
 *
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
