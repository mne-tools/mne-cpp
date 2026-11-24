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
#include <mne/base/command_line.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPMRI;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESetupMriSettings::MNESetupMriSettings(int* argc, char** argv)
: m_bOverwrite(false)
, m_bVerbose(false)
{
    CommandLineParser parser(
        "Set up FreeSurfer MRI data for use with MNE software.\n\n"
        "This tool creates the Neuromag-compatible directory structure and\n"
        "converts FreeSurfer MRI volumes (COR files or .mgz) into COR.fif\n"
        "FIFF files, following the original MNE C tool by Matti Hamalainen.\n\n"
        "Ported from SVN MNE mne_setup_mri / mne_make_cor_set.");
    parser.add_help_option();
    parser.add_version_option();

    // --subject
    parser.add_option({{"subject"}, "Subject name (defaults to $SUBJECT environment variable).", "subject", ""});

    // --subjects-dir
    parser.add_option({{"subjects-dir"}, "Subjects directory (defaults to $SUBJECTS_DIR environment variable).", "dir", ""});

    // --mri
    parser.add_option({{"mri"}, "MRI set name to process (can be specified multiple times, default: T1 brain).", "name", ""});

    // --overwrite
    parser.add_option({{"overwrite"}, "Overwrite existing data.", "", ""});

    // --verbose
    parser.add_option({{"verbose"}, "Enable verbose output.", "", ""});

    parser.process(*argc, argv, "mne_setup_mri", MNE_CPP_VERSION);

    const auto envValue = [](const char* name) {
        const char* value = std::getenv(name);
        return std::string(value ? value : "");
    };

    // Subject
    if (parser.is_set("subject")) {
        m_sSubject = parser.value("subject");
    } else {
        m_sSubject = envValue("SUBJECT");
    }

    // Subjects dir
    if (parser.is_set("subjects-dir")) {
        m_sSubjectsDir = parser.value("subjects-dir");
    } else {
        m_sSubjectsDir = envValue("SUBJECTS_DIR");
    }

    // MRI sets
    if (parser.is_set("mri")) {
        m_slMriSets = parser.values("mri");
    } else {
        m_slMriSets = {"T1", "brain"};
    }

    // Overwrite
    m_bOverwrite = parser.is_set("overwrite");

    // Verbose
    m_bVerbose = parser.is_set("verbose");
}

//=============================================================================================================

std::string MNESetupMriSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

std::string MNESetupMriSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

StringList MNESetupMriSettings::mriSets() const
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
