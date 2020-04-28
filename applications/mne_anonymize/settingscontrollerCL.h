//=============================================================================================================
/**
 * @file     SettingsControllerCL.h
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @version  dev
 * @date     September, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, Wayne Mead, John C. Mosher. All rights reserved.
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
 * @brief     SettingsControllerCL class declaration.
 *
 */

#ifndef MNEANONYMIZE_SettingsControllerCL_H
#define MNEANONYMIZE_SettingsControllerCL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE
{

//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Interface between main and fiffAnonymizer object.
 * @details Handles command line input parameters, parses them and sets up anynymizer member objects, properly,
 *          depending on the different options.
 */
class SettingsControllerCL
{

public:
    typedef QSharedPointer<SettingsControllerCL> SPtr;            /**< Shared pointer type for SettingsControllerCL. */
    typedef QSharedPointer<const SettingsControllerCL> ConstSPtr; /**< Const shared pointer type for SettingsControllerCL. */

    //=========================================================================================================
    /**
     * Constructs a SettingsControllerCL object.
     *
     * @param [in] arguments    String list containing the input arguments in form of a QStringList.
     * @param [in] name         String containing the name of the application.
     * @param [in] ver          String containing the version of the application.
     */
    SettingsControllerCL(const QStringList& arguments,
                       const QString& name,
                       const QString& ver);

    private:
    //=========================================================================================================
    /**
     * Signals the FiffAnonymizer method handling both the multi-parallel setup and the single-thread setup.
     */
    int execute();

    //=========================================================================================================
    /**
     * Configures the QCommandLineParser member object with all the necesarry options.
     */
    int initParser();

    //=========================================================================================================
    /**
     * Processes the input parser and configures the state of the FiffAnonymizer instance according to
     * the options selected. Including input and output files treatment, execution modes and anonymizing options.
     *
     * @param[in] arguments The arguments to parse.
     *
     * @return Returns true if parsing was successful, false otherwise.
     */
    int parseInputs(const QStringList& arguments);

    //=========================================================================================================
    /**
     * Prints a header message if the Verbose option has been set, a header will be printed during execution, right before the
     * file anonymizationFile signal is sent.
     */
    void printHeaderIfVerbose();

    //=========================================================================================================
    /**
     * Prints version information when the "--version" option has been used in the application call. It is the
     * first option to be parsed and if it is set, the application stops and none of the other options will be
     * executed.
     */
    void printVersionInfo();

    //=========================================================================================================

    FiffAnonymizer::SPtr m_pAnonymizer; /**< Local pointer to a Fiffanonyzer object to configure and use.*/
    QString m_sAppName;                 /**< Application name.*/
    QString m_sAppVer;                  /**< Application version number.*/
    bool m_bVerboseMode;                /**< Show header when executing.*/
    QCommandLineParser m_parser;        /**< Parser object to work with member ptr to QCoreApp and parse input command line options.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_SettingsControllerCL_H
