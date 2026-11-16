//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_script.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Inline source code carried by a Script-mode graph node — language, interpreter command, code body, optional authoring URI and integrity hash.
 *
 * @ref mne::MnaScript is the payload that turns a generic @ref mne::MnaNode
 * into a one-off custom step without forcing every user-written
 * snippet through the @ref mne::MnaOpRegistry. When the host node's
 * @c exec_mode is @c MnaNodeExecMode::Script the executor writes
 * @c code to a temporary file, launches @c interpreter (auto-
 * detected from @c language when unset) with @c interpreter_args
 * prepended, captures stdout/stderr and the exit code, and marshals
 * results back into the node's typed output ports.
 *
 * Supported languages cover the day-to-day MEG/EEG glue work:
 * @c python, @c shell, @c r, @c matlab, @c octave and @c julia.
 * @c source_uri lets the authoring GUI keep a link back to the
 * original file on disk; serialisation resolves that link into
 * @c code so a shipped project is self-contained. @c code_sha256
 * detects tampering between save and load, and @c keep_temp_file is
 * a debug aid that preserves the on-disk script after execution.
 */

#ifndef MNA_SCRIPT_H
#define MNA_SCRIPT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

#include <base/cbor.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Inline interpreter-launched source code carried by a Script-mode @ref MnaNode.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaScript
{
    std::string language;        ///< "python", "shell", "r", "matlab", "octave", "julia"
    std::string interpreter;     ///< Interpreter command (e.g. "python3", "/bin/bash", "Rscript")
    StringList interpreter_args; ///< Extra args before the script file (e.g. ["-u"] for unbuffered Python)
    std::string code;            ///< The inline source code (resolved at save-time if source_uri is set)
    std::string source_uri;      ///< Optional authoring-time reference (e.g. "file:///lab/scripts/bandpass.py")
    std::string code_sha256;     ///< SHA-256 of `code` for integrity verification
    bool keep_temp_file = false; ///< true → preserve temp script file after execution (debug aid)

    /** @return The script as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A script as written by to_json(). @return The script. */
    static MnaScript from_json(const JsonObject& json);

    /** @return The script as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A script as written by to_cbor(). @return The script. */
    static MnaScript from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_SCRIPT_H
