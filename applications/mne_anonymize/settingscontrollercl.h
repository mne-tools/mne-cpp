//=============================================================================================================
/**
 * @file     settingscontrollercl.h
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
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
 * @brief     SettingsControllerCl class declaration.
 *
 */

#ifndef MNEANONYMIZE_SETTINGSCONTROLLERCL_H
#define MNEANONYMIZE_SETTINGSCONTROLLERCL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QCommandLineParser>
#include <QFileInfo>

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
class SettingsControllerCl : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<SettingsControllerCl> SPtr;            /**< Shared pointer type for SettingsControllerCl. */
    typedef QSharedPointer<const SettingsControllerCl> ConstSPtr; /**< Const shared pointer type for SettingsControllerCl. */

    //=========================================================================================================
    /**
     * Default constructor for SettingsControllerCl object.
     */
    SettingsControllerCl();

    //=========================================================================================================
    /**
     * Constructs a SettingsControllerCl object.
     *
     * @param[in] arguments    String list containing the input arguments in form of a QStringList.
     */
    SettingsControllerCl(const QStringList& arguments);

    //=========================================================================================================
    /**
     * Return a the input file QFileInfo.
     *
     */
    QFileInfo getQFiInFile();

    //=========================================================================================================
    /**
     * Return a the output file QFileInfo.
     *
     */
    QFileInfo getQFiOutFile();

    //=========================================================================================================
    /**
     * Signals the FiffAnonymizer method handling both the multi-parallel setup and the single-thread setup.
     */
    virtual int run();

protected:
    //=========================================================================================================
    /**
     * Configures the QCommandLineParser member object with all the necesarry options.
     */
    void initParser();

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
     * If the user wants the name of the input file and the output file to be the same, well... it is not possible
     * in this universe other than masking this behaviour with an auxiliary output filename to be used during the
     * anonymizing process. Eventually, once the reading has finished correctly, the input file can be deleted and
     * the output file can then be called as the original input file. This function helps with this process.
     */
    QString generateRandomFileName();

    //=========================================================================================================
    /**
     * If an output filename is not provided this method generates a default output name.
     */
    QString generateDefaultOutputFileName();

private:

    //=========================================================================================================
    /**
     * Processes the input parser and configures the state of the FiffAnonymizer instance according to
     * the options selected. Including input and output files treatment, execution modes and anonymizing options.
     *
     * @param[in] arguments The arguments to parse.
     *
     * @return Returns true if parsing was successful, false otherwise.
     */
    int parseInOutFiles();

    //=========================================================================================================
    /**
     * The user might request throught the flag "--delete_input_file_after" to have the input file deleted. If the
     * necessary control measures allow for it, then this function will delete the input file and set the control
     * member bool variable m_bInputFileDeleted to true.
     */
    void deleteInputFile();

    //=========================================================================================================
    /**
     * This function checks if the user has requested to have the input file deleted and if the deletion has been
     * confirmed by the user througha  command line prompt. This command line promt can be bypassed through the
     * m_bDeleteInputFileConfirmation flag, which is accessed through the setDeleteInputFileAfterConfirmation public
     * method.
     */
    bool checkDeleteInputFile();

    //=========================================================================================================
    /**
     * This method is responsible to perform a complete check to see if the output filename should be changed.
     * If the outpuf file has a random name, because the user requested this name to be equal to the input filename.
     * And if the output filename has already been deleted, then it should return a true value. If the input file
     * has not been deleted, this method will prompt the user to delete it. The user might still answer no to this
     * confirmation.
     */
    bool checkRenameOutputFile();

    //=========================================================================================================
    /**
     * This method will rename the output file as the input file. It will be called only after all the necessary
     * verification steps have been tested through the checkRenameOutputFile method.
     * It sets the control member bool flag m_bOutputFileRenamed to true and if verbose mode is on, prints a
     * description message.
     */
    void renameOutputFileAsInputFile();

    //=========================================================================================================
    /**
     * Prints a header message if the Verbose option has been set, a header will be printed during execution, right before the
     * file anonymizationFile signal is sent.
     */
    inline void printHeaderIfVerbose();

    //=========================================================================================================
    /**
     * Prints a footer line or message if the Verbose option has been set, a header will be printed during execution, right before the
     * file anonymizationFile signal is sent.
     */
    inline void printFooterIfVerbose();

    //=========================================================================================================
    /**
     *
     * @brief print string to console if the object is set to Verbose Mode on. Or if Silent Mode has not been set.
     *
     * @details Many memeber functions require some text to be printed in the console. This member method allows to abstract from that call
     * the state of the object (Verbose Mode On/Off, Silent Mode On/Off) making it more easily readable.
     *
     * @param[in] str String to print.
     *
     */
    void printIfVerbose(const QString& str) const;

signals:
    /**
     * Signal the main qt core application to stop running and exit.
     */
    void finished(const int);

protected:
    FiffAnonymizer::SPtr m_pAnonymizer;     /**< Local pointer to a Fiffanonyzer object to configure and use.*/
    QString m_sAppName;                     /**< Application name.*/
    QString m_sAppVer;                      /**< Application version number.*/
    QString m_sBuildDate;                   /**< Application build date.*/
    QString m_sBuildHash;                   /**< Repository hash whenever the application was built.*/

    QCommandLineParser m_parser;            /**< Parser object to work with member ptr to QCoreApp and parse input command line options.*/

    QFileInfo m_fiInFile;                   /**< Input File info obj.*/
    QFileInfo m_fiOutFile;                  /**< Output File info obj.*/

protected:
    bool m_bGuiMode;                        /**< Object running in GUI mode.*/
    bool m_bDeleteInputFileAfter;           /**< User's request to delete the input file after anonymization.*/
    bool m_bDeleteInputFileConfirmation;    /**< User's request to avoid confirmation prompt for input file deletion.*/
    bool m_bHisIdSpecified;                 /**< User specified a "his_id" field to be used if that info is present in the input file.*/

private:
    bool m_bVerboseMode;                    /**< Show header when executing.*/
    bool m_bSilentMode;                     /**< Avoid any message to the user.*/
    bool m_bInOutFileNamesEqual;            /**< Flags user's request to have both input and output files with the same name.*/
    bool m_bInputFileDeleted;               /**< Flags if the input file has been deleted. */
    bool m_bOutFileRenamed;                 /**< Flags if the output file has been renamed to match the name the input file had. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void SettingsControllerCl::printIfVerbose(const QString& str) const
{
    if(m_bVerboseMode)
    {
        std::printf("\n%s", str.toUtf8().data());
    }
}

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_SETTINGSCONTROLLERCL_H
