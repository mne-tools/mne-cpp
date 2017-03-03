//=============================================================================================================
/**
* @file     show_fiff_settings.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    ShowFiffSettings class declaration.
*
*/

#ifndef SHOWFIFFSETTINGS_H
#define SHOWFIFFSETTINGS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Implements the show Fiff setting parser
*
* @brief Show Fiff setting implementation
*/
class ShowFiffSettings
{
public:
    typedef QSharedPointer<ShowFiffSettings> SPtr;             /**< Shared pointer type for ShowFiffSettings. */
    typedef QSharedPointer<const ShowFiffSettings> ConstSPtr;  /**< Const shared pointer type for ShowFiffSettings. */

    //=========================================================================================================
    /**
    * Default Constructor
    */
    explicit ShowFiffSettings();

    //=========================================================================================================
    /**
    * Constructs Show Fiff Settings
    *
    * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
    * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
    */
    explicit ShowFiffSettings(int *argc,char **argv);

    //=========================================================================================================
    /**
    * Destructs the Compute Forward Settings
    */
    virtual ~ShowFiffSettings();


public:
    QString     inname;                 /**< The input file. */
    int         indent  = -1;           /**< Number of spaces to use in indentation (default %d in terse and 0 in verbose output). */
    bool        verbose = false;        /**< Verbose output. */
    QList<int>  tags;                   /**< Provide information about these tags (can have multiple of these). */
    bool        long_strings = false;   /**< Print long strings in full? */
    bool        blocks_only = false;    /**< Only list the blocks (the tree structure). */

private:
    void usage(char *name);

    bool check_unrecognized_args(int argc, char **argv);
    bool check_args (int *argc,char **argv);

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} //NAMESPACE

#endif // SHOWFIFFSETTINGS_H
