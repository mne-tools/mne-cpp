//=============================================================================================================
/**
 * @file     settingscontroller.cpp
 * @author   Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @version  dev
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
 * @brief    SettingsController class definition.
 *
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontroller.h"
#include "fiffanonymizer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsController::SettingsController(const QStringList& arguments,
                                       const QString& name,
                                       const QString& ver)
: m_sAppName(name)
, m_sAppVer(ver)
, m_bShowHeaderFlag(false)
, m_bMultipleInFiles(false)
{
    initParser();
    if(parseInputs(arguments)) {
        execute();
    }
}


//*************************************************************************************************************

SettingsController::~SettingsController()
{
    m_lApps.clear();
}


//*************************************************************************************************************

void SettingsController::initParser()
{
    m_parser.setApplicationDescription(QCoreApplication::translate("main",
                                                                   "Application that removes or modifies Personal Health Information or Personal Identifiable information from a FIFF file."));
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption inFileOpt("in",QCoreApplication::translate("main","File to anonymize. Multiple --in <infile> statements can be present."),
                                 QCoreApplication::translate("main","infile"));
    m_parser.addOption(inFileOpt);

    QCommandLineOption outFileOpt("out",QCoreApplication::translate("main","Output file <outfile>. Only allowed when anonymizing one single file."),
                                  QCoreApplication::translate("main","outfile"));
    m_parser.addOption(outFileOpt);

    QCommandLineOption verboseOpt("verbose",QCoreApplication::translate("main","Prints out more information, about each specific anonymized field. Only allowed when anonymizing one single file."));
    m_parser.addOption(verboseOpt);

    QCommandLineOption quietOpt("quiet",QCoreApplication::translate("main","Show no output."));
    m_parser.addOption(quietOpt);

    QCommandLineOption deleteInFileOpt("delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization. A confirmation message will be prompted to the user. Default: false"));
    m_parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt("avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file. Default: false"));
    m_parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt("brute",QCoreApplication::translate("main","Apart from anonymizing other more usual fields in the Fiff file, if present in the input fiff file, anonymize also Subject's weight and height, and Project's ID, Name, Aim and Comment."));
    m_parser.addOption(bruteOpt);

    QCommandLineOption measDateOpt("measurement_date",
                                   QCoreApplication::translate("main","Specify the measurement date. Only when anonymizing a single file. Format: YYYMMDD Default: 20000101"),
                                   QCoreApplication::translate("main","days"));
    m_parser.addOption(measDateOpt);

    QCommandLineOption measDateOffsetOpt("measurement_date_offset",
                                         QCoreApplication::translate("main","Specify number of days to subtract to the measurement <date>. Only allowed when anonymizing a single file. Default: 0"),
                                         QCoreApplication::translate("main","date"));
    m_parser.addOption(measDateOffsetOpt);

    QCommandLineOption birthdayOpt("subject_birthday",
                                   QCoreApplication::translate("main","Specify the subject's birthday <date>. Only allowed when anonymizing a single file. Format: YYYMMDD Default: 20000101"),
                                   QCoreApplication::translate("main","date"));
    m_parser.addOption(birthdayOpt);

    QCommandLineOption birthdayOffsetOpt("subject_birthday_offset",
                                         QCoreApplication::translate("main","Specify number of <days> to subtract to the subject's birthday. Only allowed when anonymizing a single file. Default: 0"),
                                         QCoreApplication::translate("main","days"));
    m_parser.addOption(birthdayOffsetOpt);

    QCommandLineOption SubjectIdOpt("his",QCoreApplication::translate("main","Specify the Subject's Id# within the Hospital system. Only allowed when anonymizing a single file. Default: mne_anonymize"),
                                          QCoreApplication::translate("main","id#"));
    m_parser.addOption(SubjectIdOpt);
}


//*************************************************************************************************************

bool SettingsController::parseInputs(const QStringList& arguments)
{
    m_parser.process(arguments);

    if(!parseInputAndOutputFiles()) {

        return false;
    }

    if(m_parser.isSet("verbose")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Verbose does not work with multiple Input files.";
            return false;
        }
        m_bShowHeaderFlag=true;
        m_anonymizer.setVerboseMode(true);
    }

    if(m_parser.isSet("brute")) {
        m_anonymizer.setBruteMode(true);
    }

    if(m_parser.isSet("quiet")) {
        if(m_anonymizer.getVerboseMode()) {
            m_anonymizer.setVerboseMode(false);
        }
        m_anonymizer.setQuietMode(true);
    }

    if(m_parser.isSet("delete_input_file_after")) {
        m_anonymizer.setDeleteInputFileAfter(true);
    }

    if(m_parser.isSet("avoid_delete_confirmation")) {
        m_anonymizer.setDeleteInputFileAfterConfirmation(false);
    }

    if(m_parser.isSet("measurement_date")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Multiple Input files. You cannot specify the option measurement_date.";
            return false;
        }

        QString d(m_parser.value("measurement_date"));
        m_anonymizer.setMeasurementDay(d);
    }

    if(m_parser.isSet("measurement_date_offset")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Multiple Input files. You cannot specify the option measurement_date_offset.";
            return false;
        }

        QString doffset(m_parser.value("measurement_date_offset"));
        m_anonymizer.setMeasurementDayOffset(doffset.toInt());
    }

    if(m_parser.isSet("subject_birthday")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Multiple Input files. You cannot specify the option \"subject_birthday\".";
            return false;
        }

        QString birthday(m_parser.value("subject_birthday"));
        m_anonymizer.setSubjectBirthday(birthday);
    }

    if(m_parser.isSet("subject_birthday_offset")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Multiple Input files. You cannot specify the option \"subject_birthday_offset\".";
            return false;
        }
        QString bdoffset(m_parser.value("subject_birthday_offset"));
        m_anonymizer.setSubjectBirthdayOffset(bdoffset.toInt());
    }

    if(m_parser.isSet("his")) {
        if(m_bMultipleInFiles) {
            qCritical() << "Multiple Input files. You cannot specify the option \"his\".";
            return false;
        }

        m_anonymizer.setSubjectHisId(m_parser.value("his"));
    }

    return true;
}


//*************************************************************************************************************

bool SettingsController::parseInputAndOutputFiles()
{
    QStringList inFilesAux;
    if(m_parser.isSet("in")) {
        for(QString f: m_parser.values("in")) {
            inFilesAux.append(f);
        }
    }

    for(QString f: inFilesAux) {
        m_SLInFiles.append(listFilesMatchingPatternName(f));
    }

    if(m_SLInFiles.count() == 0) {
        qCritical() << "No valid input files specified.";
        return false;
    } else if(m_SLInFiles.count() == 1) {
        m_bMultipleInFiles = false;
    } else {
        m_bMultipleInFiles = true;
    }

    QString boolMultiStr(QVariant(m_bMultipleInFiles).toString());
    QString countFilesStr(QVariant(m_SLInFiles.count()).toString());

    if(m_bMultipleInFiles) {
        if(m_parser.isSet("out")) {
            qWarning() << "Multiple input files selected. Output filename option will be ignored.";
        }

        for(QString fi:m_SLInFiles) {
            QFileInfo fInfo(fi);
            QString fout = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
            m_SLOutFiles.append(fout);
        }
    } else {
        QString fileOutName;

        if(m_parser.isSet("out")) {
            QFileInfo fInfo(m_parser.value("out"));
            fInfo.makeAbsolute();
            fileOutName = fInfo.absoluteFilePath();
        } else {
            QFileInfo fInfo(m_SLInFiles.first());
            fileOutName = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
        }

        m_SLOutFiles.append(fileOutName);
    }

    if(m_SLInFiles.size() != m_SLOutFiles.size()) {
        qCritical() << "Something went wrong while parsing the input files.";
        return false;
    }

    qInfo() << QString("%1 files to anonymize:").arg(m_SLInFiles.count());
    for(int i = 0; i < m_SLInFiles.size(); ++i) {
        qInfo() << "Anonymize" << m_SLInFiles.at(i) << "->" << m_SLOutFiles.at(i);
    }

    return true;
}


//*************************************************************************************************************

void SettingsController::generateAnonymizerInstances()
{
    if(m_SLInFiles.isEmpty() || m_SLOutFiles.isEmpty()) {
        qCritical() << "No input and/or output file names specified.";
        return;
    }

    if(m_bMultipleInFiles) {
        for(int i = 0; i < m_SLInFiles.size(); ++i) {
            m_lApps.append(m_anonymizer);
            m_lApps[i].setFileIn(m_SLInFiles.at(i));
            m_lApps[i].setFileOut(m_SLOutFiles.at(i));
        }
    } else {
        m_anonymizer.setFileIn(m_SLInFiles.first());
        m_anonymizer.setFileOut(m_SLOutFiles.first());
    }
}


//*************************************************************************************************************

void SettingsController::execute()
{
    generateAnonymizerInstances();

    if(m_bMultipleInFiles) {
        QtConcurrent::blockingMap(m_lApps, &FiffAnonymizer::anonymizeFile);
    } else {
        printHeaderIfVerbose();
        m_anonymizer.anonymizeFile();
    }
}


//*************************************************************************************************************

void SettingsController::printHeaderIfVerbose()
{
    if(m_bShowHeaderFlag) {
        qInfo() << " ";
        qInfo() << "-------------------------------------------------------------------------------------------";
        qInfo() << " ";
        qInfo() << m_sAppName;
        qInfo() << "Version: " + m_sAppVer;
    }
}
