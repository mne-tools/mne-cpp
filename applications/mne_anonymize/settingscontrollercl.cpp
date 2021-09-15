//=============================================================================================================
/**
 * @file     settingscontrollercl.cpp
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
 * @brief    SettingsControllerCl class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontrollercl.h"
#include "fiffanonymizer.h"
#include "utils/buildinfo.h"
#include "utils/utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QRandomGenerator>
#include <QDir>
#include <QFileInfo>

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

SettingsControllerCl::SettingsControllerCl()
: m_pAnonymizer(FiffAnonymizer::SPtr(new FiffAnonymizer))
, m_sAppName(qApp->applicationName())
, m_sAppVer(qApp->applicationVersion())
, m_sBuildDate(QString(BUILDINFO::timestamp()))
, m_sBuildHash(QString(BUILDINFO::githash()))
, m_bGuiMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bHisIdSpecified(false)
, m_bVerboseMode(false)
, m_bSilentMode(false)
, m_bInOutFileNamesEqual(false)
, m_bInputFileDeleted(false)
, m_bOutFileRenamed(false)
{
}

//=============================================================================================================

SettingsControllerCl::SettingsControllerCl(const QStringList& arguments)
: m_pAnonymizer(FiffAnonymizer::SPtr(new FiffAnonymizer))
, m_sAppName(qApp->applicationName())
, m_sAppVer(qApp->applicationVersion())
, m_sBuildDate(QString(BUILDINFO::timestamp()))
, m_sBuildHash(QString(BUILDINFO::githash()))
, m_bGuiMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bHisIdSpecified(false)
, m_bVerboseMode(false)
, m_bSilentMode(false)
, m_bInOutFileNamesEqual(false)
, m_bInputFileDeleted(false)
, m_bOutFileRenamed(false)
{
    QObject::connect(this, &MNEANONYMIZE::SettingsControllerCl::finished,
                     qApp, &QCoreApplication::exit, Qt::QueuedConnection);

    initParser();
    if(parseInputs(arguments))
    {
        qCritical() << "Something went wrong during the parsing of input options.";
        return;
    }

    printHeaderIfVerbose();
    printIfVerbose(QString("Executing command: ") + arguments.join(" "));
}

//=============================================================================================================

void SettingsControllerCl::initParser()
{
    m_parser.setApplicationDescription(QCoreApplication::translate("main",
           "\nMNE-CPP Project. UT-Health (McGovern Medical School) Houston, Tx."
           "\nMNE_ANONYMIZE"
           "\nMain authors: Juan Garcia-Prieto <juangpc@gmail.com>"
           "\n              Wayne Mead <wayne.mead@uth.tmc.edu>"
           "\n              Lorenz Esch <lesch@mgh.harvard.edu>"
           "\n"
           "\nMNE_ANONYMIZE removes or modifies Personal Health Information and Personal Identifiable information from a FIFF file."
           "\n\nIf the information exists in the input file, the following fields will be anonymized in the output file:"
           "\n"
           "\n - Measurement Date (can be altered by some number of days)"
           "\n - MAC address of the acquisition computer"
           "\n - Text description of the acquisition system"
           "\n - Experimenter"
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
           "\n - MNE Toolbox info: Working Directory"
           "\n - MNE Toolbox info: Command line used"
           "\n "
           "\n - Additionally if there is MRI data present in the file a warning message will appear.\n"));
    m_parser.addHelpOption();

    //this breaks encapsulation. damn it. it has to be here in order to show in the help text.
    QCommandLineOption commandLineOpt("no-gui",QCoreApplication::translate("main","Command line version of this application."));
    m_parser.addOption(commandLineOpt);

    QCommandLineOption versionOpt("version",QCoreApplication::translate("main","Show the version of this application."));
    m_parser.addOption(versionOpt);

    QCommandLineOption inFileOpt(QStringList() << "i" << "in",
                                 QCoreApplication::translate("main","File to anonymize."),
                                 QCoreApplication::translate("main","infile"));
    m_parser.addOption(inFileOpt);

    QCommandLineOption outFileOpt(QStringList() << "o" << "out",
                                  QCoreApplication::translate("main","Output file <outfile>. Default \"_anonymized.fif\" will be attached to the input file name."),
                                  QCoreApplication::translate("main","outfile"));
    m_parser.addOption(outFileOpt);

    QCommandLineOption verboseOpt(QStringList() << "v" << "verbose",
                                  QCoreApplication::translate("main","Prints out more information, about each specific anonymized field. Default: false"));
    m_parser.addOption(verboseOpt);

    QCommandLineOption silentOpt(QStringList() << "s" << "silent",
                                  QCoreApplication::translate("main","Prints no output to the terminal, other than execution or configuration errors. Default: false"));
    m_parser.addOption(silentOpt);

    QCommandLineOption deleteInFileOpt(QStringList() << "d" << "delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization. A confirmation message will be prompted to the user."
                                                                          "Default: false"));
    m_parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt(QStringList() << "f" << "avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file. Default: false"));
    m_parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt(QStringList() << "b" << "brute",
                                QCoreApplication::translate("main","Anonymize additional subject information like weight, height, sex and handedness, and project data,"
                                                            " subject's data. See help above. Default: false"));
    m_parser.addOption(bruteOpt);

    QCommandLineOption measDateOpt(QStringList() << "md" << "measurement_date",
                                   QCoreApplication::translate("main","Specify the measurement date. Only when anonymizing a single file. Format: DDMMYYYY. Default: 01012000"),
                                   QCoreApplication::translate("main","days"));
    m_parser.addOption(measDateOpt);

    QCommandLineOption measDateOffsetOpt(QStringList() << "mdo" << "measurement_date_offset",
                                         QCoreApplication::translate("main","Specify number of days to subtract to the measurement. Default: 0"),
                                         QCoreApplication::translate("main","date"));
    m_parser.addOption(measDateOffsetOpt);

    QCommandLineOption birthdayOpt(QStringList() << "sb" << "subject_birthday",
                                   QCoreApplication::translate("main","Specify the subject birthday date. Format: DDMMYYYY. Default: 01012000"),
                                   QCoreApplication::translate("main","date"));
    m_parser.addOption(birthdayOpt);

    QCommandLineOption birthdayOffsetOpt(QStringList() << "sbo" << "subject_birthday_offset",
                                         QCoreApplication::translate("main","Specify number of to subtract to the subject's birthday. Default: 0"),
                                         QCoreApplication::translate("main","days"));
    m_parser.addOption(birthdayOffsetOpt);

    QCommandLineOption subjectIdOpt("his",QCoreApplication::translate("main","Specify the subject ID within the Hospital information system. Default: \"mne_anonymize\""),
                                          QCoreApplication::translate("main","id#"));
    m_parser.addOption(subjectIdOpt);

    QCommandLineOption mneEnvironmentOpt("mne_environment",
                                         QCoreApplication::translate("main","Anonymize information related to the MNE environment. "
                                                                                       "If found in the file, Working Directory or command line tags will be anonymized."));
    m_parser.addOption(mneEnvironmentOpt);
}

//=============================================================================================================

int SettingsControllerCl::parseInputs(const QStringList& arguments)
{
    m_parser.process(arguments);

    if(m_parser.isSet("no-gui"))
    {
        m_bGuiMode = false;
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

    if(m_parser.isSet("silent"))
    {
        m_bSilentMode = true;
        m_bVerboseMode = false;
        m_pAnonymizer->setVerboseMode(false);
    }

    if(m_parser.isSet("brute"))
    {
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
        m_pAnonymizer->setMeasurementDate(strMeasDate);
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
        m_pAnonymizer->setUseMeasurementDateOffset(true);
        m_pAnonymizer->setMeasurementDateOffset(intMeasDateOffset);
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
        m_pAnonymizer->setUseSubjectBirthdayOffset(true);
        m_pAnonymizer->setSubjectBirthdayOffset(strBirthdayOffset);
    }

    if(m_parser.isSet("his"))
    {
        m_bHisIdSpecified = true;
        QString strHisId(m_parser.value("his"));
        m_pAnonymizer->setSubjectHisId(strHisId);
    }

    if(m_parser.isSet("mne_environment"))
    {
        m_pAnonymizer->setMNEEnvironmentMode(true);
    }

    return 0;
}

//=============================================================================================================

int SettingsControllerCl::parseInOutFiles()
{

    if(m_parser.isSet("in"))
    {
        m_fiInFile.setFile(m_parser.value("in"));
        if(m_fiInFile.isFile())
        {
            if(m_pAnonymizer->setInFile(m_fiInFile.absoluteFilePath()))
            {
                qCritical() << "Error while setting the input file.";
                return 1;
            }
        } else {
            qCritical() << "Input file is not a file.";
            return 1;
        }
    } else {
        if(!m_bGuiMode)
        {
            qCritical() << "No valid input file specified.";
            m_parser.showHelp();
        }
    }

    if(m_parser.isSet("out"))
    {
        m_fiOutFile.setFile(m_parser.value("out"));
        if(m_fiOutFile.isDir())
        {
            qCritical() << "Error. Output file is infact a folder.";
            return 1;
        } else {
            if((m_fiInFile.absoluteFilePath() == m_fiOutFile.absoluteFilePath()))
            {
                m_bInOutFileNamesEqual = true;
                QString fileOut(QDir(m_fiInFile.absolutePath()).filePath(generateRandomFileName()));
                m_fiOutFile.setFile(fileOut);
                if(m_pAnonymizer->setOutFile(m_fiOutFile.absoluteFilePath()))
                {
                    qCritical() << "Error while setting the output file.";
                    return 1;
                }
            }
            if(m_pAnonymizer->setOutFile(m_fiOutFile.absoluteFilePath()))
            {
                qCritical() << "Error while setting the output file.";
                return 1;
            }
        }
    } else {
        if(!m_bGuiMode)
        {
            generateDefaultOutputFileName();
            if(m_pAnonymizer->setOutFile(m_fiOutFile.absoluteFilePath()))
            {
                qCritical() << "Error while setting the output file.";
                return 1;
            }
        }
    }
    return 0;
}

//=============================================================================================================

int SettingsControllerCl::run()
{
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

    if(!m_bSilentMode)
    {
        std::printf("\n%s\n", QString("MNE Anonymize finished correctly: " + m_fiInFile.fileName() + " -> " + m_fiOutFile.fileName()).toUtf8().data());
    }

    printFooterIfVerbose();

    emit finished(0);

    return 0;
}

//=============================================================================================================

bool SettingsControllerCl::checkDeleteInputFile()
{
    if(m_bDeleteInputFileAfter) //false by default
    {
        if(!m_bSilentMode)
        {
            std::printf("\n%s", QString("You have requested to delete the input file: " + m_fiInFile.fileName()).toUtf8().data());
        }

        if(m_bDeleteInputFileConfirmation) //true by default
        {
            QTextStream consoleIn(stdin);
            QString confirmation;
            std::printf("\n%s",QString("You can avoid this confirmation by using the delete_confirmation [-f] option.").toUtf8().data());
            std::printf("\n%s",QString("Are you sure you want to delete the input file? [Y/n] ").toUtf8().data());
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

void SettingsControllerCl::deleteInputFile()
{
    QFile inFile(m_fiInFile.absoluteFilePath());
    if((m_bInputFileDeleted = inFile.remove()))
    {
        printIfVerbose("Input file deleted.");
    } else {
        qCritical() << "Unable to delete the input file: " << inFile.fileName();
    }
}

//=============================================================================================================

bool SettingsControllerCl::checkRenameOutputFile()
{
    //if both files in and out have the same name, this controller class would already know and a temporary
    //random filename will be in use, during the anonymizing process, for the output file.
    //When this function is called we will check if this needs to be reverted:
    // -if the infile has been deleted already there is no conflict->outfile name = infile name.
    // -if the infile has not been deleted but the user has never been asked. They is asked.
    // -if the infile has not been deleted but the user was already asked, it means they answered NO.
    //      Thus, a warning is shown.
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
                if(!m_bSilentMode)
                {
                    std::printf("\n%s", QString("You have requested to save the output file with the same name as the input file.").toUtf8().data());
                    std::printf("\n%s", QString("This cannot be done without deleting or modifying the input file.").toUtf8().data());
                    std::printf("\n%s", QString(" ").toUtf8().data());
                }
            }
        }
    }

    return false;
}

//=============================================================================================================

void SettingsControllerCl::renameOutputFileAsInputFile()
{
    QFile auxFile(m_fiOutFile.absoluteFilePath());
    if((m_bOutFileRenamed = auxFile.rename(m_fiInFile.absoluteFilePath())))
    {
        if(m_bVerboseMode)
        {
            std::printf("\n%s",QString("Output file named: " + m_fiOutFile.fileName() + " --> renamed as: " + m_fiInFile.fileName()).toUtf8().data());
        }
        m_fiOutFile.setFile(m_fiInFile.absoluteFilePath());
    } else {
        qCritical() << "Error while renaming the output file: " << auxFile.fileName() << " as " << m_fiInFile.fileName();
    }
}

//=============================================================================================================

void SettingsControllerCl::printHeaderIfVerbose()
{
    printIfVerbose(" ");
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
    printIfVerbose(m_sAppName + "  (Version: " + m_sAppVer + ")");
    printIfVerbose("Build Date: " + m_sBuildDate);
    printIfVerbose("Build Hash: " + m_sBuildHash);
    printIfVerbose(" ");
    printIfVerbose(QString("Utils Lib Build Date: ") + UTILSLIB::BUILD_DATETIME());
    printIfVerbose(QString("Utils Lib Build Hash: ") + UTILSLIB::BUILD_HASH());
    printIfVerbose(" ");
}

//=============================================================================================================

void SettingsControllerCl::printFooterIfVerbose()
{
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
}

//=============================================================================================================

QString SettingsControllerCl::generateRandomFileName()
{
    QString randomFileName("mne_anonymize_");
    const QString charPool("abcdefghijklmnopqrstuvwxyz1234567890");
    const int randomLength(12);

    for(int i=0;i<randomLength;++i)
    {
        int p(QRandomGenerator::global()->bounded(randomLength));
        randomFileName.append(charPool.at(p));
    }

    return randomFileName.append(".fif");
}

//=============================================================================================================

QString SettingsControllerCl::generateDefaultOutputFileName()
{
    QString fileOut(QDir(m_fiInFile.absolutePath()).filePath(
                m_fiInFile.baseName() + "_anonymized." + m_fiInFile.completeSuffix()));
    m_fiOutFile.setFile(fileOut);
    return m_fiOutFile.absoluteFilePath();
}

//=============================================================================================================

QFileInfo SettingsControllerCl::getQFiInFile()
{
    return m_fiInFile;
}

//=============================================================================================================

QFileInfo SettingsControllerCl::getQFiOutFile()
{
    return m_fiOutFile;
}
