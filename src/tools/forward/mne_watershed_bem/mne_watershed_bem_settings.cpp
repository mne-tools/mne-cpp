//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_watershed_bem_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2006
 * @brief    MNEWatershedBemSettings class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "mne_watershed_bem_settings.h"
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <optional>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEWATERSHEDBEM;
using mne::CommandLineParser;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEWatershedBemSettings::MNEWatershedBemSettings(int* argc, char** argv)
: m_sVolume("T1")
, m_bOverwrite(false)
, m_bAtlas(false)
, m_bGcaAtlas(false)
, m_iPreflood(-1)
, m_bVerbose(false)
{
    CommandLineParser parser(
        "Create BEM surfaces using the watershed algorithm included with FreeSurfer.\n\n"
        "This tool invokes FreeSurfer's mri_watershed to segment brain, skull, and\n"
        "skin surfaces from an MRI volume, then creates a FIFF BEM head surface file.\n\n"
        "Ported from the original MNE shell script mne_watershed_bem by Matti Hamalainen\n"
        "(SVN $Id: mne_watershed_bem 3391 2012-11-30 21:13:09Z msh $).\n\n"
        "Cross-referenced with MNE-Python's mne.bem.make_watershed_bem().");
    parser.add_help_option();
    parser.add_version_option();

    // --subject
    parser.add_option({{"subject"}, "Subject name (defaults to $SUBJECT environment variable).", "subject", ""});

    // --subjects-dir
    parser.add_option({{"subjects-dir"}, "Subjects directory (defaults to $SUBJECTS_DIR environment variable).", "dir", ""});

    // --volume
    parser.add_option({{"volume"}, "MRI volume name (default: T1).", "name", "T1"});

    // --overwrite
    parser.add_option({{"overwrite"}, "Overwrite existing watershed files.", "", ""});

    // --atlas
    parser.add_option({{"atlas"}, "Specify the --atlas option for mri_watershed.", "", ""});

    // --gcaatlas
    parser.add_option({{"gcaatlas"}, "Use the subcortical atlas for mri_watershed.", "", ""});

    // --preflood
    parser.add_option({{"preflood"}, "Change the preflood height for mri_watershed.", "number", ""});

    // --verbose
    parser.add_option({{"verbose"}, "Enable verbose output.", "", ""});

    parser.process(*argc, argv, "mne_watershed_bem", MNE_CPP_VERSION);

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

    // FreeSurfer home
    m_sFreeSurferHome = envValue("FREESURFER_HOME");

    // Volume
    m_sVolume = parser.value("volume");

    // Overwrite
    m_bOverwrite = parser.is_set("overwrite");

    // Atlas
    m_bAtlas = parser.is_set("atlas");

    // GCA Atlas
    m_bGcaAtlas = parser.is_set("gcaatlas");

    // Preflood
    if (parser.is_set("preflood")) {
        const std::optional<int> preflood = mne::to_int(parser.value("preflood"));
        if (!preflood) {
            mne::Log::warning("Invalid preflood value: {}", parser.value("preflood"));
            m_iPreflood = -1;
        } else {
            m_iPreflood = *preflood;
        }
    }

    // Verbose
    m_bVerbose = parser.is_set("verbose");
}

//=============================================================================================================

std::string MNEWatershedBemSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

std::string MNEWatershedBemSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

std::string MNEWatershedBemSettings::freeSurferHome() const
{
    return m_sFreeSurferHome;
}

//=============================================================================================================

std::string MNEWatershedBemSettings::volume() const
{
    return m_sVolume;
}

//=============================================================================================================

bool MNEWatershedBemSettings::overwrite() const
{
    return m_bOverwrite;
}

//=============================================================================================================

bool MNEWatershedBemSettings::atlas() const
{
    return m_bAtlas;
}

//=============================================================================================================

bool MNEWatershedBemSettings::gcaAtlas() const
{
    return m_bGcaAtlas;
}

//=============================================================================================================

int MNEWatershedBemSettings::preflood() const
{
    return m_iPreflood;
}

//=============================================================================================================

bool MNEWatershedBemSettings::verbose() const
{
    return m_bVerbose;
}
