//=============================================================================================================
/**
 * @file     mna_types.h
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
 * @brief    MNA type definitions.
 *
 */

#ifndef MNA_TYPES_H
#define MNA_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
// ENUMS
//=============================================================================================================

/**
 * Describes the role a file plays within an MNA project.
 */
enum class MnaFileRole {
    Raw,
    Forward,
    Inverse,
    Covariance,
    SourceEstimate,
    Bem,
    Surface,
    Annotation,
    Digitizer,
    Transform,
    SourceSpace,
    Evoked,
    Custom
};

//=============================================================================================================

/**
 * Supported container formats.
 */
enum class MnaContainerFormat {
    Json,
    Cbor
};

//=============================================================================================================

/**
 * Describes the kind of data flowing through a graph port.
 */
enum class MnaDataKind {
    FiffRaw,            ///< Raw MEG/EEG data (FIFF format)
    Forward,            ///< Forward solution
    Inverse,            ///< Inverse operator
    Covariance,         ///< Noise or data covariance matrix
    SourceEstimate,     ///< Source-level time series
    Epochs,             ///< Epoched data
    Evoked,             ///< Averaged evoked response
    Matrix,             ///< Generic Eigen matrix (for intermediate results)
    Volume,             ///< MRI volume data
    Surface,            ///< Surface mesh (FreeSurfer)
    Bem,                ///< BEM model
    Annotation,         ///< FreeSurfer annotation / parcellation
    Label,              ///< ROI label
    RealTimeStream,     ///< Live data channel (MNE Scan / LSL / FIFF-RT)
    Custom              ///< User-defined data kind
};

//=============================================================================================================

/**
 * Direction of a port on a graph node.
 */
enum class MnaPortDir {
    Input,
    Output
};

//=============================================================================================================

/**
 * Execution mode for a graph node.
 */
enum class MnaNodeExecMode {
    Batch,   ///< Runs once on static file-based inputs (default)
    Stream,  ///< Runs continuously on real-time data (MNE Scan mode)
    Ipc,     ///< Delegates to an external process via inter-process communication
    Script   ///< Inline code executed via interpreter (Python, shell, R, …)
};

//=============================================================================================================
// HELPER FUNCTIONS
//=============================================================================================================

/**
 * Convert MnaFileRole to its string representation.
 */
