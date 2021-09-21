//=============================================================================================================
/**
 * @file     files.cpp
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "files.h"
#include <fstream>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <cstdio>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool exists(const char* filePath)
{
#if __cplusplus >= 201703L
    return std::filesystem::exists(filePath);
#else
    std::ifstream infile(filePath);
    return infile.good();
#endif
}

//=============================================================================================================

void Files::copy(const char* sourcePath, const char* destPath)
{
#if __cplusplus >= 201703L
    std::filesystem::copy(sourcePath, destPath);
#else
    if (!exists(sourcePath) || exists(destPath)){
        return;
    }

    std::ifstream source(sourcePath, std::ios::binary);
    std::ofstream destination(destPath, std::ios::binary);

    destination << source.rdbuf();
#endif
}

//=============================================================================================================

void Files::rename(const char* sourcePath, const char* destPath)
{
#if __cplusplus >= 201703L
    std::filesystem::rename(sourcePath, destPath);
#else
    if (!exists(sourcePath) || exists(destPath)){
        return;
    }

    std::rename(sourcePath, destPath);
#endif
}

//=============================================================================================================

void Files::remove(const char* filePath)
{
#if __cplusplus >= 201703L
    std::filesystem::remove(filePath);
#else
    if (!exists(filePath)){
        return;
    }
    std::remove(filePath);
#endif
}

//=============================================================================================================

void Files::create(const char *filePath)
{
    std::ofstream {filePath};
}
