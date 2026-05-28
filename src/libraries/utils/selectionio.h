//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file selectionio.h
 * @since 2022
 * @date  January 2024
 * @brief Reader / writer for MNE @c .sel channel-selection files and Brainstorm @c .mon montage files.
 *
 * @ref UTILSLIB::SelectionIO converts between two text-based
 * grouping formats that mne-cpp's GUI tools and command-line
 * pipelines hand back and forth with sibling toolchains:
 *
 * - MNE @c .sel — one selection group per line, holding the
 *   channels visible in a given montage ("Left-temporal",
 *   "All MEG", ...); shared with mne-python and mne-c.
 *
 * - Brainstorm @c .mon — one montage definition per file,
 *   used by the Brainstorm GUI for the same purpose.
 *
 * Both directions are supported (single-file @c .sel, fanned-
 * out @c .mon set) and both @c QString and @c std::string
 * overloads exist so the parser can be used from the Qt GUI
 * layer and from headless tests without a string conversion.
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
 * @brief Reader/writer for MNE .sel channel-selection files and Brainstorm .mon montage files.
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
    static bool readMNESelFile(QString path, QMultiMap<QString,QStringList> &selectionMap);

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
    static bool readBrainstormMonFile(QString path, QMultiMap<QString,QStringList> &selectionMap);

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
    static bool writeMNESelFile(QString path, const QMultiMap<QString,QStringList> &selectionMap);

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
    static bool writeBrainstormMonFiles(QString path, const QMultiMap<QString,QStringList> &selectionMap);

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
