//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Enums and string-conversion helpers for the MNA container vocabulary (file roles, data kinds, port directions, exec modes).
 *
 * Every MNA file, port and node is tagged with a small set of
 * controlled vocabularies that downstream tooling — schema validators,
 * graph executors, GUI editors — uses to dispatch on without parsing
 * strings ad-hoc. Centralising them in one header guarantees that the
 * JSON/CBOR serialisers, the op-registry loader and the executor all
 * agree on the canonical spelling.
 *
 * @ref mne::MnaFileRole "MnaFileRole" classifies the semantic purpose of a file within a
 * project (Raw, Forward, Inverse, Covariance, …) so a viewer can pick
 * the right reader without sniffing magic bytes. @ref mne::MnaDataKind "MnaDataKind"
 * describes the runtime payload travelling through a graph port
 * (FiffRaw, Epochs, SourceEstimate, RealTimeStream, …) and is what
 * @ref mne::MnaGraph::validate "MnaGraph::validate" uses to reject incompatible connections.
 * @ref mne::MnaPortDir "MnaPortDir" and @ref mne::MnaNodeExecMode "MnaNodeExecMode" (Batch/Stream/Ipc/Script)
 * complete the surface needed by the graph executor.
 *
 * The to_string() / xxx_from_string() helpers are the single source of
 * truth for serialisation; both spellings (CamelCase from the C++ side and
 * snake_case from the JSON side) are accepted on read to keep the format
 * human-authorable.
 *
 * @ref mne::MnaValues "MnaValues" carries the run-time data between graph nodes.
 */

#ifndef MNA_TYPES_H
#define MNA_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <any>
#include <functional>
#include <map>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// ENUMS
//=============================================================================================================

/**
 * Describes the role a file plays within an MNA project.
 */
enum class MnaFileRole
{
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
enum class MnaContainerFormat
{
    Json,
    Cbor
};

//=============================================================================================================

/**
 * Describes the kind of data flowing through a graph port.
 */
enum class MnaDataKind
{
    FiffRaw,        ///< Raw MEG/EEG data (FIFF format)
    Forward,        ///< Forward solution
    Inverse,        ///< Inverse operator
    Covariance,     ///< Noise or data covariance matrix
    SourceEstimate, ///< Source-level time series
    Epochs,         ///< Epoched data
    Evoked,         ///< Averaged evoked response
    Matrix,         ///< Generic Eigen matrix (for intermediate results)
    Volume,         ///< MRI volume data
    Surface,        ///< Surface mesh (FreeSurfer)
    Bem,            ///< BEM model
    Annotation,     ///< FreeSurfer annotation / parcellation
    Label,          ///< ROI label
    RealTimeStream, ///< Live data channel (MNE Scan / LSL / FIFF-RT)
    Custom          ///< User-defined data kind
};

//=============================================================================================================

/**
 * Direction of a port on a graph node.
 */
enum class MnaPortDir
{
    Input,
    Output
};

//=============================================================================================================

/**
 * Execution mode for a graph node.
 */
enum class MnaNodeExecMode
{
    Batch,  ///< Runs once on static file-based inputs (default)
    Stream, ///< Runs continuously on real-time data (MNE Scan mode)
    Ipc,    ///< Delegates to an external process via inter-process communication
    Script  ///< Inline code executed via interpreter (Python, shell, R, …)
};

//=============================================================================================================

/**
 * Run-time values flowing between graph nodes, keyed by port name (or "nodeId::portName" in an execution context).
 * Values are any C++ object, e.g. a std::shared_ptr<FiffRawData>, a double or a std::string; read them with
 * std::any_cast.
 */
using MnaValues = std::map<std::string, std::any, std::less<>>;

//=============================================================================================================
// HELPER FUNCTIONS
//=============================================================================================================

/**
 * @param[in] role   A file role.
 *
 * @return Its name in MNA files, e.g. "source_estimate".
 */
MNASHARED_EXPORT std::string_view to_string(MnaFileRole role);

/**
 * @param[in] text   A file role name.
 *
 * @return The file role, or MnaFileRole::Custom for an unknown name.
 */
MNASHARED_EXPORT MnaFileRole file_role_from_string(std::string_view text);

/**
 * @param[in] kind   A data kind.
 *
 * @return Its name in op registries, e.g. "SourceEstimate".
 */
MNASHARED_EXPORT std::string_view to_string(MnaDataKind kind);

/**
 * @param[in] kind   A data kind.
 *
 * @return Its name in MNA files, e.g. "source_estimate".
 */
MNASHARED_EXPORT std::string_view data_kind_key(MnaDataKind kind);

/**
 * @param[in] text   A data kind name in either spelling, e.g. "SourceEstimate" or "source_estimate".
 *
 * @return The data kind, or MnaDataKind::Custom for an unknown name.
 */
MNASHARED_EXPORT MnaDataKind data_kind_from_string(std::string_view text);

} // namespace mne

#endif // MNA_TYPES_H
