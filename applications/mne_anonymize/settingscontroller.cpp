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

SettingsController::SettingsController(QCoreApplication* qtApp)
: m_bMultipleInFiles(false)
, m_pQCoreApp(qtApp)
{
    initParser();
    parseInputs();
    execute();
}


//*************************************************************************************************************

SettingsController::~SettingsController()
{
    //qDeleteAll(m_pAppList.begin(),m_pAppList.end());
    m_pAppList.clear();
    //m_pQCoreApp is managed by main. no leak. Leave it alone.
}


//*************************************************************************************************************

void SettingsController::initParser()
{
    m_parser.setApplicationDescription(m_anonymizer.description);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption inFileOpt("in",QCoreApplication::translate("main","File to anonymize. Wildcards are allowed and several --in <infile> statements can be present."),
                                 QCoreApplication::translate("main","infile"));
    m_parser.addOption(inFileOpt);

    QCommandLineOption outFileOpt("out",QCoreApplication::translate("main","Output file <outfile>. Only allowed when anonymizing one single file."),
                                  QCoreApplication::translate("main","outfile"));
    m_parser.addOption(outFileOpt);

    QCommandLineOption verboseOpt("verbose",QCoreApplication::translate("main","Prints out more information."));
    m_parser.addOption(verboseOpt);

    QCommandLineOption quietOpt("quiet",QCoreApplication::translate("main","Show no output."));
    m_parser.addOption(quietOpt);

    QCommandLineOption deleteInFileOpt("delete_input_file_after",
                                       QCoreApplication::translate("main","Delete input fiff file after anonymization."));
    m_parser.addOption(deleteInFileOpt);

    QCommandLineOption deleteInFileConfirmOpt("avoid_delete_confirmation",
                                              QCoreApplication::translate("main","Avoid confirming the deletion of the input fiff file."));
    m_parser.addOption(deleteInFileConfirmOpt);

    QCommandLineOption bruteOpt("brute",QCoreApplication::translate("main","Anonymize weight, height XXX if present in the input fiff file."));
    m_parser.addOption(bruteOpt);

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

    //check if this is needed TODO
//    if(m_parser.isSet("v") || m_parser.isSet("verbose"))
    if(m_parser.isSet("verbose"))
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
                    m_SLInFiles.append(fi.absoluteFilePath());
                }
            }
        }
    }

    if(m_SLInFiles.size() == 0)
    {
        qDebug() << "Error. no valid input files. OMG!!";
        m_parser.showHelp();
    } else if(m_SLInFiles.size() == 1)
    {
        m_bMultipleInFiles = false;
    } else {
        m_bMultipleInFiles = true;
    }

    if(m_bMultipleInFiles)
    {
        if(m_parser.isSet("out"))
        {
            qDebug() << "Warning. Multiple input files selected. Output filename option will be ignored.";
        }
        for(QString fi:m_SLInFiles)
        {
            QFileInfo fInfo(fi);
            QString fout = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
            m_SLOutFiles.append(fout);
        }
    } else {
        QString fileOutName;
        if(m_parser.isSet("out"))
        {
            QFileInfo fInfo(m_parser.value("out"));
            fInfo.makeAbsolute();
            fileOutName = fInfo.fileName();
        } else {
            QFileInfo fInfo(m_SLInFiles.first());
            fileOutName = QDir(fInfo.absolutePath()).filePath(
                        fInfo.baseName() + "_anonymized." + fInfo.completeSuffix());
        }
        m_SLOutFiles.append(fileOutName);
    }

    if(m_SLInFiles.size() != m_SLOutFiles.size())
    {
        qDebug() << "Error. something went wrong while parsing the input files.";
    }

}


//*************************************************************************************************************

void SettingsController::generateAnonymizerInstances()
{
    if(m_SLInFiles.isEmpty() || m_SLOutFiles.isEmpty()) {
        qDebug() << "SettingsController::generateAnonymizerInstances - No input and/or output file names specified.";
        return;
    }

    if(m_bMultipleInFiles)
    {
        for(int i=0; i< m_SLInFiles.size(); ++i)
        {
            QSharedPointer<FiffAnonymizer> pAppAux(new FiffAnonymizer(m_anonymizer));
            pAppAux->setFileIn(m_SLInFiles.at(i));
            pAppAux->setFileOut(m_SLOutFiles.at(i));
            //m_pAppList.append(QSharedPointer<FiffAnonymizer>(pAppAux));
            //note that QList will copy & append.
            m_pAppList.append(pAppAux);
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
    if(m_bMultipleInFiles)
    {
        for(int th_i=0; th_i<m_pAppList.size(); ++th_i)
        {
            QSharedPointer<QFuture<void> > promise( new QFuture<void>);
            FiffAnonymizer localApp(*m_pAppList.at(th_i));
            *promise = QtConcurrent::run(localApp,&FiffAnonymizer::anonymizeFile);
            promisesList.append(promise);

        }
        for(int p_i=0;p_i<promisesList.size();++p_i)
        {
            promisesList.at(p_i)->waitForFinished();
        }
    } else {
        m_anonymizer.anonymizeFile();
    }
}


//*************************************************************************************************************


