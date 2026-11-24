//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_flash_bem_settings.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     May 2006
 * @brief    MNEFlashBemSettings class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_flash_bem_settings.h"
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEFLASHBEM;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEFlashBemSettings::MNEFlashBemSettings(int* argc, char** argv)
: m_bNoConvert(false)
, m_bNoFlash30(false)
{
    //
    // Read environment variables
    //
    const auto envValue = [](const char* name) {
        const char* value = std::getenv(name);
        return std::string(value ? value : "");
    };
    m_sFreeSurferHome = envValue("FREESURFER_HOME");
    m_sSubjectsDir = envValue("SUBJECTS_DIR");
    m_sSubject = envValue("SUBJECT");

    // Default flash data directory is cwd
    std::error_code ec;
    m_sFlashDir = utf8_from_path(std::filesystem::current_path(ec));

    //
    // Parse command-line arguments
    //
    CommandLineParser parser(
        "Create BEM meshes using multi-echo FLASH MRI data.\n\n"
        "Ported from the original MNE shell script mne_flash_bem by Matti Hamalainen\n"
        "(SVN $Id: mne_flash_bem 3255 2010-11-15 18:34:59Z msh $).\n\n"
        "Cross-referenced with MNE-Python's mne.bem.make_flash_bem().\n\n"
        "Before running:\n"
        "  1. Set FREESURFER_HOME, SUBJECTS_DIR, and SUBJECT environment variables\n"
        "  2. Create flash05/ (and optionally flash30/) directories with echo subdirectories\n"
        "  3. Run from the directory containing the flash data, or use --flash-dir");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"noflash30"},
                       "Only 5-degree flash angle data is available. Flash-5 echoes will be averaged "
                       "instead of using parameter maps from both 5 and 30 degree data.",
                       "",
                       ""});
    parser.add_option({{"noconvert"},
                       "Assume that the DICOM images have already been converted to MGZ format. "
                       "Skip the mri_convert step.",
                       "",
                       ""});
    parser.add_option({{"unwarp"}, "Apply gradient distortion unwarping using grad_unwarp with this option.", "option", ""});
    parser.add_option({{"subject"}, "Subject name (overrides SUBJECT env var).", "name", ""});
    parser.add_option({{"subjects-dir"}, "Subjects directory (overrides SUBJECTS_DIR env var).", "dir", ""});
    parser.add_option({{"flash-dir"},
                       "Directory containing flash05/ and flash30/ subdirectories "
                       "(default: current working directory).",
                       "dir",
                       ""});

    parser.process(*argc, argv, "mne_flash_bem", MNE_CPP_VERSION);

    if (parser.is_set("noflash30")) {
        m_bNoFlash30 = true;
    }
    if (parser.is_set("noconvert")) {
        m_bNoConvert = true;
    }
    if (parser.is_set("unwarp")) {
        m_sUnwarp = parser.value("unwarp");
    }
    if (parser.is_set("subject")) {
        m_sSubject = parser.value("subject");
    }
    if (parser.is_set("subjects-dir")) {
        m_sSubjectsDir = parser.value("subjects-dir");
    }
    if (parser.is_set("flash-dir")) {
        m_sFlashDir = parser.value("flash-dir");
    }
}

//=============================================================================================================

std::string MNEFlashBemSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

std::string MNEFlashBemSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

std::string MNEFlashBemSettings::freeSurferHome() const
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

std::string MNEFlashBemSettings::unwarp() const
{
    return m_sUnwarp;
}

//=============================================================================================================

std::string MNEFlashBemSettings::flashDir() const
{
    return m_sFlashDir;
}
