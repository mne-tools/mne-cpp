//=============================================================================================================
/**
* @file     connectivitysettings.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    ConnectivitySettings class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettings.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettings::ConnectivitySettings(const QStringList& arguments)
{
    parseArguments(arguments);
}


//*************************************************************************************************************

void ConnectivitySettings::parseArguments(const QStringList& arguments)
{
    // Command Line Parser
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption annotOption("annotType", "Annotation <type>.", "type", "aparc.a2009s");
    QCommandLineOption subjectOption("subj", "Selected <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization.", "doSourceLoc", "false");
    QCommandLineOption clustOption("doClust", "Path to clustered inverse operator.", "doClust", "true");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption connectMethodOption("connectMethod", "Connectivity <method>, i.e., 'COR', 'XCOR.", "method", "COR");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation.", "value", "3.0");//3.0f;//0.1f;//3.0f;
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "0");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "mag");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");

    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(fwdOption);
    parser.addOption(sourceLocOption);
    parser.addOption(clustOption);
    parser.addOption(covFileOption);
    parser.addOption(evokedFileOption);
    parser.addOption(connectMethodOption);
    parser.addOption(sourceLocMethodOption);
    parser.addOption(snrOption);
    parser.addOption(evokedIndexOption);
    parser.addOption(coilTypeOption);
    parser.addOption(chTypeOption);

    parser.process(arguments);

    // Init members from arguments
    m_sConnectivityMethod = parser.value(connectMethodOption);
    m_sAnnotType = parser.value(annotOption);
    m_sSubj = parser.value(subjectOption);
    m_sSubjDir = parser.value(subjectPathOption);
    m_sFwd = parser.value(fwdOption);
    m_sCov = parser.value(covFileOption);
    m_sSourceLocMethod = parser.value(sourceLocMethodOption);
    m_sMeas = parser.value(evokedFileOption);
    m_sCoilType = parser.value(coilTypeOption);
    m_sChType = parser.value(chTypeOption);

    if(parser.value(sourceLocOption) == "false" || parser.value(sourceLocOption) == "0") {
        m_bDoSourceLoc =false;
    } else if(parser.value(sourceLocOption) == "true" || parser.value(sourceLocOption) == "1") {
        m_bDoSourceLoc = true;
    }

    if(parser.value(clustOption) == "false" || parser.value(clustOption) == "0") {
        m_bDoClust =false;
    } else if(parser.value(clustOption) == "true" || parser.value(clustOption) == "1") {
        m_bDoClust = true;
    }

    m_dSnr = parser.value(snrOption).toDouble();
    m_iAveIdx = parser.value(evokedIndexOption).toInt();
}

