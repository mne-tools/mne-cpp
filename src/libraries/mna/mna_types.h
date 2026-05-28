//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_types.h
 * @since April 2026
 * @brief Enums and string-conversion helpers for the MNA container vocabulary (file roles, data kinds, port directions, exec modes).
 *
 * Every MNA file, port and node is tagged with a small set of
 * controlled vocabularies that downstream tooling — schema validators,
 * graph executors, GUI editors — uses to dispatch on without parsing
 * strings ad-hoc. Centralising them in one header guarantees that the
 * JSON/CBOR serialisers, the op-registry loader and the executor all
 * agree on the canonical spelling.
 *
 * @ref MnaFileRole classifies the semantic purpose of a file within a
 * project (Raw, Forward, Inverse, Covariance, …) so a viewer can pick
 * the right reader without sniffing magic bytes. @ref MnaDataKind
 * describes the runtime payload travelling through a graph port
 * (FiffRaw, Epochs, SourceEstimate, RealTimeStream, …) and is what
 * @ref MnaGraph::validate uses to reject incompatible connections.
 * @ref MnaPortDir and @ref MnaNodeExecMode (Batch/Stream/Ipc/Script)
 * complete the surface needed by the graph executor.
 *
 * The inline @c xxxToString / @c xxxFromString helpers are the single
 * source of truth for serialisation; both spellings (CamelCase from
 * the C++ side and snake_case from the JSON side) are accepted on
 * read to keep the format human-authorable.
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
    Event,
    VirtualChannel,
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
    case MnaFileRole::Evoked:         return QStringLiteral("evoked");
    case MnaFileRole::Event:          return QStringLiteral("event");
    case MnaFileRole::VirtualChannel: return QStringLiteral("virtual_channel");
    case MnaFileRole::Custom:         return QStringLiteral("custom");
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
    if(str == QLatin1String("event"))            return MnaFileRole::Event;
    if(str == QLatin1String("virtual_channel")) return MnaFileRole::VirtualChannel;
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
