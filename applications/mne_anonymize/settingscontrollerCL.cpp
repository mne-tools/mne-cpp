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

SettingsControllerCL::SettingsControllerCL(const QStringList& arguments,
                                           const QString& name,
                                           const QString& ver)
: m_sAppName(name)
, m_sAppVer(ver)
, m_bVerboseMode(false)
//, m_bMultipleInFiles(false)
{
    m_pAnonymizer = FiffAnonymizer::SPtr(new FiffAnonymizer);
    initParser();
    if(parseInputs(arguments) == 0)
    {
        if(execute() != 0)
        {
            qCritical() << "Something went wrong during the anonymization.";
        }
    }
}

//=============================================================================================================

void SettingsControllerCL::initParser()
{
    m_parser.setApplicationDescription(QCoreApplication::translate("main", "Application that removes or modifies Personal "
                                                                           "Health Information or Personal Identifiable information from a FIFF file."));
    m_parser.addHelpOption();

    QCommandLineOption noGUIOpt("no-gui",QCoreApplication::translate("main","Command Line version of the application."));
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
    }

    if(m_parser.isSet("version"))
    {
        printVersionInfo();
        return 1;
    }

    if(m_parser.isSet("in"))
    {
        QFileInfo fInfo(m_parser.value("in"));
        if(fInfo.isFile())
        {
            if(m_pAnonymizer->setFileIn(fInfo.absoluteFilePath()))
            {
                qCritical() << "Error while setting the input file.";
            }
        } else {
            qCritical() << "Input file is not a file." << endl;
            return 1;
        }
    } else {
        qCritical() << "No valid input file specified." << endl;
        m_parser.showHelp();
    }

    if(m_parser.isSet("out"))
    {
        QFileInfo fInfo(m_parser.value("out"));
        fInfo.makeAbsolute();
        if(fInfo.isDir())
        {
            qCritical() << "Output file is infact a folder." << endl;
            return 1;
        } else {
            if(m_pAnonymizer->setFileOut(fInfo.fileName()))
            {
                qCritical() << "Error while setting the output file.";
            }
        }
    } else {
        QFileInfo inFInfo(m_parser.value("in"));
        QString fileOut(QDir(inFInfo.absolutePath()).filePath(
                    inFInfo.baseName() + "_anonymized." + inFInfo.completeSuffix()));
        if(m_pAnonymizer->setFileOut(fileOut))
        {
            qCritical() << "Error while setting the output file.";
        }
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
        m_pAnonymizer->setDeleteInputFileAfter(true);
    }

    if(m_parser.isSet("avoid_delete_confirmation"))
    {
        m_pAnonymizer->setDeleteInputFileConfirmation(false);
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

int SettingsControllerCL::execute()
{
        printHeaderIfVerbose();
        return m_pAnonymizer->anonymizeFile();
}

//=============================================================================================================

void SettingsControllerCL::printHeaderIfVerbose()
{
    if(m_bVerboseMode) {
        qInfo() << " ";
        qInfo() << "-------------------------------------------------------------------------------------------";
        qInfo() << " ";
        qInfo() << m_sAppName;
        qInfo() << "Version: " + m_sAppVer;
        qInfo() << " ";
    }
}

//=============================================================================================================

void SettingsControllerCL::printVersionInfo()
{
    qInfo() << " ";
    qInfo() << m_sAppName;
    qInfo() << "Version: " + m_sAppVer;
    qInfo() << " ";
}
