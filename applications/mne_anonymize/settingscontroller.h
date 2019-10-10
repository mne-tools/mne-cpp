//=============================================================================================================
/**
* @file     settingscontroller.h
* @author   Juan Garcia-Prieto <Juan.GarciaPrieto@uth.tmc.edu> <juangpc@gmail.com>;
*           Wayne Mead <wayne.mead@uth.tmc.edu> <wayne.isk@gmail.com>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           John C. Mosher <John.C.Mosher@uth.tmc.edu> <jcmosher@gmail.com>;
* @version  1.0
* @date     September, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Juan Garcia-Prieto and Matti Hamalainen. All rights reserved.
*
* the following conditions are met:
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
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
* @brief     SettingsController class declaration.
*
*/

#ifndef FIFFANONYMIZER_SETTINGSCONTROLLER_H
#define FIFFANONYMIZER_SETTINGSCONTROLLER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

/** @brief Contains classes for mne_anonymize application */

namespace MNEANONYMIZE
{


//*************************************************************************************************************
//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Interface between main and fiffAnonymizer object.
* @details Handles command line input parameters, parses them and sets up anynymizer member objects, properly,
*   depending on the different options.
*/
class SettingsController
{

public:
    typedef QSharedPointer<SettingsController> SPtr;            /**< Shared pointer type for SettingsController. */
    typedef QSharedPointer<const SettingsController> ConstSPtr; /**< Const shared pointer type for SettingsController. */

    //=========================================================================================================
    /**
    * Constructs a SettingsController object.
    *
    * @param [in] arguments     The input arguments in form of a QStringList.
    */
    SettingsController(const QStringList& arguments,
                       const QString& name,
                       const QString& ver);

    //=========================================================================================================
    /**
    * Destroys SettingsController Object.
    * All space allocated for FiffAnonymizer objects will be deleted.
    */
    ~SettingsController();

    //=========================================================================================================
    /**
    * Signals the FiffAnonymizer method handling both the multi-parallel setup and the single-thread setup.
    */
    void execute();

    //=========================================================================================================

private:
    //=========================================================================================================
    /**
    * Configures the QCommandLineParser member object with all the necesarry options.
    */
    void initParser();

    //=========================================================================================================
    /**
    * Processes the input parser and configures the state of the FiffAnonymizer instance according to
    * the options selected. Including input and output files treatment, execution modes and anonymizing options.
    */
    void parseInputs(const QStringList& arguments);

    //=========================================================================================================
    /**
    * Processes the options related to input and output files, like file names or deletion options. It also
    * hables wild cards used while defining input filenames and configures the member QStringList with
    * filenames to anonymize.
    */
    void parseInputAndOutputFiles();

    //=========================================================================================================
    /**
    * Handles filenames and, if needed, generates all the necessary FiffAnonyzer objects for a concurrent
    * execution. All new instances (located in the heap) will be created through the copy constructor, as
    * exact copies from the member FiffAnonyizer object. And then modified only the necessary options.
    */
    void generateAnonymizerInstances();

    //=========================================================================================================
    /**
    * Prints a header message if the Verbose option has been set, a header will be printed during execution, right before the
    * file anonymizationFile signal is sent.
    */
    void printHeaderIfVerbose();

    //=========================================================================================================

    //we at least create one app as a member var here in the stack
    //if we later see we have more than one file we'll create more in the heap.
    //So that we can deploy a concurrent execution (one app per thread).
    FiffAnonymizer m_anonymizer;        /**< local instance of Fiffanonyzer, in which perform configurations.*/
    QString m_sAppName;                 /**< Application name.*/
    QString m_sAppVer;                  /**< Application version number.*/
    QStringList m_SLInFiles;            /**< List of input file names (absolute paths).*/
    QStringList m_SLOutFiles;           /**< List of output file names (absolute paths).*/
    bool m_bShowHeaderFlag;                 /**< Show header when executing.*/
    bool m_bMultipleInFiles;            /**< Multpiple files concurrent execution flag.*/

    QList<QSharedPointer<FiffAnonymizer> > m_pAppList; /**< list of addresses to FiffAnonyizer objects. */
    QList<QSharedPointer<QFuture<void> > > promisesList; /**< List of synchronizing waits for each concurrent execution.*/
    QCommandLineParser m_parser;        /**< parser object to work with member pointer to QCoreApp and parse input command line options.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

/**
* Finds all files in a folder matching a filename pattern (compatible with wildcard [*,?]).
* @details This is a helper function. Given a filename with some pattern. It lists all possible filenames matching the pattern.
* @param [in] Reference to a QString containing the input filename search pattern.
* @param [out] QStringList with all possible filenames compatible with the search pattern.
* It outputs a QStringList with all the possible files in the folder matching the search pattern.
*/
inline static QStringList listFilesMatchingPatternName(const QString &fileName);

} // namespace MNEANONYMIZE

#endif // FIFFANONYMIZER_SETTINGSCONTROLLER_H
