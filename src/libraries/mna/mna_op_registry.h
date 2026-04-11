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
