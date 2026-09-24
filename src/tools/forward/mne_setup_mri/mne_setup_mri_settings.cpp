//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_setup_mri_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNESetupMriSettings class definition.
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

MNESetupMriSettings::MNESetupMriSettings(int *argc, char **argv)
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

QString MNESetupMriSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MNESetupMriSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

QStringList MNESetupMriSettings::mriSets() const
{
    return m_slMriSets;
}

//=============================================================================================================

bool MNESetupMriSettings::overwrite() const
{
    return m_bOverwrite;
}

//=============================================================================================================

bool MNESetupMriSettings::verbose() const
{
    return m_bVerbose;
}
