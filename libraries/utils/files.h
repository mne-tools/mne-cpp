//=============================================================================================================
/**
 * @file     files.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     September, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the Files class.
 *
 */

#ifndef FILES_UTILS_H
#define FILES_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#ifdef QT_CORE_LIB
#include <QString>
#endif

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief The Files class
 */
class UTILSSHARED_EXPORT Files
{
public:
    Files() = delete;

    //=========================================================================================================
    /**
     * @brief exists
     * @param filePath
     * @return
     */
    static bool exists(const char* filePath);

    //=========================================================================================================
    /**
     * Copies file based on input parameters.
     *
     * @param[in] sourcePath
     * @param[in] destPath
     *
     * @return
     */
    static bool copy(const char* sourcePath, const char* destPath);

    //=========================================================================================================
    /**
     * Attempts to rename file based on input parameters.
     *
     * @param[in] sourcePath
     * @param[in] destPath
     *
     * @return Whether file was renamed successfully.
     */
    static bool rename(const char* sourcePath, const char* destPath);

    //=========================================================================================================
    /**
     * Attempts to remove file at given input param.
     *
     * @param[in] filePath  File path to file to be deleted.
     *
     * @return Returns whether file was removed.
     */
    static bool remove(const char* filePath);

    //=========================================================================================================
    /**
     * Attempts to create a file at filePath.
     *
     * @param[in] filePath  File path to new file to be created.
     *
     * @return Returns whether file was created.
     */
    static bool create(const char* filePath);

#ifdef QT_CORE_LIB // QString oveloads
    //=========================================================================================================
    /**
     * Returns whether file given by input parameter exists.
     *
     * @param [in] filePath     path to file to be checked
     *
     * @return Whether file exists
     */
    static bool exists(const QString& filePath);

    //=========================================================================================================
    /**
     * Copies file based on given input parameters.
     *
     * @param[in] sourcePath    file to be copied
     * @param[in] destPath
     *
     * @return Returns whether copy was performed.
     */
    static bool copy(const QString& sourcePath, const QString& destPath);

    //=========================================================================================================
    /**
     * Attempts to rename file based on input parameters.
     *
     * @param[in] sourcePath
     * @param[in] destPath
     *
     * @return Whether file was renamed successfully.
     */
    static bool rename(const QString& sourcePath, const QString& destPath);

    //=========================================================================================================
    /**
     * Attempts to remove file at given input param.
     *
     * @param[in] filePath  File path to file to be deleted.
     *
     * @return Returns whether file was removed.
     */
    static bool remove(const QString& filePath);

    //=========================================================================================================
    /**
     * Attempts to create a file at filePath.
     *
     * @param[in] filePath  File path to new file to be created.
     *
     * @return Returns whether file was created.
     */
    static bool create(const QString& filePath);

#endif
};

}//namepace
#endif // FILES_UTILS_H
