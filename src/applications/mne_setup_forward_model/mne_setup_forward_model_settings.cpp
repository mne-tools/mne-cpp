//=============================================================================================================
/**
 * @file     mne_setup_forward_model_settings.cpp
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
 * @brief    MneSetupForwardModelSettings class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_setup_forward_model_settings.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcessEnvironment>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPFORWARDMODEL;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneSetupForwardModelSettings::MneSetupForwardModelSettings(int *argc, char **argv)
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
    m_sSubjectsDir = QProcessEnvironment::systemEnvironment().value("SUBJECTS_DIR", "");
    m_sSubject = QProcessEnvironment::systemEnvironment().value("SUBJECT", "");

    //
    // Parse command-line arguments
    //
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Set up the BEM (Boundary Element Model) for forward modeling.\n\n"
        "This tool reads triangulated surface files (inner skull, outer skull, scalp),\n"
        "creates a BEM geometry FIFF file with conductivity assignments, exports .pnt\n"
        "and .surf files for visualization, and optionally computes the BEM solution.\n\n"
        "Ported from the original MNE shell script mne_setup_forward_model by\n"
        "Matti Hamalainen (SVN $Id: mne_setup_forward_model 3282 2011-02-02 14:28:16Z gramfort $).\n\n"
        "Cross-referenced with MNE-Python's mne.make_bem_model() and mne.make_bem_solution()."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption subjectOpt("subject",
        "Subject name (defaults to $SUBJECT environment variable).", "name");
    parser.addOption(subjectOpt);

    QCommandLineOption subjectsDirOpt("subjects-dir",
        "Subjects directory (defaults to $SUBJECTS_DIR environment variable).", "dir");
    parser.addOption(subjectsDirOpt);

    QCommandLineOption scalpcOpt("scalpc",
        "Scalp conductivity [S/m] (default: 0.3).", "value");
    parser.addOption(scalpcOpt);

    QCommandLineOption skullcOpt("skullc",
        "Skull conductivity [S/m] (default: 0.006).", "value");
    parser.addOption(skullcOpt);

    QCommandLineOption braincOpt("brainc",
        "Brain conductivity [S/m] (default: 0.3).", "value");
    parser.addOption(braincOpt);

    QCommandLineOption modelOpt("model",
        "BEM model name (without -bem.fif suffix). If omitted, an auto-generated\n"
        "name based on triangle counts is used.", "name");
    parser.addOption(modelOpt);

    QCommandLineOption homogOpt("homog",
        "Create a single-compartment (homogeneous) model using only the inner skull surface.");
    parser.addOption(homogOpt);

    QCommandLineOption surfOpt("surf",
        "Use FreeSurfer binary surface files (.surf) as input instead of ASCII\n"
        "triangle files (.tri).");
    parser.addOption(surfOpt);

    QCommandLineOption icoOpt("ico",
        "Downsample surfaces to icosahedron subdivision level (0-6). Requires that\n"
        "surfaces are isomorphic with a higher-level subdivision. Mainly useful for\n"
        "downsampling surfaces produced by mri_watershed.", "number");
    parser.addOption(icoOpt);

    QCommandLineOption nosolOpt("nosol",
        "Omit the BEM solution preparation step.");
    parser.addOption(nosolOpt);

    QCommandLineOption noswapOpt("noswap",
        "Do not swap triangle vertex winding order (assume counterclockwise ordering).\n"
        "Only effective for ASCII .tri files.");
    parser.addOption(noswapOpt);

    QCommandLineOption metersOpt("meters",
        "Triangulation coordinates are in meters. Only effective for ASCII .tri files.");
    parser.addOption(metersOpt);

    QCommandLineOption innerShiftOpt("innershift",
        "Shift inner skull surface outward by this amount [mm].", "mm");
    parser.addOption(innerShiftOpt);

    QCommandLineOption outerShiftOpt("outershift",
        "Shift outer skull surface outward by this amount [mm].", "mm");
    parser.addOption(outerShiftOpt);

    QCommandLineOption scalpShiftOpt("scalpshift",
        "Shift scalp surface outward by this amount [mm].", "mm");
    parser.addOption(scalpShiftOpt);

    QCommandLineOption overwriteOpt("overwrite",
        "Overwrite existing output files.");
    parser.addOption(overwriteOpt);

    // Build argument list from argc/argv
    QStringList args;
    for (int i = 0; i < *argc; ++i) {
        args << QString(argv[i]);
    }

    parser.process(args);

    // Apply parsed values
    if (parser.isSet(subjectOpt)) {
        m_sSubject = parser.value(subjectOpt);
    }
    if (parser.isSet(subjectsDirOpt)) {
        m_sSubjectsDir = parser.value(subjectsDirOpt);
    }
    if (parser.isSet(scalpcOpt)) {
        m_fScalpConductivity = parser.value(scalpcOpt).toFloat();
    }
    if (parser.isSet(skullcOpt)) {
        m_fSkullConductivity = parser.value(skullcOpt).toFloat();
    }
    if (parser.isSet(braincOpt)) {
        m_fBrainConductivity = parser.value(braincOpt).toFloat();
    }
    if (parser.isSet(modelOpt)) {
        m_sModelName = parser.value(modelOpt);
    }
    if (parser.isSet(homogOpt)) {
        m_bHomogeneous = true;
    }
    if (parser.isSet(surfOpt)) {
        m_bUseSurfFormat = true;
    }
    if (parser.isSet(icoOpt)) {
        m_iIcoLevel = parser.value(icoOpt).toInt();
    }
    if (parser.isSet(nosolOpt)) {
        m_bNoSolution = true;
    }
    if (parser.isSet(noswapOpt)) {
        m_bSwap = false;
    }
    if (parser.isSet(metersOpt)) {
        m_bMeters = true;
    }
    if (parser.isSet(innerShiftOpt)) {
        m_fInnerShift = parser.value(innerShiftOpt).toFloat();
    }
    if (parser.isSet(outerShiftOpt)) {
        m_fOuterShift = parser.value(outerShiftOpt).toFloat();
    }
    if (parser.isSet(scalpShiftOpt)) {
        m_fScalpShift = parser.value(scalpShiftOpt).toFloat();
    }
    if (parser.isSet(overwriteOpt)) {
        m_bOverwrite = true;
    }
}

//=============================================================================================================

QString MneSetupForwardModelSettings::subject() const
{
    return m_sSubject;
}

//=============================================================================================================

QString MneSetupForwardModelSettings::subjectsDir() const
{
    return m_sSubjectsDir;
}

//=============================================================================================================

float MneSetupForwardModelSettings::scalpConductivity() const
{
    return m_fScalpConductivity;
}

//=============================================================================================================

float MneSetupForwardModelSettings::skullConductivity() const
{
    return m_fSkullConductivity;
}

//=============================================================================================================

float MneSetupForwardModelSettings::brainConductivity() const
{
    return m_fBrainConductivity;
}

//=============================================================================================================

QString MneSetupForwardModelSettings::modelName() const
{
    return m_sModelName;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::homogeneous() const
{
    return m_bHomogeneous;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::useSurfFormat() const
{
    return m_bUseSurfFormat;
}

//=============================================================================================================

int MneSetupForwardModelSettings::icoLevel() const
{
    return m_iIcoLevel;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::noSolution() const
{
    return m_bNoSolution;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::swap() const
{
    return m_bSwap;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::meters() const
{
    return m_bMeters;
}

//=============================================================================================================

float MneSetupForwardModelSettings::innerShift() const
{
    return m_fInnerShift;
}

//=============================================================================================================

float MneSetupForwardModelSettings::outerShift() const
{
    return m_fOuterShift;
}

//=============================================================================================================

float MneSetupForwardModelSettings::scalpShift() const
{
    return m_fScalpShift;
}

//=============================================================================================================

bool MneSetupForwardModelSettings::overwrite() const
{
    return m_bOverwrite;
}
