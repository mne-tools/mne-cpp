//=============================================================================================================
/**
 * @file     mne_flash_bem_settings.cpp
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
 * @brief    MneFlashBemSettings class definition.
 *
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

MneFlashBemSettings::MneFlashBemSettings(int *argc, char **argv)
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

QString MneFlashBemSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MneFlashBemSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

QString MneFlashBemSettings::freeSurferHome() const
{
    return m_sFreeSurferHome;
}

//=============================================================================================================

bool MneFlashBemSettings::noConvert() const
{
    return m_bNoConvert;
}

//=============================================================================================================

bool MneFlashBemSettings::noFlash30() const
{
    return m_bNoFlash30;
}

//=============================================================================================================

QString MneFlashBemSettings::unwarp() const
{
    return m_sUnwarp;
}

//=============================================================================================================

QString MneFlashBemSettings::flashDir() const
{
    return m_sFlashDir;
}
