//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_setup_forward_model_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    MNESetupForwardModelSettings class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_setup_forward_model_settings.h"
#include <mne/base/command_line.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPFORWARDMODEL;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNESetupForwardModelSettings::MNESetupForwardModelSettings(int* argc, char** argv)
: m_fScalpConductivity(0.3f)
, m_fSkullConductivity(0.006f)
, m_fBrainConductivity(0.3f)
, m_bHomogeneous(false)
, m_bUseSurfFormat(false)
, m_iIcoLevel(-1)
, m_bNoSolution(false)
, m_bSwap(true)
, m_bMeters(false)
, m_fInnerShift(0.0f)
, m_fOuterShift(0.0f)
, m_fScalpShift(0.0f)
, m_bOverwrite(false)
{
    //
    // Read environment variables
    //
    const auto envValue = [](const char* name) {
        const char* value = std::getenv(name);
        return std::string(value ? value : "");
    };
    m_sSubjectsDir = envValue("SUBJECTS_DIR");
    m_sSubject = envValue("SUBJECT");

    //
    // Parse command-line arguments
    //
    const auto toFloat = [](const std::string& text) {
        return static_cast<float>(to_double(text).value_or(0.0));
    };

    CommandLineParser parser(
        "Set up the BEM (Boundary Element Model) for forward modeling.\n\n"
        "This tool reads triangulated surface files (inner skull, outer skull, scalp),\n"
        "creates a BEM geometry FIFF file with conductivity assignments, exports .pnt\n"
        "and .surf files for visualization, and optionally computes the BEM solution.\n\n"
        "Ported from the original MNE shell script mne_setup_forward_model by\n"
        "Matti Hamalainen (SVN $Id: mne_setup_forward_model 3282 2011-02-02 14:28:16Z gramfort $).\n\n"
        "Cross-referenced with MNE-Python's mne.make_bem_model() and mne.make_bem_solution().");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"subject"}, "Subject name (defaults to $SUBJECT environment variable).", "name", ""});

    parser.add_option({{"subjects-dir"}, "Subjects directory (defaults to $SUBJECTS_DIR environment variable).", "dir", ""});

    parser.add_option({{"scalpc"}, "Scalp conductivity [S/m] (default: 0.3).", "value", ""});

    parser.add_option({{"skullc"}, "Skull conductivity [S/m] (default: 0.006).", "value", ""});

    parser.add_option({{"brainc"}, "Brain conductivity [S/m] (default: 0.3).", "value", ""});

    parser.add_option({{"model"},
                       "BEM model name (without -bem.fif suffix). If omitted, an auto-generated\n"
                       "name based on triangle counts is used.",
                       "name",
                       ""});

    parser.add_option({{"homog"}, "Create a single-compartment (homogeneous) model using only the inner skull surface.", "", ""});

    parser.add_option({{"surf"},
                       "Use FreeSurfer binary surface files (.surf) as input instead of ASCII\n"
                       "triangle files (.tri).",
                       "",
                       ""});

    parser.add_option({{"ico"},
                       "Downsample surfaces to icosahedron subdivision level (0-6). Requires that\n"
                       "surfaces are isomorphic with a higher-level subdivision. Mainly useful for\n"
                       "downsampling surfaces produced by mri_watershed.",
                       "number",
                       ""});

    parser.add_option({{"nosol"}, "Omit the BEM solution preparation step.", "", ""});

    parser.add_option({{"noswap"},
                       "Do not swap triangle vertex winding order (assume counterclockwise ordering).\n"
                       "Only effective for ASCII .tri files.",
                       "",
                       ""});

    parser.add_option({{"meters"}, "Triangulation coordinates are in meters. Only effective for ASCII .tri files.", "", ""});

    parser.add_option({{"innershift"}, "Shift inner skull surface outward by this amount [mm].", "mm", ""});

    parser.add_option({{"outershift"}, "Shift outer skull surface outward by this amount [mm].", "mm", ""});

    parser.add_option({{"scalpshift"}, "Shift scalp surface outward by this amount [mm].", "mm", ""});

    parser.add_option({{"overwrite"}, "Overwrite existing output files.", "", ""});

    parser.process(*argc, argv, "mne_setup_forward_model", MNE_CPP_VERSION);

    // Apply parsed values
    if (parser.is_set("subject")) {
        m_sSubject = parser.value("subject");
    }
    if (parser.is_set("subjects-dir")) {
        m_sSubjectsDir = parser.value("subjects-dir");
    }
    if (parser.is_set("scalpc")) {
        m_fScalpConductivity = toFloat(parser.value("scalpc"));
    }
    if (parser.is_set("skullc")) {
        m_fSkullConductivity = toFloat(parser.value("skullc"));
    }
    if (parser.is_set("brainc")) {
        m_fBrainConductivity = toFloat(parser.value("brainc"));
    }
    if (parser.is_set("model")) {
        m_sModelName = parser.value("model");
    }
    if (parser.is_set("homog")) {
        m_bHomogeneous = true;
    }
    if (parser.is_set("surf")) {
        m_bUseSurfFormat = true;
    }
    if (parser.is_set("ico")) {
        m_iIcoLevel = to_int(parser.value("ico")).value_or(0);
    }
    if (parser.is_set("nosol")) {
        m_bNoSolution = true;
    }
    if (parser.is_set("noswap")) {
        m_bSwap = false;
    }
    if (parser.is_set("meters")) {
        m_bMeters = true;
    }
    if (parser.is_set("innershift")) {
        m_fInnerShift = toFloat(parser.value("innershift"));
    }
    if (parser.is_set("outershift")) {
        m_fOuterShift = toFloat(parser.value("outershift"));
    }
    if (parser.is_set("scalpshift")) {
        m_fScalpShift = toFloat(parser.value("scalpshift"));
    }
    if (parser.is_set("overwrite")) {
        m_bOverwrite = true;
    }
}

//=============================================================================================================

std::string MNESetupForwardModelSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

std::string MNESetupForwardModelSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

float MNESetupForwardModelSettings::scalpConductivity() const
{
    return m_fScalpConductivity;
}

//=============================================================================================================

float MNESetupForwardModelSettings::skullConductivity() const
{
    return m_fSkullConductivity;
}

//=============================================================================================================

float MNESetupForwardModelSettings::brainConductivity() const
{
    return m_fBrainConductivity;
}

//=============================================================================================================

std::string MNESetupForwardModelSettings::modelName() const
{
    return m_sModelName;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::homogeneous() const
{
    return m_bHomogeneous;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::useSurfFormat() const
{
    return m_bUseSurfFormat;
}

//=============================================================================================================

int MNESetupForwardModelSettings::icoLevel() const
{
    return m_iIcoLevel;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::noSolution() const
{
    return m_bNoSolution;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::swap() const
{
    return m_bSwap;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::meters() const
{
    return m_bMeters;
}

//=============================================================================================================

float MNESetupForwardModelSettings::innerShift() const
{
    return m_fInnerShift;
}

//=============================================================================================================

float MNESetupForwardModelSettings::outerShift() const
{
    return m_fOuterShift;
}

//=============================================================================================================

float MNESetupForwardModelSettings::scalpShift() const
{
    return m_fScalpShift;
}

//=============================================================================================================

bool MNESetupForwardModelSettings::overwrite() const
{
    return m_bOverwrite;
}
