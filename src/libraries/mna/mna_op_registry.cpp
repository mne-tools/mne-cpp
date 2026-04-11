//=============================================================================================================
/**
 * @file     mna_op_registry.cpp
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
 * @brief    MnaOpRegistry class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_op_registry.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaOpRegistry::MnaOpRegistry()
{
    registerBuiltInOps();
}

//=============================================================================================================

MnaOpRegistry& MnaOpRegistry::instance()
{
    static MnaOpRegistry registry;
    return registry;
}

//=============================================================================================================

void MnaOpRegistry::registerOp(const MnaOpSchema& schema)
{
    m_schemas.insert(schema.opType, schema);
}

//=============================================================================================================

bool MnaOpRegistry::hasOp(const QString& opType) const
{
    return m_schemas.contains(opType);
}

//=============================================================================================================

MnaOpSchema MnaOpRegistry::schema(const QString& opType) const
{
    return m_schemas.value(opType);
}

//=============================================================================================================

QStringList MnaOpRegistry::registeredOps() const
{
    return m_schemas.keys();
}

//=============================================================================================================

void MnaOpRegistry::registerOpFunc(const QString& opType, OpFunc func)
{
    m_funcs.insert(opType, func);
}

//=============================================================================================================

MnaOpRegistry::OpFunc MnaOpRegistry::opFunc(const QString& opType) const
{
    return m_funcs.value(opType);
}

//=============================================================================================================

void MnaOpRegistry::registerBuiltInOps()
{
    // Register core MNE-CPP operation schemas

    // load_fiff_raw
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("load_fiff_raw");
        schema.description = QStringLiteral("Load raw MEG/EEG data from a FIFF file");
        schema.library    = QStringLiteral("mne_fiff");
        schema.outputPorts.append({QStringLiteral("raw_data"), MnaDataKind::FiffRaw, true,
                                   QStringLiteral("Loaded raw data")});
        schema.attributes.append({QStringLiteral("file_path"), QMetaType::QString, true,
                                  QVariant(), QStringLiteral("Path to the FIFF file")});
        registerOp(schema);
    }

    // compute_covariance
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("compute_covariance");
        schema.description = QStringLiteral("Compute noise covariance from raw data");
        schema.library    = QStringLiteral("mne_mne");
        schema.inputPorts.append({QStringLiteral("raw_data"), MnaDataKind::FiffRaw, true,
                                  QStringLiteral("Raw data input")});
        schema.outputPorts.append({QStringLiteral("covariance"), MnaDataKind::Covariance, true,
                                   QStringLiteral("Computed noise covariance")});
        registerOp(schema);
    }

    // make_forward
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("make_forward");
        schema.description = QStringLiteral("Compute forward solution");
        schema.library    = QStringLiteral("mne_fwd");
        schema.inputPorts.append({QStringLiteral("raw_data"), MnaDataKind::FiffRaw, true,
                                  QStringLiteral("Raw data for sensor info")});
        schema.inputPorts.append({QStringLiteral("bem"), MnaDataKind::Bem, false,
                                  QStringLiteral("BEM model")});
        schema.outputPorts.append({QStringLiteral("forward"), MnaDataKind::Forward, true,
                                   QStringLiteral("Forward solution")});
        registerOp(schema);
    }

    // apply_inverse
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("apply_inverse");
        schema.description = QStringLiteral("Apply inverse operator to produce source estimates");
        schema.library    = QStringLiteral("mne_inv");
        schema.inputPorts.append({QStringLiteral("data"), MnaDataKind::Evoked, true,
                                  QStringLiteral("Evoked or raw data")});
        schema.inputPorts.append({QStringLiteral("inverse_operator"), MnaDataKind::Inverse, true,
                                  QStringLiteral("Inverse operator")});
        schema.outputPorts.append({QStringLiteral("source_estimate"), MnaDataKind::SourceEstimate, true,
                                   QStringLiteral("Source estimate")});
        schema.attributes.append({QStringLiteral("method"), QMetaType::QString, false,
                                  QStringLiteral("dSPM"), QStringLiteral("Inverse method: MNE, dSPM, sLORETA")});
        schema.attributes.append({QStringLiteral("lambda2"), QMetaType::Double, false,
                                  1.0 / 9.0, QStringLiteral("Regularization parameter")});
        registerOp(schema);
    }

    // filter_bandpass
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("filter_bandpass");
        schema.description = QStringLiteral("Apply bandpass filter to raw data");
        schema.library    = QStringLiteral("mne_dsp");
        schema.inputPorts.append({QStringLiteral("raw_data"), MnaDataKind::FiffRaw, true,
                                  QStringLiteral("Raw data to filter")});
        schema.outputPorts.append({QStringLiteral("filtered"), MnaDataKind::FiffRaw, true,
                                   QStringLiteral("Filtered data")});
        schema.attributes.append({QStringLiteral("l_freq"), QMetaType::Double, false,
                                  QVariant(), QStringLiteral("Low cutoff frequency (Hz)")});
        schema.attributes.append({QStringLiteral("h_freq"), QMetaType::Double, false,
                                  QVariant(), QStringLiteral("High cutoff frequency (Hz)")});
        registerOp(schema);
    }

    // ipc_command (generic external process)
    {
        MnaOpSchema schema;
        schema.opType     = QStringLiteral("ipc_command");
        schema.description = QStringLiteral("Execute an external command via IPC");
        schema.library    = QStringLiteral("mne_mna");
        registerOp(schema);
    }
}
