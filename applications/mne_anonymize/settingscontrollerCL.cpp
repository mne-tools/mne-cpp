//=============================================================================================================
/**
 * @file     SettingsControllerCL.cpp
 * @author   Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
 * @date     September, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Wayne Mead, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, John C. Mosher. All rights reserved.
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
 * @brief    SettingsControllerCL class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontrollerCL.h"
#include "fiffanonymizer.h"
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsControllerCL::SettingsControllerCL(const QStringList& arguments)
: m_sAppName(qApp->applicationName())
, m_sAppVer(qApp->applicationVersion())
, m_bVerboseMode(false)
, m_bInOutFileNamesEqual(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bInputFileDeleted(false)
, m_bOutFileRenamed(false)
{
    m_pAnonymizer = FiffAnonymizer::SPtr(new FiffAnonymizer);
    initParser();
    if(parseInputs(arguments))
    {
        qCritical() << "Something went wrong during the parsing of input options.";
        return;
    }

    if(execute())
    {
        qCritical() << "Error during the anonymization of the input file";
        return;
    }
}

//=============================================================================================================

void SettingsControllerCL::initParser()
{
    m_parser.setApplicationDescription(QCoreApplication::translate("main",
           "\nApplication that removes or modifies Personal Health Information or Personal Identifiable information from a FIFF file."
           "\n\nIf they exist, the following fields will be anonymized from the fif file:"
           "\n - Measurement Date"
           "\n - MAC address of the acquisition computer"
           "\n - Text description of the acquisition system"
           "\n - Experimenter acquiring the data"
           "\n - Subject Id"
           "\n - Subject First Name"
           "\n - Subject's Middle Name"
           "\n - Subject's Last Name"
           "\n - Subject's Birthday date (can be altered by some number of days)"
           "\n - Subject's Sex (only with brute option)"
           "\n - Subject's Handedness (only with brute option)"
           "\n - Subject's Weight (only with brute option)"
           "\n - Subject's Height (only with brute option)"
           "\n - Subject's Text Comment"
           "\n - Subject's Hospital Id"
           "\n - Project's Id (only with brute option)"
           "\n - Project's Name (only with brute option)"
           "\n - Project's Aim (only with brute option)"
           "\n - Project's Persons"
           "\n - Project's Comment (only with brute option)"
           "\n "
           "\n - *** Additionally if there is MRI data present in the file a warning message will appear.\n"));
    m_parser.addHelpOption();

    //just for completeness.
    QCommandLineOption noGUIOpt("gui",QCoreApplication::translate("main","GUI version of the application."));
    m_parser.addOption(noGUIOpt);

    QCommandLineOption versionOpt("version",QCoreApplication::translate("main","Prints out the version of this application."));
    m_parser.addOption(versionOpt);

    QCommandLineOption inFileOpt(QStringList() << "i" << "in",
                                 QCoreApplication::translate("main","File to anonymize. Multiple --in <infile> statements can be present (files will be "
                                                                    "processed in parallel)."),
                                 QCoreApplication::translate("main","infile"));
    m_parser.addOption(inFileOpt);

    QCommandLineOption outFileOpt(QStringList() << "o" << "out",
                                  QCoreApplication::translate("main","Output file <outfile>. Only allowed when anonymizing one single file. As default "
                                                                     "‘_anonymized.fif’ is attached to the file name."),
                                  QCoreApplication::translate("main","outfile"));
    m_parser.addOption(outFileOpt);

    QCommandLineOption verboseOpt(QStringList() << "v" << "verbose",
                                  QCoreApplication::translate("main","Prints out more information, about each specific anonymized field. Only allowed "
                                                                     "when anonymizing one single file. Default: false"));
    m_parser.addOption(verboseOpt);

    QCommandLineOption deleteInFileOpt(QStringList() << "d" << "delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization. A confirmation message will be "
                                                                          "prompted to the user. Default: false"));
    m_parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt(QStringList() << "ad" << "avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file. Default: false"));
    m_parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt("brute",
                                QCoreApplication::translate("main","Anonymize additional subject’s information like weight, height, sex and handedness, "
                                                            "and project’s ID, name, aim and comment. Default: false"));
    m_parser.addOption(bruteOpt);

    QCommandLineOption measDateOpt(QStringList() << "md" << "measurement_date",
                                   QCoreApplication::translate("main","Specify the measurement date. Only when anonymizing a single file. Format: YYYMMDD. "
                                                                      "Default: 20000101"),
                                   QCoreApplication::translate("main","days"));
    m_parser.addOption(measDateOpt);

    QCommandLineOption measDateOffsetOpt(QStringList() << "mdo" << "measurement_date_offset",
                                         QCoreApplication::translate("main","Specify number of days to subtract to the measurement . Only allowed when "
                                                                            "anonymizing a single file. Default: 0"),
                                         QCoreApplication::translate("main","date"));
    m_parser.addOption(measDateOffsetOpt);

    QCommandLineOption birthdayOpt(QStringList() << "sb" << "subject_birthday",
                                   QCoreApplication::translate("main","Specify the subject’s birthday . Only allowed when anonymizing a single file. "
                                                                      "Format: YYYMMDD. Default: 20000101"),
                                   QCoreApplication::translate("main","date"));
    m_parser.addOption(birthdayOpt);

    QCommandLineOption birthdayOffsetOpt(QStringList() << "sbo" << "subject_birthday_offset",
                                         QCoreApplication::translate("main","Specify number of to subtract to the subject's birthday. Only allowed when "
                                                                            "anonymizing a single file. Default: 0"),
                                         QCoreApplication::translate("main","days"));
    m_parser.addOption(birthdayOffsetOpt);

    QCommandLineOption SubjectIdOpt("his",QCoreApplication::translate("main","Specify the subject’s ID within the Hospital system. Only allowed when "
                                                                             "anonymizing a single file. Default: ‘mne_anonymize’"),
                                          QCoreApplication::translate("main","id#"));
    m_parser.addOption(SubjectIdOpt);

}

//=============================================================================================================

int SettingsControllerCL::parseInputs(const QStringList& arguments)
{
    m_parser.process(arguments);

    if(!m_parser.isSet("no-gui"))
    {
        qCritical() << "Error while running the application. Something went wrong.";
        return 1;
    }

    if(m_parser.isSet("version"))
    {
        m_parser.showVersion();
    }

    if(parseInOutFiles())
    {
        return 1;
    }

    if(m_parser.isSet("verbose"))
    {
        m_bVerboseMode = true;
        m_pAnonymizer->setVerboseMode(true);
    }

    if(m_parser.isSet("brute")) {
        m_pAnonymizer->setBruteMode(true);
    }

    if(m_parser.isSet("delete_input_file_after"))
    {
        m_bDeleteInputFileAfter = true;
    }

    if(m_parser.isSet("avoid_delete_confirmation"))
    {
        m_bDeleteInputFileConfirmation = false;
    }

    if(m_parser.isSet("measurement_date"))
    {
        if(m_parser.isSet("measurement_date_offset"))
        {
            qCritical() << "You cannot specify the measurement date and the measurement date offset at "
                           "the same time.";
            m_parser.showHelp();
        }
        QString strMeasDate(m_parser.value("measurement_date"));
        m_pAnonymizer->setMeasurementDay(strMeasDate);
    }

    if(m_parser.isSet("measurement_date_offset"))
    {
        if(m_parser.isSet("measurement_date"))
        {
            qCritical() << "You cannot specify the measurement date and the measurement date offset at "
                           "the same time.";
            m_parser.showHelp();
        }

        int intMeasDateOffset(m_parser.value("measurement_date_offset").toInt());
        m_pAnonymizer->setMeasurementDayOffset(intMeasDateOffset);
    }

    if(m_parser.isSet("subject_birthday"))
    {
        if(m_parser.isSet("subject_birthday_offset"))
        {
            qCritical() << "You cannot specify the subject's birthday and subject's birthday offset"
                           "the same time.";
            m_parser.showHelp();
        }

        QString strBirthday(m_parser.value("subject_birthday"));
        m_pAnonymizer->setSubjectBirthday(strBirthday);
    }

    if(m_parser.isSet("subject_birthday_offset"))
    {
        if(m_parser.isSet("subject_birthday"))
        {
            qCritical() << "You cannot specify the subject's birthday and subject's birthday offset"
                           "the same time.";
            m_parser.showHelp();
        }
        int strBirthdayOffset(m_parser.value("subject_birthday_offset").toInt());
        m_pAnonymizer->setSubjectBirthdayOffset(strBirthdayOffset);
    }

    if(m_parser.isSet("his"))
    {
        QString strHisId(m_parser.value("his"));
        m_pAnonymizer->setSubjectHisId(strHisId);
    }

    return 0;
}

//=============================================================================================================

int SettingsControllerCL::parseInOutFiles()
{

    if(m_parser.isSet("in"))
    {
        m_fiInFileInfo.setFile(m_parser.value("in"));
        if(m_fiInFileInfo.isFile())
        {
            if(m_pAnonymizer->setFileIn(m_fiInFileInfo.absoluteFilePath()))
            {
                qCritical() << "Error while setting the input file.";
                return 1;
            }
        } else {
            qCritical() << "Input file is not a file.";
            return 1;
        }
    } else {
        qCritical() << "No valid input file specified.";
        m_parser.showHelp();
    }


    if(m_parser.isSet("out"))
    {
        m_fiOutFileInfo.setFile(m_parser.value("out"));
        if(m_fiOutFileInfo.isDir())
        {
            qCritical() << "Error. Output file is infact a folder.";
            return 1;
        } else {
            if(m_fiInFileInfo.absoluteFilePath() == m_fiOutFileInfo.absoluteFilePath())
            {
                m_bInOutFileNamesEqual = true;
                QString fileOut(QDir(m_fiInFileInfo.absolutePath()).filePath(generateRandomFileName()));
                m_fiOutFileInfo.setFile(fileOut);
            }
        }
    } else {
        QString fileOut(QDir(m_fiInFileInfo.absolutePath()).filePath(
                    m_fiInFileInfo.baseName() + "_anonymized." + m_fiInFileInfo.completeSuffix()));
        m_fiOutFileInfo.setFile(fileOut);
    }
    if(m_pAnonymizer->setFileOut(m_fiOutFileInfo.absoluteFilePath()))
    {
        qCritical() << "Error while setting the output file.";
        return 1;
    }
    return 0;
}

//=============================================================================================================

int SettingsControllerCL::execute()
{
    printHeaderIfVerbose();
    if(m_pAnonymizer->anonymizeFile())
    {
        qCritical() << "Error. Program ends now.";
        return 1;
    }

    if(checkDeleteInputFile())
    {
        deleteInputFile();
    }

    if(checkRenameOutputFile())
    {
        renameOutputFileAsInputFile();
    }

    printf("\n%s", QString("MNE Anonymize finished correctly: " + m_fiInFileInfo.fileName() + " -> " + m_fiOutFileInfo.fileName()).toUtf8().data());

    printFooterIfVerbose();

    return 0;
}

//=============================================================================================================

bool SettingsControllerCL::checkDeleteInputFile()
{
    if(m_bDeleteInputFileAfter) //false by default
    {
        printf("%s", QString("You have requested to delete the input file: " + m_fiInFileInfo.fileName()).toUtf8().data());

        if(m_bDeleteInputFileConfirmation) //true by default
        {
            QTextStream consoleIn(stdin);
            QString confirmation;
            printf("\n%s",QString("You can avoid this confirmation by using the delete_confirmation option.").toUtf8().data());
            printf("\n%s",QString("Are you sure you want to delete this file? [Y/n] ").toUtf8().data());
            consoleIn >> confirmation;

            if(confirmation == "Y")
            {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

//=============================================================================================================

void SettingsControllerCL::deleteInputFile()
{
    QFile inFile(m_fiInFileInfo.absoluteFilePath());
    m_bInputFileDeleted = inFile.remove();
    printIfVerbose("Input file deleted.");
}

//=============================================================================================================

bool SettingsControllerCL::checkRenameOutputFile()
{
    //if both files in and out have the same name, Anonymizer class would already know and a temporary
    //random filename will be in use, during the anonymizing process, for the output file.
    //When this function is called Anonymizer will check if this needs to be reverted:
    // -if the infile has been deleted already there is no conflict->outfile name = infile name.
    // -if the infile has not been deleted but the user has never been asked. They is asked.
    // -if the infile has not been deleted but the user was already asked, it means they answered NO.
    //  Thus, a warning is shown.
    if(m_bInOutFileNamesEqual) {
        if(m_bDeleteInputFileAfter)
        {
            if(m_bInputFileDeleted)
            {
                return true;
            }
        } else {
            m_bDeleteInputFileAfter = true;
            if(checkDeleteInputFile())
            {
                deleteInputFile();
                return true;
            } else {
                printf("\n%s", QString("You have requested to save the output file with the same name as the input file.").toUtf8().data());
                printf("\n%s", QString("This cannot be done without deleting or modifying the input file.").toUtf8().data());
                printf("\n%s", QString(" ").toUtf8().data());
            }
        }
    }

    return false;
}

//=============================================================================================================

void SettingsControllerCL::renameOutputFileAsInputFile()
{
    QFile auxFile(m_fiOutFileInfo.fileName());
    auxFile.rename(m_fiInFileInfo.fileName());
    m_bOutFileRenamed = true;
    if(m_bVerboseMode)
    {
        printf("\n%s",QString("Output file named: " + m_fiOutFileInfo.fileName() + " --> renamed as: " + m_fiInFileInfo.fileName()).toUtf8().data());
    }
    m_fiOutFileInfo.setFile(m_fiInFileInfo.fileName());
}

//=============================================================================================================

void SettingsControllerCL::printHeaderIfVerbose()
{

    printIfVerbose(" ");
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
    printIfVerbose(m_sAppName);
    printIfVerbose("Version: " + m_sAppVer);
    printIfVerbose(" ");
}

//=============================================================================================================

void SettingsControllerCL::printFooterIfVerbose()
{
    printIfVerbose(" ");
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
}

//=============================================================================================================

QString SettingsControllerCL::generateRandomFileName()
{
    QString randomFileName("mne_anonymize_");
    const QString charPool("abcdefghijklmnopqrstuvwxyz1234567890");
    const int randomLength(12);

    for(int i=0;i<randomLength;++i)
    {
        int p=qrand() % charPool.length();
        randomFileName.append(charPool.at(p));
    }

    return randomFileName.append(".fif");
}
