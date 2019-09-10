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

//SettingsController::SettingsController()
//{

//}

SettingsController::SettingsController(QCoreApplication* qtApp)
: m_bMultipleInFiles(false)
, m_pQCoreApp(qtApp)
{
    m_pQCoreApp->setApplicationName(m_anonymizer.name);
    m_pQCoreApp->setApplicationVersion(m_anonymizer.versionStr);

    initParser();
    parseInputs();

    generateAnonymizerInstances();
    execute();
}


//*************************************************************************************************************

void SettingsController::initParser()
{
    m_parser.setApplicationDescription(m_anonymizer.description);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption verboseOpt(QStringList() << "v" << "verbose",
                                  QCoreApplication::translate("main","Prints out more information."));
    m_parser.addOption(verboseOpt);

    QCommandLineOption quietOpt("quiet",QCoreApplication::translate("main","Show no output."));
    m_parser.addOption(quietOpt);

    QCommandLineOption deleteInFileOpt("delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization."));
    m_parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt("avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file."));
    m_parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt("brute",
                                QCoreApplication::translate("main","Anonymize weight, height XXX if present in the input fiff file."));
    m_parser.addOption(bruteOpt);

    QCommandLineOption inFileOpt("in",QCoreApplication::translate("main","File to anonymize. Wildcards are allowed and seveal -in <infile> statements can be present."),
                                 QCoreApplication::translate("main","infile"));
    m_parser.addOption(inFileOpt);

    QCommandLineOption outFileOpt("out",QCoreApplication::translate("main","Output file <outfile>. Only allowed when anonymizing one single file."),
                                  QCoreApplication::translate("main","outfile"));
    m_parser.addOption(outFileOpt);


    QCommandLineOption measDateOpt("measurement_date",
                                   QCoreApplication::translate("main","Specify the measurement date. Only when anonymizing a single file. Format: YYYMMDD "),
                                   QCoreApplication::translate("main","days"));
    m_parser.addOption(measDateOpt);

    QCommandLineOption measDateOffsetOpt("measurement_date_offset",
                                         QCoreApplication::translate("main","Specify number of days to subtract to the measurement <date>. Only allowed when anonymizing a single file."),
                                         QCoreApplication::translate("main","date"));
    m_parser.addOption(measDateOffsetOpt);

    QCommandLineOption birthdayOpt("subject_birthday",
                                   QCoreApplication::translate("main","Specify the subject's birthday <date>. Only allowed when anonymizing a single file. Format: YYYMMDD "),
                                   QCoreApplication::translate("main","date"));
    m_parser.addOption(birthdayOpt);

    QCommandLineOption birthdayOffsetOpt("subject_birthday_offset",
                                         QCoreApplication::translate("main","Specify number of <days> to subtract to the subject's birthday. Only allowed when anonymizing a single file. "),
                                         QCoreApplication::translate("main","days"));
    m_parser.addOption(birthdayOffsetOpt);

}


//*************************************************************************************************************

void SettingsController::parseInputs()
{
    m_parser.process(*m_pQCoreApp);

    parseInputAndOutputFiles();

    if(m_parser.isSet("v") || m_parser.isSet("verbose"))
    {
        m_anonymizer.setVerboseMode(true);
    }

    if(m_parser.isSet("brute"))
    {
        m_anonymizer.setBruteMode(true);
    }

    if(m_parser.isSet("quiet"))
    {
        if(m_anonymizer.getVerboseMode())
        {
            m_anonymizer.setVerboseMode(false);
        }
        m_anonymizer.setQuietMode(true);
    }

    if(m_parser.isSet("delete_input_file_after"))
    {
        m_anonymizer.setDeleteInputFileAfter(true);
    }

    if(m_parser.isSet("avoid_delete_confirmation"))
    {
        m_anonymizer.setDeleteInputFileAfterConfirmation(false);
    }

    if(m_parser.isSet("measurement_date"))
    {
        if(m_bMultipleInFiles)
        {
            qDebug() << "Error. Multiple Input files. You cannot specify the option \"measurement_date\".";
            m_parser.showHelp();
        }
        QString d(m_parser.value("measurement_date"));
        m_anonymizer.setMeasurementDay(d);
    }

    if(m_parser.isSet("measurement_date_offset"))
    {
        if(m_bMultipleInFiles)
        {
            qDebug() << "Error. Multiple Input files. You cannot specify the option \"measurement_date_offset\".";
            m_parser.showHelp();
        }
        QString doffset(m_parser.value("measurement_date_offset"));
        m_anonymizer.setMeasurementDayOffset(doffset.toInt());
    }

    if(m_parser.isSet("subject_birthday"))
    {
        if(m_bMultipleInFiles)
        {
            qDebug() << "Error. Multiple Input files. You cannot specify the option \"subject_birthday\".";
            m_parser.showHelp();
        }
        QString birthday(m_parser.value("subject_birthday"));
        m_anonymizer.setSubjectBirthday(birthday);
    }

    if(m_parser.isSet("subject_birthday_offset"))
    {
        if(m_bMultipleInFiles)
        {
            qDebug() << "Error. Multiple Input files. You cannot specify the option \"subject_birthday_offset\".";
            m_parser.showHelp();
        }
        QString bdoffset(m_parser.value("subject_birthday_offset"));
        m_anonymizer.setSubjectBirthdayOffset(bdoffset.toInt());
    }
}