inline QString mnaFileRoleToString(MnaFileRole role)
{
    switch(role) {
    case MnaFileRole::Raw:            return QStringLiteral("raw");
    case MnaFileRole::Forward:        return QStringLiteral("forward");
    case MnaFileRole::Inverse:        return QStringLiteral("inverse");
    case MnaFileRole::Covariance:     return QStringLiteral("covariance");
    case MnaFileRole::SourceEstimate: return QStringLiteral("source_estimate");
    case MnaFileRole::Bem:            return QStringLiteral("bem");
    case MnaFileRole::Surface:        return QStringLiteral("surface");
    case MnaFileRole::Annotation:     return QStringLiteral("annotation");    case MnaFileRole::Digitizer:      return QStringLiteral("digitizer");
    case MnaFileRole::Transform:      return QStringLiteral("transform");
    case MnaFileRole::SourceSpace:    return QStringLiteral("source_space");
    case MnaFileRole::Evoked:         return QStringLiteral("evoked");    case MnaFileRole::Custom:         return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

//=============================================================================================================

/**
 * Convert a string to MnaFileRole.
 */
inline MnaFileRole mnaFileRoleFromString(const QString& str)
{
    if(str == QLatin1String("raw"))              return MnaFileRole::Raw;
    if(str == QLatin1String("forward"))          return MnaFileRole::Forward;
    if(str == QLatin1String("inverse"))          return MnaFileRole::Inverse;
    if(str == QLatin1String("covariance"))       return MnaFileRole::Covariance;
    if(str == QLatin1String("source_estimate"))  return MnaFileRole::SourceEstimate;
    if(str == QLatin1String("bem"))              return MnaFileRole::Bem;
    if(str == QLatin1String("surface"))          return MnaFileRole::Surface;
    if(str == QLatin1String("annotation"))       return MnaFileRole::Annotation;
    if(str == QLatin1String("digitizer"))        return MnaFileRole::Digitizer;
    if(str == QLatin1String("transform"))        return MnaFileRole::Transform;
    if(str == QLatin1String("source_space"))     return MnaFileRole::SourceSpace;
    if(str == QLatin1String("evoked"))           return MnaFileRole::Evoked;
    return MnaFileRole::Custom;
}

//=============================================================================================================

/**
 * Convert MnaDataKind to its string representation.
 */
inline QString mnaDataKindToString(MnaDataKind kind)
{
    switch (kind) {
    case MnaDataKind::FiffRaw:          return QStringLiteral("FiffRaw");
    case MnaDataKind::Forward:          return QStringLiteral("Forward");
    case MnaDataKind::Inverse:          return QStringLiteral("Inverse");
    case MnaDataKind::Covariance:       return QStringLiteral("Covariance");
    case MnaDataKind::SourceEstimate:   return QStringLiteral("SourceEstimate");
    case MnaDataKind::Epochs:           return QStringLiteral("Epochs");
    case MnaDataKind::Evoked:           return QStringLiteral("Evoked");
    case MnaDataKind::Matrix:           return QStringLiteral("Matrix");
    case MnaDataKind::Volume:           return QStringLiteral("Volume");
    case MnaDataKind::Surface:          return QStringLiteral("Surface");
    case MnaDataKind::Bem:              return QStringLiteral("Bem");
    case MnaDataKind::Annotation:       return QStringLiteral("Annotation");
    case MnaDataKind::Label:            return QStringLiteral("Label");
    case MnaDataKind::RealTimeStream:   return QStringLiteral("RealTimeStream");
    case MnaDataKind::Custom:           return QStringLiteral("Custom");
    }
    return QStringLiteral("Custom");
}

//=============================================================================================================

/**
 * Convert a string to MnaDataKind.
 */
inline MnaDataKind mnaDataKindFromString(const QString& str)
{
    if (str == QLatin1String("FiffRaw")         || str == QLatin1String("fiff_raw"))           return MnaDataKind::FiffRaw;
    if (str == QLatin1String("FiffInfo"))         return MnaDataKind::Custom; // FiffInfo is a sub-kind of FiffRaw context
    if (str == QLatin1String("Forward")         || str == QLatin1String("forward"))            return MnaDataKind::Forward;
    if (str == QLatin1String("Inverse")         || str == QLatin1String("inverse"))            return MnaDataKind::Inverse;
    if (str == QLatin1String("Covariance")      || str == QLatin1String("covariance"))         return MnaDataKind::Covariance;
    if (str == QLatin1String("SourceEstimate")  || str == QLatin1String("source_estimate"))    return MnaDataKind::SourceEstimate;
    if (str == QLatin1String("Epochs")          || str == QLatin1String("epochs"))             return MnaDataKind::Epochs;
    if (str == QLatin1String("Evoked")          || str == QLatin1String("evoked"))             return MnaDataKind::Evoked;
    if (str == QLatin1String("Matrix")          || str == QLatin1String("matrix"))             return MnaDataKind::Matrix;
    if (str == QLatin1String("Volume")          || str == QLatin1String("volume"))             return MnaDataKind::Volume;
    if (str == QLatin1String("Surface")         || str == QLatin1String("surface"))            return MnaDataKind::Surface;
    if (str == QLatin1String("Bem")             || str == QLatin1String("bem"))                return MnaDataKind::Bem;
    if (str == QLatin1String("Annotation")      || str == QLatin1String("annotation"))         return MnaDataKind::Annotation;
    if (str == QLatin1String("Label")           || str == QLatin1String("label"))              return MnaDataKind::Label;
    if (str == QLatin1String("RealTimeStream")  || str == QLatin1String("real_time_stream"))   return MnaDataKind::RealTimeStream;
    if (str == QLatin1String("FilePath")        || str == QLatin1String("DirectoryPath"))      return MnaDataKind::Custom;
    return MnaDataKind::Custom;
}

} // namespace MNALIB

#endif // MNA_TYPES_H
