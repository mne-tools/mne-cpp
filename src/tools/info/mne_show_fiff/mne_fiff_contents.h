//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_contents.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     2006
 * @version  dev
 * @brief    MNEFiffContents class declaration.
 */

#ifndef MNEFIFFCONTENTS_H
#define MNEFIFFCONTENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{

class MNEShowFiffSettings;

//=============================================================================================================
/**
 * Prints the tags of a FIFF file, named by @ref mne::FiffExplain (MNE-C mne_show_fiff.c).
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
    static bool show_fiff_contents(std::FILE* out, const MNEShowFiffSettings& settings);

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
    static bool show_fiff_contents(std::FILE* out, const std::string& name, bool verbose, const std::vector<int>& tags, int indent_step, bool long_strings, bool blocks_only);

private:
    static void print_file_id(std::FILE* out, const mne::FiffTag& tag);

    static void print_ch_info(std::FILE* out, const mne::FiffTag& tag);

    static void print_transform(std::FILE* out, const mne::FiffTag& tag);

    static void print_dig_point(std::FILE* out, const mne::FiffTag& tag);

    static void print_matrix(std::FILE* out, mne::FiffStream& stream, mne::FiffDirEntry this_ent);
};

} // NAMESPACE SHOWFIFF

#endif // MNEFIFFCONTENTS_H
