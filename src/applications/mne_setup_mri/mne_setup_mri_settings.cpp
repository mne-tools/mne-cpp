//=============================================================================================================
/**
 * @file     mne_setup_mri_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    MneSetupMriSettings class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_setup_mri_settings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QProcessEnvironment>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSetupMriSettings::MneSetupMriSettings(int *argc, char **argv)
: m_bOverwrite(false)
, m_bVerbose(false)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Set up FreeSurfer MRI data for use with MNE software.\n\n"
        "This tool creates the Neuromag-compatible directory structure and\n"
        "converts FreeSurfer MRI volumes (COR files or .mgz) into COR.fif\n"
        "FIFF files, following the original MNE C tool by Matti Hamalainen.\n\n"
        "Ported from SVN MNE mne_setup_mri / mne_make_cor_set."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    // --subject
    QCommandLineOption subjectOpt(QStringList() << "subject",
        "Subject name (defaults to $SUBJECT environment variable).",
        "subject");
    parser.addOption(subjectOpt);

    // --subjects-dir
    QCommandLineOption subjectsDirOpt(QStringList() << "subjects-dir",
        "Subjects directory (defaults to $SUBJECTS_DIR environment variable).",
        "dir");
    parser.addOption(subjectsDirOpt);

    // --mri
    QCommandLineOption mriOpt(QStringList() << "mri",
        "MRI set name to process (can be specified multiple times, default: T1 brain).",
        "name");
    parser.addOption(mriOpt);

    // --overwrite
    QCommandLineOption overwriteOpt(QStringList() << "overwrite",
        "Overwrite existing data.");
    parser.addOption(overwriteOpt);

    // --verbose
    QCommandLineOption verboseOpt(QStringList() << "verbose",
        "Enable verbose output.");
    parser.addOption(verboseOpt);

    // Build argument list from argc/argv
    QStringList args;
    for (int i = 0; i < *argc; ++i) {
        args << QString(argv[i]);
    }

    parser.process(args);

    // Subject
    if (parser.isSet(subjectOpt)) {
        m_sSubject = parser.value(subjectOpt);
    } else {
        m_sSubject = QProcessEnvironment::systemEnvironment().value("SUBJECT", "");
    }

    // Subjects dir
    if (parser.isSet(subjectsDirOpt)) {
        m_sSubjectsDir = parser.value(subjectsDirOpt);
    } else {
        m_sSubjectsDir = QProcessEnvironment::systemEnvironment().value("SUBJECTS_DIR", "");
    }

    // MRI sets
    if (parser.isSet(mriOpt)) {
        m_slMriSets = parser.values(mriOpt);
    } else {
        m_slMriSets << "T1" << "brain";
    }

    // Overwrite
    m_bOverwrite = parser.isSet(overwriteOpt);

    // Verbose
    m_bVerbose = parser.isSet(verboseOpt);
}

//=============================================================================================================

QString MneSetupMriSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MneSetupMriSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

QStringList MneSetupMriSettings::mriSets() const
{
    return m_slMriSets;
}

//=============================================================================================================

bool MneSetupMriSettings::overwrite() const
{
    return m_bOverwrite;
}

//=============================================================================================================

bool MneSetupMriSettings::verbose() const
{
    return m_bVerbose;
}
