//=============================================================================================================
/**
 * @file     mna_script.h
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
 * @brief    MnaScript struct declaration — inline code embedded in a graph node.
 *
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
 * @brief Inline code for script-type graph nodes.
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
