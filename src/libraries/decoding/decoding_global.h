//=============================================================================================================
/**
 * @file     decoding_global.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    decoding library export/import macros.
 *
 */

#ifndef DECODING_GLOBAL_H
#define DECODING_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/buildinfo.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define DECODINGSHARED_EXPORT
#elif defined(MNE_DECODING_LIBRARY)
#  define DECODINGSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define DECODINGSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace DECODINGLIB
 * @brief     M/EEG signal decoding (CSP, SPoC, SSD, LinearModel).
 *
 * Mirrors the public API of mne.decoding from MNE-Python. Classes in this
 * library delegate the core numerics to Skigen while adding MNE-specific
 * features such as transform modes, feature standardisation, and inverse
 * transforms.
 */
namespace DECODINGLIB{

//=============================================================================================================
/**
 * Returns build date and time.
 */
DECODINGSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * Returns abbreviated build git hash.
 */
DECODINGSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * Returns full build git hash.
 */
DECODINGSHARED_EXPORT const char* buildHashLong();
} // namespace DECODINGLIB

#endif // DECODING_GLOBAL_H
