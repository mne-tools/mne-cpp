//=============================================================================================================
/**
 * @file     mna_op_schema.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
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
 *
 * @brief    MnaOpSchema class declaration — contract for graph operations.
 *
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

namespace MNALIB { struct MnaNode; }

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Port descriptor within an operation schema.
 */
struct MNASHARED_EXPORT MnaOpSchemaPort
{
    QString     name;           ///< Port name
    MnaDataKind dataKind;       ///< Expected data kind
    bool        required = true;///< Must be connected?
    QString     description;    ///< Human-readable description
};

//=============================================================================================================
/**
 * Attribute descriptor within an operation schema.
 */
struct MNASHARED_EXPORT MnaOpSchemaAttr
{
    QString          name;          ///< Attribute key
    QMetaType::Type  type;          ///< Expected value type
    bool             required = false; ///< Must be set?
    QVariant         defaultValue;  ///< Default when not set
    QString          description;   ///< Human-readable description
};

//=============================================================================================================
/**
 * Contract declaring an operation's expected inputs, outputs, and attributes.
 *
 * @brief Operation schema for graph validation.
 */
class MNASHARED_EXPORT MnaOpSchema
{
public:
    QString     opType;         ///< Operation type string
    QString     version;        ///< Version of the operation (e.g. "2.2.0")
    QString     binding;        ///< Binding type: "internal", "cli", or "script"
    QString     category;       ///< Category: "io", "preprocessing", "source_estimation", etc.
    QString     description;    ///< What the operation does
    QString     library;        ///< Which library provides the implementation
    QString     executable;     ///< For CLI ops: executable name (e.g. "recon-all")
    QString     cliTemplate;    ///< For CLI ops: command template with {{placeholder}} tokens

    QList<MnaOpSchemaPort> inputPorts;   ///< Expected input ports
    QList<MnaOpSchemaPort> outputPorts;  ///< Expected output ports
    QList<MnaOpSchemaAttr> attributes;   ///< Expected attributes

    /**
     * Validate that a node conforms to this schema.
     *
     * @param[in] node      Node to validate.
     * @param[out] errors   Optional list of validation error messages.
     * @return True if the node conforms to the schema.
     */
    bool validate(const MnaNode& node, QStringList* errors = nullptr) const;
};

} // namespace MNALIB

#endif // MNA_OP_SCHEMA_H