//*************************************************************************************************************

void SettingsController::parseInputAndOutputFiles()
{
    if(m_parser.isSet("in"))
    {
        QStringList inFilesAux(m_parser.values("in"));
        for(QString f: inFilesAux)
        {
            QDir d;
            d.setNameFilters({f});
            for(QFileInfo fi: d.entryInfoList())
            {
                if(fi.isFile() && fi.isReadable())
                {
                    fi.makeAbsolute();
                    m_slInFiles.append(fi.fileName());
                }
            }
        }
    }

    if(m_slInFiles.size() == 0)
    {
        qDebug() << "Error. no valid input files. OMG!!";
        m_parser.showHelp();
    } else if(m_slInFiles.size() == 1)
    {
        m_bMultipleInFiles = false;
    } else {
        m_bMultipleInFiles = true;
    }

    if(m_bMultipleInFiles)
    {
        //        QStringList opts;
        //        opts << "out" << "measurement_date" << "measurement_date_offset" << "subject_birthday" << "subject_birthday_offset";
        //        for(QString opi:opts)
        //        {
        //            if(m_parser.isSet(opi))
        //            {
        //                qDebug() << "Error. Multiple Input files. You cannot specify the option " << opi;
        //                m_parser.showHelp();
        //            }
        //        }

        for(QString fi:m_slInFiles)
        {
            QFileInfo fInfo(fi);
            QString fout = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
            m_slOutFiles.append(fout);
        }
    } else {
        QString fileOutName;
        if(m_parser.isSet("out"))
        {
            fileOutName = m_parser.value("out");
        } else {
            QFileInfo fInfo(m_slInFiles.at(0));
            fileOutName = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
        }
    }

    if(m_slInFiles.size() != m_slOutFiles.size())
    {
        qDebug() << "Error. something went wrong while parsing the input files.";
    }
}


//*************************************************************************************************************

void SettingsController::generateAnonymizerInstances()
{
    if(m_slInFiles.isEmpty() || m_slOutFiles.isEmpty()) {
        qDebug() << "SettingsController::generateAnonymizerInstances - No input and/or output file names specified.";
        return;
    }

    if(m_bMultipleInFiles)
    {
        for(int i=0; i< m_slInFiles.size(); ++i)
        {
            //generate copy of m_anonymizer --> need a copy constructor
            FiffAnonymizer anonymizerApp;
            anonymizerApp.setFileIn(m_slInFiles.at(i));
            anonymizerApp.setFileOut(m_slOutFiles.at(i));

            m_appList.append(anonymizerApp);
        }
    } else {
        m_anonymizer.setFileIn(m_slInFiles.first());
        m_anonymizer.setFileOut(m_slOutFiles.first());
    }
}


//*************************************************************************************************************

void SettingsController::execute()
{
    if(m_bMultipleInFiles)
    {
        QFuture<void> future = QtConcurrent::map(m_appList, &FiffAnonymizer::anonymizeFile);
        future.waitForFinished();
    } else {
        m_anonymizer.anonymizeFile();
    }
}


//*************************************************************************************************************

