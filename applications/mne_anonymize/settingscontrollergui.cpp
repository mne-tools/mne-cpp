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

    //initialize options in window
    QFileInfo fileInfoIn(m_pAnonymizer->getFileNameIn());
    if (fileInfoIn.isFile())
    {
        m_pWindow->setLineEditInFile(fileInfoIn.absoluteFilePath());
    }

    QFileInfo fileInfoOut(m_pAnonymizer->getFileNameOut());
    if (fileInfoOut.isFile())
    {
        m_pWindow->setLineEditOutFile(fileInfoOut.absoluteFilePath());
    }

    m_pWindow->setCheckBoxBruteMode(m_pAnonymizer->getBruteMode());
    m_pWindow->setCheckBoxDeleteInputFileAfter(m_bDeleteInputFileAfter);
    m_pWindow->setCheckBoxAvoidDeleteConfirmation(m_bDeleteInputFileConfirmation);
    m_pWindow->setMeasurementDate(m_pAnonymizer->getMeasurementDate());
    m_pWindow->setCheckBoxMeasurementDateOffset(m_pAnonymizer->getUseMeasurementDayOffset());
    m_pWindow->setMeasurementDateOffset(m_pAnonymizer->getMeasurementDayOffset());
    m_pWindow->setCheckBoxSubjectBirthdayOffset(m_pAnonymizer->getUseSubjectBirthdayOffset());
    m_pWindow->setSubjectBirthdayOffset(m_pAnonymizer->getSubjectBirthdayOffset());
    m_pWindow->setSubjectHis(m_pAnonymizer->getSubjectHisID());

    //setup communication





//    read data method
//    anonymize method

    m_pWindow->show();
}

