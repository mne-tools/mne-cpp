//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_contents.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     December, 2016
 * @version  dev
 * @brief    MNEFiffContents class declaration.
 */

#ifndef MNEFIFFCONTENTS_H
#define MNEFIFFCONTENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_tag.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QString>
#include <QTextStream>

//=============================================================================================================
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{

class MNEShowFiffSettings;

//=============================================================================================================
/**
 * Prints the tags of a FIFF file, named by @ref FIFFLIB::FiffExplain (MNE-C mne_show_fiff.c).
 *
 * @brief Listing of the contents of a FIFF file.
 */
class MNEFiffContents
{
public:
    /**
     * Show contents of a fif file
     *
     * @param[in] out            Output file
     * @param[in] settings       Show Fiff Settings object
     *
     * @return true if succeeded
     */
    static bool show_fiff_contents(QTextStream& out, const MNEShowFiffSettings& settings);

    /**
     * Show contents of a fif file
     * Refactored: show_fiff_contents (mne_show_fiff.c)
     *
     * @param[in] out            Output file
     * @param[in] name           Input file
     * @param[in] verbose        Verbose output?
     * @param[in] tags           Output these specific tags?
     * @param[in] indent_step    Indentation step
     * @param[in] long_strings   Print long strings in full?
     * @param[in] blocks_only    Print blocks only?
     *
     * @return true if succeeded
     */
    static bool show_fiff_contents(QTextStream& out, const QString& name, bool verbose, const QList<int>& tags, int indent_step, bool long_strings, bool blocks_only);

private:
    static void print_file_id(QTextStream& out, const FIFFLIB::FiffTag::UPtr& tag);

    static void print_ch_info(QTextStream& out, const FIFFLIB::FiffTag::UPtr& tag);

    static void print_transform(QTextStream& out, const FIFFLIB::FiffTag::UPtr& tag);

    static void print_dig_point(QTextStream& out, const FIFFLIB::FiffTag::UPtr& tag);

    static void print_matrix(QTextStream& out, FIFFLIB::FiffStream::SPtr stream, FIFFLIB::FiffDirEntry::SPtr this_ent);
};

} // NAMESPACE SHOWFIFF

#endif // MNEFIFFCONTENTS_H
