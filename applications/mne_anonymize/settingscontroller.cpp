//=============================================================================================================
/**
* @file     settingscontroller.cpp
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
// INCLUDES
//=============================================================================================================


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

using namespace FIFFANONYMIZER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsController::SettingsController()
{

}

SettingsController::SettingsController(QCoreApplication * qtApp)
{

    FIFFANONYMIZER::FiffAnonymizer app;

    QCoreApplication::setApplicationName(app.name);
    QCoreApplication::setApplicationName(app.versionStr);

    QCommandLineParser parser;

    parser.setApplicationDescription(app.description);
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("in-file",QCoreApplication::translate("main","File to anonymize <infile>"),"infile");

    QCommandLineOption outFileOpt(QStringList() << "o" << "output_file",
                                  QCoreApplication::translate("main","Output file <outfile>."),
                                  QCoreApplication::translate("main","outfile"),
                                  QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/testing_output.fif");
    parser.addOption(outFileOpt);

    QCommandLineOption verboseOpt(QStringList() << "v" << "verbose",
                                  QCoreApplication::translate("main","Verbose mode. Prints out more information."));
    parser.addOption(verboseOpt);

    QCommandLineOption measDateOpt("set_measurement_date",
                                   QCoreApplication::translate("main","Specify the measurement date. Format: YYYMMDD "),
                                   QCoreApplication::translate("main","days"));
    parser.addOption(measDateOpt);

    QCommandLineOption measDateOffsetOpt("set_measurement_date_offset",
                                   QCoreApplication::translate("main","Specify number of days to subtract to the measurement date."),
                                   QCoreApplication::translate("main","date"));
    parser.addOption(measDateOffsetOpt);

    QCommandLineOption birthdayOpt("set_subject_birthday",
                                   QCoreApplication::translate("main","Specify the subject's birthday. Format: YYYMMDD "),
                                   QCoreApplication::translate("main","date"));
    parser.addOption(birthdayOpt);

    QCommandLineOption birthdayOffsetOpt("set_subject_birthday_offset",
                                   QCoreApplication::translate("main","Specify number of days to subtract to the subject's birthday."),
                                   QCoreApplication::translate("main","days"));
    parser.addOption(birthdayOffsetOpt);

    QCommandLineOption deleteInFileOpt(QStringList() << "d" << "delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization."));
    parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt("avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file."));
    parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt(
                "brute",QCoreApplication::translate("main","Anonymize weight, height XXX if present in the input fiff file."));
    parser.addOption(bruteOpt);

    QCommandLineOption quietOpt("quiet",QCoreApplication::translate("main","Show no output."));
    parser.addOption(quietOpt);

    parser.process(*qtApp);

    const QStringList args = parser.positionalArguments();
    QString fileInName(args.at(0));
    QFile fileIn(fileInName);
    app.setFileIn(&fileIn);

    QString fileOutName;
    if(parser.isSet(outFileOpt))
    {
        fileOutName = parser.value(outFileOpt);
    } else {
        QFileInfo fInfo(fileIn.fileName());
        fileOutName = QDir(fInfo.absolutePath()).filePath(
                    fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
    }
    QFile fileOut(fileOutName);
    app.setFileOut(&fileOut);

    if(parser.isSet(verboseOpt))
    {
        app.setVerboseMode(true);
    }

    if(parser.isSet(bruteOpt))
    {
        app.setBruteMode(true);
    }

    if(parser.isSet(quietOpt))
    {
        if(app.getVerboseMode())
        {
            app.setVerboseMode(false);
        }
        app.setQuietMode(true);
    }

    if(parser.isSet(measDateOffsetOpt))
    {
        QString d(parser.value(measDateOffsetOpt));
        app.setMeasurementDayOffset(d.toInt());
    }

    if(parser.isSet(birthdayOffsetOpt))
    {
        QString d(parser.value(birthdayOffsetOpt));
        app.setSubjectBirthdayOffset(d.toInt());
    }

    if(parser.isSet(measDateOpt))
    {
        app.setMeasurementDay(parser.value(measDateOpt));
    }

    if(parser.isSet(birthdayOpt))
    {
        app.setSubjectBirthday(parser.value(birthdayOpt));
    }

    app.anonymizeFile();

}

//*************************************************************************************************************
