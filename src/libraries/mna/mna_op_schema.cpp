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
 * * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * * @brief    MnaOpSchema class implementation.
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
