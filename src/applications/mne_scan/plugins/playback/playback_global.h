//=============================================================================================================
/**
 * @file     playback_global.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the Playback library export/import macros.
 *
 */

#ifndef PLAYBACK_GLOBAL_H
#define PLAYBACK_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// PREPROCESSOR DEFINES
//=============================================================================================================

#if defined(SCAN_PLAYBACK_PLUGIN)
#  define PLAYBACKSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define PLAYBACKSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

namespace PLAYBACKPLUGIN{

//=============================================================================================================
/**
 * Returns build date and time.
 */
PLAYBACKSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
PLAYBACKSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
PLAYBACKSHARED_EXPORT const char* buildHashLong();
}

#endif // PLAYBACK_GLOBAL_H
