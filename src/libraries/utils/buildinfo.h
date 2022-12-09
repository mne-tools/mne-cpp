//=============================================================================================================
/**
 * @file     buildinfo.h
 * @author   Juan G Prieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.9
 * @date     September, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Juan G Prieto, Gabriel B Motta. All rights reserved.
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
 * @brief    build information utils
 *
 */

#ifndef BUILD_INFO_LIB
#define BUILD_INFO_LIB

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstring>

namespace UTILSLIB{

//=============================================================================================================
/**
 * Returns build time (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto timeNowNow()
{
    return __TIME__;
}

//=============================================================================================================
/**
 * Returns build date (time preprocessor was run). Must be called in compiled function to return correctly.
 */
constexpr auto dateToday()
{
    return __DATE__;
}

//=============================================================================================================
/**
 * Returns build date and time (time preprocessor was run). Must be called in compiled function to return
 * correctly.
 */
constexpr auto dateTimeNow()
{
    return __DATE__ " " __TIME__;
}

//=============================================================================================================
/**
 * Returns short version of the hash of the current git commit
 */
constexpr auto gitHash()
{
#ifdef MNE_GIT_HASH_SHORT
    return MNE_GIT_HASH_SHORT;
#else
    return "Git hash not defined.";
#endif
}

//=============================================================================================================
/**
 * Returns entire hash of the current git commit
 */
constexpr auto gitHashLong()
{
#ifdef MNE_GIT_HASH_LONG
    return MNE_GIT_HASH_LONG;
#else
    return "Git hash long not defined.";
#endif
}

}

#endif
