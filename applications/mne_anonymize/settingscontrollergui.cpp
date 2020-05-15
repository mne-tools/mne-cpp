//=============================================================================================================
/**
 * @file     settingscontrollergui.cpp
 * @author   Juan GPC <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Juan GPC. All rights reserved.
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
 * @brief    SettingsControllerGUI class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontrollergui.h"
#include "mainwindow.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

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

SettingsControllerGui::SettingsControllerGui(const QStringList& arguments)
: m_pWindow(QSharedPointer<MainWindow> (new MainWindow(this)))
{
    Q_UNUSED(arguments)

    initParser();

    parseInputs(arguments);

    m_pWindow->show();
}

void SettingsControllerGui::parseInputs(const QStringList& arguments)
{
    m_parser.process(arguments);

    //parse in and out files
    if(m_parser.isSet("in"))
    {
        m_fiInFileInfo.setFile(m_parser.value("in"));
        if(m_fiInFileInfo.isFile())
        {
            if(m_pAnonymizer->setFileIn(m_fiInFileInfo.absoluteFilePath()))
            {
                qWarning() << "Error while setting the input file.";
            }
            m_pWindow->setLineEditInFile(m_fiInFileInfo.absoluteFilePath());
        } else {
            qWarning() << "Input file is not a file.";
        }
    }

    if(m_parser.isSet("out"))
    {
        m_fiOutFileInfo.setFile(m_parser.value("out"));
        if(m_fiOutFileInfo.isDir())
        {
            qWarning() << "Error. Output file is infact a folder.";
        }
    } else {
        QString fileOut(QDir(m_fiInFileInfo.absolutePath()).filePath(
                    m_fiInFileInfo.baseName() + "_anonymized." + m_fiInFileInfo.completeSuffix()));
        m_fiOutFileInfo.setFile(fileOut);
    }
    if(m_pAnonymizer->setFileOut(m_fiOutFileInfo.absoluteFilePath()))
    {
        qWarning() << "Error while setting the output file.";
    }
    m_pWindow->setLineEditOutFile(m_fiOutFileInfo.absoluteFilePath());


    if(m_parser.isSet("brute"))
    {
        m_pAnonymizer->setBruteMode(true);
        m_pWindow->setCheckBoxBruteMode(true);
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
        m_pAnonymizer->setSubjectBirthdayOffset(strBirthdayOffset);
    }

    if(m_parser.isSet("his"))
    {
        QString strHisId(m_parser.value("his"));
        m_pAnonymizer->setSubjectHisId(strHisId);
    }










}
