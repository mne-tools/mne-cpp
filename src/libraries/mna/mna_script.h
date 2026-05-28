//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_script.h
 * @since 2026
 * @date  April 2026
 * @brief Inline source code carried by a Script-mode graph node — language, interpreter command, code body, optional authoring URI and integrity hash.
 *
 * @ref MnaScript is the payload that turns a generic @ref MnaNode
 * into a one-off custom step without forcing every user-written
 * snippet through the @ref MnaOpRegistry. When the host node's
 * @c execMode is @c MnaNodeExecMode::Script the executor writes
 * @c code to a temporary file, launches @c interpreter (auto-
 * detected from @c language when unset) with @c interpreterArgs
 * prepended, captures stdout/stderr and the exit code, and marshals
 * results back into the node's typed output ports.
 *
 * Supported languages cover the day-to-day MEG/EEG glue work:
 * @c python, @c shell, @c r, @c matlab, @c octave and @c julia.
 * @c sourceUri lets the authoring GUI keep a link back to the
 * original file on disk; serialisation resolves that link into
 * @c code so a shipped project is self-contained. @c codeSha256
 * detects tampering between save and load, and @c keepTempFile is
 * a debug aid that preserves the on-disk script after execution.
 */

#ifndef MNA_SCRIPT_H
#define MNA_SCRIPT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QCborMap>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Inline script definition embedded in an MnaNode. When the node's execMode is
 * Script, the executor writes the code to a temporary file, invokes the
 * interpreter, and captures the results.
 *
 * @brief Inline interpreter-launched source code carried by a Script-mode @ref MnaNode.
 */
struct MNASHARED_EXPORT MnaScript
{
    QString     language;           ///< "python", "shell", "r", "matlab", "octave", "julia"
    QString     interpreter;        ///< Interpreter command (e.g. "python3", "/bin/bash", "Rscript")
                                    ///< Empty → auto-detect from language
    QStringList interpreterArgs;    ///< Extra args before the script file (e.g. ["-u"] for unbuffered Python)
    QString     code;               ///< The inline source code (resolved at save-time if sourceUri is set)
    QString     sourceUri;          ///< Optional authoring-time reference (e.g. "file:///lab/scripts/bandpass.py")
                                    ///< Tooling resolves this into `code` at serialization; consumers use `code`.
    QString     codeSha256;         ///< SHA-256 of `code` for integrity verification
    bool        keepTempFile = false; ///< true → preserve temp script file after execution (debug aid)

    QJsonObject toJson() const;
    static MnaScript fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaScript fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_SCRIPT_H
