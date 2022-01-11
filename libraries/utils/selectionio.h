//=============================================================================================================
/**
 * @file     selectionio.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    SelectionIO class declaration.
 *
 */

#ifndef SELECTIONIO_H
#define SELECTIONIO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

#include <vector>
#include <string>
#include <map>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// DEFINES
//=============================================================================================================

//=============================================================================================================
/**
 * Processes selection files (mne .sel) files which contain the channels for each selection group.
 *
 * @brief Processes selection files (mne .sel) files which contain the chanels for each selection group.
 */
class UTILSSHARED_EXPORT SelectionIO
{
public:
    //=========================================================================================================
    /**
     * Constructs a Filter object.
     */
    SelectionIO();

    //=========================================================================================================
    /**
     * Reads the specified MNE sel file.
     * @param[in] path holds the file path of the .sel file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool readMNESelFile(QString path, QMap<QString,QStringList> &selectionMap);

    //=========================================================================================================
    /**
     * Reads the specified MNE sel file.
     * @param[in] path holds the file path of the .sel file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool readMNESelFile(const std::string& path, std::multimap<std::string,std::vector<std::string>>& selectionMap);

    //=========================================================================================================
    /**
     * Reads the specified Brainstorm montage file.
     * @param[in] path holds the file path of the .mon file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool readBrainstormMonFile(QString path, QMap<QString,QStringList> &selectionMap);

    //=========================================================================================================
    /**
     * Reads the specified Brainstorm montage file.
     * @param[in] path holds the file path of the .mon file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool readBrainstormMonFile(const std::string& path, std::multimap<std::string,std::vector<std::string>>& selectionMap);

    //=========================================================================================================
    /**
     * Writes the specified selection groups to a single MNE .sel file.
     * @param[in] path holds the file path of the .sel file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool writeMNESelFile(QString path, const QMap<QString,QStringList> &selectionMap);

    //=========================================================================================================
    /**
     * Writes the specified selection groups to a single MNE .sel file.
     * @param[in] path holds the file path of the .sel file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool writeMNESelFile(const std::string& path, const std::map<std::string,std::vector<std::string>>& selectionMap);

    //=========================================================================================================
    /**
     * Writes the specified selection groups to different Brainstorm .mon files. The amount of written files depend on the number of selection groups in selectionMap
     * @param[in] path holds the file path of the .mon file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool writeBrainstormMonFiles(QString path, const QMap<QString,QStringList> &selectionMap);

    //=========================================================================================================
    /**
     * Writes the specified selection groups to different Brainstorm .mon files. The amount of written files depend on the number of selection groups in selectionMap
     * @param[in] path holds the file path of the .mon file which is to be read.
     * @param[in] selectionMap holds the map to which the read selection groups are stored.
     */
    static bool writeBrainstormMonFiles(const std::string& path, const std::map<std::string,std::vector<std::string>>& selectionMap);
};
} // NAMESPACE

#endif // SELECTIONIO_H
