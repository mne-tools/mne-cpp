//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_show_fiff_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     February, 2017
 * @version  dev
 * @brief    MNEShowFiffSettings class declaration.
 */

#ifndef SHOWFIFFSETTINGS_H
#define SHOWFIFFSETTINGS_H

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>
#include <vector>


//=============================================================================================================
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * Implements the show Fiff setting parser
 *
 * @brief Show Fiff setting implementation
 */
class MNEShowFiffSettings
{
public:
    typedef std::shared_ptr<MNEShowFiffSettings> SPtr;            /**< Shared pointer type for MNEShowFiffSettings. */
    typedef std::shared_ptr<const MNEShowFiffSettings> ConstSPtr; /**< Const shared pointer type for MNEShowFiffSettings. */

    //=========================================================================================================
    /**
     * Default Constructor
     */
    explicit MNEShowFiffSettings();

    //=========================================================================================================
    /**
     * Constructs Show Fiff Settings
     *
     * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
     * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
     */
    explicit MNEShowFiffSettings(int* argc, char** argv);

    //=========================================================================================================
    /**
     * Destructs the Show Fiff Settings
     */
    virtual ~MNEShowFiffSettings();

    //=========================================================================================================
    /**
     * Check whether Show Fiff Settings are correctly set.
     */
    void checkIntegrity();

public:
    std::string inname;    /**< The input file. */
    int indent;            /**< Number of spaces to use in indentation (default %d in terse and 0 in verbose output). */
    bool verbose;          /**< Verbose output. */
    std::vector<int> tags; /**< Provide information about these tags (can have multiple of these). */
    bool long_strings;     /**< Print long strings in full? */
    bool blocks_only;      /**< Only list the blocks (the tree structure). */

private:
    void usage(char* name);
    bool check_unrecognized_args(int argc, char** argv);
    bool check_args(int* argc, char** argv);
};


} //NAMESPACE SHOWFIFF

#endif // SHOWFIFFSETTINGS_H
