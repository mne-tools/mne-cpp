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
    QString     description;    ///< What the operation does
    QString     library;        ///< Which library provides the implementation

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
