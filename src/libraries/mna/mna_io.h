//=============================================================================================================
/**
 * @file     mna_io.h
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
 * @brief    MnaIO class declaration.
 *
 */

#ifndef MNA_IO_H
#define MNA_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_project.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
/**
 * Read/write MNA project files (.mna = JSON, .mnx = CBOR with "MNX1" magic header).
 */
class MNASHARED_EXPORT MnaIO
{
public:
    //=========================================================================================================
    /**
     * Read an MNA project from file. Dispatches by extension.
     *
     * @param[in] path   Path to the .mna or .mnx file.
     *
     * @return The deserialized MnaProject.
     */
    static MnaProject read(const QString& path);

    //=========================================================================================================
    /**
     * Write an MNA project to file. Dispatches by extension.
     *
     * @param[in] project   The project to write.
     * @param[in] path      Path to the .mna or .mnx file.
     *
     * @return True if successful.
     */
    static bool write(const MnaProject& project, const QString& path);

private:
    static MnaProject readJson(const QString& path);
    static bool       writeJson(const MnaProject& project, const QString& path);
    static MnaProject readCbor(const QString& path);
    static bool       writeCbor(const MnaProject& project, const QString& path);
};

} // namespace MNALIB

#endif // MNA_IO_H
