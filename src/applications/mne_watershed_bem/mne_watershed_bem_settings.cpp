//=============================================================================================================
/**
 * @file     mne_watershed_bem_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MneWatershedBemSettings class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_watershed_bem_settings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEWATERSHEDBEM;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneWatershedBemSettings::MneWatershedBemSettings(int *argc, char **argv)
: m_sVolume("T1")
, m_bOverwrite(false)
, m_bAtlas(false)
, m_bGcaAtlas(false)
, m_iPreflood(-1)
, m_bVerbose(false)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Create BEM surfaces using the watershed algorithm included with FreeSurfer.\n\n"
        "This tool invokes FreeSurfer's mri_watershed to segment brain, skull, and\n"
        "skin surfaces from an MRI volume, then creates a FIFF BEM head surface file.\n\n"
        "Ported from the original MNE shell script mne_watershed_bem by Matti Hamalainen\n"
        "(SVN $Id: mne_watershed_bem 3391 2012-11-30 21:13:09Z msh $).\n\n"
        "Cross-referenced with MNE-Python's mne.bem.make_watershed_bem()."
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

    // --volume
    QCommandLineOption volumeOpt(QStringList() << "volume",
        "MRI volume name (default: T1).",
        "name", "T1");
    parser.addOption(volumeOpt);

    // --overwrite
    QCommandLineOption overwriteOpt(QStringList() << "overwrite",
        "Overwrite existing watershed files.");
    parser.addOption(overwriteOpt);

    // --atlas
    QCommandLineOption atlasOpt(QStringList() << "atlas",
        "Specify the --atlas option for mri_watershed.");
    parser.addOption(atlasOpt);

    // --gcaatlas
    QCommandLineOption gcaAtlasOpt(QStringList() << "gcaatlas",
        "Use the subcortical atlas for mri_watershed.");
    parser.addOption(gcaAtlasOpt);

    // --preflood
    QCommandLineOption prefloodOpt(QStringList() << "preflood",
        "Change the preflood height for mri_watershed.",
        "number");
    parser.addOption(prefloodOpt);

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

    // FreeSurfer home
    m_sFreeSurferHome = QProcessEnvironment::systemEnvironment().value("FREESURFER_HOME", "");

    // Volume
    m_sVolume = parser.value(volumeOpt);

    // Overwrite
    m_bOverwrite = parser.isSet(overwriteOpt);

    // Atlas
    m_bAtlas = parser.isSet(atlasOpt);

    // GCA Atlas
    m_bGcaAtlas = parser.isSet(gcaAtlasOpt);

    // Preflood
    if (parser.isSet(prefloodOpt)) {
        bool ok;
        m_iPreflood = parser.value(prefloodOpt).toInt(&ok);
        if (!ok) {
            qWarning() << "Invalid preflood value:" << parser.value(prefloodOpt);
            m_iPreflood = -1;
        }
    }

    // Verbose
    m_bVerbose = parser.isSet(verboseOpt);
}

//=============================================================================================================

QString MneWatershedBemSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MneWatershedBemSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

QString MneWatershedBemSettings::freeSurferHome() const
{
    return m_sFreeSurferHome;
}

//=============================================================================================================

QString MneWatershedBemSettings::volume() const
{
    return m_sVolume;
}

//=============================================================================================================

bool MneWatershedBemSettings::overwrite() const
{
    return m_bOverwrite;
}

//=============================================================================================================

bool MneWatershedBemSettings::atlas() const
{
    return m_bAtlas;
}

//=============================================================================================================

bool MneWatershedBemSettings::gcaAtlas() const
{
    return m_bGcaAtlas;
}

//=============================================================================================================

int MneWatershedBemSettings::preflood() const
{
    return m_iPreflood;
}

//=============================================================================================================

bool MneWatershedBemSettings::verbose() const
{
    return m_bVerbose;
}
