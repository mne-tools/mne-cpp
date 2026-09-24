//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_flash_bem_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNEFlashBemSettings class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_flash_bem_settings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEFLASHBEM;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEFlashBemSettings::MNEFlashBemSettings(int *argc, char **argv)
: m_bNoConvert(false)
, m_bNoFlash30(false)
{
    //
    // Read environment variables
    //
    m_sFreeSurferHome = qEnvironmentVariable("FREESURFER_HOME");
    m_sSubjectsDir = qEnvironmentVariable("SUBJECTS_DIR");
    m_sSubject = qEnvironmentVariable("SUBJECT");

    // Default flash data directory is cwd
    m_sFlashDir = QDir::currentPath();

    //
    // Parse command-line arguments using QCommandLineParser
    //
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Create BEM meshes using multi-echo FLASH MRI data.\n\n"
        "Ported from the original MNE shell script mne_flash_bem by Matti Hamalainen\n"
        "(SVN $Id: mne_flash_bem 3255 2010-11-15 18:34:59Z msh $).\n\n"
        "Cross-referenced with MNE-Python's mne.bem.make_flash_bem().\n\n"
        "Before running:\n"
        "  1. Set FREESURFER_HOME, SUBJECTS_DIR, and SUBJECT environment variables\n"
        "  2. Create flash05/ (and optionally flash30/) directories with echo subdirectories\n"
        "  3. Run from the directory containing the flash data, or use --flash-dir"
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noFlash30Option(
        "noflash30",
        "Only 5-degree flash angle data is available. Flash-5 echoes will be averaged "
        "instead of using parameter maps from both 5 and 30 degree data."
    );
    parser.addOption(noFlash30Option);

    QCommandLineOption noConvertOption(
        "noconvert",
        "Assume that the DICOM images have already been converted to MGZ format. "
        "Skip the mri_convert step."
    );
    parser.addOption(noConvertOption);

    QCommandLineOption unwarpOption(
        "unwarp",
        "Apply gradient distortion unwarping using grad_unwarp with this option.",
        "option"
    );
    parser.addOption(unwarpOption);

    QCommandLineOption subjectOption(
        "subject",
        "Subject name (overrides SUBJECT env var).",
        "name"
    );
    parser.addOption(subjectOption);

    QCommandLineOption subjectsDirOption(
        "subjects-dir",
        "Subjects directory (overrides SUBJECTS_DIR env var).",
        "dir"
    );
    parser.addOption(subjectsDirOption);

    QCommandLineOption flashDirOption(
        "flash-dir",
        "Directory containing flash05/ and flash30/ subdirectories "
        "(default: current working directory).",
        "dir"
    );
    parser.addOption(flashDirOption);

    // Build argument list from argc/argv
    QStringList args;
    for (int i = 0; i < *argc; ++i) {
        args << QString(argv[i]);
    }

    parser.process(args);

    if (parser.isSet(noFlash30Option)) {
        m_bNoFlash30 = true;
    }
    if (parser.isSet(noConvertOption)) {
        m_bNoConvert = true;
    }
    if (parser.isSet(unwarpOption)) {
        m_sUnwarp = parser.value(unwarpOption);
    }
    if (parser.isSet(subjectOption)) {
        m_sSubject = parser.value(subjectOption);
    }
    if (parser.isSet(subjectsDirOption)) {
        m_sSubjectsDir = parser.value(subjectsDirOption);
    }
    if (parser.isSet(flashDirOption)) {
        m_sFlashDir = parser.value(flashDirOption);
    }
}

//=============================================================================================================

QString MNEFlashBemSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MNEFlashBemSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

QString MNEFlashBemSettings::freeSurferHome() const
{
    return m_sFreeSurferHome;
}

//=============================================================================================================

bool MNEFlashBemSettings::noConvert() const
{
    return m_bNoConvert;
}

//=============================================================================================================

bool MNEFlashBemSettings::noFlash30() const
{
    return m_bNoFlash30;
}

//=============================================================================================================

QString MNEFlashBemSettings::unwarp() const
{
    return m_sUnwarp;
}

//=============================================================================================================

QString MNEFlashBemSettings::flashDir() const
{
    return m_sFlashDir;
}
